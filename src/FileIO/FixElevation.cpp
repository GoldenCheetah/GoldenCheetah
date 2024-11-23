/*
 * Copyright (c) 2014 Jon Beverley (jon@csdl.biz)
 * Copyright (c) 2022 Ale Martinez (amtriathlon@gmail.com)
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
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

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
    public:
        // there is no config
        FixElevationConfig(QWidget *parent) : DataProcessorConfig(parent) {

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixElevationErrors));
        }

        void readConfig() {}
        void saveConfig() {}

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
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op) override;

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) const override {
            Q_UNUSED(ride);
            return new FixElevationConfig(parent);
        }

        // Localized Name
        QString name() const override {
            return tr("Fix Elevation errors");
        }

        QString id() const override {
            return "::FixElevation";
        }

        QString legacyId() const override {
            return "Fix Elevation errors";
        }

        QString explain() const override {
            return tr("Fix or add elevation data. If elevation data is "
                      "present it will be removed and overwritten."
                      "\nElevation data is provided by Open-Elevation.com public API,"
                      " consider a donation if you find it useful."
                      "\n\nINTERNET CONNECTION REQUIRED.");
        }

    private:
        QList<double> FetchElevationData(QString latLngCollection);
};

static bool fixElevationAdded = DataProcessorFactory::instance().registerProcessor(new FixElevation());

bool
FixElevation::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(config)
    Q_UNUSED(op)

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

    //loop through points and build a string to sent to Open-Elevation public API
    QList<double> elevationPoints;
    QString latLngCollection = "";
    int pointCount = 0;
    try {

        for (std::vector<elevationGPSPoint>::iterator point = elvPoints.begin();
             point != elvPoints.end(); ++point) {

            if (latLngCollection.length() == 0) {
                latLngCollection.append("{\"locations\":[");
            } else {
                latLngCollection.append(',');
            }

            // these values need extended precision or place marker jumps around.
            latLngCollection.append("{\"latitude\":" + QString::number(point->lat,'g',10));
            latLngCollection.append(',');
            latLngCollection.append("\"longitude\":" + QString::number(point->lon,'g',10) + "}");

            // To avoid 302 error for longer rides we break requests in 2000 points chunks
            if (pointCount == 2000) {
                latLngCollection.append("]}");
                elevationPoints = elevationPoints + FetchElevationData(latLngCollection);
                latLngCollection = "";
                pointCount = 0;
            } else {
                ++pointCount;
            }
        }

        // send a request for the remainder points, currently all at once for efficiency
        if (pointCount > 0) {
            latLngCollection.append("]}");
            elevationPoints = elevationPoints + FetchElevationData(latLngCollection);
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
            double elev = elevationPoints.at(i);
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

QList<double>
FixElevation::FetchElevationData(QString latLngCollection)
{
    QList<double> elevationPoints;

    QNetworkRequest request(QUrl(QString("https://api.open-elevation.com/api/v1/lookup")));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");

    QNetworkAccessManager *networkMgr = new QNetworkAccessManager();
    QNetworkReply *reply = networkMgr->post(request, latLngCollection.toUtf8());

    QEventLoop loop;
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));

    // Execute the event loop here, now we will wait here until readyRead() signal is emitted
    // which in turn will trigger event loop quit.
    loop.exec();

    QByteArray elevationJSON;
    elevationJSON = reply->readAll();

    QNetworkReply::NetworkError error = reply->error();

    delete networkMgr;

    if (error == QNetworkReply::TimeoutError)
        throw tr("Connection to remote server timed out");
    if (error != QNetworkReply::NoError)
        throw tr("Network error: %1").arg(error);

    // No error, lets parse the response
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(elevationJSON , &parseError);
    if (parseError.error == QJsonParseError::NoError) {

        // results ?
        QJsonArray results = document["results"].toArray();
        // lets look at that then
        if (results.size() > 0) {

            for(int i=0; i<results.size(); i++) {

                QJsonObject each = results.at(i).toObject();
                elevationPoints << each["elevation"].toDouble();
            }
        } else {

            throw tr("Unexpected response from server: %1").arg(QString(elevationJSON));
        }
    } else {

        // parse error
        throw tr("Parse response error: %1").arg(parseError.errorString());
    }

    return elevationPoints;
}
