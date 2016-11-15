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

#include "CloudDBStatus.h"
#include "CloudDBCommon.h"
#include "Secrets.h"

#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>


// the status code must be in sync with CloudDB GAE status codes
int CloudDBStatusClient::CloudDBStatus_Ok = 10;
int CloudDBStatusClient::CloudDBStatus_PartialFailure = 20;
int CloudDBStatusClient::CloudDBStatus_Stopped = 30;


CloudDBStatusClient::CloudDBStatusClient() {
    // only static members
}
CloudDBStatusClient::~CloudDBStatusClient() {
    // only static members

}


StatusAPIGetV1
CloudDBStatusClient::getCurrentStatus() {

    StatusAPIGetV1 retrieved;
    retrieved.Id = 0;
    retrieved.Status = 0;

    QScopedPointer<QNetworkAccessManager> l_nam(new QNetworkAccessManager());
    QNetworkReply *l_reply;

    QUrl url(CloudDBCommon::cloudDBBaseURL + "status/latest");
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, CloudDBCommon::cloudDBContentType);
    request.setRawHeader("Authorization", CloudDBCommon::cloudDBBasicAuth);
    l_reply = l_nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(l_reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (l_reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem reading latest status - please try again later")));
        return retrieved;
    };

    QByteArray result = l_reply->readAll();
    unmarshallAPIGetV1(result, &retrieved);
    return retrieved;
}


QString
CloudDBStatusClient::getCurrentStatusText(qint64 id) {

    QScopedPointer<QNetworkAccessManager> l_nam(new QNetworkAccessManager());
    QNetworkReply *l_reply;

    QUrl url(CloudDBCommon::cloudDBBaseURL + "statustext/" + QString::number(id, 10));
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, CloudDBCommon::cloudDBContentType);
    request.setRawHeader("Authorization", CloudDBCommon::cloudDBBasicAuth);
    l_reply = l_nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(l_reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (l_reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem reading status text - please try again later")));
        return "";
    };

    StatusAPIGetTextV1 textObject;
    QByteArray result = l_reply->readAll();
    unmarshallAPIGetTextV1(result, &textObject);
    return textObject.Text;

}


bool
CloudDBStatusClient::unmarshallAPIGetV1(QByteArray json, StatusAPIGetV1 *status) {

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json, &parseError);

    // all these things should not happen and we have not valid object to return
    if (parseError.error != QJsonParseError::NoError || document.isEmpty() || document.isNull()) {
        return false;
    }

    // we only get single objects here
    if (document.isObject()) {
        QJsonObject object = document.object();
        status->Id = object.value("id").toDouble();
        status->Status = object.value("status").toInt();
        status->changeDate = QDateTime::fromString(object.value("changeDate").toString(), CloudDBCommon::cloudDBTimeFormat);
        return true;
    }
    return false;
}

bool
CloudDBStatusClient::unmarshallAPIGetTextV1(QByteArray json, StatusAPIGetTextV1 *text) {

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json, &parseError);

    // all these things should not happen and we have not valid object to return
    if (parseError.error != QJsonParseError::NoError || document.isEmpty() || document.isNull()) {
        return false;
    }

    // we only get single objects here
    if (document.isObject()) {
        QJsonObject object = document.object();
        text->Id = object.value("id").toDouble();
        text->Text = object.value("text").toString();
        return true;
    }
    return false;
}



void
CloudDBStatusClient::displayCloudDBStatus() {

    StatusAPIGetV1 status = getCurrentStatus();
    if (status.Status == CloudDBStatusClient::CloudDBStatus_Ok) {
        QMessageBox::information(0, tr("CloudDB Status"), QString(tr("<b><big>Ok</b></big><br><br>All Services should be operational - no problems known.")));
    } else if (status.Status == CloudDBStatusClient::CloudDBStatus_PartialFailure) {
        QString text = getCurrentStatusText(status.Id);
        QMessageBox::warning(0, tr("CloudDB Status"), QString(tr("<b><big>Limited Service</b></big><br>Sorry - some CloudDB services are not operational.<br><br><b>More information:</b><br>%1")).arg(text));
    } else {
        QString text = getCurrentStatusText(status.Id);
        QMessageBox::critical(0, tr("CloudDB Status"), QString(tr("<b><big>Service outage</b></big><br>Sorry - CloudDB services are not operational.<br><br><b>More information:</b><br>%1")).arg(text));
    }


}











