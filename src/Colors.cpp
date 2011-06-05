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
#include <QObject>
#include <QDir>
#include "Settings.h"


static Colors *ColorList; // Initialization moved to GCColor constructor to enable translation

GCColor::GCColor(MainWindow *main) : QObject(main)
{
    static Colors ColorListInit[45] = {
        { tr("Plot Background"), "COLORPLOTBACKGROUND", Qt::white },
        { tr("Plot Thumbnail Background"), "COLORPLOTTHUMBNAIL", Qt::gray },
        { tr("Plot Title"), "COLORPLOTTITLE", Qt::black },
        { tr("Plot Selection Pen"), "COLORPLOTSELECT", Qt::blue },
        { tr("Plot TrackerPen"), "COLORPLOTTRACKER", Qt::blue },
        { tr("Plot Markers"), "COLORPLOTMARKER", Qt::black },
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
        { tr("Short Term Stress"), "COLORSTS", Qt::blue },
        { tr("Long Term Stress"), "COLORLTS", Qt::green },
        { tr("Stress Balance"), "COLORSB", Qt::black },
        { tr("Daily Stress"), "COLORDAILYSTRESS", Qt::red },
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
        { tr("Heartrate Zone 1 Shading"), "COLORHRZONE1", QColor(255,0,255) },
        { tr("Heartrate Zone 2 Shading"), "COLORHRZONE2", QColor(42,0,255) },
        { tr("Heartrate Zone 3 Shading"), "COLORHRZONE3", QColor(0,170,255) },
        { tr("Heartrate Zone 4 Shading"), "COLORHRZONE4", QColor(0,255,128) },
        { tr("Heartrate Zone 5 Shading"), "COLORHRZONE5", QColor(85,255,0) },
        { tr("Heartrate Zone 6 Shading"), "COLORHRZONE6", QColor(255,213,0) },
        { tr("Heartrate Zone 7 Shading"), "COLORHRZONE7", QColor(255,0,0) },
        { tr("Heartrate Zone 8 Shading"), "COLORHRZONE8", Qt::gray },
        { tr("Heartrate Zone 9 Shading"), "COLORHRZONE9", Qt::gray },
        { tr("Heartrate Zone 10 Shading"), "COLORHRZONE10", Qt::gray },
        { tr("Aerolab VE"), "COLORAEROVE", Qt::blue },
        { tr("Aerolab Elevation"), "COLORAEROEL", Qt::green },
        { "", "", QColor(0,0,0) },
    };
    ColorList = ColorListInit;
    readConfig();
    connect(main, SIGNAL(configChanged()), this, SLOT(readConfig()));
}

const Colors * GCColor::colorSet()
{
    return ColorList;
}

QColor
GCColor::invert(QColor color)
{
    return QColor(255-color.red(), 255-color.green(), 255-color.blue());
}

void
GCColor::readConfig()
{
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    // read in config settings and populate the color table
    for (unsigned int i=0; ColorList[i].name != ""; i++) {
        QString colortext = settings->value(ColorList[i].setting, "").toString();
        if (colortext != "") {
            // color definitions are stored as "r:g:b"
            QStringList rgb = colortext.split(":");
            ColorList[i].color = QColor(rgb[0].toInt(),
                                        rgb[1].toInt(),
                                        rgb[2].toInt());
        }
    }
}

QColor
GCColor::getColor(int colornum)
{
    return ColorList[colornum].color;
}
