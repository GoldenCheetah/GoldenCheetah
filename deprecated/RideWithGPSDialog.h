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

#ifndef RIDEWITHGPSDIALOG_H
#define RIDEWITHGPSDIALOG_H
#include "GoldenCheetah.h"

#include <QObject>
#include <QtGui>
#include <QHttp>
#include <QUrl>
#include <QScriptEngine>
#include <QNetworkReply>
#include "Context.h"
#include "RideItem.h"

class RideWithGPSDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

public:
     RideWithGPSDialog(Context *context, RideItem *item);

signals:

public slots:
     void upload();

private slots:
     void requestUpload();
     void requestUploadFinished(QNetworkReply *reply);

     void okClicked();
     void cancelClicked();

private:
     QDialog *dialog;

     QPushButton *uploadButton;
     QPushButton *searchActivityButton;
     QPushButton *getActivityButton;
     QPushButton *cancelButton;
     Context *context;
     QCheckBox *workoutTimeChk;
     QCheckBox *timeRidingChk;
     QCheckBox *totalDistanceChk;

     QLineEdit *twitterMessageEdit;

     QProgressBar *progressBar;
     QLabel *progressLabel;

     RideItem *ride;

     QString athleteId;
     QString token;
     QString tripid;
     QString activityId;
     QString uploadStatus;
     QString uploadProgress;
     QString uploadError;

     QString RIDE_WITH_GPS_URL;

     bool overwrite, loggedIn, uploadSuccessful;
};

#endif // RIDEWITHGPSDIALOG_H
