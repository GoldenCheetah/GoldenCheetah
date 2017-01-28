/*
 * Copyright (c) 2017 Joern Rischmueller (joern.rm@gmail.com)
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

#include "CloudDBTelemetry.h"
#include "CloudDBCommon.h"
#include "GcUpgrade.h"

#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QLabel>
#include <QDateTime>


int CloudDBTelemetryClient::CloudDBTelemetry_UpsertFrequence = 25;


CloudDBTelemetryClient::CloudDBTelemetryClient()
{
}
CloudDBTelemetryClient::~CloudDBTelemetryClient() {
}

void
CloudDBTelemetryClient::upsertTelemetry() {

    // do not update every time, but only every xx times GC is opened to save DB write quota
    int telemetryCount = appsettings->value(NULL, GC_TELEMETRY_UPDATE_COUNTER, 0).toInt();
    if (telemetryCount > 0 ) {
        appsettings->setValue(GC_TELEMETRY_UPDATE_COUNTER, --telemetryCount);
        return;
    }

    QNetworkAccessManager *l_nam = new QNetworkAccessManager(this);

    QString  l_telemetry_url_base = CloudDBCommon::cloudDBBaseURL + QString("telemetry");

    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, l_telemetry_url_base);

    QString os;
#ifdef Q_OS_LINUX
    os = "Linux";
#endif

#ifdef WIN32
    os = "Windows";
#endif

#ifdef Q_OS_MAC
    os = "macOS";
#endif


    // make sure we have a unique ID - if not created yet, create one and store for the future
    QString id = appsettings->value(NULL, GC_TELEMETRY_ID, QUuid::createUuid().toString()).toString();
    appsettings->setValue(GC_TELEMETRY_ID, id);

    // empty lastChange is filled with time.now() in CloudDB
    QString payload = QString("{ \"key\" : \"%1\", \"lastChange\" : \"\", \"operatingSystem\" : \"%2\","
                              " \"version\" : \"%3\", \"increment\" : %4 }")
                      .arg(id).arg( os ).arg( VERSION_LATEST )
                      .arg(CloudDBTelemetryClient::CloudDBTelemetry_UpsertFrequence);
    l_nam->put(request, payload.toUtf8());

    // the request is asynchronously - the user does not care about any response - so we just skip it

    // now we reset the countdown counter
    appsettings->setValue(GC_TELEMETRY_UPDATE_COUNTER, CloudDBTelemetryClient::CloudDBTelemetry_UpsertFrequence);

}


CloudDBAcceptTelemetryDialog::CloudDBAcceptTelemetryDialog()
{

    setWindowTitle(QString(tr("GoldenCheetah - Storage of User Location")));
    setMinimumWidth(550);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *text = new QLabel(this);
    text->setWordWrap(true);
    text->setTextFormat(Qt::RichText);
    text->setText(tr("<center><b>GoldenCheetah User Location Data</b><br>"
                     "We want to start collecting data about where our active users are located and which operating "
                     "system and GoldenCheetah version they are using. The collection is done via our Cloud DB. "
                     "We would like to use your IP address to determine your location and count how often you "
                     "are using GoldenCheetah. Besides the data mentioned we do not store any personal "
                     "information in our Cloud DB. <br><br>"
                     "<b>Can we please record your location, OS and GoldenCheetah version ?"
                     ));

    layout->addWidget(text);

    QHBoxLayout *lastRow = new QHBoxLayout;

    noButton = new QPushButton(tr("No"), this);
    connect(noButton, SIGNAL(clicked()), this, SLOT(rejectConditions()));
    yesButton = new QPushButton(tr("Yes"), this);
    yesButton->setDefault(true);
    connect(yesButton, SIGNAL(clicked()), this, SLOT(acceptConditions()));

    lastRow->addStretch();
    lastRow->addWidget(noButton);
    lastRow->addWidget(yesButton);

    layout->addLayout(lastRow);

}

void CloudDBAcceptTelemetryDialog::acceptConditions() {

    // document the decision
    appsettings->setValue(GC_ALLOW_TELEMETRY, true);
    appsettings->setValue(GC_ALLOW_TELEMETRY_DATE, QDateTime::currentDateTime().toString(Qt::ISODate));

    accept();
}

void CloudDBAcceptTelemetryDialog::rejectConditions() {

    // document the decision
    appsettings->setValue(GC_ALLOW_TELEMETRY, false);
    appsettings->setValue(GC_ALLOW_TELEMETRY_DATE, QDateTime::currentDateTime().toString(Qt::ISODate));

    reject();
}











