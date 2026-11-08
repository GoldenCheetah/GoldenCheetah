/*
 * Copyright (c) 2026 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "Garmin.h"
#include "Athlete.h"
#include "Secrets.h"
#include "Settings.h"
#include "GarminPersonalAPI.h"
#include <QByteArray>

#ifndef GARMIN_DEBUG
#define GARMIN_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (GARMIN_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (GARMIN_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

Garmin::Garmin(Context *context) : CloudService(context), context(context), root_(nullptr)
{
    downloadCompression = zip;
    filetype = uploadType::FIT;
    useMetric = true; // distance and duration metadata

    // config
    settings.insert(Key, GC_GARMIN_API_KEY);
    settings.insert(Local1, GC_GARMIN_REFRESH_TOKEN);
    settings.insert(Local2, GC_GARMIN_LAST_REFRESH);
}

Garmin::~Garmin() = default;

bool
Garmin::open(QStringList &errors)
{
    printd("Garmin::open\n");

    // do we have a key
    QString key = getSetting(GC_GARMIN_API_KEY, "").toString();
    if (key == "") {
        errors << tr("You must set a Garmin Personal API key first");
        return false;
    }
    printd("Garmin API key %s\n", key.toStdString().c_str());

    printd("Get access token for this session.\n");
    garminPersonalAPI = std::make_unique<GarminPersonalAPI>(key);

    // update our settings
    //if (access_token != "") setSetting(GC_GARMIN_TOKEN, access_token);
    //if (refresh_token != "") setSetting(GC_GARMIN_REFRESH_TOKEN, refresh_token);
    setSetting(GC_GARMIN_LAST_REFRESH, QDateTime::currentDateTime());

    // get the factory to save our settings permanently
    CloudServiceFactory::instance().saveSettings(this, context);
    return true;
}

bool
Garmin::close()
{
    printd("Garmin::close\n");
    // nothing to do for now
    return true;
}

QList<CloudServiceEntry*>
Garmin::readdir(QString path, QStringList &errors, QDateTime from, QDateTime to)
{
    printd("Garmin::readdir(%s)\n", path.toStdString().c_str());

    QList<CloudServiceEntry*> returning;

    if (!garminPersonalAPI) {
        errors << tr("Garmin service not initialized");
        return returning;
    }

    const int page = 0;

    GarminPersonalAPI::ActivityListing listing = garminPersonalAPI->activityListService(from.date(), to.date(), page);

    printd("Failure   : %i", static_cast<int>(listing.failure));
    printd("Transport : %i (%i)",listing.transportError, listing.httpCode);
    printd("Error Text: %s", listing.errorText.toStdString().c_str());
    printd("Entries : (%i)\n", listing.activities.count());

    if (listing.hasError()) {
        errors << listing.errorText;
        return returning;
    }

    for (const GarminPersonalAPI::ActivityListingEntry &entry : listing.activities) {
        CloudServiceEntry *add = newCloudServiceEntry();
        add->id = entry.activityId;
        add->name = entry.startTime.toLocalTime().toString("yyyy_MM_dd_HH_mm_ss")+".fit";
        add->label = entry.name;
        add->distance = entry.distance;
        add->duration = entry.duration;

        returning << add;
    }

    // all good ?
    printd("returning count(%lld), errors(%s)\n", returning.count(), errors.join(",").toStdString().c_str());
    return returning;
}

// read a file at location (relative to home) into passed array
bool
Garmin::readFile(QByteArray *data, QString remotename, QString remoteid)
{
    printd("Garmin::readFile(%s, %s)\n", remotename.toStdString().c_str(), remoteid.toStdString().c_str());

    if (!garminPersonalAPI || !data) {
        return false;
    }

    GarminPersonalAPI::ActivityDownload download = garminPersonalAPI->downloadService(remoteid);
    qDebug() << "=== DOWNLOAD" << download.activityId;
    qDebug() << "Failure   :" << static_cast<int>(download.failure);
    qDebug() << "Transport :" << download.transportError << "(" << download.httpCode << ")";
    qDebug() << "Error Text:" << download.errorText;
    qDebug() << "data      :" << download.data.size();

    if (download.hasError()) {
        qWarning() << "Garmin download failed:"
                   << download.errorText;
        return false;
    }

    *data = download.data;

    notifyReadComplete(
        data,
        remotename+".zip",
        tr("Completed."));

    return true;
}

static bool addGarmin() {
    CloudServiceFactory::instance().addService(new Garmin(nullptr));
    return true;
}

static bool add = addGarmin();
