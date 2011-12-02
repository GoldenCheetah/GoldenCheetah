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
#include "GoldenCheetah.h"

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
    G_OBJECT


    public:
        GCColor(MainWindow*);
        static QColor getColor(int);
        static const Colors *colorSet();
        static const Colors *defaultColorSet();
        static void resetColors();
        static QColor invert(QColor);

    public slots:
        void readConfig();
};

// return a color for a ride file
class ColorEngine : public QObject
{
    Q_OBJECT
    G_OBJECT

    public:
        ColorEngine(MainWindow*);

        QColor colorFor(QString);

    public slots:
        void configUpdate();

    private:
        QMap<QString, QColor> workoutCodes;
        QColor defaultColor;
        MainWindow *mainWindow;
};


// shorthand
#define GColor(x) GCColor::getColor(x)

#define CPLOTBACKGROUND       0
#define CRIDEPLOTBACKGROUND   1
#define CPLOTTHUMBNAIL        2
#define CPLOTTITLE            3
#define CPLOTSELECT           4
#define CPLOTTRACKER          5
#define CPLOTMARKER           6
#define CPLOTGRID             7
#define CINTERVALHIGHLIGHTER  8
#define CHEARTRATE            9
#define CSPEED                10
#define CPOWER                11
#define CCP                   12
#define CCADENCE              13
#define CALTITUDE             14
#define CALTITUDEBRUSH        15
#define CWINDSPEED            16
#define CTORQUE               17
#define CSTS                  18
#define CLTS                  19
#define CSB                   20
#define CDAILYSTRESS          21
#define CCALENDARTEXT         22
#define CZONE1                23
#define CZONE2                24
#define CZONE3                25
#define CZONE4                26
#define CZONE5                27
#define CZONE6                28
#define CZONE7                29
#define CZONE8                30
#define CZONE9                31
#define CZONE10               32
#define CHZONE1               33
#define CHZONE2               34
#define CHZONE3               35
#define CHZONE4               36
#define CHZONE5               37
#define CHZONE6               38
#define CHZONE7               39
#define CHZONE8               40
#define CHZONE9               41
#define CHZONE10              42
#define CAEROVE               43
#define CAEROEL               44
#define CCALCELL              45
#define CCALHEAD              46
#define CCALCURRENT           47
#define CCALACTUAL            48
#define CCALPLANNED           49
#define CCALTODAY             50
#define CPOPUP                51
#define CPOPUPTEXT            52
#define CTILEBAR              53
#define CTILEBARSELECT        54
#define CTOOLBAR              55
#define CRIDEGROUP            56
#define CSPINSCANLEFT         57
#define CSPINSCANRIGHT        58

#endif
