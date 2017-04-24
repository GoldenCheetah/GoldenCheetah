/*
 * Copyright (c) 2015 Luca Rasina <luca.rasina@sphtechnology.ch>
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

#include "SportPlusHealthUploader.h"
#include "ShareDialog.h"
#include "Athlete.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"
#include "mvjson.h"

#include <stdio.h>

// access to metrics
#include "TcxRideFile.h"

const QString SPH_URL( "http://www.sportplushealth.com/sport/en/api/1" );

SportPlusHealthUploader::SportPlusHealthUploader(Context *context, RideItem *ride, ShareDialog *parent ) :
    ShareDialogUploader( tr("SportPlusHealth"), context, ride, parent)
{
    exerciseId = ride->ride()->getTag("SphExercise", "");

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(dispatchReply(QNetworkReply*)));
}

bool
SportPlusHealthUploader::canUpload( QString &err )
{
    QString username = appsettings->cvalue(context->athlete->cyclist, GC_SPORTPLUSHEALTHUSER, "").toString();

    if( username.length() > 0 ){
        return true;
    }

    err = tr("Cannot upload to SportPlusHealth without credentials. Check Settings");
    return false;
}

bool
SportPlusHealthUploader::wasUploaded()
{
    return exerciseId.length() > 0;
}

void
SportPlusHealthUploader::upload()
{
    requestUpload();
}

void
SportPlusHealthUploader::requestUpload()
{
    parent->progressLabel->setText(tr("sending to SportPlusHealth..."));
    parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);

    QString username = appsettings->cvalue(context->athlete->cyclist, GC_SPORTPLUSHEALTHUSER).toString();
    QString password = appsettings->cvalue(context->athlete->cyclist, GC_SPORTPLUSHEALTHPASS).toString();

    //Building the message content
    QHttpMultiPart *body = new QHttpMultiPart( QHttpMultiPart::FormDataType );

    //Including the optional session name
    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"session_name\""));
    textPart.setBody(QByteArray(insertedName.toLatin1()));
    body->append(textPart);

    //Including the content data type
    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"format\""));
    dataTypePart.setBody("tcx");
    body->append(dataTypePart);

    //Including file in the request
    TcxFileReader reader;
    QByteArray file = reader.toByteArray(context, ride->ride(), parent->altitudeChk->isChecked(), parent->powerChk->isChecked(), parent->heartrateChk->isChecked(), parent->cadenceChk->isChecked());
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"datafile\"; filename=\"sph_file.tcx\"; type=\"text/xml\""));
    filePart.setBody(file);
    body->append(filePart);

    //Sending the authenticated post request to the API
    parent->progressBar->setValue(parent->progressBar->value()+20/parent->shareSiteCount);
    QUrl url(SPH_URL + "/" + username + "/importGC");
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("Authorization", "Basic " + QByteArray(QString("%1:%2").arg(username).arg(password).toLatin1()).toBase64());
    networkMgr.post(request, body);
    parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);
}

/*////////////////////
/// QXML HANDLER
////////////////////*/
class SphParser : public QXmlDefaultHandler
{
public:
    friend class SportPlusHealthUploader;

    bool startElement( const QString&, const QString&, const QString&, const QXmlAttributes& )
    {
        cdata = "";
        return true;
    }

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
    }

protected:
    QString cdata;
    QString error;
};

/*////////////////////
/// RESULTS HANDLER
////////////////////*/
void
SportPlusHealthUploader::dispatchReply( QNetworkReply *reply )
{
    if( reply->error() != QNetworkReply::NoError ){
        parent->progressLabel->setText(tr("error uploading to SportPlusHealth"));
        parent->errorLabel->setText( tr("request failed: ") + reply->errorString() );

        eventLoop.quit();
        return;
    }

    QVariant status = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute );
    if( ! status.isValid() || status.toInt() != 200 ){
        QVariant msg( reply->attribute( QNetworkRequest::HttpReasonPhraseAttribute ) );

        parent->progressLabel->setText(tr("error uploading to SportPlusHealth"));
        parent->errorLabel->setText( tr("request failed, Server response: %1 %2")
            .arg(status.toInt())
            .arg(msg.toString()) );

        eventLoop.quit();
        return;
    }

    finishUpload( reply );
}

class SphUploadParser : public SphParser
{
public:
    friend class SportPlusHealthUploader;

    bool endElement( const QString& a, const QString&b, const QString& qName )
    {
        if( qName == "id" ){
            id = cdata;
            return true;

        }

        return SphParser::endElement( a, b, qName );
    };

protected:
    QString id;

};

void
SportPlusHealthUploader::finishUpload(QNetworkReply *reply)
{
    //Parsing JSON server reply
    QString strReply = (QString)reply->readAll();
    MVJSONReader jsonResponse(string(strReply.toLatin1()));
    if(jsonResponse.root && jsonResponse.root->hasField("success") && jsonResponse.root->hasField("error_code")) {
        jsonResponseSuccess = jsonResponse.root->getFieldBool("success");
        jsonResponseErrorCode = jsonResponse.root->getFieldInt("error_code");
    } else {
        jsonResponseSuccess = false;
        jsonResponseErrorCode = -1;
    }
    reply->deleteLater();

    if(!jsonResponseSuccess) {
       parent->progressLabel->setText(tr("error uploading to SportPlusHealth"));
       parent->errorLabel->setText(tr("failed to upload file (cod. %1)").arg(jsonResponseErrorCode));
       ride->ride()->setTag("SphExercise", exerciseId );
       ride->setDirty(true);

       eventLoop.quit();
       return;
    }

    parent->progressBar->setValue(parent->progressBar->value()+60/parent->shareSiteCount);
    parent->progressLabel->setText(tr("successfully uploaded to SportPlusHealth"));
    eventLoop.quit();
}
