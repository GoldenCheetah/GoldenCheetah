/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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
#include "Units.h"
#include "Colors.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>

// Config widget used by the Preferences/Options config panes
class FixGaps;
class FixGapsConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixGapsConfig)

    friend class ::FixGaps;
    protected:
        QDoubleSpinBox *tolerance,
                       *beerandburrito;

    public:
        FixGapsConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixGapsInRecording));

            QFormLayout *layout = newQFormLayout(this);
            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            tolerance = new QDoubleSpinBox();
            tolerance->setMaximum(99.99);
            tolerance->setMinimum(0);
            tolerance->setSingleStep(0.1);
            tolerance->setSuffix(" " + tr("s"));

            beerandburrito = new QDoubleSpinBox();
            beerandburrito->setMaximum(99999.99);
            beerandburrito->setMinimum(0);
            beerandburrito->setSingleStep(0.1);
            beerandburrito->setSuffix(" " + tr("s"));

            layout->addRow(tr("Tolerance"), tolerance);
            layout->addRow(tr("Stop"), beerandburrito);
        }

        //~FixGapsConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        void readConfig() {
            double tol = appsettings->value(NULL, GC_DPFG_TOLERANCE, "1.0").toDouble();
            double stop = appsettings->value(NULL, GC_DPFG_STOP, "90.0").toDouble();
            tolerance->setValue(tol);
            beerandburrito->setValue(stop);
        }

        void saveConfig() {
            appsettings->setValue(GC_DPFG_TOLERANCE, tolerance->value());
            appsettings->setValue(GC_DPFG_STOP, beerandburrito->value());
        }
};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixGaps : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixGaps)

    public:
        FixGaps() {}
        ~FixGaps() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op) override;

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) const override{
            Q_UNUSED(ride);
            return new FixGapsConfig(parent);
        }

        // Localized Name
        QString name() const override {
            return tr("Fix Gaps in Recording");
        }

        QString id() const override {
            return "::FixGaps";
        }

        QString legacyId() const override {
            return "Fix Gaps in Recording";
        }

        QString explain() const override {
            return tr("Many devices, especially wireless devices, will "
                      "drop connections to the bike computer. This leads "
                      "to lost samples in the resulting data, or so-called "
                      "drops in recording.\n\n"
                      "In order to calculate peak powers and averages, it "
                      "is very helpful to remove these gaps, and either "
                      "smooth the data where it is missing or just "
                      "replace with zero value samples\n\n"
                      "This function performs this task, taking two "
                      "parameters;\n\n"
                      "tolerance - this defines the minimum size of a "
                      "recording gap (in seconds) that will be processed. "
                      "any gap shorter than this will not be affected.\n\n"
                      "stop - this defines the maximum size of "
                      "gap (in seconds) that will have a smoothing algorithm "
                      "applied. Where a gap is shorter than this value it will "
                      "be filled with values interpolated from the values "
                      "recorded before and after the gap. If it is longer "
                      "than this value, it will be filled with zero values.");
        }
};

static bool fixGapsAdded = DataProcessorFactory::instance().registerProcessor(new FixGaps());

bool
FixGaps::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)

    // get settings
    double tolerance, stop;
    if (config == NULL) { // being called automatically
        tolerance = appsettings->value(NULL, GC_DPFG_TOLERANCE, "1.0").toDouble();
        stop = appsettings->value(NULL, GC_DPFG_STOP, "90.0").toDouble();
    } else { // being called manually
        tolerance = ((FixGapsConfig*)(config))->tolerance->value();
        stop = ((FixGapsConfig*)(config))->beerandburrito->value();
    }

    // if the number of duration / number of samples
    // equals the recording interval then we don't need
    // to post-process for gaps

    // Additionally, If there are less than 2 dataPoints then there
    // is no way of post processing anyway (e.g. manual workouts)
    if (ride->dataPoints().count() < 2) return false;

    // OK, so there are probably some gaps, lets post process them
    RideFilePoint *last = NULL;
    int dropouts = 0;
    double dropouttime = 0.0;

    // put it all in a LUW
    ride->command->startLUW("Fix Gaps in Recording");

    for (int position = 0; position < ride->dataPoints().count(); position++) {
        RideFilePoint *point = ride->dataPoints()[position];

        if (NULL != last) {
            double gap = point->secs - last->secs - ride->recIntSecs();

            // if we have gps and we moved, then this isn't a stop
            bool stationary = ((last->lat || last->lon) && (point->lat || point->lon)) // gps is present
                         && last->lat == point->lat && last->lon == point->lon;

            // moved for less than stop seconds ... interpolate
            if (!stationary && gap >= tolerance && gap <= stop) {

                // what's needed?
                dropouts++;
                dropouttime += gap;

                int count = gap/ride->recIntSecs();
                double hrdelta = (point->hr - last->hr) / (double) count;
                double pwrdelta = (point->watts - last->watts) / (double) count;
                double kphdelta = (point->kph - last->kph) / (double) count;
                double kmdelta = (point->km - last->km) / (double) count;
                double caddelta = (point->cad - last->cad) / (double) count;
                double altdelta = (point->alt - last->alt) / (double) count;
                double nmdelta = (point->nm - last->nm) / (double) count;
                double londelta = (point->lon - last->lon) / (double) count;
                double latdelta = (point->lat - last->lat) / (double) count;
                double hwdelta = (point->headwind - last->headwind) / (double) count;
                double slopedelta = (point->slope - last->slope) / (double) count;
                double temperaturedelta = (point->temp - last->temp) / (double) count;
                double lrbalancedelta = (point->lrbalance>=0?point->lrbalance:50.0 - last->lrbalance) / (double) count;
                double ltedelta = (point->lte - last->lte) / (double) count;
                double rtedelta = (point->rte - last->rte) / (double) count;
                double lpsdelta = (point->lps - last->lps) / (double) count;
                double rpsdelta = (point->rps - last->rps) / (double) count;
                double lpcodelta = (point->lpco - last->lpco) / (double) count;
                double rpcodelta = (point->rpco - last->rpco) / (double) count;
                double lppbdelta = (point->lppb - last->lppb) / (double) count;
                double rppbdelta = (point->rppb - last->rppb) / (double) count;
                double lppedelta = (point->lppe - last->lppe) / (double) count;
                double rppedelta = (point->rppe - last->rppe) / (double) count;
                double lpppbdelta = (point->lpppb - last->lpppb) / (double) count;
                double rpppbdelta = (point->rpppb - last->rpppb) / (double) count;
                double lpppedelta = (point->lpppe - last->lpppe) / (double) count;
                double rpppedelta = (point->rpppe - last->rpppe) / (double) count;
                double smo2delta = (point->smo2 - last->smo2) / (double) count;
                double thbdelta = (point->thb - last->thb) / (double) count;
                double rcontactdelta = (point->rcontact - last->rcontact) / (double) count;
                double rcaddelta = (point->rcad - last->rcad) / (double) count;
                double rvertdelta = (point->rvert - last->rvert) / (double) count;
                double tcoredelta = (point->tcore - last->tcore) / (double) count;


                // add the points
                for(int i=0; i<count; i++) {
                    RideFilePoint *add = new RideFilePoint(last->secs+((i+1)*ride->recIntSecs()),
                                                           last->cad+((i+1)*caddelta),
                                                           last->hr + ((i+1)*hrdelta),
                                                           last->km + ((i+1)*kmdelta),
                                                           last->kph + ((i+1)*kphdelta),
                                                           last->nm + ((i+1)*nmdelta),
                                                           last->watts + ((i+1)*pwrdelta),
                                                           last->alt + ((i+1)*altdelta),
                                                           last->lon + ((i+1)*londelta),
                                                           last->lat + ((i+1)*latdelta),
                                                           last->headwind + ((i+1)*hwdelta),
                                                           last->slope + ((i+1)*slopedelta),
                                                           last->temp + ((i+1)*temperaturedelta),
                                                           last->lrbalance>=0 ? last->lrbalance + ((i+1)*lrbalancedelta) : last->lrbalance,
                                                           last->lte + ((i+1)*ltedelta),
                                                           last->rte + ((i+1)*rtedelta),
                                                           last->lps + ((i+1)*lpsdelta),
                                                           last->rps + ((i+1)*rpsdelta),
                                                           last->lpco + ((i+1)*lpcodelta),
                                                           last->rpco + ((i+1)*rpcodelta),
                                                           last->lppb + ((i+1)*lppbdelta),
                                                           last->rppb + ((i+1)*rppbdelta),
                                                           last->lppe + ((i+1)*lppedelta),
                                                           last->rppe + ((i+1)*rppedelta),
                                                           last->lpppb + ((i+1)*lpppbdelta),
                                                           last->rpppb + ((i+1)*rpppbdelta),
                                                           last->lpppe + ((i+1)*lpppedelta),
                                                           last->rpppe + ((i+1)*rpppedelta),
                                                           last->smo2 + ((i+1)*smo2delta),
                                                           last->thb + ((i+1)*thbdelta),
                                                           last->rvert + ((i+1)*rvertdelta),
                                                           last->rcad + ((i+1)*rcaddelta),
                                                           last->rcontact + ((i+1)*rcontactdelta),
                                                           last->tcore + ((i+1)*tcoredelta),
                                                           last->interval);

                    ride->command->insertPoint(position++, add);
                }

            // stationary or greater than stop seconds... fill with zeroes
            } else if (stationary || gap > stop) {

                dropouts++;
                dropouttime += gap;

                int count = gap/ride->recIntSecs();
                double kmdelta = (point->km - last->km) / (double) count;

                // add zero value points
                for(int i=0; i<count; i++) {
                    RideFilePoint *add = new RideFilePoint(last->secs+((i+1)*ride->recIntSecs()),
                                                           0,
                                                           0,
                                                           last->km + ((i+1)*kmdelta),
                                                           0,
                                                           0,
                                                           0,
                                                           last->alt,
                                                           0,
                                                           0,
                                                           0,
                                                           0,
                                                           0,
                                                           0,
                                                           0.0, 0.0, 0.0, 0.0, //pedal torque / smoothness
                                                           0.0, 0.0, // pedal platform offset
                                                           0.0, 0.0, 0.0, 0.0, //pedal power phase
                                                           0.0, 0.0, 0.0, 0.0, //pedal peak power phase
                                                           0.0, 0.0, // smO2 / thb
                                                           0.0, 0.0, 0.0, // running dynamics
                                                           0.0,
                                                           last->interval);
                    ride->command->insertPoint(position++, add);
                }
            }
        }
        last = point;
    }

    // end the Logical unit of work here
    ride->command->endLUW();

    ride->setTag("Dropouts", QString("%1").arg(dropouts));
    ride->setTag("Dropout Time", QString("%1").arg(dropouttime));

    if (dropouts) return true;
    else return false;
}
