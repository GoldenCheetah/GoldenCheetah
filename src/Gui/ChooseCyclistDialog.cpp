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
#include "Context.h"
#include "Athlete.h"
#include <QtGui>

static void recursiveDelete(QDir dir)
{   
    // delete the directory contents recursively before
    // removing the directory itself
    foreach(QString name, dir.entryList()) {

        // ignore . and ..
        if (name == "." || name == "..") continue;

        QString path = dir.absolutePath() + "/" + name;

        // directory?
        if (QFileInfo(path).isDir()) {
            recursiveDelete(QDir(path));
            dir.rmdir(name);
        } else dir.remove(name);
    }

    // on a mac .DS_Store gets created and we should wipe it
#ifdef Q_OS_MAC
    if (QFileInfo(dir.absolutePath() + "/.DS_Store").exists()) dir.remove(".DS_Store");
#endif
}

ChooseCyclistDialog::ChooseCyclistDialog(const QDir &home, bool allowNew) : home(home)
{
    setWindowTitle(tr("Choose an Athlete"));
    setMinimumHeight(300);

    listWidget = new QListWidget(this);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    listWidget->setIconSize(QSize(64, 64));
    listWidget->setSpacing(0);
    listWidget->setContentsMargins(0,0,0,0);

    getList();

    if (allowNew)
        newButton = new QPushButton(tr("&New..."), this);
    okButton = new QPushButton(tr("&Open"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    deleteButton = new QPushButton(tr("Delete"), this);

    okButton->setEnabled(false);
    deleteButton->setEnabled(false);

    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    if (allowNew) connect(newButton, SIGNAL(clicked()), this, SLOT(newClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(enableOkDelete(QListWidgetItem*)));
    connect(listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(accept()));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    if (allowNew) buttonLayout->addWidget(newButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(listWidget);
    mainLayout->addLayout(buttonLayout);
}

void
ChooseCyclistDialog::getList()
{
    // clean current
    listWidget->clear();

    QStringListIterator i(home.entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {
        QString name = i.next();

        QListWidgetItem *newone = new QListWidgetItem(name, listWidget);

        // get avatar image if it exists
        QString iconpath = home.absolutePath() + "/" + name + "/config/avatar.png";
        if (QFile(iconpath).exists()) {
            QPixmap px(iconpath);
            newone->setIcon(QIcon(px.scaled(64,64)));
        }

        // taller less spacing
        newone->setSizeHint(QSize(newone->sizeHint().width(), 64));

        // only allow selection of cyclists which are not already open
        foreach (MainWindow *x, mainwindows) {
            QMapIterator<QString, Tab*> t(x->tabs);
            while (t.hasNext()) {
                t.next();
                if (t.key() == name)
                    newone->setFlags(newone->flags() & ~Qt::ItemIsEnabled);

            }
        }
    }
}

QString
ChooseCyclistDialog::choice()
{
    return listWidget->currentItem()->text();
}

void
ChooseCyclistDialog::enableOkDelete(QListWidgetItem *item)
{
    okButton->setEnabled(item != NULL);
    deleteButton->setEnabled(item != NULL);
}

void
ChooseCyclistDialog::cancelClicked()
{
    reject();
}

void
ChooseCyclistDialog::deleteClicked()
{
    // nothing selected
    if (listWidget->selectedItems().count() <= 0) return;

    QListWidgetItem *item = listWidget->selectedItems().first();

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Delete athlete"));
    msgBox.setText(tr("You are about to delete %1").arg(item->text()));
    msgBox.setInformativeText(tr("This cannot be undone and all data will be permanently deleted.\n\nAre you sure?"));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

    if(msgBox.clickedButton() == msgBox.button(QMessageBox::Ok)) {

        // ok .. lets wipe the athlete !
        QDir athleteDir = QDir(home.absolutePath() + "/" + item->text());

        // zap!
        recursiveDelete(athleteDir);
        home.rmdir(item->text());

        // list again, with athlete now gone
        getList();
    }
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
    if (!name.isEmpty()) {
        QListWidgetItem *newone = new QListWidgetItem(name, listWidget);

        // get avatar image if it exists
        QString iconpath = home.absolutePath() + "/" + name + "/config/avatar.png";
        if (QFile(iconpath).exists()) {
            QPixmap px(iconpath);
            newone->setIcon(QIcon(px.scaled(64,64)));
        }

        // taller less spacing
        newone->setSizeHint(QSize(newone->sizeHint().width(), 64));
    }
}

