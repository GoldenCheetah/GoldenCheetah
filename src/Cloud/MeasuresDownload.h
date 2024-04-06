/*
 * Copyright (c) 2017 Joern Rischmueller (joern.rm@gmail.com)
 * Copyright (c) 2017 Ale Martinez (amtriathlon@gmail.com)
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

#ifndef _Gc_MeasuresDownload_h
#define _Gc_MeasuresDownload_h

#include "Context.h"

#include "WithingsDownload.h"
#include "MeasuresCsvImport.h"

#include <QDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QDateEdit>
#include <QLabel>
#include <QProgressBar>


class MeasuresDownload : public QDialog
{
    Q_OBJECT

public:
    MeasuresDownload(Context *context, MeasuresGroup *measuresGroup);
    ~MeasuresDownload();
    static void updateMeasures(Context *context,
                               MeasuresGroup *measuresGroup,
                               QList<Measure>&measures,
                               bool discardExisting=false);

private:

     Context *context;

     MeasuresGroup *measuresGroup;

     WithingsDownload *withingsDownload;
     MeasuresCsvImport *csvFileImport;

     QPushButton *downloadButton;
     QPushButton *closeButton;

     QCheckBox *discardExistingMeasures;

     // withings, csv file
     QRadioButton *downloadWithings;
     QRadioButton *downloadCSV;

     //  all, from last measure, manual date interval
     QRadioButton *dateRangeAll;
     QRadioButton *dateRangeLastMeasure;
     QRadioButton *dateRangeManual;

     QDateEdit *manualFromDate;
     QDateEdit *manualToDate;

     // Progress Bar
     QProgressBar *progressBar;

     enum source { WITHINGS = 1,
                   TP = 2,
                   CSV = 3
                 } ;

     enum timeframe { ALL = 1,
                      LAST = 2,
                      MANUAL = 3
                    } ;

private slots:
     void download();
     void close();
     void dateRangeAllSettingChanged(bool);
     void dateRangeLastSettingChanged(bool);
     void dateRangeManualSettingChanged(bool);

     void downloadWithingsSettingChanged(bool);
     void downloadCSVSettingChanged(bool);

     void downloadProgressStart(int);
     void downloadProgress(int);
     void downloadProgressEnd(int);

};


#endif
