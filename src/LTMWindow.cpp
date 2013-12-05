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

#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_marker.h>

LTMWindow::LTMWindow(Context *context) :
            GcChartWindow(context), context(context), dirty(true)
{
    setInstanceName("Metric Window");
    useToToday = useCustom = false;
    plotted = DateRange(QDate(01,01,01), QDate(01,01,01));

    // the plot
    QVBoxLayout *mainLayout = new QVBoxLayout;
    ltmPlot = new LTMPlot(this, context);
    mainLayout->addWidget(ltmPlot);
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
            << tr("Time Of Day");
    rGroupBy->setStrings(strings);
    rGroupBy->setValue(0);

    revealLayout->addWidget(rGroupBy);
    rShade = new QCheckBox(tr("Shade zones"), this);
    rEvents = new QCheckBox(tr("Show events"), this);
    QVBoxLayout *checks = new QVBoxLayout;
    checks->setSpacing(2);
    checks->setContentsMargins(0,0,0,0);
    checks->addWidget(rShade);
    checks->addWidget(rEvents);
    revealLayout->addLayout(checks);
    revealLayout->addStretch();
    setRevealLayout(revealLayout);

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

    // zoomer on the plot
    ltmZoomer = new QwtPlotZoomer(ltmPlot->canvas());
    ltmZoomer->setRubberBand(QwtPicker::RectRubberBand);
    ltmZoomer->setRubberBandPen(QColor(Qt::black));
    ltmZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    ltmZoomer->setEnabled(false);
    ltmZoomer->setMousePattern(QwtEventPattern::MouseSelect1,
                               Qt::LeftButton, Qt::ShiftModifier);
    ltmZoomer->setMousePattern(QwtEventPattern::MouseSelect2,
                               Qt::RightButton, Qt::ControlModifier);
    ltmZoomer->setMousePattern(QwtEventPattern::MouseSelect3,
                               Qt::RightButton);

    picker = new LTMToolTip(QwtPlot::xBottom, QwtPlot::yLeft,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOn,
                               ltmPlot->canvas(),
                               "");
    picker->setMousePattern(QwtEventPattern::MouseSelect1,
                            Qt::LeftButton);
    picker->setTrackerPen(QColor(Qt::black));
    QColor inv(Qt::white);
    inv.setAlpha(0);
    picker->setRubberBandPen(inv); // make it invisible
    picker->setEnabled(true);

    _canvasPicker = new LTMCanvasPicker(ltmPlot);

    ltmTool = new LTMTool(context, &settings);

    // initialise
    settings.ltmTool = ltmTool;
    settings.data = NULL;
    settings.groupBy = LTM_DAY;
    settings.legend = ltmTool->showLegend->isChecked();
    settings.events = ltmTool->showEvents->isChecked();
    settings.shadeZones = ltmTool->shadeZones->isChecked();
    rShade->setChecked(ltmTool->shadeZones->isChecked());
    rEvents->setChecked(ltmTool->showEvents->isChecked());
    cl->addWidget(ltmTool);

    connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    connect(ltmTool, SIGNAL(filterChanged()), this, SLOT(filterChanged()));
    connect(context, SIGNAL(homeFilterChanged()), this, SLOT(filterChanged()));
    connect(ltmTool->groupBy, SIGNAL(currentIndexChanged(int)), this, SLOT(groupBySelected(int)));
    connect(rGroupBy, SIGNAL(valueChanged(int)), this, SLOT(rGroupBySelected(int)));
    //!!! connect(ltmTool->saveButton, SIGNAL(clicked(bool)), this, SLOT(saveClicked(void)));
    connect(ltmTool->applyButton, SIGNAL(clicked(bool)), this, SLOT(applyClicked(void)));
    connect(ltmTool->shadeZones, SIGNAL(stateChanged(int)), this, SLOT(shadeZonesClicked(int)));
    connect(rShade, SIGNAL(stateChanged(int)), this, SLOT(shadeZonesClicked(int)));
    connect(ltmTool->showLegend, SIGNAL(stateChanged(int)), this, SLOT(showLegendClicked(int)));
    connect(ltmTool->showEvents, SIGNAL(stateChanged(int)), this, SLOT(showEventsClicked(int)));
    connect(rEvents, SIGNAL(stateChanged(int)), this, SLOT(showEventsClicked(int)));
    connect(ltmTool, SIGNAL(useCustomRange(DateRange)), this, SLOT(useCustomRange(DateRange)));
    connect(ltmTool, SIGNAL(useThruToday()), this, SLOT(useThruToday()));
    connect(ltmTool, SIGNAL(useStandardRange()), this, SLOT(useStandardRange()));
    connect(ltmTool, SIGNAL(curvesChanged()), this, SLOT(refresh()));
    connect(context, SIGNAL(filterChanged()), this, SLOT(refresh()));

    // connect pickers to ltmPlot
    connect(_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), ltmPlot, SLOT(pointHover(QwtPlotCurve*, int)));
    connect(_canvasPicker, SIGNAL(pointClicked(QwtPlotCurve*, int)), ltmPlot, SLOT(pointClicked(QwtPlotCurve*, int)));

    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh(void)));
    connect(context, SIGNAL(configChanged()), this, SLOT(refresh()));
}

LTMWindow::~LTMWindow()
{
    delete popup;
}

void
LTMWindow::rideSelected() { } // deprecated

void
LTMWindow::refreshPlot()
{
    if (amVisible() == true) {
        plotted = DateRange(settings.start.date(), settings.end.date());
        ltmPlot->setData(&settings);
        dirty = false;
    }
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
        dirty = false;

    } else {
        dirty = true;
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

        // apply filter to new date range too -- will also refresh plot
        filterChanged();
    }
}

void
LTMWindow::filterChanged()
{
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
LTMWindow::shadeZonesClicked(int state)
{
    bool checked = state;

    // only change if changed, to avoid endless looping
    if (ltmTool->shadeZones->isChecked() != checked) ltmTool->shadeZones->setChecked(checked);
    if (rShade->isChecked() != checked) rShade->setChecked(checked);
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
    bool checked = state;

    // only change if changed, to avoid endless looping
    if (ltmTool->showEvents->isChecked() != checked) ltmTool->showEvents->setChecked(checked);
    if (rEvents->isChecked() != checked) rEvents->setChecked(checked);
    settings.events = state;
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
        bool shadeZones = settings.shadeZones;
        QDateTime start = settings.start;
        QDateTime end = settings.end;

        // apply preset
        settings = ltmTool->presets[selected];

        // now get back the local chart setup
        settings.ltmTool = ltmTool;
        settings.data = &results;
        settings.measures = &measures;
        settings.groupBy = groupBy;
        settings.legend = legend;
        settings.events = events;
        settings.shadeZones = shadeZones;
        settings.start = start;
        settings.end = end;

        ltmTool->applySettings();
        refresh();
    }
}

void
LTMWindow::saveClicked()
{
    EditChartDialog editor(context, &settings, ltmTool->presets);
    if (editor.exec()) {
        ltmTool->presets.append(settings);
        settings.writeChartXML(context->athlete->home, ltmTool->presets);
        //ltmTool->presetPicker->insertItem(ltmTool->presets.count()-1, settings.name, ltmTool->presets.count()-1);
        //ltmTool->presetPicker->setCurrentIndex(ltmTool->presets.count()-1);
    }
}

int
LTMWindow::groupForDate(QDate date, int groupby)
{
    switch(groupby) {
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
    // get the date range for this point
    QDate start, end;
    LTMScaleDraw *lsd = new LTMScaleDraw(settings.start,
                        groupForDate(settings.start.date(), settings.groupBy),
                        settings.groupBy);
    lsd->dateRange((int)round(curve->sample(index).x()), start, end);
    ltmPopup->setData(settings, start, end);
    popup->show();
}
