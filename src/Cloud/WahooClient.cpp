/*
* Copyright (c) 2026 Simon-Friedrich Böttger (simonboettger (at) googlemail.com)
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

#include "WahooClient.h"

#include <QEventLoop>
#include <QScopedPointer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <climits>

// Generate logging functions for gcWahoo
// only in one source file
Q_LOGGING_CATEGORY(gcWahoo, "gc.wahoo")

QJsonObject
network2Json(const QByteArray &replyData, QStringList &errors)
{
    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(replyData, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        errors << QObject::tr("JSON parser error: %1").arg(parseError.errorString());
        return QJsonObject{};
    }

    const QJsonObject jsonRoot = document.object();
    if (jsonRoot.isEmpty()) {
        errors << QObject::tr("JSON root object empty.");
    }
    return jsonRoot;
}

QByteArray
WahooClient::getNetworkResponse(QUrl url, const QString &token, QStringList &errors, QUrlQuery params)
{
    if (token.isEmpty()) {
        errors << QObject::tr("You must authorize with Wahoo first.");
        return QByteArray{};
    }

    url.setQuery(params);
    QNetworkRequest request(url);

    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Accept", "application/json;charset=UTF-8");

    using NetworkReply = QScopedPointer<QNetworkReply, QScopedPointerDeleteLater>;
    NetworkReply reply(namGet(request));
    if (reply != nullptr && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
        // Find out what went wrong
        switch(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt()){
            case 400:
                errors << QObject::tr("Invalid request: %1").arg(request.url().toString());
                break;
            case 401:
                errors << QObject::tr("The API key %1 is revoked.").arg(token);
                break;
            case 403:
                errors << QObject::tr("No access to this resource: %1").arg(url.toString());
                break;
            case 404:
                errors << QObject::tr("Cannot find %1").arg(url.toString());
                break;
            case 405:
                errors << QObject::tr("Method %1 not allowd").arg(url.toString());
                break;
            case 406:
                errors << QObject::tr("Format is not json.");
                break;
            case 410:
                errors << QObject::tr("This endpoint doesn't exist anymore: %1").arg(url.toString());
                break;
            case 422:
                errors << QObject::tr("Unprocessable Entity because one or more parameters are missing or the API credentials are not approved.");
                break;
            case 429:
                errors << QObject::tr("Too many requests from GoldenCheetah to Wahoo API. Try again later.");
                if (reply->hasRawHeader("X-RateLimit-Reset")){
                    auto secsToReset = reply->rawHeader("X-RateLimit-Reset").toLongLong();
                    errors << QString("API will be available again at %1").arg(QDateTime::currentDateTime().addSecs(secsToReset).toString());
                }
                break;
            case 500:
                errors << QObject::tr("Problem with the Wahoo Server. Try again later.");
                break;
            case 503:
                errors << QObject::tr("The Wahoo API is temporarily offline for maintenance. Try again later.");
                break;
            default:
                errors << QObject::tr("Something with the API is wrong, the status code is %1").arg(
                    reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toString()
                );
        }
        errors << QObject::tr("Error getting network request pointer.");
        return QByteArray{};
    }

    QEventLoop loop;
    auto discard = QObject::connect(reply.get(), &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        errors << QObject::tr("Network error reading Wahoo workouts. ") << reply->errorString();
        return QByteArray{};
    }

    return reply->readAll();
}


QList<WahooWorkoutEntry>
WahooClient::fetchWorkouts(const QString &token, QStringList &errors, const QDateTime &from, const QDateTime &to)
{
    printd("WahooClient::fetchWorkouts from: (%s), to: (%s)\n",
        qPrintable(from.toString()), qPrintable(to.toString()));

    QList<WahooWorkoutEntry> results;

    // The number of workouts on each page
    const int pageSize {30};

    // The index of the first workout on the current requested page
    int offset {0};

    // The index of the last workout of the current user,
    // it will be set to the correct number when the Json object is available
    int resultCount {INT_MAX};

    while (offset < resultCount) {
        QUrl url{WAHOO_URL + "/v1/workouts"};

        QUrlQuery params;
        params.addQueryItem("page", QString("%1").arg(offset/pageSize+1));
        params.addQueryItem("per_page", QString("%1").arg(pageSize));

        const QByteArray replyData = getNetworkResponse(url, token, errors, params);
        const QJsonObject root = network2Json(replyData, errors);
        if (!errors.empty()) return results;

        const QJsonValue total = root.value("total");
        if (total.type() != QJsonValue::Double) {
            errors << QObject::tr("Cannot parse total value of JSON root.");
            return results;
        }

        resultCount = total.toInt();
        // because int is a signed type, a check is required to make sure this number makes sense
        if (resultCount < 0) {
            errors << QObject::tr("Invalid value for total value of Wahoo API response: ");
            errors << total.toVariant().toString();
            return results;
        }

        const QJsonArray workouts = root.value("workouts").toArray();
        if (workouts.isEmpty()) {
            // No current workouts in the API, we are already done here.
            return results;
        }

        for (const QJsonValue &workoutValue : workouts) {
            QJsonObject workout = workoutValue.toObject();
            QString dateStr = workout.value("starts").toString();
            QDateTime startDate = QDateTime::fromString(dateStr, Qt::ISODate).toLocalTime();

            if (!startDate.isValid()) continue;
            // "from" is always a date before "to".
            // if the startDate is older than "from", we are done,
            // because the API always returns workouts with startDates sorted
            // from today to the oldest workout.
            if (startDate < from) return results;

            // Maybe this isn't strictly needed, because CloudService always calls this
            // with "to" being today, but if its there, we should account for it.
            if (startDate > to) continue;

            WahooWorkoutEntry entry;
            entry.id = workout["id"].toVariant().toString();
            entry.label = workout["name"].toString();
            entry.isDir = false;

            QJsonObject summary = workout["workout_summary"].toObject();
            // distance_accum is in meters, while CloudServiceEntry wants the distance in km
            entry.distance = summary["distance_accum"].toVariant().toDouble() / 1000.0;

            // duration_total_accum is in seconds. CloudServiceEntry also wants seconds.
            entry.duration = summary["duration_total_accum"].toVariant().toDouble();

            entry.name = startDate.toString("yyyy_MM_dd_HH_mm_ss") + ".fit";
            results << entry;
            printd("\nAdding name %s\n \twith id: %s \n", qPrintable(entry.name), qPrintable(entry.id));
            printd("\t distance: %f \n \t duration: %f \n", entry.distance, entry.duration);
        }

        offset += pageSize;
    }
    return results;
}

QNetworkReply*
WahooClient::startFileDownload(const QString &token, const QString &remoteId, QStringList &errors)
{
    printd("WahooClient::startFileDownload remoteId: %s", qPrintable(remoteId));

    // First get the download URL from the API, which is in the workout_summary Json object
    QString requestUrl = QString(WAHOO_URL + "/v1/workouts/%1/workout_summary").arg(remoteId);

    const QByteArray replyData = getNetworkResponse(requestUrl, token, errors);
    if (!errors.isEmpty()) {
        printd("Tried to fetch %s.\n", qPrintable(requestUrl));
        return nullptr;
    }

    const QJsonObject workoutSummary = network2Json(replyData, errors);

    // The actual URL is the url tag of the file object
    const QJsonValue file = workoutSummary.value("file").toObject().value("url");
    const QString url = file.toString();
    if (url.isEmpty()) {
        errors << QObject::tr("Wahoo API did not return a file URL.");
        printd("Wahoo API response: %s\n", qPrintable(replyData));
        return nullptr;
    }
    printd("Download url: %s", qPrintable(url));

    // Now get the actual file
    QNetworkRequest request(url);
    return namGet(request);
}
