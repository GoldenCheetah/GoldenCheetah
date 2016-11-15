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

#ifndef STRAVADOWNLOADDIALOG_H
#define STRAVADOWNLOADDIALOG_H
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

class StravaDownloadDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

public:
     StravaDownloadDialog(MainWindow *mainWindow);

private slots:

     void requestLogin();
     void requestLoginFinished(QNetworkReply *reply);

     void downloadRide();

     void requestRideDetail();
     void requestRideDetailFinished(QNetworkReply *reply);

     void requestDownloadRide();
     void requestDownloadRideFinished(QNetworkReply *reply);

     void requestListRides();
     void requestListRidesFinished(QNetworkReply *reply);

     void okClicked();
     void cancelClicked();

private:
     QDialog *dialog;

     QLineEdit *activityIdEdit;

     QPushButton *downloadButton;
     QPushButton *cancelButton;
     MainWindow *mainWindow;

     QProgressBar *progressBar;
     QLabel *progressLabel;

     QTemporaryFile *tmp;
     QStringList *fileNames;
     RideFile *rideFile;

     int count;

     QString athleteId;
     QString token;
     QString activityId;

     QString err;
     QString STRAVA_URL_V1,
             STRAVA_URL_V2, STRAVA_URL_V2_SSL,
             STRAVA_URL_V3;

     bool loggedIn;
};

#endif // STRAVADOWNLOADDIALOG_H
