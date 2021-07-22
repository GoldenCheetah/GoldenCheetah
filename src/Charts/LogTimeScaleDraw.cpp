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

#include "LogTimeScaleDraw.h"

#include <qpen.h>
#include <qpainter.h>
#include "qwt_math.h"
#include "qwt_painter.h"
#include "qwt_scale_div.h"
#include "qwt_scale_map.h"
#include "qwt_scale_draw.h"

#define QwtPointArray QPolygon

struct tick_info_t {
    double x;
    char *label;
};

static tick_info_t tick_info[] = {
    {  1.0/60.0,   (char*)"1s" },
    {  5.0/60.0,   (char*)"5s" },
    { 15.0/60.0,   (char*)"15s" },
    {       0.5,   (char*)"30s" },
    {       1.0,   (char*)"1m" },
    {       2.0,   (char*)"2m" },
    {       3.0,   (char*)"3m" },
    {       4.0,   (char*)"4m" },
    {       5.0,   (char*)"5m" },
    {      10.0,   (char*)"10m" },
    {      12.0,   (char*)"12m" },
    {      20.0,   (char*)"20m" },
    {      30.0,   (char*)"30m" },
    {      60.0,   (char*)"1h" },
    {     120.0,   (char*)"2h" },
    {     180.0,   (char*)"3h" },
    {     300.0,   (char*)"5h" },
    {      -1.0,   (char*)NULL }
};

QList<double> const
LogTimeScaleDraw::ticks = QList<double>() << 1.0/60.0 << 5.0/60.0 << 15.0/60.0 << 0.5 << 1 << 2 << 3 << 5 << 10 << 20 << 30 << 60 << 120 << 180 << 300;

QList<double> const
LogTimeScaleDraw::ticksEnergy = QList<double>() << 1 << 5 << 10 << 20 << 30 << 60 << 120 << 180 << 300;

QList<double> const
LogTimeScaleDraw::ticksVeloclinic = QList<double>() << 100 << 200 << 300 << 400 << 500 << 600 << 700 << 800 << 900 << 1000
                                                    << 1100 << 1200 << 1300 << 1400 << 1500 << 1600 << 1700 << 1800 << 1900 << 2000
                                                    << 2100 << 2200 << 2300 << 2400 << 2500 << 2600 << 2700 << 2800 << 2900 << 3000;


void
LogTimeScaleDraw::drawLabel(QPainter *painter, double value) const
{
    double tickValue = value;
    if (inv_time)
        tickValue = 1.0 / value;

    QwtText lbl = tickLabel(painter->font(), tickValue);

    if ( lbl.isEmpty() )
        return;

    const QPointF pos = labelPosition(value);

    QSizeF labelSize = lbl.textSize(painter->font());
    if ( labelSize.toSize().height() % 2 )
        labelSize.setHeight(labelSize.height() + 1);

    const QTransform m = labelTransformation( pos, labelSize);

    painter->save();
    painter->setTransform(m, true);

    lbl.draw (painter, QRect(QPoint(0, 0), labelSize.toSize()) );
    painter->restore();
}

const QwtText &
LogTimeScaleDraw::tickLabel(const QFont &font, double value) const
{
    QMap<double, QwtText>::const_iterator it = labelCache.find(value);
    if ( it == labelCache.end() )
    {
        QwtText lbl;

        tick_info_t *walker = tick_info;
        while (walker->label) {
            if (walker->x == value)
                break;
            ++walker;
        }
        if (walker->label)
            lbl = QwtText(QString(walker->label));
        else
            lbl = label(value);

        lbl.setRenderFlags(0);
        lbl.setLayoutAttribute(QwtText::MinimumLayout);

        (void)lbl.textSize(font); // initialize the internal cache

        it = labelCache.insert(value, lbl);
    }

    return (*it);
}

