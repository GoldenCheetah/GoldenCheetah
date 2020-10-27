/*
 * Copyright (c) 2016 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

/* TODO - IDEAS
 *
 * addExerciseSummaryInformation - get detailed-sport-info:
 *  - replace works for mor then one underline
 *  - separation of the sections of the string works for one underline
 *    - if separating is needed:
 *    - create function and do it in loop -> elemination underline by underline
 *      - after one underline is eleminated do the next run with rest of string
 *      - do untill no underline is left
 *
 * samples:
 *  - needed steps to get the samples are done
 *  - getting samples and write the to a new Ridefile works
 *  - working with samples and combine them with downloaded file -> needs to be done
 *  - choose if downloaded file is choosen or usage of samples  -> needs to be done
*/

#include "PolarFlow.h"
#include "JsonRideFile.h"
#include "Athlete.h"
#include "Settings.h"
#include "Secrets.h"
#include "mvjson.h"
#include "Units.h"
#include "RideImportWizard.h"
//#include "MergeActivityWizard.h"
//#include "MainWindow.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#ifndef POLARFLOW_DEBUG
// TODO(gille): This should be a command line flag.
#define POLARFLOW_DEBUG false //true|false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (POLARFLOW_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (POLARFLOW_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

#ifndef POLARFLOW_IMPORTTYPE
// How to import ecxercises File|Samples
#define POLARFLOW_IMPORTTYPE "File"
#endif

#ifndef POLARFLOW_IMPORTFILEFORMAT
// ecxercise format for file import TCX|FIT|GPX
#define POLARFLOW_IMPORTFILEFORMAT "TCX"
#endif

#ifndef POLARFLOW_COMMITTRANSACTIONOVERRIDE
// committing the transaction is done by default
// so we use no overrride "false"
// for debuging purposes you have the posibility to override and not commiting the transaction
// true|false
#define POLARFLOW_COMMITTRANSACTIONOVERRIDE "false"
#endif


PolarFlow::PolarFlow(Context *context) : CloudService(context), context(context) {

    if (context) {
        nam = new QNetworkAccessManager(this);
        connect(nam, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
    }

    uploadCompression = none;
    downloadCompression = none;
    //filetype = TCX;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(OAuthToken, GC_POLARFLOW_TOKEN);
    settings.insert(Local1, GC_POLARFLOW_USER_ID);
    settings.insert(Local2, GC_POLARFLOW_TRANSACTION_ID); //
    //settings.insert(Local3, GC_POLARFLOW_EXERCISE_ID); //
    settings.insert(Local4, GC_POLARFLOW_STATUS_CODE);
    settings.insert(Local5, GC_POLARFLOW_ACTIVITY_COUNTER);
    settings.insert(Local6, GC_POLARFLOW_LAST_EXERCISE_ID);
    //settings.insert(Local7, GC_POLARFLOW_SAMPLE_SIZE); // samples - streams
    //settings.insert(Local8, GC_POLARFLOW_TIMESAMPLES_LOOPNUMBER); // samples - streams
    //settings.insert(Local9, GC_POLARFLOW_SAMPLE_RECORDINGRATE); // samples - streams
    //settings.insert(Metadata1, QString("%1::Activity Name").arg(GC_POLARFLOW_ACTIVITY_NAME));
}

PolarFlow::~PolarFlow() {
    if (context) delete nam;
}

/*
QImage
PolarFlow::logo() const
{
    printd("logo\n");
    //return QImage(":images/services/polarflow.png");
}
*/

void
PolarFlow::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

bool
PolarFlow::open(QStringList &errors) //open transaction
{
    printd("open - start\n");

    // Check Debug Override for committing Transactions
    QString commitTransactionOverride = POLARFLOW_COMMITTRANSACTIONOVERRIDE; // true/false

    // open transaction and get valid transaction id and save it to GoldenCheetahSetting
    //
    // Training data - create transaction and save temporarily transaction-id
    // https://www.polar.com/accesslink-api/?shell#create-transaction-2
    //
    printd("PolarFlow::open\n");
    // do we have a token
    QString token = getSetting(GC_POLARFLOW_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Polar Flow first");
        return false;
    }

  // Command-URL: POST https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions
    QString url = QString("%1/v3/users/%2/exercise-transactions")
            .arg(getSetting(GC_POLARFLOW_URL, "https://www.polaraccesslink.com").toString())
            .arg(getSetting(GC_POLARFLOW_USER_ID, "").toString());

    printd("URL used: %s\n", url.toStdString().c_str());

    // request using the bearer
    QNetworkRequest request(QUrl(url.toStdString().c_str()));
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Accept", "application/json");

    QString data;
    data += "";

    // create transaction
    QNetworkReply* reply = nam->post(request, data.toLatin1());

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("HTTP response code: %d\n", statusCode);

    if (statusCode == 201) {
        printd("There is new training session data available");

        // Save status code for commit transaction
        //setSetting(GC_POLARFLOW_STATUS_CODE, statusCode);
        //CloudServiceFactory::instance().saveSettings(this, context);

        // oops, no dice
        if (reply->error() != 0) {
            printd("Got error %s\n", reply->errorString().toStdString().c_str());
            errors << reply->errorString();
            return false;
        }

        // Answer-URL: https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}
        // lets extract the transaction id
        QByteArray transactions_Ary = reply->readAll();
        printd("response: %s\n", transactions_Ary.toStdString().c_str());

        //{
        //  "transaction-id": 122,
        //  "resource-uri": "https://polaraccesslink.com/v3/users/21/physical-information-transactions/32"
        //}

        //Response string is JSON data
        QJsonParseError transactions_Doc_parseError;
        QJsonDocument transactions_Doc = QJsonDocument::fromJson(transactions_Ary, &transactions_Doc_parseError);

        // failed to parse result !?
        if (transactions_Doc_parseError.error != QJsonParseError::NoError) {
            printd("Parse error! - StatusCode:%d\n", statusCode);
            errors << tr("JSON parser error \n","StatusCode:%1\n").arg(statusCode) << transactions_Doc_parseError.errorString();
            return false;
        }
        printd("transactions_Doc Error Number: %d Error String: %s \n", transactions_Doc_parseError.error, transactions_Doc_parseError.errorString().toStdString().c_str());
        //qDebug() << "open - transactions_Doc: ", transactions_Doc);

        //The number has to be converted to an int and than to a string
        //https://stackoverflow.com/questions/61075951/minimal-example-on-how-to-read-write-and-print-qjson-code-with-qjsondocument
        QJsonObject transaction_id_Obj = transactions_Doc.object();
        //qDebug() << "open - transaction_id_Object: " << transactions_Doc.object();

        QJsonValue transaction_id_Val = transaction_id_Obj.value("transaction-id");
        //qDebug() << "open - transaction_id_Value: " << transaction_id_Val;

        QString transaction_id = QString::number(transaction_id_Val.toInt());
        printd("transaction_id_String: %s \n", transaction_id.toStdString().c_str());

        // save temporary transactionID to settings
        setSetting(GC_POLARFLOW_TRANSACTION_ID, transaction_id);
        // get the factory to save our settings permanently
        CloudServiceFactory::instance().saveSettings(this, context);
        printd("open - end");
    } //if close

    if (statusCode == 204) {
    //else {
        printd("check override commit - Override: %s \n ", commitTransactionOverride.toStdString().c_str());
        if (commitTransactionOverride == "false") {
            printd("no override, so we commit the transaction \n");
            commit(errors);
        } // if override close
        printd("There is no new training session data available");
    } // if close
    return true;
}

bool
PolarFlow::commit(QStringList &errors) //commit transaction
{
    // commit transaction and delete transaction id
    // https://www.polar.com/accesslink-api/?shell#commit-transaction-2
    // prerquisitory: there has to be an open transaction (Token, TransactionID)
    printd("PolarFlow::commit - start\n");

    // do we have a token
    QString token = getSetting(GC_POLARFLOW_TOKEN, "").toString();
    if (token == "") {
        errors << tr("There is no Token");
        return false;
    }

    // do we have a transactionID
    QString transaction_id = getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString();
    if (transaction_id == "") {
        errors << tr("There is no TransactionID");
        return false;
    }

    // Command-URL: PUT https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}
    QString url = QString("%1/v3/users/%2/exercise-transactions/%3")
            .arg(getSetting(GC_POLARFLOW_URL, "https://www.polaraccesslink.com").toString())
            .arg(getSetting(GC_POLARFLOW_USER_ID, "").toString())
            .arg(getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString());

    printd("URL used: %s\n", url.toStdString().c_str());

    // request using the bearer
    QNetworkRequest request(QUrl(url.toStdString().c_str()));
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QString data;
    data += "";

    // commit transaction
    QNetworkReply* reply = nam->put(request, data.toLatin1());

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("HTTP response code: %d\n", statusCode);

    // oops, no dice
    if (reply->error() != 0) {
        printd("Got error %s\n", reply->errorString().toStdString().c_str());
        errors << reply->errorString();
        return false;
    }

    // delete temporary transactionID / StatusCode / Activities Counter / Last Exercise ID from settings
    setSetting(GC_POLARFLOW_TRANSACTION_ID, "0");
    CloudServiceFactory::instance().saveSettings(this, context);
    setSetting(GC_POLARFLOW_STATUS_CODE, "0");
    CloudServiceFactory::instance().saveSettings(this, context);
    setSetting(GC_POLARFLOW_ACTIVITY_COUNTER, "0");
    CloudServiceFactory::instance().saveSettings(this, context);
    setSetting(GC_POLARFLOW_LAST_EXERCISE_ID, "0");
    CloudServiceFactory::instance().saveSettings(this, context);

    printd("Commit close - end\n");
    return true;
}

bool
PolarFlow::close()
{
    printd("PolarFlow::End\n");
    // nothing to do for now
    return true;
}

int
PolarFlow::isoDateToSeconds(QString &string)
{
    // https://www.geeksforgeeks.org/return-statement-in-c-cpp-with-examples/?ref=lbp
    printd("isoDateToSeconds - start \n");

    QString duration_str = string;
    printd("isoDateToSeconds - duration_String ISO: %s\n", duration_str.toStdString().c_str());

    int secondsPerMinute = 60;
    int minutesPerHour = 60;
    int hoursPerDay = 24;
    float secondsAreZero = 0.000;
    int minutesAreZero = 0;
    int hoursAreZero = 0;

    bool containsSeconds = false;
    bool containsMinutes = false;
    bool containsHours = false;

    // find out if ISODate contains all needed conversation info
    if (duration_str.contains("S")) {
        containsSeconds = true;
    }
    if (duration_str.contains("M")) {
        containsMinutes = true;
    }
    if (duration_str.contains("H")) {
        containsHours = true;
    }

    // prepend seconds to ISODate if not provided
    if (containsSeconds == false) {
        duration_str.prepend("S0.000");
    }
    printd("isoDateToSeconds - duration_String ISO after eventually prepend: %s\n", duration_str.toStdString().c_str());

    // check contained letters H, M, S ; Hours, Minutes ? true / false
    QString duration_strtime;

    // Hours, Minutes and Seconds are provided in String
    if (containsHours == true && containsMinutes == true && containsSeconds == true) {
        duration_strtime = duration_str.replace("PT", "").replace("H", ":").replace("M", ":").replace("S", ""); // PT4H51M38.512S->4:51:38.512
    };
    // Minutes are Zero - Hours und Seconds are provided in String
    if (containsHours == true && containsMinutes == false && containsSeconds == true) {
        duration_strtime = duration_str.replace(QString("H"), QString("H0M"));
        duration_strtime = duration_str.replace("PT", "").replace("H", ":").replace("M", ":").replace("S", ""); // PT43H8.512S->PT43H0M8.512S->4:00:38.512
    };
    // Hours are Zero - Minutes and Seconds are provided in String
    if (containsHours == false && containsMinutes == true && containsSeconds == true) {
        duration_strtime = duration_str.replace("PT", "").prepend("0:").replace("M", ":").replace("S", ""); // 51M38.512S->0:51:38.512
    };
    // Hours and Minutes are Zero - Seconds are provided in String
    if (containsHours == false && containsMinutes == false && containsSeconds == true) {
        duration_strtime = duration_str.replace("PT", "").prepend("0:0:").replace("S", ""); //  38.512S->0:0:38.512
    };
    QTime duration_astime = QTime::fromString(duration_strtime, "h:m:s.z");
    //qDebug() << "isoDateToSeconds - duration as time:" << duration_astime;

    int durationHours = duration_astime.hour();
    int durationMinutes = duration_astime.minute();
    int durationSeconds = duration_astime.second();

    int durationTimeInSeconds = (durationHours * minutesPerHour * secondsPerMinute) + (durationMinutes * secondsPerMinute) + durationSeconds;

    //qDebug() printd("isoDateToSeconds - add duration_string: %s\n", duration_str.toStdString().c_str());
    //qDebug() printd("isoDateToSeconds - add duration_stringtime: %s\n", duration_strtime.toStdString().c_str());
    //qDebug() printd("isoDateToSeconds - add duration_astime: %s\n", duration_astime);
    printd("isoDateToSeconds - add duration_hours: %d\n", durationHours);
    printd("isoDateToSeconds - add duration_minute: %d\n", durationMinutes);
    printd("isoDateToSeconds - add duration_second: %d\n", durationSeconds);
    printd("isoDateToSeconds - duration_time in seconds: %d\n", durationTimeInSeconds);

    printd("isoDateToSeconds - end\n");
    return durationTimeInSeconds;
}

QList<CloudServiceEntry*>
PolarFlow::readdir(QString path, QStringList &errors)
//PolarFlow::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    // use transactionID getting a list of exercises
    // https://www.polar.com/accesslink-api/?shell#list-exercises

    printd("PolarFlow::readdir - start (%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    // do we have a token
    QString token = getSetting(GC_POLARFLOW_TOKEN, "").toString();
    if (token == "") {
        errors << tr("You must authorise with Polar Flow first");
        return returning;
    }

    // do we have a transactionID
    QString transaction_id = getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString();
    if (transaction_id == "") {
        errors << tr("No Transaction ID available");
        return returning;
    }

 //Command URL: GET https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}
    QString url = QString("%1/v3/users/%2/exercise-transactions/%3")
          .arg(getSetting(GC_POLARFLOW_URL, "https://www.polaraccesslink.com").toString())
          .arg(getSetting(GC_POLARFLOW_USER_ID, "").toString())
          .arg(getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString());

    printd("URL used: %s\n", url.toStdString().c_str());

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Accept", "application/json");

    // make request
    QNetworkReply *reply = nam->get(request);
    printd("response : %s\n", url.toStdString().c_str());

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // if successful, lets unpack
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("readdir - HTTP response: %d: %s\n", reply->error(), reply->errorString().toStdString().c_str());
    printd("readdir - HTTP response status code: %d\n", statusCode);

    // Save statusCode 200=OK, 204=No Content
    setSetting(GC_POLARFLOW_STATUS_CODE, statusCode);
    CloudServiceFactory::instance().saveSettings(this, context);

    // Answer-URL https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}
    // Returns a list of hyperlinks to available exercises
    // https://www.polar.com/accesslink-api/?shell#list-exercises
    QByteArray listExercises_Ary = reply->readAll();

    printd("readdir - listExercises_Array: %s\n", listExercises_Ary.toStdString().c_str());

    // parse JSON payload
    QJsonParseError listExercises_Doc_parseError;
    QJsonDocument listExercises_Doc;
                  listExercises_Doc = QJsonDocument::fromJson(listExercises_Ary, &listExercises_Doc_parseError);
    printd("readdir - listExercises_Document - Error Number: %d - Error String: %s \n", listExercises_Doc_parseError.error, listExercises_Doc_parseError.errorString().toStdString().c_str());
    //qDebug() << "readdir - listExercises_Document - Url-List to Execerises: " << listExercises_Doc;

    QJsonObject exercisesUrlList_Obj;
    QJsonArray exerciseUrls_Ary;
    QString exerciseUrl_Str;
    // Polar Activities counter, if there is no new activity the number is exactly max or smaller then max
    // https://www.polar.com/accesslink-api/?shell#training-data
    int activityCounter = 0;
    int activityCounterDelta = 0;
    int activityCounterMax = 50;
    // fetch exercise-id from last listed Activity

    // populating array with urls to activites
    if (listExercises_Doc_parseError.error == QJsonParseError::NoError) {

        exercisesUrlList_Obj = listExercises_Doc.object();
        //qDebug() << "readdir - listExercises_Object - Url-List to Execerises: " << exercisesUrlList_Obj;

        exerciseUrls_Ary = exercisesUrlList_Obj["exercises"].toArray();
        //qDebug() << "readdir - exerciseUrls_Ary - exerciseUrls: " << exerciseUrls_Ary;

        activityCounter = exerciseUrls_Ary.size() + activityCounterDelta;
        setSetting(GC_POLARFLOW_ACTIVITY_COUNTER, activityCounter);
        CloudServiceFactory::instance().saveSettings(this, context);
        printd("Counter: %d\n", activityCounter);

        if (activityCounter < activityCounterMax) {
            // if Polar Activities counter is not max, there is no new activity available
            // so we can set status code "204"
            statusCode = 204;
            printd("Counter end: %d - new Status Code: %d\n", activityCounter, statusCode);
            printd("no more new activities available\n");
            // Save status code for commit transaction
            //setSetting(GC_POLARFLOW_STATUS_CODE, statusCode);
            //CloudServiceFactory::instance().saveSettings(this, context);
        }
        if (activityCounter == activityCounterMax) {
            // if it is max, it is possible that there is no new activity available
            // so we may set status code "204"
            //statusCode = 204;
            printd("Counter end: %d - new Status Code: %d\n", activityCounter, statusCode);
            printd("there may be more new activities available\n");
            // Save status code for commit transaction
            //setSetting(GC_POLARFLOW_STATUS_CODE, statusCode);
            //CloudServiceFactory::instance().saveSettings(this, context);
        }
        // after saving, reset activity counter to zero
        //activityCounter = 0;
    }

    // running through list of exercise Urls
    for(int i = 0 ; i < exerciseUrls_Ary.size(); i++) {
        int loopCounter = i + 1; // Loop Counter starts with one not with zero
        exerciseUrl_Str = exerciseUrls_Ary[i].toString();
        printd("loop i: %d (max=%d) exerciseUrl_String - Url to exercise: %s \n", loopCounter, activityCounterMax, exerciseUrl_Str.toStdString().c_str());

        // Command URL: GET https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}
        // done per exercise Url
        QNetworkRequest request(exerciseUrl_Str);
        request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
        request.setRawHeader("Accept", "application/json");

        QNetworkReply *reply = nam->get(request);
         printd("response : %s\n", url.toStdString().c_str());

        // blocking request
        QEventLoop loop;
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();

        // if successful, lets unpack
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        printd("loop i:%d - errornumber: %d - error text: %s\n", loopCounter, reply->error(), reply->errorString().toStdString().c_str());
        printd("loop i:%d - Exercise HTTP response status code: %d\n", loopCounter, statusCode);

        // Answer-URL https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}
        // Returns JSON
        // https://www.polar.com/accesslink-api/?shell#get-exercise-summary
        QByteArray get_exercise_summary_Ary = reply->readAll();
        printd("summary - loop i:%d - Exercise summary response string: %s \n", loopCounter, get_exercise_summary_Ary.toStdString().c_str());

        //Response string is JSON data
        //https://www.polar.com/accesslink-api/?shell#get-exercise-summary
        QJsonParseError get_exercise_summary_Doc_parseError;
        QJsonDocument get_exercise_summary_Doc;
                      get_exercise_summary_Doc = QJsonDocument::fromJson(get_exercise_summary_Ary, &get_exercise_summary_Doc_parseError);
        //qDebug() << "readdir - loop i:"<< loopCounter << " - get_exercise_summary_Document - Error Number: " << get_exercise_summary_Doc_parseError.error << " - Error String: " << get_exercise_summary_Doc_parseError.errorString();
        //qDebug() << "readdir - loop i:"<< loopCounter << " - get_exercise_summary_Document: " << get_exercise_summary_Doc;

        QJsonObject get_exercise_summary_Obj = get_exercise_summary_Doc.object();
        //qDebug() << "readdir - loop i:"<< loopCounter << " - get_exercise_summary_Object: " << get_exercise_summary_Obj;

        CloudServiceEntry *add = newCloudServiceEntry();

            // each item looks like this:
            /*
              {
                "id": 1937529874,                                       // integer(int64)
                "upload-time": "2008-10-13T10:40:02Z",                  // string
                "polar-user": "https://www.polaraccesslink/v3/users/1", // string
                "transaction-id": 179879,                               // integer(int64)
                "device": "Polar M400",                                 // string
                "start-time": "2008-10-13T10:40:02Z",                   // string
                "start-time-utc-offset": 180,                           // integer(int32)
                "duration": "PT2H44M",                                  // string
                "calories": 530,                                        // integer(int32)
                "distance": 1600,                                       // number(float)
                "heart-rate": {
                  "average": 129,                                       // integer(int32)
                  "maximum": 147                                        // integer(int32)
                },
                "training-load": 143.22,                                // number(float)
                "sport": "OTHER",                                       // string
                "has-route": true,                                      // boolean
                "club-id": 999,                                         // integer(int64)
                "club-name": "Polar Club",                              // string
                "detailed-sport-info": "WATERSPORTS_WATERSKI",          // string
                "fat-percentage": 60,                                   // integer(int32)
                "carbohydrate-percentage": 38,                          // integer(int32)
                "protein-percentage": 2                                 // integer(int32)
                "running-index": 51                                     // integer(int32)
              }
             */

        //Polar reports in local time
        QDateTime startDate = QDateTime::fromString(get_exercise_summary_Obj["start-time"].toString(), Qt::ISODate);
        //startDate.setTimeSpec(Qt::UTC);
        //startDate = startDate.toLocalTime();
        //qDebug() << "readdir - loop i:"<< loopCounter << " - get_exercise_summary_Object - add name: " << startDate;

        //Convert frome ISO to Seconds
        QString duration_str = get_exercise_summary_Obj["duration"].toString(); // "PT4H51M38.512S"
        int duration_time = isoDateToSeconds(duration_str);
        printd("prepareResponse - Duration Int: %d \n", duration_time);

        add->label = QFileInfo(get_exercise_summary_Obj["detailed-sport-info"].toString()).fileName();
        add->id = QString("%1").arg(get_exercise_summary_Obj["id"].toVariant().toULongLong());

        add->isDir = false;
        add->distance = get_exercise_summary_Obj["distance"].toDouble()/1000.0f;
        add->duration = duration_time;
        add->name = startDate.toString("yyyy_MM_dd_HH_mm_ss")+".json";

        //qDebug() << "readdir - loop - get_exercise_summary_Object - add label: " << get_exercise_summary_Obj["detailed-sport-info"].toString();
        //qDebug() << "readdir - loop - get_exercise_summary_Object - add exercise id: " << QString("%1").arg(get_exercise_summary_Obj["id"].toVariant().toULongLong());
        //qDebug() << "readdir - loop - add isDir: " << isDir;
        //qDebug() << "readdir - loop - get_exercise_summary_Object - add distance: " << get_exercise_summary_Obj["distance"].toDouble()/1000.0;
        //qDebug() << "readdir - loop - add duration: " << duration_time;
        //qDebug() << "readdir - loop - get_exercise_summary_Object - add name: " << startDate.toString("yyyy_MM_dd_HH_mm_ss")+".json";

        printd("direntry: %s %s\n", add->id.toStdString().c_str(), add->name.toStdString().c_str());
        printd("number of last loop: %d (max=%d) - last Excercise Id: \n", loopCounter, activityCounterMax);

        // fetch last exercise id
        if(loopCounter == exerciseUrls_Ary.size()) {
            QString lastListedExerciseId = QString::number(get_exercise_summary_Obj["id"].toInt());
            //int lastListedExerciseId = get_exercise_summary_Obj["id"].toInt();
            setSetting(GC_POLARFLOW_LAST_EXERCISE_ID, lastListedExerciseId);
            CloudServiceFactory::instance().saveSettings(this, context);
            printd("number of last loop: %d (max=%d) - last Excercise Id (saved): %s \n", loopCounter, activityCounterMax, lastListedExerciseId.toStdString().c_str());
        }
        returning << add;
    }
    printd("loop end \n");

    // all good ?
    printd("returning count(%d), errors(%s)\n", returning.count(), errors.join(",").toStdString().c_str());
    printd("closed \n");
    return returning;
}

bool
PolarFlow::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("readFile - start\n");
    // create alias
    QString exercise_id_Str = remoteid;
    printd("PolarFlow::readFile remotename: %s - remoteid: %s \n", remotename.toStdString().c_str(), exercise_id_Str.toStdString().c_str());
    printd("readFile - exercise_id_String alias: %s\n", exercise_id_Str.toStdString().c_str());

    // do we have a token ?
    QString token = getSetting(GC_POLARFLOW_TOKEN, "").toString();
    if (token == "") return false;

    // do we have a transactionID
    QString transaction_id = getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString();
    if (transaction_id == "") return false;

    // connect to direct to exercise via remoteid from PolarFlow::readDir
    // https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}
    QString url;
            url = QString("%1/v3/users/%2/exercise-transactions/%3/exercises/%4")
            .arg(getSetting(GC_POLARFLOW_URL, "https://www.polaraccesslink.com").toString())
            .arg(getSetting(GC_POLARFLOW_USER_ID, "").toString())
            .arg(getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString())
            .arg(exercise_id_Str);

    printd("readFile - url:%s\n", url.toStdString().c_str());

    // request using the bearer token
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
    request.setRawHeader("Accept", "application/json");

    // put the file
    QNetworkReply *reply = nam->get(request);

    // remember
    mapReply(reply,remotename);
    buffers.insert(reply,data);

    // catch finished signal
    connect(reply, SIGNAL(finished()), this, SLOT(readFileCompleted()));
    connect(reply, SIGNAL(readyRead()), this, SLOT(readyRead()));
    printd("readFile - end\n\n");
    return true;
}

bool
PolarFlow::writeFile(QByteArray &data, QString remotename, RideFile *ride)
{
    printd("writeFile - start\n");
// to do - Polar Flow does not support upload
    return false;
    //return true;
}

void
PolarFlow::writeFileCompleted()
{
    printd("writeFileCompleted - start \n");
// to do - Polar Flow does not support upload
}

void
PolarFlow::readyRead()
{
    printd("start\n");
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    buffers.value(reply)->append(reply->readAll());
    //qDebug() << "readyRead - reply: " << reply;
    printd("end\n\n");
}

void
PolarFlow::readFileCompleted()
{
    printd("start\n");

    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    //qDebug() << "readFileCompleted - reply:" << reply;

    printd("reply:%s\n", buffers.value(reply)->toStdString().c_str());

    QString rename = replyName(reply);
    QByteArray* data = prepareResponse(buffers.value(reply), rename);
    //qDebug() << "readFileCompleted - data:" << data;

    notifyReadComplete(data, replyName(reply), tr("Completed."));

    printd("end\n\n");

}

void
PolarFlow::clearJsonObject(QJsonObject &object)
{
    printd("start\n");

    QStringList strList = object.keys();
    //qDebug() << "clearJsonObject - StringList" << strList;

    for(int i = 0; i < strList.size(); ++i)
    {
        object.remove(strList.at(i));
    }

    printd("JasonObject cleared \n end\n");
}

QJsonObject
//PolarFlow::createSampleStream(QJsonDocument &document, int loopnumber)
PolarFlow::createSampleStream(QJsonDocument &document, int actualSampleLoopNumber, int timeSamplesLoopNumber)
{
    printd("Start \n");

    //int timeSamplesLoopNumber = getSetting(GC_POLARFLOW_TIMESAMPLES_LOOPNUMBER, "").toInt();

    QJsonObject stream_Obj; // returned Object, sample-stream or time-stream

    //each sample-type looks like this - not each type has to be present
    //{
    //  "recording-rate": 5,                                 // integer(int32), in seconds, zero for rr-data
    //  "sample-type": "1",                                  // string(byte)
    //  "data": "0,100,102,97,97,101,103,106,96,89,88,87,98" // string, comma-separated list, zero indicates sensor is offline
    //}

    // new Array should look like Strava
    // https://developers.strava.com/docs/reference/#api-models-DistanceStream
    // [ {
    //  "type" : "distance",                                                        // integer
    //  "data" : [ 2.9, 5.8, 8.5, 11.7, 15, 19, 23.2, 28, 32.8, 38.1, 43.8, 49.5 ], // float
    //  "series_type" : "distance",                                                 // string
    //  "original_size" : 12,                                                       //
    //  "resolution" : "high"                                                       // string
    // } ]

    // new Stream with Polar samples
    // [ {
    //  "type" : "heartrate",                                                       // string
    //  "data" : [ 0,100,102,97,97,101,103,106,96,89,88,87,98 ],                    // converted to float
    //  "series_type" : "0",                                                        // integer
    //  "original_size" : 12,                                                       // integer
    //  "recording_rate" : "5"                                                      // integer
    // } ]

    if (actualSampleLoopNumber < timeSamplesLoopNumber) {
      // only renaming
      QJsonDocument sampletype_Doc = document;

      // Get sample-type ID: represents recorded sample
      QString sampletype_id_Str = QString::number(sampletype_Doc["sample-type"].toInt());
      printd("actualSample: %d - sampletype_id_String: %d\n", actualSampleLoopNumber, sampletype_id_Str.toInt());

      /*
      sample types
      ============
      samples are available in 11 different types.
      If the samples are not available for a particular activity it will be left out of the request results.
      https://www.polar.com/accesslink-api/?shell#exercise-sample-types
      sample-type name              unit
      string(byte)
       0   Heart rate               bpm
       1   Speed                    km/h
       2   Cadence 	                rpm
       3   Altitude                 m
       4   Power                    W
       5   Power pedaling index     %     // https://support.polar.com/de/support/pedalling_index
       6   Power left-right balance %
       7   Air pressure 	        hpa
       8   Running cadence 	        spm
       9   Temperaure 	            ºC
      10   Distance 	            m
      11   RR Interval 	            ms
      */

      QString samplename_Str;
      if (sampletype_id_Str == "0" ) {
          samplename_Str = "heartrate";
      }
      if (sampletype_id_Str == "1" ) {
          samplename_Str =  "speed";
      }
      if (sampletype_id_Str == "2" ) {
          samplename_Str = "cadence";
      }
      if (sampletype_id_Str == "3") {
          samplename_Str = "altitude";
      }
      if (sampletype_id_Str == "4") {
          samplename_Str = "power";
      }
      if (sampletype_id_Str == "5") {
          samplename_Str = "powerPedalingIndex";
      }
      if (sampletype_id_Str == "6") {
          samplename_Str = "powerLeftRightBalance";
      }
      if (sampletype_id_Str == "7") {
          samplename_Str =  "airPressure";
      }
      if (sampletype_id_Str == "8") {
          samplename_Str = "runningCadence";
      }
      if (sampletype_id_Str == "9") {
          samplename_Str = "temperature";
      }
      if (sampletype_id_Str == "10") {
          samplename_Str = "distance";
      }
      if (sampletype_id_Str == "11") {
          samplename_Str = "rrInterval";
      }
      printd("actualSampleLoopNumber: %d- Samples %s with id: %s found\n", actualSampleLoopNumber,  samplename_Str.toStdString().c_str(), sampletype_id_Str.toStdString().c_str());

      // Get recording-rate: each recording rate one data_value
      QString recording_rate_Str = QString::number(sampletype_Doc["recording-rate"].toInt());
      printd("actualSampleLoopNumber: %d - recording_rate_String: %s\n", actualSampleLoopNumber, recording_rate_Str.toStdString().c_str());

      // create array from list of comma separeted e.g. heart rate values
      QString datalist_Str = sampletype_Doc["data"].toString();

      // create a list of all the data we will work with
      QStringList datalist_Lst = datalist_Str.split(QLatin1Char(','));
      printd("actualSampleLoopNumber: %d - datalist_List no of elements: %d\n", actualSampleLoopNumber, datalist_Lst.size());
      //printd("ctualSampleLoopNumber: %d - datalist_String elements: %s\n", actualSampleLoopNumber, datalist_Str.toStdString().c_str());

      // create a json array and fill it. Change type from String to float number
      QJsonArray datalist_Ary;
                 foreach(QString const& data, datalist_Lst)
                 datalist_Ary.append(data.toFloat());
      //qDebug() << "createSampleStream - datalist_Ary:" << datalist_Ary;

      // save HR Sample size and recording rate for generating suitable timeStream
     /* if (sampletype_id_Str == "0") {
          setSetting(GC_POLARFLOW_SAMPLE_SIZE, datalist_Lst.size());
          setSetting(GC_POLARFLOW_SAMPLE_RECORDINGRATE, recording_rate_Str.toInt());
      } */

      // save first available Sample size and recording rate for generating suitable timeStream
      // rrIntervall (sample id 11) could not be used ?!?
      if (sampletype_id_Str != "11") {
          if (actualSampleLoopNumber == 0) {
              setSetting(GC_POLARFLOW_SAMPLE_SIZE, datalist_Lst.size());
              setSetting(GC_POLARFLOW_SAMPLE_RECORDINGRATE, recording_rate_Str.toInt());
              setSetting(GC_POLARFLOW_SAMPLE_ID, sampletype_id_Str);
              printd("actualSampleLoopNumber: %d\n", actualSampleLoopNumber);
          }
      }
      // rrIntervall (sample id 11)
      if (sampletype_id_Str == "11") {
          setSetting(GC_POLARFLOW_SAMPLE_SIZE, datalist_Lst.size());
          setSetting(GC_POLARFLOW_SAMPLE_RECORDINGRATE, 1);
          printd("actualSampleLoopNumber: %d\n", actualSampleLoopNumber);printd("actualSampleLoopNumber: %d\n", actualSampleLoopNumber);
      }
      printd("saved Sample Size: %d\n", getSetting(GC_POLARFLOW_SAMPLE_SIZE, "").toInt());
      printd("saved Sample Size Recording Rate: %d\n", getSetting(GC_POLARFLOW_SAMPLE_RECORDINGRATE, "").toInt());
      printd("saved from Sample id: %d\n", getSetting(GC_POLARFLOW_SAMPLE_ID, "").toInt());

      // Build a new Array like Strava Stream to use Syntax
      // https://www.programmerall.com/article/31271073246/
      QJsonDocument sampleStream_Doc;
      QJsonObject sampleStream_Obj;
           sampleStream_Obj.insert("type", samplename_Str);
           sampleStream_Obj.insert("data", datalist_Ary);
           sampleStream_Obj.insert("series_type", sampletype_id_Str.toInt());
           sampleStream_Obj.insert("original_size", datalist_Lst.size());
           sampleStream_Obj.insert("recording_rate", recording_rate_Str.toInt());

      //qDebug() << "createSampleStream - stream_Object:" << sampleStream_Obj;
      //return sampleStream_Obj;
      stream_Obj = sampleStream_Obj;
      clearJsonObject(sampleStream_Obj);

    } // close if loopnumber < timestream

    // create a time stream
    // time stream needs a size of SAMPLE_SIZE * SAMPLE_RECORDINGRATE
    if (actualSampleLoopNumber == timeSamplesLoopNumber) {

        // How many elements / samples are needed for timestream
        // they need to have SAMPLE_SIZE * RECORDING RATE
        // RR-Record could not be used, because RACORDING_RATE=0
        int recordingSize = getSetting(GC_POLARFLOW_SAMPLE_SIZE, "").toInt();
        int recordingRate = getSetting(GC_POLARFLOW_SAMPLE_RECORDINGRATE, "").toInt();
        //int loops = getSetting(GC_POLARFLOW_SAMPLE_SIZE, "").toInt();
        printd("actualSampleLoopNumber: %d - used Sample Size: %d\n", actualSampleLoopNumber, getSetting(GC_POLARFLOW_SAMPLE_SIZE, "").toInt());
        printd("actualSampleLoopNumber: %d - used Sample Size Recording Rate: %d\n", actualSampleLoopNumber, getSetting(GC_POLARFLOW_SAMPLE_RECORDINGRATE, "").toInt());
        printd("actualSampleLoopNumber: %d - used Sample id: %d\n", actualSampleLoopNumber, getSetting(GC_POLARFLOW_SAMPLE_ID, "").toInt());

        int timeSampleSize = (recordingSize * recordingRate);
        printd("actualSampleLoopNumber: %d - TimeSample Size: %d\n", actualSampleLoopNumber, timeSampleSize);

        // convert integer to float
        //https://www.codegrepper.com/code-examples/cpp/int+to+float+c%2B%2B
        float recording_rate = (float)recordingRate;

        QList<float> timeSampleData_Lst;
                     //timeSampleData_Lst.append(0.0); // first member is zero
        //for(int i=0; i<loops; i++) {
        for(int i=0; i<timeSampleSize; i++) {
            float localLoopNumber = (float)i;
            //float time = recording_rate*localLoopNumber;
            float time = localLoopNumber;
            timeSampleData_Lst.append(time);
        }
        printd("actualSampleLoopNumber: %d - timeSampleData_List no of elements: %d \n", actualSampleLoopNumber,  timeSampleData_Lst.size());
        //qDebug() << "createSampleStream - timeSampleData_List:" << timeSampleData_Lst;

        QJsonArray timeData_Ary;
                   //foreach(QList<int> data, timeSampleData_Lst)
                   for(int i=0; i<timeSampleData_Lst.size(); i++) {
                   timeData_Ary.append(timeSampleData_Lst.at(i));
                   }
        printd("actualSampleLoopNumber: %d - time_Array no of elements: %d \n", actualSampleLoopNumber,  timeData_Ary.size());
        //qDebug() << "createSampleStream - time_Array:" << timeData_Ary;

        QJsonObject timeStream_Obj;
                    timeStream_Obj.insert("type", "time");
                    timeStream_Obj.insert("data", timeData_Ary);
                    timeStream_Obj.insert("series_type", "");
                    timeStream_Obj.insert("original_size", timeData_Ary.size());
                    timeStream_Obj.insert("recording_rate", recordingRate);

        //qDebug() << "createSampleStream - time_Object:" << timeStream_Obj;
        //return timeStream_Obj;
        stream_Obj = timeStream_Obj;
        clearJsonObject(timeStream_Obj);
    }
    return stream_Obj;
}

void
//PolarFlow::addSamples(RideFile* ret, QString exerciseId, QString fileSuffix)
PolarFlow::addSamples(RideFile* ret, QString exerciseId)
{
    printd("start\n");
    // Get infos from requsted JSON
    // https://www.polar.com/accesslink-api/?shell#get-exercise-summary
    // https://www.polar.com/accesslink-api/?shell#get-available-samples
    // https://www.polar.com/accesslink-api/?shell#get-samples
    // https://www.polar.com/accesslink-api/?shell#exercise-sample-types
    //printd("addSamples\n");

    QString token = getSetting(GC_POLARFLOW_TOKEN, "").toString();

    QString transaction_id_Str = getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString();
    //printd("transaction_id_String: %s\n", transaction_id_Str.toStdString().c_str());

    //printd("fileSuffix: %s\n", fileSuffix.toStdString().c_str());

    // RequestURL: GET https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}/samples
    QString getAvailableSampleTypesUrls_Str = QString("%1/v3/users/%2/exercise-transactions/%3/exercises/%4/samples")
               .arg(getSetting(GC_POLARFLOW_URL, "https://www.polaraccesslink.com").toString())
               .arg(getSetting(GC_POLARFLOW_USER_ID, "").toString())
               .arg(getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString())
               .arg(exerciseId);
    printd("URL-List Sample Types: %s\n", getAvailableSampleTypesUrls_Str.toStdString().c_str());

    // request using the bearer token
    QNetworkRequest request(getAvailableSampleTypesUrls_Str);
                    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
                    request.setRawHeader("Accept", "application/json");

    // make request
    QNetworkReply *reply = nam->get(request);
    printd("response : %s\n", getAvailableSampleTypesUrls_Str.toStdString().c_str());
    //Example: Reply List of URLs
    //Example: https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}/samples/{type-id}

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // if successful, lets unpack
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    printd("fetch response: %d: %s \n", reply->error(), reply->errorString().toStdString().c_str());
    printd("HTTP response status code: %d \n", statusCode);

    // Example: https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}/samples/{type-id}
    // Returns a list of hyperlinks to available exercise sample types
    QByteArray getAvailableSampleTypesUrls_Ary = reply->readAll();
    printd("samples url array : %s \n", getAvailableSampleTypesUrls_Ary.toStdString().c_str());
    printd("amount of sample types: %d \n", getAvailableSampleTypesUrls_Ary.size()); // size is nonsense, or amount signs in string
    //printd("QByteArray of sample type urls from exercise: %s \n", getAvailableSampleTypesUrls_Ary.toStdString().c_str());

    /* Example Array
    {
      "samples": [
        "https://www.polaraccesslink.com/v3/users/12/exercise-transactions/34/exercises/56/samples/0",
        "https://www.polaraccesslink.com/v3/users/12/exercise-transactions/34/exercises/56/samples/3"
      ]
    }
    */

    // parse JSON payload
    QJsonParseError getAvailableSampleTypesUrls_Doc_parseError;
    QJsonDocument getAvailableSampleTypesUrls_Doc = QJsonDocument::fromJson(getAvailableSampleTypesUrls_Ary, &getAvailableSampleTypesUrls_Doc_parseError);
    //qDebug() << "addSamples - Document Error Number: " << getAvailableSampleTypesUrls_Doc_parseError.error << " - Document Error String: " << getAvailableSampleTypesUrls_Doc_parseError.errorString();
    //qDebug() << "addSamples - Document list of urls: " << getAvailableSampleTypesUrls_Doc;

    // https://www.polar.com/accesslink-api/?shell#exercise-sample-types
    // List of urls to available samples -> array

    // QJsonArrays declared before if-statments, usage see down below
    // https://stackoverflow.com/questions/13337529/using-variables-outside-of-an-if-statement/13337565
    QJsonArray combinedStream_Ary; // fetches all provided streams and adds time stream
    QJsonObject sampleStream_Obj; // filled with all available samples besides not provided  and added to QJsonArray streams
    QJsonObject timeStream_Obj; // timeStream_Obj will be created seperatly, but gets number of datapoints from heartrate sample, with is always provided and added to QJsonArray streams
    int numberOfAvailableSampleTyps = 0; // amount of available Sample Typs, default is set to zero
    int timeSamplesLoopNumber = 0; // is set in heartrate stream, default is set to zero

    if (getAvailableSampleTypesUrls_Doc_parseError.error == QJsonParseError::NoError) {

        QJsonObject getAvailableSampleTypesUrls_Obj = getAvailableSampleTypesUrls_Doc.object();
        //qDebug() << "addSamples - Samples Exercise - Object url sample exercises: " << getAvailableSampleTypesUrls_Obj;

        QJsonArray sampleTypesfromExercise_Ary = getAvailableSampleTypesUrls_Obj["samples"].toArray();
        //qDebug() << "addSamples - Samples Exercise - Array of Sample Urls: " << sampleTypesfromExercise_Ary;

        // How many Sample Types are available?
        numberOfAvailableSampleTyps = sampleTypesfromExercise_Ary.size();
        printd("sampleTypesfromExercise_Ary - available Amount: %d\n", numberOfAvailableSampleTyps);

        // set timeStream_Obj Loop number
        // time stream is created after all other streams, see below
        //setSetting(GC_POLARFLOW_TIMESAMPLES_LOOPNUMBER, numberOfAvailableSampleTyps + 1 );

        // create time stream at the end and move it to first Position in Array
        //timeSamplesLoopNumber = getSetting(GC_POLARFLOW_TIMESAMPLES_LOOPNUMBER, "").toInt();
        timeSamplesLoopNumber = numberOfAvailableSampleTyps + 1;
        printd("timeStream_Obj - loopnumber: %d\n", timeSamplesLoopNumber);

        // getting sample type
        // https://www.polar.com/accesslink-api/?shell#exercise-sample-types
        // seperate sample urls and get samples data and additional information
        QString sampleUrl_Str;

        // loop number of of actual aquired sample: actualSampleLoopNumber
        for(int actualSampleLoopNumber = 0; actualSampleLoopNumber < numberOfAvailableSampleTyps; actualSampleLoopNumber++) {
            int loopCounterSampleTypes = actualSampleLoopNumber + 1; // amount of Samples counts from one
            sampleUrl_Str = sampleTypesfromExercise_Ary[actualSampleLoopNumber].toString();
            printd("actualSampleLoopNumber: %d / %d - sampleUrl_String - Url to sample-series: %s \n",loopCounterSampleTypes, numberOfAvailableSampleTyps, sampleUrl_Str.toStdString().c_str());

            // Sample URL is provided in
            // Example: GET https://www.polaraccesslink.com//v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}/samples/{type-id}

            // request using the bearer token
            QNetworkRequest request(sampleUrl_Str);
                            request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
                            request.setRawHeader("Accept", "application/json");

            // make request
            QNetworkReply *reply = nam->get(request);
            printd("response : %s\n", getAvailableSampleTypesUrls_Str.toStdString().c_str());

            // blocking request
            QEventLoop loop;
            connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();

            // if successful, lets unpack
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            printd("fetch response: %d: %s\n", reply->error(), reply->errorString().toStdString().c_str());
            printd("loop actualSampleLoopNumber: %d / %d \n",loopCounterSampleTypes, numberOfAvailableSampleTyps);
            printd("HTTP response status code: %d \n", statusCode);

            // no bracktes []
            QByteArray sampletype_Ary = reply->readAll();
            //printd("loop actualSampleLoopNumber: %d / %d - sampletype_Array: %s \n",loopCounterSampleTypes, numberOfAvailableSampleTyps, sampletype_Ary.toStdString().c_str());

            // parse JSON payload
            QJsonParseError sampletype_Doc_parseError;
            QJsonDocument sampletype_Doc;
                           sampletype_Doc = QJsonDocument::fromJson(sampletype_Ary, &sampletype_Doc_parseError);
            //qDebug() << "addSamples - loop actualSampleLoopNumber:"<< loopCounterSampleTypes <<"/"<< numberOfAvailableSampleTyps << " - sampletype_Document - Error Number:" << sampletype_Doc_parseError.error << " - sampletype_Document - Error String: " << sampletype_Doc_parseError.errorString();
            //qDebug() << "addSamples - loop actualSampleLoopNumber:"<< loopCounterSampleTypes <<"/"<< numberOfAvailableSampleTyps << " - sampletype_Document:" << sampletype_Doc;
            printd("sampletype_Doc: %s \n", sampletype_Doc.toJson().toStdString().c_str());
            printd("sampletype_Doc - Error Number: %d - Error String: %s \n", sampletype_Doc_parseError.error, sampletype_Doc_parseError.errorString().toStdString().c_str());

            if (sampletype_Doc_parseError.error == QJsonParseError::NoError) {
                // we first collect all provided samples
                sampleStream_Obj = createSampleStream(sampletype_Doc, actualSampleLoopNumber, timeSamplesLoopNumber);
                //qDebug() << "addSamples - loop loopCounterSampleTypes:" << loopCounterSampleTypes <<"/"<< numberOfAvailableSampleTyps <<" - sampleStream_Obj:" << sampleStream_Obj;
                //printd("created Streams Object without sampleStream: %d / loopnumber: %d\n", sampleStream_Obj.size(), actualSampleLoopNumber);
                printd("created Streams Object without sampleStream: %d / loopnumber: %d\n", numberOfAvailableSampleTyps, actualSampleLoopNumber);

                // and save them into one Array
                combinedStream_Ary.append(sampleStream_Obj);
                //qDebug() << "addSamples - loop loopCounterSampleTypes:" << loopCounterSampleTypes <<"/"<< numberOfAvailableSampleTyps <<" - streams:" << combinedStream_Ary;
                //printd("appending to combined Stream (size) without timeStream: %d / loopnumber: %d\n", combinedStream_Ary.size(), actualSampleLoopNumber);
                printd("appending to combined Stream (size) without timeStream: %d / loopnumber: %d\n", numberOfAvailableSampleTyps, actualSampleLoopNumber);
            } // close doc parser error

        } // close get available Samples
        printd("combined Stream (size) without timeStream %d \n", combinedStream_Ary.size());

        // for correct importing we need a time stream
        // define an empty Json Document for not-provided time stream
        QJsonDocument emptyForTimestream_Doc;
        // fill QJsonObject with time data -> RideFile::secs
        timeStream_Obj = createSampleStream(emptyForTimestream_Doc, timeSamplesLoopNumber, timeSamplesLoopNumber);
        // and prepend it as first stream in QJsonArray streams
        combinedStream_Ary.prepend(timeStream_Obj);
        //printd("streams now with time: %s", combinedStream_Ary);
        printd("combined Stream (size) now with timeStream: %d / loopnumber: %d\n", combinedStream_Ary.size(), timeSamplesLoopNumber);

        // after having created a STRAVA like Stream we work with it
        // mapping names used in the Polar Flow json response
        // to the series names we use in GC
        static struct {
               RideFile::seriestype type;
               const char *polarsampletypename; // polarsampletypename
               double factor; // to convert from PolarFlow units to GC units
        } seriesnames[] = {
               // seriestype,         polarsampletypename,     conversion factor -> RideFile.h/.cpp Line 350ff
               { RideFile::secs,      "time",                  1.0f   }, // s (not provided by polar, needs to be created seperatly)
               { RideFile::hr,        "heartrate",             1.0f   }, // bpm
               { RideFile::kph,       "speed",                 1.0f   }, // km/h
               { RideFile::cad,       "cadence",               1.0f   }, // rpm
               { RideFile::alt,       "altitude",              1.0f   }, // m
               { RideFile::watts,     "power",                 1.0f   }, // W
               //{ RideFile::none,      "powerPedalingIndex",    0.0f   }, // %
               { RideFile::lrbalance, "powerLeftRightBalance", 1.0f   }, // %
               //{ RideFile::none,      "airPressure",           0.0f   }, // hpa
               { RideFile::rcad,       "runningCadence",       1.0f   }, // spm // conversation factor "2" same Value as Strava
               { RideFile::temp,       "temperature",          1.0f   }, // °C
               { RideFile::km,         "distance",             0.001f }, // m
               //{ RideFile::none,       "rrInterval",           0.001f }, // ms
               { RideFile::none,       "",                     0.0f   } // must be last in list, stop condition for loop
        };

        // data to combine with ride
        // define QList<polarSamples_Class> syntax
        class polarSamples_Class { // polarSamples_Class = strava_stream
        public:
            double factor; // for converting
            RideFile::seriestype type;
            QJsonArray samples;
        };

        // seriestypes
        //forech (string seriesname; )
        //for (int k=0; k < 20; k++) {
        //qDebug() << "addSamples" << k << "- GoldenCheetah:" << seriesnames[k].type << "- PolarFlow:" << seriesnames[k].polarsampletypename << "- conversationfactor:" << seriesnames[k].factor;
        //printd("addSamples %d - GoldenCheetah:%s - PolarFlow: %s - conversationfactor: %d\n", k, seriesnames[k].type, seriesnames[k].polarsampletypename, seriesnames[k].factor);
        //} // close for seriestype

        // create a list of all the data we will work with
        QList<polarSamples_Class> polarSampleClassData_QLst;
        // Example: 1.0f; RideFile::hr; 0,100,102,97,97,101,103,106,96,89,88,87
        //qDebug() << "addSamples - Samples - loop i:" << loopCounterSampleTypes <<"- data: " << polarSampleClassData_QLst;

        // examine the returned object and extract sample data
        for(int combinedStreamLoop=0; combinedStreamLoop<combinedStream_Ary.size(); combinedStreamLoop++) {
            int jPlusOne = combinedStreamLoop + 1; // amount of Points counts from one
            QJsonObject combinedStream_Obj = combinedStream_Ary.at(combinedStreamLoop).toObject();
            QString type = combinedStream_Obj["type"].toString();
            printd("loop combinedStreamLoop: %d / %d - exercise sample type type: %s\n", jPlusOne, combinedStream_Ary.size(), type.toStdString().c_str());

            for(int seriesLoop=0; seriesnames[seriesLoop].type != RideFile::none; seriesLoop++) {
                QString name = seriesnames[seriesLoop].polarsampletypename;
                printd("loopcombinedStreamLoop: %d - loop seriesLoop: %d - series name: %s\n", combinedStreamLoop, seriesLoop, name.toStdString().c_str());
                polarSamples_Class add;

                // contained?
                if (type == name) {
                    add.factor = seriesnames[seriesLoop].factor;
                    add.type = seriesnames[seriesLoop].type;
                    add.samples = combinedStream_Obj["data"].toArray();
                    //qDebug() << "addSamples - loop k: " << k << "- streams factor:" << seriesnames[k].factor;
                    //qDebug() << "addSamples - loop k: " << k << "- streams type:" << seriesnames[k].type;
                    //qDebug() << "addSamples - loop k: " << k << "- streams samples size:" << combinedStream_Obj["data"].toArray().size();
                    //qDebug() << "addSamples - loop k: " << k << "- streams samples:" << combinedStream_Obj["data"].toArray();

                    polarSampleClassData_QLst << add;
                    //qDebug() << "addSamples - loop k: " << k << "- polarSampleClassData_QLst : " << polarSampleClassData_QLst;
                    break; // breaks to j loop
                  } // close type
            } // close stream data - loop seriesLoop
        printd("combining samples  endend\n");
        } // close streams size - loop combinedStreamLoop

        // choose method updating imported file or append for fresh files
        printd("use imported or update samples \n");
        // fresh file method needs to be created
        bool isImported = false;
        //printd("isImported Starting Value %\n", isImported.toString()); //StdString().c_Str());
        // compare amount of datapoints from imported file with the amount available samplepoints
        bool numberOfPointsAreEven = false;

        // How many datapoints and samplepoints are there? Is the Size even?
        int dataPointsImport = ret->dataPoints().count();
            if (dataPointsImport != 0) {
                dataPointsImport = 0;
            }
        printd("dataPointsImport %d:\n", dataPointsImport);

        int samplePoints = getSetting(GC_POLARFLOW_SAMPLE_SIZE, "").toInt();
        printd("amount of samplePoints: %d:\n", samplePoints);

        polarSamples_Class polarSamples_QLst;
        int polarSamples_QLstPoints;
            polarSamples_QLstPoints = polarSamples_QLst.samples.count();

        printd("polarSamples_QLstPoints (Zero at this point): %d\n", polarSamples_QLstPoints);

        // now check if they are even, have they the same size ?
        if (dataPointsImport == samplePoints) {
            numberOfPointsAreEven = true;
        }
        //qDebug() << "addSamples - numberOfPointsAreEven:" << numberOfPointsAreEven;

        // Updating imported Values with Values from Samples
        if (ret->isDataPresent(RideFile::hr) && numberOfPointsAreEven == true ) {
            // counting through datapoints / samplepoints
            int index = 0; // loop variable
            int indexdelta = 1; // decreasing by one
            int indexEnd = 0; // amount of datapoints in ONE polarSamples_QLst
            int indexPlusOne = 0; // one more than index only for Debug - counting from one instead of zero

            // counting through samples data
            int countSamples = 0; // loop variable
            int countSamplesdelta = 1; // decreasing by one
            int countSamplesEnd = 0; // start value for the end condition, later updated
            int countSamplesPlusOne = 0; // one more than countSamples only for Debug - counting from one instead of zero

            int polarSampleType = 1;
            int polarSampleTypeDelta = 1; // decreasing by one

            // set some other start values
            int streamSize = 0; // start value for streamSize
            double valueStart = RideFile::NIL;
            double value = RideFile::NIL;

            printd("countSamplesEnd: %d\n", countSamplesEnd);
            printd("indexEnd: %d\n", indexEnd);

            //Create empty Lists for collecting values from factorized sample data
            QList<double> polarSampleTypeSecs;
            QList<double> polarSampleTypeLat;
            QList<double> polarSampleTypeLon;
            QList<double> polarSampleTypeHr;
            QList<double> polarSampleTypeKph;
            QList<double> polarSampleTypeCad;
            QList<double> polarSampleTypeAlt;
            QList<double> polarSampleTypeWatts;
            QList<double> polarSampleTypePowerpedalingindex;
            QList<double> polarSampleTypeLRBalance;
            QList<double> polarSampleTypeRcad;
            QList<double> polarSampleTypeTemp;
            QList<double> polarSampleTypeKm;
            QList<double> polarSampleTypeRRInterval;

            // define stop conditions
            streamSize = combinedStream_Ary.size();
            //if (ret->isDataPresent(RideFile::lat) && ret->isDataPresent(RideFile::lon)) {
            //streamSize = streamSize + 2;
            //}
            /*if (polarSamples_QLst.type == "rrIntervall") {
            streamSize = streamSize - 1;
            }*/

            //indexEnd = roundf((samplePoints / streamSize) + 0.5 );
            indexEnd = dataPointsImport; // How much datapoints / samplepoints are in ONE stream ?

            //countSamplesEnd = samplePoints;
            countSamplesEnd = roundf((samplePoints * streamSize)); // + 0.5); // How many datapoints / samplepoints are in all streams ?
            //countSamplesEnd = polarSamples_QLst.samples.count();
            //countSamplesEnd = combinedStream_Ary.size();

            // run through
            for (index; index < indexEnd; index = index + indexdelta){
               indexPlusOne = index + 1; // amount of datapoints / samplepoints counts from one
               printd("index: %d / %d \n", indexPlusOne, indexEnd);

               // move through streams if they're waiting for this point
               foreach(polarSamples_QLst, polarSampleClassData_QLst) {

                 if (polarSamples_QLst.type == RideFile::secs) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;

                    //if (polarSamples_QLst.type == RideFile::secs) {
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::secs)) {
                             value = ret->getPointValue(index, RideFile::secs);
                            //value = value;
                            printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::secs))) {
                             value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                             printd("sample choosen\n");
                         }
                         polarSampleTypeSecs.append(value);
                         //polarSampleTypeSecs.replace(index, value);
                         //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- Secs countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd  << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 if (polarSamples_QLst.type == RideFile::lat) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                        value = valueStart;
                        if (ret->isDataPresent(RideFile::lat)) {
                            value = ret->getPointValue(index, RideFile::lat);
                            printd("imported choosen\n");
                        }
                        if (!(ret->isDataPresent(RideFile::lat))) {
                            value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                            printd("sample choosen\n");
                        }
                        polarSampleTypeLat.append(value);
                        //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- Lat countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                    }
                 }

                 if (polarSamples_QLst.type == RideFile::lon) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::lon)) {
                             value = ret->getPointValue(index, RideFile::lon);
                             printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::lon))) {
                             value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                             printd("sample choosen\n");
                         }
                         polarSampleTypeLon.append(value);
                         //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- Lon countSamplesPlusOne:"<< countSamplesPlusOne << "/" << countSamplesEnd  << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 if (polarSamples_QLst.type == RideFile::hr) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::hr)) {
                             value = ret->getPointValue(index, RideFile::hr);
                             printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::hr))) {
                             value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                             printd("sample choosen\n");
                         }
                         polarSampleTypeHr.append(value);
                         //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- HR countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd  << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 if (polarSamples_QLst.type == RideFile::kph) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::kph)) {
                             value = ret->getPointValue(index, RideFile::kph);
                             printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::kph))) {
                             value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                             printd("sample choosen\n");
                         }
                         polarSampleTypeKph.append(value);
                         //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- Kph countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd  << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 if (polarSamples_QLst.type == RideFile::cad) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::cad)) {
                             value = ret->getPointValue(index, RideFile::cad);
                             printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::cad))) {
                             value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                             printd("sample choosen\n");
                         }
                         polarSampleTypeCad.append(value);
                         //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- Cad countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd  << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 if (polarSamples_QLst.type == RideFile::alt) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::alt)) {
                             value = ret->getPointValue(index, RideFile::alt);
                             printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::alt))) {
                               value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                               printd("sample choosen\n");
                         }
                         polarSampleTypeAlt.append(value);
                         //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- Alt countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd  << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 if (polarSamples_QLst.type == RideFile::watts) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::watts)) {
                             value = ret->getPointValue(index, RideFile::watts);
                             printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::watts))) {
                             value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                             printd("sample choosen\n");
                         }
                         polarSampleTypeWatts.append(value);
                        //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- Watts countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd  << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 if (polarSamples_QLst.type == RideFile::rcad) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::rcad)) {
                             value = ret->getPointValue(index, RideFile::rcad);
                             printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::rcad))) {
                             value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                             printd("sample choosen\n");
                         }
                         polarSampleTypeRcad.append(value);
                         //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- Rcad countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd  << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 if (polarSamples_QLst.type == RideFile::lrbalance) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::lrbalance)) {
                             value = ret->getPointValue(index, RideFile::lrbalance);
                             printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::lrbalance))) {
                             value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                             printd("sample choosen\n");
                         }
                         polarSampleTypeLRBalance.append(value);
                         //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- lrBalance countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd  << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 if (polarSamples_QLst.type == RideFile::temp) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::temp)) {
                             value = ret->getPointValue(index, RideFile::temp);
                             printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::temp))) {
                             value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                             printd("sample choosen\n");
                         }
                         polarSampleTypeTemp.append(value);
                         //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- Temp countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd  << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 if (polarSamples_QLst.type == RideFile::km) {
                     if (countSamples < countSamplesEnd) {
                         countSamplesPlusOne = countSamples + 1;
                         value = valueStart;
                         if (ret->isDataPresent(RideFile::km)) {
                             value = ret->getPointValue(index, RideFile::km);
                             printd("imported choosen\n");
                         }
                         if (!(ret->isDataPresent(RideFile::km))) {
                             value = polarSamples_QLst.factor * polarSamples_QLst.samples.at(index).toDouble();
                             printd("sample choosen\n");
                         }
                         polarSampleTypeKm.append(value);
                         //qDebug() << "addSamples - index:"<< indexPlusOne << "/" << indexEnd << "- Km countSamples:"<< countSamplesPlusOne << "/" << countSamplesEnd << "- add polarSamples_QLst.type:" << polarSamples_QLst.type << ", value:" << value;
                     }
                 }

                 //value = valueStart;
                 printd("index: %d / %d - countSamples - next round\n", indexPlusOne, indexEnd);
                 //}
                 // loopnumber decrease
                 countSamples = countSamples + countSamplesdelta;
                 //value = valueStart;

                if (countSamples == countSamplesEnd) {
                    //countSamples = 0;
                    printd("index: %d / %d - countedSamples: %d / %d \n", indexPlusOne, indexEnd, countSamplesPlusOne, countSamplesEnd);
                    //qDebug() << "addSamples - QList - Secs data:" <<polarSampleTypeSecs;
                    //qDebug() << "addSamples - QList - Hr data:" <<polarSampleTypeHr;
                    //qDebug() << "addSamples - QList - Lat data:" <<polarSampleTypeLat;
                    printd("QList - Secs size: %d \n", polarSampleTypeSecs.size());
                    printd("QList - Hr size: %d \n", polarSampleTypeHr.size());
                    printd("QList - Lat size: %d \n", polarSampleTypeLat.size());
                    printd("QList - Lon size: %d \n", polarSampleTypeLon.size());
                    printd("QList - Kph size: %d \n", polarSampleTypeKph.size());
                    printd("QList - Cad size: %d \n", polarSampleTypeCad.size());
                    printd("QList - Alt size: %d \n", polarSampleTypeAlt.size());
                    printd("QList - Watts size: %d \n", polarSampleTypeWatts.size());
                    printd("QList - Powerpedalingindex size: %d \n", polarSampleTypePowerpedalingindex.size());
                    printd("QList - LRBalance size: %d \n", polarSampleTypeLRBalance.size());
                    printd("QList - Rcad size: %d \n", polarSampleTypeRcad.size());
                    printd("QList - Temp size: %d \n", polarSampleTypeTemp.size());
                    printd("QList - Km size: %d \n", polarSampleTypeKm.size());
                } // close if countSamples counter

               } // close foreach-loop
            } // if index loop - filling QLists<polarSampleType*>

            // Sorting in Update and afterwards remove polarSamples_QLst.type
            printd("before appendOrUpdate countSamples: %d \n", countSamples);

            // update or append Points in RideFile with Streams data
            index = 0;
            for (index; index < indexEnd; index = index + indexdelta){
                 indexPlusOne = index + 1; // amount of Points counts from one
                 printd("index: %d / %d \n", indexPlusOne, indexEnd);
/*
                //for (index; index < dataPointsImport; index = index + indexdelta){
                ret->appendOrUpdatePoint(
                         // double secs,              double cad,              double hr,              double km,
                            if (polarSampleTypeSecs.size()>0) polarSampleTypeSecs.at(index),
                            polarSampleTypeCad.at(index),
                            polarSampleTypeHr.at(index),
                            polarSampleTypeKm.at(index),
                         // double kph,              double nm,     double watts,              double alt,
                            polarSampleTypeKph.at(index), RideFile::NIL, polarSampleTypeWatts.at(index), polarSampleTypeAlt.at(index),
                         // double lon,              double lat,              double headwind,
                            polarSampleTypeLon.at(index), polarSampleTypeLat.at(index), RideFile::NIL,
                         // double slope,  double temp,              double lrbalance,
                            RideFile::NIL, polarSampleTypeTemp.at(index), polarSampleTypeLRBalance.at(index),
                         // double lte,    double rte,    double lps,    double rps,
                            RideFile::NIL, RideFile::NIL, RideFile::NIL, RideFile::NIL,
                         // double lpco,   double rpco,
                            RideFile::NIL, RideFile::NIL,
                         // double lppb,   double rppb,   double lppe,   double rppe,
                            RideFile::NIL, RideFile::NIL, RideFile::NIL, RideFile::NIL,
                         // double lpppb,  double rpppb,  double lpppe,  double rpppe,
                            RideFile::NIL, RideFile::NIL, RideFile::NIL, RideFile::NIL,
                         // double smo2,   double thb,
                            RideFile::NIL, RideFile::NIL,
                         // double rvert,  double rcad,            double rcontact, double tcore,
                            RideFile::NIL, polarSampleTypeRcad.at(index), RideFile::NIL, RideFile::NIL,
                         // int interval, bool forceAppend
                            0, false);
*/
                //qDebug() << "addSamples - appendOrUpdate"<< indexPlusOne << "/" << indexEnd << "- ret:" << ret;
                printd("appendOrUpdate - End \n");
            } // close index counter loop
            //value = valueStart;

            isImported = true;

        } // close if - isDataPresent

         //qDebug() << "addSamples - isImported:" << isImported;

        // Add Sample-data to RideFile
        if (isImported == false) {
            bool end = false;
            int index=0;
            printd("do-while data List size / available Streams: %d \n", polarSampleClassData_QLst.size());

            do {
                RideFilePoint add;

                // move through streams if they're waiting for this point
                foreach(polarSamples_Class sampleStream_QLst, polarSampleClassData_QLst) {

                  // if this stream still has List of data to consume
                  if (index < sampleStream_QLst.samples.count()) {
                      double lat;
                      double lon;
                      double value;
/*
                    if (sampleStream_QLst.type == RideFile::lat) {

                        lat = sampleStream_QLst.factor * sampleStream_QLst.samples.at(index).toArray().at(0).toDouble();
                        lon = sampleStream_QLst.factor * sampleStream_QLst.samples.at(index).toArray().at(1).toDouble();

                        // this is one for us, update and move on
                        add.setValue(RideFile::lat, lat);
                        add.setValue(RideFile::lon, lon);

                    } else {
*/
                        // hr, distance et al
                        value = sampleStream_QLst.factor * sampleStream_QLst.samples.at(index).toDouble();

                        // this is one for us, update and move on
                        add.setValue(sampleStream_QLst.type, value);
//                      }
                    //qDebug() << "addSamples - do-while"<< index << "/" << sampleStream_QLst.samples.count()  << "- else add sampleStream_QLst.type, value:" << sampleStream_QLst.type << ", " << value;
                  } else
                      end = true;
                }

                ret->appendPoint(add); // example: ret->appendPoint(RideFile::hr, 182)
                //printd("do-while streamSize %d \n", sampleStream_QLst.size());
                //qDebug() << "addSamples - do-while" << index << "- ret:" << ret;
                index++;

            } while (!end);
            printd("do-while - number of samples per stream: %d \n", index); // << "- ret:" << ret;

        } // close if isImported

  } // close get exercise Summary
  printd("all samples added - end \n");
}

void
PolarFlow::fixLapSwim(RideFile* ret, QJsonArray laps)
{
    printd("fixLapSwim - start \n");
/*
    // Lap Swim & SmartRecording enabled: use distance from laps to fix samples
    if (ret->isSwim() && ret->isDataPresent(RideFile::km) && !ret->isDataPresent(RideFile::lat)) {

        int lastSecs = 0;
        double lastDist = 0.0;
        for (int i=0; i<laps.count() && i<ret->intervals().count(); i++) {
            QJsonObject lap = laps[i].toObject();
            double start = ret->intervals()[i]->start;
            double end = ret->intervals()[i]->stop;
            int startIndex = ret->timeIndex(start) ? ret->timeIndex(start)-1 : 0;
            double km = ret->getPointValue(startIndex, RideFile::km);

            // fill gaps and fix distance before the lap
            double deltaDist = (start>lastSecs && km>lastDist+0.001) ? (km-lastDist)/(start-lastSecs) : 0;
            double kph = 3600.0*deltaDist;
            for (int secs=lastSecs; secs<start; secs++) {
                ret->appendOrUpdatePoint(secs, 0.0, 0.0, lastDist+deltaDist,
                        kph, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        RideFile::NA, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0, false);
                // force update, even when secs or kph are 0
                if (secs == 0 || kph == 0.0)
                    ret->setPointValue(secs, RideFile::kph, kph);
                lastDist += deltaDist;
            }

            // fill gaps and fix distance inside the lap
            deltaDist = 0.001*lap["distance"].toDouble()/(end-start);
            kph = 3600.0*deltaDist;
            for (int secs=start; secs<end; secs++) {
                ret->appendOrUpdatePoint(secs, 0.0, 0.0, lastDist+deltaDist,
                        kph, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        RideFile::NA, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0.0, 0.0,
                        0.0, 0.0, 0.0, 0.0,
                        0, false);
                // force update, even when secs or kph are 0
                if (secs == 0 || kph == 0.0)
                    ret->setPointValue(secs, RideFile::kph, kph);
                lastDist += deltaDist;
            }
            lastSecs = end;
        }
    }
*/
    printd("fixLapSwim - end \n");
}

void
PolarFlow::fixSmartRecording(RideFile* ret)
{
    printd("fixSmartRecording - start \n");
    QVariant isGarminSmartRecording = appsettings->value(NULL, GC_GARMIN_SMARTRECORD,Qt::Checked);
    // do nothing if disabled
    if (isGarminSmartRecording.toInt() == 0) return;
        //qDebug() << "fixSmartRecording - isGarminSmartRecording" << isGarminSmartRecording;


    QVariant GarminHWM = appsettings->value(NULL, GC_GARMIN_HWMARK);
    //qDebug() << "fixSmartRecording - GarminHWM" << GarminHWM;
    // default to 30 seconds
    if (GarminHWM.isNull() || GarminHWM.toInt() == 0) GarminHWM.setValue(30);
    // minimum 90 seconds for swims
    if (ret->isSwim() && GarminHWM.toInt()<90) GarminHWM.setValue(90);

    // The following fragment was adapted from FixGaps

    // If there are less than 2 dataPoints then there
    // is no way of post processing anyway (e.g. manual workouts)
    //qDebug() << "fixSmartRecording - ret" << ret;
    if (ret->dataPoints().count() < 2) return;

    // OK, so there are probably some gaps, lets post process them
    RideFilePoint *last = NULL;
    int dropouts = 0;
    double dropouttime = 0.0;
    printd("fixSmartRecording - dropoutime: %f \n", dropouttime);

    for (int position = 0; position < ret->dataPoints().count(); position++) {
        RideFilePoint *point = ret->dataPoints()[position];
        //printd("fixSmartRecording - loop position: %i \n", position);

        if (NULL != last) {
            double gap = point->secs - last->secs - ret->recIntSecs();

            // if we have gps and we moved, then this isn't a stop
            bool stationary = ((last->lat || last->lon) && (point->lat || point->lon)) // gps is present
                         && last->lat == point->lat && last->lon == point->lon;

            // moved for less than GarminHWM seconds ... interpolate
            if (!stationary && gap >= 1 && gap <= GarminHWM.toInt()) {

                // what's needed?
                dropouts++;
                dropouttime += gap;

                int count = gap/ret->recIntSecs();
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
                double lrbalancedelta = (point->lrbalance - last->lrbalance) / (double) count;
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
                    printd("fixSmartRecording - loop i: %i \n", i);
                    RideFilePoint *add = new RideFilePoint(last->secs+((i+1)*ret->recIntSecs()),
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
                                                           last->lrbalance + ((i+1)*lrbalancedelta),
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

                    ret->insertPoint(position++, add);
                }

            }
        }
        last = point;
    }
}

QString
PolarFlow::saveFile(QByteArray &array, QString exerciseId, QString filetype, QDateTime starttime, QString detailedSportInfo_Str)
{
    printd("save File - start \n");
    printd("save File - used exerciseId: %s \n", exerciseId.toStdString().c_str());
    printd("save File - used filetype / fileSuffix: %s \n", filetype.toStdString().c_str());


    QString starttime_format;
            starttime_format = "yyyy_MM_dd_HH_mm_ss";
    QString starttime_str;
            starttime_str = starttime.toString(starttime_format);

    QString filename;
            filename = context->athlete->home->tmpActivities().canonicalPath() + "/" + starttime_str + "_" + detailedSportInfo_Str + "_"+ exerciseId  + "." + filetype;
    printd("save File - filename - %s \n", filename.toStdString().c_str());

    //https://stackoverflow.com/questions/12988131/qt4-write-qbytearray-to-file-with-filename
    QSaveFile tempFile(filename);
              tempFile.open(QIODevice::WriteOnly);
              tempFile.write(array);
              tempFile.commit();

    printd("save File - returnd filename %s \n", filename.toStdString().c_str());
    printd( "save File - end \n");
    return filename;
}

QString
PolarFlow::getFile(QString exerciseId, QString filetype, QDateTime starttime, QString detailedSportInfo_Str)
{
    // https://www.polar.com/accesslink-api/?shell#get-gpx
    // returns training session in GPX (GPS Exchange format)
    // https://www.polar.com/accesslink-api/?shell#get-tcx
    // returns gzipped TCX
    // https://www.polar.com/accesslink-api/?shell#get-fit-beta
    // returns FIT file

    // Status Codes for available Data
    int dataAvailable = 200;

    printd("get File - GPX / TCX / Fit beta - start \n");

    printd("PolarFlow::getFile \n");

    printd("get File - ExerciseID: %s \n", exerciseId.toStdString().c_str());

    QString token = getSetting(GC_POLARFLOW_TOKEN, "").toString();

    QString transaction_id_Str = getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString();
    printd("get File - transaction_id_String: %s \n", transaction_id_Str.toStdString().c_str());

 // RequestURL: GET https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}/gpx
 // RequestURL: GET https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}/tcx
 // RequestURL: GET https://www.polaraccesslink.com/v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}/fit

    QString filename;

 // first filetype GPX
    if (filetype == "gpx") {

        QString get_GPX_Str;
                get_GPX_Str = QString("%1/v3/users/%2/exercise-transactions/%3/exercises/%4/gpx")
                   .arg(getSetting(GC_POLARFLOW_URL, "https://www.polaraccesslink.com").toString())
                   .arg(getSetting(GC_POLARFLOW_USER_ID, "").toString())
                   .arg(getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString())
                   .arg(exerciseId);
        printd("get File - GPX - URL: %s \n", get_GPX_Str.toStdString().c_str());

        // request using the bearer token, application/gpx+xml
        QNetworkRequest requestGPX(get_GPX_Str);
                        requestGPX.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
                        requestGPX.setRawHeader("Accept", "application/gpx+xml");

        // make request
        QNetworkReply *replyGPX = nam->get(requestGPX);
        printd("response : %s\n", get_GPX_Str.toStdString().c_str());
        // Example: Reply List of URLs /v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}/gpx

        // blocking request
        QEventLoop loopGPX;
        connect(replyGPX, SIGNAL(finished()), &loopGPX, SLOT(quit()));
        loopGPX.exec();

        // if successful, lets unpack
        int statusCodeGPX = replyGPX->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        printd("fetch response: %d: %s\n", replyGPX->error(), replyGPX->errorString().toStdString().c_str());
        printd("get File - GPX - Samples - HTTP response status code: %d \n", statusCodeGPX);


        QByteArray get_GPX_Ary = replyGPX->readAll();
        //qDebug() << "get File - GPX - reply:" << reply;
        //printd("get File - GPX - QByteArray: %s \n", get_GPX_Ary.toStdString().c_str());

        if (statusCodeGPX == dataAvailable) {
            QString filenameGPX;
                    filenameGPX = saveFile(get_GPX_Ary, exerciseId, filetype, starttime, detailedSportInfo_Str);
            printd("get File - GPX - filename: %s \n", filenameGPX.toStdString().c_str());
            printd("get File - GPX end \n");
            filename = filenameGPX;
        } else filename = "noContent";
    } // close filetype gpx

 // second file TCX
    if (filetype == "tcx") {
        QString get_TCX_Str;
                get_TCX_Str = QString("%1/v3/users/%2/exercise-transactions/%3/exercises/%4/tcx")
                   .arg(getSetting(GC_POLARFLOW_URL, "https://www.polaraccesslink.com").toString())
                   .arg(getSetting(GC_POLARFLOW_USER_ID, "").toString())
                   .arg(getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString())
                   .arg(exerciseId);
        printd("get File - TCX - URL: %s \n", get_TCX_Str.toStdString().c_str());

        // request using the bearer token, application/vnd.garmin.tcx+xml
        QNetworkRequest requestTCX(get_TCX_Str);
                        requestTCX.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
                        requestTCX.setRawHeader("Accept", "application/vnd.garmin.tcx+xml");

        // make request
        QNetworkReply *replyTCX = nam->get(requestTCX);
        printd("response : %s\n", get_TCX_Str.toStdString().c_str());
        // Example: Reply List of URLs /v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}/tcx

        // blocking request
        QEventLoop loopTCX;
        connect(replyTCX, SIGNAL(finished()), &loopTCX, SLOT(quit()));
        loopTCX.exec();

        // if successful, lets unpack
        int statusCodeTCX = replyTCX->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        printd("fetch response: %d: %s\n", replyTCX->error(), replyTCX->errorString().toStdString().c_str());
        printd("get File - TCX - HTTP response status code: %d \n", statusCodeTCX);


        QByteArray get_TCX_Ary = replyTCX->readAll();
        //printd("page : %s\n", get_TCX_Ary.toStdString().c_str());

       if (statusCodeTCX == dataAvailable) {
            QString filenameTCX;
                    filenameTCX = saveFile(get_TCX_Ary, exerciseId, filetype, starttime, detailedSportInfo_Str);

            printd("get File - TCX - filename: %s \n", filenameTCX.toStdString().c_str());
            printd("get File - TCX - end \n");
            filename = filenameTCX;
        } else filename = "noContent";
    } // close filetype tcx

 // third file FIT Beta
    if (filetype == "fit") {
        QString get_FIT_Str;
                get_FIT_Str = QString("%1/v3/users/%2/exercise-transactions/%3/exercises/%4/fit")
                   .arg(getSetting(GC_POLARFLOW_URL, "https://www.polaraccesslink.com").toString())
                   .arg(getSetting(GC_POLARFLOW_USER_ID, "").toString())
                   .arg(getSetting(GC_POLARFLOW_TRANSACTION_ID, "").toString())
                   .arg(exerciseId);
        printd("get File - FIT Beta - URL-List Samples: %s \n", get_FIT_Str.toStdString().c_str());

        // request using the bearer token, */*
        QNetworkRequest requestFIT(get_FIT_Str);
                        requestFIT.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());
                        requestFIT.setRawHeader("Accept", "*/*");

        // make request
        QNetworkReply *replyFIT = nam->get(requestFIT);
        printd("response : %s\n", get_FIT_Str.toStdString().c_str());
        // Example: Reply List of URLs /v3/users/{user-id}/exercise-transactions/{transaction-id}/exercises/{exercise-id}/fit

        // blocking request
        QEventLoop loopFIT;
        connect(replyFIT, SIGNAL(finished()), &loopFIT, SLOT(quit()));
        loopFIT.exec();

        // if successful, lets unpack
        int statusCodeFIT = replyFIT->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        printd("fetch response: %d: %s\n", replyFIT->error(), replyFIT->errorString().toStdString().c_str());
        printd("get File - FIT Beta - Samples - HTTP response status code: %d \n", statusCodeFIT);

        QByteArray get_FIT_Ary = replyFIT->readAll();
        //printd("get File - FIT Beta - QByteArray: %s \n", getAvailableSampleTypesUrls_Ary.toStdString().c_str());

        if (statusCodeFIT == dataAvailable) {
            QString filenameFIT;
                    filenameFIT = saveFile(get_FIT_Ary, exerciseId, filetype, starttime, detailedSportInfo_Str);

            printd("get File - FIT Beta - filename: %s \n", filenameFIT.toStdString().c_str());
            printd("get File - FIT Beta - close \n");
            filename = filenameFIT;
        } else filename = "noContent";
    } // close filetype fit beta

    // filename to return
    printd("get File - returned filename: %s \n", filename.toStdString().c_str());
    printd("get File - end \n");
    return filename;
}

void
PolarFlow::ImportFiles(QJsonObject &object, QString exerciseId)
{
    /*
    printd("acquireAndImportFiles - Remotestring: %d \n", remoteid);
    //qDebug() << "acquireAndImportFiles - data:" << object;

    QList<QString> fileName_Lst; // for RideImportWizard
    QString fileName_Str;
    QString stype = object["sport"].toString();
    if (stype.endsWith("SWIMMING")) {
        fileName_Str = getFITBeta(exerciseId);
        getTCX(exerciseId);
        getGPX(exerciseId);
    }
    else {
        fileName_Str = saveTCX(exerciseId);
        getFITBeta(exerciseId);
        getGPX(exerciseId);
    }
        fileName_Lst.append(fileName_Str);

    printd("acquireAndImportFiles - filename_String: %s \n" << fileName_Str.toStdString().c_str());
    //qDebug() << "acquireAndImportFiles - filename_String-List: " << fileName_Lst;

// First: Take Infos from tcx-File
    // Start with Importfile - set isAutoImport
    //RideImportWizard *rideImportFile = new RideImportWizard(fileName_Lst, context);
                      //rideImportFile->process();
                      //rideImportFile->isAutoImport();
                      //rideImportFile->activateSave(); // does not work this way
                      //rideImportFile->autoImportMode; // does not work this way
                      //rideImportFile->autoImportStealth; // does not work this way
// Import of Ride is completed
    //delete rideImportFile;
    */
}

void
PolarFlow::addExerciseSummaryInformation(RideFile* ret, QJsonObject &object)
{
    printd("addExerciseSummaryInformation - start \n");

    // Get start-time
    QDateTime starttime = QDateTime::fromString(object["start-time"].toString(), Qt::ISODate);

    // Get upload-time
    QDateTime uploadtime = QDateTime::fromString(object["upload-time"].toString(), Qt::ISODate);
       //rideImport->setTag("Upload Time", uploadtime);

    // Get device
    if (object["device"].toString().length()>0)
        ret->setDeviceType(object["device"].toString());
    else
        ret->setDeviceType("Polar"); // The device type is unknown

    if (object["device-id"].toString().length()>0)
        ret->setTag("device id", object["device-id"].toString());

    // Get start-time-utc-offset
    if (!object["start-time-utc-offset"].isNull()) {
        QJsonValue startTimeUtcOffset_Val = object.value("start-time-utc-offset");
        QString startTimeUtcOffset_Str = QString::number(startTimeUtcOffset_Val.toInt());
        //int calories = calories_Str.toInt();
        ret->setTag("Start Time Utc Offset", startTimeUtcOffset_Str);
    }

    // Get duration
    int duration_time = 0; // start value
    if (object["duration"].toString().length()>0) {
        QString duration_str = object["duration"].toString();        // "PT4H51M38.512S"
        duration_time = isoDateToSeconds(duration_str);
    }

    if (object["moving_time"].toDouble()>0) {
        QMap<QString,QString> map;
        map.insert("value", QString("%1").arg(object["moving_time"].toDouble()));
        ret->metricOverrides.insert("time_riding", map);
    }
    if (duration_time>0) { // elapsed time = Polar Duration
        QMap<QString,QString> map;
        map.insert("value", QString("%1").arg(duration_time));
        ret->metricOverrides.insert("workout_time", map);
    }

    // Get calories
    if (!object["calories"].isNull()) {
        QJsonValue calories_Val = object.value("calories");
        QString calories_Str = QString::number(calories_Val.toInt());
        //int calories = calories_Str.toInt();
        ret->setTag("calories", calories_Str);
    }

    // Get distance
    if (object["distance"].toDouble()>0) {
        QMap<QString,QString> map;
        map.insert("value", QString("%1").arg(object["distance"].toDouble()/1000.0));
        ret->metricOverrides.insert("total_distance", map);
     }

    // Get heart-rate average and maximum
    if (!object["heart-rate"].isNull()) {
        QJsonValue heartrate_Val;
                   heartrate_Val = object.value(QString("heart-rate"));
        QJsonObject heartrate_Obj;
                    heartrate_Obj = heartrate_Val.toObject();
        //qDebug() << "prepareResponse - heartrate_Value:" << heartrate_Val;
        if (!heartrate_Obj["average"].isNull()) {
            QJsonValue averageHr_Val = heartrate_Obj.value("average");
            QString averageHr = QString::number(averageHr_Val.toInt());
            QMap<QString,QString> map;
            map.insert("value", averageHr);
            ret->metricOverrides.insert("Average Heart Rate", map);
        }
        if (!heartrate_Obj["maximum"].isNull()) {
            QJsonValue maximumHr_Val = heartrate_Obj.value("maximum");
            QString maximumHr = QString::number(maximumHr_Val.toInt());
            QMap<QString,QString> map;
            map.insert("value", maximumHr);
            ret->metricOverrides.insert("Maximum Heart Rate", map);
        }
    }

    // get training-load
    if (object["training-load"].toDouble()>0) {
        QJsonValue trainingLoad_Val = object.value("training-load");
        QString trainingLoad_Str = QString::number(trainingLoad_Val.toDouble());
        ret->setTag("training load", trainingLoad_Str);
     }

    // Get sport - main
    if (!object["sport"].isNull()) {
        QString stype = object["sport"].toString();
        if (stype.endsWith("CYCLING")) ret->setTag("Sport", "Bike");
        else if (stype.endsWith("RUNNING")) ret->setTag("Sport", "Run");
        else if (stype.endsWith("SWIMMING")) ret->setTag("Sport", "Swim");
        else if (stype.endsWith("OTHER")) ret->setTag("Sport", "");
        else ret->setTag("Sport", stype);
        // Set SubSport to preserve the original when Sport was mapped
        if (stype != ret->getTag("Sport", "")) ret->setTag("SubSport", stype);
        //qDebug() << "prepareResponse - stype: " << stype;
    }

    // has route
    if (!object["has-route"].isNull()) {
        ret->setTag("has route", object["has-route"].toBool() ? "1" : "0");
    }

    // get club-id
    if (!object["club-id"].isNull()) {
        QJsonValue club_id_Val = object.value("club-id");
        QString club_id = QString::number(club_id_Val.toInt());
        ret->setTag("club-id", club_id);
    }

    // get club-name
    if (object["club-name"].isString()) {
        ret->setTag("club-name", object["club-name"].toString());
    }

    // get detailed-sport-info
    // https://www.polar.com/accesslink-api/?shell#detailed-sport-info-values-in-exercise-entity
    if (object["detailed-sport-info"].isString()) {
        QString detailedSportInfo = object["detailed-sport-info"].toString();
        QString seperated1; // first string
        QString seperated2;
        QString seperated3;
        QString seperated4;
        QString seperated5;
        QString seperated1FC; // first charater from first string
        QString seperated2FC;
        QString seperated3FC;
        QString seperated4FC;
        QString seperated5FC;
        QString seperated1UC; // first character uppercase
        QString seperated2UC;
        QString seperated3UC;
        QString seperated4UC;
        QString seperated5UC;

        QString searchFor = "_";
        QString replaceWith = " ";
        QString detailedSportInfoToLower;
        QString detailedSportInfoToLowerFCUC; // detailed sports info with first charater uppercase
        /*
        // this version separates the "sections" of the string
        int j = 0; // counting variable
        int stringsize = detailedSportInfo.size();
        int before = 0;
        int after = 0;
        // search for divider "_", we assume there is only one "_"
        // this solution works for one underline in the string

        while ((j = detailedSportInfo.indexOf(searchFor, j)) != -1) {
            before = j - 0;
            after = stringsize - (j + 1);
            printd("addExerciseSummaryInformation - Found _ at index position: %d \n", j);
            printd("addExerciseSummaryInformation - before: %d \n", before);
            printd("addExerciseSummaryInformation - after: %d \n", after);
            ++j;
        }
        QString detailedSportInfoLeft = detailedSportInfo.left(before).toLower();
        QString detailedSportInfoRight = detailedSportInfo.right(after).toLower();
        if (before <= 0) {
                detailedSportInfoToLower = detailedSportInfo.toLower();
        }
        if ( before > 0) {
                detailedSportInfoToLower = detailedSportInfoLeft+" "+detailedSportInfoRight;
        }
        */
        detailedSportInfoToLower = detailedSportInfo.replace(searchFor,replaceWith).toLower();
        printd("addExerciseSummaryInformation - detailed sport info original: %s \n", detailedSportInfo.toStdString().c_str());

        // First Character should be Uppercase
        QString::SectionFlag emptyParts = QString::SectionSkipEmpty; //skip empty parts
        seperated1 = detailedSportInfoToLower.section(' ', 0, 0, emptyParts);
        seperated1FC = seperated1.left(1);
        seperated2 = detailedSportInfoToLower.section(' ', 1, 1, emptyParts);
        seperated2FC = seperated2.left(1);
        seperated3 = detailedSportInfoToLower.section(' ', 2, 2, emptyParts);
        seperated3FC = seperated3.left(1);
        seperated4 = detailedSportInfoToLower.section(' ', 3, 3, emptyParts);
        seperated4FC = seperated4.left(1);
        seperated5 = detailedSportInfoToLower.section(' ', 4, 4, emptyParts);
        seperated5FC = seperated5.left(1);
        printd("detailed sport info seperated first string: %s \n", seperated1.toStdString().c_str());
        printd("detailed sport info seperated first character from first string: %s \n", seperated1FC.toStdString().c_str());
        printd("detailed sport info seperated second string: %s \n", seperated2.toStdString().c_str());
        printd("detailed sport info seperated second character from first string: %s \n", seperated2FC.toStdString().c_str());
        printd("detailed sport info seperated third string: %s \n", seperated3.toStdString().c_str());
        printd("detailed sport info seperated third character from first string: %s \n", seperated3FC.toStdString().c_str());
        printd("detailed sport info seperated fourth string: %s \n", seperated4.toStdString().c_str());
        printd("detailed sport info seperated fourth character from first string: %s \n", seperated4FC.toStdString().c_str());
        printd("detailed sport info seperated fith string: %s \n", seperated5.toStdString().c_str());
        printd("detailed sport info seperated fith character from first string: %s \n", seperated5FC.toStdString().c_str());

        seperated1UC = seperated1.replace(0, 1, seperated1FC.toUpper());
        seperated2UC = seperated2.replace(0, 1, seperated2FC.toUpper());
        seperated3UC = seperated3.replace(0, 1, seperated3FC.toUpper());
        seperated4UC = seperated4.replace(0, 1, seperated4FC.toUpper());
        seperated5UC = seperated5.replace(0, 1, seperated5FC.toUpper());
        printd("detailed sport info seperated first character from first string uppercase: %s \n", seperated1UC.toStdString().c_str());
        printd("detailed sport info seperated second character from first string uppercase: %s \n", seperated2UC.toStdString().c_str());
        printd("detailed sport info seperated third character from first string uppercase: %s \n", seperated3UC.toStdString().c_str());
        printd("detailed sport info seperated fourth character from first string uppercase: %s \n", seperated4UC.toStdString().c_str());
        printd("detailed sport info seperated fith character from first string uppercase: %s \n", seperated5UC.toStdString().c_str());

        // create one String without whitespaces
        detailedSportInfoToLowerFCUC = (seperated1UC + " " + seperated2UC + " " + seperated3UC + " " + seperated4UC + " " + seperated5UC).simplified();
        printd("addExerciseSummaryInformation - detailed sport info FCUC: %s \n", detailedSportInfoToLowerFCUC.toStdString().c_str());

        //printd("addExerciseSummaryInformation - detailed sport info number of chars divider: %d \n", j);
        //printd("addExerciseSummaryInformation - detailed sport info number of chars before: %d \n", before);
        //printd("addExerciseSummaryInformation - detailed sport info number of chars after: %d \n", after);
        //printd("addExerciseSummaryInformation - detailed sport info left: %s \n", detailedSportInfoLeft.toStdString().c_str());
        //printd("addExerciseSummaryInformation - detailed sport info right: %s \n", detailedSportInfoRight.toStdString().c_str());
        printd("addExerciseSummaryInformation - detailed sport info: %s \n", detailedSportInfoToLower.toStdString().c_str());
        //ret->setTag("SubSport", detailedSportInfoToLower);
        ret->setTag("SubSport", detailedSportInfoToLowerFCUC);
    }

    // get fat-percentage
    if (!object["fat-percentage"].isNull()) {
        QJsonValue fatPercentage_Val = object.value("fat-percentage");
        QString fatPercentage = QString::number(fatPercentage_Val.toInt());
        ret->setTag("fat-percentage", fatPercentage);
    }

    // get carbohydrate-percentage
    if (!object["carbohydrate-percentage"].isNull()) {
        QJsonValue carbohydratePercentage_Val = object.value("carbohydrate-percentage");
        QString carbohydratePercentage = QString::number(carbohydratePercentage_Val.toInt());
        ret->setTag("carbohydrate-percentage", carbohydratePercentage);
    }

    // get protein-percentage
    if (!object["protein-percentage"].isNull()) {
        QJsonValue proteinPercentage_Val = object.value("protein-percentage");
        QString proteinPercentage = QString::number(proteinPercentage_Val.toInt());
        ret->setTag("protein-percentage", proteinPercentage);
    }

    // get running-index
    if (!object["running-index"].isNull()) {
        QJsonValue runningIndex_Val = object.value("running-index");
        QString runningIndex = QString::number(runningIndex_Val.toInt());
        ret->setTag("running-index", runningIndex);
    }

    printd("addExerciseSummaryInformation - end \n");
}

QByteArray*
PolarFlow::prepareResponse(QByteArray* data, QString &name)
{
    printd("prepareResponse - start \n");
    //qDebug() << "prepareResponse data:" << data;
    //qDebug() << "prepareResponse name:" << name;

    // Save original name
    QString originalName = name;

    // how will the exercise be acquired? via "File"-Import or via "Streams"
    QString typeOfImporting; // "File|Samples"
            typeOfImporting = POLARFLOW_IMPORTTYPE;
            //typeOfImporting = "File";
    //printd("PolarFlow::prepareResponse: %s \n", originalName);

    QJsonParseError parseError;
    QJsonDocument get_exercise_summary_Doc = QJsonDocument::fromJson(data->constData(), &parseError);
    printd("prepareResponse - get_exercise_summary_Doc - Error Number: %d - Error String: %s \n", parseError.error, parseError.errorString().toStdString().c_str());
    //qDebug() << "prepareResponse - get_exercise_summary_Document:" << get_exercise_summary_Doc;

    // which variables are needed overall
    QString exercise_id_Str;
    QDateTime starttime;

    // if path was returned all is good, lets set root
    if (parseError.error == QJsonParseError::NoError) {
        QJsonObject get_exercise_summary_Obj = get_exercise_summary_Doc.object();

        // each item looks like this:
        //https://www.polar.com/accesslink-api/?shell#get-exercise-summary
        //https://www.polar.com/accesslink-api/?shell#tocSexercise
        //https://www.polar.com/accesslink-api/?shell#tocSheartrate
        /*
          {
            "id": 1937529874,                                       integer(int64)
            "upload-time": "2008-10-13T10:40:02Z",                  string
            "polar-user": "https://www.polaraccesslink/v3/users/1", string
            "transaction-id": 179879,                               integer(int64)
            "device": "Polar M400",                                 string
            "device-id": "1111AAAA",                                string
            "start-time": "2008-10-13T10:40:02Z",                   string
            "start-time-utc-offset": 180,                           integer(int32)
            "duration": "PT2H44M",                                  string
            "calories": 530,                                        integer(int32)
            "distance": 1600,                                       number(float)
            "heart-rate": {
              "average": 129,                                       integer(int32)
              "maximum": 147                                        integer(int32)
            },
            "training-load": 143.22,                                number(float)
            "sport": "OTHER",                                       string
            "has-route": true,                                      boolean
            "club-id": 999,                                         integer(int64)
            "club-name": "Polar Club",                              string
            "detailed-sport-info": "WATERSPORTS_WATERSKI",          string
            "fat-percentage": 60,                                   integer(int32)
            "carbohydrate-percentage": 38,                          integer(int32)
            "protein-percentage": 2                                 integer(int32)
          }
         */

        // Get Exercise ID - needed for download file and getting sample-data
        exercise_id_Str = QString::number(get_exercise_summary_Obj["id"].toInt());
        printd("prepareResponse - ExerciseID_String: %s \n", exercise_id_Str.toStdString().c_str());

        // Get start-time
        starttime = QDateTime::fromString(get_exercise_summary_Obj["start-time"].toString(), Qt::ISODate);

        // Acquire Excercise in all available file formats
        // get detailed-sport-info
        // https://www.polar.com/accesslink-api/#detailed-sport-info-values-in-exercise-entity
        // detailed sports info is written in capital letters and all spaces are replaced by an underline by Polar default
        QString detailedSportInfo_Str = "DetailedSportInfoIsEmpty"; // set default to not empty
        if (get_exercise_summary_Obj["detailed-sport-info"].isString()) {
            detailedSportInfo_Str = get_exercise_summary_Obj["detailed-sport-info"].toString();
        }

        // getting all available file-types = tcx, gpx, fitBeta
        // we will download all available filetypes
        QStringList errorsImport;
        QString filetype;         // for downloading

        // download tcx
        filetype = "tcx";
        QString filenameWithPathTCX = getFile(exercise_id_Str, filetype, starttime, detailedSportInfo_Str);

        // download gpx
        filetype = "gpx";
        QString filenameWithPathGPX = getFile(exercise_id_Str, filetype, starttime, detailedSportInfo_Str);

        // download fit
        filetype = "fit";
        QString filenameWithPathFITBeta = getFile(exercise_id_Str, filetype, starttime, detailedSportInfo_Str);

        printd("prepareResponse - filename with Path TCX: %s \n", filenameWithPathTCX.toStdString().c_str());
        printd("prepareResponse - filename with Path GPX: %s \n", filenameWithPathGPX.toStdString().c_str());
        printd("prepareResponse - filename with Path FITBeta: %s \n", filenameWithPathFITBeta.toStdString().c_str());

        // Import Excercise via File-Import
        if (typeOfImporting == "File") {
        //Which type will be imported? "TCX" or "FIT"
        // set file name for import in GC - use TCX - Format because notes are included
        QString filenameForImport; // file for importing

        QString filetypeForImport;
                filetypeForImport = POLARFLOW_IMPORTFILEFORMAT;
                //filetypeForImport = "TCX";

        if (filetypeForImport == "TCX") {
            filenameForImport = filenameWithPathTCX;
        }
        if (filetypeForImport == "GPX") {
            filenameForImport = filenameWithPathGPX;
        }
        if (filetypeForImport == "FIT") {
            filenameForImport = filenameWithPathFITBeta;
        }

        // we need the full Path for importing downloaded file
        QString filenameWithPathSuffix = QFileInfo(filenameForImport).suffix();
        printd("prepareResponse - filename with Path Suffix: %s \n", filenameWithPathSuffix.toStdString().c_str());

        QFile importfile(filenameForImport);
        printd("prepareResponse - filename with Path: %s \n", filenameForImport.toStdString().c_str());

        QFileInfo filenameWithPathComplete(filenameForImport);
        QString filename = filenameWithPathComplete.fileName();
        printd("prepareResponse - filename: %s \n", filename.toStdString().c_str());

        // Import downloaded file and set some extra data from exercise summary to RideFile: rideImport
        RideFile *rideImport = RideFileFactory::instance().openRideFile(context, importfile, errorsImport);
        //qDebug() << "prepareResponse - uncompressRide - errors:" << errorsImport;

        // minimal information added to imported file, so we can see where we got the information from
        // Set Source Filename
        if (filename.length()>0) {
            rideImport->setTag("Source Filename", filename);
        }

        // set polar exercise id in metadata (to show where we got it from - to add View on Polar link in Summary view
        if (!get_exercise_summary_Obj["id"].isNull()) {
            rideImport->setTag("Polar Exercise ID",  exercise_id_Str);
        }

        // add the rest of available excercise informations
        addExerciseSummaryInformation(rideImport, get_exercise_summary_Obj);

        //printd("prepareResponse - fixSmartRecording: %s \n", rideImport);
        fixSmartRecording(rideImport);
        JsonFileReader rideImportReader;
        data->clear();
        //What is included? (Context *context, const RideFile *ride, bool withAlt, bool withWatts, bool withHr, bool withCad)
        data->append(rideImportReader.toByteArray(context, rideImport, true, true, true, true));
        // temp ride not needed anymore
        delete rideImport;
        }

        // Acquire Excercise via Stream-Import
        if (typeOfImporting == "Samples") {
            // Create a new Ridefile for catching all sample data
            // Get Exercise Starttime - needed for RideFile: rideWithSamples
            starttime = QDateTime::fromString(get_exercise_summary_Obj["start-time"].toString(), Qt::ISODate);

            // 1s samples with start time
            RideFile *rideWithSamples = new RideFile(starttime.toUTC(), 1.0f);
            // set polar exercise id in metadata - to show where we got it from - to add View on Polar link in Summary view
            if (!get_exercise_summary_Obj["id"].isNull()) {
                rideWithSamples->setTag("Polar Exercise ID",  exercise_id_Str);
            }

            // add rest of metadata and return start time as value
            addExerciseSummaryInformation(rideWithSamples, get_exercise_summary_Obj);

            // Adding SampleData to Ridefile
            //addSamples(rideWithSamples, exercise_id_Str, filenameWithPathSuffix);
            addSamples(rideWithSamples, exercise_id_Str);

            // swim laps / own laps
            /*
            if (!get_exercise_summary_Obj["laps"].isNull()) {
                QJsonArray laps = get_exercise_summary_Obj["laps"].toArray();

                double last_lap = 0.0;
                foreach (QJsonValue value, laps) {
                    QJsonObject lap = value.toObject();

                    double start = starttime.secsTo(QDateTime::fromString(lap["start-time"].toString(), Qt::ISODate));
                    if (start < last_lap) start = last_lap + 1; // Don't overlap
                    double end = start + duration_time; //lap["elapsed_time"].toDouble() - 1;

                    last_lap = end;

                    rideImport->addInterval(RideFileInterval::USER, start, end, lap["name"].toString());
                } // close foreach - loop
                // Fix distance from laps and fill gaps for pool swims
                fixLapSwim(rideImport, laps);
             } //close laps if
            */

            fixSmartRecording(rideWithSamples);
            //printd("prepareResponse - fixSmartRecording: %s \n", rideWithSamples);
            JsonFileReader rideWithSamplesReader;

            data->clear();
            //What is included? (Context *context, const RideFile *ride, bool withAlt, bool withWatts, bool withHr, bool withCad)
            data->append(rideWithSamplesReader.toByteArray(context, rideWithSamples, true, true, true, true));

            //new MergeActivityWizard(currentAthleteTab->context))->exec()
            //currentAthleteTab->context->ride && currentAthleteTab->context->ride->ride() && currentAthleteTab->context->ride->ride()->dataPoints().count()) (new MergeActivityWizard(currentAthleteTab->context))->exec();
            //MergeActivityWizard *merge = new MergeActivityWizard(rideImport, rideWithSamples);
            //MergeActivityWizard::setRide(QWizard->rideMerge, rideWithSamples)

            // temp ride not needed anymore
            delete rideWithSamples;
        }

        //printd("reply:%s\n", data->toStdString().c_str());
        printd(" end \n");
    } // close lets set root

    // if we have aquired the last exercise from list, so we can commit the transaction
    QStringList emptyerrorlist;
    QString lastListedExerciseId = getSetting(GC_POLARFLOW_LAST_EXERCISE_ID, "").toString();
    QString commitTransactionOverride = POLARFLOW_COMMITTRANSACTIONOVERRIDE; // true/false

    // What will we do? Commit Transaction or not ?
    // first check, if all exercises are aquired
    printd("last exercise id: %s \n", lastListedExerciseId.toStdString().c_str());
    printd("this exercise id: %s \n", exercise_id_Str.toStdString().c_str());

    // if last listed Exercise Id is NOT the same as actual Exercise Id then we can NOT commit the transaction
    if (lastListedExerciseId != exercise_id_Str) {
        printd("not calling commit, because is it not the last exercise\n");
    }
    // else  we can commit the transaction
    else {
        printd("we can commit the transaction, because we fetched the last exercise \n");
        printd("check debug override commit- last exercise id: %s - Override: %s \n ", commitTransactionOverride.toStdString().c_str(), commitTransactionOverride.toStdString().c_str());
        if (commitTransactionOverride == "false") {
            printd("no override, so we commit the transaction with \n");
            printd("we fetched all available exercises: %s = %s \n",exercise_id_Str.toStdString().c_str() ,lastListedExerciseId.toStdString().c_str());
            commit(emptyerrorlist);
        }

     }
    // at last return aquired data
    return data;
}


// development put on hold - AccessLink API compatibility issues with Desktop applications
//#if 0
static bool addPolarFlow() {
    CloudServiceFactory::instance().addService(new PolarFlow(NULL));
    return true;
}

static bool add = addPolarFlow();
//#endif
