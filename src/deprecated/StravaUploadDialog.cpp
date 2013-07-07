/*
 * Copyright (c) 2011-2013 Justin F. Knotzke (jknotzke@shampoo.ca),
 *                         Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "StravaUploadDialog.h"
#include "Settings.h"
#include <QHttp>
#include <QUrl>
#include <QScriptEngine>
#include "TimeUtils.h"
#include "Units.h"

// acccess to metrics
#include "MetricAggregator.h"
#include "RideMetric.h"
#include "DBAccess.h"
#include "TcxRideFile.h"

StravaUploadDialog::StravaUploadDialog(MainWindow *mainWindow, RideItem *item) :
    mainWindow(mainWindow)
{
    STRAVA_URL1 = "http://www.strava.com/api/v1/";
    STRAVA_URL2 = "http://www.strava.com/api/v2/";
    STRAVA_URL_SSL = "https://www.strava.com/api/v2/";

    ride = item;
    uploadId = ride->ride()->getTag("Strava uploadId", "");
    activityId = ride->ride()->getTag("Strava actityId", "");

    //setAttribute(Qt::WA_DeleteOnClose); // we allocate these on the stack
    setWindowTitle("Strava");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *groupBox = new QGroupBox(tr("Choose which channels you wish to send: "));

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

    QGridLayout *vbox = new QGridLayout();
    //vbox->addWidget(gpsChk,0,0);
    vbox->addWidget(powerChk,0,0);
    vbox->addWidget(altitudeChk,0,1);
    vbox->addWidget(cadenceChk,1,0);
    vbox->addWidget(heartrateChk,1,1);

    groupBox->setLayout(vbox);
    mainLayout->addWidget(groupBox);

    // show progress
    QVBoxLayout *progressLayout = new QVBoxLayout;
    progressBar = new QProgressBar(this);
    progressLabel = new QLabel("", this);

    progressLayout->addWidget(progressBar);
    progressLayout->addWidget(progressLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    uploadButton = new QPushButton(tr("&Upload Ride"), this);
    buttonLayout->addWidget(uploadButton);
    /*searchActivityButton = new QPushButton(tr("&Update info"), this);
    buttonLayout->addWidget(searchActivityButton);
    getActivityButton = new QPushButton(tr("&Load Ride"), this);
    buttonLayout->addWidget(getActivityButton);*/
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);

    //mainLayout->addWidget(groupBox);
    mainLayout->addLayout(progressLayout);
    mainLayout->addLayout(buttonLayout);

    connect(uploadButton, SIGNAL(clicked()), this, SLOT(uploadToStrava()));
    //connect(searchActivityButton, SIGNAL(clicked()), this, SLOT(getActivityFromStrava()));
    //connect(getActivityButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    //connect(workoutTimeChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    //connect(timeRidingChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    //connect(totalDistanceChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));

    //uploadToStrava();
}


void
StravaUploadDialog::uploadToStrava()
{
    show();
    overwrite = true;

    if(activityId.length()>0)
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

    requestLogin();

    if(!loggedIn)
    {
        /*QMessageBox aMsgBox;
        aMsgBox.setText(tr("Cannot login to Strava. Check username/password"));
        aMsgBox.exec();*/
        reject();
    }
    requestUpload();

    if(!uploadSuccessful)
    {
        progressLabel->setText("Error uploading to Strava");
    }
    else
    {
        //requestVerifyUpload();
        progressLabel->setText(tr("Successfully uploaded to Strava\n")+uploadStatus);
    }
    uploadButton->setVisible(false);
    cancelButton->setText("OK");
    QApplication::processEvents();
}

void
StravaUploadDialog::getActivityFromStrava()
{
    if (token.length()==0)
        requestLogin();

    if(!loggedIn)
    {
        /*QMessageBox aMsgBox;
        aMsgBox.setText(tr("Cannot login to Strava. Check username/password"));
        aMsgBox.exec();*/
        reject();
    }

    requestSearchRide();
    //verifyUpload();
    //QMessageBox aMsgBox;
    //aMsgBox.setText(tr("Successfully uploaded to Strava\n")+uploadStatus);
    //aMsgBox.exec();

    return;
}

void
StravaUploadDialog::okClicked()
{
    dialog->accept();
    return;
}

void
StravaUploadDialog::cancelClicked()
{
    dialog->reject();
    return;
}


void
StravaUploadDialog::requestLogin()
{
    progressLabel->setText(tr("Login..."));
    progressBar->setValue(5);

    QString username = appsettings->cvalue(mainWindow->cyclist, GC_STRUSER).toString();
    QString password = appsettings->cvalue(mainWindow->cyclist, GC_STRPASS).toString();

    QNetworkAccessManager networkMgr;
    QEventLoop eventLoop;
    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestLoginFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    QByteArray data;
    /*data += "{\"email\": \"" + username + "\",";
    data += "\"password\": \"" + password + "\",";
    data += "\"agreed_to_terms\": \"true\"}";*/

    QUrl params;

    params.addQueryItem("email", username);
    params.addQueryItem("password",password);
    params.addQueryItem("agreed_to_terms", "true");
    data = params.encodedQuery();

    QUrl url = QUrl( STRAVA_URL_SSL + "/authentication/login");
    QNetworkRequest request = QNetworkRequest(url);
    //request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    networkMgr.post( request, data);
    eventLoop.exec();
}

void
StravaUploadDialog::requestLoginFinished(QNetworkReply *reply)
{
    loggedIn = false;


    if (reply->error() == QNetworkReply::NoError)
    {
        QString response = reply->readLine();
        //qDebug() << response;

        if(response.contains("token"))
        {
            QScriptValue sc;
            QScriptEngine se;

            sc = se.evaluate("("+response+")");
            token = sc.property("token").toString();
            athleteId = sc.property("athlete").property("id").toString();

            loggedIn = true;
        }
        else
        {
            token = "";
        }
    }
    else
    {
        token = "";
        //qDebug() <<  reply->error();

        #ifdef Q_OS_MACX
        #define GC_PREF tr("Golden Cheetah->Preferences")
        #else
        #define GC_PREF tr("Tools->Options")
        #endif
        QString advise = QString(tr("Please make sure to fill the Strava login info found under\n %1.")).arg(GC_PREF);
        QMessageBox err(QMessageBox::Critical, tr("Strava login Error"), advise);
        err.exec();
    }
}


// Documentation is at:
// https://strava.pbworks.com/w/page/39241255/v2%20upload%20create

void
StravaUploadDialog::requestUpload()
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

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestUploadFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    bool json = false;

    if (json) {
        QString out;

        QVector<RideFilePoint*> vectorPoints = ride->ride()->dataPoints();
        int totalSize = vectorPoints.size();

        int size = 0;

        out += "{\"token\": \"" + token + "\",\"data\":[";
        foreach (const RideFilePoint *point, ride->ride()->dataPoints())
        {
            size++;

            if (point->secs == 0.0)
                continue;

            diffSecs = point->secs - prevSecs;
            prevSecs = point->secs;
            rideDateTime = rideDateTime.addSecs(diffSecs);

            out += "[\"";
            out += rideDateTime.toUTC().toString(Qt::ISODate);
            out += "\",";
            out += QString("%1").arg(point->lat,0,'f',GPS_COORD_TO_STRING);
            out += ",";
            out += QString("%1").arg(point->lon,0,'f',GPS_COORD_TO_STRING);

            if (altitudeChk->isChecked()) {
                out += ",";
                out += QString("%1").arg(point->alt);

            }
            if (powerChk->isChecked()) {
                out += ",";
                out += QString("%1").arg(point->watts);
            }
            if (altitudeChk->isChecked()) {
                out += ",";
                out += QString("%1").arg(point->cad);
            }
            if (heartrateChk->isChecked()) {
                out += ",";
                out += QString("%1").arg(point->hr);
            }
            out += "]";
            if(totalSize == size)
                out += "],";
           else
               out += ",";
        }
        out += "\"type\": \"json\", ";
        out += "\"data_fields\": \[\"time\", \"latitude\", \"longitude\"";

        if (altitudeChk->isChecked())
            out += ", \"elevation\"";
        if (powerChk->isChecked())
            out += ", \"watts\"";
        if (cadenceChk->isChecked())
            out += ", \"cadence\"";
        if (heartrateChk->isChecked())
            out += ", \"heartrate\"";
        out += "]}";

        QUrl url = QUrl(STRAVA_URL2 + "/upload");
        QNetworkRequest request = QNetworkRequest(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        progressBar->setValue(40);
        progressLabel->setText(tr("Upload ride... Sending"));

        networkMgr.post( request, out.toAscii());
    } else {
        QByteArray data;
        QUrl params;
        TcxFileReader reader;

        params.addQueryItem("token", token);
        params.addQueryItem("type", "tcx");
        params.addQueryItem("data", reader.toByteArray(mainWindow, ride->ride(), altitudeChk->isChecked(), powerChk->isChecked(), heartrateChk->isChecked(), cadenceChk->isChecked()));
        data = params.encodedQuery();

        QUrl url = QUrl(STRAVA_URL2 + "/upload");
        QNetworkRequest request = QNetworkRequest(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

        progressBar->setValue(40);
        progressLabel->setText(tr("Upload ride... Sending"));

        networkMgr.post( request, data);
    }

    //qDebug() << out;

    eventLoop.exec();
}

void
StravaUploadDialog::requestUploadFinished(QNetworkReply *reply)
{
    progressBar->setValue(90);
    progressLabel->setText(tr("Upload finished."));

    uploadSuccessful = false;
    if (reply->error() != QNetworkReply::NoError)
        qDebug() << "Error from upload " <<reply->error();
    else
    {
        QString response = reply->readLine();

        //qDebug() << response;

        QScriptValue sc;
        QScriptEngine se;

        sc = se.evaluate("("+response+")");
        uploadId = sc.property("upload_id").toString();

        ride->ride()->setTag("Strava uploadId", uploadId);
        ride->setDirty(true);

        //qDebug() << "uploadId: " << uploadId;

        progressBar->setValue(100);
        uploadSuccessful = true;
    }
    //qDebug() << reply->readAll();
}

void
StravaUploadDialog::requestVerifyUpload()
{
    progressBar->setValue(0);
    progressLabel->setText(tr("Ride processing..."));

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestVerifyUploadFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    QByteArray out;


    QUrl url = QUrl(STRAVA_URL2 + "/upload/status/"+uploadId+"?token="+token);
    QNetworkRequest request = QNetworkRequest(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    networkMgr.post( request, out);
    eventLoop.exec();
}

void
StravaUploadDialog::requestVerifyUploadFinished(QNetworkReply *reply)
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

        activityId = sc.property("activity_id").toString();

        if (activityId.length() == 0) {
            requestVerifyUpload();
            return;
        }

        ride->ride()->setTag("Strava actityId", activityId);
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

    //qDebug() << reply->readAll();
}

void
StravaUploadDialog::requestSearchRide()
{
    progressBar->setValue(0);
    progressLabel->setText(tr("Searching corresponding Ride"));

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestSearchRideFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));
    QByteArray out;


    QUrl url = QUrl(STRAVA_URL1 + "rides?athleteId="+athleteId+"&startDate="+ride->ride()->startTime().toString("yyyy-MM-dd")+"&endDate="+ride->ride()->startTime().addDays(1).toString("yyyy-MM-dd"));

    QNetworkRequest request = QNetworkRequest(url);
    //request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    networkMgr.get(request);
    eventLoop.exec();
}

void
StravaUploadDialog::requestSearchRideFinished(QNetworkReply *reply)
{
    uploadSuccessful = false;
    if (reply->error() != QNetworkReply::NoError)
        qDebug() << "Error from upload " <<reply->error();
    else {
        QString response = reply->readLine();

        //qDebug() << response;
    }

    //qDebug() << reply->readAll();
}
