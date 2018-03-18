/*
 * Copyright (c) 2018 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_OpenData_h
#define GC_OpenData_h

#include "Athlete.h"
#include "Context.h"
#include "RideCache.h"

#include <QThread>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// we maintain a list of receiving servers here:
#define OPENDATA_SERVERSURL "http://www.goldencheetah.org/opendata.json"

class OpenData : public QThread {

    Q_OBJECT

    public:

        OpenData(Context *context);
        virtual ~OpenData();

        void postData() { start(); }

        // run in a thread
        void run();

    signals:

        // Progress indicator with end user message
        // n > 0 processing a step
        // x == 0 error at step n
        // n == 0 finished
        void progress(int n, int x, QString message);

    protected:

        Context *context;

    private slots:
        void onSslErrors(QNetworkReply *reply, const QList<QSslError>&error);
};

#endif
