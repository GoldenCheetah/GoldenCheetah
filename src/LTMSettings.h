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

#include <QtGui>
#include <QList>
#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_scale_draw.h>
#include <qwt_symbol.h>

class LTMTool;
class SummaryMetrics;
class MainWindow;
class RideMetric;

// group by settings
#define LTM_DAY     1
#define LTM_WEEK    2
#define LTM_MONTH   3
#define LTM_YEAR    4

// type of metric
// is it from the ridemetric factory or PMC stresscalculator or metadata
#define METRIC_DB     1
#define METRIC_PM     2
#define METRIC_META   3

// We catalogue each metric and the curve settings etc here
class MetricDetail {
    public:

    MetricDetail() : type(METRIC_DB), name(""), metric(NULL), smooth(false), trend(false), topN(0),
                     baseline(0.0), curveStyle(QwtPlotCurve::Lines), symbolStyle(QwtSymbol::NoSymbol),
                     penColor(Qt::black), penAlpha(0), penWidth(1.0), penStyle(0),
                     brushColor(Qt::black), brushAlpha(0) {}

    bool operator< (MetricDetail right) const { return name < right.name; }

    int type;

    QString symbol, name;
    const RideMetric *metric;

    QString uname, uunits; // user specified name and units (axis choice)

    // user configurable settings
    bool smooth,         // smooth the curve
         trend;          // add a trend line
    int topN;            // highlight top N points
    double baseline;     // baseline for chart

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

// used to maintain details about the metrics being plotted
class LTMSettings {

    public:

        void writeChartXML(QDir, QList<LTMSettings>);
        void readChartXML(QDir, QList<LTMSettings>&charts);

        QString name;
        QString title;
        QDateTime start;
        QDateTime end;
        int groupBy;
        bool shadeZones;
        QList<MetricDetail> metrics;
        QList<SummaryMetrics> *data;
        LTMTool *ltmTool;
};

class EditChartDialog : public QDialog
{
    Q_OBJECT

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
