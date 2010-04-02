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
#define CPLOTTITLE            1
#define CPLOTSELECT           2
#define CPLOTTRACKER          3
#define CPLOTMARKER           4
#define CPLOTGRID             5
#define CINTERVALHIGHLIGHTER  6
#define CHEARTRATE            7
#define CSPEED                8
#define CPOWER                9
#define CCP                   10
#define CCADENCE              11
#define CALTITUDE             12
#define CALTITUDEBRUSH        13
#define CWINDSPEED            14
#define CTORQUE               15
#define CSTS                  16
#define CLTS                  17
#define CSB                   18
#define CDAILYSTRESS          19
#define CCALENDARTEXT         20
#define CZONE1                21
#define CZONE2                22
#define CZONE3                23
#define CZONE4                24
#define CZONE5                25
#define CZONE6                26
#define CZONE7                27
#define CZONE8                28
#define CZONE9                29
#define CZONE10               30
#define CAEROVE               31
#define CAEROEL               32

#endif
