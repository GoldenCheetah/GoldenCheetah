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

#ifndef GC_Velohero_h
#define GC_Velohero_h

#include "CloudService.h"
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QImage>

// parsing xml responses
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>

class Velohero : public CloudService {

    Q_OBJECT

    public:

        QString id() const { return "Velohero"; }
        QString uiName() const { return tr("Velohero"); }
        QString description() const { return (tr("Upload your rides to the independent and free cycling training log")); }
        QImage logo() const { return QImage(":images/services/velohero.png"); }

        Velohero(Context *context);
        CloudService *clone(Context *context) { return new Velohero(context); }
        ~Velohero();

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
        QString sessionId;
        CloudServiceEntry *root_;

        QMap<QNetworkReply*, QByteArray*> buffers;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};

// parse upload response
class VeloHeroParser : public QXmlDefaultHandler
{
    friend class Velohero;

public:

    bool startElement(const QString&, const QString&, const QString&, const QXmlAttributes&) {
        cdata = "";
        return true;
    };

    bool endElement(const QString&, const QString&, const QString& qName) {
        if(qName == "error"){
            error = cdata;
            return true;
        }
        return true;
    }

    bool characters(const QString& str)
    {
        cdata += str;
        return true;
    };

protected:

    QString cdata;
    QString error;
};

// parsing the response from a session id request
class VeloHeroSessionParser : public VeloHeroParser
{
    friend class Velohero;

public:

    bool endElement(const QString& a, const QString&b, const QString& qName)
    {
        if(qName == "session"){
            session = cdata;
            return true;
        }
        return VeloHeroParser::endElement(a, b, qName);
    };

protected:
    QString session;
};

class VeloHeroPostParser : public VeloHeroParser
{
public:
    friend class Velohero;

    bool endElement( const QString& a, const QString&b, const QString& qName )
    {
        if( qName == "id" ){
            id = cdata;
            return true;

        }

        return VeloHeroParser::endElement(a, b, qName);
    };

protected:
    QString id;
};
#endif
