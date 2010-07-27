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

static Colors ColorList[35] = {
    { "Plot Background", "COLORPLOTBACKGROUND", Qt::white },
    { "Plot Thumbnail Background", "COLORPLOTTHUMBNAIL", Qt::gray },
    { "Plot Title", "COLORPLOTTITLE", Qt::black },
    { "Plot Selection Pen", "COLORPLOTSELECT", Qt::blue },
    { "Plot TrackerPen", "COLORPLOTTRACKER", Qt::blue },
    { "Plot Markers", "COLORPLOTMARKER", Qt::black },
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
    { "Aerolab VE", "COLORAEROVE", Qt::blue },
    { "Aerolab Elevation", "COLORAEROEL", Qt::green },
    { "", "", QColor(0,0,0) },
};


GCColor::GCColor(MainWindow *main) : QObject(main)
{
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
