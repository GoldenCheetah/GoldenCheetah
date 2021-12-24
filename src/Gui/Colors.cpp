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

#include "Colors.h"
#include "Context.h"
#include "Context.h"
#include "Athlete.h"
#include "RideMetadata.h"
#include <QObject>
#include <QByteArray>
#include <QDir>
#include "Settings.h"

#ifdef Q_OS_WIN
#include <windows.h>
#ifdef GC_HAVE_DWM
#include "dwmapi.h"
#endif
#endif

//
// Dialog scaling
//
double dpiXFactor, dpiYFactor;  // scale factor to apply to display
QFont baseFont;                 // base font scaled to display (before user scaling applied)

// find the right pixelSize for font and height
int pixelSizeForFont(QFont &font, int height)
{
    static QMap<int,int> maps; // cache as expensive to calculate

    int pixelsize = maps.value(height, 0);
    if (pixelsize) return pixelsize;

    QFont with = font;
    pixelsize=6;
    do {
        with.setPixelSize(pixelsize+1);
        QFontMetrics fm(with);
        if (fm.tightBoundingRect("Fy").height() > height) {
            maps.insert(height, pixelsize);
            return pixelsize;
        }
        else pixelsize++;

    } while (pixelsize<200); // should never loop that much

    // should never get here, unless its a huge font needed!
    return pixelsize;
}

//
// A selection of distinct colours, user can adjust also
//
QList<QColor> standardColors;
static bool initStandardColors()
{
    standardColors << QColor(Qt::magenta);
    standardColors << QColor(Qt::cyan);
    standardColors << QColor(Qt::yellow);
    standardColors << QColor(Qt::red);
    standardColors << QColor(Qt::blue);
    standardColors << QColor(Qt::gray);
    standardColors << QColor(Qt::darkCyan);
    standardColors << QColor(Qt::green);
    standardColors << QColor(Qt::darkRed);
    standardColors << QColor(Qt::darkGreen);
    standardColors << QColor(Qt::darkBlue);
    standardColors << QColor(Qt::darkMagenta);

    return true;
}
static bool init = initStandardColors();

// the standard themes, a global object
static Themes allThemes;

// Number of configurable metric colors + 1 for sentinel value
static Colors ColorList[CNUMOFCFGCOLORS+1],
              LightDefaultColorList[CNUMOFCFGCOLORS+1],
              DarkDefaultColorList[CNUMOFCFGCOLORS+1];

static void copyArray(Colors source[], Colors target[])
{
    for (int i=0; source[i].name != ""; i++)
        target[i] = source[i];
}

unsigned long Colors::fingerprint(const Colors *set)
{
    QByteArray ba;
    while(set->name != "") {
        ba.append(set->color.name());
        set++;
    }
    return qChecksum(ba, ba.length());
}

#ifdef Q_OS_WIN
// handle dpi scaling on windows
static float windowsDpiScale()
{
    HDC screen = GetDC( 0 );
    FLOAT dpiX = static_cast<FLOAT>( GetDeviceCaps( screen, LOGPIXELSX ) );
    ReleaseDC( 0, screen );
    return dpiX / 96.0f;
}

// global
float GCDPIScale = windowsDpiScale();
#else
float GCDPIScale = 1.0f;
#endif

// initialization called from constructor to enable translation
void GCColor::setupColors()
{
    // consider removing when we can guarantee extended initialisation support in gcc
    // (c++0x not supported by Qt currently and not planned for 4.8 or 5.0)
    Colors init[CNUMOFCFGCOLORS+1] = {
        { tr("Chart"), tr("Plot Background"), "COLORPLOTBACKGROUND", QColor(52,52,52) },
        { tr("Chart"), tr("Performance Plot Background"), "COLORRIDEPLOTBACKGROUND", QColor(52,52,52) },
        { tr("Chart"), tr("Trend Plot Background"), "COLORTRENDPLOTBACKGROUND", Qt::black },
        { tr("Chart"), tr("Train Plot Background"), "COLORTRAINPLOTBACKGROUND", Qt::black },
        { tr("Chart"), tr("Plot Symbols"), "COLORRIDEPLOTSYMBOLS", Qt::cyan },
        { tr("Chart"), tr("Performance Plot X Axis"), "COLORRIDEPLOTXAXIS", Qt::blue },
        { tr("Chart"), tr("Performance Plot Y Axis"), "COLORRIDEPLOTYAXIS", Qt::red },
        { tr("Chart"), tr("Plot Thumbnail Background"), "COLORPLOTTHUMBNAIL", Qt::gray },
        { tr("Chart"), tr("Plot Title"), "COLORPLOTTITLE", Qt::gray },
        { tr("Chart"), tr("Plot Selection Pen"), "COLORPLOTSELECT", Qt::blue },
        { tr("Chart"), tr("Plot TrackerPen"), "COLORPLOTTRACKER", Qt::blue },
        { tr("Chart"), tr("Plot Markers"), "COLORPLOTMARKER", Qt::cyan },
        { tr("Chart"), tr("Plot Grid"), "COLORGRID", QColor(65,65,65) },
        { tr("Chart"), tr("Interval Highlighter"), "COLORINTERVALHIGHLIGHTER", Qt::blue },
        { tr("Data"), tr("Heart Rate"), "COLORHEARTRATE", Qt::red },
        { tr("Data"), tr("Core Temperature"), "COLORCORETEMP", QColor(255, 173, 92) },
        { tr("Data"), tr("Speed"), "COLORSPEED", Qt::green },
        { tr("Data"), tr("Acceleration"), "COLORACCEL", Qt::cyan },
        { tr("Data"), tr("Power"), "COLORPOWER", Qt::yellow },
        { tr("Data"), tr("Iso Power"), "CNPOWER", Qt::magenta },
        { tr("Data"), tr("Skiba xPower"), "CXPOWER", Qt::magenta },
        { tr("Data"), tr("Altitude Power"), "CAPOWER", Qt::magenta },
        { tr("Data"), tr("Train Target Power"), "CTPOWER", Qt::blue },
        { tr("Data"), tr("Critical Power"), "COLORCP", Qt::cyan },
        { tr("Data"), tr("Cadence"), "COLORCADENCE", QColor(0,204,204) },
        { tr("Data"), tr("Altitude"), "COLORALTITUTDE", QColor(Qt::gray) },
        { tr("Data"), tr("Altitude Shading"), "COLORALTITUDESHADE", QColor(Qt::lightGray) },
        { tr("Data"), tr("Wind Speed"), "COLORWINDSPEED", Qt::darkGreen },
        { tr("Data"), tr("Torque"), "COLORTORQUE", Qt::magenta },
        { tr("Data"), tr("Slope"), "CSLOPE", Qt::green },
        { tr("Data"), tr("Gear Ratio"), "COLORGEAR", QColor(0xff, 0x90, 0x00) },
        { tr("Data"), tr("Run Vertical Oscillation"), "COLORRVERT", QColor(0xff, 0x90, 0x00) }, // same as garmin connect colors
        { tr("Data"), tr("Run Cadence"), "COLORRCAD", QColor(0xff, 0x90, 0x00) }, // same as garmin connect colors
        { tr("Data"), tr("Run Ground Contact"), "COLORGCT", QColor(0xff, 0x90, 0x00) }, // same as garmin connect colors
        { tr("Data"), tr("Muscle Oxygen (SmO2)"), "COLORSMO2", QColor(0x00, 0x89, 0x77) }, // green same as moxy monitor
        { tr("Data"), tr("Haemoglobin Mass (tHb)"), "COLORTHB", QColor(0xa3,0x44,0x02) },  // brown same as moxy monitor
        { tr("Data"), tr("Oxygenated Haemoglobin (O2Hb)"), "CO2HB", QColor(0xd1,0x05,0x72) },
        { tr("Data"), tr("Deoxygenated Haemoglobin (HHb)"), "CHHB", QColor(0x00,0x7f,0xcc) },
        { tr("Data"), tr("Load"), "COLORLOAD", Qt::yellow },
        { tr("Data"), tr("BikeStress"), "COLORTSS", Qt::green },
        { tr("Data"), tr("Short Term Stress"), "COLORSTS", Qt::blue },
        { tr("Data"), tr("Long Term Stress"), "COLORLTS", Qt::green },
        { tr("Data"), tr("Stress Balance"), "COLORSB", QColor(180,140,140) },
        { tr("Data"), tr("Daily Stress"), "COLORDAILYSTRESS", Qt::red },
        { tr("Data"), "Bike Score (TM)", "COLORBIKESCORE", Qt::gray },
        { tr("Gui"), tr("Calendar Text"), "COLORCALENDARTEXT", Qt::gray },
        { tr("Data"), tr("Power Zone 1 Shading"), "COLORZONE1", QColor(255,0,255) },
        { tr("Data"), tr("Power Zone 2 Shading"), "COLORZONE2", QColor(42,0,255) },
        { tr("Data"), tr("Power Zone 3 Shading"), "COLORZONE3", QColor(0,170,255) },
        { tr("Data"), tr("Power Zone 4 Shading"), "COLORZONE4", QColor(0,255,128) },
        { tr("Data"), tr("Power Zone 5 Shading"), "COLORZONE5", QColor(85,255,0) },
        { tr("Data"), tr("Power Zone 6 Shading"), "COLORZONE6", QColor(255,213,0) },
        { tr("Data"), tr("Power Zone 7 Shading"), "COLORZONE7", QColor(255,0,0) },
        { tr("Data"), tr("Power Zone 8 Shading"), "COLORZONE8", Qt::gray },
        { tr("Data"), tr("Power Zone 9 Shading"), "COLORZONE9", Qt::gray },
        { tr("Data"), tr("Power Zone 10 Shading"), "COLORZONE10", Qt::gray },
        { tr("Data"), tr("HR Zone 1 Shading"), "HRCOLORZONE1", QColor(255,0,255) },
        { tr("Data"), tr("HR Zone 2 Shading"), "HRCOLORZONE2", QColor(42,0,255) },
        { tr("Data"), tr("HR Zone 3 Shading"), "HRCOLORZONE3", QColor(0,170,255) },
        { tr("Data"), tr("HR Zone 4 Shading"), "HRCOLORZONE4", QColor(0,255,128) },
        { tr("Data"), tr("HR Zone 5 Shading"), "HRCOLORZONE5", QColor(85,255,0) },
        { tr("Data"), tr("HR Zone 6 Shading"), "HRCOLORZONE6", QColor(255,213,0) },
        { tr("Data"), tr("HR Zone 7 Shading"), "HRCOLORZONE7", QColor(255,0,0) },
        { tr("Data"), tr("HR Zone 8 Shading"), "HRCOLORZONE8", Qt::gray },
        { tr("Data"), tr("HR Zone 9 Shading"), "HRCOLORZONE9", Qt::gray },
        { tr("Data"), tr("HR Zone 10 Shading"), "HRCOLORZONE10", Qt::gray },
        { tr("Data"), tr("Aerolab Vrtual Elevation"), "COLORAEROVE", Qt::blue },
        { tr("Data"), tr("Aerolab Elevation"), "COLORAEROEL", Qt::green },
        { tr("Gui"), tr("Calendar background"), "CCALCELL", Qt::white },
        { tr("Gui"), tr("Calendar heading"), "CCALHEAD", QColor(230,230,230) },
        { tr("Gui"), tr("Calendar Current Selection"), "CCALCURRENT", QColor(255,213,0) },
        { tr("Gui"), tr("Calendar Actual Workout"), "CCALACTUAL", Qt::green },
        { tr("Gui"), tr("Calendar Planned Workout"), "CCALPLANNED", Qt::yellow },
        { tr("Gui"), tr("Calendar Today"), "CCALTODAY", Qt::cyan },
        { tr("Gui"), tr("Pop Up Windows Background"), "CPOPUP", Qt::lightGray },
        { tr("Gui"), tr("Pop Up Windows Foreground"), "CPOPUPTEXT", Qt::white },
        { tr("Gui"), tr("Chart Bar Unselected"), "CTILEBAR", Qt::gray },
        { tr("Gui"), tr("Chart Bar Selected"), "CTILEBARSELECT", Qt::yellow },
        { tr("Gui"), tr("ToolBar Background"), "CTOOLBAR", Qt::white },
        { tr("Gui"), tr("Activity History Group"), "CRIDEGROUP", QColor(236,246,255) },
        { tr("Data"), tr("SpinScan Left"), "CSPINSCANLEFT", Qt::gray },
        { tr("Data"), tr("SpinScan Right"), "CSPINSCANRIGHT", Qt::cyan },
        { tr("Data"), tr("Temperature"), "COLORTEMPERATURE", Qt::yellow },
        { tr("Data"), tr("Default Dial Color"), "CDIAL", Qt::gray },
        { tr("Data"), tr("Alternate Power"), "CALTPOWER", Qt::magenta },
        { tr("Data"), tr("Left Balance"), "CBALANCELEFT", QColor(178,0,0) },
        { tr("Data"), tr("Right Balance"), "CBALANCERIGHT", QColor(128,0,50) },
        { tr("Data"), tr("W' Balance"), "CWBAL", Qt::red },
        { tr("Data"), tr("Mean-maximal Power"), "CRIDECP", Qt::red },
        { tr("Data"), tr("Aerobic TISS"), "CATISS", Qt::magenta },
        { tr("Data"), tr("Anaerobic TISS"), "CANTISS", Qt::cyan },
        { tr("Data"), tr("Left Torque Effectiveness"), "CLTE", Qt::cyan },
        { tr("Data"), tr("Right Torque Effectiveness"), "CRTE", Qt::magenta },
        { tr("Data"), tr("Left Pedal Smoothness"), "CLPS", Qt::cyan },
        { tr("Data"), tr("Right Pedal Smoothness"), "CRPS", Qt::magenta },
#ifdef GC_HAVE_DWM
        { tr("Gui"), tr("Toolbar and Sidebar"), "CCHROME", QColor(1,1,1) },
#else
#ifdef Q_OS_MAC
        { tr("Gui"), tr("Sidebar background"), "CCHROME", QColor(213,213,213) },
#else
        { tr("Gui"), tr("Sidebar background"), "CCHROME", QColor(0xec,0xec,0xec) },
#endif
#endif
        { tr("Gui"), tr("Overview Background"), "COVERVIEWBACKGROUND", QColor(0,0,0) },
        { tr("Gui"), tr("Overview Tile Background"), "CCARDBACKGROUND", QColor(52,52,52) },
        { tr("Data"), tr("VO2"), "CVO2", Qt::magenta },
        { tr("Data"), tr("Ventilation"), "CVENTILATION", Qt::cyan },
        { tr("Data"), tr("VCO2"), "CVCO2", Qt::green },
        { tr("Data"), tr("Tidal Volume"), "CTIDALVOLUME", Qt::yellow },
        { tr("Data"), tr("Respiratory Frequency"), "CRESPFREQUENCY", Qt::yellow },
        { tr("Data"), tr("FeO2"), "CFEO2", Qt::yellow },
        { tr("Gui"), tr("Toolbar Hover"), "CHOVER", Qt::lightGray },
        { tr("Gui"), tr("Chartbar background"), "CCHARTBAR", Qt::lightGray },
        { tr("Gui"), tr("Overview Tile Background Alternate"), "CCARDBACKGROUND2", QColor(0,0,0) },
        { tr("Gui"), tr("Overview Tile Background Vibrant"), "CCARDBACKGROUND3", QColor(52,52,52) },
        { "", "", "", QColor(0,0,0) },
    };

    // set the defaults to system defaults
    init[CCALCURRENT].color = QPalette().color(QPalette::Highlight);
    init[CTOOLBAR].color = QPalette().color(QPalette::Window);

#ifdef Q_OS_MAC
    // if on yosemite set default chrome to #e5e5e5
    if (QSysInfo::MacintoshVersion == 12) {
        init[CCHROME].color = QColor(0xe5,0xe5,0xe5);
        appsettings->setValue(GC_CHROME, "Flat");
    }
#endif
    copyArray(init, DarkDefaultColorList);
    copyArray(init, LightDefaultColorList);
    copyArray(init, ColorList);

    // lets update the Light colors to ones that are more
    // appropriate on a light background, since the init versions
    // were all selected on the basis of a dark background
    // note: we don't update them all so old standard charts
    // aren't affected to badly, but may do so in the future
    LightDefaultColorList[0].color = QColor(255,255,255); // 0:Plot Background
    LightDefaultColorList[1].color = QColor(255,255,255); // 1:Performance Plot Background
    LightDefaultColorList[2].color = QColor(255,255,255); // 2:Trend Plot Background
    LightDefaultColorList[3].color = QColor(255,255,255); // 3:Train Plot Background
    LightDefaultColorList[4].color = QColor(101,105,165); // 4:Plot Symbols
    LightDefaultColorList[5].color = QColor(101,105,165); // 5:Performance Plot X Axis
    LightDefaultColorList[6].color = QColor(101,105,165); // 6:Performance Plot Y Axis
    LightDefaultColorList[7].color = QColor(160,160,164); // 7:Plot Thumbnail Background
    LightDefaultColorList[8].color = QColor(0,0,0); // 8:Plot Title
    LightDefaultColorList[9].color = QColor(194,194,194); // 9:Plot Selection Pen
    LightDefaultColorList[10].color = QColor(194,194,194); // 10:Plot TrackerPen
    LightDefaultColorList[11].color = QColor(101,105,165); // 11:Plot Markers
    LightDefaultColorList[12].color = QColor(232,232,232); // 12:Plot Grid
    LightDefaultColorList[13].color = QColor(194,194,194); // 13:Interval Highlighter
    LightDefaultColorList[14].color = QColor(255,0,0); // 14:Heart Rate
    LightDefaultColorList[15].color = QColor(131,8,255); // 15:Core Temperature
    LightDefaultColorList[16].color = QColor(0,102,0); // 16:Speed
    LightDefaultColorList[17].color = QColor(0,146,146); // 17:Acceleration
    LightDefaultColorList[18].color = QColor(255,170,0); // 18:Power
    LightDefaultColorList[19].color = QColor(255,0,255); // 19:Iso Power
    LightDefaultColorList[20].color = QColor(158,0,158); // 20:Skiba xPower
    LightDefaultColorList[21].color = QColor(255,0,255); // 21:Altitude Power
    LightDefaultColorList[22].color = QColor(0,0,255); // 22:Train Target Power
    LightDefaultColorList[23].color = QColor(167,0,109); // 23:Critical Power
    LightDefaultColorList[24].color = QColor(0,126,126); // 24:Cadence
    LightDefaultColorList[25].color = QColor(119,119,122); // 25:Altitude
    LightDefaultColorList[26].color = QColor(114,114,114); // 26:Altitude Shading
    LightDefaultColorList[27].color = QColor(0,128,0); // 27:Wind Speed
    LightDefaultColorList[28].color = QColor(111,0,111); // 28:Torque
    LightDefaultColorList[29].color = QColor(0,127,0); // 29:Slope
    LightDefaultColorList[30].color = QColor(255,144,0); // 30:Gear Ratio
    LightDefaultColorList[31].color = QColor(255,144,0); // 31:Run Vertical Oscillation
    LightDefaultColorList[32].color = QColor(255,144,0); // 32:Run Cadence
    LightDefaultColorList[33].color = QColor(255,144,0); // 33:Run Ground Contact
    LightDefaultColorList[34].color = QColor(0,137,119); // 34:Muscle Oxygen (SmO2)
    LightDefaultColorList[35].color = QColor(163,68,2); // 35:Haemoglobin Mass (tHb)
    LightDefaultColorList[36].color = QColor(209,5,114); // 36:Oxygenated Haemoglobin (O2Hb)
    LightDefaultColorList[37].color = QColor(0,127,204); // 37:Deoxygenated Haemoglobin (HHb)
    LightDefaultColorList[38].color = QColor(127,127,0); // 38:Load
    LightDefaultColorList[39].color = QColor(0,81,0); // 39:BikeStress
    LightDefaultColorList[40].color = QColor(227,12,255); // 40:Short Term Stress
    LightDefaultColorList[41].color = QColor(16,0,195); // 41:Long Term Stress
    LightDefaultColorList[42].color = QColor(209,193,23); // 42:Stress Balance
    LightDefaultColorList[43].color = QColor(255,0,0); // 43:Daily Stress
    LightDefaultColorList[44].color = QColor(160,160,164); // 44:Bike Score (TM)
    LightDefaultColorList[45].color = QColor(0,0,0); // 45:Calendar Text
    LightDefaultColorList[46].color = QColor(255,0,255); // 46:Power Zone 1 Shading
    LightDefaultColorList[47].color = QColor(42,0,255); // 47:Power Zone 2 Shading
    LightDefaultColorList[48].color = QColor(0,170,255); // 48:Power Zone 3 Shading
    LightDefaultColorList[49].color = QColor(0,255,128); // 49:Power Zone 4 Shading
    LightDefaultColorList[50].color = QColor(85,255,0); // 50:Power Zone 5 Shading
    LightDefaultColorList[51].color = QColor(255,213,0); // 51:Power Zone 6 Shading
    LightDefaultColorList[52].color = QColor(255,0,0); // 52:Power Zone 7 Shading
    LightDefaultColorList[53].color = QColor(160,160,164); // 53:Power Zone 8 Shading
    LightDefaultColorList[54].color = QColor(160,160,164); // 54:Power Zone 9 Shading
    LightDefaultColorList[55].color = QColor(160,160,164); // 55:Power Zone 10 Shading
    LightDefaultColorList[56].color = QColor(255,0,255); // 56:HR Zone 1 Shading
    LightDefaultColorList[57].color = QColor(42,0,255); // 57:HR Zone 2 Shading
    LightDefaultColorList[58].color = QColor(0,170,255); // 58:HR Zone 3 Shading
    LightDefaultColorList[59].color = QColor(0,255,128); // 59:HR Zone 4 Shading
    LightDefaultColorList[60].color = QColor(85,255,0); // 60:HR Zone 5 Shading
    LightDefaultColorList[61].color = QColor(255,213,0); // 61:HR Zone 6 Shading
    LightDefaultColorList[62].color = QColor(255,0,0); // 62:HR Zone 7 Shading
    LightDefaultColorList[63].color = QColor(160,160,164); // 63:HR Zone 8 Shading
    LightDefaultColorList[64].color = QColor(160,160,164); // 64:HR Zone 9 Shading
    LightDefaultColorList[65].color = QColor(160,160,164); // 65:HR Zone 10 Shading
    LightDefaultColorList[66].color = QColor(0,0,255); // 66:Aerolab VE
    LightDefaultColorList[67].color = QColor(0,255,0); // 67:Aerolab Elevation
    LightDefaultColorList[68].color = QColor(255,255,255); // 68:Calendar background
    LightDefaultColorList[69].color = QColor(230,230,230); // 69:Calendar heading
    LightDefaultColorList[70].color = QColor(48,140,198); // 70:Calendar Current Selection
    LightDefaultColorList[71].color = QColor(0,255,0); // 71:Calendar Actual Workout
    LightDefaultColorList[72].color = QColor(255,177,21); // 72:Calendar Planned Workout
    LightDefaultColorList[73].color = QColor(0,255,255); // 73:Calendar Today
    LightDefaultColorList[74].color = QColor(255,255,255); // 74:Pop Up Windows Background
    LightDefaultColorList[75].color = QColor(119,119,119); // 75:Pop Up Windows Foreground
    LightDefaultColorList[76].color = QColor(160,160,164); // 76:Chart Bar Unselected
    LightDefaultColorList[77].color = QColor(255,255,0); // 77:Chart Bar Selected
    LightDefaultColorList[78].color = QColor(239,239,239); // 78:ToolBar Background
    LightDefaultColorList[79].color = QColor(236,246,255); // 79:Activity History Group
    LightDefaultColorList[80].color = QColor(0,164,101); // 80:SpinScan Left
    LightDefaultColorList[81].color = QColor(0,130,130); // 81:SpinScan Right
    LightDefaultColorList[82].color = QColor(0,107,188); // 82:Temperature
    LightDefaultColorList[83].color = QColor(160,160,164); // 83:Default Dial Color
    LightDefaultColorList[84].color = QColor(255,0,255); // 84:Alternate Power
    LightDefaultColorList[85].color = QColor(178,0,0); // 85:Left Balance
    LightDefaultColorList[86].color = QColor(128,0,50); // 86:Right Balance
    LightDefaultColorList[87].color = QColor(186,57,59); // 87:W' Balance
    LightDefaultColorList[88].color = QColor(255,85,255); // 88:CP Curve
    LightDefaultColorList[89].color = QColor(146,0,146); // 89:Aerobic TISS
    LightDefaultColorList[90].color = QColor(0,130,130); // 90:Anaerobic TISS
    LightDefaultColorList[91].color = QColor(0,137,137); // 91:Left Torque Effectiveness
    LightDefaultColorList[92].color = QColor(145,0,145); // 92:Right Torque Effectiveness
    LightDefaultColorList[93].color = QColor(0,146,146); // 93:Left Pedal Smoothness
    LightDefaultColorList[94].color = QColor(117,0,117); // 94:Right Pedal Smoothness
    LightDefaultColorList[95].color = QColor(54,55,75); // 95:Toolbar and Sidebar
    LightDefaultColorList[96].color = QColor(227,224,232); // 96:Overview Background
    LightDefaultColorList[97].color = QColor(255,255,255); // 97:Overview Tile Background
    LightDefaultColorList[98].color = QColor(255,25,167); // 98:VO2
    LightDefaultColorList[99].color = QColor(27,203,177); // 99:Ventilation
    LightDefaultColorList[100].color = QColor(0,121,0); // 100:VCO2
    LightDefaultColorList[101].color = QColor(101,44,45); // 101:Tidal Volume
    LightDefaultColorList[102].color = QColor(134,74,255); // 102:Respiratory Frequency
    LightDefaultColorList[103].color = QColor(255,46,46); // 103:FeO2
    LightDefaultColorList[106].color = QColor(180,180,180); // 106:Tile Alternate
    LightDefaultColorList[107].color = QColor(0xee,0xf8,0xff); // 107:Tile Vibrant
}

// default settings for fonts etc
// we err on the side of caution -- smaller is better
struct SizeSettings defaultAppearance[] ={

    // small screens include netbooks and old vga 800x600, 1024x768
    { 1024, 768,  8,8,6,6,6,    800, 600 },

    // medium screen size includes typical 16:9 pc formats and TV screens
    { 1280, 800,  8,8,6,6,6,    800, 600},

    // high resolution screens 
    { 1650, 1080,  10,10,8,8,8,   1024,650 },

    // very big panels, incl. e.g.  mac 27"
    { 9999, 9999,  10,10,8,8,8,   1280,700 },

    { 0,0,0,0,0,0,0,0,0 },
};

struct SizeSettings
GCColor::defaultSizes(int width, int height)
{
    for (int i=0; defaultAppearance[i].maxheight; i++) {

        if (height > defaultAppearance[i].maxheight && width > defaultAppearance[i].maxwidth)
            continue;

        else return defaultAppearance[i];

    }
    return defaultAppearance[0]; // shouldn't get here
}


// returns a luminance for a color from 0 (dark) to 255 (very light) 127 is a half way house gray
double GCColor::luminance(QColor color)
{
    QColor cRGB = color.convertTo(QColor::Rgb);

    // based upon http://en.wikipedia.org/wiki/Luminance_(relative)
    return (0.2126f * double(cRGB.red()))  + 
           (0.7152f * double(cRGB.green())) +
           (0.0722f * double(cRGB.blue()));
}

QColor GCColor::invertColor(QColor bgColor)
{
    if (bgColor==QColor(Qt::darkGray)) return QColor(Qt::white); // darkGray is popular!
    return GCColor::luminance(bgColor) < 127 ? QColor(Qt::white) : QColor(Qt::black);
}

QColor GCColor::alternateColor(QColor bgColor)
{
    //QColor color = QColor::fromHsv(bgColor.hue(), bgColor.saturation() * 2, bgColor.value());
    if (bgColor.value() < 128)
        return QColor(75,75,75); // avoid darkGray as clashes with text cursor on hidpi displays (!)
    else
        return QColor(Qt::lightGray);
}

const Colors * GCColor::colorSet()
{
    return ColorList;
}

const Colors * GCColor::defaultColorSet(bool dark)
{
    if (dark) return DarkDefaultColorList;
    else return LightDefaultColorList;
}

void GCColor::resetColors()
{
    copyArray(DarkDefaultColorList, ColorList);
}

void
GCColor::readConfig()
{
    // read in config settings and populate the color table
    for (unsigned int i=0; ColorList[i].name != ""; i++) {
        QString colortext = appsettings->value(NULL, ColorList[i].setting, "").toString();
        if (colortext != "") {
            // color definitions are stored as "r:g:b"
            QStringList rgb = colortext.split(":");
            ColorList[i].color = QColor(rgb[0].toInt(),
                                        rgb[1].toInt(),
                                        rgb[2].toInt());
        } else {

            // set sensible defaults for any not set (as new colors are added)
            if (ColorList[i].name == "CTOOLBAR") {
                QPalette def;
                ColorList[i].color = def.color(QPalette::Window);
            }
            if (ColorList[i].name == "CCHARTBAR") {
                ColorList[i].color = ColorList[CTOOLBAR].color;
            }
            if (ColorList[i].name == "CCALCURRENT") {
                QPalette def;
                ColorList[i].color = def.color(QPalette::Highlight);

            }
        }
    }
#ifdef Q_OS_MAC
    // if on yosemite set default chrome to #e5e5e5
    if (QSysInfo::MacintoshVersion == 12) {
        ColorList[CCHROME].color = QColor(0xe5,0xe5,0xe5);
    }
#endif
}

QColor
GCColor::getColor(int colornum)
{
    return ColorList[colornum].color;
}

void
GCColor::setColor(int colornum, QColor color)
{
    ColorList[colornum].color = color;
}

Themes &
GCColor::themes()
{
    return allThemes;
}

ColorEngine::ColorEngine(GlobalContext *gc) : defaultColor(QColor(Qt::white)), gc(gc)
{
    configChanged(CONFIG_NOTECOLOR);
    connect(gc, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
}

void ColorEngine::configChanged(qint32)
{
    // clear existing
    workoutCodes.clear();

    // reverse
    reverseColor = GColor(CPLOTBACKGROUND);

    // setup the keyword/color combinations from config settings
    foreach (KeywordDefinition keyword, gc->rideMetadata->getKeywords()) {
        if (keyword.name == "Default")
            defaultColor = keyword.color; // we actually ignore this now
        else if (keyword.name == "Reverse")
            reverseColor = keyword.color;  // to set the foreground when use as background is set
        else {
            workoutCodes[keyword.name] = keyword.color;

            // alternative texts in notes
            foreach (QString token, keyword.tokens) {
                workoutCodes[token] = keyword.color;
            }
        }
    }
}

QColor
ColorEngine::colorFor(QString text)
{
    QColor color = QColor(1,1,1,1); // the default color has an alpha of 1, not possible otherwise

    foreach(QString code, workoutCodes.keys()) {
        if (text.contains(code, Qt::CaseInsensitive)) {
           color = workoutCodes[code];
        }
    }
    return color;
}

QString
GCColor::css(bool ridesummary)
{
    QColor bgColor = ridesummary ? GColor(CPLOTBACKGROUND) : GColor(CTRENDPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);
    //QColor altColor = GCColor::alternateColor(bgColor); // not yet ?

    return QString("<style> "
                   "html { overflow: auto; }"
                   "body { position: absolute; "
                   "       top: %4px; left: %4px; bottom: %4px; right: %4px; padding: 0px; "
                   "       overflow-y: hidden; overflow-x: hidden; color: %3; background-color: %2; }"
                   "body:hover { overflow-y: scroll; }"
                   "h1 { color: %1; background-color: %2; } "
                   "h2 { color: %1; background-color: %2; } "
                   "h3 { color: %1; background-color: %2; } "
                   "h4 { color: %1; background-color: %2; } "
                   "b { color: %1; background-color: %2; } "
                   "#sharp { color: %1; background-color: darkGray; font-weight: bold; } "
	           ".tooltip { position: relative; display: inline-block; } "
		   ".tooltip .tooltiptext { visibility: hidden; background-color: %2; color: %1; text-align: center; padding: %4px 0; border-radius: %5px; position: absolute; z-index: 1; width: %6px; margin-left: -%7px; top: 100%; left: 50%; opacity: 0; transition: opacity 0.3s; } "
		   ".tooltip:hover .tooltiptext { visibility: visible; opacity: 1; } "
#ifdef Q_OS_MAC
                   "::-webkit-scrollbar-thumb { border-radius: 4px; background: rgba(0,0,0,0.5);  "
                   "-webkit-box-shadow: inset 0 0 1px rgba(255,255,255,0.6); }"
                   "::-webkit-scrollbar { width: 9; background: %2; } "
#else
                   "::-webkit-scrollbar-thumb { background: darkGray; } "
                   "::-webkit-scrollbar-thumb:hover { background: lightGray; } "
                   "::-webkit-scrollbar { width: %5px; background: %2; } "
#endif
                   "</style> ").arg(GColor(CPLOTMARKER).name())
                               .arg(bgColor.name())
                               .arg(fgColor.name())
                               .arg(5 * dpiXFactor)
                               .arg(6 * dpiXFactor)
                               .arg(200 * dpiXFactor)
                               .arg(100 * dpiXFactor);
}
QPalette 
GCColor::palette()
{
    // make it to order, to reflect current config
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setBrush(QPalette::Background, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setBrush(QPalette::Base, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Normal, QPalette::Window, GCColor::invertColor(GColor(CPLOTBACKGROUND)));

    return palette;
}

QString 
GCColor::stylesheet(bool train)
{
    // make it to order to reflect current config
    QColor bgColor = train ? GColor(CTRAINPLOTBACKGROUND) : GColor(CPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);
    return QString("QTreeView { color: %2; background: %1; }"
                   "QTableWidget { color: %2; background: %1; }"
#ifndef Q_OS_MAC
                   "QHeaderView { background-color: %1; color: %2; }"
                   "QHeaderView::section { background-color: %1; color: %2; border: 0px ; }"
#endif
                   "QTableWidget::item:hover { color: black; background: lightGray; }"
                   "QTreeView::item:hover { color: black; background: lightGray; }").arg(bgColor.name()).arg(fgColor.name());
}

bool
GCColor::isFlat()
{
    return true;
}

// setup a linearGradient for the metallic backgrounds used on things like
// the toolbar, sidebar handles and so on
QLinearGradient
GCColor::linearGradient(int size, bool active, bool alternate)
{
    QLinearGradient returning;

    QString chrome = appsettings->value(NULL, GC_CHROME, "Flat").toString();

    if (chrome == "Mac") {
        int shade, inshade;
        if (!alternate) {
#ifdef Q_OS_MAC
            shade = 178;
            inshade = 225;
#else
            shade = 200;
            inshade = 250;
#endif
        } else {
#ifdef Q_OS_MAC
            inshade = 225;
            shade = 210;
#else
            inshade = 250;
            shade = 225;
#endif
        }

        // metallic
        if (active) {
            returning = QLinearGradient(0, 0, 0, size);
            returning.setColorAt(0.0, QColor(shade,shade,shade, 100));
            returning.setColorAt(0.5, QColor(shade,shade,shade, 180));
            returning.setColorAt(1.0, QColor(shade,shade,shade, 255));
            returning.setSpread(QGradient::PadSpread);
        } else {
            returning = QLinearGradient(0, 0, 0, size);
            returning.setColorAt(0.0, QColor(inshade,inshade,inshade, 100));
            returning.setColorAt(0.5, QColor(inshade,inshade,inshade, 180));
            returning.setColorAt(1.0, QColor(inshade,inshade,inshade, 255));
            returning.setSpread(QGradient::PadSpread);
        }

    } else {

        QColor color = GColor(CCHROME);

//
// The DWM api is how the MS windows color settings should be accessed
//
#ifdef GC_HAVE_DWM

        if (color == QColor(1,1,1)) { // use system default, user hasn't changed

            // use Windows API
            DWORD wincolor = 0;
            BOOL opaque = FALSE;

            HRESULT hr = DwmGetColorizationColor(&wincolor, &opaque);
            if (SUCCEEDED(hr)) {
                BYTE red = GetRValue(wincolor);
                BYTE green = GetGValue(wincolor);
                BYTE blue = GetBValue(wincolor);
                color = QColor::fromRgb(red,green,blue,255);
            } 
        }
#endif

        // just blocks of color
        returning = QLinearGradient(0, 0, 0, size);
        returning.setColorAt(0.0, color);
        returning.setColorAt(1.0, color);

    }

    return returning; 
}

void
GCColor::dumpColors()
{
    for(unsigned int i=0; ColorList[i].name != ""; i++) {
        fprintf(stderr, "ColorList[%d].color = QColor(%d,%d,%d); // %d:%s\n", i, ColorList[i].color.red(),ColorList[i].color.green(),ColorList[i].color.blue(),
                                                            i, ColorList[i].name.toStdString().c_str());
    }
}

QStringList
GCColor::getConfigKeys() {

    QStringList result;
    for (unsigned int i=0; ColorList[i].name != ""; i++) {
        result.append(ColorList[i].setting);
    }
    return result;
}


//
// Themes
//

Themes::Themes()
{
    // initialise the array of themes, lets just start with a compiled in list
    QList<QColor> colors;
    ColorTheme add("", true, QList<QColor>());

    //
    // Add all the standard themes
    //

    // MODERN DARK (Sublime Editor inspired)
    add.name = tr("Modern Dark");
    add.dark = true;
    add.stealth = false;
    colors << QColor(19,19,19) // Plot Background
           << QColor(32,32,32) // Toolbar and Sidebar Chrome
           << QColor(85,170,255) // Accent color (markers)
           << QColor(194,194,194) // Selection color
           << QColor(Qt::yellow) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(Qt::green) // Speed
           << QColor(255,170,0) // Power
           << QColor(0,204,204) // Cadence
           << QColor(Qt::magenta) // Torque
           << QColor(19,19,19) // Overview Background
           << QColor(39,39,39) // Overview Tile Background
           << QColor(60,60,60) // Overview Tile Background 2
           << QColor(84,84,84);// Overview Tile Background 3
    add.colors = colors;
    themes << add;
    colors.clear();


    // MODERN LIGHT (SAP Fiori Belize inspired)
    add.name = tr("Modern Light");
    add.dark = false;
    add.stealth = false;
    colors << QColor(Qt::white)  // Plot Background
           << QColor(0xef,0xf4,0xf9) // Toolbar and Sidebar Chrome
           << QColor(0x26,0x84,0xf6) // Accent color (markers)
           << QColor(Qt::blue) // Selection color
           << QColor(Qt::darkMagenta) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(85,170,0) // Speed
           << QColor(255,170,0) // Power
           << QColor(0,204,204) // Cadence
           << QColor(Qt::magenta) // Torque
           << QColor(0xcb,0xdc,0xea) // Overview Background
           << QColor(255,255,255) // Overview Tile Background
           << QColor(247,252,255) // Overview Tile Background 2
           << QColor(231,241,250);// Overview Tile Background 3
    add.colors = colors;
    themes << add;
    colors.clear();

    // STEALTH DARK (tab placement and mostly black)
    add.name = tr("Modern Stealth Dark");
    add.dark = true;
    add.stealth = true;
    colors << QColor(19,19,19) // Plot Background
           << QColor(19,19,19) // Toolbar and Sidebar Chrome
           << QColor(85,170,255) // Accent color (markers)
           << QColor(194,194,194) // Selection color
           << QColor(Qt::yellow) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(Qt::green) // Speed
           << QColor(255,170,0) // Power
           << QColor(0,204,204) // Cadence
           << QColor(Qt::magenta) // Torque
           << QColor(19,19,19) // Overview Background
           << QColor(30,30,30) // Overview Tile Background
           << QColor(38,38,38) // Overview Tile Background 2
           << QColor(88,88,88);// Overview Tile Background 3
    add.colors = colors;
    themes << add;
    colors.clear();

    // STEALTH LIGHT (tab placement and mostly white)
    add.name = tr("Modern Stealth Light");
    add.dark = false;
    add.stealth = true;
    colors << QColor(255,255,255) // Plot Background
           << QColor(255,255,255) // Toolbar and Sidebar Chrome
           << QColor(52,99,255) // Accent color (markers)
           << QColor(Qt::blue) // Selection color
           << QColor(Qt::darkMagenta) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(85,170,0) // Speed
           << QColor(255,170,0) // Power
           << QColor(0,204,204) // Cadence
           << QColor(Qt::magenta) // Torque
           << QColor(255,255,255) // Overview Background
           << QColor(245,245,245) // Overview Tile Background
           << QColor(227,227,227) // Overview Tile Background 2
           << QColor(202,202,202);// Overview Tile Background 3
    add.colors = colors;
    themes << add;
    colors.clear();

    add.name = tr("Gnome Adwaita Dark");
    add.dark = true;
    add.stealth = false;
    colors << QColor(19,19,19)  // Plot Background
           << QColor(44,49,51) // Toolbar and Sidebar Chrome
           << QColor(85,170,255) // Accent color (markers)
           << QColor(194,194,194) // Selection color
           << QColor(Qt::yellow) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(Qt::green) // Speed
           << QColor(255,170,0) // Power
           << QColor(0,204,204) // Cadence
           << QColor(Qt::magenta) // Torque
           << QColor(19,19,19) // Overview Background
           << QColor(44,49,51) // Overview Tile Background
           << QColor(57,63,66) // Overview Tile Background 2
           << QColor(73,81,91);// Overview Tile Background 3
    add.colors = colors;
    themes  << add;
    colors.clear();

    add.name = tr("Team Colours (light)");
    add.dark = false;
    add.stealth = false;
    colors << QColor(Qt::white)  // Plot Background
           << QColor(0x36,0x37,0x4b) // Toolbar and Sidebar Chrome
           << QColor(0x65,0x69,0xa5) // Accent color (markers)
           << QColor(194,194,194) // Selection color
           << QColor(Qt::darkMagenta) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(Qt::darkGreen) // Speed
           << QColor(255,170,0) // Power
           << QColor(0,204,204) // Cadence
           << QColor(Qt::magenta) // Torque
           << QColor(0xe3,0xe0,0xe8) // Overview Background
           << QColor(Qt::white) // Overview Tile Background
           << QColor(252,249,255) // Overview Tile Background 2
           << QColor(235,235,250);// Overview Tile Background 3
    add.colors = colors;
    themes  << add;
    colors.clear();

    add.name = tr("Ollie's Oatmeal (light)");
    add.dark = false;
    add.stealth = false;
    colors << QColor(255,255,255)  // Plot Background
           << QColor(63,69,58) // Toolbar and Sidebar Chrome
           << QColor(0x8d,0x57,0x30) // Accent color (markers)
           << QColor(194,194,194) // Selection color
           << QColor(Qt::darkMagenta) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(Qt::green) // Speed
           << QColor(255,170,0) // Power
           << QColor(0,204,204) // Cadence
           << QColor(Qt::magenta) // Torque
           << QColor(192,201,197) // Overview Background
           << QColor(235,241,234) // Overview Tile Background
           << QColor(250,255,247) // Overview Tile Background 2
           << QColor(83,93,82);// Overview Tile Background 3
    add.colors = colors;
    themes  << add;
    colors.clear();

    add.name = tr("Mustang (dark)"); // ** DARK **
    add.dark = true;
    add.stealth = false;
    colors << QColor(0,0,0)  // Plot Background
           << QColor(35,35,35) // Toolbar and Sidebar Chrome
           << QColor(255,152,0) // Accent color (markers)
           << QColor(Qt::white) // Selection color
           << QColor(126,138,162) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(Qt::green) // Speed
           << QColor(Qt::yellow) // Power
           << QColor(0,204,204) // Cadence
           << QColor(Qt::magenta) // Torque
           << QColor(0,0,0) // Overview Background
           << QColor(42,42,42) // Overview Tile Background
           << QColor(30,30,30) // Overview Tile Background 2
           << QColor(80,80,80);// Overview Tile Background 3
    add.colors = colors;
    themes  << add;
    colors.clear();

    add.name = tr("Mono (dark)"); // New v3.1 default colors // ** DARK **
    add.dark = true;
    add.stealth = false;
    colors << QColor(Qt::black)  // Plot Background
           << QColor(Qt::black) // Toolbar and Sidebar Chrome
           << QColor(Qt::white) // Accent color (markers)
           << QColor(Qt::white) // Selection color
           << QColor(Qt::white) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(Qt::green) // Speed
           << QColor(Qt::yellow) // Power
           << QColor(0,204,204) // Cadence
           << QColor(Qt::magenta) // Torque
           << QColor(0,0,0) // Overview Background
           << QColor(42,42,42) // Overview Tile Background
           << QColor(42,42,42) // Overview Tile Background 2
           << QColor(42,42,42);// Overview Tile Background 3
    add.colors = colors;
    themes  << add;
    colors.clear();

    add.name = tr("Mono (light)"); // New v3.1 default colors // ** LIGHT **
    add.dark = false;
    add.stealth = false;
    colors  << QColor(Qt::white)  // Plot Background
           << QColor(Qt::white) // Toolbar and Sidebar Chrome
           << QColor(Qt::black) // Accent color (markers)
           << QColor(Qt::black) // Selection color
           << QColor(Qt::black) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(Qt::green) // Speed
           << QColor(Qt::black) // Power
           << QColor(0,204,204) // Cadence
           << QColor(Qt::magenta) // Torque
           << QColor(255,255,255) // Overview Background
           << QColor(245,245,245) // Overview Tile Background
           << QColor(245,245,245) // Overview Tile Background 2
           << QColor(245,245,245);// Overview Tile Background 3
    add.colors = colors;
    themes  << add;
    colors.clear();

    // we can add more later ....
    add.name = tr("Schoberer (light)"); // Old GoldenCheetah colors // ** LIGHT **
    add.dark = false;
    add.stealth = false;
    colors << QColor(Qt::white)  // Plot Background
           << QColor(0xec,0xec,0xec) // Toolbar and Sidebar Chrome
           << QColor(Qt::black) // Accent color (markers)
           << QColor(Qt::green) // Selection color
           << QColor(Qt::red) // Critical Power and W'Bal
           << QColor(Qt::red) // Heartrate
           << QColor(Qt::magenta) // Speed
           << QColor(Qt::green) // Power
           << QColor(Qt::blue) // Cadence
           << QColor(Qt::darkGreen) // Torque
           << QColor(255,255,255) // Overview Background
           << QColor(245,245,245) // Overview Tile Background
           << QColor(245,245,245) // Overview Tile Background 2
           << QColor(245,245,245);// Overview Tile Background 3
    add.colors = colors;
    themes  << add;
    colors.clear();

}

// NOTE: this is duplicated in Pages.cpp:1407:ColorsPage::applyThemeClicked()
//       you need to change there too. Sorry.
void
GCColor::applyTheme(int index) 
{
    // now get the theme selected
    ColorTheme theme = GCColor::themes().themes[index];

    for (int i=0; ColorList[i].name != ""; i++) {

        QColor color;

        // apply theme to color
        switch(i) {

        case CPLOTBACKGROUND:
        case CRIDEPLOTBACKGROUND:
        case CTRENDPLOTBACKGROUND:

            color = theme.colors[0]; // background color
            break;

        case CTRAINPLOTBACKGROUND:
            // always, and I mean always default to a black background
            color = QColor(Qt::black);
            break;

        case CCARDBACKGROUND:
            // set back to light black for dark themes
            // and gray for light themes
            color = theme.colors[11];
            break;

        case CCARDBACKGROUND2:
            // set back to light black for dark themes
            // and gray for light themes
            color = theme.colors[12];
            break;

        case CCARDBACKGROUND3:
            // set back to light black for dark themes
            // and gray for light themes
            color = theme.colors[13];
            break;

        case COVERVIEWBACKGROUND:
            color = theme.colors[10];
            break;

        case CCHROME:
        case CCHARTBAR:
        case CTOOLBAR: // we always keep them the same, but user can make different
            color = theme.colors[1];
            break;

        case CHOVER:
            // stealthy themes use overview card background for hover color since they are close
            // all other themes get a boring default
            color = theme.stealth ? ColorList[96].color : (theme.dark ? QColor(50,50,50) : QColor(200,200,200));
            break;

        case CPLOTSYMBOL:
        case CRIDEPLOTXAXIS:
        case CRIDEPLOTYAXIS:
        case CPLOTMARKER:
            color = theme.colors[2]; // accent color
            break;

        case CPLOTSELECT:
        case CPLOTTRACKER:
        case CINTERVALHIGHLIGHTER:
            color = theme.colors[3]; // select color
            break;


        case CPLOTGRID: // grid doesn't have a theme color
                        // we make it barely distinguishable from background
            {
                QColor bg = theme.colors[0];
                if(bg == QColor(Qt::black)) color = QColor(30,30,30);
                else color = bg.darker(110);
            }
            break;

        case CCP:
        case CWBAL:
        case CRIDECP:
            color = theme.colors[4];
            break;

        case CHEARTRATE:
            color = theme.colors[5];
            break;

        case CSPEED:
            color = theme.colors[6];
            break;

        case CPOWER:
            color = theme.colors[7];
            break;

        case CCADENCE:
            color = theme.colors[8];
            break;

        case CTORQUE:
            color = theme.colors[9];
            break;


        default:
            if (theme.dark) color = DarkDefaultColorList[i].color;
            else color = LightDefaultColorList[i].color;
        }

        // theme applied !
        ColorList[i].color = color;

        QString colorstring = QString("%1:%2:%3").arg(color.red())
                                                 .arg(color.green())
                                                 .arg(color.blue());
        appsettings->setValue(ColorList[i].setting, colorstring);
    }

#ifdef Q_OS_MAC
    // if on yosemite we always set default chrome to #e5e5e5 and flat
    if (QSysInfo::MacintoshVersion == 12) {
        QColor color = QColor(0xe5,0xe5,0xe5);
        ColorList[CCHROME].color = color;
        QString colorstring = QString("%1:%2:%3").arg(color.red())
                                                 .arg(color.green())
                                                 .arg(color.blue());
        appsettings->setValue(ColorList[CCHROME].setting, colorstring);
        appsettings->setValue(GC_CHROME, "Flat");
    }
#endif
}

//
// ColorLabel - just paints a swatch of the first 5 colors
//
void
ColorLabel::paintEvent(QPaintEvent *)
{

    QPainter painter(this);
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing, true);

    // grr. some want a rect others want a rectf
    QRectF allF(0,0,width(),height());
    QRect all(0,0,width(),height());

    const double x = width() / 5; 
    const double y = height();

    // now do all the color blocks
    for (int i=0; i<5; i++) {

        QRectF block (i*x,0,x,y);
        painter.fillRect(block, theme.colors[i]);
    }

    painter.restore();
}

QPixmap colouredPixmapFromPNG(QString filename, QColor color)
{
    QImage pngImage;
    pngImage.load(filename);

    // adjust color for black
    QImage colored = pngImage.convertToFormat(QImage::Format_Indexed8);
    colored.setColor(0, color.rgb());

    return QPixmap::fromImage(colored);
}

QIcon colouredIconFromPNG(QString filename, QColor color)
{
    return QIcon(colouredPixmapFromPNG(filename, color));
}

