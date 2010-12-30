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

Season::Season()
{
    type = season;  // by default seasons are of type season
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
