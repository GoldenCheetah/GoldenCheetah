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

#include "CloudDBCommon.h"
#include "CloudDBStatus.h"
#include "Settings.h"
#include "Colors.h"

#include <QLabel>
#include <QMessageBox>


CloudDBAcceptConditionsDialog::CloudDBAcceptConditionsDialog(QString athlete) : athlete(athlete)
{

    setWindowTitle(QString(tr("GoldenCheetah CloudDB - Terms and Conditions")));
    setMinimumWidth(550);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QPushButton *important = new QPushButton(style()->standardIcon(QStyle::SP_MessageBoxInformation), "", this);
    important->setFixedSize(80*dpiXFactor,80*dpiYFactor);
    important->setFlat(true);
    important->setIconSize(QSize(80*dpiXFactor,80*dpiYFactor));
    important->setAutoFillBackground(false);
    important->setFocusPolicy(Qt::NoFocus);

    QLabel *header = new QLabel(this);
    header->setWordWrap(true);
    header->setTextFormat(Qt::RichText);
    header->setText(QString(tr("<b><big>Please read terms and conditions carefully before accepting !</big></b>")));

    QHBoxLayout *toprow = new QHBoxLayout;
    toprow->addWidget(important);
    toprow->addWidget(header);
    layout->addLayout(toprow);

    QLabel *text = new QLabel(this);
    text->setWordWrap(true);
    text->setTextFormat(Qt::RichText);
    text->setText(tr("<center><b>What is GoldenCheetah CloudDB</b><br>"
                     "You have requested a function with shares or retrieves GoldenCheetah "
                     "configuration artifacts like chart definitions, worksouts, custom metrics,... "
                     "in a cloud based data storage extension which is running on Google "
                     "App Engine in a data center of variable location as provided by Google "
                     "and/or choosen by the GoldenCheetah team.<br>"
                     "<center><b>License and Privacy Rules for Sharing Artifacts</b><br>"
                     "By sharing an artifact to CloudDB you agree to provide it under "
                     "<a href=\"http://creativecommons.org/licenses/by/4.0/\" target=\"_blank\">"
                     "Creative Common License 4.0</a>. "
                     "When sharing you are asked to enter a 'Nickname' and an 'Email-Address' "
                     "which are stored together with a unique identifier (which is automatically created for your "
                     "athlete) in the CloudDB. This data is only used to support the CloudDB service "
                     "and will not be disclosed or shared for other purposes (e.g. advertisement).<br>"
                     "<center><b>License and Rules for Importing Artifacts</b><br>"
                     "By importing an artifact from CloudDB you accept the license agreement "
                     "<a href=\"http://creativecommons.org/licenses/by/4.0/\" target=\"_blank\">"
                     "Creative Common License 4.0</a>. "
                     "Any artifcats are provided as-is without warranty of any kind, either expressed "
                     "or implied, including, but not limited to, the implied warrenty of fitness "
                     "for a particular purpose. The entire risk related to the quality of the artifact "
                     "is with you. There is no liability regarding damages to your data trough the use of any "
                     "of the artifacts and there will be no quality check/ensurance by the GoldenCheetah team "
                     "for the provided artifacts.<br>"
                     "<center><b>CloudDB Service Availability Disclaimer</b><br>"
                     "The service is provisioned free-of-charge to the users of GoldenCheetah. "
                     "There is no guarantee nor service level agreement on the 7x24 availabiliy of CloudDB services. "
                     "The GoldenCheetah team has the right to delete artifacts from the CloudDB at any time "
                     "and without prior notice to the providing user (e.g. due to quality issues, or due "
                     "to limitations in storage capacity). The GoldenCheetah team has the right to suspend "
                     "or stop the complete service or parts of the service at any time and without prior notice "
                     "to the users.<br>"
                     "<center><b>CloudDB Service Rules</b><br>"
                     "The use of the service is exclusively granted to GoldenCheetah users - and the only permitted way "
                     "to access any of the artifacts is through the GoldenCheetah application. Any attempt "
                     "to access CloudDB data directly is forbidden and will either be blocked or may even end in suspending "
                     "or stopping the service in general.<br>"
                     "<center><b>Accept or Reject</b><br>"
                     "By pressing 'Accept the rules and conditions of CloudDB' you confirm that you have read and fully "
                     "understood the above text and you agree to follow all of these. This decision is recorded so that "
                     "you will not asked again when using CloudDB related functions. <br><br>"
                     "If you are not sure you can choose 'Reject and don't use CloudDB'. In this case this agreement "
                     "request will appear again whenever you request to use a CloudDB related function in GoldenCheetah.<br>"
                     "<center><b>CloudDB Source Code</b><br>"
                     "The source code of CloudDB is published under 'GNU AFFERO GENERAL PUBLIC LICENSE Version 3' here: "
                     "<a href= \"https://github.com/GoldenCheetah/CloudDB\" target=\"_blank\">"
                     "CloudDB Repository</a><br><br>"
                     ));

    scrollText = new QScrollArea();
    scrollText->setWidget(text);
    scrollText->setWidgetResizable(true);
    layout->addWidget(scrollText);

    QHBoxLayout *lastRow = new QHBoxLayout;

    proceedButton = new QPushButton(tr("Accept the rules and conditions of CloudDB"), this);
    proceedButton->setEnabled(true);
    connect(proceedButton, SIGNAL(clicked()), this, SLOT(acceptConditions()));
    abortButton = new QPushButton(tr("Reject and don't use CloudDB"), this);
    abortButton->setDefault(true);
    connect(abortButton, SIGNAL(clicked()), this, SLOT(rejectConditions()));

    lastRow->addWidget(abortButton);
    lastRow->addStretch();
    lastRow->addWidget(proceedButton);
    layout->addLayout(lastRow);

}

void CloudDBAcceptConditionsDialog::acceptConditions() {

    // document the decision
    appsettings->setCValue(athlete, GC_CLOUDDB_TC_ACCEPTANCE, true);
    appsettings->setCValue(athlete, GC_CLOUDDB_TC_ACCEPTANCE_DATE,
                           QDateTime::currentDateTime().toString(Qt::ISODate));

    accept();
}

void CloudDBAcceptConditionsDialog::rejectConditions() {

    // document the decision
    appsettings->setCValue(athlete, GC_CLOUDDB_TC_ACCEPTANCE, false);
    appsettings->setCValue(athlete, GC_CLOUDDB_TC_ACCEPTANCE_DATE,
                           QDateTime::currentDateTime().toString(Qt::ISODate));

    reject();
}

// Initialize static members of CloudDBCommon

QString CloudDBCommon::cloudDBBaseURL = QString("https://%1.appspot.com/v1/").arg(GC_CLOUD_DB_APP_NAME);
QVariant  CloudDBCommon::cloudDBContentType = QVariant("application/json");
QByteArray CloudDBCommon::cloudDBBasicAuth = "Basic " + QByteArray(GC_CLOUD_DB_BASIC_AUTH) ;

QList<QString> CloudDBCommon::cloudDBLangsIds = QList<QString>() << "en" << "fr" << "ja" << "pt-br" << "it" << "de" << "ru" << "cs" << "es" << "pt" << "zh-cn" << "zh-tw" << "nl" << "sv" << "xx";

QList<QString> CloudDBCommon::cloudDBLangs = QList<QString>() << QObject::tr("English") << QObject::tr("French") << QObject::tr("Japanese") << QObject::tr("Portugese (Brazil)") <<
                                             QObject::tr("Italian") << QObject::tr("German") << QObject::tr("Russian") << QObject::tr("Czech") <<
                                             QObject::tr("Spanish") << QObject::tr("Portugese") << QObject::tr("Chinese (Simplified)") << QObject::tr("Chinese (Traditional)") << QObject::tr("Dutch") << QObject::tr("Swedish") << QObject::tr("Other");

QList<QString> CloudDBCommon::cloudDBSportIds = QList<QString>() << "bike" << "run" << "swim" << "other";

QList<QString> CloudDBCommon::cloudDBSports = QList<QString>() << QObject::tr("Bike") << QObject::tr("Run") << QObject::tr("Swim") << QObject::tr("Other");

QString CloudDBCommon::cloudDBTimeFormat = "yyyy-MM-ddTHH:mm:ssZ";

bool CloudDBCommon::addCuratorFeatures = false;

// Initialize static member for CloudDBDataStatus
bool CloudDBHeader::chartHeaderStatusStale = true;
bool CloudDBHeader::userMetricHeaderStatusStale = true;

// Common (static) methods


void
CloudDBCommon::prepareRequest(QNetworkRequest &request, QString urlString, QUrlQuery *query) {

    QUrl url(urlString);
    if (query) {
        url.setQuery(query->query());
    }
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, CloudDBCommon::cloudDBContentType);
    request.setRawHeader("Authorization", CloudDBCommon::cloudDBBasicAuth);
}

bool
CloudDBCommon::replyReceivedAndOk(QNetworkReply *reply) {

    // blocking request
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {

        CloudDBCommon::processReplyStatusCodes(reply);
        return false;

    }
    return true;
}

void
CloudDBCommon::processReplyStatusCodes(QNetworkReply *reply) {

    // PROBLEM - the replies provided are in some of our case not
    // the real HTTP replies - so we do some interpretation to
    // get proper responses to the user
    // main objective is to differentiate "Over Quota" from other problems
    if (reply->error() == QNetworkReply::ServiceUnavailableError) {

        // check for "Over Quota" - checking the body GAE is providing with 2 keywords
        // which should even work when the response text is slighly changed.
        QByteArray body = reply->readAll();
        if (body.contains("503") && (body.contains("Quota"))) {
            QMessageBox::warning(0, tr("CloudDB"), QString(tr("Usage has exceeded the free quota - please try again later.")));
            return;
        }
    }

    // and here how it should work / jus interpreting what was send through HTTP
    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    if ( !statusCode.isValid() ) {
        QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem with CloudDB - please try again later.")));
        return;
    }
    int code = statusCode.toInt();
    switch (code) {
    case CloudDBCommon::APIresponseForbidden :
        QMessageBox::warning(0, tr("CloudDB"), QString(tr("Authorization problem with CloudDB - please try again later.")));
        break;
    case CloudDBCommon::APIresponseOverQuota :
        QMessageBox::warning(0, tr("CloudDB"), QString(tr("Usage has exceeded the free quota - please try again later.")));
        break;
    case CloudDBCommon::APIresponseServiceProblem :
        CloudDBStatusClient::displayCloudDBStatus();
        break;
    default:
        QByteArray body = reply->readAll();
        qDebug()<<"CloudDB error reply:"<<body;
        QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem with CloudDB - response code: %1 - please try again later."))
                             .arg(QString::number(code, 10)));
    }

}

bool
CloudDBCommon::unmarshallAPIHeaderV1(QByteArray json, QList<CommonAPIHeaderV1> *charts) {

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json, &parseError);

    // all these things should not happen and we have not valid object to return
    if (parseError.error != QJsonParseError::NoError || document.isEmpty() || document.isNull()) {
        return false;
    }

    // do we have a single object or an array ?
    if (document.isObject()) {
        CommonAPIHeaderV1 chartHeader;
        QJsonObject object = document.object();
        // handle addtional Chart Header properties - if one exists, all are expected to exist
        if (!object.value("chartView").isUndefined()) {
            chartHeader.ChartType = object.value("chartType").toString();
            chartHeader.ChartView = object.value("chartView").toString();
            chartHeader.ChartSport = object.value("chartSport").toString();
        }
        QJsonObject header = object["header"].toObject();
        unmarshallAPIHeaderV1Object(&header, &chartHeader);
        charts->append(chartHeader);

    } else if (document.isArray()) {
        QJsonArray array(document.array());
        for (int i = 0; i< array.size(); i++) {
            QJsonValue value = array.at(i);
            if (value.isObject()) {
                CommonAPIHeaderV1 chartHeader;
                QJsonObject object = value.toObject();
                // handle addtional Chart Header properties - if one exists, all are expected to exist
                if (!object.value("chartView").isUndefined()) {
                    chartHeader.ChartType = object.value("chartType").toString();
                    chartHeader.ChartView = object.value("chartView").toString();
                    chartHeader.ChartSport = object.value("chartSport").toString();
                }
                QJsonObject header = object["header"].toObject();
                unmarshallAPIHeaderV1Object(&header, &chartHeader);
                charts->append(chartHeader);
            }
        }
    }

    return true;
}

void
CloudDBCommon::sslErrors(QNetworkReply* reply ,QList<QSslError> errors)
{
    QString errorString = "";
    foreach (const QSslError e, errors ) {
        if (!errorString.isEmpty())
            errorString += ", ";
        errorString += e.errorString();
    }
    QMessageBox::warning(NULL, tr("HTTP"), tr("SSL error(s) has occurred: %1").arg(errorString));
    reply->ignoreSslErrors();
}


void
CloudDBCommon::unmarshallAPIHeaderV1Object(QJsonObject* object, CommonAPIHeaderV1* chartHeader) {

    chartHeader->Id = object->value("id").toDouble();
    chartHeader->Key = object->value("key").toString();
    chartHeader->Name = object->value("name").toString();
    chartHeader->Description = object->value("description").toString();
    chartHeader->Language = object->value("language").toString();
    chartHeader->GcVersion = object->value("gcversion").toString();
    chartHeader->LastChanged = QDateTime::fromString(object->value("lastChange").toString(), CloudDBCommon::cloudDBTimeFormat);
    chartHeader->CreatorId = object->value("creatorId").toString();
    chartHeader->Curated = object->value("curated").toBool();
    chartHeader->Deleted = object->value("deleted").toBool();

}

void
CloudDBCommon::marshallAPIHeaderV1Object(QJsonObject& json_header, CommonAPIHeaderV1& header) {

    json_header["id"] = header.Id;
    json_header["key"] = header.Key;
    json_header["name"] = header.Name;
    json_header["description"] = header.Description;
    json_header["gcversion"] = header.GcVersion;
    json_header["lastChange"] = header.LastChanged.toString(CloudDBCommon::cloudDBTimeFormat);
    json_header["creatorid"] = header.CreatorId;
    json_header["language"] = header.Language;
    json_header["curated"] = header.Curated;
    json_header["deleted"] = header.Deleted;

}


QString
CloudDBCommon::encodeHTML ( const QString& encodeMe )
{
    QString temp;

    for (int index(0); index < encodeMe.size(); index++)
    {
        QChar character(encodeMe.at(index));

        switch (character.unicode())
        {
        case '&':
            temp += "&amp;"; break;

        case '\'':
            temp += "&apos;"; break;

        case '"':
            temp += "&quot;"; break;

        case '<':
            temp += "&lt;"; break;

        case '>':
            temp += "&gt;"; break;

        case '\n':
            temp += "<br>"; break;

        default:
            temp += character;
            break;
        }
    }

    return temp;
}



bool
CloudDBHeader::writeHeaderCache(QList<CommonAPIHeaderV1>* header, CloudDBHeaderType headerType, QString cache_Dir) {

   // make sure the subdir exists
   QDir cacheDir(cache_Dir);
   if (cacheDir.exists()) {
       cacheDir.mkdir("header");
   } else {
       return false;
   }
   QString fileName;
   if (headerType == CloudDB_Chart) {
       fileName = "h_charts.dat";
   } else if (headerType == CloudDB_UserMetric) {
       fileName = "h_usermetrics.dat";
   } else {
       return false;
   }

   QFile file(cache_Dir+"/header/"+fileName);
   if (!file.open(QIODevice::WriteOnly)) return false;
   QDataStream out(&file);
   out.setVersion(QDataStream::Qt_4_6);
   // track a version to be able change data structure
   out << header_magic_string;
   out << header_cache_version;
   foreach (CommonAPIHeaderV1 h, *header) {
       out << h.Id;
       out << h.Key;
       out << h.Name;
       out << h.Description;
       out << h.Language;
       out << h.GcVersion;
       out << h.LastChanged;
       out << h.CreatorId;
       out << h.Deleted;
       out << h.Curated;
       if (headerType == CloudDB_Chart) {
           out << h.ChartSport;
           out << h.ChartType;
           out << h.ChartView;
       }
   }
   file.close();
   return true;
}

bool
CloudDBHeader::readHeaderCache(QList<CommonAPIHeaderV1>* header, CloudDBHeaderType headerType, QString cache_Dir) {

    QString fileName;
    if (headerType == CloudDB_Chart) {
        fileName = "h_charts.dat";
    } else if (headerType == CloudDB_UserMetric) {
        fileName = "h_usermetrics.dat";
    } else {
        return false;
    }
    QFile file(cache_Dir+"/header/"+fileName);
    if (!file.open(QIODevice::ReadOnly)) return false;
    QDataStream in(&file);
    in.setVersion(QDataStream::Qt_4_6);
    // track a version to be able change data structure
    int magic_string;
    int version;

    in >> magic_string;
    if (magic_string != header_magic_string) {
        // wrong file format detected / delete old cache close and exit
        // cache will be re-written based data loaded from CloudDB
        file.remove();
        return false;
    }

    in >> version;
    if (version != header_cache_version) {
       // change of version, delete old cache and exit
       file.remove();
       return false;
    }
    CommonAPIHeaderV1 h;
    while (!in.atEnd()) {
        in >> h.Id;
        in >> h.Key;
        in >> h.Name;
        in >> h.Description;
        in >> h.Language;
        in >> h.GcVersion;
        in >> h.LastChanged;
        in >> h.CreatorId;
        in >> h.Deleted;
        in >> h.Curated;
        if (headerType == CloudDB_Chart) {
            in >> h.ChartSport;
            in >> h.ChartType;
            in >> h.ChartView;
        }
        header->append(h);
    }
    file.close();
    return true;

}

bool
CloudDBHeader::getAllCachedHeader(QList<CommonAPIHeaderV1> *objectHeader, CloudDBHeaderType type, QString cache_Dir,
                                  QString url, QNetworkAccessManager* nam, QNetworkReply* reply) {

    QDateTime selectAfter;

    // first check the cache
    objectHeader->clear();
    CloudDBHeader::readHeaderCache(objectHeader, type, cache_Dir);
    if (objectHeader->size()>0) {
        // header are selected from CloudDB sorted - so the first one is always the newest one
        selectAfter = objectHeader->at(0).LastChanged.addSecs(1); // DB has Microseconds - we not - so round up to next full second
    } else {
        // we do not have charts before 2000 :-)
        selectAfter = QDateTime(QDate(2000,01,01).startOfDay());
    }

    // now get the missing headers (in bulks of xxx - since GAE is not nicely handling high single call volumes)

    QList<quint64> updatedIds;
    QList<QString> updatedStringIds;
    QList<CommonAPIHeaderV1> *retrievedHeader = new QList<CommonAPIHeaderV1>;
    QList<CommonAPIHeaderV1> *newHeader = new QList<CommonAPIHeaderV1>;

    do {

        QUrlQuery query;
        query.addQueryItem("dateFrom", selectAfter.toString(CloudDBCommon::cloudDBTimeFormat));
        QNetworkRequest request;
        CloudDBCommon::prepareRequest(request, url, &query);

        reply = nam->get(request);

        // wait for reply (synchronously) and process error codes as necessary
        if (!CloudDBCommon::replyReceivedAndOk(reply)) { 
		    delete retrievedHeader;
			delete newHeader;
            return false;
		};

        QByteArray result = reply->readAll();
        retrievedHeader->clear();
        CloudDBCommon::unmarshallAPIHeaderV1(result, retrievedHeader);
        foreach (CommonAPIHeaderV1 retrieved, *retrievedHeader) {
            // consider the retrieval order from CloudDB - the oldest header is in first position
            if (retrieved.Id > 0) {
                updatedIds.append(retrieved.Id);
            }
            if (!retrieved.Key.isEmpty()) {
                updatedStringIds.append(retrieved.Key);

            }
            newHeader->append(retrieved);
        }

        // youngest header is the last in the list
        if (retrievedHeader->size() > 0) {
            selectAfter = retrievedHeader->last().LastChanged.addSecs(1);
        }
        // NOTE - the headerSize "200" MUST BE IN SYNC WITH CLOUDDB get.limit(200)
        // by adding it here one additional call to CloudDB for the last get can be avoided

    } while (retrievedHeader->size() >= 200);

    delete retrievedHeader;

    //now merge cache data with selected data

    //first remove duplicate entries from cache which have recently changed/updated on CloudDB
    QMutableListIterator<CommonAPIHeaderV1> it(*objectHeader);
    while (it.hasNext()) {
        CommonAPIHeaderV1 header = it.next();
        if ((header.Id > 0 && updatedIds.contains(header.Id)) ||
            (header.Key.size() > 0 && updatedStringIds.contains(header.Key))) {
            // update cache (Header Cache)
            it.remove();
        }
    }

    //now we have the missing (new and updated) chart header so add the in the correct sequence
    while (!newHeader->isEmpty()) {
        objectHeader->insert(0, newHeader->takeFirst());

    }
    delete newHeader;

    // store cache for next time
    CloudDBHeader::writeHeaderCache(objectHeader, type, cache_Dir);

    return true;
}

















