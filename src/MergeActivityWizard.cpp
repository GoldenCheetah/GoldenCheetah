/*
 * Copyright (c) 2011-13 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

/**
 * Wizard to merge 2 files from same ride
 *
 **/
MergeActivityWizard::MergeActivityWizard(Context *context) : QWizard(context->mainWindow), context(context)
{
#ifdef Q_OS_MAX
    setWizardStyle(QWizard::ModernStyle);
#endif
    setWindowTitle(tr("Merge rides"));

    // current ride
    ride1 = const_cast<RideItem*>(context->currentRideItem());
    ride2 = NULL;

    // 5 step process, although Conflict may be skipped
    mergeWelcome = new MergeWelcome(this);
    addPage(mergeWelcome);
    mergeUpload = new MergeUpload(this);
    addPage(mergeUpload);
    mergeSync = new MergeSync(this);
    addPage(mergeSync);
    //mergeParameters = new MergeParameters(this);
    //addPage(mergeParameters);
    mergeSelect = new MergeSelect(this);
    addPage(mergeSelect);
    mergeConfirm = new MergeConfirm(this);
    addPage(mergeConfirm);
}


/*----------------------------------------------------------------------
 * Wizard Pages
 *--------------------------------------------------------------------*/

// welcome
MergeWelcome::MergeWelcome(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Merge Ride"));
    setSubTitle(tr("Lets get started"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QLabel *label = new QLabel(tr("This wizard will help you to merge 2 different files\n"
                               "from the same ride into a single file."));
    label->setWordWrap(true);

    layout->addWidget(label);
    layout->addStretch();
}

// Upload
MergeUpload::MergeUpload(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Upload"));
    setSubTitle(tr("Select file to merge"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    //: Do not change the time format in translation, keep hh:mm:ss !
    QLabel *ride1Label = new QLabel(tr("Current ride")+" "+parent->ride1->dateTime.toString(tr("MMM d, yyyy - hh:mm:ss")));
    layout->addWidget(ride1Label);
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,QSizePolicy::Expanding );
    layout->addItem( spacer );

    QLabel *label = new QLabel(tr("Select the file to merge to this ride."));
    label->setWordWrap(true);

    layout->addWidget(label);

    uploadButton = new QPushButton(tr("&Upload"));
    layout->addWidget(uploadButton);
    connect(uploadButton, SIGNAL(clicked()), this, SLOT(importFile()));

    labelSuccess = new QLabel("--");
    layout->addWidget(labelSuccess);
    ride2Label = new QLabel("");
    layout->addWidget(ride2Label);

    layout->addStretch();
}

void
MergeUpload::importFile()
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
        importFile(fileNamesCopy);
    }
}

void
MergeUpload::importFile(QList<QString> files)
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
                //wizard->addRideFile(ride);
                labelSuccess->setText(tr("File uploaded"));
                //: Do not change the time format in translation, keep hh:mm:ss !
                ride2Label->setText(tr("Second ride")+" "+ride->startTime().toString(tr("MMM d, yyyy - hh:mm:ss")));
                wizard->ride2 = ride;
                emit completeChanged();
            }

        } else {
            errors.append(tr("Error - Unknown file type"));
        }


    }
}


bool MergeUpload::isComplete() const
{
  return (wizard->ride2 != NULL);
}



// Synchronise start of files
MergeSync::MergeSync(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Synchronise"));
    setSubTitle(tr("Start of rides"));

    // Plot files
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QHBoxLayout *smallPlot1Layout = new QHBoxLayout;
    smallPlot1 = new SmallPlot(this);
    smallPlot1->setData(parent->ride1);
    smallPlot1->setFixedHeight(150);
    smallPlot1Layout->addWidget(smallPlot1);
    layout->addItem(smallPlot1Layout);

    bg1 = new MergeSyncBackground(this, wizard->ride1->ride());
    bg1->attach(smallPlot1);

    QHBoxLayout *smallPlot2Layout = new QHBoxLayout;
    smallPlot2 = new SmallPlot(this);
    smallPlot2->setFixedHeight(150);
    smallPlot2Layout->addWidget(smallPlot2);
    layout->addItem(smallPlot2Layout);

    bg2 = new MergeSyncBackground(this);
    bg2->attach(smallPlot2);

    warning = new QLabel(this);
    warning->setWordWrap(true);
    QFont font;
    font.setWeight(QFont::Bold);
    warning->setFont(font);

    QHBoxLayout *delayLayout = new QHBoxLayout;
    QLabel *delayLabel = new QLabel(tr("Delay"), this);
    delayEdit = new QLineEdit(this);
    delayEdit->setFixedWidth(30);

    delaySlider = new QSlider(Qt::Horizontal, this);
    delaySlider->setTickPosition(QSlider::TicksBelow);
    delaySlider->setTickInterval(60);
    delaySlider->setMinimum(-600);
    delaySlider->setMaximum(600);
    delayEdit->setValidator(new QIntValidator(delayEdit));

    QPushButton *delayAutoButton = new QPushButton(tr("&Auto"), this);

    delayLayout->addWidget(delayLabel);
    delayLayout->addSpacing(10);
    delayLayout->addWidget(delayEdit);
    delayLayout->addSpacing(10);
    delayLayout->addWidget(delaySlider);
    delayLayout->addSpacing(10);
    delayLayout->addWidget(delayAutoButton);
    delayLayout->addStretch();
    layout->addItem(delayLayout);

    layout->addStretch();
    layout->addWidget(warning);

    connect(delayEdit, SIGNAL(editingFinished()), this, SLOT(setDelayFromLineEdit()));
    connect(delaySlider, SIGNAL(valueChanged(int)), this, SLOT(setDelayFromSlider()));
    connect(delayAutoButton, SIGNAL(clicked()), this, SLOT(findBestDelay()));
}

void
MergeSync::initializePage()
{
    bg2->setRide(wizard->ride2);
    smallPlot2->setData(wizard->ride2);

    // Search channels to synchronise
    watts = wizard->ride1->ride()->isDataPresent(RideFile::watts) &&
                 wizard->ride2->isDataPresent(RideFile::watts);

    cad = wizard->ride1->ride()->isDataPresent(RideFile::cad) &&
               wizard->ride2->isDataPresent(RideFile::cad);

    kph =  wizard->ride1->ride()->isDataPresent(RideFile::kph) &&
                wizard->ride2->isDataPresent(RideFile::kph);

    alt =  wizard->ride1->ride()->isDataPresent(RideFile::alt) &&
                wizard->ride2->isDataPresent(RideFile::alt);

    hr = wizard->ride1->ride()->isDataPresent(RideFile::hr) &&
              wizard->ride2->isDataPresent(RideFile::hr);

    seriesCount = 5; //watts, cad, kph, alt, hr;
    samplesCount = 5;
    samplesLength = 300;

    findBestDelay();
}

QVector<DataPoint*>
MergeSync::getSamplesForRide(RideFile *ride1)
{
    QVector<DataPoint*> sample;
    if (!ride1->isDataPresent(RideFile::secs))
        return sample;

    int index = 0;

    double minTime = ride1->getMinPoint(RideFile::secs).toDouble();
    double maxTime = ride1->getMaxPoint(RideFile::secs).toDouble();

    DataPoint *previousPoint = new DataPoint(ride1->getPointValue(index, RideFile::secs),  ride1->getPointValue(index, RideFile::watts),  ride1->getPointValue(index, RideFile::cad),  ride1->getPointValue(index, RideFile::kph), ride1->getPointValue(index, RideFile::alt), ride1->getPointValue(index++, RideFile::hr));
    DataPoint *currentPoint = new DataPoint(ride1->getPointValue(index, RideFile::secs),  ride1->getPointValue(index, RideFile::watts),  ride1->getPointValue(index, RideFile::cad),  ride1->getPointValue(index, RideFile::kph), ride1->getPointValue(index, RideFile::alt), ride1->getPointValue(index++, RideFile::hr));
    DataPoint *nextPoint = new DataPoint(ride1->getPointValue(index, RideFile::secs),  ride1->getPointValue(index, RideFile::watts),  ride1->getPointValue(index, RideFile::cad),  ride1->getPointValue(index, RideFile::kph), ride1->getPointValue(index, RideFile::alt), ride1->getPointValue(index++, RideFile::hr));

    for (int secs = minTime+1; secs < maxTime; ++secs) {
        double hr = 0.0, watts = 0.0, alt = 0.0, cad = 0.0, kph = 0.0;
        while (currentPoint->time <= secs) {
            // current point
            if (currentPoint->time>secs-1) {
                double pond = (currentPoint->time-secs+1);
                hr+=pond*currentPoint->hr;
                watts+=pond*currentPoint->watts;
                alt+=pond*currentPoint->alt;
                cad+=pond*currentPoint->cad;
                kph+=pond*currentPoint->kph;
            }
            previousPoint = currentPoint;
            currentPoint = nextPoint;

            //watts, cad, kph, alt, hr;
            //if (secs < maxTime-ride1->recIntSecs())
            if (index <= ride1->timeIndex(maxTime))
                nextPoint = new DataPoint(ride1->getPointValue(index, RideFile::secs),  ride1->getPointValue(index, RideFile::watts),  ride1->getPointValue(index, RideFile::cad),  ride1->getPointValue(index, RideFile::kph), ride1->getPointValue(index, RideFile::alt), ride1->getPointValue(index++, RideFile::hr));
        }
        // next point
        // pause ?
        if (currentPoint->time>secs+300) {
            secs = currentPoint->time;
            continue;
        }
        // next point contribution
        if (currentPoint->time>secs && previousPoint->time<secs) {
            double pond = (secs-previousPoint->time);
            hr+=pond*currentPoint->hr;
            watts+=pond*currentPoint->watts;
            alt+=pond*currentPoint->alt;
            cad+=pond*currentPoint->cad;
            kph+=pond*currentPoint->kph;
        }

        DataPoint *pt = new DataPoint(secs, watts, cad, kph, alt, hr);
        sample.append(pt);
    }
    return sample;
}

void
MergeSync::analyse(QVector<DataPoint*> points1, QVector<DataPoint*> points2, int analysesCount)
{
    for (int j=0;j<points2.count();j++) {
        for (int sample=0;sample<samplesCount;sample++) {
            DataPoint pt = diffForSeries(points1, points2, sample*samplesLength, samplesLength);
            for (int series=0;series<seriesCount;series++) {
                double r=1;
                //watts, cad, kph, alt, hr;
                if (series==0 && watts)
                    r = pt.watts;
                else if (series==1 && cad)
                    r = pt.cad;
                else if (series==2 && kph)
                    r = pt.kph;
                else if (series==3 && alt)
                    r = pt.alt;
                else if (series==4 && hr)
                    r = pt.hr;

                if (minR.count()<series+1) {
                    minR.append(QList<double>());
                }
                if (minR[series].count()<sample+1+analysesCount*samplesCount) {
                    minR[series].append(1);
                }
                if (delay.count()<series+1) {
                    delay.append(QList<int>());
                }
                if (delay[series].count()<sample+1+analysesCount*samplesCount) {
                    delay[series].append(1);
                }

                if ( r<minR[series][analysesCount*samplesCount+sample]) {
                    minR[series][analysesCount*samplesCount+sample] = r;
                    delay[series][analysesCount*samplesCount+sample] = (analysesCount?-1:1)*(j-sample*samplesLength);
                }
            }
        }
        points2.remove(0);
    }
}

void
MergeSync::findBestDelay()
{
    findDelays(wizard->ride1->ride(), wizard->ride2);
    //printDelays();
    int delay = bestDelay();
    //printDelays();
    setDelay(delay);
}

void
MergeSync::findDelays(RideFile *ride1, RideFile *ride2)
{
    QVector<DataPoint*> sample1 = getSamplesForRide(ride1);
    QVector<DataPoint*> sample2 = getSamplesForRide(ride2);

    analyse(sample1, sample2, 0);
    analyse(sample2, sample1, 1);
}

void
MergeSync::printDelays()
{
    for (int series=0;series<seriesCount;series++) {
        QString seriesName = "";
        if (series==0)
            seriesName = "watts";
        else if (series==1)
            seriesName = "cad";
        else if (series==2)
            seriesName = "kph";
        else if (series==3)
            seriesName = "alt";
        else if (series==4)
            seriesName = "hr";
        fprintf(stderr, "  series %d %s\n", series, seriesName.toLatin1().constData());

        for (int i=0;i<delay[series].count();i++) {
            fprintf(stderr, "   delay for %d.%d: %d - %f\n", i, series, delay[series].at(i), minR[series].at(i));
        }
    }
}

int
MergeSync::bestDelay()
{
    double minDiff=-1;
    for (int series=0;series<seriesCount;series++) {
        for (int i=0;i<delay[series].count();i++) {
            if (minR[series][i]<minDiff || minDiff==-1) {
                minDiff = minR[series][i];
                i=-1;
            }
            else if (minR[series][i]>minDiff*3 || minR[series][i]>samplesLength/10000.0) {
                delay[series].removeAt(i);
                minR[series].removeAt(i);
                i--;
            }
        }
    }

    int bestSeries = -1;
    int bestSeriesValue = 1; //minimum 2 results in series to confirm

    // No priority for a series
    // Delay from series with more good results (more than 1)
    // Only if value are in 10sec frame
    int result = 0;
    for (int series=0;series<seriesCount;series++) {
        if (delay[series].count()>bestSeriesValue) {
            double avg = delay[series].at(0);
            int minDelay = delay[series].at(0);
            int maxDelay = delay[series].at(0);
            for (int i=1;i<delay[series].count();i++) {
                avg += delay[series].at(i);
                minDelay=qMin(minDelay,delay[series].at(i));
                maxDelay=qMax(maxDelay,delay[series].at(i));
            }
            // Max difference 10 secs
            if (maxDelay-minDelay<=10) {
                result = round(avg/delay[series].count());
                bestSeries = series;
                bestSeriesValue = delay[series].count();
            }
        }
    }


    if (bestSeries != -1) {
        QString seriesName = "";
        if (bestSeries==0)
            seriesName = tr("watts");
        else if (bestSeries==1)
            seriesName = tr("cad");
        else if (bestSeries==2)
            seriesName = tr("kph");
        else if (bestSeries==3)
            seriesName = tr("alt");
        else if (bestSeries==4)
            seriesName = tr("hr");
        warning->setText(QString(tr("Delay on matching %1 series.")).arg(seriesName));
    } else
        warning->setText(QString(tr("Unable to match datas")));
    return result;
}

void
MergeSync::setDelay(int delay)
{
    wizard->delay = delay;
    delaySlider->setValue(delay);
    delayEdit->setText(QString("%1").arg(delay));
    if (delay>=0) {
        bg1->setStartTime(0);
        bg2->setStartTime(delay);
    } else {
        bg1->setStartTime(-delay);
        bg2->setStartTime(0);
    }

    double duration1 = wizard->ride1->ride()->getMaxPoint(RideFile::secs).toDouble()-wizard->ride1->ride()->dataPoints().at(0)->secs;
    double duration2 = wizard->ride2->getMaxPoint(RideFile::secs).toDouble()-wizard->ride2->dataPoints().at(0)->secs;
    double stop = 0;

    if (duration2 - duration1 >= delay)
        stop = duration1+delay;
    else
        stop = -duration2+delay;

    wizard->stop = stop;

    if (stop>0) {
        bg1->setEndTime(0);
        bg2->setEndTime(stop);
    } else if (stop<0) {
        bg1->setEndTime(-stop);
        bg2->setEndTime(0);
    }

    smallPlot1->replot();
    smallPlot2->replot();
}

void
MergeSync::setDelayFromLineEdit()
{
    int delay = delayEdit->text().toInt();
    setDelay(delay);
}

void
MergeSync::setDelayFromSlider()
{
    setDelay(delaySlider->value());
}

DataPoint
MergeSync::diffForSeries(QVector<DataPoint*> a1, QVector<DataPoint*> a2, int start, int length)
{
    DataPoint result(1,1,1,1,1,1);
    if (a1.count()-start < length  || a2.count()<length)
        return result;

    //watts, cad, kph, alt, hr;
    double totalWatts = 0, totalCad = 0, totalKph = 0, totalAlt = 0, totalHr = 0;
    double diffWatts = 0, diffCad = 0, diffKph = 0, diffAlt = 0, diffHr = 0;
    double offsetAlt = 0;
    double variabilityAlt = 0;

    for (int i=0;i<length;i++)
    {
        if (hr) {
            diffHr+=qAbs(a1[i+start]->hr-a2[i]->hr);
            totalHr+=a1[i+start]->hr;
        }
        if (watts) {
            diffWatts+=qAbs(a1[i+start]->watts-a2[i]->watts);
            totalWatts+=a1[i+start]->watts;
        }
        if (cad) {
            diffCad+=qAbs(a1[i+start]->cad-a2[i]->cad);
            totalCad+=a1[i+start]->cad;
        }
        if (kph) {
            diffKph+=qAbs(a1[i+start]->kph-a2[i]->kph);
            totalKph+=a1[i+start]->kph;
        }
        if (alt) {
            if (i==0)
                offsetAlt = a1[i+start]->alt-a2[i]->alt;
            else
                variabilityAlt += qAbs(a1[i+start]->alt-a1[i+start-1]->alt);
            diffAlt+=qAbs(a1[i+start]->alt-a2[i]->alt-offsetAlt);
            totalAlt+=qAbs(a1[i+start]->alt);
        }
    }

    result.alt = (totalAlt>0 && variabilityAlt>samplesLength/4) ? diffAlt/totalAlt : 1.0;
    result.kph = totalKph>0 ? diffKph/totalKph : 1.0;
    result.cad = totalCad>0 ? diffCad/totalCad : 1.0;
    result.watts = totalWatts>0 ? diffWatts/totalWatts : 1.0;
    result.hr = totalHr>0 ? diffHr/totalHr : 1.0;

    //fprintf(stderr, "   pt alt:%f kph:%f cad:%f watts:%f hr:%f \n", result.alt, result.kph, result.cad, result.watts, result.hr);
    return result;
}

// parameters
MergeParameters::MergeParameters(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Merge Parameters"));
    setSubTitle(tr("Configure how file are synchronised"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QLabel *label = new QLabel(tr("No parameters at this time.\n"));
    label->setWordWrap(true);

    layout->addWidget(label);

    QGridLayout *grid = new QGridLayout;

    layout->addLayout(grid);
    layout->addStretch();

}

void
MergeParameters::valueChanged()
{
    //wizard->minimumGap = minimumGap->value();
}

// select
MergeSelect::MergeSelect(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Select"));
    setSubTitle(tr("Select series for merged file."));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    QHBoxLayout *dataControl1Layout = new QHBoxLayout;

    powerGroup = new QButtonGroup;
    altPowerGroup = new QButtonGroup;
    hrGroup = new QButtonGroup;
    speedGroup = new QButtonGroup;
    cadGroup = new QButtonGroup;
    altGroup = new QButtonGroup;
    gpsGroup = new QButtonGroup;

    keepPower1 = new QRadioButton(tr("Power"));
    powerGroup->addButton(keepPower1);
    dataControl1Layout->addWidget(keepPower1);

    keepAltPower1 = new QRadioButton(tr("AltPower"));
    altPowerGroup->addButton(keepAltPower1);
    dataControl1Layout->addWidget(keepAltPower1);

    keepHr1 = new QRadioButton(tr("Heart Rate"));
    hrGroup->addButton(keepHr1);
    dataControl1Layout->addWidget(keepHr1);

    keepSpeed1 = new QRadioButton(tr("Speed"));
    speedGroup->addButton(keepSpeed1);
    dataControl1Layout->addWidget(keepSpeed1);

    keepCad1 = new QRadioButton(tr("Cadence"));
    cadGroup->addButton(keepCad1);
    dataControl1Layout->addWidget(keepCad1);

    keepAlt1 = new QRadioButton(tr("Altitude"));
    altGroup->addButton(keepAlt1);
    dataControl1Layout->addWidget(keepAlt1);

    keepGPS1 = new QRadioButton(tr("GPS"));
    gpsGroup->addButton(keepGPS1);
    dataControl1Layout->addWidget(keepGPS1);

    mainLayout->addLayout(dataControl1Layout);

    QHBoxLayout *dataControl2Layout = new QHBoxLayout;

    keepPower2 = new QRadioButton(tr("Power"), this);
    powerGroup->addButton(keepPower2);
    dataControl2Layout->addWidget(keepPower2);

    keepAltPower2 = new QRadioButton(tr("AltPower"), this);
    altPowerGroup->addButton(keepAltPower2);
    dataControl2Layout->addWidget(keepAltPower2);

    keepHr2 = new QRadioButton(tr("Heart Rate"), this);
    hrGroup->addButton(keepHr2);
    dataControl2Layout->addWidget(keepHr2);

    keepSpeed2 = new QRadioButton(tr("Speed"), this);
    speedGroup->addButton(keepSpeed2);
    dataControl2Layout->addWidget(keepSpeed2);

    keepCad2 = new QRadioButton(tr("Cadence"), this);
    cadGroup->addButton(keepCad2);
    dataControl2Layout->addWidget(keepCad2);

    keepAlt2 = new QRadioButton(tr("Altitude"), this);
    altGroup->addButton(keepAlt2);
    dataControl2Layout->addWidget(keepAlt2);

    keepGPS2 = new QRadioButton(tr("GPS"), this);
    gpsGroup->addButton(keepGPS2);
    dataControl2Layout->addWidget(keepGPS2);

    mainLayout->addLayout(dataControl2Layout);

    QHBoxLayout *dataControl3Layout = new QHBoxLayout;

    noPower = new QRadioButton(tr("No Power"));
    powerGroup->addButton(noPower);
    dataControl3Layout->addWidget(noPower);

    noAltPower = new QRadioButton(tr("No Alt Power"));
    altPowerGroup->addButton(noAltPower);
    dataControl3Layout->addWidget(noAltPower);

    noHr = new QRadioButton(tr("No Heart Rate"));
    hrGroup->addButton(noHr);
    dataControl3Layout->addWidget(noHr);

    noSpeed = new QRadioButton(tr("No Speed"));
    speedGroup->addButton(noSpeed);
    dataControl3Layout->addWidget(noSpeed);

    noCad = new QRadioButton(tr("No Cadence"));
    cadGroup->addButton(noCad);
    dataControl3Layout->addWidget(noCad);

    noAlt = new QRadioButton(tr("No Altitude"));
    altGroup->addButton(noAlt);
    dataControl3Layout->addWidget(noAlt);

    noGPS = new QRadioButton(tr("No GPS"));
    gpsGroup->addButton(noGPS);
    dataControl3Layout->addWidget(noGPS);

    mainLayout->addLayout(dataControl3Layout);
    setLayout(mainLayout);

}

void
MergeSelect::initializePage()
{

    const RideFileDataPresent *dataPresent1 = wizard->ride1->ride()->areDataPresent();
    const RideFileDataPresent *dataPresent2 = wizard->ride2->areDataPresent();

    // Visible
    if (!dataPresent1->watts && !dataPresent2->watts) {
        keepPower1->setFixedWidth(0);
        keepPower2->setFixedWidth(0);
        noPower->setFixedWidth(0);
    }
    if (!dataPresent1->watts || !dataPresent2->watts) {
        keepAltPower1->setFixedWidth(0);
        keepAltPower2->setFixedWidth(0);
        noAltPower->setFixedWidth(0);
    }
    if (!dataPresent1->hr && !dataPresent2->hr) {
        keepHr1->setFixedWidth(0);
        keepHr2->setFixedWidth(0);
        noHr->setFixedWidth(0);
    }
    if (!dataPresent1->kph && !dataPresent2->kph) {
        keepSpeed1->setFixedWidth(0);
        keepSpeed2->setFixedWidth(0);
        noSpeed->setFixedWidth(0);
    }
    if (!dataPresent1->cad && !dataPresent2->cad) {
        keepCad1->setFixedWidth(0);
        keepCad2->setFixedWidth(0);
        noCad->setFixedWidth(0);
    }
    if (!dataPresent1->alt && !dataPresent2->alt) {
        keepAlt1->setFixedWidth(0);
        keepAlt2->setFixedWidth(0);
        noAlt->setFixedWidth(0);
    }
    if (!dataPresent1->lat && !dataPresent2->lat) {
        keepGPS1->setFixedWidth(0);
        keepGPS2->setFixedWidth(0);
        noGPS->setFixedWidth(0);
    }

    // Initialise
    if (&dataPresent1 != NULL && wizard->ride1->ride() && wizard->ride1->ride()->deviceType() != QString("Manual CSV")) {
            keepPower1->setEnabled(dataPresent1->watts);
            keepPower1->setChecked(dataPresent1->watts);
            keepHr1->setEnabled(dataPresent1->hr);
            keepHr1->setChecked(dataPresent1->hr);
            keepSpeed1->setEnabled(dataPresent1->kph);
            keepSpeed1->setChecked(dataPresent1->kph);
            keepCad1->setEnabled(dataPresent1->cad);
            keepCad1->setChecked(dataPresent1->cad);
            keepAlt1->setEnabled(dataPresent1->alt);
            keepAlt1->setChecked(dataPresent1->alt);
            keepGPS1->setEnabled(dataPresent1->lat);
            keepGPS1->setChecked(dataPresent1->lat);

    } else {
            keepPower1->setEnabled(false);
            keepHr1->setEnabled(false);
            keepSpeed1->setEnabled(false);
            keepCad1->setEnabled(false);
            keepAlt1->setEnabled(false);
            keepGPS1->setEnabled(false);
    }



    if (&dataPresent2 != NULL && wizard->ride2 && wizard->ride2->deviceType() != QString("Manual CSV")) {
            keepPower2->setEnabled(dataPresent2->watts);
            keepPower2->setChecked(dataPresent2->watts && !keepPower1->isChecked());
            keepHr2->setEnabled(dataPresent2->hr);
            keepHr2->setChecked(dataPresent2->hr && !keepHr1->isChecked());
            keepSpeed2->setEnabled(dataPresent2->kph);
            keepSpeed2->setChecked(dataPresent2->kph && !keepSpeed1->isChecked());
            keepCad2->setEnabled(dataPresent2->cad);
            keepCad2->setChecked(dataPresent2->cad && !keepCad1->isChecked());
            keepAlt2->setEnabled(dataPresent2->alt);
            keepAlt2->setChecked(dataPresent2->alt && !keepAlt1->isChecked());
            keepGPS2->setEnabled(dataPresent2->lat);
            keepGPS2->setChecked(dataPresent2->lat && !keepGPS1->isChecked());

    } else {
            keepPower2->setEnabled(false);
            keepHr2->setEnabled(false);
            keepSpeed2->setEnabled(false);
            keepCad2->setEnabled(false);
            keepAlt2->setEnabled(false);
            keepGPS2->setEnabled(false);
    }

    noPower->setChecked(!keepPower1->isChecked() && !keepPower2->isChecked());
    noAltPower->setChecked(!keepAltPower1->isChecked() && !keepAltPower2->isChecked());
    noHr->setChecked(!keepHr1->isChecked() && !keepHr2->isChecked());
    noSpeed->setChecked(!keepSpeed1->isChecked() && !keepSpeed2->isChecked());
    noCad->setChecked(!keepCad1->isChecked() && !keepCad2->isChecked());
    noAlt->setChecked(!keepAlt1->isChecked() && !keepAlt2->isChecked());
    noGPS->setChecked(!keepGPS1->isChecked() && !keepGPS2->isChecked());
}

MergeConfirm::MergeConfirm(MergeActivityWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Confirm"));
    setSubTitle(tr("Proceed with merge"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->addStretch();
}

bool
MergeConfirm::validatePage()
{
    // LETS DO IT NOW!
    QChar zero = QLatin1Char ( '0' );

    //--------------------------------------
    // STEP 1 : copy the second file as backup
    //--------------------------------------


    // Setup the ridetime as a QDateTime
    /*QDateTime ridedatetime2 = ride2->startTime();
    QString suffix2 = QFileInfo(file2->fileName()).suffix();

    QString targetnosuffix2 = QString ( "%1_%2_%3_%4_%5_%6" )
                               .arg ( ridedatetime2.date().year(), 4, 10, zero )
                               .arg ( ridedatetime2.date().month(), 2, 10, zero )
                               .arg ( ridedatetime2.date().day(), 2, 10, zero )
                               .arg ( ridedatetime2.time().hour(), 2, 10, zero )
                               .arg ( ridedatetime2.time().minute(), 2, 10, zero )
                               .arg ( ridedatetime2.time().second(), 2, 10, zero );

    QString target2 = QString ("%1.%2" )
                           .arg ( targetnosuffix2 )
                           .arg ( suffix2 );
    QString fulltarget2 = ride1->path + "/" + target2 + ".bak";

    file2->copy(fulltarget2);*/

    //--------------------------------------
    // STEP 2 : rename the first file as backup
    //--------------------------------------
    QFile   currentFile(wizard->ride1->path + QDir::separator() + wizard->ride1->fileName);
    QFile   savedFile;

    // rename as backup current
    currentFile.rename(currentFile.fileName(), currentFile.fileName() + ".bak");


    //--------------------------------------
    // STEP 3 : merge files
    //--------------------------------------
    RideItem *ride1 = wizard->ride1;
    RideFile *ride2 = wizard->ride2;
    //qDebug() << "- ride 1 : " << ride1->ride()->dataPoints().count() << " points\n";
    //qDebug() << "- ride 2 : " << ride2->dataPoints().count() << " points\n";

    if (wizard->mergeSelect->keepPower2->isChecked())
       ride1->ride()->setDataPresent(RideFile::watts, true);
    if (wizard->mergeSelect->keepSpeed2->isChecked())
       ride1->ride()->setDataPresent(RideFile::kph, true);
    if (wizard->mergeSelect->keepHr2->isChecked())
       ride1->ride()->setDataPresent(RideFile::hr, true);
    if (wizard->mergeSelect->keepCad2->isChecked())
       ride1->ride()->setDataPresent(RideFile::cad, true);
    if (wizard->mergeSelect->keepAlt2->isChecked())
       ride1->ride()->setDataPresent(RideFile::alt, true);
    if (wizard->mergeSelect->keepGPS2->isChecked()) {
       ride1->ride()->setDataPresent(RideFile::lon, true);
       ride1->ride()->setDataPresent(RideFile::lat, true);
    }

    if (wizard->mergeSelect->noPower->isChecked())
       ride1->ride()->setDataPresent(RideFile::watts, false);
    if (wizard->mergeSelect->noSpeed->isChecked())
       ride1->ride()->setDataPresent(RideFile::kph, false);
    if (wizard->mergeSelect->noHr->isChecked())
       ride1->ride()->setDataPresent(RideFile::hr, false);
    if (wizard->mergeSelect->noCad->isChecked())
       ride1->ride()->setDataPresent(RideFile::cad, false);
    if (wizard->mergeSelect->noAlt->isChecked())
       ride1->ride()->setDataPresent(RideFile::alt, false);
    if (wizard->mergeSelect->noGPS->isChecked()) {
       ride1->ride()->setDataPresent(RideFile::lon, false);
       ride1->ride()->setDataPresent(RideFile::lat, false);
    }

    //int i = 0;
    int k = 0;

    double previousSecs1 = -1;
    double previousSecs2 = -1;
    foreach (RideFilePoint *point, ride1->ride()->dataPoints()) {
        double secs = point->secs;

        RideFilePoint *point3 = new RideFilePoint();
        point3->watts = 0;
        point3->kph = 0;
        point3->hr = 0;
        point3->cad = 0;
        point3->alt = 0;
        point3->lon = 0;
        point3->lat = 0;

        // Manage delay
        if ( wizard->delay>=0) {
            while ((k <  ride2->dataPoints().count()) && (ride2->dataPoints().at(k)->secs < wizard->delay)) {
                previousSecs2 = ride2->dataPoints().at(k)->secs;
                k++;
            }
        } else {
            if (secs < -wizard->delay)
                continue;
        }
        // Manage end
        if ( wizard->stop<0) {
            if (point->secs>-wizard->stop)
                break;
        }

        // current point
        RideFilePoint *point2 = NULL;
        while ((k <  ride2->dataPoints().count()) && (ride2->dataPoints().at(k)->secs <= secs+wizard->delay)) {
            // current point
            point2 = ride2->dataPoints().at(k) ;
            double pond = (point2->secs-previousSecs2)/(secs-previousSecs1);

            point3->watts += pond * point2->watts;
            point3->kph += pond * point2->kph;
            point3->hr += pond * point2->hr;
            point3->cad += pond * point2->cad;
            point3->alt += pond * point2->alt;
            point3->lon += pond * point2->lon;
            point3->lat += pond * point2->lat;

            previousSecs2 = ride2->dataPoints().at(k)->secs;

            k++;
        }
        // next point contribution
        if (k <  ride2->dataPoints().count() && previousSecs2 < secs+wizard->delay) {
            // next point
            RideFilePoint *nextpoint2 = ride2->dataPoints().at(k) ;
            double pond = (secs+wizard->delay-previousSecs2)/(secs-previousSecs1);

            point3->watts += pond * nextpoint2->watts;
            point3->kph += pond * nextpoint2->kph;
            point3->hr += pond * nextpoint2->hr;
            point3->cad += pond * nextpoint2->cad;
            point3->alt += pond * nextpoint2->alt;
            point3->lon += pond * nextpoint2->lon;
            point3->lat += pond * nextpoint2->lat;

            previousSecs2 = secs+wizard->delay;
        }


        if (wizard->mergeSelect->keepPower2->isChecked()) {
           //qDebug() << "- watts : " << point->watts  << "-" << ((point2 && point2 != NULL)?point2->watts:0)  << "-" << point3->watts;
           point->watts = point3->watts;
        }

        if (wizard->mergeSelect->keepSpeed2->isChecked()) {
           //qDebug() << "- kph : " << point->kph  << "-" << point3->kph;
           point->kph = point3->kph;
        }

        if (wizard->mergeSelect->keepHr2->isChecked()) {
           //qDebug() << "- hr : " << point->hr  << "-" << point3->hr;
           point->hr = point3->hr;
        }

        if (wizard->mergeSelect->keepCad2->isChecked()) {
           //qDebug() << "- cad : " << point->cad  << "-" << point3->cad;
           point->cad = point3->cad;
        }

        if (wizard->mergeSelect->keepAlt2->isChecked()) {
           //qDebug() << "- alt : " << point->alt  << "-" << point3->alt;
           point->alt = point3->alt;
        }

        if (wizard->mergeSelect->keepGPS2->isChecked()) {
           //qDebug() << "- lat : " << point->lat  << "-" << point3->lat;
           //qDebug() << "- lon : " << point->lon  << "-" << point3->lon;
           point->lat = point3->lat;
           point->lon = point3->lon;
        }

        previousSecs1 = secs;
    }



    //--------------------------------------
    // STEP 4 : save the new (merged) file
    //--------------------------------------

    // Has the date/time changed?
    QDateTime ridedatetime = ride1->ride()->startTime();
    QString targetnosuffix = QString ( "%1_%2_%3_%4_%5_%6" )
                               .arg ( ridedatetime.date().year(), 4, 10, zero )
                               .arg ( ridedatetime.date().month(), 2, 10, zero )
                               .arg ( ridedatetime.date().day(), 2, 10, zero )
                               .arg ( ridedatetime.time().hour(), 2, 10, zero )
                               .arg ( ridedatetime.time().minute(), 2, 10, zero )
                               .arg ( ridedatetime.time().second(), 2, 10, zero );

    savedFile.setFileName(ride1->path + QDir::separator() + targetnosuffix + ".json");

    // save in GC format
    JsonFileReader reader;
    reader.writeRideFile(wizard->context, ride1->ride(), savedFile);
    ride1->setFileName(QFileInfo(savedFile).canonicalPath(), QFileInfo(savedFile).fileName());


    // We are done
    return true;
}
