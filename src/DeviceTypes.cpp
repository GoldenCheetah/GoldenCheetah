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
#include <QDebug>
#include "DeviceTypes.h"

// NOTE:
// Device Types not fully supported in this release (functionally) have
// been commented out. As new features or existing features are updated to
// use this new class then the lines will be uncommented
//
// As a result, only Realtime uses this feature at present (and the associated
// configuration DeviceConfiguration class and preferences pane

static DeviceType SupportedDevices[] =
{
#ifdef Q_OS_WIN32
      { DEV_ANTLOCAL, DEV_USB,     (char *) "Native ANT+",           true,    false },
#else
      { DEV_ANTLOCAL, DEV_SERIAL,  (char *) "Native ANT+",           true,    false },
#endif
      { DEV_CT,       DEV_SERIAL,  (char *) "Racermate Computrainer",true,    false },
#ifdef GC_HAVE_LIBUSB
      { DEV_FORTIUS,  DEV_LIBUSB,  (char *) "Tacx Fortius",          true,    false },
#endif
      { DEV_GSERVER,  DEV_TCP,     (char *) "Golden Cheetah Server", false,   false },
      { DEV_NULL,     DEV_TCP,     (char *) "Null device (testing)", false,   false },
      { DEV_ANTPLUS,  DEV_QUARQ,   (char *) "ANT+ via Quarqd",       true,    false },
//    { DEV_PT,       DEV_SERIAL,  (char *) "Powertap Head Unit",    false,   true  },
//    { DEV_SRM,      DEV_SERIAL,  (char *) "SRM PowerControl V/VI", false,   true  },
//    { DEV_GCLIENT,  DEV_TCP,     (char *) "Golden Cheetah Client", false,   false },
      { 0, 0, NULL, 0, 0 }
};

DeviceTypes::DeviceTypes()
{
    for (int i=0; SupportedDevices[i].type;i++)
         Supported.append(SupportedDevices[i]);
}

DeviceTypes::~DeviceTypes()
{}

QList<DeviceType> DeviceTypes::getList()
{
    return Supported;
}

DeviceType DeviceTypes::getType(int type)
{
    for (int i=0; i< Supported.count(); i++) {
        if (Supported.at(i).type == type)
            return Supported.at(i);
    }
    return Supported.at(0); // yuck.whatever
}
