/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_GcBubble_h
#define _GC_GcBubble_h 1
#include "GoldenCheetah.h"
#include "Context.h"

#include <QtGui>
#include "Colors.h"

class GcBubble : public QWidget
{
    Q_OBJECT
    G_OBJECT

    public:
        GcBubble(Context *context = NULL);
        void setText(QString); // set the text displayed according to filename

    protected:  
        // cached state
        QSettings *settings;
        bool useMetricUnits;

    signals:

    public slots:
        void setPos(int x, int y, Qt::Orientation orientation = Qt::Horizontal);

    protected:
        virtual void paintEvent(QPaintEvent *);

    private:
        Context *context;

        int borderWidth;

        QPoint start; // where the bubble spike is
        QPoint coords; // where on the screen (global) the bubble should be placed
                       // adjusted so the start point is just to the right of the cursor

        enum pos { left, right, top, bottom };
        typedef enum pos SpikePosition;

        SpikePosition spikePosition;

        QPainterPath path;

        Qt::Orientation orientation; // horizontal or vertical?

        QWidget *display;

        QLabel *topleft, *topmiddle, *topright; // workout, date, sport
        QLabel *bike, *run, *swim; // icons in top right
        QLabel *topmiddle2; // time
        QLabel *m1, *m2, *m3, *m4; // dist, duration, tss, if
        QLabel *speed, *power, *hr, *cad, *torque, *alt, *gps, *temp, *wind; // icons at bottom
};

#endif // _GC_GcBubble_h
