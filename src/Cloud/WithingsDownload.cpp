/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "WithingsDownload.h"
#include "MainWindow.h"
#include "Athlete.h"
#include "RideCache.h"
#include <QMessageBox>

WithingsDownload::WithingsDownload(Context *context) : context(context)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    parser = new WithingsParser;
}

bool
WithingsDownload::download()
{
    // account for trailing slash, remove it if it is there (it was the default in preferences)
    QString server = appsettings->cvalue(context->athlete->cyclist, GC_WIURL, "http://wbsapi.withings.net").toString();
    if (server.endsWith("/")) server=server.mid(0, server.length()-1);

    QString request = QString("%1/measure?action=getmeas&userid=%2&publickey=%3")
                             .arg(server)
                             .arg(appsettings->cvalue(context->athlete->cyclist, GC_WIUSER, "").toString())
                             .arg(appsettings->cvalue(context->athlete->cyclist, GC_WIKEY, "").toString());


    QNetworkReply *reply = nam->get(QNetworkRequest(QUrl(request)));

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(context->mainWindow, tr("Withings Data Download"), reply->errorString());
        return false;
    }
    return true;
}

void
WithingsDownload::downloadFinished(QNetworkReply *reply)
{
    QString text = reply->readAll();
    QStringList errors;

    // parse it
    parser->parse(text, errors);

    int allMeasures = context->athlete->withings().count();
    int receivedMeasures = parser->readings().count();
    int newMeasures = receivedMeasures - allMeasures;

    if (receivedMeasures == 0) {
        newMeasures = 0;
    }

    QString status = tr("No new measurements");
    if (newMeasures > 0) status = QString(tr("%1 new measurements received.")).arg(newMeasures);

    QMessageBox::information(context->mainWindow, tr("Withings Data Download"), status);

    // hacky for now, just refresh for all dates where we have withings data
    // will go with SQL shortly.
    if (newMeasures) {

        // if refresh is running cancel it !
        context->athlete->rideCache->cancel();

        // store in athlete
        context->athlete->setWithings(parser->readings());

        // now save data away if we actually got something !
        // doing it here means we don't overwrite previous responses
        // when we fail to get any data (e.g. errors / network problems)
        QFile withingsJSON(QString("%1/withings.json").arg(context->athlete->home->cache().canonicalPath()));
        if (withingsJSON.open(QFile::WriteOnly)) {

            QTextStream stream(&withingsJSON);
            stream << text;
            withingsJSON.close();
        }

        // do a refresh, it will check if needed
        context->athlete->rideCache->refresh();
    }
    return;
}

