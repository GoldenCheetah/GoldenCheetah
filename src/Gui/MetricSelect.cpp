/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "MetricSelect.h"
#include "RideFileCache.h"
#include "SpecialFields.h"
#include "ActionButtonBox.h"
#include "Colors.h"


MetricSelect::MetricSelect(QWidget *parent, Context *context, int scope)
    : QLineEdit(parent), context(context), scope(scope), _metric(NULL)
{

    // add metrics if we're selecting those
    if (scope & Metric) {
        const RideMetricFactory &factory = RideMetricFactory::instance();
        foreach(QString name, factory.allMetrics()) {
            const RideMetric *m = factory.rideMetric(name);
            QString text = m->internalName();
            text = text.replace(' ','_');
            if (text.startsWith("BikeScore")) text="BikeScore"; // stoopid tm sign in name ffs sean
            listText << text;
            metricMap.insert(text, m);
        }
    }

    // add meta if we're selecting this
    if (scope & Meta) {

        SpecialFields& sp = SpecialFields::getInstance();

        // now add the ride metadata fields -- should be the same generally
        foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
            QString text = field.name;
            if (!sp.isMetric(text)) {

                // translate to internal name if name has non Latin1 characters
                text = sp.internalName(text);
                text = text.replace(" ","_");
                metaMap.insert(text, field.name);
            }
            listText << text;
        }
    }

    // set the autocompleter, if there is something to autocomplete
    if (listText.count()) {
        completer = new QCompleter(listText, this);
        setCompleter(completer);
    }
}

// set text via metric symbol
void
MetricSelect::setSymbol(QString symbol)
{
    if ((scope & MetricSelect::Metric) == 0) return;

    // get the ridemetric
    RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *m = factory.rideMetric(symbol);

    // lets set as per normal...
    if (m) {

        QString text = m->internalName();
        text = text.replace(' ','_');
        if (text.startsWith("BikeScore")) text="BikeScore"; // stoopid tm sign in name ffs sean
        setText(text);
        setWhatsThis(m->description());
    }
    // check it...
    isValid();
}
// is the entry we have in there even valid?
bool
MetricSelect::isValid()
{
    _metric = NULL;
    if (scope & MetricSelect::Metric) {
        _metric = metricMap.value(text(), NULL);
        if (_metric) return true;
    }

    if (scope & MetricSelect::Meta) {
        if (metaMap.value(text(),"") != "") return true;
    }

    return false;
}

bool
MetricSelect::isMetric()
{
    isValid();
    return (_metric != NULL);
}

bool
MetricSelect::isMeta()
{
    if (!isMetric() && metaMap.contains(text())) return true;
    return false;
}

const RideMetric *
MetricSelect::rideMetric()
{
    isValid(); // check
    return _metric;
}

QString MetricSelect::metaname()
{
    isValid();
    if (_metric) return "";
    else return metaMap.value(text(), "");
}

void
MetricSelect::setMeta(QString name)
{
    if (_metric) return;
    else {
        // find it
        QMapIterator<QString,QString>it(metaMap);
        while(it.hasNext()) {
            it.next();
            if (it.value() == name) {
                setText(it.key());
                break;
            }
        }
    }
}

SeriesSelect::SeriesSelect(QWidget *parent, int scope) : QComboBox(parent), scope(scope)
{
    // set the scope
    QStringList symbols;
    if (scope == SeriesSelect::All)  {
        foreach(QString symbol, RideFile::symbols()) {
            if (symbol != "") symbols << symbol; // filter out dummy values
        }
    } else if (scope == SeriesSelect::MMP) {
        foreach(RideFile::SeriesType series, RideFileCache::meanMaxList()) {
            symbols << RideFile::symbolForSeries(series);
        }
    } else if (scope == SeriesSelect::Zones) {
        symbols << "POWER" << "HEARTRATE" << "SPEED" << "WBAL";
    }

    foreach(QString symbol, symbols) {
        addItem(symbol, static_cast<int>(RideFile::seriesForSymbol(symbol)));
    }
}

void
SeriesSelect::setSeries(RideFile::SeriesType x)
{
    // loop through values to determine which to select
    for(int i=0; i<count(); i++) {
        if (itemData(i, Qt::UserRole).toInt() == static_cast<int>(x)) {
            setCurrentIndex(i);
            return;
        }
    }
}

RideFile::SeriesType
SeriesSelect::series()
{
    if (currentIndex() < 0)  return RideFile::none;
    return static_cast<RideFile::SeriesType>(itemData(currentIndex(), Qt::UserRole).toInt());
}


//////////////////////////////////////////////////////////////////////////////
// MultiMetricSelector

MultiMetricSelector::MultiMetricSelector
(const QString &leftLabel, const QString &rightLabel, const QStringList &selectedMetrics, QWidget *parent)
: QWidget(parent)
{
    filterEdit = new QLineEdit();
    filterEdit->setPlaceholderText(tr("Filter..."));

    availList = new QListWidget();
    availList->setSortingEnabled(true);
    availList->setAlternatingRowColors(true);
    availList->setSelectionMode(QAbstractItemView::SingleSelection);

    QVBoxLayout *availLayout = new QVBoxLayout();
    availLayout->addWidget(new QLabel(leftLabel));
    availLayout->addWidget(filterEdit);
    availLayout->addWidget(availList);

    selectedList = new QListWidget();
    selectedList->setAlternatingRowColors(true);
    selectedList->setSelectionMode(QAbstractItemView::SingleSelection);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::UpDownGroup);
    actionButtons->defaultConnect(ActionButtonBox::UpDownGroup, selectedList);

    QVBoxLayout *selectedLayout = new QVBoxLayout();
    selectedLayout->addWidget(new QLabel(rightLabel));
    selectedLayout->addWidget(selectedList);
    selectedLayout->addWidget(actionButtons);

#ifndef Q_OS_MAC
    unselectButton = new QToolButton(this);
    unselectButton->setArrowType(Qt::LeftArrow);
    unselectButton->setFixedSize(20 * dpiXFactor, 20 * dpiYFactor);
    selectButton = new QToolButton(this);
    selectButton->setArrowType(Qt::RightArrow);
    selectButton->setFixedSize(20 * dpiXFactor, 20 * dpiYFactor);
#else
    unselectButton = new QPushButton("<");
    selectButton = new QPushButton(">");
#endif
    unselectButton->setEnabled(false);
    selectButton->setEnabled(false);

    QHBoxLayout *inexcLayout = new QHBoxLayout();
    inexcLayout->addStretch();
    inexcLayout->addWidget(unselectButton);
    inexcLayout->addWidget(selectButton);
    inexcLayout->addStretch();

    QVBoxLayout *buttonGrid = new QVBoxLayout();
    buttonGrid->addStretch();
    buttonGrid->addLayout(inexcLayout);
    buttonGrid->addStretch();

    QHBoxLayout *hlayout = new QHBoxLayout(this);;
    hlayout->addLayout(availLayout, 2);
    hlayout->addLayout(buttonGrid, 1);
    hlayout->addLayout(selectedLayout, 2);

    setSymbols(selectedMetrics);

    connect(actionButtons, &ActionButtonBox::upRequested, this, &MultiMetricSelector::upClicked);
    connect(actionButtons, &ActionButtonBox::downRequested, this, &MultiMetricSelector::downClicked);
    connect(unselectButton, &QAbstractButton::clicked, this, &MultiMetricSelector::unselectClicked);
    connect(selectButton,  &QAbstractButton::clicked, this, &MultiMetricSelector::selectClicked);
    connect(availList, &QListWidget::itemSelectionChanged, this, &MultiMetricSelector::updateSelectionButtons);
    connect(selectedList, &QListWidget::itemSelectionChanged, this, &MultiMetricSelector::updateSelectionButtons);
    connect(filterEdit, &QLineEdit::textChanged, this, &MultiMetricSelector::filterAvail);
}


void
MultiMetricSelector::setSymbols
(const QStringList &selectedMetrics)
{
    availList->blockSignals(true);
    selectedList->blockSignals(true);

    availList->clear();
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i = 0; i < factory.metricCount(); ++i) {
        QString symbol = factory.metricName(i);
        if (selectedMetrics.contains(symbol) || symbol.startsWith("compatibility_")) {
            continue;
        }
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QListWidgetItem *item = new QListWidgetItem(Utils::unprotect(m->name()));
        item->setData(Qt::UserRole, symbol);
        item->setToolTip(m->description());
        availList->addItem(item);
    }
    selectedList->clear();
    for (const QString &symbol : selectedMetrics) {
        if (! factory.haveMetric(symbol)) {
            continue;
        }
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QListWidgetItem *item = new QListWidgetItem(Utils::unprotect(m->name()));
        item->setData(Qt::UserRole, symbol);
        item->setToolTip(m->description());
        selectedList->addItem(item);
    }

    availList->blockSignals(false);
    selectedList->blockSignals(false);
    updateSelectionButtons();
    emit selectedChanged();
}


QStringList
MultiMetricSelector::getSymbols
() const
{
    QStringList metrics;
    for (int i = 0; i < selectedList->count(); ++i) {
        metrics << selectedList->item(i)->data(Qt::UserRole).toString();
    }
    return metrics;
}


void
MultiMetricSelector::updateMetrics
()
{
    setSymbols(getSymbols());
    if (! filterEdit->text().isEmpty()) {
        filterAvail(filterEdit->text());
    }
}


void
MultiMetricSelector::filterAvail
(const QString &text)
{
    for (int i = 0; i < availList->count(); ++i) {
        QListWidgetItem *item = availList->item(i);
        item->setHidden(! item->text().contains(text, Qt::CaseInsensitive));
    }
}


void
MultiMetricSelector::upClicked
()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    assert(row > 0);
    selectedList->takeItem(row);
    selectedList->insertItem(row - 1, item);
    selectedList->setCurrentItem(item);

    emit selectedChanged();
}


void
MultiMetricSelector::downClicked
()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    assert(row < selectedList->count() - 1);
    selectedList->takeItem(row);
    selectedList->insertItem(row + 1, item);
    selectedList->setCurrentItem(item);

    emit selectedChanged();
}


void
MultiMetricSelector::unselectClicked
()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    selectedList->takeItem(selectedList->row(item));
    availList->addItem(item);
    updateSelectionButtons();

    emit selectedChanged();
}


void
MultiMetricSelector::selectClicked
()
{
    assert(!availList->selectedItems().isEmpty());
    QListWidgetItem *item = availList->selectedItems().first();
    availList->takeItem(availList->row(item));
    selectedList->addItem(item);
    updateSelectionButtons();

    emit selectedChanged();
}


void
MultiMetricSelector::updateSelectionButtons
()
{
    unselectButton->setEnabled(! selectedList->selectedItems().isEmpty());
    selectButton->setEnabled(! availList->selectedItems().isEmpty());
}
