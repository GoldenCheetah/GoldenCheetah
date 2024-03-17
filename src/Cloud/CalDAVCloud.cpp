/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2013 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "CalDAVCloud.h"
#include "Athlete.h"
#include "Settings.h"
#include "mvjson.h"
#include <QByteArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>


#ifndef CALDAV_DEBUG
#define CALDAV_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
    if (CALDAV_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
    }                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
    do {                                                                \
        if (CALDAV_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
        }                                                               \
    } while(0)
#endif

CalDAVCloud::CalDAVCloud(Context *context, CalDAV::type variant) : CloudService(context), context(context), variant(variant) {

    // config
    if (variant == CalDAV::Webcal) {
        settings.insert(URL, GC_WEBCAL_URL);
    } else {
        settings.insert(URL, GC_DVURL);
        settings.insert(Username, GC_DVUSER);
        settings.insert(Password, GC_DVPASS);
    }
}

CalDAVCloud::~CalDAVCloud() {
    // We need to save the variant for compatibility with older code
    // until CalDAV code is fully integrated in the Cloud Services framework
    settings.insert(Local1, GC_DVCALDAVTYPE);
    setSetting(GC_DVCALDAVTYPE, variant);
    CloudServiceFactory::instance().saveSettings(this, context);
}

QImage CalDAVCloud::logo() const
{
    return QImage(":images/services/google.png");
}

static bool addCalDAVCloud() {
    CloudServiceFactory::instance().addService(new CalDAVCloud(NULL, CalDAV::Standard));
    CloudServiceFactory::instance().addService(new CalDAVCloud(NULL, CalDAV::Webcal));
    return true;
}

static bool add = addCalDAVCloud();

