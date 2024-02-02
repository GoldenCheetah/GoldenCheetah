/*
 * Copyright (c) 2016 Alejandro Martinez (amtriathlon@gmail.com)
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

#include "DataProcessor.h"
#include "Settings.h"
#include "Context.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>
#include <QHash>

#include <cmath>

// Config widget used by the Preferences/Options config panes
class FixRiderPosition;
class FixRiderPositionConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixRiderPositionConfig)

    friend class ::FixRiderPosition;
    protected:
        QHBoxLayout *layout;
        QLabel *noCadDelayLabel;
        QSpinBox *noCadDelay;
        QLabel *noKphDelayLabel;
        QSpinBox *noKphDelay;
        QLabel *noKphTriggerLabel;
        QSpinBox *noKphTrigger;

    public:
        FixRiderPositionConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixRiderPosition));

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            noCadDelayLabel = new QLabel(tr("Rest cadence delay (s)"));
            noKphDelayLabel = new QLabel(tr("Standby speed delay (s)"));
            noKphTriggerLabel = new QLabel(tr("Standby speed limit (kph)"));

            noCadDelay = new QSpinBox();
            noCadDelay->setMaximum(50);
            noCadDelay->setMinimum(0);
            noCadDelay->setSingleStep(1);
            noCadDelay->setValue(10);
            noKphDelay = new QSpinBox();
            noKphDelay->setMaximum(50);
            noKphDelay->setMinimum(0);
            noKphDelay->setSingleStep(1);
            noKphDelay->setValue(5);
            noKphTrigger = new QSpinBox();
            noKphTrigger->setMaximum(10);
            noKphTrigger->setMinimum(0);
            noKphTrigger->setSingleStep(1);
            noKphTrigger->setValue(1);
            layout->addWidget(noCadDelayLabel);
            layout->addWidget(noCadDelay);
            layout->addWidget(noKphDelayLabel);
            layout->addWidget(noKphDelay);
            layout->addWidget(noKphTriggerLabel);
            layout->addWidget(noKphTrigger);
            layout->addStretch();
        }

        //~FixRiderPositionConfig() {}  // deliberately not declared since Qt will
                                // delete the widget and its children when
                                // the config pane is deleted

        QString explain() {
            return tr("Power sensor will indicate rider \"position transition\" "
                      "when rider position change is supposed.\n"
                      "When this is not confirmed position have to be "
                      "considered as previous position.\n"
                      "In this case we have to remove those inaccurate "
                      "transitions. Otherwise, when position is confirmed, "
                      "we have to use the transition event as initial "
                      "timestamp for this new position.\n"
                      "In addition this fix procedure will convert "
                      "seated position with no cadence into \"aero\" position "
                      "and timeframes without movement into \"off\" position.\n");
        }

        void readConfig() {
            noCadDelay->setValue(appsettings->value(NULL, GC_FIXPOSITION_NOCAD_DELAY, "0").toDouble());
        }

        void saveConfig() {
            appsettings->setValue(GC_FIXPOSITION_NOCAD_DELAY, noCadDelay->value());
        }
};


// RideFile Dataprocessor -- used to update sample information
//                           based on rider position data
//                           to ensure consistency after edits.
//

class FixRiderPosition : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixRiderPosition)

    public:
        FixRiderPosition() {}
        ~FixRiderPosition() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixRiderPositionConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Fix Rider Position"));
        }
};

static bool FixRiderPositionAdded = DataProcessorFactory::instance().registerProcessor(QString("Fix Rider Position"), new FixRiderPosition());

bool
FixRiderPosition::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    // get settings
    double noCadDelay;
    double noKphDelay;
    double noKphTrigger;
    if (config == NULL) { // being called automatically
        noCadDelay = appsettings->value(NULL, GC_FIXPOSITION_NOCAD_DELAY, "0").toDouble();
        noKphDelay = appsettings->value(NULL, GC_FIXPOSITION_NOKPH_DELAY, "0").toDouble();
        noKphTrigger = appsettings->value(NULL, GC_FIXPOSITION_NOKPH_TRIGGER, "0").toDouble();
    } else { // being called manually
        noCadDelay = ((FixRiderPositionConfig*)(config))->noCadDelay->value();
        noKphDelay = ((FixRiderPositionConfig*)(config))->noKphDelay->value();
        noKphTrigger = ((FixRiderPositionConfig*)(config))->noKphTrigger->value();
    }

    if (noCadDelay == 0.0) // If Rest cadence is not configured set it to 10s (empriric)
        noCadDelay = 10.0;

    if (noKphDelay == 0.0) // If no speed delay is not configured set it to 5s (empriric)
        noKphDelay = 5.0;

    if (noKphTrigger == 0.0) // If no speed is not configured set it to 1kph (empriric)
        noKphTrigger = 1.0;

    // assert activity is bike
    if (noCadDelay == 0.0 || noKphDelay == 0.0 || noKphTrigger == 0.0 || !ride->isBike()) return false;

    // get POSITION XData indices and check for POSITION XData
    XDataSeries *series = ride->xdata("POSITION");
    if (!series || series->datapoints.isEmpty()) return false;

    int positionIdx = -1;
    for (int a=0; a<series->valuename.count(); a++) {
        if (series->valuename.at(a) == "POSITION")
            positionIdx = a;
    }
    // Position is mandatory
    if (positionIdx == -1) return false;

    // ok, lets start a transaction and process existing XDATA
    ride->command->startLUW("Fix Rider Position");


    // process change to "aero" position when cadence = 0
    if (ride->isDataPresent(ride->cad) && ride->isDataPresent(ride->kph)) {

        double noCadStartTime = 0.0;
        double noKphStartTime = 0.0;
        double kmStartAero = 0.0;
        double kmStartOff = 0.0;
        double noCadEndTime = 0.0;
        double noKphEndTime = 0.0;
        double kmEndAero = 0.0;
        double kmEndOff = 0.0;

        foreach (const RideFilePoint* pt, ride->dataPoints()) {

            if (pt->cad==0.0 && pt->kph>noKphTrigger && noCadStartTime==0.0) {
                noCadStartTime=pt->secs;
                kmStartAero=pt->km;
            }

            if (pt->kph<=noKphTrigger && noKphStartTime==0.0) {
                noKphStartTime=pt->secs;
                kmStartOff=pt->km;
            }

            // Check speed feedback to determine "Off" position
            // Note: "Off" position take precedence over "Aero" position
            // when speed came back above noKphTrigger then we fulfill previous period as "off"
            // observed speed was below trigger since configured delay thus add
            // position "off" in XDATA or change existing to "off"
            if (pt->kph>noKphTrigger && noKphStartTime!=0.0 && noKphEndTime==0.0
                    && (pt->secs-noKphStartTime>noKphDelay)) {

                noKphEndTime=pt->secs;
                kmEndOff=pt->km;

                // additional point when going to "off"
                for (int i=0; i< series->datapoints.count(); i++) {
                    XDataPoint *p = series->datapoints.at(i);
                    if (p->secs==noKphStartTime) {
                        ride->command->setXDataPointValue("POSITION", i, positionIdx+2, static_cast<double>(RealtimeData::off));
                        // qDebug() << "off: change position to 'off' for start point";
                        break;
                    }

                    if (p->secs > noKphStartTime) {
                        XDataPoint *addPoint = new XDataPoint;
                        addPoint->secs = noKphStartTime;
                        addPoint->km   = kmStartOff;
                        addPoint->number[positionIdx] = static_cast<double>(RealtimeData::off);
                        ride->command->insertXDataPoint("POSITION", i, addPoint);
                        // qDebug() << "off: add point with position 'off' for start point";
                        break;
                    }
                }

                // additional point when going back to seated
                for (int i=0; i< series->datapoints.count(); i++) {
                    XDataPoint *p = series->datapoints.at(i);
                    if (p->secs==noKphEndTime) {
                        ride->command->setXDataPointValue("POSITION", i, positionIdx+2, static_cast<double>(RealtimeData::seated));
                        // qDebug() << "off: change position to 'seated' for end point";
                        break;
                    }

                    if (p->secs > noKphEndTime) {
                        XDataPoint *addPoint = new XDataPoint;
                        addPoint->secs = noKphEndTime;
                        addPoint->km   = kmEndOff;
                        addPoint->number[positionIdx] = static_cast<double>(RealtimeData::seated);
                        ride->command->insertXDataPoint("POSITION", i, addPoint);
                        // qDebug() << "off: add point with position 'seated' for point end";
                        break;
                    }
                }

                // change position for points in between start and end
                for (int i=0; i< series->datapoints.count(); i++) {
                    XDataPoint *p = series->datapoints.at(i);
                    if ((noKphStartTime < p->secs) && (p->secs < noKphEndTime)) {
                        ride->command->setXDataPointValue("POSITION", i, positionIdx+2, static_cast<double>(RealtimeData::off));
                        // qDebug() << "off: change position to 'off' for points in between start and end";
                    }
                }
            }

            // when pedalling again we fulfill previous period as "aero"
            // observed cadence was 0 since configured delay thus add
            // position "aero" in XDATA or change existing to "aero"
            if (pt->cad!=0.0 && pt->kph>noKphTrigger && noCadStartTime!=0.0 && noCadEndTime==0.0
                    && (pt->secs-noCadStartTime>noCadDelay)) {

                noCadEndTime=pt->secs;
                kmEndAero=pt->km;


                // additional point when going to aero
                for (int i=0; i< series->datapoints.count(); i++) {
                    XDataPoint *p = series->datapoints.at(i);
                    if (p->secs==noCadStartTime) {
                        ride->command->setXDataPointValue("POSITION", i, positionIdx+2, static_cast<double>(RealtimeData::aero));
                        // qDebug() << "aero: change position to 'aero' for start point";
                        break;
                    }

                    if (p->secs > noCadStartTime) {
                        XDataPoint *addPoint = new XDataPoint;
                        addPoint->secs = noCadStartTime;
                        addPoint->km   = kmStartAero;
                        addPoint->number[positionIdx] = static_cast<double>(RealtimeData::aero);
                        ride->command->insertXDataPoint("POSITION", i, addPoint);
                        // qDebug() << "aero: add point with position 'aero' for start point";
                        break;
                    }
                }

                // additional point when going back to seated
                for (int i=0; i< series->datapoints.count(); i++) {
                    XDataPoint *p = series->datapoints.at(i);
                    if (p->secs==noCadEndTime) {
                        ride->command->setXDataPointValue("POSITION", i, positionIdx+2, static_cast<double>(RealtimeData::seated));
                        // qDebug() << "aero: change position to 'seated' for end point";
                        break;
                    }

                    if (p->secs > noCadEndTime) {
                        XDataPoint *addPoint = new XDataPoint;
                        addPoint->secs = noCadEndTime;
                        addPoint->km   = kmEndAero;
                        addPoint->number[positionIdx] = static_cast<double>(RealtimeData::seated);
                        ride->command->insertXDataPoint("POSITION", i, addPoint);
                        // qDebug() << "aero: add point with position 'seated' for end point";
                        break;
                    }
                }

                // change position for points in between start and end
                for (int i=0; i< series->datapoints.count(); i++) {
                    XDataPoint *p = series->datapoints.at(i);
                    if ((noCadStartTime < p->secs) && (p->secs < noCadEndTime)) {
                        ride->command->setXDataPointValue("POSITION", i, positionIdx+2, static_cast<double>(RealtimeData::aero));
                        // qDebug() << "aero: change position to 'aero' for points in between start and end";
                    }
                }
            }

            if (pt->cad!=0.0 || pt->kph<=noKphTrigger) {
                noCadStartTime = 0.0;
                noCadEndTime   = 0.0;
            }

            if (pt->kph>noKphTrigger) {
                noKphStartTime = 0.0;
                noKphEndTime   = 0.0;
            }

        }
    }

    // process transition events
    // based on ANT+ Device Profile - Bicycle Power Rev 5.1
    RealtimeData::riderPosition previousConfirmedPosition = RealtimeData::seated;
    int previousIdx = 0;
    for (int i=0; i< series->datapoints.count(); i++) {
        XDataPoint *currentPoint = series->datapoints.at(i);
        XDataPoint *previousPoint = series->datapoints.at(previousIdx);

        // if new position is "seated" and previous was "transistionToSeated"
        // then change previous position to "seated"
        if (previousPoint->number[positionIdx]==RealtimeData::transistionToSeated
            && currentPoint->number[positionIdx]==RealtimeData::seated)
        {
            previousPoint->number[positionIdx] = RealtimeData::seated;
        }
        // if new position is "standing" and previous was "transitionToStanding"
        // then change previous position to "standing"
        else if (previousPoint->number[positionIdx]==RealtimeData::transitionToStanding
            && currentPoint->number[positionIdx]==RealtimeData::standing)
        {
            previousPoint->number[positionIdx] = RealtimeData::standing;
        }

        // if previous position was "transitionTo..."
        // then change previous position to previous confirmed position
        else if (previousPoint->number[positionIdx]==RealtimeData::transistionToSeated
            || previousPoint->number[positionIdx]==RealtimeData::transitionToStanding)
        {
            previousPoint->number[positionIdx] = previousConfirmedPosition;
        }

        // if position is "seated" or "standing" or "aero" then
        // previous confirmed position = current position
        if (currentPoint->number[positionIdx]==RealtimeData::seated
            || currentPoint->number[positionIdx]==RealtimeData::standing
            || currentPoint->number[positionIdx]==RealtimeData::aero
            || currentPoint->number[positionIdx]==RealtimeData::off)
        {
            previousConfirmedPosition = static_cast<RealtimeData::riderPosition>(currentPoint->number[positionIdx]);
        }
        previousIdx=i;
    }

    ride->command->endLUW();
    // force metric update
    ride->context->rideItem()->isstale = true;
    ride->context->rideItem()->refresh();

    return true;
}
