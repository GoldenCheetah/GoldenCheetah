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
#include <QDir>
#include "Settings.h"

// the standard themes, a global object
static Themes allThemes;

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
        { tr("Plot Background"), "COLORPLOTBACKGROUND", QColor(52,52,52) },
        { tr("Ride Plot Background"), "COLORRIDEPLOTBACKGROUND", QColor(52,52,52) },
        { tr("Train Plot Background"), "COLORTRAINPLOTBACKGROUND", Qt::black },
        { tr("Plot Symbols"), "COLORRIDEPLOTSYMBOLS", Qt::cyan },
        { tr("Ride Plot X Axis"), "COLORRIDEPLOTXAXIS", Qt::blue },
        { tr("Ride Plot Y Axis"), "COLORRIDEPLOTYAXIS", Qt::red },
        { tr("Plot Thumbnail Background"), "COLORPLOTTHUMBNAIL", Qt::gray },
        { tr("Plot Title"), "COLORPLOTTITLE", Qt::black },
        { tr("Plot Selection Pen"), "COLORPLOTSELECT", Qt::blue },
        { tr("Plot TrackerPen"), "COLORPLOTTRACKER", Qt::blue },
        { tr("Plot Markers"), "COLORPLOTMARKER", Qt::cyan },
        { tr("Plot Grid"), "COLORGRID", QColor(65,65,65) },
        { tr("Interval Highlighter"), "COLORINTERVALHIGHLIGHTER", Qt::blue },
        { tr("Heart Rate"), "COLORHEARTRATE", Qt::red },
        { tr("Speed"), "COLORSPEED", Qt::green },
        { tr("Acceleration"), "COLORACCEL", Qt::cyan },
        { tr("Power"), "COLORPOWER", Qt::yellow },
        { tr("Normalised Power"), "CNPOWER", Qt::magenta },
        { tr("Skiba xPower"), "CXPOWER", Qt::magenta },
        { tr("Altitude Power"), "CAPOWER", Qt::magenta },
        { tr("Critical Power"), "COLORCP", Qt::cyan },
        { tr("Cadence"), "COLORCADENCE", QColor(0,204,204) },
        { tr("Altitude"), "COLORALTITUTDE", QColor(Qt::gray) },
        { tr("Altitude Shading"), "COLORALTITUDESHADE", QColor(Qt::lightGray) },
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
        { tr("Calendar Current Selection"), "CCALCURRENT", QColor(255,213,0) },
        { tr("Calendar Actual Workout"), "CCALACTUAL", Qt::green },
        { tr("Calendar Planned Workout"), "CCALPLANNED", Qt::yellow },
        { tr("Calendar Today"), "CCALTODAY", Qt::cyan },
        { tr("Pop Up Windows Background"), "CPOPUP", Qt::lightGray },
        { tr("Pop Up Windows Foreground"), "CPOPUPTEXT", Qt::white },
        { tr("Chart Bar Unselected"), "CTILEBAR", Qt::gray },
        { tr("Chart Bar Selected"), "CTILEBARSELECT", Qt::yellow },
        { tr("ToolBar Background"), "CTOOLBAR", Qt::white },
        { tr("Ride History Group"), "CRIDEGROUP", QColor(236,246,255) },
        { tr("SpinScan Left"), "CSPINSCANLEFT", Qt::gray },
        { tr("SpinScan Right"), "CSPINSCANRIGHT", Qt::cyan },
        { tr("Temperature"), "COLORTEMPERATURE", Qt::yellow },
        { tr("Default Dial Color"), "CDIAL", Qt::gray },
        { tr("Alternate Power"), "CALTPOWER", Qt::magenta },
        { tr("Left Balance"), "CBALANCELEFT", QColor(178,0,0) },
        { tr("Right Balance"), "CBALANCERIGHT", QColor(128,0,50) },
        { tr("W' Balance"), "CWBAL", Qt::red },
        { tr("Ride CP Curve"), "CRIDECP", Qt::red },
        { tr("Aerobic TISS"), "CATISS", Qt::magenta },
        { tr("Anaerobic TISS"), "CANTISS", Qt::cyan },
        { tr("Left Torque Effectiveness"), "CLTE", Qt::cyan },
        { tr("Right Torque Effectiveness"), "CRTE", Qt::magenta },
        { tr("Left Pedal Smoothness"), "CLPS", Qt::cyan },
        { tr("Right Pedal Smoothness"), "CRPS", Qt::magenta },
        { "", "", QColor(0,0,0) },
    };

    // set the defaults to system detaults
    init[CCALCURRENT].color = QPalette().color(QPalette::Highlight);
    init[CTOOLBAR].color = QPalette().color(QPalette::Window);

    copyArray(init, DefaultColorList);
    copyArray(init, ColorList);
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

GCColor::GCColor(Context *context) : QObject(context)
{
    setupColors();
    readConfig();
    connect(context, SIGNAL(configChanged()), this, SLOT(readConfig()));
}

QColor GCColor::invertColor(QColor bgColor)
{
    QColor cRGB = bgColor.convertTo(QColor::Rgb);
    // lets work it out..
    int r = cRGB.red() < 128 ? 255 : 0;
    int g = cRGB.green() < 128 ? 255 : 0;
    int b = cRGB.blue() < 128 ? 255 : 0;
    return QColor(r,g,b);
}

QColor GCColor::alternateColor(QColor bgColor)
{
    //QColor color = QColor::fromHsv(bgColor.hue(), bgColor.saturation() * 2, bgColor.value());
    if (bgColor.value() < 128)
        return QColor(Qt::darkGray);
    else
        return QColor(Qt::lightGray);
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
            if (ColorList[i].name == "CCALCURRENT") {
                QPalette def;
                ColorList[i].color = def.color(QPalette::Highlight);

            }
        }
    }
}

QColor
GCColor::getColor(int colornum)
{
    return ColorList[colornum].color;
}

Themes &
GCColor::themes()
{
    return allThemes;
}

ColorEngine::ColorEngine(Context* context) : QObject(context), defaultColor(QColor(Qt::white)), context(context)
{
    configUpdate();
    connect(context, SIGNAL(configChanged()), this, SLOT(configUpdate()));
}

void ColorEngine::configUpdate()
{
    // clear existing
    workoutCodes.clear();

    // setup the keyword/color combinations from config settings
    foreach (KeywordDefinition keyword, context->athlete->rideMetadata()->getKeywords()) {
        if (keyword.name == "Default")
            defaultColor = keyword.color; // we actually ignore this now
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
GCColor::css()
{
    QColor bgColor = GColor(CPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);
    //QColor altColor = GCColor::alternateColor(bgColor); // not yet ?

    return QString("<style> "
                   "body { color: %3; background-color: %2; }"
                   "h1 { color: %1; background-color: %2; } "
                   "h2 { color: %1; background-color: %2; } "
                   "h3 { color: %1; background-color: %2; } "
                   "h4 { color: %1; background-color: %2; } "
                   "b { color: %1; background-color: %2; } "
                   "</style> ").arg(GColor(CPLOTMARKER).name())
                               .arg(bgColor.name())
                               .arg(fgColor.name());
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
GCColor::stylesheet()
{
    // make it to order to reflect current config
    QColor bgColor = GColor(CPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);
    return QString("QTreeView { color: %2; background: %1; }"
                   "QTableWidget { color: %2; background: %1; }"
                   "QTableWidget::item:hover { color: black; background: lightGray; }"
                   "QTreeView::item:hover { color: black; background: lightGray; }").arg(bgColor.name()).arg(fgColor.name());
}

//
// Themes
//

Themes::Themes()
{
    // initialise the array of themes, lets just start with a compiled in list
    QList<QColor> colors;
    ColorTheme add("", QList<QColor>());

    //
    // Add all the standard themes
    //
    add.name = tr("Default"); // New v3.1 default colors // ** DARK **
    colors << QColor(52,52,52) << QColor(Qt::white) << QColor(Qt::cyan) << QColor(Qt::blue) << QColor(Qt::red);
    //            HR              Speed                Power                 Cadence             Torque
    colors << QColor(Qt::red) << QColor(Qt::green) << QColor(Qt::yellow) << QColor(0,204,204) << QColor(Qt::magenta) ;
    add.colors = colors;
    themes << add;
    colors.clear();


    // now some popular combos from Kueler
    add.name = tr("Neutral Blue"); // ** DARK **
    colors << QColor(25,52,65) << QColor(252,255,245) << QColor(209,219,189) << QColor(145,170,157) << QColor(62,96,188);
    //            HR              Speed                Power                 Cadence             Torque
    colors << QColor(Qt::red) << QColor(Qt::green) << QColor(Qt::yellow) << QColor(0,204,204) << QColor(Qt::magenta) ;
    add.colors = colors;
    themes << add;
    colors.clear();

    add.name = tr("Firenze"); // ** LIGHT **
    colors << QColor(255,240,165) << QColor(Qt::darkGray) << QColor(70,137,102) << QColor(182,73,38) << QColor(142,40,0);
    //            HR              Speed                Power                 Cadence             Torque
    colors << QColor(Qt::red) << QColor(Qt::green) << QColor(85,90,127) << QColor(0,204,204) << QColor(Qt::magenta) ;
    add.colors = colors;
    themes << add;
    colors.clear();

    add.name = tr("Mustang"); // ** DARK **
    colors << QColor(0,0,0) << QColor(255,255,255) << QColor(255,152,0) << QColor(Qt::white) << QColor(126,138,162);
    //            HR              Speed                Power                 Cadence             Torque
    colors << QColor(Qt::red) << QColor(Qt::green) << QColor(Qt::yellow) << QColor(0,204,204) << QColor(Qt::magenta) ;
    add.colors = colors;
    themes << add;
    colors.clear();

    add.name = tr("Japanese Garden"); // ** DARK **
    colors << QColor(56,37,19) << QColor(216,202,168) << QColor(92,131,47) << QColor(54,57,66) << QColor(40,73,7);
    //            HR              Speed                Power                 Cadence             Torque
    colors << QColor(Qt::red) << QColor(Qt::green) << QColor(Qt::yellow) << QColor(0,204,204) << QColor(Qt::magenta) ;
    add.colors = colors;
    themes << add;
    colors.clear();

    add.name = tr("Zen and Tea"); // ** DARK **
    colors << QColor(246,255,224) << QColor(149,171,99) << QColor(16,34,43) << QColor(226,240,214) << QColor(189,214,132);
    //            HR              Speed                Power                 Cadence             Torque
    colors << QColor(Qt::red) << QColor(Qt::green) << QColor(Qt::yellow) << QColor(0,204,204) << QColor(Qt::magenta) ;
    add.colors = colors;
    themes << add;
    colors.clear();

    add.name = tr("Mono (dark)"); // New v3.1 default colors // ** DARK **
    colors << QColor(Qt::black) << QColor(Qt::white) << QColor(Qt::white) << QColor(Qt::white) << QColor(Qt::white);
    //            HR              Speed                Power                 Cadence             Torque
    colors << QColor(Qt::red) << QColor(Qt::green) << QColor(Qt::yellow) << QColor(0,204,204) << QColor(Qt::magenta) ;
    add.colors = colors;
    themes << add;
    colors.clear();

    add.name = tr("Mono (light)"); // New v3.1 default colors // ** LIGHT **
    colors << QColor(Qt::white) << QColor(Qt::black) << QColor(Qt::black) << QColor(Qt::black) << QColor(Qt::black);
    //            HR              Speed                Power                 Cadence             Torque
    colors << QColor(Qt::red) << QColor(Qt::green) << QColor(Qt::black) << QColor(0,204,204) << QColor(Qt::magenta) ;
    add.colors = colors;
    themes << add;
    colors.clear();

    // we can add more later ....
    add.name = tr("Schoberer"); // Old GoldenCheetah colors // ** LIGHT **
    colors << QColor(Qt::white) << QColor(Qt::darkGray) << QColor(Qt::black) << QColor(Qt::green) << QColor(Qt::red);
    //            HR              Speed                Power                 Cadence             Torque
    colors << QColor(Qt::red) << QColor(Qt::magenta) << QColor(Qt::green) << QColor(Qt::blue) << QColor(Qt::darkGreen) ;
    add.colors = colors;
    themes << add;
    colors.clear();

    // we can add more later ....
    add.name = tr("Classic"); // Old GoldenCheetah colors // ** LIGHT **
    colors << QColor(Qt::white) << QColor(Qt::black) << QColor(Qt::darkRed) << QColor(Qt::blue) << QColor(Qt::red);
    //            HR              Speed                Power                 Cadence             Torque
    colors << QColor(Qt::red) << QColor(Qt::green) << QColor(Qt::blue) << QColor(0,204,204) << QColor(Qt::magenta) ;
    add.colors = colors;
    themes << add;
    colors.clear();

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
