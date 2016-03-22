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
#include "LTMChartParser.h"
#include "TabView.h"
#include "Context.h"
#include "Context.h"
#include "Athlete.h"
#include "RideCache.h"
#include "RideFileCache.h"
#include "Settings.h"
#include "cmath"
#include "Units.h" // for MILES_PER_KM
#include "HelpWhatsThis.h"

#ifdef GC_HAS_CLOUD_DB
#include "CloudDBCommon.h"
#include "CloudDBChart.h"
#include "GcUpgrade.h"
#endif

#ifdef NOWEBKIT
#include <QWebEngineSettings>
#endif

#include <QtGui>
#include <QString>
#include <QDebug>
#include <QStyle>
#include <QStyleFactory>

// span slider specials
#include <qxtspanslider.h>
#include <QStyleFactory>
#include <QStyle>
#include <QScrollBar>

#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_marker.h>

LTMWindow::LTMWindow(Context *context) :
            GcChartWindow(context), context(context), dirty(true), stackDirty(true), compareDirty(true)
{
    useToToday = useCustom = false;
    plotted = DateRange(QDate(01,01,01), QDate(01,01,01));
    lastRefresh = QTime::currentTime().addSecs(-10);

    // the plot
    QVBoxLayout *mainLayout = new QVBoxLayout;

    QPalette palette;
    palette.setBrush(QPalette::Background, QBrush(GColor(CTRENDPLOTBACKGROUND)));

    // single plot
    plotWidget = new QWidget(this);
    plotWidget->setPalette(palette);
    QVBoxLayout *plotLayout = new QVBoxLayout(plotWidget);
    plotLayout->setSpacing(0);
    plotLayout->setContentsMargins(0,0,0,0);
    
    ltmPlot = new LTMPlot(this, context, true);
    spanSlider = new QxtSpanSlider(Qt::Horizontal, this);
    spanSlider->setFocusPolicy(Qt::NoFocus);
    spanSlider->setHandleMovementMode(QxtSpanSlider::NoOverlapping);
    spanSlider->setLowerValue(0);
    spanSlider->setUpperValue(15);

    QFont smallFont;
    smallFont.setPointSize(6);

    scrollLeft = new QPushButton("<", this);
    scrollLeft->setFont(smallFont);
    scrollLeft->setAutoRepeat(true);
    scrollLeft->setFixedHeight(16);
    scrollLeft->setFixedWidth(16);
    scrollLeft->setContentsMargins(0,0,0,0);

    scrollRight = new QPushButton(">", this);
    scrollRight->setFont(smallFont);
    scrollRight->setAutoRepeat(true);
    scrollRight->setFixedHeight(16);
    scrollRight->setFixedWidth(16);
    scrollRight->setContentsMargins(0,0,0,0);

    QHBoxLayout *span = new QHBoxLayout;
    span->addWidget(scrollLeft);
    span->addWidget(spanSlider);
    span->addWidget(scrollRight);
    plotLayout->addWidget(ltmPlot);
    plotLayout->addLayout(span);

#ifdef Q_OS_MAC
    // BUG in QMacStyle and painting of spanSlider
    // so we use a plain style to avoid it, but only
    // on a MAC, since win and linux are fine
#if QT_VERSION > 0x5000
    QStyle *style = QStyleFactory::create("fusion");
#else
    QStyle *style = QStyleFactory::create("Cleanlooks");
#endif
    spanSlider->setStyle(style);
    scrollLeft->setStyle(style);
    scrollRight->setStyle(style);
#endif

    // the stack of plots
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
    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply
#ifdef NOWEBKIT
    dataSummary = new QWebEngineView(this);
    dataSummary->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, defaultFont.pointSize()+1);
    dataSummary->settings()->setFontFamily(QWebEngineSettings::StandardFont, defaultFont.family());
#else
    dataSummary = new QWebView(this);
    dataSummary->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize()+1);
    dataSummary->settings()->setFontFamily(QWebSettings::StandardFont, defaultFont.family());
#endif
    dataSummary->setContentsMargins(0,0,0,0);
    dataSummary->page()->view()->setContentsMargins(0,0,0,0);
    dataSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    dataSummary->setAcceptDrops(false);

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
    stackWidget->addWidget(plotWidget);
    stackWidget->addWidget(dataSummary);
    stackWidget->addWidget(plotArea);
    stackWidget->addWidget(compareplotArea);
    stackWidget->setCurrentIndex(0);
    mainLayout->addWidget(stackWidget);
    setChartLayout(mainLayout);

    HelpWhatsThis *helpStack = new HelpWhatsThis(stackWidget);
    stackWidget->setWhatsThis(helpStack->getWhatsThisText(HelpWhatsThis::ChartTrends_MetricTrends));

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
    QAction *exportConfig = new QAction(tr("Export Chart Configuration..."), this);
    addAction(exportConfig);
#ifdef GC_HAS_CLOUD_DB
    QAction *shareConfig = new QAction(tr("Export Chart Configuration to CloudDB..."), this);
    addAction(shareConfig);
#endif
    // the controls
    QWidget *c = new QWidget;
    c->setContentsMargins(0,0,0,0);
    HelpWhatsThis *helpConfig = new HelpWhatsThis(c);
    c->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartTrends_MetricTrends));
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
    connect(this, SIGNAL(styleChanged(int)), this, SLOT(styleChanged(int)));
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
    connect(context, SIGNAL(filterChanged()), this, SLOT(filterChanged()));
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(refreshUpdate(QDate)));
    connect(context, SIGNAL(refreshEnd()), this, SLOT(refresh()));

    // comparing things
    connect(context, SIGNAL(compareDateRangesStateChanged(bool)), this, SLOT(compareChanged()));
    connect(context, SIGNAL(compareDateRangesChanged()), this, SLOT(compareChanged()));

    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(rideSaved(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(presetSelected(int)), this, SLOT(presetSelected(int)));

    // custom menu item
    connect(exportData, SIGNAL(triggered()), this, SLOT(exportData()));
    connect(exportConfig, SIGNAL(triggered()), this, SLOT(exportConfig()));
#if GC_HAS_CLOUD_DB
    connect(shareConfig, SIGNAL(triggered()), this, SLOT(shareConfig()));
#endif

    // normal view
    connect(spanSlider, SIGNAL(lowerPositionChanged(int)), this, SLOT(spanSliderChanged()));
    connect(spanSlider, SIGNAL(upperPositionChanged(int)), this, SLOT(spanSliderChanged()));
    connect(scrollLeft, SIGNAL(clicked()), this, SLOT(moveLeft()));
    connect(scrollRight, SIGNAL(clicked()), this, SLOT(moveRight()));

    configChanged(CONFIG_APPEARANCE);
}

LTMWindow::~LTMWindow()
{
    delete popup;
}

void
LTMWindow::hideBasic()
{
    ltmTool->hideBasic();
}

void
LTMWindow::configChanged(qint32)
{
#ifndef Q_OS_MAC
    plotArea->setStyleSheet(TabView::ourStyleSheet());
    compareplotArea->setStyleSheet(TabView::ourStyleSheet());
#endif
    refresh();
}

void
LTMWindow::styleChanged(int style)
{
    if (style) {
        // hide spanslider
        spanSlider->hide();
        scrollLeft->hide();
        scrollRight->hide();
    } else {
        // show spanslider
        spanSlider->show();
        scrollLeft->show();
        scrollRight->show();
    }
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
        settings.bests = &bestsresults;
        settings.groupBy = groupBy;
        settings.legend = legend;
        settings.events = events;
        settings.stack = stack;
        settings.shadeZones = shadeZones;
        settings.start = start;
        settings.end = end;

        // Set the specification
        FilterSet fs;
        fs.addFilter(context->isfiltered, context->filters);
        fs.addFilter(context->ishomefiltered, context->homeFilters);
        fs.addFilter(ltmTool->isFiltered(), ltmTool->filters());
        settings.specification.setFilterSet(fs);
        settings.specification.setDateRange(DateRange(settings.start.date(), settings.end.date()));

        ltmTool->applySettings();
        refresh();

        setProperty("subtitle", settings.name);

    } else {

        setProperty("subtitle", property("title").toString());
    }
}

void
LTMWindow::refreshUpdate(QDate here)
{
    if (isVisible() && here > settings.start.date() && lastRefresh.secsTo(QTime::currentTime()) > 5) {
        lastRefresh = QTime::currentTime();
        refresh();
    }
}

void
LTMWindow::moveLeft()
{
    // move across by 5% of the span, or to zero if not much left
    int span = spanSlider->upperValue() - spanSlider->lowerValue();
    int delta = span / 20;
    if (delta > (spanSlider->lowerValue() - spanSlider->minimum()))
        delta = spanSlider->lowerValue() - spanSlider->minimum();

    spanSlider->setLowerValue(spanSlider->lowerValue()-delta);
    spanSlider->setUpperValue(spanSlider->upperValue()-delta);

    spanSliderChanged();
}

void
LTMWindow::moveRight()
{
    // move across by 5% of the span, or to zero if not much left
    int span = spanSlider->upperValue() - spanSlider->lowerValue();
    int delta = span / 20;
    if (delta > (spanSlider->maximum() - spanSlider->upperValue()))
        delta = spanSlider->maximum() - spanSlider->upperValue();

    spanSlider->setLowerValue(spanSlider->lowerValue()+delta);
    spanSlider->setUpperValue(spanSlider->upperValue()+delta);

    spanSliderChanged();
}

void
LTMWindow::spanSliderChanged()
{
    // so reset the axis range for ltmPlot
    ltmPlot->setAxisScale(QwtPlot::xBottom, spanSlider->lowerValue(), spanSlider->upperValue());
    ltmPlot->replot();
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

                spanSlider->setMinimum(ltmPlot->axisScaleDiv(QwtPlot::xBottom).lowerBound());
                spanSlider->setMaximum(ltmPlot->axisScaleDiv(QwtPlot::xBottom).upperBound());
                spanSlider->setLowerValue(spanSlider->minimum());
                spanSlider->setUpperValue(spanSlider->maximum());
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
    setProperty("color", GColor(CTRENDPLOTBACKGROUND)); // called on config change

    // not if in compare mode
    if (isCompare()) return; 

    // refresh for changes to ridefiles / zones
    if (amVisible() == true) {

        bestsresults.clear();
        bestsresults = RideFileCache::getAllBestsFor(context, settings.metrics, settings.specification);

        refreshPlot();
        repaint(); // title changes color when filters change

        // set spanslider to limits of ltmPlot

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

    if (amVisible() == false) return;

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

    // Set the specification
    FilterSet fs;
    fs.addFilter(context->isfiltered, context->filters);
    fs.addFilter(context->ishomefiltered, context->homeFilters);
    fs.addFilter(ltmTool->isFiltered(), ltmTool->filters());
    settings.specification.setFilterSet(fs);
    settings.specification.setDateRange(DateRange(settings.start.date(), settings.end.date()));

    // if we want weeks and start is not a monday go back to the monday
    int dow = settings.start.date().dayOfWeek();
    if (settings.groupBy == LTM_WEEK && dow >1 && settings.start != QDateTime(QDate(), QTime(0,0)))
        settings.start = settings.start.addDays(-1*(dow-1));

    // we need to get data again and apply filter
    bestsresults.clear();
    bestsresults = RideFileCache::getAllBestsFor(context, settings.metrics, settings.specification);
    settings.bests = &bestsresults;

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
        settings.bests = &bestsresults;
        settings.groupBy = groupBy;
        settings.legend = legend;
        settings.events = events;
        settings.stack = stack;
        settings.shadeZones = shadeZones;
        settings.start = start;
        settings.end = end;

        // Set the specification
        FilterSet fs;
        fs.addFilter(context->isfiltered, context->filters);
        fs.addFilter(context->ishomefiltered, context->homeFilters);
        fs.addFilter(ltmTool->isFiltered(), ltmTool->filters());
        settings.specification.setFilterSet(fs);
        settings.specification.setDateRange(DateRange(settings.start.date(), settings.end.date()));

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
#ifndef NOWEBKIT
    // clear to force refresh
    dataSummary->page()->mainFrame()->setHtml("");
#endif

    // get string
    QString summary = dataTable(true);

    // now set it
#ifdef NOWEBKIT
    dataSummary->page()->setHtml(summary);
#else
    dataSummary->page()->mainFrame()->setHtml(summary);
#endif
}

// for storing curve data without using a curve
class TableCurveData {
    public:
        TableCurveData() { n=0; x.resize(0); y.resize(0); }
        QVector<double> x,y;
        int n;
};

QString
LTMWindow::dataTable(bool html)
{
    // truncate date range to the actual data when not set to any date
    if (context->athlete->rideCache->rides().count()) {

        QDateTime first = context->athlete->rideCache->rides().first()->dateTime;
        QDateTime last = context->athlete->rideCache->rides().last()->dateTime;

        // end
        if (settings.end == QDateTime() || settings.end.date() > QDate::currentDate().addYears(40))
                settings.end = last;

        // start
        if (settings.start == QDateTime() || settings.start.date() < QDate::currentDate().addYears(-40))
            settings.start = first;
    }

    // now set to new (avoids a weird crash)
    QString summary;

    QColor bgColor = GColor(CTRENDPLOTBACKGROUND);
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
    //        This is performed by reusing the existing code in
    //        LTMPlot for creating curve data, but storing it
    //        in columns and forceing zero values
    QList<TableCurveData> columns;
    bool first=true;
    int rows = 0;

    // create curve data for each metric detail to iterate over
    foreach(MetricDetail metricDetail, settings.metrics) {
        TableCurveData add;

        ltmPlot->settings=&settings; // for stack mode ltmPlot isn't set
        ltmPlot->createCurveData(context, &settings, metricDetail, add.x, add.y, add.n, true);

        columns << add;

        // truncate to shortest set of rows available as 
        // we dont pad with zeroes in the data table
        if (first) rows=add.n;
        else if (add.n < rows) rows=add.n;
        first=false;
    }

    //
    // STEP 2: PREPARE HTML TABLE FROM AGGREGATED DATA
    //         But note there will be no data if there are no curves of if there
    //         is no date range selected of no data anyway!
    //
    if (rows) {

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
            if (name == "Coggan Acute Training Load" || name == tr("Coggan Acute Training Load")) name = "ATL";
            if (name == "Coggan Chronic Training Load" || name == tr("Coggan Chronic Training Load")) name = "CTL";
            if (name == "Coggan Training Stress Balance" || name == tr("Coggan Training Stress Balance")) name = "TSB";

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

        for(int row=0; row<=rows; row++) {

            // in day mode we don't list all the zeroes .. its too many!
            bool nonzero = false;
            if (settings.groupBy == LTM_DAY) {

                // nonzeros?
                for(int j=0; j<columns.count(); j++) 
                    if (int(columns[j].y[row])) nonzero = true;

                // skip all zeroes if day mode
                if (nonzero == false) continue;
            }

            // alternating colors on html output
            if (html) {
                if (row%2) summary += "<tr bgcolor='" + altColor.name() + "'>";
                else summary += "<tr>";
            }

            // First column, date / month year etc
            if (html) summary += "<td align=\"center\" valign=\"top\">%1</td>";
            else summary += "%1";
            summary = summary.arg(lsd.label(columns[0].x[row]+0.5).text().replace("\n", " "));

            // Remaining columns - each metric value
            for(int j=0; j<columns.count(); j++) {
                if (html) summary += "<td align=\"center\" valign=\"top\">%1</td>";
                else summary += ", %1";

                // now format the actual value....
                const RideMetric *m = settings.metrics[j].metric;
                if (m != NULL) {

                    // handle precision of 1 for seconds converted to hours
                    int precision = m->precision();
                    if (settings.metrics[j].uunits == "seconds" || settings.metrics[j].uunits == tr("seconds")) precision=1;

                    // we have a metric so lets be precise ...
                    QString v = QString("%1").arg(columns[j].y[row], 0, 'f', precision);

                    summary = summary.arg(v);

                } else {
                    // no precision
                    summary = summary.arg(QString("%1").arg(columns[j].y[row], 0, 'f', 0));
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
LTMWindow::exportConfig()
{
    // collect the config to export
    QList<LTMSettings> mine;
    mine << settings;
    mine[0].title = mine[0].name = title();

    // get a filename
    QString filename = title()+".xml";
    filename = QFileDialog::getSaveFileName(this, tr("Export Chart Config"),  filename, title()+".xml (*.xml)");

    // export it!
    if (!filename.isEmpty()) {
        LTMChartParser::serialize(filename, mine);
    }
}

#ifdef GC_HAS_CLOUD_DB
void
LTMWindow::shareConfig()
{
    // check for CloudDB T&C acceptance
    if (!(appsettings->cvalue(context->athlete->cyclist, GC_CLOUDDB_TC_ACCEPTANCE, false).toBool())) {
        CloudDBAcceptConditionsDialog acceptDialog(context->athlete->cyclist);
        acceptDialog.setModal(true);
        if (acceptDialog.exec() == QDialog::Rejected) {
            return;
        };
    }

    // collect the config to export
    QList<LTMSettings> mine;
    mine << settings;
    mine[0].title = mine[0].name = title();

    ChartAPIv1 chart;
    chart.Header.Name = title();
    int version = VERSION_LATEST;
    chart.Header.GcVersion =  QString::number(version);
    LTMChartParser::serializeToQString(&chart.ChartXML, mine);
    QPixmap picture;
    menuButton->hide();
#if QT_VERSION > 0x050000
    picture = grab(geometry());
#else
    picture = QPixmap::grabWidget (this);
#endif
    QBuffer buffer(&chart.Image);
    buffer.open(QIODevice::WriteOnly);
    picture.save(&buffer, "PNG"); // writes pixmap into bytes in PNG format (a bit larger than JPG - but much better in Quality when importing)
    buffer.close();

    chart.Header.CreatorId = appsettings->cvalue(context->athlete->cyclist, GC_ATHLETE_ID, "").toString();
    chart.Header.Curated = false;
    chart.Header.Deleted = false;

    // now complete the chart with for the user manually added fields
    CloudDBChartObjectDialog dialog(chart, context->athlete->cyclist);
    if (dialog.exec() == QDialog::Accepted) {
        CloudDBChartClient c;
        if (c.postChart(dialog.getChart())) {
            CloudDBHeader::setChartHeaderStale(true);
        }
    }
}
#endif


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
