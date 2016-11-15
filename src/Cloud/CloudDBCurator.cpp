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

#include "CloudDBCurator.h"
#include "CloudDBCommon.h"

#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>



CloudDBCuratorClient::CloudDBCuratorClient()
{
    g_nam = new QNetworkAccessManager(this);

    // common definitions used

    g_curator_url_base = CloudDBCommon::cloudDBBaseURL + QString("curator");

}
CloudDBCuratorClient::~CloudDBCuratorClient() {
    delete g_nam;
}

bool
CloudDBCuratorClient::isCurator(QString uuid) {

    QUrlQuery query;
    query.addQueryItem("curatorId", uuid);
    QUrl url(g_curator_url_base);
    url.setQuery(query);
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, CloudDBCommon::cloudDBContentType);
    request.setRawHeader("Authorization", CloudDBCommon::cloudDBBasicAuth);
    g_reply = g_nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(g_reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (g_reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(0, tr("CloudDB"), QString(tr("Technical problem reading curator list - please try again later")));
        return false;
    };

    QByteArray result = g_reply->readAll();
    QList<CuratorAPIv1> *retrievedCurator = new QList<CuratorAPIv1>;
    unmarshallAPIv1(result, retrievedCurator);

    if (retrievedCurator->size()>0) {
        return true;
    }
    return false;
}

bool
CloudDBCuratorClient::unmarshallAPIv1(QByteArray json, QList<CuratorAPIv1> *curatorList) {

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json, &parseError);

    // all these things should not happen and we have not valid object to return
    if (parseError.error != QJsonParseError::NoError || document.isEmpty() || document.isNull()) {
        return false;
    }

    // do we have a single object or an array ?
    if (document.isObject()) {
        CuratorAPIv1 curator;
        QJsonObject object = document.object();
        unmarshallAPIv1Object(&object, &curator);
        curatorList->append(curator);

    } else if (document.isArray()) {
        QJsonArray array(document.array());
        for (int i = 0; i< array.size(); i++) {
            QJsonValue value = array.at(i);
            if (value.isObject()) {
                CuratorAPIv1 curator;
                QJsonObject object = document.object();
                unmarshallAPIv1Object(&object, &curator);
                curatorList->append(curator);
            }
        }
    }

    return true;
}

void
CloudDBCuratorClient::unmarshallAPIv1Object(QJsonObject* object, CuratorAPIv1* curator) {

    curator->Id = object->value("id").toDouble();
    curator->CuratorId = object->value("curatorId").toString();
    curator->Nickname = object->value("nickname").toString();
    curator->Email = object->value("email").toString();

}








