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

#ifndef CHARTEXCHANGE_H
#define CHARTEXCHANGE_H

#include "LTMSettings.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QSslSocket>
#include <QUuid>
#include <QUrl>
#include <QTableWidget>


// API Structure V1 (flattened) must be in sync with the Structure used for the V1
// but uses the correct QT datatypes

struct ChartAPIv1 {
    quint64 Id;
    QString Name;
    QString Description;
    QString Language;
    QString GcVersion;
    QString ChartXML;
    int     ChartVersion;
    QByteArray Image;
    int     StatusId;
    QDateTime LastChanged;
    QString CreatorId;
    QString CreatorNick;
    QString CreatorEmail;
    bool    Curated;
};

enum ChartStatus {
    ChartCreated = 0,
    ChartUpdated = 1,
    ChartDeleted = 2,
    ChartCurated = 3
};

class ChartExchangeClient : public QObject
{
    Q_OBJECT

public:

    ChartExchangeClient();
    ~ChartExchangeClient();

    bool postChart(ChartAPIv1 chart);
    ChartAPIv1 getChartByID(qint64 id);
    QList<ChartAPIv1> *getChartsByDate(QDateTime changedAfter);

    bool sslLibMissing() { return noSSLlib; }

public slots:

    // Catch NAM signals ...
    void sslErrors(QNetworkReply*,QList<QSslError>);

private:
    bool noSSLlib;

    QNetworkAccessManager* g_nam;
    QNetworkReply *g_reply;

    QString  g_chart_url_base;
    QVariant g_header_content_type;
    QByteArray g_header_basic_auth;

    static bool unmarshallAPIv1(QByteArray json, QList<ChartAPIv1>* charts);
    static void unmarshallAPIv1Object(QJsonObject* object, ChartAPIv1* chart);


};

struct ChartExchangePresets {
    QString name;
    QString description;
    QPixmap image;
    LTMSettings ltmSettings;
};

class ChartExchangeRetrieveDialog : public QDialog
{
    Q_OBJECT

public:

    ChartExchangeRetrieveDialog();
    ~ChartExchangeRetrieveDialog();

    LTMSettings getSelectedSettings() {return selected; }

private slots:

    void selectedClicked();
    void cancelClicked();


private:

    LTMSettings selected;

    ChartExchangeClient* client;
    QList<ChartExchangePresets> *allPresets;

    QTableWidget *tableWidget;
    QPushButton *selectButton, *cancelButton;

    // helper methods
    QList<ChartExchangePresets> *getAllSharedPresets(); //just for testing the logic - not the final method
    void getData();


};


class ChartExchangePublishDialog : public QDialog
{
    Q_OBJECT

public:

    ChartExchangePublishDialog(ChartAPIv1 data);
    ~ChartExchangePublishDialog();

    ChartAPIv1 getData() { return data; }

private slots:
    void publishClicked();
    void cancelClicked();


private:

    ChartAPIv1 data;

    QPushButton *publishButton, *cancelButton;
    QLineEdit *name;
    QLabel *image;
    QTextEdit *description;
    QLineEdit *nickName;
    QLineEdit *email;
    //QComboBox *language;
    QLabel *gcVersionString;
    QLabel *creatorId;

};


#endif // CHARTEXCHANGE_H
