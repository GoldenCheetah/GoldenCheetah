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
class Colors
{
public:
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
#define CNUMOFCFGCOLORS       70

#define CPLOTBACKGROUND       0
#define CRIDEPLOTBACKGROUND   1
#define CPLOTSYMBOL           2
#define CRIDEPLOTXAXIS        3
#define CRIDEPLOTYAXIS        4
#define CPLOTTHUMBNAIL        5
#define CPLOTTITLE            6
#define CPLOTSELECT           7
#define CPLOTTRACKER          8
#define CPLOTMARKER           9
#define CPLOTGRID             10
#define CINTERVALHIGHLIGHTER  11
#define CHEARTRATE            12
#define CSPEED                13
#define CPOWER                14
#define CCP                   15
#define CCADENCE              16
#define CALTITUDE             17
#define CALTITUDEBRUSH        18
#define CWINDSPEED            19
#define CTORQUE               20
#define CLOAD                 21
#define CTSS                  22
#define CSTS                  23
#define CLTS                  24
#define CSB                   25
#define CDAILYSTRESS          26
#define CBIKESCORE            27
#define CCALENDARTEXT         28
#define CZONE1                29
#define CZONE2                30
#define CZONE3                31
#define CZONE4                32
#define CZONE5                33
#define CZONE6                34
#define CZONE7                35
#define CZONE8                36
#define CZONE9                37
#define CZONE10               38
#define CHZONE1               39
#define CHZONE2               40
#define CHZONE3               41
#define CHZONE4               42
#define CHZONE5               43
#define CHZONE6               44
#define CHZONE7               45
#define CHZONE8               46
#define CHZONE9               47
#define CHZONE10              48
#define CAEROVE               49
#define CAEROEL               50
#define CCALCELL              51
#define CCALHEAD              52
#define CCALCURRENT           53
#define CCALACTUAL            54
#define CCALPLANNED           55
#define CCALTODAY             56
#define CPOPUP                57
#define CPOPUPTEXT            58
#define CTILEBAR              59
#define CTILEBARSELECT        60
#define CTOOLBAR              61
#define CRIDEGROUP            62
#define CSPINSCANLEFT         63
#define CSPINSCANRIGHT        64
#define CTEMP                 65
#define CDIAL                 66
#define CALTPOWER             67
#define CBALANCELEFT          68
#define CBALANCERIGHT         69

#endif
