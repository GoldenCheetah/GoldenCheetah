/*
 * Copyright (c) 2012 Rainer Clasen <bj@zuto.de>
 * Copyright (c) 2014 Nils Knieling copied from TtbDialog.cpp
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

#include "VeloHeroDialog.h"
#include "Athlete.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"

#include <stdio.h>

// acccess to metrics
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

const QString VELOHERO_URL( "http://app.velohero.com" );

class VeloHeroParser : public QXmlDefaultHandler
{
public:
    friend class VeloHeroDialog;

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


VeloHeroDialog::VeloHeroDialog(Context *context, RideItem *item) :
    context(context),
    ride( item )
{
    exerciseId = ride->ride()->getTag("VeloHeroExercise", "");

    setWindowTitle("Velo Hero");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QVBoxLayout *progressLayout = new QVBoxLayout;
    progressLabel = new QLabel("", this);
    progressBar = new QProgressBar(this);
    progressLayout->addWidget(progressBar);
    progressLayout->addWidget(progressLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    closeButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(progressLayout);
    mainLayout->addLayout(buttonLayout);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(closeClicked()));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this,
        SLOT(dispatchReply(QNetworkReply*)));

    show();

    uploadToVeloHero();
}

void
VeloHeroDialog::uploadToVeloHero()
{
    // TODO: check for user/pw

    // TODO: ask for overwrite

    progressBar->setValue(0);
    requestSession();

}

void
VeloHeroDialog::closeClicked()
{
    reject();
}

void
VeloHeroDialog::requestSession()
{
    progressLabel->setText(tr("getting new session..."));

    currentRequest = reqSession;

    QString username = appsettings->cvalue(context->athlete->cyclist, GC_VELOHEROUSER).toString();
    QString password = appsettings->cvalue(context->athlete->cyclist, GC_VELOHEROPASS).toString();

#if QT_VERSION > 0x050000
    QUrlQuery urlquery( VELOHERO_URL + "/sso" );
#else
    QUrl urlquery( VELOHERO_URL + "/sso" );
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
    progressBar->setValue(6);
}

void
VeloHeroDialog::requestUpload()
{
    assert(sessionId.length() > 0 );

    progressLabel->setText(tr("preparing upload ..."));

    QHttpMultiPart *body = new QHttpMultiPart( QHttpMultiPart::FormDataType );

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader,
    QVariant("form-data; name=\"upload_submit\""));
    textPart.setBody("hrm");
    body->append( textPart );

    QString fname = context->athlete->home->temp().absoluteFilePath(".velohero-upload.pwx" );
    QFile *uploadFile = new QFile( fname );
    uploadFile->setParent(body);

    PwxFileReader reader;
    reader.writeRideFile(context, ride->ride(), *uploadFile );
    progressBar->setValue(12);

    int limit = 16777216; // 16MB
    if( uploadFile->size() >= limit ){
        progressLabel->setText(tr("temporary file too large for upload: %1 > %1 bytes")
            .arg(uploadFile->size())
            .arg(limit) );
        closeButton->setText(tr("&Close"));
        return;
    }

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
    QVariant("application/occtet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
    QVariant("form-data; name=\"file\"; filename=\"gc-upload-velohero.pwx\""));
    uploadFile->open(QIODevice::ReadOnly);
    filePart.setBodyDevice(uploadFile);
    body->append( filePart );


    progressLabel->setText(tr("uploading ..."));

    currentRequest = reqUpload;

#if QT_VERSION > 0x050000
    QUrlQuery urlquery( VELOHERO_URL + "/upload/file" );
#else
    QUrl urlquery( VELOHERO_URL + "/upload/file" );
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

    connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this,
        SLOT(uploadProgress(qint64,qint64)));
}

void
VeloHeroDialog::uploadProgress(qint64 sent, qint64 total)
{
    if( total < 0 )
        return;

    progressBar->setValue( 12.0 + sent / total * 88 );
}

void
VeloHeroDialog::dispatchReply( QNetworkReply *reply )
{
    if( reply->error() != QNetworkReply::NoError ){
        progressLabel->setText( tr("request failed: ")
            + reply->errorString() );
        closeButton->setText(tr("&Close"));
        return;
    }

    QVariant status = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    if( ! status.isValid() || status.toInt() != 200 ){
        QVariant msg( reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ) );

        progressLabel->setText( tr("request failed, server response: %1 %2")
            .arg(status.toInt())
            .arg(msg.toString()) );
        closeButton->setText(tr("&Close"));

        return;
    }

    switch( currentRequest ){
      case reqSession:
        finishSession( reply );
        break;

      case reqUpload:
        finishUpload( reply );
        break;
    }
}

class VeloHeroSessionParser : public VeloHeroParser
{
public:
    friend class VeloHeroDialog;

    bool endElement( const QString& a, const QString&b, const QString& qName )
    {
        if( qName == "session" ){
            session = cdata;
            return true;

        }

        return VeloHeroParser::endElement( a, b, qName );
    };

protected:
    QString session;

};

void
VeloHeroDialog::finishSession(QNetworkReply *reply)
{
    progressBar->setValue(8);

    VeloHeroSessionParser handler;
    QXmlInputSource source( reply );

    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );

    if( ! reader.parse( source ) ){
        progressLabel->setText(tr("failed to parse session response: ")
            +handler.errorString());
        closeButton->setText(tr("&Close"));
        return;
    }

    if( handler.error.length() > 0 ){
        progressLabel->setText(tr("failed to get new session: ")
            +handler.error );
        closeButton->setText(tr("&Close"));
        return;
    }

    sessionId = handler.session;

    if( sessionId.length() == 0 ){
        progressLabel->setText(tr("got empty session"));
        closeButton->setText(tr("&Close"));
        return;
    }

    requestUpload();
}

class VeloHeroUploadParser : public VeloHeroParser
{
public:
    friend class VeloHeroDialog;

    bool endElement( const QString& a, const QString&b, const QString& qName )
    {
        if( qName == "id" ){
            id = cdata;
            return true;

        }

        return VeloHeroParser::endElement( a, b, qName );
    };

protected:
    QString id;

};

void
VeloHeroDialog::finishUpload(QNetworkReply *reply)
{
    progressBar->setValue(100);
    closeButton->setText(tr("&Close"));

    VeloHeroUploadParser handler;
    QXmlInputSource source( reply );

    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );

    if( ! reader.parse( source ) ){
        progressLabel->setText(tr("failed to parse upload response: ")
            +handler.errorString());
        return;
    }

    if( handler.error.length() > 0 ){
        progressLabel->setText(tr("failed to upload file: ")
            +handler.error );
        return;
    }

    exerciseId = handler.id;

    if( exerciseId.length() == 0 ){
        progressLabel->setText(tr("got empty exercise"));
        return;
    }

    progressLabel->setText(tr("successfully uploaded as %1")
        .arg(exerciseId));

    ride->ride()->setTag("VeloHeroExercise", exerciseId );
    ride->setDirty(true);

    accept();
}



