/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#include "GoldenCheetah.h"
#include "Settings.h"
#include "Colors.h"
#include "GcUpgrade.h"
#include "Athlete.h"
#include "VideoWindow.h"
#include "ErgFile.h"
#include "RideFile.h"
#include "JsonRideFile.h"
#include "Context.h"
#include "DataProcessor.h"
#include "MainWindow.h"
#include "TrainDB.h"
#include "CloudService.h"

#include <QDebug>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>

bool
GcUpgrade::upgradeConfirmedByUser(const QDir &home)
{

    // since the upgrade can fail / multiple times, don't rely on the "lastUpdated" version,
    // but track the "upgrade success" separately in the settings on user level
    bool folderUpgradeSuccess =  appsettings->cvalue(home.dirName(), GC_UPGRADE_FOLDER_SUCCESS, false).toBool();

    //  reset upgrade flag - in case flag is set "true" and subfolder "/activities" and /config do NOT exist
    //  or - if they exists, but config does not contain any files, then something is wrong with the upgrade flag -
    //  (potentially due to a not fitting data restore)
    //  so let's reset the flag to "false" and run the upgrade again - this can never go wrong (if their is
    //  nothing to be upgrade - upgrade a success anyway

    AthleteDirectoryStructure newHome(home);
    if (folderUpgradeSuccess && !newHome.upgradedDirectoriesHaveData()) {
       folderUpgradeSuccess = false;
       appsettings->setCValue(home.dirName(), GC_UPGRADE_FOLDER_SUCCESS, false);
    }

    if (!folderUpgradeSuccess) {

        GcUpgradeExecuteDialog msgBox(home);
        if (msgBox.exec() == QDialog::Accepted) return true;

        // if not accepted
        return false;

    }

    return true; // if there is no upgrade needed, just proceed
}

int
GcUpgrade::upgrade(const QDir &home)
{

    // what was the last version? -- do we need to upgrade?
    int last = appsettings->cvalue(home.dirName(), GC_VERSION_USED, 0).toInt();

    // Upgrade processing was introduced in Version 3 -- below must be performed
    // for athlete directories from prior to Version 3
    // and can essentially be used as a template for all major release
    // upgrades as it deletes old stuff and sets clean

    //----------------------------------------------------------------------
    // 3.0 upgrade processing
    //----------------------------------------------------------------------
    if (!last || last < VERSION3_BUILD) {

        // For now we always do the same thing
        // when we have some maturity with versions we may
        // choose to do different things
        if (last < VERSION3_BUILD) {

            // 1. Delete old files
            QStringList oldfiles;
            oldfiles << "*.cpi";
            oldfiles << "*.bak";
            foreach (QString oldfile, home.entryList(oldfiles, QDir::Files)) {
                QFile old(QString("%1/%2").arg(home.canonicalPath()).arg(oldfile));
                old.remove();

            }

            // 2. Remove old CLucece 'index'
            QFile index(QString("%1/index").arg(home.canonicalPath()));
            if (index.exists()) {
                removeIndex(index);
            }

            // 3. Remove metricDBv3 - force rebuild including the search index
            QFile db(QString("%1/metricDBv3").arg(home.canonicalPath()));
            if (db.exists()) db.remove();

            // 4. Set default weight to 75kg if currently zero
            double weight_ = appsettings->cvalue(home.dirName(), GC_WEIGHT, "75.0").toString().toDouble();
            if (weight_ <= 0.00) appsettings->setCValue(home.dirName(), GC_WEIGHT, "75.0");

            // 5. startup with common sidebars shown (less ugly)
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/LTM/hide"), true);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/LTM/hide/0"), false);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/LTM/hide/1"), false);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/LTM/hide/2"), false);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/LTM/hide/3"), true);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/analysis/hide"), true);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/analysis/hide/0"), false);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/analysis/hide/1"), true);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/analysis/hide/2"), false);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/analysis/hide/3"), true);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/diary/hide"), true);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/diary/hide/0"), false);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/diary/hide/1"), false);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/diary/hide/2"), true);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/train/hide"), true);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/train/hide/0"), false);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/train/hide/1"), false);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/train/hide/2"), false);
            appsettings->setCValue(home.dirName(), GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/train/hide/3"), false);

            // 6. Delete any old measures.xml -- its for withings only
            QFile msxml(QString("%1/measures.xml").arg(home.canonicalPath()));
            if (msxml.exists()) msxml.remove();


        }
    }

    // Versions after 3 should add their upgrade processing at this point
    // DO NOT CHANGE THE VERSION 3 UPGRADE PROCESS ABOVE, ADD TO IT BELOW

    //----------------------------------------------------------------------
    // 3.0 SP2 upgrade processing
    //----------------------------------------------------------------------
    if (last < VERSION3_SP2) {

        // 2. Remove old CLucece 'index'
        QFile index(QString("%1/index").arg(home.canonicalPath()));
        if (index.exists()) {
            removeIndex(index);
        }

        // 3. Remove metricDBv3 - force rebuild including the search index
        QFile db(QString("%1/metricDBv3").arg(home.canonicalPath()));
        if (db.exists()) db.remove();

    }

    //----------------------------------------------------------------------
    // 3.1 upgrade processing
    //----------------------------------------------------------------------

    if (last < VERSION31_BUILD) {

        // We sought to reset the user defaults in v3.1 to
        // move away from the ugly default used since GC first
        // released. This is the first time we actively applied
        // a new theme and color setting for users.

        // For a full breakdown of all activities applied in VERSION 3.1
        // they are listed in detail on the associated gitub issue:
        // see https://github.com/GoldenCheetah/GoldenCheetah/issues/883

        // 1. Delete all backup, CPX, Metrics and Lucene Index
        QStringList oldfiles;
        oldfiles << "*.cpi";
        oldfiles << "*.bak";
        foreach (QString oldfile, home.entryList(oldfiles, QDir::Files)) {
            QFile old(QString("%1/%2").arg(home.canonicalPath()).arg(oldfile));
            old.remove();
        }

        QFile index(QString("%1/index").arg(home.canonicalPath()));
        if (index.exists()) {
            removeIndex(index);
        }

        QFile db(QString("%1/metricDBv3").arg(home.canonicalPath()));
        if (db.exists()) db.remove();


        // 2. Remove any old charts.xml (it will be WRONG!)
        QFile charts(QString("%1/charts.xml").arg(home.canonicalPath()));
        if (charts.exists()) charts.remove();

        // 3. Reset colour defaults **
        GCColor::applyTheme(0); // set to default theme

        // 4. Theme and Chrome Color
        QString theme = "Flat";
        QColor chromeColor = QColor(0xec,0xec,0xec);
#ifdef Q_OS_MAC
        // Yosemite or earlier
        if (QSysInfo::MacintoshVersion >= 12) {

            chromeColor = QColor(235,235,235);
        } else {

            // prior to Yosemite .. metallic
            theme = "Mac";
            chromeColor = QColor(215,215,215);
        }
#endif
        QString colorstring = QString("%1:%2:%3").arg(chromeColor.red())
                                                 .arg(chromeColor.green())
                                                 .arg(chromeColor.blue());
        appsettings->setValue("CCHROME", colorstring);
        GCColor::setColor(CCHROME, chromeColor);

        // 5. Metrics and Notes keywords
        QString filename = home.canonicalPath()+"/metadata.xml";
        if (QFile(filename).exists()) {

            QList<KeywordDefinition> keywordDefinitions;
            QList<FieldDefinition>   fieldDefinitions;
            QString colorfield;
            QList<DefaultDefinition> defaultDefinitions;

            // read em in
            RideMetadata::readXML(filename, keywordDefinitions, fieldDefinitions, colorfield, defaultDefinitions);

            bool updated=false;

            //
            // ADD METRICS TO METADATA TAB
            //
            int pos = -1;
            int indexTSS=-1, indexAnTISS=-1, indexAeTISS=-1;
            for(int i=0; i < fieldDefinitions.count(); i++) {

                // current ...
                FieldDefinition f = fieldDefinitions[i];

                if (f.tab == tr("Metric") && pos < 0) pos = i;
                if (f.name == "TSS") indexTSS=i;
                if (f.name == tr("Aerobic TISS")) indexAeTISS=i;
                if (f.name == tr("Anaerobic TISS")) indexAnTISS=i;
            }

            // ok, we need to add them to the metadata
            if (indexTSS < 0 || indexAnTISS < 0 || indexAeTISS < 0) {

                // lets add all at the same place
                if (indexTSS >= 0) pos = indexTSS;
                else if (indexAnTISS >= 0) pos = indexAnTISS;
                else if (indexAeTISS >= 0) pos = indexAeTISS;

                // ok, one at a time, using this as a template
                FieldDefinition add;
                add.tab = pos >= 0 ? fieldDefinitions[pos].tab : tr("Metric");
                add.diary = false;
                add.type = 4; // double

                // now set pos to non-negative if needed
                if (pos < 0) pos = 1;

                // add them
                if (indexAnTISS < 0) {
                    add.name = tr("Anaerobic TISS");
                    fieldDefinitions.insert(pos, add);
                }
                if (indexAeTISS < 0) {
                    add.name = tr("Aerobic TISS");
                    fieldDefinitions.insert(pos, add);
                }
                if (indexTSS < 0) {
                    add.name = tr("TSS");
                    fieldDefinitions.insert(pos, add);
                }
                updated = true;
            }

            //
            // DEPRECATE 'default' color keyword and if needed
            // ADD 'Reverse' color keyword
            //
            int defaultIndex = -1, reverseIndex = -1;
            for(int i=0; i<keywordDefinitions.count(); i++) {
                if (keywordDefinitions[i].name == "Default") defaultIndex = i;
                if (keywordDefinitions[i].name == "Reverse") reverseIndex = i;
            }

            // no more default
            if (defaultIndex >= 0) {
                updated = true;
                keywordDefinitions.removeAt(defaultIndex);
            }

            // no reverse ?
            if (reverseIndex < 0) {
                updated = true;
                KeywordDefinition add;
                add.name = "Reverse";
                add.color = QColor(Qt::black);
                keywordDefinitions << add;
            }

            if (updated) {
                // write a new updated version
                RideMetadata::serialize(filename, keywordDefinitions, fieldDefinitions, colorfield, defaultDefinitions);
            }
        }

        // ** NOTE:
        // ** Suggestions to update CP/W'/Zones have been ignored due to the
        // ** high risk of breaking user setups -- this is due to the complexity
        // ** and multiple ways the user can manage their zones.

        // BELOW ARE PROBLEMATIC TOO:
        // ** there are no functions to read/write the layout.xml
        // ** files without refactoring HomeWindow to do so -- which
        // ** is a risky change and instead we will need the user
        // ** to reset their layout to get the latest chart setup:
        // Add a W'bal chart to the ride view
        // Add a CP History chart to the trend view
        // Add a Library chart to the trend view

        // PM deprecation has been handled by returning an LTM chart with
        // PMC curves when an PM chart is still in the layout.


    }

    //----------------------------------------------------------------------
    // 3.2 (formerly 3.11) upgrade processing
    //----------------------------------------------------------------------

    if (last < VERSION311_BUILD) {

        // add here the standard upgrade tasks

    }


    //----------------------------------------------------------------------
    // 3.3 upgrade processing
    //----------------------------------------------------------------------

    if (last < VERSION33_BUILD) {

        trainDB->upgradeDefaultEntriesWorkout();
    }


    //----------------------------------------------------------------------
    // 3.5 upgrade processing
    //----------------------------------------------------------------------
    if (last < VERSION35_BUILD) {

        // metallic style deprecated
        appsettings->setValue(GC_CHROME, "Flat");

        // set the default scale factor to 1.0, if not already done
        if (appsettings->value(NULL, GC_FONT_SCALE, "0").toDouble() == 0)
            appsettings->setValue(GC_FONT_SCALE, QVariant::fromValue(1.0f));

        // cloud services now need a setting to say if active or not
        CloudServiceFactory::instance().upgrade(home.dirName());
    }

    //----------------------------------------------------------------------
    // 3.6 upgrade processing
    //----------------------------------------------------------------------
    if (last < VERSION36_BUILD) {

        // reset themes on basis of plot background (first 2 themes are default dark and light themes
        if (GCColor::luminance(GColor(CPLOTBACKGROUND)) < 127)  GCColor::applyTheme(0);
        else GCColor::applyTheme(1);

    }

    //----------------------------------------------------------------------
    // ... here any further Release Number dependent Upgrade Process is to be added ...
    //----------------------------------------------------------------------



    //----------------------------------------------------------------------
    // All Version dependent Upgrade Steps are done ...
    //----------------------------------------------------------------------

    // FINALLY -- Set latest version - so only tries to upgrade once
    appsettings->setCValue(home.dirName(), GC_VERSION_USED, VERSION_LATEST);

    //----------------------------------------------------------------------
    // 3.2 new subfolder introduction and upgrade processing
    //----------------------------------------------------------------------


    // now the special "folder structure" upgrade - which is tracked separately on success

    bool folderUpgradeSuccess =  appsettings->cvalue(home.dirName(), GC_UPGRADE_FOLDER_SUCCESS, false).toBool();

    // now let's check if upgrade is necessary and do the job
    if (!folderUpgradeSuccess) {

        // initials logs,...
        errorCount = 0;
        upgradeLog = new GcUpgradeLogDialog(home);
        upgradeLog->show();

        // 1. create the new subDirs structure
        upgradeLog->append(tr("Start creating of: Directories... "),3);

        AthleteDirectoryStructure newHome(home);
        newHome.createAllSubdirs();
        //now we can created the QSettings for the Athlete and Migrate the oldseetings
        appsettings->initializeQSettingsAthlete(gcroot, home.dirName());

        if (!newHome.subDirsExist()) {
           upgradeLog->append(QString(tr("Error: Creation of subdirectories failed")),2);
           errorCount++;
        } else {
           upgradeLog->append(QString(tr("Creation of subdirectories successful")),2);
        }

        // 2. Delete all backup, CPX, Metrics and Lucene Index
        QStringList oldfiles;
        oldfiles << "*.cpi";
        oldfiles << "*.cpx";
        foreach (QString oldfile, home.entryList(oldfiles, QDir::Files)) {
            QFile old(QString("%1/%2").arg(home.canonicalPath()).arg(oldfile));
            old.remove();
        }

        QFile index(QString("%1/index").arg(home.canonicalPath()));
        if (index.exists()) {
            removeIndex(index);
        }

        QFile db(QString("%1/metricDBv3").arg(home.canonicalPath()));
        if (db.exists()) db.remove();

        //3. move the existing files to the new SubDirs
        //3.1 move config

        QStringList configFiles;
        configFiles << "*.xml";
        configFiles << "*.zones";
        configFiles << "avatar.*";
        upgradeLog->append(tr("Start copying of: Configuration files... "),3);

        int ok = 0; int fail = 0;
        foreach (QString configFile, newHome.root().entryList(configFiles, QDir::Files)) {
            bool success = moveFile(QString("%1/%2").arg(newHome.root().canonicalPath()).arg(configFile),
                         QString("%1/%2").arg(newHome.config().canonicalPath()).arg(configFile));
            if (success) {
                ok++;
            } else {
                fail++;
             }
        }
        errorCount += fail;
        upgradeLog->append(QString(tr("%1 configuration files moved to subdirectory: %2 - %3 failed" ))
                  .arg(QString::number(ok)).arg(newHome.config().dirName()).arg(QString::number(fail)),2);

        // 3.2 move the calendar
        QStringList calFiles;
        calFiles << "*.ics";
        upgradeLog->append(tr("Start copying of: Calendar files..."),3);

        ok = 0; fail = 0;
        foreach (QString calFile, newHome.root().entryList(calFiles, QDir::Files)) {
            bool success = moveFile(QString("%1/%2").arg(newHome.root().canonicalPath()).arg(calFile),
                     QString("%1/%2").arg(newHome.calendar().canonicalPath()).arg(calFile));
            if (success) {
                ok++;
            } else {
                fail++;
            }
        }
        errorCount += fail;
        upgradeLog->append(QString((tr("%1 calendar files moved to subdirectory: %2 - %3 failed"))
                  .arg(QString::number(ok)).arg(newHome.calendar().dirName()).arg(QString::number(fail))),2);

        // 3.3 move the logs
        QStringList logFiles;
        logFiles << "*.log";
        upgradeLog->append(tr("Start copying of: Log files..."),3);
        ok = 0; fail = 0;
        foreach (QString logFile, newHome.root().entryList(logFiles, QDir::Files)) {
            bool success = moveFile(QString("%1/%2").arg(newHome.root().canonicalPath()).arg(logFile),
                     QString("%1/%2").arg(newHome.logs().canonicalPath()).arg(logFile));
            if (success) {
                ok++;
            } else {
                fail++;

            }
        }
        errorCount += fail;
        upgradeLog->append(QString((tr("%1 log files moved to subdirectory: %2 - %3 failed"))
                  .arg(QString::number(ok)).arg(newHome.logs().dirName()).arg(QString::number(fail))),2);


        // 3.4 move the .JSON and .GC first
        QStringList jsonFiles;
        jsonFiles << "*.json";
        jsonFiles << "*.gc";
        upgradeLog->append(tr("Start copying of: Activity files (.JSON / .GC)..."),3);
        ok = 0; fail = 0;
        foreach (QString jsonFile, newHome.root().entryList(jsonFiles, QDir::Files)) {
            bool success = moveFile(QString("%1/%2").arg(newHome.root().canonicalPath()).arg(jsonFile),
                     QString("%1/%2").arg(newHome.activities().canonicalPath()).arg(jsonFile));
            if (success) {
                ok++;
            } else {
                fail++;
            }
        }
        errorCount += fail;
        upgradeLog->append(QString(tr("%1 activity (.JSON, .GC) files moved to subdirectory: %2 - %3 failed" ))
                  .arg(QString::number(ok)).arg(newHome.activities().dirName()).arg(QString::number(fail)),2);


        // 3.6 keep the .BAK files and store in /fileBackup()
        QStringList bakFiles;
        bakFiles << "*.bak";
        upgradeLog->append(tr("Start copying of: Activity files (.BAK)..."),3);
        ok = 0; fail = 0;
        foreach (QString bakFile, newHome.root().entryList(bakFiles, QDir::Files)) {
            bool success = moveFile(QString("%1/%2").arg(newHome.root().canonicalPath()).arg(bakFile),
                     QString("%1/%2").arg(newHome.fileBackup().canonicalPath()).arg(bakFile));
            if (success) {
                ok++;
            } else {
                fail++;
            }
        }
        errorCount += fail;
        upgradeLog->append(QString(tr("%1 activity backup (.BAK) files moved to subdirectory: %2 - %3 failed" ))
                  .arg(QString::number(ok)).arg(newHome.fileBackup().dirName()).arg(QString::number(fail)),2);

        // 3.6 now sort the rest of the files (extension checks are re-use)
        MediaHelper mediaFile;
        ok = 0; fail = 0;
        upgradeLog->append(tr("Start copying of: Media and Workout files... "),3);
        foreach (QString otherFile, newHome.root().entryList(QDir::Files)) {
            // check for workout and media files
            if (mediaFile.isMedia(otherFile) || ErgFile::isWorkout(otherFile)) {
                QDir r;
                bool success = moveFile(QString("%1/%2").arg(newHome.root().canonicalPath()).arg(otherFile),
                         QString("%1/%2").arg(newHome.workouts().canonicalPath()).arg(otherFile));
                if (success) {
                    ok++;
                } else {
                    fail++;
                }
            }
        }
        errorCount += fail;
        upgradeLog->append(QString(tr("%1 media and workout files moved to subdirectory: \\%2 - %3 failed"))
                  .arg(QString::number(ok)).arg(newHome.workouts().dirName()).arg(QString::number(fail)),2);

        // the conversion of all activities to .json is done in "lateUpgrade" - since the prerequisites
        // on the "context" setup are not fulfilled at this early stage


    }

    // other 3.2 upgrade tasks, mostly cosmetic
    if (last < VERSION32_BUILD) {

        // trend plot matches ride plot, as newly introduced
        // just do for first time we run 3.2 and set to ride plot
        QColor color = GCColor::getColor(CRIDEPLOTBACKGROUND);
        GCColor::setColor(CTRENDPLOTBACKGROUND, color);

        // and update config
        QString colorstring = QString("%1:%2:%3").arg(color.red())
                                                 .arg(color.green())
                                                 .arg(color.blue());
        appsettings->setValue("COLORTRENDPLOTBACKGROUND", colorstring);

        // and on non-Mac platforms we want flat look and feel
        // by default now, the metal look is de-rigeur
#ifndef Q_OS_MAC
        color = QColor(0xe5,0xe5,0xe5);
        colorstring = QString("%1:%2:%3").arg(color.red())
                                         .arg(color.green())
                                         .arg(color.blue());
        appsettings->setValue("CCHROME", colorstring);
        appsettings->setValue(GC_CHROME, "Flat");
#endif

    }
    return 0;
}


int
GcUpgrade::upgradeLate(Context *context)
{
    // check the special "folder structure" upgrade - which is tracked separately on success
    bool folderUpgradeSuccess =  appsettings->cvalue(context->athlete->home->root().dirName(), GC_UPGRADE_FOLDER_SUCCESS, false).toBool();
    if (!folderUpgradeSuccess) {

        // switch off automatic Fix tools (
        DataProcessorFactory::instance().setAutoProcessRule(false);

        // 1. convert the rest of activities to .json and move the converted ones to /imports
        // prepare the suffixes for check of the files
        QStringList suffixList = RideFileFactory::instance().suffixes();
        QRegExp suffixes(QString("^(%1)$").arg(suffixList.join("|")));
        suffixes.setCaseSensitivity(Qt::CaseInsensitive);

        int ok = 0; int fail = 0; int okConvert = 0; int failConvert = 0;
        upgradeLog->append(tr("Start conversion of native activity files to GoldenCheetah .JSON format..."),3 );

        // GC file Name format
        QRegExp rx ("^((\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d)_(\\d\\d))\\.(.+)$");
        bool fileNameValid;

        foreach (QString activitiesFileName, context->athlete->home->root().entryList(QDir::Files)) {

            QString fullFileName = context->athlete->home->root().canonicalPath() + "/" + activitiesFileName;
            QFileInfo fullFileInfo(fullFileName);

            if (fullFileInfo.exists() && fullFileInfo.isFile() && fullFileInfo.isReadable()) {

                if (suffixes.exactMatch(fullFileInfo.suffix()))   {
                    // We have an activity file which is NOT JSON or GC (since they have been moved already) - let's convert
                    QStringList errors;
                    QFile currentFile(fullFileName);

                    // Check if the filename is formattted according the GC filename format (since this format is used to determine
                    // the ride start date and time
                    fileNameValid = true;
                    if (!rx.exactMatch(fullFileInfo.fileName())) {
                        fileNameValid = false;
                    } else {
                        // format is fine, check if the file name really contains a valid date/time info
                        QDate date(rx.cap(2).toInt(), rx.cap(3).toInt(),rx.cap(4).toInt());
                        QTime time(rx.cap(5).toInt(), rx.cap(6).toInt(),rx.cap(7).toInt());

                        if (!(date.isValid() && time.isValid())) {
                            fileNameValid = false;
                        }
                    }

                    // only process if the file name contains a valid date/time - otherwise user needs to check
                    if (fileNameValid) {

                        RideFile *ride = RideFileFactory::instance().openRideFile(context, currentFile, errors);

                        // did it parse ok ? (all files here were alrady parsed when importing)
                        if (ride) {

                            // serialize
                            QString targetFileName = activitiesFileName;
                            int dot = targetFileName.lastIndexOf(".");
                            assert(dot >= 0);
                            targetFileName.truncate(dot);
                            targetFileName.append(".json");
                            // add Source File Tag + New File Name
                            ride->setTag("Source Filename", activitiesFileName);
                            ride->setTag("Filename", targetFileName);
                            JsonFileReader reader;
                            QFile target(context->athlete->home->activities().canonicalPath() + "/" + targetFileName);
                            reader.writeRideFile(context, ride, target);
                            okConvert++;
                            upgradeLog->append(tr("-> Information: Activity %1 - Successfully converted to .JSON").arg(activitiesFileName));

                            // copy source file to the /imports folder (only if conversion was successful)
                            bool success = moveFile(QString("%1/%2").arg(context->athlete->home->root().canonicalPath()).arg(activitiesFileName),
                                                    QString("%1/%2").arg(context->athlete->home->imports().canonicalPath()).arg(activitiesFileName));
                            if (success) {
                                ok++;
                            } else {
                                fail++;
                            }
                        } else {
                            failConvert++;
                            upgradeLog->append(tr("-> Error: Activity %1 - Conversion errors: ").arg(activitiesFileName),2);
                            foreach (QString error, errors) {
                                upgradeLog->append(tr("......... message(s) of .JSON conversion): ") + error);
                                upgradeLog->append("<br>");
                            }
                        }

                        // clear
                        delete ride;

                    } else {
                        failConvert++;
                        upgradeLog->append(tr("-> Error: Activity %1 - Invalid File Name (expected format 'YYYY_MM_DD_HH_MM_SS.%2')").arg(activitiesFileName).arg(fullFileInfo.suffix()),2);
                    }
                }

            } else {
                failConvert++;
                upgradeLog->append(tr("-> Error: Activity %1 - Problem reading file").arg(activitiesFileName),2);
            }
        }

        errorCount+=fail;
        errorCount+=failConvert;

        upgradeLog->append(QString(tr("%1 activity files converted to .JSON and stored in subdirectory: %2 - %3 failed" ))
                  .arg(QString::number(okConvert)).arg(context->athlete->home->activities().dirName()).arg(QString::number(failConvert)),2);

        upgradeLog->append(QString(tr("%1 converted activity source files moved to subdirectory: %2 - %3 failed" ))
                  .arg(QString::number(ok)).arg(context->athlete->home->imports().dirName()).arg(QString::number(fail)),2);

        // Total Number of errors
        if (errorCount == 0) {
           upgradeLog->append(QString(tr("Summary: No errors detected - upgrade successful" )),3);
           appsettings->setCValue(context->athlete->home->root().dirName(), GC_UPGRADE_FOLDER_SUCCESS, true);
        } else {
            upgradeLog->append(QString(tr("Summary: %1 errors detected - please check log details before proceeding" ))
                  .arg(QString::number(errorCount)),3);

            upgradeLog->append(QString(tr("<center><br>After choosing 'Proceed to Athlete', the system will open the athlete window using the "
                                          "converted data. Depending on the errors this might lead to follow-up errors and incomplete "
                                          "athlete data. You may either fix the error(s) in your directory directly, or go back "
                                          "to your last backup and correct the error(s) in the source data. <br>The upgrade process "
                                          "will be done again each time you open the athlete, until the conversion was "
                                          "successful - and had no more errors.</center>")),2);

            upgradeLog->append(QString(tr("<center><br>Latest information about possible upgrade problems and concepts to resolve them are available in the<br>"
                                         "<a href= \"https://github.com/GoldenCheetah/GoldenCheetah/wiki/Upgrade_v3.2_Troubleshooting_Guide\" target=\"_blank\">"
                                         "Upgrade v3.2 Troubleshooting Guide<a>")),1);

            // document that upgrade failed at least one time
            appsettings->setCValue(context->athlete->home->root().dirName(), GC_UPGRADE_FOLDER_SUCCESS, false);

        }

        // switch automatic Fix tools on again
        DataProcessorFactory::instance().setAutoProcessRule(true);

        // show upgrade log
        upgradeLog->enableButtons();
        upgradeLog->exec();

        // user can only select "Accept" to end with the upgrade step
        return 0;

    }

    return 0;
}

bool
GcUpgrade::moveFile(const QString &source, const QString &target) {

    QFile r(source);

    // first try it with a rename
    if (r.rename(target)) return true; // job is done

    // now the harder variant (copy & delete)
    if (r.copy(target))
    {
        // try to remove - but if this fails, no problem, file has been copied at least
        if (!r.remove()) {
            // log, even though it's not very critical it needs to be cleaned up - so report as error
            upgradeLog->append(QString(tr("-> Error: Deletion of copied file '%1' failed" )).arg(source),2);
            return false;
        } else {
            // copy & remove worked fine - we are done
            return true;
        }
    }

    // copying failed - so give up and report
    upgradeLog->append((tr("-> Error moving file : ") + source),2);
    return false;

}


class FileUtil
{
    public:
        static bool removeDir(const QString &dirName) {

            bool result = true;
            QDir dir(dirName);

            if (dir.exists(dirName)) {
                foreach(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
                    if (info.isDir()) {
                        result = FileUtil::removeDir(info.absoluteFilePath());

                    } else {

                        result = QFile::remove(info.absoluteFilePath());
                    }

                    if (!result) { return result; }

                }
                result = dir.rmdir(dirName);
            }

        return result;
    }

};


void
GcUpgrade::removeIndex(QFile &index)
{
    FileUtil::removeDir(index.fileName());
}


GcUpgradeExecuteDialog::GcUpgradeExecuteDialog(QDir athleteHomeDir) : QDialog(NULL, Qt::Dialog)
{

    const QString athlete = athleteHomeDir.dirName();

    setWindowTitle(QString(tr("Athlete %1").arg(athlete)));
    this->setMinimumWidth(550*dpiXFactor);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *toprow = new QHBoxLayout;

    QPushButton *critical = new QPushButton(style()->standardIcon(QStyle::SP_MessageBoxWarning), "", this);
    critical->setFixedSize(80*dpiXFactor,80*dpiYFactor);
    critical->setFlat(true);
    critical->setIconSize(QSize(80*dpiXFactor,80*dpiYFactor));
    critical->setAutoFillBackground(false);
    critical->setFocusPolicy(Qt::NoFocus);

    QLabel *header = new QLabel(this);
    header->setWordWrap(true);
    header->setTextFormat(Qt::RichText);
    header->setText(QString(tr("<center><h2>Upgrade of Athlete:<br>%1<br></h2></center>")).arg(athlete));

    QLabel *text = new QLabel(this);
    text->setWordWrap(true);
    text->setTextFormat(Qt::RichText);
    text->setText(tr("<center><b>Backup your 'Athlete' data first!<br>"
                     "<b>Please read carefully before proceeding!</b></center> <br> <br>"
                     "With Version 3.2 the 'Athlete' directory has been refactored "
                     "by adding a set of subdirectories which hold the different types "
                     "of GoldenCheetah files.<br><br>"
                     "The new structure is:<br>"
                     "-> Activity files: <samp>/activities</samp><br>"
                     "-> Configuration files: <samp>/config</samp><br>"
                     "-> Download files: <samp>/downloads</samp><br>"
                     "-> Import files: <samp>/imports</samp><br>"
                     "-> Backups of Activity files: <samp>/bak</samp><br>"
                     "-> Workout related files: <samp>/workouts</samp><br>"
                     "-> Cache files: <samp>/cache</samp><br>"
                     "-> Calendar files: <samp>/calendar</samp><br>"
                     "-> Log files: <samp>/logs</samp><br>"
                     "-> Temp files: <samp>/temp</samp><br>"
                     "-> Temp for Activities: <samp>/tempActivities</samp><br>"
                     "-> Train View recordings: <samp>/recordings</samp><br>"
                     "-> Quarantined files: <samp>/quarantine</samp><br><br>"

                     "The upgrade process will create the new directory structure and move "
                     "the existing files to the new directories as needed. During the upgrade "
                     "all activity files will be converted to GoldenCheetah's native "
                     "file format .JSON and moved to the <br><samp>/activities</samp> folder. The source files "
                     "are moved to the <samp>/imports</samp> folder.<br><br>"
                     "Starting with version 3.2 all downloads from devices or imported "
                     "activity files will be converted to GoldenCheetah's file "
                     "format during import/download. The original files will be stored - depending on the source - "
                     "in <samp>/downloads</samp> or <br><samp>/imports</samp> folder.<br><br>"
                     "<center><b>Please make sure that you have done a backup of your athlete data "
                     "before proceeding with the upgrade. We can't take responsibility for "
                     "any loss of data during the process. </b> </center> <br>"
                     ));
    scrollText = new QScrollArea();
    scrollText->setWidget(text);

    QLabel *footer1 = new QLabel(this);
    footer1->setWordWrap(true);
    footer1->setTextFormat(Qt::RichText);
    footer1->setText(QString(tr("<center>Please backup the athlete directory:</center>")));

    QLabel *footer2 = new QLabel(this);
    footer2->setWordWrap(true);
    footer2->setTextFormat(Qt::RichText);
    footer2->setTextInteractionFlags(Qt::TextSelectableByMouse);
    footer2->setText(QString("<center><b>%1</b></center>").arg(athleteHomeDir.absolutePath()));

    toprow->addWidget(critical);
    toprow->addWidget(header);
    layout->addLayout(toprow);
    layout->addWidget(scrollText);
    layout->addWidget(footer1);
    layout->addWidget(footer2);


    QHBoxLayout *lastRow = new QHBoxLayout;

    proceedButton = new QPushButton(tr("Accept conditions and proceed with Upgrade"), this);
    proceedButton->setEnabled(true);
    connect(proceedButton, SIGNAL(clicked()), this, SLOT(accept()));
    abortButton = new QPushButton(tr("Abort Upgrade"), this);
    abortButton->setDefault(true);
    connect(abortButton, SIGNAL(clicked()), this, SLOT(reject()));


    lastRow->addWidget(abortButton);
    lastRow->addWidget(proceedButton);
    lastRow->addStretch();
    layout->addLayout(lastRow);

}



GcUpgradeLogDialog::GcUpgradeLogDialog(QDir homeDir) : QDialog(NULL, Qt::Dialog), home(homeDir)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    setWindowTitle(QString(tr("Athlete %1").arg(home.root().dirName())));

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *toprow = new QHBoxLayout;

    QPushButton *information = new QPushButton(style()->standardIcon(QStyle::SP_MessageBoxInformation), "", this);
    information->setFixedSize(60*dpiXFactor,60*dpiYFactor);
    information->setFlat(true);
    information->setIconSize(QSize(60*dpiXFactor,60*dpiYFactor));
    information->setAutoFillBackground(false);
    information->setFocusPolicy(Qt::NoFocus);

    QLabel *header = new QLabel(this);
    header->setWordWrap(true);
    header->setTextFormat(Qt::RichText);
    header->setText(tr("<h1>Upgrade log: GoldenCheetah v3.2</h1>"));

    toprow->addWidget(information);
    toprow->addWidget(header);
    layout->addLayout(toprow);

    QFont defaultFont;

    report = new QWebEngineView(this);
    report->setContentsMargins(0,0,0,0);
    report->page()->view()->setContentsMargins(0,0,0,0);
    report->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    //XXX WEBENGINE report->page()->setLinkDelegationPolicy(QWebPage::DelegateAllLinks);
    report->setContextMenuPolicy(Qt::NoContextMenu);
    report->setAcceptDrops(false);
    report->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, defaultFont.pointSize()+1);
    report->settings()->setFontFamily(QWebEngineSettings::StandardFont, defaultFont.family());

    connect(report, SIGNAL(linkClicked(QUrl)), this, SLOT(linkClickedSlot(QUrl)));

    layout->addWidget(report);

    QHBoxLayout *lastRow = new QHBoxLayout;

    // create the buttons for the whole Dialog - but only make visible what is needed at the point in time
    proceedButton = new QPushButton(tr("Proceed to Athlete"), this);
    connect(proceedButton, SIGNAL(clicked()), this, SLOT(accept()));
    saveAsButton = new QPushButton(tr("Save Upgrade Report..."), this);
    saveAsButton->setDefault(true);
    connect(saveAsButton, SIGNAL(clicked()), this, SLOT(saveAs()));

    // during logging, disable the buttons, so user can't escape
    proceedButton->setDisabled(true);
    saveAsButton->setDisabled(true);


    lastRow->addWidget(saveAsButton);
    lastRow->addWidget(proceedButton);
    lastRow->addStretch();
    layout->addLayout(lastRow);

    // the cyclist...
    reportText += QString("<center><h1>Cyclist: \"%1\"</h1></center><br>").arg(home.root().dirName());
    report->page()->setHtml(reportText);
}

GcUpgradeLogDialog::~GcUpgradeLogDialog()
{
    if (report) delete report->page();
}

void
GcUpgradeLogDialog::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName( this, tr("Save Log"), QDir::homePath(), tr("Text File (*.txt)"));

    // now write to it
    QFile file(fileName);
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    if (file.open(QIODevice::WriteOnly)) {

        // write the texts
        out << reportText;

    }
    file.close();
}

void
GcUpgradeLogDialog::linkClickedSlot( QUrl url )
{
    QDesktopServices::openUrl( url );

}


void
GcUpgradeLogDialog::append(QString text, int level) {

        switch (level) {
        case 3:
            reportText += QString("<h2>%1</h2>").arg(text);
            break;
        case 2:
            reportText += QString("<h3>%1</h3>").arg(text);
            break;
        case 1:
            reportText += QString("<h1>%1</h1>").arg(text);
            break;
        default: // any other
            reportText += QString("%1 <br>").arg(text);
        }

        report->page()->setHtml(reportText);
        //XXX WEBENGINE report->page()->setScrollBarValue(Qt::Vertical, report->page()->mainFrame()->scrollBarMaximum(Qt::Vertical));
        this->repaint();
        QApplication::processEvents();

}

void
GcUpgradeLogDialog::enableButtons() {
    saveAsButton->setDisabled(false);
    proceedButton->setDisabled(false);

}
