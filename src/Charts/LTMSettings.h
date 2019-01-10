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

#ifndef _GC_LTMSettings_h
#define _GC_LTMSettings_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QList>
#include <QDialog>
#include <QDataStream>
#include <QMessageBox>

#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>

#include "RideFile.h" // for SeriesType
#include "Specification.h" // for SeriesType
#include "RideMetric.h" // for RideMetric::MetricType

class LTMTool;
class LTMSettings;
class Context;
class RideMetric;
class RideBest;

// change history for LTMSettings
// Prior to 13 no history was maintained
// Version   Date        Who              Description
// 13        25 Jul 2015 Mark Liversedge  Update to charts.xml to show version number
// 14        13 Aug 2015 Mark Liversedge  Added formula metric type
// 15        13 Aug 2015 Mark Liversedge  Added formula aggregation type Avg, Total, Low etc
// 16        14 Aug 2015 Mark Liversedge  Added curve specific filter
// 17        01 Nov 2017 Ale Martinez     Added Daily Measure type (Body/Hrv)
// 18        05 Jan 2018 Mark Liversedge  Performance tests and weekly performances
// 19        07 Jan 2018 Mark Liversedge  Flagged as possibly submaximal weekly best

#define LTM_VERSION_NUMBER 19

// group by settings
#define LTM_DAY     1
#define LTM_WEEK    2
#define LTM_MONTH   3
#define LTM_YEAR    4
#define LTM_TOD     5
#define LTM_ALL     6

// type of metric
#define METRIC_DB          1
#define METRIC_PM          2
#define METRIC_META        3
#define METRIC_MEASURE     4 // DEPRECATED DO NOT USE
#define METRIC_BEST        5
#define METRIC_ESTIMATE    6
#define METRIC_STRESS      7
#define METRIC_FORMULA     8
#define METRIC_D_MEASURE   9
#define METRIC_PERFORMANCE 10
#define METRIC_BANISTER    11

// type of estimate
#define ESTIMATE_WPRIME  0
#define ESTIMATE_CP      1
#define ESTIMATE_FTP     2
#define ESTIMATE_PMAX    3
#define ESTIMATE_BEST    4
#define ESTIMATE_EI      5
#define ESTIMATE_VO2MAX  6

// type of stress
#define STRESS_STS          0
#define STRESS_LTS          1
#define STRESS_SB           2
#define STRESS_RR           3
#define STRESS_PLANNED_STS  4
#define STRESS_PLANNED_LTS  5
#define STRESS_PLANNED_SB   6
#define STRESS_PLANNED_RR   7
#define STRESS_EXPECTED_STS 8
#define STRESS_EXPECTED_LTS 9
#define STRESS_EXPECTED_SB  10
#define STRESS_EXPECTED_RR  11

// type of banister curve
#define BANISTER_NTE         0
#define BANISTER_PTE         1
#define BANISTER_PERFORMANCE 2
#define BANISTER_CP          3

// type of values
#define VALUES_CALCULATED   0
#define VALUES_PLANNED      1
#define VALUES_EXPECTED     2

// We catalogue each metric and the curve settings etc here
class MetricDetail {
    public:

    MetricDetail() : type(METRIC_DB), stack(false), hidden(false), model(""), formulaType(RideMetric::Average), name(""), 
                     metric(NULL), stressType(0), measureGroup(0), measureField(0), tests(true), perfs(true),
                     smooth(false), trendtype(0), topN(0), lowestN(0), topOut(0), baseline(0.0), 
                     curveStyle(QwtPlotCurve::Lines), symbolStyle(QwtSymbol::NoSymbol),
                     penColor(Qt::black), penAlpha(0), penWidth(1.0), penStyle(0),
                     brushColor(Qt::black), brushAlpha(0), fillCurve(false), labels(false), curve(NULL) {}

    bool operator< (MetricDetail right) const { return name < right.name; }

    int type;
    bool stack; // should this be stacked?
    bool hidden; // should this be hidden ? (toggled via clicking on legend)

    QString model; // short code for model selected
    int estimate; // 0-4 for W', CP, FTP, PMAX
    int estimateDuration;       // n x units below for seconds
    int estimateDuration_units; // 1=secs, 60=mins, 3600=hours
    bool wpk; // absolute or wpk 

    // for FORMULAs
    QString formula;
    RideMetric::MetricType formulaType;

    // for METRICS
    QString symbol;
    QString bestSymbol;
    QString name, units;
    const RideMetric *metric;

    // for BESTS
    int duration;       // n x units below for seconds
    int duration_units; // 1=secs, 60=mins, 3600=hours
    RideFile::SeriesType series; // what series are we doing the peak for

    // for STRESS
    int stressType;     // 0-LTS 1-STS 2-SB 3-RR

    // for DAILY MEASURES
    int measureGroup;   // 0-BODY 1-HRV
    int measureField;   // Weight, RMSSD, etc.

    // for PERFORMANCES
    bool tests;
    bool perfs;
    bool submax;

    // GENERAL SETTINGS FOR A METRIC
    QString uname, uunits; // user specified name and units (axis choice)

    // user configurable settings
    bool smooth;         // smooth the curve
    int trendtype;       // 0 - no trend, 1 - linear, 2 - quadratic
    int topN;            // highlight top N points
    int lowestN;            // highlight top N points
    int topOut;          // highlight N ranked outlier points
    double baseline;     // baseline for chart

    // filter
    bool showOnPlot;
    int filter;         // 0 no filter, 1 = include, 2 = exclude
    double from, to;

    QString datafilter;

    // curve type and symbol
    QwtPlotCurve::CurveStyle curveStyle;      // how should this metric be plotted?
    QwtSymbol::Style symbolStyle;             // display a symbol

    // pen
    QColor penColor;
    int penAlpha;
    double penWidth;
    int penStyle;

    // brush
    QColor brushColor;
    int brushAlpha;

    // fill curve
    bool fillCurve;

    // text labels against values
    bool labels;

    // curve on the chart to spot this...
    QwtPlotCurve *curve;
};

// so we can marshal and unmarshall LTMSettings when we save
// asa QVariant we need to provide our own functions to
// do this
QDataStream &operator<<(QDataStream &out, const LTMSettings &settings);
QDataStream &operator>>(QDataStream &in, LTMSettings &settings);

// used to maintain details about the metrics being plotted
class LTMSettings {

    public:

        LTMSettings() {
            // we need to register the stream operators
            qRegisterMetaTypeStreamOperators<LTMSettings>("LTMSettings");
            bests = NULL;
            ltmTool = NULL;
        }

        void writeChartXML(QDir, QList<LTMSettings>);
        void readChartXML(QDir, bool, QList<LTMSettings>&charts);
        void translateMetrics(bool useMetricUnits);

        QString name;
        QString title;
        QDateTime start;
        QDateTime end;
        int groupBy;
        bool shadeZones;
        bool showData;
        bool legend;
        bool events;
        bool stack;
        int stackWidth;

        Specification specification;
        QList<MetricDetail> metrics;
        QList<RideBest> *bests;

        LTMTool *ltmTool;
        QString field1, field2;
};
Q_DECLARE_METATYPE(LTMSettings);

class EditChartDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        EditChartDialog(Context *, LTMSettings *, QList<LTMSettings>);

    public slots:
        void okClicked();
        void cancelClicked();

    private:
        Context *context;
        LTMSettings *settings;

        QList<LTMSettings> presets;
        QLineEdit *chartName;
        QPushButton *okButton, *cancelButton;
};

#endif
