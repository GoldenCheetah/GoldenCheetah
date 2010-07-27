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

#ifndef _GC_Colors_h
#define _GC_Colors_h 1

#include <QObject>
#include <QString>
#include <QColor>

class MainWindow;

struct Colors
{
        QString name,
                setting;
        QColor  color;
};

class GCColor : public QObject
{
    Q_OBJECT

    public:
        GCColor(MainWindow*);
        static QColor getColor(int);
        static const Colors *colorSet();
        static QColor invert(QColor);

    public slots:
        void readConfig();
};

// shorthand
#define GColor(x) GCColor::getColor(x)

#define CPLOTBACKGROUND       0
#define CPLOTTHUMBNAIL        1
#define CPLOTTITLE            2
#define CPLOTSELECT           3
#define CPLOTTRACKER          4
#define CPLOTMARKER           5
#define CPLOTGRID             6
#define CINTERVALHIGHLIGHTER  7
#define CHEARTRATE            8
#define CSPEED                9
#define CPOWER                10
#define CCP                   11
#define CCADENCE              12
#define CALTITUDE             13
#define CALTITUDEBRUSH        14
#define CWINDSPEED            15
#define CTORQUE               16
#define CSTS                  17
#define CLTS                  18
#define CSB                   19
#define CDAILYSTRESS          20
#define CCALENDARTEXT         21
#define CZONE1                22
#define CZONE2                23
#define CZONE3                24
#define CZONE4                25
#define CZONE5                26
#define CZONE6                27
#define CZONE7                28
#define CZONE8                29
#define CZONE9                30
#define CZONE10               31
#define CAEROVE               32
#define CAEROEL               33

#endif
