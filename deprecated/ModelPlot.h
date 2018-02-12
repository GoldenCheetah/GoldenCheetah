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
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTimer>
#include <QLineEdit>
#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include "Context.h"

// sintegral fork of QwtPlot3D defines IS_NAN as isnan instead of std::isnan
// so we add a macro to fix that; this only applies when compiling on QT5.5.1
#if QT_VERSION >= 0x050501
#define isnan(x) std::isnan(x)
#endif

#include <qwt3d_global.h>

// qwtplot3d api changes between 0.2.x and 0.3.x
#if QWT3D_MINOR_VERSION > 2
#include <qwt3d_gridplot.h>
#include <qwt3d_plot3d.h>
#else
#include <qwt3d_surfaceplot.h>
#endif

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
#define MODEL_HEADWIND      16
#define MODEL_SLOPE         17
#define MODEL_TEMP          18
#define MODEL_LRBALANCE     19
#define MODEL_RV            22
#define MODEL_RCAD          23
#define MODEL_RGCT          24
#define MODEL_GEAR          25
#define MODEL_SMO2          26
#define MODEL_THB           27

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

#define SHOW_INTERVALS 1
#define SHOW_FRAME       2

// the core surface plot
// qwtplot3d api changes between 0.2.x and 0.3.x
#if QWT3D_MINOR_VERSION > 2
class ModelPlot : public GridPlot
#else
class ModelPlot : public SurfacePlot
#endif
{
    Q_OBJECT
    G_OBJECT


    public:
        ModelPlot(Context *, QWidget *parent, ModelSettings *p = NULL);

        Context *context;

        void setData(ModelSettings *);
        void resetViewPoint();
        void setStyle(int);
        void setGrid(bool);
        void setLegend(bool, int);
        void setFrame(bool);
        void setZPane(int);

        ModelDataProvider *modelDataProvider; // used by enrichment

        // used by the Bar Enrichment
        double diag_;
        int   intervals_;                // SHOW_INTERVALS | SHOW_MAX
        double zpane;
        QHash<QString, double> iz;         // for selected intervals
        QHash<QString, double> inum;      // for selected intervals

    public slots:
        void configChanged(qint32);

    protected:

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
    Bar(ModelPlot *);

    Qwt3D::Enrichment* clone() const {return new Bar(*this);}

    void drawBegin();
    void drawEnd();
    void draw(Qwt3D::Triple const&);

private:
    double level_;
    ModelPlot *model;
    //double diag_;
};

// an enrichment for all plot types to "drown" the plot in water
class Water : public Qwt3D::VertexEnrichment
{
    public:
        Water();
        Water(ModelPlot *);
        Qwt3D::Enrichment* clone() const {return new Water(*this);}

        void drawBegin();
        void drawEnd();
        void draw(Qwt3D::Triple const&);
        ModelPlot *model;
};

#endif // _GC_ModelPlot_h
