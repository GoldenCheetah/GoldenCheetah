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
#include <QNetworkCookie>
#include <QSslError>
#include "MainWindow.h"
#include "RideItem.h"
#include <QDialog>
#include <QProgressBar>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QCheckBox>
#include <QGroupBox>

class ShareDialog;

// abstract base class for all uploaders
class ShareDialogUploader : public QObject {
    Q_OBJECT
    G_OBJECT

public:
    ShareDialogUploader( const QString &name, Context *context, RideItem *item, ShareDialog *parent = 0)
        : _name( name ), context( context ), ride( item ), parent(parent)
        {};

    const QString &name() { return _name; };
    virtual bool canUpload( QString &err );
    virtual bool wasUploaded();
    virtual void upload()=0;

    QString insertedName;

private:
    QString _name;

protected:
    Context *context;
    RideItem *ride;
    ShareDialog *parent;
};

// uploader to strava.com
class StravaUploader : public ShareDialogUploader
{
    Q_OBJECT
    G_OBJECT

public:
    StravaUploader(Context *context, RideItem *item, ShareDialog *parent = 0);

    virtual bool canUpload( QString &err );
    virtual bool wasUploaded();
    virtual void upload();

private slots:
    void requestUploadStrava();
    void requestUploadStravaFinished(QNetworkReply *reply);

    void requestVerifyUpload();
    void requestVerifyUploadFinished(QNetworkReply *reply);

private:
    QString token;

    bool loggedIn, uploadSuccessful;

    QEventLoop *eventLoop;
    QNetworkAccessManager *networkManager;

    int stravaUploadId;
    int stravaActivityId;
    QString uploadStatus;
    int uploadProgress;
};

// uploader to ridewithgps.com
class RideWithGpsUploader : public ShareDialogUploader
{
    Q_OBJECT
    G_OBJECT

public:
    RideWithGpsUploader(Context *context, RideItem *item, ShareDialog *parent = 0);

    virtual bool canUpload( QString &err );
    virtual bool wasUploaded();
    virtual void upload();

private slots:

    void requestUploadRideWithGPS();
    void requestUploadRideWithGPSFinished(QNetworkReply *reply);

private:
    QString rideWithGpsActivityId;

    bool loggedIn, uploadSuccessful;
};

// uploader to cyclinganalytics.com
class CyclingAnalyticsUploader : public ShareDialogUploader
{
    Q_OBJECT
    G_OBJECT

public:
    CyclingAnalyticsUploader(Context *context, RideItem *item, ShareDialog *parent = 0);

    virtual bool canUpload( QString &err );
    virtual bool wasUploaded();
    virtual void upload();

private slots:
    void requestUploadCyclingAnalytics();
    void requestUploadCyclingAnalyticsFinished(QNetworkReply *reply);

private:
    QString token;

    bool loggedIn, uploadSuccessful;

    QString uploadStatus;
    int uploadProgress;
    int cyclingAnalyticsUploadId;
};

// uploader to selfloops.com
class SelfLoopsUploader : public ShareDialogUploader
{
    Q_OBJECT
    G_OBJECT

public:
    SelfLoopsUploader(Context *context, RideItem *item, ShareDialog *parent = 0);

    virtual bool canUpload( QString &err );
    virtual bool wasUploaded();
    virtual void upload();

private slots:
    void requestUploadSelfLoops();
    void requestUploadSelfLoopsFinished(QNetworkReply *reply);

private:
    QString token;

    bool loggedIn, uploadSuccessful;

    int selfloopsUploadId, selfloopsActivityId;

    QString uploadStatus;
    int uploadProgress;
};

#if 0 // Not enabled in v3.1
// uploader to connect.garmin.com
class GarminUploader : public ShareDialogUploader
{
    Q_OBJECT
    G_OBJECT

public:
    GarminUploader(Context *context, RideItem *item, ShareDialog *parent = 0);

    virtual bool canUpload( QString &err );
    virtual bool wasUploaded();
    virtual void upload();

private slots:
    void requestFlowExecutionKey();
    void requestFlowExecutionKeyFinished(QNetworkReply *reply);

    void requestLoginGarmin();
    void requestLoginGarminFinished(QNetworkReply *reply);

    void requestUploadGarmin();
    void requestUploadGarminFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager networkMgr;

    QUrl serverUrl;
    QString flowExecutionKey;
    QString ticket;

    bool loggedIn, uploadSuccessful;

    QString garminUploadId, garminActivityId;

    QString uploadStatus;
    int uploadProgress;
};
#endif

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
    QCheckBox *privateChk;
    QCheckBox *commuteChk;
    QCheckBox *trainerChk;

    int shareSiteCount;
signals:

public slots:
     void upload();

private slots:
     void okClicked();
     void closeClicked();

private:
     void doUploader( ShareDialogUploader *uploader );

     Context *context;
     QDialog *dialog;

     QPushButton *uploadButton;
     QPushButton *closeButton;


     QCheckBox *stravaChk;
     QCheckBox *cyclingAnalyticsChk;
     QCheckBox *rideWithGPSChk;
     QCheckBox *selfLoopsChk;
     QCheckBox *veloHeroChk;
     QCheckBox *trainingstagebuchChk;
     QCheckBox *sportplushealthChk;
     //QCheckBox *garminChk;

     RideItem *ride;

     ShareDialogUploader *stravaUploader;
     ShareDialogUploader *cyclingAnalyticsUploader;
     ShareDialogUploader *rideWithGpsUploader;
     ShareDialogUploader *selfLoopsUploader;
     ShareDialogUploader *veloHeroUploader;
     ShareDialogUploader *trainingstagebuchUploader;
     ShareDialogUploader *sportplushealthUploader;
     //ShareDialogUploader *garminUploader;

     QString athleteId;
};



#endif // SHAREDIALOGDIALOG_H
