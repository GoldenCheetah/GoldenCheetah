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

#include "CalendarDownload.h"
#include "MainWindow.h"
#include "Athlete.h"
#ifdef GC_HAVE_ICAL
#include "ICalendar.h"
#include <libical/ical.h>
#endif

CalendarDownload::CalendarDownload(Context *context) : context(context)
{
    nam = new QNetworkAccessManager(this);
    connect(nam, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
}

bool
CalendarDownload::download()
{
    QString request = appsettings->cvalue(context->athlete->cyclist, GC_WEBCAL_URL, "").toString();
    if (request == "") return false;
    else {
        // change webcal to http, since it is basically the same port
        QRegularExpression webcal("^webcal:");
        request.replace(webcal, QString("http:"));
    }

    QNetworkReply *reply = nam->get(QNetworkRequest(QUrl(request)));

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(context->mainWidget(), tr("Calendar Data Download"), reply->errorString());
        return false;
    }
    return true;
}

void
CalendarDownload::downloadFinished(QNetworkReply *reply)
{
    QString fulltext = reply->readAll();
    QStringList errors;

#ifdef GC_HAVE_ICAL
    QString remoteCache = context->athlete->home->calendar().absolutePath()+"/remote.ics";
    QFile remoteCacheFile(remoteCache);

    if (fulltext != "") {

        // update remote cache - write to it!
        if (!remoteCacheFile.open(QFile::ReadWrite | QFile::Text)) {
            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("Problem Saving Calendar Download"));
            msgBox.setInformativeText(tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(remoteCache));
            msgBox.exec();
            return;
        }
        QTextStream out(&remoteCacheFile);
        out << fulltext;
        remoteCacheFile.close();

    } else {

        if (remoteCacheFile.exists()) {
            QMessageBox msgBox;
            msgBox.setText(tr("Remote Calendar not available, reverting to cached workouts."));
            msgBox.setIcon(QMessageBox::Information);
            msgBox.exec();

            // read cache
            // read in the whole thing
            remoteCacheFile.open(QFile::ReadOnly | QFile::Text);
            QTextStream in(&remoteCacheFile);
            fulltext = in.readAll();
            remoteCacheFile.close();
        }
    }

    if (fulltext != "") context->athlete->rideCalendar->refreshRemote(fulltext);
#endif
}
