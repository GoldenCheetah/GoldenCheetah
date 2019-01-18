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
#include "Colors.h"
#include "RideCache.h"
#include "MainWindow.h"
#include "HelpWhatsThis.h"
#include "LocationInterpolation.h"

// minimum R-squared fit when trying to find offsets to
// merge ride files. Lower numbers mean happier to take
// and answer that is less likely to be correct, but then
// its possibly better than nothing !
//
// I have found that with real world data, where no data
// has been resampled then fits >0.8 are usual 
// We always use the best fit anyway, this is just to
// decide what to discard as not valuable
static const double MINIMUM_R2_FIT = 0.75f; 
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
#ifdef Q_OS_MAC
    setWizardStyle(QWizard::ModernStyle);
#endif
    setWindowTitle(tr("Combine Activities"));

    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Activity_CombineRides));

    setMinimumHeight(600 *dpiXFactor);
    setMinimumWidth(550 *dpiYFactor);

    // initialise before setRide since it checks
    // to see if memory needs to be freed first
    ride1 = ride2 = NULL;
    combinedItem = new RideItem(NULL, context);
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

    // preserve XData
    if (with && *here) {
        foreach (XDataSeries *xdata, with->xdata())
            (*here)->addXData(xdata->name, new XDataSeries(*xdata));
    }
}

void 
MergeActivityWizard::analyse()
{
    // looking at the parameters determine the offset
    // default to align left if all else fails !
    offset1 = offset2 = 0;

    switch(strategy) {

    case 0: // align on the time of day
            // assumes the recording devices have
            // a clock that is well synchronised
    {
            double diff = ride1->startTime().toMSecsSinceEpoch() - ride2->startTime().toMSecsSinceEpoch();
            int samples = (diff/recIntSecs)/1000.0f;

            if (samples < 0) { // ride2 was later than ride 1
                offset2 = abs(samples);
                offset1 = 0;
            } else { // ride1 was later than ride 2
                offset1 = samples;
                offset2 = 0;
            }

            // but wait, is that too long ?
            if (offset1 > ride1->dataPoints().count() ||
                offset2 > ride2->dataPoints().count()) {

                // fallback to align same time
                offset1 = offset2 = 0;
            }
            //qDebug()<<"clocks make ride1 offset="<<offset1<<"and ride2 offset="<<offset2;

    }
            break;

    case 1: // align on shared series
            // using a shotgun algorithm
    {
            // calculate the R2 fit using the current offset
            // for the first shared series
            RideFile *base = ride1;
            RideFile *fit = ride2;

            // always fit smaller to larger
            int diff = ride1->dataPoints().count() - ride2->dataPoints().count();
            if (diff < 0) { // ride2 has more points
                base=ride2;
                fit= ride1;
            } else { // ride1 has more points
                base=ride1;
                fit=ride2;
            }

            double bestFit=0.0;
            int offsetFit=0;
            RideFile::SeriesType bestSeries=RideFile::none;

            QMapIterator<RideFile::SeriesType, QCheckBox *> i(rightSeries);
            while(i.hasNext()) {
                i.next();
                if (i.key() != RideFile::km && leftSeries.value(i.key(), NULL) != NULL) {

                    // for each shared series look for best fit
                    RideFile::SeriesType shared = i.key();

                    double bestR2 = 0.0f;
                    int bestOffset = 0;

                    // no more than shifting by a third of the ride backwards or forwards
                    for(int offset=-1 * (base->dataPoints().count()/3); 
                            offset<base->dataPoints().count()/3; 
                            offset++) {

                        double SStot=0.0f, SSres=0.0f;
                        double mean =0.0f;
                        int count=0;

                        for(int i=0; (i+offset)<base->dataPoints().count(); i++) {
                            if ((i+offset)>0) {
                                mean += base->dataPoints()[i+offset]->value(shared);
                            }
                            count++;
                        }
                        mean /= double(count);
                
                        for(int i=0; (i+offset)<base->dataPoints().count() && i<fit->dataPoints().count(); i++) {
                            if((i+offset)>0) {
                                SSres += pow(base->dataPoints()[i+offset]->value(shared) - fit->dataPoints()[i]->value(shared), 2);
                                SStot += pow(base->dataPoints()[i+offset]->value(shared) - mean, 2);
                            }
                        }

                        double R2= 1.0f - (SSres/SStot);
                        if (R2 > bestR2) {
                            bestR2= R2;
                            bestOffset=offset;
                        }
                    }

                    // is this a better fit ?
                    if (bestR2 > bestFit) {
                        bestFit=bestR2;
                        offsetFit=bestOffset;
                        bestSeries=shared;
                        Q_UNUSED(bestSeries); // shutup compiler when qDebugs commented out below
                    }
                    //qDebug()<<"best R2="<<bestR2<<"at best offset"<<bestOffset<<"with series"<<fit->seriesName(shared);
                }
            }
            //qDebug()<<"THEREFORE: best R2="<<bestFit<<"at best offset"<<offsetFit<<"with series"<<ride1->seriesName(bestSeries);

            // so lets turn that into an offset for ride1 and ride2
            if (bestFit > MINIMUM_R2_FIT) {
                if (diff <0) { // ride2 was the base
                    if (offsetFit<0) {
                        offset2=offsetFit * -1;
                        offset1=0;
                    } else {
                        offset1=offsetFit;
                        offset2=0;
                    }
                } else {
                    if (offsetFit<0) {
                        offset1=offsetFit * -1;
                        offset2=0;
                    } else {
                        offset2=offsetFit;
                        offset1=0;
                    }
                }
            }
    }
            break;

    case 2: // align begin
            offset1=0;
            offset2=0;
            break;

    case 3: // align end
        {
            int offset = ride1->dataPoints().count() - ride2->dataPoints().count();
            if (offset < 0) {
                offset1 = abs(offset);
                offset2 = 0;
            } else {
                offset2 = abs(offset);
                offset1 = 0;
            }
        }
        break;

    case 4: // merge (and interpolate) on distance
            // In this case merge iterator will find its own way populating samples with data
            // from ride2 samples that have overlapped distance.
        offset1 = 0;
        offset2 = 0;
        break;
    }
}

// Distance merge:
// Each data point in both rides has a distance. This merge pulls selected properties
// from ride2 data onto ride1, keeping distance in sync and interpolating location
// from ride1's distance and ride2's location data.
void
MergeActivityWizard::mergeRideSamplesByDistance()
{
    offset1 = 0;
    offset2 = 0;

    RideFilePoint last;

    GeoPointInterpolator gpi;

    int j = 0;  // copy index
    int ii = 0; // interpolation index
    int ride1nextdistanceindex = 0;
    double ride1nextdistance = 0.0;

    for (int i = 0; i < ride1->dataPoints().count(); i++) {

        // fresh point
        RideFilePoint add;
        add.secs = i * recIntSecs;
        add.km = last.km; // if not getting copied at least stay in same place!

        // fold in ride 1 values
        if (offset1 <= i && i < ride1->dataPoints().count() + offset1) {

            RideFilePoint source = *(ride1->dataPoints()[i - offset1]);

            // copy across the data we want
            QMapIterator<RideFile::SeriesType, QCheckBox*> io(leftSeries);
            while (io.hasNext()) {
                io.next();
                // we want this series !
                if (io.value()->isChecked()) {
                    add.setValue(io.key(), source.value(io.key()));
                }
            }
        }

        // maintain ride1 'nextdistance' index and distance (used for interpolating slope.)
        if (add.km >= ride1nextdistance) {
            while (offset1 <= ride1nextdistanceindex && ride1nextdistanceindex < ride1->dataPoints().count() + offset1) {
                RideFilePoint* point = (ride1->dataPoints()[ride1nextdistanceindex - offset1]);
                if (point->km != add.km) {
                    ride1nextdistance = point->km;
                    break;
                }

                ride1nextdistanceindex++;
            }
        }

        // Additional samples to interpolator
        while (gpi.WantsInput(add.km)) {
            if (ii < ride2->dataPoints().count()) {
                const RideFilePoint * pii = (ride2->dataPoints()[ii]);
                geolocation geo(pii->lat, pii->lon, pii->alt);
                gpi.Push(pii->km, geo);

                ii++;
            } else {
                gpi.NotifyInputComplete();
                break;
            }
        }

        // Maintain ride2 copy index
        while ((j < ride2->dataPoints().count()) && (ride2->dataPoints()[j]->km < add.km)) {
            j++;
        }

        // Compute interpolated location from current distance.
        geolocation interpLoc = gpi.Interpolate(add.km);

        RideFilePoint source = *(ride2->dataPoints()[j]);

        // fold in ride 2 values
        {
            // copy across the data we want
            QMapIterator<RideFile::SeriesType, QCheckBox*> io(rightSeries);
            while (io.hasNext()) {
                io.next();
                // we want this series !
                if (io.value()->isChecked()) {
                    // For location data substitute interpolated value for ride2 value.
                    switch (io.key()) {
                    case RideFile::lat:
                        add.setValue(io.key(), interpLoc.Lat());
                        break;
                    case RideFile::lon:
                        add.setValue(io.key(), interpLoc.Long());
                        break;
                    case RideFile::alt:
                        add.setValue(io.key(), interpLoc.Alt());
                        break;
                    case RideFile::slope:
                        {
                            // Obtain interpolated future altitude using next unique ride1 distance
                            // since that location is of the next altitude that will be recorded.
                            // This is more stable than using the actual point slope at current
                            // location and ensures that slope will match recorded altitudes.
                            double slope = 0.0;
                            if (ride1nextdistance != add.km)
                            {
                                geolocation interpLocE = gpi.Interpolate(ride1nextdistance);
                                double altitudeDeltaM = (interpLocE.Alt() - interpLoc.Alt());
                                double distanceDeltaM = 1000 * (ride1nextdistance - add.km);
                                slope = 100.0 * (altitudeDeltaM / distanceDeltaM);
                            }
                            add.setValue(io.key(), slope);
                        }
                        break;
                    default:
                        add.setValue(io.key(), source.value(io.key()));
                        break;
                    }
                }
            }
        }

        combined->appendPoint(add);
        last = add;
    }
}

void 
MergeActivityWizard::combine()
{
    // and build a new one
    combined = new RideFile(ride1);
    combinedItem->setRide(combined); // will delete old one

    // create a combined ride applying the parameters
    // from the wizard for join or merge

    if (mode == 1) { // JOIN

        // easy peasy -- loop through one then the other !
        RideFilePoint *lp = NULL;

        foreach(RideFilePoint *p, ride1->dataPoints()) {
            combined->appendPoint(*p);
            lp = p;
        }

        // preserve XData from first ride
        foreach (XDataSeries *xdata, ride1->xdata())
            combined->addXData(xdata->name, new XDataSeries (*xdata));

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

        // and XData from second ride, append if series already present
        foreach (XDataSeries *xdata, ride2->xdata()) {
            if (combined->xdata().contains(xdata->name)) {
                foreach (XDataPoint *point, xdata->datapoints) {
                    XDataPoint *pt = new XDataPoint(*point);
                    pt->secs = point->secs + timeOffset;
                    pt->km = point->km + distanceOffset;
                    combined->xdata(xdata->name)->datapoints.append(pt);
                }
            } else {
                XDataSeries *xd = new XDataSeries(*xdata);
                xd->datapoints.clear();
                foreach (XDataPoint *point, xdata->datapoints) {
                    XDataPoint *pt = new XDataPoint(*point);
                    pt->secs = point->secs + timeOffset;
                    pt->km = point->km + distanceOffset;
                    xd->datapoints.append(pt);
                }
                combined->addXData(xd->name, xd);
            }
        }

        // any intervals with a number name? find the last
        int intervalN=0;
        foreach(RideFileInterval *interval, ride1->intervals()) {
            int x = interval->name.toInt();
            if (interval->name == QString("%1").arg(x)) {
                if (x > intervalN) intervalN = x;
            }
        }

        // now run through the intervals for the second ride
        // and add them but renumber any intervals that are just numbers
        foreach(RideFileInterval *interval, ride2->intervals()) {
            int x = interval->name.toInt();
            if (interval->name == QString("%1").arg(x)) {
                interval->name = QString("%1").arg(x+intervalN);
            }
            combined->addInterval(interval->type,
                                  interval->start + timeOffset,
                                  interval->stop + timeOffset, 
                                  interval->name);
        }

    }
    else if(strategy == 4) {
        mergeRideSamplesByDistance();
    }
    else { // MERGE

        RideFilePoint last;

        for (int i=0; i<ride1->dataPoints().count() + offset1 ||
                      i<ride2->dataPoints().count() + offset2; i++) {

            // fresh point
            RideFilePoint add;
            add.secs = i * recIntSecs;
            add.km = last.km; // if not getting copied at least stay in same place!

            // fold in ride 1 values
            if (offset1 <= i && i < ride1->dataPoints().count()+offset1) {

                RideFilePoint source = *(ride1->dataPoints()[i-offset1]);
   
                // copy across the data we want
                QMapIterator<RideFile::SeriesType,QCheckBox*> i(leftSeries);
                while(i.hasNext()) {
                    i.next();
                    // we want this series !
                    if (i.value()->isChecked()) {
                        add.setValue(i.key(), source.value(i.key()));
                    }
                }
            }

            // fold in ride 2 values
            if (offset2 <= i && i < ride2->dataPoints().count()+offset2) {

                RideFilePoint source = *(ride2->dataPoints()[i-offset2]);
   
                // copy across the data we want
                QMapIterator<RideFile::SeriesType,QCheckBox*> i(rightSeries);
                while(i.hasNext()) {
                    i.next();
                    // we want this series !
                    if (i.value()->isChecked()) {
                        add.setValue(i.key(), source.value(i.key()));
                    }
                }
            }

            combined->appendPoint(add);
            last = add;
        }

        // Just preserve XData from first ride and add XData from the second,
        // if not already present, in the future we could let the user to
        // choose, like for standard series
        foreach (XDataSeries *xdata, ride1->xdata())
            combined->addXData(xdata->name, new XDataSeries (*xdata));
        foreach (XDataSeries *xdata, ride2->xdata()) {
            if (!combined->xdata().contains(xdata->name))
                combined->addXData(xdata->name, new XDataSeries (*xdata));
        }

        // now realign the intervals, first we need to
        // clear what we already have in combined
        combined->clearIntervals();

        // run through what we got then
        foreach(RideFileInterval *interval, ride1->intervals()) {
            combined->addInterval(interval->type,
                                  interval->start + offset1,
                                  interval->stop + offset1, 
                                  interval->name);
        }
        // run through what we got then
        foreach(RideFileInterval *interval, ride2->intervals()) {
            combined->addInterval(interval->type,
                                  interval->start + offset2,
                                  interval->stop + offset2, 
                                  interval->name);
        }
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
    p->setStyleSheet(QString("font-size: %1px;").arg(12 * dpiXFactor));
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "import");
    layout->addWidget(p);

    // download from a device
    p = new QCommandLinkButton(tr("Download from Device"), 
                                tr("Download data from a serial port device such as a Moxy Muscle Oxygen Monitor or"
                                   " bike computer to combine with the current activity."), this);
    p->setStyleSheet(QString("font-size: %1px;").arg(12 * dpiXFactor));
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "download");
    layout->addWidget(p);

    // select from rides on same day
    p = new QCommandLinkButton(tr("Existing Activity"), 
                                tr("Combine data from an activity that has already been imported or downloaded into"
                                   " GoldenCheetah. Selecting from a list of all available activities. "), this);
    p->setStyleSheet(QString("font-size: %1px;").arg(12 * dpiXFactor));
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
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setUniformRowHeights(true);
    files->setIndentation(0);

    // populate with each ride in the ridelist
    foreach (RideItem *rideItem, wizard->context->athlete->rideCache->rides()) {

        QTreeWidgetItem *add = new QTreeWidgetItem(files->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // we will wipe the original file
        add->setText(0, rideItem->fileName);
        add->setText(1, rideItem->dateTime.toString(tr("dd MMM yyyy")));
        add->setText(2, rideItem->dateTime.toString("hh:mm:ss"));
    }

    for(int i = 0; i < 3; i++)
        files->resizeColumnToContents(i);

    // Sort by date descending
    files->invisibleRootItem()->sortChildren(0, Qt::DescendingOrder);

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

    // start at same time
    p = new QCommandLinkButton(tr("Interpolate location data based upon distance"),
        tr("Merge the two activity streams, interpolating location and slope values based upon mutual distance."), this);
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, "distance");
    layout->addWidget(p);
    label = new QLabel("", this);
    layout->addWidget(label);

    next = 60;
    setFinalPage(false);
}

void
MergeStrategy::initializePage()
{
    // are there any shared data -- ignoring time and distance
    bool hasShared = false;
    QMapIterator<RideFile::SeriesType, QCheckBox *> i(wizard->rightSeries);
    while(i.hasNext()) {
        i.next();
        if (i.key() != RideFile::km && wizard->leftSeries.value(i.key(), NULL) != NULL)
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
    } else if (p == "distance") {
        wizard->strategy = 4; // merge ...
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
    setSubTitle(tr("Adjust merge alignment in time (use cursor and pgDown/pgUp keys for precise alignment)"));

    // need more space on this page!
    setContentsMargins(0,0,0,0);

    // Plot files
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->setSpacing(5 *dpiXFactor);
    layout->setContentsMargins(5,5,5,5);

    spanSlider = new QxtSpanSlider(Qt::Horizontal, this);
    spanSlider->setFocusPolicy(Qt::NoFocus);
    spanSlider->setHandleMovementMode(QxtSpanSlider::NoOverlapping);
    spanSlider->setLowerValue(0);
    spanSlider->setUpperValue(15);
#ifdef Q_OS_MAC
    // BUG in QMacStyle and painting of spanSlider
    // so we use a plain style to avoid it, but only
    // on a MAC, since win and linux are fine
#if QT_VERSION > 0x5000
    QStyle *style = QStyleFactory::create("fusion");
#else
    QStyle *style = QStyleFactory::create("Cleanlooks");
#endif
    spanSlider->setStyle(style);
#endif

    fullPlot = new AllPlot(this, NULL, wizard->context);
    fullPlot->setPaintBrush(0);
#ifdef Q_OS_MAC
    fullPlot->setFixedHeight(300);
#else
    fullPlot->setFixedHeight(300);
#endif
    fullPlot->setHighlightIntervals(false);
    static_cast<QwtPlotCanvas*>(fullPlot->canvas())->setBorderRadius(0);
    fullPlot->setWantAxis(false, true);
    QPalette pal = palette();
    fullPlot->axisWidget(QwtPlot::xBottom)->setPalette(pal);

    layout->addWidget(spanSlider);
    layout->addWidget(fullPlot);
    layout->addStretch();

    QLabel *adjust = new QLabel(tr("Adjust:"));
    offsetLabel = new QLabel("--");
    adjustSlider = new QSlider(Qt::Horizontal, this);
    reset = new QPushButton(tr("Reset"));

    QHBoxLayout *hl = new QHBoxLayout;
    hl->addWidget(adjust);
    hl->addWidget(offsetLabel);
    hl->addWidget(adjustSlider);
    hl->addWidget(reset);
    layout->addLayout(hl);
    layout->addStretch();

    connect(spanSlider, SIGNAL(lowerPositionChanged(int)), this, SLOT(zoomChanged()));
    connect(spanSlider, SIGNAL(upperPositionChanged(int)), this, SLOT(zoomChanged()));

    connect(reset, SIGNAL(clicked()), this, SLOT(resetClicked()));
    connect(adjustSlider, SIGNAL(valueChanged(int)), this, SLOT(offsetChanged()));
}

void
MergeAdjust::initializePage()
{
    // remember so we can reset
    offset1 = wizard->offset1;
    offset2 = wizard->offset2;

    // setup plot
    fullPlot->setDataFromRide(wizard->combinedItem, QList<UserData*>());
    spanSlider->setMinimum(0);
    spanSlider->setMaximum(wizard->combined->dataPoints().last()->secs);
    spanSlider->setLowerValue(spanSlider->minimum());
    spanSlider->setUpperValue(spanSlider->maximum());
    zoomChanged();

    // what to show?
    fullPlot->setShow(RideFile::none, false); // switch all off

    // now add in what we got
    QMapIterator<RideFile::SeriesType, QCheckBox *> i(wizard->rightSeries);
    while(i.hasNext()) {
        i.next();
        if (i.value()->isChecked())
            fullPlot->setShow(i.key(), true);
    }
    QMapIterator<RideFile::SeriesType, QCheckBox *> j(wizard->leftSeries);
    while(j.hasNext()) {
        j.next();
        if (j.value()->isChecked())
            fullPlot->setShow(j.key(), true);
    }

    // setup adjuster 
    adjustSlider->setMinimum(-1 * wizard->combined->dataPoints().count());
    adjustSlider->setMaximum(wizard->combined->dataPoints().count());
    adjustSlider->setValue(wizard->offset2 - wizard->offset1);

    // allow slider to be controlled by keyboard
    adjustSlider->setSingleStep(1);
    adjustSlider->setPageStep(10);
    adjustSlider->setFocusPolicy(Qt::StrongFocus);
}

void 
MergeAdjust::offsetChanged()
{
    offsetLabel->setText(QString("%1 secs").arg(adjustSlider->value() * -1));

    if (adjustSlider->value() < 0) {
        wizard->offset1 = adjustSlider->value() * -1;
        wizard->offset2 = 0;
    } else {
        wizard->offset1 = 0;
        wizard->offset2 = adjustSlider->value();
    }
    wizard->combine();

    fullPlot->setDataFromRide(wizard->combinedItem, QList<UserData*>());

    bool rescale = (spanSlider->minimum() == spanSlider->lowerValue() &&
                    spanSlider->maximum() == spanSlider->upperValue());

    spanSlider->setMinimum(0);
    spanSlider->setMaximum(wizard->combined->dataPoints().last()->secs);

    if (rescale) {
        spanSlider->setLowerValue(spanSlider->minimum());
        spanSlider->setUpperValue(spanSlider->maximum());
        zoomChanged();
    }
}

void 
MergeAdjust::resetClicked()
{
    wizard->offset1 = offset1;
    wizard->offset2 = offset2;
    initializePage();
}


void
MergeAdjust::zoomChanged()
{
    fullPlot->setAxisScale(QwtPlot::xBottom, spanSlider->lowerValue()/60.0f, spanSlider->upperValue()/60.0f);
    fullPlot->replot();
}

// select
MergeSelect::MergeSelect(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Merge Data Series"));
    setSubTitle(tr("Select the series to merge together"));

    QVBoxLayout *mainlayout = new QVBoxLayout(this);

    QFont def;
    def.setWeight(QFont::Bold);

    QHBoxLayout *namesHeader = new QHBoxLayout;
    leftNameHeader = new QLabel(tr("Target activity"), this);
    rightNameHeader = new QLabel(tr("Source of additional data series"), this);
    leftNameHeader->setFont(def);
    leftNameHeader->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    rightNameHeader->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    rightNameHeader->setFont(def);
    namesHeader->addWidget(leftNameHeader);
    namesHeader->addWidget(rightNameHeader);
    mainlayout->addLayout(namesHeader);

    QHBoxLayout *names = new QHBoxLayout;
    leftName = new QLabel("", this);
    rightName = new QLabel("", this);
    leftName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    rightName->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
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
    leftName->setText(QString("%1 (%2)").arg(wizard->ride1->startTime().toString(tr("d MMM yy hh:mm:ss")))
                                         .arg(wizard->ride1->deviceType()));

    rightName->setText(QString("%1 (%2)").arg(wizard->ride2->startTime().toString(tr("d MMM yy hh:mm:ss")))
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

    QLabel *label = new QLabel(tr("Press Finish to update the current activity with "
                               " the combined data.\n\n"
                               "The changes will be saved and cannot be undone.\n\n"
                               "If you press continue the activity will be saved, if you "
                               "do not want to continue either go back and change "
                               "the settings or press cancel to abort."));
    label->setWordWrap(true);

    layout->addWidget(label);
    layout->addStretch();
}

bool
MergeConfirm::validatePage()
{
    // We are done -- recalculate derived series, save and mark done
    wizard->current->notifyRideDataChanged();
    wizard->combined->recalculateDerivedSeries(true);
    wizard->current->setRide(wizard->combined);
    wizard->context->mainWindow->saveSilent(wizard->context, wizard->current);
    wizard->current->setDirty(false); // lose changes

    return true;
}
