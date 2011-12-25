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

static Colors ColorList[62], DefaultColorList[62];

static void copyArray(Colors source[], Colors target[])
{
    for (int i=0; source[i].name != ""; i++)
        target[i] = source[i];
}

static bool setupColors()
{
    // consider removing when we can guarantee extended initialisation support in gcc
    // (c++0x not supported by Qt currently and not planned for 4.8 or 5.0)
    Colors init[62] = {
        { "Plot Background", "COLORPLOTBACKGROUND", Qt::white },
        { "Ride Plot Background", "COLORRIDEPLOTBACKGROUND", Qt::black },
        { "Plot Thumbnail Background", "COLORPLOTTHUMBNAIL", Qt::gray },
        { "Plot Title", "COLORPLOTTITLE", Qt::black },
        { "Plot Selection Pen", "COLORPLOTSELECT", Qt::blue },
        { "Plot TrackerPen", "COLORPLOTTRACKER", Qt::blue },
        { "Plot Markers", "COLORPLOTMARKER", Qt::gray },
        { "Plot Grid", "COLORGRID", Qt::black },
        { "Interval Highlighter", "COLORINTERVALHIGHLIGHTER", Qt::blue },
        { "Heart Rate", "COLORHEARTRATE", Qt::blue },
        { "Speed", "COLORSPEED", Qt::green },
        { "Power", "COLORPOWER", Qt::red },
        { "Critical Power", "COLORCP", Qt::red },
        { "Cadence", "COLORCADENCE", QColor(0,204,204) },
        { "Altitude", "COLORALTITUTDE", QColor(124,91,31) },
        { "Altitude Shading", "COLORALTITUDESHADE", QColor(124,91,31) },
        { "Wind Speed", "COLORWINDSPEED", Qt::darkGreen },
        { "Torque", "COLORTORQUE", Qt::magenta },
        { "Short Term Stress", "COLORSTS", Qt::blue },
        { "Long Term Stress", "COLORLTS", Qt::green },
        { "Stress Balance", "COLORSB", Qt::black },
        { "Daily Stress", "COLORDAILYSTRESS", Qt::red },
        { "Calendar Text", "COLORCALENDARTEXT", Qt::black },
        { "Power Zone 1 Shading", "COLORZONE1", QColor(255,0,255) },
        { "Power Zone 2 Shading", "COLORZONE2", QColor(42,0,255) },
        { "Power Zone 3 Shading", "COLORZONE3", QColor(0,170,255) },
        { "Power Zone 4 Shading", "COLORZONE4", QColor(0,255,128) },
        { "Power Zone 5 Shading", "COLORZONE5", QColor(85,255,0) },
        { "Power Zone 6 Shading", "COLORZONE6", QColor(255,213,0) },
        { "Power Zone 7 Shading", "COLORZONE7", QColor(255,0,0) },
        { "Power Zone 8 Shading", "COLORZONE8", Qt::gray },
        { "Power Zone 9 Shading", "COLORZONE9", Qt::gray },
        { "Power Zone 10 Shading", "COLORZONE10", Qt::gray },
        { "HR Zone 1 Shading", "HRCOLORZONE1", QColor(255,0,255) },
        { "HR Zone 2 Shading", "HRCOLORZONE2", QColor(42,0,255) },
        { "HR Zone 3 Shading", "HRCOLORZONE3", QColor(0,170,255) },
        { "HR Zone 4 Shading", "HRCOLORZONE4", QColor(0,255,128) },
        { "HR Zone 5 Shading", "HRCOLORZONE5", QColor(85,255,0) },
        { "HR Zone 6 Shading", "HRCOLORZONE6", QColor(255,213,0) },
        { "HR Zone 7 Shading", "HRCOLORZONE7", QColor(255,0,0) },
        { "HR Zone 8 Shading", "HRCOLORZONE8", Qt::gray },
        { "HR Zone 9 Shading", "HRCOLORZONE9", Qt::gray },
        { "HR Zone 10 Shading", "HRCOLORZONE10", Qt::gray },
        { "Aerolab VE", "COLORAEROVE", Qt::blue },
        { "Aerolab Elevation", "COLORAEROEL", Qt::green },
        { "Calendar background", "CCALCELL", Qt::white },
        { "Calendar heading", "CCALHEAD", QColor(230,230,230) },
        { "Calendar Current Selection", "CCALCURRENT", Qt::darkBlue },
        { "Calendar Actual Workout", "CCALACTUAL", Qt::green },
        { "Calendar Planned Workout", "CCALPLANNED", Qt::yellow },
        { "Calendar Today", "CCALTODAY", Qt::cyan },
        { "Pop Up Windows Background", "CPOPUP", Qt::lightGray },
        { "Pop Up Windows Foreground", "CPOPUPTEXT", Qt::white },
        { "Chart Bar Unselected", "CTILEBAR", Qt::gray },
        { "Chart Bar Selected", "CTILEBARSELECT", Qt::yellow },
        { "ToolBar Background", "CTOOLBAR", Qt::white },
        { "Activity History Group", "CRIDEGROUP", QColor(236,246,255) },
        { "SpinScan Left", "CSPINSCANLEFT", Qt::gray },
        { "SpinScan Right", "CSPINSCANRIGHT", Qt::cyan },
        { "Temperature", "COLORTEMPERATURE", Qt::yellow },
        { "Default Dial Color", "CDIAL", Qt::gray },
        { "", "", QColor(0,0,0) },
    };

    copyArray(init, DefaultColorList);
    copyArray(init, ColorList);

    return true;
}

// initialise on startup
static bool setColors = setupColors();


GCColor::GCColor(MainWindow *main) : QObject(main)
{
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
