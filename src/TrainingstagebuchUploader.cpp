/*
 * Copyright (c) 2012 Rainer Clasen <bj@zuto.de>
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

#include "TrainingstagebuchUploader.h"
#include "ShareDialog.h"
#include "Athlete.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"

#include <stdio.h>

// access to metrics
#include "PwxRideFile.h"

/*
 * - reuse single QNetworkAccessManager for all requests to allow
 * connection re-use.
 *
 * - replies must be handled differently - based on request, but there's
 * just a single mgr.finish() handler.
 *
 * - can't connect() to QNetworkReply::finish, as there's a
 * slight chance for a race condition: request finished between call to
 * mgr.post() and connect(). Therefore have to connect(QNetworkAccessManager::finish)
 *
 * - can't subclass QNetworkRequest and add attribute, as
 * QNetworkAccessManager only takes a reference and clones a plain
 * QNetworkRequest from it.
 *
 * - so... hack around this with currentRequest + dispatchRequest TODO: fix
 */

const QString TTB_URL( "http://trainingstagebuch.org" );

class TtbParser : public QXmlDefaultHandler
{
public:
    friend class TrainingstagebuchUploader;

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


TrainingstagebuchUploader::TrainingstagebuchUploader(Context *context, RideItem *ride, ShareDialog *parent ) :
    ShareDialogUploader( tr("Trainingstagebuch.org"), context, ride, parent),
    proMember( false )
{
    exerciseId = ride->ride()->getTag("TtbExercise", "");

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this,
        SLOT(dispatchReply(QNetworkReply*)));
}

bool
TrainingstagebuchUploader::canUpload( QString &err )
{
    QString username = appsettings->cvalue(context->athlete->cyclist, GC_TTBUSER).toString();

    if( username.length() > 0 ){
        return true;
    }

    err = tr("Cannot upload to Trainingstagebuch without credentials. Check Settings");
    return false;
}

bool
TrainingstagebuchUploader::wasUploaded()
{
    return exerciseId.length() > 0;
}


void
TrainingstagebuchUploader::upload()
{
    uploadSuccessful = false;
    requestSettings();

    if( !uploadSuccessful ){
        parent->progressLabel->setText(tr("Error uploading to Trainingstagebuch.org"));

    } else {
        parent->progressLabel->setText(tr("successfully uploaded to Trainingstagebuch.org as %1")
            .arg(exerciseId));

        ride->ride()->setTag("TtbExercise", exerciseId );
        ride->setDirty(true);
    }
}

void
TrainingstagebuchUploader::requestSettings()
{
    parent->progressLabel->setText(tr("getting Settings from Trainingstagebuch.org..."));

    currentRequest = reqSettings;

    QString username = appsettings->cvalue(context->athlete->cyclist, GC_TTBUSER).toString();
    QString password = appsettings->cvalue(context->athlete->cyclist, GC_TTBPASS).toString();

#if QT_VERSION > 0x050000
    QUrlQuery urlquery( TTB_URL + "/settings/list" );
#else
    QUrl urlquery( TTB_URL + "/settings/list" );
#endif
    urlquery.addQueryItem( "view", "xml" );
    urlquery.addQueryItem( "user", username );
    urlquery.addQueryItem( "pass", password );

#if QT_VERSION > 0x050000
    QUrl url;
    url.setQuery(urlquery);
    QNetworkRequest request = QNetworkRequest(url);
#else
    QNetworkRequest request = QNetworkRequest(urlquery);
#endif
    request.setRawHeader( "Accept-Encoding", "identity" );
    request.setRawHeader( "Accept", "application/xml" );
    request.setRawHeader( "Accept-Charset", "utf-8" );

    networkMgr.get( request );
    parent->progressBar->setValue(parent->progressBar->value()+5/parent->shareSiteCount);
    eventLoop.exec();
}

void
TrainingstagebuchUploader::requestSession()
{
    parent->progressLabel->setText(tr("getting new Trainingstagebuch.org Session..."));

    currentRequest = reqSession;

    QString username = appsettings->cvalue(context->athlete->cyclist, GC_TTBUSER).toString();
    QString password = appsettings->cvalue(context->athlete->cyclist, GC_TTBPASS).toString();

#if QT_VERSION > 0x050000
    QUrlQuery urlquery( TTB_URL + "/login/sso" );
#else
    QUrl urlquery( TTB_URL + "/login/sso" );
#endif
    urlquery.addQueryItem( "view", "xml" );
    urlquery.addQueryItem( "user", username );
    urlquery.addQueryItem( "pass", password );

#if QT_VERSION > 0x050000
    QUrl url;
    url.setQuery(urlquery);
    QNetworkRequest request = QNetworkRequest(url);
#else
    QNetworkRequest request = QNetworkRequest(urlquery);
#endif

    request.setRawHeader( "Accept-Encoding", "identity" );
    request.setRawHeader( "Accept", "application/xml" );
    request.setRawHeader( "Accept-Charset", "utf-8" );

    networkMgr.get( request );
    parent->progressBar->setValue(parent->progressBar->value()+5/parent->shareSiteCount);
}

void
TrainingstagebuchUploader::requestUpload()
{
    assert(sessionId.length() > 0 );

    parent->progressLabel->setText(tr("preparing Trainingstagebuch.org data ..."));

    QHttpMultiPart *body = new QHttpMultiPart( QHttpMultiPart::FormDataType );

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
    QVariant("form-data; name=\"upload_submit\""));
    textPart.setBody("hrm");
    body->append( textPart );


    QString fname = context->athlete->home->temp().absoluteFilePath(".ttbupload.pwx" );
    QFile *uploadFile = new QFile( fname );
    uploadFile->setParent(body);

    PwxFileReader reader;
    reader.writeRideFile(context, ride->ride(), *uploadFile );
    parent->progressBar->setValue(parent->progressBar->value()+20/parent->shareSiteCount);

    int limit = proMember
        ? 8 * 1024 * 1024
        : 4 * 1024 * 1024;
    if( uploadFile->size() >= limit ){
        parent->errorLabel->setText(tr("temporary file too large for upload: %1 > %1 bytes")
            .arg(uploadFile->size())
            .arg(limit) );

        eventLoop.quit();
        return;
    }

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
    QVariant("application/occtet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
    QVariant("form-data; name=\"file\"; filename=\"gc-upload-ttb.pwx\""));
    uploadFile->open(QIODevice::ReadOnly);
    filePart.setBodyDevice(uploadFile);
    body->append( filePart );


    parent->progressLabel->setText(tr("sending to Trainingstagebuch.org ..."));

    currentRequest = reqUpload;

#if QT_VERSION > 0x050000
    QUrlQuery urlquery( TTB_URL + "/file/upload" );
#else
    QUrl urlquery( TTB_URL + "/file/upload" );
#endif
    urlquery.addQueryItem( "view", "xml" );
    urlquery.addQueryItem( "sso", sessionId );

#if QT_VERSION > 0x050000
    QUrl url;
    url.setQuery(urlquery);
    QNetworkRequest request = QNetworkRequest(url);
#else
    QNetworkRequest request = QNetworkRequest(urlquery);
#endif

    request.setRawHeader( "Accept-Encoding", "identity" );
    request.setRawHeader( "Accept", "application/xml" );
    request.setRawHeader( "Accept-Charset", "utf-8" );

    QNetworkReply *reply = networkMgr.post( request, body );
    body->setParent( reply );
}

void
TrainingstagebuchUploader::dispatchReply( QNetworkReply *reply )
{
    if( reply->error() != QNetworkReply::NoError ){
        parent->errorLabel->setText( tr("request failed: ")
            + reply->errorString() );

        eventLoop.quit();
        return;
    }

    QVariant status = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    if( ! status.isValid() || status.toInt() != 200 ){
        QVariant msg( reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ) );

        parent->errorLabel->setText( tr("request failed, Server response: %1 %2")
            .arg(status.toInt())
            .arg(msg.toString()) );

        eventLoop.quit();

        return;
    }

    switch( currentRequest ){
      case reqSettings:
        finishSettings( reply );
        break;

      case reqSession:
        finishSession( reply );
        break;

      case reqUpload:
        finishUpload( reply );
        break;
    }
}

class TtbSettingsParser : public TtbParser
{
public:
    friend class TrainingstagebuchUploader;

    TtbSettingsParser() :
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

        return TtbParser::endElement( a, b, qName );
    };

protected:
    bool    pro;
    QString session;

    QRegExp reFalse;
};

void
TrainingstagebuchUploader::finishSettings(QNetworkReply *reply)
{
    parent->progressBar->setValue(parent->progressBar->value()+5/parent->shareSiteCount);

    TtbSettingsParser handler;
    QXmlInputSource source( reply );

    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );

    if( ! reader.parse( source ) ){
        parent->errorLabel->setText(tr("failed to parse Settings response: ")
            +handler.errorString());

        eventLoop.quit();
        return;
    }

    if( handler.error.length() > 0 ){
        parent->errorLabel->setText(tr("failed to get settings: ")
            +handler.error );

        eventLoop.quit();
        return;
    }

    sessionId = handler.session;
    proMember = handler.pro;

    if( sessionId.length() == 0 ){
        requestSession();
    } else {
        requestUpload();
    }
}

class TtbSessionParser : public TtbParser
{
public:
    friend class TrainingstagebuchUploader;

    bool endElement( const QString& a, const QString&b, const QString& qName )
    {
        if( qName == "session" ){
            session = cdata;
            return true;

        }

        return TtbParser::endElement( a, b, qName );
    };

protected:
    QString session;

};

void
TrainingstagebuchUploader::finishSession(QNetworkReply *reply)
{
    parent->progressBar->setValue(parent->progressBar->value()+5/parent->shareSiteCount);

    TtbSessionParser handler;
    QXmlInputSource source( reply );

    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );

    if( ! reader.parse( source ) ){
        parent->errorLabel->setText(tr("failed to parse Session response: ")
            +handler.errorString());

        eventLoop.quit();
        return;
    }

    if( handler.error.length() > 0 ){
        parent->errorLabel->setText(tr("failed to get new session: ")
            +handler.error );

        eventLoop.quit();
        return;
    }

    sessionId = handler.session;

    if( sessionId.length() == 0 ){
        parent->errorLabel->setText(tr("got empty session"));

        eventLoop.quit();
        return;
    }

    requestUpload();
}

class TtbUploadParser : public TtbParser
{
public:
    friend class TrainingstagebuchUploader;

    bool endElement( const QString& a, const QString&b, const QString& qName )
    {
        if( qName == "id" ){
            id = cdata;
            return true;

        }

        return TtbParser::endElement( a, b, qName );
    };

protected:
    QString id;

};

void
TrainingstagebuchUploader::finishUpload(QNetworkReply *reply)
{
    parent->progressBar->setValue(parent->progressBar->value()+60/parent->shareSiteCount);

    TtbUploadParser handler;
    QXmlInputSource source( reply );

    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );

    if( ! reader.parse( source ) ){
        parent->errorLabel->setText(tr("failed to parse upload response: ")
            +handler.errorString());

        eventLoop.quit();
        return;
    }

    if( handler.error.length() > 0 ){
        parent->errorLabel->setText(tr("failed to upload file: ")
            +handler.error );

        eventLoop.quit();
        return;
    }

    exerciseId = handler.id;

    if( exerciseId.length() == 0 ){
        parent->errorLabel->setText(tr("got empty exercise"));

        eventLoop.quit();
        return;
    }

    uploadSuccessful = true;
    eventLoop.quit();
}



