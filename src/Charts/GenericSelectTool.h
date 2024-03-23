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

#ifndef _GC_GenericSelectTool_h
#define _GC_GenericSelectTool_h 1

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

#include "GenericPlot.h"

class GenericPlot;
class GenericLegend;

// for dynamic calculations
class GenericCalculator
{
    public:
    GenericCalculator() { initialise(); }

    void initialise();
    void addPoint(QPointF);
    void finalise();

    int count;
    double m,b,r2,see; // r2 and y=mx +b
    double lrsumx2, lrsumxy; // used to resolve m and b
    struct {
        double max, min, sum, mean;
        double lrsum;
    } x,y;
    QColor color; // for paint

    QVector<QPointF> actual;// keep track of data for finalise to use

    // what were we calculated on? (so paint can transform)
    QAbstractAxis *xaxis,*yaxis;
    QAbstractSeries *series;
    QDateTime midnight; // used for transform time to seconds
};

// hover points etc
struct SeriesPoint {
    QAbstractSeries *series;    // series this is a point for
    QPointF xy;                 // the actual xy value (not screen position)
};

// for watcing scene events
class GenericSelectTool : public QObject, public QGraphicsItem
{

    Q_OBJECT
    Q_INTERFACES(QGraphicsItem)

    static constexpr double gl_linemarker = 7;
    static constexpr double gl_scattermarker = 10;

    friend class ::GenericPlot;

    public:
        GenericSelectTool(GenericPlot *);
        enum stateType { INACTIVE, SIZING, MOVING, DRAGGING, ACTIVE } state; // what state are we in?
        enum modeType { RECTANGLE, XRANGE, LASSOO, CIRCLE } mode; // what mode are we in?
        typedef modeType SelectionMode;
        typedef stateType SelectionState;

        // set mode
        void setMode(SelectionMode mode) { this->mode=mode;  }

        // some series was shown or hidden...
        void setSeriesVisible(QString name, bool visible);

        // check cursor and isolate/unisolate
        bool isolateAxes(QPointF scenepos);

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
        bool seriesClicked();

    Q_SIGNALS:
        void seriesClicked(QAbstractSeries*,GPointF);
        void hover(GPointF value, QString name, QAbstractSeries*series); // mouse cursor is over a point on the chart
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
        GPointF hoverpoint;
        QAbstractSeries *hoverseries;

        // line plot uses this
        QList<SeriesPoint> hoverpoints;

        // axis we are hovering over?
        QAbstractAxis *hoveraxis;
        QList<QAbstractSeries*> hidden; // hidden by isolate axis

        // selections from original during selection
        QMap<QAbstractSeries*, QAbstractSeries*> selections;
        QMap<QAbstractSeries*, GenericCalculator> stats;
        QList<QAbstractSeries*> ignore; // temp series we add during selection to be ignored

        // timeout click and hold to drag
        QTimer drag;

        // did selection change on last event?
        bool rectchanged;

};

#endif
