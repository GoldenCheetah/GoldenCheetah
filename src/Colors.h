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

// set appearace defaults based upon screen size
struct SizeSettings {

    // this applies up to the following geometry
    int maxheight,
        maxwidth;

    // font size
    int defaultFont,
        titleFont,
        markerFont,
        labelFont,
        calendarFont,
        popupFont;

    // screen dimension
    int width,
        height;
};

extern SizeSettings defaultAppearance[];
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

        void setupColors();
    public:
        GCColor(MainWindow*);
        static QColor getColor(int);
        static const Colors *colorSet();
        static const Colors *defaultColorSet();
        static void resetColors();
        static QColor invert(QColor);
        static struct SizeSettings defaultSizes(int width, int height);

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

// Define how many cconfigurable metric colors are available
#define CNUMOFCFGCOLORS       68

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
#define CLOAD                 18
#define CTSS                  19
#define CSTS                  20
#define CLTS                  21
#define CSB                   22
#define CDAILYSTRESS          23
#define CBIKESCORE            24
#define CCALENDARTEXT         25
#define CZONE1                26
#define CZONE2                27
#define CZONE3                28
#define CZONE4                29
#define CZONE5                30
#define CZONE6                31
#define CZONE7                32
#define CZONE8                33
#define CZONE9                34
#define CZONE10               35
#define CHZONE1               36
#define CHZONE2               37
#define CHZONE3               38
#define CHZONE4               39
#define CHZONE5               40
#define CHZONE6               41
#define CHZONE7               42
#define CHZONE8               43
#define CHZONE9               44
#define CHZONE10              45
#define CAEROVE               46
#define CAEROEL               47
#define CCALCELL              48
#define CCALHEAD              49
#define CCALCURRENT           50
#define CCALACTUAL            51
#define CCALPLANNED           52
#define CCALTODAY             53
#define CPOPUP                54
#define CPOPUPTEXT            55
#define CTILEBAR              56
#define CTILEBARSELECT        57
#define CTOOLBAR              58
#define CRIDEGROUP            59
#define CSPINSCANLEFT         60
#define CSPINSCANRIGHT        61
#define CTEMP                 62
#define CDIAL                 63
#define CALTPOWER             64
#define CBALANCELEFT          65
#define CBALANCERIGHT         66

#endif
