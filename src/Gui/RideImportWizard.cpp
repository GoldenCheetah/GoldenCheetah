/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
 * Additions Copyright (c) 2021 Paul Johnson (paulj49457@gmail.com)
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

#include "Context.h"
#include "Athlete.h"
#include "MainWindow.h"

#include "ArchiveFile.h"

#include "RideItem.h"
#include "RideFile.h"
#include "RideImportWizard.h"
#include "RideCache.h"

#include "RideAutoImportConfig.h"
#include "HelpWhatsThis.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"

#include "GcRideFile.h"
#include "JsonRideFile.h"
#include "TcxRideFile.h" // for opening multi-ride file
#include "DataProcessor.h"
#include "RideMetadata.h" // for linked defaults processing

#include <QDebug>
#include <QWaitCondition>
#include <QMessageBox>

ImportFile::ImportFile() :
    activityFullSource(""), overwriteableFile(false),
    copyOnImport(COPY_ON_IMPORT) {}

ImportFile::ImportFile(const QString& fileName) :
    activityFullSource(fileName), overwriteableFile(false),
    copyOnImport(COPY_ON_IMPORT) {}


importCopyType ImportFile::stringToCopyType(const QString& importString)
{
    if (importString == "Yes") {
        return COPY_ON_IMPORT;
    }
    else if (importString == "Exclude") {
        return EXCLUDED_FROM_IMPORT;
    }
    else {
        qDebug() << "ImportFile::stringToCopyType - conversion failed for: " << importString;
        // default to standard behaviour
        return COPY_ON_IMPORT;
    }
}

QString ImportFile::copyTypeToString(const importCopyType importCopy)
{
    switch (importCopy) {
    case COPY_ON_IMPORT:
        return "Yes";
    case EXCLUDED_FROM_IMPORT:
        return "Exclude";
    default:
        qDebug() << "ImportFile::copyTypeToString - conversion failed for: " << importCopy;
        // No obvious behaviour as a default, so return "Error"
        return QString("Error");
    }
}

enum WizardTable {
    FILENAME_COLUMN = 0,
    DATE_COLUMN,
    TIME_COLUMN,
    DURATION_COLUMN,
    DISTANCE_COLUMN,
    COPY_ON_IMPORT_COLUMN,
    STATUS_COLUMN,
    TOTAL_NUM_COLUMNS,
};

enum WizardTable1 {
    DIRECTORY_RULE_COLUMN = 0,
    IMPORT_RULE_COLUMN,
    DIRECTORY_STATUS_RULE_COLUMN,
    TOTAL_NUM_RULE_COLUMNS,
};

// drag and drop passes urls ... convert to a list of files and call main constructor
RideImportWizard::RideImportWizard(QList<QUrl> *urls, Context *context, QWidget *parent) : QDialog(parent), context(context)
{
    _importInProcess = true;
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    QList<ImportFile> filesToImport;
    for (int i=0; i<urls->count(); i++)
        filesToImport.append(ImportFile(QFileInfo(urls->value(i).toLocalFile()).absoluteFilePath()));
    autoImportMode = false;
    autoImportStealth = false;
    init(filesToImport, context);
    _importInProcess = false;

}

RideImportWizard::RideImportWizard(QList<QString> files, Context *context, QWidget *parent) : QDialog(parent), context(context)
{
    _importInProcess = true;
    setAttribute(Qt::WA_DeleteOnClose);
    autoImportMode = false;
    autoImportStealth = false;

    QList<ImportFile> filesToImport;
    for (int i = 0; i < files.count(); i++)
        filesToImport.append(ImportFile(files.at(i)));

    init(filesToImport, context);
    _importInProcess = false;

}


RideImportWizard::RideImportWizard(RideAutoImportConfig *dirs, Context *context, QWidget *parent) : QDialog(parent), context(context), importConfig(dirs)
{
    _importInProcess = true;
    autoImportMode = true;
    autoImportStealth = true;

    if (autoImportStealth) hide();

    QList<ImportFile> files;

    // get the directories & rules
    QList<RideAutoImportRule> rules = importConfig->getConfig();

    // prepare the widget to show the status of the directory
    directoryWidget = new QTableWidget(rules.count(), TOTAL_NUM_RULE_COLUMNS, this);

    directoryWidget->verticalHeader()->setDefaultSectionSize(20 *dpiYFactor);

    QTableWidgetItem *directoryHeading = new QTableWidgetItem;
    directoryHeading->setText(tr("Directory"));
    directoryWidget->setHorizontalHeaderItem(DIRECTORY_RULE_COLUMN, directoryHeading);

    QTableWidgetItem *importRuleHeading = new QTableWidgetItem;
    importRuleHeading->setText(tr("Import Rule"));
    directoryWidget->setHorizontalHeaderItem(IMPORT_RULE_COLUMN, importRuleHeading);

    QTableWidgetItem *statusHeading = new QTableWidgetItem;
    statusHeading->setText(tr("Directory Status"));
    directoryWidget->setHorizontalHeaderItem(DIRECTORY_STATUS_RULE_COLUMN, statusHeading);

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
        directoryWidget->setItem(i,DIRECTORY_RULE_COLUMN,t);

        // Import Rule
        QList<QString> descriptions = rule.getRuleDescriptions();
        t = new QTableWidgetItem();
        t->setText(descriptions.at(rule.getImportRule()));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        directoryWidget->setItem(i,IMPORT_RULE_COLUMN,t);

        // Import Status
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        directoryWidget->setItem(i,DIRECTORY_STATUS_RULE_COLUMN,t);

        // only add files if configured to do so
        if (rule.getImportRule() == RideAutoImportRule::noImport) {
            directoryWidget->item(i,DIRECTORY_STATUS_RULE_COLUMN)->setText(tr("No import"));
            continue;
        }
        // do some checks on the directory first
        QString currentImportDirectory = rule.getDirectory();
        if (currentImportDirectory == "") {
            directoryWidget->item(i,DIRECTORY_STATUS_RULE_COLUMN)->setText(tr("No directory"));
            continue;
        }
        QDir *importDir = new QDir (currentImportDirectory);
        if (!importDir->exists()) {    // directory might not be available (USB,..)
            directoryWidget->item(i,DIRECTORY_STATUS_RULE_COLUMN)->setText(tr("Directory not available"));
            continue;
        }
        if (!importDir->isReadable()) {
            directoryWidget->item(i,DIRECTORY_STATUS_RULE_COLUMN)->setText(tr("Directory not readable"));
            continue;
        }

        // determine timerange in the past which should considerd in import
        QDate selectAfter = QDate::currentDate();
        switch(rule.getImportRule()) {
        case RideAutoImportRule::importLast90days:
        case RideAutoImportRule::importBackground90:
            selectAfter = selectAfter.addDays(Q_INT64_C(-90));
            break;
        case RideAutoImportRule::importLast180days:
        case RideAutoImportRule::importBackground180:
            selectAfter = selectAfter.addDays(Q_INT64_C(-180));
            break;
        case RideAutoImportRule::importLast360days:
        case RideAutoImportRule::importBackground360:
            selectAfter = selectAfter.addDays(Q_INT64_C(-360));
            break;
        }

        // if any of the rules says "with Dialog" then we keep the dialog - if not it's stealth
        switch (rule.getImportRule()) {

        case RideAutoImportRule::importAll:
        case RideAutoImportRule::importLast90days:
        case RideAutoImportRule::importLast180days:
        case RideAutoImportRule::importLast360days:
            autoImportStealth = false;
            break;
        }

        // now get the files with their full names      
        QFileInfoList fileInfos = importDir->entryInfoList(allFormats, QDir::Files, QDir::NoSort);
        if (!fileInfos.isEmpty()) {
            int j = 0;
            foreach(QFileInfo f, fileInfos) {
                // append following the import rules
                switch (rule.getImportRule()) {
                case RideAutoImportRule::importAll:
                case RideAutoImportRule::importBackgroundAll:
                    files.append(ImportFile(f.absoluteFilePath()));
                    j++;
                    break;
                case RideAutoImportRule::importLast90days:
                case RideAutoImportRule::importLast180days:
                case RideAutoImportRule::importLast360days:
                case RideAutoImportRule::importBackground90:
                case RideAutoImportRule::importBackground180:
                case RideAutoImportRule::importBackground360:
                    if (f.created().date() >= selectAfter || f.lastModified().date() >= selectAfter) {
                        files.append(ImportFile(f.absoluteFilePath()));
                        j++;
                    };
                    break;
                }
            }
            if (j > 0) {
                directoryWidget->item(i, DIRECTORY_STATUS_RULE_COLUMN)->setText(tr("%1 files for import selected").arg(QString::number(j)));
            } else {
                directoryWidget->item(i, DIRECTORY_STATUS_RULE_COLUMN)->setText(tr("No files in requested time range"));
            }
        } else {
            directoryWidget->item(i, DIRECTORY_STATUS_RULE_COLUMN)->setText(tr("No activity files found"));
            continue;
        }
    }

    directoryWidget->setColumnWidth(DIRECTORY_RULE_COLUMN, 480 *dpiXFactor);
    directoryWidget->setColumnWidth(IMPORT_RULE_COLUMN, 240 *dpiXFactor);
    directoryWidget->setColumnWidth(DIRECTORY_STATUS_RULE_COLUMN, 200 * dpiXFactor);

    init(files, context);

    _importInProcess = false;


}

void
RideImportWizard::init(QList<ImportFile> original, Context * /*mainWindow*/)
{
    filenames.clear();

    // expand files if they are archives - this may involve unzipping or extracting
    //                                     files into a subdirectory, so we also clean-up
    //                                     before we close.
    QList<ImportFile> files = expandFiles(original);

    // setup Help
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Activity_Import));

    // initialise dialog box
    tableWidget = new QTableWidget(files.count(), TOTAL_NUM_COLUMNS, this);

    tableWidget->setItemDelegate(new RideDelegate(DATE_COLUMN)); // use a delegate for column 1 date
    tableWidget->verticalHeader()->setDefaultSectionSize(20 *dpiXFactor);
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
    cancelButton->setAutoDefault(false);
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
    setWindowTitle(tr("Import Files"));
    QTableWidgetItem *filenameHeading = new QTableWidgetItem;
    filenameHeading->setText(tr("Filename"));
    tableWidget->setHorizontalHeaderItem(FILENAME_COLUMN, filenameHeading);

    QTableWidgetItem *dateHeading = new QTableWidgetItem;
    dateHeading->setText(tr("Date"));
    tableWidget->setHorizontalHeaderItem(DATE_COLUMN, dateHeading);

    QTableWidgetItem *timeHeading = new QTableWidgetItem;
    timeHeading->setText(tr("Time"));
    tableWidget->setHorizontalHeaderItem(TIME_COLUMN, timeHeading);

    QTableWidgetItem *durationHeading = new QTableWidgetItem;
    durationHeading->setText(tr("Duration"));
    tableWidget->setHorizontalHeaderItem(DURATION_COLUMN, durationHeading);

    QTableWidgetItem *distanceHeading = new QTableWidgetItem;
    distanceHeading->setText(tr("Distance"));
    tableWidget->setHorizontalHeaderItem(DISTANCE_COLUMN, distanceHeading);
    
    QTableWidgetItem* copyOnImportHeading = new QTableWidgetItem;
    copyOnImportHeading->setText(tr("Import"));
    tableWidget->setHorizontalHeaderItem(COPY_ON_IMPORT_COLUMN, copyOnImportHeading);

    QTableWidgetItem *statusHeading = new QTableWidgetItem;
    statusHeading->setText(tr("Import Status"));
    tableWidget->setHorizontalHeaderItem(STATUS_COLUMN, statusHeading);

    // save target dir for the file import
    this->homeImports = context->athlete->home->imports();
    this->homeActivities = context->athlete->home->activities();
    this->tmpActivities = context->athlete->home->tmpActivities();

    // Fill in the filenames and all the textItems
    for (int i=0; i < files.count(); i++) {
        QTableWidgetItem *t;

        filenames.append(ImportFile(QFileInfo(files[i].activityFullSource).canonicalFilePath()));
        blanks.append(true); // by default editable

        // Filename
        t = new QTableWidgetItem();
        if (autoImportMode)
            t->setText(QFileInfo(files[i].activityFullSource).canonicalFilePath());
        else
            t->setText(QFileInfo(files[i].activityFullSource).fileName());
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        tableWidget->setItem(i,FILENAME_COLUMN,t);

        // Date
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags()  | Qt::ItemIsEditable);
        t->setBackgroundColor(Qt::red);
        tableWidget->setItem(i,DATE_COLUMN,t);

        // Time
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() | Qt::ItemIsEditable);
        tableWidget->setItem(i,TIME_COLUMN,t);

        // Duration
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        tableWidget->setItem(i,DURATION_COLUMN,t);

        // Distance
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        tableWidget->setItem(i,DISTANCE_COLUMN,t);

        // Copy on Import
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() | Qt::ItemIsEditable);
        tableWidget->setItem(i,COPY_ON_IMPORT_COLUMN, t);

        // Import Status
        t = new QTableWidgetItem();
        t->setText(tr(""));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        tableWidget->setItem(i,STATUS_COLUMN,t);
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
    tableWidget->setColumnWidth(FILENAME_COLUMN, 200*dpiXFactor);
    tableWidget->setColumnWidth(DATE_COLUMN, 90*dpiXFactor);
    tableWidget->setColumnWidth(TIME_COLUMN, 90*dpiXFactor);
    tableWidget->setColumnWidth(DURATION_COLUMN, 90*dpiXFactor);
    tableWidget->setColumnWidth(DISTANCE_COLUMN, 70*dpiXFactor);
    tableWidget->setColumnWidth(COPY_ON_IMPORT_COLUMN, 70*dpiXFactor);
    tableWidget->setColumnWidth(STATUS_COLUMN, 250*dpiXFactor);
    tableWidget->horizontalHeader()->setStretchLastSection(true);

    // max height for 16 items and a scrollbar on right if > 16 items
    // for some reason the window is wider for 10-16 items too.
    // someone that understands width/geometry and layouts better
    // than me should clean up the logic here, no doubt using a
    // single call to geometry/hint. But for now it looks good
    // on Leopard, no doubt not so good on Windows
    resize((920 +
           ((files.count() > 16 ? 24 : 0) +
           ((files.count() > 9 && files.count() < 17) ? 8 : 0)))*dpiXFactor,
           (118 + ((files.count() > 16 ? 17*20 : (files.count()+1) * 20)
           + ((autoImportMode) ? 100 : 0)))*dpiYFactor); // assume not more the 5 directory in average

    if (autoImportMode) directoryWidget->adjustSize();
    tableWidget->adjustSize();

    // set number of files / so that a caller of the constructor can decide what to do
    numberOfFiles = files.count();
}

QList<ImportFile>
RideImportWizard::expandFiles(QList<ImportFile> files)
{
    // we keep a list of what we're returning
    QList<ImportFile> expanded;
    QRegExp archives("^(zip|gzip)$",  Qt::CaseInsensitive);

    foreach(ImportFile impFile, files) {

        if (archives.exactMatch(QFileInfo(impFile.activityFullSource).suffix())) {
            // its an archive so lets check - but only to one depth
            // archives that contain archives can get in the sea
            QList<QString> contents = Archive::dir(impFile.activityFullSource);
            if (contents.count() == 0) {
                expanded.append(impFile);
            } else {
                // we need to extract the contents and return those
                QStringList ex = Archive::extract(impFile.activityFullSource, contents, const_cast<AthleteDirectoryStructure*>(context->athlete->directoryStructure())->tmpActivities().absolutePath());
                deleteMe += ex;
                foreach(QString exfile, ex) { expanded.append(ImportFile(exfile)); }
            }
        } else {
            expanded.append(impFile);
        }
    }

    return expanded;
}

int
RideImportWizard::getNumberOfFiles() {
    return numberOfFiles;
}

int
RideImportWizard::process()
{

    // import process is starting
    _importInProcess = true;

    // Make visible and put in front prior to running down the list & processing...
    if (!autoImportStealth) {
        if (!isActiveWindow()) activateWindow();
        this->show();
    }

    // set progress bar limits - for each file we
    // will make 5 passes over the files
    //         1. checking if a file is readable
    //         2. parsing it with the RideFileReader
    //         3. [optional] collect date/time information from user
    //         4. Discard duplicates, excluded files and those with errors, then copy the valid files into Library
    //         5. Process for CPI (not implemented yet)

    // So, therefore the progress bar runs from 0 to files*4. (since step 5 is not implemented yet)
    progressBar->setMinimum(0);
    progressBar->setMaximum(filenames.count()*4);

    // Pass one - Is it valid?
    phaseLabel->setText(tr("Step 1 of 4: Check file permissions"));
    for (int i=0; i < filenames.count(); i++) {

        // get fullpath name for processing
        QFileInfo thisfile(filenames[i].activityFullSource);

        if (!thisfile.exists())  tableWidget->item(i,STATUS_COLUMN)->setText(tr("Error - File does not exist."));
        else if (!thisfile.isFile())  tableWidget->item(i,STATUS_COLUMN)->setText(tr("Error - Not a file."));
        else if (!thisfile.isReadable())  tableWidget->item(i,STATUS_COLUMN)->setText(tr("Error - File is not readable."));
        else if (thisfile.fileName().endsWith("json") && thisfile.fileName().startsWith("{"))  tableWidget->item(i,STATUS_COLUMN)->setText(tr("Error - Opendata summary."));
        else {

            // is it one we understand ?
            QStringList suffixList = RideFileFactory::instance().suffixes();
            QRegExp suffixes(QString("^(%1)$").arg(suffixList.join("|")));
            suffixes.setCaseSensitivity(Qt::CaseInsensitive);

            // strip off gz or zip as openRideFile will sort that for us
            // since some file names contain "." as separator, not only for suffixes,
            // find the file-type suffix in a 2 step approach
            QStringList allNameParts = thisfile.fileName().split(".");
            QString suffix = tr("undefined");
            if (!allNameParts.isEmpty()) {
                if (allNameParts.last().toLower() == "zip" ||
                    allNameParts.last().toLower() == "gz") {
                    // gz/zip are handled by openRideFile
                    allNameParts.removeLast();
                }
                if (!allNameParts.isEmpty()) {
                    suffix = allNameParts.last();
                }
            }

            if (suffixes.exactMatch(suffix)) {

                // Woot. We know how to parse this baby
                tableWidget->item(i,STATUS_COLUMN)->setText(tr("Queued"));

            } else {
                tableWidget->item(i,STATUS_COLUMN)->setText(tr("Error - Unknown file type") + ": " + suffix);
            }
        }
      
        progressBar->setValue(progressBar->value()+1);

    }

    if (aborted) { done(0); return 0; }
    repaint();
    QApplication::processEvents();

    // Pass 2 - Read in with the relevant RideFileReader method

   phaseLabel->setText(tr("Step 2 of 4: Validating Files"));
   for (int i = 0; i < filenames.count(); i++) {

       // if there isn't a problem with the file
       if (tableWidget->item(i, STATUS_COLUMN)->text().startsWith(tr("Error"))) continue;

       // if the file is excluded from importation, then prevent editing 
       if (isFileExcludedFromImportation(i)) {
           QTableWidgetItem* t = tableWidget->item(i, STATUS_COLUMN);
           t->setText(tr("Failed - File is already in the exclusion list in /imports"));
           t = tableWidget->item(i, COPY_ON_IMPORT_COLUMN);
           t->setText(ImportFile::copyTypeToString(filenames[i].copyOnImport).toStdString().c_str());
           t->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
           t->setFlags(t->flags() & (~Qt::ItemIsEditable));
           t = tableWidget->item(i, DATE_COLUMN);
           t->setFlags(t->flags() & (~Qt::ItemIsEditable));
           t = tableWidget->item(i, TIME_COLUMN);
           t->setFlags(t->flags() & (~Qt::ItemIsEditable));
           continue;
       }

       QStringList errors;
       QFile thisfile(filenames[i].activityFullSource);

       tableWidget->item(i, STATUS_COLUMN)->setText(tr("Parsing..."));
       tableWidget->setCurrentCell(i, STATUS_COLUMN);
       QApplication::processEvents();

       if (aborted) { done(0); return 0; }
       this->repaint();
       QApplication::processEvents();

       QList<RideFile*> rides;
       RideFile* ride = RideFileFactory::instance().openRideFile(context, thisfile, errors, &rides);

       // does "thisfile" contain more than one ride?
       if (rides.count() > 1) {

           int here = i;

           // remove current filename from state arrays and tableview
           filenames.removeAt(here);
           blanks.removeAt(here);
           tableWidget->removeRow(here);

           // resize dialog according to the number of rows we expect
           int willhave = filenames.count() + rides.count();
           resize((920 + ((willhave > 16 ? 24 : 0) +
               ((willhave > 9 && willhave < 17) ? 8 : 0))) * dpiXFactor,
               (118 + ((willhave > 16 ? 17 * 20 : (willhave + 1) * 20))) * dpiYFactor);


           // ok so create a temporary file and add to the tableWidget
           // we write as JSON to ensure we don't lose data e.g. XDATA.
           int counter = 0;
           foreach(RideFile * extracted, rides) {

               // write as a temporary file, using the original
               // filename with "-n" appended
               QString fulltarget = QDir::tempPath() + "/" + QFileInfo(thisfile).baseName() + QString("-%1.json").arg(counter + 1);
               JsonFileReader reader;
               QFile target(fulltarget);
               reader.writeRideFile(context, extracted, target);
               deleteMe.append(fulltarget);
               delete extracted;

               // now add each temporary file, with importation the same as its parent file ...
               ImportFile insFile(fulltarget);
               filenames.insert(here, insFile);
               blanks.insert(here, true); // by default editable
               tableWidget->insertRow(here + counter);

               QTableWidgetItem* t;

               // Filename
               t = new QTableWidgetItem();
               t->setText(fulltarget);
               t->setFlags(t->flags() & (~Qt::ItemIsEditable));
               tableWidget->setItem(here + counter, FILENAME_COLUMN, t);

               // Date
               t = new QTableWidgetItem();
               t->setText(tr(""));
               t->setFlags(t->flags() | Qt::ItemIsEditable);
               t->setBackgroundColor(Qt::red);
               tableWidget->setItem(here + counter, DATE_COLUMN, t);

               // Time
               t = new QTableWidgetItem();
               t->setText(tr(""));
               t->setFlags(t->flags() | Qt::ItemIsEditable);
               tableWidget->setItem(here + counter, TIME_COLUMN, t);

               // Duration
               t = new QTableWidgetItem();
               t->setText(tr(""));
               t->setFlags(t->flags() & (~Qt::ItemIsEditable));
               tableWidget->setItem(here + counter, DURATION_COLUMN, t);

               // Distance
               t = new QTableWidgetItem();
               t->setText(tr(""));
               t->setFlags(t->flags() & (~Qt::ItemIsEditable));
               tableWidget->setItem(here + counter, DISTANCE_COLUMN, t);

               // Copy on Import
               t = new QTableWidgetItem();
               t->setText(tr(""));
               t->setFlags(t->flags() | Qt::ItemIsEditable);
               tableWidget->setItem(here + counter, COPY_ON_IMPORT_COLUMN, t);

               // Import Status
               t = new QTableWidgetItem();
               t->setText(tr(""));
               t->setFlags(t->flags() & (~Qt::ItemIsEditable));
               tableWidget->setItem(here + counter, STATUS_COLUMN, t);

               counter++;

               tableWidget->adjustSize();
           }
           QApplication::processEvents();


           // progress bar needs to adjust...
           progressBar->setMaximum(filenames.count() * 4);

           // then go back one and re-parse from there
           rides.clear();

           i--;
           goto next; // buttugly I know, but count em across 100,000 lines of code
                      // Could this uglyness be removed by using "continue", or does the i-- cause issues ?
       }

       // did it parse ok?
       if (ride) {

           // ride != NULL but !errors.isEmpty() means they're just warnings
           if (errors.isEmpty())
               tableWidget->item(i, STATUS_COLUMN)->setText(tr("Validated"));
           else {
               tableWidget->item(i, STATUS_COLUMN)->setText(tr("Warning - ") + errors.join(tr(";")));
           }

           // Set Date and Time
           if (!ride->startTime().isValid()) {

               // Poo. The user needs to supply the date/time for this ride
               blanks[i] = true;
               tableWidget->item(i, DATE_COLUMN)->setText(tr(""));
               tableWidget->item(i, TIME_COLUMN)->setText(tr(""));

           } else {

               // Cool, the date and time was extracted from the source file
               blanks[i] = false;
               tableWidget->item(i, DATE_COLUMN)->setText(ride->startTime().date().toString(Qt::ISODate));
               tableWidget->item(i, TIME_COLUMN)->setText(ride->startTime().toString("hh:mm:ss"));
           }

           tableWidget->item(i, DATE_COLUMN)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // put in the middle
           tableWidget->item(i, TIME_COLUMN)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // put in the middle

           // time and distance from tags (.gc files)
           QMap<QString, QString> lookup;
           lookup = ride->metricOverrides.value("total_distance");
           double km = lookup.value("value", "0.0").toDouble();

           lookup = ride->metricOverrides.value("workout_time");
           int secs = lookup.value("value", "0.0").toDouble();

           // show duration by looking at last data point
           if (!ride->dataPoints().isEmpty() && ride->dataPoints().last() != NULL) {
               if (!secs) secs = ride->dataPoints().last()->secs + ride->recIntSecs();
               if (!km) km = ride->dataPoints().last()->km;
           }

           QChar zero = QLatin1Char('0');
           QString time = QString("%1:%2:%3").arg(secs / 3600, 2, 10, zero)
               .arg(secs % 3600 / 60, 2, 10, zero)
               .arg(secs % 60, 2, 10, zero);
           tableWidget->item(i, DURATION_COLUMN)->setText(time);
           tableWidget->item(i, DURATION_COLUMN)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // put in the middle

           // show distance by looking at last data point
           QString dist = GlobalContext::context()->useMetricUnits
               ? QString("%1 km").arg(km, 0, 'f', 1)
               : QString("%1 mi").arg(km * MILES_PER_KM, 0, 'f', 1);
           tableWidget->item(i, DISTANCE_COLUMN)->setText(dist);
           tableWidget->item(i, DISTANCE_COLUMN)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

           // Add copy on import information  
           tableWidget->item(i, COPY_ON_IMPORT_COLUMN)->setText(ImportFile::copyTypeToString(filenames[i].copyOnImport).toStdString().c_str());
           tableWidget->item(i, COPY_ON_IMPORT_COLUMN)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter); // put in the middle

           delete ride;
       } else {
           // nope - can't handle this file
           tableWidget->item(i, STATUS_COLUMN)->setText(tr("Error - ") + errors.join(tr(";")));
       }
       progressBar->setValue(progressBar->value()+1);
       QApplication::processEvents();
       if (aborted) { done(0); return 0; }
       this->repaint();

       next:;
   }

    // Pass 3 - get missing date and times for imported files
    //         Actually allow us to edit date on ANY ride, we
    //         make sure that the ride date/time is set from
    //         the filename and never from the ride data

    int needdates=0;
    for (int i=0; i<filenames.count(); i++) {

        // ignore errors
        QTableWidgetItem *t = tableWidget->item(i,STATUS_COLUMN);
        if (t->text().startsWith(tr("Error"))) continue;

        // ignore the excluded import files, as these don't need dates or times to be entered by the user.
        if (filenames[i].copyOnImport == importCopyType::EXCLUDED_FROM_IMPORT) continue;

        if (blanks[i]) needdates++; // count the blanks tho -- these MUST be edited

        // does nothing for the moment
        progressBar->setValue(progressBar->value()+1);
        progressBar->repaint();
   }

   // lets make the text more helpful!
   if (needdates) {
     phaseLabel->setText(QString(tr("Step 3 of 4: %1 ride(s) are missing the date and time.")).arg(needdates));
   } else {
     phaseLabel->setText(tr("Step 3 of 4: Change Date/Time or Save to continue."));
   }

   // get it on top to save / correct missing dates
   if (autoImportStealth && needdates > 0) {
       // leave the stealth mode
       this->show();
       activateWindow();
   } else {
       if (!isActiveWindow()) activateWindow();
   }
   // Wait for user to press save
   abortButton->setText(tr("Save"));
   aborted = false;

   cancelButton->setHidden(false);
   todayButton->setHidden(false);
   overFiles->setHidden(false);

   if (needdates == 0) {
      // no need to wait for the user to input dates
      // and press save if all the dates are set
      // (i.e. they got set by the file importers already)
      // do nothing for now since we need to confirm dates
      // and confirm overwrite files rather than importing
      // without user intervention

      abortButton->setDisabled(false);
      activateSave();
      if (autoImportStealth) abortClicked();  // simulate "Save" by User

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
        QTableWidgetItem *t = tableWidget->item(i,STATUS_COLUMN);
        if (t->text().startsWith(tr("Error"))) continue;

       // date needed?
        t = tableWidget->item(i,DATE_COLUMN);
        if (t->text() == "") return;

        // time needed?
        t = tableWidget->item(i,TIME_COLUMN);
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
            if (tableWidget->item(i,FILENAME_COLUMN)->isSelected() ||
                tableWidget->item(i,DATE_COLUMN)->isSelected() ||
                tableWidget->item(i,TIME_COLUMN)->isSelected() ||
                tableWidget->item(i,DURATION_COLUMN)->isSelected() ||
                tableWidget->item(i,DISTANCE_COLUMN)->isSelected() ||
                tableWidget->item(i,COPY_ON_IMPORT_COLUMN)->isSelected() ||
                tableWidget->item(i,STATUS_COLUMN)->isSelected()) {
                tableWidget->editItem(tableWidget->item(i,DATE_COLUMN));
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
        if (tableWidget->item(i,FILENAME_COLUMN)->isSelected() ||
            tableWidget->item(i,DATE_COLUMN)->isSelected() ||
            tableWidget->item(i,TIME_COLUMN)->isSelected() ||
            tableWidget->item(i,DURATION_COLUMN)->isSelected() ||
            tableWidget->item(i,DISTANCE_COLUMN)->isSelected() ||
            tableWidget->item(i,COPY_ON_IMPORT_COLUMN)->isSelected() ||
            tableWidget->item(i,STATUS_COLUMN)->isSelected()) {
            countselected++;

            QTime duration = QTime().fromString(tableWidget->item(i,DURATION_COLUMN)->text(), "hh:mm:ss");
            totalduration += duration.hour() * 3600 +
                             duration.minute() * 60 +
                             duration.second();
        }
    }

    // More than a days worth of rides so can't squeeze into a single day!
    if (totalduration > (24 * 3600)) {
        QMessageBox::warning ( this, tr ( "Invalid Selection" ), tr ( "More than 24hrs of activities to fit into a day" ));
        return;
    }

    // if it is today then start from now - total rides duration
    // if that goes negative then just do the std thing
    int rstart=0;
    if (index == 1) {

        QTime timenow = QTime::currentTime();

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
        if (tableWidget->item(i,FILENAME_COLUMN)->isSelected() ||
            tableWidget->item(i,DATE_COLUMN)->isSelected() ||
            tableWidget->item(i,TIME_COLUMN)->isSelected() ||
            tableWidget->item(i,DURATION_COLUMN)->isSelected() ||
            tableWidget->item(i,DISTANCE_COLUMN)->isSelected() ||
            tableWidget->item(i,COPY_ON_IMPORT_COLUMN)->isSelected() ||
            tableWidget->item(i,STATUS_COLUMN)->isSelected()) {

            // set the date to date selected
            tableWidget->item(i,DATE_COLUMN)->setText(selectedDate.toString(Qt::ISODate));
            // look at rides with missing start time - we need to populate those

            // ride duration
            QTime duration = QTime().fromString(tableWidget->item(i,DURATION_COLUMN)->text(), "hh:mm:ss");

            // ride start time
            QTime time(rstart/3600, rstart%3600/60, rstart%60);
            tableWidget->item(i,TIME_COLUMN)->setText(time.toString("hh:mm:ss"));
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
    // NOTE: abort button morphs into save and finish button later - so all 3 variants are processed here

    // if done when labelled abort we kill off this dialog
    // & removed to avoid issues with kde AutoCheckAccelerators
    QString label = QString(abortButton->text()).replace("&", "");

    // Process "ABORT"
    if (label == tr("Abort")) {
        hide();
        aborted=true; // terminated. I'll be back.
        return;
    }

    // Process "FINISH"
    if (label == tr("Finish")) {
       // phew. our work is done. -- lets force an update stats...
       hide();
       if (autoImportStealth) {
           // inform the user that the work is done
           QMessageBox::information(NULL, tr("Auto Import"), tr("Automatic import from defined directories is completed."));
       }
       done(0);
       return;
    }

    // Process "SAVE"

    // SAVE STEP 2 - set the labels and make the text un-editable
    phaseLabel->setText(tr("Step 4 of 4: Save to Library"));

    abortButton->setText(tr("Abort"));
    aborted = false;
    cancelButton->setHidden(true);
    todayButton->setHidden(true);
    overFiles->setHidden(true);

    // now set this fields uneditable again ... yeesh.
    for (int i=0; i <filenames.count(); i++) {
        QTableWidgetItem *t = tableWidget->item(i,DATE_COLUMN);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        t = tableWidget->item(i,TIME_COLUMN);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        t = tableWidget->item(i, COPY_ON_IMPORT_COLUMN);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
    }

    QChar zero = QLatin1Char ( '0' );

    // Count the number of successfully imported files

    int excludedImports = 0;
    int overwrittenFiles = 0;
    int alreadyExists = 0;
    int importsErrors = 0;
    QList <ImportFile> validFilenames;

    // SAVE STEP 3A - one-by-one find all the files that should be imported
    for (int i = 0; i < filenames.count(); i++) {

        QApplication::processEvents();
        if (aborted) { done(0); return; }
        this->repaint();

        if (tableWidget->item(i, STATUS_COLUMN)->text().startsWith(tr("Error"))) {
            tableWidget->item(i, COPY_ON_IMPORT_COLUMN)->setText(tr("Not Copied"));
            importsErrors++;
            progressBar->setValue(progressBar->value()+1);
            continue; // skip errors
        } 

        filenames[i].copyOnImport = ImportFile::stringToCopyType(tableWidget->item(i, COPY_ON_IMPORT_COLUMN)->text());

        if (filenames[i].copyOnImport == EXCLUDED_FROM_IMPORT) {

            addFileToImportExclusionList(filenames[i].activityFullSource);
            tableWidget->item(i, STATUS_COLUMN)->setText(tr("Failed - File in the exclusion list in /imports"));
            excludedImports++;
            progressBar->setValue(progressBar->value()+1);
            continue; // skip file requested to be excluded by the user
        }

        tableWidget->item(i, STATUS_COLUMN)->setText(tr("Saving..."));
        tableWidget->setCurrentCell(i, STATUS_COLUMN);

        // prepare the new file names for the next steps - basic name and .JSON in GC format

        QDateTime ridedatetime = QDateTime(QDate().fromString(tableWidget->item(i, DATE_COLUMN)->text(), Qt::ISODate),
                                           QTime().fromString(tableWidget->item(i, TIME_COLUMN)->text(), "hh:mm:ss"));
        QString targetNoSuffix = QString("%1_%2_%3_%4_%5_%6")
            .arg(ridedatetime.date().year(), 4, 10, zero)
            .arg(ridedatetime.date().month(), 2, 10, zero)
            .arg(ridedatetime.date().day(), 2, 10, zero)
            .arg(ridedatetime.time().hour(), 2, 10, zero)
            .arg(ridedatetime.time().minute(), 2, 10, zero)
            .arg(ridedatetime.time().second(), 2, 10, zero);
        QString activitiesTarget = QString("%1.%2").arg(targetNoSuffix).arg("json");

        // create filenames incl. directory path for GC .JSON for both /tmpActivities and /activities directory
        QString tmpActivitiesFulltarget = tmpActivities.canonicalPath() + "/" + activitiesTarget;
        QString finalActivitiesFulltarget = homeActivities.canonicalPath() + "/" + activitiesTarget;

        ImportFile imFile;
        
        // check if a ride at this point of time already exists in /activities - if yes, skip import if overwrite isn't selected
        if (QFileInfo(finalActivitiesFulltarget).exists()) {
            if (!(imFile.overwriteableFile = overwriteFiles)) {
                tableWidget->item(i, STATUS_COLUMN)->setText(tr("Failed - Activity already exists in /activities"));
                tableWidget->item(i, COPY_ON_IMPORT_COLUMN)->setText(tr("Not Copied"));
                alreadyExists++;
                progressBar->setValue(progressBar->value() + 1);
                continue;
            }
        }

        // in addition, also check the RideCache for a Ride with the same point in Time in UTC, which also indicates
        // that there was already a ride imported - reason is that RideCache start time is in UTC, while the file Name is in "localTime"
        // which causes problems when importing the same file (for files which do not have time/date in the file name),
        // while the computer has been set to a different time zone
        if (context->athlete->rideCache->getRide(ridedatetime.toUTC())) {
            if (!(imFile.overwriteableFile = overwriteFiles)) {
                tableWidget->item(i, STATUS_COLUMN)->setText(tr("Failed - Activity with same start date/time exists in /activities"));
                tableWidget->item(i, COPY_ON_IMPORT_COLUMN)->setText(tr("Not Copied"));
                alreadyExists++;
                progressBar->setValue(progressBar->value() + 1);
                continue;
            }
        }

        // Populate attributes for files that can be imported
        // i.e. no errors or excluded files, and no duplicates (depending on optional overwriteFiles)
        imFile.tableRow = i; 
        imFile.activityFullSource = filenames[i].activityFullSource;
        imFile.copyOnImport = filenames[i].copyOnImport;
        imFile.targetNoSuffix = targetNoSuffix;
        imFile.ridedatetime = ridedatetime;
        imFile.targetWithSuffix = activitiesTarget;
        imFile.tmpActivitiesFulltarget = tmpActivitiesFulltarget;
        imFile.finalActivitiesFulltarget = finalActivitiesFulltarget;
        validFilenames.append(imFile);
    }

    int successfulImports = 0;

    // Save the currently selected ride item.
    // To achieve batch processing we are going to need to change this in the context temporarily
    // because the python processing is structured that the currently selected RideItem is the one
    // that gets processed with its associated RideFile. See #4095 for consequences on "import".
    RideItem* rideISafe = context->ride;

    // Saving now - process the files than need to be imported, one-by-one
    for (int j = 0; j < validFilenames.count(); j++) {

        bool srcInImportsDir(true);
        QString importsTarget;
        QFileInfo sourceFileInfo(validFilenames[j].activityFullSource);

        // If the source file is not in "/imports" directory (if it's not taken from there as source), set up the importTarget
        if (sourceFileInfo.canonicalPath() != homeImports.canonicalPath()) {
            // Add the GC file base name (targetnosuffix) to create unique file names during import (for identification)
            // Note: There should not be 2 ride files with exactly the same time stamp (as this is also not foreseen for the .json)
            importsTarget = sourceFileInfo.baseName() + "_" + validFilenames[j].targetNoSuffix + "." + sourceFileInfo.suffix();
            srcInImportsDir = false;
        } else {
            // file is re-imported from /imports - keep the name for .JSON Source File Tag
            importsTarget = sourceFileInfo.fileName();
        }
        
        // SAVE STEP 4 - open the file with the respective format reader and export as .JSON
        // to track if addRideCache() has caused an error due to bad data we work with a interim directory for the activities
        // -- first   export to /tmpactivities
        // -- second  create RideCache() entry
        // -- third   copy source file to / imports(if required)
        // -- fourth  move file from /tmpactivities to /activities
        // -- 

        // serialize the file to .JSON
        QStringList errors;
        QFile thisfile(validFilenames[j].activityFullSource);
        RideFile * tempRideF = RideFileFactory::instance().openRideFile(context, thisfile, errors);

        // did the input file parse ok ? (should be fine here - since it was already checked before - but just in case)
        if (!tempRideF) {
            tableWidget->item(validFilenames[j].tableRow, STATUS_COLUMN)->setText(tr("Error - Import of activity file failed"));
            tableWidget->item(validFilenames[j].tableRow, COPY_ON_IMPORT_COLUMN)->setText(tr("Not Copied"));
            importsErrors++;
            continue;
        }

        // update ridedatetime and set the Source File name
        tempRideF->setStartTime(validFilenames[j].ridedatetime);
        tempRideF->setTag("Source Filename", importsTarget);
        tempRideF->setTag("Filename", validFilenames[j].targetWithSuffix);
        if (errors.count() > 0)
            tempRideF->setTag("Import errors", errors.join("\n"));

        // process linked defaults
        GlobalContext::context()->rideMetadata->setLinkedDefaults(tempRideF);

        // this text will get overwritten by an error message or sucessful import message.
        tableWidget->item(validFilenames[j].tableRow, STATUS_COLUMN)->setText(tr("Processing..."));

        // write converted ride file to disk
        JsonFileReader reader;
        QFile target(validFilenames[j].tmpActivitiesFulltarget);
        if (!reader.writeRideFile(context, tempRideF, target)) {
            tableWidget->item(validFilenames[j].tableRow, STATUS_COLUMN)->setText(tr("Error - %1 temporary file creation failed").arg(validFilenames[j].tmpActivitiesFulltarget));
            tableWidget->item(validFilenames[j].tableRow, COPY_ON_IMPORT_COLUMN)->setText(tr("Not Copied"));
            importsErrors++;
            delete tempRideF; // clear temporary import ride file
            continue;
        }

        // clear temporary import ride file
        delete tempRideF;

        // now add the Ride to the RideCache, this step should always be successful as if the filename
        // already exists it is replaced in RideCache, otherwise the new ride is added to RideCache.
        RideItem* rideI = context->athlete->rideCache->addRide(validFilenames[j].targetWithSuffix,
                                    bool(j == validFilenames.count()-1),    // only signal rideAdded on the last entry when mass importing
                                    bool(j == validFilenames.count() - 1),  // only signal rideSelected on the last entry when mass importing
                                    true,                                   // file is available only in /tmpActivities, so use this one please
                                    false);                                 // planned defaulted to false


        if (!rideI) {
            tableWidget->item(validFilenames[j].tableRow, STATUS_COLUMN)->setText(tr("Error - %1 cannot be found in RideCache!").arg(validFilenames[j].targetWithSuffix));
            tableWidget->item(validFilenames[j].tableRow, COPY_ON_IMPORT_COLUMN)->setText(tr("Not Copied"));
            importsErrors++;
            continue;
        }

        // opens the ride item's associated ride file if it isn't already open
        RideFile* processingRideF = rideI->ride();

        if (!processingRideF) {
            tableWidget->item(validFilenames[j].tableRow, STATUS_COLUMN)->setText(tr("Error - %1 cannot open ride file to process!").arg(validFilenames[j].targetWithSuffix));
            importsErrors++;
            continue;
        }

        // The Python processors need the ride item (rideI) setup in the context as well as the processingRideF, because the python processors can manipulate the
        // raw data sets and the metadata tags, while the builtin core processors only need the ride file (processingRideF) to be setup.
        context->ride = rideI;

        // run the data processors on the ride file
        DataProcessorFactory::instance().autoProcess(processingRideF, "Auto", "Import");
        processingRideF->recalculateDerivedSeries();
        rideI->refresh();

        // write processed ride file to disk
        if (!reader.writeRideFile(context, processingRideF, target)) {
            tableWidget->item(validFilenames[j].tableRow, STATUS_COLUMN)->setText(tr("Error - %1 processed file creation failed").arg(validFilenames[j].targetWithSuffix));
            importsErrors++;
            continue;
        }

        // Up until this point the ride file has been stored in /tmpActivities, so that if there was a problem it would be available
        // on GC restart to understand the problem, but now it can be copied to /imports and moved to the "clean" /activities folder

        // copy the source file to "/imports" directory (if it's not taken from there as source) and copying to /Imports is required
        copySourceFileToImportDir(validFilenames[j], importsTarget, srcInImportsDir);

        // let's move the file to the real /activities folder
        if (!moveFile(validFilenames[j].tmpActivitiesFulltarget, validFilenames[j].finalActivitiesFulltarget)) {
            tableWidget->item(validFilenames[j].tableRow, STATUS_COLUMN)->setText(tr("Error - Moving %1 to /activities").arg(validFilenames[j].targetWithSuffix));
            tableWidget->item(validFilenames[j].tableRow, COPY_ON_IMPORT_COLUMN)->setText(tr("Not Copied"));
            importsErrors++;
            continue;
        }

        rideI->setFileName(homeActivities.canonicalPath(), validFilenames[j].targetWithSuffix);

        // Record the successful import
        if (validFilenames[j].overwriteableFile) {
            overwrittenFiles++;
            tableWidget->item(validFilenames[j].tableRow, STATUS_COLUMN)->setText(tr("Import successful - Overwritten in /activities"));
        } else {
            successfulImports++;
            tableWidget->item(validFilenames[j].tableRow, STATUS_COLUMN)->setText(tr("Import successful - Saved in /activities"));
        }

        QApplication::processEvents();
        if (aborted) { done(0); return; }
        progressBar->setValue(progressBar->value()+1);
        this->repaint();
    }

    // Restore the selected ride before the batch processing
    context->ride = rideISafe;

    tableWidget->setSortingEnabled(true); // so you can browse through errors etc
    QString donemessage = QString(tr("Import Complete: successful [ newfiles %1, overwritten %2 ]   unsuccessful [ existing skipped %3, excluded %4, errors %5 ]   total of %6 files."))
            .arg(successfulImports, 1, 10, zero)
            .arg(overwrittenFiles, 1, 10, zero)
            .arg(alreadyExists, 1, 10, zero)
            .arg(excludedImports, 1, 10, zero)
            .arg(importsErrors, 1, 10, zero)
            .arg(filenames.count(), 1, 10, zero);
    progressBar->setValue(progressBar->maximum());
    phaseLabel->setText(donemessage);
    abortButton->setText(tr("Finish"));
    aborted = false;
    _importInProcess = false;  // Re-enable the window's close icon

    if (autoImportStealth) {
        abortClicked();  // simulate pressing the "Finish" button - even if the window got visible
    } else {
        if (!isActiveWindow()) activateWindow();
    }

    // If importation was error free then tidy up, otherwise keep /tmpactivities files for fault finding.
    if (importsErrors == 0) {
        foreach(QString name, deleteMe) QFile(name).remove();
    }
}

const QString exclusionListName("/GC_Auto_Import_Exclusion_List.txt");

bool
RideImportWizard::isFileExcludedFromImportation(const int i) {

    QFile exclusionList(homeImports.canonicalPath() + exclusionListName);

    // open the exclusion list or create one if it doesn't exist
    if (exclusionList.open(QIODevice::ReadOnly | QIODevice::Text)) {

        // Search to see if file to be excluded is already 
        while (!exclusionList.atEnd()) {
            QString line = exclusionList.readLine();
            if (line.contains(filenames[i].activityFullSource)) {
                filenames[i].copyOnImport = EXCLUDED_FROM_IMPORT;
                exclusionList.close();
                return true;
            }
        }
    }

    exclusionList.close();
    return false;
}

void
RideImportWizard::addFileToImportExclusionList(const QString& importExcludedFile) {

    QFile exclusionList(homeImports.canonicalPath() + exclusionListName);

    // open the exclusion list or create one if it doesn't exist
    if (exclusionList.open(QIODevice::ReadOnly | QIODevice::Text)) {

        // Search to see if file to be excluded is already 
        while (!exclusionList.atEnd()) {
            QString line = exclusionList.readLine();
            if (line.contains(importExcludedFile)) {
                exclusionList.close();
                return;
            }
        }
        exclusionList.close();
    }

    // open the exclusion list or Qt will create one if it doesn't exist
    if (exclusionList.open(QIODevice::ReadWrite | QIODevice::Append | QIODevice::Text)) {

        // Add the file on its own line
        QFile excludedFile(QString(importExcludedFile) + "\n");
        const char* excludedFileChars(excludedFile.fileName().toStdString().c_str());
        exclusionList.write(excludedFileChars, qstrlen(excludedFileChars));
        exclusionList.close();
    }
 }

void
RideImportWizard::copySourceFileToImportDir(const ImportFile& source, const QString& importsTarget, bool srcInImportsDir) {

    // Copy the source file to "/imports" directory (if it's not taken from there as source) and copying to /Imports is required           
    if (!srcInImportsDir) {

        if (source.copyOnImport == COPY_ON_IMPORT) {

            QString importsFulltarget = homeImports.canonicalPath() + "/" + importsTarget;
            QFile tgt(importsFulltarget);
            QFile src(source.activityFullSource);

            // If a file already exists in the import directory with the same name as import target, then to maintain
            // the integrity between the activities and import directories, remove the exisiting import directory file
            // before copying the new activity file to /imports.
            if (tgt.exists()) {
                // delete old target
                if (!tgt.remove()) {
                    // tgt is proving immoveable  :(
                    tableWidget->item(source.tableRow, COPY_ON_IMPORT_COLUMN)->setText(tr("Blocked"));
                    return;
                }
            }

            // copy src to tgt
            if (src.copy(importsFulltarget)) {
                tableWidget->item(source.tableRow, COPY_ON_IMPORT_COLUMN)->setText(tr("Copied"));
            } else {
                tableWidget->item(source.tableRow, COPY_ON_IMPORT_COLUMN)->setText(tr("Failed"));
            }
        } else {
            qDebug() << "RideImportWizard::copySourceFileToImportDir - unhandled case: " << source.copyOnImport;
        }
    } else {
        // the file being imported is the /import directory file ! therefore no action is required.
        tableWidget->item(source.tableRow, COPY_ON_IMPORT_COLUMN)->setText(tr("Import=>"));
    }
}

bool
RideImportWizard::moveFile(const QString& source, const QString& target) {

    QFile src(source);
    QFile tgt(target);

    if (tgt.exists()) {
        // move it out of the way
        if (tgt.rename(target + "_old")) {
            // move src to tgt
            if (src.rename(target)) {
                // delete old target
                tgt.remove();
                return true;
            } else {
                // failed to move src to tgt, so rename tgt back to original name
                tgt.rename(target);
                return false;
            }
        } else {
            // tgt is proving immoveable
            return false;
        }
    } else {
        // move src to tgt
        return(src.rename(target));
    }
}

void
RideImportWizard::closeEvent(QCloseEvent* event)
{
    if (_importInProcess)
        event->ignore();
    else
        event->accept();
}

void
RideImportWizard::done(int rc)
{
    _importInProcess = false;
    QDialog::done(rc);
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
        myOption.displayAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
        drawDisplay(painter, myOption, myOption.rect, text);
        drawFocus(painter, myOption, myOption.rect);

    } else if (index.column() == dateColumn+1) {

        QString value = index.model()->data(index, Qt::DisplayRole).toString();
        // display with angles to show its doing something
        QString text = QString("%1").arg(value);

        QStyleOptionViewItem myOption = option;
        myOption.displayAlignment = Qt::AlignHCenter | Qt::AlignVCenter;
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
        dateEdit->setDisplayFormat("yyyy-MM-dd"); // ISO Format, no translation analog Qt::ISODate
        connect(dateEdit, SIGNAL(editingFinished()), this, SLOT(commitAndCloseDateEditor()));
        return dateEdit;
    } else if (index.column() == dateColumn+1) {
        // edit that time
        QTimeEdit *timeEdit = new QTimeEdit(parent);
        timeEdit->setDisplayFormat("hh:mm:ss");
        connect(timeEdit, SIGNAL(editingFinished()), this, SLOT(commitAndCloseTimeEditor()));
        return timeEdit;
    } else if (index.column() == COPY_ON_IMPORT_COLUMN) {
        QComboBox* comboFileButton = new QComboBox(parent);
        comboFileButton->addItem(ImportFile::copyTypeToString(COPY_ON_IMPORT));
        comboFileButton->addItem(ImportFile::copyTypeToString(EXCLUDED_FROM_IMPORT));
        connect(comboFileButton, QOverload<int>::of(&QComboBox::activated), this, &RideDelegate::commitAndCloseAutoImportEditor);
        return comboFileButton;
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

// user hit tab or return so save away the data to our model
void RideDelegate::commitAndCloseAutoImportEditor(int /* index */)
{
    QComboBox* comboFileButton = qobject_cast<QComboBox*>(sender());
    emit commitData(comboFileButton);
    emit closeEditor(comboFileButton);
}

// We don't set anything because the data is saved within the view not the model!
void RideDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    // stored as text field
    if (index.column() == dateColumn) {
        QDateEdit *dateEdit = qobject_cast<QDateEdit *>(editor);
        QDate date = QDate().fromString(index.model()->data(index, Qt::DisplayRole).toString(), Qt::ISODate);
        dateEdit->setDate(date);
    } else if (index.column() == dateColumn+1) {
        QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(editor);
        QTime time = QTime().fromString(index.model()->data(index, Qt::DisplayRole).toString(), "hh:mm:ss");;
        timeEdit->setTime(time);
    } else if (index.column() == COPY_ON_IMPORT_COLUMN) {
        QComboBox* comboBox = qobject_cast<QComboBox*>(editor);
        QString importString(index.model()->data(index, Qt::DisplayRole).toString());
        comboBox->setCurrentIndex(ImportFile::stringToCopyType(importString));
    }
}

// We don't set anything because the data is saved within the view not the model!
void RideDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    // stored as text field
    if (index.column() == dateColumn) {
        QDateEdit *dateEdit = qobject_cast<QDateEdit *>(editor);
        QString value = dateEdit->date().toString(Qt::ISODate);
        // Place in the view
        model->setData(index, value, Qt::DisplayRole);
    } else if (index.column() == dateColumn+1) {
        QTimeEdit *timeEdit = qobject_cast<QTimeEdit *>(editor);
        QString value = timeEdit->time().toString("hh:mm:ss");
        model->setData(index, value, Qt::DisplayRole);
    }
    else if (index.column() == COPY_ON_IMPORT_COLUMN) {
        QComboBox* comboBox = qobject_cast<QComboBox*>(editor);
        QString value(ImportFile::copyTypeToString((importCopyType)comboBox->currentIndex()));
        model->setData(index, value, Qt::DisplayRole);
    }
}

