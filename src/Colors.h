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

#include <QString>
#include <QObject>
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
        static unsigned long fingerprint(const Colors*set);
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

    public:
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
        static QString css(bool ridesummary=true);
        static QPalette palette();
        static QString stylesheet();
        static void readConfig();
        static void setupColors();

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
        void configChanged(qint32);

    private:
        QMap<QString, QColor> workoutCodes;
        Context *context;
};


// shorthand
#define GColor(x) GCColor::getColor(x)

// Define how many cconfigurable metric colors are available
#define CNUMOFCFGCOLORS       94

#define CPLOTBACKGROUND       0
#define CRIDEPLOTBACKGROUND   1
#define CTRENDPLOTBACKGROUND  2
#define CTRAINPLOTBACKGROUND  3
#define CPLOTSYMBOL           4
#define CRIDEPLOTXAXIS        5
#define CRIDEPLOTYAXIS        6
#define CPLOTTHUMBNAIL        7
#define CPLOTTITLE            8
#define CPLOTSELECT           9
#define CPLOTTRACKER          10
#define CPLOTMARKER           11
#define CPLOTGRID             12
#define CINTERVALHIGHLIGHTER  13
#define CHEARTRATE            14
#define CSPEED                15
#define CACCELERATION         16
#define CPOWER                17
#define CNPOWER               18
#define CXPOWER               19
#define CAPOWER               20
#define CCP                   21
#define CCADENCE              22
#define CALTITUDE             23
#define CALTITUDEBRUSH        24
#define CWINDSPEED            25
#define CTORQUE               26
#define CSLOPE                27
#define CGEAR                 28
#define CRV                   29
#define CRCAD                 30
#define CRGCT                 31
#define CSMO2                 32
#define CTHB                  33
#define CO2HB                 34
#define CHHB                  35
#define CLOAD                 36
#define CTSS                  37
#define CSTS                  38
#define CLTS                  39
#define CSB                   40
#define CDAILYSTRESS          41
#define CBIKESCORE            42
#define CCALENDARTEXT         43
#define CZONE1                44
#define CZONE2                45
#define CZONE3                46
#define CZONE4                47
#define CZONE5                48
#define CZONE6                49
#define CZONE7                50
#define CZONE8                51
#define CZONE9                52
#define CZONE10               53
#define CHZONE1               54
#define CHZONE2               55
#define CHZONE3               56
#define CHZONE4               57
#define CHZONE5               58
#define CHZONE6               59
#define CHZONE7               60
#define CHZONE8               61
#define CHZONE9               62
#define CHZONE10              63
#define CAEROVE               64
#define CAEROEL               65
#define CCALCELL              66
#define CCALHEAD              67
#define CCALCURRENT           68
#define CCALACTUAL            69
#define CCALPLANNED           70
#define CCALTODAY             71
#define CPOPUP                72
#define CPOPUPTEXT            73
#define CTILEBAR              74
#define CTILEBARSELECT        75
#define CTOOLBAR              76
#define CRIDEGROUP            77
#define CSPINSCANLEFT         78
#define CSPINSCANRIGHT        79
#define CTEMP                 80
#define CDIAL                 81
#define CALTPOWER             82
#define CBALANCELEFT          83
#define CBALANCERIGHT         84
#define CWBAL                 85
#define CRIDECP               86
#define CATISS                87
#define CANTISS               88
#define CLTE                  89
#define CRTE                  90
#define CLPS                  91
#define CRPS                  92
#define CCHROME               93

#endif
