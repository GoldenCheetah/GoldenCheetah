/*
 * Copyright (c) 2026 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "GarminPersonalAPI.h"

#include <QDebug>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSslError>
#include <QSslSocket>
#include <QScopedPointer>
#include <QTimeZone>
#include <QUrl>
#include <QUrlQuery>
#include <QStringList>

#include <algorithm>


namespace {

    const char * const tokenServicePath = "personalapitoken-service/personal-api/personal-api-key/token";
    const char * const listServicePath = "activitylist-service/activities/search/activities";
    const char * const downloadServicePath = "download-service/files/activity/";
    const char * const hrvServicePath = "hrv-service/hrv/daily/%1/%2";
    const char * const userSummaryServicePath = "usersummary-service/v2/dailysummaries";
    const char * const sleepServicePath = "sleep-service/sleep/dailySleepSummariesByDate";
    const char * const weightServicePath = "weight-service/weight/all/%1/%2";

    class BusyGuard
    {
    public:
        explicit BusyGuard(bool &flag) : flag_(flag), acquired_(! flag) {
            if (acquired_) {
                flag_ = true;
            }
        }

        ~BusyGuard() {
            if (acquired_) {
                flag_ = false;
            }
        }

        BusyGuard(const BusyGuard &) = delete;

        BusyGuard &operator=(const BusyGuard &) = delete;

        bool acquired() const {
            return acquired_;
        }
    private:
        bool &flag_;
        bool acquired_;
    };


    QDateTime
    utcFromMSecs
    (qint64 ms)
    {
        return QDateTime::fromMSecsSinceEpoch(ms, QTimeZone::utc());
    }


    QString
    dateToString
    (const QDate &date)
    {
        return date.toString("yyyy-MM-dd");
    }


    QDate
    dateFromString
    (const QString &text)
    {
        return QDate::fromString(text, "yyyy-MM-dd");
    }


    bool
    splitDateTime
    (const QString &text, QDate &date, QTime &time)
    {
        date = dateFromString(text.left(10));
        time = QTime::fromString(text.mid(11), "HH:mm:ss");
        return date.isValid() && time.isValid();
    }


    QDateTime
    utcFromString
    (const QString &text)
    {
        QDate date;
        QTime time;
        if (! splitDateTime(text, date, time)) {
            return QDateTime();
        }
        return QDateTime(date, time, QTimeZone::utc());
    }


    QDateTime
    localFromString
    (const QString &text)
    {
        QDate date;
        QTime time;
        if (! splitDateTime(text, date, time)) {
            return QDateTime();
        }
        return QDateTime(date, time);
    }


    bool
    contentTypeIs
    (const QString &actual, const QString &expected)
    {
        const QString mime = actual.section(';', 0, 0).trimmed();
        return mime.compare(expected, Qt::CaseInsensitive) == 0;
    }


    QDateTime
    jwtExpiry
    (const QString &token)
    {
        const QList<QByteArray> parts = token.toLatin1().split('.');
        if (parts.size() != 3) {
            return QDateTime();
        }
        const QByteArray payload = QByteArray::fromBase64(parts[1], QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        QJsonParseError jsonError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &jsonError);
        if (jsonError.error != QJsonParseError::NoError || ! doc.isObject()) {
            return QDateTime();
        }
        const QJsonValue exp = doc.object().value("exp");
        if (! exp.isDouble() || exp.toDouble() <= 0) {
            return QDateTime();
        }
        return utcFromMSecs(static_cast<qint64>(exp.toDouble()) * 1000);
    }


    QString
    extractToken
    (const QByteArray &body)
    {
        QByteArray text = body.trimmed();
        if (text.startsWith('{')) {
            const QJsonObject obj = QJsonDocument::fromJson(text).object();
            for (char const *key : {"access_token", "accessToken", "token"}) {
                const QString value = obj.value(QLatin1String(key)).toString().trimmed();
                if (! value.isEmpty()) {
                    return value;
                }
            }
            return QString();
        }
        if (text.size() >= 2 && text.startsWith('"') && text.endsWith('"')) {
            text = text.mid(1, text.size() - 2).trimmed();
        }
        const QString token = QString::fromLatin1(text);
        for (const QChar c : token) {
            if (c.isSpace()) {
                return QString();
            }
        }
        return token;
    }

}


GarminPersonalAPI::GarminPersonalAPI
(const QString &key, const QString &token, const QDateTime &tokenLastRefresh, QObject *parent)
: QObject(parent), manager(new QNetworkAccessManager(this)), apiKey(key), accessToken(token.trimmed()), lastAccessRefresh(tokenLastRefresh)
{
    if (! accessToken.isEmpty()) {
        tokenExpiry = jwtExpiry(accessToken);
        if (! tokenExpiry.isValid() && lastAccessRefresh.isValid()) {
            // Not a JWT: fall back to accessTTL
            tokenExpiry = lastAccessRefresh.addSecs(accessTTL);
        }
    }
}


GarminPersonalAPI::ActivityListing
GarminPersonalAPI::activityListService
(const QDate &startDate, const QDate &endDate, int page)
{
    ActivityListing ret;
    ret.page = std::max(0, page);
    ret.hasMore = false;

    BusyGuard guard(requestInProgress);
    if (! guard.acquired()) {
        ret.fail(Failure::Busy, tr("Another Garmin request is already in progress"));
        return ret;
    }

    if (   ! startDate.isValid()
        || ! endDate.isValid()
        || startDate > endDate) {
        ret.fail(Failure::InvalidArgument, tr("Invalid date range"));
        return ret;
    }

    QMap<QString, QString> query;
    query.insert("limit", QString::number(listLimit));
    query.insert("start", QString::number(static_cast<qint64>(ret.page) * listLimit));
    query.insert("startDate", dateToString(startDate));
    query.insert("endDate", dateToString(endDate));
    query.insert("sortOrder", "asc");

    const QString expectedContentType = "application/json";
    const HttpReply reply = getService(listServicePath, query, listTimeoutMS, expectedContentType);
    static_cast<BasicReply &>(ret) = reply;
    if (ret.hasError()) {
        return ret;
    }

    if (! contentTypeIs(reply.contentType, expectedContentType)) {
        ret.fail(Failure::InvalidReply, tr("Unexpected content type in reply: %1").arg(reply.contentType));
        return ret;
    }

    QJsonParseError jsonError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(reply.data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        ret.fail(Failure::InvalidReply, tr("Failed to parse JSON: %1").arg(jsonError.errorString()));
        return ret;
    }
    if (! jsonDoc.isArray()) {
        ret.fail(Failure::InvalidReply, tr("Invalid JSON: expected an array of activities"));
        return ret;
    }

    const QJsonArray jsonArr = jsonDoc.array();
    for (const QJsonValue &value : jsonArr) {
        const QJsonObject activityObj = value.toObject();

        const qint64 id = activityObj.value("activityId").toInteger();
        if (id <= 0) {
            qWarning() << "Garmin activity listing: skipping entry without activityId";
            continue;
        }

        ActivityListingEntry entry;
        entry.activityId = QString::number(id);
        entry.name = activityObj.value("activityName").toString();

        const double beginTimestamp = activityObj.value("beginTimestamp").toDouble();
        if (beginTimestamp > 0) {
            entry.startTime = utcFromMSecs(static_cast<qint64>(beginTimestamp));
        } else {
            entry.startTime = utcFromString(activityObj.value("startTimeGMT").toString());
        }
        entry.startTimeLocal = localFromString(activityObj.value("startTimeLocal").toString());
        entry.duration = activityObj.value("duration").toDouble();
        entry.distance = activityObj.value("distance").toDouble() / 1000.0;

        ret.activities << entry;
    }
    ret.hasMore = (jsonArr.size() == listLimit);

    return ret;
}


GarminPersonalAPI::ActivityDownload
GarminPersonalAPI::downloadService
(const QString &activityId)
{
    ActivityDownload ret;
    ret.activityId = activityId;

    BusyGuard guard(requestInProgress);
    if (! guard.acquired()) {
        ret.fail(Failure::Busy, tr("Another Garmin request is already in progress"));
        return ret;
    }

    static const QRegularExpression idPattern("^[0-9]{5,15}$");
    if (! idPattern.match(activityId).hasMatch()) {
        ret.fail(Failure::InvalidArgument, tr("Invalid activity id"));
        return ret;
    }

    const HttpReply reply = getService(QString(downloadServicePath) + activityId, QMap<QString, QString>(), downloadTimeoutMS, "application/x-zip-compressed");
    static_cast<BasicReply &>(ret) = reply;
    if (ret.hasError()) {
        return ret;
    }

    if (reply.data.isEmpty()) {
        ret.fail(Failure::InvalidReply, tr("Empty download reply"));
    } else if (! reply.data.startsWith(QByteArrayLiteral("PK\x03\x04"))) {
        ret.fail(Failure::InvalidReply, tr("The reply is not a ZIP archive (content type: %1)").arg(reply.contentType));
    } else {
        ret.data = reply.data;
    }

    return ret;
}


GarminPersonalAPI::HrvListing
GarminPersonalAPI::hrvService
(const QDate &startDate, const QDate &endDate)
{
    HrvListing ret;

    BusyGuard guard(requestInProgress);
    if (! guard.acquired()) {
        ret.fail(Failure::Busy, tr("Another Garmin request is already in progress"));
        return ret;
    }

    if (   ! startDate.isValid()
        || ! endDate.isValid()
        || startDate > endDate) {
        ret.fail(Failure::InvalidArgument, tr("Invalid date range"));
        return ret;
    }

    QMap<QString, QString> query;

    const QString expectedContentType = "application/json";
    const HttpReply reply = getService(QString(hrvServicePath).arg(dateToString(startDate), dateToString(endDate)), query, listTimeoutMS, expectedContentType);
    static_cast<BasicReply &>(ret) = reply;
    if (ret.hasError()) {
        return ret;
    }

    if (! contentTypeIs(reply.contentType, expectedContentType)) {
        ret.fail(Failure::InvalidReply, tr("Unexpected content type in reply: %1").arg(reply.contentType));
        return ret;
    }

    QJsonParseError jsonError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(reply.data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        ret.fail(Failure::InvalidReply, tr("Failed to parse JSON: %1").arg(jsonError.errorString()));
        return ret;
    }
    if (! jsonDoc.isObject()) {
        ret.fail(Failure::InvalidReply, tr("Invalid JSON: expected an object of hrv summaries"));
        return ret;
    }

    const QJsonObject mainObj = jsonDoc.object();
    const QJsonArray hrvArray = mainObj.value("hrvSummaries").toArray();
    for (const QJsonValue &value : hrvArray) {
        const QJsonObject dayObj = value.toObject();

        QDate day = dateFromString(dayObj.value("calendarDate").toString());
        HrvEntry hrvEntry;
        hrvEntry.lastNightAvg = dayObj.value("lastNightAvg").toInt();
        if (   day.isValid()
            && day >= startDate
            && day <= endDate
            && hrvEntry.lastNightAvg > 0) {
            ret.hrvEntries.insert(day, hrvEntry);
        }
    }

    return ret;
}


GarminPersonalAPI::UserSummaryListing
GarminPersonalAPI::userSummaryService
(const QDate &startDate, const QDate &endDate)
{
    UserSummaryListing ret;

    BusyGuard guard(requestInProgress);
    if (! guard.acquired()) {
        ret.fail(Failure::Busy, tr("Another Garmin request is already in progress"));
        return ret;
    }

    if (   ! startDate.isValid()
        || ! endDate.isValid()
        || startDate > endDate) {
        ret.fail(Failure::InvalidArgument, tr("Invalid date range"));
        return ret;
    }

    QMap<QString, QString> query;
    query.insert("startDate", dateToString(startDate));
    query.insert("endDate", dateToString(endDate));

    const QString expectedContentType = "application/json";
    const HttpReply reply = getService(userSummaryServicePath, query, listTimeoutMS, expectedContentType);
    static_cast<BasicReply &>(ret) = reply;
    if (ret.hasError()) {
        return ret;
    }

    if (! contentTypeIs(reply.contentType, expectedContentType)) {
        ret.fail(Failure::InvalidReply, tr("Unexpected content type in reply: %1").arg(reply.contentType));
        return ret;
    }

    QJsonParseError jsonError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(reply.data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        ret.fail(Failure::InvalidReply, tr("Failed to parse JSON: %1").arg(jsonError.errorString()));
        return ret;
    }
    if (! jsonDoc.isObject()) {
        ret.fail(Failure::InvalidReply, tr("Invalid JSON: expected an object of user summaries"));
        return ret;
    }

    const QJsonObject mainObj = jsonDoc.object();
    const QJsonArray summaryArray = mainObj.value("data").toArray();
    for (const QJsonValue &value : summaryArray) {
        const QJsonObject dayObj = value.toObject();
        const QJsonObject hrObj = dayObj.value("heartRate").toObject();
        const QJsonObject calObj = dayObj.value("calories").toObject();
        const QJsonObject bbObj = dayObj.value("bodyBattery").toObject();
        const QJsonObject intObj = dayObj.value("intensityMinutes").toObject();
        const QJsonObject strObj = dayObj.value("stress").toObject();
        const QJsonObject movObj = dayObj.value("movement").toObject();
        const QJsonObject stepObj = movObj.value("steps").toObject();
        const QJsonObject pushObj = movObj.value("pushes").toObject();
        const QJsonObject faObj = movObj.value("floorsAscended").toObject();
        const QJsonObject fdObj = movObj.value("floorsDescended").toObject();

        QDate day = dateFromString(dayObj.value("calendarDate").toString());
        UserSummaryEntry userSummaryEntry;

        userSummaryEntry.hrMinValue = hrObj.value("minValue").toInt();
        userSummaryEntry.hrMaxValue = hrObj.value("maxValue").toInt();
        userSummaryEntry.hrRestingValue = hrObj.value("restingValue").toInt();

        userSummaryEntry.calBurnedResting = calObj.value("burnedResting").toInt();
        userSummaryEntry.calBurnedActive = calObj.value("burnedActive").toInt();
        userSummaryEntry.calBurnedTotal = calObj.value("burnedTotal").toInt();

        userSummaryEntry.bbMinValue = bbObj.value("minValue").toInt();
        userSummaryEntry.bbMaxValue = bbObj.value("maxValue").toInt();
        userSummaryEntry.bbChargedValue = bbObj.value("chargedValue").toInt();
        userSummaryEntry.bbDrainedValue = bbObj.value("drainedValue").toInt();
        userSummaryEntry.bbEndOfDayValue = bbObj.value("endOfDayValue").toInt();

        userSummaryEntry.intModerate = intObj.value("moderate").toInt();
        userSummaryEntry.intVigorous = intObj.value("vigorous").toInt();

        userSummaryEntry.strAvgLevel = strObj.value("avgLevel").toInt();
        userSummaryEntry.strMaxLevel = strObj.value("maxLevel").toInt();
        userSummaryEntry.strTotalDuration = strObj.value("totalDurationInMillis").toInt() / 1000.0;
        userSummaryEntry.strRestDuration = strObj.value("restDurationInMillis").toInt() / 1000.0;
        userSummaryEntry.strActivityDuration = strObj.value("activityDurationInMillis").toInt() / 1000.0;
        userSummaryEntry.strUncategorizedDuration = strObj.value("uncategorizedDurationInMillis").toInt() / 1000.0;
        userSummaryEntry.strLowStressDuration = strObj.value("lowStressDurationInMillis").toInt() / 1000.0;
        userSummaryEntry.strMediumStressDuration = strObj.value("mediumStressDurationInMillis").toInt() / 1000.0;
        userSummaryEntry.strHighStressDuration = strObj.value("highStressDurationInMillis").toInt() / 1000.0;

        userSummaryEntry.movStepsGoal = stepObj.value("goal").toInt();
        userSummaryEntry.movSteps = stepObj.value("value").toInt();
        userSummaryEntry.movStepsDist = stepObj.value("distanceInMeters").toDouble();
        userSummaryEntry.movPushesGoal = pushObj.value("goal").toInt();
        userSummaryEntry.movPushes = pushObj.value("value").toInt();
        userSummaryEntry.movPushesDist = pushObj.value("distanceInMeters").toDouble();
        userSummaryEntry.movFloorsAscGoal = faObj.value("goal").toInt();
        userSummaryEntry.movFloorsAsc = faObj.value("value").toInt();
        userSummaryEntry.movFloorsAscDist = faObj.value("distanceInMeters").toDouble();
        userSummaryEntry.movFloorsDescGoal = fdObj.value("goal").toInt();
        userSummaryEntry.movFloorsDesc = fdObj.value("value").toInt();
        userSummaryEntry.movFloorsDescDist = fdObj.value("distanceInMeters").toDouble();

        if (   day.isValid()
            && day >= startDate
            && day <= endDate) {
            ret.userSummaryEntries.insert(day, userSummaryEntry);
        }
    }

    return ret;
}


GarminPersonalAPI::SleepListing
GarminPersonalAPI::sleepService
(const QDate &startDate, const QDate &endDate)
{
    SleepListing ret;

    BusyGuard guard(requestInProgress);
    if (! guard.acquired()) {
        ret.fail(Failure::Busy, tr("Another Garmin request is already in progress"));
        return ret;
    }

    if (   ! startDate.isValid()
        || ! endDate.isValid()
        || startDate > endDate) {
        ret.fail(Failure::InvalidArgument, tr("Invalid date range"));
        return ret;
    }

    QMap<QString, QString> query;
    query.insert("startDate", dateToString(startDate));
    query.insert("endDate", dateToString(endDate));

    const QString expectedContentType = "application/json";
    const HttpReply reply = getService(sleepServicePath, query, listTimeoutMS, expectedContentType);
    static_cast<BasicReply &>(ret) = reply;
    if (ret.hasError()) {
        return ret;
    }

    if (! contentTypeIs(reply.contentType, expectedContentType)) {
        ret.fail(Failure::InvalidReply, tr("Unexpected content type in reply: %1").arg(reply.contentType));
        return ret;
    }

    QJsonParseError jsonError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(reply.data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        ret.fail(Failure::InvalidReply, tr("Failed to parse JSON: %1").arg(jsonError.errorString()));
        return ret;
    }
    if (! jsonDoc.isArray()) {
        ret.fail(Failure::InvalidReply, tr("Invalid JSON: expected an array of sleep summaries"));
        return ret;
    }

    const QJsonArray sleepArray = jsonDoc.array();
    for (const QJsonValue &value : sleepArray) {
        const QJsonObject dayObj = value.toObject();
        const QJsonObject scObj = dayObj.value("sleepScores").toObject();
        const QJsonObject scOvObj = scObj.value("overall").toObject();

        QDate day = dateFromString(dayObj.value("calendarDate").toString());
        SleepEntry sleepEntry;

        sleepEntry.sleepTime = dayObj.value("sleepTimeSeconds").toInt();
        sleepEntry.napTime = dayObj.value("napTimeSeconds").toInt();
        sleepEntry.unmeasurableSleep = dayObj.value("unmeasurableSleepSeconds").toInt();
        sleepEntry.deepSleep = dayObj.value("deepSleepSeconds").toInt();
        sleepEntry.lightSleep = dayObj.value("lightSleepSeconds").toInt();
        sleepEntry.remSleep = dayObj.value("remSleepSeconds").toInt();
        sleepEntry.awakeSleep = dayObj.value("awakeSleepSeconds").toInt();

        sleepEntry.avgRespiration = dayObj.value("averageRespirationValue").toDouble();
        sleepEntry.lowestRespiration = dayObj.value("lowestRespirationValue").toDouble();
        sleepEntry.highestRespiration = dayObj.value("highestRespirationValue").toDouble();
        sleepEntry.awakeCount = dayObj.value("awakeCount").toInt();
        sleepEntry.avgSleepStress = dayObj.value("avgSleepStress").toDouble();
        sleepEntry.avgHR = dayObj.value("avgHeartRate").toDouble();

        sleepEntry.scoreOverall = scOvObj.value("value").toInt();

        if (   day.isValid()
            && day >= startDate
            && day <= endDate) {
            ret.sleepEntries.insert(day, sleepEntry);
        }
    }

    return ret;
}


GarminPersonalAPI::WeightListing
GarminPersonalAPI::weightService
(const QDate &startDate, const QDate &endDate)
{
    WeightListing ret;

    BusyGuard guard(requestInProgress);
    if (! guard.acquired()) {
        ret.fail(Failure::Busy, tr("Another Garmin request is already in progress"));
        return ret;
    }

    if (   ! startDate.isValid()
        || ! endDate.isValid()
        || startDate > endDate) {
        ret.fail(Failure::InvalidArgument, tr("Invalid date range"));
        return ret;
    }

    QMap<QString, QString> query;

    const QString expectedContentType = "application/json";
    const HttpReply reply = getService(QString(weightServicePath).arg(dateToString(startDate), dateToString(endDate)), query, listTimeoutMS, expectedContentType);
    static_cast<BasicReply &>(ret) = reply;
    if (ret.hasError()) {
        return ret;
    }

    if (! contentTypeIs(reply.contentType, expectedContentType)) {
        ret.fail(Failure::InvalidReply, tr("Unexpected content type in reply: %1").arg(reply.contentType));
        return ret;
    }

    QJsonParseError jsonError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(reply.data, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        ret.fail(Failure::InvalidReply, tr("Failed to parse JSON: %1").arg(jsonError.errorString()));
        return ret;
    }
    if (! jsonDoc.isArray()) {
        ret.fail(Failure::InvalidReply, tr("Invalid JSON: expected an array of weight summaries"));
        return ret;
    }

    const QJsonArray weightArray = jsonDoc.array();
    for (const QJsonValue &value : weightArray) {
        const QJsonObject dayObj = value.toObject();

        QDate day = dateFromString(dayObj.value("calendarDate").toString());
        WeightEntry entry;

        entry.startTime = QDateTime::fromString(dayObj.value("timeStampGMT").toString(), Qt::ISODateWithMs);
        entry.startTimeLocal = QDateTime::fromString(dayObj.value("date").toString(), Qt::ISODateWithMs);
        entry.weight = dayObj.value("weight").toDouble();
        entry.bmi = dayObj.value("bmi").toDouble();
        entry.bodyFat = dayObj.value("bodyFat").toDouble();
        entry.bodyWater = dayObj.value("bodyWater").toDouble();
        entry.boneMass = dayObj.value("boneMass").toInt();
        entry.muscleMass = dayObj.value("muscleMass").toInt();
        entry.physiqueRating = dayObj.value("physiqueRating").toInt();
        entry.visceralFat = dayObj.value("visceralFat").toInt();
        entry.metabolicAge = dayObj.value("metabolicAge").toDouble();
        entry.weightDelta = dayObj.value("weightDelta").toDouble();

        if (   day.isValid()
            && day >= startDate
            && day <= endDate) {
            ret.weightEntries.insert(day, entry);
        }
    }

    return ret;
}


GarminPersonalAPI::AthleteMetricsListing
GarminPersonalAPI::athleteMetrics
(const QDate &startDate, const QDate &endDate, Metrics metrics)
{
    AthleteMetricsListing ret;
    if (ret.failure == Failure::None && metrics.testFlag(Metric::Hrv)) {
        HrvListing hrvListing = hrvService(startDate, endDate);
        static_cast<BasicReply &>(ret) = hrvListing;
        if (hrvListing.failure == Failure::None) {
            ret.availableMetrics |= Metric::Hrv;
            for (auto [day, hrv] : hrvListing.hrvEntries.asKeyValueRange()) {
                ret.metrics[day].hrv = hrv;
            }
        }
    }
    if (ret.failure == Failure::None && metrics.testFlag(Metric::UserSummary)) {
        UserSummaryListing userSummaryListing = userSummaryService(startDate, endDate);
        static_cast<BasicReply &>(ret) = userSummaryListing;
        if (userSummaryListing.failure == Failure::None) {
            ret.availableMetrics |= Metric::UserSummary;
            for (auto [day, userSummary] : userSummaryListing.userSummaryEntries.asKeyValueRange()) {
                ret.metrics[day].userSummary = userSummary;
            }
        }
    }
    if (ret.failure == Failure::None && metrics.testFlag(Metric::Sleep)) {
        SleepListing sleepListing = sleepService(startDate, endDate);
        static_cast<BasicReply &>(ret) = sleepListing;
        if (sleepListing.failure == Failure::None) {
            ret.availableMetrics |= Metric::Sleep;
            for (auto [day, sleep] : sleepListing.sleepEntries.asKeyValueRange()) {
                ret.metrics[day].sleep = sleep;
            }
        }
    }
    if (ret.failure == Failure::None && metrics.testFlag(Metric::Weight)) {
        WeightListing weightListing = weightService(startDate, endDate);
        static_cast<BasicReply &>(ret) = weightListing;
        if (weightListing.failure == Failure::None) {
            ret.availableMetrics |= Metric::Weight;
            for (auto [day, weight] : weightListing.weightEntries.asKeyValueRange()) {
                ret.metrics[day].weight = weight;
            }
        }
    }
    return ret;
}


QString
GarminPersonalAPI::getAccessToken
() const
{
    return accessToken;
}


QDateTime
GarminPersonalAPI::getTokenLastRefresh
() const
{
    return lastAccessRefresh;
}


QDateTime
GarminPersonalAPI::getTokenExpiry
() const
{
    return tokenExpiry;
}


GarminPersonalAPI::HttpReply
GarminPersonalAPI::getService
(const QString &path, const QMap<QString, QString> &query, int timeoutMS, const QString &accept)
{
    HttpReply ret;
    BasicReply tokenFailure;

    if (! ensureAccessToken(false, tokenFailure)) {
        static_cast<BasicReply &>(ret) = tokenFailure;
        return ret;
    }

    auto attempt = [&]() {
        QMap<QString, QString> headers;
        headers.insert("Authorization", QString("Bearer %1").arg(accessToken));
        headers.insert("Accept", accept);
        return getSync(path, headers, query, timeoutMS);
    };

    ret = attempt();
    if (ret.httpCode == 401) {
        // The token was rejected before its expiry: get a new one and retry once
        if (! ensureAccessToken(true, tokenFailure)) {
            static_cast<BasicReply &>(ret) = tokenFailure;
            return ret;
        }
        ret = attempt();
    }
    return ret;
}


bool
GarminPersonalAPI::tokenIsUsable
(const QDateTime &nowUtc) const
{
    if (accessToken.isEmpty() || ! tokenExpiry.isValid()) {
        return false;
    }
    const qint64 secsLeft = nowUtc.secsTo(tokenExpiry);
    if (secsLeft > refreshMarginSecs) {
        return true;
    }
    // Near the end of its validity: keep using a token that was fetched a moment
    // ago instead of requesting a new one before every call
    return    secsLeft > 30
           && lastAccessRefresh.isValid()
           && lastAccessRefresh.secsTo(nowUtc) < 300;
}


bool
GarminPersonalAPI::ensureAccessToken
(bool forceRefresh, BasicReply &tokenFailure)
{
    const QDateTime now = QDateTime::currentDateTimeUtc();
    if (! forceRefresh && tokenIsUsable(now)) {
        return true;
    }
    const bool stillValid =    ! forceRefresh
                            && ! accessToken.isEmpty()
                            && tokenExpiry.isValid()
                            && now < tokenExpiry;
    if (tokenRetryNotBefore.isValid() && now < tokenRetryNotBefore) {
        if (stillValid) {
            return true;
        }
        tokenFailure = lastTokenFailure;
        tokenFailure.errorText = tr("%1 (not retrying before %2)").arg(lastTokenFailure.errorText, tokenRetryNotBefore.toLocalTime().toString("HH:mm:ss"));
        return false;
    }

    QMap<QString, QString> headers;
    headers.insert("Authorization", QString("ApiKey %1").arg(apiKey));
    const HttpReply res = postSync(tokenServicePath, headers, tokenTimeoutMS);

    QString token;
    QDateTime expiry;
    if (! res.hasError()) {
        token = extractToken(res.data);
        if (! token.isEmpty()) {
            expiry = jwtExpiry(token);
            if (! expiry.isValid()) {
                expiry = now.addSecs(accessTTL);
            }
        }
    }

    if (! token.isEmpty() && expiry > now) {
        accessToken = token;
        tokenExpiry = expiry;
        lastAccessRefresh = QDateTime::currentDateTime();
        lastTokenFailure = BasicReply();
        tokenRetryNotBefore = QDateTime();
        return true;
    }

    BasicReply f = res;
    int blockSecs = otherFailureBlockSecs;
    if (res.isAuthError()) {
        f.errorText = tr("Garmin rejected the API key (HTTP %1). It is invalid, expired or deleted; create a new one in Garmin Connect (%2)")
                        .arg(res.httpCode)
                        .arg(garminCreateUrl);
        blockSecs = authFailureBlockSecs;
    } else if (res.hasError()) {
        f.errorText = tr("Could not obtain an access token: %1").arg(res.errorText);
    } else if (token.isEmpty()) {
        f.fail(Failure::InvalidReply, tr("Could not obtain an access token: the reply could not be understood"));
    } else {
        f.fail(Failure::InvalidReply, tr("Could not obtain an access token: the received token is already expired"));
    }

    qDebug() << "Token request failed:" << f.httpCode << f.errorText;

    if (! stillValid) {
        accessToken.clear();
        tokenExpiry = QDateTime();
    }
    lastTokenFailure = f;
    tokenRetryNotBefore = now.addSecs(blockSecs);
    tokenFailure = f;
    return stillValid;
}


GarminPersonalAPI::HttpReply
GarminPersonalAPI::getSync
(const QString &path, const QMap<QString, QString> &headers, const QMap<QString, QString> &query, int timeoutMS) const
{
    QNetworkRequest request;
    enrichRequest(request, path, query);
    enrichRequestHeader(request, headers);
    return execute(request, false, timeoutMS);
}


GarminPersonalAPI::HttpReply
GarminPersonalAPI::postSync
(const QString &path, const QMap<QString, QString> &headers, int timeoutMS) const
{
    QNetworkRequest request;
    enrichRequest(request, path);
    enrichRequestHeader(request, headers);
    return execute(request, true, timeoutMS);
}


GarminPersonalAPI::HttpReply
GarminPersonalAPI::execute
(const QNetworkRequest &request, bool post, int timeoutMS) const
{
    using ReplyPtr = QScopedPointer<QNetworkReply, QScopedPointerDeleteLater>;

    if (! QSslSocket::supportsSsl()) {
        HttpReply ret;
        ret.fail(Failure::Ssl, tr("This build of Qt has no usable TLS support (available backends: %1)").arg(QSslSocket::availableBackends().join(", ")));
        return ret;
    }

    QNetworkRequest req(request);
    req.setTransferTimeout(timeoutMS);

    ReplyPtr reply(post ? manager->post(req, QByteArray()) : manager->get(req));

    QList<QSslError> sslErrors;
    connect(reply.get(), &QNetworkReply::sslErrors, reply.get(), [&sslErrors](const QList<QSslError> &errors) { sslErrors = errors; });

    if (! reply->isFinished()) {
        QEventLoop loop;
        connect(reply.get(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();
    }

    HttpReply ret;
    ret.transportError = reply->error();
    ret.httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    ret.data = reply->readAll();
    ret.contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();

    if (   ret.transportError == QNetworkReply::OperationCanceledError
        && ret.httpCode == 0) {
        ret.transportError = QNetworkReply::TimeoutError;
    }

    const bool httpSuccess = ret.httpCode >= 200 && ret.httpCode <= 299;
    const bool httpAnswered = ret.httpCode != 0;
    const bool sslFailure =    ! httpAnswered
                            && (   ret.transportError == QNetworkReply::SslHandshakeFailedError
                                || ! sslErrors.isEmpty());
    if (sslFailure) {
        QStringList details;
        for (const QSslError &sslError : std::as_const(sslErrors)) {
            details << sslError.errorString();
        }
        if (details.isEmpty()) {
            details << reply->errorString();
        }
        details.removeDuplicates();

        ret.data.clear();
        ret.fail(Failure::Ssl, tr("Could not establish a secure connection to Garmin (%1). Check that the system date and time are correct "
                                  "and whether a proxy, VPN or security software intercepts HTTPS traffic").arg(details.join("; ")));
        qDebug() << "TLS failure:" << details
                 << "backend:" << QSslSocket::activeBackend()
                 << QSslSocket::sslLibraryVersionString();
    } else if (   ! httpAnswered
               || (   httpSuccess
                   && ret.transportError != QNetworkReply::NoError)) {
        ret.data.clear();
        ret.fail(Failure::Transport, (ret.transportError == QNetworkReply::TimeoutError)
                                     ? tr("Request timed out (no data for %1 s)").arg(timeoutMS / 1000)
                                     : tr("Network error: %1").arg(reply->errorString()));
    } else if (! httpSuccess) {
        ret.failure = (ret.httpCode == 401 || ret.httpCode == 403) ? Failure::Auth : Failure::Http;
        ret.errorText = describeHttpStatus(ret.httpCode, reply.get());
        if (   ret.contentType.contains("json", Qt::CaseInsensitive)
            || ret.contentType.startsWith("text/", Qt::CaseInsensitive)) {
            const QString snippet = QString::fromUtf8(ret.data.left(200)).simplified();
            if (! snippet.isEmpty()) {
                ret.errorText += " - " + snippet;
            }
        }
    }

    return ret;
}


QString
GarminPersonalAPI::describeHttpStatus
(int httpCode, const QNetworkReply *reply)
{
    switch (httpCode) {
    case 400:
        return tr("Invalid request (HTTP 400)");
    case 401:
        return tr("Authorization failed (HTTP 401): the token or API key is invalid, expired or revoked");
    case 403:
        return tr("No access to this resource (HTTP 403)");
    case 404:
        return tr("Resource not found (HTTP 404)");
    case 429: {
        QString msg = tr("Too many requests (HTTP 429)");
        if (reply->hasRawHeader("Retry-After")) {
            msg += tr(", retry after %1").arg(QString::fromLatin1(reply->rawHeader("Retry-After")));
        }
        return msg;
    }
    default:
        if (httpCode >= 500) {
            return tr("Garmin server error (HTTP %1), try again later").arg(httpCode);
        }
        return tr("Unexpected reply (HTTP %1): %2").arg(httpCode).arg(reply->errorString());
    }
}


void
GarminPersonalAPI::enrichRequest
(QNetworkRequest &request, const QString &path, const QMap<QString, QString> &query) const
{
    QUrl url(baseUrl);
    url.setPath(baseUrl.path() + "/" + path);
    QUrlQuery urlQuery;
    for (auto [key, value] : query.asKeyValueRange()) {
        urlQuery.addQueryItem(key, value);
    }
    url.setQuery(urlQuery);
    request.setUrl(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);
}


void
GarminPersonalAPI::enrichRequestHeader
(QNetworkRequest &request, const QMap<QString, QString> &headers) const
{
    for (auto [key, value] : headers.asKeyValueRange()) {
        request.setRawHeader(key.toLatin1(), value.toLatin1());
    }
}
