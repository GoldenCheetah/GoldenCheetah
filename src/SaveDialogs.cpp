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
#include "MainWindow.h"
#include "Tab.h"
#include "Athlete.h"
#include "RideCache.h"
#include "GcRideFile.h"
#include "JsonRideFile.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideFileCommand.h"
#include "Settings.h"
#include "SaveDialogs.h"

//----------------------------------------------------------------------
// Utility functions to get and set WARN on CONVERT application setting
//----------------------------------------------------------------------
static bool
warnOnConvert()
{
    bool setting;

    QVariant warnsetting = appsettings->value(NULL, GC_WARNCONVERT);
    if (warnsetting.isNull()) setting = true;
    else setting = warnsetting.toBool();
    return setting;
}

void
setWarnOnConvert(bool setting)
{
    appsettings->setValue(GC_WARNCONVERT, setting);
}

static bool
warnExit()
{
    return appsettings->value(NULL, GC_WARNEXIT, true).toBool();
}

void
setWarnExit(bool setting)
{
    appsettings->setValue(GC_WARNEXIT, setting);
}

//----------------------------------------------------------------------
// User selected Save... menu option, prompt if conversion is needed
//----------------------------------------------------------------------
bool
MainWindow::saveRideSingleDialog(Context *context, RideItem *rideItem)
{
    if (rideItem->isDirty() == false) return false; // nothing to save you must be a ^S addict.

    // get file type
    QFile   currentFile(rideItem->path + QDir::separator() + rideItem->fileName);
    QFileInfo currentFI(currentFile);
    QString currentType =  currentFI.completeSuffix().toUpper();

    // either prompt etc, or just save that file away!
    if (currentType != "GC" && warnOnConvert() == true) {
        SaveSingleDialogWidget dialog(this, context, rideItem);
        dialog.exec();
        return true;
    } else {
        // go for it, the user doesn't want warnings!
        saveSilent(context, rideItem);
        return true;
    }
}

//----------------------------------------------------------------------
// Check if data needs saving on exit and prompt user for action
//----------------------------------------------------------------------
bool
MainWindow::saveRideExitDialog(Context *context)
{
    QList<RideItem*> dirtyList;

    // have we been told to not warn on exit?
    if (warnExit() == false) return true; // just close regardless!

    // get a list of rides to save
    foreach (RideItem *rideItem, context->athlete->rideCache->rides())
        if (rideItem->isDirty() == true) 
            dirtyList.append(rideItem);

    // we have some files to save...
    if (dirtyList.count() > 0) {
        SaveOnExitDialogWidget dialog(this, context, dirtyList);
        int result = dialog.exec();
        if (result == QDialog::Rejected) return false; // cancel that closeEvent!
    }

    // You can exit and close now
    return true;
}

//----------------------------------------------------------------------
// Silently save ride and convert to GC format without warning user
//----------------------------------------------------------------------
void
MainWindow::saveSilent(Context *context, RideItem *rideItem)
{
    QFile   currentFile(rideItem->path + QDir::separator() + rideItem->fileName);
    QFileInfo currentFI(currentFile);
    QString currentType =  currentFI.completeSuffix().toUpper();
    QFile   savedFile;
    bool    convert;

    // Do we need to convert the file type?
    if (currentType != "JSON") convert = true;
    else convert = false;

    // Has the date/time changed?
    QDateTime ridedatetime = rideItem->ride()->startTime();
    QChar zero = QLatin1Char ( '0' );
    QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                               .arg ( ridedatetime.date().year(), 4, 10, zero )
                               .arg ( ridedatetime.date().month(), 2, 10, zero )
                               .arg ( ridedatetime.date().day(), 2, 10, zero )
                               .arg ( ridedatetime.time().hour(), 2, 10, zero )
                               .arg ( ridedatetime.time().minute(), 2, 10, zero )
                               .arg ( ridedatetime.time().second(), 2, 10, zero );

    // if there is a notes file we need to rename it (cpi we will ignore)
    QFile notesFile(currentFI.canonicalPath() + QDir::separator() + currentFI.baseName() + ".notes");
    if (notesFile.exists()) notesFile.remove();

    // When datetime changes we need to update
    // the filename & rename/delete old file
    // we also need to preserve the notes file
    if (currentFI.baseName() != targetnosuffix) {

        // rename as backup current if converting, or just delete it if its already .gc
        // unlink previous .bak if it is already there
        if (convert) {
            QFile::remove(currentFile.fileName()+".bak"); // ignore errors if not there
            currentFile.rename(currentFile.fileName(), currentFile.fileName() + ".bak");
        } else currentFile.remove();
        convert = false; // we just did it already!

        // set the new filename & Start time everywhere
        currentFile.setFileName(rideItem->path + QDir::separator() + targetnosuffix + ".json");
        rideItem->setFileName(QFileInfo(currentFile).canonicalPath(), QFileInfo(currentFile).fileName());
    }

    // set target filename
    if (convert) {
        // rename the source
        savedFile.setFileName(currentFI.canonicalPath() + QDir::separator() + currentFI.baseName() + ".json");
    } else {
        savedFile.setFileName(currentFile.fileName());
    }

    // update the change history
    QString log = rideItem->ride()->getTag("Change History", "");
    log +=  tr("Changes on ");
    log +=  QDateTime::currentDateTime().toString() + ":";
    log += '\n' + rideItem->ride()->command->changeLog();
    rideItem->ride()->setTag("Change History", log);

    // save in GC format
    JsonFileReader reader;
    reader.writeRideFile(context, rideItem->ride(), savedFile);

    // rename the file and update the rideItem list to reflect the change
    if (convert) {

        // rename on disk
        QFile::remove(currentFile.fileName()+".bak"); // ignore errors if not there
        currentFile.rename(currentFile.fileName(), currentFile.fileName() + ".bak");

        // rename in memory
        rideItem->setFileName(QFileInfo(savedFile).canonicalPath(), QFileInfo(savedFile).fileName());
    }


    // mark clean as we have now saved the data
    rideItem->ride()->emitSaved();
}

//----------------------------------------------------------------------
// Save Single File Dialog Widget
//----------------------------------------------------------------------
SaveSingleDialogWidget::SaveSingleDialogWidget(MainWindow *mainWindow, Context *context, RideItem *rideItem) :
    QDialog(mainWindow, Qt::Dialog), mainWindow(mainWindow), context(context), rideItem(rideItem)
{
    setWindowTitle(tr("Save and Conversion"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Warning text
    warnText = new QLabel(tr("WARNING\n\nYou have made changes to ") + rideItem->fileName + tr(" If you want to save\nthem, we need to convert the ride to GoldenCheetah\'s\nnative format. Should we do so?\n"));
    mainLayout->addWidget(warnText);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    saveButton = new QPushButton(tr("&Save and Convert"), this);
    buttonLayout->addWidget(saveButton);
    abandonButton = new QPushButton(tr("&Discard Changes"), this);
    buttonLayout->addWidget(abandonButton);
    cancelButton = new QPushButton(tr("&Cancel Save"), this);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    // Don't warn me!
    warnCheckBox = new QCheckBox(tr("Always warn me about file conversions"), this);
    warnCheckBox->setChecked(true);
    mainLayout->addWidget(warnCheckBox);

    // connect up slots
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveClicked()));
    connect(abandonButton, SIGNAL(clicked()), this, SLOT(abandonClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(warnCheckBox, SIGNAL(clicked()), this, SLOT(warnSettingClicked()));
}

void
SaveSingleDialogWidget::saveClicked()
{
    mainWindow->saveSilent(context, rideItem);
    accept();
}

void
SaveSingleDialogWidget::abandonClicked()
{
    rideItem->setDirty(false); // lose changes
    reject();
}

void
SaveSingleDialogWidget::cancelClicked()
{
    reject();
}

void
SaveSingleDialogWidget::warnSettingClicked()
{
    setWarnOnConvert(warnCheckBox->isChecked());
}

//----------------------------------------------------------------------
// Save on Exit File Dialog Widget
//----------------------------------------------------------------------

SaveOnExitDialogWidget::SaveOnExitDialogWidget(MainWindow *mainWindow, Context *context, QList<RideItem *>dirtyList) :
    QDialog(mainWindow, Qt::Dialog), mainWindow(mainWindow), context(context), dirtyList(dirtyList)
{
    setWindowTitle("Save Changes");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Warning text
    warnText = new QLabel(tr("WARNING\n\nYou have made changes to some rides which\nhave not been saved. They are listed below."));
    mainLayout->addWidget(warnText);

    // File List
    dirtyFiles = new QTableWidget(dirtyList.count(), 0, this);
    dirtyFiles->setColumnCount(2);
    dirtyFiles->horizontalHeader()->hide();
    dirtyFiles->verticalHeader()->hide();

    // Populate with dirty List
    for (int i=0; i<dirtyList.count(); i++) {
        // checkbox
        QCheckBox *c = new QCheckBox;
        c->setCheckState(Qt::Checked);
        dirtyFiles->setCellWidget(i,0,c);

        // filename
        QTableWidgetItem *t = new QTableWidgetItem;
        t->setText(dirtyList.at(i)->fileName);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        dirtyFiles->setItem(i,1,t);
    }

    // prettify the list
    dirtyFiles->setShowGrid(false);
    dirtyFiles->resizeColumnToContents(0);
    dirtyFiles->resizeColumnToContents(1);
    mainLayout->addWidget(dirtyFiles);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    saveButton = new QPushButton(tr("&Save and Exit"), this);
    buttonLayout->addWidget(saveButton);
    abandonButton = new QPushButton(tr("&Discard and Exit"), this);
    buttonLayout->addWidget(abandonButton);
    cancelButton = new QPushButton(tr("&Cancel Exit"), this);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    // Don't warn me!
    exitWarnCheckBox = new QCheckBox(tr("Always check for unsaved changes on exit"), this);
    exitWarnCheckBox->setChecked(true);
    mainLayout->addWidget(exitWarnCheckBox);

    // connect up slots
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveClicked()));
    connect(abandonButton, SIGNAL(clicked()), this, SLOT(abandonClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(exitWarnCheckBox, SIGNAL(clicked()), this, SLOT(warnSettingClicked()));
}

void
SaveOnExitDialogWidget::saveClicked()
{
    // whizz through the list and save one by one using
    // singleSave to ensure warnings are given if neccessary
    for (int i=0; i<dirtyList.count(); i++) {
        QCheckBox *c = (QCheckBox *)dirtyFiles->cellWidget(i,0);
        if (c->isChecked()) {
            mainWindow->saveRideSingleDialog(context, dirtyList.at(i));
        }
    }
    accept();
}

void
SaveOnExitDialogWidget::abandonClicked()
{
    accept();
}

void
SaveOnExitDialogWidget::cancelClicked()
{
    reject();
}

void
SaveOnExitDialogWidget::warnSettingClicked()
{
    setWarnExit(exitWarnCheckBox->isChecked());
}
