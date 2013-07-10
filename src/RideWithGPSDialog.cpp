/*
 * Copyright (c) 2012 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "RideWithGPSDialog.h"
#include "Athlete.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"

// acccess to metrics
#include "MetricAggregator.h"
#include "RideMetric.h"
#include "DBAccess.h"
#include "TcxRideFile.h"

RideWithGPSDialog::RideWithGPSDialog(Context *context, RideItem *item) :
    context(context)
{
    RIDE_WITH_GPS_URL = "http://ridewithgps.com";

    ride = item;

    setWindowTitle("RideWithGPS");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    /*QGroupBox *groupBox = new QGroupBox(tr("Choose which metrics you wish to send: "));

    workoutTimeChk = new QCheckBox(tr("Workout Time"));
    timeRidingChk = new QCheckBox(tr("Time Riding"));
    totalDistanceChk = new QCheckBox(tr("Total Distance"));

    QGridLayout *vbox = new QGridLayout();
    vbox->addWidget(workoutTimeChk,0,0);
    vbox->addWidget(timeRidingChk,0,1);
    vbox->addWidget(totalDistanceChk,1,1);
    groupBox->setLayout(vbox);*/

    // show progress
    QVBoxLayout *progressLayout = new QVBoxLayout;
    progressBar = new QProgressBar(this);
    progressLabel = new QLabel("", this);

    progressLayout->addWidget(progressBar);
    progressLayout->addWidget(progressLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    /*uploadButton = new QPushButton(tr("&Upload Ride"), this);
    buttonLayout->addWidget(uploadButton);
    searchActivityButton = new QPushButton(tr("&Update info"), this);
    buttonLayout->addWidget(searchActivityButton);
    getActivityButton = new QPushButton(tr("&Load Ride"), this);
    buttonLayout->addWidget(getActivityButton);*/
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);

    //mainLayout->addWidget(groupBox);
    mainLayout->addLayout(progressLayout);
    mainLayout->addLayout(buttonLayout);

    //connect(uploadButton, SIGNAL(clicked()), this, SLOT(uploadToStrava()));
    //connect(searchActivityButton, SIGNAL(clicked()), this, SLOT(getActivityFromStrava()));
    //connect(getActivityButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    //connect(workoutTimeChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    //connect(timeRidingChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));
    //connect(totalDistanceChk, SIGNAL(stateChanged(int)),  this, SLOT(onCheck(int)));

    upload();
}


void
RideWithGPSDialog::upload()
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

    requestUpload();

    if(!uploadSuccessful)
    {
        progressLabel->setText(tr("Error uploading to RideWithGPS")+"\n"+uploadError);
    }
    else
    {
        progressLabel->setText(tr("Successfully uploaded to RideWithGPS"));
    }
    cancelButton->setText("OK");
    QApplication::processEvents();
}



void
RideWithGPSDialog::okClicked()
{
    dialog->accept();
    return;
}

void
RideWithGPSDialog::cancelClicked()
{
    dialog->reject();
    return;
}

void
RideWithGPSDialog::requestUpload()
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

        data += ", \"e\": ";
        data += QString("%1").arg(point->alt);
        data += ", \"p\": ";
        data += QString("%1").arg(point->watts);
        data += ", \"c\": ";
        data += QString("%1").arg(point->cad);
        data += ", \"h\": ";
        data += QString("%1").arg(point->hr);

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
RideWithGPSDialog::requestUploadFinished(QNetworkReply *reply)
{
    progressBar->setValue(90);
    progressLabel->setText(tr("Upload finished."));

    uploadSuccessful = false;

    QString response = reply->readLine();
    //qDebug() << response;

    QScriptValue sc;
    QScriptEngine se;

    sc = se.evaluate("("+response+")");
    uploadError = sc.property("error").toString();

    if (uploadError.length()>0 || reply->error() != QNetworkReply::NoError) {
        //qDebug() << "Error from upload " <<reply->error();
    }
    else
    {
        tripid = sc.property("trip").property("id").toString();

        ride->ride()->setTag("RideWithGPS tripid", tripid);
        ride->setDirty(true);

        //qDebug() << "tripid: " << tripid;

        progressBar->setValue(100);
        uploadSuccessful = true;
    }
}
