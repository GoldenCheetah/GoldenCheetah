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

#ifndef _GC_GenericChart_h
#define _GC_GenericChart_h 1

#include "GoldenCheetah.h"
#include "Settings.h"
#include "Context.h"
#include "Athlete.h"
#include "Colors.h"
#include "RCanvas.h"
#include "RideMetric.h"

#include "GenericSelectTool.h"
#include "GenericLegend.h"

#include <QScrollArea>

// a chart annotation
class GenericAnnotationInfo {

    public:

        enum annotationType { None, Label, VLine, HLine, LR, Voronoi };
        typedef annotationType AnnotationType;
        AnnotationType type;

        GenericAnnotationInfo(AnnotationType type) : type(type) {}

        // labels
        QStringList labels;

        // voronoi
        QString vname;
        QVector<double> vx, vy; //voronoi digram

        // vline and hline
        Qt::PenStyle linestyle; // also used by LR
        double value;
        QString text;

        QString color; // color name eg. red or #fefefe

        // linear regression
        double slope, intercept, r2;
};

// keeping track of the series info
class GenericSeriesInfo {

    public:

        GenericSeriesInfo(QString name, QVector<double> xseries, QVector<double> yseries, QVector<QString> fseries, QString xname, QString yname,
                      QStringList labels, QStringList colors, int line, int symbol, int size, QString color, int opacity, bool opengl,
                      bool legend, bool datalabels, bool fill, RideMetric::MetricType aggregateby, QList<GenericAnnotationInfo> annotations) :
                      user1(NULL), user2(NULL), user3(NULL), user4(NULL),
                      name(name), xseries(xseries), yseries(yseries), fseries(fseries), xname(xname), yname(yname),
                      labels(labels), colors(colors),
                      line(line), symbol(symbol), size(size), color(color), opacity(opacity), opengl(opengl), legend(legend), datalabels(datalabels), fill(fill),
                      aggregateby(aggregateby), annotations(annotations)
                      {}

        GenericSeriesInfo() :
            user1(NULL), user2(NULL), user3(NULL), user4(NULL),
            line(static_cast<int>(Qt::PenStyle::SolidLine)),
            symbol(0), size(2.00), //XXX todo
            color("red"), opacity(100.0), opengl(true), legend(true), datalabels(false), fill(false)
        {}

        // available for use (e.g. UserChartSettings)
        void *user1, *user2, *user3, *user4;
        QString string1, string2, string3, string4;

        // properties, from setCurve(...)
        QString name;
        QString group;
        QVector<double> xseries;
        QVector<double> yseries;
        QVector<QString> fseries;
        QString xname;
        QString yname;
        QStringList labels;
        QStringList colors;
        int line;
        int symbol;
        double size;
        QString color;
        int opacity;
        bool opengl;
        bool legend;
        bool datalabels;
        bool fill;
        RideMetric::MetricType aggregateby;

        QList<GenericAnnotationInfo> annotations;
};


class GenericChartInfo {

    // not actually used by genericchart as the info
    // is all maintained within the chart itself, but
    // can be used by others when configuring a chart
    // so for example is used by the UserChartSettings
    // dialog
    public:

        // default values, better here than spread across the codebase.
        GenericChartInfo() : localinfo(NULL), type(1), animate(false), legendpos(2), stack(false), orientation(Qt::Vertical), scale(1.0) {}

        // available for use (e.g. UserChartSettings)
        void *localinfo;

        QString title;
        QString description;

        int type;
        bool animate;
        int legendpos;
        bool stack; // stack series instead of on same chart
        int orientation; // layout horizontal or vertical
        double scale; // scale font sizes
        QColor bgcolor; // background color of plots and legends
};

// general axis info
class GenericAxisInfo {

    public:
        enum axisinfoType { CONTINUOUS=0,                 // Continious range
                            DATERANGE=1,                  // Date
                            TIME=2,                       // Duration, Time
                            CATEGORY=3,                   // labelled with categories
                            LAST=3                        // maintain as list grows
                          };
        typedef enum axisinfoType AxisInfoType;

        enum axisgroupbyType { NONE=0,                    // no grouping
                               DAY=1,                     // group to day, e.g. aggregate 2-a-days
                               WEEK=2,                    // week, starting on Monday (make configurable later?)
                               MONTH=3,                   // calendar month
                               YEAR=4                     // calendar year
                          };

        typedef enum axisgroupbyType AxisGroupBy;

        // used for settings / user interaction
        static QString axisTypeDescription(AxisInfoType x) {
            switch (x) {
            default:
            case CONTINUOUS: return (QObject::tr("Continuous")); break;
            case DATERANGE: return (QObject::tr("Date")); break;
            case TIME: return (QObject::tr("Time")); break;
            case CATEGORY: return (QObject::tr("Category")); break;
            }
        }

        GenericAxisInfo(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories, double smooth=0, int groupby=0) :
                      type(static_cast<AxisInfoType>(type)),
                      name(name), align(static_cast<Qt::AlignmentFlag>(align)),
                      minx(min), maxx(max), visible(visible), log(log),
                      labelcolorstring(labelcolor), axiscolorstring(color), categories(categories),
                      smooth(smooth), groupby(static_cast<AxisGroupBy>(groupby))
                      {}

        GenericAxisInfo() : type(AxisInfoType::CONTINUOUS), orientation(Qt::Vertical), align(Qt::AlignLeft),
                            miny(0), maxy(0), minx(0), maxx(0), visible (true), fixed(false), log(false),
                            minorgrid(false), majorgrid(true), labelcolor(GColor(CPLOTMARKER)),
                            axiscolor(GColor(CPLOTMARKER)), smooth(0), groupby(NONE) {}

        static int findAxis(QList<GenericAxisInfo>infos, QString name) {
            for (int i=0; i<infos.count(); i++)
                if (infos[i].name == name)
                    return i;
            return -1; // not found
        }

        GenericAxisInfo(Qt::Orientations orientation, QString name) : name(name), orientation(orientation) {
            miny=maxy=minx=maxx=0;
            fixed=log=false;
            visible=minorgrid=majorgrid=true;
            type=CONTINUOUS;
            smooth=0;
            groupby=NONE;
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
        QList<QAbstractSeries*> decorations;

        // data is all public to avoid tedious get/set
        AxisInfoType type; // what type of axis is this?
        QString name;
        Qt::Orientations orientation;
        Qt::AlignmentFlag align;
        double miny, maxy, minx, maxx; // updated as we see points, set the range
        bool visible,fixed, log, minorgrid, majorgrid; // settings
        QColor labelcolor, axiscolor; // aesthetics
        QString labelcolorstring, axiscolorstring;
        QStringList categories;

        // aggregation and smoothing
        double smooth;
        AxisGroupBy groupby;
};

// keeping track of all our plots
class GenericPlot;
class GenericPlotInfo {

    public:

        // when working out what to do with existing plots
        enum { init, active, matched, deleteme } state;

        // initial
        GenericPlotInfo(QString xaxis) : state(init), plot(NULL), xaxis(xaxis) {}

        bool matches(const GenericPlotInfo &other) const {
            // we need to have same series and same xaxis
            if (other.xaxis != xaxis) return false;

            // same number of series
            if (other.series.count() != series.count()) return false;

            // all my series are in their series?
            foreach(GenericSeriesInfo info, series) {
                bool found=false;
                // is in other?
                foreach(GenericSeriesInfo oinfo, other.series) {
                    if (oinfo.name == info.name && oinfo.yname == info.yname && oinfo.xname == info.xname)
                        found=true;
                }
                if (found == false) return false;
            }
            return true;
        }

        static int findPlot(const QList<GenericPlotInfo>list, const GenericPlotInfo findme) {
            for(int i=0; i<list.count(); i++) {
                // find a plot with same xaxis and series
                if (list[i].matches(findme))
                    return i;
            }
            return -1;
        }

        // the plot object created- and then matched against
        GenericPlot *plot;

        // axes
        QString xaxis;

        QList<GenericSeriesInfo> series;
        QList<GenericAxisInfo> axes;
};

// the chart
class ChartSpace;
class GenericChart : public QWidget {

    Q_OBJECT

    public:

        friend class GenericSelectTool;
        friend class GenericLegend;

        GenericChart(QWidget *parent, Context *context);

    public slots:

        // set chart settings
        bool initialiseChart(QString title, int type, bool animate, int legendpos, bool stack, int orientation, double scale=1.0f);

        // add a curve, associating an axis
        bool addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QVector<QString> fseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int line, int symbol, int size, QString color, int opacity, bool opengl, bool legend, bool datalabels,
                      bool fill, RideMetric::MetricType mtype, QList<GenericAnnotationInfo>annotations);

        // helper for Python and R charts fseries is a stringlist
        bool addCurve(QString name, QVector<double> xseries, QVector<double> yseries, QStringList fseries, QString xname, QString yname,
                      QStringList labels, QStringList colors,
                      int line, int symbol, int size, QString color, int opacity, bool opengl, bool legend, bool datalabels, bool fill);

        // configure axis, after curves added
        bool configureAxis(QString name, bool visible, int align, double min, double max,
                      int type, QString labelcolor, QString color, bool log, QStringList categories);

        // plot background
        void setBackgroundColor(QColor);
        void setGraphicsItem(QGraphicsItem *item);

        // post processing clean up / add decorations / helpers etc
        void finaliseChart();

        // config changed?
        void configChanged(qint32);

    protected:

        // legend and selector need acces to these
        QVBoxLayout *mainLayout;
        QHBoxLayout *lrLayout;

        // chart settings
        QString title;
        int type;
        bool animate;
        int legendpos;
        bool stack; // stack series instead of on same chart
        int orientation; // layout horizontal or vertical
        double scale;
        QColor bgcolor;
        double smooth;
        GenericAxisInfo::AxisGroupBy groupby;

        // once series, axis etc have all been
        // cached, we need to pre-process the data
        // before setting up plots.
        void preprocessData();

        // when we get new settings/calls we
        // collect together to prepare before
        // actually creating plots since we want
        // to see the whole picture- try to reuse
        // plots as much as possible to avoid
        // the cost of creating new ones.
        QList<GenericSeriesInfo> newSeries;
        QList<GenericAxisInfo> newAxes;

        // active plots
        QList<GenericPlotInfo> currentPlots;

    private:
        Context *context;
        QScrollArea *stackFrame;
        QGraphicsItem *item;
        bool blocked;
};
#endif
