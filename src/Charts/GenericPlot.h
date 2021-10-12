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

#include "GenericSelectTool.h"
#include "GenericLegend.h"
#include "GenericAnnotations.h"
#include "GenericChart.h"

// keep aligned to library.py
#define GC_CHART_LINE      1
#define GC_CHART_SCATTER   2
#define GC_CHART_BAR       3
#define GC_CHART_PIE       4
#define GC_CHART_STACK     5 // stacked bar
#define GC_CHART_PERCENT   6 // stacked percentage

class GenericPlot;
class GenericLegend;
class GenericSelectTool;
class GenericAxisInfo;
class GenericAnnotationInfo;

// the chart
class ChartSpace;
class GenericPlot : public QWidget {

    Q_OBJECT

    public:

        static constexpr double gl_minheight = 225;
        static QString gl_dateformat;
        static QString gl_timeformat;

        friend class GenericSelectTool;
        friend class GenericAnnotationController;
        friend class GenericLines;
        friend class GenericLegend;

        GenericPlot(QWidget *parent, Context *context, QGraphicsItem *item);

        // some helper functions
        static QColor seriesColor(QAbstractSeries* series);

        enum annotationType { LINE=0,                 // Continious range
                              RECTANGLE=1,
                              TEXT=2,
                              LABEL=3
                          };
        typedef enum annotationType AnnotationType;

        double scale() const { return scale_; }

    public slots:

        void configChanged(qint32);

        // background color
        QColor backgroundColor() { return bgcolor_; }
        void setBackgroundColor(QColor bgcolor);

        // set chart settings
        bool initialiseChart(QString title, int type, bool animate, int legendpos, double scale=1.0f);

        // add a curve, associating an axis
        bool addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QVector<QString> fseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int line, int symbol, int size, QString color, int opacity, bool opengl, bool legend, bool datalabels, bool fill);

        // adding annotations
        void addAnnotation(AnnotationType, QAbstractSeries*, double yvalue); // LINE
        void addAnnotation(AnnotationType, QString, QColor=QColor(Qt::gray)); // LABEL
        void addVoronoi(QString name, QVector<double>, QVector<double>); // VORONOI

        // configure axis, after curves added
        bool configureAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories);

        // post processing clean up / add decorations / helpers etc
        void finaliseChart();

        // do we want to see this series?
        void setSeriesVisible(QString name, bool visible);

        // adding and clearing a voronoi diagram from the chart
        void clearVoronoi();
        void plotVoronoi();

        // watching scene events and managing interaction
        void seriesClicked(QAbstractSeries*series, GPointF point);
        bool eventHandler(int eventsource, void *obj, QEvent *event);
        void barsetHover(bool status, int index, QBarSet *barset);
        void plotAreaChanged();
        void pieHover(QPieSlice *slice, bool state);

        // access structures
        QAbstractSeries *curve(QString name) { return curves.value(name, NULL); }

    protected:

        // legend and selector need acces to these
        QChartView *chartview;
        GenericSelectTool *selector;
        QChart *qchart;
        QVBoxLayout *mainLayout;
        QBoxLayout *leftLayout;
        QBoxLayout *holder; // who has me at the mo?

        // trap widget events and pass to event handler
        bool eventFilter(QObject *, QEvent *e);

        // axes geometry (hacked from scene)
        // used by select tool to know when
        // over an axis and where to paint
        QMap<QAbstractAxis*,QRectF> axisRect;

        // the legend
        GenericLegend *legend;
        QStringList havelegend;

        // quadtrees
        QMap<QAbstractSeries*, Quadtree*> quadtrees;

        // annotations
        GenericAnnotationController *annotationController;

        QList<QLabel *> labels;

        QString vname;
        QVector<double> vx, vy; //voronoi digram
        GenericLines *voronoidiagram; // draws the lines on the chart

    private:
        Context *context;
        QGraphicsItem *item;
        int charttype;

        // curves
        QMap<QString, QAbstractSeries *>curves;

        // filenames
        QMap<QAbstractSeries*, QVector<QString> > filenames;

        // decorations (symbols for line charts, lines for scatter)
        QMap<QAbstractSeries*, QAbstractSeries *>decorations;

        // axes
        QMap<QString, GenericAxisInfo *>axisinfos;

        // barsets
        QList<QBarSet*> barsets;
        QBarSeries *barseries;
        QStackedBarSeries *stackbarseries;
        QPercentBarSeries *percentbarseries;
        QList<QString> categories;

        // axis placement (before user interacts)
        // alternates as axis added
        bool left, bottom;
        double scale_;
        QColor bgcolor_;
};
#endif
