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

#include "FreeSearch.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "RideCache.h"

FreeSearch::FreeSearch(QObject *parent, Context *context) : QObject(parent), context(context)
{
    // nothing to do, all the data we need is in the ridecache
}

FreeSearch::~FreeSearch()
{
}

QList<QString> FreeSearch::search(QString query)
{
    filenames.clear();

    QStringList tokens = query.split(" ", QString::SkipEmptyParts);

    foreach(RideItem*item, context->athlete->rideCache->rides()) {

        QMapIterator<QString,QString> meta(item->metadata());
        meta.toFront();
        while (meta.hasNext()) {
            meta.next();

            // does the super string contain the tokens?
            foreach(QString token, tokens) {

                if (meta.value().contains(token, Qt::CaseInsensitive)) {
                    filenames << item->fileName;
                    goto nextItem;
                }
            }
        }
        nextItem:;
    }

    emit results(filenames);

    return filenames;
}
