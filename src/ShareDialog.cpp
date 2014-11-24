/*
 * Copyright (c) 2013 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "ShareDialog.h"
#include "Athlete.h"
#include "Settings.h"
#include <QUrl>
#include <QHttpMultiPart>
#include "mvjson.h"
#include "TimeUtils.h"
#include "Units.h"
#include "VeloHeroUploader.h"
#include "TrainingstagebuchUploader.h"

// access to metrics
#include "MetricAggregator.h"
#include "RideMetric.h"
#include "DBAccess.h"
#include "TcxRideFile.h"

#include <zlib.h>

bool
ShareDialogUploader::canUpload( QString &err )
{
    (void)err;

    return true;
}

bool
ShareDialogUploader::wasUploaded()
{
    return false;
}

//
// Utility function to create a QByteArray of data in GZIP format
// This is essentially the same as qCompress but creates it in
// GZIP format (with recquisite headers) instead of ZLIB's format
// which has less filename info in the header
//
static QByteArray zCompress(const QByteArray &source)
{
    // int size is source.size()
    // const char *data is source.data()
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    // note that (15+16) below means windowbits+_16_ adds the gzip header/footer
    deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, (15+16), 8, Z_DEFAULT_STRATEGY);

    // input data
    strm.avail_in = source.size();
    strm.next_in = (Bytef *)source.data();

    // output data - on stack not heap, will be released
    QByteArray dest(source.size()/2, '\0'); // should compress by 50%, if not don't bother

    strm.avail_out = source.size()/2;
    strm.next_out = (Bytef *)dest.data();

    // now compress!
    deflate(&strm, Z_FINISH);

    // return byte array on the stack
    return QByteArray(dest.data(), (source.size()/2) - strm.avail_out);
}

ShareDialog::ShareDialog(Context *context, RideItem *item) :
    context(context)
{
    setWindowTitle(tr("Share your ride"));

    // make the dialog a resonable size
    setMinimumWidth(550);
    setMinimumHeight(400);

    ride = item;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGroupBox *groupBox1 = new QGroupBox(tr("Choose which sites you wish to share on: "));

    QGridLayout *vbox1 = new QGridLayout();
    unsigned int col = 0;

    QString err;

#ifdef GC_HAVE_LIBOAUTH
    stravaUploader = new StravaUploader(context, ride, this);
    stravaChk = new QCheckBox(tr("Strava"));
    if( ! stravaUploader->canUpload( err ) ){
        stravaChk->setEnabled( false );
    } else if( ! stravaUploader->wasUploaded() ){
        stravaChk->setChecked( true );
    }
    vbox1->addWidget(stravaChk,0,col++);
#endif

    rideWithGpsUploader = new RideWithGpsUploader(context, ride, this);
    rideWithGPSChk = new QCheckBox(tr("Ride With GPS"));
    if( ! rideWithGpsUploader->canUpload( err ) ){
        rideWithGPSChk->setEnabled( false );
    } else if( ! rideWithGpsUploader->wasUploaded() ){
        rideWithGPSChk->setChecked( true );
    }
    vbox1->addWidget(rideWithGPSChk,0,col++);

#ifdef GC_HAVE_LIBOAUTH
    cyclingAnalyticsUploader = new CyclingAnalyticsUploader(context, ride, this);
    cyclingAnalyticsChk = new QCheckBox(tr("Cycling Analytics"));
    if( ! cyclingAnalyticsUploader->canUpload( err ) ){
        cyclingAnalyticsChk->setEnabled( false );
    } else if( ! cyclingAnalyticsUploader->wasUploaded() ){
        cyclingAnalyticsChk->setChecked( true );
    }
    vbox1->addWidget(cyclingAnalyticsChk,0,col++);
#endif

    selfLoopsUploader = new SelfLoopsUploader(context, ride, this);
    selfLoopsChk = new QCheckBox(tr("Selfloops"));
    if( ! selfLoopsUploader->canUpload( err ) ){
        selfLoopsChk->setEnabled( false );
    } else if( ! selfLoopsUploader->wasUploaded() ){
        selfLoopsChk->setChecked( true );
    }
    vbox1->addWidget(selfLoopsChk,0,col++);

    veloHeroUploader = new VeloHeroUploader(context, ride, this);
    veloHeroChk = new QCheckBox(tr("VeloHero"));
    if( ! veloHeroUploader->canUpload( err ) ){
        veloHeroChk->setEnabled( false );
    } else if( ! veloHeroUploader->wasUploaded() ){
        veloHeroChk->setChecked( true );
    }
    vbox1->addWidget(veloHeroChk,0,col++);

    trainingstagebuchUploader = new TrainingstagebuchUploader(context, ride, this);
    trainingstagebuchChk = new QCheckBox(tr("Trainingstagebuch.org"));
    if( ! trainingstagebuchUploader->canUpload( err ) ){
        trainingstagebuchChk->setEnabled( false );
    } else if( ! trainingstagebuchUploader->wasUploaded() ){
        trainingstagebuchChk->setChecked( true );
    }
    vbox1->addWidget(trainingstagebuchChk,0,col++);

    //garminUploader = new GarminUploader(context, ride, this); // not in 3.1
    //garminChk = new QCheckBox(tr("Garmin Connect"));
    //garminChk->setVisible(false);
    //if( ! garminUploader->canUpload( err ) ){
    //    garminChk->setEnabled( false );
    //}
    //if( ! garminUploader->wasUploaded() ){
    //    garminChk->setChecked( true );
    //}
    //vbox1->addWidget(garminChk,0,col++);


    groupBox1->setLayout(vbox1);
    mainLayout->addWidget(groupBox1);

    QGroupBox *groupBox2 = new QGroupBox(tr("Choose a name for your ride: "));

    titleEdit = new QLineEdit();

    // If we have a ride, and it can be opened then lets set the default for the title
    // We try metadata fields; Title, then Name then Route then Workout Code
    if (ride && ride->ride()) {

        // is "Title" set?
        if (!ride->ride()->getTag("Title", "").isEmpty()) {
            titleEdit->setText(ride->ride()->getTag("Title", ""));
        } else {

            // is "Name" set?
            if (!ride->ride()->getTag("Name", "").isEmpty()) {
                titleEdit->setText(ride->ride()->getTag("Name", ""));
            } else {

                // is "Route" set?
                if (!ride->ride()->getTag("Route", "").isEmpty()) {
                    titleEdit->setText(ride->ride()->getTag("Route", ""));
                } else {

                    //  is Workout Code set?
                    if (!ride->ride()->getTag("Workout Code", "").isEmpty()) {
                        titleEdit->setText(ride->ride()->getTag("Workout Code", ""));
                    }
                }
            }
        }
    }

    QGridLayout *vbox2 = new QGridLayout();
    vbox2->addWidget(titleEdit,0,0);

    groupBox2->setLayout(vbox2);
    mainLayout->addWidget(groupBox2);

    QGroupBox *groupBox3 = new QGroupBox(tr("Choose which data series you wish to send: "));

    //gpsChk = new QCheckBox(tr("GPS"));
    altitudeChk = new QCheckBox(tr("Altitude"));
    powerChk = new QCheckBox(tr("Power"));
    cadenceChk = new QCheckBox(tr("Cadence"));
    heartrateChk = new QCheckBox(tr("Heartrate"));

    const RideFileDataPresent *dataPresent = ride->ride()->areDataPresent();
    altitudeChk->setEnabled(dataPresent->alt);
    altitudeChk->setChecked(dataPresent->alt);
    powerChk->setEnabled(dataPresent->watts);
    powerChk->setChecked(dataPresent->watts);
    cadenceChk->setEnabled(dataPresent->cad);
    cadenceChk->setChecked(dataPresent->cad);
    heartrateChk->setEnabled(dataPresent->hr);
    heartrateChk->setChecked(dataPresent->hr);

    QGridLayout *vbox3 = new QGridLayout();
    //vbox3->addWidget(gpsChk,0,0);
    vbox3->addWidget(powerChk,0,0);
    vbox3->addWidget(altitudeChk,0,1);
    vbox3->addWidget(cadenceChk,1,0);
    vbox3->addWidget(heartrateChk,1,1);

    groupBox3->setLayout(vbox3);
    mainLayout->addWidget(groupBox3);

    // show progress
    QVBoxLayout *progressLayout = new QVBoxLayout;
    progressBar = new QProgressBar(this);
    progressLabel = new QLabel("", this);
    errorLabel = new QLabel("", this);

    progressLayout->addWidget(progressBar);
    progressLayout->addWidget(progressLabel);
    progressLayout->addWidget(errorLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    uploadButton = new QPushButton(tr("&Upload Ride"), this);
    buttonLayout->addWidget(uploadButton);
    closeButton = new QPushButton(tr("&Close"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(progressLayout);
    mainLayout->addLayout(buttonLayout);

    connect(uploadButton, SIGNAL(clicked()), this, SLOT(upload()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));
}


void
ShareDialog::upload()
{
    show();

    if ( !rideWithGPSChk->isChecked() && !selfLoopsChk->isChecked()
        && !veloHeroChk->isChecked() && !trainingstagebuchChk->isChecked()
#ifdef GC_HAVE_LIBOAUTH
        && !stravaChk->isChecked() && !cyclingAnalyticsChk->isChecked()
#endif
        //&& !garminChk->isChecked()
        ) {
        QMessageBox aMsgBox;
        aMsgBox.setText(tr("No share site selected !"));
        aMsgBox.exec();
        return;
    }

    uploadButton->setEnabled(false);

    shareSiteCount = 0;
    progressBar->setValue(0);
    progressLabel->setText("");
    errorLabel->setText("");

#ifdef GC_HAVE_LIBOAUTH
    if (stravaChk->isChecked()) {
        shareSiteCount ++;
    }
#endif
    if (rideWithGPSChk->isChecked()) {
        shareSiteCount ++;
    }
#ifdef GC_HAVE_LIBOAUTH
    if (cyclingAnalyticsChk->isChecked()) {
        shareSiteCount ++;
    }
#endif
    if (selfLoopsChk->isChecked()) {
        shareSiteCount ++;
    }
    if (veloHeroChk->isChecked()) {
        shareSiteCount ++;
    }
    if (trainingstagebuchChk->isChecked()) {
        shareSiteCount ++;
    }
    //if (garminChk->isChecked()) {
    //    shareSiteCount ++;
    //}

#ifdef GC_HAVE_LIBOAUTH
    if (stravaChk->isEnabled() && stravaChk->isChecked()) {
        doUploader( stravaUploader );
    }
#endif
    if (rideWithGPSChk->isEnabled() && rideWithGPSChk->isChecked()) {
        doUploader( rideWithGpsUploader );
    }
#ifdef GC_HAVE_LIBOAUTH
    if (cyclingAnalyticsChk->isEnabled() && cyclingAnalyticsChk->isChecked()) {
        doUploader( cyclingAnalyticsUploader );
    }
#endif
    if (selfLoopsChk->isEnabled() && selfLoopsChk->isChecked()) {
        doUploader( selfLoopsUploader );
    }
    if (veloHeroChk->isEnabled() && veloHeroChk->isChecked()) {
        doUploader( veloHeroUploader );
    }
    if (trainingstagebuchChk->isEnabled() && trainingstagebuchChk->isChecked()) {
        doUploader( trainingstagebuchUploader );
    }
    //if (garminChk->isEnabled() && garminChk->isChecked()) {
    //    doUploader( garminUploader );
    //}

    uploadButton->setEnabled(true);
}

void
ShareDialog::okClicked()
{
    dialog->accept();
    return;
}

void
ShareDialog::closeClicked()
{
    dialog->reject();
    return;
}


void
ShareDialog::doUploader( ShareDialogUploader *uploader )
{
    assert(uploader);

    if( uploader->wasUploaded() ){
        dialog = new QDialog();
        QVBoxLayout *layout = new QVBoxLayout;

        QVBoxLayout *layoutLabel = new QVBoxLayout();
        QLabel *label = new QLabel();
        label->setText(tr("This Ride is marked as already on %1. Are you sure you want to upload it?")
            .arg(uploader->name()) );
        layoutLabel->addWidget(label);

        QPushButton *ok = new QPushButton(tr("OK"), dialog);
        QPushButton *cancel = new QPushButton(tr("Cancel"), dialog);
        QHBoxLayout *buttons = new QHBoxLayout();
        buttons->addStretch();
        buttons->addWidget(cancel);
        buttons->addWidget(ok);

        connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
        connect(cancel, SIGNAL(clicked()), this, SLOT(closeClicked()));

        layout->addLayout(layoutLabel);
        layout->addLayout(buttons);

        dialog->setLayout(layout);

        if (!dialog->exec()) return;
    }

    uploader->upload();
}

#ifdef GC_HAVE_LIBOAUTH
StravaUploader::StravaUploader(Context *context, RideItem *ride, ShareDialog *parent) :
    ShareDialogUploader( tr("Strava"), context, ride, parent)
{
    stravaUploadId = ride->ride()->getTag("Strava uploadId", "0").toInt();
    eventLoop = new QEventLoop(this);
    networkManager = new QNetworkAccessManager(this);
}

bool
StravaUploader::canUpload( QString &err )
{
#ifdef GC_STRAVA_CLIENT_SECRET
   token = appsettings->cvalue(context->athlete->cyclist, GC_STRAVA_TOKEN, "").toString();
   if( token!="" )
        return true;

    err = tr("no Strava token set. Please authorize in Settings.");
#else
    err = tr("Strava support isn't enabled in this build");
#endif
    return false;
}

bool
StravaUploader::wasUploaded()
{
    return stravaUploadId>0;
}

void
StravaUploader::upload()
{
    // OAuth no more login
    token = appsettings->cvalue(context->athlete->cyclist, GC_STRAVA_TOKEN, "").toString();
    if (token=="")
        return;

    requestUploadStrava();

    if(!uploadSuccessful)
    {
        parent->progressLabel->setText(tr("Error uploading to Strava"));
    }
    else
    {
        //requestVerifyUpload();
        parent->progressLabel->setText(tr("Successfully uploaded to Strava\n"));
    }
}

// Documentation is at:
// https://strava.pbworks.com/w/page/39241255/v2%20upload%20create
void
StravaUploader::requestUploadStrava()
{
    parent->progressLabel->setText(tr("Upload ride to Strava..."));
    parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);

    int year = ride->fileName.left(4).toInt();
    int month = ride->fileName.mid(5,2).toInt();
    int day = ride->fileName.mid(8,2).toInt();
    int hour = ride->fileName.mid(11,2).toInt();
    int minute = ride->fileName.mid(14,2).toInt();;
    int second = ride->fileName.mid(17,2).toInt();;

    QDate rideDate = QDate(year, month, day);
    QTime rideTime = QTime(hour, minute, second);
    QDateTime rideDateTime = QDateTime(rideDate, rideTime);

    // trap network response from access manager
    networkManager->disconnect();
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestUploadStravaFinished(QNetworkReply*)));
    connect(networkManager, SIGNAL(finished(QNetworkReply *)), eventLoop, SLOT(quit()));

    TcxFileReader reader;

    QUrl url = QUrl( "https://www.strava.com/api/v3/uploads" ); // The V3 API doc said "https://api.strava.com" but it is not working yet
    QNetworkRequest request = QNetworkRequest(url);

    //QString boundary = QString::number(qrand() * (90000000000) / (RAND_MAX + 1) + 10000000000, 16);
    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();

    QByteArray file = reader.toByteArray(context, ride->ride(), parent->altitudeChk->isChecked(), parent->powerChk->isChecked(), parent->heartrateChk->isChecked(), parent->cadenceChk->isChecked());

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary(boundary.toLatin1());

    QHttpPart accessTokenPart;
    accessTokenPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"access_token\""));
    accessTokenPart.setBody(token.toLatin1());

    QHttpPart activityTypePart;
    activityTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"activity_type\""));
    activityTypePart.setBody("ride");

    QHttpPart activityNamePart;
    activityNamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"activity_name\""));
    activityNamePart.setBody(QString(parent->titleEdit->text()).toLatin1());

    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data_type\""));
    dataTypePart.setBody("tcx");

    QHttpPart externalIdPart;
    externalIdPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"external_id\""));
    externalIdPart.setBody("Ride");

    QHttpPart privatePart;
    privatePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"private\""));
    privatePart.setBody("TRUE");

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("text/xml"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"file.tcx\"; type=\"text/xml\""));
    filePart.setBody(file);


    multiPart->append(accessTokenPart);
    multiPart->append(activityTypePart);
    multiPart->append(activityNamePart);
    multiPart->append(dataTypePart);
    multiPart->append(externalIdPart);
    multiPart->append(privatePart);
    multiPart->append(filePart);

    networkManager->post(request, multiPart);

    parent->progressBar->setValue(parent->progressBar->value()+30/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Upload ride... Sending to Strava"));

    eventLoop->exec();
}

void
StravaUploader::requestUploadStravaFinished(QNetworkReply *reply)
{
    parent->progressBar->setValue(parent->progressBar->value()+50/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Upload to Strava finished."));

    uploadSuccessful = false;

    QString response = reply->readLine();
    //qDebug() << response;

    // use a lightweight json parser to do this
    QString uploadError="invalid response or parser error";
    try {

        // parse !
        MVJSONReader jsonResponse(string(response.toLatin1()));

        // get error field
        if (jsonResponse.root->hasField("error")) {
            uploadError = jsonResponse.root->getFieldString("error").c_str();
        } else {
            uploadError = ""; // no error
        }

        // get upload_id, but if not available use id
        if (jsonResponse.root->hasField("upload_id")) {
            stravaUploadId = jsonResponse.root->getFieldInt("upload_id");
        } else if (jsonResponse.root->hasField("id")) {
            stravaUploadId = jsonResponse.root->getFieldInt("id");
        } else {
            stravaUploadId = 0;
        }
    } catch(...) { // not really sure what exceptions to expect so do them all (bad, sorry)
        uploadError=tr("invalid response or parser exception.");
    }

    if (uploadError.toLower() == "none" || uploadError.toLower() == "null")
        uploadError = "";

    if (uploadError.length()>0 || reply->error() != QNetworkReply::NoError)
    {
        //qDebug() << "Error " << reply->error() ;
        //qDebug() << "Error " << uploadError;
        parent->errorLabel->setText(parent->errorLabel->text()+ tr(" Error from Strava: ") + uploadError + "\n" );
    }
    else
    {

        ride->ride()->setTag("Strava uploadId", QString("%1").arg(stravaUploadId));
        ride->setDirty(true);

        //qDebug() << "uploadId: " << stravaUploadId;

        parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);
        uploadSuccessful = true;
    }
}

void
StravaUploader::requestVerifyUpload()
{
    parent->progressBar->setValue(0);
    parent->progressLabel->setText(tr("Ride processing..."));

    // reconnect for verify
    networkManager->disconnect();
    connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestVerifyUploadFinished(QNetworkReply*)));
    connect(networkManager, SIGNAL(finished(QNetworkReply *)), eventLoop, SLOT(quit()));
    QByteArray out;

    QUrl url = QUrl("https://www.strava.com/api/v3/upload/status/"+QString("%1").arg(stravaUploadId)+"?token="+token);
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    networkManager->post(request, out);
    eventLoop->exec();
}

void
StravaUploader::requestVerifyUploadFinished(QNetworkReply *reply)
{
    uploadSuccessful = false;
    if (reply->error() != QNetworkReply::NoError)
        qDebug() << "Error from upload " <<reply->error();
    else {

        try {

            // parse the response
            QString response = reply->readLine();
            MVJSONReader jsonResponse(string(response.toLatin1()));

            // get values
            uploadProgress = jsonResponse.root->getFieldInt("upload_progress");
            uploadStatus = jsonResponse.root->getFieldString("upload_status").c_str();
            stravaActivityId = jsonResponse.root->getFieldInt("activity_id");

        } catch(...) {

            // problem!
            uploadProgress = 0;
            uploadStatus = "response error or parser exception";
            stravaActivityId = 0;
        }

        //qDebug() << "upload_progress: " << uploadProgress;
        //qDebug() << "upload_status: " << uploadStatus;
        parent->progressBar->setValue(uploadProgress);
        parent->progressLabel->setText(uploadStatus);

        // not done yet
        if (stravaActivityId == 0 || uploadProgress < 97) {
            requestVerifyUpload();
            return;
        }

        // success
        ride->ride()->setTag("Strava activityId", QString("%1").arg(stravaActivityId));
        ride->setDirty(true);
        uploadSuccessful = true;
    }
}
#endif // GC_HAVE_LIBOAUTH for strava

RideWithGpsUploader::RideWithGpsUploader(Context *context, RideItem *ride, ShareDialog *parent) :
    ShareDialogUploader( tr("Ride With GPS"), context, ride, parent)
{
    rideWithGpsActivityId = ride->ride()->getTag("RideWithGPS tripid", "");
}

bool
RideWithGpsUploader::canUpload( QString &err )
{
    QString username = appsettings->cvalue(context->athlete->cyclist, GC_RWGPSUSER).toString();
    if( username.length() > 0 )
        return true;

    err = tr("no credentials set for RideWithGps. Please check Settings.");
    return false;
}

bool
RideWithGpsUploader::wasUploaded()
{
    return rideWithGpsActivityId.length()>0;
}

void
RideWithGpsUploader::upload()
{
    requestUploadRideWithGPS();

    if(!uploadSuccessful)
    {
        parent->progressLabel->setText(tr("Error uploading to RideWithGPS")+"\n");
    }
    else
    {
        parent->progressLabel->setText(tr("Successfully uploaded to RideWithGPS"));
    }
}

void
RideWithGpsUploader::requestUploadRideWithGPS()
{
    parent->progressLabel->setText(tr("Upload ride..."));
    parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;

    int prevSecs = 0;
    long diffSecs = 0;

    int year = ride->fileName.left(4).toInt();
    int month = ride->fileName.mid(5,2).toInt();
    int day = ride->fileName.mid(8,2).toInt();
    int hour = ride->fileName.mid(11,2).toInt();
    int minute = ride->fileName.mid(14,2).toInt();;
    int second = ride->fileName.mid(17,2).toInt();;

    QDate rideDate = QDate(year, month, day);
    QTime rideTime = QTime(hour, minute, second);
    QDateTime rideDateTime = QDateTime(rideDate, rideTime);

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestUploadRideWithGPSFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    QString out, data;

    QVector<RideFilePoint*> vectorPoints = ride->ride()->dataPoints();
    int totalSize = vectorPoints.size();

    int size = 0;

    QString username = appsettings->cvalue(context->athlete->cyclist, GC_RWGPSUSER).toString();
    QString password = appsettings->cvalue(context->athlete->cyclist, GC_RWGPSPASS).toString();

    // application/json
    out += "{\"apikey\": \"p24n3a9e\", ";
    out += "\"email\": \""+username+"\", ";
    out += "\"password\": \""+password+"\", ";
    out += "\"track_points\": \"";

    data += "\[";
    foreach (const RideFilePoint *point, ride->ride()->dataPoints())
    {
        size++;

        if (point->secs == 0.0)
            continue;

        diffSecs = point->secs - prevSecs;
        prevSecs = point->secs;
        rideDateTime = rideDateTime.addSecs(diffSecs);

        data += "{\"x\": ";
        data += QString("%1").arg(point->lon,0,'f',GPS_COORD_TO_STRING);
        data += ", \"y\": ";
        data += QString("%1").arg(point->lat,0,'f',GPS_COORD_TO_STRING);
        data += ", \"t\": ";
        data += QString("%1").arg(rideDateTime.toTime_t());

        if (parent->altitudeChk->isChecked()) {
            data += ", \"e\": ";
            data += QString("%1").arg(point->alt);
        }
        if (parent->powerChk->isChecked()) {
            data += ", \"p\": ";
            data += QString("%1").arg(point->watts);
        }
        if (parent->cadenceChk->isChecked()) {
            data += ", \"c\": ";
            data += QString("%1").arg(point->cad);
        }
        if (parent->heartrateChk->isChecked()) {
            data += ", \"h\": ";
            data += QString("%1").arg(point->hr);
        }

        data += "}";

        if(size < totalSize)
           data += ",";
    }
    data += "]";
    out += data.replace("\"","\\\"");
    out += "\"}";

    QUrl url = QUrl("http://ridewithgps.com/trips.json");
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    parent->progressBar->setValue(parent->progressBar->value()+30/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Upload ride... Sending to RideWithGPS"));

    networkMgr.post( request, out.toLatin1());
    eventLoop.exec();
}

void
RideWithGpsUploader::requestUploadRideWithGPSFinished(QNetworkReply *reply)
{
    parent->progressBar->setValue(parent->progressBar->value()+50/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Upload to RideWithGPS finished."));
    uploadSuccessful = false;

    QString uploadError;
    int tripid = 0;

    try {

        // parse the response
        QString response = reply->readAll();
        MVJSONReader jsonResponse(string(response.toLatin1()));

        // get values
        uploadError = jsonResponse.root->getFieldString("error").c_str();
        if (jsonResponse.root->hasField("trip")) {
            tripid = jsonResponse.root->getField("trip")->getFieldInt("id");
        }

    } catch(...) {

        // problem!
        uploadError = "bad response or parser exception.";
    }

    // no error so clear
    if (uploadError.toLower() == "none" || uploadError.toLower() == "null")
        uploadError = "";

    if (uploadError.length()>0 || reply->error() != QNetworkReply::NoError) {
        //qDebug() << "Error " << reply->error();
        //qDebug() << "Error; " << uploadError;
        parent->errorLabel->setText(parent->errorLabel->text()+ tr(" Error from RideWithGPS: ") + uploadError + "\n" );
    }
    else
    {
        ride->ride()->setTag("RideWithGPS tripid", QString("%1").arg(tripid));
        ride->setDirty(true);

        //qDebug() << "tripid: " << tripid;
        parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);
        uploadSuccessful = true;
    }
}

#ifdef GC_HAVE_LIBOAUTH

CyclingAnalyticsUploader::CyclingAnalyticsUploader(Context *context, RideItem *ride, ShareDialog *parent) :
    ShareDialogUploader(tr("CyclingAnalytics"), context, ride, parent)
{
    cyclingAnalyticsUploadId = ride->ride()->getTag("CyclingAnalytics uploadId", "0").toInt();
}

bool
CyclingAnalyticsUploader::canUpload( QString &err )
{
#ifdef GC_CYCLINGANALYTICS_CLIENT_SECRET
    token = appsettings->cvalue(context->athlete->cyclist, GC_CYCLINGANALYTICS_TOKEN, "").toString();
   if( token!="" )
        return true;

    err = tr("no CyclingAnalytics token set. Please authorize in Settings.");
#else
    err = tr("CyclingAnalytics support isn't enabled in this build");
#endif
    return false;
}

bool
CyclingAnalyticsUploader::wasUploaded()
{
    return cyclingAnalyticsUploadId>0;
}

void
CyclingAnalyticsUploader::upload()
{
    // OAuth no more login
    token = appsettings->cvalue(context->athlete->cyclist, GC_CYCLINGANALYTICS_TOKEN, "").toString();
    if (token=="")
        return;

    requestUploadCyclingAnalytics();

    if(!uploadSuccessful)
    {
        parent->progressLabel->setText(tr("Error uploading to CyclingAnalytics"));
    }
    else
    {
        parent->progressLabel->setText(tr("Successfully uploaded to CyclingAnalytics"));
    }
}

void
CyclingAnalyticsUploader::requestUploadCyclingAnalytics()
{
    parent->progressLabel->setText(tr("Upload ride to CyclingAnalytics..."));
    parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestUploadCyclingAnalyticsFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    QUrl url = QUrl( "https://www.cyclinganalytics.com/api/me/upload" );
    QNetworkRequest request = QNetworkRequest(url);

    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();

    TcxFileReader reader;
    QByteArray file = reader.toByteArray(context, ride->ride(), parent->altitudeChk->isChecked(), parent->powerChk->isChecked(), parent->heartrateChk->isChecked(), parent->cadenceChk->isChecked());

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary(boundary.toLatin1());

    request.setRawHeader("Authorization", (QString("Bearer %1").arg(token)).toLatin1());

    QHttpPart activityNamePart;
    activityNamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"title\""));
    activityNamePart.setBody(QString(parent->titleEdit->text()).toLatin1());

    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"format\""));
    dataTypePart.setBody("tcx");

    QHttpPart filenamePart;
    filenamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"filename\""));
    filenamePart.setBody("file.tcx");

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data\"; filename=\"file.tcx\"; type=\"text/xml\""));
    filePart.setBody(file);

    multiPart->append(activityNamePart);
    multiPart->append(filenamePart);
    multiPart->append(dataTypePart);
    multiPart->append(filePart);

    networkMgr.post(request, multiPart);

    parent->progressBar->setValue(parent->progressBar->value()+30/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Upload ride... Sending to CyclingAnalytics"));

    eventLoop.exec();
}

void
CyclingAnalyticsUploader::requestUploadCyclingAnalyticsFinished(QNetworkReply *reply)
{
    parent->progressBar->setValue(parent->progressBar->value()+50/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Upload to CyclingAnalytics finished."));
    uploadSuccessful = false;

    QString uploadError;
    try {

        // parse the response
        QString response = reply->readAll();
        MVJSONReader jsonResponse(string(response.toLatin1()));

        // get values
        uploadError = jsonResponse.root->getFieldString("error").c_str();
        cyclingAnalyticsUploadId = jsonResponse.root->getFieldInt("upload_id");

    } catch(...) {

        // problem!
        uploadError = "bad response or parser exception.";
        cyclingAnalyticsUploadId = 0;
    }

    // if not there clean out
    if (uploadError.toLower() == "none" || uploadError.toLower() == "null")
        uploadError = "";

    if (uploadError.length()>0 || reply->error() != QNetworkReply::NoError) {
        //qDebug() << "Error " << reply->error() ;
        //qDebug() << "Error " << uploadError;
        parent->errorLabel->setText(parent->errorLabel->text()+ tr(" Error from CyclingAnalytics: ") + uploadError + "\n" );

    } else {

        // Success
        ride->ride()->setTag("CyclingAnalytics uploadId", QString("%1").arg(cyclingAnalyticsUploadId));
        ride->setDirty(true);

        //qDebug() << "uploadId: " << cyclingAnalyticsUploadId;
        parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);
        uploadSuccessful = true;
    }
}
#endif // GC_HAVE_LIBOAUTH for cyclingAnalytics

SelfLoopsUploader::SelfLoopsUploader(Context *context, RideItem *ride, ShareDialog *parent) :
    ShareDialogUploader(tr("SelfLoops"),context, ride, parent)
{
    selfloopsUploadId = ride->ride()->getTag("Selfloops uploadId", "0").toInt();
    selfloopsActivityId = ride->ride()->getTag("Selfloops activityId", "0").toInt();
}

bool
SelfLoopsUploader::canUpload( QString &err )
{
    QString username = appsettings->cvalue(context->athlete->cyclist, GC_SELUSER).toString();
    if( username.length() > 0 )
        return true;

    err = tr("no credentials set for SelfLoops. Please check Settings.");
    return false;
}

bool
SelfLoopsUploader::wasUploaded()
{
    return selfloopsActivityId>0;
}

void
SelfLoopsUploader::upload()
{
    requestUploadSelfLoops();

    if(!uploadSuccessful)
    {
        parent->progressLabel->setText(tr("Error uploading to Selfloops"));
    }
    else
    {
        parent->progressLabel->setText(tr("Successfully uploaded to Selfloops"));
    }
}

/*
  make a multipart HTTP POST at the following path "/restapi/public/activities/upload.json"
  on selflloops web site using SSL.
  The requested parameters are:
   - "email" the email of a valid SelfLoops account
   - "pw" the password
   - "tcxfile" the zipped TCX file (example: test.tcx.gz).

   On success, response message contains a JSON encoded data
   with the new "activity_id" created.
   On error, SelfLoops response contains a JSON encoded data with "error_code" and "message" key.
*/
void
SelfLoopsUploader::requestUploadSelfLoops()
{
    parent->progressLabel->setText(tr("Upload ride to Selfloops..."));
    parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestUploadSelfLoopsFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    QUrl url = QUrl( "https://www.selfloops.com/restapi/public/activities/upload.json" );
    QNetworkRequest request = QNetworkRequest(url);

    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();

    // The TCX file have to be gzipped
    TcxFileReader reader;
    QByteArray file = zCompress(reader.toByteArray(context, ride->ride(), parent->altitudeChk->isChecked(), parent->powerChk->isChecked(), parent->heartrateChk->isChecked(), parent->cadenceChk->isChecked()));

    QString username = appsettings->cvalue(context->athlete->cyclist, GC_SELUSER).toString();
    QString password = appsettings->cvalue(context->athlete->cyclist, GC_SELPASS).toString();

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::MixedType);
    multiPart->setBoundary(boundary.toLatin1());

    QHttpPart emailPart;
    emailPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"email\""));
    emailPart.setBody(username.toLatin1());

    QHttpPart passwordPart;
    passwordPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"pw\""));
    passwordPart.setBody(password.toLatin1());

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"tcxfile\"; filename=\"myfile.tcx.gz\"; type=\"application/x-gzip\""));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-gzip");
    filePart.setBody(file);

    multiPart->append(emailPart);
    multiPart->append(passwordPart);
    multiPart->append(filePart);

    networkMgr.post(request, multiPart);

    parent->progressBar->setValue(parent->progressBar->value()+30/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Upload ride... Sending to Selfloops"));

    eventLoop.exec();
}

void
SelfLoopsUploader::requestUploadSelfLoopsFinished(QNetworkReply *reply)
{
    parent->progressBar->setValue(parent->progressBar->value()+50/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Upload to Selfloops finished."));

    uploadSuccessful = false;

    int error;
    QString uploadError;
    try {

        // parse the response
        QString response = reply->readAll();
        MVJSONReader jsonResponse(string(response.toLatin1()));

        // get values
        error = jsonResponse.root->getFieldInt("error_code");
        uploadError = jsonResponse.root->getFieldString("message").c_str();
        selfloopsActivityId = jsonResponse.root->getFieldInt("activity_id");

    } catch(...) {

        // problem!
        error = 500;
        uploadError = "bad response or parser exception.";
        selfloopsActivityId = 0;
    }

    if (error>0 || reply->error() != QNetworkReply::NoError) {
        //qDebug() << "Error " << reply->error() ;
        //qDebug() << "Error " << uploadError;
        parent->errorLabel->setText(parent->errorLabel->text()+ tr(" Error from Selfloops: ") + uploadError + "\n" );

    } else {

        //qDebug() << "activity: " << selfloopsActivityId;

        ride->ride()->setTag("Selfloops activityId", QString("%1").arg(selfloopsActivityId));
        ride->setDirty(true);

        parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);
        uploadSuccessful = true;
    }
}

#if 0 // NOT AVAILABLE -- COMMENTED OUT FOR VERSION 3.1
/******************/
/* Garmin Connect */
/******************/

GarminUploader::GarminUploader(Context *context, RideItem *ride, ShareDialog *parent) :
    ShareDialogUploader( tr("Garmin Connect"), context, ride, parent)
{
    garminUploadId = ride->ride()->getTag("Garmin Connect uploadId", "");
    garminActivityId = ride->ride()->getTag("Garmin Connect activityId", "");
}

bool
GarminUploader::wasUploaded()
{
    return garminActivityId.length()>0;
}

void
GarminUploader::upload()
{
    requestFlowExecutionKey();
    //requestUploadGarmin();

    if(!uploadSuccessful)
    {
        parent->progressLabel->setText(tr("Error uploading to Garmin Connect"));
    }
    else
    {
        parent->progressLabel->setText(tr("Successfully uploaded to Garmin Connect"));
    }
}

void
GarminUploader::requestFlowExecutionKey()
{
    parent->progressLabel->setText(tr("Login to Garmin Connect..."));
    parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);

    QEventLoop eventLoop;

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFlowExecutionKeyFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    serverUrl = QUrl("https://sso.garmin.com/sso/login?service=http%3A%2F%2Fconnect.garmin.com%2Fpost-auth%2Flogin&webhost=olaxpw-connect07.garmin.com&source=http%3A%2F%2Fconnect.garmin.com%2Fde-DE%2Fsignin&redirectAfterAccountLoginUrl=http%3A%2F%2Fconnect.garmin.com%2Fpost-auth%2Flogin&redirectAfterAccountCreationUrl=http%3A%2F%2Fconnect.garmin.com%2Fpost-auth%2Flogin&gauthHost=https%3A%2F%2Fsso.garmin.com%2Fsso&locale=en&id=gauth-widget&cssUrl=https%3A%2F%2Fstatic.garmincdn.com%2Fcom.garmin.connect%2Fui%2Fsrc-css%2Fgauth-custom.css&clientId=GarminConnect&rememberMeShown=true&rememberMeChecked=false&createAccountShown=true&openCreateAccount=false&usernameShown=true&displayNameShown=false&consumeServiceTicket=false&initialFocus=true&embedWidget=false");

    QNetworkRequest request = QNetworkRequest(serverUrl);

    qDebug() << "url" << serverUrl;

    networkMgr.get(request);

    // holding pattern
    eventLoop.exec();
}

void
GarminUploader::requestFlowExecutionKeyFinished(QNetworkReply *reply)
{
    parent->progressBar->setValue(parent->progressBar->value()+50/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Login to Garmin Connect finished."));

    QString response = reply->readAll();
    //qDebug() << "response" << response;

    int i = response.indexOf("[", response.indexOf("flowExecutionKey"))+1;
    int j = response.indexOf("]", i);
    flowExecutionKey = response.mid(i, j-i);
    qDebug() << "flowExecutionKey" << flowExecutionKey;

    QVariant v = reply->header(QNetworkRequest::SetCookieHeader);
    serverUrl = QUrl( "https://sso.garmin.com/sso/login" );

#if QT_VERSION > 0x050000
    QUrlQuery params;
#else
    QUrl params = serverUrl;
#endif

    params.addQueryItem("service","http%3A%2F%2Fconnect.garmin.com%2Fpost-auth%2Flogin");
    params.addQueryItem("clientId","GarminConnect");
    params.addQueryItem("consumeServiceTicket","false");

#if QT_VERSION > 0x050000
    serverUrl.setQuery(params);
#endif

    qDebug() << "serverUrl" << serverUrl;

    requestLoginGarmin();
}

void
GarminUploader::requestLoginGarmin()
{
    qDebug() << "requestLoginGarmin()";

    parent->progressLabel->setText(tr("Login to Garmin Connect..."));
    parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);

    QEventLoop eventLoop;

    disconnect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestFlowExecutionKeyFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestLoginGarminFinished(QNetworkReply*)));


    QNetworkRequest request = QNetworkRequest(serverUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader,QVariant("application/x-www-form-urlencoded"));

    QByteArray data;
#if QT_VERSION > 0x050000
    QUrlQuery params;
#else
    QUrl params;
#endif

    params.addQueryItem("_eventId","submit");
    params.addQueryItem("embed","true");

    params.addQueryItem("lt",flowExecutionKey);
    params.addQueryItem("username","grauser");
    params.addQueryItem("password","_garmin55");

#if QT_VERSION > 0x050000
    //data.append(params.query(QUrl::FullyEncoded));
    data=params.query(QUrl::FullyEncoded).toUtf8();
#else
    data=params.encodedQuery();
#endif

    networkMgr.post(request, data);

    // holding pattern
    eventLoop.exec();
}

void
GarminUploader::requestLoginGarminFinished(QNetworkReply *reply)
{
    qDebug() << "requestLoginGarminFinished()";

    parent->progressBar->setValue(parent->progressBar->value()+50/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Login to Garmin Connect finished."));

    QString response = reply->readAll();
    //qDebug() << "response2" << response;

    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error " << reply->error() ;
    } else {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        qDebug() << "statusCode " << statusCode;

        if(statusCode == 200) {

            sc = se.evaluate("("+response+")");
            QString response_url = sc.property("response_url").toString();
            qDebug() << "response_url" << response_url;

            int i = response.indexOf("?ticket=")+8;
            int j = response.indexOf("'", i);
            ticket = response.mid(i, j-i);
            qDebug() << "ticket" << ticket;

            requestUploadGarmin();
        }
        else if(statusCode == 302) { // or statusCode == 301
            QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            qDebug() << "redirectUrl " << redirectUrl;
            //serverUrl = redirectUrl;
            //requestLoginGarmin();
        }
    }

}

void
GarminUploader::requestUploadGarmin()
{
    qDebug() << "requestUploadGarmin()";

    parent->progressLabel->setText(tr("Upload ride to Garmin Connect..."));
    parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);

    QEventLoop eventLoop;

    disconnect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestLoginGarminFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestUploadGarminFinished(QNetworkReply*)));

    // #/proxy/upload-service-1.1/json/upload/.fit
    // fit_file = FITIO.Dump(activity)
    // files = {"data": ("tap-sync-" + str(os.getpid()) + "-" + activity.UID + ".fit", fit_file)}
    // cookies = self._get_cookies(record=serviceRecord)
    // self._rate_limit()
    // res = requests.post("http://connect.garmin.com/proxy/upload-service-1.1/json/upload/.tcx", files=files, cookies=cookies)
    // res = res.json()["detailedImportResult"]


    QUrl url = QUrl( "http://connect.garmin.com/proxy/upload-service-1.1/json/upload/.tcx" );
    QNetworkRequest request = QNetworkRequest(url);

    QString boundary = QVariant(qrand()).toString()+QVariant(qrand()).toString()+QVariant(qrand()).toString();

    // The TCX file have to be gzipped
    TcxFileReader reader;
    QByteArray file = reader.toByteArray(context, ride->ride(), parent->altitudeChk->isChecked(), parent->powerChk->isChecked(), parent->heartrateChk->isChecked(), parent->cadenceChk->isChecked());


    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::MixedType);
    multiPart->setBoundary(boundary.toLatin1());


    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"tcxfile\"; filename=\"myfile.tcx.\"; type=\"application/x-gzip\""));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-gzip");
    filePart.setBody(file);

    multiPart->append(filePart);

    networkMgr.post(request, multiPart);

    parent->progressBar->setValue(parent->progressBar->value()+30/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Upload ride... Sending to Garmin Connect"));

    eventLoop.exec();
}

void
GarminUploader::requestUploadGarminFinished(QNetworkReply *reply)
{
    qDebug() << "requestUploadGarminFinished()";

    parent->progressBar->setValue(parent->progressBar->value()+50/parent->shareSiteCount);
    parent->progressLabel->setText(tr("Upload to Garmin Connect finished."));

    uploadSuccessful = false;

    QString response = reply->readAll();
    qDebug() << "response" << response;

    sc = se.evaluate("("+response+")");
    QString error = sc.property("error_code").toString();
    QString uploadError = sc.property("message").toString();

    if (error.length()>0 || reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error " << reply->error() ;
        qDebug() << "Error " << uploadError;
        parent->errorLabel->setText(parent->errorLabel->text()+ tr(" Error from Garmin Connect: ") + uploadError + "\n" );
    }
    else
    {
        garminActivityId = sc.property("activity_id").toString();

        ride->ride()->setTag("Garmin Connect activityId", garminActivityId);
        ride->setDirty(true);

        qDebug() << "activity: " << garminActivityId;

        parent->progressBar->setValue(parent->progressBar->value()+10/parent->shareSiteCount);
        uploadSuccessful = true;
    }
}

#endif
