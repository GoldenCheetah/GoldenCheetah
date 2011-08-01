/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#include "ChooseCyclistDialog.h"
#include "NewCyclistDialog.h"
#include "MainWindow.h"
#include <QtGui>

ChooseCyclistDialog::ChooseCyclistDialog(const QDir &home, bool allowNew) :
    home(home)
{
    setWindowTitle(tr("Choose a Cyclist"));

    listWidget = new QListWidget(this);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    QStringListIterator i(home.entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {
        QString name = i.next();

        // only offer cyclists which are not already open
        bool available=true;
        foreach (MainWindow *x, mainwindows)
            if (x->cyclist == name) available=false;

        if (available) new QListWidgetItem(name, listWidget);
    }

    if (allowNew)
        newButton = new QPushButton(tr("&New..."), this);
    okButton = new QPushButton(tr("&Open"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);

    okButton->setEnabled(false);

    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    if (allowNew)
        connect(newButton, SIGNAL(clicked()), this, SLOT(newClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(listWidget,
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(enableOk(QListWidgetItem*)));
    connect(listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(accept()));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(okButton);
    if (allowNew)
        buttonLayout->addWidget(newButton);
    buttonLayout->addWidget(cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(listWidget);
    mainLayout->addLayout(buttonLayout);
}

QString
ChooseCyclistDialog::choice()
{
    return listWidget->currentItem()->text();
}

void
ChooseCyclistDialog::enableOk(QListWidgetItem *item)
{
    okButton->setEnabled(item != NULL);
}

void
ChooseCyclistDialog::cancelClicked()
{
    reject();
}

QString
ChooseCyclistDialog::newCyclistDialog(QDir &homeDir, QWidget *)
{
    NewCyclistDialog *newone = new NewCyclistDialog(homeDir);

    // get new one..
    QString name;
    if (newone->exec() == QDialog::Accepted) 
        name = newone->name->text();
    else
        name = "";

    // zap the dialog now we have the results
    delete newone;

    // blank if cancelled
    return name;
}

void
ChooseCyclistDialog::newClicked()
{
    QString name = newCyclistDialog(home, this);
    if (!name.isEmpty())
        new QListWidgetItem(name, listWidget);
}

