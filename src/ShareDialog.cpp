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
#include "Settings.h"
#include <QHttp>
#include <QUrl>
#include <QHttpMultiPart>
#include <QScriptEngine>
#include "TimeUtils.h"
#include "Units.h"

// acccess to metrics
#include "MetricAggregator.h"
#include "RideMetric.h"
#include "DBAccess.h"
#include "TcxRideFile.h"

ShareDialog::ShareDialog(MainWindow *mainWindow, RideItem *item) :
    mainWindow(mainWindow)
{
    setWindowTitle("Share your ride");

    // make the dialog a resonable size
    setMinimumWidth(550);
    setMinimumHeight(400);

    ride = item;
    stravaUploadId = ride->ride()->getTag("Strava uploadId", "");
    stravaActivityId = ride->ride()->getTag("Strava actityId", "");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGroupBox *groupBox1 = new QGroupBox(tr("Choose which sites you wish to share on: "));

    stravaChk = new QCheckBox(tr("Strava"));
#ifndef GC_STRAVA_CLIENT_SECRET
    stravaChk->setEnabled(false);
#endif
    rideWithGPSChk = new QCheckBox(tr("Ride With GPS"));

    QGridLayout *vbox1 = new QGridLayout();
    vbox1->addWidget(stravaChk,0,0);
    vbox1->addWidget(rideWithGPSChk,0,2);

    groupBox1->setLayout(vbox1);
    mainLayout->addWidget(groupBox1);

    QGroupBox *groupBox2 = new QGroupBox(tr("Choose a name for your ride: "));

    titleEdit = new QLineEdit();

    QGridLayout *vbox2 = new QGridLayout();
    vbox2->addWidget(titleEdit,0,0);

    groupBox2->setLayout(vbox2);
    mainLayout->addWidget(groupBox2);

    QGroupBox *groupBox3 = new QGroupBox(tr("Choose which channels you wish to send: "));

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

    progressLayout->addWidget(progressBar);
    progressLayout->addWidget(progressLabel);

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

    //connect(searchActivityButton, SIGNAL(clicked()), this, SLOT(getActivityFromStrava()));
    //connect(getActivityButton, SIGNAL(clicked()), this, SLOT(reject()));
    //connect(workoutTimeChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    //connect(timeRidingChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    //connect(totalDistanceChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
}


void
ShareDialog::upload()
{
    show();
    overwrite = true;

    if (!stravaChk->isChecked() && !rideWithGPSChk->isChecked()) {
        QMessageBox aMsgBox;
        aMsgBox.setText(tr("No share site selected !"));
        aMsgBox.exec();
        return;
    }

    if (stravaChk->isChecked()) {
        uploadStrava();
    }

    if (rideWithGPSChk->isChecked()) {
        uploadRideWithGPS();
    }
}

void
ShareDialog::uploadStrava()
{
    // OAuth no more login
    token = appsettings->cvalue(mainWindow->cyclist, GC_STRAVA_TOKEN, "").toString();
    if (token=="")
    {
        QMessageBox aMsgBox;
        aMsgBox.setText(tr("Cannot login to Strava. Check permission"));
        aMsgBox.exec();
        return;
    }

    // allready shared ?
    if(stravaActivityId.length()>0)
    {
        overwrite = false;

        dialog = new QDialog();
        QVBoxLayout *layout = new QVBoxLayout;

        QVBoxLayout *layoutLabel = new QVBoxLayout();
        QLabel *label = new QLabel();
        label->setText(tr("This Ride is marked as already on Strava. Are you sure you want to upload it?"));
        layoutLabel->addWidget(label);

        QPushButton *ok = new QPushButton(tr("OK"), this);
        QPushButton *cancel = new QPushButton(tr("Cancel"), this);
        QHBoxLayout *buttons = new QHBoxLayout();
        buttons->addStretch();
        buttons->addWidget(cancel);
        buttons->addWidget(ok);

        connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
        connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

        layout->addLayout(layoutLabel);
        layout->addLayout(buttons);

        dialog->setLayout(layout);

        if (!dialog->exec()) return;
    }

    requestUploadStrava();

    if(!uploadSuccessful)
    {
        progressLabel->setText("Error uploading to Strava");
    }
    else
    {
        //requestVerifyUpload();
        progressLabel->setText(tr("Successfully uploaded to Strava\n")+uploadStatus);
    }
}

void
ShareDialog::uploadRideWithGPS()
{
    if(stravaActivityId.length()>0)
    {
        overwrite = false;

        dialog = new QDialog();
        QVBoxLayout *layout = new QVBoxLayout;

        QVBoxLayout *layoutLabel = new QVBoxLayout();
        QLabel *label = new QLabel();
        label->setText(tr("This Ride is marked as already on RideWithGPS. Are you sure you want to upload it?"));
        layoutLabel->addWidget(label);

        QPushButton *ok = new QPushButton(tr("OK"), this);
        QPushButton *cancel = new QPushButton(tr("Cancel"), this);
        QHBoxLayout *buttons = new QHBoxLayout();
        buttons->addStretch();
        buttons->addWidget(cancel);
        buttons->addWidget(ok);

        connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
        connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));

        layout->addLayout(layoutLabel);
        layout->addLayout(buttons);

        dialog->setLayout(layout);

        if (!dialog->exec()) return;
    }

    requestUploadRideWithGPS();

    if(!uploadSuccessful)
    {
        progressLabel->setText(tr("Error uploading to RideWithGPS")+"\n");
    }
    else
    {
        progressLabel->setText(tr("Successfully uploaded to RideWithGPS"));
    }
}

void
ShareDialog::requestUploadRideWithGPS()
{
    progressLabel->setText(tr("Upload ride..."));
    progressBar->setValue(10);

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

    QString username = appsettings->cvalue(mainWindow->cyclist, GC_RWGPSUSER).toString();
    QString password = appsettings->cvalue(mainWindow->cyclist, GC_RWGPSPASS).toString();

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

        if (altitudeChk->isChecked()) {
            data += ", \"e\": ";
            data += QString("%1").arg(point->alt);
        }
        if (powerChk->isChecked()) {
            data += ", \"p\": ";
            data += QString("%1").arg(point->watts);
        }
        if (cadenceChk->isChecked()) {
            data += ", \"c\": ";
            data += QString("%1").arg(point->cad);
        }
        if (heartrateChk->isChecked()) {
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

    progressBar->setValue(40);
    progressLabel->setText(tr("Upload ride... Sending"));

    networkMgr.post( request, out.toAscii());
    eventLoop.exec();
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
ShareDialog::sslErrorHandler(QNetworkReply* qnr, const QList<QSslError> & errlist)
{
    qDebug() << "sslErrorHandler: ";
    // show list of all ssl errors
    foreach (QSslError err, errlist)
        qDebug() << "ssl error: " << err;

    qnr->ignoreSslErrors();
}

// Documentation is at:
// https://strava.pbworks.com/w/page/39241255/v2%20upload%20create

void
ShareDialog::requestUploadStrava()
{
    progressLabel->setText(tr("Upload ride to Strava..."));
    progressBar->setValue(10);

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;

    int year = ride->fileName.left(4).toInt();
    int month = ride->fileName.mid(5,2).toInt();
    int day = ride->fileName.mid(8,2).toInt();
    int hour = ride->fileName.mid(11,2).toInt();
    int minute = ride->fileName.mid(14,2).toInt();;
    int second = ride->fileName.mid(17,2).toInt();;

    QDate rideDate = QDate(year, month, day);
    QTime rideTime = QTime(hour, minute, second);
    QDateTime rideDateTime = QDateTime(rideDate, rideTime);

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestUploadStravaFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    connect(&networkMgr, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(sslErrorHandler(QNetworkReply*, const QList<QSslError> & )));


    TcxFileReader reader;

    STRAVA_URL_SSL = "https://www.strava.com/api/v3/uploads" ; // The V3 API doc said "https://api.strava.com" but it is not working yet

    QUrl url = QUrl( STRAVA_URL_SSL );
    QNetworkRequest request = QNetworkRequest(url);
// GJC test
//QString boundary = QString::number(qrand() * 100, 16);
    QString boundary = QString::number(qrand() * (90000000000ULL) / (RAND_MAX + 1) + 10000000000ULL, 16);
    QByteArray file = reader.toByteArray(mainWindow, ride->ride(), altitudeChk->isChecked(), powerChk->isChecked(), heartrateChk->isChecked(), cadenceChk->isChecked());

    // MULTIPART *****************

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary(boundary.toAscii());

    QHttpPart accessTokenPart;
    accessTokenPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"access_token\""));
    accessTokenPart.setBody(token.toAscii());

    QHttpPart activityTypePart;
    activityTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"activity_type\""));
    activityTypePart.setBody("Ride");

    QHttpPart activityNamePart;
    activityNamePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"activity_name\""));
    activityNamePart.setBody(QString(titleEdit->text() + " #GoldenCheetah").toAscii());

    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data_type\""));
    dataTypePart.setBody("tcx");

    QHttpPart externalIdPart;
    externalIdPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"external_id\""));
    externalIdPart.setBody("test1");

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

    QScopedPointer<QNetworkReply> reply( networkMgr.post(request, multiPart) );
    multiPart->setParent(reply.data());

    progressBar->setValue(40);
    progressLabel->setText(tr("Upload ride... Sending"));



    eventLoop.exec();
}

void
ShareDialog::requestUploadStravaFinished(QNetworkReply *reply)
{
    progressBar->setValue(90);
    progressLabel->setText(tr("Upload to Strava finished."));

    uploadSuccessful = false;
    if (reply->error() != QNetworkReply::NoError)
        qDebug() << "Error from upload " <<reply->error();
    else
    {
        QString response = reply->readLine();

        qDebug() << response;

        QScriptValue sc;
        QScriptEngine se;

        sc = se.evaluate("("+response+")");
        stravaUploadId = sc.property("upload_id").toString();

        ride->ride()->setTag("Strava uploadId", stravaUploadId);
        ride->setDirty(true);

        qDebug() << "uploadId: " << stravaUploadId;

        progressBar->setValue(100);
        uploadSuccessful = true;
    }
    qDebug() << reply->readAll();
}

void
ShareDialog::requestUploadRideWithGPSFinished(QNetworkReply *reply)
{
    progressBar->setValue(90);
    progressLabel->setText(tr("Upload to RideWithGPS finished."));

    uploadSuccessful = false;

    QString response = reply->readLine();
    //qDebug() << response;

    QScriptValue sc;
    QScriptEngine se;

    sc = se.evaluate("("+response+")");
    QString uploadError = sc.property("error").toString();

    if (uploadError.length()>0 || reply->error() != QNetworkReply::NoError) {
        //qDebug() << "Error from upload " <<reply->error();
    }
    else
    {
        QString tripid = sc.property("trip").property("id").toString();

        ride->ride()->setTag("RideWithGPS tripid", tripid);
        ride->setDirty(true);

        //qDebug() << "tripid: " << tripid;

        progressBar->setValue(100);
        uploadSuccessful = true;
    }
}

void
ShareDialog::requestVerifyUpload()
{
    progressBar->setValue(0);
    progressLabel->setText(tr("Ride processing..."));

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestVerifyUploadFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    QByteArray out;

    QUrl url = QUrl(STRAVA_URL_SSL + "/upload/status/"+stravaUploadId+"?token="+token);
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    networkMgr.post( request, out);
    eventLoop.exec();
}

void
ShareDialog::requestVerifyUploadFinished(QNetworkReply *reply)
{
    uploadSuccessful = false;
    if (reply->error() != QNetworkReply::NoError)
        qDebug() << "Error from upload " <<reply->error();
    else {
        QString response = reply->readLine();

        //qDebug() << response;

        QScriptValue sc;
        QScriptEngine se;

        sc = se.evaluate("("+response+")");
        uploadProgress = sc.property("upload_progress").toString();

        //qDebug() << "upload_progress: " << uploadProgress;
        progressBar->setValue(uploadProgress.toInt());

        stravaActivityId = sc.property("activity_id").toString();

        if (stravaActivityId.length() == 0) {
            requestVerifyUpload();
            return;
        }

        ride->ride()->setTag("Strava actityId", stravaActivityId);
        ride->setDirty(true);

        sc = se.evaluate("("+response+")");
        uploadStatus = sc.property("upload_status").toString();

        //qDebug() << "upload_status: " << uploadStatus;
        progressLabel->setText(uploadStatus);

        if (uploadProgress.toInt() < 97) {
            requestVerifyUpload();
            return;
        }
        uploadSuccessful = true;
    }
}
