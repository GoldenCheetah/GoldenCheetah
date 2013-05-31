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

#include <sys/time.h>
#include <QObject>
#include "ANTMessage.h"

#ifndef ANTLOGGER_H
#define ANTLOGGER_H

class ANTLogger : public QObject
{
    Q_OBJECT
public:
    explicit ANTLogger(QObject *parent = 0);
    
signals:
    
public slots:
    void logRawAntMessage(const ANTMessage message, const struct timeval timestamp);
    void open();
    void close();

private:
    // antlog.bin ant message stream
    QFile antlog;
    bool isLogging;

};

#endif // ANTLOGGER_H
