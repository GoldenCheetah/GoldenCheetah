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

#ifndef CLOUDDBVERSION_H
#define CLOUDDBVERSION_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QDialog>
#include <QScrollArea>


struct VersionAPIGetV1 {
    qint64  Id;
    int Version;
    QString VersionText;
    int Type;
    QString URL;
    QString Text;
};

enum VersionType {Release=10, ReleaseCandidate=20, DevelopmentBuild=30};



class CloudDBVersionClient : public QObject
{
    Q_OBJECT

public:

    CloudDBVersionClient();
    ~CloudDBVersionClient();

    static int CloudDBVersion_Release;
    static int CloudDBVersion_ReleaseCandidate;
    static int CloudDBVersion_DevelopmentBuild;

    static int CloudDBVersion_Days_Delay;

    void informUserAboutLatestVersions();

private slots:

    void showVersionPopup(QNetworkReply*);

private:

    bool unmarshallAPIGetV1(QByteArray , QList<VersionAPIGetV1> *versionList );



};

class CloudDBUpdateAvailableDialog : public QDialog
{
    Q_OBJECT

public:
    CloudDBUpdateAvailableDialog(QList<VersionAPIGetV1> versions);

private slots:
    void doNotAskAgain();
    void askAgainOnNextStart();

private:

    QString athlete;

    QScrollArea *scrollText;
    QPushButton *doNotAskAgainButton;
    QPushButton *askAgainNextStartButton;

    QList<VersionAPIGetV1> versions;

};



#endif // CLOUDDBVersion_H
