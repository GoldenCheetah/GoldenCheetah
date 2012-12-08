/*
 * Copyright (c) 2009 Robert Carlsen (robert@robertcarlsen.net)
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
 *  DayScaleDraw.h
 *  GoldenCheetah
 *
 *  Provides specialized formatting for Plot axes
 */

#include <QApplication>
#include <qwt_scale_draw.h>

class DaysScaleDraw: public QwtScaleDraw
    {
    Q_DECLARE_TR_FUNCTIONS(DaysScaleDraw)

    public:
        DaysScaleDraw()
        {
        }
        virtual QwtText label(double v) const
        {
            switch(int(v))
            {
                case 1:
                    return QString(tr("Mon"));
                    break;
                case 2:
                    return QString(tr("Tue"));
                    break;
                case 3:
                    return QString(tr("Wed"));
                    break;
                case 4:
                    return QString(tr("Thu"));
                    break;
                case 5:
                    return QString(tr("Fri"));
                    break;
                case 6:
                    return QString(tr("Sat"));
                    break;
                case 7:
                    return QString(tr("Sun"));
                    break;
                default:
                    return QString(int(v));
                    break;
            }
        }
    };
