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

#include <QObject>
#include <QString>
#include <QColor>

class MainWindow;

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
        calendarFont,
        popupFont;

    // screen dimension
    int width,
        height;
};

extern SizeSettings defaultAppearance[];
class Colors
{
public:
        QString name,
                setting;
        QColor  color;
};

class GCColor : public QObject
{
    Q_OBJECT
    G_OBJECT

        void setupColors();
    public:
        GCColor(MainWindow*);
        static QColor getColor(int);
        static const Colors *colorSet();
        static const Colors *defaultColorSet();
        static void resetColors();
        static QColor invert(QColor);
        static struct SizeSettings defaultSizes(int width, int height);

    public slots:
        void readConfig();
};

// return a color for a ride file
class ColorEngine : public QObject
{
    Q_OBJECT
    G_OBJECT

    public:
        ColorEngine(MainWindow*);

        QColor colorFor(QString);

    public slots:
        void configUpdate();

    private:
        QMap<QString, QColor> workoutCodes;
        QColor defaultColor;
        MainWindow *mainWindow;
};


// shorthand
#define GColor(x) GCColor::getColor(x)

// Define how many cconfigurable metric colors are available
#define CNUMOFCFGCOLORS       69

#define CPLOTBACKGROUND       0
#define CRIDEPLOTBACKGROUND   1
#define CRIDEPLOTXAXIS        2
#define CRIDEPLOTYAXIS        3
#define CPLOTTHUMBNAIL        4
#define CPLOTTITLE            5
#define CPLOTSELECT           6
#define CPLOTTRACKER          7
#define CPLOTMARKER           8
#define CPLOTGRID             9
#define CINTERVALHIGHLIGHTER  10
#define CHEARTRATE            11
#define CSPEED                12
#define CPOWER                13
#define CCP                   14
#define CCADENCE              15
#define CALTITUDE             16
#define CALTITUDEBRUSH        17
#define CWINDSPEED            18
#define CTORQUE               19
#define CLOAD                 20
#define CTSS                  21
#define CSTS                  22
#define CLTS                  23
#define CSB                   24
#define CDAILYSTRESS          25
#define CBIKESCORE            26
#define CCALENDARTEXT         27
#define CZONE1                28
#define CZONE2                29
#define CZONE3                30
#define CZONE4                31
#define CZONE5                32
#define CZONE6                33
#define CZONE7                34
#define CZONE8                35
#define CZONE9                36
#define CZONE10               37
#define CHZONE1               38
#define CHZONE2               39
#define CHZONE3               40
#define CHZONE4               41
#define CHZONE5               42
#define CHZONE6               43
#define CHZONE7               44
#define CHZONE8               45
#define CHZONE9               46
#define CHZONE10              47
#define CAEROVE               48
#define CAEROEL               49
#define CCALCELL              50
#define CCALHEAD              51
#define CCALCURRENT           52
#define CCALACTUAL            53
#define CCALPLANNED           54
#define CCALTODAY             55
#define CPOPUP                56
#define CPOPUPTEXT            57
#define CTILEBAR              58
#define CTILEBARSELECT        59
#define CTOOLBAR              60
#define CRIDEGROUP            61
#define CSPINSCANLEFT         62
#define CSPINSCANRIGHT        63
#define CTEMP                 64
#define CDIAL                 65
#define CALTPOWER             66
#define CBALANCELEFT          67
#define CBALANCERIGHT         68

#endif
