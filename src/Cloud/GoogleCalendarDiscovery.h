/*
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

#ifndef _Gc_GoogleCalendarDiscovery_h
#define _Gc_GoogleCalendarDiscovery_h

#include <QString>
#include <QList>

#include "CloudService.h"
#include "CalDAVDiscovery.h"

class GoogleCalendarDiscovery {
public:
    static bool listCalendars(CloudService *cloudService, QList<CalDAVDiscovery::CalendarInfo> *results, QString *errorOut = nullptr);
};

#endif
