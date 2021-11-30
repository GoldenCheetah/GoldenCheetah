/*
 * Copyright (c) 2014 Jon Beverley (jon@csdl.biz)
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
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>

// MapQuest default API key.
// If you have reliability problems with Fix Elevation, caused by too
// many API requests per day using this key, then apply for your own
// Free API key at https://developer.mapquest.com/.
// You can then add it to gcconfig.pri

#ifndef GC_MAPQUESTAPI_KEY 
#define GC_MAPQUESTAPI_KEY "Fmjtd%7Cluur20uznu%2Ca2%3Do5-9ayshw"
#endif

struct elevationGPSPoint {
    int rideFileIndex;
    double lon;
    double lat;
};

// Config widget used by the Preferences/Options config panes
class FixElevation;
class FixElevationConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixElevationConfig)

    friend class ::FixElevation;

    protected:
        QHBoxLayout *layout;
        QLabel *akLabel;
        QLineEdit *ak;

    public:
        // there is no config
        FixElevationConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixElevationErrors));

            layout = new QHBoxLayout(this);

            layout->setContentsMargins(0,0,0,0);
            setContentsMargins(0,0,0,0);

            akLabel = new QLabel(tr("MapQuest API Key"));

            ak = new QLineEdit();
            layout->addWidget(akLabel);
            layout->addWidget(ak);
            layout->addStretch();
        }

        QString explain() {
            return(QString(tr("Fix or add elevation data. If elevation data is "
                           "present it will be removed and overwritten."
                           "\n\nMapQuest API Key is optional, you can get a free one from "
                           "https::/developer/mapquest.com/ to have your own transaction limits."
                           "\n\nINTERNET CONNECTION REQUIRED.")));
        }

        void readConfig() {
            ak->setText(appsettings->value(NULL, GC_DPFE_AK, "").toString());
        }

        void saveConfig() {
            appsettings->setValue(GC_DPFE_AK, ak->text());
        }

};


// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixElevation : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixElevation)

    public:
        FixElevation() {}
        ~FixElevation() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixElevationConfig(parent);
        }

        // Localized Name
        QString name() {
            return (tr("Fix Elevation errors"));
        }

    private:
        QString apiKey;

        QStringList FetchElevationDataFromMapQuest(QString latLngCollection);

};

static bool fixElevationAdded = DataProcessorFactory::instance().registerProcessor(QString("Fix Elevation errors"), new FixElevation());

bool
FixElevation::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)

    // get settings
    if (config == NULL) { // being called automatically
        apiKey = appsettings->value(NULL, GC_DPFE_AK, "").toString();
    } else { // being called manually
        apiKey = ((FixElevationConfig*)(config))->ak->text();
    }

    // Cannot process without without GPS data
    if (!ride || ride->areDataPresent()->lat == false || ride->areDataPresent()->lon == false)
        return false;

    int errors=0;

    std::vector<elevationGPSPoint> elvPoints;

    ride->command->startLUW("Fix Elevation Data");

    int lastDistance = 0;
    for (int i=0; i<ride->dataPoints().count(); i++) {
        // is the gps point any good?
        if (ride->dataPoints()[i]->lat && ride->dataPoints()[i]->lat >= double(-56) && ride->dataPoints()[i]->lat <= double(60) &&
            ride->dataPoints()[i]->lon && ride->dataPoints()[i]->lon >= double(-180) && ride->dataPoints()[i]->lon <= double(180)) {
            if (lastDistance < (int) (ride->dataPoints()[i]->km * 1000)) {
                elevationGPSPoint elvPoint;

                elvPoint.lat = (double) ((int) (ride->dataPoints()[i]->lat*100000))/ (double) 100000;
                elvPoint.lon = (double) ((int) (ride->dataPoints()[i]->lon*100000))/ (double) 100000;
                elvPoint.rideFileIndex = i;

                elvPoints.push_back(elvPoint);

                //grab a gps point every 20 meters
                lastDistance = (int) (ride->dataPoints()[i]->km * 1000) + 20;
            }
            ride->command->setPointValue(i, RideFile::alt, 0);
        }
    }

    //loop through points and build a string to sent to MapQuest
    QStringList elevationPoints;
    QString latLngCollection = "";
    int pointCount = 0;
    try {
        for (std::vector<elevationGPSPoint>::iterator point = elvPoints.begin();
             point != elvPoints.end(); ++point) {
            if (latLngCollection.length() != 0) {
                latLngCollection.append(',');
            }
            // these values need extended precision or place marker jumps around.
            latLngCollection.append(QString::number(point->lat,'g',10));
            latLngCollection.append(',');
            latLngCollection.append(QString::number(point->lon,'g',10));
            if (pointCount == 400) {
                elevationPoints = elevationPoints + FetchElevationDataFromMapQuest(latLngCollection);
                latLngCollection = "";
                pointCount = 0;
            } else {
                ++pointCount;
            }
        }
        if (pointCount > 0) {
            elevationPoints = elevationPoints + FetchElevationDataFromMapQuest(latLngCollection);
        }
    } catch (QString err) {
        qDebug() << "Cannot fetch elevation data: " << err;
        QMessageBox oops(QMessageBox::Critical, tr("Fix Elevation Data not possible"),
                         tr("The following problem occured: %1").arg(err));
        oops.exec();
        // close LUW
        ride->command->endLUW();
        return false;
    }


    if (elevationPoints.length() > 0) {
        QVector<double> smoothArray(elevationPoints.length());
        double lastGoodElevation = 0;
        for (int i=0; i<elevationPoints.length(); i++) {
            double elev = QString(elevationPoints.at(i).mid(elevationPoints.at(i).indexOf("|")+1)).toDouble();
            if (elev>-1000) {
                lastGoodElevation = elev;
                smoothArray[i] = elev;
            } else {
                smoothArray[i] = lastGoodElevation;
            }
        }

        // initialise rolling average
        double rtot = 0;
        for (int i=10; i>0 && elevationPoints.length()-i >=0; i--) {
            rtot += smoothArray[elevationPoints.length()-i];
        }

        // now run backwards setting the rolling average
        for (int i=elevationPoints.length()-1; i>=10; i--) {
            double here = smoothArray[i];
            smoothArray[i] = rtot / 10;
            rtot -= here;
            rtot += smoothArray[i-10];
        }
        int loopCount = 0;

        for( std::vector<elevationGPSPoint>::iterator point = elvPoints.begin() ; point != elvPoints.end() ; ++point ) {
            double elev = smoothArray.size() > loopCount ? smoothArray[loopCount] : -100;
            // ignore any seriously negative points
            if (elev>-100) ride->command->setPointValue(point->rideFileIndex, RideFile::alt, elev);
            ++loopCount;
        }

        // set data present if not currently so
        if (ride->areDataPresent()->alt == false) ride->command->setDataPresent(RideFile::alt, true);


        int lastgood = -1;  // where did we last have decent GPS data?
        for (int i=0; i<ride->dataPoints().count(); i++) {
            // is this one decent?
            if (ride->dataPoints()[i]->alt != double(0)) {

                if (lastgood != -1 && (lastgood+1) != i) {
                    // interpolate from last good to here
                    // then set last good to here
                    double deltaAlt = (ride->dataPoints()[i]->alt - ride->dataPoints()[lastgood]->alt) / double(i-lastgood);
                    for (int j=lastgood+1; j<i; j++) {
                        ride->command->setPointValue(j, RideFile::alt, ride->dataPoints()[lastgood]->alt + (double(j-lastgood)*deltaAlt));
                        errors++;
                    }
                } else if (lastgood == -1) {
                    // fill to front
                    for (int j=0; j<i; j++) {
                        ride->command->setPointValue(j, RideFile::alt, ride->dataPoints()[i]->alt);
                        errors++;
                    }
                }
                lastgood = i;
            }
        }

        // fill to end...
        if (lastgood != -1 && lastgood != (ride->dataPoints().count()-1)) {
           // fill from lastgood to end with lastgood
            for (int j=lastgood+1; j<ride->dataPoints().count(); j++) {
                ride->command->setPointValue(j, RideFile::alt, ride->dataPoints()[lastgood]->alt);
                errors++;
            }
        }

        // Invalidate slope data to be recomputed based on new altitude data
        if (ride->areDataPresent()->slope == true)
            ride->command->setDataPresent(RideFile::slope, false);
    }

    // close LUW
    ride->command->endLUW();

    if (errors) {
        ride->setTag("GPS errors", QString("%1").arg(errors));
        return true;
    } else
        return false;
}

QStringList
FixElevation::FetchElevationDataFromMapQuest(QString latLngCollection)
{
    // http://open.mapquestapi.com/elevation/v1/profile?key=Fmjtd%7Cluur20uznu%2Ca2%3Do5-9ayshw&shapeFormat=raw&latLngCollection=52.677,0.94589,52.6769,0.94565,52.6767,0.94545,52.6765,0.94529,52.6764,0.94511,52.6762,0.94488,52.6761,0.94466,52.6759,0.94453,52.6758,0.9447,52.6756,0.94497,52.6756,0.94527,52.6758,0.94553,52.6759,0.94572,52.6761,0.94594,52.6763,0.94611,52.6765,0.94627,52.6766,0.94635,52.6768,0.94639,52.6771,0.9464,52.6772,0.94639,52.6775,0.94637,52.6777,0.94638,52.6779,0.9464,52.6781,0.94645,52.6783,0.94652,52.6784,0.9466,52.6786,0.94672,52.6788,0.94686,52.679,0.94701,52.6792,0.94713,52.6794,0.94721,52.6796,0.94724
    QStringList elevationPoints;
    QNetworkAccessManager *networkMgr = new QNetworkAccessManager();
    QNetworkReply *reply = networkMgr->get( QNetworkRequest(
            QUrl( QString("http://open.mapquestapi.com/elevation/v1/profile?key=%1"
                          "&shapeFormat=raw&useFilter=true&latLngCollection=" + latLngCollection )
                              .arg(apiKey.isEmpty() ? GC_MAPQUESTAPI_KEY : apiKey) ) ) );

    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

    // Execute the event loop here, now we will wait here until readyRead() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();

    QString elevationJSON;
    elevationJSON = reply->readAll();

    QNetworkReply::NetworkError error = reply->error();

    delete networkMgr;

    if (elevationJSON.contains("Exceeded developer limit configuration"))
        throw QString(QObject::tr("Developer limit exceeded"));
    if (elevationJSON.contains("exceeded the number of monthly transactions"))
        throw QString(QObject::tr("Monthly free plan limit exceeded"));
    if (elevationJSON.contains("Bad Request"))
        throw QString(QObject::tr("Bad request"));
    if (elevationJSON.contains("Gateway Timeout"))
        throw QString(QObject::tr("Gateway Timeout"));
    if (error == QNetworkReply::TimeoutError)
        throw QString(QObject::tr("Connection to remote server timed out"));
    if (error != QNetworkReply::NoError)
        throw QString(QObject::tr("Networkerror: %1")).arg(error);

    elevationJSON = elevationJSON.mid(elevationJSON.indexOf("elevationProfile") + 19);
    elevationJSON = elevationJSON.mid(0, elevationJSON.indexOf("]"));
    elevationJSON.replace("{\"distance\":", "");
    elevationJSON.replace(",\"height\":", "|");
    elevationJSON.replace("}", "");
    //qDebug() << elevationJSON;
    elevationPoints = elevationJSON.split(",");

    return elevationPoints;
}
