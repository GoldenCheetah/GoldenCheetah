 /* Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

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

unsigned long Colors::fingerprint(const std::map<GCol, Colors>& colorTable)
{
    QByteArray ba;
    for (auto& colorElemt : colorTable) {
        ba.append(colorElemt.second.name);
    }
    return qChecksum(ba, ba.length());
}

GCColor::GCColor() {

    // A selection of distinct colours, user can adjust also
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

    // Initialise colours
    // consider removing when we can guarantee extended initialisation support in gcc
    // (c++0x not supported by Qt currently and not planned for 4.8 or 5.0)
    colorTable = {

        // The Light colors are more appropriate on a light background, since the colorList versions were all selected on the basis of a dark background
        // note: we don't update them all so old standard charts aren't affected too badly, but may do so in the future

        // GCol reference,  group name, color name, color setting, darkColorList, lightColorList, colorList
        { GCol::PLOTBACKGROUND, { tr("Chart"), tr("Plot Background"), "COLORPLOTBACKGROUND", QColor(52,52,52), QColor(255,255,255), Qt::black } },
        { GCol::RIDEPLOTBACKGROUND, { tr("Chart"), tr("Performance Plot Background"), "COLORRIDEPLOTBACKGROUND", QColor(52,52,52), QColor(255,255,255), Qt::black } },
        { GCol::TRENDPLOTBACKGROUND, { tr("Chart"), tr("Trend Plot Background"), "COLORTRENDPLOTBACKGROUND", Qt::black, QColor(255,255,255), Qt::black } },
        { GCol::TRAINPLOTBACKGROUND, { tr("Chart"), tr("Train Plot Background"), "COLORTRAINPLOTBACKGROUND", Qt::black, QColor(255,255,255), Qt::black } },
        { GCol::PLOTSYMBOL, { tr("Chart"), tr("Plot Symbols"), "COLORRIDEPLOTSYMBOLS", Qt::cyan, QColor(101,105,165), Qt::black } },
        { GCol::RIDEPLOTXAXIS, { tr("Chart"), tr("Performance Plot X Axis"), "COLORRIDEPLOTXAXIS", Qt::blue, QColor(101,105,165), Qt::black } },
        { GCol::RIDEPLOTYAXIS, { tr("Chart"), tr("Performance Plot Y Axis"), "COLORRIDEPLOTYAXIS", Qt::red, QColor(101,105,165), Qt::black } },
        { GCol::PLOTTHUMBNAIL, { tr("Chart"), tr("Plot Thumbnail Background"), "COLORPLOTTHUMBNAIL", Qt::gray, QColor(160,160,164), Qt::black } },
        { GCol::PLOTTITLE, { tr("Chart"), tr("Plot Title"), "COLORPLOTTITLE", Qt::gray, QColor(0,0,0), Qt::black } },
        { GCol::PLOTSELECT, { tr("Chart"), tr("Plot Selection Pen"), "COLORPLOTSELECT", Qt::blue, QColor(194,194,194), Qt::black } },
        { GCol::PLOTTRACKER, { tr("Chart"), tr("Plot TrackerPen"), "COLORPLOTTRACKER", Qt::blue, QColor(194,194,194), Qt::black } },
        { GCol::PLOTMARKER, { tr("Chart"), tr("Plot Markers"), "COLORPLOTMARKER", Qt::cyan, QColor(101,105,165), Qt::black } },
        { GCol::PLOTGRID, { tr("Chart"), tr("Plot Grid"), "COLORGRID", QColor(65,65,65), QColor(232,232,232), Qt::black } },
        { GCol::INTERVALHIGHLIGHTER, { tr("Chart"), tr("Interval Highlighter"), "COLORINTERVALHIGHLIGHTER", Qt::blue, QColor(194,194,194), Qt::black } },
        { GCol::HEARTRATE, { tr("Data"), tr("Heart Rate"), "COLORHEARTRATE", Qt::red, QColor(255,0,0), Qt::black } },
        { GCol::CORETEMP, { tr("Data"), tr("Core Temperature"), "COLORCORETEMP", QColor(255, 173, 92), QColor(131,8,255), Qt::black } },
        { GCol::SPEED, { tr("Data"), tr("Speed"), "COLORSPEED", Qt::green, QColor(0, 102, 0), Qt::black } },
        { GCol::ACCELERATION, { tr("Data"), tr("Acceleration"), "COLORACCEL", Qt::cyan, QColor(0, 146, 146), Qt::black } },
        { GCol::POWER, { tr("Data"), tr("Power"), "COLORPOWER", Qt::yellow, QColor(255, 170, 0), Qt::black } },
        { GCol::NPOWER, { tr("Data"), tr("Iso Power"), "CNPOWER", Qt::magenta, QColor(255, 0, 255), Qt::black } },
        { GCol::XPOWER, { tr("Data"), tr("Skiba xPower"), "CXPOWER", Qt::magenta, QColor(158, 0, 158), Qt::black } },
        { GCol::APOWER, { tr("Data"), tr("Altitude Power"), "CAPOWER", Qt::magenta, QColor(255, 0, 255), Qt::black } },
        { GCol::TPOWER, { tr("Data"), tr("Train Target Power"), "CTPOWER", Qt::blue, QColor(0, 0, 255), Qt::black } },
        { GCol::CP, { tr("Data"), tr("Critical Power"), "COLORCP", Qt::cyan, QColor(167, 0, 109), Qt::black } },
        { GCol::CADENCE, { tr("Data"), tr("Cadence"), "COLORCADENCE", QColor(0,204,204), QColor(0, 126, 126), Qt::black } },
        { GCol::ALTITUDE, { tr("Data"), tr("Altitude"), "COLORALTITUTDE", Qt::gray, QColor(119, 119, 122), Qt::black } },
        { GCol::ALTITUDEBRUSH, { tr("Data"), tr("Altitude Shading"), "COLORALTITUDESHADE", Qt::lightGray, QColor(114, 114, 114), Qt::black } },
        { GCol::WINDSPEED, { tr("Data"), tr("Wind Speed"), "COLORWINDSPEED", Qt::darkGreen, QColor(0, 128, 0), Qt::black } },
        { GCol::TORQUE, { tr("Data"), tr("Torque"), "COLORTORQUE", Qt::magenta, QColor(111, 0, 111), Qt::black } },
        { GCol::SLOPE, { tr("Data"), tr("Slope"), "CSLOPE", Qt::green, QColor(0, 127, 0), Qt::black } },
        { GCol::GEAR, { tr("Data"), tr("Gear Ratio"), "COLORGEAR", QColor(0xff, 0x90, 0x00), QColor(255, 144, 0), Qt::black } },
        { GCol::RV, { tr("Data"), tr("Run Vertical Oscillation"), "COLORRVERT", QColor(0xff, 0x90, 0x00), QColor(255, 144, 0), Qt::black } }, // same as garmin connect colors
        { GCol::RCAD, { tr("Data"), tr("Run Cadence"), "COLORRCAD", QColor(0xff, 0x90, 0x00), QColor(255, 144, 0), Qt::black } }, // same as garmin connect colors
        { GCol::RGCT, { tr("Data"), tr("Run Ground Contact"), "COLORGCT", QColor(0xff, 0x90, 0x00), QColor(255, 144, 0), Qt::black } }, // same as garmin connect colors
        { GCol::SMO2, { tr("Data"), tr("Muscle Oxygen (SmO2)"), "COLORSMO2", QColor(0x00, 0x89, 0x77), QColor(0, 137, 119), Qt::black } }, // green same as moxy monitor
        { GCol::THB, { tr("Data"), tr("Haemoglobin Mass (tHb)"), "COLORTHB", QColor(0xa3,0x44,0x02), QColor(163, 68, 2), Qt::black } },  // brown same as moxy monitor
        { GCol::O2HB, { tr("Data"), tr("Oxygenated Haemoglobin (O2Hb)"), "CO2HB", QColor(0xd1,0x05,0x72), QColor(209, 5, 114), Qt::black } },
        { GCol::HHB, { tr("Data"), tr("Deoxygenated Haemoglobin (HHb)"), "CHHB", QColor(0x00,0x7f,0xcc), QColor(0, 127, 204), Qt::black } },
        { GCol::LOAD, { tr("Data"), tr("Load"), "COLORLOAD", Qt::yellow, QColor(127, 127, 0), Qt::black } },
        { GCol::TSS, { tr("Data"), tr("BikeStress"), "COLORTSS", Qt::green, QColor(0, 81, 0), Qt::black } },
        { GCol::STS, { tr("Data"), tr("Short Term Stress"), "COLORSTS", Qt::blue, QColor(227, 12, 255), Qt::black } },
        { GCol::LTS, { tr("Data"), tr("Long Term Stress"), "COLORLTS", Qt::green, QColor(16, 0, 195), Qt::black } },
        { GCol::SB, { tr("Data"), tr("Stress Balance"), "COLORSB", QColor(180,140,140), QColor(209, 193, 23), Qt::black } },
        { GCol::DAILYSTRESS, { tr("Data"), tr("Daily Stress"), "COLORDAILYSTRESS", Qt::red, QColor(255, 0, 0), Qt::black } },
        { GCol::BIKESCORE, { tr("Data"), "Bike Score (TM)", "COLORBIKESCORE", Qt::gray, QColor(160, 160, 164), Qt::black } },
        { GCol::CALENDARTEXT, { tr("Gui"), tr("Calendar Text"), "COLORCALENDARTEXT", Qt::gray, QColor(0, 0, 0), Qt::black } },
        { GCol::ZONE1, { tr("Data"), tr("Power Zone 1 Shading"), "COLORZONE1", QColor(255,0,255), QColor(255, 0, 255), Qt::black } },
        { GCol::ZONE2, { tr("Data"), tr("Power Zone 2 Shading"), "COLORZONE2", QColor(42,0,255), QColor(42, 0, 255), Qt::black } },
        { GCol::ZONE3, { tr("Data"), tr("Power Zone 3 Shading"), "COLORZONE3", QColor(0,170,255), QColor(0, 170, 255), Qt::black } },
        { GCol::ZONE4, { tr("Data"), tr("Power Zone 4 Shading"), "COLORZONE4", QColor(0,255,128), QColor(0, 255, 128), Qt::black } },
        { GCol::ZONE5, { tr("Data"), tr("Power Zone 5 Shading"), "COLORZONE5", QColor(85,255,0), QColor(85, 255, 0), Qt::black } },
        { GCol::ZONE6, { tr("Data"), tr("Power Zone 6 Shading"), "COLORZONE6", QColor(255,213,0), QColor(255, 213, 0), Qt::black } },
        { GCol::ZONE7, { tr("Data"), tr("Power Zone 7 Shading"), "COLORZONE7", QColor(255,0,0), QColor(255, 0, 0), Qt::black } },
        { GCol::ZONE8, { tr("Data"), tr("Power Zone 8 Shading"), "COLORZONE8", Qt::gray, QColor(160, 160, 164), Qt::black } },
        { GCol::ZONE9, { tr("Data"), tr("Power Zone 9 Shading"), "COLORZONE9", Qt::gray, QColor(160, 160, 164), Qt::black } },
        { GCol::ZONE10, { tr("Data"), tr("Power Zone 10 Shading"), "COLORZONE10", Qt::gray, QColor(160, 160, 164), Qt::black } },
        { GCol::HZONE1, { tr("Data"), tr("HR Zone 1 Shading"), "HRCOLORZONE1", QColor(255,0,255), QColor(255, 0, 255), Qt::black } },
        { GCol::HZONE2, { tr("Data"), tr("HR Zone 2 Shading"), "HRCOLORZONE2", QColor(42,0,255), QColor(42, 0, 255), Qt::black } },
        { GCol::HZONE3, { tr("Data"), tr("HR Zone 3 Shading"), "HRCOLORZONE3", QColor(0,170,255), QColor(0, 170, 255), Qt::black } },
        { GCol::HZONE4, { tr("Data"), tr("HR Zone 4 Shading"), "HRCOLORZONE4", QColor(0,255,128), QColor(0, 255, 128), Qt::black } },
        { GCol::HZONE5, { tr("Data"), tr("HR Zone 5 Shading"), "HRCOLORZONE5", QColor(85,255,0), QColor(85, 255, 0), Qt::black } },
        { GCol::HZONE6, { tr("Data"), tr("HR Zone 6 Shading"), "HRCOLORZONE6", QColor(255,213,0), QColor(255, 213, 0), Qt::black } },
        { GCol::HZONE7, { tr("Data"), tr("HR Zone 7 Shading"), "HRCOLORZONE7", QColor(255,0,0), QColor(255, 0, 0), Qt::black } },
        { GCol::HZONE8, { tr("Data"), tr("HR Zone 8 Shading"), "HRCOLORZONE8", Qt::gray, QColor(160, 160, 164), Qt::black } },
        { GCol::HZONE9, { tr("Data"), tr("HR Zone 9 Shading"), "HRCOLORZONE9", Qt::gray, QColor(160, 160, 164), Qt::black } },
        { GCol::HZONE10, { tr("Data"), tr("HR Zone 10 Shading"), "HRCOLORZONE10", Qt::gray, QColor(160, 160, 164), Qt::black } },
        { GCol::AEROVE, { tr("Data"), tr("Aerolab Vrtual Elevation"), "COLORAEROVE", Qt::blue, QColor(0, 0, 255), Qt::black } },
        { GCol::AEROEL, { tr("Data"), tr("Aerolab Elevation"), "COLORAEROEL", Qt::green, QColor(0, 255, 0), Qt::black } },
        { GCol::CALCELL, { tr("Gui"), tr("Calendar background"), "CCALCELL", Qt::white, QColor(255, 255, 255), Qt::black } },
        { GCol::CALHEAD, { tr("Gui"), tr("Calendar heading"), "CCALHEAD", QColor(230,230,230), QColor(230, 230, 230), Qt::black } },
        { GCol::CALCURRENT, { tr("Gui"), tr("Calendar Current Selection"), "CCALCURRENT", QPalette().color(QPalette::Highlight), QColor(48, 140, 198), Qt::black } }, // set the defaults to system defaults
        { GCol::CALACTUAL, { tr("Gui"), tr("Calendar Actual Workout"), "CCALACTUAL", Qt::green, QColor(0, 255, 0), Qt::black } },
        { GCol::CALPLANNED, { tr("Gui"), tr("Calendar Planned Workout"), "CCALPLANNED", Qt::yellow, QColor(255, 177, 21), Qt::black } },
        { GCol::CALTODAY, { tr("Gui"), tr("Calendar Today"), "CCALTODAY", Qt::cyan, QColor(0, 255, 255), Qt::black } },
        { GCol::POPUP, { tr("Gui"), tr("Pop Up Windows Background"), "CPOPUP", Qt::lightGray, QColor(255, 255, 255), Qt::black } },
        { GCol::POPUPTEXT, { tr("Gui"), tr("Pop Up Windows Foreground"), "CPOPUPTEXT", Qt::white, QColor(119, 119, 119), Qt::black } },
        { GCol::TILEBAR, { tr("Gui"), tr("Chart Bar Unselected"), "CTILEBAR", Qt::gray, QColor(160, 160, 164), Qt::black } },
        { GCol::TILEBARSELECT, { tr("Gui"), tr("Chart Bar Selected"), "CTILEBARSELECT", Qt::yellow, QColor(255, 255, 0), Qt::black } },
        { GCol::TOOLBAR, { tr("Gui"), tr("ToolBar Background"), "CTOOLBAR", QPalette().color(QPalette::Window), QColor(239, 239, 239), Qt::black } }, // set the defaults to system defaults
        { GCol::RIDEGROUP, { tr("Gui"), tr("Activity History Group"), "CRIDEGROUP", QColor(236,246,255), QColor(236, 246, 255), Qt::black } },
        { GCol::SPINSCANLEFT, { tr("Data"), tr("SpinScan Left"), "CSPINSCANLEFT", Qt::gray, QColor(0, 164, 101), Qt::black } },
        { GCol::SPINSCANRIGHT, { tr("Data"), tr("SpinScan Right"), "CSPINSCANRIGHT", Qt::cyan, QColor(0, 130, 130), Qt::black } },
        { GCol::TEMP, { tr("Data"), tr("Temperature"), "COLORTEMPERATURE", Qt::yellow, QColor(0, 107, 188), Qt::black } },
        { GCol::DIAL, { tr("Data"), tr("Default Dial Color"), "CDIAL", Qt::gray, QColor(160, 160, 164), Qt::black } },
        { GCol::ALTPOWER, { tr("Data"), tr("Alternate Power"), "CALTPOWER", Qt::magenta, QColor(255, 0, 255), Qt::black } },
        { GCol::BALANCELEFT, { tr("Data"), tr("Left Balance"), "CBALANCELEFT", QColor(178,0,0), QColor(178, 0, 0), Qt::black } },
        { GCol::BALANCERIGHT, { tr("Data"), tr("Right Balance"), "CBALANCERIGHT", QColor(128,0,50), QColor(128, 0, 50), Qt::black } },
        { GCol::WBAL, { tr("Data"), tr("W' Balance"), "CWBAL", Qt::red, QColor(186, 57, 59), Qt::black } },
        { GCol::RIDECP, { tr("Data"), tr("Mean-maximal Power"), "CRIDECP", Qt::red, QColor(255, 85, 255), Qt::black } },
        { GCol::ATISS, { tr("Data"), tr("Aerobic TISS"), "CATISS", Qt::magenta, QColor(146, 0, 146), Qt::black } },
        { GCol::ANTISS, { tr("Data"), tr("Anaerobic TISS"), "CANTISS", Qt::cyan, QColor(0, 130, 130), Qt::black } },
        { GCol::LTE, { tr("Data"), tr("Left Torque Effectiveness"), "CLTE", Qt::cyan, QColor(0, 137, 137), Qt::black } },
        { GCol::RTE, { tr("Data"), tr("Right Torque Effectiveness"), "CRTE", Qt::magenta, QColor(145, 0, 145), Qt::black } },
        { GCol::LPS, { tr("Data"), tr("Left Pedal Smoothness"), "CLPS", Qt::cyan, QColor(0, 146, 146), Qt::black } },
        { GCol::RPS, { tr("Data"), tr("Right Pedal Smoothness"), "CRPS", Qt::magenta, QColor(117, 0, 117), Qt::black } },
#ifdef GC_HAVE_DWM
        { GCol::CHROME, { tr("Gui"), tr("Toolbar and Sidebar"), "CCHROME", QColor(1,1,1), QColor(54, 55, 75), Qt::black } },
#else
#ifdef Q_OS_MAC
        { GCol::CHROME, { tr("Gui"), tr("Sidebar background"), "CCHROME", QColor(213,213,213), QColor(54, 55, 75), Qt::black } },
#else
        { GCol::CHROME, { tr("Gui"), tr("Sidebar background"), "CCHROME", QColor(0xec,0xec,0xec), QColor(54, 55, 75), Qt::black } },
#endif
#endif
        { GCol::OVERVIEWBACKGROUND, { tr("Gui"), tr("Overview Background"), "COVERVIEWBACKGROUND", QColor(0,0,0), QColor(227, 224, 232), Qt::black } },
        { GCol::CARDBACKGROUND, { tr("Gui"), tr("Overview Tile Background"), "CCARDBACKGROUND", QColor(52,52,52), QColor(255, 255, 255), Qt::black } },
        { GCol::VO2, { tr("Data"), tr("VO2"), "CVO2", Qt::magenta, QColor(255, 25, 167), Qt::black } },
        { GCol::VENTILATION, { tr("Data"), tr("Ventilation"), "CVENTILATION", Qt::cyan, QColor(27, 203, 177), Qt::black } },
        { GCol::VCO2, { tr("Data"), tr("VCO2"), "CVCO2", Qt::green, QColor(0, 121, 0), Qt::black } },
        { GCol::TIDALVOLUME, { tr("Data"), tr("Tidal Volume"), "CTIDALVOLUME", Qt::yellow, QColor(101, 44, 45), Qt::black } },
        { GCol::RESPFREQUENCY, { tr("Data"), tr("Respiratory Frequency"), "CRESPFREQUENCY", Qt::yellow, QColor(134, 74, 255), Qt::black } },
        { GCol::FEO2, { tr("Data"), tr("FeO2"), "CFEO2", Qt::yellow, QColor(255, 46, 46), Qt::black } },
        { GCol::HOVER, { tr("Gui"), tr("Toolbar Hover"), "CHOVER", Qt::lightGray, QColor(), Qt::black } },
        { GCol::CHARTBAR, { tr("Gui"), tr("Chartbar background"), "CCHARTBAR", Qt::lightGray, QColor(), Qt::black } },
        { GCol::CARDBACKGROUND2, { tr("Gui"), tr("Overview Tile Background Alternate"), "CCARDBACKGROUND2", QColor(0,0,0), QColor(180, 180, 180), Qt::black } },
        { GCol::CARDBACKGROUND3, { tr("Gui"), tr("Overview Tile Background Vibrant"), "CCARDBACKGROUND3", QColor(52,52,52), QColor(238, 248, 255), Qt::black } },
        { GCol::MAPROUTELINE, { tr("Gui"), tr("Map Route Line"), "MAPROUTELINE", Qt::red, QColor(255, 0, 0), Qt::black } },
        { GCol::COLORRR, { tr("Data"), tr("Stress Ramp Rate"), "COLORRR", Qt::green, QColor(0, 255, 0), Qt::black } }
    };

    resetColors();
}

QColor GCColor::standardQColor(int index) {

    return standardColors[index % standardColors.size()];
}

// functions to convert between gColors -> rgb colors -> qColors
// An RGB color has red & green = 1, and blue is set to a GCol enumerated value

QColor GCColor::gColorToRGBColor(const GCol& gColor) {
    return (gColor < GCol::NUMOFCFGCOLORS) ? QColor(1, 1, static_cast<int>(gColor)) : INVALIDGCOLOR;
}

bool GCColor::isRGBColor(const QColor& qColor) {
    return (qColor.red() == 1 && qColor.green() == 1);
}

QColor GCColor::rgbColorToQColor(const QColor& qColor) {
    return isRGBColor(qColor) ? getQColor(static_cast<GCol>(qColor.blue())) : qColor;
}


// returns the luminance of a GCol, 0 (dark) to 255 (very light) 127 is a half way house gray
double GCColor::luminance(const GCol& gColor) {
    return (gColor < GCol::NUMOFCFGCOLORS) ? luminance(colorTable[gColor].qColor) : INVALIDGCOLORLUMINANCE;
}

// returns the luminance of a QColor, 0 (dark) to 255 (very light) 127 is a half way house gray
double GCColor::luminance(const QColor& qColor)
{
    QColor cRGB = qColor.convertTo(QColor::Rgb);

    // based upon http://en.wikipedia.org/wiki/Luminance_(relative)
    return (0.2126f * double(cRGB.red()))  + 
           (0.7152f * double(cRGB.green())) +
           (0.0722f * double(cRGB.blue()));
}

QColor GCColor::invertQColor(const GCol& gColor) {
    return (gColor < GCol::NUMOFCFGCOLORS) ? invertQColor(colorTable[gColor].qColor) : INVALIDGCOLOR;
}

QColor GCColor::invertQColor(const QColor& qColor)
{
    if (qColor ==QColor(Qt::darkGray)) return QColor(Qt::white); // darkGray is popular!
    return luminance(qColor) < 127 ? QColor(Qt::white) : QColor(Qt::black);
}

QColor GCColor::alternateQColor(const GCol& gColor) {
    return (gColor < GCol::NUMOFCFGCOLORS) ? alternateQColor(colorTable[gColor].qColor) : INVALIDGCOLOR;
}

QColor GCColor::alternateQColor(const QColor& bgColor)
{
    //QColor color = QColor::fromHsv(bgColor.hue(), bgColor.saturation() * 2, bgColor.value());
    if (bgColor.value() < 128)
        return QColor(75,75,75); // avoid darkGray as clashes with text cursor on hidpi displays (!)
    else
        return QColor(Qt::lightGray);
}

QColor GCColor::selectedQColor(const QColor& qColor)
{
     // if foreground is white then we're "dark" if it's
     // black the we're "light" so this controls palette
     bool dark = invertQColor(qColor) == QColor(Qt::white);

     bool isblack = (qColor == QColor(Qt::black)); // e.g. mustang theme

     // on select background color
     QColor bg_select(qColor);
     if (dark) bg_select = bg_select.lighter(200);
     else bg_select = bg_select.darker(200);
     if (isblack) bg_select = QColor(30, 30, 30);

     return bg_select;
}

std::map<GCol, Colors>& GCColor::getColorTable()
{
    return colorTable;
}

void GCColor::resetColors()
{
    for (auto& colorElemt : colorTable) {
        colorElemt.second.qColor = colorElemt.second.qDarkColor;
    }
}

void
GCColor::readConfig()
{
    // read in config settings and populate the color table
    for (auto& colorElemt : colorTable) {

        QString colortext = appsettings->value(NULL, colorElemt.second.setting, "").toString();

        if (colortext != "") {
            // color definitions are stored as "r:g:b"
            QStringList rgb = colortext.split(":");
            colorElemt.second.qColor = QColor(rgb[0].toInt(),
                                              rgb[1].toInt(),
                                              rgb[2].toInt());
            } else {

            // set sensible defaults for any not set (as new colors are added)
            if (colorElemt.first == GCol::TOOLBAR) {
                QPalette def;
                colorElemt.second.qColor = def.color(QPalette::Window);
            }
            if (colorElemt.first == GCol::CHARTBAR) {
                colorElemt.second.qColor = colorTable[GCol::TOOLBAR].qColor;
            }
            if (colorElemt.first == GCol::CALCURRENT) {
                QPalette def;
                colorElemt.second.qColor = def.color(QPalette::Highlight);
            }
        }
    }
#ifdef Q_OS_MAC
    // if on yosemite set default chrome to #e5e5e5
    // TODO dgr
    if (QSysInfo::productVersion().toFloat() >= 12 | QSysInfo::productVersion().toFloat() < 13) {
        ColorList[CCHROME].color = QColor(0xe5,0xe5,0xe5);
    }
#endif
}

QColor
GCColor::getQColor(const GCol& gColor)
{
    return (gColor < GCol::NUMOFCFGCOLORS) ? colorTable[gColor].qColor : INVALIDGCOLOR;
}

bool
GCColor::setQColor(const GCol& gColor, const QColor& qColor)
{
    if (gColor < GCol::NUMOFCFGCOLORS) colorTable[gColor].qColor = qColor;
    return (gColor < GCol::NUMOFCFGCOLORS);
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
    reverseColor = GColor(GCol::PLOTBACKGROUND);

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
    QColor bgColor = ridesummary ? GColor(GCol::PLOTBACKGROUND) : GColor(GCol::TRENDPLOTBACKGROUND);
    QColor fgColor = GInvertColor(bgColor);
    //QColor altColor = GAlternateColor(bgColor); // not yet ?

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
                   "</style> ").arg(GColor(GCol::PLOTMARKER).name())
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
    palette.setBrush(QPalette::Window, QBrush(GColor(GCol::PLOTBACKGROUND)));
    palette.setBrush(QPalette::Base, QBrush(GColor(GCol::PLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GInvertColor(GColor(GCol::PLOTBACKGROUND)));
    palette.setColor(QPalette::Text, GInvertColor(GColor(GCol::PLOTBACKGROUND)));
    palette.setColor(QPalette::Normal, QPalette::Window, GInvertColor(GColor(GCol::PLOTBACKGROUND)));

    return palette;
}

QString 
GCColor::stylesheet(bool train)
{
    // make it to order to reflect current config
    QColor bgColor = train ? GColor(GCol::TRAINPLOTBACKGROUND) : GColor(GCol::PLOTBACKGROUND);

    QColor fgColor = GInvertColor(bgColor);
    QColor bgSelColor = selectedQColor(bgColor);
    QColor fgSelColor = GInvertColor(bgSelColor);

    return QString("QTreeView { color: %2; background: %1; }"
                   "%3"
                   "QTableWidget { color: %2; background: %1; }"
#ifndef Q_OS_MAC
                   "QHeaderView { background-color: %1; color: %2; }"
                   "QHeaderView::section { background-color: %1; color: %2; border: 0px ; }"
#endif
                   "QTableWidget::item:hover { color: black; background: lightGray; }"
                   "QTreeView::item:hover { color: black; background: lightGray; }"
                   "QTreeView::item:selected { color: %4; background-color: %3; }"
                  ).arg(bgColor.name()).arg(fgColor.name()).arg(bgSelColor.name()).arg(fgSelColor.name());
}

// setup a linearGradient for the metallic backgrounds used on things like
// the toolbar, sidebar handles and so on
QLinearGradient
GCColor::linearGradient(int size)
{
    QLinearGradient returning;
    QColor qColor = GColor(GCol::CHROME);

//
// The DWM api is how the MS windows color settings should be accessed
//
#ifdef GC_HAVE_DWM

    if (qColor == QColor(1,1,1)) { // use system default, user hasn't changed

        // use Windows API
        DWORD wincolor = 0;
        BOOL opaque = FALSE;

        HRESULT hr = DwmGetColorizationColor(&wincolor, &opaque);
        if (SUCCEEDED(hr)) {
            BYTE red = GetRValue(wincolor);
            BYTE green = GetGValue(wincolor);
            BYTE blue = GetBValue(wincolor);
            qColor = QColor::fromRgb(red,green,blue,255);
        }
    }
#endif

    // just blocks of color
    returning = QLinearGradient(0, 0, 0, size);
    returning.setColorAt(0.0, qColor);
    returning.setColorAt(1.0, qColor);


    return returning; 
}

void
GCColor::dumpColors()
{
    for (const auto& colorElemt : colorTable) {
        fprintf(stderr, "colorTable[%d].color = QColor(%d,%d,%d); // %d:%s\n",
                                        colorElemt.first,
                                        colorElemt.second.qColor.red(),
                                        colorElemt.second.qColor.green(),
                                        colorElemt.second.qColor.blue(),
                                        colorElemt.first, colorElemt.second.name.toStdString().c_str());
    }
}

QStringList
GCColor::getConfigKeys() {

    QStringList result;
    for (const auto& colorElemt : colorTable) {
        result.append(colorElemt.second.setting);
    }
    return result;
}


//
// Themes
//

Themes::Themes() {

    // initialise the array of themes, lets just start with a compiled in list
     ColorTheme add;

    //
    // Add all the standard themes
    //

    // MODERN DARK (Sublime Editor inspired)
    add.name = tr("Modern Dark");
    add.dark = true;
    add.stealth = false;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(19, 19, 19); // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(32, 32, 32); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(85, 170, 255); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(194, 194, 194); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(Qt::yellow); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(Qt::green); // Speed
    add.colors[GThmCol::POWER] = QColor(255, 170, 0); // Power
    add.colors[GThmCol::CADENCE] = QColor(0, 204, 204); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::magenta); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(19, 19, 19); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(39, 39, 39); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(60, 60, 60); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(84, 84, 84);// Overview Tile Background 3
    themes << add;


    // MODERN LIGHT (SAP Fiori Belize inspired)
    add.name = tr("Modern Light");
    add.dark = false;
    add.stealth = false;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(Qt::white);  // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(0xef, 0xf4, 0xf9); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(0x26, 0x84, 0xf6); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(Qt::blue); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(Qt::darkMagenta); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(85, 170, 0); // Speed
    add.colors[GThmCol::POWER] = QColor(255, 170, 0); // Power
    add.colors[GThmCol::CADENCE] = QColor(0, 204, 204); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::magenta); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(0xcb, 0xdc, 0xea); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(255, 255, 255); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(247, 252, 255); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(231, 241, 250);// Overview Tile Background 3
    themes << add;


    // STEALTH DARK (tab placement and mostly black)
    add.name = tr("Modern Stealth Dark");
    add.dark = true;
    add.stealth = true;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(19, 19, 19); // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(19, 19, 19); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(85, 170, 255); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(194, 194, 194); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(Qt::yellow); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(Qt::green); // Speed
    add.colors[GThmCol::POWER] = QColor(255, 170, 0); // Power
    add.colors[GThmCol::CADENCE] = QColor(0, 204, 204); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::magenta); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(19, 19, 19); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(30, 30, 30); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(38, 38, 38); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(88, 88, 88);// Overview Tile Background 3
    themes << add;


    // STEALTH LIGHT (tab placement and mostly white)
    add.name = tr("Modern Stealth Light");
    add.dark = false;
    add.stealth = true;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(255, 255, 255); // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(255, 255, 255); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(52, 99, 255); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(Qt::blue); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(Qt::darkMagenta); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(85, 170, 0); // Speed
    add.colors[GThmCol::POWER] = QColor(255, 170, 0); // Power
    add.colors[GThmCol::CADENCE] = QColor(0, 204, 204); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::magenta); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(255, 255, 255); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(245, 245, 245); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(227, 227, 227); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(202, 202, 202);// Overview Tile Background 3
    themes << add;


    add.name = tr("Gnome Adwaita Dark");
    add.dark = true;
    add.stealth = false;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(19, 19, 19);  // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(44, 49, 51); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(85, 170, 255); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(194, 194, 194); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(Qt::yellow); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(Qt::green); // Speed
    add.colors[GThmCol::POWER] = QColor(255, 170, 0); // Power
    add.colors[GThmCol::CADENCE] = QColor(0, 204, 204); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::magenta); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(19, 19, 19); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(44, 49, 51); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(57, 63, 66); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(73, 81, 91);// Overview Tile Background 3
    themes << add;


    add.name = tr("Team Colours (light)");
    add.dark = false;
    add.stealth = false;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(Qt::white);  // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(0x36, 0x37, 0x4b); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(0x65, 0x69, 0xa5); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(194, 194, 194); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(Qt::darkMagenta); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(Qt::darkGreen); // Speed
    add.colors[GThmCol::POWER] = QColor(255, 170, 0); // Power
    add.colors[GThmCol::CADENCE] = QColor(0, 204, 204); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::magenta); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(0xe3, 0xe0, 0xe8); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(Qt::white); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(252, 249, 255); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(235, 235, 250);// Overview Tile Background 3
    themes << add;


    add.name = tr("Ollie's Oatmeal (light)");
    add.dark = false;
    add.stealth = false;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(255, 255, 255);  // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(63, 69, 58); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(0x8d, 0x57, 0x30); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(194, 194, 194); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(Qt::darkMagenta); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(Qt::green); // Speed
    add.colors[GThmCol::POWER] = QColor(255, 170, 0); // Power
    add.colors[GThmCol::CADENCE] = QColor(0, 204, 204); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::magenta); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(192, 201, 197); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(235, 241, 234); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(250, 255, 247); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(83, 93, 82);// Overview Tile Background 3
    themes << add;


    add.name = tr("Mustang (dark)"); // ** DARK **
    add.dark = true;
    add.stealth = false;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(0, 0, 0);  // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(35, 35, 35); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(255, 152, 0); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(Qt::white); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(126, 138, 162); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(Qt::green); // Speed
    add.colors[GThmCol::POWER] = QColor(Qt::yellow); // Power
    add.colors[GThmCol::CADENCE] = QColor(0, 204, 204); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::magenta); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(0, 0, 0); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(42, 42, 42); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(30, 30, 30); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(80, 80, 80);// Overview Tile Background 3
    themes << add;


    add.name = tr("Mono (dark)"); // New v3.1 default colors // ** DARK **
    add.dark = true;
    add.stealth = false;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(Qt::black);  // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(Qt::black); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(Qt::white); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(Qt::white); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(Qt::white); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(Qt::green); // Speed
    add.colors[GThmCol::POWER] = QColor(Qt::yellow); // Power
    add.colors[GThmCol::CADENCE] = QColor(0, 204, 204); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::magenta); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(0, 0, 0); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(42, 42, 42); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(42, 42, 42); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(42, 42, 42);// Overview Tile Background 3
    themes << add;


    add.name = tr("Mono (light)"); // New v3.1 default colors // ** LIGHT **
    add.dark = false;
    add.stealth = false;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(Qt::white);  // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(Qt::white); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(Qt::black); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(Qt::black); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(Qt::black); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(Qt::green); // Speed
    add.colors[GThmCol::POWER] = QColor(Qt::black); // Power
    add.colors[GThmCol::CADENCE] = QColor(0, 204, 204); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::magenta); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(255, 255, 255); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(245, 245, 245); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(245, 245, 245); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(245, 245, 245);// Overview Tile Background 3
    themes << add;


    // we can add more later ....
    add.name = tr("Schoberer (light)"); // Old GoldenCheetah colors // ** LIGHT **
    add.dark = false;
    add.stealth = false;
    add.colors[GThmCol::PLOTBACKGROUND] = QColor(Qt::white);  // Plot Background
    add.colors[GThmCol::TOOLBARANDSIDEBAR] = QColor(0xec, 0xec, 0xec); // Toolbar and Sidebar Chrome
    add.colors[GThmCol::ACCENTCOLORMARKERS] = QColor(Qt::black); // Accent color (markers)
    add.colors[GThmCol::SELECTIONCOLOR] = QColor(Qt::green); // Selection color
    add.colors[GThmCol::CPWBALRIDECP] = QColor(Qt::red); // Critical Power and W'Bal
    add.colors[GThmCol::HEARTRATE] = QColor(Qt::red); // Heartrate
    add.colors[GThmCol::SPEED] = QColor(Qt::magenta); // Speed
    add.colors[GThmCol::POWER] = QColor(Qt::green); // Power
    add.colors[GThmCol::CADENCE] = QColor(Qt::blue); // Cadence
    add.colors[GThmCol::TORQUE] = QColor(Qt::darkGreen); // Torque
    add.colors[GThmCol::OVERVIEWBACKGROUND] = QColor(255, 255, 255); // Overview Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND1] = QColor(245, 245, 245); // Overview Tile Background
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND2] = QColor(245, 245, 245); // Overview Tile Background 2
    add.colors[GThmCol::OVERVIEWTILEBACKGROUND3] = QColor(245, 245, 245);// Overview Tile Background 3
    themes << add;

}

void
GCColor::applyTheme(int index) 
{
    QColor qColor;

    for (auto& colorElemt : colorTable) {

        qColor = determineThemeQColor(index, colorElemt.first);

        // theme applied !
        colorElemt.second.qColor = qColor;

        QString colorstring = QString("%1:%2:%3").arg(qColor.red())
            .arg(qColor.green())
            .arg(qColor.blue());
        appsettings->setValue(colorElemt.second.setting, colorstring);
    }

#ifdef Q_OS_MAC
    // if on yosemite we always set default chrome to #e5e5e5 and flat
    // TODO dgr
    qDebug() << "productVersion:" << QSysInfo::productVersion();
    if (QSysInfo::productVersion().toFloat() >= 12 | QSysInfo::productVersion().toFloat() < 13) {
        QColor color = QColor(0xe5,0xe5,0xe5);
        ColorList[CCHROME].color = color;
        QString colorstring = QString("%1:%2:%3").arg(color.red())
                                                 .arg(color.green())
                                                 .arg(color.blue());
        appsettings->setValue(ColorList[CCHROME].setting, colorstring);
    }
#endif
}

QColor
GCColor::determineThemeQColor(int index, const GCol& gColor) {

    QColor qColor;

    // now get the theme selected
    ColorTheme theme = Themes::inst()->themes[index];

    // apply theme to color
    switch (gColor) {

    case GCol::PLOTBACKGROUND:
    case GCol::RIDEPLOTBACKGROUND:
    case GCol::TRENDPLOTBACKGROUND:

        qColor = theme.colors[GThmCol::PLOTBACKGROUND]; // background color
        break;

    case GCol::TRAINPLOTBACKGROUND:
        // always, and I mean always default to a black background
        qColor = QColor(Qt::black);
        break;

    case GCol::CARDBACKGROUND:
        // set back to light black for dark themes
        // and gray for light themes
        qColor = theme.colors[GThmCol::OVERVIEWTILEBACKGROUND1];
        break;

    case GCol::CARDBACKGROUND2:
        // set back to light black for dark themes
        // and gray for light themes
        qColor = theme.colors[GThmCol::OVERVIEWTILEBACKGROUND2];
        break;

    case GCol::CARDBACKGROUND3:
        // set back to light black for dark themes
        // and gray for light themes
        qColor = theme.colors[GThmCol::OVERVIEWTILEBACKGROUND3];
        break;

    case GCol::OVERVIEWBACKGROUND:
        // set back to light black for dark themes
        // and gray for light themes
        qColor = theme.colors[GThmCol::OVERVIEWBACKGROUND];
        break;

    case GCol::CHROME:
    case GCol::CHARTBAR:
    case GCol::TOOLBAR:
        // we always keep them the same, but user can make different
        //  set to black for dark themese and grey for light themes
        qColor = theme.colors[GThmCol::TOOLBARANDSIDEBAR];
        break;

    case GCol::HOVER:
        // stealthy themes use overview card background for hover color since they are close
        // all other themes get a boring default
        //qColor = theme.stealth ? colorTable[gColor].qColor : (theme.dark ? QColor(50, 50, 50) : QColor(200, 200, 200));
        qColor = theme.stealth ? theme.colors[GThmCol::OVERVIEWTILEBACKGROUND1] : (theme.dark ? QColor(50, 50, 50) : QColor(200, 200, 200));
        break;

    case GCol::PLOTSYMBOL:
    case GCol::RIDEPLOTXAXIS:
    case GCol::RIDEPLOTYAXIS:
    case GCol::PLOTMARKER:
        qColor = theme.colors[GThmCol::ACCENTCOLORMARKERS];
        break;

    case GCol::PLOTSELECT:
    case GCol::PLOTTRACKER:
    case GCol::INTERVALHIGHLIGHTER:
        qColor = theme.colors[GThmCol::SELECTIONCOLOR];
        break;

    case GCol::PLOTGRID:
        // grid doesn't have a theme color
        // we make it barely distinguishable from background
        {
            QColor bg = theme.colors[GThmCol::PLOTBACKGROUND];
            qColor = (bg == QColor(Qt::black)) ? QColor(30, 30, 30) : bg.darker(110);
        }
        break;

    case GCol::CP:
    case GCol::WBAL:
    case GCol::RIDECP:
        qColor = theme.colors[GThmCol::CPWBALRIDECP];
        break;

    case GCol::HEARTRATE:
        qColor = theme.colors[GThmCol::HEARTRATE];
        break;

    case GCol::SPEED:
        qColor = theme.colors[GThmCol::SPEED];
        break;

    case GCol::POWER:
        qColor = theme.colors[GThmCol::POWER];
        break;

    case GCol::CADENCE:
        qColor = theme.colors[GThmCol::CADENCE];
        break;

    case GCol::TORQUE:
        qColor = theme.colors[GThmCol::TORQUE];
        break;

    default:
        qColor = (gColor < GCol::NUMOFCFGCOLORS) ? (theme.dark) ? colorTable[gColor].qDarkColor : colorTable[gColor].qLightColor : INVALIDGCOLOR;
    }

    return qColor;
}

//
// ColorLabel - just paints a swatch of the colors
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

    const double x = width() / theme.colors.size();
    const double y = height();

    // now do all the color blocks
    for (int i=0; i<theme.colors.size(); i++) {

        QRectF block (i*x,0,x,y);
        painter.fillRect(block, theme.colors[static_cast<GThmCol>(i)]);
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

