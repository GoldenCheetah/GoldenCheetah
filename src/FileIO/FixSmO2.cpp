/*
 * Copyright (c) 2015 Damien Grauser
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
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>

// Config widget used by the Preferences/Options config panes
class FixSmO2;
class FixSmO2Config : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixSmO2Config)

    friend class ::FixSmO2;
    protected:
        QHBoxLayout *layout;

    public:
        FixSmO2Config(QWidget *parent) : DataProcessorConfig(parent) {




        }

        //~FixSmO2Config() {} // deliberately not declared since Qt will delete
                              // the widget and its children when the config pane is deleted

        QString explain() {
            return(QString(tr("Occasionally SmO2 (%) will erroneously "
                           "report high values (0% or >100%).\n\n"
                           "This function will look for spikes/anomalies "
                           "in SmO2 data and replace the erroneous data "
                           "by smoothing/interpolating the data from either "
                           "side of the 3 points in question")));
        }

        void readConfig() { }
        void saveConfig() { }

};


class FixSmO2 : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixSpikes)

    public:
        FixSmO2() {}
        ~FixSmO2() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent) {
            return new FixSmO2Config(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Fix SmO2 Anomaly"));
        }
};

static bool fixSmO2Added = DataProcessorFactory::instance().registerProcessor(QString("Fix SmO2 Anomaly"), new FixSmO2());

bool
FixSmO2::postProcess(RideFile *ride, DataProcessorConfig *config=0)
{
    Q_UNUSED(config);
    // does this ride have power?
    if (ride->areDataPresent()->watts == false) return false;

    // get settings
    double max = 100;

    int windowsize = 30 / ride->recIntSecs();

    // We use a window size of 30s to find spikes
    // if the ride is shorter, don't bother
    // is no way of post processing anyway (e.g. manual workouts)
    if (windowsize > ride->dataPoints().count()) return false;

    // Find the power outliers

    int spikes = 0;

    // create a data array for the outlier algorithm
    QVector<double> smo2;
    QVector<double> secs;

    foreach (RideFilePoint *point, ride->dataPoints()) {
        smo2.append(point->smo2);
        secs.append(point->secs);
    }


    // TODO If we kept only min max value we don't need to use LTMOutliers
    LTMOutliers *outliers = new LTMOutliers(secs.data(), smo2.data(), smo2.count(), windowsize, false);
    ride->command->startLUW("Fix SmO2 in Recording");

    for (int i=0; i<secs.count(); i++) {

        // ok, so its highly variant but is it over
        // the max value we are willing to accept?
        if (outliers->getYForRank(i) < max && outliers->getYForRank(i)>0) continue;


        //qDebug() << "spike at " << outliers->getXForRank(i) << outliers->getYForRank(i);

        // Houston, we have a spike

        // which one is it
        int pos = outliers->getIndexForRank(i);
        double left=0.0, right=0.0;

        if (pos > 2)  {
            int nb = 0;
            for (int j=1; j<4; j++) {
                if (ride->dataPoints()[pos-j]->smo2>0 && ride->dataPoints()[pos-j]->smo2<100) {
                    left += ride->dataPoints()[pos-j]->smo2;
                    nb++;
                }
            }
            if (nb > 0)
                left = left / nb;
        }
        if (pos < (ride->dataPoints().count()))  {
            int nb = 0;
            for (int j = 1; j < 4 && pos + j < ride->dataPoints().count();
                 j++) {
                if (ride->dataPoints()[pos + j]->smo2 > 0 &&
                    ride->dataPoints()[pos + j]->smo2 < 100) {
                    right = ride->dataPoints()[pos + j]->smo2;
                    nb++;
                }
            }
            if (nb > 0)
                right = right / nb;
        }

        if (left != 0 && right != 0 && (left+right)/2.0 != outliers->getYForRank(i)) {
            spikes++;

            ride->command->setPointValue(pos, RideFile::smo2, (left+right)/2.0);
            //qDebug() << "replace by "<< (left+right)/2.0;
        }
    }
    ride->command->endLUW();

    delete outliers;

    if (spikes) return true;
    else return false;
}
