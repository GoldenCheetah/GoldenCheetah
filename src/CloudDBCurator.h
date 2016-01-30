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

#ifndef CLOUDDBCURATOR_H
#define CLOUDDBCURATOR_H

#include <QNetworkAccessManager>
#include <QNetworkReply>

struct CuratorAPIv1 {
    qint64  Id;
    QString CuratorId;
    QString Nickname;
    QString Email;
};


class CloudDBCuratorClient : public QObject
{
    Q_OBJECT

public:

    CloudDBCuratorClient();
    ~CloudDBCuratorClient();

    bool isCurator(QString uuid);

private:

    QNetworkAccessManager* g_nam;
    QNetworkReply *g_reply;

    QString  g_curator_url_base;

    static bool unmarshallAPIv1(QByteArray , QList<CuratorAPIv1>* );
    static void unmarshallAPIv1Object(QJsonObject* , CuratorAPIv1* );

};



#endif // CLOUDDBCORE_H
