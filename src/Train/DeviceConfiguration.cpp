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
#include "RealtimeController.h"

#define PI M_PI

//
// Model for the device list, populates tie deviceList QTableView
//

DeviceConfiguration::DeviceConfiguration()
{
    // just set all to empty!
    type=0;
    wheelSize=2100;
    postProcess=0;
    stridelength=80;
    controller = NULL;
    virtualPowerDefinitionString = "";
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

            configStr = QString("%1%2").arg(GC_DEV_STRIDE).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.stridelength = configVal.toInt();
            if (Entry.stridelength == 0) Entry.stridelength = 78; // default to 78cm

            configStr = QString("%1%2").arg(GC_DEV_PROF).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.deviceProfile = configVal.toString();

            configStr = QString("%1%2").arg(GC_DEV_VIRTUAL).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.postProcess = configVal.toInt();

            configStr = QString("%1%2").arg(GC_DEV_VIRTUALPOWER).arg(i+1);
            configVal = appsettings->value(NULL, configStr);
            Entry.virtualPowerDefinitionString = configVal.toString();

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

        // stridelength
        configStr = QString("%1%2").arg(GC_DEV_STRIDE).arg(i+1);
        appsettings->setValue(configStr, QString("%1").arg(Configuration.at(i).stridelength));

        // portSpec
        configStr = QString("%1%2").arg(GC_DEV_SPEC).arg(i+1);
        appsettings->setValue(configStr, Configuration.at(i).portSpec);

        // deviceProfile
        configStr = QString("%1%2").arg(GC_DEV_PROF).arg(i+1);
        appsettings->setValue(configStr, Configuration.at(i).deviceProfile);

        // virtual post Process and definition string
        VirtualPowerTrainerManager& vptm = Configuration.at(i).controller->virtualPowerTrainerManager;

        int postProcess = Configuration.at(i).postProcess;
        bool isPredefinedPostProcess = vptm.IsPredefinedVirtualPowerTrainerIndex(postProcess);

        int postProcessStoreValue = postProcess;
        QString s = Configuration.at(i).virtualPowerDefinitionString;

        if (!isPredefinedPostProcess) {
            postProcessStoreValue = 0;
            vptm.GetVirtualPowerTrainerAsString(postProcess, s);
        }

        configStr = QString("%1%2").arg(GC_DEV_VIRTUAL).arg(i + 1);
        appsettings->setValue(configStr, postProcessStoreValue);

        configStr = QString("%1%2").arg(GC_DEV_VIRTUALPOWER).arg(i + 1);
        appsettings->setValue(configStr, QString(s));
    }
}

const QStringList
WheelSize::RIM_SIZES = QStringList() << "--" << "700c/29er/622mm" << "650b/27.5\"/584mm" << "650c/571mm" << "26\"/559mm";

static const float RIM_DIAMETERS[] = {0, 622, 584, 571, 559};

const QStringList
WheelSize::TIRE_SIZES = QStringList() << "--" << "20mm" << "23mm" << "25mm" << "28mm" << "1.00\"" << "1.50\"" << "1.75\"" << "1.90\"" << "1.95\"" << "2.00\"" << "2.10\"" << "2.125\"";

                                    // -- 20mm 23mm 25mm 28mm 1.00"  1.50"  1.75"  1.90"  1.95"  2.00"  2.10"  2.125"
static const float TIRE_DIAMETERS[] = {0, 40,  46,  50,  56,  50.8f,  76.2f,  88.9f,  96.5f,  99.1f,  101.6f, 106.7f, 108};

int
WheelSize::calcPerimeter(int rimSizeIndex, int tireSizeIndex)
{
    // http://www.bikecalc.com/wheel_size_math

    if (rimSizeIndex>=0
        && rimSizeIndex<static_cast<int>(sizeof(RIM_DIAMETERS)/sizeof(*RIM_DIAMETERS))
        && tireSizeIndex>=0
        && tireSizeIndex<=static_cast<int>(sizeof(TIRE_DIAMETERS)/sizeof(*TIRE_DIAMETERS))) {

        float rim = RIM_DIAMETERS[rimSizeIndex];
        float tire = TIRE_DIAMETERS[tireSizeIndex];

        if (rim>0 && tire>0) {
            return round((rim+tire) * PI);
        }
    }
    return 0;
}
