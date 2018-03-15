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

#include "HrvMeasuresDownload.h"
#include "HrvMeasures.h"
#include "Athlete.h"
#include "RideCache.h"
#include "HelpWhatsThis.h"

#include <QList>
#include <QMutableListIterator>
#include <QGroupBox>
#include <QMessageBox>


HrvMeasuresDownload::HrvMeasuresDownload(Context *context) : context(context) {

    setWindowTitle(tr("HRV Measurements download"));
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Tools_Download_HrvMeasures));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *groupBox1 = new QGroupBox(tr("Choose the download or import source"));
    downloadCSV = new QRadioButton(tr("Import CSV file"));
    QVBoxLayout *vbox1 = new QVBoxLayout;
    vbox1->addWidget(downloadCSV);
    groupBox1->setLayout(vbox1);
    mainLayout->addWidget(groupBox1);

    QGroupBox *groupBox2 = new QGroupBox(tr("Choose date range for download"));
    dateRangeAll = new QRadioButton(tr("From date of first recorded activity to today"));
    dateRangeLastMeasure = new QRadioButton(tr("From date of last downloaded measurement to today"));
    dateRangeManual = new QRadioButton(tr("Enter manually:"));
    dateRangeAll->setChecked(true);
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

    // select the default checked
    downloadCSV->setChecked(true);

    // initialize the downloader
    csvFileImport = new HrvMeasuresCsvImport(context);

    // connect the progress bar
    connect(csvFileImport, SIGNAL(downloadStarted(int)), this, SLOT(downloadProgressStart(int)));
    connect(csvFileImport, SIGNAL(downloadProgress(int)), this, SLOT(downloadProgress(int)));
    connect(csvFileImport, SIGNAL(downloadEnded(int)), this, SLOT(downloadProgressEnd(int)));
}

HrvMeasuresDownload::~HrvMeasuresDownload() {

    delete csvFileImport;

}


void
HrvMeasuresDownload::download() {

   // de-activate the buttons
   downloadButton->setEnabled(false);
   closeButton->setEnabled(false);

   progressBar->setMaximum(1);
   progressBar->setValue(0);

   // do the job
   HrvMeasures* pHrvMeasures = dynamic_cast <HrvMeasures*>(context->athlete->measures->getGroup(Measures::Hrv));
   QList<HrvMeasure> current = pHrvMeasures->hrvMeasures();
   QList<HrvMeasure> hrvMeasures;
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
   if (downloadCSV->isChecked()) {
       downloadOk = csvFileImport->getHrvMeasures(err, fromDate, toDate, hrvMeasures);
   } else return;

   if (downloadOk) {

       // selection from various source may not be 100% accurate w.r.t. the from/to date filtering
       // so remove all measures which do not fit the selection from/to interval
       QMutableListIterator<HrvMeasure> i(hrvMeasures);
       HrvMeasure c;
       while (i.hasNext()) {
           c = i.next();
           if (c.when <= fromDate || c.when >= toDate) {
               i.remove();
           }
       }

       updateMeasures(context, hrvMeasures, discardExistingMeasures->isChecked());

#ifdef Q_MAC_OS
       // since progressBar on MacOS does not show the % values
       QMessageBox msgBox;
       msgBox.setText(tr("Download completed."));
       msgBox.exec();
#endif

   } else {
       // handle error document in err String
       QMessageBox::warning(this, tr("HRV Measurements"), tr("Downloading of HRV measurements failed with error: %1").arg(err));
   }

   // re-activate the buttons
   downloadButton->setEnabled(true);
   closeButton->setEnabled(true);

}

void
HrvMeasuresDownload::close() {

    accept();

}

void
HrvMeasuresDownload::dateRangeAllSettingChanged(bool checked) {

    if (checked) {
        manualFromDate->setEnabled(false);
        manualToDate->setEnabled(false);
    }
}


void
HrvMeasuresDownload::dateRangeLastSettingChanged(bool checked) {

    if (checked) {
        manualFromDate->setEnabled(false);
        manualToDate->setEnabled(false);
    }
}

void
HrvMeasuresDownload::dateRangeManualSettingChanged(bool checked) {

    if (checked) {
        manualFromDate->setEnabled(true);
        manualToDate->setEnabled(true);
    }
}

void
HrvMeasuresDownload::downloadProgressStart(int total) {
    progressBar->setMaximum(total);
}
void
HrvMeasuresDownload::downloadProgress(int current) {
    progressBar->setValue(current);
}

void
HrvMeasuresDownload::downloadProgressEnd(int final) {
    progressBar->setValue(final);
}

void
HrvMeasuresDownload::updateMeasures(Context *context,
                                    QList<HrvMeasure>&hrvMeasures,
                                    bool discardExisting) {

   HrvMeasures* pHrvMeasures = dynamic_cast <HrvMeasures*>(context->athlete->measures->getGroup(Measures::Hrv));
   QList<HrvMeasure> current = pHrvMeasures->hrvMeasures();

   // we discard only if we have new data loaded - otherwise keep what is there
   if (discardExisting) {
       // now the new measures do not contain any "ts" of the current data any more
       current.clear();
   };

   // if exists, merge current data with new data - new data has preferences
   // no merging of HRV data which has the same time stamp - new wins
   if (current.count() > 0) {
       // remove entry from current list if a new entry with same timestamp exists
       QMutableListIterator<HrvMeasure> i(current);
       HrvMeasure c;
       while (i.hasNext()) {
           c = i.next();
           foreach(HrvMeasure m, hrvMeasures) {
               if (m.when == c.when) {
                   i.remove();
               }
           }
       }
       // combine the result (without duplicates)
       hrvMeasures.append(current);
   }

   // update measures in cache and on file store
   context->athlete->rideCache->cancel();

   // store in athlete
   pHrvMeasures->setHrvMeasures(hrvMeasures);

   // now save data away if we actually got something !
   // doing it here means we don't overwrite previous responses
   // when we fail to get any data (e.g. errors / network problems)
   pHrvMeasures->write();

   // do a refresh, it will check if needed
   context->athlete->rideCache->refresh();
}
