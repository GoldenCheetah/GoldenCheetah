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
#include "MainWindow.h"
#include "SummaryMetrics.h"
#include "Settings.h"
#include "math.h"
#include "Units.h" // for MILES_PER_KM

#include <QtGui>
#include <QString>

#include <qwt_plot_panner.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_marker.h>

LTMWindow::LTMWindow(MainWindow *parent, bool useMetricUnits, const QDir &home) :
            LTMPlotContainer(parent), home(home),
            useMetricUnits(useMetricUnits), dirty(true)
{
    main = parent;
    setInstanceName("Metric Window");

    // the plot
    QVBoxLayout *mainLayout = new QVBoxLayout;
    ltmPlot = new LTMPlot(this, main, home);
    mainLayout->addWidget(ltmPlot);
    setLayout(mainLayout);

    // the controls
    QWidget *c = new QWidget;
    c->setContentsMargins(0,0,0,0);
    QVBoxLayout *cl = new QVBoxLayout(c);
    cl->setContentsMargins(0,0,0,0);
    cl->setSpacing(0);
    setControls(c);

    // the popup
    popup = new GcPane();
    ltmPopup = new LTMPopup(main);
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

    ltmTool = new LTMTool(parent, home);

    // initialise
    settings.ltmTool = ltmTool;
    settings.data = NULL;
    settings.groupBy = LTM_DAY;
    settings.legend = ltmTool->showLegend->isChecked();
    settings.shadeZones = ltmTool->shadeZones->isChecked();
    cl->addWidget(ltmTool);

    connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    connect(ltmTool, SIGNAL(filterChanged()), this, SLOT(filterChanged()));
    connect(ltmTool, SIGNAL(metricSelected()), this, SLOT(metricSelected()));
    connect(ltmTool->groupBy, SIGNAL(currentIndexChanged(int)), this, SLOT(groupBySelected(int)));
    connect(ltmTool->saveButton, SIGNAL(clicked(bool)), this, SLOT(saveClicked(void)));
    connect(ltmTool->manageButton, SIGNAL(clicked(bool)), this, SLOT(manageClicked(void)));
    connect(ltmTool->presetPicker, SIGNAL(currentIndexChanged(int)), this, SLOT(chartSelected(int)));
    connect(ltmTool->shadeZones, SIGNAL(stateChanged(int)), this, SLOT(shadeZonesClicked(int)));
    connect(ltmTool->showLegend, SIGNAL(stateChanged(int)), this, SLOT(showLegendClicked(int)));

    // connect pickers to ltmPlot
    connect(_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), ltmPlot, SLOT(pointHover(QwtPlotCurve*, int)));
    connect(_canvasPicker, SIGNAL(pointClicked(QwtPlotCurve*, int)), ltmPlot, SLOT(pointClicked(QwtPlotCurve*, int)));
    connect(picker, SIGNAL(moved(QPoint)), ltmPlot, SLOT(pickerMoved(QPoint)));
    connect(picker, SIGNAL(appended(const QPoint &)), ltmPlot, SLOT(pickerAppended(const QPoint &)));

    // config changes or ride file activities cause a redraw/refresh (but only if visible)
    //connect(main, SIGNAL(rideSelected()), this, SLOT(rideSelected(void)));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(main, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh(void)));
    connect(main, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh(void)));
    connect(main, SIGNAL(configChanged()), this, SLOT(refresh()));
}

LTMWindow::~LTMWindow()
{
    //qDebug()<<"delete metricdb... crash!!!";
    //if (metricDB != NULL) delete metricDB; //XXX CRASH!!!! -- needs fixing
    delete popup;
}

void
LTMWindow::rideSelected()
{
#if 0
    if (active == true) {

        // mimic user first selection now that
        // we are active - choose a chart and
        // use the first available date range
        ltmTool->selectDateRange(0);
        chartSelected(0);
#endif
    if (amVisible() == true && dirty == true) {

        // plot needs to be redrawn
        refresh();
    } else if (amVisible() == false) {
        popup->hide();
    }
}

void
LTMWindow::refreshPlot()
{
    if (amVisible() == true) {
        ltmPlot->setData(&settings);
        dirty = false;
    }
}

// total redraw, reread data etc
void
LTMWindow::refresh()
{

    // refresh for changes to ridefiles / zones
    if (amVisible() == true && main->metricDB != NULL) {
        // if config has changed get new useMetricUnits
        useMetricUnits = main->useMetricUnits;
        results.clear(); // clear any old data
        results = main->metricDB->getAllMetricsFor(settings.start, settings.end);
        measures.clear(); // clear any old data
        measures = main->metricDB->getAllMeasuresFor(settings.start, settings.end);
        refreshPlot();
        dirty = false;
    } else {
        dirty = true;
    }
}

void
LTMWindow::metricSelected()
{
    // wipe existing settings
    settings.metrics.clear();

    foreach(QTreeWidgetItem *metric, ltmTool->selectedMetrics()) {
        if (metric->type() != ROOT_TYPE) {
            QString symbol = ltmTool->metricSymbol(metric);
            settings.metrics.append(ltmTool->metricDetails(metric));
        }
    }
    refreshPlot();
}

void
LTMWindow::dateRangeChanged(DateRange range)
{ Q_UNUSED( range )
    if (!amVisible() && !dirty) return;

    settings.data = &results;
    settings.measures = &measures;

    // apply filter to new date range too
    filterChanged();

    refreshPlot();
}

void
LTMWindow::filterChanged()
{
    settings.start = QDateTime(myDateRange.from, QTime(0,0));
    settings.end   = QDateTime(myDateRange.to, QTime(24,0,0));
    settings.title = myDateRange.name;
    settings.data = &results;

    // if we want weeks and start is not a monday go back to the monday
    int dow = myDateRange.from.dayOfWeek();
    if (settings.groupBy == LTM_WEEK && dow >1 && myDateRange.from != QDate())
        settings.start = settings.start.addDays(-1*(dow-1));

    // we need to get data again and apply filter
    results.clear(); // clear any old data
    results = main->metricDB->getAllMetricsFor(settings.start, settings.end);

    // loop through results removing any not in stringlist..
    if (ltmTool->isFiltered()) {

        QList<SummaryMetrics> filteredresults;
        foreach (SummaryMetrics x, results) {
            if (ltmTool->filters().contains(x.getFileName()))
                filteredresults << x;
        }
        results = filteredresults;
        settings.data = &results;
    }

    refreshPlot();
}

void
LTMWindow::groupBySelected(int selected)
{
    if (selected >= 0) {
        settings.groupBy = ltmTool->groupBy->itemData(selected).toInt();
        refreshPlot();
    }
}

void
LTMWindow::shadeZonesClicked(int state)
{
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
LTMWindow::chartSelected(int selected)
{
    if (selected >= 0) {
        // what is the index of the chart?
        int chartid = ltmTool->presetPicker->itemData(selected).toInt();
        ltmTool->applySettings(&ltmTool->presets[chartid]);
    }
}

void
LTMWindow::saveClicked()
{
    EditChartDialog editor(main, &settings, ltmTool->presets);
    if (editor.exec()) {
        ltmTool->presets.append(settings);
        settings.writeChartXML(main->home, ltmTool->presets);
        ltmTool->presetPicker->insertItem(ltmTool->presets.count()-1, settings.name, ltmTool->presets.count()-1);
        ltmTool->presetPicker->setCurrentIndex(ltmTool->presets.count()-1);
    }
}

void
LTMWindow::manageClicked()
{
    QList<LTMSettings> charts = ltmTool->presets; // get current
    ChartManagerDialog editor(main, &charts);
    if (editor.exec()) {
        // wipe the current and add the new
        ltmTool->presets = charts;
        ltmTool->presetPicker->clear();
        // update the presets to reflect the change
        for(int i=0; i<ltmTool->presets.count(); i++)
            ltmTool->presetPicker->addItem(ltmTool->presets[i].name, i);

        // update charts.xml
        settings.writeChartXML(main->home, ltmTool->presets);
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
