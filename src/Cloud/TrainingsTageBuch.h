/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_TrainingsTageBuch_h
#define GC_TrainingsTageBuch_h
#include <QImage>

// Cloud Services and HTTP
#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>

// XML parsing
#include <QUrl>
#include <QHttpMultiPart>
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>
#include <QNetworkReply>
#include <QUrlQuery>

class TrainingsTageBuch : public CloudService {

    Q_OBJECT

    public:

        QString id() const { return "TrainingsTageBuch"; }
        QString uiName() const { return tr("Trainingstagebuch"); }
        QString description() const { return (tr("Upload to your online and mobile training log.")); }
        QImage logo() const { return QImage(":images/services/trainingstagebuch.png"); }

        TrainingsTageBuch(Context *context);
        CloudService *clone(Context *context) { return new TrainingsTageBuch(context); }
        ~TrainingsTageBuch();

        // upload only and authenticates with a user and password
        int capabilities() const { return UserPass | Upload; }

        // open/connect and close/disconnect
        bool open(QStringList &errors);
        bool close();

        // write a file
        bool writeFile(QByteArray &data, QString remotename, RideFile *ride);

    public slots:

        // sending data
        void writeFileCompleted();

    private:
        Context *context;
        QNetworkAccessManager *nam;
        QNetworkReply *reply;
        CloudServiceEntry *root_;

        QString sessionId;
        bool proMember;

        QMap<QNetworkReply*, QByteArray*> buffers;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};

class TTBParser : public QXmlDefaultHandler
{
public:
    friend class TrainingsTageBuch;

    bool startElement( const QString&, const QString&, const QString&,
        const QXmlAttributes& )
    {
        cdata = "";
        return true;
    };

    bool endElement( const QString&, const QString&, const QString& qName )
    {
        if( qName == "error" ){
            error = cdata;
            return true;
        }
        return true;
    }

    bool characters( const QString& str)
    {
        cdata += str;
        return true;
    };

protected:
    QString cdata;
    QString error;
};

class TTBSettingsParser : public TTBParser
{
public:
    friend class TrainingsTageBuch;

    TTBSettingsParser() :
        pro(false),
        reFalse("\\s*(0|false|no|)\\s*",Qt::CaseInsensitive )
    {};

    bool endElement( const QString& a, const QString&b, const QString& qName )
    {
        if( qName == "pro" ){
            pro = ! reFalse.exactMatch(cdata);
            return true;

        } else if( qName == "session" ){
            session = cdata;
            return true;

        }

        return TTBParser::endElement( a, b, qName );
    };

protected:
    bool    pro;
    QString session;

    QRegExp reFalse;
};

class TTBSessionParser : public TTBParser
{
public:
    friend class TrainingsTageBuch;

    bool endElement( const QString& a, const QString&b, const QString& qName )
    {
        if( qName == "session" ){
            session = cdata;
            return true;

        }

        return TTBParser::endElement( a, b, qName );
    };

protected:
    QString session;

};

class TTBUploadParser : public TTBParser
{
public:
    friend class TrainingsTageBuch;

    bool endElement( const QString& a, const QString&b, const QString& qName )
    {
        if( qName == "id" ){
            id = cdata;
            return true;

        }

        return TTBParser::endElement( a, b, qName );
    };

protected:
    QString id;

};
#endif
