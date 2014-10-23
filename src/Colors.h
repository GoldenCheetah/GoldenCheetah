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
#define CNUMOFCFGCOLORS       93

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
#define CO2HB                 33
#define CHHB                  34
#define CLOAD                 35
#define CTSS                  36
#define CSTS                  37
#define CLTS                  38
#define CSB                   39
#define CDAILYSTRESS          40
#define CBIKESCORE            41
#define CCALENDARTEXT         42
#define CZONE1                43
#define CZONE2                44
#define CZONE3                45
#define CZONE4                46
#define CZONE5                47
#define CZONE6                48
#define CZONE7                49
#define CZONE8                50
#define CZONE9                51
#define CZONE10               52
#define CHZONE1               53
#define CHZONE2               54
#define CHZONE3               55
#define CHZONE4               56
#define CHZONE5               57
#define CHZONE6               58
#define CHZONE7               59
#define CHZONE8               60
#define CHZONE9               61
#define CHZONE10              62
#define CAEROVE               63
#define CAEROEL               64
#define CCALCELL              65
#define CCALHEAD              66
#define CCALCURRENT           67
#define CCALACTUAL            68
#define CCALPLANNED           69
#define CCALTODAY             70
#define CPOPUP                71
#define CPOPUPTEXT            72
#define CTILEBAR              73
#define CTILEBARSELECT        74
#define CTOOLBAR              75
#define CRIDEGROUP            76
#define CSPINSCANLEFT         77
#define CSPINSCANRIGHT        78
#define CTEMP                 79
#define CDIAL                 80
#define CALTPOWER             81
#define CBALANCELEFT          82
#define CBALANCERIGHT         83
#define CWBAL                 84
#define CRIDECP               85
#define CATISS                86
#define CANTISS               87
#define CLTE                  88
#define CRTE                  89
#define CLPS                  90
#define CRPS                  91
#define CCHROME               92

#endif
