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

#include "GoogleCalDAVCloud.h"
#include "Athlete.h"
#include "Settings.h"

GoogleCalDAVCloud::GoogleCalDAVCloud
(Context *context)
: CloudService(context), context(context)
{
    settings.insert(OAuthToken, GC_GOOGLECAL_TOKEN);
    settings.insert(Local1, GC_GOOGLECAL_RESOLVEDURL);
    settings.insert(Local2, GC_GOOGLECAL_CALENDARNAME);
    settings.insert(Local3, GC_GOOGLECAL_REFRESH_TOKEN);
    settings.insert(Local4, GC_GOOGLECAL_LAST_REFRESH);
}


GoogleCalDAVCloud::~GoogleCalDAVCloud
()
{
    CloudServiceFactory::instance().saveSettings(this, context);
}


CloudService*
GoogleCalDAVCloud::clone
(Context *context)
{
    return new GoogleCalDAVCloud(context);
}


int
GoogleCalDAVCloud::type
() const
{
    return CloudService::Calendar;
}


int
GoogleCalDAVCloud::capabilities
() const
{
    return OAuth;
}


QString
GoogleCalDAVCloud::id
() const
{
    return "Google Calendar";
}


QString
GoogleCalDAVCloud::uiName
() const
{
    return "Google Calendar";
}


QString
GoogleCalDAVCloud::description
() const
{
    return tr("Sync planned workouts to Google Calendar.");
}


QImage
GoogleCalDAVCloud::logo
() const
{
    return QImage(":images/services/google.png");
}


static bool
addGoogleCalDAVCloud
()
{
    CloudServiceFactory::instance().addService(new GoogleCalDAVCloud(NULL));
    return true;
}


static bool add = addGoogleCalDAVCloud();
