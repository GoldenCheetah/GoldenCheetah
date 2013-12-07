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
#define CNUMOFCFGCOLORS       75

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
#define CNPOWER               15
#define CXPOWER               16
#define CAPOWER               17
#define CCP                   18
#define CCADENCE              19
#define CALTITUDE             20
#define CALTITUDEBRUSH        21
#define CWINDSPEED            22
#define CTORQUE               23
#define CLOAD                 24
#define CTSS                  25
#define CSTS                  26
#define CLTS                  27
#define CSB                   28
#define CDAILYSTRESS          29
#define CBIKESCORE            30
#define CCALENDARTEXT         31
#define CZONE1                32
#define CZONE2                33
#define CZONE3                34
#define CZONE4                35
#define CZONE5                36
#define CZONE6                37
#define CZONE7                38
#define CZONE8                39
#define CZONE9                40
#define CZONE10               41
#define CHZONE1               42
#define CHZONE2               43
#define CHZONE3               44
#define CHZONE4               45
#define CHZONE5               46
#define CHZONE6               47
#define CHZONE7               48
#define CHZONE8               49
#define CHZONE9               50
#define CHZONE10              51
#define CAEROVE               52
#define CAEROEL               53
#define CCALCELL              54
#define CCALHEAD              55
#define CCALCURRENT           56
#define CCALACTUAL            57
#define CCALPLANNED           58
#define CCALTODAY             59
#define CPOPUP                60
#define CPOPUPTEXT            61
#define CTILEBAR              62
#define CTILEBARSELECT        63
#define CTOOLBAR              64
#define CRIDEGROUP            65
#define CSPINSCANLEFT         66
#define CSPINSCANRIGHT        67
#define CTEMP                 68
#define CDIAL                 69
#define CALTPOWER             70
#define CBALANCELEFT          71
#define CBALANCERIGHT         72
#define CWBAL                 73
#define CRIDECP               74

#endif
