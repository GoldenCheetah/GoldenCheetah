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

#include "Season.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include <QString>
#include <QFile>
#include <QXmlInputSource>
#include "SeasonParser.h"
#include <QXmlSimpleReader>

QStringList
SeasonEvent::priorityList()
{
    return QStringList()<<" "<<tr("A")<<tr("B")<<tr("C")<<tr("D")<<tr("E");
}

static QList<QString> _setSeasonTypes()
{
    QList<QString> returning;
    returning << "Season"
              << "Cycle"
              << "Adhoc"
              << "System";
    return returning;
}

QList<QString> Season::types = _setSeasonTypes();

SeasonOffset::SeasonOffset()
{
    years = 1;
    months = 1;
    weeks = 1;
}

SeasonOffset::SeasonOffset(int _years, int _months, qint64 _weeks)
{
    years = _years;
    months = _months;
    weeks = _weeks;
}

QDate SeasonOffset::getStart(QDate reference) const
{
    if (years<=0) {
        return QDate(reference.year(), 1,1).addYears(years);
    } else if (months<=0) {
        return QDate(reference.year(), reference.month(),1).addMonths(months);
    } else if (weeks<=0) {
        // from Mon-Sun
        return reference.addDays((Qt::Monday - reference.dayOfWeek()) + 7*weeks);
    } else {
        return QDate();
    }
}

SeasonLength::SeasonLength()
{
    years = 0;
    months = 0;
    days = 0;
}

SeasonLength::SeasonLength(int _years, int _months, qint64 _days)
{
    years = _years;
    months = _months;
    days = _days;
}

bool SeasonLength::operator==(const SeasonLength& length)
{
    return years==length.years && months==length.months && days==length.days;
}

QDate SeasonLength::addTo(QDate start) const
{
    return start.addYears(years).addMonths(months).addDays(days-1);
}

QDate SeasonLength::substractFrom(QDate end) const
{
    return end.addYears(-years).addMonths(-months).addDays(1-days);
}

Season::Season()
{
    type = season;  // by default seasons are of type season
    _id = QUuid::createUuid(); // in case it isn't set yet
    _seed = 0;
    _low = -50;
    _offset = SeasonOffset();
    _length = SeasonLength();
    _start = QDate();
    _end = QDate();
}

QString Season::getName()
{
    return name;
}

QDate Season::getStart(QDate reference) const
{
    QDate start = _offset.getStart(reference);

    if (start==QDate()) {
        if (getLength()==SeasonLength()) {
            // fixed season
            return _start;
        } else {
            // relative season with a specified end
            return _length.substractFrom(reference);
        }
    } else {
        // relative season with a specified start
        return start;
    }
}

QDate Season::getEnd(QDate reference) const
{
    QDate start = _offset.getStart(reference);

    if (start==QDate()) {
        if (getLength()==SeasonLength()) {
            // fixed season
            return _end;
        } else {
            // relative season with a specified end
            return reference;
        }
    } else {
        // relative season with a specified start
        return _length.addTo(start);
    }
}

QDate Season::getStart() const
{
    return getStart(QDate::currentDate());
}

QDate Season::getEnd() const
{
    return getEnd(QDate::currentDate());
}

int Season::getType()
{
    return type;
}

void Season::setEnd(QDate end)
{
    _offset = SeasonOffset();
    _length = SeasonLength();
    _end = end;
}

void Season::setStart(QDate start)
{
    _offset = SeasonOffset();
    _length = SeasonLength();
    _start = start;
}

void Season::setOffsetAndLength(int offetYears, int offsetMonths, qint64 offsetWeeks, int years, int months, qint64 days)
{
    type = temporary;
    _offset = SeasonOffset(offetYears, offsetMonths, offsetWeeks);
    _length = SeasonLength(years, months, days);
    _start = QDate();
    _end = QDate();
}

void Season::setLength(int years, int months, qint64 days)
{
    type = temporary;
    _offset = SeasonOffset();
    _length = SeasonLength(years, months, days);
    _start = QDate();
    _end = QDate();
}

void Season::setName(QString _name)
{
    name = _name;
}

void Season::setType(int _type)
{
    type = _type;
}

bool Season::LessThanForStarts(const Season &a, const Season &b)
{
    return a.getStart().toJulianDay() < b.getStart().toJulianDay();
}

/*----------------------------------------------------------------------
 * EDIT SEASON DIALOG
 *--------------------------------------------------------------------*/
EditSeasonDialog::EditSeasonDialog(Context *context, Season *season) :
    QDialog(context->mainWindow, Qt::Dialog), context(context), season(season)
{
    setWindowTitle(tr("Edit Date Range"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Grid
    QGridLayout *grid = new QGridLayout;
    QLabel *name = new QLabel(tr("Name"));
    QLabel *type = new QLabel(tr("Type"));
    QLabel *from = new QLabel(tr("From"));
    QLabel *to = new QLabel(tr("To"));
    QLabel *seed = new QLabel(tr("Starting LTS"));
    QLabel *low  = new QLabel(tr("Lowest SB"));

    nameEdit = new QLineEdit(this);
    nameEdit->setText(season->getName());

    typeEdit = new QComboBox;
    typeEdit->addItem(tr("Season"), Season::season);
    typeEdit->addItem(tr("Cycle"), Season::cycle);
    typeEdit->addItem(tr("Adhoc"), Season::adhoc);
    typeEdit->setCurrentIndex(typeEdit->findData(season->getType()));

    fromEdit = new QDateEdit(this);
    fromEdit->setDate(season->getStart());
    fromEdit->setCalendarPopup(true);

    toEdit = new QDateEdit(this);
    toEdit->setDate(season->getEnd());
    toEdit->setCalendarPopup(true);

    seedEdit = new QDoubleSpinBox(this);
    seedEdit->setDecimals(0);
    seedEdit->setRange(0.0, 300.0);
    seedEdit->setSingleStep(1.0);
    seedEdit->setWrapping(true);
    seedEdit->setAlignment(Qt::AlignLeft);
    seedEdit->setValue(season->getSeed());

    lowEdit = new QDoubleSpinBox(this);
    lowEdit->setDecimals(0);
    lowEdit->setRange(-500, 0);
    lowEdit->setSingleStep(1.0);
    lowEdit->setWrapping(true);
    lowEdit->setAlignment(Qt::AlignLeft);
    lowEdit->setValue(season->getLow());


    grid->addWidget(name, 0,0);
    grid->addWidget(nameEdit, 0,1);
    grid->addWidget(type, 1,0);
    grid->addWidget(typeEdit, 1,1);
    grid->addWidget(from, 2,0);
    grid->addWidget(fromEdit, 2,1);
    grid->addWidget(to, 3,0);
    grid->addWidget(toEdit, 3,1);
    grid->addWidget(seed, 4,0);
    grid->addWidget(seedEdit, 4,1);
    grid->addWidget(low, 5,0);
    grid->addWidget(lowEdit, 5,1);

    mainLayout->addLayout(grid);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    applyButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);
    mainLayout->addLayout(buttonLayout);

    // connect up slots
    connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(nameEdit, SIGNAL(textChanged(const QString &)), this, SLOT(nameChanged()));

    // initialize button state
    nameChanged();
}

void
EditSeasonDialog::applyClicked()
{
    // get the values back
    season->setName(nameEdit->text());
    season->setType(typeEdit->itemData(typeEdit->currentIndex()).toInt());
    season->setStart(fromEdit->date());
    season->setEnd(toEdit->date());
    season->setSeed(seedEdit->value());
    season->setLow(lowEdit->value());
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

/*----------------------------------------------------------------------
 * EDIT SEASONEVENT DIALOG
 *--------------------------------------------------------------------*/
EditSeasonEventDialog::EditSeasonEventDialog(Context *context, SeasonEvent *event, Season &season) :
    QDialog(context->mainWindow, Qt::Dialog), context(context), event(event)
{
    setWindowTitle(tr("Edit Event"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Grid
    QGridLayout *grid = new QGridLayout;
    QLabel *name = new QLabel(tr("Name"));
    QLabel *date = new QLabel(tr("Date"));
    QLabel *priority = new QLabel(tr("Priority"));
    QLabel *description = new QLabel(tr("Description"));

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

    grid->addWidget(name, 0,0);
    grid->addWidget(nameEdit, 0,1);
    grid->addWidget(date, 1,0);
    grid->addWidget(dateEdit, 1,1);
    grid->addWidget(priority, 2,0);
    grid->addWidget(priorityEdit, 2,1);
    grid->addWidget(description, 3, 0);
    grid->addWidget(descriptionEdit, 4,0,4,2);

    mainLayout->addLayout(grid);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    applyButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);
    mainLayout->addLayout(buttonLayout);

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

//
// Manage the seasons array
//
void
Seasons::readSeasons()
{
    QFile seasonFile(home.canonicalPath() + "/seasons.xml");
    QXmlInputSource source( &seasonFile );
    QXmlSimpleReader xmlReader;
    SeasonParser handler;
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );
    seasons = handler.getSeasons();

    Season season;

    // add Default Date Ranges
    season.setName(tr("All Dates"));
    season.setOffsetAndLength(-50, 1, 1, 100, 0, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000001}"));
    seasons.append(season);

    season.setName(tr("This Year"));
    season.setOffsetAndLength(0, 1, 1, 1, 0, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000002}"));
    seasons.append(season);

    season.setName(tr("This Month"));
    season.setOffsetAndLength(1, 0, 1, 0, 1, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000003}"));
    seasons.append(season);

    season.setName(tr("Last Month"));
    season.setOffsetAndLength(1, -1, 1, 0, 1, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000013}"));
    seasons.append(season);

    season.setName(tr("This Week"));
    season.setOffsetAndLength(1, 1, 0, 0, 0, 7);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000004}"));
    seasons.append(season);

    season.setName(tr("Last Week"));
    season.setOffsetAndLength(1, 1, -1, 0, 0, 7);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000014}"));
    seasons.append(season);

    season.setName(tr("Last 24 hours"));
    season.setLength(0, 0, 2);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000016}"));
    seasons.append(season);

    season.setName(tr("Last 7 days"));
    season.setLength(0, 0, 7);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000005}"));
    seasons.append(season);

    season.setName(tr("Last 14 days"));
    season.setLength(0, 0, 14);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000006}"));
    seasons.append(season);

    season.setName(tr("Last 21 days"));
    season.setLength(0, 0, 21);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000011}"));
    seasons.append(season);

    season.setName(tr("Last 28 days"));
    season.setLength(0, 0, 28);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000007}"));
    seasons.append(season);

    season.setName(tr("Last 6 weeks"));
    season.setLength(0, 0, 42);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000015}"));
    seasons.append(season);

    season.setName(tr("Last 2 months"));
    season.setLength(0, 2, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000008}"));
    seasons.append(season);

    season.setName(tr("Last 3 months"));
    season.setLength(0, 3, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000012}"));
    seasons.append(season);

    season.setName(tr("Last 6 months"));
    season.setLength(0, 6, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000009}"));
    seasons.append(season);

    season.setName(tr("Last 12 months"));
    season.setLength(1, 0, 0);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000010}"));
    seasons.append(season);

    seasonsChanged(); // signal!
}

int
Seasons::newSeason(QString name, QDate start, QDate end, int type)
{
    Season add;
    add.setName(name);
    add.setStart(start);
    add.setEnd(end);
    add.setType(type);
    seasons.insert(0, add);

    // save changes away
    writeSeasons();

    return 0; // always add at the top
}

void
Seasons::updateSeason(int index, QString name, QDate start, QDate end, int type)
{
    seasons[index].setName(name);
    seasons[index].setStart(start);
    seasons[index].setEnd(end);
    seasons[index].setType(type);

    // save changes away
    writeSeasons();

}

void
Seasons::deleteSeason(int index)
{
    // now delete!
    seasons.removeAt(index);
    writeSeasons();
}

void
Seasons::writeSeasons()
{
    // update seasons.xml
    QString file = QString(home.canonicalPath() + "/seasons.xml");
    SeasonParser::serialize(file, seasons);

    seasonsChanged(); // signal!
}

SeasonTreeView::SeasonTreeView(Context *context) : context(context)
{
    setDragDropMode(QAbstractItemView::DragDrop);
    setDefaultDropAction(Qt::MoveAction);
    setDragDropOverwriteMode(true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

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
    if (context->athlete->seasons->seasons[idx1].type != Season::temporary) {

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
    stream << items.count();
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
            stream << context->athlete->seasons->seasons[seasonIdx].name + "/" + context->athlete->seasons->seasons[seasonIdx].phases[phaseIdx].name; // name
            stream << context->athlete->seasons->seasons[seasonIdx].phases[phaseIdx].getStart();
            stream << context->athlete->seasons->seasons[seasonIdx].phases[phaseIdx].getEnd();
        } else {
            stream << context->athlete->seasons->seasons[seasonIdx].name; // name
            stream << context->athlete->seasons->seasons[seasonIdx].getStart();
            stream << context->athlete->seasons->seasons[seasonIdx].getEnd();
        }

    }

    // and return as mime data
    returning->setData("application/x-gc-seasons", rawData);
    return returning;
}

static QList<QString> _setPhaseTypes()
{
    QList<QString> returning;
    returning << "Phase"
              << "Prep"
              << "Base"
              << "Build"
              << "Peak"
              << "Camp";
    return returning;
}

QList<QString> Phase::types = _setPhaseTypes();

Phase::Phase() : Season()
{
    type = phase;  // by default phase are of type phase
    _id = QUuid::createUuid(); // in case it isn't set yet
    _seed = 0;
    _low = -50;
}

Phase::Phase(QString _name, QDate start, QDate end) : Season()
{
    type = phase;  // by default phase are of type phase
    name = _name;
    _start = start;
    _end = end;
    _id = QUuid::createUuid(); // in case it isn't set yet
    _seed = 0;
    _low = -50;
}

/*----------------------------------------------------------------------
 * EDIT PHASE DIALOG
 *--------------------------------------------------------------------*/
EditPhaseDialog::EditPhaseDialog(Context *context, Phase *phase, Season &season) :
    QDialog(context->mainWindow, Qt::Dialog), context(context), phase(phase)
{
    setWindowTitle(tr("Edit Date Range"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Grid
    QGridLayout *grid = new QGridLayout;
    QLabel *name = new QLabel(tr("Name"));
    QLabel *type = new QLabel(tr("Type"));
    QLabel *from = new QLabel(tr("From"));
    QLabel *to = new QLabel(tr("To"));
    QLabel *seed = new QLabel(tr("Starting LTS"));
    QLabel *low  = new QLabel(tr("Lowest SB"));

    nameEdit = new QLineEdit(this);
    nameEdit->setText(phase->getName());

    typeEdit = new QComboBox;
    typeEdit->addItem(tr("Phase"), Phase::phase);
    typeEdit->addItem(tr("Prep"),  Phase::prep);
    typeEdit->addItem(tr("Base"),  Phase::base);
    typeEdit->addItem(tr("Build"), Phase::build);
    typeEdit->addItem(tr("Camp"), Phase::build);
    typeEdit->setCurrentIndex(typeEdit->findData(phase->getType()));

    fromEdit = new QDateEdit(this);
    fromEdit->setDateRange(season.getStart(), season.getEnd());
    fromEdit->setDate(phase->getStart());
    fromEdit->setCalendarPopup(true);

    toEdit = new QDateEdit(this);
    toEdit->setDateRange(season.getStart(), season.getEnd());
    toEdit->setDate(phase->getEnd());
    toEdit->setCalendarPopup(true);

    seedEdit = new QDoubleSpinBox(this);
    seedEdit->setDecimals(0);
    seedEdit->setRange(0.0, 300.0);
    seedEdit->setSingleStep(1.0);
    seedEdit->setWrapping(true);
    seedEdit->setAlignment(Qt::AlignLeft);
    seedEdit->setValue(phase->getSeed());

    lowEdit = new QDoubleSpinBox(this);
    lowEdit->setDecimals(0);
    lowEdit->setRange(-500, 0);
    lowEdit->setSingleStep(1.0);
    lowEdit->setWrapping(true);
    lowEdit->setAlignment(Qt::AlignLeft);
    lowEdit->setValue(phase->getLow());


    grid->addWidget(name, 0,0);
    grid->addWidget(nameEdit, 0,1);
    grid->addWidget(type, 1,0);
    grid->addWidget(typeEdit, 1,1);
    grid->addWidget(from, 2,0);
    grid->addWidget(fromEdit, 2,1);
    grid->addWidget(to, 3,0);
    grid->addWidget(toEdit, 3,1);
    grid->addWidget(seed, 4,0);
    grid->addWidget(seedEdit, 4,1);
    grid->addWidget(low, 5,0);
    grid->addWidget(lowEdit, 5,1);

    mainLayout->addLayout(grid);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    applyButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);
    mainLayout->addLayout(buttonLayout);

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
    phase->setStart(fromEdit->date());
    phase->setEnd(toEdit->date());
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
