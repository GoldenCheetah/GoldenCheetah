/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "CompareDateRange.h"

#include "Context.h"
#include "RideFileCache.h"
#include "Season.h"
#include <QColor>

RideFileCache *
CompareDateRange::rideFileCache()
{
    // refresh cache if incomplete, return otherwise
    if (cache) {
        if (cache->incomplete == false) return cache;
        else {
            delete cache;
            cache = NULL;
        }
    }

    // create one and set        
    return (cache = new RideFileCache(sourceContext, start, end, false, QStringList(), true));
}

CompareDateRange::~CompareDateRange()
{
    if (cache) {
        //XXX need to reference count! delete cache;
        cache = NULL;
    }
}
