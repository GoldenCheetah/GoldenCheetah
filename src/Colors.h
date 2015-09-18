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
extern float GCDPIScale; // font scaling for display
extern QColor standardColor(int num);

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

        // for upgrade/migration of Config
        static QStringList getConfigKeys();

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
#define CNUMOFCFGCOLORS       95

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
#define CCORETEMP             15
#define CSPEED                16
#define CACCELERATION         17
#define CPOWER                18
#define CNPOWER               19
#define CXPOWER               20
#define CAPOWER               21
#define CCP                   22
#define CCADENCE              23
#define CALTITUDE             24
#define CALTITUDEBRUSH        25
#define CWINDSPEED            26
#define CTORQUE               27
#define CSLOPE                28
#define CGEAR                 29
#define CRV                   30
#define CRCAD                 31
#define CRGCT                 32
#define CSMO2                 33
#define CTHB                  34
#define CO2HB                 35
#define CHHB                  36
#define CLOAD                 37
#define CTSS                  38
#define CSTS                  39
#define CLTS                  40
#define CSB                   41
#define CDAILYSTRESS          42
#define CBIKESCORE            43
#define CCALENDARTEXT         44
#define CZONE1                45
#define CZONE2                46
#define CZONE3                47
#define CZONE4                48
#define CZONE5                49
#define CZONE6                50
#define CZONE7                51
#define CZONE8                52
#define CZONE9                53
#define CZONE10               54
#define CHZONE1               55
#define CHZONE2               56
#define CHZONE3               57
#define CHZONE4               58
#define CHZONE5               59
#define CHZONE6               60
#define CHZONE7               61
#define CHZONE8               62
#define CHZONE9               63
#define CHZONE10              64
#define CAEROVE               65
#define CAEROEL               66
#define CCALCELL              67
#define CCALHEAD              68
#define CCALCURRENT           69
#define CCALACTUAL            70
#define CCALPLANNED           71
#define CCALTODAY             72
#define CPOPUP                73
#define CPOPUPTEXT            74
#define CTILEBAR              75
#define CTILEBARSELECT        76
#define CTOOLBAR              77
#define CRIDEGROUP            78
#define CSPINSCANLEFT         79
#define CSPINSCANRIGHT        80
#define CTEMP                 81
#define CDIAL                 82
#define CALTPOWER             83
#define CBALANCELEFT          84
#define CBALANCERIGHT         85
#define CWBAL                 86
#define CRIDECP               87
#define CATISS                88
#define CANTISS               89
#define CLTE                  90
#define CRTE                  91
#define CLPS                  92
#define CRPS                  93
#define CCHROME               94

#endif
