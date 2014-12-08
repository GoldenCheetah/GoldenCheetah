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

WithingsDownload::WithingsDownload(Context *context) : context(context)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    parser = new WithingsParser;
}

bool
WithingsDownload::download()
{
    QString request = QString("%1/measure?action=getmeas&userid=%2&publickey=%3")
                             .arg(appsettings->cvalue(context->athlete->cyclist, GC_WIURL, "http://wbsapi.withings.net").toString())
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

    // now save data away
    QFile withingsJSON(QString("%1/withings.json").arg(context->athlete->home->cache().canonicalPath()));
    if (withingsJSON.open(QFile::WriteOnly)) {

        QTextStream stream(&withingsJSON);
        stream << text;
        withingsJSON.close();
    }

    // parse it
    parser->parse(text, errors);

    int allMeasures = context->athlete->withings().count();
    int newMeasures = parser->readings().count() - allMeasures;

    QString status = QString(tr("%1 new on %2 measurements received.")).arg(newMeasures).arg(allMeasures);
    QMessageBox::information(context->mainWindow, tr("Withings Data Download"), status);

    // store in athlete
    context->athlete->setWithings(parser->readings());

#if 0
    //main->metricDB->db()->connection().transaction();
    newMeasures = 0;
    allMeasures = parser->readings().count();
    QDateTime olderDate;

    // get list of what we have
    QList<SummaryMetrics> list = context->athlete->metricDB->getAllMeasuresFor(QDateTime(QDate(1900,01,01), QTime(0,0,0)),QDateTime::currentDateTime());

    // start  a transaction
    context->athlete->metricDB->db()->connection().transaction();

    foreach (WithingsReading x, parser->readings()) {

        // do we have it already ?
        bool have = false;
        foreach(SummaryMetrics m, list) {
            if (m.getDateTime().date() == x.when.date()) have = true;
        }

        if (!have) {
            newMeasures ++;
            SummaryMetrics add;
            add.setDateTime(x.when);
            add.setText("Weight", QString("%1").arg(x.weightkg));
            add.setText("Height", QString("%1").arg(x.sizemeter));
            add.setText("Lean Mass", QString("%1").arg(x.leankg));
            add.setText("Fat Mass", QString("%1").arg(x.fatkg));
            add.setText("Fat Ratio", QString("%1").arg(x.fatpercent));

            context->athlete->metricDB->importMeasure(&add);

            if (olderDate.isNull() || x.when.date() <olderDate.date()) olderDate = x.when;
        }
    }

    // commit the transaction
    context->athlete->metricDB->db()->connection().transaction();

    QString status = QString(tr("%1 new on %2 measurements received.")).arg(newMeasures).arg(allMeasures);
    QMessageBox::information(context->mainWindow, tr("Withings Data Download"), status);

    if (newMeasures) {
        context->athlete->isclean = false;
        context->athlete->metricDB->refreshMetrics(olderDate);
    }
#endif
    return;
}

