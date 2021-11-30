/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#ifndef CLOUDDBCOMMON_H
#define CLOUDDBCOMMON_H

#include "Settings.h"
#include "Secrets.h"


#include <QObject>
#include <QScrollArea>
#include <QDialog>
#include <QNetworkReply>


// Central Classes and Constants related to CloudDB

// API Structure V1 must be in sync with the Structure used for the V1 on CloudDB
// but uses the correct QT datatypes

typedef struct {
    qint64 Id;
    QString Key;
    QString Name;
    QString Description;
    QString Language;
    QString GcVersion;
    QDateTime LastChanged;
    QString CreatorId;
    bool    Curated;
    bool    Deleted;
    // gchart specific header cache fields
    QString ChartType;
    QString ChartView;
    QString ChartSport;
} CommonAPIHeaderV1;


class CloudDBCommon : public QObject
{
    Q_OBJECT

public:

    static void prepareRequest(QNetworkRequest &request, QString urlString, QUrlQuery *query = NULL);
    static void processReplyStatusCodes(QNetworkReply *reply);
    static bool replyReceivedAndOk(QNetworkReply *reply);

    static void sslErrors(QNetworkReply* reply ,QList<QSslError> errors);

    static bool unmarshallAPIHeaderV1(QByteArray , QList<CommonAPIHeaderV1>* );
    static void unmarshallAPIHeaderV1Object(QJsonObject* , CommonAPIHeaderV1* chart);
    static void marshallAPIHeaderV1Object(QJsonObject&, CommonAPIHeaderV1 &header);
    static QString encodeHTML ( const QString& );

    static bool addCuratorFeatures;

    static QString cloudDBBaseURL;
    static QVariant cloudDBContentType;
    static QByteArray cloudDBBasicAuth;

    // Languages explicitely supported to store artifacts == UI Languages
    static QList<QString> cloudDBLangsIds;
    static QList<QString> cloudDBLangs;

    // Sport Types supported
    static QList<QString> cloudDBSportIds;
    static QList<QString> cloudDBSports;

    static QString cloudDBTimeFormat;
    static const int APIresponseOk = 200; // also used in case of 204 (No Content)
    static const int APIresponseCreated = 201;
    static const int APIresponseForbidden = 403;
    static const int APIresponseConflict = 409;
    static const int APIresponseServiceProblem = 422; // CloudDB response if Status <> 10 (Ok)
    static const int APIresponseOverQuota = 503;
    static const int APIresponseOthers = 999;

    enum userRole { UserImport, UserEdit, CuratorEdit };
    typedef enum userRole UserRole;


};


// -------------------------------------------------------
// Dialog to show T&C for the use of CloudDB offering,
// and accepting those before executing any CloudDB
// functions.
// -------------------------------------------------------

class CloudDBAcceptConditionsDialog : public QDialog
{
    Q_OBJECT

public:
    CloudDBAcceptConditionsDialog(QString athlete);

private slots:
    void acceptConditions();
    void rejectConditions();

private:

    QString athlete;

    QScrollArea *scrollText;
    QPushButton *proceedButton;
    QPushButton *abortButton;

};

class CloudDBHeader
{

public:

    enum headerType { CloudDB_Chart, CloudDB_UserMetric };
    typedef enum headerType CloudDBHeaderType;

    static bool writeHeaderCache(QList<CommonAPIHeaderV1>*, CloudDBHeaderType headerType, QString cache_Dir );
    static bool readHeaderCache(QList<CommonAPIHeaderV1>*, CloudDBHeaderType headerType, QString cache_Dir );
    static bool getAllCachedHeader(QList<CommonAPIHeaderV1> *objectHeader, CloudDBHeaderType type, QString cache_Dir, QString url,
                                      QNetworkAccessManager *nam, QNetworkReply *reply);


    static void setChartHeaderStale(bool b) {chartHeaderStatusStale = b;}
    static bool isStaleChartHeader() {return chartHeaderStatusStale;}

    static void setUserMetricHeaderStale(bool b) {userMetricHeaderStatusStale = b;}
    static bool isStaleUserMetricHeader() {return userMetricHeaderStatusStale;}

private:

    static const int header_magic_string = 1253346430;
    static const int header_cache_version = 3;  //increase version to clear existing cache

    static bool chartHeaderStatusStale;
    static bool userMetricHeaderStatusStale;

};




#endif // CLOUDDBCOMMON_H
