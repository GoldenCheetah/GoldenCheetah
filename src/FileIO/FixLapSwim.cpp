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
#include "Colors.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>
#include <QHash>

#include <cmath>

// Config widget used by the Preferences/Options config panes
class FixLapSwim;
class FixLapSwimConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixLapSwimConfig)

    friend class ::FixLapSwim;
    protected:
        QSpinBox *pl;
        QSpinBox *minRest;

    public:
        FixLapSwimConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixLapSwim));

            QFormLayout *layout = newQFormLayout(this);
            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            pl = new QSpinBox();
            pl->setMaximum(50);
            pl->setMinimum(0);
            pl->setSingleStep(1);
            pl->setSuffix(" " + tr("m"));

            minRest = new QSpinBox();
            minRest->setMaximum(10);
            minRest->setMinimum(1);
            minRest->setSingleStep(1);
            minRest->setSuffix(" " + tr("s"));

            layout->addRow(tr("Pool Length"), pl);
            layout->addRow(tr("Min Rest"), minRest);
        }

        //~FixLapSwimConfig() {}  // deliberately not declared since Qt will
                                // delete the widget and its children when
                                // the config pane is deleted

        void readConfig() {
            pl->setValue(appsettings->value(NULL, GC_DPFLS_PL, "0").toDouble());
            minRest->setValue(appsettings->value(NULL, GC_DPFLS_MR, "3").toDouble());
        }

        void saveConfig() {
            appsettings->setValue(GC_DPFLS_PL, pl->value());
            appsettings->setValue(GC_DPFLS_MR, minRest->value());
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
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op) override;

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) const override {
            Q_UNUSED(ride);
            return new FixLapSwimConfig(parent);
        }

        // Localized Name
        QString name() const override {
            return tr("Fix Lap Swim from Length Data");
        }

        QString id() const override {
            return "::FixLapSwim";
        }

        QString legacyId() const override {
            return "Fix Lap Swim";
        }

        QString explain() const override {
            return tr("Sometimes length times or distances from "
                      "lap swimming files are wrong.\n"
                      "You can fix length-by-length information "
                      "in Editor SWIM tab: TYPE, DURATION and STROKES.\n"
                      "This tool recompute accumulated time, distance "
                      "and sample Speed/Cadence from the updated lengths.\n"
                      "Laps are recreated using length distance changes as markers\n\n"
                      "Pool Length (in meters) allows to re-define the "
                      "field if value is > 0\n\n"
                      "Lengths shorter than Min Rest (in seconds) "
                      "are removed and their duration carried to "
                      "the next length.\n\n");
        }
};

static bool FixLapSwimAdded = DataProcessorFactory::instance().registerProcessor(new FixLapSwim());

bool
FixLapSwim::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    // get settings
    double pl;
    double minRest;
    if (config == NULL) { // being called automatically
        // when the file is created use poll length in the file
        pl = (op == "NEW") ? 0.0 : appsettings->value(NULL, GC_DPFLS_PL, "0").toDouble();
        minRest = appsettings->value(NULL, GC_DPFLS_MR, "3").toDouble();
    } else { // being called manually
        pl = ((FixLapSwimConfig*)(config))->pl->value();
        minRest = ((FixLapSwimConfig*)(config))->minRest->value();
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

    // Preserve existing data (HR, Temp, etc.)
    QHash<double, RideFilePoint> ptHash;
    foreach (const RideFilePoint* pt, ride->dataPoints()) {
        ptHash.insert(pt->secs, *pt);
    }
    // delete current lap markers
    ride->clearIntervals();
    // ok, lets start a transaction and drop existing samples
    ride->command->startLUW("Fix Lap Swim");
    ride->command->deletePoints(0, ride->dataPoints().count());

    // Remove lengths shorter than minRest secs and carry that time over the next length
    double prev_length_duration = 0.0;
    for (int i=0; i<series->datapoints.count(); i++) {

        double length_duration = series->datapoints.at(i)->number[durationIdx];
        if (length_duration < minRest) {
            // remove shorter than minRest length, update row index and carry duration
            ride->command->deleteXDataPoints("SWIM", i, 1);
            i--;
            prev_length_duration += length_duration;
        } else if (prev_length_duration > 0.0) {
            // increaase duration from removed lengths
            ride->command->setXDataPointValue("SWIM", i, durationIdx+2, prev_length_duration+length_duration);
            prev_length_duration = 0.0;
        }
    }

    double last_time = 0.0;
    double last_distance = 0.0;
    double frac_time = 0.0;
    int interval = 0;
    double kph, cad;
    double prev_length_distance = 0;

    for (int i=0; i< series->datapoints.count(); i++) {

        // update accumulated time and distance for each length
        ride->command->setXDataPointValue("SWIM", i, 0, last_time);
        ride->command->setXDataPointValue("SWIM", i, 1, last_distance);

        // use length data to recreate sample records and lap markers
        XDataPoint *p = series->datapoints.at(i);

        // another pool length or pause
        double length_distance = (p->number[typeIdx] ? pl / 1000.0 : 0.0);

        // Adjust truncated length duration using fractional carry
        double length_duration = p->number[durationIdx] + frac_time;
        frac_time = modf(length_duration, &length_duration);

        // Cadence from Strokes and Duration, if Strokes available
        if (p->number[typeIdx] > 0.0 && p->number[durationIdx] > 0.0) {
            cad = (strokesIdx == -1) ? 0.0 :
                  60.0 * p->number[strokesIdx] / p->number[durationIdx];
        } else { // pause length
            cad = 0.0;
        }

       // only fill 100x the maximal smart recording gap defined
       // in preferences - we don't want to crash / stall on bad
       // or corrupt files
       if (length_duration > 0 && length_duration < 100*GarminHWM.toInt()) {
           QVector<struct RideFilePoint> newRows;
           kph = 3600.0 * length_distance / p->number[durationIdx];
           double deltaDist = length_duration > 1 ? length_distance / (length_duration - 1) : 0.0;
           if (length_distance != prev_length_distance) interval++; // length distance changes mark laps
           for (int i = 0; i < length_duration; i++) {
               // recover previous data or create a new sample point,
               // and fix time/speed/distance/cadence/interval
               RideFilePoint pt = ptHash.value(last_time + i);
               pt.secs = last_time + i;
               pt.cad = cad;
               pt.km = last_distance + i * deltaDist;
               pt.kph = pt.secs > 0 ? kph : 0.0;
               pt.interval = interval;
               newRows << pt;
            }
            ride->command->appendPoints(newRows);
            last_time += length_duration;
            last_distance += length_distance;
            prev_length_distance = length_distance;
       }
       // Alternative way to mark pauses: Rest seconds after each length
       if (restIdx>0 && p->number[restIdx]>0) {
           QVector<struct RideFilePoint> newRows;
           interval++; // pauses mark laps
           for (int i=0; i<p->number[restIdx] && i<100*GarminHWM.toInt(); i++) {
               // recover previous data or create a new sample point,
               // and fix time/speed/distance/cadence/interval
               RideFilePoint pt = ptHash.value(last_time + i);
               pt.secs = last_time + i;
               pt.cad = 0.0;
               pt.km = last_distance;
               pt.kph = 0.0;
               pt.interval = interval;
               newRows << pt;
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
    if (op != "NEW") {
        // rebuild intervals and force metric update
        ride->fillInIntervals();
        ride->context->rideItem()->isstale = true;
        ride->context->rideItem()->refresh();
    }

    return true;
}
