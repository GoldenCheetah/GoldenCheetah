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

class DeviceConfiguration
{
    public:   // direct update to class vars from deviceModel!
    DeviceConfiguration();

    int type;
    QString name,
         portSpec,
         deviceProfile;        // device specific data
                                // used by ANT to store ANTIDs
                                // available for use by all devices

    bool isDefaultDownload,     // not implemented yet
         isDefaultRealtime;     // not implemented yet

    int  postProcess;
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

#endif
