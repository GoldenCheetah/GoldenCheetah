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

#include "TreeMapWindow.h"
#include "LTMTool.h"
#include "TreeMapPlot.h"
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

TreeMapWindow::TreeMapWindow(MainWindow *parent, bool useMetricUnits, const QDir &home) :
            GcWindow(parent), main(parent), home(home),
            useMetricUnits(useMetricUnits), active(false), dirty(true)
{
    setInstanceName("Treemap Window");

    // the plot
    mainLayout = new QVBoxLayout;
    ltmPlot = new TreeMapPlot(this, main, home);
    mainLayout->addWidget(ltmPlot);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    setLayout(mainLayout);

    // the controls
    QWidget *c = new QWidget;
    QFormLayout *cl = new QFormLayout(c);
    setControls(c);

    // read metadata.xml
    QString filename = main->home.absolutePath()+"/metadata.xml";
    if (!QFile(filename).exists()) filename = ":/xml/metadata.xml";
    RideMetadata::readXML(filename, keywordDefinitions, fieldDefinitions);

    //title = new QLabel(this);
    //QFont font;
    //font.setPointSize(14);
    //font.setWeight(QFont::Bold);
    //title->setFont(font);
    //title->setFixedHeight(20);
    //title->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    // widgets

    // setup the popup widget
    popup = new GcPane();
    ltmPopup = new LTMPopup(main);
    QVBoxLayout *popupLayout = new QVBoxLayout();
    popupLayout->addWidget(ltmPopup);
    popup->setLayout(popupLayout);

    ltmTool = new LTMTool(parent, home, false); // false for single selection
    settings.ltmTool = ltmTool;

    // initialise
    settings.data = NULL;
    settings.groupBy = LTM_DAY;
    if (appsettings->value(this, GC_SHADEZONES, true).toBool()==true)
        settings.shadeZones = true;
    else
        settings.shadeZones = false;

    // controls
    field1 = new QComboBox(this);
    addTextFields(field1);
    field2 = new QComboBox(this);
    addTextFields(field2);

    field1->setCurrentIndex(field1->findText(appsettings->value(this, GC_TM_FIRST, "None").toString()));
    field2->setCurrentIndex(field2->findText(appsettings->value(this, GC_TM_SECOND, "None").toString()));
    settings.field1 = field1->currentText();
    settings.field2 = field2->currentText();

    cl->addRow(new QLabel("First"), field1);
    cl->addRow(new QLabel("Second"), field2);
    cl->addRow(ltmTool);

    connect(ltmTool, SIGNAL(dateRangeSelected(const Season *)), this, SLOT(dateRangeSelected(const Season *)));
    connect(ltmTool, SIGNAL(metricSelected()), this, SLOT(metricSelected()));
    connect(field1, SIGNAL(currentIndexChanged(int)), this, SLOT(fieldSelected(int)));
    connect(field2, SIGNAL(currentIndexChanged(int)), this, SLOT(fieldSelected(int)));

    // config changes or ride file activities cause a redraw/refresh (but only if active)
    //connect(main, SIGNAL(rideSelected()), this, SLOT(rideSelected(void)));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(main, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh(void)));
    connect(main, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh(void)));
    connect(main, SIGNAL(configChanged()), this, SLOT(refresh()));
}

TreeMapWindow::~TreeMapWindow()
{
    delete popup;
}

QString
TreeMapWindow::dateRange() const
{
    return ltmTool->_dateRange();
}

void
TreeMapWindow::setDateRange(QString s)
{
    ltmTool->setDateRange(s);
}

void
TreeMapWindow::rideSelected()
{
    active = amVisible();

    if (active == true) {
        // mimic user first selection now that
        // we are active - choose a chart and
        // use the first available date range
        ltmTool->selectDateRange(0);

        // default to duration
        ltmTool->selectMetric(appsettings->value(this, GC_TM_METRIC, "workout_time").toString());

    } else if (active == true && dirty == true) {

        // plot needs to be redrawn
        refresh();
    } else if (active == false) {
        popup->hide();
    }
}

void
TreeMapWindow::refreshPlot()
{
    if (active == true) ltmPlot->setData(&settings);
}

// total redraw, reread data etc
void
TreeMapWindow::refresh()
{
    // refresh for changes to ridefiles / zones
    if (active == true) {
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
TreeMapWindow::metricSelected()
{
    // wipe existing settings
    settings.metrics.clear();

    foreach(QTreeWidgetItem *metric, ltmTool->selectedMetrics()) {
        if (metric->type() != ROOT_TYPE) {
            QString symbol = ltmTool->metricSymbol(metric);
            settings.metrics.append(ltmTool->metricDetails(metric));
            appsettings->setValue(GC_TM_METRIC, settings.metrics[0].symbol);
        }
    }
    ltmPlot->setData(&settings);
}

void
TreeMapWindow::dateRangeSelected(const Season *selected)
{
    if (selected) {
        Season dateRange = *selected;

        settings.start = QDateTime(dateRange.getStart(), QTime(0,0));
        settings.end   = QDateTime(dateRange.getEnd(), QTime(24,0,0));
        settings.title = dateRange.getName();
        //title->setText(settings.title);
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

        ltmPlot->setData(&settings);
    }
}

void
TreeMapWindow::fieldSelected(int)
{
    settings.field1 = field1->currentText();
    settings.field2 = field2->currentText();
    ltmPlot->setData(&settings);
    appsettings->setValue(GC_TM_FIRST, field1->currentText());
    appsettings->setValue(GC_TM_SECOND, field2->currentText());
}

int
TreeMapWindow::groupForDate(QDate date, int groupby)
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
TreeMapWindow::pointClicked(QwtPlotCurve*curve, int index)
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

void
TreeMapWindow::addTextFields(QComboBox *combo)
{
    combo->addItem("None");
    foreach (FieldDefinition x, fieldDefinitions) {
        if (x.type < 4) combo->addItem(x.name);
    }
}
