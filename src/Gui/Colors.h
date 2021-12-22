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
#include <QGradient>
#include <QLinearGradient>


// A selection of distinct colours, user can adjust also
extern QList<QColor> standardColors;
extern QIcon colouredIconFromPNG(QString filename, QColor color);
extern QPixmap colouredPixmapFromPNG(QString filename, QColor color);

// dialog scaling
extern double dpiXFactor, dpiYFactor;
extern QFont baseFont;

// turn color to rgb, checks if a named color
#define StandardColor(x) (QColor(1,1,x))
#define NamedColor(x) (x.red()==1 && x.green()==1)
#define RGBColor(x) (NamedColor(x) ? GColor(x.blue()) : x)

// get the pixel size for a font that is no taller
// than height in pixels (let QT adapt for device ratios)
int pixelSizeForFont(QFont &font, int height);

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
        QString group,
                name,
                setting;
        QColor  color;
};

class ColorTheme
{
    public:
        ColorTheme(QString name, bool dark, QList<QColor>colors) : name(name), dark(dark), colors(colors) {}

        // all public
        QString name;
        bool dark;
        bool stealth;
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
        static const Colors *defaultColorSet(bool dark);
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
        static QString stylesheet(bool train=false);
        static void readConfig();
        static void setupColors();
        static void dumpColors();

        // for upgrade/migration of Config
        static QStringList getConfigKeys();

};

// color chooser that also supports the standard colors (CPLOTMARKER, CPOWER)
// and returns them as a QColor(1,1,1,<int>) where <int> is the color number
// .e.g CPOWER is 18, see below for full list
#if 0
class GColorDialog : public QDialog
{
    GColorDialog(QWidget *parent);

};
#endif

// return a color for a ride file
class GlobalContext;
class ColorEngine : public QObject
{
    Q_OBJECT
    G_OBJECT

    public:
        ColorEngine(GlobalContext *);

        QColor colorFor(QString);
        QColor defaultColor, reverseColor;

    public slots:
        void configChanged(qint32);

    private:
        QMap<QString, QColor> workoutCodes;
        GlobalContext *gc; // bootstrapping
};


// shorthand
#define GColor(x) GCColor::getColor(x)

// Define how many cconfigurable metric colors are available
#define CNUMOFCFGCOLORS       108

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
#define CTPOWER               22
#define CCP                   23
#define CCADENCE              24
#define CALTITUDE             25
#define CALTITUDEBRUSH        26
#define CWINDSPEED            27
#define CTORQUE               28
#define CSLOPE                29
#define CGEAR                 30
#define CRV                   31
#define CRCAD                 32
#define CRGCT                 33
#define CSMO2                 34
#define CTHB                  35
#define CO2HB                 36
#define CHHB                  37
#define CLOAD                 38
#define CTSS                  39
#define CSTS                  40
#define CLTS                  41
#define CSB                   42
#define CDAILYSTRESS          43
#define CBIKESCORE            44
#define CCALENDARTEXT         45
#define CZONE1                46
#define CZONE2                47
#define CZONE3                48
#define CZONE4                49
#define CZONE5                50
#define CZONE6                51
#define CZONE7                52
#define CZONE8                53
#define CZONE9                54
#define CZONE10               55
#define CHZONE1               56
#define CHZONE2               57
#define CHZONE3               58
#define CHZONE4               59
#define CHZONE5               60
#define CHZONE6               61
#define CHZONE7               62
#define CHZONE8               63
#define CHZONE9               64
#define CHZONE10              65
#define CAEROVE               66
#define CAEROEL               67
#define CCALCELL              68
#define CCALHEAD              69
#define CCALCURRENT           70
#define CCALACTUAL            71
#define CCALPLANNED           72
#define CCALTODAY             73
#define CPOPUP                74
#define CPOPUPTEXT            75
#define CTILEBAR              76
#define CTILEBARSELECT        77
#define CTOOLBAR              78
#define CRIDEGROUP            79
#define CSPINSCANLEFT         80
#define CSPINSCANRIGHT        81
#define CTEMP                 82
#define CDIAL                 83
#define CALTPOWER             84
#define CBALANCELEFT          85
#define CBALANCERIGHT         86
#define CWBAL                 87
#define CRIDECP               88
#define CATISS                89
#define CANTISS               90
#define CLTE                  91
#define CRTE                  92
#define CLPS                  93
#define CRPS                  94
#define CCHROME               95
#define COVERVIEWBACKGROUND   96
#define CCARDBACKGROUND       97
#define CVO2                  98
#define CVENTILATION          99
#define CVCO2                 100
#define CTIDALVOLUME          101
#define CRESPFREQUENCY        102
#define CFEO2                 103
#define CHOVER                104
#define CCHARTBAR             105
#define CCARDBACKGROUND2      106
#define CCARDBACKGROUND3      107
#endif
