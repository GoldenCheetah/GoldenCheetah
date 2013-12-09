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

#if QT_VERSION < 0x040000
#include <qwmatrix.h>
#define QwtMatrix QWMatrix
#define QwtPointArray QPointArray
#else
#include <qmatrix.h>
#define QwtMatrix QMatrix
#define QwtPointArray QPolygon
#endif

struct tick_info_t {
    double x;
    char *label;
};

static tick_info_t tick_info[] = {
    {  1.0/60.0,    "1s" },
    {  5.0/60.0,    "5s" },
    { 15.0/60.0,   "15s" },
    {       0.5,   "30s" },
    {       1.0,    "1m" },
    {       2.0,    "2m" },
    {       3.0,    "3m" },
    {       5.0,    "5m" },
    {      10.0,   "10m" },
    {      20.0,   "20m" },
    {      30.0,   "30m" },
    {      60.0,    "1h" },
    {     120.0,    "2h" },
    {     180.0,    "3h" },
    {     300.0,    "5h" },
    {      -1.0,    NULL }
};

void
LogTimeScaleDraw::drawLabel(QPainter *painter, double value) const
{
    QwtText lbl = tickLabel(painter->font(), value);
    if ( lbl.isEmpty() )
        return;

    const QPointF pos = labelPosition(value);

    QSizeF labelSize = lbl.textSize(painter->font());
    if ( labelSize.toSize().height() % 2 )
        labelSize.setHeight(labelSize.height() + 1);

    const QwtMatrix m = labelTransformation( pos, labelSize).toAffine();

    painter->save();
#if QT_VERSION < 0x040000
    painter->setWorldMatrix(m, true);
#else
    painter->setMatrix(m, true);
#endif

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

