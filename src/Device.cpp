/* 
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

#include "Device.h"
#include "Serial.h"

static QVector<Device::ListFunction> *listFunctions;

bool
Device::addListFunction(ListFunction f)
{
    if (!listFunctions)
        listFunctions = new QVector<Device::ListFunction>;
    listFunctions->append(f);
    return true;
}

QVector<DevicePtr>
Device::listDevices(QString &err)
{
    err = "";
    QVector<DevicePtr> result;
    for (int i = 0; listFunctions && i < listFunctions->size(); ++i) {
        QVector<DevicePtr> tmp = (*listFunctions)[i](err);
        if (err == "")
            result << tmp;
        else
            err += "\n";
    }
    return result;
}

