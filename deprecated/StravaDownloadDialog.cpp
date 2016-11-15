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

#include "StravaDownloadDialog.h"
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
#include "RideImportWizard.h"
#include "TcxRideFile.h"


// Download a Strava activity using the Strava API V1
// http://www.strava.com/api/v1/streams/9999
// which returns a json string with time-series of speed, position, heartrate, etc.
//
// Be aware that these APIs are being deprecated.
// The new API coming in early 2013.
//
// TODO work with the v3
// http://strava.github.com/api/v3/activities/streams/

StravaDownloadDialog::StravaDownloadDialog(MainWindow *mainWindow) :
    mainWindow(mainWindow)
{
    STRAVA_URL_V1 = "http://www.strava.com/api/v1/";
    STRAVA_URL_V2 = "http://www.strava.com/api/v2/";
    STRAVA_URL_V3 = "http://www.strava.com/api/v3/";

    setWindowTitle("Strava download");
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *groupBox = new QGroupBox(tr("Choose activityId to download: "));

    QHBoxLayout *hbox = new QHBoxLayout();
    activityIdEdit = new QLineEdit();
    hbox->addWidget(activityIdEdit);

    groupBox->setLayout(hbox);
    mainLayout->addWidget(groupBox);

    // show progress
    QVBoxLayout *progressLayout = new QVBoxLayout;
    progressBar = new QProgressBar(this);
    progressLabel = new QLabel("", this);

    progressLayout->addWidget(progressBar);
    progressLayout->addWidget(progressLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout;

    downloadButton = new QPushButton(tr("&Download Ride"), this);
    buttonLayout->addWidget(downloadButton);

    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(progressLayout);
    mainLayout->addLayout(buttonLayout);

    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadRide()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void
StravaDownloadDialog::downloadRide()
{
    activityId = activityIdEdit->text();
    if (activityId.trimmed().isEmpty()) {
        progressLabel->setText(tr("Enter an activity Id"));
        return;
    }

    if (activityId.toLong() == 0)  {
        progressLabel->setText(tr("Enter a valid activity Id"));
        return;
    }

    fileNames = new QStringList();

    progressBar->setValue(5);
    count = 1;
    requestRideDetail();
    //requestListRides();
}

void
StravaDownloadDialog::requestRideDetail()
{
    show();

    tmp = new QTemporaryFile(QDir(QDir::tempPath()).absoluteFilePath(".download."+activityId+".XXXXXX.strava"));
    //QString tmpl = mainWindow->home.absoluteFilePath(".download.XXXXXX."+activityId+".strava"); //
    //tmp.setFileTemplate(tmpl);
    tmp->setAutoRemove(true);

    if (!tmp->open()) {
       progressLabel->setText(tr("Failed to create temporary file ")
            + tmp->fileName() + ": " + tmp->error());

        return;
    }
    tmp->resize(0);

    progressLabel->setText(tr("Get activity %1 details").arg(activityId));

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestRideDetailFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    QUrl url = QUrl(STRAVA_URL_V1 + "rides/"+activityId);

    QNetworkRequest request = QNetworkRequest(url);

    networkMgr.get(request);
    eventLoop.exec();
    progressBar->setValue(progressBar->value()+5/count);
}

void
StravaDownloadDialog::requestRideDetailFinished(QNetworkReply *reply)
{
    progressBar->setValue(15);

    if (reply->error() != QNetworkReply::NoError)
        progressLabel->setText(QString(tr("Error from ride details %1")).arg(reply->error()));
    else {
        QString response = reply->readLine();

        QScriptValue sc;
        QScriptEngine se;

        sc = se.evaluate("("+response+")");
        QString error = sc.property("error").toString();
        if (!error.isEmpty()) {
            progressLabel->setText(error);
            return;
        }

        tmp->write(response.toAscii());
        progressBar->setValue(progressBar->value()+10/count);

        requestDownloadRide();
    }
}

void
StravaDownloadDialog::requestDownloadRide()
{
    show();

    progressLabel->setText(tr("Download activity %1").arg(activityId));

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestDownloadRideFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    QUrl url = QUrl(STRAVA_URL_V1 + "streams/"+activityId);

    QNetworkRequest request = QNetworkRequest(url);

    networkMgr.get(request);
    eventLoop.exec();
    progressBar->setValue(progressBar->value()+20/count);
}

void
StravaDownloadDialog::requestDownloadRideFinished(QNetworkReply *reply)
{
    progressBar->setValue(60);

    if (reply->error() != QNetworkReply::NoError)
        progressLabel->setText(QString(tr("Error from upload %1")).arg(reply->error()));
    else {
        QString response = reply->readLine();

        progressBar->setValue(70);
        tmp->seek(tmp->size()-2);
        tmp->write(",\"streams\":");
        tmp->write(response.toAscii());
        tmp->write("},\"api_version\":\"1\"}");
        tmp->close();
        progressBar->setValue(progressBar->value()+60/count);

        fileNames->append(tmp->fileName());

        if (fileNames->count() == count) {
            close();
            RideImportWizard *import = new RideImportWizard(*fileNames, mainWindow->home, mainWindow);
            import->process();
            progressBar->setValue(100);
        }
    }
}

void
StravaDownloadDialog::okClicked()
{
    dialog->accept();
    return;
}

void
StravaDownloadDialog::cancelClicked()
{
    dialog->reject();
    return;
}


void
StravaDownloadDialog::requestLogin()
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

    QUrl url = QUrl( STRAVA_URL_V2_SSL + "/authentication/login");
    QNetworkRequest request = QNetworkRequest(url);
    //request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

    networkMgr.post( request, data);
    eventLoop.exec();
}

void
StravaDownloadDialog::requestLoginFinished(QNetworkReply *reply)
{
    loggedIn = false;


    if (reply->error() == QNetworkReply::NoError)
    {
        QString response = reply->readLine();

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


void
StravaDownloadDialog::requestListRides()
{
    QString athleteId = "775290"; //Rica 131476

    progressBar->setValue(0);
    progressLabel->setText(tr("Download athlete"));

    QEventLoop eventLoop;
    QNetworkAccessManager networkMgr;

    connect(&networkMgr, SIGNAL(finished(QNetworkReply*)), this, SLOT(requestListRidesFinished(QNetworkReply*)));
    connect(&networkMgr, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    QUrl url = QUrl(STRAVA_URL_V1 + "rides?athleteId="+athleteId+"&offset=147");//+"&offset=0"

    QNetworkRequest request = QNetworkRequest(url);

    networkMgr.get(request);
    eventLoop.exec();
    progressBar->setValue(5);
}

void
StravaDownloadDialog::requestListRidesFinished(QNetworkReply *reply)
{
    progressBar->setValue(10);

    if (reply->error() != QNetworkReply::NoError)
        qDebug() << "Error from upload " <<reply->error();
    else {
        QString response = reply->readLine();

        QScriptValue sc;
        QScriptEngine se;

        sc = se.evaluate("("+response+")");
        count = sc.property("rides").property("length").toInteger();

        //count = 10;
        for(int i = 0; i < count; i++) {
            activityId = sc.property("rides").property(i).property("id").toString();

            requestRideDetail();
        }


    }
}


