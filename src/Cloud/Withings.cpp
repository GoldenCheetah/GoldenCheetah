/*
 * Copyright (c) 2016 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "Withings.h"
#include "Athlete.h"
#include "Settings.h"

// Stub only, for configuration
Withings::Withings(Context *context) : CloudService(context), context(context) {

    // config
    settings.insert(OAuthToken, GC_NOKIA_TOKEN);
    settings.insert(Local1, GC_NOKIA_REFRESH_TOKEN);
}

static bool addWithings() {
    CloudServiceFactory::instance().addService(new Withings(NULL));
    return true;
}

static bool add = addWithings();
