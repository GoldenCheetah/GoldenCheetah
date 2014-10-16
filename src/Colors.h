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
#include <QLabel>

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
        calendarFont;

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

class ColorTheme
{
    public:
        ColorTheme(QString name, QList<QColor>colors) : name(name), colors(colors) {}

        // all public
        QString name;
        QList<QColor> colors;
};

class Themes
{

    Q_DECLARE_TR_FUNCTIONS(Themes);

    public:
        Themes(); // will init the array of themes

        QList<ColorTheme> themes;
};

class ColorLabel : public QLabel
{
    Q_OBJECT

    public:
        ColorLabel(ColorTheme theme) : theme(theme) {}

        void paintEvent(QPaintEvent *);

        ColorTheme theme;
};

class GCColor : public QObject
{
    Q_OBJECT
    G_OBJECT

        void setupColors();
    public:
        GCColor(Context *);
        static QColor getColor(int);
        static void setColor(int,QColor);
        static const Colors *colorSet();
        static const Colors *defaultColorSet();
        static void resetColors();
        static struct SizeSettings defaultSizes(int width, int height);
        static double luminance(QColor color); // return the relative luminance
        static QColor invertColor(QColor); // return the contrasting color
        static QColor alternateColor(QColor); // return the alternate background
        static QColor htmlCode(QColor x) { return x.name(); } // return the alternate background
        static Themes &themes(); 
        static void applyTheme(int index);

        // for styling things with current preferences
        static bool isFlat();
        static QLinearGradient linearGradient(int size, bool active, bool alternate=false);
        static QString css();
        static QPalette palette();
        static QString stylesheet();

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
        QColor defaultColor, reverseColor;

    public slots:
        void configUpdate();

    private:
        QMap<QString, QColor> workoutCodes;
        Context *context;
};


// shorthand
#define GColor(x) GCColor::getColor(x)

// Define how many cconfigurable metric colors are available
#define CNUMOFCFGCOLORS       91

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
#define CSLOPE                26
#define CGEAR                 27
#define CRV                   28
#define CRCAD                 29
#define CRGCT                 30
#define CSMO2                 31
#define CTHB                  32
#define CLOAD                 33
#define CTSS                  34
#define CSTS                  35
#define CLTS                  36
#define CSB                   37
#define CDAILYSTRESS          38
#define CBIKESCORE            39
#define CCALENDARTEXT         40
#define CZONE1                41
#define CZONE2                42
#define CZONE3                43
#define CZONE4                44
#define CZONE5                45
#define CZONE6                46
#define CZONE7                47
#define CZONE8                48
#define CZONE9                49
#define CZONE10               50
#define CHZONE1               51
#define CHZONE2               52
#define CHZONE3               53
#define CHZONE4               54
#define CHZONE5               55
#define CHZONE6               56
#define CHZONE7               57
#define CHZONE8               58
#define CHZONE9               59
#define CHZONE10              60
#define CAEROVE               61
#define CAEROEL               62
#define CCALCELL              63
#define CCALHEAD              64
#define CCALCURRENT           65
#define CCALACTUAL            66
#define CCALPLANNED           67
#define CCALTODAY             68
#define CPOPUP                69
#define CPOPUPTEXT            70
#define CTILEBAR              71
#define CTILEBARSELECT        72
#define CTOOLBAR              73
#define CRIDEGROUP            74
#define CSPINSCANLEFT         75
#define CSPINSCANRIGHT        76
#define CTEMP                 77
#define CDIAL                 78
#define CALTPOWER             79
#define CBALANCELEFT          80
#define CBALANCERIGHT         81
#define CWBAL                 82
#define CRIDECP               83
#define CATISS                84
#define CANTISS               85
#define CLTE                  86
#define CRTE                  87
#define CLPS                  88
#define CRPS                  89
#define CCHROME               90

#endif
