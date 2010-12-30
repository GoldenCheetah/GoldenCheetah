/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "RealtimeData.h"
#include <QtGui>

#ifndef _GC_RaceDispatcher_h
#define _GC_RaceDispatcher_h 1
#include "GoldenCheetah.h"

class RaceDispatcher : public QWidget
{
    Q_OBJECT
    G_OBJECT


public:
    RaceDispatcher(QWidget *);

signals:
    void telemetryReceived(RealtimeData *);

public slots:
    void dispatch(RealtimeData *);

private:
    QMutex mutex;

};

extern RaceDispatcher *DISPATCHER; // global reference for the dispatcher

#endif
