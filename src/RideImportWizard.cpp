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

#include <assert.h>
#include <QDebug>
#include "RideItem.h"
#include "RideFile.h"
#include "RideImportWizard.h"
#include "MainWindow.h"
#include "QuarqRideFile.h"
#include <QWaitCondition>
#include "Settings.h"
#include "Units.h"
#include "GcRideFile.h"


// drag and drop passes urls ... convert to a list of files and call main constructor
RideImportWizard::RideImportWizard(QList<QUrl> *urls, QDir &home, MainWindow *main, QWidget *parent) : QDialog(parent)
{
    QList<QString> filenames;
    for (int i=0; i<urls->count(); i++)
        filenames.append(QFileInfo(urls->value(i).toLocalFile()).absoluteFilePath());
    init(filenames, home, main);
    filenames.clear();
}

RideImportWizard::RideImportWizard(QList<QString> files, QDir &home, MainWindow *main, QWidget *parent) : QDialog(parent)
{
    init(files, home, main);
}

void
RideImportWizard::init(QList<QString> files, QDir &home, MainWindow *main)
{

    // initialise dialog box
    tableWidget = new QTableWidget(files.count(), 6, this);
    tableWidget->setItemDelegate(new RideDelegate(1)); // use a delegate for column 1 date
    tableWidget->verticalHeader()->setDefaultSectionSize(20);
    phaseLabel = new QLabel;
    progressBar = new QProgressBar();
    todayButton = new QComboBox();
    todayButton->addItem(tr("Select Date..."));
    todayButton->addItem(tr("Today"));
    todayButton->addItem(tr("Last Monday"));
    todayButton->addItem(tr("Last Tuesday"));
    todayButton->addItem(tr("Last Wednesday"));
    todayButton->addItem(tr("Last Thursday"));
    todayButton->addItem(tr("Last Friday"));
    todayButton->addItem(tr("Last Saturday"));
    todayButton->addItem(tr("Last Sunday"));
    todayButton->addItem(tr("Choose Date"));
    cancelButton = new QPushButton(tr("Cancel"));
    abortButton = new QPushButton(tr("Abort"));
    overFiles = new QCheckBox(tr("Overwrite Existing Files"));
    // initially the cancel, overwrite and today widgets are hidden
    // they only appear whilst we are asking the user for dates
    cancelButton->setHidden(true);
    todayButton->setHidden(true);
    overFiles->setHidden(true);
    overwriteFiles = false;

    aborted = false;

    // NOTE: abort button morphs into save and finish button later
    connect(abortButton, SIGNAL(clicked()), this, SLOT(abortClicked()));

    // only used when editing dates
    connect(todayButton, SIGNAL(activated(int)), this, SLOT(todayClicked(int)));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(overFiles, SIGNAL(clicked()), this, SLOT(overClicked()));

    // title & headings
    setWindowTitle(tr("Import Ride Files"));
    QTableWidgetItem *filenameHeading = new QTableWidgetItem;
    filenameHeading->setText(tr("Filename"));
    tableWidget->setHorizontalHeaderItem(0, filenameHeading);

    QTableWidgetItem *dateHeading = new QTableWidgetItem;
    dateHeading->setText(tr("Date"));
    tableWidget->setHorizontalHeaderItem(1, dateHeading);

    QTableWidgetItem *timeHeading = new QTableWidgetItem;
    timeHeading->setText(tr("Time"));
    tableWidget->setHorizontalHeaderItem(2, timeHeading);

    QTableWidgetItem *durationHeading = new QTableWidgetItem;
    durationHeading->setText(tr("Duration"));
    tableWidget->setHorizontalHeaderItem(3, durationHeading);

    QTableWidgetItem *distanceHeading = new QTableWidgetItem;
    distanceHeading->setText(tr("Distance"));
    tableWidget->setHorizontalHeaderItem(4, distanceHeading);

    QTableWidgetItem *statusHeading = new QTableWidgetItem;
    statusHeading->setText(tr("Import Status"));
    tableWidget->setHorizontalHeaderItem(5, statusHeading);

    // save target dir
    this->home = home;
    mainWindow = main;

    // Fill in the filenames and all the textItems
    for (int i=0; i < files.count(); i++) {
        QTableWidgetItem *t;

        filenames.append(QFileInfo(files[i]).absoluteFilePath());
        blanks.append(true); // by default editable

        // Filename
        t = new QTableWidgetItem();
        t->setText(QFileInfo(files[i]).fileName());
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        tableWidget->setItem(i,0,t);

        // Date
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags()  | Qt::ItemIsEditable);
        t->setBackgroundColor(Qt::red);
        tableWidget->setItem(i,1,t);

        // Time
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() | Qt::ItemIsEditable);
        tableWidget->setItem(i,2,t);

        // Duration
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        tableWidget->setItem(i,3,t);

        // Distance
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        tableWidget->setItem(i,4,t);

        // Import Status
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        tableWidget->setItem(i,5,t);
    }

    // put into our dialog box
    // layout
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(phaseLabel);
    buttons->addStretch();
    buttons->addWidget(todayButton);
    buttons->addStretch();
    buttons->addWidget(overFiles);
    buttons->addWidget(cancelButton);
    buttons->addWidget(abortButton);

    QVBoxLayout *contents = new QVBoxLayout(this);
    contents->addWidget(tableWidget);
    contents->addWidget(progressBar);
    contents->addLayout(buttons);
    setLayout(contents);

    // adjust all the sizes to look tidy
    tableWidget->setColumnWidth(0, 200); // filename
    tableWidget->setColumnWidth(1, 120); // date
    tableWidget->setColumnWidth(2, 120); // time
    tableWidget->setColumnWidth(3, 100); // duration
    tableWidget->setColumnWidth(4, 70); // distance
    tableWidget->setColumnWidth(5, 250); // status

    // max height for 16 items and a scrollbar on right if > 16 items
    // for some reason the window is wider for 10-16 items too.
    // someone that understands width/geometry and layouts better
    // than me should clean up the logic here, no doubt using a
    // single call to geometry/hint. But for now it looks good
    // on Leopard, no doubt not so good on Windows
    resize(920 +
           ((files.count() > 16 ? 24 : 0) +
           (files.count() > 9 && files.count() < 17) ? 8 : 0),
           118 + (files.count() > 16 ? 17*20 : (files.count()+1) * 20));

    tableWidget->adjustSize();

    // Refresh prior to running down the list & processing...
    this->show();
}

int
RideImportWizard::process()
{

    // set progress bar limits - for each file we
    // will make 5 passes over the files
    //         1. checking it is a file ane readable
    //         2. parsing it with the RideFileReader
    //         3. [optional] collect date/time information from user
    //         4. copy file into Library
    //         5. Process for CPI (not implemented yet)

    // So, therefore the progress bar runs from 0 to files*4. (since step 5 is not implemented yet)
    progressBar->setMinimum(0);
    progressBar->setMaximum(filenames.count()*4);

    // Pass one - Is it valid?
    phaseLabel->setText(tr("Step 1 of 4: Check file permissions"));
    for (int i=0; i < filenames.count(); i++) {

        // get fullpath name for processing
        QFileInfo thisfile(filenames[i]);

        if (thisfile.exists() && thisfile.isFile() && thisfile.isReadable()) {

            // is it one we understand ?
            QStringList suffixList = RideFileFactory::instance().suffixes();
            QRegExp suffixes(QString("^(%1)$").arg(suffixList.join("|")));
            suffixes.setCaseSensitivity(Qt::CaseInsensitive);

            if (suffixes.exactMatch(thisfile.suffix())) {

        // Woot. We know how to parse this baby
                tableWidget->item(i,5)->setText(tr("Queued"));

            } else {
                tableWidget->item(i,5)->setText(tr("Error - Unknown file type"));
            }

        } else {
            //  Cannot open
            tableWidget->item(i,5)->setText(tr("Error - Not a valid file"));
        }

        progressBar->setValue(progressBar->value()+1);

    }

    QApplication::processEvents();
    if (aborted) { done(0); }
    repaint();

    // Pass 2 - Read in with the relevant RideFileReader method

    phaseLabel->setText(tr("Step 2 of 4: Validating Files"));
   for (int i=0; i< filenames.count(); i++) {


        // does the status say Queued?
        if (!tableWidget->item(i,5)->text().startsWith(tr("Error"))) {

              QStringList errors;
              QFile thisfile(filenames[i]);

              tableWidget->item(i,5)->setText(tr("Parsing..."));
              tableWidget->setCurrentCell(i,5);
              QApplication::processEvents();
              if (aborted) { done(0); }
              this->repaint();

              boost::scoped_ptr<RideFile> ride(RideFileFactory::instance().openRideFile(thisfile, errors));

              // did it parse ok?
              if (ride) {

                   // ride != NULL but !errors.isEmpty() means they're just warnings
                   if (errors.isEmpty())
                       tableWidget->item(i,5)->setText(tr("Validated"));
                   else
                       tableWidget->item(i,5)->setText(tr("Warning - ") + errors.join(tr(" ")));

                   // Set Date and Time
                   if (ride->startTime().isNull()) {

                       // Poo. The user needs to supply the date/time for this ride
                       blanks[i] = true;
                       tableWidget->item(i,1)->setText(tr(""));
                       tableWidget->item(i,2)->setText(tr(""));

                   } else {

                       // Cool, the date and time was extrcted from the source file
                       blanks[i] = false;
                       tableWidget->item(i,1)->setText(ride->startTime().toString(tr("dd MMM yyyy")));
                       tableWidget->item(i,2)->setText(ride->startTime().toString(tr("hh:mm:ss ap")));
                   }

                   tableWidget->item(i,1)->setTextAlignment(Qt::AlignRight); // put in the middle
                   tableWidget->item(i,2)->setTextAlignment(Qt::AlignRight); // put in the middle

                   boost::shared_ptr<QSettings> settings = GetApplicationSettings();
                   QVariant unit = settings->value(GC_UNIT);
                   bool metric = unit.toString() == "Metric";

                   // time and distance from tags (.gc files)
                   QMap<QString,QString> lookup;
                   lookup = ride->metricOverrides.value("total_distance");
                   double km = lookup.value("value", "0.0").toDouble();

                   lookup = ride->metricOverrides.value("workout_time");
                   int secs = lookup.value("value", "0.0").toDouble();

                   // show duration by looking at last data point
                   if (!ride->dataPoints().isEmpty() && ride->dataPoints().last() != NULL) {
                       if (!secs) secs = ride->dataPoints().last()->secs;
                       if (!km) km = ride->dataPoints().last()->km;
                   }

                   QChar zero = QLatin1Char ( '0' );
                   QString time = QString("%1:%2:%3").arg(secs/3600,2,10,zero)
                       .arg(secs%3600/60,2,10,zero)
                       .arg(secs%60,2,10,zero);
                   tableWidget->item(i,3)->setText(time);
                   tableWidget->item(i,3)->setTextAlignment(Qt::AlignHCenter); // put in the middle

                   // show distance by looking at last data point
                   QString dist = metric
                       ? QString ("%1 km").arg(km, 0, 'f', 1)
                       : QString ("%1 mi").arg(km * MILES_PER_KM, 0, 'f', 1);
                   tableWidget->item(i,4)->setText(dist);
                   tableWidget->item(i,4)->setTextAlignment(Qt::AlignRight); // put in the middle
               } else {
                   // nope - can't handle this file
                   tableWidget->item(i,5)->setText(tr("Error - ") + errors.join(tr(" ")));
               }
        }
        progressBar->setValue(progressBar->value()+1);
        QApplication::processEvents();
        if (aborted) { done(0); }
        this->repaint();
    }

    // Pass 3 - get missing date and times for imported files
    //         Actually allow us to edit date on ANY ride, we
    //         make sure that the ride date/time is set from
    //         the filename and never from the ride data
    phaseLabel->setText(tr("Step 3 of 4: Confirm Date and Time"));

    int needdates=0;
    for (int i=0; i<filenames.count(); i++) {

        // ignore errors
        QTableWidgetItem *t = tableWidget->item(i,5);
        if (t->text().startsWith(tr("Error"))) continue;

        if (blanks[i]) needdates++; // count the blanks tho -- these MUST be edited

        // does nothing for the moment
        progressBar->setValue(progressBar->value()+1);
        progressBar->repaint();
   }

   // Wait for user to press save
   abortButton->setText(tr("Save"));
   aborted = false;

   cancelButton->setHidden(false);
   todayButton->setHidden(false);
   overFiles->setHidden(false);

   if (needdates == 0) {
      // no need to wait for the user to input dates
      // nd press save if all the dates are set
      // (i.e. they got set by the file importers already)
      // do nothing for now since we need to confirm dates
      // and confirm overwrite files rather than importing
      // without user intervention

      abortButton->setDisabled(false);

      // abortClicked();
   } else {

      // de-activate Save button until the dates and times are sorted
      abortButton->setDisabled(true);
   }
   connect(tableWidget, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(activateSave()));

   return 0;
}

void
RideImportWizard::overClicked()
{
    overwriteFiles = overFiles->isChecked();
}

void
RideImportWizard::activateSave()
{

   for (int i=0; i<filenames.count(); i++) {

        // ignore errors
        QTableWidgetItem *t = tableWidget->item(i,5);
        if (t->text().startsWith(tr("Error"))) continue;

       // date needed?
        t = tableWidget->item(i,1);
        if (t->text() == "") return;

        // time needed?
        t = tableWidget->item(i,2);
        if (t->text() == "") return;
   }
   // if you got here then all entries that need a date have a date
   abortButton->setDisabled(false);
}

// the code here is tedious. checking for conditions about total ride time
// and if it fits in a day, then distributing them over the day, but
// only if the time is not set, and then if its being set to today
// then start must be before the current time cause you can't log
// data for future rides ... and so on.
//
// but then this feature is gonna save a lot of typing over time ...
//
void
RideImportWizard::todayClicked(int index)
{
    QDate selectedDate; // the date we're gonna apply to the row(s) highlighted

    // set the index back to 0, so we can select again
    todayButton->setCurrentIndex(0);

    // 0 = nothing selected, 1 = Today - 2 = last monday thru to 8 = last sunday
    if (index == 0) { // no selection
        return;
    } else if (index == 1) { // today
        selectedDate = QDate().currentDate();
    } else if (index == 9) { // other date - set focus on first highlighted date
        for (int i=0; i<filenames.count(); i++) {
            if (tableWidget->item(i,0)->isSelected() ||
                tableWidget->item(i,1)->isSelected() ||
                tableWidget->item(i,2)->isSelected() ||
                tableWidget->item(i,3)->isSelected() ||
                tableWidget->item(i,4)->isSelected() ||
                tableWidget->item(i,5)->isSelected()) {
                tableWidget->editItem(tableWidget->item(i,1));
                return;
            }
        }
        return;
    } else { // last mon/tue/wed/thu/fri/sat/sun
        int daysago =  (QDate().currentDate().dayOfWeek() - index + 8) % 7;
        if (!daysago) daysago=7; // e.g.last wednesday is 7 days ago if today is wednesday
        selectedDate = QDate().fromJulianDay(QDate().currentDate().toJulianDay()-daysago);
    }

    // Only apply to selected date - set time to current time - ride duration
    // pretty daft but at least it sets it to something, anything is gonna be random
    int countselected = 0;
    int totalduration = 0;
    for (int i=0; i< filenames.count(); i++) {
        if (tableWidget->item(i,0)->isSelected() ||
            tableWidget->item(i,1)->isSelected() ||
            tableWidget->item(i,2)->isSelected() ||
            tableWidget->item(i,3)->isSelected() ||
            tableWidget->item(i,4)->isSelected() ||
            tableWidget->item(i,5)->isSelected()) {
            countselected++;

            QTime duration = QTime().fromString(tableWidget->item(i,3)->text(), tr("hh:mm:ss"));
            totalduration += duration.hour() * 3600 +
                             duration.minute() * 60 +
                             duration.second();
        }
    }

    // More than a days worth of rides so can't squeeze into a single day!
    if (totalduration > (24 * 3600)) {
        QMessageBox::warning ( this, tr ( "Invalid Selection" ), tr ( "More than 24hrs of rides to fit into a day" ));
        return;
    }

    // if it is today then start from now - total rides duration
    // if that goes negative then just do the std thing
    int rstart=0;
    if (index == 1) {

        QTime timenow = QTime().currentTime();

        int now  = timenow.hour() * 3600 +
                   timenow.minute() * 60 +
                   timenow.second();

        rstart = now - totalduration;

    }

    if (rstart <= 0) { // i.e. still not set properly...
        // if it is not being set to "today" then spread them across the
        // day back to back with equal time before and after noon.
        rstart = ((24*3600) - totalduration) /2;
    }

    // zip through the rows and where highlighted set the date
    // if the start time is not set set it to rstart and increment
    // by the duration of the ride.
    for (int i=0; i< filenames.count(); i++) {
        if (tableWidget->item(i,0)->isSelected() ||
            tableWidget->item(i,1)->isSelected() ||
            tableWidget->item(i,2)->isSelected() ||
            tableWidget->item(i,3)->isSelected() ||
            tableWidget->item(i,4)->isSelected() ||
            tableWidget->item(i,5)->isSelected()) {

            // set the date to date selected
            tableWidget->item(i,1)->setText(selectedDate.toString(tr("dd MMM yyyy")));
            // look at rides with missing start time - we need to populate those

            // ride duration
            QTime duration = QTime().fromString(tableWidget->item(i,3)->text(), tr("hh:mm:ss"));

            // ride start time
            QTime time(rstart/3600, rstart%3600/60, rstart%60);
            tableWidget->item(i,2)->setText(time.toString(tr("hh:mm:ss a")));
            rstart += duration.hour() * 3600 +
                      duration.minute() * 60 +
                      duration.second();
        }
    }
    // phew! - repaint!
    QApplication::processEvents();
    tableWidget->repaint();
}

void
RideImportWizard::cancelClicked()
{
    done(0); // you are the weakest link, goodbye.
}

// info structure used by cpi updater
struct cpi_file_info {
    QString file, inname, outname;
};


void
RideImportWizard::abortClicked()
{

    // if done when labelled abort we kill off this dialog
    QString label = abortButton->text();

    if (label == tr("Abort")) {
        aborted=true; // terminated. I'll be back.
        return;
    }

    if (label == tr("Finish")) {
       // phew. our work is done.
       done(0);
       return;
    }

   int needdates=0;
   for (int i=0; i<filenames.count(); i++) {

        QTableWidgetItem *t = tableWidget->item(i,5);
        if (t->text().startsWith(tr("Error"))) continue;
       // date needed?
        t = tableWidget->item(i,1);
        if (t->text() == "") {
            needdates++;
        }
        t->setFlags(t->flags() | (Qt::ItemIsEditable));

        // time needed?
        t = tableWidget->item(i,2);
        if (t->text() == "") {
            needdates++;
        }
        t->setFlags(t->flags() | (Qt::ItemIsEditable));
   }

   if (needdates) return; // no dice dude, we need those dates filled in!

    // if done when labelled save we copy the files and run the cpi calculator
    phaseLabel->setText(tr("Step 4 of 4: Save to Library"));

    abortButton->setText(tr("Abort"));
    aborted = false;
    cancelButton->setHidden(true);
    todayButton->setHidden(true);
    overFiles->setHidden(true);

    // now set this fields uneditable again ... yeesh.
    for (int i=0; i <filenames.count(); i++) {
            QTableWidgetItem *t = tableWidget->item(i,1);
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            t = tableWidget->item(i,2);
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
    }

    QChar zero = QLatin1Char ( '0' );

    for (int i=0; i< filenames.count(); i++) {

        if (tableWidget->item(i,5)->text().startsWith(tr("Error"))) continue; // skip error

        tableWidget->item(i,5)->setText(tr("Saving..."));
        tableWidget->setCurrentCell(i,5);
        QApplication::processEvents();
        if (aborted) { done(0); }
        this->repaint();

        // Setup the ridetime as a QDateTime
        QDateTime ridedatetime = QDateTime(QDate().fromString(tableWidget->item(i,1)->text(), tr("dd MMM yyyy")),
                                 QTime().fromString(tableWidget->item(i,2)->text(), tr("hh:mm:ss a")));
        QString suffix = QFileInfo(filenames[i]).suffix();
        QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                               .arg ( ridedatetime.date().year(), 4, 10, zero )
                               .arg ( ridedatetime.date().month(), 2, 10, zero )
                               .arg ( ridedatetime.date().day(), 2, 10, zero )
                               .arg ( ridedatetime.time().hour(), 2, 10, zero )
                               .arg ( ridedatetime.time().minute(), 2, 10, zero )
                               .arg ( ridedatetime.time().second(), 2, 10, zero );
        QString target = QString ("%1.%2" )
                               .arg ( targetnosuffix )
                               .arg ( suffix );
        QString fulltarget = home.absolutePath() + "/" + target;

        // if its a gc file we need to parse and serialize
        // using the ridedatetime and target filename
        if (filenames[i].endsWith(".gc", Qt::CaseInsensitive)) {

            bool existed;
            if ((existed=QFileInfo(fulltarget).exists()) && !overwriteFiles) {
                tableWidget->item(i,5)->setText(tr("Error - File exists"));
            } else {

                // read the file (again)
                QStringList errors;
                QFile thisfile(filenames[i]);
                RideFile *ride(RideFileFactory::instance().openRideFile(thisfile, errors));

                // update ridedatetime
                ride->setStartTime(ridedatetime);

                // serialize
                GcFileReader reader;
                QFile target(fulltarget);
                reader.writeRideFile(ride, target);

                // clear
                delete ride;

                if (existed) {
                    tableWidget->item(i,5)->setText(tr("File Overwritten"));
                } else {
                    tableWidget->item(i,5)->setText(tr("File Saved"));
                    mainWindow->addRide(QFileInfo(fulltarget).fileName(), true);
                }
            }

        } else {

            // for native file formats the filename IS the ride date time so
            // no need to write -- we just copy

            // so now we have sourcefile in 'filenames[i]' and target file name in 'target'
            if (!fulltarget.compare(filenames[i])) { // they are the same file! so skip copy
                tableWidget->item(i,5)->setText(tr("Error - Source is Target"));
            } else if (QFileInfo(fulltarget).exists()) {
                if (overwriteFiles) {
                    tableWidget->item(i,5)->setText(tr("Overwriting file..."));
                    QFile source(filenames[i]);
                    QString fulltargettmp(home.absolutePath() + tr("/") + targetnosuffix + tr(".tmp"));

                    if (source.copy(fulltargettmp)) {
                        // tmp version saved now zap original
                        QFile original(fulltarget);
                        original.remove(); // zap!
                        // mv tmp to target
                        QFile temp(fulltargettmp);
                        if (temp.rename(fulltarget)) {
                            tableWidget->item(i,5)->setText(tr("File Overwritten"));
                            //no need to add since its already there!
                        } else
                            tableWidget->item(i,5)->setText(tr("Error - overwrite failed"));
                    } else {
                        tableWidget->item(i,5)->setText(tr("Error - overwrite failed"));
                    }
                } else {
                    tableWidget->item(i,5)->setText(tr("Error - File exists"));
                }
            } else {
                    tableWidget->item(i,5)->setText(tr("Saving file..."));
                    QFile source(filenames[i]);
                    if (source.copy(fulltarget)) {
                        tableWidget->item(i,5)->setText(tr("File Saved"));
                        mainWindow->addRide(QFileInfo(fulltarget).fileName(), true); // add to tree view
                        // free immediately otherwise all imported rides are cached
                        // and with large imports this can lead to memory exhaustion
                        mainWindow->rideItem()->freeMemory();
                    } else
                        tableWidget->item(i,5)->setText(tr("Error - copy failed"));
            }
        }
        QApplication::processEvents();
        if (aborted) { done(0); }
        progressBar->setValue(progressBar->value()+1);
        this->repaint();
    }

#if 0 // NOT UNTIL CPINTPLOT.CPP IS REFACTORED TO SEPERATE CPI FILES MAINTENANCE FROM CP PLOT CODE
    // if done when labelled save we copy the files and run the cpi calculator
    phaseLabel->setText(tr("Step 5 of 5: Calculating Critical Powers"));

   abortButton->setText(tr("Abort"));
   aborted = false;

    for (int i=0; i< filenames.count(); i++) {

        if (!tableWidget->item(i,5)->text().startsWith(tr("Error"))) {
            tableWidget->item(i,5)->setText(tr("Calculating..."));
            tableWidget->setCurrentCell(i,5);
            QApplication::processEvents();
            if (aborted) { done(0); }
            this->repaint();

            // calculated

            // change status
            tableWidget->item(i,5)->setText(tr("Completed."));
        }
        QApplication::processEvents();
        if (aborted) { done(0); }
        progressBar->setValue(progressBar->value()+1);
        this->repaint();
    }
#endif // not until CPINTPLOT IS REFACTORED


    // how did we get on in the end then ...
    int completed = 0;
    for (int i=0; i< filenames.count(); i++)
        if (!tableWidget->item(i,5)->text().startsWith(tr("Error"))) {
            completed++;
    }

    tableWidget->setSortingEnabled(true); // so you can browse through errors etc
    QString donemessage = QString(tr("Import Complete. %1 of %2 successful."))
                                  .arg(completed, 1, 10, zero)
                                  .arg(filenames.count(), 1, 10, zero);
    progressBar->setValue(progressBar->maximum());
    phaseLabel->setText(donemessage);
    abortButton->setText(tr("Finish"));
    aborted = false;
}

// destructor - not sure it ever gets called tho, even by RideImportWizard::done()
RideImportWizard::~RideImportWizard()
{
    // Fill in the filenames and all the textItems
    for (int i=0; i < filenames.count(); i++) {
        QTableWidgetItem *t;
        t = tableWidget->item(i,0); delete t;
        t = tableWidget->item(i,1); delete t;
        t = tableWidget->item(i,2); delete t;
        t = tableWidget->item(i,3); delete t;
        t = tableWidget->item(i,4); delete t;
        t = tableWidget->item(i,5); delete t;
    }
    delete tableWidget;
    filenames.clear();
    delete phaseLabel;
    delete progressBar;
    delete abortButton;
    delete cancelButton;
    delete todayButton;
    delete overFiles;
}


// Below is the code to implement a custom ItemDelegate to support
// editing of the date and time of the Ride inside a QTableWidget
// the ItemDelegate is registered for every cell in the table
// and only kicks into life for the columns in which the date and
// time are stored, and only when the DateTime for that row is 0.

// construct the delegate - save away the column containing the date
//                          bearing in mind the time is in the next
//                          column.
RideDelegate::RideDelegate(int dateColumn, QObject *parent) : QItemDelegate(parent)
{
    this->dateColumn = dateColumn;
}


// paint the cells for date and time - pass through for all the
// other cells. Uses the model data to display.
void RideDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    if (index.column() == dateColumn) {

        QString value = index.model()->data(index, Qt::DisplayRole).toString();
        // display with angles to show its doing something
        QString text = QString("%1").arg(value);

        QStyleOptionViewItem myOption = option;
        myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
        drawDisplay(painter, myOption, myOption.rect, text);
        drawFocus(painter, myOption, myOption.rect);

    } else if (index.column() == dateColumn+1) {

        QString value = index.model()->data(index, Qt::DisplayRole).toString();
        // display with angles to show its doing something
        QString text = QString("%1").arg(value);

        QStyleOptionViewItem myOption = option;
        myOption.displayAlignment = Qt::AlignRight | Qt::AlignVCenter;
        drawDisplay(painter, myOption, myOption.rect, text);
        drawFocus(painter, myOption, myOption.rect);

    } else {

        QItemDelegate::paint(painter, option, index);

    }
}

// setup editor for edit of field!!
QWidget *RideDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == dateColumn) {

        // edit that date!
        QDateEdit *dateEdit = new QDateEdit(parent);
        dateEdit->setDisplayFormat(tr("dd MMM yyyy"));
        connect(dateEdit, SIGNAL(editingFinished()), this, SLOT(commitAndCloseDateEditor()));
        return dateEdit;
    } else if (index.column() == dateColumn+1) {

        // edit that time
        QTimeEdit *timeEdit = new QTimeEdit(parent);
        timeEdit->setDisplayFormat("hh:mm:ss a");
        connect(timeEdit, SIGNAL(editingFinished()), this, SLOT(commitAndCloseTimeEditor()));
        return timeEdit;
    } else {
        return QItemDelegate::createEditor(parent, option, index);
    }
}

// user hit tab or return so save away the data to our model
void RideDelegate::commitAndCloseDateEditor()
{
    QDateEdit *editor = qobject_cast<QDateEdit *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

// user hit tab or return so save away the data to our model
void RideDelegate::commitAndCloseTimeEditor()
{
    QTimeEdit *editor = qobject_cast<QTimeEdit *>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}

// We don't set anything because the data is saved within the view not the model!
void RideDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
   // stored as text field
    if (index.column() == dateColumn) {
        QDateEdit *dateEdit = qobject_cast<QDateEdit *>(editor);
        QDate date = QDate().fromString(index.model()->data(index, Qt::DisplayRole).toString(), tr("dd MMM yyyy"));
        dateEdit->setDate(date);
    } else if (index.column() == dateColumn+1) {
        QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(editor);
        QTime time = QTime().fromString(index.model()->data(index, Qt::DisplayRole).toString(), "hh:mm:ss a");;
        timeEdit->setTime(time);
    }
}

// We don't set anything because the data is saved within the view not the model!
void RideDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{

    // stored as text field
    if (index.column() == dateColumn) {
        QDateEdit *dateEdit = qobject_cast<QDateEdit *>(editor);
        QString value = dateEdit->date().toString(tr("dd MMM yyyy"));
        // Place in the view
        model->setData(index, value, Qt::DisplayRole);
    } else if (index.column() == dateColumn+1) {
        QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(editor);
        QString value = timeEdit->time().toString("hh:mm:ss a");
        model->setData(index, value, Qt::DisplayRole);
    }
}

