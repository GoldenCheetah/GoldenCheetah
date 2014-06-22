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
#include <QDebug>

int 
GcUpgrade::upgrade(const QDir &home)
{ 
    // what was the last version? -- do we need to upgrade?
    int last = appsettings->cvalue(home.dirName(), GC_VERSION_USED, 0).toInt();

    // Upgrade processing was introduced in Version 3 -- below must be performed
    // for athlete directories from prior to Version 3
    // and can essentially be used as a template for all major release
    // upgrades as it delets old stuff and sets clean

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
            appsettings->setCValue(home.dirName(), "splitter/LTM/hide", true);
            appsettings->setCValue(home.dirName(), "splitter/LTM/hide/0", false);
            appsettings->setCValue(home.dirName(), "splitter/LTM/hide/1", false);
            appsettings->setCValue(home.dirName(), "splitter/LTM/hide/2", false);
            appsettings->setCValue(home.dirName(), "splitter/LTM/hide/3", true);
            appsettings->setCValue(home.dirName(), "splitter/analysis/hide", true);
            appsettings->setCValue(home.dirName(), "splitter/analysis/hide/0", false);
            appsettings->setCValue(home.dirName(), "splitter/analysis/hide/1", true);
            appsettings->setCValue(home.dirName(), "splitter/analysis/hide/2", false);
            appsettings->setCValue(home.dirName(), "splitter/analysis/hide/3", true);
            appsettings->setCValue(home.dirName(), "splitter/diary/hide", true);
            appsettings->setCValue(home.dirName(), "splitter/diary/hide/0", false);
            appsettings->setCValue(home.dirName(), "splitter/diary/hide/1", false);
            appsettings->setCValue(home.dirName(), "splitter/diary/hide/2", true);
            appsettings->setCValue(home.dirName(), "splitter/train/hide", true);
            appsettings->setCValue(home.dirName(), "splitter/train/hide/0", false);
            appsettings->setCValue(home.dirName(), "splitter/train/hide/1", false);
            appsettings->setCValue(home.dirName(), "splitter/train/hide/2", false);
            appsettings->setCValue(home.dirName(), "splitter/train/hide/3", false);

            // 6. Delete any old measures.xml -- its for withings only
            QFile msxml(QString("%1/measures.xml").arg(home.canonicalPath()));
            if (msxml.exists()) msxml.remove();

            // FINALLY -- Set latest version - so only tries to upgrade once
            appsettings->setCValue(home.dirName(), GC_VERSION_USED, VERSION_LATEST);
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

    if (false && last < VERSION31_BUILD) { // << note this is not activated yet

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
        QColor chromeColor = QColor(Qt::darkGray);
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

            // read em in
            RideMetadata::readXML(filename, keywordDefinitions, fieldDefinitions, colorfield);

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
                RideMetadata::serialize(filename, keywordDefinitions, fieldDefinitions, colorfield);
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

    return 0;
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
