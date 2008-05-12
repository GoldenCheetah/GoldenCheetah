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

extern tick_info_t tick_info[];

void 
LogTimeScaleDraw::drawLabel(QPainter *painter, double value) const 
{
    QwtText lbl = tickLabel(painter->font(), value);
    if ( lbl.isEmpty() )
        return; 

    const QPoint pos = labelPosition(value);

    QSize labelSize = lbl.textSize(painter->font());
    if ( labelSize.height() % 2 )
        labelSize.setHeight(labelSize.height() + 1);
    
    const QwtMatrix m = labelMatrix( pos, labelSize);

    painter->save();
#if QT_VERSION < 0x040000
    painter->setWorldMatrix(m, true);
#else
    painter->setMatrix(m, true);
#endif

    lbl.draw (painter, QRect(QPoint(0, 0), labelSize) );
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

