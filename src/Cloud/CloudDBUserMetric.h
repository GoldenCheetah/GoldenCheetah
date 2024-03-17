/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
 * Copyright (c) 2020 Ale Martinez (amtriathlon@gmail.com)
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

#ifndef CLOUDDBUSERMETRIC_H
#define CLOUDDBUSERMETRIC_H

#include "UserMetricSettings.h"
#include "Settings.h"
#include "CloudDBCommon.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>
#include <QSslSocket>
#include <QUuid>
#include <QUrl>
#include <QTableWidget>


// API Structure V1 must be in sync with the Structure used for the V1 on CloudDB
// but uses the correct QT datatypes

struct UserMetricAPIv1 {
    CommonAPIHeaderV1 Header;
    QString UserMetricXML;
    QString CreatorNick;
    QString CreatorEmail;
};


class CloudDBUserMetricClient : public QObject
{
    Q_OBJECT

public:

    CloudDBUserMetricClient();
    ~CloudDBUserMetricClient();

    bool postUserMetric(UserMetricAPIv1 usermetric);
    bool putUserMetric(UserMetricAPIv1 usermetric);
    bool getUserMetricByID(qint64 id, UserMetricAPIv1 *usermetric);
    bool deleteUserMetricByID(qint64 id);
    bool curateUserMetricByID(qint64 id, bool newStatus);
    bool getAllUserMetricHeader(QList<CommonAPIHeaderV1>* header);

    bool sslLibMissing() { return noSSLlib; }

public slots:

    // Catch NAM signals ...
    void sslErrors(QNetworkReply*,QList<QSslError>);

private:
    bool noSSLlib;

    QNetworkAccessManager* g_nam;
    QNetworkReply *g_reply;
    QString g_cacheDir;

    static const int usermetric_magic_string = 1029384756;
    static const int usermetric_cache_version = 3;

    QString  g_usermetric_url_base;
    QString  g_usermetric_url_header;
    QString  g_usermetriccuration_url_base;
    QString  g_usermetricdownloadincr_url_base;

    bool writeUserMetricCache(UserMetricAPIv1 *);
    bool readUserMetricCache(qint64 id, UserMetricAPIv1 *usermetric);
    void deleteUserMetricCache(qint64 id);
    void cleanUserMetricCache(QList<CommonAPIHeaderV1> *objectHeader);

    static bool unmarshallAPIv1(QByteArray , QList<UserMetricAPIv1>* );
    static void unmarshallAPIv1Object(QJsonObject* , UserMetricAPIv1* );

};

struct UserMetricWorkingStructure {
    qint64  id;
    QString name;
    QString description;
    QString creatorNick;
    QString language;
    QDateTime createdAt;
    QString usermetricXML;
    bool createdByMe;
};

class CloudDBUserMetricListDialog : public QDialog
{
    Q_OBJECT

public:

    CloudDBUserMetricListDialog();
    ~CloudDBUserMetricListDialog();

    bool prepareData(QString athlete, CloudDBCommon::UserRole role);
    QList<QString> getSelectedSettings() {return g_selected; }

    // re-implemented
    void closeEvent(QCloseEvent* event);

private slots:

    void addAndCloseClicked();
    void closeClicked();
    void resetToStartClicked();
    void nextSetClicked();
    void prevSetClicked();
    void ownUserMetricsToggled(bool);
    void toggleTextFilterApply();
    void curationStateFilterChanged(int);
    void languageFilterChanged(int);
    void textFilterEditingFinished();
    void cellDoubleClicked(int, int);

    void curateCuratorEdit();
    void deleteCuratorEdit();
    void editCuratorEdit();

    void deleteUserEdit();
    void editUserEdit();

private:

    CloudDBCommon::UserRole g_role;

    // general

    const int const_stepSize;

    CloudDBUserMetricClient* g_client;
    QList<UserMetricWorkingStructure> *g_currentPresets;
    int g_currentIndex;
    QList<CommonAPIHeaderV1>* g_currentHeaderList;
    QList<CommonAPIHeaderV1>* g_fullHeaderList;
    bool g_textFilterActive;
    QString g_currentAthleteId;
    bool g_networkrequestactive;


    // UI elements
    QLabel *showing;
    QString showingTextTemplate;
    QPushButton *resetToStart;
    QPushButton *nextSet;
    QPushButton *prevSet;
    QCheckBox *ownUserMetricsOnly;
    QComboBox *curationStateCombo;
    QComboBox *langCombo;
    QLineEdit *textFilter;
    QPushButton *textFilterApply;

    QTableWidget *tableWidget;
    QHBoxLayout *showingLayout, *filterLayout;
    QHBoxLayout *buttonUserGetLayout, *buttonUserEditLayout, *buttonCuratorEditLayout;
    QVBoxLayout *mainLayout;

    // UserRole - UserGet
    QList<QString> g_selected;
    QPushButton *addAndCloseUserGetButton, *closeUserGetButton;

    // UserRole - UserEdit
    QPushButton *deleteUserEditButton, *editUserEditButton, *closeUserEditButton;

    // UserRole - Curator Edit
    QPushButton *curateCuratorEditButton, *editCuratorEditButton, *deleteCuratorEditButton, *closeCuratorButton;

    // helper methods
    void updateCurrentPresets(int, int);
    void setVisibleButtonsForRole();
    void applyAllFilters();
    bool refreshStaleUserMetricHeader();
    QString encodeHTML ( const QString& );

};

class CloudDBUserMetricObjectDialog : public QDialog
{
    Q_OBJECT

public:

    CloudDBUserMetricObjectDialog(UserMetricAPIv1 data, QString athlete, bool update = false);
    ~CloudDBUserMetricObjectDialog();

    UserMetricAPIv1 getUserMetric() { return data; }

private slots:
    void publishClicked();
    void cancelClicked();

    void nameTextChanged(QString);
    void nameEditingFinished();
    void emailTextChanged(QString);
    void emailEditingFinished();

private:

    UserMetricAPIv1 data;
    QString athlete;
    bool update;

    QPushButton *publishButton, *cancelButton;

    QLineEdit *name;
    QString nameDefault;
    bool nameOk;

    QComboBox *langCombo;

    QTextEdit *description;
    QString descriptionDefault;

    QLineEdit *nickName;

    QLineEdit *email;
    bool emailOk;

    QLabel *gcVersionString;
    QLabel *creatorId;

};


#endif // CLOUDDBUSERMETRIC_H
