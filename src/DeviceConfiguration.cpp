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

#include <QDir> // used by settings.h
#include <QDebug>
#include <QString>
#include "Settings.h"
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"

//
// Model for the device list, populates tie deviceList QTableView
//

DeviceConfiguration::DeviceConfiguration()
{
    // just set all to empty!
    type=0;
    defaultString="";
    wheelSize=2100;
    postProcess=0;
    controller=NULL;
}


DeviceConfigurations::DeviceConfigurations()
{
    // read in the config - it updates the local Entries QList
    readConfig();
}

DeviceConfigurations::~DeviceConfigurations()
{
}


QList<DeviceConfiguration>
DeviceConfigurations::getList()
{
    return Entries;
}

// serialise/deserialise config
QList<DeviceConfiguration>
DeviceConfigurations::readConfig()
{
    int count;

    // get count of devices
    QVariant configVal = appsettings->value(NULL, GC_DEV_COUNT);
    if (configVal.isNull()) {
        count=0;
    } else {
        count = configVal.toInt();
    }

    // get list of supported devices
    DeviceTypes all;

    // for each device
    for (int i=0; i< count; i++) {

            DeviceConfiguration Entry;

            QString configStr = QString("%1%2").arg(GC_DEV_TYPE).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.type = configVal.toInt();

            bool supported = false;
            foreach(DeviceType s, all.getList()) {
                if (s.type == Entry.type) supported = true;
            }

            // skip unsupported devices
            if (supported == false) continue;

            configStr = QString("%1%2").arg(GC_DEV_NAME).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.name = configVal.toString();

            configStr = QString("%1%2").arg(GC_DEV_SPEC).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.portSpec = configVal.toString();

            configStr = QString("%1%2").arg(GC_DEV_WHEEL).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.wheelSize = configVal.toInt();
            if (Entry.wheelSize == 0) Entry.wheelSize = 2100; // default to 700C

            configStr = QString("%1%2").arg(GC_DEV_PROF).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.deviceProfile = configVal.toString();

            configStr = QString("%1%2").arg(GC_DEV_DEF).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.defaultString = configVal.toString();

            configStr = QString("%1%2").arg(GC_DEV_VIRTUAL).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.postProcess = configVal.toInt();

            Entries.append(Entry);
    }
    return Entries;
}

void
DeviceConfigurations::writeConfig(QList<DeviceConfiguration> Configuration)
{
    // loop through the entries in the Configuration QList
    // writing to the GC settings

    int i=0;
    appsettings->setValue(GC_DEV_COUNT, Configuration.count());
    for (i=0; i < Configuration.count(); i++) {

        // name
        QString configStr = QString("%1%2").arg(GC_DEV_NAME).arg(i+1);
        appsettings->setValue(configStr, Configuration.at(i).name);

        // type
        configStr = QString("%1%2").arg(GC_DEV_TYPE).arg(i+1);
        appsettings->setValue(configStr, Configuration.at(i).type);
        
        // wheel size
        configStr = QString("%1%2").arg(GC_DEV_WHEEL).arg(i+1);
        appsettings->setValue(configStr, Configuration.at(i).wheelSize);

        // portSpec
        configStr = QString("%1%2").arg(GC_DEV_SPEC).arg(i+1);
        appsettings->setValue(configStr, Configuration.at(i).portSpec);

        // deviceProfile
        configStr = QString("%1%2").arg(GC_DEV_PROF).arg(i+1);
        appsettings->setValue(configStr, Configuration.at(i).deviceProfile);

        // default string
        configStr = QString("%1%2").arg(GC_DEV_DEF).arg(i+1);
        appsettings->setValue(configStr, Configuration.at(i).defaultString);

        // virtual post Process...
        configStr = QString("%1%2").arg(GC_DEV_VIRTUAL).arg(i+1);
        appsettings->setValue(configStr, Configuration.at(i).postProcess);
    }

}
