/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_TPDownloadDialog_h
#define _GC_TPDownloadDialog_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QDialog>
#include <QCheckBox>
#include <QHeaderView>
#include <QProgressBar>
#include <QMessageBox>
#include <QTreeWidget>
#include <QTabWidget>
#include <QLabel>

#include "RideCache.h"
#include "RideItem.h"
#include "RideFile.h"
#include "TPDownload.h"
#include "TPUpload.h"

class Context;

class TPDownloadDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        TPDownloadDialog(Context *context);
	
    protected:  
        // cached state
        QSettings *settings;
        bool useMetricUnits;

    public slots:
        void completedAthlete(QString, QList<QMap<QString,QString> >);
        void completedWorkout(QList<QMap<QString,QString> >);
        void completedDownload(QDomDocument);
        void completedUpload(QString);
        void cancelClicked();
        void refreshClicked();
        void tabChanged(int);
        void downloadClicked();
        void refreshCount();
        void refreshUpCount();
        void refreshSyncCount();
        void selectAllChanged(int);
        void selectAllUpChanged(int);
        void selectAllSyncChanged(int);

    private:
        Context *context;
        TPDownload *downloader;
        TPUpload *uploader;
        TPAthlete *athleter;
        TPWorkout *workouter;

        bool downloading;
        bool sync;
        bool aborted;

        // Quick lists for checking if file exists
        // locally (rideFiles) or remotely (uploadFiles)
        QStringList rideFiles;
        QStringList uploadFiles;

        // keeping track of progress...
        int downloadcounter,    // *x* of n downloading
            downloadtotal,      // x of *n* downloading
            successful,         // how many downloaded ok?
            listindex;          // where in rideList we've got to

        bool saveRide(RideFile *, QDomDocument &, QStringList &);
        bool syncNext();        // kick off another download/upload
                                // returns false if none left
        bool downloadNext();    // kick off another download
                                // returns false if none left
        bool uploadNext();     // kick off another upload
                                // returns false if none left

        // tabs - Upload/Download
        QTabWidget *tabs;

        // athlete selection
        QMap<QString, QString> athlete;
        QComboBox *athleteCombo;

        QPushButton *refreshButton;
        QPushButton *cancelButton;
        QPushButton *downloadButton;

        QDateEdit *from, *to;

        // Download
        QCheckBox *selectAll;
        QTreeWidget *rideList;

        // Upload
        QCheckBox *selectAllUp;
        QTreeWidget *rideListUp;

        // Sync
        QCheckBox *selectAllSync;
        QTreeWidget *rideListSync;
        QComboBox *syncMode;

        // show progress
        QProgressBar *progressBar;
        QLabel *progressLabel;

        QCheckBox *overwrite;
};

#endif // _GC_TPDownloadDialog_h
