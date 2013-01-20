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
#import <WFConnector/hardware_connector_types.h>

// Utility
static QString toQString(const NSString *nsstr)
{
    NSRange range;
    range.location = 0;
    range.length = [nsstr length];
    QString result(range.length, QChar(0));
 
    unichar *chars = new unichar[range.length];
    [nsstr getCharacters:chars range:range];
    result = QString::fromUtf16(chars, range.length);
    delete[] chars;
    return result;
}

// Thi source file contains the private objc interface (WFBridge) that
// sits atop the Wahoo Fitness APIs at the top of the source file.
//
// This is then follwoed by the C++ public interface implementation that
// sits atop that private interface (WFBridge)

//----------------------------------------------------------------------
// Objective C -- Private interface / Bridge to WF API classes
//----------------------------------------------------------------------

@interface WFBridge : NSObject <WFHardwareConnectorDelegate> {

@public
    QPointer<WFApi> qtw; // the QT QObject public class

@private
    WFHardwareConnector *sharedConnector;
}

@property (retain) WFHardwareConnector *sharedConnector;
@end

@implementation WFBridge

@synthesize sharedConnector;

//**********************************************************************
// METHODS
//**********************************************************************

// initialise by getting the WF API singleton
-(id)init
{

    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc]init];

    // initialise
    self.sharedConnector = [WFHardwareConnector sharedConnector];
    [self.sharedConnector setDelegate:self];
   
    [pool drain]; 
    return self;
}

//version
-(NSString *) apiVersion { return [self.sharedConnector apiVersion]; }

// State of BTLE support and hardware
-(BOOL)hasBTLESupport { return [self.sharedConnector hasBTLESupport]; }

// By default BTLE is disabled
-(BOOL)isBTLEEnabled { return [self.sharedConnector isBTLEEnabled]; }
-(BOOL)enableBTLE:(BOOL)bEnable inBondingMode:(BOOL)bBondingMode {
    return [self.sharedConnector enableBTLE:bEnable inBondingMode:bBondingMode];
}

// ready to scan
-(BOOL)isCommunicationHWReady { return [self.sharedConnector isCommunicationHWReady]; }

// scan
-(BOOL)discoverDevicesOfType:(WFSensorType_t)eSensorType onNetwork:(WFNetworkType_t)eNetworkType searchTimeout:(NSTimeInterval)timeout
{
    return [self.sharedConnector discoverDevicesOfType:eSensorType onNetwork:eNetworkType searchTimeout:timeout];
}

//**********************************************************************
// EVENTS / SIGNALS
//**********************************************************************

// WFHardwareConnectorDelegate Functions
-(void)hardwareConnector:(WFHardwareConnector*)hwConnector connectedSensor:(WFSensorConnection*)connectionInfo
{
    qtw->connectedSensor(connectionInfo);
}

-(void)hardwareConnector:(WFHardwareConnector*)hwConnector didDiscoverDevices:(NSSet*)connectionParams searchCompleted:(BOOL)bCompleted
{
    qtw->didDiscoverDevices(); //XXX convert array
}

-(void)hardwareConnector:(WFHardwareConnector*)hwConnector disconnectedSensor:(WFSensorConnection*)connectionInfo
{
    qtw->disconnectedSensor(connectionInfo);
}

-(void)hardwareConnector:(WFHardwareConnector*)hwConnector stateChanged:(WFHardwareConnectorState_t)currentState
{
    qtw->stateChanged();
}

-(void)hardwareConnectorHasData
{
    qtw->hasData();
}

-(void) hardwareConnector:(WFHardwareConnector*)hwConnector hasFirmwareUpdateAvailableForConnection:(WFSensorConnection*)connectionInfo required:(BOOL)required withWahooUtilityAppURL:(NSURL *)wahooUtilityAppURL
{
    qtw->hasFirmwareUpdateAvalableForConnection(); //XXX do what?
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
    wf = [[WFBridge alloc] init];
    wf->qtw = this;
}

// Destroy the bridge to the WF API
WFApi::~WFApi()
{
    [wf release];
}

//**********************************************************************
// METHODS
//**********************************************************************

QString WFApi::apiVersion() { return toQString([wf apiVersion]); }
bool WFApi::isBTLEEnabled() { return [wf isBTLEEnabled]; }
bool WFApi::hasBTLESupport() { return [wf hasBTLESupport]; }
bool WFApi::isCommunicationHWReady() { return [wf isCommunicationHWReady]; }
bool WFApi::enableBTLE(bool enable, bool bondingmode) {
    return [wf enableBTLE:enable inBondingMode:bondingmode];
}

bool
WFApi::discoverDevicesOfType(int eSensorType, int eNetworkType, int timeout)
{
    // ignore ehat was passed for now...
    return [wf discoverDevicesOfType:WF_SENSORTYPE_BIKE_POWER onNetwork:WF_NETWORKTYPE_BTLE searchTimeout:15.00];
}

//**********************************************************************
// SLOTS
//**********************************************************************

void
WFApi::connectedSensor(void*)
{
qDebug()<<"connectedSensor";
}

void
WFApi::didDiscoverDevices()
{
qDebug()<<"didDiscoverDevices";
}

void
WFApi::disconnectedSensor(void*)
{
qDebug()<<"disconnectedSensor";
}

void
WFApi::hasData()
{
qDebug()<<"hasData";
}

void
WFApi::hasFirmwareUpdateAvalableForConnection()
{
qDebug()<<"hasFormware...";
}

void
WFApi::stateChanged()
{
qDebug()<<"state changed...";
}
