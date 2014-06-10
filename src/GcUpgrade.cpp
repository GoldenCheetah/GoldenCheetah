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

    // 3.0 SP1 upgrade processing
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
