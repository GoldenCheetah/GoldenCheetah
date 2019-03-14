/*
 * Copyright (c) 2015 Damien Grauser (Damien.Grauser@gmail.com)
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
#include "LTMOutliers.h"
#include "Units.h"
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>
#include "LocationInterpolation.h"

// Config widget used by the Preferences/Options config panes
class FixDeriveDistance;
class FixDeriveDistanceConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixDeriveDistanceConfig)

    friend class ::FixDeriveDistance;
    protected:
        QHBoxLayout *layout;

        QCheckBox *useCubicSplines;

        QLabel *bikeWeightLabel;
        QDoubleSpinBox *bikeWeight;
        QLabel *crrLabel;
        QDoubleSpinBox *crr;

    public:
        FixDeriveDistanceConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_EstimateDistanceValues));

            layout = new QHBoxLayout(this);

            useCubicSplines = new QCheckBox(tr("Use Cubic Splines"), this);
            useCubicSplines->setCheckState(Qt::Checked);
            layout->addWidget(useCubicSplines);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            layout->addStretch();
        }

        //~FixDeriveDistanceConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        void readConfig() {
        }

        void saveConfig() {
        }

        QString explain() {
            return(QString(tr("Derive distance based on "
                              "ridefile's GPS locations\n\n"
                              "This process will populate distance information (and override existing distance information if present.)"
                              "The cubic splines processing estimates distance across polynomial curve, "
                              "otherwise this feature will compute geometric arc distance between ride points."
                              "\n\n")));
        }

};


// RideFile Dataprocessor -- Derive estimated distance based on GPS position
//
class FixDeriveDistance : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixDeriveDistance)

    public:
        FixDeriveDistance() {}
        ~FixDeriveDistance() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixDeriveDistanceConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Estimate Distance Values"));
        }
};

static bool FixDeriveDistanceAdded = DataProcessorFactory::instance().registerProcessor(QString("Estimate Distance Values"), new FixDeriveDistance());

double _deg2rad(double deg) {
  return (deg * M_PI / 180);
}

bool
FixDeriveDistance::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(config)
    Q_UNUSED(op)

    bool fUseCubicSplines = ((FixDeriveDistanceConfig*)(config))->useCubicSplines->isChecked();
    bool fUseSpeedAndTime = false;

    GeoPointInterpolator gpi;
    int ii = 0; // interpolator index
    double cubicDistanceKM = 0.0;

    double distanceFromSpeedTime = 0.0;
    double lastSecs = 0.0;
    bool fHasSpeedTime = (ride->areDataPresent()->kph && ride->areDataPresent()->secs);

    if (fUseSpeedAndTime && !fHasSpeedTime)
        return false;

    // if its already there do nothing !
    //if (ride->areDataPresent()->km) return false;

    // no dice if we don't have any gps location information.
    if (!fUseSpeedAndTime && !ride->areDataPresent()->lat)
        return false;

    bool fHasAlt = ride->areDataPresent()->alt;

    // apply the change
    ride->command->startLUW("Estimate Distance");

    {
        double km = 0.0;
        double lastLat = 0;
        double lastLon = 0;

        for (int i=0; i<ride->dataPoints().count(); i++) {

            RideFilePoint *p = ride->dataPoints()[i];

            // Compute distance using cubic splines.
            while (gpi.WantsInput(i)) {

                // If bracket is present, always sum its length before any push.
                // This maintains cubicDistanceKM as distance to current bracket
                // start.
                double d0, d1;
                if (gpi.GetBracket(d0, d1)) {
                    cubicDistanceKM += (gpi.SplineLength(d0, d1) / 1000.0);
                }

                if (ii >= ride->dataPoints().count()) {
                    // Past end of ride points, continue pushing final point.
                    RideFilePoint *pii = ride->dataPoints()[ii-1];
                    geolocation geo(pii->lat, pii->lon, fHasAlt ? pii->alt : 0.0);
                    if (geo.IsReasonableGeoLocation()) {
                        gpi.Push(ii, geo);
                        //gpi.NotifyInputComplete();
                    }
                } else {
                    // Use index for distance, since we just use it as an enumeration
                    RideFilePoint *pii = ride->dataPoints()[ii];
                    geolocation geo(pii->lat, pii->lon, fHasAlt ? pii->alt : 0.0);
                    if (geo.IsReasonableGeoLocation()) {
                        gpi.Push(ii, geo);
                        ii++;
                    }
                }
            }

            // Compute distance using speed and time (has high variance.)
            // Currently computed because it is interesting to see, but no
            // option enabled to apply it.
            double distDelta = 0.0;
            if (fHasSpeedTime)
            {
                double secs = p->secs;
                double kph = p->kph;

                double secDelta = secs - lastSecs;
                distDelta = secDelta * kph / 3600;

                distanceFromSpeedTime += distDelta;

                lastSecs = secs;
            }

            // Compute distance using geometric arc length between points.
            double _theta, _dist;
            _theta = lastLon - p->lon;
            if (lastLat == 0.0 || lastLon == 0.0 || p->lat == 0.0 || p->lon == 0.0 || (_theta == 0.0 && (lastLat - p->lat) == 0.0)) {
                 _dist = 0;
            }
            else {
                _dist = sin(_deg2rad(lastLat)) * sin(_deg2rad(p->lat)) + cos(_deg2rad(lastLat)) * cos(_deg2rad(p->lat)) * cos(_deg2rad(_theta));
                _dist = acos(_dist) * 6371;
            }
            if (std::isnan(_dist) == false && _dist<0.05)
                km += _dist;
            lastLat = p->lat;
            lastLon = p->lon;

            // Write distance into ride file.
            double distanceKM = km;

            // Select distance estimate to use
            if (fUseSpeedAndTime) {
                distanceKM = distanceFromSpeedTime;
            } else if (fUseCubicSplines) {
                distanceKM = cubicDistanceKM;
            } else {
                distanceKM = km;
            }

            // Apply selected distance estimate to ride file
            ride->command->setPointValue(i, RideFile::km, distanceKM);
        }

        ride->setDataPresent(ride->km, true);

        // update the change history
        QString log = ride->getTag("Change History", "");
        log +=  tr("Derive Distance from GPS on ");
        log +=  QDateTime::currentDateTime().toString();
        if (ride->command->changeLog().count()>0)
            log +=  ":\n" + ride->command->changeLog();
        ride->setTag("Change History", log);
    }

    ride->command->endLUW();

    return true;
}

