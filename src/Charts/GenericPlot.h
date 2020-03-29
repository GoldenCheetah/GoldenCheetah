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

// keep aligned to library.py
#define GC_CHART_LINE      1
#define GC_CHART_SCATTER   2
#define GC_CHART_BAR       3
#define GC_CHART_PIE       4

class GenericPlot;
class GenericLegend;
class GenericSelectTool;
class GenericAxisInfo;

// the chart
class GenericPlot : public QWidget {

    Q_OBJECT

    public:

        static constexpr double gl_minheight = 225;
        static QString gl_dateformat;
        static QString gl_timeformat;

        friend class GenericSelectTool;
        friend class GenericLegend;

        GenericPlot(QWidget *parent, Context *context);

        // some helper functions
        static QColor seriesColor(QAbstractSeries* series);

        enum annotationType { LINE=0,                 // Continious range
                              RECTANGLE=1,
                              TEXT=2,
                              LABEL=3
                          };
        typedef enum annotationType AnnotationType;

    public slots:

        void configChanged(qint32);

        // set chart settings
        bool initialiseChart(QString title, int type, bool animate, int legendpos);

        // add a curve, associating an axis
        bool addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int line, int symbol, int size, QString color, int opacity, bool opengl, bool legend, bool datalabels);

        // adding annotations
        void addAnnotation(AnnotationType, QAbstractSeries*, double yvalue); // LINE
        void addAnnotation(AnnotationType, QString, QColor=QColor(Qt::gray)); // LABEL

        // configure axis, after curves added
        bool configureAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories);

        // post processing clean up / add decorations / helpers etc
        void finaliseChart();

        // do we want to see this series?
        void setSeriesVisible(QString name, bool visible);

        // watching scene events and managing interaction
        bool eventHandler(int eventsource, void *obj, QEvent *event);
        void barsetHover(bool status, int index, QBarSet *barset);
        void plotAreaChanged();


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


        // annotation labels
        QList<QLabel *> labels;

    private:
        Context *context;
        int charttype;

        // curves
        QMap<QString, QAbstractSeries *>curves;

        // decorations (symbols for line charts, lines for scatter)
        QMap<QAbstractSeries*, QAbstractSeries *>decorations;

        // axes
        QMap<QString, GenericAxisInfo *>axisinfos;

        // barsets
        QList<QBarSet*> barsets;
        QBarSeries *barseries;

        // axis placement (before user interacts)
        // alternates as axis added
        bool left, bottom;
};
#endif
