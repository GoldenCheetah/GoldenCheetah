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
#include <QDataStream>

#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>

class LTMTool;
class LTMSettings;
class SummaryMetrics;
class MainWindow;
class RideMetric;

// group by settings
#define LTM_DAY     1
#define LTM_WEEK    2
#define LTM_MONTH   3
#define LTM_YEAR    4
#define LTM_TOD     5

// type of metric
#define METRIC_DB        1
#define METRIC_PM        2
#define METRIC_META      3
#define METRIC_MEASURE   4

// We catalogue each metric and the curve settings etc here
class MetricDetail {
    public:

    MetricDetail() : type(METRIC_DB), stack(false), name(""), metric(NULL), smooth(false), trend(false), topN(0),
                     topOut(0), baseline(0.0), curveStyle(QwtPlotCurve::Lines), symbolStyle(QwtSymbol::NoSymbol),
                     penColor(Qt::black), penAlpha(0), penWidth(1.0), penStyle(0),
                     brushColor(Qt::black), brushAlpha(0) {}

    bool operator< (MetricDetail right) const { return name < right.name; }

    int type;
    bool stack; // should this be stacked?

    QString symbol, name, units;
    const RideMetric *metric;

    QString uname, uunits; // user specified name and units (axis choice)

    // user configurable settings
    bool smooth,         // smooth the curve
         trend;          // add a trend line
    int topN;            // highlight top N points
    int topOut;          // highlight N ranked outlier points
    double baseline;     // baseline for chart

    // filter
    bool showOnPlot;
    int filter;         // 0 no filter, 1 = include, 2 = exclude
    double from, to;

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
};

// so we can marshal and unmarshall LTMSettings when we save
// asa QVariant we need to provide our own functions to
// do this
QDataStream &operator<<(QDataStream &out, const LTMSettings &settings);
QDataStream &operator>>(QDataStream &in, LTMSettings &settings);
Q_DECLARE_METATYPE(LTMSettings);

// used to maintain details about the metrics being plotted
class LTMSettings {

    public:

        LTMSettings() {
            // we need to register the stream operators
            qRegisterMetaTypeStreamOperators<LTMSettings>("LTMSettings");
        }

        void writeChartXML(QDir, QList<LTMSettings>);
        void readChartXML(QDir, QList<LTMSettings>&charts);

        QString name;
        QString title;
        QDateTime start;
        QDateTime end;
        int groupBy;
        bool shadeZones;
        bool legend;
        QList<MetricDetail> metrics;
        QList<SummaryMetrics> *data;
        QList<SummaryMetrics> *measures;
        LTMTool *ltmTool;
        QString field1, field2;
};

class EditChartDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        EditChartDialog(MainWindow *, LTMSettings *, QList<LTMSettings>);

    public slots:
        void okClicked();
        void cancelClicked();

    private:
        MainWindow *mainWindow;
        LTMSettings *settings;

        QList<LTMSettings> presets;
        QLineEdit *chartName;
        QPushButton *okButton, *cancelButton;
};

class ChartManagerDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        ChartManagerDialog(MainWindow *, QList<LTMSettings> *);

    public slots:
        void okClicked();
        void cancelClicked();
        void exportClicked();
        void importClicked();
        void upClicked();
        void downClicked();
        void renameClicked();
        void deleteClicked();

    private:
        MainWindow *mainWindow;
        QList<LTMSettings> *presets;

        QLineEdit *chartName;

        QTreeWidget *charts;

        QPushButton *importButton, *exportButton;
        QPushButton *upButton, *downButton, *renameButton, *deleteButton;
        QPushButton *okButton, *cancelButton;
};

#endif
