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

#ifndef CLOUDDBCHART_H
#define CLOUDDBCHART_H

#include "LTMSettings.h"
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

struct ChartAPIv1 {
    CommonAPIHeaderV1 Header;
    QString ChartSport;
    QString ChartType;
    QString ChartView;
    QString ChartDef;
    QByteArray Image;
    QString CreatorNick;
    QString CreatorEmail;
};


class CloudDBChartClient : public QObject
{
    Q_OBJECT

public:

    CloudDBChartClient();
    ~CloudDBChartClient();

    bool postChart(ChartAPIv1 chart);
    bool putChart(ChartAPIv1 chart);
    bool getChartByID(qint64 id, ChartAPIv1 *chart);
    bool deleteChartByID(qint64 id);
    bool curateChartByID(qint64 id, bool newStatus);
    void incrementDownloadCounterByID(qint64 id);
    bool getAllChartHeader(QList<CommonAPIHeaderV1>* header);
    bool clearAllCache();

    bool sslLibMissing() { return noSSLlib; }

public slots:

    // Catch NAM signals ...
    void sslErrors(QNetworkReply*,QList<QSslError>);

private:
    bool noSSLlib;

    QNetworkAccessManager* g_nam;
    QNetworkReply *g_reply;
    QString g_cacheDir;

    static const int chart_magic_string = 1029384756;
    static const int chart_cache_version = 3;

    QString  g_chart_url_base;
    QString  g_chart_url_header;
    QString  g_chartcuration_url_base;
    QString  g_chartdownloadincr_url_base;

    bool writeChartCache(ChartAPIv1 *);
    bool readChartCache(qint64 id, ChartAPIv1 *chart);
    void deleteChartCache(qint64 id);
    void cleanChartCache(QList<CommonAPIHeaderV1> *objectHeader);

    static bool unmarshallAPIv1(QByteArray , QList<ChartAPIv1>* );
    static void unmarshallAPIv1Object(QJsonObject* , ChartAPIv1* );

};

struct ChartWorkingStructure {
    qint64  id;
    QString name;
    QString description;
    QString creatorNick;
    QString language;
    QDateTime createdAt;
    QPixmap image;
    QString gchartType;
    QString gchartView;
    QString gchartDef;
    QString gchartSport;
    bool createdByMe;
};

class CloudDBChartListDialog : public QDialog
{
    Q_OBJECT

public:

    CloudDBChartListDialog();
    ~CloudDBChartListDialog();

    bool prepareData(QString athlete, CloudDBCommon::UserRole role, int chartView = 0);
    QList<QString> getSelectedSettings() {return g_selected; }

    // re-implemented
    void closeEvent(QCloseEvent* event);

private slots:

    void addAndCloseClicked();
    void closeClicked();
    void resetToStartClicked();
    void nextSetClicked();
    void prevSetClicked();
    void clearCacheClicked();
    void ownChartsToggled(bool);
    void toggleTextFilterApply();
    void curationStateFilterChanged(int);
    void sportComboFilterChanged(int);
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

    CloudDBChartClient* g_client;
    QList<ChartWorkingStructure> *g_currentPresets;
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
    QPushButton *clearCacheButton;
    QCheckBox *ownChartsOnly;
    QComboBox *curationStateCombo;
    QComboBox *langCombo;
    QComboBox *sportCombo;
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
    int g_chartView;

    // UserRole - Curator Edit
    QPushButton *curateCuratorEditButton, *editCuratorEditButton, *deleteCuratorEditButton, *closeCuratorButton;

    // helper methods
    void updateCurrentPresets(int, int);
    void setVisibleButtonsForRole();
    void applyAllFilters();
    bool refreshStaleChartHeader();

};

class CloudDBChartShowPictureDialog : public QDialog
{
    Q_OBJECT

public:

    CloudDBChartShowPictureDialog(QByteArray imageData);
    ~CloudDBChartShowPictureDialog();

public slots:
    void resizeEvent(QResizeEvent *);

private slots:
    void okClicked();


private:

    QByteArray imageData;
    QPixmap chartImage;
    QLabel *imageLabel;
    QPushButton *okButton;

};


class CloudDBChartObjectDialog : public QDialog
{
    Q_OBJECT

public:

    CloudDBChartObjectDialog(ChartAPIv1 data, QString athlete, bool update = false);
    ~CloudDBChartObjectDialog();

    ChartAPIv1 getChart() { return data; }

private slots:
    void publishClicked();
    void cancelClicked();

    void nameTextChanged(QString);
    void nameEditingFinished();
    void emailTextChanged(QString);
    void emailEditingFinished();

private:

    ChartAPIv1 data;
    QString athlete;
    bool update;

    QPushButton *publishButton, *cancelButton;

    QLineEdit *name;
    QString nameDefault;
    bool nameOk;

    QComboBox *langCombo;
    QComboBox *sportCombo;

    QLabel *image;

    QTextEdit *description;
    QString descriptionDefault;

    QLineEdit *nickName;

    QLineEdit *email;
    bool emailOk;

    //QComboBox *language;
    QLabel *gcVersionString;
    QLabel *creatorId;


};


#endif // CLOUDDBCHART_H
