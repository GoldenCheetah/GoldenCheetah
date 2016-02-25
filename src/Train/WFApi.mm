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
#import <WFConnector/WFConnectionParams.h>
#import <WFConnector/WFDeviceParams.h>
#import <WFConnector/WFSensorConnection.h>
#import <WFConnector/WFSensorData.h>
#import <WFConnector/WFBikePowerData.h>
#import <WFConnector/WFBikePowerConnection.h>
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

static inline NSString* fromQString(const QString &string)
{
    const QByteArray utf8 = string.toUtf8();
    const char* cString = utf8.constData();
    return [[NSString alloc] initWithUTF8String:cString];
}

// Thi source file contains the private objc interface (WFBridge) that
// sits atop the Wahoo Fitness APIs at the top of the source file.
//
// This is then follwoed by the C++ public interface implementation that
// sits atop that private interface (WFBridge)

//----------------------------------------------------------------------
// Objective C -- Private interface
//----------------------------------------------------------------------

@interface WFBridge : NSObject <WFHardwareConnectorDelegate, WFSensorConnectionDelegate> {
@public
    QPointer<WFApi> qtw; // the QT QObject public class
    NSMutableArray *discoveredSensors;
}

@end

@implementation WFBridge

//============================================================================
// Hardware Connector Methods
//============================================================================

// retreive the API version
-(NSString *) apiVersion { return [[WFHardwareConnector sharedConnector] apiVersion]; }

// BTLE state and enablement
-(BOOL)hasBTLESupport { return [[WFHardwareConnector sharedConnector] hasBTLESupport]; }
-(BOOL)isBTLEEnabled { return [[WFHardwareConnector sharedConnector] isBTLEEnabled]; }
-(BOOL)enableBTLE:(BOOL)bEnable inBondingMode:(BOOL)bBondingMode {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc]init];
    bool result = [[WFHardwareConnector sharedConnector] enableBTLE:bEnable inBondingMode:bBondingMode];
    [pool drain];
    return result;
}
-(BOOL)isCommunicationHWReady { return [[WFHardwareConnector sharedConnector] isCommunicationHWReady]; }
-(int)currentState { return [[WFHardwareConnector sharedConnector] currentState]; }

// Initialise the WFBridge singleton
-(id)init
{
    // initialise
    discoveredSensors = [[NSMutableArray alloc] init];
    [[WFHardwareConnector sharedConnector] setDelegate:self];
    [self enableBTLE:TRUE inBondingMode:false];
    return self;
}

// scan for devices and stored details
-(BOOL)discoverDevicesOfType:(WFSensorType_t)eSensorType onNetwork:(WFNetworkType_t)eNetworkType searchTimeout:(NSTimeInterval)timeout
{
    // get rid of past scans / stop any in progress
    [discoveredSensors removeAllObjects];
    [[WFHardwareConnector sharedConnector] cancelDiscoveryOnNetwork:WF_NETWORKTYPE_BTLE];

    // go looking
    [[WFHardwareConnector sharedConnector] discoverDevicesOfType:eSensorType onNetwork:eNetworkType searchTimeout:timeout]; //XXX ignoringreturn
    return true;
}
-(int)deviceCount { return [discoveredSensors count]; }
-(NSString*)deviceUUID:(int)n
{
    WFConnectionParams* connParams = (WFConnectionParams*)[discoveredSensors objectAtIndex:n];
    return connParams.device1.deviceUUIDString;
}
-(int)deviceType:(int)n
{
    WFConnectionParams* connParams = (WFConnectionParams*)[discoveredSensors objectAtIndex:n];
    return (int)connParams.sensorType;
}
-(int)deviceSubType:(int)n
{
    WFConnectionParams* connParams = (WFConnectionParams*)[discoveredSensors objectAtIndex:n];
    return (int)connParams.sensorSubType;
}

//============================================================================
// Sensor Connection Methods
//============================================================================

// connect and disconnect a device
-(WFSensorConnection *)connectDevice: (NSString *)uuid
{
    // just in case there is a discovery in action, lets cancel it...
    [[WFHardwareConnector sharedConnector] cancelDiscoveryOnNetwork:WF_NETWORKTYPE_BTLE];

    WFDeviceParams* dev = [[WFDeviceParams alloc] init];
    dev.deviceUUIDString = uuid;
    dev.networkType = WF_NETWORKTYPE_BTLE;

    WFConnectionParams* params = [[WFConnectionParams alloc] init];
    params.sensorType = WF_SENSORTYPE_BIKE_POWER;
    params.networkType = WF_NETWORKTYPE_BTLE;
    params.sensorSubType = WF_SENSOR_SUBTYPE_BIKE_POWER_KICKR;
    params.searchTimeout = 5;
    params.device1 = dev;

    // request the sensor connection.
    WFSensorConnection *sensorConnection = (WFBikePowerConnection*)[[WFHardwareConnector sharedConnector] requestSensorConnection:params];

    // set delegate to receive connection status changes.
    sensorConnection.delegate = self;


    return sensorConnection;
}
-(int)connectionStatus:(WFSensorConnection*)sensorConnection { return (int)[sensorConnection connectionStatus]; }
- (BOOL)disconnectDevice:(WFSensorConnection*)sensorConnection { [sensorConnection disconnect]; return true; }
- (BOOL)isConnected:(WFSensorConnection*)sensorConnection { return [sensorConnection isConnected]; }

// get telemetry
- (WFBikePowerData*) getData:(WFSensorConnection*)sensorConnection { return (WFBikePowerData*)[sensorConnection getData]; }

// trainer setup / load
- (void) setSlopeMode:(WFBikePowerConnection*)sensorConnection { [sensorConnection trainerSetSimMode:85 rollingResistance:0.0004 windResistance:0.6]; }
- (void) setErgoMode:(WFBikePowerConnection*)sensorConnection { [sensorConnection trainerSetErgMode:100]; }
- (void) setSlope:(WFBikePowerConnection*)sensorConnection slope:(double)slope { [sensorConnection trainerSetGrade:slope]; }
- (void) setLoad:(WFBikePowerConnection*)sensorConnection load:(int)load { [sensorConnection trainerSetErgMode:load]; }

//============================================================================
// Sensor connection updates (delegate methods)
//============================================================================

// state changed
- (void)connection:(WFSensorConnection*)connectionInfo stateChanged:(WFSensorConnectionStatus_t)connState
{   Q_UNUSED(connectionInfo);
    qtw->connectionState(connState);
}
// timed out
- (void)connectionDidTimeout:(WFSensorConnection*)connectionInfo
{   Q_UNUSED(connectionInfo);
    qtw->connectionTimeout();
}
// telemetry available
- (BOOL) hasData:(WFSensorConnection*)sensorConnection { return [sensorConnection hasData]; }

// firmware available for this sensor
-(void) hardwareConnector:(WFHardwareConnector*)hwConnector hasFirmwareUpdateAvailableForConnection:(WFSensorConnection*)connectionInfo required:(BOOL)required withWahooUtilityAppURL:(NSURL *)wahooUtilityAppURL
{   Q_UNUSED(hwConnector);
    Q_UNUSED(connectionInfo);
    Q_UNUSED(required);
    Q_UNUSED(wahooUtilityAppURL);
    qtw->hasFirmwareUpdateAvalableForConnection(); //XXX do what?
}

//============================================================================
// Hardware connector updates (delegate methods)
//============================================================================

// state changed on connector
-(void)hardwareConnector:(WFHardwareConnector*)hwConnector stateChanged:(WFHardwareConnectorState_t)currentState
{   Q_UNUSED(hwConnector);
    Q_UNUSED(currentState);
    qtw->stateChanged();
}

// connection established
-(void)hardwareConnector:(WFHardwareConnector*)hwConnector connectedSensor:(WFSensorConnection*)connectionInfo
{   Q_UNUSED(hwConnector);
    qtw->connectedSensor(connectionInfo);
}

// data has arrived on connector
-(void)hardwareConnectorHasData { qtw->connectorHasData(); }

// a sensor was disconnected
-(void)hardwareConnector:(WFHardwareConnector*)hwConnector disconnectedSensor:(WFSensorConnection*)connectionInfo
{   Q_UNUSED(hwConnector);
    qtw->disconnectedSensor(connectionInfo);
}

// devices discovered
-(void)hardwareConnector:(WFHardwareConnector*)hwConnector didDiscoverDevices:(NSSet*)connectionParams searchCompleted:(BOOL)bCompleted
{   Q_UNUSED(hwConnector);

    if (!bCompleted) {
        // add discovered devices -- as they are discovered, not at the end.
        for (WFConnectionParams* connParams in connectionParams) {
            [discoveredSensors addObject:connParams];
        }   
    }

    qtw->didDiscoverDevices([discoveredSensors count], bCompleted);
}

-(NSAutoreleasePool*) getPool { return [[NSAutoreleasePool alloc] init]; }
-(void) freePool:(NSAutoreleasePool*)pool { [pool release]; }
@end

//----------------------------------------------------------------------
// C++ PUBLIC Interface
//----------------------------------------------------------------------

// Singleton API class
WFApi *_gc_wfapi = NULL;
WFApi::WFApi()
{
    wf = [[WFBridge alloc] init];
    wf->qtw = this;
}
WFApi::~WFApi() { [wf release]; }

//============================================================================
// wrappers for methods in private implementation above
//============================================================================
QString WFApi::apiVersion() { return toQString([wf apiVersion]); }
bool WFApi::isBTLEEnabled() { return [wf isBTLEEnabled]; }
bool WFApi::hasBTLESupport() { return [wf hasBTLESupport]; }
bool WFApi::isCommunicationHWReady() { return [wf isCommunicationHWReady]; }
bool WFApi::enableBTLE(bool enable, bool bondingmode) {
    return [wf enableBTLE:enable inBondingMode:bondingmode];
}
int WFApi::currentState() { return [wf currentState]; }

bool
WFApi::discoverDevicesOfType(int eSensorType)
{
    // ignore ehat was passed for now...
    return [wf discoverDevicesOfType:(WFSensorType_t)eSensorType onNetwork:WF_NETWORKTYPE_BTLE searchTimeout:15.00];
}

QString WFApi::deviceUUID(int n)
{
    if (n>=0 && n<deviceCount()) return toQString([wf deviceUUID:n]);
    else return "";
}
int WFApi::deviceType(int n)
{
    if (n>=0 && n<deviceCount()) return (int)[wf deviceType:n];
    else return -1;
}
int WFApi::deviceSubType(int n)
{
    if (n>=0 && n<deviceCount()) return (int)[wf deviceSubType:n];
    else return -1;
}

int WFApi::connectionStatus(int sd) { return [wf connectionStatus:(WFSensorConnection*)connections.at(sd)]; }
bool WFApi::isConnected(int sd) { return [wf isConnected:(WFSensorConnection*)connections.at(sd)]; }
bool WFApi::hasData(int sd) { return [wf hasData:(WFSensorConnection*)connections.at(sd)]; }
int WFApi::connectDevice(QString uuid) {
    void *conn = (void*)[wf connectDevice: fromQString(uuid)];
    connections.append(conn);
    return (connections.count()-1);
}
bool WFApi::disconnectDevice(int sd) { return [wf disconnectDevice:(WFSensorConnection*)connections.at(sd)]; }
int WFApi::deviceCount() { return [wf deviceCount]; }

// set slope or ergo mode
void WFApi::setSlopeMode(int sd) { [wf setSlopeMode:(WFBikePowerConnection*)connections.at(sd)]; }
void WFApi::setErgoMode(int sd) { [wf setErgoMode:(WFBikePowerConnection*)connections.at(sd)]; }
void WFApi::setSlope(int sd, double n) { [wf setSlope:(WFBikePowerConnection*)connections.at(sd) slope:n]; }
void WFApi::setLoad(int sd, int n) { [wf setLoad:(WFBikePowerConnection*)connections.at(sd) load:n]; }

//============================================================================
// methods called by delegate on updates
//============================================================================
void WFApi::connectedSensor(void*) { }
void WFApi::didDiscoverDevices(int count, bool finished) { if (finished) emit discoverFinished();
                                                           else emit discoveredDevices(count,finished); }
void WFApi::disconnectedSensor(void*) { }
void WFApi::hasFirmwareUpdateAvalableForConnection() { }
void WFApi::stateChanged() { emit currentStateChanged(currentState()); }
void WFApi::connectionState(int status) { emit connectionStateChanged(status); }
void WFApi::connectionTimeout() { }
void WFApi::connectorHasData() { emit connectionHasData(); }
void WFApi::getRealtimeData(int sd, RealtimeData *rt) {
    WFBikePowerData *data = [wf getData:(WFSensorConnection*)connections.at(sd)];
    rt->setWatts((int)[data instantPower]);
    rt->setCadence((int)[data instantCadence]);
    rt->setWheelRpm((int)[data instantWheelRPM]);
}
void * WFApi::getPool() { return (void*)[wf getPool]; }
void WFApi::freePool(void *pool) { [wf freePool:(NSAutoreleasePool*)pool]; }
