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
#include "MainWindow.h"
#include "RideMetadata.h"
#include <QObject>
#include <QDir>
#include "Settings.h"

// Number of confugurable metric colors + 1 for sentinel value
static Colors ColorList[CNUMOFCFGCOLORS+1], DefaultColorList[CNUMOFCFGCOLORS+1];

static void copyArray(Colors source[], Colors target[])
{
    for (int i=0; source[i].name != ""; i++)
        target[i] = source[i];
}

// initialization called from constructor to enable translation
void GCColor::setupColors()
{
    // consider removing when we can guarantee extended initialisation support in gcc
    // (c++0x not supported by Qt currently and not planned for 4.8 or 5.0)
    Colors init[CNUMOFCFGCOLORS+1] = {
        { tr("Plot Background"), "COLORPLOTBACKGROUND", Qt::white },
        { tr("Ride Plot Background"), "COLORRIDEPLOTBACKGROUND", Qt::black },
        { tr("Plot Symbols"), "COLORRIDEPLOTSYMBOLS", Qt::gray },
        { tr("Ride Plot X Axis"), "COLORRIDEPLOTXAXIS", Qt::blue },
        { tr("Ride Plot Y Axis"), "COLORRIDEPLOTYAXIS", Qt::red },
        { tr("Plot Thumbnail Background"), "COLORPLOTTHUMBNAIL", Qt::gray },
        { tr("Plot Title"), "COLORPLOTTITLE", Qt::black },
        { tr("Plot Selection Pen"), "COLORPLOTSELECT", Qt::blue },
        { tr("Plot TrackerPen"), "COLORPLOTTRACKER", Qt::blue },
        { tr("Plot Markers"), "COLORPLOTMARKER", Qt::gray },
        { tr("Plot Grid"), "COLORGRID", Qt::black },
        { tr("Interval Highlighter"), "COLORINTERVALHIGHLIGHTER", Qt::blue },
        { tr("Heart Rate"), "COLORHEARTRATE", Qt::blue },
        { tr("Speed"), "COLORSPEED", Qt::green },
        { tr("Power"), "COLORPOWER", Qt::red },
        { tr("Critical Power"), "COLORCP", Qt::red },
        { tr("Cadence"), "COLORCADENCE", QColor(0,204,204) },
        { tr("Altitude"), "COLORALTITUTDE", QColor(124,91,31) },
        { tr("Altitude Shading"), "COLORALTITUDESHADE", QColor(124,91,31) },
        { tr("Wind Speed"), "COLORWINDSPEED", Qt::darkGreen },
        { tr("Torque"), "COLORTORQUE", Qt::magenta },
        { tr("Load"), "COLORLOAD", Qt::yellow },
        { tr("TSS"), "COLORTSS", Qt::green },
        { tr("Short Term Stress"), "COLORSTS", Qt::blue },
        { tr("Long Term Stress"), "COLORLTS", Qt::green },
        { tr("Stress Balance"), "COLORSB", Qt::black },
        { tr("Daily Stress"), "COLORDAILYSTRESS", Qt::red },
        { tr("Bike Score"), "COLORBIKESCORE", Qt::gray },
        { tr("Calendar Text"), "COLORCALENDARTEXT", Qt::black },
        { tr("Power Zone 1 Shading"), "COLORZONE1", QColor(255,0,255) },
        { tr("Power Zone 2 Shading"), "COLORZONE2", QColor(42,0,255) },
        { tr("Power Zone 3 Shading"), "COLORZONE3", QColor(0,170,255) },
        { tr("Power Zone 4 Shading"), "COLORZONE4", QColor(0,255,128) },
        { tr("Power Zone 5 Shading"), "COLORZONE5", QColor(85,255,0) },
        { tr("Power Zone 6 Shading"), "COLORZONE6", QColor(255,213,0) },
        { tr("Power Zone 7 Shading"), "COLORZONE7", QColor(255,0,0) },
        { tr("Power Zone 8 Shading"), "COLORZONE8", Qt::gray },
        { tr("Power Zone 9 Shading"), "COLORZONE9", Qt::gray },
        { tr("Power Zone 10 Shading"), "COLORZONE10", Qt::gray },
        { tr("HR Zone 1 Shading"), "HRCOLORZONE1", QColor(255,0,255) },
        { tr("HR Zone 2 Shading"), "HRCOLORZONE2", QColor(42,0,255) },
        { tr("HR Zone 3 Shading"), "HRCOLORZONE3", QColor(0,170,255) },
        { tr("HR Zone 4 Shading"), "HRCOLORZONE4", QColor(0,255,128) },
        { tr("HR Zone 5 Shading"), "HRCOLORZONE5", QColor(85,255,0) },
        { tr("HR Zone 6 Shading"), "HRCOLORZONE6", QColor(255,213,0) },
        { tr("HR Zone 7 Shading"), "HRCOLORZONE7", QColor(255,0,0) },
        { tr("HR Zone 8 Shading"), "HRCOLORZONE8", Qt::gray },
        { tr("HR Zone 9 Shading"), "HRCOLORZONE9", Qt::gray },
        { tr("HR Zone 10 Shading"), "HRCOLORZONE10", Qt::gray },
        { tr("Aerolab VE"), "COLORAEROVE", Qt::blue },
        { tr("Aerolab Elevation"), "COLORAEROEL", Qt::green },
        { tr("Calendar background"), "CCALCELL", Qt::white },
        { tr("Calendar heading"), "CCALHEAD", QColor(230,230,230) },
        { tr("Calendar Current Selection"), "CCALCURRENT", Qt::darkBlue },
        { tr("Calendar Actual Workout"), "CCALACTUAL", Qt::green },
        { tr("Calendar Planned Workout"), "CCALPLANNED", Qt::yellow },
        { tr("Calendar Today"), "CCALTODAY", Qt::cyan },
        { tr("Pop Up Windows Background"), "CPOPUP", Qt::lightGray },
        { tr("Pop Up Windows Foreground"), "CPOPUPTEXT", Qt::white },
        { tr("Chart Bar Unselected"), "CTILEBAR", Qt::gray },
        { tr("Chart Bar Selected"), "CTILEBARSELECT", Qt::yellow },
        { tr("ToolBar Background"), "CTOOLBAR", Qt::white },
        { tr("Activity History Group"), "CRIDEGROUP", QColor(236,246,255) },
        { tr("SpinScan Left"), "CSPINSCANLEFT", Qt::gray },
        { tr("SpinScan Right"), "CSPINSCANRIGHT", Qt::cyan },
        { tr("Temperature"), "COLORTEMPERATURE", Qt::yellow },
        { tr("Default Dial Color"), "CDIAL", Qt::gray },
        { tr("Alternate Power"), "CALTPOWER", Qt::magenta },
        { tr("Left Balance"), "CBALANCELEFT", QColor(178,0,0) },
        { tr("Right Balance"), "CBALANCERIGHT", QColor(128,0,50) },
        { "", "", QColor(0,0,0) },
    };

    copyArray(init, DefaultColorList);
    copyArray(init, ColorList);
}

// default settings for fonts etc
// we err on the side of caution -- smaller is better
struct SizeSettings defaultAppearance[] ={

    // small screens include netbooks and old vga 800x600, 1024x768
    { 1024, 768,  8,8,6,6,6,6,    800, 600 },

    // medium screen size includes typical 16:9 pc formats and TV screens
    { 1280, 800,  8,8,6,6,6,6,    800, 600},

    // high resolution screens 
    { 1650, 1080,  10,10,8,8,8,8,   1024,650 },

    // very big panels, incl. e.g.  mac 27"
    { 9999, 9999,  10,10,8,8,8,8,   1280,700 },

    { 0,0,0,0,0,0,0,0,0,0 },
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

GCColor::GCColor(MainWindow *main) : QObject(main)
{
    setupColors();
    readConfig();
    connect(main, SIGNAL(configChanged()), this, SLOT(readConfig()));
}

const Colors * GCColor::colorSet()
{
    return ColorList;
}

const Colors * GCColor::defaultColorSet()
{
    return DefaultColorList;
}

void GCColor::resetColors()
{
    copyArray(DefaultColorList, ColorList);
}

QColor
GCColor::invert(QColor color)
{
    return QColor(255-color.red(), 255-color.green(), 255-color.blue());
}

void
GCColor::readConfig()
{
    // read in config settings and populate the color table
    for (unsigned int i=0; ColorList[i].name != ""; i++) {
        QString colortext = appsettings->value(this, ColorList[i].setting, "").toString();
        if (colortext != "") {
            // color definitions are stored as "r:g:b"
            QStringList rgb = colortext.split(":");
            ColorList[i].color = QColor(rgb[0].toInt(),
                                        rgb[1].toInt(),
                                        rgb[2].toInt());
        } else {
            // set sensible defaults for any not set...
            if (ColorList[i].name == "CTOOLBAR") {
                QPalette def;
                ColorList[i].color = def.color(QPalette::Window);
            }
        }
    }
}

QColor
GCColor::getColor(int colornum)
{
    return ColorList[colornum].color;
}

ColorEngine::ColorEngine(MainWindow* main) : QObject(main), defaultColor(QColor(Qt::white)), mainWindow(main)
{
    configUpdate();
    connect(mainWindow, SIGNAL(configChanged()), this, SLOT(configUpdate()));
}

void ColorEngine::configUpdate()
{
    // clear existing
    workoutCodes.clear();

    // setup the keyword/color combinations from config settings
    foreach (KeywordDefinition keyword, mainWindow->rideMetadata()->getKeywords()) {
        if (keyword.name == "Default")
            defaultColor = keyword.color;
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
    QColor color = defaultColor;

    foreach(QString code, workoutCodes.keys()) {
        if (text.contains(code, Qt::CaseInsensitive)) {
           color = workoutCodes[code];
        }
    }
    return color;
}
