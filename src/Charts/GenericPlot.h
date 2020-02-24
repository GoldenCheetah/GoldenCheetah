/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_GenericPlot_h
#define _GC_GenericPlot_h 1

#include <QWidget>
#include <QPointF>
#include <QString>
#include <QDebug>
#include <QColor>
#include <QTextEdit>
#include <QScrollBar>
#include <QCheckBox>
#include <QSplitter>
#include <QByteArray>
#include <string.h>
#include <QtCharts>
#include <QGraphicsItem>
#include <QFontMetrics>
#include "Quadtree.h"

#include "GoldenCheetah.h"
#include "Settings.h"
#include "Context.h"
#include "Athlete.h"
#include "Colors.h"
#include "RCanvas.h"

// keep aligned to library.py
#define GC_CHART_LINE      1
#define GC_CHART_SCATTER   2
#define GC_CHART_BAR       3
#define GC_CHART_PIE       4

class GenericPlot;
class SelectionTool;
class GenericLegend;

class GenericLegendItem : public QWidget {

    Q_OBJECT

    public:
        GenericLegendItem(Context *context, QWidget *parent, QString name, QColor color);

    public slots:

        void paintEvent(QPaintEvent *event);
        void setValue(double p) { hasvalue=true; value=p; update(); } // set value to display
        void noValue() { hasvalue=false; update(); } // no value to display
        void configChanged(qint32); // context changed

    private:
        Context *context;
        QString name;
        QColor color;

        bool hasvalue;
        double value;

        // geometry for painting fast / updated on config changes
        QRectF blockrect, namerect, valuerect, linerect;

};

class GenericLegend : public QWidget {

    Q_OBJECT

    public:
        GenericLegend(Context *context, GenericPlot *parent);

        void addSeries(QString name, QAbstractSeries *series);
        void addX(QString name, QAbstractSeries *series);
        void removeSeries(QString name);
        void removeAllSeries();

    Q_SIGNALS:
        void clicked(QString name, bool enabled); // someone clicked on a legend and enabled/disabled it

    public slots:
        void hover(QPointF value, QString name, QAbstractSeries*series);
        void unhover(QString name);
        void unhoverx();

    private:
        // a label has a unique name, not directly tide to
        // a series or axis value, it depends...
        Context *context;
        GenericPlot *plot;
        QHBoxLayout *layout;
        QMap<QString,GenericLegendItem*> items;
        QString xname;
};

// general axis info
class AxisInfo {
public:
        enum axisinfoType { CONTINUOUS=0,                 // Continious range
                            DATERANGE=1,                  // Date
                            TIME=2,                       // Duration, Time
                            CATEGORY=3                // labelled with categories
                          };
        typedef enum axisinfoType AxisInfoType;

        AxisInfo(Qt::Orientations orientation, QString name) : name(name), orientation(orientation) {
            miny=maxy=minx=maxx=0;
            fixed=log=false;
            visible=minorgrid=majorgrid=true;
            type=CONTINUOUS;
            axiscolor=labelcolor=GColor(CPLOTMARKER);
        }

        void point(double x, double y) {
            if (fixed) return;
            if (x>maxx) maxx=x;
            if (x<minx) minx=x;
            if (y>maxy) maxy=y;
            if (y<miny) miny=y;
        }

        double min() {
            if (orientation == Qt::Horizontal) return minx;
            else return miny;
        }
        double max() {
            if (orientation == Qt::Horizontal) return maxx;
            else return maxy;
        }

        Qt::AlignmentFlag locate() {
            return align;
        }

        // series we are associated with
        QList<QAbstractSeries*> series;

        // data is all public to avoid tedious get/set
        QString name;
        Qt::Orientations orientation;
        Qt::AlignmentFlag align;
        double miny, maxy, minx, maxx; // updated as we see points, set the range
        bool visible,fixed, log, minorgrid, majorgrid; // settings
        QColor labelcolor, axiscolor; // aesthetics
        QStringList categories;
        AxisInfoType type; // what type of axis is this?
};

// for dynamic calculations
class Calculator
{
    public:
    Calculator() {}

    void initialise();
    void addPoint(QPointF);
    void finalise();

    int count;
    double m,b,r2; // r2 and y=mx +b
    double sumx2, sumxy; // used to resolve m and b
    struct {
        double max, min, sum, mean;
    } x,y;
    QColor color; // for paint

    // what were we calculated on? (so paint can transform)
    QAbstractAxis *xaxis,*yaxis;
    QAbstractSeries *series;
};

// hover points etc
struct SeriesPoint {
    QAbstractSeries *series;    // series this is a point for
    QPointF xy;                 // the actual xy value (not screen position)
};

// for watcing scene events
class SelectionTool : public QObject, public QGraphicsItem
{

    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

    friend class ::GenericPlot;

    public:
        SelectionTool(GenericPlot *);
        enum stateType { INACTIVE, SIZING, MOVING, DRAGGING, ACTIVE } state; // what state are we in?
        enum modeType { RECTANGLE, XRANGE, LASSOO, CIRCLE } mode; // what mode are we in?
        typedef modeType SelectionMode;
        typedef stateType SelectionState;

        // set mode
        void setMode(SelectionMode mode) { this->mode=mode;  }

        // is invisible and tiny. we are just an observer
        bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

        // we do nothing mate
        void paint(QPainter*, const QStyleOptionGraphicsItem *, QWidget*);
        QRectF boundingRect() const;

        // an interaction happened
        bool reset();
        bool clicked(QPointF);
        bool moved(QPointF);
        bool released(QPointF);
        bool wheel(int);

        // value coords
        double miny(QAbstractSeries*);
        double maxy(QAbstractSeries*);
        double minx(QAbstractSeries*);
        double maxx(QAbstractSeries*);

        // update the scene to reflect current state
        void updateScene();
        void resetSelections(); // clear out selections stuff

    public slots:
        void dragStart();

    Q_SIGNALS:
        void hover(QPointF value, QString name, QAbstractSeries*series); // mouse cursor is over a point on the chart
        void unhover(QString name); // mouse cursor is no longer over a point on the chart
        void unhoverx(); // when we aren't hovering on anything at all

    protected:
        QRectF rect;
        QPainterPath *lassoo; // when lassoing objects

    private:
        GenericPlot *host;
        QPointF start, startingpos, finish; // when calculating distances during transitions
        QPointF spos; // last point we saw

        // scatter does this xxx TODO refactor into hoverpoints
        QPointF hoverpoint;
        QAbstractSeries *hoverseries;
        // line plot uses this
        QList<SeriesPoint> hoverpoints;

        // selections from original during selection
        QMap<QAbstractSeries*, QAbstractSeries*> selections;
        QMap<QAbstractSeries*, Calculator> stats;
        QList<QAbstractSeries*> ignore; // temp series we add during selection to be ignored

        // timeout click and hold to drag
        QTimer drag;

};

// the chart
class GenericPlot : public QWidget {

    Q_OBJECT

    public:

        friend class ::SelectionTool;
        friend class ::GenericLegend;

        GenericPlot(QWidget *parent, Context *context);

        // some helper functions
        static QColor seriesColor(QAbstractSeries* series);

    public slots:
        void configChanged(qint32);

        // set chart settings
        bool initialiseChart(QString title, int type, bool animate);

        // add a curve, associating an axis
        bool addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int line, int symbol, int size, QString color, int opacity, bool opengl);

        // configure axis, after curves added
        bool configureAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories);

        // post processing clean up / add decorations / helpers etc
        void finaliseChart();

        // watching scene events and managing interaction
        bool eventHandler(int eventsource, void *obj, QEvent *event);

    protected:

        // legend and selector need acces to these
        QChartView *chartview;
        SelectionTool *selector;
        QChart *qchart;

        // trap widget events and pass to event handler
        bool eventFilter(QObject *, QEvent *e);

        // working with axes
        double min(QAbstractAxis*);
        double max(QAbstractAxis*);

        // the legend
        GenericLegend *legend;

        // quadtrees
        QMap<QAbstractSeries*, Quadtree*> quadtrees;

    private:
        Context *context;
        int charttype;

        // curves
        QMap<QString, QAbstractSeries *>curves;

        // axes
        QMap<QString, AxisInfo *>axisinfos;

        // barsets
        QList<QBarSet*> barsets;
        QBarSeries *barseries;

        // axis placement (before user interacts)
        // alternates as axis added
        bool left, bottom;
};


#endif
