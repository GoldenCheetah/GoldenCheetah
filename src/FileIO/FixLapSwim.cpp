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

#include <cmath>

// Config widget used by the Preferences/Options config panes
class FixLapSwim;
class FixLapSwimConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixLapSwimConfig)

    friend class ::FixLapSwim;
    protected:
        QHBoxLayout *layout;
        QLabel *plLabel;
        QSpinBox *pl;

    public:
        FixLapSwimConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixLapSwim));

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            plLabel = new QLabel(tr("Pool Length (m)"));

            pl = new QSpinBox();
            pl->setMaximum(50);
            pl->setMinimum(0);
            pl->setSingleStep(1);
            layout->addWidget(plLabel);
            layout->addWidget(pl);
            layout->addStretch();
        }

        //~FixLapSwimConfig() {}  // deliberately not declared since Qt will
                                // delete the widget and its children when
                                // the config pane is deleted

        QString explain() {
            return tr("Sometimes length times or distances from "
                      "lap swimming files are wrong.\n"
                      "You can fix length-by-length information "
                      "in Editor SWIM tab: TYPE, DURATION and STROKES.\n"
                      "This tool recompute accumulated time, distance "
                      "and sample Speed/Cadence from the updated lengths.\n"
                      "Laps are recreated using pause lengths as markers\n\n"
                      "Pool Length (in meters) allows to re-define the "
                      "field if value is > 0");
        }

        void readConfig() {
            pl->setValue(appsettings->value(NULL, GC_DPFLS_PL, "0").toDouble());
        }

        void saveConfig() {
            appsettings->setValue(GC_DPFLS_PL, pl->value());
        }
};


// RideFile Dataprocessor -- used to update sample information
//                           based on length-by-length data
//                           to ensure consistency after edits.
//
class FixLapSwim : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixLapSwim)

    public:
        FixLapSwim() {}
        ~FixLapSwim() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixLapSwimConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Fix Lap Swim from Length Data"));
        }
};

static bool FixLapSwimAdded = DataProcessorFactory::instance().registerProcessor(QString("Fix Lap Swim"), new FixLapSwim());

bool
FixLapSwim::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)

    // get settings
    double pl;
    if (config == NULL) { // being called automatically
        pl = appsettings->value(NULL, GC_DPFLS_PL, "0").toDouble();
    } else { // being called manually
        pl = ((FixLapSwimConfig*)(config))->pl->value();
    }

    if (pl == 0.0) // If Pool Length is not configured, get from metadata
        pl = ride->getTag("Pool Length", "0").toDouble();

    // no dice if we don't know Pool Length or the activity is not a swim
    if (pl == 0.0 || !ride->isSwim()) return false;

    // get SWIM XData indices and check for SWIM XData
    XDataSeries *series = ride->xdata("SWIM");
    if (!series || series->datapoints.isEmpty()) return false;

    int typeIdx = -1, durationIdx = -1, strokesIdx = -1, restIdx = -1;
    for (int a=0; a<series->valuename.count(); a++) {
        if (series->valuename.at(a) == "TYPE")
            typeIdx = a;
        else if (series->valuename.at(a) == "DURATION")
            durationIdx = a;
        else if (series->valuename.at(a) == "STROKES")
            strokesIdx = a;
        else if (series->valuename.at(a) == "REST")
            restIdx = a;
    }
    // Stroke Type or Duration are mandatory, Strokes only to compute cadence
    if (typeIdx == -1 || durationIdx == -1) return false;

    QVariant GarminHWM = appsettings->value(NULL, GC_GARMIN_HWMARK);
    if (GarminHWM.isNull() || GarminHWM.toInt() == 0) GarminHWM.setValue(25); // default to 25 seconds.

    // Preserve HR data
    QVector<double> hrdata(ride->dataPoints().count());
    for (int i=0; i<ride->dataPoints().count(); i++)
        hrdata[i] = ride->dataPoints()[i]->hr;

    // delete current lap markers
    ride->clearIntervals();
    // ok, lets start a transaction and drop existing samples
    ride->command->startLUW("Fix Lap Swim");
    ride->command->deletePoints(0, ride->dataPoints().count());

    double last_time = 0.0;
    double last_distance = 0.0;
    double frac_time = 0.0;
    int interval = 1;
    double kph, cad;

    for (int i=0; i< series->datapoints.count(); i++) {

        // update accumulated time and distance for each length
        ride->command->setXDataPointValue("SWIM", i, 0, last_time);
        ride->command->setXDataPointValue("SWIM", i, 1, last_distance);

        // use length data to recreate sample records and lap markers
        XDataPoint *p = series->datapoints.at(i);

        // another pool length or pause
        double length_distance = (p->number[typeIdx] ? pl / 1000.0 : 0.0);

        // Adjust length duration using fractional carry
        double length_duration = p->number[durationIdx] + frac_time;
        frac_time = modf(length_duration, &length_duration);

        // Cadence from Strokes and Duration, if Strokes available
        if (p->number[typeIdx] > 0.0 && length_duration > 0.0) {
            cad = (strokesIdx == -1) ? 0.0 :
                  round(60.0 * p->number[strokesIdx] / length_duration);
        } else { // pause length
            cad = 0.0;
        }

       // only fill 100x the maximal smart recording gap defined
       // in preferences - we don't want to crash / stall on bad
       // or corrupt files
       if (length_duration > 0 && length_duration < 100*GarminHWM.toInt()) {
           QVector<struct RideFilePoint> newRows;
           kph = 3600.0 * length_distance / length_duration;
           if (length_distance == 0.0) interval++; // pauses mark laps
           for (int i = 0; i < length_duration; i++) {
               double hr = hrdata.value(last_time + i, 0.0); // recover HR data
               newRows << RideFilePoint(
                   last_time + i, cad, hr,
                   last_distance + (length_distance * i/length_duration),
                   kph, 0.0, 0.0, 0.0, 0.0, 0.0,
                   0.0, 0.0,
                   RideFile::NA,RideFile::NA,
                   0.0, 0.0,0.0, 0.0,
                   0.0, 0.0,
                   0.0, 0.0,0.0, 0.0,
                   0.0, 0.0,0.0, 0.0,
                   0.0, 0.0,
                   0.0, 0.0, 0.0, 0.0,
                   interval);
            }
            ride->command->appendPoints(newRows);
            last_time += length_duration;
            last_distance += length_distance;
            if (length_distance == 0.0) interval++; // pauses mark laps
       }
       // Alternative way to mark pauses: Rest seconds after each length
       if (restIdx>0 && p->number[restIdx]>0) {
           QVector<struct RideFilePoint> newRows;
           interval++; // pauses mark laps
           for (int i=0; i<p->number[restIdx] && i<100*GarminHWM.toInt(); i++) {
               double hr = hrdata.value(last_time + i, 0.0); // recover HR data
               newRows << RideFilePoint(
                   last_time + i, 0.0, hr,
                   last_distance,
                   0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                   0.0, 0.0,
                   RideFile::NA,RideFile::NA,
                   0.0, 0.0,0.0, 0.0,
                   0.0, 0.0,
                   0.0, 0.0,0.0, 0.0,
                   0.0, 0.0,0.0, 0.0,
                   0.0, 0.0,
                   0.0, 0.0, 0.0, 0.0,
                   interval);
           }
           ride->command->appendPoints(newRows);
           last_time += p->number[restIdx];
           interval++; // pauses mark laps
       }

    }

    // Update Rec. Interval, Pool Length, set data present and commit
    ride->setRecIntSecs(1.0);
    ride->setTag("Pool Length", QString("%1").arg(pl));
    ride->setDataPresent(ride->km, true);
    ride->setDataPresent(ride->kph, true);
    ride->setDataPresent(ride->cad, strokesIdx>0);
    ride->command->endLUW();
    // rebuild intervals and force metric update
    ride->fillInIntervals();
    ride->context->rideItem()->isstale = true;
    ride->context->rideItem()->refresh();

    return true;
}
