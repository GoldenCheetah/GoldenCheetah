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

#include "CommPort.h"
#include "Serial.h"

static QVector<CommPort::ListFunction> *listFunctions;

bool
CommPort::addListFunction(ListFunction f)
{
    if (!listFunctions)
        listFunctions = new QVector<CommPort::ListFunction>;
    listFunctions->append(f);
    return true;
}

QVector<CommPortPtr>
CommPort::listCommPorts(QString &err)
{
    QStringList errors;
    QVector<CommPortPtr> result;
    for (int i = 0; listFunctions && i < listFunctions->size(); ++i) {
        QString thisError = "";
        QVector<CommPortPtr> tmp = (*listFunctions)[i](thisError);
        if (thisError == "")
            result << tmp;
        else
            errors << thisError;
    }
    if (result.isEmpty() && !errors.isEmpty())
        err = errors.join("\n");
    return result;
}

