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
            useMetricUnits(useMetricUnits), active(false), dirty(true)
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
    QFormLayout *cl = new QFormLayout(c);
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
    ltmZoomer->setSelectionFlags(QwtPicker::DragSelection
                                 | QwtPicker::CornerToCorner);
    ltmZoomer->setTrackerMode(QwtPicker::AlwaysOff);
    ltmZoomer->setEnabled(false);
    ltmZoomer->setMousePattern(QwtEventPattern::MouseSelect2,
                               Qt::RightButton, Qt::ControlModifier);
    ltmZoomer->setMousePattern(QwtEventPattern::MouseSelect3,
                               Qt::RightButton);

    picker = new LTMToolTip(QwtPlot::xBottom, QwtPlot::yLeft,
                               QwtPicker::PointSelection,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOn,
                               ltmPlot->canvas(),
                               "");
    picker->setMousePattern(QwtEventPattern::MouseSelect1,
                            Qt::LeftButton, Qt::ShiftModifier);
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
    settings.legend = true;
    if (appsettings->value(this, GC_SHADEZONES, true).toBool()==true)
        settings.shadeZones = true;
    else
        settings.shadeZones = false;

    QLabel *presetLabel = new QLabel(tr("Chart"));
    presetPicker = new QComboBox;
    presetPicker->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    cl->addRow(presetLabel, presetPicker);

    // read charts.xml and populate the picker
    LTMSettings reader;
    reader.readChartXML(home, presets);
    for(int i=0; i<presets.count(); i++)
        presetPicker->addItem(presets[i].name, i);
    presetPicker->setCurrentIndex(-1);

    groupBy = new QComboBox;
    groupBy->addItem("Days", LTM_DAY);
    groupBy->addItem("Weeks", LTM_WEEK);
    groupBy->addItem("Months", LTM_MONTH);
    groupBy->addItem("Years", LTM_YEAR);
    groupBy->addItem("Time Of Day", LTM_TOD);
    groupBy->setCurrentIndex(0);
    cl->addRow(new QLabel("Group by"), groupBy);

    shadeZones = new QCheckBox("Shade Zones");
    shadeZones->setChecked(settings.shadeZones);
    cl->addRow(shadeZones);

    showLegend = new QCheckBox("Show Legend");
    showLegend->setChecked(settings.legend);
    cl->addRow(showLegend);

    // controls
    saveButton = new QPushButton(tr("Add"));
    cl->addRow(saveButton);
    manageButton = new QPushButton(tr("Manage"));
    cl->addRow(manageButton);
    cl->addRow(ltmTool);

    connect(ltmTool, SIGNAL(dateRangeSelected(const Season *)), this, SLOT(dateRangeSelected(const Season *)));
    connect(ltmTool, SIGNAL(metricSelected()), this, SLOT(metricSelected()));
    connect(groupBy, SIGNAL(currentIndexChanged(int)), this, SLOT(groupBySelected(int)));
    connect(saveButton, SIGNAL(clicked(bool)), this, SLOT(saveClicked(void)));
    connect(manageButton, SIGNAL(clicked(bool)), this, SLOT(manageClicked(void)));
    connect(presetPicker, SIGNAL(currentIndexChanged(int)), this, SLOT(chartSelected(int)));
    connect(shadeZones, SIGNAL(stateChanged(int)), this, SLOT(shadeZonesClicked(int)));
    connect(showLegend, SIGNAL(stateChanged(int)), this, SLOT(showLegendClicked(int)));

    // connect pickers to ltmPlot
    connect(_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), ltmPlot, SLOT(pointHover(QwtPlotCurve*, int)));
    connect(_canvasPicker, SIGNAL(pointClicked(QwtPlotCurve*, int)), ltmPlot, SLOT(pointClicked(QwtPlotCurve*, int)));
    connect(picker, SIGNAL(moved(QPoint)), ltmPlot, SLOT(pickerMoved(QPoint)));
    connect(picker, SIGNAL(appended(const QPoint &)), ltmPlot, SLOT(pickerAppended(const QPoint &)));

    // config changes or ride file activities cause a redraw/refresh (but only if active)
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

QString
LTMWindow::dateRange() const
{
    return ltmTool->_dateRange();
}

void
LTMWindow::setDateRange(QString s)
{
    ltmTool->setDateRange(s);
}

void
LTMWindow::rideSelected()
{
    active = amVisible();

#if 0
    if (active == true) {

        // mimic user first selection now that
        // we are active - choose a chart and
        // use the first available date range
        ltmTool->selectDateRange(0);
        chartSelected(0);
#endif
    if (active == true && dirty == true) {

        // plot needs to be redrawn
        refresh();
    } else if (active == false) {
        popup->hide();
    }
}

void
LTMWindow::refreshPlot()
{
    if (active == true) ltmPlot->setData(&settings);
}

// total redraw, reread data etc
void
LTMWindow::refresh()
{
    // refresh for changes to ridefiles / zones
    if (active == true && main->metricDB != NULL) {
        // if config has changed get new useMetricUnits
        useMetricUnits = appsettings->value(this, GC_UNIT).toString() == "Metric";

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
LTMWindow::dateRangeSelected(const Season *selected)
{
    if (selected) {
        Season dateRange = *selected;

        settings.start = QDateTime(dateRange.getStart(), QTime(0,0));
        settings.end   = QDateTime(dateRange.getEnd(), QTime(24,0,0));
        settings.title = dateRange.getName();
        settings.data = &results;
        settings.measures = &measures;

        // if we want weeks and start is not a monday go back to the monday
        int dow = dateRange.getStart().dayOfWeek();
        if (settings.groupBy == LTM_WEEK && dow >1 && dateRange.getStart() != QDate())
            settings.start = settings.start.addDays(-1*(dow-1));

        // get the data
        results.clear(); // clear any old data
        results = main->metricDB->getAllMetricsFor(settings.start, settings.end);
        measures.clear(); // clear any old data
        measures = main->metricDB->getAllMeasuresFor(settings.start, settings.end);
        refreshPlot();
    }
}

void
LTMWindow::groupBySelected(int selected)
{
    if (selected >= 0) {
        settings.groupBy = groupBy->itemData(selected).toInt();
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
        int chartid = presetPicker->itemData(selected).toInt();
        ltmTool->applySettings(&presets[chartid]);
    }
}

void
LTMWindow::saveClicked()
{
    EditChartDialog editor(main, &settings, presets);
    if (editor.exec()) {
        presets.append(settings);
        settings.writeChartXML(main->home, presets);
        presetPicker->insertItem(presets.count()-1, settings.name, presets.count()-1);
        presetPicker->setCurrentIndex(presets.count()-1);
    }
}

void
LTMWindow::manageClicked()
{
    QList<LTMSettings> charts = presets; // get current
    ChartManagerDialog editor(main, &charts);
    if (editor.exec()) {
        // wipe the current and add the new
        presets = charts;
        presetPicker->clear();
        // update the presets to reflect the change
        for(int i=0; i<presets.count(); i++)
            presetPicker->addItem(presets[i].name, i);

        // update charts.xml
        settings.writeChartXML(main->home, presets);
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
    lsd->dateRange((int)round(curve->x(index)), start, end);
    ltmPopup->setData(settings, start, end);
    popup->show();
}
