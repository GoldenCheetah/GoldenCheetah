/*
 * Copyright (c) 2015 Vianney Boyer (vlcvboyer@gmail.com)
 * Copyright (c) 2016 Damien Grauser
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
#include "Settings.h"
#include "Units.h"
#include "LocationInterpolation.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>
#include <QCommandLinkButton>
#include <QFileDialog>

#define PI M_PI

// Config widget used by the Preferences/Options config panes
class FixDeriveHeadwind;
class FixDeriveHeadwindConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixDeriveHeadwindConfig)

    friend class ::FixDeriveHeadwind;

    protected:

    public:
        FixDeriveHeadwindConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_EstimateHeadwindValues));
        }

        //~FixDeriveHeadwindConfig() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        void readConfig() { }
        void saveConfig() { }
};

class FixDeriveHeadwind : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixDeriveHeadwind)

    public:
        FixDeriveHeadwind() {}
        ~FixDeriveHeadwind() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op) override;

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) const override {
            Q_UNUSED(ride);
            return new FixDeriveHeadwindConfig(parent);
        }

        // Localized Name
        QString name() const override {
            return tr("Estimate Headwind Values");
        }

        QString id() const override {
            return "::FixDeriveHeadwind";
        }

        QString legacyId() const override {
            return "Estimate Headwind Values";
        }

        QString explain() const override{
            return tr("Use weather broadcasted data in FIT file to derive Headwind.");
        }
};

static bool FixDeriveHeadwindAdded = DataProcessorFactory::instance().registerProcessor(new FixDeriveHeadwind());

bool
FixDeriveHeadwind::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(config)
    Q_UNUSED(op)

    double bearing = 0; // used to compute headwind depending on wind/cyclist bearing difference

    XDataSeries *series = ride->xdata("WEATHER");
    if (!series) // no WEATHER
        return false;

    int winspeedIdx = -1, windheadingIdx = -1;
    for (int a=0; a<series->valuename.count(); a++) {
        if (series->valuename.at(a) == "WINDSPEED")
            winspeedIdx = a;
        if (series->valuename.at(a) == "WINDHEADING")
            windheadingIdx = a;
    }

    if (winspeedIdx == -1) // no WINDSPEED
        return false;

    if (windheadingIdx == -1) // no WINDHEADING
        return false;

    if (!ride->areDataPresent()->lat || !ride->areDataPresent()->lon) // no GPS data
        return false;

    if (ride->dataPoints().count() == 0) // no point
        return false;

    // apply the change
    ride->command->startLUW("Estimate Headwind");

    int b=0;
    double windspeed = 0.0, windheading = 0.0;
    ride->dataPoints().at(0)->headwind = ride->dataPoints().at(0)->kph;

    for (int i=1; i<ride->dataPoints().count(); i++) {
        RideFilePoint *point = ride->dataPoints()[i];
        RideFilePoint *prevPoint = ride->dataPoints()[i-1];

        for (int j=b; j<series->datapoints.count(); j++) {
           if (series->datapoints.at(j)->secs>point->secs)
               break;
           b=j;
           // Wind speed (mm/s)
           windspeed = series->datapoints.at(j)->number[winspeedIdx];
           // Wind heading (0deg=North)
           windheading = series->datapoints.at(j)->number[windheadingIdx];
        }

        // ensure a movement occurred and valid lat/lon in order to compute cyclist direction
        if (prevPoint->lat != point->lat || prevPoint->lon != point->lon)
        {
            geolocation prevLoc(prevPoint->lat, prevPoint->lon, prevPoint->alt);
            geolocation loc(point->lat, point->lon, point->alt);

            if (prevLoc.IsReasonableGeoLocation() && loc.IsReasonableGeoLocation())
            {
                bearing = prevLoc.BearingTo(loc);
            }
        } //else keep previous bearing or 0 at beginning

        double headwind = cos(bearing - (windheading/ 180.0 * PI)) * (windspeed) + point->kph;
        //qDebug() << point->kph << headwind  << "(" << windspeed << windheading << ")";

        point->headwind = headwind;
    }

    ride->setDataPresent(ride->headwind, true);

    // update the change history
    QString log = ride->getTag("Change History", "");
    log +=  tr("Derive Headwind from weather on ");
    log +=  QDateTime::currentDateTime().toString();
    if (ride->command->changeLog().length()>0)
        log +=  ":\n" + ride->command->changeLog();
    ride->setTag("Change History", log);

    // process LOW
    ride->command->endLUW();

    return true;
}
