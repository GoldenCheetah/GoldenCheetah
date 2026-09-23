/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2026 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _Gc_CalDAV_h
#define _Gc_CalDAV_h
#include "GoldenCheetah.h"

#include <libical/ical.h>

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslSocket>
#include <QUuid>
#include <QHash>

#include "Context.h"
#include "Athlete.h"
#include "Settings.h"

// rideitem
#include "RideItem.h"
#include "RideFile.h"
#include "JsonRideFile.h"

// SeasonEvent
#include "Season.h"

#include "CloudService.h"
#include "CalDAVAuth.h"

class CalDAV : public QObject
{
    Q_OBJECT

public:

    enum class EntryType : int {
        Season = 0,
        Phase,
        Event,
        PlannedActivity,
        ActualActivity
    };

    struct CalEntry {
        QString title;
        QString description;
        QDateTime start;
        QDateTime end;
        bool allDay = false;
        QString id;
        EntryType type;

        inline static const QString idSuffix = "@goldencheetah.org";

        QString getScopedId() const;
        static bool getIdParts(QString input, EntryType * const entryType, QString * const idPart, QString * const originalId);
    };

    CalDAV(Context *context, CloudService *cloudService);

    bool upload(const CalEntry &calEntry);
    bool remove(const CalEntry &calEntry);
    bool remove(QString id);
    bool list();
    bool isConfigured() const;
    bool uploadBlocking(const CalEntry &calEntry, QString *errorOut = nullptr);
    bool removeBlocking(QString id, QString *errorOut = nullptr);
    bool listBlocking(QStringList *namesOut, QString *errorOut = nullptr);

signals:

    void uploadFinished(QString id, bool success, QString errorString);
    void listFinished(QStringList resourceNames, bool success, QString errorString);

public slots:

    void requestReply(QNetworkReply *reply);
    void sslErrors(QNetworkReply*, QList<QSslError>);

private:

    enum class Op { Put, Delete, List };

    struct RequestContext {
        Op op;
        QString id; // resource id, without prefix/suffix; unused for List
    };

    bool put(QString id, QByteArray vcardtext);

    QString collectionUrl() const;          // CalDAVAuth::collectionUrl(cloudService)
    QString resourceUrl(QString id) const;  // collectionUrl() + id + ".ics"
    void applyAuth(QNetworkRequest &request) const; // CalDAVAuth::applyAuth(cloudService, request)

    Context *context;
    CloudService *cloudService;
    QNetworkAccessManager *nam;

    QHash<QNetworkReply*, RequestContext> inFlight;
};

#endif
