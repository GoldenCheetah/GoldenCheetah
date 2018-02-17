/*
 * Copyright (c) 2018 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_Archive_h
#define _GC_Archive_h 1
#include "GoldenCheetah.h"

#include <QObject>

class Archive : public QObject
{
    Q_OBJECT

public:
    // all helpers
    static QList<QString> dir(QString name); // just get a list of the archive without extracting it
    static QStringList extract(QString archivename, QList<QString> list, QString folder); // just get a list of the archive without extracting it

signals:

public slots:

protected:
    QStringList contents;
};

#endif
