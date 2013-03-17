/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "IntervalItem.h"
#include "RideFile.h"

IntervalItem::IntervalItem(const RideFile *ride, QString name, double start, double stop, double startKM, double stopKM, int displaySequence) : ride(ride), start(start), stop(stop), startKM(startKM), stopKM(stopKM), displaySequence(displaySequence)
{
    setText(0, name);
}

/*----------------------------------------------------------------------
 * Interval rename dialog
 *--------------------------------------------------------------------*/
RenameIntervalDialog::RenameIntervalDialog(QString &string, QWidget *parent) :
    QDialog(parent, Qt::Dialog), string(string)
{
    setWindowTitle(tr("Renumber Intervals"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Grid
    QGridLayout *grid = new QGridLayout;
    QLabel *name = new QLabel("Name");

    nameEdit = new QLineEdit(this);
    nameEdit->setText(string);

    grid->addWidget(name, 0,0);
    grid->addWidget(nameEdit, 0,1);

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
RenameIntervalDialog::applyClicked()
{
    // get the values back
    string = nameEdit->text();
    accept();
}

void
RenameIntervalDialog::cancelClicked()
{
    reject();
}
