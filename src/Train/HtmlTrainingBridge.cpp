/*
 * Copyright (c) 2026 GoldenCheetah
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

#include "HtmlTrainingBridge.h"
#include "Context.h"
#include "RealtimeData.h"
#include "ErgFile.h"
#include "LocationInterpolation.h" // for geolocation struct
#include "Athlete.h"    // m_context->athlete
#include "Zones.h"      // Zones
#include "HrZones.h"    // HrZones
#include "RideFile.h"   // RideFile::sportTag()

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>
#include <cmath>


// some helper functions

static QJsonArray toJsonArray(const QList<int> &xs)
{
    QJsonArray a;
    for (int v : xs) a.append(v);
    return a;
}

static QJsonArray toJsonArray(const QList<QString> &xs)
{
    QJsonArray a;
    for (const auto &s : xs) a.append(s);
    return a;
}

static QJsonArray toJsonArrayD(const QList<double> &xs)
{
    QJsonArray a;
    for (double v : xs) a.append(v);
    return a;
}

static double deg2rad(double deg)
{
    return deg * M_PI / 180.0;
}

static double haversineDistance(double lat1, double lon1, double lat2, double lon2)
{
    if (lat1 == lat2 && lon1 == lon2) return 0.0;

    double dLat = deg2rad(lat2 - lat1);
    double dLon = deg2rad(lon2 - lon1);
    double lat1_rad = deg2rad(lat1);
    double lat2_rad = deg2rad(lat2);

    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(dLon / 2) * sin(dLon / 2);

    return 6371.0 * 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
}

HtmlTrainingBridge::HtmlTrainingBridge(Context *context, QObject *parent)
    : QObject(parent),
      m_context(context),
      m_plannedRoute(),
      m_currentTelemetry(),
      m_telemetryThrottle(new QTimer(this)),
      m_hasPendingTelemetry(false),
      m_pendingTelemetrySample()
{
    Q_ASSERT(m_context != nullptr);

    // 1Hz throttle
    m_telemetryThrottle->setInterval(1000);
    connect(m_telemetryThrottle, &QTimer::timeout,
            this, &HtmlTrainingBridge::onTelemetryThrottle);

    // Connect context signals
    if (m_context) {
        connect(m_context, &Context::telemetryUpdate, this, &HtmlTrainingBridge::onTelemetryUpdate);
        connect(m_context, static_cast<void (Context::*)(ErgFile*)>(&Context::ergFileSelected), this, &HtmlTrainingBridge::onErgFileSelected);
        connect(m_context, &Context::stop, this, &HtmlTrainingBridge::onStop);
        connect(m_context, &Context::start, this, &HtmlTrainingBridge::onStart);
        connect(m_context, &Context::pause, this, &HtmlTrainingBridge::onPause);
        connect(m_context, &Context::unpause, this, &HtmlTrainingBridge::onResume);
    }
}

HtmlTrainingBridge::~HtmlTrainingBridge()
{
    // m_telemetryThrottle cleaned up by QObject hierarchy
}

QString HtmlTrainingBridge::getConfig()
{

    /*****************
    {
    "max_power_w": 250,
    "max_hr_bpm": 200,
    "power": {
        "cp": 290,
        "ftp": 285,
        "wprime": 20000,
        "pmax": 900,
        "aetp": 220,
        "zoneLows": [0, 160, 200, 240, 280, 320, 360],
        "zoneNames": ["Z1", "Z2", "Z3", "Z4", "Z5", "Z6", "Z7"]
    },
    "hr": {
        "lthr": 170,
        "aethr": 150,
        "restHr": 50,
        "maxHr": 190,
        "zoneLows": [0, 115, 140, 155, 170, 180],
        "zoneNames": ["Z1", "Z2", "Z3", "Z4", "Z5"],
        "zoneTrimps": [0.9, 1.1, 1.2, 2.0, 5.0]
    }
    }
    *****************/

    QJsonObject obj;
    // Defaults in case athlete or zones are not defined
    int maxPower = 250;
    int maxHr = 200;

    // ---- Athlete zones (Bike only)
    if (m_context && m_context->athlete) {

        const QString sportTag = RideFile::sportTag("Bike");
        const QDate today = QDate::currentDate();

        // Power
        if (const Zones *pz = m_context->athlete->zones(sportTag)) {
            const int range = pz->whichRange(today);
            if (range >= 0) {
                QJsonObject p;
                p["cp"]     = pz->getCP(range);
                p["ftp"]    = pz->getFTP(range);
                p["wprime"] = pz->getWprime(range);
                p["pmax"]   = pz->getPmax(range);
                p["aetp"]   = pz->getAeT(range);
                p["zoneLows"]  = toJsonArray(pz->getZoneLows(range));
                p["zoneNames"] = toJsonArray(pz->getZoneNames(range));
                obj["power"] = p;

                const int pmax = pz->getPmax(range);
                if (pmax > 0) {
                    maxPower = pmax;
                } else {
                    const QList<int> highs = pz->getZoneHighs(range);
                    if (!highs.isEmpty() && highs.last() > 0) {
                        maxPower = highs.last();
                    } else {
                        const QList<int> lows = pz->getZoneLows(range);
                        if (!lows.isEmpty()) maxPower = static_cast<int>(std::ceil(lows.last() * 1.15));
                    }
                }

            }
        }

        // HR
        if (const HrZones *hz = m_context->athlete->hrZones(sportTag)) {
            const int range = hz->whichRange(today);
            if (range >= 0) {
                QJsonObject h;
                h["lthr"]   = hz->getLT(range);
                h["aethr"]  = hz->getAeT(range);
                h["restHr"] = hz->getRestHr(range);
                h["maxHr"]  = hz->getMaxHr(range);
                h["zoneLows"]   = toJsonArray(hz->getZoneLows(range));
                h["zoneNames"]  = toJsonArray(hz->getZoneNames(range));
                h["zoneTrimps"] = toJsonArrayD(hz->getZoneTrimps(range)); // opcional
                obj["hr"] = h;

                const int configuredMax = hz->getMaxHr(range);
                if (configuredMax > 0) {
                    maxHr = configuredMax;
                } else {
                    const QList<int> highs = hz->getZoneHighs(range);
                    if (!highs.isEmpty() && highs.last() > 0) {
                        maxHr = highs.last();
                    } else {
                        const int lthr = hz->getLT(range);
                        if (lthr > 0) maxHr = static_cast<int>(std::ceil(lthr * 1.10));
                    }
                }
            }
        }
    }

    obj["max_power_w"] = maxPower;
    obj["max_hr_bpm"] = maxHr;

    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QString HtmlTrainingBridge::getPlannedRoute()
{
    return m_plannedRoute;
}

void HtmlTrainingBridge::onTelemetryUpdate(const RealtimeData &rt)
{
    double lat = rt.getLatitude();
    double lon = rt.getLongitude();
    double alt = rt.getAltitude();
    double slope = rt.getSlope();
    double speed = rt.getSpeed();
    double hr = rt.getHr();
    double power = rt.getWatts();
    double cadence = rt.getCadence();
    double distance = rt.getDistance();
    qint64 msecs = rt.getMsecs();

    // Queue for throttled emission
    QJsonObject tel;
    tel["timestamp"] = formatTimestamp(msecs);
    tel["lat"] = lat;
    tel["lon"] = lon;
    tel["alt"] = alt;
    tel["slope"] = slope;
    tel["speed_kmh"] = speed;
    tel["hr_bpm"] = hr;
    tel["power_w"] = power;
    tel["cadence_rpm"] = cadence;
    tel["distance_km"] = distance;

    m_pendingTelemetrySample = QString::fromUtf8(QJsonDocument(tel).toJson(QJsonDocument::Compact));
    m_hasPendingTelemetry = true;

    if (!m_telemetryThrottle->isActive()) {
        m_telemetryThrottle->start();
    }
}

void HtmlTrainingBridge::onTelemetryThrottle()
{
    if (m_hasPendingTelemetry) {
        m_hasPendingTelemetry = false;
        emit telemetry(m_pendingTelemetrySample);
    }
    m_telemetryThrottle->stop();
}

void HtmlTrainingBridge::onErgFileSelected(ErgFile *file)
{
    if (!file || !file->hasGPS()) {
        m_plannedRoute = QStringLiteral(R"({"geojson":{"type":"FeatureCollection","features":[]},"elevProfile":[]})");
        emit plannedRouteChanged(m_plannedRoute);
        return;
    }

    QJsonArray elevProfile;
    QJsonArray features;

    auto round1 = [](double v) { return std::round(v * 10.0) / 10.0; };
    auto round3 = [](double v) { return std::round(v * 1000.0) / 1000.0; };

    QVector<ErgFilePoint> points;
    points.reserve(file->Points.size());
    for (const ErgFilePoint &pt : file->Points) {
        geolocation geo(pt.lat, pt.lon, pt.y);
        if (geo.IsReasonableGeoLocation()) {
            points.append(pt);
        }
    }

    if (points.isEmpty()) {
        m_plannedRoute = QStringLiteral(R"({"geojson":{"type":"FeatureCollection","features":[]},"elevProfile":[]})");
        emit plannedRouteChanged(m_plannedRoute);
        return;
    }

    double cumulativeDistanceMeters = 0.0;
    double markerIntervalMeters = 5000.0;
    double nextMarkerMeters = markerIntervalMeters;

    const ErgFilePoint &p0 = points[0];
    QJsonArray elevPoint;
    elevPoint.append(0.0);
    elevPoint.append(round1(p0.y));
    elevPoint.append(0.0);
    elevPoint.append(p0.lon);
    elevPoint.append(p0.lat);
    elevProfile.append(elevPoint);

    // Aggregate small consecutive points into longer segments to avoid
    // huge slope spikes caused by near-zero distance pairs.
    const double MIN_SEGMENT_METERS = 5.0;
    double segStartLat = points[0].lat;
    double segStartLon = points[0].lon;
    double segStartEle = points[0].y;
    double segAccumulatedM = 0.0;

    QJsonArray currentLineCoords;
    QJsonArray startCoord;
    startCoord.append(segStartLon);
    startCoord.append(segStartLat);
    currentLineCoords.append(startCoord);


    for (int i = 1; i < points.size(); ++i) {
        const ErgFilePoint &p1 = points[i - 1];
        const ErgFilePoint &p2 = points[i];

        const double distKm = haversineDistance(p1.lat, p1.lon, p2.lat, p2.lon);
        const double distM = distKm * 1000.0;
        cumulativeDistanceMeters += distM;
        segAccumulatedM += distM;

        QJsonArray pointCoord;
        pointCoord.append(p2.lon);
        pointCoord.append(p2.lat);
        currentLineCoords.append(pointCoord);

        const bool isLast = (i == points.size() - 1);

        if (segAccumulatedM >= MIN_SEGMENT_METERS || isLast) {
            double slope = 0.0;
            if (segAccumulatedM > 0.0) {
                const double unevenness = p2.y - segStartEle;
                slope = (unevenness / segAccumulatedM) * 100.0;
            }

            const double kmActual = round3(cumulativeDistanceMeters / 1000.0);
            const double eleActual = round1(p2.y);
            slope = round1(slope);

            QJsonArray elevPoint;
            elevPoint.append(kmActual);
            elevPoint.append(eleActual);
            elevPoint.append(slope);
            elevPoint.append(p2.lon);
            elevPoint.append(p2.lat);
            elevProfile.append(elevPoint);

            QJsonObject lineFeature;
            lineFeature["type"] = "Feature";

            QJsonObject lineProps;
            lineProps["slope"] = slope;
            lineFeature["properties"] = lineProps;

            QJsonObject lineGeom;
            lineGeom["type"] = "LineString";
            lineGeom["coordinates"] = currentLineCoords;
            lineFeature["geometry"] = lineGeom;
            features.append(lineFeature);

            segStartLat = p2.lat;
            segStartLon = p2.lon;
            segStartEle = p2.y;
            segAccumulatedM = 0.0;

            currentLineCoords = QJsonArray();
            currentLineCoords.append(pointCoord);
        }

        while (cumulativeDistanceMeters >= nextMarkerMeters) {
            double ratio = 1.0;
            if (distM > 0.0) {
                ratio = (nextMarkerMeters - (cumulativeDistanceMeters - distM)) / distM;
            }
            double markerLon = p1.lon + (p2.lon - p1.lon) * ratio;
            double markerLat = p1.lat + (p2.lat - p1.lat) * ratio;

            QJsonObject markerFeature;
            markerFeature["type"] = "Feature";

            QJsonObject markerProps;
            markerProps["km"] = static_cast<int>(nextMarkerMeters / 1000.0);
            markerProps["is_marker"] = true;
            markerFeature["properties"] = markerProps;

            QJsonObject markerGeom;
            markerGeom["type"] = "Point";
            QJsonArray markerCoord;
            markerCoord.append(markerLon);
            markerCoord.append(markerLat);
            markerGeom["coordinates"] = markerCoord;
            markerFeature["geometry"] = markerGeom;

            features.append(markerFeature);
            nextMarkerMeters += markerIntervalMeters;
        }
    }


    QJsonObject routeGeoJson;
    routeGeoJson["type"] = "FeatureCollection";
    routeGeoJson["features"] = features;

    QJsonObject route;
    route["geojson"] = routeGeoJson;
    route["elevProfile"] = elevProfile;

    m_plannedRoute = QString::fromUtf8(QJsonDocument(route).toJson(QJsonDocument::Compact));

    emit plannedRouteChanged(m_plannedRoute);
}

void HtmlTrainingBridge::onStart()  { emit stateChanged("start"); }

void HtmlTrainingBridge::onPause()  { emit stateChanged("pause"); }

void HtmlTrainingBridge::onResume() { emit stateChanged("resume"); }

void HtmlTrainingBridge::onStop()
{
    m_hasPendingTelemetry = false;
    if (m_telemetryThrottle) {
        m_telemetryThrottle->stop();
    }
    emit stateChanged("stop");
}

QString HtmlTrainingBridge::formatTimestamp(qint64 msecs) const
{
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(msecs, Qt::UTC);
    return dt.toString(Qt::ISODate);
}
