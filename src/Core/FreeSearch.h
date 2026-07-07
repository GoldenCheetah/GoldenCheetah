/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_FreeSearch_h
#define _GC_FreeSearch_h

#include <QObject>
#include <QString>
#include <QDir>
#include <QMutex>

#include "Context.h"
#include "RideMetadata.h"
#include "RideCache.h"
#include "RideItem.h"

class FreeSearch : public QObject
{
    Q_OBJECT

public:
    FreeSearch();
    ~FreeSearch();

protected:

public slots:

    // search metadata texts in ridecache
    QList<QString> search(Context* context, QString query);

signals:
    void results(QStringList);

private:
    // Query results
    QStringList filenames;
};

#endif
