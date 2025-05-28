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
#include "IntervalItem.h"
#include "RideCache.h"

FreeSearch::FreeSearch()
{
    // nothing to do, all the data we need is in the ridecache
}

FreeSearch::~FreeSearch()
{
}

static QStringList searchSplit(QString string)
{
    const QString whitespace(" \n\r\t");
    QStringList returning;
    bool inQuotes = false;

    QString current;
    for (int i=0; i<string.length(); i++) {

        // got a bit of whitespace after a word so split and add
        if (current != "" && !inQuotes && whitespace.contains(string[i])) {
            returning << current;
            current = "";
            continue;
        }

        // quote delimeted
        if (string[i] == '\"') {
            if (inQuotes) {
                returning << current;
                current = "";
                inQuotes = false;
            } else {
                inQuotes = true;
            }
            continue;
        }

        // escaped
        if (string[i] == '\\' && i < (string.length()-1)) {
            i++;
            current += string[i];
            continue;
        }

        // just append current character
        current += string[i];
    }

    if (current != "") returning << current;

    return returning;
}

QList<QString> FreeSearch::search(Context* context, QString query)
{
    filenames.clear();

    // search split will tokenise and handle quoting and escaping
    QStringList tokens = searchSplit(query);

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

        // user intervals - even autodiscovered
        foreach(IntervalItem *interval, item->intervals()) {

            foreach (QString token, tokens) {

                if (interval->name.contains(token, Qt::CaseInsensitive)) {
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
