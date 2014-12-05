/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "SplitActivityWizard.h"
#include "MainWindow.h"
#include "Athlete.h"
#include "Context.h"

// Minimum gap in recording to find a natural break to split
static const double defaultMinimumGap = 1; // 1 minute

// Minimum size of segment to identify as a new ride
static const double defaultMinimumSegmentSize = 5; // 5 minutes

// Main wizard
SplitActivityWizard::SplitActivityWizard(Context *context) : QWizard(context->mainWindow), context(context)
{
#ifdef Q_OS_MAX
    setWizardStyle(QWizard::ModernStyle);
#endif

    // delete when done
    setAttribute(Qt::WA_DeleteOnClose);

    // Minimum 600x500 for when selecting intervals
    setMinimumHeight(500);
    setMinimumWidth(600);

    // title
    setWindowTitle(tr("Split Ride"));

    // set ride - unconst since we will wipe it away eventually
    rideItem = const_cast<RideItem*>(context->currentRideItem());

    // Set sensible defaults
    keepOriginal = false;
    minimumGap = defaultMinimumGap;
    minimumSegmentSize = defaultMinimumSegmentSize;
    usedMinimumSegmentSize = usedMinimumGap = -1;

    // set initial intervals list, will be adjusted
    // if the user modifies the default parameters
    intervals = new QTreeWidget;
    intervals->headerItem()->setText(0, tr(""));
    intervals->headerItem()->setText(1, tr("Start"));
    intervals->headerItem()->setText(2, tr(""));
    intervals->headerItem()->setText(3, tr("Stop"));
    intervals->headerItem()->setText(4, tr("Duration"));
    intervals->headerItem()->setText(5, tr("Distance"));
    intervals->headerItem()->setText(6, tr("Interval Name"));
    intervals->setColumnCount(7);
    intervals->setColumnWidth(0,30);
    intervals->setColumnWidth(1,80);
    intervals->setColumnWidth(2,30);
    intervals->setColumnWidth(3,80);
    intervals->setColumnWidth(4,80);
    intervals->setColumnWidth(5,80);
    intervals->setSelectionMode(QAbstractItemView::NoSelection);
    intervals->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    intervals->setUniformRowHeights(true);
    intervals->setIndentation(0);

    files = new QTreeWidget;
    files->headerItem()->setText(0, tr("Filename"));
    files->headerItem()->setText(1, tr("Date"));
    files->headerItem()->setText(2, tr("Time"));
    files->headerItem()->setText(3, tr("Duration"));
    files->headerItem()->setText(4, tr("Distance"));
    files->headerItem()->setText(5, tr("Action"));
    files->setColumnCount(6);
    files->setColumnWidth(0, 190); // filename
    files->setColumnWidth(1, 95); // date
    files->setColumnWidth(2, 90); // time
    files->setColumnWidth(3, 75); // duration
    files->setColumnWidth(4, 75); // distance
    files->setSelectionMode(QAbstractItemView::SingleSelection);
    files->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    files->setUniformRowHeights(true);
    files->setIndentation(0);

    // just the hr and power as a plot
    smallPlot = new SmallPlot(this);
    smallPlot->setFixedHeight(100);
    smallPlot->setData(rideItem);

    bg = new SplitBackground(this);
    bg->attach(smallPlot);

    // 5 step process, although Conflict may be skipped
    addPage(new SplitWelcome(this));
    addPage(new SplitKeep(this));
    addPage(new SplitParameters(this));
    addPage(new SplitSelect(this));
    addPage(new SplitConfirm(this));

    done = false;
}

void
SplitActivityWizard::setIntervalsList(SplitSelect *selector)
{
    // didn't change so no need to rebuild
    if (usedMinimumGap == minimumGap && usedMinimumSegmentSize == minimumSegmentSize) return;

    // clear the table
    intervals->clear();

    // convert to seconds
    int minimumGap = this->minimumGap * 60;
    int minimumSegmentSize = this->minimumSegmentSize * 60;

    // remember the last ones we used
    usedMinimumSegmentSize = this->minimumSegmentSize;
    usedMinimumGap = this->minimumGap;

    // find segments where gap is greater than minimumGap
    // and segment size is > minimumSize. If a segment is shorter
    // than minimumSize then ignore it (i.e. treat is as part of the gap)
    QList<RideFileInterval> segments;
    double segmentStart = 0;
    double segmentEnd = 0;

    double lastSecs = 0;
    bool first = true;

    int counter = 0;
    foreach (RideFilePoint *p, rideItem->ride()->dataPoints()) {

        if (first == true) {

            segmentStart = segmentEnd = lastSecs = p->secs;
            first = false;

        } else {

            if ((p->secs - segmentEnd) >= minimumGap) {

                if ((segmentEnd-segmentStart) >= minimumSegmentSize) {

                    // we have a candidate
                    segments.append(RideFileInterval(segmentStart, segmentEnd,
                                    QString(tr("Ride Segment #%1")).arg(++counter)));

                }
                segmentEnd = segmentStart = p->secs;

            } else {

                // keep accumulating
                segmentEnd = p->secs;
            }
        } 
        lastSecs = p->secs;
    }

    // we got to the end, is there a segment here?
    if ((segmentEnd-segmentStart) >= minimumSegmentSize) {

        // we have a candidate
        segments.append(RideFileInterval(segmentStart, segmentEnd,
                                             QString(tr("Ride Segment #%1")).arg(++counter)));

    }


    // now look at the segments and add any gaps in recording
    // because these MUST be honoured (i.e. user cannot unmark
    // these points, otherwise gaps in recording are retained
    // in the resulting file
    foreach(RideFileInterval *p, gaps) { delete p; } gaps.clear(); // wip old ones

    double lastsecs = rideItem->ride()->dataPoints().first()->secs;
    int gapnum = 0;
    foreach(RideFileInterval ride, segments) {
        if (ride.start > lastsecs) {
            // we have a gap
            gapnum++;

            // add to gap list
            RideFileInterval *gap = new RideFileInterval(lastsecs,
                                                         ride.start,
                                                         QString(tr("Gap in recording #%1")).arg(gapnum));
            gaps.append(gap);

            // add to interval list
            segments.append(RideFileInterval(*gap));
        }
        lastsecs = ride.stop;
    }
    if (lastsecs < rideItem->ride()->dataPoints().last()->secs) {
        // gap at the end
        gapnum++;

        // add to gap list
        RideFileInterval *gap = new RideFileInterval(lastsecs,
                                                     rideItem->ride()->dataPoints().last()->secs,
                                                     QString(tr("Gap in recording #%1")).arg(gapnum));
        gaps.append(gap);

        // add to interval list
        segments.append(RideFileInterval(*gap));
    }

    // first entry in list should always be entire file
    // so we can mark the start and stop for splitting
    segments.insert(0, RideFileInterval(rideItem->ride()->dataPoints().first()->secs,
                                     rideItem->ride()->dataPoints().last()->secs,
                                     tr("Entire Ride")));

    // now fold in the ride intervals
    segments.append(rideItem->ride()->intervals());

    // now lets sort the segments in start order
    qSort(segments.begin(), segments.end());

    // first just add all the current ride intervals
    counter = 0;
    QChar zero = QLatin1Char('0');
    foreach (RideFileInterval interval, segments) {

        // DO NOT skip intervals that are too short
        //if (interval.stop - interval.start < minimumSegmentSize) continue;

        QTreeWidgetItem *add = new QTreeWidgetItem(intervals->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // we set these intervals as checked by default
        bool checkit = (interval.name.startsWith(tr("Gap in recording")) ||
                        interval.name == tr("Entire Ride"));

        // disable checkbox editing (i.e. mandatory split) at gaps in recording
        // we have turned this off from user requests, may reinstate or choose
        // to fix gaps when they are left behind after split.
        bool disableit = false; /* (interval.name.startsWith("Gap in recording")); */

        // selector start
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(checkit);
        checkBox->setEnabled(!disableit);
        intervals->setItemWidget(add, 0, checkBox);

        connect(checkBox, SIGNAL(stateChanged(int)), selector, SLOT(refreshMarkers()));

        // interval start
        int secs = interval.start;
        add->setText(1, QString("%1:%2:%3")
                        .arg(secs/3600,2,10,zero)
                        .arg(secs%3600/60,2,10,zero)
                        .arg(secs%60,2,10,zero));

        // selector stop
        checkBox = new QCheckBox("", this);
        checkBox->setChecked(checkit);
        checkBox->setEnabled(!disableit);
        intervals->setItemWidget(add, 2, checkBox);

        connect(checkBox, SIGNAL(stateChanged(int)), selector, SLOT(refreshMarkers()));

        // interval start
        secs = interval.stop;
        add->setText(3, QString("%1:%2:%3")
                        .arg(secs/3600,2,10,zero)
                        .arg(secs%3600/60,2,10,zero)
                        .arg(secs%60,2,10,zero));

        // interval duration
        secs = interval.stop - interval.start;
        add->setText(4, QString("%1:%2:%3")
                        .arg(secs/3600,2,10,zero)
                        .arg(secs%3600/60,2,10,zero)
                        .arg(secs%60,2,10,zero));

        // interval distance
        double distance = rideItem->ride()->timeToDistance(interval.stop) - 
                          rideItem->ride()->timeToDistance(interval.start);
        add->setText(5, QString("%1 %2")
                        .arg(distance * (context->athlete->useMetricUnits ? 1 : MILES_PER_KM), 0, 'f', 2)
                        .arg(context->athlete->useMetricUnits ? "km" : "mi"));

        // interval name
        add->setText(6, interval.name);

        // hiddden columns with dataPoint from/to
        add->setText(7, QString("%1").arg(rideItem->ride()->timeIndex(interval.start)));
        add->setText(8, QString("%1").arg(rideItem->ride()->timeIndex(interval.stop)));

        counter++;
    }
    smallPlot->replot();
}

void
SplitActivityWizard::setFilesList()
{
    // clear the table
    files->clear();

    // fold in current ride -- if we are removing
    if (keepOriginal == false) {
        // we will wipe the original file
        QTreeWidgetItem *add = new QTreeWidgetItem(files->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        add->setText(0, rideItem->fileName);
        add->setText(1, rideItem->ride()->startTime().toString(tr("dd MMM yyyy")));
        add->setText(2, rideItem->ride()->startTime().toString("hh:mm:ss"));

        // get duration and distance, yuk, dup of code below, and from RideImportWizard
        int secs=0;
        double km=0;
        if (!rideItem->ride()->dataPoints().isEmpty() && rideItem->ride()->dataPoints().last() != NULL) {
            if (!secs) secs = rideItem->ride()->dataPoints().last()->secs;
            if (!km) km = rideItem->ride()->dataPoints().last()->km;
        }

        // set duration
        QChar zero = QLatin1Char ( '0' );
        QString time = QString("%1:%2:%3").arg(secs/3600,2,10,zero)
            .arg(secs%3600/60,2,10,zero)
            .arg(secs%60,2,10,zero);
        add->setText(3, time);

        // set distance
        QString dist = context->athlete->useMetricUnits
            ? QString ("%1 km").arg(km, 0, 'f', 1)
            : QString ("%1 mi").arg(km * MILES_PER_KM, 0, 'f', 1);
        add->setText(4, dist);

        // interval action
        add->setText(5, tr("Remove"));
    }

    // create a row for each file and action
    QChar zero = QLatin1Char('0');
    foreach (RideFile *ride, activities) {

        QTreeWidgetItem *add = new QTreeWidgetItem(files->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        QString filename = QString ("%1_%2_%3_%4_%5_%6.json")
                           .arg(ride->startTime().date().year(), 4, 10, zero)
                           .arg(ride->startTime().date().month(), 2, 10, zero)
                           .arg(ride->startTime().date().day(), 2, 10, zero)
                           .arg(ride->startTime().time().hour(), 2, 10, zero)
                           .arg(ride->startTime().time().minute(), 2, 10, zero)
                           .arg(ride->startTime().time().second(), 2, 10, zero);

        // filename
        add->setText(0, filename);

        // date and time
        add->setText(1, ride->startTime().toString(tr("dd MMM yyyy")));
        add->setText(2, ride->startTime().toString("hh:mm:ss"));

        // get duration and distance
        int secs=0;
        double km=0;
        if (!ride->dataPoints().isEmpty() && ride->dataPoints().last() != NULL) {
            if (!secs) secs = ride->dataPoints().last()->secs;
            if (!km) km = ride->dataPoints().last()->km;
        }

        // set duration
        QChar zero = QLatin1Char ( '0' );
        QString time = QString("%1:%2:%3").arg(secs/3600,2,10,zero)
            .arg(secs%3600/60,2,10,zero)
            .arg(secs%60,2,10,zero);
        add->setText(3, time);

        // set distance
        QString dist = context->athlete->useMetricUnits
            ? QString ("%1 km").arg(km, 0, 'f', 1)
            : QString ("%1 mi").arg(km * MILES_PER_KM, 0, 'f', 1);
        add->setText(4, dist);

        // interval action
        add->setText(5, tr("Create"));
    }

    // tidy up column widths
    files->resizeColumnToContents(5);
}

// if the file in question has a backup file already then
// return the filename (without the path), otherwise return
// an empty string
QString
SplitActivityWizard::hasBackup(QString filename)
{
    QString backupFilename = context->athlete->home->fileBackup().canonicalPath() + "/" + filename + ".bak";

    if (QFile(backupFilename).exists()) {

        return QString(filename + ".bak");

    } else {

        return "";
    }
}

QStringList
SplitActivityWizard::conflicts(QDateTime datetime)
{
    QStringList returning;

    // Check if an existing ride has the same starttime
    QChar zero = QLatin1Char('0');
    QString targetnosuffix = QString ("%1_%2_%3_%4_%5_%6")
                           .arg(datetime.date().year(), 4, 10, zero)
                           .arg(datetime.date().month(), 2, 10, zero)
                           .arg(datetime.date().day(), 2, 10, zero)
                           .arg(datetime.time().hour(), 2, 10, zero)
                           .arg(datetime.time().minute(), 2, 10, zero)
                           .arg(datetime.time().second(), 2, 10, zero);

    // now make a regexp for all know ride types
    foreach(QString suffix, RideFileFactory::instance().suffixes()) {

        QString conflict = context->athlete->home->activities().canonicalPath() + "/" + targetnosuffix + "." + suffix;
        if (QFile(conflict).exists()) returning << conflict;
    }
    return returning;
}

/*----------------------------------------------------------------------
 * Wizard Pages
 *--------------------------------------------------------------------*/

// welcome
SplitWelcome::SplitWelcome(SplitActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Split Ride"));
    setSubTitle(tr("Lets get started"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QLabel *label = new QLabel(tr("This wizard will help you split the current ride "
                               "into multiple rides\n\n"
                               "The wizard will identify segments of uninterrupted "
                               "ride and allow you to select which ones to "
                               "save as new rides. You will also be able to "
                               "select any currently defined intervals too.\n\n"
                               "If the newly created ride clashes with an existing "
                               "ride (same date and time) then the wizard will adjust "
                               "the start time by one or more seconds to avoid losing or "
                               "overwriting any existing rides."));
    label->setWordWrap(true);

    layout->addWidget(label);
    layout->addStretch();
}

// Keep original?
SplitKeep::SplitKeep(SplitActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Keep original"));
    setSubTitle(tr("Do you want to keep the original ride?"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QLabel *label = new QLabel(tr("If you want to keep the current ride then you "
                               "should ensure you have clicked on the \"Keep original "
                               "ride\" check box below.\n\n"
                               "If you do not choose to keep the original ride "
                               "it will be backed up before removing it from the "
                               "ride history.\n\n"));
    label->setWordWrap(true);

    keepOriginal = new QCheckBox(tr("Keep original ride"), this);
    keepOriginal->setChecked(wizard->keepOriginal);

    warning = new QLabel(this);
    warning->setWordWrap(true);
    QFont font;
    font.setWeight(QFont::Bold);
    warning->setFont(font);

    setWarning();

    layout->addWidget(label);
    layout->addWidget(keepOriginal);
    layout->addStretch();
    layout->addWidget(warning);

    connect(keepOriginal, SIGNAL(stateChanged(int)), this, SLOT(keepOriginalChanged()));
}

// parameters
SplitParameters::SplitParameters(SplitActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Split Parameters"));
    setSubTitle(tr("Configure how segments are found"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QLabel *label = new QLabel(tr("This wizard will find segments of ride to save "
                               "by looking for gaps in recording. \n\n"
                               "You can define the minimum length, in time, a gap "
                               "in recording should be in order to mark the end of "
                               "one segment and the beginning of another.\n\n"
                               "In addition, you can set a minimum segment size. "
                               "Any segment smaller than this limit will be ignored.\n\n"));
    label->setWordWrap(true);

    layout->addWidget(label);

    QGridLayout *grid = new QGridLayout;
    QLabel *minGap = new QLabel(tr("Minimum Gap (minutes)"), this);
    QLabel *minSize = new QLabel(tr("Minimum Segment Size (minutes)"), this);

    minimumGap = new QDoubleSpinBox(this);
    minimumGap->setDecimals(0);
    minimumGap->setSingleStep(1.0);
    minimumGap->setValue(wizard->minimumGap);

    minimumSegmentSize = new QDoubleSpinBox(this);
    minimumSegmentSize->setDecimals(0);
    minimumSegmentSize->setSingleStep(1.0);
    minimumSegmentSize->setValue(wizard->minimumSegmentSize);

    grid->addWidget(minGap,0,0);
    grid->addWidget(minimumGap,0,1,Qt::AlignLeft);
    grid->addWidget(minSize,1,0);
    grid->addWidget(minimumSegmentSize,1,1,Qt::AlignLeft);

    layout->addLayout(grid);
    layout->addStretch();

    connect (minimumGap, SIGNAL(valueChanged(double)), this, SLOT(valueChanged()));
    connect (minimumSegmentSize, SIGNAL(valueChanged(double)), this, SLOT(valueChanged()));
}

void
SplitParameters::valueChanged()
{
    wizard->minimumGap = minimumGap->value();
    wizard->minimumSegmentSize = minimumSegmentSize->value();
}

void
SplitKeep::keepOriginalChanged()
{
    wizard->keepOriginal = keepOriginal->isChecked();
    setWarning();
}

void
SplitKeep::setWarning()
{
    if (!keepOriginal->isChecked()) {

        if (wizard->hasBackup(wizard->rideItem->fileName) != "") {

            warning->setText(tr("WARNING: The current ride will be backed up and "
                             "removed, but a backup already exists. The existing "
                             "backup will therefore be overwritten."));
            return;
        }
    } 
    warning->setText("");
}

// Select
SplitSelect::SplitSelect(SplitActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Select Split Markers"));
    setSubTitle(tr("Ride will be split between marker points selected"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    layout->addWidget(wizard->smallPlot);
    layout->addWidget(wizard->intervals);
}
void
SplitSelect::initializePage()
{
    wizard->setIntervalsList(this);
    refreshMarkers();
}

void
SplitSelect::refreshMarkers()
{
    // update the markers on the plot
    foreach (QwtPlotMarker *m, wizard->markers) {
        // remove from the plot and delete
        m->detach();
        delete m;
    }
    wizard->markers.clear();
    wizard->marks.clear(); // dataPoint indexes

    // now refresh them
    for(int i=0; i<wizard->intervals->invisibleRootItem()->childCount(); i++) {

        QTreeWidgetItem *current = wizard->intervals->invisibleRootItem()->child(i);

        // add marker for start?
        if (static_cast<QCheckBox*>(wizard->intervals->itemWidget(current,0))->isChecked()) {

            long index = current->text(7).toInt();
            double point = wizard->rideItem->ride()->dataPoints().at(index)->secs;

            wizard->marks.append(index);
            QwtPlotMarker *add = new QwtPlotMarker;
            wizard->markers.append(add);

            // vertical line will do for now
            add->setLineStyle(QwtPlotMarker::VLine);
            add->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            add->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));
            add->setValue(point / 60.0, 0.0);
            add->attach(wizard->smallPlot);
        }

        // add marker for stop?
        if (static_cast<QCheckBox*>(wizard->intervals->itemWidget(current,2))->isChecked()) {

            long index = current->text(8).toInt();
            double point = wizard->rideItem->ride()->dataPoints().at(index)->secs;

            wizard->marks.append(index);
            QwtPlotMarker *add = new QwtPlotMarker;
            wizard->markers.append(add);

            // vertical line will do for now
            add->setLineStyle(QwtPlotMarker::VLine);
            add->setLabelAlignment(Qt::AlignRight | Qt::AlignTop);
            add->setLinePen(QPen(GColor(CPLOTMARKER), 0, Qt::DashDotLine));
            add->setValue(point / 60.0, 0.0);
            add->attach(wizard->smallPlot);
        }
    }
    wizard->smallPlot->replot();
}

// Confirm
SplitConfirm::SplitConfirm(SplitActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Confirm"));
    setSubTitle(tr("Split ride cannot be undone"));

    setCommitPage(true);
    setButtonText(QWizard::CommitButton, tr("Confirm"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    layout->addWidget(wizard->files);
}

// create an array of rides
void
SplitConfirm::initializePage()
{
    // clear the current array
    foreach(RideFile *ride, wizard->activities) {
        delete ride;
    }
    wizard->activities.clear();

    // create a sorted list of markers, since we
    // may have duplicates and the sequence is
    // not guaranteed to be ordered
    QList<long> points;
    foreach(long mark, wizard->marks) points.append(mark); // marks are indexes to ensure absolute accuracy
    qSort(points.begin(), points.end());

    // Create a new ride for each marked segment
    long lastmark = -1;
    foreach(long mark, points) {

        if (mark == lastmark) continue;

        if (lastmark != -1) {

            // ignore gaps!
            if (!wizard->bg->isGap(wizard->rideItem->ride()->dataPoints().at(lastmark)->secs/60.0,
                                   wizard->rideItem->ride()->dataPoints().at(mark)->secs/60.0)) {

                RideFile *add = createRideFile(lastmark, mark);
                wizard->activities.append(add);
            }
        }
        lastmark = mark;
    }

    // now lets adjust the starttime to avoid conflicts
    // and record adjustment in a metadata field for transparency
    // it will always conflict with current ride, so we pick that
    // up as a special case.
    // we check against existing rides AND the rides we WILL create
    QString originalFileName = wizard->context->athlete->home->activities().canonicalPath() + "/" + wizard->rideItem->fileName;
    QList<QDateTime> toBeCreated;
    foreach(RideFile *ride, wizard->activities) {

        int adjust = 0;
        QStringList conflicts = wizard->conflicts(ride->startTime());

        while (conflicts.count() || toBeCreated.contains(ride->startTime())) {

            // if the only conflict is with the original filename
            // and it will be wiped then don't worry about it
            // we also check conflicts of rides the WILl be created
            if (conflicts.count() == 1 &&
                conflicts.at(0) == originalFileName &&
                wizard->keepOriginal == false &&
                !toBeCreated.contains(ride->startTime())) {
                break;
            }

            adjust++;
            ride->setStartTime(ride->startTime().addSecs(1));
            conflicts = wizard->conflicts(ride->startTime());
        }

        // record the fact we adjusted...
        if (adjust) {
            ride->setTag("Start Time Adjust", QString("%1 seconds").arg(adjust));
        }
        toBeCreated.append(ride->startTime());
    }

    wizard->setFilesList();
}

//create a new ride file from the current ride file
//by using datapoints from index start to stop
RideFile *
SplitConfirm::createRideFile(long start, long stop)
{
    RideFile *returning = new RideFile; // target
    RideFile *ride = wizard->rideItem->ride(); // source

    // set offset in seconds, make sure in bounds too
    double offset = 0;
    double distanceoffset = 0;
    if (start < ride->dataPoints().count() && start >= 0) {
        offset = ride->dataPoints().at(start)->secs;
        distanceoffset = ride->dataPoints().at(start)->km;
    }

    // copy first class variables (adjust starttime to include offset)
    returning->setStartTime(ride->startTime().addSecs(offset));
    returning->setRecIntSecs(ride->recIntSecs());
    returning->setDeviceType(ride->deviceType());
    returning->setFileFormat(ride->fileFormat());

    // lets keep the metadata too
    const_cast<QMap<QString,QString>&>(returning->tags()) = QMap<QString,QString>(ride->tags());

    // now the dataPoints, check in bounds too!
    for(long i=start; i<stop && i<ride->dataPoints().count(); i++) {
        RideFilePoint *p = ride->dataPoints().at(i);

        returning->appendPoint(p->secs - offset, // start from zero!
                               p->cad, p->hr, p->km - distanceoffset, p->kph,
                               p->nm, p->watts, p->alt, p->lon, p->lat,
                               p->headwind, p->slope, p->temp, p->lrbalance, p->lte, p->rte, p->lps, p->rps,
                               p->smo2, p->thb,
                               p->rvert, p->rcad, p->rcontact,
                               p->interval);
    }

    // lets keep intervals that start in our section truncating them
    // if neccessary (some folks want to keep lap markers)
    double startTime = wizard->rideItem->ride()->dataPoints().at(start)->secs;
    double stopTime = wizard->rideItem->ride()->dataPoints().at(stop)->secs;
    foreach (RideFileInterval interval, wizard->rideItem->ride()->intervals()) {

        if (interval.start >= startTime && interval.start <= stopTime) {
            if (interval.stop > stopTime)
                returning->addInterval(interval.start - offset, stopTime, interval.name);
            else 
                returning->addInterval(interval.start - offset, interval.stop - offset, interval.name);
        }
    }
    return returning;
}

bool
SplitConfirm::validatePage()
{
    if(QMessageBox::question(this, tr("Confirm"),
       QString(tr("%1 file(s) will be created.\n\nAre you sure you wish to proceed?"))
       .arg(wizard->activities.count()),
       QMessageBox::Ok|QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {

        // LETS DO IT NOW!
        // first do we need to remove the current ride?
        if (wizard->keepOriginal == false) {

            wizard->context->athlete->removeCurrentRide();
            QTreeWidgetItem *current = wizard->files->invisibleRootItem()->child(0);
            current->setText(5, tr("Removed"));
        }

        // whizz through and create each new ride
        int off = wizard->keepOriginal ? 0 : 1; // skip first line to remove or not?
        for(int i=0; i<wizard->activities.count(); i++) {

            QTreeWidgetItem *current = wizard->files->invisibleRootItem()->child(i+off);
            QString target = wizard->context->athlete->home->activities().canonicalPath() + "/" + current->text(0);

            JsonFileReader reader;
            QFile out(target);
            reader.writeRideFile(wizard->context, wizard->activities.at(i), out);

            current->setText(5, tr("Saved"));

            wizard->context->athlete->addRide(QFileInfo(out).fileName(), true);
        }

        // now make this page the last (so we can see what was done)
        setTitle(tr("Completed"));
        setSubTitle(tr("Split Ride Completed"));

        wizard->done = true;

        return true;

    } else return false;
}

bool
SplitConfirm::isComplete() const
{
    return !wizard->done;
}
