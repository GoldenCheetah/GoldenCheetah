/*;
 * Copyright (c) 2013 Darren Hague
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

 #include "ANTLogger.h"

ANTLogger::ANTLogger(QObject *parent) : QObject(parent)
{
    isLogging=false;
}

void
ANTLogger::open()
{
    if (isLogging) return;

    antlog.setFileName("antlog.bin");
    antlog.open(QIODevice::WriteOnly | QIODevice::Truncate);
    isLogging=true;
}

void
ANTLogger::close()
{
    if (!isLogging) return;

    antlog.close();
    isLogging=false;
}

void ANTLogger::logRawAntMessage(const ANTMessage message, const struct timeval timestamp)
{
    Q_UNUSED(timestamp); // not used at present
    if (isLogging) {
        QDataStream out(&antlog);

        for (int i=0; i<ANT_MAX_MESSAGE_SIZE; i++)
            out<<message.data[i];
    }
}
