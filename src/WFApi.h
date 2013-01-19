/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRWFApiY; without even the implied warranty of MERCHWFApiABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef gc_WFApi_h
#define gc_WFApi_h

// GC 
#include "GoldenCheetah.h"
#include "Settings.h"
#include "RealtimeData.h"
#include "DeviceConfiguration.h"

// QT
#include <QPointer>
#include <QObject>

// global singleton
class WFApi;
extern WFApi *_gc_wfapi;

// declare the provate implementation for objc
#ifdef __OBJC__
@class WFBridge;
#else
// irritating dependencies
#endif

class WFApi : public QObject // QOBject for signals
{

   Q_OBJECT


public:
    WFApi(); // single instance
    ~WFApi();

    static WFApi *getInstance() { return _gc_wfapi; } // singleton

signals:

public slots:

private:

    // the native api bridge -- private implementation in
    // WFApi.mm -- bridge between the QT/C++ world and the
    // WF/Objc world
#ifdef __OBJC__
    WFBridge *wf; // when included in objc sources
#else /* __OBJC__ */
    void *wb;       // when included in C++ sources
#endif /* __OBJC__ */

};

#endif
