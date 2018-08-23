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

Season::Season()
{
    type = season;  // by default seasons are of type season
    _id = QUuid::createUuid(); // in case it isn't set yet
    _seed = 0;
    _low = -50;
}

QString Season::getName() {

    return name;
}

QDate Season::getStart()
{
    return start;
}

QDate Season::getEnd()
{
    return end;
}

int Season::getType()
{
    return type;
}

// how many days prior does this season represent
// if value is 0 then its not a Last x days kind of season
int Season::prior()
{
    //  Last x days, Last x months turn into number of days prior
    if (id() == QUuid("{00000000-0000-0000-0000-000000000005}")) return -7;
    if (id() == QUuid("{00000000-0000-0000-0000-000000000006}")) return -14;
    if (id() == QUuid("{00000000-0000-0000-0000-000000000011}")) return -21;
    if (id() == QUuid("{00000000-0000-0000-0000-000000000007}")) return -28;
    if (id() == QUuid("{00000000-0000-0000-0000-000000000008}")) return -60;
    if (id() == QUuid("{00000000-0000-0000-0000-000000000012}")) return -90;
    if (id() == QUuid("{00000000-0000-0000-0000-000000000009}")) return -180;
    if (id() == QUuid("{00000000-0000-0000-0000-000000000010}")) return -365;
    if (id() == QUuid("{00000000-0000-0000-0000-000000000015}")) return -42;
    if (id() == QUuid("{00000000-0000-0000-0000-000000000016}")) return -1;
    return 0;
}

void Season::setEnd(QDate _end)
{
    end = _end;
}

void Season::setStart(QDate _start)
{
    start = _start;
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
    return a.start.toJulianDay() < b.start.toJulianDay();
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
    QDate today = QDate::currentDate();
    QDate eom = QDate(today.year(), today.month(), today.daysInMonth());

    // add Default Date Ranges
    season.setName(tr("All Dates"));
    season.setType(Season::temporary);
    season.setStart(QDate::currentDate().addYears(-50));
    season.setEnd(QDate::currentDate().addYears(50));
    season.setId(QUuid("{00000000-0000-0000-0000-000000000001}"));
    seasons.append(season);

    season.setName(tr("This Year"));
    season.setType(Season::temporary);
    season.setStart(QDate(today.year(), 1,1));
    season.setEnd(QDate(today.year(), 12, 31));
    season.setId(QUuid("{00000000-0000-0000-0000-000000000002}"));
    seasons.append(season);

    season.setName(tr("This Month"));
    season.setType(Season::temporary);
    season.setStart(QDate(today.year(), today.month(),1));
    season.setEnd(eom);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000003}"));
    seasons.append(season);

    season.setName(tr("Last Month"));
    season.setType(Season::temporary);
    season.setStart(QDate(today.year(), today.month(),1).addMonths(-1));
    season.setEnd(QDate(today.year(), today.month(),1).addDays(-1));
    season.setId(QUuid("{00000000-0000-0000-0000-000000000013}"));
    seasons.append(season);

    season.setName(tr("This Week"));
    season.setType(Season::temporary);
    // from Mon-Sun
    QDate wstart = QDate::currentDate();
    wstart = wstart.addDays(Qt::Monday - wstart.dayOfWeek());
    QDate wend = wstart.addDays(6); // first day + 6 more
    season.setStart(wstart);
    season.setEnd(wend);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000004}"));
    seasons.append(season);

    season.setName(tr("Last Week"));
    season.setType(Season::temporary);
    // from Mon-Sun
    season.setStart(wstart.addDays(-7));
    season.setEnd(wend.addDays(-7));
    season.setId(QUuid("{00000000-0000-0000-0000-000000000014}"));
    seasons.append(season);

    season.setName(tr("Last 24 hours"));
    season.setType(Season::temporary);
    season.setStart(today.addDays(-1)); // today plus previous 6
    season.setEnd(today);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000016}"));
    seasons.append(season);

    season.setName(tr("Last 7 days"));
    season.setType(Season::temporary);
    season.setStart(today.addDays(-6)); // today plus previous 6
    season.setEnd(today);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000005}"));
    seasons.append(season);

    season.setName(tr("Last 14 days"));
    season.setType(Season::temporary);
    season.setStart(today.addDays(-13));
    season.setEnd(today);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000006}"));
    seasons.append(season);

    season.setName(tr("Last 21 days"));
    season.setType(Season::temporary);
    season.setStart(today.addDays(-20));
    season.setEnd(today);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000011}"));
    seasons.append(season);

    season.setName(tr("Last 28 days"));
    season.setType(Season::temporary);
    season.setStart(today.addDays(-27));
    season.setEnd(today);
    season.setId(QUuid("{00000000-0000-0000-0000-000000000007}"));
    seasons.append(season);

    season.setName(tr("Last 6 weeks"));
    season.setType(Season::temporary);
    season.setEnd(today);
    season.setStart(today.addDays(-41));
    season.setId(QUuid("{00000000-0000-0000-0000-000000000015}"));
    seasons.append(season);

    season.setName(tr("Last 2 months"));
    season.setType(Season::temporary);
    season.setEnd(today);
    season.setStart(today.addMonths(-2));
    season.setId(QUuid("{00000000-0000-0000-0000-000000000008}"));
    seasons.append(season);

    season.setName(tr("Last 3 months"));
    season.setType(Season::temporary);
    season.setEnd(today);
    season.setStart(today.addMonths(-3));
    season.setId(QUuid("{00000000-0000-0000-0000-000000000012}"));
    seasons.append(season);

    season.setName(tr("Last 6 months"));
    season.setType(Season::temporary);
    season.setEnd(today);
    season.setStart(today.addMonths(-6));
    season.setId(QUuid("{00000000-0000-0000-0000-000000000009}"));
    seasons.append(season);

    season.setName(tr("Last 12 months"));
    season.setType(Season::temporary);
    season.setEnd(today);
    season.setStart(today.addMonths(-12));
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

Season
Seasons::seasonFor(QDate date)
{
    foreach(Season season, seasons)
        if (date >= season.start && date <= season.end)
            return season;

    // if not found just return an all dates season
    Season returning; // just retun an all dates
    returning.setName(tr("All Dates"));
    returning.setType(Season::temporary);
    returning.setStart(QDate::currentDate().addYears(-50));
    returning.setEnd(QDate::currentDate().addYears(50));
    returning.setId(QUuid("{00000000-0000-0000-0000-000000000001}"));

    return returning;
}

SeasonTreeView::SeasonTreeView(Context *context) : context(context)
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

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
SeasonTreeView::mimeData (const QList<QTreeWidgetItem *> items) const
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
            stream << context->athlete->seasons->seasons[seasonIdx].phases[phaseIdx].start;
            stream << context->athlete->seasons->seasons[seasonIdx].phases[phaseIdx].end;
            stream << (quint64)context->athlete->seasons->seasons[seasonIdx].phases[phaseIdx].days();
        } else {
            stream << context->athlete->seasons->seasons[seasonIdx].name; // name
            stream << context->athlete->seasons->seasons[seasonIdx].start;
            stream << context->athlete->seasons->seasons[seasonIdx].end;
            stream << (quint64)context->athlete->seasons->seasons[seasonIdx].days();
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

Phase::Phase(QString _name, QDate _start, QDate _end) : Season()
{
    type = phase;  // by default phase are of type phase
    name = _name;
    start = _start;
    end = _end;
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
