/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#include "SeasonDialogs.h"
#include "Seasons.h"
#include "Athlete.h"

#include <QString>
#include <QFormLayout>
#include <QDialogButtonBox>


/*----------------------------------------------------------------------
 * EDIT SEASON DIALOG
 *--------------------------------------------------------------------*/

EditSeasonDialog::EditSeasonDialog
(Context *context, Season *season)
: QDialog(context->mainWindow, Qt::Dialog), context(context), season(season)
{
    setWindowTitle(tr("Edit Date Range"));

    nameEdit = new QLineEdit();

    typeCombo = new QComboBox();
    typeCombo->addItem(tr("Season"), Season::season);
    typeCombo->addItem(tr("Cycle"), Season::cycle);
    typeCombo->addItem(tr("Adhoc"), Season::adhoc);

    startCombo = new QComboBox();
    startCombo->addItem(tr("Absolute Start"), absolute);
    startCombo->addItem(tr("Relative Start"), relative);
    startCombo->addItem(tr("Duration before End"), duration);

    startValueStack = new QStackedWidget();

    startAbsoluteEdit = new QDateEdit();
    startAbsoluteEdit->setCalendarPopup(true);
    startValueStack->addWidget(startAbsoluteEdit);

    QWidget *startRelative = new QWidget();
    QHBoxLayout *startRelativeLayout = new QHBoxLayout(startRelative);
    startRelativeLayout->setContentsMargins(0, 0, 0, 0);
    startRelativeEdit = new QSpinBox();
    startRelativeEdit->setRange(0, 52);
    startRelativeEdit->setSingleStep(1);
    startRelativeEdit->setWrapping(true);
    startRelativeEdit->setAlignment(Qt::AlignLeft);
    startRelativeCombo = new QComboBox();
    startRelativeCombo->addItem(tr("Weeks ago"), SeasonOffset::week);
    startRelativeCombo->addItem(tr("Months ago"), SeasonOffset::month);
    startRelativeCombo->addItem(tr("Years ago"), SeasonOffset::year);
    startRelativeLayout->addWidget(startRelativeEdit);
    startRelativeLayout->addWidget(startRelativeCombo);
    startValueStack->addWidget(startRelative);

    QWidget *startDuration = new QWidget();
    QHBoxLayout *startDurationLayout = new QHBoxLayout(startDuration);
    startDurationLayout->setContentsMargins(0, 0, 0, 0);
    startDurationYearsEdit = new QSpinBox();
    startDurationYearsEdit->setSuffix(" " + tr("Years"));
    startDurationYearsEdit->setRange(0, 50);
    startDurationYearsEdit->setSingleStep(1);
    startDurationYearsEdit->setWrapping(true);
    startDurationYearsEdit->setAlignment(Qt::AlignLeft);
    startDurationMonthsEdit = new QSpinBox();
    startDurationMonthsEdit->setSuffix(" " + tr("Months"));
    startDurationMonthsEdit->setRange(0, 12);
    startDurationMonthsEdit->setSingleStep(1);
    startDurationMonthsEdit->setWrapping(true);
    startDurationMonthsEdit->setAlignment(Qt::AlignLeft);
    startDurationDaysEdit = new QSpinBox();
    startDurationDaysEdit->setSuffix(" " + tr("Days"));
    startDurationDaysEdit->setRange(0, 31);
    startDurationDaysEdit->setSingleStep(1);
    startDurationDaysEdit->setWrapping(true);
    startDurationDaysEdit->setAlignment(Qt::AlignLeft);
    startDurationLayout->addWidget(startDurationYearsEdit);
    startDurationLayout->addWidget(startDurationMonthsEdit);
    startDurationLayout->addWidget(startDurationDaysEdit);
    startValueStack->addWidget(startDuration);

    endCombo = new QComboBox();
    endCombo->addItem(tr("Absolute End"), absolute);
    endCombo->addItem(tr("Relative End"), relative);
    endCombo->addItem(tr("Duration after Start"), duration);
    endCombo->addItem(tr("Year to Date"), ytd);

    endValueStack = new QStackedWidget();

    endAbsoluteEdit = new QDateEdit();
    endAbsoluteEdit->setCalendarPopup(true);
    endValueStack->addWidget(endAbsoluteEdit);

    QWidget *endRelative = new QWidget();
    QHBoxLayout *endRelativeLayout = new QHBoxLayout(endRelative);
    endRelativeLayout->setContentsMargins(0, 0, 0, 0);
    endRelativeEdit = new QSpinBox();
    endRelativeEdit->setRange(0, 52);
    endRelativeEdit->setSingleStep(1);
    endRelativeEdit->setWrapping(true);
    endRelativeEdit->setAlignment(Qt::AlignLeft);
    endRelativeCombo = new QComboBox();
    endRelativeCombo->addItem(tr("Weeks ago"), SeasonOffset::week);
    endRelativeCombo->addItem(tr("Months ago"), SeasonOffset::month);
    endRelativeCombo->addItem(tr("Years ago"), SeasonOffset::year);
    endRelativeLayout->addWidget(endRelativeEdit);
    endRelativeLayout->addWidget(endRelativeCombo);
    endValueStack->addWidget(endRelative);

    QWidget *endDuration = new QWidget();
    QHBoxLayout *endDurationLayout = new QHBoxLayout(endDuration);
    endDurationLayout->setContentsMargins(0, 0, 0, 0);
    endDurationYearsEdit = new QSpinBox();
    endDurationYearsEdit->setSuffix(" " + tr("Years"));
    endDurationYearsEdit->setRange(0, 50);
    endDurationYearsEdit->setSingleStep(1);
    endDurationYearsEdit->setWrapping(true);
    endDurationYearsEdit->setAlignment(Qt::AlignLeft);
    endDurationMonthsEdit = new QSpinBox();
    endDurationMonthsEdit->setSuffix(" " + tr("Months"));
    endDurationMonthsEdit->setRange(0, 12);
    endDurationMonthsEdit->setSingleStep(1);
    endDurationMonthsEdit->setWrapping(true);
    endDurationMonthsEdit->setAlignment(Qt::AlignLeft);
    endDurationDaysEdit = new QSpinBox();
    endDurationDaysEdit->setSuffix(" " + tr("Days"));
    endDurationDaysEdit->setRange(0, 31);
    endDurationDaysEdit->setSingleStep(1);
    endDurationDaysEdit->setWrapping(true);
    endDurationDaysEdit->setAlignment(Qt::AlignLeft);
    endDurationLayout->addWidget(endDurationYearsEdit);
    endDurationLayout->addWidget(endDurationMonthsEdit);
    endDurationLayout->addWidget(endDurationDaysEdit);

    endValueStack->addWidget(endDuration);
    endValueStack->addWidget(new QLabel(tr("Up to current day and month")));

    statusLabel = new QLabel();
    statusLabel->setWordWrap(true);
    warningLabel = new QLabel();
    warningLabel->setWordWrap(true);

    seedEdit = new QDoubleSpinBox();
    seedEdit->setDecimals(0);
    seedEdit->setRange(0.0, 300.0);
    seedEdit->setSingleStep(1.0);
    seedEdit->setWrapping(true);
    seedEdit->setAlignment(Qt::AlignLeft);

    lowEdit = new QDoubleSpinBox();
    lowEdit->setDecimals(0);
    lowEdit->setRange(-500, 0);
    lowEdit->setSingleStep(1.0);
    lowEdit->setWrapping(true);
    lowEdit->setAlignment(Qt::AlignLeft);

    QDialogButtonBox *buttonBox = new QDialogButtonBox();
    applyButton = buttonBox->addButton(tr("&OK"), QDialogButtonBox::AcceptRole);
    QPushButton *cancelButton = buttonBox->addButton(tr("&Cancel"), QDialogButtonBox::RejectRole);

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->addRow(tr("Name"), nameEdit);
    formLayout->addRow(tr("Type"), typeCombo);
    formLayout->addRow(startCombo, startValueStack);
    formLayout->addRow(endCombo, endValueStack);
    formLayout->addRow(tr("As of today"), statusLabel);
    formLayout->addRow(tr("Starting LTS"), seedEdit);
    formLayout->addRow(tr("Lowest SB"), lowEdit);
    formLayout->addRow(warningLabel);
    formLayout->addRow(buttonBox);

    // connect up slots
    connect(typeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateAllowedCombinations()));

    connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(nameEdit, SIGNAL(textChanged(const QString &)), this, SLOT(nameChanged()));

    connect(startCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(startTypeChanged(int)));
    connect(startCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTimeRange()));
    connect(startRelativeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTimeRange()));
    connect(startAbsoluteEdit, SIGNAL(dateChanged(QDate)), this, SLOT(updateTimeRange()));
    connect(startRelativeEdit, SIGNAL(valueChanged(int)), this, SLOT(updateTimeRange()));
    connect(startDurationYearsEdit, SIGNAL(valueChanged(int)), this, SLOT(updateTimeRange()));
    connect(startDurationMonthsEdit, SIGNAL(valueChanged(int)), this, SLOT(updateTimeRange()));
    connect(startDurationDaysEdit, SIGNAL(valueChanged(int)), this, SLOT(updateTimeRange()));
    connect(endCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(endTypeChanged(int)));
    connect(endCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTimeRange()));
    connect(endRelativeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTimeRange()));
    connect(endAbsoluteEdit, SIGNAL(dateChanged(QDate)), this, SLOT(updateTimeRange()));
    connect(endRelativeEdit, SIGNAL(valueChanged(int)), this, SLOT(updateTimeRange()));
    connect(endDurationYearsEdit, SIGNAL(valueChanged(int)), this, SLOT(updateTimeRange()));
    connect(endDurationMonthsEdit, SIGNAL(valueChanged(int)), this, SLOT(updateTimeRange()));
    connect(endDurationDaysEdit, SIGNAL(valueChanged(int)), this, SLOT(updateTimeRange()));

    // initialize button state
    nameChanged();
    transferSeasonToUI(*season);
    updateTimeRange();
    updateAllowedCombinations();
}

void
EditSeasonDialog::applyClicked()
{
    transferUIToSeason(*season);
    accept();
}

void
EditSeasonDialog::cancelClicked()
{
    reject();
}

void EditSeasonDialog::nameChanged()
{
    applyButton->setEnabled(!nameEdit->text().isEmpty());
}


void
EditSeasonDialog::updateAllowedCombinations
()
{
    // Allow relative and duration/length based ranges for type Season, but disable for Adhoc and Cycle
    if (typeCombo->currentData().toInt() != Season::season) {
        startCombo->setCurrentIndex(startCombo->findData(absolute));
        endCombo->setCurrentIndex(endCombo->findData(absolute));

        setEnabledItem(startCombo, relative, false);
        setEnabledItem(startCombo, duration, false);

        setEnabledItem(endCombo, relative, false);
        setEnabledItem(endCombo, duration, false);
        setEnabledItem(endCombo, ytd, false);
    } else {
        setEnabledItem(startCombo, relative, true);
        setEnabledItem(startCombo, duration, true);

        setEnabledItem(endCombo, relative, true);
        setEnabledItem(endCombo, duration, true);
        setEnabledItem(endCombo, ytd, true);
    }

    // Disable relative ranges if Phases or Events are associated with this season
    if (! season->hasPhaseOrEvent()) {
        setEnabledItem(startCombo, relative, false);
        setEnabledItem(endCombo, relative, false);
        setEnabledItem(endCombo, ytd, false);
    }

    // Disable invalid combinations (duration + duration, duration + ytd)
    int startComboData = startCombo->currentData().toInt();
    if (startComboData == duration) {
        setEnabledItem(endCombo, duration, false);
        setEnabledItem(endCombo, ytd, false);
    }
    int endComboData = endCombo->currentData().toInt();
    if (endComboData == duration || endComboData == ytd) {
        setEnabledItem(startCombo, duration, false);
    }
}


void
EditSeasonDialog::startTypeChanged
(int idx)
{
    startValueStack->setCurrentIndex(idx);
    updateAllowedCombinations();
}


void
EditSeasonDialog::endTypeChanged
(int idx)
{
    endValueStack->setCurrentIndex(idx);
    updateAllowedCombinations();
}


void
EditSeasonDialog::updateTimeRange
()
{
    Season season;
    transferUIToSeason(season);
    QDate start = season.getStart();
    QDate end = season.getEnd();
    statusLabel->setText(tr("%1 - %2").arg(start.toString(tr("dd MMM yyyy"))).arg(end.toString(tr("dd MMM yyyy"))));
    if (start > end) {
        warningLabel->setText(tr("<b>WARNING</b> Start is after end, season will be empty"));
    } else {
        warningLabel->clear();
    }
}


void
EditSeasonDialog::transferUIToSeason
(Season &season) const
{
    season.resetTimeRange();

    season.setName(nameEdit->text());
    season.setType(typeCombo->itemData(typeCombo->currentIndex()).toInt());
    season.setSeed(seedEdit->value());
    season.setLow(lowEdit->value());

    int years;
    int months;
    int weeks;
    switch (startCombo->currentData().toInt()) {
    case absolute:
        season.setAbsoluteStart(startAbsoluteEdit->date());
        break;
    case relative:
        getSeasonOffset(startRelativeEdit->value(), startRelativeCombo->currentData().toInt(), years, months, weeks);
        season.setOffsetStart(years, months, weeks, false);
        break;
    case duration:
        season.setLength(startDurationYearsEdit->value(), startDurationMonthsEdit->value(), startDurationDaysEdit->value());
        break;
    default:
        break;
    }
    switch (endCombo->currentData().toInt()) {
    case absolute:
        season.setAbsoluteEnd(endAbsoluteEdit->date());
        break;
    case relative:
        getSeasonOffset(endRelativeEdit->value(), endRelativeCombo->currentData().toInt(), years, months, weeks);
        season.setOffsetEnd(years, months, weeks, false);
        break;
    case duration:
        season.setLength(endDurationYearsEdit->value(), endDurationMonthsEdit->value(), endDurationDaysEdit->value());
        break;
    case ytd:
        season.setYtd();
        break;
    default:
        break;
    }
}


void
EditSeasonDialog::transferSeasonToUI
(Season &season)
{
    nameEdit->setText(season.getName());
    typeCombo->setCurrentIndex(typeCombo->findData(season.getType()));

    if (season.getAbsoluteStart().isValid()) {
        startAbsoluteEdit->setDate(season.getAbsoluteStart());
        startCombo->setCurrentIndex(startCombo->findData(absolute));
    } else if (season.getOffsetStart().isValid()) {
        SeasonOffset offset = season.getOffsetStart();
        startCombo->setCurrentIndex(startCombo->findData(relative));
        std::pair<SeasonOffset::SeasonOffsetType, int> item = offset.getSignificantItem();
        if (item.first != SeasonOffset::invalid) {
            startRelativeCombo->setCurrentIndex(startRelativeCombo->findData(item.first));
            startRelativeEdit->setValue(-item.second);
            startCombo->setCurrentIndex(startCombo->findData(relative));
        } else {
            startCombo->setCurrentIndex(startCombo->findData(absolute));
        }
    } else if (   season.getLength().isValid()
               && (   season.getAbsoluteEnd().isValid()
                   || season.getOffsetEnd().isValid())) {
        SeasonLength length = season.getLength();
        startDurationYearsEdit->setValue(length.getYears());
        startDurationMonthsEdit->setValue(length.getMonths());
        startDurationDaysEdit->setValue(length.getDays());
        startCombo->setCurrentIndex(startCombo->findData(duration));
    } else {
        startCombo->setCurrentIndex(startCombo->findData(absolute));
    }

    if (season.getAbsoluteEnd().isValid()) {
        endAbsoluteEdit->setDate(season.getAbsoluteEnd());
        endCombo->setCurrentIndex(endCombo->findData(absolute));
    } else if (season.getOffsetEnd().isValid()) {
        SeasonOffset offset = season.getOffsetEnd();
        endCombo->setCurrentIndex(startCombo->findData(relative));
        std::pair<SeasonOffset::SeasonOffsetType, int> item = offset.getSignificantItem();
        if (item.first != SeasonOffset::invalid) {
            endRelativeCombo->setCurrentIndex(endRelativeCombo->findData(item.first));
            endRelativeEdit->setValue(-item.second);
            endCombo->setCurrentIndex(endCombo->findData(relative));
        } else {
            endCombo->setCurrentIndex(endCombo->findData(absolute));
        }
    } else if (   season.getLength().isValid()
               && (   season.getAbsoluteStart().isValid()
                   || season.getOffsetStart().isValid())) {
        SeasonLength length = season.getLength();
        endDurationYearsEdit->setValue(length.getYears());
        endDurationMonthsEdit->setValue(length.getMonths());
        endDurationDaysEdit->setValue(length.getDays());
        endCombo->setCurrentIndex(endCombo->findData(duration));
    } else if (season.isYtd()) {
        endCombo->setCurrentIndex(endCombo->findData(ytd));
    }

    seedEdit->setValue(season.getSeed());
    lowEdit->setValue(season.getLow());
}


void
EditSeasonDialog::setEnabledItem
(QComboBox *combo, int data, bool enabled)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel*>(combo->model());
    if (! model) {
        return;
    }

    QStandardItem *item = model->item(combo->findData(data));
    if (! item) {
        return;
    }
    item->setEnabled(enabled);
}


void
EditSeasonDialog::getSeasonOffset
(int duration, int typeIndex, int &years, int &months, int &weeks) const
{
    years = 1;
    months = 1;
    weeks = 1;
    switch (typeIndex) {
    case SeasonOffset::week:
        weeks = -1 * duration;
        break;
    case SeasonOffset::month:
        months = -1 * duration;
        break;
    case SeasonOffset::year:
        years = -1 * duration;
        break;
    default:
        break;
    }
}


/*----------------------------------------------------------------------
 * EDIT SEASONEVENT DIALOG
 *--------------------------------------------------------------------*/

EditSeasonEventDialog::EditSeasonEventDialog(Context *context, SeasonEvent *event, Season &season) :
    QDialog(context->mainWindow, Qt::Dialog), context(context), event(event)
{
    setWindowTitle(tr("Edit Event"));

    nameEdit = new QLineEdit(this);
    nameEdit->setText(event->name);

    dateEdit = new QDateEdit(this);
    dateEdit->setDateRange(season.getStart(), season.getEnd());
    dateEdit->setDate(event->date);
    dateEdit->setCalendarPopup(true);

    priorityEdit = new QComboBox(this);
    foreach(QString priority, SeasonEvent::priorityList()) priorityEdit->addItem(priority);
    priorityEdit->setCurrentIndex(event->priority);

    descriptionEdit = new QTextEdit(this);
    descriptionEdit->setText(event->description);

    QDialogButtonBox *buttonBox = new QDialogButtonBox();
    applyButton = buttonBox->addButton(tr("&OK"), QDialogButtonBox::AcceptRole);
    QPushButton *cancelButton = buttonBox->addButton(tr("&Cancel"), QDialogButtonBox::RejectRole);

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->addRow(tr("Name"), nameEdit);
    formLayout->addRow(tr("Date"), dateEdit);
    formLayout->addRow(tr("Priority"), priorityEdit);
    formLayout->addRow(tr("Description"), descriptionEdit);
    formLayout->addRow(buttonBox);

    // connect up slots
    connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(nameEdit, SIGNAL(textChanged(const QString &)), this, SLOT(nameChanged()));

    // initialize button state
    nameChanged();
}

void
EditSeasonEventDialog::applyClicked()
{
    // get the values back
    event->name = nameEdit->text();
    event->date = dateEdit->date();
    event->priority = priorityEdit->currentIndex();
    event->description = descriptionEdit->toPlainText();
    accept();
}

void
EditSeasonEventDialog::cancelClicked()
{
    reject();
}

void EditSeasonEventDialog::nameChanged()
{
    applyButton->setEnabled(!nameEdit->text().isEmpty());
}


/*----------------------------------------------------------------------
 * EDIT PHASE DIALOG
 *--------------------------------------------------------------------*/

EditPhaseDialog::EditPhaseDialog(Context *context, Phase *phase, Season &season) :
    QDialog(context->mainWindow, Qt::Dialog), context(context), phase(phase)
{
    setWindowTitle(tr("Edit Date Range"));

    nameEdit = new QLineEdit();
    nameEdit->setText(phase->getName());

    typeEdit = new QComboBox();
    typeEdit->addItem(tr("Phase"), Phase::phase);
    typeEdit->addItem(tr("Prep"),  Phase::prep);
    typeEdit->addItem(tr("Base"),  Phase::base);
    typeEdit->addItem(tr("Build"), Phase::build);
    typeEdit->addItem(tr("Camp"), Phase::camp);
    typeEdit->setCurrentIndex(typeEdit->findData(phase->getType()));

    fromEdit = new QDateEdit();
    fromEdit->setDateRange(season.getStart(), season.getEnd());
    fromEdit->setDate(phase->getStart());
    fromEdit->setCalendarPopup(true);

    toEdit = new QDateEdit();
    toEdit->setDateRange(season.getStart(), season.getEnd());
    toEdit->setDate(phase->getEnd());
    toEdit->setCalendarPopup(true);

    seedEdit = new QDoubleSpinBox();
    seedEdit->setDecimals(0);
    seedEdit->setRange(0.0, 300.0);
    seedEdit->setSingleStep(1.0);
    seedEdit->setWrapping(true);
    seedEdit->setAlignment(Qt::AlignLeft);
    seedEdit->setValue(phase->getSeed());

    lowEdit = new QDoubleSpinBox();
    lowEdit->setDecimals(0);
    lowEdit->setRange(-500, 0);
    lowEdit->setSingleStep(1.0);
    lowEdit->setWrapping(true);
    lowEdit->setAlignment(Qt::AlignLeft);
    lowEdit->setValue(phase->getLow());

    QDialogButtonBox *buttonBox = new QDialogButtonBox();
    applyButton = buttonBox->addButton(tr("&OK"), QDialogButtonBox::AcceptRole);
    QPushButton *cancelButton = buttonBox->addButton(tr("&Cancel"), QDialogButtonBox::RejectRole);

    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->addRow(tr("Name"), nameEdit);
    formLayout->addRow(tr("Type"), typeEdit);
    formLayout->addRow(tr("From"), fromEdit);
    formLayout->addRow(tr("To"), toEdit);
    formLayout->addRow(tr("Starting LTS"), seedEdit);
    formLayout->addRow(tr("Lowest SB"), lowEdit);
    formLayout->addRow(buttonBox);

    // connect up slots
    connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(nameEdit, SIGNAL(textChanged(const QString &)), this, SLOT(nameChanged()));

    // initialize button state
    nameChanged();
}

void
EditPhaseDialog::applyClicked()
{
    // get the values back
    phase->setName(nameEdit->text());
    phase->setType(typeEdit->itemData(typeEdit->currentIndex()).toInt());
    phase->setAbsoluteStart(fromEdit->date());
    phase->setAbsoluteEnd(toEdit->date());
    phase->setSeed(seedEdit->value());
    phase->setLow(lowEdit->value());
    accept();
}

void
EditPhaseDialog::cancelClicked()
{
    reject();
}

void EditPhaseDialog::nameChanged()
{
    applyButton->setEnabled(!nameEdit->text().isEmpty());
}


/*----------------------------------------------------------------------
 * SEASON TREE VIEW
 *--------------------------------------------------------------------*/

SeasonTreeView::SeasonTreeView(Context *context) : context(context)
{
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setDragDropOverwriteMode(true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

}

QStringList
SeasonTreeView::mimeTypes() const
{
    QStringList returning;
    returning << "application/x-gc-seasons";

    return returning;
}

QMimeData *
SeasonTreeView::mimeData
#if QT_VERSION < 0x060000
(const QList<QTreeWidgetItem *> items) const
#else
(const QList<QTreeWidgetItem *> &items) const
#endif
{
    QMimeData *returning = new QMimeData;

    // we need to pack into a byte array
    QByteArray rawData;
    QDataStream stream(&rawData, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_6);

    // pack data
    stream << (quint64)(context); // where did this come from?
    stream << int(items.count());
    foreach (QTreeWidgetItem *p, items) {
        int seasonIdx = -1;
        int phaseIdx = -1;

        if (p->type() >= Season::season && p->type() < Phase::phase) {
            // get the season this is for
            seasonIdx = p->treeWidget()->invisibleRootItem()->indexOfChild(p);
        } else if (p->type() >= Phase::phase) {
            // get the season this is for
            seasonIdx = p->treeWidget()->invisibleRootItem()->indexOfChild(p->parent());
            phaseIdx = p->parent()->indexOfChild(p);
        }

        // season[index] ...
        if (phaseIdx > -1) {
            stream << context->athlete->seasons->seasons[seasonIdx].getName() + "/" + context->athlete->seasons->seasons[seasonIdx].phases[phaseIdx].getName(); // name
            stream << context->athlete->seasons->seasons[seasonIdx].phases[phaseIdx].getStart();
            stream << context->athlete->seasons->seasons[seasonIdx].phases[phaseIdx].getEnd();
        } else {
            stream << context->athlete->seasons->seasons[seasonIdx].getName(); // name
            stream << context->athlete->seasons->seasons[seasonIdx].getStart();
            stream << context->athlete->seasons->seasons[seasonIdx].getEnd();
        }

    }

    // and return as mime data
    returning->setData("application/x-gc-seasons", rawData);
    return returning;
}

void
SeasonTreeView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->source() != this || event->dropAction() != Qt::MoveAction) return;

    QAbstractItemView::dragEnterEvent(event);
}

void
SeasonTreeView::dropEvent(QDropEvent* event)
{
    // get the list of the items that are about to be dropped
    QTreeWidgetItem* item = selectedItems()[0];

    // row we started on
    int idx1 = indexFromItem(item).row();

    // don't move temp 'system generated' date ranges!
    if (context->athlete->seasons->seasons[idx1].getType() != Season::temporary) {

        // the default implementation takes care of the actual move inside the tree
        QTreeWidget::dropEvent(event);

        // moved to !
        int idx2 = indexFromItem(item).row();

        // notify subscribers in some useful way
        Q_EMIT itemMoved(item, idx1, idx2);

    } else {

        event->ignore();
    }
}


/*----------------------------------------------------------------------
 * SEASON EVENT TREE VIEW
 *--------------------------------------------------------------------*/

SeasonEventTreeView::SeasonEventTreeView()
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(true);
}

void
SeasonEventTreeView::dropEvent(QDropEvent* event)
{
    // get the list of the items that are about to be dropped
    QTreeWidgetItem* item = selectedItems()[0];

    // row we started on
    int idx1 = indexFromItem(item).row();

    // the default implementation takes care of the actual move inside the tree
    QTreeWidget::dropEvent(event);

    // moved to !
    int idx2 = indexFromItem(item).row();

    // notify subscribers in some useful way
    Q_EMIT itemMoved(item, idx1, idx2);
}
