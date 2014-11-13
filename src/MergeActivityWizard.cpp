/*
 * Copyright (c) 2011-13 Damien Grauser (Damien.Grauser@pev-geneve.ch)
 *               2014    Mark Liversedgge (liversedge@gmail.com)
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

#include "MergeActivityWizard.h"
#include "Context.h"
#include "MainWindow.h"

/*----------------------------------------------------------------------
 *
 * Page Flow Summary
 *
 * 10 MergeWelcome       Welcome message
 *
 * 20 MergeSource        Select source (import or download)
 *    25 MergeDownload   Download from device
 *    27 MergeChoose     Select a ride from a list
 *
 * 30 MergeMode          Select mode (join / merge)
 *
 * if merge (not join)
 *    40 MergeSelect     Select series to combine
 *    50 MergeStrategy   Select strategy for aligning data
 *    60 MergeAdjust     Manually adjust alignment 
 *
 * 70 MergeConfirm       Confirm and Save away
 *
 *---------------------------------------------------------------------*/

MergeActivityWizard::MergeActivityWizard(Context *context) : QWizard(context->mainWindow), context(context)
{
#ifdef Q_OS_MAX
    setWizardStyle(QWizard::ModernStyle);
#endif
    setWindowTitle(tr("Combine Activities"));

    setFixedHeight(530);
    setFixedWidth(550);

    // initialise before setRide since it checks
    // to see if memory needs to be freed first
    ride1 = ride2 = NULL;
    combined = NULL;

    // current ride
    current = const_cast<RideItem*>(context->currentRideItem());
    recIntSecs= current->ride()->recIntSecs();
    setRide(&ride1, current->ride());

    // default to merge not join
    // and merge on device clocks
    mode = 0;
    strategy = 0;

    // 5 step process, although Conflict may be skipped
    setPage(10, new MergeWelcome(this));
    setPage(20, new MergeSource(this));
    setPage(25, new MergeDownload(this));
    setPage(27, new MergeChoose(this));
    setPage(30, new MergeMode(this)); 
    setPage(40, new MergeSelect(this)); 
    setPage(50, new MergeStrategy(this)); 
    setPage(60, new MergeAdjust(this)); // might need to rename to adjust
    setPage(1000, new MergeConfirm(this));
}

void
MergeActivityWizard::setRide(RideFile **here, RideFile *with)
{
    // wipe current
    if (*here) delete *here;

    // set with new resampled / filled
    // you may be tempted to optimise this out
    // but by cloning a working copy like this
    // we start with a clean ride and no derived
    // data to 'pollute' the process
    if (with) *here = with->resample(recIntSecs);
    else *here = NULL;
}

void 
MergeActivityWizard::analyse()
{
    // looking at the paramters determine the offset
    // to be used by both rides XXX not written yet !

    // XXX just make em start together for now !
    offset1=0;
    offset2=0;
}

void 
MergeActivityWizard::combine()
{
    // zap whatever we have
    if (combined) delete combined;

    // create a combined ride applying the parameters
    // from the wizard for join or merge

    if (mode == 1) { // JOIN

        // easy peasy -- loop through one then the other !
        combined = new RideFile(ride1);

        RideFilePoint *lp = NULL;

        foreach(RideFilePoint *p, ride1->dataPoints()) {
            combined->appendPoint(*p);
            lp = p;
        }

        // now add the data from the second one!
        double distanceOffset=0;
        double timeOffset=0;
        bool first=true;

        foreach(RideFilePoint *p, ride2->dataPoints()) {
            if (first) {

                if (lp) {
                    distanceOffset = lp->km + p->km;
                    timeOffset = lp->secs + p->secs + recIntSecs;
                }
                first = false;
            }

            RideFilePoint add = *p;
            add.secs += timeOffset;
            add.km += distanceOffset;

            combined->appendPoint(add);
        }

        // any intervals with a number name? find the last
        int intervalN=0;
        foreach(RideFileInterval interval, ride1->intervals()) {
            int x = interval.name.toInt();
            if (interval.name == QString("%1").arg(x)) {
                if (x > intervalN) intervalN = x;
            }
        }

        // now run through the intervals for the second ride
        // and add them but renumber any intervals that are just numbers
        foreach(RideFileInterval interval, ride2->intervals()) {
            int x = interval.name.toInt();
            if (interval.name == QString("%1").arg(x)) {
                interval.name = QString("%1").arg(x+intervalN);
            }
            combined->addInterval(interval.start + timeOffset,
                                  interval.stop + timeOffset, 
                                  interval.name);
        }

    } else { // MERGE

    }
}

/*----------------------------------------------------------------------
 * Wizard Pages
 *--------------------------------------------------------------------*/

// welcome
MergeWelcome::MergeWelcome(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Combine Activities"));
    setSubTitle(tr("Lets get started"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QLabel *label = new QLabel(tr("This wizard will help you combine data with "
                               "the currently selected activity.\n\n"
                               "You will be able to import or download data before "
                               "merging or joining the data and manually adjusting "
                               "the alignment of data series before it is saved."));
    label->setWordWrap(true);

    layout->addWidget(label);
    layout->addStretch();
}

//Select source for merge
MergeSource::MergeSource(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Select Source"));
    setSubTitle(tr("Where is the data you want to combine ?"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mapped(QString)), this, SLOT(clicked(QString)));

    // select a file
    QCommandLinkButton *p = new QCommandLinkButton(tr("Import from a File"), 
                                tr("Import and combine from a file on your hard disk"
                                   " or device mounted as a USB disk to combine with "
                                   "the current activity."), this);
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "import");
    layout->addWidget(p);

    // download from a device
    p = new QCommandLinkButton(tr("Download from Device"), 
                                tr("Download data from a serial port device such as a Moxy Muscle Oxygen Monitor or"
                                   " bike computer to combine with the current activity."), this);
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "download");
    layout->addWidget(p);

    // select from rides on same day
    p = new QCommandLinkButton(tr("Existing Activity"), 
                                tr("Combine data from an activity that has already been imported or downloaded into"
                                   " GoldenCheetah. Selecting from a list of all available activities. "), this);
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "choose");
    layout->addWidget(p);

    label = new QLabel("", this);
    layout->addWidget(label);

    next = 30;
    setFinalPage(false);
}

void
MergeSource::initializePage()
{
}
   

void
MergeSource::clicked(QString p)
{
    // reset -- particularly since we might get here from
    //          other pages hitting 'Back'
    initializePage();

    // move on if we imported one
    if (p == "import" && importFile() == true) {
        next = 30;
        wizard->next();
    }

    // move on if we downloaded one
    if (p == "download") {
        next = 25;
        wizard->next();
    }

    // select from a list
    if (p == "choose") {
        next = 27;
        wizard->next();
    }

}

bool
MergeSource::importFile()
{
    QVariant lastDirVar = appsettings->value(this, GC_SETTINGS_LAST_IMPORT_PATH);
    QString lastDir = (lastDirVar != QVariant())
        ? lastDirVar.toString() : QDir::homePath();

    const RideFileFactory &rff = RideFileFactory::instance();
    QStringList suffixList = rff.suffixes();
    suffixList.replaceInStrings(QRegExp("^"), "*.");
    QStringList fileNames;
    QStringList allFormats;
    allFormats << QString(tr("All Supported Formats (%1)")).arg(suffixList.join(" "));
    foreach(QString suffix, rff.suffixes())
        allFormats << QString("%1 (*.%2)").arg(rff.description(suffix)).arg(suffix);
    allFormats << tr("All files (*.*)");
    fileNames = QFileDialog::getOpenFileNames(
        this, tr("Import from File"), lastDir,
        allFormats.join(";;"));
    if (!fileNames.isEmpty()) {
        lastDir = QFileInfo(fileNames.front()).absolutePath();
        appsettings->setValue(GC_SETTINGS_LAST_IMPORT_PATH, lastDir);
        QStringList fileNamesCopy = fileNames; // QT doc says iterate over a copy
        return importFile(fileNamesCopy);
    }
    return false;
}

bool
MergeSource::importFile(QList<QString> files)
{
    // get fullpath name for processing
    QFileInfo filename = QFileInfo(files[0]).absoluteFilePath();

    QFile thisfile(files[0]);
    QFileInfo thisfileinfo(files[0]);
    QStringList errors;

    if (thisfileinfo.exists() && thisfileinfo.isFile() && thisfileinfo.isReadable()) {

        // is it one we understand ?
        QStringList suffixList = RideFileFactory::instance().suffixes();
        QRegExp suffixes(QString("^(%1)$").arg(suffixList.join("|")));
        suffixes.setCaseSensitivity(Qt::CaseInsensitive);

        if (suffixes.exactMatch(thisfileinfo.suffix())) {

            RideFile *ride = RideFileFactory::instance().openRideFile(wizard->context, thisfile, errors);

            // did it parse ok?
            if (ride) {

                wizard->setRide(&wizard->ride2, ride);
                return true;
            }

        } else {

            wizard->setRide(&wizard->ride2, NULL);
            errors.append(tr("Error - Unknown file type"));
            return false;
        }
    }
    return false;
}

// Download dialog embedded
MergeDownload::MergeDownload(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Download Activity"));
    setSubTitle(tr("Download Activity to Combine"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    // embed download dialog
    QMdiArea *mdiarea = new QMdiArea(this);
    layout->addWidget(mdiarea);

    // get a download dialog -- embedded
    DownloadRideDialog *download = new DownloadRideDialog(parent->context, true);
    //download->setInputMode(QInputDialog.TextInput);
    QMdiSubWindow *w = mdiarea->addSubWindow(download, Qt::FramelessWindowHint);

    // maximize and no title bar etc
    w->showMaximized();
    layout->addStretch();

    connect(download, SIGNAL(downloadStarts()), this, SLOT(downloadStarts()));
    connect(download, SIGNAL(downloadEnds()), this, SLOT(downloadEnds()));
    connect(download, SIGNAL(downloadFiles(QList<DeviceDownloadFile>)), this, 
                      SLOT(downloadFiles(QList<DeviceDownloadFile>)));
}

void 
MergeDownload::downloadFiles(QList<DeviceDownloadFile>files)
{
    // Bingo ! only one ride so must be this one
    // saves having to select from a pesky list
    // XXX WE NEED TO UPDATE DOWNLOADRIDEDIALOG TO
    //     ALLOW SELECTION OF RIDES TO PROCESS AND
    //     ALSO MAKE IT LIMIT THIS TO ONE RIDE WHEN
    //     EMBEDDED IN MERGEDOWNLOAD ! XXX
    if (files.count() == 1) {

        QFile thisfile(files[0].name);
        QStringList errors;

        RideFileReader *reader = RideFileFactory::instance().readerForSuffix(files[0].extension);
        if (!reader) return;
 
        RideFile *ride = reader->openRideFile(thisfile, errors);

        // did it parse ok?
        if (ride) {

            wizard->setRide(&wizard->ride2 ,ride);
            next = 30;
            wizard->next();
            return;
        } else {

            wizard->setRide(&wizard->ride2, NULL);
            errors.append(tr("Error - Unknown file type"));
            return;
        }
    }
}

void
MergeDownload::downloadStarts()
{
    wizard->button(QWizard::BackButton)->setEnabled(false);
}

void
MergeDownload::downloadEnds()
{
    wizard->button(QWizard::BackButton)->setEnabled(true);
}

void
MergeDownload::initializePage()
{
    // might be needed for download ????
}

// Choose from the ride list
MergeChoose::MergeChoose(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Choose an Activity"));
    setSubTitle(tr("Choose an Existing activity to Combine"));

    chosen = false;

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    files = new QTreeWidget;
    files->headerItem()->setText(0, tr("Filename"));
    files->headerItem()->setText(1, tr("Date"));
    files->headerItem()->setText(2, tr("Time"));

    files->setColumnCount(3);
    files->setColumnWidth(0, 190); // filename
    files->setColumnWidth(1, 95); // date
    files->setColumnWidth(2, 90); // time
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setUniformRowHeights(true);
    files->setIndentation(0);

    // populate with each ride in the ridelist
    const QTreeWidgetItem *allRides = wizard->context->athlete->allRideItems();

    for (int i=allRides->childCount()-1; i>=0; i--) {

        RideItem *rideItem = static_cast<RideItem*>(allRides->child(i));

        QTreeWidgetItem *add = new QTreeWidgetItem(files->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // we will wipe the original file
        add->setText(0, rideItem->fileName);
        add->setText(1, rideItem->dateTime.toString(tr("dd MMM yyyy")));
        add->setText(2, rideItem->dateTime.toString("hh:mm:ss"));
    }

    layout->addWidget(files);
    connect(files, SIGNAL(itemSelectionChanged()), this, SLOT(selected()));
}

void
MergeChoose::selected()
{
    wizard->button(QWizard::BackButton)->setEnabled(true);
    chosen = true;
    next = 30;
    emit completeChanged();
}

bool
MergeChoose::validatePage()
{
    // make sure the currently selected ride has data
    // and can be opened etc
    QString filename = "none";

    // which one is selected ?
    if (files->currentItem()) filename=files->currentItem()->text(0);
    
    // open it..
    QStringList errors;
    QList<RideFile*> rides;
    QFile thisfile(QString(wizard->context->athlete->home->activities().absolutePath()+"/"+filename));
    RideFile *ride = RideFileFactory::instance().openRideFile(wizard->context, thisfile, errors, &rides);

    if (ride && ride->dataPoints().count()) {

        wizard->setRide(&wizard->ride2, ride);
        return true;
    }
    return false;
}
   
void
MergeChoose::initializePage()
{
}
   
//Select mode for merge
MergeMode::MergeMode(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Select Mode"));
    setSubTitle(tr("How would you like to combine the data ?"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mapped(QString)), this, SLOT(clicked(QString)));

    // merge
    QCommandLinkButton *p = new QCommandLinkButton(tr("Merge Data to add another data series"), 
                                tr("Merge data series from one recording into the current activity"
                                   " where different types of data (e.g. O2 data from a Moxy) have been "
                                   " recorded by different devices. Taking care to align the data in time."), this);
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "merge");
    layout->addWidget(p);

    // join
    p = new QCommandLinkButton(tr("Join Data to form a longer activity"), 
                                tr("Append the data to the end of the current activity "
                                   " to create a longer activity that was recorded in multiple"
                                   " parts."), this);
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "join");
    layout->addWidget(p);

    label = new QLabel("", this);
    layout->addWidget(label);

    next = 40;
    setFinalPage(false);
}

void
MergeMode::initializePage()
{
}
   

void
MergeMode::clicked(QString p)
{
    // reset -- particularly since we might get here from
    //          other pages hitting 'Back'
    initializePage();

    if (p == "join") {
        wizard->mode = 1; // join is easy !
        wizard->combine(); // analyse not required just combined them !
        next = 1000;
    } else {
        // where to next ?
        wizard->mode = 0; // merge ...
        next = 40;
    }
    wizard->next();
}

//Select merge strategy
MergeStrategy::MergeStrategy(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Select Strategy"));
    setSubTitle(tr("How should we align the data ?"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mapped(QString)), this, SLOT(clicked(QString)));

    // time
    QCommandLinkButton *p = new QCommandLinkButton(tr("Align using start time"), 
                                tr("Align the data from the two activities based upon the start time "
                                   "for the activities. This will work well if the devices used to "
                                   "record the data have their clocks synchronised / close to each other."), this);
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "time");
    layout->addWidget(p);

    // shared data
    shared = new QCommandLinkButton(tr("Align using shared data series"), 
                                tr("If the two activities both contain the same data series, for example "
                                   "where both devices recorded cadence or perhaps HR, then we can align the other "
                                   "data series in time by matching the peaks and troughs in the shared data."), this);
    connect(shared, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(shared, "shared");
    layout->addWidget(shared);

    // start at same time
    p = new QCommandLinkButton(tr("Align starting together"), 
                                tr("Regardless of the timestamp on the activity, align with both "
                                   "activities starting at the same time."), this);
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "left");
    layout->addWidget(p);

    // start at same time
    p = new QCommandLinkButton(tr("Align ending together"), 
                                tr("Regardless of the timestamp on the activity, align with both "
                                   "activities ending at the same time."), this);
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "right");
    layout->addWidget(p);

    label = new QLabel("", this);
    layout->addWidget(label);

    next = 60;
    setFinalPage(false);
}

void
MergeStrategy::initializePage()
{
    // are there any shared data ????
    bool hasShared = false;
    QMapIterator<RideFile::SeriesType, QCheckBox *> i(wizard->rightSeries);
    while(i.hasNext()) {
        i.next();
        if (wizard->leftSeries.value(i.key(), NULL) != NULL)
            hasShared = true;
    }
    shared->setEnabled(hasShared);
}
   

void
MergeStrategy::clicked(QString p)
{
    // reset -- particularly since we might get here from
    //          other pages hitting 'Back'
    initializePage();

    if (p == "time") {
        wizard->strategy = 0;
    } else if (p == "shared" ) {
        // where to next ?
        wizard->strategy = 1; // merge ...
    } else if (p == "left" ) {
        wizard->strategy = 2; // merge ...
    } else if (p == "right" ) {
        wizard->strategy = 3; // merge ...
    }

    // now run strategy and get on
    wizard->analyse();
    wizard->combine();

    // lets do this thing !
    next = 60;
    wizard->next();
}

// Synchronise start of files
MergeAdjust::MergeAdjust(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Adjust Alignment"));
    setSubTitle(tr("Adjust merge alignment in time"));

    // Plot files
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
}

void
MergeAdjust::initializePage()
{
}

// select
MergeSelect::MergeSelect(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Merge Data Series"));
    setSubTitle(tr("Select the series to merge together"));

    QVBoxLayout *mainlayout = new QVBoxLayout(this);

    QFont def;
    def.setWeight(QFont::Bold);

    QHBoxLayout *names = new QHBoxLayout;
    leftName = new QLabel("Current Selection", this);
    rightName = new QLabel("right", this);
    leftName->setFixedHeight(20);
    leftName->setFont(def);
    leftName->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    rightName->setFixedHeight(20);
    rightName->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    rightName->setFont(def);
    names->addWidget(leftName);
    names->addWidget(rightName);
    mainlayout->addLayout(names);

    QHBoxLayout *leftright = new QHBoxLayout;
    mainlayout->addLayout(leftright);

    QScrollArea *left = new QScrollArea(this);
    left->setAutoFillBackground(false);
    left->setWidgetResizable(true);
    left->setContentsMargins(0,0,0,0);
    QScrollArea *right = new QScrollArea(this);
    right->setAutoFillBackground(false);
    right->setWidgetResizable(true);
    right->setContentsMargins(0,0,0,0);

    leftright->addWidget(left);
    leftright->addWidget(right);

    // checkboxes on the left
    QWidget *leftWidget = new QWidget(this);
    leftWidget->setContentsMargins(0,0,0,0);
    leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0,0,0,0);
    leftLayout->setSpacing(0);

    // checkboxes on the right
    QWidget *rightWidget = new QWidget(this);
    rightWidget->setContentsMargins(0,0,0,0);
    rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0,0,0,0);
    rightLayout->setSpacing(0);

    left->setWidget(leftWidget);
    right->setWidget(rightWidget);
}

void
MergeSelect::initializePage()
{
    rightName->setText(QString("%1 (%2)").arg(wizard->ride2->startTime().toString("d MMM yy hh:mm:ss"))
                                         .arg(wizard->ride2->deviceType()));

    // create a checkbox for each series present in 
    QMapIterator<RideFile::SeriesType, QCheckBox*> i(wizard->leftSeries);
    while(i.hasNext()) {
        i.next();
        delete i.value();
    }
    QMapIterator<RideFile::SeriesType, QCheckBox*> j(wizard->rightSeries);
    while(j.hasNext()) {
        j.next();
        delete j.value();
    }
    wizard->leftSeries.clear();
    wizard->rightSeries.clear();

    // get rid of the stretch
    leftLayout->takeAt(0);
    rightLayout->takeAt(0);

    for(int i=0; i < static_cast<int>(RideFile::none); i++) {

        // save us casting all the time 
        RideFile::SeriesType series = static_cast<RideFile::SeriesType>(i);

        // the one thing we don't select !
        if (series == RideFile::secs) continue;

        bool lefthas = false;
        if (wizard->ride1->isDataPresent(series)) {
            lefthas = true;

            QCheckBox *add = new QCheckBox(wizard->ride1->seriesName(series));
            add->setChecked(true);
            add->setEnabled(false);

            leftLayout->addWidget(add);
            wizard->leftSeries.insert(series, add);
        }

        if (wizard->ride2->isDataPresent(series)) {
            QCheckBox *add = new QCheckBox(wizard->ride2->seriesName(series));
            add->setChecked(!lefthas);

            rightLayout->addWidget(add);
            wizard->rightSeries.insert(series, add);

            connect(add, SIGNAL(stateChanged(int)), this, SLOT(checkboxes()));
        }
    }

    leftLayout->addStretch();
    rightLayout->addStretch();
}

void
MergeSelect::checkboxes()
{
    // as we turn on/off right side checkboxes we need to
    // turn off/on left side checkboxes
    QMapIterator<RideFile::SeriesType,QCheckBox*> i(wizard->rightSeries);
    while(i.hasNext()) {

        i.next();
        QCheckBox *left=NULL;
        if ((left=wizard->leftSeries.value(i.key(), NULL)) != NULL) {
            left->setChecked(!i.value()->isChecked());
        }
    }
}

MergeConfirm::MergeConfirm(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Confirm"));
    setSubTitle(tr("Complete and Save"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QLabel *label = new QLabel(tr("Press Finish to update the current ride with "
                               " the combined data.\n\n"
                               "The changes will not be saved until you save them "
                               " so you can check and revert or save.\n\n"
                               "If you continue the ride will be updated, if you "
                               "do not want to continue either go back and change "
                               "the settings or press cancel to abort."));
    label->setWordWrap(true);

    layout->addWidget(label);
    layout->addStretch();
}

bool
MergeConfirm::validatePage()
{
    // We are done -- update BUT DOESNT SAVE
    //                user can now check !
    wizard->current->setRide(wizard->combined);
    wizard->context->notifyRideDirty();
    return true;
}
