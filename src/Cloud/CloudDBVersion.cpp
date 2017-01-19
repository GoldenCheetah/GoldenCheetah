/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#include "CloudDBVersion.h"
#include "CloudDBCommon.h"
#include "Secrets.h"
#include "GcUpgrade.h"

#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QUrlQuery>
#include <QMessageBox>
#include <QEventLoop>
#include <QLabel>


// the Version code must be in sync with CloudDB GAE Version codes
int CloudDBVersionClient::CloudDBVersion_Release = 10;
int CloudDBVersionClient::CloudDBVersion_ReleaseCandidate = 20;
int CloudDBVersionClient::CloudDBVersion_DevelopmentBuild = 30;


CloudDBVersionClient::CloudDBVersionClient() {
    // only static members
}
CloudDBVersionClient::~CloudDBVersionClient() {
    // only static members

}


QList<VersionAPIGetV1>
CloudDBVersionClient::getLatestVersions() {

    // consider any updates done since the last start and consider only newer versions
    int lastVersion = appsettings->value(NULL, GC_LAST_VERSION_CHECKED, 0).toInt();
    if (VERSION_LATEST > lastVersion) {
        appsettings->setValue(GC_LAST_VERSION_CHECKED, VERSION_LATEST);
        lastVersion = VERSION_LATEST;
    }

    QList<VersionAPIGetV1> retrieved;

    QScopedPointer<QNetworkAccessManager> l_nam(new QNetworkAccessManager());
    QNetworkReply *l_reply;

    QNetworkRequest request;
    QUrlQuery query;
    query.addQueryItem("version", QString::number(lastVersion));
    CloudDBCommon::prepareRequest(request, CloudDBCommon::cloudDBBaseURL+"version", &query);
    l_reply = l_nam->get(request);

    // blocking request
    QEventLoop loop;
    connect(l_reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    if (l_reply->error() == QNetworkReply::NoError) {
        QByteArray result = l_reply->readAll();
        unmarshallAPIGetV1(result, &retrieved);
    };   // silently ignore network errors, since the user can't do anything about it anyway

    return retrieved;
}



bool
CloudDBVersionClient::unmarshallAPIGetV1(QByteArray json, QList<VersionAPIGetV1> *versionList) {

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(json, &parseError);


    // all these things should not happen and we have not valid object to return
    if (parseError.error != QJsonParseError::NoError || document.isEmpty() || document.isNull()) {
        return false;
    }

    // we only get single objects here
    if (document.isObject()) {
        VersionAPIGetV1 version;
        QJsonObject object = document.object();
        version.Id = object.value("id").toDouble();
        version.Version = object.value("version").toInt();
        version.VersionText = object.value("versionText").toString();
        version.Type = object.value("releaseType").toInt();
        version.URL = object.value("downloadURL").toString();
        version.Text = object.value("text").toString();
        versionList->append(version);
    } else if (document.isArray()) {
        QJsonArray array(document.array());
        for (int i = 0; i< array.size(); i++) {
            QJsonValue value = array.at(i);
            if (value.isObject()) {
                VersionAPIGetV1 version;
                QJsonObject object = value.toObject();
                version.Id = object.value("id").toDouble();
                version.Version = object.value("version").toInt();
                version.VersionText = object.value("versionText").toString();
                version.Type = object.value("releaseType").toInt();
                version.URL = object.value("downloadURL").toString();
                version.Text = object.value("text").toString();
                versionList->append(version);
            }
        }
    }

    return true;
}


CloudDBUpdateAvailableDialog::CloudDBUpdateAvailableDialog(QList<VersionAPIGetV1> versions) : versions(versions)
{

    setWindowTitle(QString(tr("GoldenCheetah - Check for new versions")));
    setMinimumWidth(750);

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
    header->setText(QString(tr("<b><big>New Version(s) of GoldenCheetah are available</big></b>")));

    QHBoxLayout *toprow = new QHBoxLayout;
    toprow->addWidget(important);
    toprow->addWidget(header);
    layout->addLayout(toprow);

    QLabel *text = new QLabel(this);
    text->setWordWrap(true);
    text->setTextFormat(Qt::RichText);
    text->setOpenExternalLinks(true);

    // build text
    QString messageText;
    VersionAPIGetV1 version;
    foreach (version, versions) {
        if (version.Type == CloudDBVersionClient::CloudDBVersion_Release) {
            messageText.append("<h2>" + tr("Release: %1 ").arg(version.VersionText));
        } else if (version.Type == CloudDBVersionClient::CloudDBVersion_ReleaseCandidate) {
            messageText.append("<h2>" + tr("Release Candidate: %1 ").arg(version.VersionText));
        } else if (version.Type == CloudDBVersionClient::CloudDBVersion_DevelopmentBuild) {
            messageText.append("<h2>" + tr("Development Release: %1 ").arg(version.VersionText));
        };
        messageText.append( QString("</h2><b><a href=\"%1\">%2</a></b><br><br>").arg(version.URL).arg(version.URL) +
                            version.Text + "<br>");
    };

    text->setText(messageText);

    scrollText = new QScrollArea();
    scrollText->setWidget(text);
    scrollText->setWidgetResizable(true);
    layout->addWidget(scrollText);

    QHBoxLayout *lastRow = new QHBoxLayout;

    doNotAskAgainButton = new QPushButton(tr("Do not ask again for these versions"), this);
    doNotAskAgainButton->setEnabled(true);
    connect(doNotAskAgainButton, SIGNAL(clicked()), this, SLOT(doNotAskAgain()));
    askAgainNextStartButton = new QPushButton(tr("Show available versions again on next start"), this);
    askAgainNextStartButton->setDefault(true);
    connect(askAgainNextStartButton, SIGNAL(clicked()), this, SLOT(askAgainOnNextStart()));

    lastRow->addWidget(askAgainNextStartButton);
    lastRow->addStretch();
    lastRow->addWidget(doNotAskAgainButton);
    layout->addLayout(lastRow);

}

void CloudDBUpdateAvailableDialog::doNotAskAgain() {

    // update the version too which update check was done, too not show it again
    if (versions.count()>0) {
        appsettings->setValue(GC_LAST_VERSION_CHECKED, versions.at(0).Version);
    }
    accept();
}

void CloudDBUpdateAvailableDialog::askAgainOnNextStart() {

    // no update to the version to be checked against - same procedure next start
    reject();
}










