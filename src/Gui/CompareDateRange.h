/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_CompareDateRange_h
#define _GC_CompareDateRange_h

class Context;
class RideFileCache;

#include "Specification.h"

#include <QColor>
#include <QDate>
#include <QString>

class CompareDateRange
{
    public:
        CompareDateRange() : context(NULL), sourceContext(NULL), checked(false), cache(NULL) {}
        ~CompareDateRange();

        Context *context;
        QString name;
        QColor color;
        Specification specification;
        QDate start, end;
        Context *sourceContext;
        bool checked;

        bool isChecked() const { return checked; }
        void setChecked(bool x) { checked=x; }

        RideFileCache *rideFileCache();

    private:
        RideFileCache *cache;
};

#endif
