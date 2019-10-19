/*
 * Copyright (c) 2009 Eric Murray (ericm@lne.com)
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "ManualRideDialog.h"
#include "Context.h"
#include "Colors.h"
#include "Athlete.h"
#include "RideCache.h"
#include "RideItem.h"
#include "Settings.h"
#include "RideMetadata.h"
#include <string.h>
#include <errno.h>
#include <QtGui>
#include <cmath>
#include "Units.h"
#include "HelpWhatsThis.h"

//
// Get the estimate factors
//
void 
ManualRideDialog::deriveFactors()
{
    // we already calculated for that day range
    if (days->value() == daysago) return;
    else daysago = days->value();

    // working variables
    timeKJ = distanceKJ = timeTSS = distanceTSS = timeBS = distanceBS = timeDP = distanceDP = 0.0;

    // do we have any rides?
    if (context->athlete->rideCache->rides().count()) {

        // last 'n' days calculation
        double seconds, distance, bs, dp, tss, kj;
        seconds = distance = tss = kj = bs = dp = 0;
        int rides = 0;

        // fall back to 'all time' calculation
        double totalseconds, totaldistance, totalbs, totaldp, totaltss, totalkj;
        totalseconds = totaldistance = totaltss = totalkj = totalbs = totaldp = 0;

        // iterate over the ride cache
        foreach (RideItem *ride, context->athlete->rideCache->rides()) {

            // skip those with no time or distance values (not comparing doubles)
            if (ride->getForSymbol("time_riding") == 0 || ride->getForSymbol("total_distance") == 0) continue;

            // how many days ago was it?
            int days =  ride->dateTime.daysTo(QDateTime::currentDateTime());

            // only use rides in last 'n' days
            if (days >= 0 && days < daysago) {

                bs += ride->getForSymbol("skiba_bike_score");
                seconds += ride->getForSymbol("time_riding");
                distance += ride->getForSymbol("total_distance");
                dp += ride->getForSymbol("daniels_points");
                tss += ride->getForSymbol("coggan_tss");
                kj += ride->getForSymbol("total_work");

                rides++;
            } 
            totalbs += ride->getForSymbol("skiba_bike_score");
            totalseconds += ride->getForSymbol("time_riding");
            totaldistance += ride->getForSymbol("total_distance");
            totaldp += ride->getForSymbol("daniels_points");
            totaltss += ride->getForSymbol("coggan_tss");
            totalkj += ride->getForSymbol("total_work");

        }

        // total values, not just last 'n' days -- but avoid divide by zero
        if (totalseconds && totaldistance) {
            if (!context->athlete->useMetricUnits) totaldistance *= MILES_PER_KM;
            timeBS = (totalbs * 3600) / totalseconds;  // BS per hour
            distanceBS = totalbs / totaldistance;  // BS per mile or km
            timeDP = (totaldp * 3600) / totalseconds;  // DP per hour
            distanceDP = totaldp / totaldistance;  // DP per mile or km
            timeTSS = (totaltss * 3600) / totalseconds;  // DP per hour
            distanceTSS = totaltss / totaldistance;  // DP per mile or km
            timeKJ = (totalkj * 3600) / totalseconds;  // DP per hour
            distanceKJ = totalkj / totaldistance;  // DP per mile or km
        }

        // don't use defaults if we have rides in last 'n' days
        if (rides) {
            if (!context->athlete->useMetricUnits) distance *= MILES_PER_KM;
            timeBS = (bs * 3600) / seconds;  // BS per hour
            distanceBS = bs / distance;  // BS per mile or km
            timeDP = (dp * 3600) / seconds;  // DP per hour
            distanceDP = dp / distance;  // DP per mile or km
            timeTSS = (tss * 3600) / seconds;  // DP per hour
            distanceTSS = tss / distance;  // DP per mile or km
            timeKJ = (kj * 3600) / seconds;  // DP per hour
            distanceKJ = kj / distance;  // DP per mile or km
        }
    }
}

ManualRideDialog::ManualRideDialog(Context *context) : context(context)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Manual Entry"));
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Activity_Manual));
#ifdef Q_OS_MAC
    setMinimumSize(610,415);
#else
    setMinimumSize(615 *dpiXFactor,360 *dpiYFactor);
#endif

    // we haven't derived factors yet
    daysago = -1;

    // we haven't lapsEditor yet
    lapsEditor = NULL;

    //
    // Create the GUI widgets
    //

    // BASIC DATA

    // ride start date and start time
    QLabel *dateLabel = new QLabel(tr("Date:"), this);
    dateEdit = new QDateEdit(this);
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setCalendarPopup(true);

    QLabel *timeLabel = new QLabel(tr("Start time:"), this);
    timeEdit = new QTimeEdit(this);
    timeEdit->setDisplayFormat("hh:mm:ss");
    timeEdit->setTime(QTime::currentTime().addSecs(-4 * 3600)); // 4 hours ago by default

    // Lap information
    QLabel *lapsLabel = new QLabel(tr("Pace intervals:"), this);
    lapsButton = new QPushButton(tr("&Laps Editor"), this);
    lapsButton->setEnabled(false); // it is enables when sport is selected

    // ride duration
    QLabel *durationLabel = new QLabel(tr("Duration:"), this);
    duration = new QTimeEdit(this);
    duration->setDisplayFormat("hh:mm:ss");

    // ride distance
    QString distanceString = QString(tr("Distance (%1):")).arg(context->athlete->useMetricUnits ? "km" : "miles");
    QLabel *distanceLabel = new QLabel(distanceString, this);
    distance = new QDoubleSpinBox(this);
    distance->setSingleStep(1.0);
    distance->setDecimals(2);
    distance->setMinimum(0);
    distance->setMaximum(999);

    QLabel *wcodeLabel = new QLabel(tr("Workout Code:"), this);
    wcode = new QLineEdit(this);
    QLabel *sportLabel = new QLabel(tr("Sport:"), this);
    sport = new QLineEdit(this);
    QLabel *notesLabel = new QLabel(tr("Notes:"), this);
    notes = new QTextEdit(this);
    notes->setAcceptRichText(false);

    // Set completer for Sport and Workout Code fields
    RideMetadata *rideMetadata = context->athlete->rideMetadata();
    if (rideMetadata) {
        foreach (FieldDefinition field, rideMetadata->getFields()) {
            if (field.name == "Sport")
                sport->setCompleter(field.getCompleter(this, context->athlete->rideCache));
            else if (field.name == "Workout Code")
                wcode->setCompleter(field.getCompleter(this, context->athlete->rideCache));
        }
    }

    // METRICS

    // average heartrate
    QLabel *avgBPMLabel = new QLabel(tr("Average HR:"), this);
    avgBPM = new QDoubleSpinBox(this);
    avgBPM->setSingleStep(1.0);
    avgBPM->setDecimals(0);
    avgBPM->setMinimum(0);
    avgBPM->setMaximum(250);

    // average power
    QLabel *avgWattsLabel = new QLabel(tr("Average Watts:"), this);
    avgWatts = new QDoubleSpinBox(this);
    avgWatts->setSingleStep(1.0);
    avgWatts->setDecimals(0);
    avgWatts->setMinimum(0);
    avgWatts->setMaximum(2500);

    // average cadence
    QLabel *avgRPMLabel = new QLabel(tr("Average Cadence:"), this);
    avgRPM = new QDoubleSpinBox(this);
    avgRPM->setSingleStep(1.0);
    avgRPM->setDecimals(0);
    avgRPM->setMinimum(0);
    avgRPM->setMaximum(250);

    // average speed
    QLabel *avgKPHLabel = new QLabel(tr("Average Speed:"), this);
    avgKPH = new QDoubleSpinBox(this);
    avgKPH->setSingleStep(1.0);
    avgKPH->setDecimals(1);
    avgKPH->setMinimum(0);
    avgKPH->setMaximum(100);

    // how to estimate stress
    QLabel *modeLabel = new QLabel(tr("Estimate Stress by:"), this);
    byDuration = new QRadioButton(tr("Duration"));
    byDistance = new QRadioButton(tr("Distance"));
    byManual = new QRadioButton(tr("Manually"));

    days = new QDoubleSpinBox(this);
    days->setSingleStep(1.0);
    days->setDecimals(0);
    days->setMinimum(0);
    days->setMaximum(999);
    days->setValue(appsettings->value(this, GC_BIKESCOREDAYS, "30").toInt());
    QLabel *dayLabel = new QLabel(tr("Estimate Stress days:"), this);

    // Restore from last time
    QVariant BSmode = appsettings->value(this, GC_BIKESCOREMODE); // remember from before
    if (BSmode.toString() == "time") byDuration->setChecked(true);
    else if (BSmode.toString() == "dist") byDistance->setChecked(true);
    else byManual->setChecked(true);

    // Derived metrics
    QLabel *BSLabel = new QLabel("BikeScore (TM): ", this);
    BS = new QDoubleSpinBox(this);
    BS->setSingleStep(1.0);
    BS->setDecimals(0);
    BS->setMinimum(0);
    BS->setMaximum(999);
    
    QLabel *DPLabel = new QLabel(tr("Daniel Points: "), this);
    DP = new QDoubleSpinBox(this);
    DP->setSingleStep(1.0);
    DP->setDecimals(0);
    DP->setMinimum(0);
    DP->setMaximum(999);

    QLabel *TSSLabel = new QLabel(tr("BikeStress: "), this);
    BikeStress = new QDoubleSpinBox(this);
    BikeStress->setSingleStep(1.0);
    BikeStress->setDecimals(0);
    BikeStress->setMinimum(0);
    BikeStress->setMaximum(999);

    QLabel *KJLabel = new QLabel(tr("Work (KJ):"), this);
    KJ = new QDoubleSpinBox(this);
    KJ->setSingleStep(1.0);
    KJ->setDecimals(0);
    KJ->setMinimum(0);
    KJ->setMaximum(9999);

    // buttons
    okButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);

    // save by default -- we don't overwrite and
    // the user will expect enter to save file
    okButton->setDefault(true);
    cancelButton->setDefault(false);
    lapsButton->setDefault(false);

    //
    // LAY OUT THE GUI
    //
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QHBoxLayout *squeeze = new QHBoxLayout;
    mainLayout->addLayout(squeeze);
    QGridLayout *basicLayout = new QGridLayout;
    squeeze->addLayout(basicLayout);
    QGridLayout *notesLayout = new QGridLayout;
    squeeze->addLayout(notesLayout);

    notesLayout->addWidget(wcodeLabel, 0,0, Qt::AlignLeft);
    notesLayout->addWidget(wcode, 0,1, Qt::AlignLeft);
    notesLayout->addWidget(sportLabel, 1,0, Qt::AlignLeft);
    notesLayout->addWidget(sport, 1,1, Qt::AlignLeft);
    notesLayout->addWidget(notesLabel, 2,0, Qt::AlignTop|Qt::AlignLeft);
    notesLayout->addWidget(notes, 2,1, Qt::AlignLeft);

    basicLayout->addWidget(dateLabel, 0,0,Qt::AlignLeft);
    basicLayout->addWidget(dateEdit, 0,1,Qt::AlignLeft);
    basicLayout->addWidget(timeLabel, 1,0,Qt::AlignLeft);
    basicLayout->addWidget(timeEdit, 1,1,Qt::AlignLeft);
    basicLayout->addWidget(lapsLabel, 2,0,Qt::AlignLeft);
    basicLayout->addWidget(lapsButton, 2,1, Qt::AlignLeft);
    basicLayout->addWidget(durationLabel, 3,0,Qt::AlignLeft);
    basicLayout->addWidget(duration, 3,1, Qt::AlignLeft);
    basicLayout->addWidget(distanceLabel, 4,0, Qt::AlignLeft);
    basicLayout->addWidget(distance, 4,1, Qt::AlignLeft);

    QGroupBox *metricBox = new QGroupBox(tr("Metrics"),this);
    QGridLayout *metricLayout = new QGridLayout;
    metricBox->setLayout(metricLayout);
    mainLayout->addWidget(metricBox);

    metricLayout->addWidget(avgBPMLabel, 0,0,Qt::AlignLeft);
    metricLayout->addWidget(avgBPM, 0,1,Qt::AlignLeft);
    metricLayout->addWidget(avgWattsLabel, 1,0,Qt::AlignLeft);
    metricLayout->addWidget(avgWatts, 1,1,Qt::AlignLeft);
    metricLayout->addWidget(avgRPMLabel, 2,0,Qt::AlignLeft);
    metricLayout->addWidget(avgRPM, 2,1,Qt::AlignLeft);
    metricLayout->addWidget(avgKPHLabel, 3,0,Qt::AlignLeft);
    metricLayout->addWidget(avgKPH, 3,1,Qt::AlignLeft);

    QHBoxLayout *estimateby = new QHBoxLayout;
    estimateby->addWidget(modeLabel);
    estimateby->addWidget(byDuration);
    estimateby->addWidget(byDistance);
    estimateby->addWidget(byManual);
    metricLayout->addLayout(estimateby, 0,2,1,-1,Qt::AlignLeft);


    QHBoxLayout *daysLayout = new QHBoxLayout;
    daysLayout->addWidget(dayLabel);
    daysLayout->addWidget(days);
    metricLayout->addLayout(daysLayout, 1,2,1,-1,Qt::AlignLeft);

    metricLayout->addWidget(TSSLabel, 2,2, Qt::AlignLeft);
    metricLayout->addWidget(BikeStress, 2,3, Qt::AlignLeft);
    metricLayout->addWidget(KJLabel, 2,4, Qt::AlignLeft);
    metricLayout->addWidget(KJ, 2,5, Qt::AlignLeft);
    metricLayout->addWidget(BSLabel, 3,2, Qt::AlignLeft);
    metricLayout->addWidget(BS, 3,3, Qt::AlignLeft);
    metricLayout->addWidget(DPLabel, 3,4, Qt::AlignLeft);
    metricLayout->addWidget(DP, 3,5, Qt::AlignLeft);

    QHBoxLayout *buttons = new QHBoxLayout;
    mainLayout->addLayout(buttons);
    buttons->addStretch();
    buttons->addWidget(okButton);
    buttons->addWidget(cancelButton);

    // sport is used to enable/disable and to configure LapsEditor
    connect(sport, SIGNAL(textChanged(QString)), this, SLOT(sportChanged(QString)));

    // if any of the fields used to determine estimation are changed then
    // lets re-calculate and apply
    connect(distance, SIGNAL(valueChanged(double)), this, SLOT(estimate()));
    connect(duration, SIGNAL(timeChanged(QTime)), this, SLOT(estimate()));
    connect(byDistance, SIGNAL(toggled(bool)), this, SLOT(estimate()));
    connect(byDuration, SIGNAL(toggled(bool)), this, SLOT(estimate()));
    connect(byManual, SIGNAL(toggled(bool)), this, SLOT(estimate()));
    connect(days, SIGNAL(valueChanged(double)), this, SLOT(estimate()));

    // dialog buttons
    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(lapsButton, SIGNAL(clicked()), this, SLOT(lapsClicked()));

    // initialise estimates / widgets enabled
    estimate();
}

ManualRideDialog::~ManualRideDialog() {
    delete lapsEditor;
}

void
ManualRideDialog::estimate()
{
    if (byManual->isChecked()) {
        BS->setEnabled(true);
        DP->setEnabled(true);
        BikeStress->setEnabled(true);
        KJ->setEnabled(true);

        return; // no calculation - manually entered by user
    } else {

        BS->setEnabled(false);
        DP->setEnabled(false);
        BikeStress->setEnabled(false);
        KJ->setEnabled(false);
    }

    deriveFactors(); // if days changed..

    if (byDuration->isChecked()) {
        // by time
        QTime time = duration->time();
        double hours = (time.hour()) + (time.minute() / 60.00) + (time.second() / 3600.00);

        BS->setValue(hours * timeBS);
        DP->setValue(hours * timeDP);
        BikeStress->setValue(hours * timeTSS);
        KJ->setValue(hours * timeKJ);

    } else {
        // by distance
        double dist = distance->value();

        BS->setValue(dist * distanceBS);
        DP->setValue(dist * distanceDP);
        BikeStress->setValue(dist * distanceTSS);
        KJ->setValue(dist * distanceKJ);
    }
}

void
ManualRideDialog::cancelClicked()
{
    reject();
}

void
ManualRideDialog::okClicked()
{
    // remember parameters
    appsettings->setValue(GC_BIKESCOREDAYS, days->value());
    appsettings->setValue(GC_BIKESCOREMODE,
        byDistance->isChecked() ? "dist" :  
       (byDuration->isChecked() ? "time" : "manual"));

    // create a new ridefile
    RideFile *rideFile = new RideFile();

    // set the first class variables
    QDateTime rideDateTime = QDateTime(dateEdit->date(), timeEdit->time());
    rideFile->setStartTime(rideDateTime);
    rideFile->setRecIntSecs(0.00);
    rideFile->setDeviceType("Manual");
    rideFile->setFileFormat("GoldenCheetah Json");

    // get samples from Laps Editor, if available
    if (lapsEditor && lapsEditor->dataPoints().count() > 0) {
        rideFile->setRecIntSecs(1.00);
        foreach(RideFilePoint *point, lapsEditor->dataPoints())
            rideFile->appendPoint(*point);
        rideFile->fillInIntervals();
    }

    // basic data
    if (distance->value()) {
        QMap<QString,QString> override;
        if (!context->athlete->useMetricUnits) {
            override.insert("value", QString("%1").arg(distance->value() * KM_PER_MILE));
        }else{
            override.insert("value", QString("%1").arg(distance->value()));
        }
        rideFile->metricOverrides.insert("total_distance", override);
    }

    QTime time = duration->time();
    double seconds = (time.hour() * 3600) + (time.minute() * 60.00) + (time.second());
    if (seconds) {
        QMap<QString,QString> override;
        override.insert("value", QString("%1").arg(seconds));
        rideFile->metricOverrides.insert("workout_time", override);
        // Override time_riding to duration
        // only if we don't have better information
        if (!lapsEditor || lapsEditor->dataPoints().count() == 0)
            rideFile->metricOverrides.insert("time_riding", override);
    }

    // basic metadata
    rideFile->setTag("Sport", sport->text());
    rideFile->setTag("Workout Code", wcode->text());
    rideFile->setTag("Notes", notes->document()->toPlainText());

    // average metrics
    if (avgBPM->value()) {
        QMap<QString,QString> override;
        override.insert("value", QString("%1").arg(avgBPM->value()));
        rideFile->metricOverrides.insert("average_hr", override);
    }
    if (avgRPM->value()) {
        QMap<QString,QString> override;
        override.insert("value", QString("%1").arg(avgRPM->value()));
        rideFile->metricOverrides.insert("average_cad", override);
    }
    if (avgKPH->value()) {
        QMap<QString,QString> override;
        // Avg Speed is shown according to units preferences
        double kph = (context->athlete->useMetricUnits ? 1.0 : KM_PER_MILE) * avgKPH->value();
        override.insert("value", QString("%1").arg(kph));
        rideFile->metricOverrides.insert("average_speed", override);
    }
    if (avgWatts->value()) {
        QMap<QString,QString> override;
        override.insert("value", QString("%1").arg(avgWatts->value()));
        rideFile->metricOverrides.insert("average_power", override);
    }

    // stress metrics
    if (BS->value()) {
        QMap<QString,QString> override;
        override.insert("value", QString("%1").arg(BS->value()));
        rideFile->metricOverrides.insert("skiba_bike_score", override);
    }
    if (DP->value()) {
        QMap<QString,QString> override;
        override.insert("value", QString("%1").arg(DP->value()));
        rideFile->metricOverrides.insert("daniels_points", override);
    }
    if (BikeStress->value()) {
        QMap<QString,QString> override;
        override.insert("value", QString("%1").arg(BikeStress->value()));
        rideFile->metricOverrides.insert("coggan_tss", override);
    }
    if (KJ->value()) {
        QMap<QString,QString> override;
        override.insert("value", QString("%1").arg(KJ->value()));
        rideFile->metricOverrides.insert("total_work", override);
    }

    // process linked defaults
    context->athlete->rideMetadata()->setLinkedDefaults(rideFile);

    // what should the filename be?
    QChar zero = QLatin1Char ('0');
    QString basename = QString ("%1_%2_%3_%4_%5_%6")
                           .arg (rideDateTime.date().year(), 4, 10, zero)
                           .arg (rideDateTime.date().month(), 2, 10, zero)
                           .arg (rideDateTime.date().day(), 2, 10, zero)
                           .arg (rideDateTime.time().hour(), 2, 10, zero)
                           .arg (rideDateTime.time().minute(), 2, 10, zero)
                           .arg (rideDateTime.time().second(), 2, 10, zero);

    QString filename = context->athlete->home->activities().canonicalPath() + "/" + basename + ".json";

    QFile out(filename);
    bool success = RideFileFactory::instance().writeRideFile(context, rideFile, out, "json");
    delete rideFile;

    if (success) {

        // refresh metric db etc
        context->athlete->addRide(basename + ".json", true);
        accept();

    } else {

        // rather than dance around renaming existing rides, this time we will let the user
        // work it out -- they may actually want to keep an existing ride, so we shouldn't
        // rename it silently.
        QMessageBox oops(QMessageBox::Critical, tr("Unable to save"),
                         tr("There is already an activity with the same start time or you do not have permissions to save a file."));
        oops.exec();
    }
}

void
ManualRideDialog::sportChanged(const QString& text)
{
    // remove previous Laps Editor and create a new one according to sport
    // only for swimming and running.
    delete lapsEditor;
    if (text == "Swim" || text == tr("Swim")) {
        lapsEditor = new LapsEditor(true);
        lapsButton->setEnabled(true);
    } else if (text == "Run" || text == tr("Run")) {
        lapsEditor = new LapsEditor(false);
        lapsButton->setEnabled(true);
    } else {
        lapsEditor = NULL;
        lapsButton->setEnabled(false);
    }
}

void
ManualRideDialog::lapsClicked()
{
    if (lapsEditor && lapsEditor->exec() && lapsEditor->dataPoints().count() > 0) {
        // update duration and distance
        duration->setTime(QTime(0, 0, 0).addSecs(lapsEditor->dataPoints().count()));
        distance->setValue(lapsEditor->dataPoints()[lapsEditor->dataPoints().count()-1]->km / (context->athlete->useMetricUnits ? 1.0 : KM_PER_MILE));
    }
}
