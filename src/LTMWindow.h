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

#ifndef _GC_LTMWindow_h
#define _GC_LTMWindow_h 1
#include "GoldenCheetah.h"
#include "MainWindow.h" // for isfiltered and filters

#include <QtGui>
#include <QTimer>
#include "Context.h"
#include "MetricAggregator.h"
#include "Season.h"
#include "LTMPlot.h"
#include "LTMPopup.h"
#include "LTMTool.h"
#include "LTMSettings.h"
#include "LTMCanvasPicker.h"
#include "GcPane.h"

#include <math.h>

class QwtPlotPanner;
class QwtPlotPicker;
class QwtPlotZoomer;

#include <qwt_plot_picker.h>
#include <qwt_text_engine.h>
#include <qwt_picker_machine.h>
#include <qwt_compat.h>

#include "qxtstringspinbox.h" // for reveal control groupby selection

// track the cursor and display the value for the chosen axis
class LTMToolTip : public QwtPlotPicker
{
    public:
    LTMToolTip(int xaxis, int yaxis,
                RubberBand rb, DisplayMode dm, QwtPlotCanvas *pc, QString fmt) :
                QwtPlotPicker(xaxis, yaxis, rb, dm, pc),
        format(fmt) { setStateMachine(new QwtPickerDragPointMachine());}
    virtual QwtText trackerText(const QPoint &/*pos*/) const
    {
        QColor bg = QColor(255,255, 170); // toolyip yellow
#if QT_VERSION >= 0x040300
        bg.setAlpha(200);
#endif
        QwtText text;
        QFont def;
        //def.setPointSize(8); // too small on low res displays (Mac)
        //double val = ceil(pos.y()*100) / 100; // round to 2 decimal place
        //text.setText(QString("%1 %2").arg(val).arg(format), QwtText::PlainText);
        text.setText(tip);
        text.setFont(def);
        text.setBackgroundBrush( QBrush( bg ));
        text.setRenderFlags(Qt::AlignLeft | Qt::AlignTop);
        return text;
    }
    void setFormat(QString fmt) { format = fmt; }
    void setText(QString txt) { tip = txt; }
    private:
    QString format;
    QString tip;
};

class LTMWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int chart READ chart WRITE setChart USER true) // adding/deleting charts will cause this index to fail
                                                              // not a big issue since the preset is APPLIED when it is
                                                              // selected. We don't really need to remember what was used
                                                              // but it may be helpful to some users
    Q_PROPERTY(int bin READ bin WRITE setBin USER true)
    Q_PROPERTY(bool shade READ shade WRITE setShade USER true)
    Q_PROPERTY(bool legend READ legend WRITE setLegend USER true)
    Q_PROPERTY(bool events READ events WRITE setEvents USER true)
#ifdef GC_HAVE_LUCENE
    Q_PROPERTY(QString filter READ filter WRITE setFilter USER true)
#endif
    Q_PROPERTY(QDate fromDate READ fromDate WRITE setFromDate USER true)
    Q_PROPERTY(QDate toDate READ toDate WRITE setToDate USER true)
    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate USER true)
    Q_PROPERTY(int lastN READ lastN WRITE setLastN USER true)
    Q_PROPERTY(int lastNX READ lastNX WRITE setLastNX USER true)
    Q_PROPERTY(int prevN READ prevN WRITE setPrevN USER true)
    Q_PROPERTY(LTMSettings settings READ getSettings WRITE applySettings USER true)
    Q_PROPERTY(int useSelected READ useSelected WRITE setUseSelected USER true) // !! must be last property !!

    public:

        LTMWindow(Context *);
        ~LTMWindow();
        LTMToolTip *toolTip() { return picker; }

        // reveal / filters
        bool hasReveal() { return true; }
#ifdef GC_HAVE_LUCENE
        bool isFiltered() const { return (ltmTool->isFiltered() || context->isfiltered); }
#endif

        // used by children
        Context *context;

        // get/set properties
        int chart() const { return ltmTool->presetPicker->currentIndex(); }
        void setChart(int x) { ltmTool->presetPicker->setCurrentIndex(x); }
        int bin() const { return ltmTool->groupBy->currentIndex(); }
        void setBin(int x) { rGroupBy->setValue(x); ltmTool->groupBy->setCurrentIndex(x); }
        bool shade() const { return ltmTool->shadeZones->isChecked(); }
        void setShade(bool x) { ltmTool->shadeZones->setChecked(x); }
        bool legend() const { return ltmTool->showLegend->isChecked(); }
        void setLegend(bool x) { ltmTool->showLegend->setChecked(x); }
        bool events() const { return ltmTool->showEvents->isChecked(); }
        void setEvents(bool x) { ltmTool->showEvents->setChecked(x); }

        int useSelected() { return ltmTool->dateSetting->mode(); }
        void setUseSelected(int x) { ltmTool->dateSetting->setMode(x); }

        QDate fromDate() { return ltmTool->dateSetting->fromDate(); }
        void setFromDate(QDate date)  { return ltmTool->dateSetting->setFromDate(date); }
        QDate toDate() { return ltmTool->dateSetting->toDate(); }
        void setToDate(QDate date)  { return ltmTool->dateSetting->setToDate(date); }
        QDate startDate() { return ltmTool->dateSetting->startDate(); }
        void setStartDate(QDate date)  { return ltmTool->dateSetting->setStartDate(date); }

        int lastN() { return ltmTool->dateSetting->lastN(); }
        void setLastN(int x) { ltmTool->dateSetting->setLastN(x); }
        int lastNX() { return ltmTool->dateSetting->lastNX(); }
        void setLastNX(int x) { ltmTool->dateSetting->setLastNX(x); }

        int prevN() { return ltmTool->dateSetting->prevN(); }
        void setPrevN(int x) { ltmTool->dateSetting->setPrevN(x); }

#ifdef GC_HAVE_LUCENE
        QString filter() const { return ltmTool->searchBox->filter(); }
        void setFilter(QString x) { ltmTool->searchBox->setFilter(x); }
#endif

        LTMSettings getSettings() const { return settings; }
        void applySettings(LTMSettings x) { ltmTool->applySettings(&x); }

    public slots:
        void rideSelected();
        void refreshPlot();
        void dateRangeChanged(DateRange);
        void filterChanged();
        void metricSelected();
        void groupBySelected(int);
        void rGroupBySelected(int);
        void shadeZonesClicked(int);
        void showEventsClicked(int);
        void showLegendClicked(int);
        void chartSelected(int);
        void saveClicked();
        void manageClicked();
        void refresh();
        void pointClicked(QwtPlotCurve*, int);
        int groupForDate(QDate, int);

        void useCustomRange(DateRange);
        void useStandardRange();
        void useThruToday();

    private:
        // passed from Context *
        DateRange plotted;

        bool useCustom;
        bool useToToday; // truncate to today
        DateRange custom; // custom date range supplied

        // qwt picker
        LTMToolTip *picker;
        LTMCanvasPicker *_canvasPicker; // allow point selection/hover

        // popup - the GcPane to display within
        //         and the LTMPopup contents widdget
        GcPane *popup;
        LTMPopup *ltmPopup;

        // local state
        bool dirty;
        LTMSettings settings; // all the plot settings
        QList<SummaryMetrics> results;
        QList<SummaryMetrics> measures;

        // Widgets
        LTMPlot *ltmPlot;
        QwtPlotZoomer *ltmZoomer;
        LTMTool *ltmTool;

        // reveal controls
        QxtStringSpinBox    *rGroupBy;
        QCheckBox           *rShade,
                            *rEvents;
};

#endif // _GC_LTMWindow_h
