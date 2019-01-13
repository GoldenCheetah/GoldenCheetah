/*
 * Copyright (c) 2017 Joern Rischmueller (joern.rm@gmail.com)
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

#include "BodyMeasuresDownload.h"
#include "BodyMeasures.h"
#include "Athlete.h"
#include "RideCache.h"
#include "HelpWhatsThis.h"

#include <QList>
#include <QMutableListIterator>
#include <QGroupBox>
#include <QMessageBox>


BodyMeasuresDownload::BodyMeasuresDownload(Context *context) : context(context) {

    setWindowTitle(tr("Body Measurements download"));
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Tools_Download_BodyMeasures));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *groupBox1 = new QGroupBox(tr("Choose the download or import source"));
    downloadWithings = new QRadioButton(tr("Withings"));
    downloadTP = new QRadioButton(tr("Today's Plan"));
    downloadCSV = new QRadioButton(tr("Import CSV file"));
    QVBoxLayout *vbox1 = new QVBoxLayout;
    vbox1->addWidget(downloadWithings);
    vbox1->addWidget(downloadTP);
    vbox1->addWidget(downloadCSV);
    groupBox1->setLayout(vbox1);
    mainLayout->addWidget(groupBox1);

    QGroupBox *groupBox2 = new QGroupBox(tr("Choose date range for download"));
    dateRangeAll = new QRadioButton(tr("From date of first recorded activity to today"));
    dateRangeLastMeasure = new QRadioButton(tr("From date of last downloaded measurement to today"));
    dateRangeManual = new QRadioButton(tr("Enter manually:"));
    QVBoxLayout *vbox2 = new QVBoxLayout;
    vbox2->addWidget(dateRangeAll);
    vbox2->addWidget(dateRangeLastMeasure);
    vbox2->addWidget(dateRangeManual);
    vbox2->addStretch();
    QLabel *fromLabel = new QLabel("From");
    QLabel *toLabel = new QLabel("To");
    manualFromDate = new QDateEdit(this);
    manualFromDate->setDate(QDate::currentDate().addMonths(-1));
    manualToDate = new QDateEdit(this);
    manualToDate->setDate(QDate::currentDate());
    manualFromDate->setCalendarPopup(true);
    manualToDate->setCalendarPopup(true);
    manualFromDate->setEnabled(false);
    manualToDate->setEnabled(false);
    QHBoxLayout *dateRangeLayout = new QHBoxLayout;
    dateRangeLayout->addStretch();
    dateRangeLayout->addWidget(fromLabel);
    dateRangeLayout->addWidget(manualFromDate);
    dateRangeLayout->addWidget(toLabel);
    dateRangeLayout->addWidget(manualToDate);
    vbox2->addLayout(dateRangeLayout);
    groupBox2->setLayout(vbox2);
    mainLayout->addWidget(groupBox2);

    discardExistingMeasures = new QCheckBox(tr("Discard all existing measurements"), this);
    discardExistingMeasures->setChecked(false);
    mainLayout->addWidget(discardExistingMeasures);

    progressBar = new QProgressBar(this);
    progressBar->setMinimum(0);
    progressBar->setMaximum(1);
    progressBar->setValue(0);
    QHBoxLayout *progressLayout = new QHBoxLayout;
    progressLayout->addWidget(progressBar);
    mainLayout->addLayout(progressLayout);

    downloadButton = new QPushButton(tr("Download"));
    closeButton = new QPushButton(tr("Close"));
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(downloadButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);


    connect(downloadButton, SIGNAL(clicked()), this, SLOT(download()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(dateRangeAll, SIGNAL(toggled(bool)), this, SLOT(dateRangeAllSettingChanged(bool)));
    connect(dateRangeLastMeasure, SIGNAL(toggled(bool)), this, SLOT(dateRangeLastSettingChanged(bool)));
    connect(dateRangeManual, SIGNAL(toggled(bool)), this, SLOT(dateRangeManualSettingChanged(bool)));

    connect(downloadWithings, SIGNAL(toggled(bool)), this, SLOT(downloadWithingsSettingChanged(bool)));
    connect(downloadTP, SIGNAL(toggled(bool)), this, SLOT(downloadTPSettingChanged(bool)));
    connect(downloadCSV, SIGNAL(toggled(bool)), this, SLOT(downloadCSVSettingChanged(bool)));

    // don't allow options which are not authorized
    QString strToken =appsettings->cvalue(context->athlete->cyclist, GC_WITHINGS_TOKEN, "").toString();
    QString strSecret= appsettings->cvalue(context->athlete->cyclist, GC_WITHINGS_SECRET, "").toString();

    QString strToken2 =appsettings->cvalue(context->athlete->cyclist, GC_NOKIA_TOKEN, "").toString();

    if (strToken2 =="" && strToken == "" && strSecret == "") {
        downloadWithings->setEnabled(false);
    }

    QString token = appsettings->cvalue(context->athlete->cyclist, GC_TODAYSPLAN_TOKEN, "").toString();
    if (token == "") {
        downloadTP->setEnabled(false);
    }


    // select the default checked / based on available properties and last selection
    int last_selection = appsettings->cvalue(context->athlete->cyclist, GC_BM_LAST_TYPE, 0).toInt();
    bool done = false;
    if (downloadWithings->isEnabled()) {
        if (last_selection == 0 || last_selection == WITHINGS) {
            downloadWithings->setChecked(true);
            done = true;
        }
    }
    if (!done && downloadTP->isEnabled()) {
        if (last_selection == 0 || last_selection == TP) {
            downloadTP->setChecked(true);
            done = true;
        }
    }
    if (!done) {
        downloadCSV->setChecked(true);
    }


    // set the default from "last"
    int last_timeframe = appsettings->cvalue(context->athlete->cyclist, GC_BM_LAST_TIMEFRAME, ALL).toInt();
    switch (last_timeframe) {
    case ALL:
        dateRangeAll->setChecked(true);
        break;
    case LAST:
        dateRangeLastMeasure->setChecked(true);
        break;
    case MANUAL:
        dateRangeManual->setChecked(true);
        manualFromDate->setEnabled(true);
        manualToDate->setEnabled(true);
        break;
    default:
        dateRangeAll->setChecked(true);
    }
    // initialize the downloader
    withingsDownload = new WithingsDownload(context);
    todaysPlanBodyMeasureDownload = new TodaysPlanBodyMeasures(context);
    csvFileImport = new BodyMeasuresCsvImport(context);

    // connect the progress bar
    connect(todaysPlanBodyMeasureDownload, SIGNAL(downloadStarted(int)), this, SLOT(downloadProgressStart(int)));
    connect(todaysPlanBodyMeasureDownload, SIGNAL(downloadProgress(int)), this, SLOT(downloadProgress(int)));
    connect(todaysPlanBodyMeasureDownload, SIGNAL(downloadEnded(int)), this, SLOT(downloadProgressEnd(int)));

    connect(withingsDownload, SIGNAL(downloadStarted(int)), this, SLOT(downloadProgressStart(int)));
    connect(withingsDownload, SIGNAL(downloadProgress(int)), this, SLOT(downloadProgress(int)));
    connect(withingsDownload, SIGNAL(downloadEnded(int)), this, SLOT(downloadProgressEnd(int)));

    connect(csvFileImport, SIGNAL(downloadStarted(int)), this, SLOT(downloadProgressStart(int)));
    connect(csvFileImport, SIGNAL(downloadProgress(int)), this, SLOT(downloadProgress(int)));
    connect(csvFileImport, SIGNAL(downloadEnded(int)), this, SLOT(downloadProgressEnd(int)));
}

BodyMeasuresDownload::~BodyMeasuresDownload() {

    delete withingsDownload;
    delete todaysPlanBodyMeasureDownload;
    delete csvFileImport;

}


void
BodyMeasuresDownload::download() {

   // de-activate the buttons
   downloadButton->setEnabled(false);
   closeButton->setEnabled(false);

   progressBar->setMaximum(1);
   progressBar->setValue(0);

   // do the job
   BodyMeasures* pBodyMeasures = dynamic_cast <BodyMeasures*>(context->athlete->measures->getGroup(Measures::Body));
   QList<BodyMeasure> current = pBodyMeasures->bodyMeasures();
   QList<BodyMeasure> bodyMeasures;
   QDateTime fromDate;
   QDateTime toDate;
   QDateTime firstRideDate;
   QString err = "";
   bool downloadOk = false;

   // get the date of first ride as potential "from" value
   QList<QDateTime> rideDates = context->athlete->rideCache->getAllDates();
   if (rideDates.count() > 0) {
       firstRideDate = rideDates.at(0);
   }  else {
       firstRideDate = QDateTime::fromMSecsSinceEpoch(0);
   };

   // determine the date range
   if (dateRangeAll->isChecked()) {
       fromDate = firstRideDate;
       toDate = QDateTime::currentDateTimeUtc();
   } else if (dateRangeLastMeasure->isChecked()) {
       if (current.count() > 0) {
           fromDate = current.last().when.addSecs(1);
       } else {
           // use a reasonable default
           fromDate = firstRideDate;
       }
       toDate = QDateTime::currentDateTimeUtc();
   } else if (dateRangeManual->isChecked()) {
       if (manualFromDate->dateTime() > manualToDate->dateTime()) {
           QMessageBox::warning(this, tr("Body Measurements"), tr("Invalid date range - please check your input"));
           // re-activate the buttons
           downloadButton->setEnabled(true);
           closeButton->setEnabled(true);
           return;
       }
       fromDate.setDate(manualFromDate->date());
       fromDate.setTime(QTime(0,0));
       toDate.setDate(manualToDate->date());
   } else return;
   // to Time is always "end of the day"
   toDate.setTime(QTime(23,59));

   // do the download
   if (downloadWithings->isChecked()) {
     downloadOk = withingsDownload->getBodyMeasures(err, fromDate, toDate, bodyMeasures);
   } else if (downloadTP->isChecked()) {
     downloadOk = todaysPlanBodyMeasureDownload->getBodyMeasures(err, fromDate, toDate, bodyMeasures);
   } else if (downloadCSV->isChecked()) {
       downloadOk = csvFileImport->getBodyMeasures(err, fromDate, toDate, bodyMeasures);
   } else return;

   if (downloadOk) {

       // selection from various source may not be 100% accurate w.r.t. the from/to date filtering
       // e.g. on TodaysPlan the measureDate and selectionDate can deviate
       // so remove all measures which do not fit the selection from/to interval
       QMutableListIterator<BodyMeasure> i(bodyMeasures);
       BodyMeasure c;
       while (i.hasNext()) {
           c = i.next();
           if (c.when <= fromDate || c.when >= toDate) {
               i.remove();
           }
       }

       // we discard only if we have new data loaded - otherwise keep what is there
       if (discardExistingMeasures->isChecked()) {
           // now the new measures do not contain any "ts" of the current data any more
           current.clear();
       };

       // if exists, merge current data with new data - new data has preferences
       // no merging of weight data which has the same time stamp - new wins
       if (current.count() > 0) {
           // remove entry from current list if a new entry with same timestamp exists
           QMutableListIterator<BodyMeasure> i(current);
           BodyMeasure c;
           while (i.hasNext()) {
               c = i.next();
               foreach(BodyMeasure m, bodyMeasures) {
                   if (m.when == c.when) {
                       i.remove();
                   }
               }
           }
           // combine the result (without duplicates)
           bodyMeasures.append(current);
       }

       // update measures in cache and on file store
       context->athlete->rideCache->cancel();

       // store in athlete
       pBodyMeasures->setBodyMeasures(bodyMeasures);

       // now save data away if we actually got something !
       // doing it here means we don't overwrite previous responses
       // when we fail to get any data (e.g. errors / network problems)
       pBodyMeasures->write();

       // do a refresh, it will check if needed
       context->athlete->rideCache->refresh();

#ifdef Q_MAC_OS
       // since progressBar on MacOS does not show the % values
       QMessageBox msgBox;
       msgBox.setText(tr("Download completed."));
       msgBox.exec();
#endif

   } else {
       // handle error document in err String
       QMessageBox::warning(this, tr("Body Measurements"), tr("Downloading of body measurements failed with error: %1").arg(err));
   }

   // re-activate the buttons
   downloadButton->setEnabled(true);
   closeButton->setEnabled(true);

}

void
BodyMeasuresDownload::close() {

    accept();

}

void
BodyMeasuresDownload::dateRangeAllSettingChanged(bool checked) {

    if (checked) {
        manualFromDate->setEnabled(false);
        manualToDate->setEnabled(false);
        appsettings->setCValue(context->athlete->cyclist, GC_BM_LAST_TIMEFRAME, ALL);
    }
}


void
BodyMeasuresDownload::dateRangeLastSettingChanged(bool checked) {

    if (checked) {
        manualFromDate->setEnabled(false);
        manualToDate->setEnabled(false);
        appsettings->setCValue(context->athlete->cyclist, GC_BM_LAST_TIMEFRAME, LAST);
    }
}

void
BodyMeasuresDownload::dateRangeManualSettingChanged(bool checked) {

    if (checked) {
        manualFromDate->setEnabled(true);
        manualToDate->setEnabled(true);
        appsettings->setCValue(context->athlete->cyclist, GC_BM_LAST_TIMEFRAME, MANUAL);
    }
}

void
BodyMeasuresDownload::downloadWithingsSettingChanged(bool checked) {

    if (checked) {
        appsettings->setCValue(context->athlete->cyclist, GC_BM_LAST_TYPE, WITHINGS);
    }
}

void
BodyMeasuresDownload::downloadTPSettingChanged(bool checked) {

    if (checked) {
        appsettings->setCValue(context->athlete->cyclist, GC_BM_LAST_TYPE, TP);
    }
}

void
BodyMeasuresDownload::downloadCSVSettingChanged(bool checked) {

    if (checked) {
        appsettings->setCValue(context->athlete->cyclist, GC_BM_LAST_TYPE, CSV);
    }
}


void
BodyMeasuresDownload::downloadProgressStart(int total) {
    progressBar->setMaximum(total);
}
void
BodyMeasuresDownload::downloadProgress(int current) {
    progressBar->setValue(current);
}

void
BodyMeasuresDownload::downloadProgressEnd(int final) {
    progressBar->setValue(final);
}
