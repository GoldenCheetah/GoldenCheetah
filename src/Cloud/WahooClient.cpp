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
#include <QVariant>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QTimeZone>
#include <QTimer>
#include <QThread>
#include <QByteArray>
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
        printd("\nJSON parser error for %s\n", qPrintable(document.toJson()));
        return QJsonObject{};
    }

    const QJsonObject jsonRoot = document.object();
    if (jsonRoot.isEmpty()) {
        errors << QObject::tr("JSON root object empty.");
        printd("\nNo JSON root object in %s\n", qPrintable(document.toJson()));
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
    printd("\nQuerying %s\n", qPrintable(url.toString()));
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
        }

    } else if (reply == nullptr){
        errors << "Nullptr in getNetworkRequest.";
    }

    if (!errors.isEmpty()) {
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
    printd("\nWahooClient::fetchWorkouts from: (%s), to: (%s)\n",
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
        if (!errors.empty()) {
            printd("\nErrors in fetchWorkouts: %s\n", qPrintable(errors.join(".")));
            return results; 
        }

        const QJsonValue total = root.value("total");
        if (total.type() != QJsonValue::Double) {
            printd("Error parsing total value. replyData: %s\n", qPrintable(replyData));
            errors << QObject::tr("Cannot parse total value of JSON root.");
            return results;
        }

        resultCount = total.toInt();
        // because int is a signed type, a check is required to make sure this number makes sense
        if (resultCount < 0) {
            errors << QObject::tr("Invalid value for total value of Wahoo API response: ");
            errors << total.toVariant().toString();
            printd("\ntotal is invalid: %s\n", qPrintable(total.toVariant().toString()));
            return results;
        }

        const QJsonArray workouts = root.value("workouts").toArray();
        if (workouts.isEmpty()) {
            // No current workouts in the API, we are already done here.
            printd("\nWorkouts is empty.\n");
            return results;
        }

        for (const auto &workoutValue : workouts) {
            QJsonObject workout = workoutValue.toObject();
            QString dateStr = workout.value("starts").toString();
            QDateTime startDate = QDateTime::fromString(dateStr, Qt::ISODate).toLocalTime();
            printd("\nstartDate: %s\n", qPrintable(startDate.toString()));

            if (!startDate.isValid()) {continue; }
            // "from" is always a date before "to".
            // if the startDate is older than "from", we are done,
            // because the API always returns workouts with startDates sorted
            // from today to the oldest workout.
            if (startDate < from) { return results; }

            // Maybe this isn't strictly needed, because CloudService always calls this
            // with "to" being today, but if its there, we should account for it.
            if (startDate > to) {continue; }

            WahooWorkoutEntry entry;
            entry.id = workout["id"].toVariant().toString();
            entry.label = workout["name"].toString();
            entry.isDir = false;

            QJsonObject summary = workout["workout_summary"].toObject();

            // Sometimes the Wahoo API has "ghost" entries with empty summary objects
            // Don't add these entries
            if (!summary.contains("file")) {continue; }

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
    printd("\nWahooClient::startFileDownload remoteId: %s\n", qPrintable(remoteId));

    // First get the download URL from the API, which is in the workout_summary Json object
    QString requestUrl = QString(WAHOO_URL + "/v1/workouts/%1/workout_summary").arg(remoteId);

    const QByteArray replyData = getNetworkResponse(requestUrl, token, errors);
    if (!errors.isEmpty()) {
        printd("\nTried to fetch %s.\nErrors: %s\n. replyData: %s\n", 
            qPrintable(requestUrl), qPrintable(errors.join(".")), qPrintable(replyData));
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
    printd("\nDownload url: %s\n", qPrintable(url));

    // Now get the actual file
    QNetworkRequest request(url);
    return namGet(request);
}

QNetworkReply *WahooClient::sendFile(const QString &token, QByteArray &data,
                                     const QString &remotename,
                                     const QString &remoteid) {
    printd("\nWahooClient::sendFile with token %s, remotename %s, id %s\n",
        qPrintable(token), qPrintable(remotename), qPrintable(remoteid));

    // Prepare a QUrlQuery
    const QUrl url = QUrl(WAHOO_URL + "/v1/workout_file_uploads");
    const QString fileField = QStringLiteral("data:application/vnd.fit;base64,%1").arg(data.toBase64());
    printd("\nfileField.size: %lli", data.toBase64().size());

    QUrlQuery form;
    form.addQueryItem(QStringLiteral("workout_file_upload[file]"), fileField);
    form.addQueryItem(QStringLiteral("workout_file_upload[filename]"), remotename);
    form.addQueryItem(QStringLiteral("workout_file_upload[time_zone]"), QTimeZone::systemTimeZoneId());
    form.addQueryItem(QStringLiteral("workout_file_upload[workout_name]"), remoteid);

    QByteArray body = form.toString(QUrl::FullyEncoded).toUtf8();
    
    // Create a QNetworkRequest
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toLatin1());
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));
    
    printd("\nCalling API with %s\n", qPrintable(request.url().toString()));
    QNetworkReply *reply = namPost(request, body);
    return reply;
}

QString WahooClient::checkUpload(QNetworkReply *reply, const QString &token) {
    printd("\nWahooClient::checkUpload.\n");

    // Do we have a network error?
    if (reply->error() != QNetworkReply::NoError) {
        printd("\nNetwork reply error. \n%s\n", qPrintable(reply->readAll()));
        return reply->errorString();
    }

    // Can it be converted to a Json?
    const auto response = reply->readAll();
    printd("\nNetwork response in checkUpload. Response: \n%s\n", qPrintable( response));
    
    QStringList errors;
    QJsonObject root = network2Json(response, errors);
    if (!errors.empty()) { 
        printd("\nNetwork response returned errors. \n%s\n", qPrintable(errors.join(". ")));
        return errors.join(". "); 
    }

    // Has the json a non empty error value?
    const QJsonValue jsonError = root.value("error");
    if (!jsonError.isNull()) { 
        printd("\nJson error is non empty. \n%s\n", qPrintable(jsonError.toString()));
        return jsonError.toString(); 
    }

    // Get the upload token
    const QString uploadToken = root.value("token").toString();
    
    // Wait until status complete
    while(errors.isEmpty()) {
        QUrl url = QUrl(QString(WAHOO_URL + "/v1/workout_file_uploads/%1").arg(uploadToken));
        printd("\nCalling %s\n", qPrintable(url.toString()));

        auto replyData = getNetworkResponse(url, token, errors);
        printd("\n Network reply is: \n%s\n", qPrintable(replyData));
        QThread::sleep(1);

        root = network2Json(replyData, errors);

        const QJsonValue total = root.value("status");
        if (total.toString() == "complete") {
            printd("\ncheckUpload Completed.\n")
            return "Completed.";
        }
    }
    printd("\nReturning with errors:\n%s", qPrintable(errors.join(".")));
    return errors.join(".");
}