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
#include <QString>
#include <QFile>
#include <QXmlInputSource>
#include "SeasonParser.h"
#include <QXmlSimpleReader>

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

/*----------------------------------------------------------------------
 * EDIT SEASON DIALOG
 *--------------------------------------------------------------------*/
EditSeasonDialog::EditSeasonDialog(MainWindow *mainWindow, Season *season) :
    QDialog(mainWindow, Qt::Dialog), mainWindow(mainWindow), season(season)
{
    setWindowTitle(tr("Edit Date Range"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Grid
    QGridLayout *grid = new QGridLayout;
    QLabel *name = new QLabel("Name");
    QLabel *type = new QLabel("Type");
    QLabel *from = new QLabel("From");
    QLabel *to = new QLabel("To");

    nameEdit = new QLineEdit(this);
    nameEdit->setText(season->getName());

    typeEdit = new QComboBox;
    typeEdit->addItem("Season", Season::season);
    typeEdit->addItem("Cycle", Season::cycle);
    typeEdit->addItem("Adhoc", Season::adhoc);
    typeEdit->setCurrentIndex(typeEdit->findData(season->getType()));

    fromEdit = new QDateEdit(this);
    fromEdit->setDate(season->getStart());

    toEdit = new QDateEdit(this);
    toEdit->setDate(season->getEnd());

    grid->addWidget(name, 0,0);
    grid->addWidget(nameEdit, 0,1);
    grid->addWidget(type, 1,0);
    grid->addWidget(typeEdit, 1,1);
    grid->addWidget(from, 2,0);
    grid->addWidget(fromEdit, 2,1);
    grid->addWidget(to, 3,0);
    grid->addWidget(toEdit, 3,1);

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
}

void
EditSeasonDialog::applyClicked()
{
    // get the values back
    season->setName(nameEdit->text());
    season->setType(typeEdit->itemData(typeEdit->currentIndex()).toInt());
    season->setStart(fromEdit->date());
    season->setEnd(toEdit->date());
    accept();
}
void
EditSeasonDialog::cancelClicked()
{
    reject();
}

/*----------------------------------------------------------------------
 * EDIT SEASONEVENT DIALOG
 *--------------------------------------------------------------------*/
EditSeasonEventDialog::EditSeasonEventDialog(MainWindow *mainWindow, SeasonEvent *event) :
    QDialog(mainWindow, Qt::Dialog), mainWindow(mainWindow), event(event)
{
    setWindowTitle(tr("Edit Event"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Grid
    QGridLayout *grid = new QGridLayout;
    QLabel *name = new QLabel("Name");
    QLabel *date = new QLabel("Date");

    nameEdit = new QLineEdit(this);
    nameEdit->setText(event->name);

    dateEdit = new QDateEdit(this);
    dateEdit->setDate(event->date);

    grid->addWidget(name, 0,0);
    grid->addWidget(nameEdit, 0,1);
    grid->addWidget(date, 1,0);
    grid->addWidget(dateEdit, 1,1);

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
}

void
EditSeasonEventDialog::applyClicked()
{
    // get the values back
    event->name = nameEdit->text();
    event->date = dateEdit->date();
    accept();
}

void
EditSeasonEventDialog::cancelClicked()
{
    reject();
}

//
// Manage the seasons array
//
void
Seasons::readSeasons()
{
    QFile seasonFile(home.absolutePath() + "/seasons.xml");
    QXmlInputSource source( &seasonFile );
    QXmlSimpleReader xmlReader;
    SeasonParser( handler );
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

    season.setName(tr("Last 3 months"));
    season.setType(Season::temporary);
    season.setEnd(today);
    season.setStart(today.addMonths(-3));
    season.setId(QUuid("{00000000-0000-0000-0000-000000000008}"));
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
    QString file = QString(home.absolutePath() + "/seasons.xml");
    SeasonParser::serialize(file, seasons);

    seasonsChanged(); // signal!
}

SeasonTreeView::SeasonTreeView()
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
    // item and original position
    QTreeWidgetItem *item = currentItem();
    int idx1 = invisibleRootItem()->indexOfChild(item);
    int idx2 = indexAt(event->pos()).row();

    // finalise drop event
    QTreeWidget::dropEvent(event);

    // emit the itemMoved signal
    Q_EMIT itemMoved(item, idx1, idx2);
}
