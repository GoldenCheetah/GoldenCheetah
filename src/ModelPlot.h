/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_ModelPlot_h
#define _GC_ModelPlot_h 1

#include <QtGui>
#include <QTimer>
#include "MainWindow.h"

#include <qwt3d_surfaceplot.h>
#include <qwt3d_function.h>
#include <qwt3d_color.h>
#include <qwt3d_colorlegend.h>
#include <qwt3d_types.h>

#define MODEL_NONE          0
#define MODEL_POWER         1
#define MODEL_CADENCE       2
#define MODEL_HEARTRATE     3
#define MODEL_SPEED         4
#define MODEL_ALT           5
#define MODEL_TORQUE        6
#define MODEL_TIME          7
#define MODEL_DISTANCE      8
#define MODEL_INTERVAL      9
#define MODEL_LAT           10
#define MODEL_LONG          11
#define MODEL_XYTIME        12
#define MODEL_POWERZONE     13
#define MODEL_CPV           14
#define MODEL_AEPF          15

using namespace Qwt3D;

// the data provider for the plot
class ModelDataProvider;
class ModelDataColor;
class ModelSettings;
class Bar;
class Water;

#define STYLE_BAR     1
#define STYLE_GRID    2
#define STYLE_SURFACE 3
#define STYLE_DOTS    4

// the core surface plot
class BasicModelPlot : public SurfacePlot
{
    Q_OBJECT

    public:
        BasicModelPlot(MainWindow *, ModelSettings *);
        void setData(ModelSettings *);
        void resetViewPoint();
        void setStyle(int);
        void setGrid(bool);
        void setLegend(bool, int);
        void setFrame(bool);
        void setZPane(int);

        ModelDataProvider *modelDataProvider; // used by enrichment

    public slots:
        void configChanged();

    protected:

        // passed from MainWindow
        MainWindow *main;
        ModelDataColor    *modelDataColor;

        Bar *bar;
        Water *water;
        Qwt3D::PLOTSTYLE surface;

        int currentStyle;
};


// an enrichment for the surface plot to show bars instead of a surface
class Bar : public Qwt3D::VertexEnrichment
{
public:
    Bar();

    Qwt3D::Enrichment* clone() const {return new Bar(*this);}

    void drawBegin();
    void drawEnd();
    void draw(Qwt3D::Triple const&);

private:
    double level_;
    //double diag_;
};

// an enrichment for all plot types to "drown" the plot in water
class Water : public Qwt3D::VertexEnrichment
{
    public:
        Water();
        Qwt3D::Enrichment* clone() const {return new Water(*this);}

        void drawBegin();
        void drawEnd();
        void draw(Qwt3D::Triple const&);
};

// just a frame containing the raw 3d plot (for now)
class ModelPlot : public QFrame
{
    Q_OBJECT

    public:


        ModelPlot(MainWindow *, ModelSettings *);
        void setData(ModelSettings *settings);
        void resetViewPoint();
        void setStyle(int);
        void setGrid(bool);
        void setLegend(bool, int);
        void setFrame(bool);
        void setZPane(int);

    public slots:
        void setResolution(int);

    private:
        MainWindow *main;
        QVBoxLayout *layout;
        BasicModelPlot *basicModelPlot;
};


#endif // _GC_ModelPlot_h
