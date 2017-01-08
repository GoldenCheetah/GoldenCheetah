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

#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>
#include <QEventLoop>
#include <QLabel>
#include <QDateTime>



CloudDBTelemetryClient::CloudDBTelemetryClient()
{
  // only static members
}
CloudDBTelemetryClient::~CloudDBTelemetryClient() {
  // only static members
}

void
CloudDBTelemetryClient::storeTelemetry() {

    QScopedPointer<QNetworkAccessManager> l_nam(new QNetworkAccessManager());
    QNetworkReply *l_reply;

    QString  l_telemetry_url_base = CloudDBCommon::cloudDBBaseURL + QString("telemetry");

    QNetworkRequest request;
    CloudDBCommon::prepareRequest(request, l_telemetry_url_base);
    // empty createDate is filled with time.now() in CloudDB
    l_reply = l_nam->post(request, "{ \"createDate\": \"\" }");

    // blocking request (need to wait otherwise NetworkAccessManager is destroyed for call finished)
    QEventLoop loop;
    connect(l_reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    // ignore any errors or reply - user does not need to be informed in case of problems
}


CloudDBAcceptTelemetryDialog::CloudDBAcceptTelemetryDialog()
{

    setWindowTitle(QString(tr("GoldenCheetah - Storage of User Location")));
    setMinimumWidth(550);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QPushButton *important = new QPushButton(style()->standardIcon(QStyle::SP_MessageBoxInformation), "", this);
    important->setFixedSize(80,80);
    important->setFlat(true);
    important->setIconSize(QSize(80,80));
    important->setAutoFillBackground(false);
    important->setFocusPolicy(Qt::NoFocus);

    QLabel *header = new QLabel(this);
    header->setWordWrap(true);
    header->setTextFormat(Qt::RichText);
    header->setText(QString(tr("<b><big>Please read carefully before accepting !</big></b>")));

    QHBoxLayout *toprow = new QHBoxLayout;
    toprow->addWidget(important);
    toprow->addWidget(header);
    layout->addLayout(toprow);

    QLabel *text = new QLabel(this);
    text->setWordWrap(true);
    text->setTextFormat(Qt::RichText);
    text->setText(tr("<center><b>GoldenCheetah User Location Storage</b><br>"
                     "GoldenCheetah would like to store your location, retrieved from your IP address "
                     "to get an overview, where GoldenCheetah users are located to create location aware "
                     "statics of GoldenCheetah users.<br>"
                     "The location will be derived and stored once from your current IP address.<br>"
                     "GoldenCheetah will NOT store your IP address, but only derived location information "
                     "which are Country, City, Region and City Lattitude/Longitude.<br>"
                     ));

    layout->addWidget(text);

    proceedButton = new QPushButton(tr("Yes, a agree that my current location is stored in GoldenCheetah's CloudDB"), this);
    proceedButton->setEnabled(true);
    connect(proceedButton, SIGNAL(clicked()), this, SLOT(acceptConditions()));
    abortButton = new QPushButton(tr("No, I do not want my location to be stored in GoldenCheetah's CloudDB"), this);
    abortButton->setDefault(true);
    connect(abortButton, SIGNAL(clicked()), this, SLOT(rejectConditions()));

    layout->addStretch();
    layout->addWidget(abortButton);
    layout->addWidget(proceedButton);

}

void CloudDBAcceptTelemetryDialog::acceptConditions() {

    // document the decision
    appsettings->setValue(GC_ALLOW_TELEMETRY, "userAccepted");
    appsettings->setValue(GC_ALLOW_TELEMETRY_DATE, QDateTime::currentDateTime().toString(Qt::ISODate));

    accept();
}

void CloudDBAcceptTelemetryDialog::rejectConditions() {

    // document the decision
    appsettings->setValue(GC_ALLOW_TELEMETRY, "userRejected");
    appsettings->setValue(GC_ALLOW_TELEMETRY_DATE, QDateTime::currentDateTime().toString(Qt::ISODate));

    reject();
}











