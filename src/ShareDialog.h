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

#ifndef SHAREDIALOG_H
#define SHAREDIALOG_H
#include "GoldenCheetah.h"

#include <QObject>
#include <QtGui>
#include <QNetworkReply>
#include <QSslError>
#include "MainWindow.h"
#include "RideItem.h"
#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QCheckBox>
#include <QGroupBox>

#ifdef GC_HAVE_LIBOAUTH
extern "C" {
#include <oauth.h>
}
#endif

class ShareDialog;

// uploader to strava.com
class StravaUploader : public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    StravaUploader(Context *context, RideItem *item, ShareDialog *parent = 0);

    void upload();

private slots:
    void requestUploadStrava();
    void requestUploadStravaFinished(QNetworkReply *reply);

    void requestVerifyUpload();
    void requestVerifyUploadFinished(QNetworkReply *reply);

    void okClicked();
    void closeClicked();

private:
    Context *context;
    ShareDialog *parent;
    RideItem *ride;
    QDialog *dialog;

    QString token;

    QString stravaUploadId, stravaActivityId;

    bool loggedIn, uploadSuccessful;
    bool overwrite;

    QString uploadStatus;
    QString uploadProgress;
};

// uploader to ridewithgps.com
class RideWithGpsUploader : public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    RideWithGpsUploader(Context *context, RideItem *item, ShareDialog *parent = 0);

    void upload();

private slots:

    void requestUploadRideWithGPS();
    void requestUploadRideWithGPSFinished(QNetworkReply *reply);

    void okClicked();
    void closeClicked();

private:
    Context *context;
    ShareDialog *parent;
    RideItem *ride;
    QDialog *dialog;

    QString rideWithGpsActivityId;

    bool loggedIn, uploadSuccessful;
    bool overwrite;
};

// uploader to cyclinganalytics.com
class CyclingAnalyticsUploader : public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    CyclingAnalyticsUploader(Context *context, RideItem *item, ShareDialog *parent = 0);

    void upload();

private slots:
    void requestUploadCyclingAnalytics();
    void requestUploadCyclingAnalyticsFinished(QNetworkReply *reply);

    void okClicked();
    void closeClicked();

private:
    Context *context;
    ShareDialog *parent;
    RideItem *ride;
    QDialog *dialog;

    QString token;

    bool loggedIn, uploadSuccessful;
    bool overwrite;

    QString cyclingAnalyticsUploadId;

    QString uploadStatus;
    QString uploadProgress;
};

// uploader to selfloops.com
class SelfLoopsUploader : public QObject
{
    Q_OBJECT
    G_OBJECT

public:
    SelfLoopsUploader(Context *context, RideItem *item, ShareDialog *parent = 0);

    void upload();

private slots:
    void requestUploadSelfLoops();
    void requestUploadSelfLoopsFinished(QNetworkReply *reply);

    void okClicked();
    void closeClicked();

private:
    Context *context;
    ShareDialog *parent;
    RideItem *ride;
    QDialog *dialog;

    QString token;

    bool loggedIn, uploadSuccessful;
    bool overwrite;

    QString selfloopsUploadId, selfloopsActivityId;

    QString uploadStatus;
    QString uploadProgress;
};

class ShareDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

public:
    ShareDialog(Context *context, RideItem *item);

    QProgressBar *progressBar;
    QLabel *progressLabel, *errorLabel;

    QLineEdit *titleEdit;

    //QCheckBox *gpsChk;
    QCheckBox *altitudeChk;
    QCheckBox *powerChk;
    QCheckBox *cadenceChk;
    QCheckBox *heartrateChk;

    int shareSiteCount;
signals:

public slots:
     void upload();

private:
     Context *context;

     QPushButton *uploadButton;
     QPushButton *closeButton;


     QCheckBox *stravaChk;
     QCheckBox *rideWithGPSChk;
     QCheckBox *cyclingAnalyticsChk;
     QCheckBox *selfLoopsChk;

     RideItem *ride;

     StravaUploader *stravaUploader;
     RideWithGpsUploader *rideWithGpsUploader;
     CyclingAnalyticsUploader *cyclingAnalyticsUploader;
     SelfLoopsUploader *selfLoopsUploader;

     QString athleteId;
};



#endif // SHAREDIALOGDIALOG_H
