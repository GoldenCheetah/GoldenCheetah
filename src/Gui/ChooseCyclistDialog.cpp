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
#include "NewAthleteWizard.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "Colors.h"
#include <QtGui>
#include <QMessageBox>

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

ChooseCyclistDialog::ChooseCyclistDialog(const QDir &home) : home(home)
{
    setWindowTitle(tr("Choose an Athlete"));
    setMinimumHeight(300 * dpiYFactor);
    setMinimumWidth(350 * dpiXFactor);

    listWidget = new QListWidget(this);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    listWidget->setIconSize(QSize(64 *dpiXFactor, 64 *dpiYFactor));
    listWidget->setSpacing(0);
    listWidget->setContentsMargins(0,0,0,0);

    getList();

    newButton = new QPushButton(tr("&New..."), this);
    deleteButton = new QPushButton(tr("Delete"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    okButton = new QPushButton(tr("&Open"), this);

    okButton->setEnabled(false);
    deleteButton->setEnabled(false);

    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(newButton, SIGNAL(clicked()), this, SLOT(newClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(listWidget, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(enableOkDelete(QListWidgetItem*)));
    connect(listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(accept()));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(newButton);
    buttonLayout->addWidget(deleteButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(listWidget);
    mainLayout->addLayout(buttonLayout);

    if (listWidget->count() == 0) {
        newClicked();
    }
}

void
ChooseCyclistDialog::getList()
{
    // clean current
    listWidget->clear();

    QStringListIterator i(home.entryList(QDir::Dirs | QDir::NoDotAndDotDot));
    while (i.hasNext()) {
        QString name = i.next();
        SKIP_QTWE_CACHE  // skip Folder Names created by QTWebEngine on Windows

        // ignore dot folders
        if (name.startsWith(".")) continue;

        QListWidgetItem *newone = new QListWidgetItem(name, listWidget);

        // get avatar image if it exists
        QString iconpath = home.absolutePath() + "/" + name + "/config/avatar.png";
        if (QFile(iconpath).exists()) {
            QPixmap px(iconpath);
            newone->setIcon(QIcon(px.scaled(64 *dpiXFactor,64 *dpiYFactor)));
        }

        // taller less spacing
        newone->setSizeHint(QSize(newone->sizeHint().width(), 64 *dpiYFactor));

        // only allow selection of cyclists which are not already open
        foreach (MainWindow *x, mainwindows) {
            QMapIterator<QString, AthleteTab*> t(x->athletetabs);
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
    okButton->setEnabled(item != nullptr);
    if (listWidget->count() > 0) {
        okButton->setDefault(item != nullptr);
    } else {
        newButton->setDefault(true);
    }
    deleteButton->setEnabled(item != nullptr);
}

void
ChooseCyclistDialog::cancelClicked()
{
    reject();
}

bool
ChooseCyclistDialog::deleteAthlete(QDir &homeDir, QString name, QWidget *parent)
{
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(tr("Delete athlete"));
    msgBox.setText(tr("You are about to delete %1").arg(name));
    msgBox.setInformativeText(tr("This cannot be undone and all data will be permanently deleted.\n\nAre you sure?"));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

    if(msgBox.clickedButton() == msgBox.button(QMessageBox::Ok)) {

        // ok .. lets wipe the athlete !
        QDir athleteDir = QDir(homeDir.absolutePath() + "/" + name);

        // zap!
        recursiveDelete(athleteDir);
        homeDir.rmdir(name);

        return true;
    }
    return false;
}

void
ChooseCyclistDialog::deleteClicked()
{
    // nothing selected
    if (listWidget->selectedItems().count() <= 0) return;

    QListWidgetItem *item = listWidget->selectedItems().first();

    if(deleteAthlete(home, item->text(), this)) {

        // list again, with athlete now gone
        getList();
    }
}

QString
ChooseCyclistDialog::newAthleteWizard(QDir &homeDir)
{
    NewAthleteWizard newAthleteWizard(homeDir);
    if (newAthleteWizard.exec() == QDialog::Accepted) {
        return newAthleteWizard.getName();
    } else {
        return "";
    }
}

void
ChooseCyclistDialog::newClicked()
{
    QString name = newAthleteWizard(home);
    if (!name.isEmpty()) {
        QListWidgetItem *newone = new QListWidgetItem(name, listWidget);

        // get avatar image if it exists
        QString iconpath = home.absolutePath() + "/" + name + "/config/avatar.png";
        if (QFile(iconpath).exists()) {
            QPixmap px(iconpath);
            newone->setIcon(QIcon(px.scaled(64 *dpiXFactor,64 *dpiYFactor)));
        }

        // taller less spacing
        newone->setSizeHint(QSize(newone->sizeHint().width(), 64 *dpiYFactor));
    }
}

