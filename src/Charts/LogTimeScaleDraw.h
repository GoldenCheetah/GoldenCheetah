/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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
 *
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 */

#ifndef _GC_LogTimeScaleDraw_h
#define _GC_LogTimeScaleDraw_h 1
#include "GoldenCheetah.h"

#include "qwt_scale_draw.h"
#include <qmap.h>

class LogTimeScaleDraw : public QwtScaleDraw
{
    public:
        LogTimeScaleDraw() : QwtScaleDraw(), inv_time(false) {}

        static const QList<double> ticks;
        static const QList<double> ticksEnergy;
        static const QList<double> ticksVeloclinic;
        bool inv_time;

    protected:

        virtual void drawLabel(QPainter *p, double val) const;
        virtual const QwtText & tickLabel(const QFont &font, double value) const;

        mutable QMap<double, QwtText> labelCache;

    private:

};

#endif // _GC_LogTimeScaleDraw_h

