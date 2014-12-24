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

#ifndef _Gc_CalendarDownload_h
#define _Gc_CalendarDownload_h
#include "GoldenCheetah.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>

#include "Context.h"
#include "Settings.h"
/* #include "CalendarParser.h" */

class CalendarDownload : public QObject
{
    Q_OBJECT
    G_OBJECT


public:
    CalendarDownload(Context *context);

    bool download();
public slots:
    void downloadFinished(QNetworkReply *reply);

private:
    Context *context;
    QNetworkAccessManager *nam;
/*    CalendarParser *parser; */
};
#endif
