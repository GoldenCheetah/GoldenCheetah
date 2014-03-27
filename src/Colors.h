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
        static struct SizeSettings defaultSizes(int width, int height);
        static QColor invertColor(QColor); // return the contrasting color
        static QColor alternateColor(QColor); // return the alternate background
        static QColor htmlCode(QColor x) { return x.name(); } // return the alternate background
        static QString css();

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
#define CNUMOFCFGCOLORS       79

#define CPLOTBACKGROUND       0
#define CRIDEPLOTBACKGROUND   1
#define CTRAINPLOTBACKGROUND  2
#define CPLOTSYMBOL           3
#define CRIDEPLOTXAXIS        4
#define CRIDEPLOTYAXIS        5
#define CPLOTTHUMBNAIL        6
#define CPLOTTITLE            7
#define CPLOTSELECT           8
#define CPLOTTRACKER          9
#define CPLOTMARKER           10
#define CPLOTGRID             11
#define CINTERVALHIGHLIGHTER  12
#define CHEARTRATE            13
#define CSPEED                14
#define CACCELERATION         15
#define CPOWER                16
#define CNPOWER               17
#define CXPOWER               18
#define CAPOWER               19
#define CCP                   20
#define CCADENCE              21
#define CALTITUDE             22
#define CALTITUDEBRUSH        23
#define CWINDSPEED            24
#define CTORQUE               25
#define CLOAD                 26
#define CTSS                  27
#define CSTS                  28
#define CLTS                  29
#define CSB                   30
#define CDAILYSTRESS          31
#define CBIKESCORE            32
#define CCALENDARTEXT         33
#define CZONE1                34
#define CZONE2                35
#define CZONE3                36
#define CZONE4                37
#define CZONE5                38
#define CZONE6                39
#define CZONE7                40
#define CZONE8                41
#define CZONE9                42
#define CZONE10               43
#define CHZONE1               44
#define CHZONE2               45
#define CHZONE3               46
#define CHZONE4               47
#define CHZONE5               48
#define CHZONE6               49
#define CHZONE7               50
#define CHZONE8               51
#define CHZONE9               52
#define CHZONE10              53
#define CAEROVE               54
#define CAEROEL               55
#define CCALCELL              56
#define CCALHEAD              57
#define CCALCURRENT           58
#define CCALACTUAL            59
#define CCALPLANNED           60
#define CCALTODAY             61
#define CPOPUP                62
#define CPOPUPTEXT            63
#define CTILEBAR              64
#define CTILEBARSELECT        65
#define CTOOLBAR              66
#define CRIDEGROUP            67
#define CSPINSCANLEFT         68
#define CSPINSCANRIGHT        69
#define CTEMP                 70
#define CDIAL                 71
#define CALTPOWER             72
#define CBALANCELEFT          73
#define CBALANCERIGHT         74
#define CWBAL                 75
#define CRIDECP               76
#define CATISS                77
#define CANTISS               78

#endif
