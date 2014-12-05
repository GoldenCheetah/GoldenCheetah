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

#include <QDebug>
#include "MainWindow.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideImportWizard.h"
#include "Context.h"
#include "Athlete.h"
#include "QuarqRideFile.h"
#include <QWaitCondition>
#include "Settings.h"
#include "Units.h"
#include "GcRideFile.h"
#include "JsonRideFile.h"
#include "TcxRideFile.h"
#include "MetricAggregator.h"
#include "RideAutoImportConfig.h"


// drag and drop passes urls ... convert to a list of files and call main constructor
RideImportWizard::RideImportWizard(QList<QUrl> *urls, Context *context, QWidget *parent) : QDialog(parent), context(context)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    QList<QString> filenames;
    for (int i=0; i<urls->count(); i++)
        filenames.append(QFileInfo(urls->value(i).toLocalFile()).absoluteFilePath());
    autoImportMode = false;
    init(filenames, context);
    filenames.clear();
}

RideImportWizard::RideImportWizard(QList<QString> files, Context *context, QWidget *parent) : QDialog(parent), context(context)
{
    autoImportMode = false;
    init(files, context);
}


RideImportWizard::RideImportWizard(RideAutoImportConfig *dirs, Context *context, QWidget *parent) : QDialog(parent), context(context), importConfig(dirs)
{
    autoImportMode = true;
    QList<QString> files;

    // get the directories
    QList<RideAutoImportRule> rules = importConfig->getConfig();

    // prepare the widget to show the status of the directory
    directoryWidget = new QTableWidget(rules.count(), 3, this);

    directoryWidget->verticalHeader()->setDefaultSectionSize(20);

    QTableWidgetItem *directoryHeading = new QTableWidgetItem;
    directoryHeading->setText(tr("Directory"));
    directoryWidget->setHorizontalHeaderItem(0, directoryHeading);

    QTableWidgetItem *importRuleHeading = new QTableWidgetItem;
    importRuleHeading->setText(tr("Import Rule"));
    directoryWidget->setHorizontalHeaderItem(1, importRuleHeading);

    QTableWidgetItem *statusHeading = new QTableWidgetItem;
    statusHeading->setText(tr("Directory Status"));
    directoryWidget->setHorizontalHeaderItem(2, statusHeading);

    // and get the allowed files formats
    const RideFileFactory &rff = RideFileFactory::instance();
    QStringList suffixList = rff.suffixes();
    suffixList.replaceInStrings(QRegExp("^"), "*.");
    QStringList allFormats;
    foreach(QString suffix, rff.suffixes())
        allFormats << QString("*.%1").arg(suffix);

    // Fill in the directory names and importRuleStatus
    int i=-1;
    foreach (RideAutoImportRule rule, rules){
        i++; // do it here to allow "continue" - and start with "0"
        QTableWidgetItem *t;

        // Directory
        t = new QTableWidgetItem();
        t->setText(rule.getDirectory());
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        directoryWidget->setItem(i,0,t);

        // Import Rule
        QList<QString> descriptions = rule.getRuleDescriptions();
        t = new QTableWidgetItem();
        t->setText(descriptions.at(rule.getImportRule()));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        directoryWidget->setItem(i,1,t);

        // Import Status
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        directoryWidget->setItem(i,2,t);

        // only add files if configured to do so
        if (rule.getImportRule() == 0) {
            directoryWidget->item(i,2)->setText(tr("No import"));
            continue;
        }

        // do some checks on the directory first
        QString currentImportDirectory = rule.getDirectory();
        if (currentImportDirectory == "") {
            directoryWidget->item(i,2)->setText(tr("No directory"));
            continue;
        }
        QDir *importDir = new QDir (currentImportDirectory);
        if (!importDir->exists()) {    // directory might not be available (USB,..)
            directoryWidget->item(i,2)->setText(tr("Directory not available"));
            continue;
        }
        if (!importDir->isReadable()) {
            directoryWidget->item(i,2)->setText(tr("Directory not readable"));
            continue;
        }

        // now get the files with their full names
        QFileInfoList fileInfos = importDir->entryInfoList(allFormats, QDir::Files, QDir::NoSort);
        if (!fileInfos.isEmpty()) {
            int j = 0;
            foreach(QFileInfo f, fileInfos) {
                files.append(f.absoluteFilePath());
                j++;
            }
            directoryWidget->item(i,2)->setText(tr("%1 files for import selected").arg(QString::number(j)));
          } else {
            directoryWidget->item(i,2)->setText(tr("No activity files found"));
            continue;
        }
    }

    directoryWidget->setColumnWidth(0, 480);
    directoryWidget->setColumnWidth(1, 150);
    directoryWidget->setColumnWidth(2, 250);

    init(files, context);

}


void
RideImportWizard::init(QList<QString> files, Context * /*mainWindow*/)
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
    //overFiles = new QCheckBox(tr("Overwrite Existing Files"));  // deprecate for this release... XXX
    // initially the cancel, overwrite and today widgets are hidden
    // they only appear whilst we are asking the user for dates
    cancelButton->setHidden(true);
    todayButton->setHidden(true);
    //overFiles->setHidden(true);  // deprecate for this release... XXX
    //overwriteFiles = false;

    aborted = false;

    // NOTE: abort button morphs into save and finish button later
    connect(abortButton, SIGNAL(clicked()), this, SLOT(abortClicked()));

    // only used when editing dates
    connect(todayButton, SIGNAL(activated(int)), this, SLOT(todayClicked(int)));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    // connect(overFiles, SIGNAL(clicked()), this, SLOT(overClicked()));  // deprecate for this release... XXX

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

    // save target dir for the file import
    this->homeImports = context->athlete->home->imports();
    this->homeActivities = context->athlete->home->activities();

    // Fill in the filenames and all the textItems
    for (int i=0; i < files.count(); i++) {
        QTableWidgetItem *t;

        filenames.append(QFileInfo(files[i]).canonicalFilePath());
        blanks.append(true); // by default editable

        // Filename
        t = new QTableWidgetItem();
        if (autoImportMode)
            t->setText(QFileInfo(files[i]).canonicalFilePath());
        else
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
    // buttons->addWidget(overFiles); // deprecate for this release... XXX
    buttons->addWidget(cancelButton);
    buttons->addWidget(abortButton);

    QVBoxLayout *contents = new QVBoxLayout(this);
    if (autoImportMode) {
        contents->addWidget(directoryWidget);
        // only show table if files are available in autoimportMode
        if (files.count() == 0) {
            tableWidget->setVisible(false);
            progressBar->setVisible(false);
        }
    }
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
           ((files.count() > 9 && files.count() < 17) ? 8 : 0)),
           118 + ((files.count() > 16 ? 17*20 : (files.count()+1) * 20)
           + ((autoImportMode) ? 100 : 0))); // assume not more the 5 directory in average

    if (autoImportMode) directoryWidget->adjustSize();
    tableWidget->adjustSize();

    // set number of files / so that a caller of the constructor can decide what to do
    numberOfFiles = files.count();
}

int
RideImportWizard::getNumberOfFiles() {
    return numberOfFiles;
}

int
RideImportWizard::process()
{

    // Make visible and put in front prior to running down the list & processing...
    if (!isActiveWindow()) activateWindow();
    this->show();

    // set progress bar limits - for each file we
    // will make 5 passes over the files
    //         1. checking if a file is readable
    //         2. parsing it with the RideFileReader
    //         3. [optional] collect date/time information from user
    //         4. copy file into Library
    //         5. Process for CPI (not implemented yet)

    // So, therefore the progress bar runs from 0 to files*4. (since step 5 is not implemented yet)
    progressBar->setMinimum(0);
    progressBar->setMaximum(filenames.count()*4);
    if (!isActiveWindow()) activateWindow();


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

    if (aborted) { done(0); }
    if (!isActiveWindow()) activateWindow();
    repaint();
    QApplication::processEvents();

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
              if (!isActiveWindow()) activateWindow();
              this->repaint();
              QApplication::processEvents();

              QList<RideFile*> rides;
              RideFile *ride = RideFileFactory::instance().openRideFile(context, thisfile, errors, &rides);

              // is this an archive of files?
              if (rides.count() > 1) {

                 int here = i;

                 // remove current filename from state arrays and tableview
                 filenames.removeAt(here);
                 blanks.removeAt(here);
                 tableWidget->removeRow(here);

                 // resize dialog according to the number of rows we expect
                 int willhave = filenames.count() + rides.count();
                 resize(920 + ((willhave > 16 ? 24 : 0) +
                     ((willhave > 9 && willhave < 17) ? 8 : 0)),
                     118 + ((willhave > 16 ? 17*20 : (willhave+1) * 20)));


                 // ok so create a temporary file and add to the tableWidget
                 int counter = 0;
                 foreach(RideFile *extracted, rides) {

                     // write as a temporary file, using the original
                     // filename with "-n" appended
                     QString fulltarget = QDir::tempPath() + "/" + QFileInfo(thisfile).baseName() + QString("-%1.tcx").arg(counter+1);
                     TcxFileReader reader;
                     QFile target(fulltarget);
                     reader.writeRideFile(context, extracted, target);
                     deleteMe.append(fulltarget);
                     delete extracted;
                     
                     // now add each temporary file ...
                     filenames.insert(here, fulltarget);
                     blanks.insert(here, true); // by default editable
                     tableWidget->insertRow(here+counter);

                     QTableWidgetItem *t;

                     // Filename
                     t = new QTableWidgetItem();
                     t->setText(fulltarget);
                     t->setFlags(t->flags() & (~Qt::ItemIsEditable));
                     tableWidget->setItem(here+counter,0,t);

                     // Date
                     t = new QTableWidgetItem();
                     t->setText(tr(""));
                     t->setFlags(t->flags()  | Qt::ItemIsEditable);
                     t->setBackgroundColor(Qt::red);
                     tableWidget->setItem(here+counter,1,t);

                     // Time
                     t = new QTableWidgetItem();
                     t->setText(tr(""));
                     t->setFlags(t->flags() | Qt::ItemIsEditable);
                     tableWidget->setItem(here+counter,2,t);

                     // Duration
                     t = new QTableWidgetItem();
                     t->setText(tr(""));
                     t->setFlags(t->flags() & (~Qt::ItemIsEditable));
                     tableWidget->setItem(here+counter,3,t);

                     // Distance
                     t = new QTableWidgetItem();
                     t->setText(tr(""));
                     t->setFlags(t->flags() & (~Qt::ItemIsEditable));
                     tableWidget->setItem(here+counter,4,t);

                     // Import Status
                     t = new QTableWidgetItem();
                     t->setText(tr(""));
                     t->setFlags(t->flags() & (~Qt::ItemIsEditable));
                     tableWidget->setItem(here+counter,5,t);

                     counter++;

                     tableWidget->adjustSize();
                 }
                 QApplication::processEvents();


                 // progress bar needs to adjust...
                 progressBar->setMaximum(filenames.count()*4);

                 // then go back one and re-parse from there
                 rides.clear();
   
                 i--;
                 goto next; // buttugly I know, but count em across 100,000 lines of code

              }

              // did it parse ok?
              if (ride) {

                   // ride != NULL but !errors.isEmpty() means they're just warnings
                   if (errors.isEmpty())
                       tableWidget->item(i,5)->setText(tr("Validated"));
                   else {
                       tableWidget->item(i,5)->setText(tr("Warning - ") + errors.join(tr(" ")));
                   }

                   // Set Date and Time
                   if (ride->startTime().isNull()) {

                       // Poo. The user needs to supply the date/time for this ride
                       blanks[i] = true;
                       tableWidget->item(i,1)->setText(tr(""));
                       tableWidget->item(i,2)->setText(tr(""));

                   } else {

                       // Cool, the date and time was extracted from the source file
                       blanks[i] = false;
                       tableWidget->item(i,1)->setText(ride->startTime().toString(tr("dd MMM yyyy")));
                       tableWidget->item(i,2)->setText(ride->startTime().toString("hh:mm:ss"));
                   }

                   tableWidget->item(i,1)->setTextAlignment(Qt::AlignRight); // put in the middle
                   tableWidget->item(i,2)->setTextAlignment(Qt::AlignRight); // put in the middle

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
                   QString dist = context->athlete->useMetricUnits
                       ? QString ("%1 km").arg(km, 0, 'f', 1)
                       : QString ("%1 mi").arg(km * MILES_PER_KM, 0, 'f', 1);
                   tableWidget->item(i,4)->setText(dist);
                   tableWidget->item(i,4)->setTextAlignment(Qt::AlignRight); // put in the middle

                   delete ride;
               } else {
                   // nope - can't handle this file
                   tableWidget->item(i,5)->setText(tr("Error - ") + errors.join(tr(" ")));
               }
        }
        progressBar->setValue(progressBar->value()+1);
        QApplication::processEvents();
        if (aborted) { done(0); }
        if (!isActiveWindow()) activateWindow();
        this->repaint();

        next:;
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
        if (!isActiveWindow()) activateWindow();
        progressBar->repaint();
   }
   // get it on top
   activateWindow();

   // Wait for user to press save
   abortButton->setText(tr("Save"));
   aborted = false;

   cancelButton->setHidden(false);
   todayButton->setHidden(false);
   //overFiles->setHidden(false); // deprecate for this release... XXX

   if (needdates == 0) {
      // no need to wait for the user to input dates
      // and press save if all the dates are set
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

   // in autoimport mode / no files, skip "Save",... and goto "Finish"
   if (autoImportMode && filenames.count()== 0) {
       cancelButton->setHidden(true);
       todayButton->setHidden(true);
       abortButton->setDisabled(false);
       phaseLabel->setText(tr("No files for automatic import selected."));
       abortButton->setText(tr("Finish"));
       aborted = false;
   }

   return 0;
}

//void
//RideImportWizard::overClicked()
//{
    //overwriteFiles = overFiles->isChecked(); //deprecate in this release XXX
//}

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

            QTime duration = QTime().fromString(tableWidget->item(i,3)->text(), "hh:mm:ss");
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
            QTime duration = QTime().fromString(tableWidget->item(i,3)->text(), "hh:mm:ss");

            // ride start time
            QTime time(rstart/3600, rstart%3600/60, rstart%60);
            tableWidget->item(i,2)->setText(time.toString("hh:mm:ss"));
            rstart += duration.hour() * 3600 +
                      duration.minute() * 60 +
                      duration.second();
        }
    }
    // phew! - repaint!
    QApplication::processEvents();
    if (!isActiveWindow()) activateWindow();
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
        hide();
        context->athlete->isclean = false;
        context->athlete->metricDB->refreshMetrics();
        aborted=true; // terminated. I'll be back.
        return;
    }

    if (label == tr("Finish")) {
       // phew. our work is done. -- lets force an update stats...
       hide();
       context->athlete->isclean = false;
       context->athlete->metricDB->refreshMetrics();
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
    //overFiles->setHidden(true);  // deprecate for this release XXX

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
        if (!isActiveWindow()) activateWindow();
        this->repaint();

        // serialize the file to .JSON format and copy the source file to the "/imports" directory
        // add the date/time of the target to the source file name (for identification)

        // Prepare the new file names - Setup the ridetime as a QDateTime
        QDateTime ridedatetime = QDateTime(QDate().fromString(tableWidget->item(i,1)->text(), tr("dd MMM yyyy")),
                                 QTime().fromString(tableWidget->item(i,2)->text(), "hh:mm:ss"));
        QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                               .arg ( ridedatetime.date().year(), 4, 10, zero )
                               .arg ( ridedatetime.date().month(), 2, 10, zero )
                               .arg ( ridedatetime.date().day(), 2, 10, zero )
                               .arg ( ridedatetime.time().hour(), 2, 10, zero )
                               .arg ( ridedatetime.time().minute(), 2, 10, zero )
                               .arg ( ridedatetime.time().second(), 2, 10, zero );

        // file name for the .JSON in /activities directory
        QString activitiesTarget = QString ("%1.%2" )
                               .arg ( targetnosuffix )
                               .arg ( "json" );
        QString activitiesFulltarget = homeActivities.canonicalPath() + "/" + activitiesTarget;

        // file name for the copy to /imports directory
        bool reimport = false;  // check if file is taken from /imports or /downloads - then no need to rename or copy
        QString importsFulltarget = "";
        QString importsTarget = "";
        QFileInfo importsFile (filenames[i]);
        if (importsFile.canonicalFilePath() == (homeImports.canonicalPath() + "/" + targetnosuffix + "." + importsFile.suffix())) {
            reimport = true;
        } else {
            reimport = false;
            // add the GC file base name to create unique file names during import
            // there should not be 2 ride files with exactly the same time stamp (as this is also not foreseen for the .json)
            importsTarget = importsFile.baseName() + "_" + targetnosuffix + "." + importsFile.suffix();
            importsFulltarget = homeImports.canonicalPath() + "/" + importsTarget;
        }


        // check if a ride at this point of time already exists in /activities
        if (QFileInfo(activitiesFulltarget).exists()) {
            tableWidget->item(i,5)->setText(tr("Error - Activity file exists"));
        } else {

            // First copy of source (if required) then create .JSON (in case of error the last error will be shown)
            // so start wih the less the less critical part first

            tableWidget->item(i,5)->setText(tr("Saving file..."));
            if (!reimport) {
                // copy the source file to /imports with adjusted name
                QFile source(filenames[i]);
                if (!source.copy(importsFulltarget)) {
                    tableWidget->item(i,5)->setText(tr("Error - copy of %1 to import directory failed").arg(importsTarget));
                }
            }

            // serialize the file to .JSON
            QStringList errors;
            QFile thisfile(filenames[i]);
            RideFile *ride(RideFileFactory::instance().openRideFile(context, thisfile, errors));

            // update ridedatetime and set the Source File name
            ride->setStartTime(ridedatetime);
            ride->setTag("Source Filename", importsTarget);

            // serialize
            JsonFileReader reader;
            QFile target(activitiesFulltarget);
            if (reader.writeRideFile(context, ride, target)) {

                tableWidget->item(i,5)->setText(tr("File Saved"));
                context->athlete->addRide(QFileInfo(activitiesFulltarget).fileName(),
                                          tableWidget->rowCount() < 20 ? true : false); // don't signal if mass importing
            }  else {
                tableWidget->item(i,5)->setText(tr("Error - .JSON creation failed"));
            }
            // clear
            delete ride;
        }

        QApplication::processEvents();
        if (aborted) { done(0); }
        progressBar->setValue(progressBar->value()+1);
        if (!isActiveWindow()) activateWindow();
        this->repaint();
    }

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


// clean up files
RideImportWizard::~RideImportWizard()
{
    foreach(QString name, deleteMe) QFile(name).remove();
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
        timeEdit->setDisplayFormat("hh:mm:ss");
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
        QTime time = QTime().fromString(index.model()->data(index, Qt::DisplayRole).toString(), "hh:mm:ss");;
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
        QString value = timeEdit->time().toString("hh:mm:ss");
        model->setData(index, value, Qt::DisplayRole);
    }
}

