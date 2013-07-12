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
    parser->parse(text, errors);

    //main->metricDB->db()->connection().transaction();
    newMeasures = 0;
    allMeasures = parser->readings().count();
    QDateTime olderDate;


    foreach (WithingsReading x, parser->readings()) {
        QList<SummaryMetrics> list = context->athlete->metricDB->getAllMeasuresFor(x.when,x.when);
        bool presentOrEmpty = false;
        for (int i=0;i<list.size();i++) {
            SummaryMetrics sm = list.at(i);
            if (((x.weightkg == 0) || (sm.getText("Weight", "").length()>0))  &&
                ((x.sizemeter == 0) || (sm.getText("Height", "").length()>0))   &&
                ((x.leankg>0) || (sm.getText("Lean Mass", "").length()>0))  &&
                ((x.fatkg>0) || (sm.getText("Fat Mass", "").length()>0))  &&
                ((x.fatpercent>0) || (sm.getText("Fat Ratio", "").length()>0))) {
                presentOrEmpty = true;
            }
        }

        if (!presentOrEmpty) {
            newMeasures ++;
            SummaryMetrics add;
            add.setDateTime(x.when);
            add.setText("Weight", QString("%1").arg(x.weightkg));
            add.setText("Height", QString("%1").arg(x.sizemeter));
            add.setText("Lean Mass", QString("%1").arg(x.leankg));
            add.setText("Fat Mass", QString("%1").arg(x.fatkg));
            add.setText("Fat Ratio", QString("%1").arg(x.fatpercent));

            context->athlete->metricDB->importMeasure(&add);

            if (olderDate.isNull() || x.when<olderDate)
                olderDate = x.when;
        }
    }

    QString status = QString(tr("%1 new on %2 measurements received.")).arg(newMeasures).arg(allMeasures);
    QMessageBox::information(context->mainWindow, tr("Withings Data Download"), status);

    if (!olderDate.isNull()) {
        context->athlete->isclean = false;
        context->athlete->metricDB->refreshMetrics(olderDate);
    }
    return;
}

