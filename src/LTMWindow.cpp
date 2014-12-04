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

#include "LTMWindow.h"
#include "LTMTool.h"
#include "LTMPlot.h"
#include "LTMSettings.h"
#include "TabView.h"
#include "Context.h"
#include "Context.h"
#include "Athlete.h"
#include "RideFileCache.h"
#include "SummaryMetrics.h"
#include "Settings.h"
#include "math.h"
#include "Units.h" // for MILES_PER_KM

#include <QtGui>
#include <QString>
#include <QDebug>
#include <QWebView>
#include <QWebFrame>
#include <QStyle>
#include <QStyleFactory>

#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_marker.h>

LTMWindow::LTMWindow(Context *context) :
            GcChartWindow(context), context(context), dirty(true), stackDirty(true), compareDirty(true)
{
    useToToday = useCustom = false;
    plotted = DateRange(QDate(01,01,01), QDate(01,01,01));

    // the plot
    QVBoxLayout *mainLayout = new QVBoxLayout;
    ltmPlot = new LTMPlot(this, context, true);

    // the stack of plots
    QPalette palette;
    palette.setBrush(QPalette::Background, QBrush(GColor(CPLOTBACKGROUND)));

    plotsWidget = new QWidget(this);
    plotsWidget->setPalette(palette);
    plotsLayout = new QVBoxLayout(plotsWidget);
    plotsLayout->setSpacing(0);
    plotsLayout->setContentsMargins(0,0,0,0);

    plotArea = new QScrollArea(this);
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    plotArea->setStyle(cde);
#endif
    plotArea->setAutoFillBackground(false);
    plotArea->setWidgetResizable(true);
    plotArea->setWidget(plotsWidget);
    plotArea->setFrameStyle(QFrame::NoFrame);
    plotArea->setContentsMargins(0,0,0,0);
    plotArea->setPalette(palette);

    // the data table
    dataSummary = new QWebView(this);
    dataSummary->setContentsMargins(0,0,0,0);
    dataSummary->page()->view()->setContentsMargins(0,0,0,0);
    dataSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    dataSummary->setAcceptDrops(false);

    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply
    dataSummary->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize()+1);
    dataSummary->settings()->setFontFamily(QWebSettings::StandardFont, defaultFont.family());

    // compare plot page
    compareplotsWidget = new QWidget(this);
    compareplotsWidget->setPalette(palette);
    compareplotsLayout = new QVBoxLayout(compareplotsWidget);
    compareplotsLayout->setSpacing(0);
    compareplotsLayout->setContentsMargins(0,0,0,0);

    compareplotArea = new QScrollArea(this);
#ifdef Q_OS_WIN
    cde = QStyleFactory::create(OS_STYLE);
    compareplotArea->setStyle(cde);
#endif
    compareplotArea->setAutoFillBackground(false);
    compareplotArea->setWidgetResizable(true);
    compareplotArea->setWidget(compareplotsWidget);
    compareplotArea->setFrameStyle(QFrame::NoFrame);
    compareplotArea->setContentsMargins(0,0,0,0);
    compareplotArea->setPalette(palette);

    // the stack
    stackWidget = new QStackedWidget(this);
    stackWidget->addWidget(ltmPlot);
    stackWidget->addWidget(dataSummary);
    stackWidget->addWidget(plotArea);
    stackWidget->addWidget(compareplotArea);
    stackWidget->setCurrentIndex(0);
    mainLayout->addWidget(stackWidget);
    setChartLayout(mainLayout);

    // reveal controls
    QHBoxLayout *revealLayout = new QHBoxLayout;
    revealLayout->setContentsMargins(0,0,0,0);
    revealLayout->addStretch();
    revealLayout->addWidget(new QLabel(tr("Group by"),this));

    rGroupBy = new QxtStringSpinBox(this);
    QStringList strings;
    strings << tr("Days")
            << tr("Weeks")
            << tr("Months")
            << tr("Years")
            << tr("Time Of Day")
            << tr("All");
    rGroupBy->setStrings(strings);
    rGroupBy->setValue(0);

    revealLayout->addWidget(rGroupBy);
    rData = new QCheckBox(tr("Data Table"), this);
    rStack = new QCheckBox(tr("Stacked"), this);
    QVBoxLayout *checks = new QVBoxLayout;
    checks->setSpacing(2);
    checks->setContentsMargins(0,0,0,0);
    checks->addWidget(rData);
    checks->addWidget(rStack);
    revealLayout->addLayout(checks);
    revealLayout->addStretch();
    setRevealLayout(revealLayout);

    // add additional menu items before setting
    // controls since the menu is SET from setControls
    QAction *exportData = new QAction(tr("Export Chart Data..."), this);
    addAction(exportData);

    // the controls
    QWidget *c = new QWidget;
    c->setContentsMargins(0,0,0,0);
    QVBoxLayout *cl = new QVBoxLayout(c);
    cl->setContentsMargins(0,0,0,0);
    cl->setSpacing(0);
    setControls(c);

    // the popup
    popup = new GcPane();
    ltmPopup = new LTMPopup(context);
    QVBoxLayout *popupLayout = new QVBoxLayout();
    popupLayout->addWidget(ltmPopup);
    popup->setLayout(popupLayout);

    ltmTool = new LTMTool(context, &settings);

    // initialise
    settings.ltmTool = ltmTool;
    settings.data = NULL;
    settings.groupBy = LTM_DAY;
    settings.legend = ltmTool->showLegend->isChecked();
    settings.events = ltmTool->showEvents->isChecked();
    settings.shadeZones = ltmTool->shadeZones->isChecked();
    settings.showData = ltmTool->showData->isChecked();
    settings.stack = ltmTool->showStack->isChecked();
    settings.stackWidth = ltmTool->stackSlider->value();
    rData->setChecked(ltmTool->showData->isChecked());
    rStack->setChecked(ltmTool->showStack->isChecked());
    cl->addWidget(ltmTool);

    connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    connect(ltmTool, SIGNAL(filterChanged()), this, SLOT(filterChanged()));
    connect(context, SIGNAL(homeFilterChanged()), this, SLOT(filterChanged()));
    connect(ltmTool->groupBy, SIGNAL(currentIndexChanged(int)), this, SLOT(groupBySelected(int)));
    connect(rGroupBy, SIGNAL(valueChanged(int)), this, SLOT(rGroupBySelected(int)));
    connect(ltmTool->applyButton, SIGNAL(clicked(bool)), this, SLOT(applyClicked(void)));
    connect(ltmTool->shadeZones, SIGNAL(stateChanged(int)), this, SLOT(shadeZonesClicked(int)));
    connect(ltmTool->showData, SIGNAL(stateChanged(int)), this, SLOT(showDataClicked(int)));
    connect(rData, SIGNAL(stateChanged(int)), this, SLOT(showDataClicked(int)));
    connect(ltmTool->showStack, SIGNAL(stateChanged(int)), this, SLOT(showStackClicked(int)));
    connect(rStack, SIGNAL(stateChanged(int)), this, SLOT(showStackClicked(int)));
    connect(ltmTool->stackSlider, SIGNAL(valueChanged(int)), this, SLOT(zoomSliderChanged()));
    connect(ltmTool->showLegend, SIGNAL(stateChanged(int)), this, SLOT(showLegendClicked(int)));
    connect(ltmTool->showEvents, SIGNAL(stateChanged(int)), this, SLOT(showEventsClicked(int)));
    connect(ltmTool, SIGNAL(useCustomRange(DateRange)), this, SLOT(useCustomRange(DateRange)));
    connect(ltmTool, SIGNAL(useThruToday()), this, SLOT(useThruToday()));
    connect(ltmTool, SIGNAL(useStandardRange()), this, SLOT(useStandardRange()));
    connect(ltmTool, SIGNAL(curvesChanged()), this, SLOT(refresh()));
    connect(context, SIGNAL(filterChanged()), this, SLOT(refresh()));

    // comparing things
    connect(context, SIGNAL(compareDateRangesStateChanged(bool)), this, SLOT(compareChanged()));
    connect(context, SIGNAL(compareDateRangesChanged()), this, SLOT(compareChanged()));

    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(context, SIGNAL(presetSelected(int)), this, SLOT(presetSelected(int)));

    // custom menu item
    connect(exportData, SIGNAL(triggered()), this, SLOT(exportData()));

    configChanged();
}

LTMWindow::~LTMWindow()
{
    delete popup;
}

void
LTMWindow::configChanged()
{
#ifndef Q_OS_MAC
    plotArea->setStyleSheet(TabView::ourStyleSheet());
    compareplotArea->setStyleSheet(TabView::ourStyleSheet());
#endif
    refresh();
}

void
LTMWindow::compareChanged()
{
    if (!amVisible()) {
        compareDirty = true;
        return;
    }

    if (isCompare()) {

        // refresh plot handles the compare case
        refreshPlot();

    } else {

        // forced refresh back to normal
        stackDirty = dirty = true;
        filterChanged(); // forces reread etc
    }
    repaint();
}

void
LTMWindow::rideSelected() { } // deprecated

void
LTMWindow::presetSelected(int index)
{
    // apply a preset if we are configured to do that...
    if (ltmTool->usePreset->isChecked()) {

        // save chart setup
        int groupBy = settings.groupBy;
        bool legend = settings.legend;
        bool events = settings.events;
        bool stack = settings.stack;
        bool shadeZones = settings.shadeZones;
        QDateTime start = settings.start;
        QDateTime end = settings.end;

        // apply preset
        settings = context->athlete->presets[index];

        // now get back the local chart setup
        settings.ltmTool = ltmTool;
        settings.data = &results;
        settings.bests = &bestsresults;
        settings.measures = &measures;
        settings.groupBy = groupBy;
        settings.legend = legend;
        settings.events = events;
        settings.stack = stack;
        settings.shadeZones = shadeZones;
        settings.start = start;
        settings.end = end;

        ltmTool->applySettings();
        refresh();
    }
}

void
LTMWindow::refreshPlot()
{
    if (amVisible() == true) {

        if (isCompare()) {

            // COMPARE PLOTS
            stackWidget->setCurrentIndex(3);
            refreshCompare();

        } else if (ltmTool->showData->isChecked()) {

            //  DATA TABLE
            stackWidget->setCurrentIndex(1);
            refreshDataTable();

        } else {

            if (ltmTool->showStack->isChecked()) {

                // STACK PLOTS
                refreshStackPlots();
                stackWidget->setCurrentIndex(2);
                stackDirty = false;

            } else {

                // NORMAL PLOTS
                plotted = DateRange(settings.start.date(), settings.end.date());
                ltmPlot->setData(&settings);
                stackWidget->setCurrentIndex(0);
                dirty = false;
            }

        }
    }
}

void
LTMWindow::refreshCompare()
{
    // not if in compare mode
    if (!isCompare()) return; 

    // setup stacks but only if needed
    //if (!stackDirty) return; // lets come back to that!

    setUpdatesEnabled(false);

    // delete old and create new...
    //    QScrollArea *plotArea;
    //    QWidget *plotsWidget;
    //    QVBoxLayout *plotsLayout;
    //    QList<LTMSettings> plotSettings;
    foreach (LTMPlot *p, compareplots) {
        compareplotsLayout->removeWidget(p);
        delete p;
    }
    compareplots.clear();
    compareplotSettings.clear();

    if (compareplotsLayout->count() == 1) {
        compareplotsLayout->takeAt(0); // remove the stretch
    }

    // now lets create them all again
    // based upon the current settings
    // we create a plot for each curve
    // but where they are stacked we put
    // them all in the SAME plot
    // so we go once through picking out
    // the stacked items and once through
    // for all the rest of the curves
    LTMSettings plotSetting = settings;
    plotSetting.metrics.clear();
    foreach(MetricDetail m, settings.metrics) {
        if (m.stack) plotSetting.metrics << m;
    }

    bool first = true;

    // create ltmPlot with this
    if (plotSetting.metrics.count()) {

        compareplotSettings << plotSetting;

        // create and setup the plot
        LTMPlot *stacked = new LTMPlot(this, context, first);
        stacked->setCompareData(&compareplotSettings.last()); // setData using the compare data
        stacked->setFixedHeight(200); // maybe make this adjustable later

        // no longer first
        first = false;

        // now add
        compareplotsLayout->addWidget(stacked);
        compareplots << stacked;
    }

    // OK, now one plot for each curve
    // that isn't stacked!
    foreach(MetricDetail m, settings.metrics) {

        // ignore stacks
        if (m.stack) continue;

        plotSetting = settings;
        plotSetting.metrics.clear();
        plotSetting.metrics << m;
        compareplotSettings << plotSetting;

        // create and setup the plot
        LTMPlot *plot = new LTMPlot(this, context, first);
        plot->setCompareData(&compareplotSettings.last()); // setData using the compare data

        // no longer first
        first = false;

        // now add
        compareplotsLayout->addWidget(plot);
        compareplots << plot;
    }

    // squash em up
    compareplotsLayout->addStretch();

    // set a common X-AXIS
    if (settings.groupBy != LTM_TOD) {
        int MAXX=0;
        foreach(LTMPlot *p, compareplots) {
            if (p->getMaxX() > MAXX) MAXX=p->getMaxX();
        }
        foreach(LTMPlot *p, compareplots) {
            p->setMaxX(MAXX);
        }
    }


    // resize to choice
    zoomSliderChanged();

    // we no longer dirty
    compareDirty = false;

    setUpdatesEnabled(true);
}

void 
LTMWindow::refreshStackPlots()
{
    // not if in compare mode
    if (isCompare()) return; 

    // setup stacks but only if needed
    //if (!stackDirty) return; // lets come back to that!

    setUpdatesEnabled(false);

    // delete old and create new...
    //    QScrollArea *plotArea;
    //    QWidget *plotsWidget;
    //    QVBoxLayout *plotsLayout;
    //    QList<LTMSettings> plotSettings;
    foreach (LTMPlot *p, plots) {
        plotsLayout->removeWidget(p);
        delete p;
    }
    plots.clear();
    plotSettings.clear();

    if (plotsLayout->count() == 1) {
        plotsLayout->takeAt(0); // remove the stretch
    }

    // now lets create them all again
    // based upon the current settings
    // we create a plot for each curve
    // but where they are stacked we put
    // them all in the SAME plot
    // so we go once through picking out
    // the stacked items and once through
    // for all the rest of the curves
    LTMSettings plotSetting = settings;
    plotSetting.metrics.clear();
    foreach(MetricDetail m, settings.metrics) {
        if (m.stack) plotSetting.metrics << m;
    }

    bool first = true;

    // create ltmPlot with this
    if (plotSetting.metrics.count()) {

        plotSettings << plotSetting;

        // create and setup the plot
        LTMPlot *stacked = new LTMPlot(this, context, first);
        stacked->setData(&plotSettings.last());
        stacked->setFixedHeight(200); // maybe make this adjustable later

        // no longer first
        first = false;

        // now add
        plotsLayout->addWidget(stacked);
        plots << stacked;
    }

    // OK, now one plot for each curve
    // that isn't stacked!
    foreach(MetricDetail m, settings.metrics) {

        // ignore stacks
        if (m.stack) continue;

        plotSetting = settings;
        plotSetting.metrics.clear();
        plotSetting.metrics << m;
        plotSettings << plotSetting;

        // create and setup the plot
        LTMPlot *plot = new LTMPlot(this, context, first);
        plot->setData(&plotSettings.last());

        // no longer first
        first = false;

        // now add
        plotsLayout->addWidget(plot);
        plots << plot;
    }

    // squash em up
    plotsLayout->addStretch();

    // resize to choice
    zoomSliderChanged();

    // we no longer dirty
    stackDirty = false;

    setUpdatesEnabled(true);
}

void
LTMWindow::zoomSliderChanged()
{
    static int add[] = { 0, 20, 50, 80, 100, 150, 200, 400 };
    int index = ltmTool->stackSlider->value();

    settings.stackWidth = ltmTool->stackSlider->value();
    setUpdatesEnabled(false);

    // do the compare and the noncompare plots
    // at the same time, as we don't need to worry
    // about optimising out as its fast anyway
    foreach(LTMPlot *plot, plots) {
        plot->setFixedHeight(150 + add[index]);
    }
    foreach(LTMPlot *plot, compareplots) {
        plot->setFixedHeight(150 + add[index]);
    }
    setUpdatesEnabled(true);
}

void
LTMWindow::useCustomRange(DateRange range)
{
    // plot using the supplied range
    useCustom = true;
    useToToday = false;
    custom = range;
    dateRangeChanged(custom);
}

void
LTMWindow::useStandardRange()
{
    useToToday = useCustom = false;
    dateRangeChanged(myDateRange);
}

void
LTMWindow::useThruToday()
{
    // plot using the supplied range
    useCustom = false;
    useToToday = true;
    custom = myDateRange;
    if (custom.to > QDate::currentDate()) custom.to = QDate::currentDate();
    dateRangeChanged(custom);
}

// total redraw, reread data etc
void
LTMWindow::refresh()
{
    setProperty("color", GColor(CPLOTBACKGROUND)); // called on config change

    // not if in compare mode
    if (isCompare()) return; 

    // refresh for changes to ridefiles / zones
    if (amVisible() == true && context->athlete->metricDB != NULL) {
        results.clear(); // clear any old data
        results = context->athlete->metricDB->getAllMetricsFor(settings.start, settings.end);
        measures.clear(); // clear any old data
        measures = context->athlete->metricDB->getAllMeasuresFor(settings.start, settings.end);
        bestsresults.clear();
        bestsresults = RideFileCache::getAllBestsFor(context, settings.metrics, settings.start, settings.end);
        refreshPlot();
        repaint(); // title changes color when filters change

    } else {
        stackDirty = dirty = true;
    }
}

void
LTMWindow::dateRangeChanged(DateRange range)
{
    // do we need to use custom range?
    if (useCustom || useToToday) range = custom;

    // we already plotted that date range
    if (amVisible() || dirty || range.from != plotted.from || range.to  != plotted.to) {

         settings.data = &results;
         settings.measures = &measures;
         settings.bests = &bestsresults;

        // we let all the state get updated, but lets not actually plot
        // whilst in compare mode -- but when compare mode ends we will
        // call filterChanged, so need to record the fact that the date
        // range changed whilst we were in compare mode
        if (!isCompare()) {

            // apply filter to new date range too -- will also refresh plot
            filterChanged();
        } else {

            // we've been told to redraw so maybe
            // compare mode was switched whilst we were
            // not visible, lets refresh
            if (compareDirty) compareChanged();
        }
    }
}

void
LTMWindow::filterChanged()
{
    // ignore in compare mode
    if (isCompare()) return;

    if (amVisible() == false || context->athlete->metricDB == NULL) return;

    if (useCustom) {

        settings.start = QDateTime(custom.from, QTime(0,0));
        settings.end   = QDateTime(custom.to, QTime(24,0,0));

    } else if (useToToday) {

        settings.start = QDateTime(myDateRange.from, QTime(0,0));
        settings.end   = QDateTime(myDateRange.to, QTime(24,0,0));

        QDate today = QDate::currentDate();
        if (settings.end.date() > today) settings.end = QDateTime(today, QTime(24,0,0));

    } else {

        settings.start = QDateTime(myDateRange.from, QTime(0,0));
        settings.end   = QDateTime(myDateRange.to, QTime(24,0,0));

    }
    settings.title = myDateRange.name;
    settings.data = &results;
    settings.bests = &bestsresults;
    settings.measures = &measures;

    // if we want weeks and start is not a monday go back to the monday
    int dow = settings.start.date().dayOfWeek();
    if (settings.groupBy == LTM_WEEK && dow >1 && settings.start != QDateTime(QDate(), QTime(0,0)))
        settings.start = settings.start.addDays(-1*(dow-1));

    // we need to get data again and apply filter
    results.clear(); // clear any old data
    results = context->athlete->metricDB->getAllMetricsFor(settings.start, settings.end);
    measures.clear(); // clear any old data
    measures = context->athlete->metricDB->getAllMeasuresFor(settings.start, settings.end);
    bestsresults.clear();
    bestsresults = RideFileCache::getAllBestsFor(context, settings.metrics, settings.start, settings.end);

    // loop through results removing any not in stringlist..
    if (ltmTool->isFiltered()) {

        // metrics filtering
        QList<SummaryMetrics> filteredresults;
        foreach (SummaryMetrics x, results) {
            if (ltmTool->filters().contains(x.getFileName()))
                filteredresults << x;
        }
        results = filteredresults;

        // metrics filtering
        QList<SummaryMetrics> filteredbestsresults;
        foreach (SummaryMetrics x, bestsresults) {
            if (ltmTool->filters().contains(x.getFileName()))
                filteredbestsresults << x;
        }
        bestsresults = filteredbestsresults;

        settings.data = &results;
        settings.measures = &measures;
        settings.bests = &bestsresults;
    }

    if (context->ishomefiltered) {

        // metrics filtering
        QList<SummaryMetrics> filteredresults;
        foreach (SummaryMetrics x, results) {
            if (context->homeFilters.contains(x.getFileName()))
                filteredresults << x;
        }
        results = filteredresults;

        // metrics filtering
        QList<SummaryMetrics> filteredbestsresults;
        foreach (SummaryMetrics x, bestsresults) {
            if (context->homeFilters.contains(x.getFileName()))
                filteredbestsresults << x;
        }
        bestsresults = filteredbestsresults;

        settings.data = &results;
        settings.measures = &measures;
        settings.bests = &bestsresults;
    }

    refreshPlot();

    repaint(); // just for the title..
}

void
LTMWindow::rGroupBySelected(int selected)
{
    if (selected >= 0) {
        settings.groupBy = ltmTool->groupBy->itemData(selected).toInt();
        ltmTool->groupBy->setCurrentIndex(selected);
    }
}

void
LTMWindow::groupBySelected(int selected)
{
    if (selected >= 0) {
        settings.groupBy = ltmTool->groupBy->itemData(selected).toInt();
        rGroupBy->setValue(selected);
        refreshPlot();
    }
}

void
LTMWindow::showDataClicked(int state)
{
    bool checked = state;

    // only change if changed, to avoid endless looping
    if (ltmTool->showData->isChecked() != checked) ltmTool->showData->setChecked(checked);
    if (rData->isChecked()!=checked) rData->setChecked(checked);

    if (settings.showData != checked) {
        settings.showData = checked;
        refreshPlot();
    }
}

void
LTMWindow::shadeZonesClicked(int state)
{
    bool checked = state;

    // only change if changed, to avoid endless looping
    if (ltmTool->shadeZones->isChecked() != checked) ltmTool->shadeZones->setChecked(checked);
    settings.shadeZones = state;
    refreshPlot();
}

void
LTMWindow::showLegendClicked(int state)
{
    settings.legend = state;
    refreshPlot();
}

void
LTMWindow::showEventsClicked(int state)
{
    settings.events = bool(state);
    refreshPlot();
}

void
LTMWindow::showStackClicked(int state)
{
    bool checked = state;

    // only change if changed, to avoid endless looping
    if (ltmTool->showStack->isChecked() != checked) ltmTool->showStack->setChecked(checked);
    if (rStack->isChecked() != checked) rStack->setChecked(checked);

    settings.stack = checked;
    refreshPlot();
}

void
LTMWindow::applyClicked()
{
    if (ltmTool->charts->selectedItems().count() == 0) return;

    int selected = ltmTool->charts->invisibleRootItem()->indexOfChild(ltmTool->charts->selectedItems().first());
    if (selected >= 0) {

        // save chart setup
        int groupBy = settings.groupBy;
        bool legend = settings.legend;
        bool events = settings.events;
        bool stack = settings.stack;
        bool shadeZones = settings.shadeZones;
        QDateTime start = settings.start;
        QDateTime end = settings.end;

        // apply preset
        settings = context->athlete->presets[selected];

        // now get back the local chart setup
        settings.ltmTool = ltmTool;
        settings.data = &results;
        settings.bests = &bestsresults;
        settings.measures = &measures;
        settings.groupBy = groupBy;
        settings.legend = legend;
        settings.events = events;
        settings.stack = stack;
        settings.shadeZones = shadeZones;
        settings.start = start;
        settings.end = end;

        ltmTool->applySettings();
        refresh();
    }
}

int
LTMWindow::groupForDate(QDate date)
{
    switch(settings.groupBy) {
    case LTM_WEEK:
        {
        // must start from 1 not zero!
        return 1 + ((date.toJulianDay() - settings.start.date().toJulianDay()) / 7);
        }
    case LTM_MONTH: return (date.year()*12) + date.month();
    case LTM_YEAR:  return date.year();
    case LTM_DAY:
    default:
        return date.toJulianDay();

    }
}
void
LTMWindow::pointClicked(QwtPlotCurve*curve, int index)
{
    // initialize date and time to senseful boundaries
    QDate start = QDate(1900,1,1);
    QDate end   = QDate(2999,12,31);
    QTime time = QTime(0, 0, 0, 0);

    // now fill the correct values for context
    if (settings.groupBy != LTM_TOD) {
      // get the date range for this point (for all date-dependent grouping)
      LTMScaleDraw *lsd = new LTMScaleDraw(settings.start,
                          groupForDate(settings.start.date()),
                          settings.groupBy);
      lsd->dateRange((int)round(curve->sample(index).x()), start, end); }
    else {
      // special treatment for LTM_TOD as time dependent grouping
      time = QTime((int)round(curve->sample(index).x()), 0, 0, 0);
    }
    // feed the popup with data
    ltmPopup->setData(settings, start, end, time);
    popup->show();
}

class GroupedData {

    public:
        QVector<double> x, y; // x and y data for each metric
        int maxdays;
};

void
LTMWindow::refreshDataTable()
{
    // clear to force refresh
	dataSummary->page()->mainFrame()->setHtml("");

    // get string
    QString summary = dataTable(true);

    // now set it
	dataSummary->page()->mainFrame()->setHtml(summary);
}

QString
LTMWindow::dataTable(bool html)
{
    // truncate date range to the actual data when not set to any date
    if (settings.data != NULL && (*settings.data).count() != 0) {

        // end
        if (settings.end == QDateTime() || settings.end.date() > QDate::currentDate().addYears(40))
                settings.end = (*settings.data).last().getRideDate();

        // start
        if (settings.start == QDateTime() || settings.start.date() < QDate::currentDate().addYears(-40))
            settings.start = (*settings.data).first().getRideDate();
    }

    // need to redo this
    ltmPlot->resetPMC();

    // now set to new (avoids a weird crash)
    QString summary;

    QColor bgColor = GColor(CPLOTBACKGROUND);
    QColor altColor = GCColor::alternateColor(bgColor);

    // html page prettified with a title
    if (html) {

        summary = GCColor::css();
        summary += "<center>";

        // device summary for ride summary, otherwise how many activities?
        summary += "<p><h3>" + settings.title + tr(" grouped by ");

        switch (settings.groupBy) {
        case LTM_DAY :
            summary += tr("day");
            break;
        case LTM_WEEK :
            summary += tr("week");
            break;
        case LTM_MONTH :
            summary += tr("month");
            break;
        case LTM_YEAR :
            summary += tr("year");
            break;
        case LTM_TOD :
            summary += tr("time of day");
            break;
        case LTM_ALL :
            summary += tr("All");
            break;
        }
        summary += "</h3><p>";
    }

    //
    // STEP1: AGGREGATE DATA INTO GROUPBY FOR EACH METRIC
    //        This is essentially a refactored version of createCurveData
    //        from LTMPlot, but updated to produce aggregates for all metrics
    //        at once, so we can embed into an HTML table
    //
    QList<GroupedData> aggregates;

    // for estimates we take the metric data and augment
    // it with the estimate that applies for that date
    QList<SummaryMetrics> estimates = *(settings.data);

    foreach (MetricDetail metricDetail, settings.metrics) {

        // do we aggregate zero values ?
        bool aggZero = metricDetail.metric ? metricDetail.metric->aggregateZero() : false;

        QList<SummaryMetrics> *data = NULL; // source data (metrics, bests etc)
        GroupedData a; // aggregated data

        // resize the curve array to maximum possible size
        a.maxdays = groupForDate(settings.end.date()) - groupForDate(settings.start.date());
        a.x.resize(a.maxdays+1);
        a.y.resize(a.maxdays+1);


        // set source for data
        QList<SummaryMetrics> PMCdata;
        if (metricDetail.type == METRIC_DB || metricDetail.type == METRIC_META) {
            data = settings.data;
        } else if (metricDetail.type == METRIC_MEASURE) {
            data = settings.measures;
        } else if (metricDetail.type == METRIC_PM) {
            // PMC fixup later
            ltmPlot->createPMCCurveData(context, &settings, metricDetail, PMCdata);
            data = &PMCdata;
        } else if (metricDetail.type == METRIC_BEST) {
            data = settings.bests;
        } else if (metricDetail.type == METRIC_ESTIMATE) {

            // WE BASICALLY TAKE A COPY OF THE RIDE METRICS AND
            // ADD IN THE ESTIMATE FOR THAT DAY -- SO YOU ONLY
            // GET ESTIMATES FOR DAYS WITH RIDES

            // lets refresh the model data if we don't have any
            if (context->athlete->PDEstimates.count() == 0) 
                context->athlete->metricDB->refreshCPModelMetrics(); 

            // lets nip through all the rides and add in the estimate
            // that applied for that date
            data = &estimates;

            // update each ride by adding the estimate for that day
            for (int i=0; i<estimates.count(); i++) {

                // get the date
                QDate date = estimates[i].getRideDate().date();

                // get the value
                double value = 0;
                foreach(PDEstimate est, context->athlete->PDEstimates) {

                    // ooh this is a match!
                    if (date >= est.from && date <= est.to && est.wpk == metricDetail.wpk 
                         && est.model == metricDetail.model) {

                        switch(metricDetail.estimate) {
                        case ESTIMATE_WPRIME :
                            value = est.WPrime;
                            break;

                        case ESTIMATE_CP :
                            value = est.CP;
                            break;

                        case ESTIMATE_FTP :
                            value = est.FTP;
                            break;

                        case ESTIMATE_PMAX :
                            value = est.PMax;
                            break;

                        case ESTIMATE_BEST :
                            {
                                value = 0;

                                // we need to find the model 
                                foreach(PDModel *model, ltmPlot->models) {

                                    // not the one we want
                                    if (model->code() != metricDetail.model) continue;

                                    // set the parameters previously derived
                                    model->loadParameters(est.parameters);

                                    // get the model estimate for our duration
                                    value = model->y(metricDetail.estimateDuration * metricDetail.estimateDuration_units);
                                }
                            }
                            break;

                        case ESTIMATE_EI :
                            value = est.EI;
                            break;
                        }
                        break;
                    }
                }

                // insert the value for this symbol
                estimates[i].setForSymbol(metricDetail.symbol, value);
            }
        }

        // initialise before looping through the data for this metric
        int n=-1;
        int lastDay=groupForDate(settings.start.date());
        unsigned long secondsPerGroupBy=0;
        bool wantZero = true;

        foreach (SummaryMetrics rideMetrics, *data) { 

            // filter out unwanted rides but not for PMC type metrics
            // because that needs to be done in the stress calculator
            if (metricDetail.type != METRIC_PM && context->isfiltered && 
                !context->filters.contains(rideMetrics.getFileName())) continue;

            // day we are on
            int currentDay = groupForDate(rideMetrics.getRideDate().date());

            // value for day -- measures are stored differently
            double value;
            if (metricDetail.type == METRIC_MEASURE)
                value = rideMetrics.getText(metricDetail.symbol, "0.0").toDouble();
            else if (metricDetail.type == METRIC_BEST)
                value = rideMetrics.getForSymbol(metricDetail.bestSymbol);
            else
                value = rideMetrics.getForSymbol(metricDetail.symbol);

            // check values are bounded to stop QWT going berserk
            if (isnan(value) || isinf(value)) value = 0;

            // set aggZero to false and value to zero if is temperature and -255
            if (metricDetail.metric && metricDetail.metric->symbol() == "average_temp" && value == RideFile::NoTemp) {
                value = 0;
                aggZero = false;
            }

            // Special computed metrics (LTS/STS) have a null metric pointer
            if (metricDetail.type != METRIC_BEST && metricDetail.metric) {
                // Because the metrics may have been setup differently than the athlete profile, try to
                // avoid aberrations by trying to match the units in the legend; if they are not
                // recognized, then trust that the user knows what she wants (or has been given).
                // What we should do is separate user axis labeling from selected units in the UI and model.
                bool convert;
                if (metricDetail.metric->units(true) == metricDetail.uunits) {
                    convert = false;
                } else if (metricDetail.metric->units(false) == metricDetail.uunits) {
                    convert = true;
                } else {
                    convert = context->athlete->useMetricUnits == false;
                }
                // convert from stored metric value to imperial if warranted
                if (convert) {
                    value *= metricDetail.metric->conversion();
                    value += metricDetail.metric->conversionSum();
                }


            // convert seconds to hours
            if (metricDetail.metric->units(true) == "seconds" ||
                metricDetail.metric->units(true) == tr("seconds")) value /= 3600;
            }

            if (value || wantZero) {
                unsigned long seconds = rideMetrics.getForSymbol("workout_time");
                if (metricDetail.type == METRIC_BEST || metricDetail.type == METRIC_MEASURE) seconds = 1;
                if (n < a.x.size() && currentDay > lastDay) {
                    if (lastDay && wantZero) {
                        while (n<(a.x.size()-1) && lastDay<currentDay) {
                            lastDay++;
                            n++;
                            a.x[n]=lastDay - groupForDate(settings.start.date());
                            a.y[n]=0;
                        }
                    } else {
                        n++;
                    }

                    a.y[n] = value;
                    a.x[n] = currentDay - groupForDate(settings.start.date());

                    if (value || aggZero) secondsPerGroupBy = seconds; // reset for new group

                } else {
                    // sum totals, average averages and choose best for Peaks
                    int type = metricDetail.metric ? metricDetail.metric->type() : RideMetric::Average;

                    if (metricDetail.uunits == "Ramp" ||
                        metricDetail.uunits == tr("Ramp")) type = RideMetric::Total;

                    if (metricDetail.type == METRIC_BEST) type = RideMetric::Peak;

                    // just in case
                    if (n < 0) n=0;

                    switch (type) {
                    case RideMetric::Total:
                        a.y[n] += value;
                        break;
                    case RideMetric::Average:
                        {
                        // average should be calculated taking into account
                        // the duration of the ride, otherwise high value but
                        // short rides will skew the overall average
                        if (value || aggZero) a.y[n] = ((a.y[n]*secondsPerGroupBy)+(seconds*value)) / (secondsPerGroupBy+seconds);
                        break;
                        }
                    case RideMetric::Low:
                        if (value < a.y[n]) a.y[n] = value;
                        break;
                    case RideMetric::Peak:
                        if (value > a.y[n]) a.y[n] = value;
                        break;
                    }
                    secondsPerGroupBy += seconds; // increment for same group
                }
                lastDay = currentDay;
            }
        }

        // save to our list
        aggregates << a;
    }

    //
    // STEP 2: PREPARE HTML TABLE FROM AGGREGATED DATA
    //         But note there will be no data if there are no curves of if there
    //         is no date range selected of no data anyway!
    //
    if (aggregates.count()) {

        // fill in the remainder if data doesn't extend to
        // the period we are summarising
        if (settings.groupBy != LTM_ALL) {
            for (int n=0; n < aggregates[0].x.count(); n++) {
                aggregates[0].x[n] = n;
            }
        }

        // formatting ...
        LTMScaleDraw lsd(settings.start, groupForDate(settings.start.date()), settings.groupBy);

        if (html) {
            // table and headings 50% for 1 metric, 70% for 2 metrics, 90% for 3 metrics or more
            summary += "<table border=0 cellspacing=3 width=\"%1%%\"><tr><td align=\"center\" valigne=\"top\"><b>%2</b></td>";
            summary = summary.arg(settings.metrics.count() >= 3 ? 90 : (30 + (settings.metrics.count() * 20))).arg(tr("Date"));
        } else {
            summary += tr("Date");
        }

        // metric name
        for (int i=0; i < settings.metrics.count(); i++) {

            if (html) summary += "<td align=\"center\" valign=\"top\"><b>%1</b></td>";
            else summary += ", %1";

            QString name = settings.metrics[i].uname;
            if (name == "Coggan Acute Training Load" or name == tr("Coggan Acute Training Load")) name = "ATL";
            if (name == "Coggan Chronic Training Load" or name == tr("Coggan Chronic Training Load")) name = "CTL";
            if (name == "Coggan Training Stress Balance" or name == tr("Coggan Training Stress Balance")) name = "TSB";

            summary = summary.arg(name);
        }

        if (html) {

            // html table and units on next line
            summary += "</tr><tr><td></td>";

            // units
            for (int i=0; i < settings.metrics.count(); i++) {
                summary += "<td align=\"center\" valign=\"top\">"
                        "<b>%1</b></td>";
                QString units = settings.metrics[i].uunits;
                if (units == "seconds" || units == tr("seconds")) units = tr("hours");
                if (units == settings.metrics[i].uname) units = "";
                summary = summary.arg(units != "" ? QString("(%1)").arg(units) : "");
            }
            summary += "</tr>";

        } else {

            // end of heading for CSV
            summary += "\n";
        }

        int row=0;
        for(int i=0; i<aggregates[0].y.count(); i++) {

            // in day mode we don't list all the zeroes .. its too many!
            bool nonzero = false;
            if (settings.groupBy == LTM_DAY) {

                // nonzeros?
                for(int j=0; j<aggregates.count(); j++) 
                    if (int(aggregates[j].y[i])) nonzero = true;

                // skip all zeroes if day mode
                if (nonzero == false) continue;
            }

            // alternating colors on html output
            if (html) {
                if (row%2) summary += "<tr bgcolor='" + altColor.name() + "'>";
                else summary += "<tr>";
                row++;
            }

            // First column, date / month year etc
            if (html) summary += "<td align=\"center\" valign=\"top\">%1</td>";
            else summary += "%1";
            summary = summary.arg(lsd.label(aggregates[0].x[i]+0.5).text().replace("\n", " "));

            // Remaining columns - each metric value
            for(int j=0; j<aggregates.count(); j++) {
                if (html) summary += "<td align=\"center\" valign=\"top\">%1</td>";
                else summary += ", %1";

                // now format the actual value....
                const RideMetric *m = settings.metrics[j].metric;
                if (m != NULL) {

                    // handle precision of 1 for seconds converted to hours
                    int precision = m->precision();
                    if (settings.metrics[j].uunits == "seconds" || settings.metrics[j].uunits == tr("seconds")) precision=1;

                    // we have a metric so lets be precise ...
                    QString v = QString("%1").arg(aggregates[j].y[i], 0, 'f', precision);

                    summary = summary.arg(v);

                } else {
                    // no precision
                    summary = summary.arg(QString("%1").arg(aggregates[j].y[i], 0, 'f', 0));
                }
            }

            // ok, this row is done
            if (html) summary += "</tr>";
            else summary += "\n"; // csv newline
        }

        // close table on html page
        if (html) summary += "</table>";
    }

    // all done !
    if (html) summary += "</center>";

    return summary;
}

void
LTMWindow::exportData()
{
    QString filename = title()+".csv";
    filename = QFileDialog::getSaveFileName(this, tr("Save Chart Data as CSV"),  QString(), title()+".csv (*.csv)");

    if (!filename.isEmpty()) {

        // can we open the file ?
        QFile f(filename);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return; // couldn't open file

        // generate file content
        QString content = dataTable(false); // want csv

        // open stream and write header
        QTextStream stream(&f);
        stream << content;

        // and we're done
        f.close();
    }
}
