/*
 * Copyright (c) 2009 Steve Gribble (gribble [at] cs.washington.edu) and
 *                    Mark Liversedge (liversedge@gmail.com)
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

#include <stdio.h>

#include "NullController.h"
#include "RaceDispatcher.h"
#include "RealtimeData.h"

NullController::NullController(RealtimeWindow *parent,
                                                 DeviceConfiguration *)
  : RealtimeController(parent), parent(parent), load(100)
{
}

int NullController::start() {
  return 0;
}

int NullController::stop() {
  return 0;
}

int NullController::pause() {
  return 0;
}

int NullController::restart() {
  return 0;
}

void NullController::getRealtimeData(RealtimeData &rtData) {
    RealtimeData null;
    null.setName((char *)"Null");
    null.setWatts(load);
    rtData = null;
}

void NullController::pushRealtimeData(RealtimeData &) {
}
