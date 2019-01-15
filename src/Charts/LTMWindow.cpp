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
#include "Banister.h"
#include "RideCache.h"
#include "RideFileCache.h"
#include "Settings.h"
#include "cmath"
#include "float.h"
#include "Units.h" // for MILES_PER_KM
#include "HelpWhatsThis.h"
#include "GcOverlayWidget.h"

#ifdef NOWEBKIT
#include <QWebEngineSettings>
#include <QDesktopWidget>
#endif

#include <QtGlobal>
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
            GcChartWindow(context), context(context), dirty(true), stackDirty(true), compareDirty(true), firstshow(true)
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
    
    ltmPlot = new LTMPlot(this, context, 0);
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
#if QT_VERSION >= 0x050800
    // stop stealing focus!
    dataSummary->settings()->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, false);
#endif
    //XXXdataSummary->setEnabled(false); // stop grabbing focus
    if (dpiXFactor > 1) {
    // 80 lines per page on hidpi screens (?)
        int pixelsize = pixelSizeForFont(defaultFont, QApplication::desktop()->geometry().height()/80);
        dataSummary->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, pixelsize);
    } else {
        dataSummary->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, defaultFont.pointSize()+1);
    }
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
    rGroupBy->setMinimumWidth(100);

    revealLayout->addWidget(rGroupBy);
    rData = new QCheckBox(tr("Data Table"), this);
    rStack = new QCheckBox(tr("Stacked"), this);
    QVBoxLayout *checks = new QVBoxLayout;
    checks->setSpacing(2 *dpiXFactor);
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

    // the banister overlay
    QWidget *ban=new QWidget(this);
    addHelper(tr("Banister Model"), ban);

    QGridLayout *bang= new QGridLayout(ban);
    bang->setColumnStretch(0, 40);
    bang->setColumnStretch(1, 30);
    bang->setColumnStretch(2, 20);

    // interactive elements
    banCombo = new QComboBox(this);
    banT1 = new QDoubleSpinBox(this);
    banT2 = new QDoubleSpinBox(this);

    // labels etc
    ilabel = new QLabel(tr("Impulse Metric"), this);
    plabel = new QLabel(tr("Peak"), this);
    peaklabel = new QLabel(this);
    peaklabel->setText("296w on 3rd July");
    t1label1 = new QLabel(tr("Positive decay"), this);
    t1label2 = new QLabel(tr("days"), this);
    t2label1 = new QLabel(tr("Negative decay"), this);
    t2label2 = new QLabel(tr("days"), this);
    RMSElabel = new QLabel(this);
    RMSElabel->setText("RMSE 2.9 for 22 tests.");

    // add to layout
    bang->addWidget(ilabel,0,0);
    bang->addWidget(banCombo,0,1,1,2,Qt::AlignLeft);
    bang->addWidget(plabel, 1,0);
    bang->addWidget(peaklabel,1,1,1,2,Qt::AlignLeft);
    bang->addWidget(t1label1,2,0);
    bang->addWidget(banT1,2,1);
    bang->addWidget(t1label2,2,2);
    bang->addWidget(t2label1,3,0);
    bang->addWidget(banT2,3,1);
    bang->addWidget(t2label2,3,2);
    bang->addWidget(RMSElabel,4,0,1,3);

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
    connect(ltmTool->showBanister, SIGNAL(stateChanged(int)), this, SLOT(refresh()));
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
    connect(context, SIGNAL(estimatesRefreshed()), this, SLOT(refresh()));

    // comparing things
    connect(context, SIGNAL(compareDateRangesStateChanged(bool)), this, SLOT(compareChanged()));
    connect(context, SIGNAL(compareDateRangesChanged()), this, SLOT(compareChanged()));

    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(rideSaved(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(presetSelected(int)), this, SLOT(presetSelected(int)));
    connect(context->athlete->seasons, SIGNAL(seasonsChanged()), this, SLOT(refreshPlot()));

    // custom menu item
    connect(exportData, SIGNAL(triggered()), this, SLOT(exportData()));

    // normal view
    connect(spanSlider, SIGNAL(lowerPositionChanged(int)), this, SLOT(spanSliderChanged()));
    connect(spanSlider, SIGNAL(upperPositionChanged(int)), this, SLOT(spanSliderChanged()));
    connect(scrollLeft, SIGNAL(clicked()), this, SLOT(moveLeft()));
    connect(scrollRight, SIGNAL(clicked()), this, SLOT(moveRight()));

    // refresh banister data when combo changes
    connect(banCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(refreshBanister(int)));
    connect(banT1, SIGNAL(valueChanged(double)), this, SLOT(tuneBanister()));
    connect(banT2, SIGNAL(valueChanged(double)), this, SLOT(tuneBanister()));
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
LTMWindow::showBanister(bool relevant)
{
    RideMetricFactory &factory = RideMetricFactory::instance();

    if (relevant && banister()) {

        // we reset the combo so lets remember where we were at
        int remember=0;
        if (banCombo->count()) remember=banCombo->currentIndex();

        QStringList symbols;

        // lets setup the widgets
        banCombo->clear();
        foreach(MetricDetail metricDetail, settings.metrics) {
            if (metricDetail.type == METRIC_BANISTER) {
                if (!symbols.contains(metricDetail.symbol)) {
                    const RideMetric *m = factory.rideMetric(metricDetail.symbol);
                    if (m) {
                        symbols << metricDetail.symbol;

                        // bloody TM in bikescore is TEE DEE US
                        QString name=m->name();
                        if (name.startsWith("BikeScore")) name = QString("BikeScore");

                        banCombo->addItem(name, QVariant(metricDetail.symbol));
                    }
                }
            }
        }

        // go back to remembered value
        if (remember < banCombo->count()) banCombo->setCurrentIndex(remember);

        // now get the metric values etc
        refreshBanister(banCombo->currentIndex());
        overlayWidget->show();

    } else {

        // ignore it
        overlayWidget->hide();
    }
}

bool
LTMWindow::event(QEvent *event)
{
    // nasty nasty nasty hack to move widgets as soon as the widget geometry
    // is set properly by the layout system, by default the width is 100 and
    // we wait for it to be set properly then put our helper widget on the RHS
    if (event->type() == QEvent::Resize && geometry().width() != 100) {

        // put somewhere nice on first show
        if (firstshow) {
            firstshow = false;
            helperWidget()->resize(400*dpiXFactor, 150*dpiYFactor);
            helperWidget()->move(mainWidget()->geometry().width()-(500*dpiXFactor), 90*dpiYFactor);
        }

        // if off the screen move on screen
        if (helperWidget()->geometry().x() > geometry().width()) {
            helperWidget()->move(mainWidget()->geometry().width()-(500*dpiXFactor), 90*dpiYFactor);
        }
    }
    return QWidget::event(event);
}

void
LTMWindow::tuneBanister()
{

    // if we have a banister...
    if (banCombo->count() && banT1->value() >0 && banT2->value()>0) {

        // lookup and set
        Banister *banister = context->athlete->getBanisterFor(banCombo->currentData().toString(),0,0);

        // when user adjusts the t1/t2 parameters we need to refit
        if (banT1->value() < banister->t1 || banT1->value() > banister->t1 ||
            banT2->value() < banister->t2 || banT2->value() > banister->t2) {

            // lets adjust it them
            banister->setDecay(banT1->value(), banT2->value());
        }

        // replot
        refreshPlot();
    }
}
void
LTMWindow::refreshBanister(int index)
{
    if (index >= 0 && index < banCombo->count()) {

        // lookup and set
        Banister *banister = context->athlete->getBanisterFor(banCombo->currentData().toString(),0,0);
        banT1->setValue(banister->t1);
        banT2->setValue(banister->t2);

        int CP=0;
        QDate when = banister->getPeakCP(settings.start.date(), settings.end.date(), CP);

        // set peak label
        if (CP >0 && when != QDate()) peaklabel ->setText(QString("%1 watts on %2").arg(CP).arg(when.toString("d MMM yyyy")));
        else peaklabel->setText("");

        // set RMSE for current view
        int count;
        double RMSE = banister->RMSE(settings.start.date(), settings.end.date(), count);
        if (count && RMSE >0) RMSElabel->setText(QString("RMSE %1 for %2 tests.").arg(RMSE, 0, 'f', 2).arg(count));
        else RMSElabel->setText("");

    } else {
        // clear
        peaklabel->setText("");
        RMSElabel->setText("");
    }
}

void
LTMWindow::configChanged(qint32)
{
    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CTRENDPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);

    // inverted palette for data etc
    QPalette whitepalette;
    whitepalette.setBrush(QPalette::Window, QBrush(GColor(CTRENDPLOTBACKGROUND)));
    whitepalette.setBrush(QPalette::Background, QBrush(GColor(CTRENDPLOTBACKGROUND)));
    whitepalette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CTRENDPLOTBACKGROUND)));
    whitepalette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    whitepalette.setColor(QPalette::Text, GCColor::invertColor(GColor(CTRENDPLOTBACKGROUND)));

    QFont font;
    font.setPointSize(12); // reasonably big
    ilabel->setFont(font);
    plabel->setFont(font);
    t1label1->setFont(font);
    t1label2->setFont(font);
    plabel->setFont(font);
    peaklabel->setFont(font);
    t2label1->setFont(font);
    t2label2->setFont(font);
    RMSElabel->setFont(font);
    banT1->setFont(font);
    banT2->setFont(font);
    banCombo->setFont(font);

    ilabel->setPalette(palette);
    plabel->setPalette(palette);
    t1label1->setPalette(palette);
    t1label2->setPalette(palette);
    plabel->setPalette(palette);
    peaklabel->setPalette(whitepalette);
    t2label1->setPalette(palette);
    t2label2->setPalette(palette);
    RMSElabel->setPalette(whitepalette);

#ifndef Q_OS_MAC
    banT1->setStyleSheet(TabView::ourStyleSheet());
    banT2->setStyleSheet(TabView::ourStyleSheet());
    banCombo->setStyleSheet(TabView::ourStyleSheet());
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
            showBanister(false); // never
            stackWidget->setCurrentIndex(3);
            refreshCompare();

        } else if (ltmTool->showData->isChecked()) {

            //  DATA TABLE
            showBanister(false); // never
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

    int position = 0;

    // create ltmPlot with this
    if (plotSetting.metrics.count()) {

        compareplotSettings << plotSetting;

        // create and setup the plot
        LTMPlot *stacked = new LTMPlot(this, context, position++);
        stacked->setCompareData(&compareplotSettings.last()); // setData using the compare data
        stacked->setFixedHeight(200); // maybe make this adjustable later

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
        LTMPlot *plot = new LTMPlot(this, context, position++);
        plot->setCompareData(&compareplotSettings.last()); // setData using the compare data

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

    int position = 0;

    // create ltmPlot with this
    if (plotSetting.metrics.count()) {

        plotSettings << plotSetting;

        // create and setup the plot
        LTMPlot *stacked = new LTMPlot(this, context, position++);
        stacked->setData(&plotSettings.last());
        stacked->setFixedHeight(200); // maybe make this adjustable later

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
        LTMPlot *plot = new LTMPlot(this, context, position++);
        plot->setData(&plotSettings.last());

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
    bool firstXvalue=true;
    double lowestFirstXvalue = DBL_MAX;
    double highestFirstXvalue = 0.0;

    // create curve data for each metric detail to iterate over
    foreach(MetricDetail metricDetail, settings.metrics) {
        TableCurveData add;

        ltmPlot->settings=&settings; // for stack mode ltmPlot isn't set
        ltmPlot->createCurveData(context, &settings, metricDetail, add.x, add.y, add.n, true);

        columns << add;

        // check if "x" value of all metrics is the same for all colums and find
        // the lowest "x" value and highest "x" value to which all columns need to be aligned
        if (add.n > 0) {
            if (firstXvalue) {
                lowestFirstXvalue = highestFirstXvalue = add.x[0];
                firstXvalue = false;
            } else {
                if (add.x[0] < lowestFirstXvalue) {
                    lowestFirstXvalue = add.x[0];
                }
                if (add.x[0] > highestFirstXvalue) {
                    highestFirstXvalue = add.x[0];
                }
            }
        }

        // truncate to shortest set of rows available as 
        // we dont pad with zeroes in the data table
        if (first) rows=add.n;
        else if (add.n < rows) rows=add.n;
        first=false;
    }

    // align the starting X values of all columns using the
    // lowest xValue and highest xValue as borders
    // for columns which have data at all - and if there is something to adjust
    if (!firstXvalue && lowestFirstXvalue != highestFirstXvalue) {
        for (int i = 0; i< columns.count(); i++) {
            if (columns[i].n > 0) {
                double xValue = columns[i].x[0];
                while (columns[i].x[0] > lowestFirstXvalue) {
                    xValue--;
                    columns[i].x.prepend(xValue);
                    columns[i].y.prepend(0.0);
                    columns[i].n++;
                }
            }
        }
        // adjust number of visible rows in table
        rows += qRound(highestFirstXvalue-lowestFirstXvalue);
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

        QList<QVector<double> > hdatas;
        QList<QString> fontcolors;

        // highlight
        for (int a=0; a < settings.metrics.count(); a++) {
            MetricDetail metricDetail = settings.metrics[a];

            int brightness = metricDetail.penColor.red() *0.299 + metricDetail.penColor.green()*0.587 + metricDetail.penColor.blue()*0.114;
            fontcolors.append( brightness > 128 ? "#000" : "#fff" );

            // highlight lowest / top N values
            if (metricDetail.lowestN > 0 || metricDetail.topN > 0) {
                QMap<double, int> sortedList;

                // copy the yvalues, retaining the offset
                for(int i=0; i<columns[a].y.count(); i++) {
                    // pmc metrics we highlight TROUGHS
                    if (metricDetail.type == METRIC_STRESS || metricDetail.type == METRIC_PM) {
                        if (i && i < (columns[a].y.count()-1) // not at start/end
                            && ((columns[a].y[i-1] > columns[a].y[i] && columns[a].y[i+1] > columns[a].y[i]) || // is a trough
                                (columns[a].y[i-1] < columns[a].y[i] && columns[a].y[i+1] < columns[a].y[i])))  // is a peak
                            sortedList.insert(columns[a].y[i], i);
                    } else
                        sortedList.insert(columns[a].y[i], i);
                }

                // copy the top N values
                QVector<double> hdata;
                hdata.resize(metricDetail.topN + metricDetail.lowestN);


                // QMap orders the list so start at the top and work
                // backwards for topN
                int counter = 0;
                QMapIterator<double, int> i(sortedList);
                if (metricDetail.topN) {
                    i.toBack();
                    while (i.hasPrevious() && counter < metricDetail.topN) {
                        i.previous();
                        hdata[counter] = i.value();
                        counter++;
                    }
                }

                if (metricDetail.lowestN) {
                    i.toFront();
                    counter = 0; // and forwards for bottomN
                    while (i.hasNext() && counter < metricDetail.lowestN) {
                        i.next();
                        hdata[metricDetail.topN + counter] = i.value();
                        counter++;
                    }
                }
                hdatas.append(hdata);
            } else {
                // add an empty vector to maintain alignment with fontcolors
                QVector<double> hdata;
                hdatas.append(hdata);
            }
        }

        // metric name
        for (int i=0; i < settings.metrics.count(); i++) {

            if (html) summary += "<td align=\"center\" style=\"font-weight:bold;background-color:%2;color:%3\" valign=\"top\">%1</td>";
            else summary += ", %1";

            QString name = settings.metrics[i].uname;
            QString bcolor = settings.metrics[i].penColor.lighter(80).name();

            if (name == "Coggan Acute Training Load" || name == tr("Coggan Acute Training Load")) name = "ATL";
            if (name == "Coggan Chronic Training Load" || name == tr("Coggan Chronic Training Load")) name = "CTL";
            if (name == "Coggan Training Stress Balance" || name == tr("Coggan Training Stress Balance")) name = "TSB";

            summary = summary.arg(name);
            if (html) {
                summary = summary.arg(bcolor);
                summary = summary.arg(fontcolors.at(i));
            }
        }

        if (html) {

            // html table and units on next line
            summary += "</tr><tr><td></td>";

            // units
            for (int i=0; i < settings.metrics.count(); i++) {
                summary += "<td align=\"center\" style=\"font-weight:bold;background-color:%2;color:%3\" valign=\"top\">"
                        "%1</td>";
                QString units = settings.metrics[i].uunits;
                QString bcolor = settings.metrics[i].penColor.lighter(80).name();


                if (units == "seconds" || units == tr("seconds")) units = tr("hours");
                if (units == settings.metrics[i].uname) units = "";
                summary = summary.arg(units != "" ? QString("(%1)").arg(units) : "");
                summary = summary.arg(bcolor);
                summary = summary.arg(fontcolors.at(i));
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
                if (html) summary += "<td align=\"center\" style=\"%2\" valign=\"top\">%1</td>";
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
                    summary = summary.arg(QString("%1").arg(columns[j].y[row], 0, 'f', 1));
                }

                //
                if (hdatas.at(j).contains(row)) {
                    QString c = QString("background-color:%1;color:%2").arg(settings.metrics[j].penColor.name()).arg(fontcolors.at(j));
                    summary = summary.arg(c);
                } else
                    summary = summary.arg("");
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
