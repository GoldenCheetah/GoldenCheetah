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

WithingsDownload::WithingsDownload(MainWindow *main) : main(main)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    parser = new WithingsParser;
}

bool
WithingsDownload::download()
{
    QString request = QString("%1/measure?action=getmeas&userid=%2&publickey=%3")
                             .arg(appsettings->cvalue(main->cyclist, GC_WIURL, "http://wbsapi.withings.net").toString())
                             .arg(appsettings->cvalue(main->cyclist, GC_WIUSER, "").toString())
                             .arg(appsettings->cvalue(main->cyclist, GC_WIKEY, "").toString());


    QNetworkReply *reply = nam->get(QNetworkRequest(QUrl(request)));

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(main, tr("Withings Data Download"), reply->errorString());
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
    QString status = QString(tr("%1 measurements received.")).arg(parser->readings().count());
    QMessageBox::information(main, tr("Withings Data Download"), status);

    //main->metricDB->db()->connection().transaction();

    // debug output to examine data downloaded
    foreach (WithingsReading x, parser->readings()) {
        SummaryMetrics add;
        add.setDateTime(x.when);
        add.setText("Weight", QString("%1").arg(x.weightkg));
        add.setText("Height", QString("%1").arg(x.sizemeter));
        add.setText("Lean Mass", QString("%1").arg(x.leankg));
        add.setText("Fat Mass", QString("%1").arg(x.fatkg));
        add.setText("Fat Ratio", QString("%1").arg(x.fatpercent));

        main->metricDB->importMeasure(&add);
    }
    //main->metricDB->db()->connection().commit();
    return;
}

