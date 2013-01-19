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

#include "WFApi.h"

// WF API Headers
#import <WFConnector/WFHardwareConnector.h>

// Thi source file contains the private objc interface (WFBridge) that
// sits atop the Wahoo Fitness APIs at the top of the source file.
//
// This is then follwoed by the C++ public interface implementation that
// sits atop that private interface (WFBridge)

//----------------------------------------------------------------------
// Objective C -- Private interface / Bridge to WF API classes
//----------------------------------------------------------------------

@interface WFBridge : NSObject
{
@public
    QPointer<WFApi> qtw; // the QT QObject public class
@private
    WFHardwareConnector *sharedConnector;
}
@end

@implementation WFBridge
-(void)alloc {

    sharedConnector = [WFHardwareConnector sharedConnector];
}
@end

//----------------------------------------------------------------------
// C++ Public interface
//----------------------------------------------------------------------

// Iniiialise the singleton at startup
static WFApi *_gc_wfapi_init()
{
    return new WFApi;
}
WFApi *_gc_wfapi = _gc_wfapi_init();

// Construct the bridge to the WF API
WFApi::WFApi()
{
    wf = [WFBridge alloc];
    wf->qtw = this;
}

// Destroy the bridge to the WF API
WFApi::~WFApi()
{
    [wf release];
}

