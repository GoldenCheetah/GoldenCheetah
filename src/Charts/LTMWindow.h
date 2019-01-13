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
#include <QStackedWidget>
#ifdef NOWEBKIT
#include <QWebEngineView>
#else
#include <QWebView>
#include <QWebFrame>
#endif
#include <QTimer>
#include "Context.h"
#include "Season.h"
#include "LTMPlot.h"
#include "LTMPopup.h"
#include "LTMTool.h"
#include "LTMSettings.h"
#include "LTMCanvasPicker.h"
#include "GcPane.h"
#include "Settings.h"
#include "Colors.h"

#include <cmath>

class QwtPlotPanner;
class QxtSpanSlider;
class QwtPlotPicker;
class AllPlot;

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
                RubberBand rb, DisplayMode dm, QWidget *pc, QString fmt) :
                QwtPlotPicker(xaxis, yaxis, rb, dm, pc),
        format(fmt) { setStateMachine(new QwtPickerDragPointMachine());}

    virtual QRect trackerRect(const QFont &font) const
    {
        return QwtPlotPicker::trackerRect(font).adjusted(-3,-3,3,3);
    }

    virtual QwtText trackerText(const QPoint &/*pos*/) const
    {
        QwtText text(tip);

        QFont stGiles;
        stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
        stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());
        stGiles.setWeight(QFont::Bold);
        text.setFont(stGiles);

        text.setBackgroundBrush(QBrush( GColor(CPLOTMARKER)));
        text.setColor(GColor(CRIDEPLOTBACKGROUND));
        text.setBorderRadius(6);
        text.setRenderFlags(Qt::AlignCenter | Qt::AlignVCenter);

        return text;
    }
    void setFormat(QString fmt) { format = fmt; }
    void setText(QString txt) { tip = txt; }

    protected:
    friend class ::AllPlot;

    private:
    QString format;
    QString tip;
};

class LTMWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(bool preset READ preset WRITE setPreset USER true)
    Q_PROPERTY(int bin READ bin WRITE setBin USER true)
    Q_PROPERTY(bool shade READ shade WRITE setShade USER true)
    Q_PROPERTY(bool data READ data WRITE setData USER true)
    Q_PROPERTY(bool stack READ stack WRITE setStack USER true)
    Q_PROPERTY(int stackWidth READ stackW WRITE setStackW USER true)
    Q_PROPERTY(bool legend READ legend WRITE setLegend USER true)
    Q_PROPERTY(bool events READ events WRITE setEvents USER true)
    Q_PROPERTY(bool banister READ banister WRITE setBanister USER true)
    Q_PROPERTY(QString filter READ filter WRITE setFilter USER true)
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

        // reveal / filters
        bool hasReveal() { return true; }
        bool isFiltered() const { return (ltmTool->isFiltered() || context->ishomefiltered || context->isfiltered); }

        // comparing things
        bool isCompare() const { return context->isCompareDateRanges; }

        // used by children
        Context *context;

        // get/set properties
        int bin() const { return ltmTool->groupBy->currentIndex(); }
        void setBin(int x) { rGroupBy->setValue(x); ltmTool->groupBy->setCurrentIndex(x); }
        bool shade() const { return ltmTool->shadeZones->isChecked(); }
        void setShade(bool x) { ltmTool->shadeZones->setChecked(x); }
        bool data() const { return ltmTool->showData->isChecked(); }
        void setData(bool x) { ltmTool->showData->setChecked(x); }
        bool legend() const { return ltmTool->showLegend->isChecked(); }
        void setLegend(bool x) { ltmTool->showLegend->setChecked(x); }
        bool events() const { return ltmTool->showEvents->isChecked(); }
        void setEvents(bool x) { ltmTool->showEvents->setChecked(x); }
        bool banister() const { return ltmTool->showBanister->isChecked(); }
        void setBanister(bool x) { ltmTool->showBanister->setChecked(x); }
        bool stack() const { return ltmTool->showStack->isChecked(); }
        void setStack(bool x) { ltmTool->showStack->setChecked(x); }
        int stackW() const { return ltmTool->stackSlider->value(); }
        void setStackW(int x) { ltmTool->stackSlider->setValue(x); }

        bool preset() const { return ltmTool->usePreset->isChecked(); }
        void setPreset(bool x) { ltmTool->usePreset->setChecked(x); }

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

        QString filter() const { return ltmTool->searchBox->filter(); }
        void setFilter(QString x) { ltmTool->searchBox->setFilter(x); }

        LTMSettings getSettings() const { return settings; }
        void applySettings(LTMSettings x) { 
            bool events =settings.events; // remember - they don't get saved in settings..
            settings = x; 
            settings.events = events; 
            settings.ltmTool = ltmTool;
            ltmTool->applySettings();
        }

        // when part of library don't show the basic
        // tab, its not relevant, but do make sure the
        // use sidebar settings is not true !
        void hideBasic(); 

    public slots:
        void rideSelected();        // notification to refresh
        void presetSelected(int index); // when a preset is selected in the sidebar


        // show the banister helper
        void showBanister(bool relevant);    // show banister helper if relevant to plot
        void tuneBanister();                 // when t1/t2 change
        void refreshBanister(int);           // refresh banister helper

        void refreshPlot();         // normal mode
        void refreshCompare();      // compare mode
        void refreshStackPlots();   // stacked plots
        void refreshDataTable();    // data table

        void styleChanged(int);
        void compareChanged();
        void dateRangeChanged(DateRange);
        void filterChanged();
        void groupBySelected(int);
        void showEventsClicked(int);
        void rGroupBySelected(int);
        void shadeZonesClicked(int);
        void showDataClicked(int);
        void showStackClicked(int);
        void zoomSliderChanged();
        void showLegendClicked(int);
        void applyClicked();
        void refreshUpdate(QDate);
        void refresh();
        void pointClicked(QwtPlotCurve*, int);
        int groupForDate(QDate);

        void useCustomRange(DateRange);
        void useStandardRange();
        void useThruToday();

        // user changed the date range
        void spanSliderChanged();
        void moveLeft();
        void moveRight();

        void exportData();
        void exportConfig();

        QString dataTable(bool html=true); // true as html, false as csv

        void configChanged(qint32);

    protected:

        // receive events
        bool event(QEvent *event);

    private:
        // passed from Context *
        DateRange plotted;

        bool useCustom;
        bool useToToday; // truncate to today
        DateRange custom; // custom date range supplied

        // stack for plot or summary
        QStackedWidget *stackWidget;

        // summary view
#ifdef NOWEBKIT
        QWebEngineView *dataSummary;
#else
        QWebView *dataSummary;
#endif


        // popup - the GcPane to display within
        //         and the LTMPopup contents widdget
        GcPane *popup;
        LTMPopup *ltmPopup;

        // local state
        bool dirty;
        bool stackDirty;
        bool compareDirty;

        LTMSettings settings; // all the plot settings
        QList<RideBest> bestsresults;

        // when one curve per plot we split the settings
        QScrollArea *plotArea;
        QWidget *plotWidget, *plotsWidget;
        QVBoxLayout *plotsLayout;
        QList<LTMSettings> plotSettings;
        QList<LTMPlot *> plots;

        // when comparing things we have a plot for each data series
        // with a curve for each date range on the plot
        QScrollArea *compareplotArea;
        QWidget *compareplotsWidget;
        QVBoxLayout *compareplotsLayout;
        QList<LTMSettings> compareplotSettings;
        QList<LTMPlot *> compareplots;

        // Widgets
        LTMPlot *ltmPlot;
        QxtSpanSlider *spanSlider;
        QPushButton *scrollLeft, *scrollRight;
        LTMTool *ltmTool;

        // reveal controls
        QxtStringSpinBox    *rGroupBy;
        QCheckBox           *rData,
                            *rStack;

        // banister helper
        QComboBox *banCombo;
        QDoubleSpinBox *banT1;
        QDoubleSpinBox *banT2;
        QLabel *ilabel, *plabel, *peaklabel, *t1label1, *t1label2, *t2label1, *t2label2, *RMSElabel;

        QTime lastRefresh;
        bool firstshow;
};

#endif // _GC_LTMWindow_h
