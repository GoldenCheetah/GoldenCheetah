/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#include <QProgressDialog>
#include <QMessageBox>
#if QT_VERSION > 0x050400
#include <QStorageInfo>
#endif

#include "Athlete.h"
#include "AthleteBackup.h"
#include "Settings.h"
#include "GcUpgrade.h"

#include "../qzip/zipwriter.h"
#include "../qzip/zipreader.h"



AthleteBackup::AthleteBackup(Context *context)
{

    this->context = context;
    cyclist = context->athlete->cyclist;

    // set the directories to be backed up
    // for a FULL backup basically all data folders
    sourceFolder.append(context->athlete->home->activities());
    sourceFolder.append(context->athlete->home->imports());
    sourceFolder.append(context->athlete->home->records());
    sourceFolder.append(context->athlete->home->downloads());
    sourceFolder.append(context->athlete->home->fileBackup());
    sourceFolder.append(context->athlete->home->config());
    sourceFolder.append(context->athlete->home->calendar());
    sourceFolder.append(context->athlete->home->workouts());

}

AthleteBackup::~AthleteBackup()
{

}

void
AthleteBackup::backupOnClose()
{
    int backupPeriod = appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_PERIOD, 0).toInt();
    QString backupFolder = appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_FOLDER, "").toString();
    if (backupPeriod == 0 || backupFolder == "" ) return;
    int backupCounter = appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_COUNTER, 0).toInt();
    backupCounter++;
    if (backupCounter < backupPeriod) {
        appsettings->setCValue(context->athlete->cyclist, GC_AUTOBACKUP_COUNTER, backupCounter);
        return;
    }

    // backup requested so lets see if we have something to backup and if yes, how much
    int fileCount = 0;
    qint64 fileSize = 0;
    // count the files for the progress bar and the calculate the overall size
    foreach (QDir folder, sourceFolder) {
        // get all files
        foreach (QFileInfo fileName, folder.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks)) {
           fileCount++;
           fileSize += fileName.size();

        }
    }

    if (fileCount == 0) {
       QMessageBox::information(NULL, tr("Auto Backup"), tr("No files found for athlete %1 - all athlete sub-directories are empty.").arg(cyclist));
       return;
    }

#if QT_VERSION > 0x050400
    // if if there is enough space available for the backup
    QStorageInfo storage(backupFolder);
    if (storage.isValid() && storage.isReady()) {
        // let's assume a 1:5 Zip compression to have enough space available for the ZIP
        if (storage.bytesAvailable() < fileSize / 5) {
            QMessageBox::warning(NULL, tr("Auto Backup"), tr("Not enough space available on disk: %1 - no backup .zip file created").arg(storage.rootPath()));
            return;
        }
    } else {
        QMessageBox::warning(NULL, tr("Auto Backup"), tr("Directory %1 not available. No backup .zip file created for athlete %2.").arg(backupFolder).arg(cyclist));
        return;
    }
#else
    QDir checkDir(backupFolder);
    if (!checkDir.exists()) {
        QMessageBox::warning(NULL, tr("Auto Backup"), tr("Directory %1 not available. No backup .zip file created for athlete %2.").arg(backupFolder).arg(cyclist));
        return;
    }
#endif

    QChar zero = QLatin1Char('0');
    QString targetFileName = QString( "GC_%1_%2_%3_%4_%5_%6_%7_%8.zip" )
                       .arg ( VERSION_LATEST )
                       .arg ( cyclist )
                       .arg ( QDate::currentDate().year(), 4, 10, zero )
                       .arg ( QDate::currentDate().month(), 2, 10, zero )
                       .arg ( QDate::currentDate().day(), 2, 10, zero )
                       .arg ( QTime::currentTime().hour(), 2, 10, zero )
                       .arg ( QTime::currentTime().minute(), 2, 10, zero )
                       .arg ( QTime::currentTime().second(), 2, 10, zero );


    // add files using zip writer
    QFile zipFile(backupFolder+"/"+targetFileName);
    if (!zipFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(NULL, tr("Auto Backup"), tr("Backup file %1 cannot be created.").arg(zipFile.fileName()));
        return;
    }
    zipFile.close();
    ZipWriter writer(zipFile.fileName());

    QProgressDialog progress(tr("Adding files to backup %1 for athlete %2 ...").arg(targetFileName).arg(cyclist), tr("Abort Backup and Reset Counter"), 0, fileCount, NULL);
    progress.setWindowModality(Qt::WindowModal);

    // now do the Zipping
    bool userCanceled = false;
    int fileCounter = 0;
    foreach (QDir folder, sourceFolder) {
        // get all files
        writer.addDirectory(folder.dirName());
        foreach (QFileInfo fileName, folder.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks)) {
            QFile file(fileName.canonicalFilePath());
            if (file.open(QIODevice::ReadOnly)) {
                if (progress.wasCanceled()) {
                    userCanceled = true;
                    break;
                }
                writer.addFile(folder.dirName()+"/"+fileName.fileName(), file.readAll());
                progress.setValue(fileCounter);
                fileCounter++;
                file.close();
            }
        }
        if (userCanceled) break;
    }

    // final processing
    writer.close();

    // delete the .ZIP file if the user canceled the backup
    if (userCanceled) {
        zipFile.remove();
    } else {
        // we are done, full progress and reset of counter
        progress.setValue(fileCount);
    };
    appsettings->setCValue(context->athlete->cyclist, GC_AUTOBACKUP_COUNTER, 0);

}



