/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
 * GCol & GThmCol - Copyright (c) 2022 Paul Johnson (paul49457@gmail.com)
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

#include <map>
#include <QString>
#include <QObject>
#include <QColor>
#include <QLabel>
#include <QGradient>
#include <QLinearGradient>

 // Define how many configurable metric colors are available
enum class GCol : uint8_t {
    PLOTBACKGROUND = 0,
    RIDEPLOTBACKGROUND,
    TRENDPLOTBACKGROUND,
    TRAINPLOTBACKGROUND,
    PLOTSYMBOL,
    RIDEPLOTXAXIS,
    RIDEPLOTYAXIS,
    PLOTTHUMBNAIL,
    PLOTTITLE,
    PLOTSELECT,
    PLOTTRACKER,
    PLOTMARKER,
    PLOTGRID,
    INTERVALHIGHLIGHTER,
    HEARTRATE,
    CORETEMP,
    SPEED,
    ACCELERATION,
    POWER,
    NPOWER,
    XPOWER,
    APOWER,
    TPOWER,
    CP,
    CADENCE,
    ALTITUDE,
    ALTITUDEBRUSH,
    WINDSPEED,
    TORQUE,
    SLOPE,
    GEAR,
    RV,
    RCAD,
    RGCT,
    SMO2,
    THB,
    O2HB,
    HHB,
    LOAD,
    TSS,
    STS,
    LTS,
    SB,
    DAILYSTRESS,
    BIKESCORE,
    CALENDARTEXT,
    ZONE1,
    ZONE2,
    ZONE3,
    ZONE4,
    ZONE5,
    ZONE6,
    ZONE7,
    ZONE8,
    ZONE9,
    ZONE10,
    HZONE1,
    HZONE2,
    HZONE3,
    HZONE4,
    HZONE5,
    HZONE6,
    HZONE7,
    HZONE8,
    HZONE9,
    HZONE10,
    AEROVE,
    AEROEL,
    CALCELL,
    CALHEAD,
    CALCURRENT,
    CALACTUAL,
    CALPLANNED,
    CALTODAY,
    POPUP,
    POPUPTEXT,
    TILEBAR ,
    TILEBARSELECT,
    TOOLBAR,
    RIDEGROUP,
    SPINSCANLEFT,
    SPINSCANRIGHT,
    TEMP,
    DIAL,
    ALTPOWER,
    BALANCELEFT,
    BALANCERIGHT,
    WBAL,
    RIDECP,
    ATISS,
    ANTISS,
    LTE,
    RTE,
    LPS,
    RPS,
    CHROME,
    OVERVIEWBACKGROUND,
    CARDBACKGROUND,
    VO2,
    VENTILATION,
    VCO2,
    TIDALVOLUME,
    RESPFREQUENCY,
    FEO2,
    HOVER,
    CHARTBAR,
    CARDBACKGROUND2,
    CARDBACKGROUND3,
    MAPROUTELINE,
    COLORRR,
    NUMOFCFGCOLORS
};

// Returned colors when functions are accessed with an out of range value
#define INVALIDGCOLORLUMINANCE  127
#define INVALIDGCOLOR           QColor(127, 127, 127)

// A selection of distinct colours, user can adjust also
extern QIcon colouredIconFromPNG(QString filename, QColor color);
extern QPixmap colouredPixmapFromPNG(QString filename, QColor color);

// dialog scaling
extern double dpiXFactor, dpiYFactor;
extern QFont baseFont;

// get the pixel size for a font that is no taller
// than height in pixels (let QT adapt for device ratios)
int pixelSizeForFont(QFont &font, int height);

// Color definitions within a theme
enum class GThmCol : uint8_t {
    PLOTBACKGROUND = 0,         // Plot Background
    TOOLBARANDSIDEBAR,          // Toolbar and Sidebar Chrome
    ACCENTCOLORMARKERS,         // Accent color (markers" >
    SELECTIONCOLOR,             // Selection color
    CPWBALRIDECP,               // Critical Power and W'Bal
    HEARTRATE,                  // Heartrate
    SPEED,                      // Speed
    POWER,                      // Power
    CADENCE,                    // Cadence
    TORQUE,                     // Toprque
    OVERVIEWBACKGROUND,         // Overview Background
    OVERVIEWTILEBACKGROUND1,    // Overview Tile Background
    OVERVIEWTILEBACKGROUND2,    // Overview Tile Background 2
    OVERVIEWTILEBACKGROUND3,    // Overview Tile Background 3
};

class ColorTheme
{
    public:
        ColorTheme() : name(""), dark(false), stealth(false) {};

        // all public
        QString name;
        bool dark;
        bool stealth;
        std::map<GThmCol, QColor> colors;
};

class Themes
{
    Q_DECLARE_TR_FUNCTIONS(Themes);

    public:

        QList<ColorTheme> themes;

        Themes(Themes const&) = delete;
        Themes& operator=(Themes const&) = delete;

        static Themes* inst() {
            static Themes* s{ new Themes };
            return s;
        }

    private:
        Themes();
 };

class ColorLabel : public QLabel
{
    Q_OBJECT

    public:
        ColorLabel(ColorTheme theme) : theme(theme) {}

        void paintEvent(QPaintEvent *);

        ColorTheme theme;
};

class Colors {
public:
    static unsigned long fingerprint(const std::map<GCol, Colors>& colorTable);

    // all public
    const QString group;
    const QString name;
    const QString setting;
    const QColor  qDarkColor;
    const QColor  qLightColor;
    QColor  qColor;
};

class GCColor : public QObject
{
    Q_OBJECT

    public:
        GCColor(GCColor const&) = delete;
        GCColor& operator=(GCColor const&) = delete;

        static GCColor* inst() {
            static GCColor* s{ new GCColor };
            return s;
        }

        // gColor and qColor functions
        QColor getQColor(const GCol& gColor);
        bool setQColor(const GCol& gColor, const QColor& qColor);
        double luminance(const GCol& gColor); // return the relative luminance
        double luminance(const QColor& qColor); // return the relative luminance
        QColor selectedQColor(const QColor& qColor);
        QColor invertQColor(const GCol& gColor); // return the contrasting color
        QColor invertQColor(const QColor& qColor); // return the contrasting color
        QColor alternateQColor(const GCol& gColor); // return the alternate background
        QColor alternateQColor(const QColor& qColor); // return the alternate background

        // functions to convert between gColors -> rgb colors -> qColors
        // An RGB color has red & green = 1, and blue is set to a GCol enumerated value
        QColor gColorToRGBColor(const GCol& gColor);
        bool isRGBColor(const QColor& qc);
        QColor rgbColorToQColor(const QColor& qColor);

        QColor determineThemeQColor(int index, const GCol& gColor);

        void resetColors();
        std::map<GCol, Colors>& getColorTable();

        void applyTheme(int index);

        // for styling things with current preferences
        QLinearGradient linearGradient(int size);
        QString css(bool ridesummary = true);
        QPalette palette();
        QString stylesheet(bool train = false);
        QColor standardQColor(int index);

        void readConfig();
        void dumpColors();

        // for upgrade/migration of Config
        QStringList getConfigKeys();

    private:
        GCColor();

        // Number of configurable metric colors
        std::map<GCol, Colors> colorTable;

        QList<QColor> standardColors;
};

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

// shorthand macros due to number of instances

#define GStandardQColor(x) (GCColor::inst()->standardQColor(x))

// gColor and qColor functions
#define GColor(x) (GCColor::inst()->getQColor(x))
#define GAlternateColor(x) (GCColor::inst()->alternateQColor(x))
#define GLuminance(x) (GCColor::inst()->luminance(x))
#define GInvertColor(x) (GCColor::inst()->invertQColor(x))
#define GSelectedColor(x) (GCColor::inst()->selectedQColor(x))

// functions to convert between gColors -> rgb colors -> qColors
// An RGB color has red & green = 1, and blue is set to a GCol enumerated value
#define GColorToRGBColor(x) (GCColor::inst()->gColorToRGBColor(x))
#define GIsRGBColor(x) (GCColor::inst()->isRGBColor(x))
#define GRGBColorToQColor(x) (GCColor::inst()->rgbColorToQColor(x))

#endif
