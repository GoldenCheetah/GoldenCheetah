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
    isDefaultDownload=false;
    isDefaultRealtime=false;
    postProcess=0;
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

    boost::shared_ptr<QSettings> settings = GetApplicationSettings();

    // get count of devices
    QVariant configVal = settings->value(GC_DEV_COUNT);
    if (configVal.isNull()) {
        count=0;
    } else {
        count = configVal.toInt();
    }

    // for each device
    for (int i=0; i< count; i++) {

            DeviceConfiguration Entry;

            QString configStr = QString("%1%2").arg(GC_DEV_NAME).arg(i+1);
            configVal = settings->value(configStr);
            Entry.name = configVal.toString();

            configStr = QString("%1%2").arg(GC_DEV_SPEC).arg(i+1);
            configVal = settings->value(configStr);
            Entry.portSpec = configVal.toString();

            configStr = QString("%1%2").arg(GC_DEV_TYPE).arg(i+1);
            configVal = settings->value(configStr);
            Entry.type = configVal.toInt();

            configStr = QString("%1%2").arg(GC_DEV_PROF).arg(i+1);
            configVal = settings->value(configStr);
            Entry.deviceProfile = configVal.toString();

            configStr = QString("%1%2").arg(GC_DEV_DEFI).arg(i+1);
            configVal = settings->value(configStr);
            Entry.isDefaultDownload = configVal.toInt();

            configStr = QString("%1%2").arg(GC_DEV_DEFR).arg(i+1);
            configVal = settings->value(configStr);
            Entry.isDefaultRealtime = configVal.toInt();

            configStr = QString("%1%2").arg(GC_DEV_VIRTUAL).arg(i+1);
            configVal = settings->value(configStr);
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
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();

    settings->setValue(GC_DEV_COUNT, Configuration.count());
    for (i=0; i < Configuration.count(); i++) {

        // name
        QString configStr = QString("%1%2").arg(GC_DEV_NAME).arg(i+1);
        settings->setValue(configStr, Configuration.at(i).name);

        // type
        configStr = QString("%1%2").arg(GC_DEV_TYPE).arg(i+1);
        settings->setValue(configStr, Configuration.at(i).type);

        // portSpec
        configStr = QString("%1%2").arg(GC_DEV_SPEC).arg(i+1);
        settings->setValue(configStr, Configuration.at(i).portSpec);

        // deviceProfile
        configStr = QString("%1%2").arg(GC_DEV_PROF).arg(i+1);
        settings->setValue(configStr, Configuration.at(i).deviceProfile);

        // isDefaultDownload
        configStr = QString("%1%2").arg(GC_DEV_DEFI).arg(i+1);
        settings->setValue(configStr, Configuration.at(i).isDefaultDownload);

        // isDefaultRealtime
        configStr = QString("%1%2").arg(GC_DEV_DEFR).arg(i+1);
        settings->setValue(configStr, Configuration.at(i).isDefaultRealtime);

        // virtual post Process...
        configStr = QString("%1%2").arg(GC_DEV_VIRTUAL).arg(i+1);
        settings->setValue(configStr, Configuration.at(i).postProcess);
    }

}
