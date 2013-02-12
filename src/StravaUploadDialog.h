/*
 * Copyright (c) 2011-2013 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#ifndef STRAVAUPLOADDIALOG_H
#define STRAVAUPLOADDIALOG_H
#include "GoldenCheetah.h"

#include <QObject>
#include <QtGui>
#include "MainWindow.h"
#include "RideItem.h"

#ifdef GC_HAVE_LIBOAUTH
extern "C" {
#include <oauth.h>
}
#endif

class StravaUploadDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

public:
     StravaUploadDialog(MainWindow *mainWindow, RideItem *item);

signals:

public slots:
     void uploadToStrava();

private slots:
     void getActivityFromStrava();

     void requestLogin();
     void requestLoginFinished(QNetworkReply *reply);

     void requestUpload();
     void requestUploadFinished(QNetworkReply *reply);

     void requestVerifyUpload();
     void requestVerifyUploadFinished(QNetworkReply *reply);

     void requestSearchRide();
     void requestSearchRideFinished(QNetworkReply *reply);

     void okClicked();
     void cancelClicked();

private:
     QDialog *dialog;

     QPushButton *uploadButton;
     QPushButton *searchActivityButton;
     QPushButton *getActivityButton;
     QPushButton *cancelButton;
     MainWindow *mainWindow;

     //QCheckBox *gpsChk;
     QCheckBox *altitudeChk;
     QCheckBox *powerChk;
     QCheckBox *cadenceChk;
     QCheckBox *heartrateChk;

     QLineEdit *twitterMessageEdit;

     QProgressBar *progressBar;
     QLabel *progressLabel;

     RideItem *ride;

     QString athleteId;
     QString token;
     QString uploadId;
     QString activityId;
     QString uploadStatus;
     QString uploadProgress;

     QString STRAVA_URL1, STRAVA_URL2, STRAVA_URL_SSL;

     bool overwrite, loggedIn, uploadSuccessful;
};

#endif // STRAVAUPLOADDIALOG_H
