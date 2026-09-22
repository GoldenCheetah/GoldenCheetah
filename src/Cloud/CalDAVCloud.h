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


#ifndef GC_CalDAVCloud_h
#define GC_CalDAVCloud_h

#include "CloudService.h"

class QNetworkReply;
class QNetworkAccessManager;

class CalDAVCloud : public CloudService {

    Q_OBJECT

    public:

        int type() const { return CloudService::Calendar; }

        int capabilities() const { return UserPass | Upload | Query; }

        QString id() const { return "CalDAV Calendar"; }
        QString uiName() const { return "CalDAV Calendar"; }
        QString description() const { return tr("Generic CalDAV Calendar such as Nextcloud or Apple iCloud"); }

        QImage logo() const;

        CalDAVCloud(Context *context);
        CloudService *clone(Context *context) { return new CalDAVCloud(context); }
        ~CalDAVCloud();

    private:
        Context *context;

};
#endif
