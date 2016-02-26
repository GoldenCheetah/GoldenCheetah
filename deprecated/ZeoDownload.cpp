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

#include "ZeoDownload.h"
#include "MainWindow.h"
#include "Athlete.h"
#include "MetricAggregator.h"
#include <QScriptEngine>

ZeoDownload::ZeoDownload(Context *context) : context(context)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));

    base_api_url = "/zeows/api/v1/sleeperService/";
    api_key = "B8B1881541731200E67B902FC933E87E"; // GC key

    QString _username = appsettings->cvalue(context->athlete->cyclist, GC_ZEOUSER, "").toString();
    QString _password = appsettings->cvalue(context->athlete->cyclist, GC_ZEOPASS, "").toString();

    usernameAndPasswordEncoded = QString(_username + ":" + _password).toUtf8().toBase64();
}

bool
ZeoDownload::download()
{
    newMeasures = 0;
    allMeasures = 0;

    // Build the URL to retrieve dates with sleep data.
    QString url = appsettings->cvalue(context->athlete->cyclist, GC_ZEOURL, "http://app-pro.myzeo.com:8080").toString() + base_api_url + "getAllDatesWithSleepData" + "?key=" + api_key;
    QNetworkRequest request = QNetworkRequest(QUrl(url));
    request.setRawHeader("Authorization", "Basic " + usernameAndPasswordEncoded.toLatin1());
    request.setRawHeader("Referer" , "http://www.goldencheetah.org");
    request.setRawHeader("Accept" , "application/json");

    QNetworkReply *reply = nam->get(request);

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(context->mainWindow, tr("Zeo Data Download"), reply->errorString());
        return false;
    }
    return true;
}

void
ZeoDownload::downloadFinished(QNetworkReply *reply)
{
    if (dates.empty()) {
        // We have to build a list of dates
        QString response = reply->readAll();
        //qDebug() << "response " << response;

        QScriptValue sc;
        QScriptEngine se;

        sc = se.evaluate("("+response+")").property("response").property("dateList").property("date");
        int length = sc.property("length").toInteger();

        dates.clear();
        for (int i=0;i<length;i++) {
            QString day = sc.property(i).property("day").toString();
            QString month = sc.property(i).property("month").toString();
            QString year = sc.property(i).property("year").toString();

            dates.append(QDate(year.toInt(), month.toInt(), day.toInt()));
        }
        allMeasures = dates.count();
    } else {
        // We have to read data for the date
        QString response = reply->readAll();
        //qDebug() << "response2 " << response;

        QScriptValue sc;
        QScriptEngine se;

        sc = se.evaluate("("+response+")").property("response").property("sleepStats");
        QString zq = sc.property("zq").toString();

        int deep = sc.property("timeInDeep").toInteger();
        int light = sc.property("timeInLight").toInteger();
        int rem = sc.property("timeInRem").toInteger();
        int time = deep + light + rem;

        if (dates.first() > QDate().addMonths(-1)) {
            //qDebug() << dates.first() << zq;

            SummaryMetrics add;
            add.setDateTime(QDateTime(dates.first(), QTime(0,0,0)));
            add.setText("Sleep index (ZQ)", QString("%1").arg(zq));
            add.setText("Sleep time", QString("%1").arg(time));
            context->athlete->metricDB->importMeasure(&add);
        }

        dates.removeFirst();
    }

    if (!nextDate()) {
        QString status = QString(tr("%1 new on %2 measurements received.")).arg(newMeasures).arg(allMeasures);
        QMessageBox::information(context->mainWindow, tr("Zeo Data Download"), status);
    }
    return;
}

bool
ZeoDownload::nextDate()
{
    if (!dates.empty()) {
        bool present = false;
        QDate date = dates.first();
        QString dateStr = date.toString("yyyy-MM-dd");

        QDateTime dateTime = QDateTime(date, QTime(0,0,0));
        QList<SummaryMetrics> list = context->athlete->metricDB->getAllMeasuresFor(dateTime,dateTime);
        for (int i=0;i<list.size();i++) {
            SummaryMetrics sm = list.at(i);
            if (sm.getText("Sleep time", "").length()>0 && sm.getText("Sleep index (ZQ)", "").length()>0) {
                present = true;
                dates.removeFirst();
                return nextDate();
            }
        }

        if (!present) {
            newMeasures ++;
            // Build the URL to retrieve the stats for date
            QString url = appsettings->cvalue(context->athlete->cyclist, GC_ZEOURL, "http://app-pro.myzeo.com:8080").toString() + base_api_url + "getSleepStatsForDate" + "?key=" + api_key + "&date=" + dateStr;
            QNetworkRequest request = QNetworkRequest(QUrl(url));
            request.setRawHeader("Authorization", "Basic " + usernameAndPasswordEncoded.toLatin1());
            request.setRawHeader("Referer" , "http://www.goldencheetah.org");
            request.setRawHeader("Accept" , "application/json");

            QNetworkReply *reply = nam->get(request);

            if (reply->error() != QNetworkReply::NoError) {
                QMessageBox::warning(context->mainWindow, tr("Zeo Data Download"), reply->errorString());
            } else
                return true;
        }
    }

    return false;
}
