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


// Lists all the devices configured in preferences

#ifndef _GC_DeviceConfiguration_h
#define  _GC_DeviceConfiguration_h
#include "GoldenCheetah.h"

class RealtimeController;

class DeviceConfiguration
{
    public:   // direct update to class vars from deviceModel!
    DeviceConfiguration();

    int type;
    QString name,
         portSpec,
         deviceProfile;         // device specific data
                                // used by ANT to store ANTIDs
                                // available for use by all devices

    int wheelSize;              // set wheel size for each device
    double inertialMomentKGM2;  // trainer inertial moment in kg m^2
    int stridelength;           // stride length in cm

    int postProcess;
    QString virtualPowerDefinitionString;

    RealtimeController *controller; // can be used to allocate controller for this device
                                    // although a bit odd, it makes synchronising the config
                                    // with runtime allocation a bit simpler in traintool
                                    // we could subclass and add our own, but this aint so bad
};

class DeviceConfigurations
{
    public:
        DeviceConfigurations();
        ~DeviceConfigurations();

        QList<DeviceConfiguration> getList();

        // serialise/deserialise config
        QList<DeviceConfiguration> readConfig();
        void writeConfig(QList<DeviceConfiguration>);

    private:
        QList<DeviceConfiguration> Entries;       // all the Configurations

};

class WheelSize
{
    public:
        const static QStringList RIM_SIZES;
        const static QStringList TIRE_SIZES;

        static int         calcPerimeter(int rimSizeIndex, int tireSizeIndex);

};

#endif
