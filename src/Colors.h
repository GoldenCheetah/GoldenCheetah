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

class Context;

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
        GCColor(Context *);
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
        ColorEngine(Context *);

        QColor colorFor(QString);

    public slots:
        void configUpdate();

    private:
        QMap<QString, QColor> workoutCodes;
        QColor defaultColor;
        Context *context;
};


// shorthand
#define GColor(x) GCColor::getColor(x)

// Define how many cconfigurable metric colors are available
#define CNUMOFCFGCOLORS       76

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
#define CACCELERATION         14
#define CPOWER                15
#define CNPOWER               16
#define CXPOWER               17
#define CAPOWER               18
#define CCP                   19
#define CCADENCE              20
#define CALTITUDE             21
#define CALTITUDEBRUSH        22
#define CWINDSPEED            23
#define CTORQUE               24
#define CLOAD                 25
#define CTSS                  26
#define CSTS                  27
#define CLTS                  28
#define CSB                   29
#define CDAILYSTRESS          30
#define CBIKESCORE            31
#define CCALENDARTEXT         32
#define CZONE1                33
#define CZONE2                34
#define CZONE3                35
#define CZONE4                36
#define CZONE5                37
#define CZONE6                38
#define CZONE7                39
#define CZONE8                40
#define CZONE9                41
#define CZONE10               42
#define CHZONE1               43
#define CHZONE2               44
#define CHZONE3               45
#define CHZONE4               46
#define CHZONE5               47
#define CHZONE6               48
#define CHZONE7               49
#define CHZONE8               50
#define CHZONE9               51
#define CHZONE10              52
#define CAEROVE               53
#define CAEROEL               54
#define CCALCELL              55
#define CCALHEAD              56
#define CCALCURRENT           57
#define CCALACTUAL            58
#define CCALPLANNED           59
#define CCALTODAY             60
#define CPOPUP                61
#define CPOPUPTEXT            62
#define CTILEBAR              63
#define CTILEBARSELECT        64
#define CTOOLBAR              65
#define CRIDEGROUP            66
#define CSPINSCANLEFT         67
#define CSPINSCANRIGHT        68
#define CTEMP                 69
#define CDIAL                 70
#define CALTPOWER             71
#define CBALANCELEFT          72
#define CBALANCERIGHT         73
#define CWBAL                 74
#define CRIDECP               75

#endif
