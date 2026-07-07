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



//
// Calendars are still downloaded and managed using the old code
// See CalDAV.cpp and CalendarDownload.cpp
//
// This is a stub service to use the configuration framework provided
// by the CloudService API to make it seamless for users
//
// As part of v4 planning development we may revisit this and refactor
// this class to support open/read/write etc
//

#ifndef GC_CalDAVCloud_h
#define GC_CalDAVCloud_h

#include "CloudService.h"
#include "CalDAV.h"

class QNetworkReply;
class QNetworkAccessManager;

class CalDAVCloud : public CloudService {

    Q_OBJECT

    public:

        int type() const { return CloudService::Calendar; }
        int capabilities() const { return OAuth; }

        QString id() const {
            switch(variant) {
            case CalDAV::Webcal: return "Web Calendar";
            default:
            case CalDAV::Standard: return "CalDAV Calendar";
            }
        }

        QString uiName() const {
            switch(variant) {
            case CalDAV::Webcal: return "Web Calendar";
            default:
            case CalDAV::Standard: return "CalDAV Calendar";
            }
        }

        QString description() const {
            switch(variant) {
            case CalDAV::Webcal: return tr("Web Calendar using iCal format as a web resource");
            default:
            case CalDAV::Standard: return tr("Generic CalDAV Calendar such as Apple iCloud calendar");
            }
        }

        QImage logo() const;

        // we can be instantiated as a generic CalDAV service or a Webcal
        // CalDAV service. We don't have separate implementations, so at startup
        // we register one of each with the cloud service factory
        CalDAVCloud(Context *context, CalDAV::type variant);
        CloudService *clone(Context *context) { return new CalDAVCloud(context, variant); }
        ~CalDAVCloud();

    private:
        Context *context;
        CalDAV::type variant;

};
#endif
