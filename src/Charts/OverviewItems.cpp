/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#include "OverviewItems.h"

#include "AbstractView.h"
#include "Athlete.h"
#include "RideCache.h"
#include "IntervalItem.h"

#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"

#include "PMCData.h"
#include "RideMetadata.h"

#include "DataFilter.h"
#include "Utils.h"
#include "TimeUtils.h"
#include "AthleteTab.h"
#include "LTMTool.h"
#include "RideNavigator.h"

#include "UserChartOverviewItem.h"

#include <cmath>
#include <QGraphicsSceneMouseEvent>
#include <QGLWidget>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

bool
OverviewItemConfig::registerItems()
{
    // get the factory
    ChartSpaceItemRegistry &registry = ChartSpaceItemRegistry::instance();

    // Register      TYPE                          SHORT                      DESCRIPTION                                        SCOPE            CREATOR
    registry.addItem(OverviewItemType::USERCHART,  QObject::tr("User Chart"), QObject::tr("User defined interactive chart"),     OverviewScope::ANALYSIS|OverviewScope::TRENDS, UserChartOverviewItem::create);
    registry.addItem(OverviewItemType::METRIC,     QObject::tr("Metric"),     QObject::tr("Metric and Sparkline"),               OverviewScope::ANALYSIS|OverviewScope::TRENDS, MetricOverviewItem::create);
    registry.addItem(OverviewItemType::KPI,        QObject::tr("KPI"),        QObject::tr("KPI calculation and progress bar"),   OverviewScope::ANALYSIS|OverviewScope::TRENDS, KPIOverviewItem::create);
    registry.addItem(OverviewItemType::DATATABLE,  QObject::tr("Table"),      QObject::tr("Table of data"),                      OverviewScope::ANALYSIS|OverviewScope::TRENDS, DataOverviewItem::create);
    registry.addItem(OverviewItemType::TOPN,       QObject::tr("Bests"),      QObject::tr("Ranked list of bests"),               OverviewScope::TRENDS,                         TopNOverviewItem::create);
    registry.addItem(OverviewItemType::META,       QObject::tr("Metadata"),   QObject::tr("Metadata and Sparkline"),             OverviewScope::ANALYSIS,                       MetaOverviewItem::create);
    registry.addItem(OverviewItemType::ZONE,       QObject::tr("Zones"),      QObject::tr("Zone Histogram"),                     OverviewScope::ANALYSIS|OverviewScope::TRENDS, ZoneOverviewItem::create);
    registry.addItem(OverviewItemType::RPE,        QObject::tr("RPE"),        QObject::tr("RPE Widget"),                         OverviewScope::ANALYSIS,                       RPEOverviewItem::create);
    registry.addItem(OverviewItemType::INTERVAL,   QObject::tr("Intervals"),  QObject::tr("Interval Bubble Chart"),              OverviewScope::ANALYSIS,                       IntervalOverviewItem::createInterval);
    registry.addItem(OverviewItemType::ACTIVITIES, QObject::tr("Activities"), QObject::tr("Activities Bubble Chart"),            OverviewScope::TRENDS,                         IntervalOverviewItem::createActivities);
    registry.addItem(OverviewItemType::PMC,        QObject::tr("PMC"),        QObject::tr("PMC Status Summary"),                 OverviewScope::ANALYSIS,                       PMCOverviewItem::create);
    registry.addItem(OverviewItemType::ROUTE,      QObject::tr("Route"),      QObject::tr("Route Summary"),                      OverviewScope::ANALYSIS,                       RouteOverviewItem::create);
    registry.addItem(OverviewItemType::DONUT,      QObject::tr("Donut"),      QObject::tr("Metric breakdown by category"),       OverviewScope::TRENDS,                         DonutOverviewItem::create);

    return true;
}

static void setFilter(ChartSpaceItem *item, Specification &spec)
{
    // trends view filter
    if (item->parent->scope & OverviewScope::TRENDS) {

        // general filters
        FilterSet fs;
        fs.addFilter(item->parent->context->isfiltered, item->parent->context->filters);
        fs.addFilter(item->parent->context->ishomefiltered, item->parent->context->homeFilters);

        // property gets set after chartspace is initialised, so when we start up its not
        // available, but comes later...
        if (item->parent->window->myPerspective != NULL)
            fs.addFilter(item->parent->window->myPerspective->isFiltered(), item->parent->window->myPerspective->filterlist(item->parent->myDateRange));

        // local filter
        fs.addFilter(item->datafilter != "", SearchFilterBox::matches(item->parent->context, item->datafilter));
        spec.setFilterSet(fs);
    }
    return;
}

RPEOverviewItem::RPEOverviewItem(ChartSpace *parent, QString name) : ChartSpaceItem(parent, name)
{
    // a META widget, "RPE" using the FOSTER modified 0-10 scale
    this->type = OverviewItemType::RPE;

    sparkline = new Sparkline(this, name);
    rperating = new RPErating(this, name);

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();

}

RPEOverviewItem::~RPEOverviewItem()
{
    delete rperating;
    delete sparkline;
}

KPIOverviewItem::KPIOverviewItem(ChartSpace *parent, QString name, double start, double stop, QString program, QString units, bool istime) : ChartSpaceItem(parent, name)
{
    this->type = OverviewItemType::KPI;
    this->start = start;
    this->stop = stop;
    this->program = program;
    this->units = units;
    this->istime = istime;

    value ="";
    progressbar = new ProgressBar(this, start, stop, value.toDouble());

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();
}

KPIOverviewItem::~KPIOverviewItem()
{
    // delete progress bar; //XXX todo
    delete progressbar;
}

DataOverviewItem::DataOverviewItem(ChartSpace *parent, QString name, QString program) : ChartSpaceItem(parent, name)
{
    this->type = OverviewItemType::DATATABLE;
    this->program = program;

    click = false;
    lastsort = sortcolumn = -1;
    lastorder = Qt::AscendingOrder;
    clickthru = NULL;

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();

    scrollbar = new VScrollBar(this, parent);

    // refresh when interval events happen (only on analysis view)
    if (parent->scope == OverviewScope::ANALYSIS) {
        //connect(parent->context, SIGNAL(intervalSelected()), this, SLOT(intervalSelectRefresh()));
        //connect(parent->context, SIGNAL(intervalsChanged()), this, SLOT(intervalSelectRefresh()));
        //connect(parent->context, SIGNAL(intervalsUpdate(RideItem*)), this, SLOT(intervalSelectRefresh()));
        connect(parent->context, SIGNAL(intervalHover(IntervalItem*)), this, SLOT(intervalHover(IntervalItem*)));
        //connect(parent->context, SIGNAL(intervalZoom(IntervalItem*)), this, SLOT(intervalSelectRefresh()));
        //connect(parent->context, SIGNAL(intervalItemSelectionChanged(IntervalItem*)), this, SLOT(intervalSelectRefresh()));
    }
}

DataOverviewItem::~DataOverviewItem()
{
}

#define DATA_TABLE_TOTALS      1
#define DATA_TABLE_AVERAGES    2
#define DATA_TABLE_MAXIMUMS    3
#define DATA_TABLE_METRICS     4
#define DATA_TABLE_ZONES       5
#define DATA_TABLE_INTERVALS   6
#define DATA_TABLE_TRENDS      9

// this is the old metric configuration, now deprecated, we will look it up
// when setting the legacy metrics, as a convenience for users
#define GC_SETTINGS_SUMMARY_METRICS     "<global-general>rideSummaryWindow/summaryMetrics"

QString DataOverviewItem::getLegacyProgram(int type, DataFilterRuntime &rt, bool trends)
{
    QString program;

    // we need to aggregate on trends view
    QString prefix = trends ? "agg" : "";
    bool replace=false;

    switch(type) {

    case DATA_TABLE_TRENDS:
            program =
            "{\n"
            "\n"
            "# column names, if using metrics then best\n"
            "# to use metricname() to get correct name for locale\n"
            "# otherwise it won't translate to other languages\n"
            "names {\n"
            "    metricname(date,\n"
            "         Duration,\n"
            "         Time_Moving,\n"
            "         Distance,\n"
            "         Work,\n"
            "         W'_Work,\n"
            "         30_min_Peak_Power);\n"
            "}\n"
            "\n"
            "# column units, if using metrics then best\n"
            "# to use metricunit() function to get correct string\n"
            "# for locale and metric/imperial\n"
            "units {\n"
            "    metricunit(date,\n"
            "         Duration,\n"
            "         Time_Moving,\n"
            "         Distance,\n"
            "         Work,\n"
            "         W'_Work,\n"
            "         30_min_Peak_Power);\n"
            "}\n"
            "\n"
            "# values to display as doubles or strings\n"
            "# if using metrics always best to use asstring()\n"
            "# to convert correctly with dp, metric/imperial\n"
            "# or specific formats eg. rowing pace xx/500m\n"
            "values { \n"
            "    c(metricstrings(date),\n"
            "      metricstrings(Duration),\n"
            "      metricstrings(Time_Moving),\n"
            "      metricstrings(Distance),\n"
            "      metricstrings(Work),\n"
            "      metricstrings(W'_Work),\n"
            "      metricstrings(30_min_Peak_Power)); \n"
            "} \n"
            "\n"
            "# heat values for coloring the cell\n"
            "# must be between 0 and 1 where the\n"
            "heat { \n"
            "    c(normalize(0,0,metrics(date)),\n"
            "      normalize(0,0,metrics(Duration)),\n"
            "      normalize(0,0,metrics(Time_Moving)),\n"
            "      normalize(0,0,metrics(Distance)),\n"
            "      normalize(0,0,metrics(Work)),\n"
            "      normalize(0,0,metrics(W'_Work)),\n"
            "      normalize(0,300,metrics(30_min_Peak_Power))); \n"
            "} \n"
            "\n"
            "# Click thru for the row, we can set the file\n"
            "# this row represents. In the same way as a user chart.\n"
            "f { \n"
            "    filename();\n"
            "}\n"
            "\n"
            "}";
    break;

    case DATA_TABLE_AVERAGES:
    program =
            "{\n"
            "\n"
            "# column names, if using metrics then best\n"
            "# to use metricname() to get correct name for locale\n"
            "# otherwise it won't translate to other languages\n"
            "names { \n"
            "    metricname(Athlete_Weight,\n"
            "         Average_Speed,\n"
            "         Average_Power,\n"
            "         Average_Heart_Rate,\n"
            "         Average_Cadence);\n"
            "}\n"
            "\n"
            "# column units, if using metrics then best\n"
            "# to use metricunit() function to get correct string\n"
            "# for locale and metric/imperial\n"
            "units { \n"
            "    metricunit(Athlete_Weight,\n"
            "         Average_Speed,\n"
            "         Average_Power,\n"
            "         Average_Heart_Rate,\n"
            "         Average_Cadence);\n"
            "}\n"
            "\n"
            "# values to display as doubles or strings\n"
            "# if using metrics always best to use asstring()\n"
            "# to convert correctly with dp, metric/imperial\n"
            "# or specific formats eg. rowing pace xx/500m\n"
            "values { \n"
            "    as%1string(Athlete_Weight,\n"
            "         Average_Speed,\n"
            "         Average_Power,\n"
            "         Average_Heart_Rate,\n"
            "         Average_Cadence); \n"
            "} \n"
            "\n"
            "}\n";
    replace = true;
    break;

    case DATA_TABLE_MAXIMUMS:
    program =
            "{\n"
            "\n"
            "# column names, if using metrics then best\n"
            "# to use metricname() to get correct name for locale\n"
            "# otherwise it won't translate to other languages\n"
            "names { \n"
            "    metricname(Max_Speed,\n"
            "         Max_Power,\n"
            "         Max_Heartrate,\n"
            "         Max_Cadence,\n"
            "         Max_W'_Expended);\n"
            "}\n"
            "\n"
            "# column units, if using metrics then best\n"
            "# to use metricunit() function to get correct string\n"
            "# for locale and metric/imperial\n"
            "units { \n"
            "    metricunit(Max_Speed,\n"
            "         Max_Power,\n"
            "         Max_Heartrate,\n"
            "         Max_Cadence,\n"
            "         Max_W'_Expended);\n"
            "}\n"
            "\n"
            "# values to display as doubles or strings\n"
            "# if using metrics always best to use asstring()\n"
            "# to convert correctly with dp, metric/imperial\n"
            "# or specific formats eg. rowing pace xx/500m\n"
            "values { \n"
            "    as%1string(Max_Speed,\n"
            "         Max_Power,\n"
            "         Max_Heartrate,\n"
            "         Max_Cadence,\n"
            "         Max_W'_Expended); \n"
            "} \n"
            "\n"
            "}\n";
        replace=true;
    break;

    case DATA_TABLE_METRICS:
    {
        // get a string list of the metrics to show
        QString s;
        if (appsettings->contains(GC_SETTINGS_SUMMARY_METRICS)) s = appsettings->value(NULL, GC_SETTINGS_SUMMARY_METRICS).toString();
        else s = GC_SETTINGS_FAVOURITE_METRICS_DEFAULT;
        QStringList symbols = s.split(",");

        // convert metric symbols to names we can use in the program
        QStringList list;
        QMapIterator<QString, QString> i(rt.lookupMap);
        while (i.hasNext()) {
            i.next();
            if (symbols.contains(i.value())) list << i.key();
        }

        // generate a program
        QString metrics = list.join(",\n              ");

        program =
            "{\n"
            "\n"
            "# column names, if using metrics then best\n"
            "# to use metricname() to get correct name for locale\n"
            "# otherwise it won't translate to other languages\n"
            "names { \n"
            "    metricname(" + metrics + ");\n"
            "}\n"
            "\n"
            "# column units, if using metrics then best\n"
            "# to use metricunit() function to get correct string\n"
            "# for locale and metric/imperial\n"
            "units { \n"
            "    metricunit(" + metrics +  ");\n"
            "}\n"
            "\n"
            "# values to display as doubles or strings\n"
            "# if using metrics always best to use asstring()\n"
            "# to convert correctly with dp, metric/imperial\n"
            "# or specific formats eg. rowing pace xx/500m\n"
            "values { \n"
            "    as%1string(" + metrics + ");\n"
            "} \n"
            "\n"
            "}\n";
            replace = true;
    }
    break;

    case DATA_TABLE_INTERVALS:
    {
        // get a string list of the metrics to show
        QString s;
        if (appsettings->contains(GC_SETTINGS_FAVOURITE_METRICS)) s = appsettings->value(NULL, GC_SETTINGS_FAVOURITE_METRICS).toString();
        else s = GC_SETTINGS_FAVOURITE_METRICS_DEFAULT;
        QStringList symbols = s.split(",");

        // convert metric symbols to names we can use in the program
        QStringList list;
        list << "name";
        QMapIterator<QString, QString> i(rt.lookupMap);
        while (i.hasNext()) {
            i.next();
            if (symbols.contains(i.value())) list << i.key();
        }

        // generate a program
        QString metrics = list.join(",\n              ");

        program =
            "{\n"
            "\n"
            "# column names, if using metrics then best\n"
            "# to use metricname() to get correct name for locale\n"
            "# otherwise it won't translate to other languages\n"
            "names { \n"
            "    metricname(" + metrics + ");\n"
            "}\n"
            "\n"
            "# column units, if using metrics then best\n"
            "# to use metricunit() function to get correct string\n"
            "# for locale and metric/imperial\n"
            "units { \n"
            "    metricunit(" + metrics +  ");\n"
            "}\n"
            "\n"
            "# values to display as doubles or strings\n"
            "# if using metrics always best to use asstring()\n"
            "# to convert correctly with dp, metric/imperial\n"
            "# or specific formats eg. rowing pace xx/500m\n"
            "values { \n"
            "    c(";

            bool first=true;
            foreach(QString metric, list) {

                // commas
                if (first) first = false;
                else program += ",\n      ";

                program += "intervalstrings(" + metric + ")";
            }

            program += ");\n}\n\n";

            program +=
            "# interval names indicate which interval\n"
            "# the row represents so we can select\n"
            "# and hover over them\n"
            "i {\n"
            "    intervalstrings(name);\n"
            "}\n\n";

            program +=
            "# heatmap values are from 0-1 so we use the\n"
            "# normalize() function to calculate it for the metrics\n"
            "# in question. When we use normalize(0,0,metric) we\n"
            "# will always get no heat, you can edit the min\n"
            "# max values to set the range to set the heat color\n"
            "heat { \n"
            "    c(";

            first=true;
            foreach(QString metric, list) {

                // commas
                if (first) first = false;
                else program += ",\n      ";

                program += "normalize(0, 0, intervals(" + metric + "))";
            }

            program += ");\n}\n\n}\n";
    }
    break;

    case DATA_TABLE_ZONES:
    program =
            "{\n"
            "\n"
            "# column names, if using metrics then best\n"
            "# to use metricname() to get correct name for locale\n"
            "# otherwise it won't translate to other languages\n"
            "names { \n"
            "    c(\"Name\",\"Description\",\"Low\",\"High\",\"Time\",\"%\");\n"
            "}\n"
            "\n"
            "# column units, if using metrics then best\n"
            "# to use metricunit() function to get correct string\n"
            "# for locale and metric/imperial\n"
            "units {\n"
            "\n"
            "    c(\"\",\n"
            "      \"\",\n"
            "      zones(power,units),\n"
            "      zones(power,units), \"\", \"\");\n"
            "}\n"
            "\n"
            "# values to display as doubles or strings\n"
            "# if using metrics always best to use asstring()\n"
            "# to convert correctly with dp, metric/imperial\n"
            "# or specific formats eg. rowing pace xx/500m\n"
            "values { \n"
            "\n"
            "   c( zones(power,name),\n"
            "      zones(power,description),\n"
            "      zones(power,low),\n"
            "      zones(power,high),\n"
            "      zones(power,time),\n"
            "      zones(power,percent));\n"
            "} \n"
            "\n"
            "}\n";
    break;

    default:
    case DATA_TABLE_TOTALS:
    program =
            "{\n"
            "\n"
            "# column names, if using metrics then best\n"
            "# to use metricname() to get correct name for locale\n"
            "# otherwise it won't translate to other languages\n"
            "names { \n"
            "    metricname(Duration,\n"
            "         Time_Moving,\n"
            "         Distance,\n"
            "         Work,\n"
            "         W'_Work,\n"
            "         Elevation_Gain);\n"
            "}\n"
            "\n"
            "# column units, if using metrics then best\n"
            "# to use metricunit() function to get correct string\n"
            "# for locale and metric/imperial\n"
            "units { \n"
            "    metricunit(Duration,\n"
            "         Time_Moving,\n"
            "         Distance,\n"
            "         Work,\n"
            "         W'_Work,\n"
            "         Elevation_Gain);\n"
            "}\n"
            "\n"
            "# values to display as doubles or strings\n"
            "# if using metrics always best to use asstring()\n"
            "# to convert correctly with dp, metric/imperial\n"
            "# or specific formats eg. rowing pace xx/500m\n"
            "values { \n"
            "    as%1string(Duration,\n"
            "             Time_Moving,\n"
            "             Distance,\n"
            "             Work,\n"
            "             W'_Work,\n"
            "             Elevation_Gain); \n"
            "} \n"
            "\n"
            "}\n";
            replace = true;
    break;

    }
    return replace ? QString(program).arg(prefix) : program;
}

ChartSpaceItem *
DataOverviewItem::create(ChartSpace *parent) {

    // temporary - bit expensive, but creation is expensive anyway
    DataFilter df(parent, parent->context);

    if (parent->scope == ANALYSIS) return new DataOverviewItem(parent, "Totals", getLegacyProgram(DATA_TABLE_TOTALS, df.rt, false));
    else return new DataOverviewItem(parent, "Activities", getLegacyProgram(DATA_TABLE_TRENDS, df.rt, true));
}

void
DataOverviewItem::dragChanged(bool x)
{
    if (x) scrollbar->hide();
    else scrollbar->show();
}

RouteOverviewItem::RouteOverviewItem(ChartSpace *parent, QString name) : ChartSpaceItem(parent, name)
{
    this->type = OverviewItemType::ROUTE;
    routeline = new Routeline(this, name);

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();
}

RouteOverviewItem::~RouteOverviewItem()
{
    delete routeline;
}

static const QStringList timeInZones = QStringList()
        << "time_in_zone_L1"
        << "time_in_zone_L2"
        << "time_in_zone_L3"
        << "time_in_zone_L4"
        << "time_in_zone_L5"
        << "time_in_zone_L6"
        << "time_in_zone_L7"
        << "time_in_zone_L8"
        << "time_in_zone_L9"
        << "time_in_zone_L10";

static const QStringList timeInZonesPolarized = QStringList()
        << "time_in_zone_LI"
        << "time_in_zone_LII"
        << "time_in_zone_LIII";

static const QStringList paceTimeInZones = QStringList()
        << "time_in_zone_P1"
        << "time_in_zone_P2"
        << "time_in_zone_P3"
        << "time_in_zone_P4"
        << "time_in_zone_P5"
        << "time_in_zone_P6"
        << "time_in_zone_P7"
        << "time_in_zone_P8"
        << "time_in_zone_P9"
        << "time_in_zone_P10";

static const QStringList paceTimeInZonesPolarized = QStringList()
        << "time_in_zone_PI"
        << "time_in_zone_PII"
        << "time_in_zone_PIII";

static const QStringList timeInZonesHR = QStringList()
        << "time_in_zone_H1"
        << "time_in_zone_H2"
        << "time_in_zone_H3"
        << "time_in_zone_H4"
        << "time_in_zone_H5"
        << "time_in_zone_H6"
        << "time_in_zone_H7"
        << "time_in_zone_H8"
        << "time_in_zone_H9"
        << "time_in_zone_H10";

static const QStringList timeInZonesHRPolarized = QStringList()
        << "time_in_zone_HI"
        << "time_in_zone_HII"
        << "time_in_zone_HIII";

static const QStringList timeInZonesWBAL = QStringList()
        << "wtime_in_zone_L1"
        << "wtime_in_zone_L2"
        << "wtime_in_zone_L3"
        << "wtime_in_zone_L4";

ZoneOverviewItem::ZoneOverviewItem(ChartSpace *parent, QString name, RideFile::seriestype series, bool polarized) : ChartSpaceItem(parent, name)
{
    this->type = OverviewItemType::ZONE;
    this->series = series;
    this->polarized = polarized;

    // basic chart setup
    chart = new QChart(this);
    chart->setBackgroundVisible(false); // draw on canvas
    chart->legend()->setVisible(false); // no legends
    chart->setTitle(""); // none wanted
    chart->setAnimationOptions(QChart::NoAnimation);

    // we have a mid sized font for chart labels etc
    chart->setFont(parent->midfont);

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();

    barset = NULL;
    barseries = NULL;
    barcategoryaxis = NULL;

    // setup
    configChanged(0);
}

void
ZoneOverviewItem::configChanged(qint32)
{
    if (barcategoryaxis) delete barcategoryaxis;
    if (barset) delete barset;
    if (barseries) delete barseries;

    // needs a set of bars
    barset = new QBarSet(tr("Time In Zone"), this);
    barset->setLabelFont(parent->midfont);

    // config changed...
    if (series == RideFile::hr) {
        barset->setLabelColor(GColor(CHEARTRATE));
        barset->setBorderColor(GColor(CHEARTRATE));
        barset->setBrush(GColor(CHEARTRATE));
    } else if (series == RideFile::watts) {
        barset->setLabelColor(GColor(CPOWER));
        barset->setBorderColor(GColor(CPOWER));
        barset->setBrush(GColor(CPOWER));
    } else if (series == RideFile::wbal) {
        barset->setLabelColor(GColor(CWBAL));
        barset->setBorderColor(GColor(CWBAL));
        barset->setBrush(GColor(CWBAL));
    } else if (series == RideFile::kph) {
        barset->setLabelColor(GColor(CSPEED));
        barset->setBorderColor(GColor(CSPEED));
        barset->setBrush(GColor(CSPEED));
    }

    categories.clear();

    //
    // HEARTRATE
    //
    if (!polarized && series == RideFile::hr && parent->context->athlete->hrZones("Bike")) {
        // set the zero values
        for(int i=0; i<parent->context->athlete->hrZones("Bike")->getScheme().nzones_default; i++) {
            *barset << 0;
            categories << parent->context->athlete->hrZones("Bike")->getScheme().zone_default_name[i];
        }
    }

    //
    // POWER
    //
    if (!polarized && series == RideFile::watts && parent->context->athlete->zones("Bike")) {
        // set the zero values
        for(int i=0; i<parent->context->athlete->zones("Bike")->getScheme().nzones_default; i++) {
            *barset << 0;
            categories << parent->context->athlete->zones("Bike")->getScheme().zone_default_name[i];
        }
    }

    //
    // PACE
    //
    if (!polarized && series == RideFile::kph && parent->context->athlete->paceZones(false)) {
        // set the zero values
        for(int i=0; i<parent->context->athlete->paceZones(false)->getScheme().nzones_default; i++) {
            *barset << 0;
            categories << parent->context->athlete->paceZones(false)->getScheme().zone_default_name[i];
        }
    }

    //
    // W'BAL and Polarized
    //
    if (series == RideFile::wbal) {
        categories << "Low" << "Med" << "High" << "Ext";
        *barset << 0 << 0 << 0 << 0;
    } else if (polarized) {
        categories << "I" << "II" << "III";
        *barset << 0 << 0 << 0;
    }

    // bar series and categories setup, same for all
    barseries = new QBarSeries(this);
    barseries->setLabelsPosition(QAbstractBarSeries::LabelsOutsideEnd);
    barseries->setLabelsVisible(true);
    barseries->setLabelsFormat("@value %");
    barseries->append(barset);
    chart->addSeries(barseries);


    // x-axis labels etc
    barcategoryaxis = new QBarCategoryAxis(this);
    barcategoryaxis->setLabelsFont(parent->midfont);
    barcategoryaxis->setLabelsColor(QColor(100,100,100));
    barcategoryaxis->setGridLineVisible(false);
    barcategoryaxis->setCategories(categories);

    // config axes
    QPen axisPen(GColor(CCARDBACKGROUND));
    axisPen.setWidth(1); // almost invisible
    chart->createDefaultAxes();
    chart->setAxisX(barcategoryaxis, barseries);
    barcategoryaxis->setLinePen(axisPen);
    barcategoryaxis->setLineVisible(false);
    chart->axisY(barseries)->setLinePen(axisPen);
    chart->axisY(barseries)->setLineVisible(false);
    chart->axisY(barseries)->setLabelsVisible(false);
    chart->axisY(barseries)->setRange(0,100);
    chart->axisY(barseries)->setGridLineVisible(false);

}

ZoneOverviewItem::~ZoneOverviewItem()
{
    delete chart;
}

DonutOverviewItem::DonutOverviewItem(ChartSpace *parent, QString name, QString symbol, QString meta) : ChartSpaceItem(parent, name)
{
    this->type = OverviewItemType::DONUT;
    this->symbol = symbol;
    this->meta = meta;

    RideMetricFactory &factory = RideMetricFactory::instance();
    this->metric = const_cast<RideMetric*>(factory.rideMetric(symbol));

    chart = new QChart(this);

    // basic chart setup
    chart->setBackgroundVisible(false); // draw on canvas
    chart->legend()->setVisible(false); // no legends
    chart->setTitle(""); // none wanted
    chart->setAnimationOptions(QChart::AllAnimations);

    // we have a mid sized font for chart labels etc
    chart->setFont(parent->midfont);

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();

}

DonutOverviewItem::~DonutOverviewItem()
{
    delete chart;
}

MetricOverviewItem::MetricOverviewItem(ChartSpace *parent, QString name, QString symbol) : ChartSpaceItem(parent, name)
{
    // metric
    this->type = OverviewItemType::METRIC;
    this->symbol = symbol;

    RideMetricFactory &factory = RideMetricFactory::instance();
    this->metric = const_cast<RideMetric*>(factory.rideMetric(symbol));
    if (metric) units = metric->units(GlobalContext::context()->useMetricUnits);

    // prepare the gold, silver and bronze medal
    gold = colouredPixmapFromPNG(":/images/medal.png", QColor(249,166,2)).scaledToWidth(ROWHEIGHT*2);
    silver = colouredPixmapFromPNG(":/images/medal.png", QColor(192,192,192)).scaledToWidth(ROWHEIGHT*2);
    bronze = colouredPixmapFromPNG(":/images/medal.png", QColor(184,115,51)).scaledToWidth(ROWHEIGHT*2);

    // we may plot the metric sparkline if the tile is big enough
    bool bigdot = parent->scope == ANALYSIS ? true : false;
    sparkline = new Sparkline(this, name, bigdot);

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();
}

MetricOverviewItem::~MetricOverviewItem()
{
    delete sparkline;
}

TopNOverviewItem::TopNOverviewItem(ChartSpace *parent, QString name, QString symbol) : ChartSpaceItem(parent, name), click(false), clickthru(NULL)
{
    // metric
    this->type = OverviewItemType::TOPN;
    this->symbol = symbol;

    animator=new QPropertyAnimation(this, "transition");

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();

    configChanged(0);
}

void
TopNOverviewItem::configChanged(qint32)
{
    RideMetricFactory &factory = RideMetricFactory::instance();
    this->metric = const_cast<RideMetric*>(factory.rideMetric(symbol));
    if (metric) units = metric->units(GlobalContext::context()->useMetricUnits);
}

TopNOverviewItem::~TopNOverviewItem()
{
    animator->stop();
    delete animator;
}

PMCOverviewItem::PMCOverviewItem(ChartSpace *parent, QString symbol) : ChartSpaceItem(parent, "")
{
    // PMC doesn't have a title as we show multiple things
    // metric
    this->type = OverviewItemType::PMC;
    this->symbol = symbol;

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();
}

PMCOverviewItem::~PMCOverviewItem()
{
}

MetaOverviewItem::MetaOverviewItem(ChartSpace *parent, QString name, QString symbol) : ChartSpaceItem(parent, name)
{
    // metric or meta or pmc
    this->type = OverviewItemType::META;
    this->symbol = symbol;
    sparkline = NULL;

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();

    configChanged(0);
}

void
MetaOverviewItem::configChanged(qint32)
{
    //  Get the field type
    fieldtype = -1;
    foreach(FieldDefinition p, GlobalContext::context()->rideMetadata->getFields()) {
        if (p.name == symbol) {
            fieldtype = p.type;
            break;
         }
    }

    // sparkline if are we numeric?
    if (fieldtype == FIELD_INTEGER || fieldtype == FIELD_DOUBLE) {
        if (sparkline == NULL) sparkline = new Sparkline(this, name);
    } else {
        if (sparkline) {
            delete sparkline;
            sparkline = NULL;
        }
    }
}

MetaOverviewItem::~MetaOverviewItem()
{
    if (sparkline) delete sparkline;
}

IntervalOverviewItem::IntervalOverviewItem(ChartSpace *parent, QString name, QString xsymbol, QString ysymbol, QString zsymbol) : ChartSpaceItem(parent, name)
{
    if (parent->scope == OverviewScope::ANALYSIS) this->type = OverviewItemType::INTERVAL;
    if (parent->scope == OverviewScope::TRENDS) this->type = OverviewItemType::ACTIVITIES;

    this->xsymbol = xsymbol;
    this->ysymbol = ysymbol;
    this->zsymbol = zsymbol;
    this->item = NULL;
    block=false;

    // we may plot the metric sparkline if the tile is big enough
    bubble = new BubbleViz(this, "intervals");
    bubble->setIntervalHoverSignal(true);

    configwidget = new OverviewItemConfig(this);
    configwidget->hide();

    // refresh when interval events happen (only on analysis view)
    if (parent->scope == OverviewScope::ANALYSIS) {
        connect(parent->context, SIGNAL(intervalSelected()), this, SLOT(intervalSelectRefresh()));
        //connect(parent->context, SIGNAL(intervalsChanged()), this, SLOT(intervalSelectRefresh()));
        connect(parent->context, SIGNAL(intervalsUpdate(RideItem*)), this, SLOT(intervalSelectRefresh()));
        connect(parent->context, SIGNAL(intervalHover(IntervalItem*)), this, SLOT(intervalHover(IntervalItem*)));
        //connect(parent->context, SIGNAL(intervalZoom(IntervalItem*)), this, SLOT(intervalSelectRefresh()));
        //connect(parent->context, SIGNAL(intervalItemSelectionChanged(IntervalItem*)), this, SLOT(intervalSelectRefresh()));
    }
}

IntervalOverviewItem::~IntervalOverviewItem()
{
    delete bubble;
}

void
KPIOverviewItem::setData(RideItem *item)
{
    if (item == NULL || item->ride() == NULL) return;

    // calculate the value...
    DataFilter parser(this, item->context, program);
    Result res = parser.evaluate(item, NULL);

    // set to zero for daft values
    value = res.string();
    if (value == "nan") value ="";

    // now set the progressbar
    progressbar->setValue(start, stop, value.toDouble());

    // convert value to hh:mm:ss when istime
    if (istime) value = time_to_string(value.toDouble(), true);

    // show/hide widgets on the basis of geometry
    itemGeometryChanged();
}

void
KPIOverviewItem::setDateRange(DateRange dr)
{
    Specification spec;
    spec.setDateRange(dr);
    setFilter(this, spec);

    // calculate the value...
    DataFilter parser(this, parent->context, program);
    Result res = parser.evaluate(spec, dr);

    // set to zero for daft values
    value = res.string();
    if (value == "nan") value ="";

    // now set the progressbar
    progressbar->setValue(start, stop, value.toDouble());

    // convert value to hh:mm:ss when istime
    if (istime) value = time_to_string(value.toDouble(), true);

    // show/hide widgets on the basis of geometry
    itemGeometryChanged();
}

void
DataOverviewItem::setData(RideItem *item)
{
    // reset interval hovering
    hovered = hoverinterval = hoversignal = "";

    // remove old values
    names.clear();
    units.clear();
    values.clear();
    files.clear();
    intervals.clear();
    heat.clear();

    if (item == NULL || item->ride() == NULL) return;

    // calculate the value...
    DataFilter parser(this, item->context, program);

    // so long as it evaluated correctly we can call the functions
    if (parser.root() && parser.errorList().isEmpty() && parent->context->currentRideItem()) {

        Specification spec;
        DateRange dr;

        // find them, users may forget or get it wrong ...
        fnames = parser.rt.functions.contains("names") ? parser.rt.functions.value("names") : NULL;
        funits = parser.rt.functions.contains("units") ? parser.rt.functions.value("units") : NULL;
        fvalues = parser.rt.functions.contains("values") ? parser.rt.functions.value("values") : NULL;
        ffiles = parser.rt.functions.contains("f") ? parser.rt.functions.value("f") : NULL;
        fintervals = parser.rt.functions.contains("i") ? parser.rt.functions.value("i") : NULL;
        fheat = parser.rt.functions.contains("heat") ? parser.rt.functions.value("heat") : NULL;

        // fetch the data
        if (fnames) names = parser.root()->eval(&parser.rt, fnames, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr).asString();
        if (funits) units = parser.root()->eval(&parser.rt, funits, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr).asString();
        if (fvalues) values = parser.root()->eval(&parser.rt, fvalues, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr).asString();
        if (ffiles) files = parser.root()->eval(&parser.rt, ffiles, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr).asString();
        if (fintervals) intervals = parser.root()->eval(&parser.rt, fintervals, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr).asString();
        if (fheat) heat = parser.root()->eval(&parser.rt, fheat, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr).asNumeric();

        postProcess();
    }
}

void
IntervalOverviewItem::intervalHover(IntervalItem *hover)
{
    this->hover = hover;
    intervalSelectRefresh();
}

void
IntervalOverviewItem::intervalSelectRefresh()
{
    // reset but don't animate
    if (item) setData(item, false);
}

void
DataOverviewItem::postProcess()
{
    // we never show units called "seconds" - they generally are time strings
    for(int i=0; i<units.count(); i++)
        if (units[i] == tr("seconds"))
            units[i] = "";

    // if we have some columns and more values than columns then show as rows
    // otherwise its a simple 3 column table; name value unit
    multirow = (names.count() > 0 && values.count() > names.count());

    // what are the max column sizes
    QFontMetrics fm(parent->midfont);

    // lets set the column widths by content
    // when painting the overall geometry is
    // taken into account to center/space this
    // but we set the minimum columns widths
    // based upon the data
    columnWidths.clear();

    if (!multirow) {

        // Just 3 columns - name value unit
        // max name width
        double maxwidth=0;
        foreach(QString name, names) {
            double width = fm.boundingRect(name).width();
            if (width > maxwidth) maxwidth=width;
        }
        columnWidths << maxwidth;

        // max unit width
        maxwidth=0;
        foreach(QString unit, units) {
            double width = fm.boundingRect(unit).width();
            if (width > maxwidth) maxwidth=width;
        }
        columnWidths << maxwidth;

        // max value width
        maxwidth=0;
        foreach(QString value, values) {
            double width = fm.boundingRect(value).width();
            if (width > maxwidth) maxwidth=width;
        }
        columnWidths << maxwidth;

        // tell the scrollbar we don't need scrolling
        scrollbar->setAreaHeight(0);

    } else {

        // One column per series - name1  name2  name3  name4
        //                         unit1  unit2  unit3  unit4
        //                         val1   val4   val7   val10
        //                         val2   val5   val8   val11
        //                         val3   val6   val9   val12
        int rows = values.count() / names.count();
        for(int i=0; i<names.count(); i++) {

            double maxwidth = names.count() > i ? fm.boundingRect(names[i]).width() : 0; // heading row
            double width = units.count() > i ? fm.boundingRect(units[i]).width() : 0;    // units row
            if (width > maxwidth) maxwidth = width;

            // now see if there are any values that are wider
            int offset = rows * i;
            for (int j=0; j<rows && offset+j < values.count(); j++) {
                width = fm.boundingRect(values[offset + j]).width();
                if (width > maxwidth) maxwidth = width;
            }

            columnWidths << maxwidth;
        }

        // tell scrollbar how big the scroll area is
        QFontMetrics fm(multirow ? parent->smallfont : parent->midfont, parent->device());
        double lineheight = fm.boundingRect("XXX").height() * 1.2f;
        scrollbar->setAreaHeight(rows * lineheight); // no scrolling
    }

    // keep the same sorting...
    if (lastsort != -1) sort(lastsort, lastorder);

    // show/hide widgets on the basis of geometry
    itemGeometryChanged();
}

void
DataOverviewItem::sort(int column, Qt::SortOrder order)
{
    if (column >= names.count()) return; // out of bounds

    // step 1: infer the type for column
    int isdate=0, istime=0, isnumber=0, isstring=0;

    int rows = values.count() / names.count();

    // dates in German are weird, a month is "Mai", "Juli" or even "Jan."
    // even when requesting a date in format dd MMM yyyy
    // remember: a dot (.) inside brackets ([]) does NOT need to be escaped
    QRegExp redate("^[0-9][0-9] [.A-Za-zÀ-ž\u0370-\u03FF\u0400-\u04FF]+ [0-9][0-9]*$");
    QRegExp retime("^[0-9:]*$");
    QRegExp renumber("^[0-9.-]*$");

    for(int i= rows * column; i<values.count() && i< rows * (column+1) ; i++) {
        QString &val = values[i];

        // check, the order here is important
        if (renumber.exactMatch(val)) isnumber++; // numbers + .
        else if (retime.exactMatch(val)) istime++; // numbers + :
        else if (redate.exactMatch(val)) isdate++; // numbers + date
        else isstring++; // all bets are off
    }
    // step 2: generate an argsort index as strings or numbers
    QVector<int> argsortindex;
    if (isstring) {

        QVector<QString> in;
        for(int i= rows * column; i<values.count() && i< rows * (column+1) ; i++) in<<values[i];
        argsortindex = Utils::argsort(in, order==Qt::AscendingOrder);

    } else {

        const QDate epoch(1970,1,1);
        QVector<double> in;
        for(int i= rows * column; i<values.count() && i< rows * (column+1) ; i++) {

            QString &val = values[i];

            // convert to double based upon type
            if (renumber.exactMatch(val)) in << val.toDouble();
            else if (retime.exactMatch(val)) {

                // time formats are painful- these are formats we use in the code...
                QStringList formats = { "h:mm:ss", "hh:mm:ss", "mm:ss", "s" };
                QTime attempt;

                foreach(QString format, formats) {
                    attempt = QTime::fromString(val, format);
                    if (attempt.isValid()) break;
                }

                in << QTime(0,0,0).secsTo(attempt);

            } else if (redate.exactMatch(val)) {

                // common formats
                QStringList formats = { "dd MMM yyyy", "dd MMM yy" };
                QDate attempt;

                foreach(QString format, formats) {
                    attempt = QDate::fromString(val, format);
                    if (attempt.isValid()) break;
                }
                in <<  epoch.daysTo(attempt);

            } else in << 0; // nope, don't understand
        }
        argsortindex = Utils::argsort(in, order==Qt::AscendingOrder);
    }

    // step 3: reorder values by the argsort index
    QVector<QString> ordered = values;
    for(int i=0; i<names.count(); i++) {
        // resequence column i
        for(int k=0; k<argsortindex.count() && (i*rows)+k < ordered.count(); k++)
            ordered[(i*rows)+k] = values[(i*rows)+argsortindex[k]];
    }

    // phew!
    values = ordered;

    QVector<double> heatordered = heat;
    for(int i=0; i<names.count(); i++) {
        // resequence column i
        for(int k=0; k<argsortindex.count() && (i*rows)+k < heatordered.count(); k++)
            heatordered[(i*rows)+k] = heat[(i*rows)+argsortindex[k]];
    }

    heat = heatordered;

    // don't forget the filenames used in clickthru
    if (files.count()) {
        QVector<QString> newfiles = files;

        // resequence
        for(int k=0; k<argsortindex.count() && k < newfiles.count(); k++)
            newfiles[k] = files[argsortindex[k]];

        files = newfiles;
    }

    // don't forget the intervals used in hover
    if (intervals.count()) {
        QVector<QString> newintervals = intervals;

        // resequence
        for(int k=0; k<argsortindex.count() && k < newintervals.count(); k++)
            newintervals[k] = intervals[argsortindex[k]];

        intervals = newintervals;
    }

    lastsort = column;
    lastorder = order;
}

void
DataOverviewItem::intervalHover(IntervalItem *item)
{
    if (item) hovered = item->name;
    else hovered = "";

    if (intervals.count()) update();
}

void
DataOverviewItem::setDateRange(DateRange dr)
{
    // remove old values
    names.clear();
    units.clear();
    values.clear();
    files.clear();
    heat.clear();

    // calculate the value...
    DataFilter parser(this, parent->context, program);

    // so long as it evaluated correctly we can call the functions
    if (parser.root() && parser.errorList().isEmpty() && parent->context->currentRideItem()) {

        Specification spec;
        spec.setDateRange(dr);
        setFilter(this, spec);

        // find them, users may forget or get it wrong ...
        fnames = parser.rt.functions.contains("names") ? parser.rt.functions.value("names") : NULL;
        funits = parser.rt.functions.contains("units") ? parser.rt.functions.value("units") : NULL;
        fvalues = parser.rt.functions.contains("values") ? parser.rt.functions.value("values") : NULL;
        ffiles = parser.rt.functions.contains("f") ? parser.rt.functions.value("f") : NULL;
        fheat = parser.rt.functions.contains("heat") ? parser.rt.functions.value("heat") : NULL;

        // fetch the data
        if (fnames) names = parser.root()->eval(&parser.rt, fnames, Result(0), 0, const_cast<RideItem*>(parent->context->currentRideItem()), NULL, NULL, spec, dr).asString();
        if (funits) units = parser.root()->eval(&parser.rt, funits, Result(0), 0, const_cast<RideItem*>(parent->context->currentRideItem()), NULL, NULL, spec, dr).asString();
        if (fvalues) values = parser.root()->eval(&parser.rt, fvalues, Result(0), 0, const_cast<RideItem*>(parent->context->currentRideItem()), NULL, NULL, spec, dr).asString();
        if (ffiles) files = parser.root()->eval(&parser.rt, ffiles, Result(0), 0, const_cast<RideItem*>(parent->context->currentRideItem()), NULL, NULL, spec, dr).asString();
        if (fheat) heat = parser.root()->eval(&parser.rt, fheat, Result(0), 0, const_cast<RideItem*>(parent->context->currentRideItem()), NULL, NULL, spec, dr).asNumeric();

        postProcess();
    }
}

void
RPEOverviewItem::setData(RideItem *item)
{
    if (item == NULL || item->ride() == NULL) return;

    // get last 30 days, if they exist
    QList<QPointF> points;

    // get the metadata value
    value = item->getText("RPE", "0");
    rperating->setValue(value);
    double v = value.toDouble();
    if (std::isinf(v) || std::isnan(v)) v=0;
    points << QPointF(SPARKDAYS, value.toDouble());

    // set the chart values with the last 10 rides
    int index = parent->context->athlete->rideCache->rides().indexOf(item);

    int offset = 1;
    double min = v;
    double max = v;
    double sum=0, count=0, avg = 0;
    while(index-offset >=0) { // ultimately go no further back than first ever ride

        // get value from items before me
        RideItem *prior = parent->context->athlete->rideCache->rides().at(index-offset);

        // are we still in range ?
        const qint64 old = prior->dateTime.daysTo(item->dateTime);
        if (old > SPARKDAYS) break;

        // only activities with matching sport flags
        if (prior->isRun == item->isRun && prior->isSwim == item->isSwim) {

           double v = prior->getText("RPE", "0").toDouble();
           if (std::isinf(v) || std::isnan(v)) v=0;

           // new no zero value
            if (v) {
                sum += v;
                count++;

                points<<QPointF(SPARKDAYS-old, v);
                if (v < min) min = v;
                if (v > max) max = v;
            }
        }

        offset++;
    }

    if (count) avg = sum / count;
    else avg = 0;

    // which way up should the arrow be?
    up = v > avg ? true : false;

    // add some space, if only one value +/- 10%
    double diff = (max-min)/10.0f;
    showrange=true;
    if (diff==0)  showrange=false;

    // update the sparkline
    sparkline->setPoints(points);

    // set range
    sparkline->setRange(min-diff,max+diff); // add 10% to each direction

    // set the values for upper lower
    upper = QString("%1").arg(max);
    lower = QString("%1").arg(min);
    mean = QString("%1").arg(avg, 0, 'f', 0);
}

void
MetricOverviewItem::setData(RideItem *item)
{
    if (item == NULL || item->ride() == NULL) return;

    // get last 30 days, if they exist
    QList<QPointF> points;

    // get the metric value
    value = item->getStringForSymbol(symbol, GlobalContext::context()->useMetricUnits);
    if (value == "nan") value ="";
    double v = item->getForSymbol(symbol, GlobalContext::context()->useMetricUnits);
    if (std::isinf(v) || std::isnan(v)) v=0;

    points << QPointF(SPARKDAYS, v);

    // set the chart values with the last 10 rides
    int index = parent->context->athlete->rideCache->rides().indexOf(item);

    int offset = 1;
    double min = v;
    double max = v;
    double sum=0, count=0, avg = 0;
    while(index-offset >=0) { // ultimately go no further back than first ever ride

        // get value from items before me
        RideItem *prior = parent->context->athlete->rideCache->rides().at(index-offset);

        // are we still in range ?
        const qint64 old = prior->dateTime.daysTo(item->dateTime);
        if (old > SPARKDAYS) break;

        // only activities with matching sport flags
        if (prior->isRun == item->isRun && prior->isSwim == item->isSwim) {

            double v = prior->getForSymbol(symbol, GlobalContext::context()->useMetricUnits);
            if (std::isinf(v) || std::isnan(v)) v=0;

            // new no zero value
            if (v) {
                sum += v;
                count++;

                points<<QPointF(SPARKDAYS-old, v);
                if (v < min) min = v;
                if (v > max) max = v;
            }
        }

        offset++;
    }

    if (count) avg = sum / count;
    else avg = 0;

    // which way up should the arrow be?
    up = v > avg ? true : false;
    up = (metric && metric->isLowerBetter()) ? !up : up;

    // add some space, if only one value +/- 10%
    double diff = (max-min)/10.0f;
    showrange=true;
    if (diff==0) {
        showrange=false;
        diff = value.toDouble()/10.0f;
    }

    int rank30=0; // 30d rank
    int rank90=0; // 90d rank
    int rank365=0; // 365d rank
    int alltime=0; // all time
    int career=0; // Career
    bool first=true;
    index = parent->context->athlete->rideCache->rides().count()-1;
    v = item->getForSymbol(symbol, GlobalContext::context()->useMetricUnits);
    while(index >=0) { // ultimately go no further back than first ever ride

        // get value from items before me
        RideItem *prior = parent->context->athlete->rideCache->rides().at(index);
        if (prior == item) {
            index--;
            continue;
        }

        // days ago?
        int daysago = prior->dateTime.date().daysTo(item->dateTime.date());
        double priorv = prior->getForSymbol(symbol);

        // lower or higher
        if (metric && metric->isLowerBetter()) {
            if (daysago >= 0) {
                if (daysago < 30 && priorv <= v) rank30++;
                if (daysago < 90 && priorv <= v) rank90++;
                if (daysago < 365 && priorv <= v) rank365++;
                if (priorv <= v) alltime++;
            }
            if (priorv <= v) career++;

        } else {
            if (daysago >= 0) {
                if (daysago < 30 && priorv >= v) rank30++;
                if (daysago < 90 && priorv >= v) rank90++;
                if (daysago < 365 && priorv >= v) rank365++;
                if (priorv >= v) alltime++;
            }
            if (priorv >= v) career++;
        }

        first=false;
        index--;
    }

    // set rankstring
    rank=99;
    if (first != true) {
        // we get to compare
        if (alltime < 3) {
            beststring = tr("Career");
            rank = career+1;
        } else if (alltime < 3) {
            beststring = tr("So far");
            rank = alltime+1;
        } else if (rank365 < 3) {
            beststring = tr("Year");
            rank = rank365+1;
        } else if (rank90 < 3) {
            beststring = tr("90d");
            rank = rank90+1;
        } else if (rank30 < 3) {
            beststring = tr("30d");
            rank = rank30+1;
        } else {
            beststring = "";
            rank=99;
        }
    }

    // update the sparkline
    sparkline->setPoints(points);

    // set range
    sparkline->setRange(min-diff,max+diff); // add 10% to each direction

    // set the values for upper lower
    const RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *m = factory.rideMetric(symbol);
    if (m) {
        upper = m->toString(max);
        lower = m->toString(min);
        mean = m->toString(avg);
    }
}

void
MetricOverviewItem::setDateRange(DateRange dr)
{
    if (!metric) return; // avoid crashes when metric is not available

    // for metrics lets truncate to today
    if (dr.to > QDate::currentDate()) dr.to = QDate::currentDate();

    Specification spec;
    spec.setDateRange(dr);
    setFilter(this, spec);

    // aggregate sum and count etc
    double v=0; // value
    double c=0; // count
    bool first=true;
    foreach(RideItem *item, parent->context->athlete->rideCache->rides()) {

        if (!spec.pass(item)) continue;

        // get value and count
        double value = item->getForSymbol(symbol, GlobalContext::context()->useMetricUnits);
        double count = item->getCountForSymbol(symbol);
        if (count <= 0) count = 1;

        // ignore zeroes when aggregating?
        if (metric->aggregateZero() == false && value == 0) continue;

        // what we gonna do with this?
        switch(metric->type()) {
        case RideMetric::StdDev:
        case RideMetric::MeanSquareRoot:
        case RideMetric::Average:
            v += value*count;
            c += count;
            break;
        case RideMetric::Total:
        case RideMetric::RunningTotal:
            v += value;
            break;
        case RideMetric::Peak:
            if (first || value > v) v = value;
            break;
        case RideMetric::Low:
            if (first || value < v) v = value;
            break;
            break;
        }
        first = false;
    }

    // now apply averaging etc
    switch(metric->type()) {
    case RideMetric::StdDev:
    case RideMetric::MeanSquareRoot:
    case RideMetric::Average:
        if (c) v  = v / c;
        else v = 0;
        break;
    default: break;
    }

    // get the metric value
    const RideMetricFactory &factory = RideMetricFactory::instance();
    RideMetric *m = const_cast<RideMetric*>(factory.rideMetric(symbol));
    if (std::isinf(v) || std::isnan(v)) v=0;
    if (m) {
        value = m->toString(v);
    } else {
        value = Utils::removeDP(QString("%1").arg(v));
        if (value == "nan") value ="";
    }

    // metric history
    QList<QPointF> points;

    // how many days
    QDate earliest(1900,01,01);
    sparkline->setDays(earliest.daysTo(dr.to) - earliest.daysTo(dr.from));

    double min=0, max=0;
    double sum=0;
    first=true;
    foreach(RideItem *item, parent->context->athlete->rideCache->rides()) {

        if (!spec.pass(item)) continue;

        double v = item->getForSymbol(symbol, GlobalContext::context()->useMetricUnits);

        // no zero values
        if (v == 0) continue;

        // cum sum for Total and RunningTotals
        if (metric->type() == RideMetric::Total || metric->type() == RideMetric::RunningTotal) {
            sum += v;
            v = sum;
        }

        points << QPointF(earliest.daysTo(item->dateTime.date()) - earliest.daysTo(dr.from), v);

        if (v < min) min=v;
        if (first || v > max) max=v;
        first = false;
    }

    // do we want fill?
    sparkline->setFill(metric->type()== RideMetric::Total || metric->type()== RideMetric::RunningTotal);

    // update the sparkline
    sparkline->setPoints(points);

    // set range
    sparkline->setRange(min*1.1,max*1.1); // add 10% to each direction

}

static bool entrylessthan(struct topnentry &a, const topnentry &b) { return a.v < b.v; }
static bool entrymorethan(struct topnentry &a, const topnentry &b) { return a.v > b.v; }

void
TopNOverviewItem::setDateRange(DateRange dr)
{
    if (!metric) return; // avoid crashes when metric is not available

    // clear out the old values
    ranked.clear();

    // filtering
    Specification spec;
    spec.setDateRange(dr);
    setFilter(this, spec);

    // pmc data
    PMCData stressdata(parent->context, spec, "coggan_tss");
    maxvalue="";
    maxv=0; // must never have -ve max
    minv=0; // always zero minimum
    foreach(RideItem *item, parent->context->athlete->rideCache->rides()) {

        if (!spec.pass(item)) continue;

        // get value and count
        double v = item->getForSymbol(symbol, GlobalContext::context()->useMetricUnits);
        QString value = item->getStringForSymbol(symbol, GlobalContext::context()->useMetricUnits);
        int index = stressdata.indexOf(item->dateTime.date());
        double tsb = 0;
        if (index >= 0 && index < stressdata.sb().count()) tsb = stressdata.sb()[index];

        // add to the list
        QColor color = (item->color.red() == 1 && item->color.green() == 1 && item->color.blue() == 1) ? GColor(CPLOTMARKER) : item->color;
        ranked << topnentry(item->dateTime.date(), v, value, color, tsb, item);

        // biggest value?
        if (v > maxv) {
            maxvalue=value;
            maxv = v;
        }

        // minv should be 0 unless it goes negative
        if (v < minv) minv=v;
    }

    // sort the list
    if (metric->type() == RideMetric::Low || metric->isLowerBetter()) std::sort(ranked.begin(), ranked.end(), entrylessthan);
    else std::sort(ranked.begin(), ranked.end(), entrymorethan);

    // change painting details
    itemGeometryChanged();

    // animate the transition
    animator->stop();
    animator->setStartValue(0);
    animator->setEndValue(100);
    animator->setEasingCurve(QEasingCurve::OutQuad);
    animator->setDuration(400);
    animator->start();
}

void
MetaOverviewItem::setData(RideItem *item)
{
    if (item == NULL || item->ride() == NULL) return;

    // non-numeric META
    if (!sparkline)  value = item->getText(symbol, "");

    // set the sparkline for numeric meta fields
    if (sparkline) {

        // get last 30 days, if they exist
        QList<QPointF> points;

        // include current activity value
        double v;

        // get the metadata value
        value = item->getText(symbol, "0");
        if (fieldtype == FIELD_DOUBLE) v = value.toDouble();
        else v = value.toInt();
        if (std::isinf(v) || std::isnan(v)) v=0;
        points << QPointF(SPARKDAYS, v);

        // set the chart values with the last 10 rides
        int index = parent->context->athlete->rideCache->rides().indexOf(item);

        int offset = 1;
        double min = v;
        double max = v;
        double sum=0, count=0, avg = 0;
        while(index-offset >=0) { // ultimately go no further back than first ever ride

            // get value from items before me
            RideItem *prior = parent->context->athlete->rideCache->rides().at(index-offset);

            // are we still in range ?
            const qint64 old = prior->dateTime.daysTo(item->dateTime);
            if (old > SPARKDAYS) break;

            // only activities with matching sport flags
            if (prior->isRun == item->isRun && prior->isSwim == item->isSwim) {

                double v;

                if (fieldtype == FIELD_DOUBLE)  v = prior->getText(symbol, "").toDouble();
                else v = prior->getText(symbol, "").toInt();

                if (std::isinf(v) || std::isnan(v)) v=0;

                // new no zero value
                if (v) {
                    sum += v;
                    count++;

                    points<<QPointF(SPARKDAYS-old, v);
                    if (v < min) min = v;
                    if (v > max) max = v;
                }
            }
            offset++;
        }

        if (count) avg = sum / count;
        else avg = 0;

        // which way up should the arrow be?
        up = v > avg ? true : false;

        // add some space, if only one value +/- 10%
        double diff = (max-min)/10.0f;
        showrange=true;
        if (diff==0) {
            showrange=false;
            diff = value.toDouble()/10.0f;
        }

        // update the sparkline
        sparkline->setPoints(points);

        // set range
        sparkline->setRange(min-diff,max+diff); // add 10% to each direction

        // set the values for upper lower
        upper = QString("%1").arg(max);
        lower = QString("%1").arg(min);
        mean = QString("%1").arg(avg, 0, 'f', 0);
    }
}

void
PMCOverviewItem::setData(RideItem *item)
{
    if (item == NULL || item->ride() == NULL) return;

    // get lts, sts, sb, rr for the input metric
    PMCData *pmc = parent->context->athlete->getPMCFor(symbol);

    QDate date = item ? item->dateTime.date() : QDate();
    lts = pmc->lts(date);
    sts = pmc->sts(date);
    stress = pmc->stress(date);
    sb = pmc->sb(date);
    rr = pmc->rr(date);

}

static bool lessthan(const aggmeta &a, const aggmeta &b)
{
    return a.value > b.value;
}

void
DonutOverviewItem::setDateRange(DateRange dr)
{
    if (!metric) return; // avoid crashes when metric is not available

    // stop any animation before starting, just in case- stops a crash
    // when we update a chart in the middle of its animation
    if (chart) chart->setAnimationOptions(QChart::NoAnimation);;

    // enable animation when setting values (disabled at all other times)
    if (chart) chart->setAnimationOptions(QChart::SeriesAnimations);

    struct aggregator {
        aggregator(double v, double c) : value(v), count(c) {}
        double value, count;
    };

    Specification spec;
    spec.setDateRange(dr);
    setFilter(this, spec);

    // aggregate sum and count etc
    QMap<QString, aggregator> data;
    foreach(RideItem *item, parent->context->athlete->rideCache->rides()) {

        if (!spec.pass(item)) continue;

        // get meta value
        QString category = item->getText(meta, "");
        aggregator d = data.value(category, aggregator(-1,-1));

        // is this first time we've seen this meta value?
        bool first = false;
        if (d.value == -1 && d.count == -1) {
            first = true;
            d.value=0;
            d.count=0;
        }

        // get metric value and count
        double value = item->getForSymbol(symbol, GlobalContext::context()->useMetricUnits);
        double count = item->getCountForSymbol(symbol);
        if (count <= 0) count = 1;

        // ignore zeroes when aggregating?
        if (metric->aggregateZero() == false && value == 0) continue;

        // what we gonna do with this?
        switch(metric->type()) {
        case RideMetric::StdDev:
        case RideMetric::MeanSquareRoot:
        case RideMetric::Average:
            d.value = (d.value*d.count) + (value * count); // convert to sum
            d.count += count;
            d.value = d.value / d.count; // turn back to average
            break;
        case RideMetric::Total:
        case RideMetric::RunningTotal:
            d.value += value;
            break;
        case RideMetric::Peak:
            if (first || value > d.value) d.value = value;
            break;
        case RideMetric::Low:
            if (first || value < d.value) d.value = value;
            break;
            break;
        }

        // update map
        data.insert(category, d);
    }

    // now create a sorted list of values
    values.clear();

    double sum=0;
    QMapIterator<QString, aggregator>it(data);
    while (it.hasNext()) {
        it.next();
        values << aggmeta(it.key(), it.value().value, 0, it.value().count);
        sum += it.value().value;
    }

    // calculate as percentages
    for(int i=0; i<values.count(); i++) values[i].percentage = (values[i].value / sum) * 100;

    // sort with highest values first
    std::sort(values.begin(), values.end(), lessthan);

    // wipe any existing series
    chart->removeAllSeries();

    // now set the pie chart
    QPieSeries *add = new QPieSeries();
    connect(add, SIGNAL(hovered(QPieSlice*,bool)), this, SLOT(hoverSlice(QPieSlice*,bool)));

    add->setPieSize(0.7);
    add->setHoleSize(0.5);

    // setup the slices
    int maxslices=8; // more than this and we aggregate into a category 'other'
    int minslices=5; // more than this and we get a small font for labels
    for (int i=0; i<values.count() && i<maxslices; i++) {
        // get label?
        add->append(values[i].category.trimmed() == "" ? "blank" : values[i].category, values[i].percentage);
    }

    // add "other"
    if (values.count() >= maxslices) {
        // other....
        double sum=0;
        for(int i=maxslices; i<values.count(); i++) {
            sum += values[i].percentage;
        }
        add->append("other", sum);
    }

    // now do the colors
    double i=1;
    QColor min=GColor(CPLOTMARKER);
    QColor max=GCColor::invertColor(GColor(CCARDBACKGROUND));
    bool exploded=false;
    foreach(QPieSlice *slice, add->slices()) {

        //slice->setExploded();
        slice->setLabelVisible();
        slice->setPen(Qt::NoPen);

        // gradient color
        QColor color = QColor(min.red() + (double(max.red()-min.red()) * (i/double(add->slices().count()))),
                              min.green() + (double(max.green()-min.green()) * (i/double(add->slices().count()))),
                              min.blue() + (double(max.blue()-min.blue()) * (i/double(add->slices().count()))));

        slice->setColor(color);
        slice->setLabelColor(QColor(150,150,150));
        if (values.count() <= minslices) slice->setLabelFont(parent->midfont);
        else slice->setLabelFont(parent->tinyfont);

        // set the largest value that isn't duff to exploded and red, so it stands out from the rest
        if (exploded == false && slice->label() != "blank" && slice->label() != "other") {
            slice->setExploded(true);
            slice->setColor(QColor(Qt::darkRed));
            exploded=true;
            i--; // save a hue
        }

        //if (i <colors.size()) slice->setBrush(QColor(colors.at(i)));
        //else slice->setBrush(Qt::red);
        i++;
    }

    // shadows on pie
    chart->setDropShadowEnabled(false);

    // set the pie chart
    chart->addSeries(add);
}

void
DonutOverviewItem::hoverSlice(QPieSlice *slice, bool state)
{
    if (state == true) {
        value = QString("%1%").arg(round(slice->percentage()*100));
        valuename=slice->label();
    } else {
        value = ""; // unhover
        valuename = "";
    }
    update();
}

void
ZoneOverviewItem::setDateRange(DateRange dr)
{
    QVector<double> vals(10); // max 10 seems ok
    vals.fill(0);

    // stop any animation before starting, just in case- stops a crash
    // when we update a chart in the middle of its animation
    if (chart) chart->setAnimationOptions(QChart::NoAnimation);;

    // enable animation when setting values (disabled at all other times)
    if (chart) chart->setAnimationOptions(QChart::SeriesAnimations);

    Specification spec;
    spec.setDateRange(dr);
    setFilter(this, spec);

    // aggregate sum and count etc
    foreach(RideItem *item, parent->context->athlete->rideCache->rides()) {

        if (!spec.pass(item)) continue;

        switch(series) {

            //
            // HEARTRATE
            //
            case RideFile::hr:
            {
                if (polarized) {
                    for(int i=0; i<3; i++) {
                        vals[i] += item->getForSymbol(timeInZonesHRPolarized[i]);
                    }
                } else if (parent->context->athlete->hrZones(item->sport)) {

                    int numhrzones;
                    int hrrange = parent->context->athlete->hrZones(item->sport)->whichRange(item->dateTime.date());

                    if (hrrange > -1) {

                        numhrzones = parent->context->athlete->hrZones(item->sport)->numZones(hrrange);
                        for(int i=0; i<categories.count() && i < numhrzones;i++) {
                            vals[i] += item->getForSymbol(timeInZonesHR[i]);
                        }
                    }
                }
            }
            break;

            //
            // POWER
            //
            default:
            case RideFile::watts:
            {
                if (polarized) {
                    for(int i=0; i<3; i++) {
                        vals[i] += item->getForSymbol(timeInZonesPolarized[i]);
                    }
                } else if (parent->context->athlete->zones(item->sport)) {

                    int numzones;
                    int range = parent->context->athlete->zones(item->sport)->whichRange(item->dateTime.date());

                    if (range > -1) {

                        numzones = parent->context->athlete->zones(item->sport)->numZones(range);
                        for(int i=0; i<categories.count() && i < numzones;i++) {
                            vals[i] += item->getForSymbol(timeInZones[i]);
                        }
                    }
                }
            }
            break;

            //
            // PACE
            //
            case RideFile::kph:
            {
                if (polarized) {
                    for(int i=0; i<3; i++) {
                        vals[i] += item->getForSymbol(paceTimeInZonesPolarized[i]);
                    }
                } else if ((item->isRun || item->isSwim) && parent->context->athlete->paceZones(item->isSwim)) {

                    int numzones;
                    int range = parent->context->athlete->paceZones(item->isSwim)->whichRange(item->dateTime.date());

                    if (range > -1) {

                        numzones = parent->context->athlete->paceZones(item->isSwim)->numZones(range);
                        for(int i=0; i<categories.count() && i < numzones;i++) {
                            vals[i] += item->getForSymbol(paceTimeInZones[i]);
                        }
                    }
                }
            }
            break;

            case RideFile::wbal:
            {
                for(int i=0; i<4; i++) {
                    vals[i] += item->getForSymbol(timeInZonesWBAL[i]);
                }
            }
            break;
        }
    }

    // now update the barset converting to percentages
    double sum=0;
    for(int i=0; i<categories.count();i++) sum += vals[i];
    for(int i=0; i<categories.count();i++) {
        if (sum) barset->replace(i, round(vals[i]/sum * 100));
        else barset->replace(i, round(vals[i]/sum * 100));
    }
}

void
ZoneOverviewItem::setData(RideItem *item)
{
    if (item == NULL || item->ride() == NULL) return;

    // stop any animation before starting, just in case- stops a crash
    // when we update a chart in the middle of its animation
    if (chart) chart->setAnimationOptions(QChart::NoAnimation);;


    // enable animation when setting values (disabled at all other times)
    if (chart) chart->setAnimationOptions(QChart::SeriesAnimations);

    switch(series) {

    //
    // HEARTRATE
    //
    case RideFile::hr:
    {
        if (polarized) {
            // get total time in zones
            double sum=0;
            for(int i=0; i<3; i++) sum += round(item->getForSymbol(timeInZonesHRPolarized[i]));

            // update as percent of total
            for(int i=0; i<3; i++) {
                double time =round(item->getForSymbol(timeInZonesHRPolarized[i]));
                if (time > 0 && sum > 0) barset->replace(i, round((time/sum) * 100));
                else barset->replace(i, 0);
            }
        } else if (parent->context->athlete->hrZones(item->sport)) {

            int numhrzones;
            int hrrange = parent->context->athlete->hrZones(item->sport)->whichRange(item->dateTime.date());

            if (hrrange > -1) {

                double sum=0;
                numhrzones = parent->context->athlete->hrZones(item->sport)->numZones(hrrange);
                for(int i=0; i<categories.count() && i < numhrzones;i++) {
                    sum += item->getForSymbol(timeInZonesHR[i]);
                }

                // update as percent of total
                for(int i=0; i<categories.count(); i++) {
                    double time =round(item->getForSymbol(timeInZonesHR[i]));
                    if (time > 0 && sum > 0) barset->replace(i, round((time/sum) * 100));
                    else barset->replace(i, 0);
                }

            } else {

                for(int i=0; i<categories.count(); i++) barset->replace(i, 0);
            }

        } else {

            for(int i=0; i<categories.count(); i++) barset->replace(i, 0);
        }
    }
    break;

    //
    // POWER
    //
    default:
    case RideFile::watts:
    {
        if (polarized) {
            // get total time in zones
            double sum=0;
            for(int i=0; i<3; i++) sum += round(item->getForSymbol(timeInZonesPolarized[i]));

            // update as percent of total
            for(int i=0; i<3; i++) {
                double time =round(item->getForSymbol(timeInZonesPolarized[i]));
                if (time > 0 && sum > 0) barset->replace(i, round((time/sum) * 100));
                else barset->replace(i, 0);
            }
        } else if (parent->context->athlete->zones(item->sport)) {

            int numzones;
            int range = parent->context->athlete->zones(item->sport)->whichRange(item->dateTime.date());

            if (range > -1) {

                double sum=0;
                numzones = parent->context->athlete->zones(item->sport)->numZones(range);
                for(int i=0; i<categories.count() && i < numzones;i++) {
                    sum += item->getForSymbol(timeInZones[i]);
                }

                // update as percent of total
                for(int i=0; i<categories.count(); i++) {
                    double time =round(item->getForSymbol(timeInZones[i]));
                    if (time > 0 && sum > 0) barset->replace(i, round((time/sum) * 100));
                    else barset->replace(i, 0);
                }

            } else {

                for(int i=0; i<categories.count(); i++) barset->replace(i, 0);
            }

        } else {

            for(int i=0; i<categories.count(); i++) barset->replace(i, 0);
        }
    }
    break;

    //
    // PACE
    //
    case RideFile::kph:
    {
        if (polarized) {
            // get total time in zones
            double sum=0;
            for(int i=0; i<3; i++) sum += round(item->getForSymbol(paceTimeInZonesPolarized[i]));

            // update as percent of total
            for(int i=0; i<3; i++) {
                double time =round(item->getForSymbol(paceTimeInZonesPolarized[i]));
                if (time > 0 && sum > 0) barset->replace(i, round((time/sum) * 100));
                else barset->replace(i, 0);
            }
        } else if ((item->isRun || item->isSwim) && parent->context->athlete->paceZones(item->isSwim)) {

            int numzones;
            int range = parent->context->athlete->paceZones(item->isSwim)->whichRange(item->dateTime.date());

            if (range > -1) {

                double sum=0;
                numzones = parent->context->athlete->paceZones(item->isSwim)->numZones(range);
                for(int i=0; i<categories.count() && i < numzones;i++) {
                    sum += item->getForSymbol(paceTimeInZones[i]);
                }

                // update as percent of total
                for(int i=0; i<categories.count(); i++) {
                    double time =round(item->getForSymbol(paceTimeInZones[i]));
                    if (time > 0 && sum > 0) barset->replace(i, round((time/sum) * 100));
                    else barset->replace(i, 0);
                }

            } else {

                for(int i=0; i<categories.count(); i++) barset->replace(i, 0);
            }

        } else {

            for(int i=0; i<categories.count(); i++) barset->replace(i, 0);
        }
    }
    break;

    case RideFile::wbal:
    {
        // get total time in zones
        double sum=0;
        for(int i=0; i<4; i++) sum += round(item->getForSymbol(timeInZonesWBAL[i]));

        // update as percent of total
        for(int i=0; i<4; i++) {
            double time =round(item->getForSymbol(timeInZonesWBAL[i]));
            if (time > 0 && sum > 0) barset->replace(i, round((time/sum) * 100));
            else barset->replace(i, 0);
        }
    }
    break;

    } // switch
}

void
RouteOverviewItem::setData(RideItem *item)
{
    if (item == NULL || item->ride() == NULL) return;

    if (scene() != NULL) {

        // only if we're place on the scene
        if (item->ride() && item->ride()->areDataPresent()->lat) {
            if (!routeline->isVisible()) routeline->show();
            routeline->setData(item);
        } else routeline->hide();
    }
}

void
IntervalOverviewItem::setDateRange(DateRange dr)
{
    // for metrics lets truncate to today
    if (dr.to > QDate::currentDate()) dr.to = QDate::currentDate();

    RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *xm = factory.rideMetric(xsymbol);
    const RideMetric *ym = factory.rideMetric(ysymbol);
    if (!xm || !ym) return; // avoid crashes when metrics are not available

    xdp = xm->precision();
    ydp = ym->precision();
    bubble->setAxisNames(xm ? xm->name() : "NA", ym ? ym->name() : "NA");

    Specification spec;
    spec.setDateRange(dr);
    setFilter(this, spec);

    double minx = 0;
    double maxx = 0;
    double miny = 0;
    double maxy = 0;
    double xoff = 0;
    double yoff = 0;
    bool first=true;

    QList<BPointF> points;
    foreach(RideItem *item, parent->context->athlete->rideCache->rides()) {

        if (!spec.pass(item)) continue;


        // get the x and y VALUE
        double x = item->getForSymbol(xsymbol, GlobalContext::context()->useMetricUnits);
        double y = item->getForSymbol(ysymbol, GlobalContext::context()->useMetricUnits);
        double z = item->getForSymbol(zsymbol, GlobalContext::context()->useMetricUnits);

        // truncate dates and use offsets
        if (first && xm->isDate())  xoff = x;
        if (first && ym->isDate())  yoff = y;
        x -= xoff;
        y -= yoff;

        BPointF add;
        add.x = x;
        add.xoff = xoff;
        add.y = y;
        add.yoff = yoff;
        add.z = z;
        add.fill = item->color;
        add.item = item; // for click thru
        if (add.fill.red() == 1 && add.fill.green() == 1 && add.fill.blue() == 1) add.fill = GColor(CPLOTMARKER);
        add.label = item->getText("Workout Code","blank");
        points << add;

        if (first || x<minx) minx=x;
        if (first || y<miny) miny=y;
        if (first || x>maxx) maxx=x;
        if (first || y>maxy) maxy=y;
        first = false;
    }

    // set scale
    double ydiff = (maxy-miny) / 10.0f;
    if (miny >= 0 && ydiff > miny) miny = ydiff;
    double xdiff = (maxx-minx) / 10.0f;
    if (minx >= 0 && xdiff > minx) minx = xdiff;
    maxx=ceil(maxx); minx=floor(minx);
    maxy=ceil(maxy); miny=floor(miny);

    // set range before points to filter
    bubble->setPoints(points, minx,maxx,miny,maxy, true);
}

void
IntervalOverviewItem::setData(RideItem *item)
{
    this->item = item;

    if (item == NULL || item->ride() == NULL) return;

    setData(item, true);
}

void
IntervalOverviewItem::setData(RideItem *item, bool animate)
{

    if (block) return;
    block = true;

    RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *xm = factory.rideMetric(xsymbol);
    const RideMetric *ym = factory.rideMetric(ysymbol);
    if (!xm || !ym) return; // avoid crashes when metrics are not available
    xdp = xm->precision();
    ydp = ym->precision();
    bubble->setAxisNames(xm ? xm->name() : "NA", ym ? ym->name() : "NA");

    double minx = 999999999;
    double maxx =-999999999;
    double miny = 999999999;
    double maxy =-999999999;

    //set the x, y series
    QList<BPointF> points;
    foreach(IntervalItem *interval, item->intervals()) {
        // get the x and y VALUE
        double x = interval->getForSymbol(xsymbol, GlobalContext::context()->useMetricUnits);
        double y = interval->getForSymbol(ysymbol, GlobalContext::context()->useMetricUnits);
        double z = interval->getForSymbol(zsymbol, GlobalContext::context()->useMetricUnits);

        BPointF add;
        add.x = x;
        add.y = y;
        add.z = z;

        if (interval == this->hover || interval->selected) add.fill = GColor(CPLOTMARKER);
        else add.fill = interval->color;
        add.label = interval->name;
        points << add;

        if (x<minx) minx=x;
        if (y<miny) miny=y;
        if (x>maxx) maxx=x;
        if (y>maxy) maxy=y;
    }


    // set scale
#if 0 // not clear why this is here or what it is supposed to do
      // but it results in items being filtered out.
    double ydiff = (maxy-miny) / 10.0f;
    if (miny >= 0 && ydiff > miny) miny = ydiff;
    double xdiff = (maxx-minx) / 10.0f;
    if (minx >= 0 && xdiff > minx) minx = xdiff;
#endif
    maxx=round(maxx); minx=round(minx);
    maxy=round(maxy); miny=round(miny);

    // set range before points to filter
    bubble->setPoints(points, minx,maxx,miny,maxy, animate);

    // avoid reentrancy
    block=false;
}


void
RPEOverviewItem::itemGeometryChanged() {

    QRectF geom = geometry();

    if (sparkline) {

        // make space for the rpe rating widget if needed
        int minh=6;
        if (!drag && geom.height() > (ROWHEIGHT*5)+20) {
            rperating->show();
            rperating->setGeometry(20+(ROWHEIGHT*2), ROWHEIGHT*3, geom.width()-40-(ROWHEIGHT*4), ROWHEIGHT*2);
        } else { // not set for meta or metric
            rperating->hide();
        }
        minh=7;

        // space enough?
        if (!drag && geom.height() > (ROWHEIGHT*minh)) {
            sparkline->show();
            sparkline->setGeometry(20, ROWHEIGHT*(minh-2), geom.width()-40, geom.height()-20-(ROWHEIGHT*(minh-2)));
        } else {
            sparkline->hide();
        }
    }
}

void
KPIOverviewItem::itemGeometryChanged() {

    QRectF geom = geometry();

    if (progressbar) {

        // make space for the progress bar
        int minh=6;

        // space enough?
        if (!drag && geom.height() > (ROWHEIGHT*minh) && (start != 0 || stop != 0)) {
            progressbar->setGeometry(20, ROWHEIGHT*(minh-2), geom.width()-40, geom.height()-20-(ROWHEIGHT*(minh-2)));
            progressbar->show();
        } else {
            progressbar->hide();
        }
    }
}

void
DataOverviewItem::itemGeometryChanged()
{
    if (names.count() > 0) {

        // if we are multirow we need to set the scrollbar
        int rows = values.count() / names.count();

        if (rows > 2) {

            // lets work out the dataarea, bit of a faff and need to keep aligned with paint
            // method - todo: refactor out duplicated code
            QFontMetrics fm(multirow ? parent->smallfont : parent->midfont, parent->device());
            double lineheight = fm.boundingRect("XXX").height() * 1.2f;
            double scrollwidth = fm.boundingRect("X").width();
            QRectF paintarea = QRectF(20,ROWHEIGHT*2, geometry().width()-40, geometry().height()-20-(ROWHEIGHT*2));
            QRectF dataarea = paintarea;
            dataarea.setY(dataarea.y() + (lineheight*2) + (lineheight*0.25f)); // 0.2 is the line spacing

            // set geometry to rhs
            scrollbar->setGeometry(dataarea.right() - scrollwidth, dataarea.top(), scrollwidth, dataarea.height());

        } else {
            // not needed
            scrollbar->setGeometry(0,0,0,0);
        }

    } else {

        // not needed- optimising out just adds another calc
        scrollbar->setGeometry(0,0,0,0);
    }
}

void
MetricOverviewItem::itemGeometryChanged() {

    QRectF geom = geometry();

    if (sparkline) {

        // make space for the rpe rating widget if needed
        int minh=6;

        // space enough?
        if (!drag && geom.height() > (ROWHEIGHT*minh)) {
            sparkline->show();
            sparkline->setGeometry(20, ROWHEIGHT*(minh-2), geom.width()-40, geom.height()-20-(ROWHEIGHT*(minh-2)));
        } else {
            sparkline->hide();
        }
    }
}

// painter truncates the list depending upon the size of the widget
void TopNOverviewItem::itemGeometryChanged() { }

void
MetaOverviewItem::itemGeometryChanged() {

    QRectF geom = geometry();

    if (sparkline) {

        // make space for the rpe rating widget if needed
        int minh=6;

        // space enough?
        if (!drag && geom.height() > (ROWHEIGHT*minh)) {
            sparkline->show();
            sparkline->setGeometry(20, ROWHEIGHT*(minh-2), geom.width()-40, geom.height()-20-(ROWHEIGHT*(minh-2)));
        } else {
            sparkline->hide();
        }
    }
}

void
PMCOverviewItem::itemGeometryChanged() { }

void
RouteOverviewItem::itemGeometryChanged() {

    QRectF geom = geometry();

    // route map needs adding to scene etc
    if (!drag) {
        if (myRideItem && myRideItem->ride() && myRideItem->ride()->areDataPresent()->lat) routeline->show();
        routeline->setGeometry(20,ROWHEIGHT+40, geom.width()-40, geom.height()-(60+ROWHEIGHT));

    } else routeline->hide();
}

void
IntervalOverviewItem::itemGeometryChanged() {

    QRectF geom = geometry();

    if (!drag) bubble->show();

    // disable animation when changing geometry
    bubble->setGeometry(20,20+(ROWHEIGHT*2), geom.width()-40, geom.height()-(40+(ROWHEIGHT*2)));
}


void
ZoneOverviewItem::dragChanged(bool drag)
{
    if (chart) {
        if (drag) chart->hide();
        else chart->show();
    }
}
void
ZoneOverviewItem::itemGeometryChanged() {

    QRectF geom = geometry();

    // if we contain charts etc lets update their geom
    if (!drag) chart->show();

    // disable animation when changing geometry
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setGeometry(20,20+(ROWHEIGHT*2), geom.width()-40, geom.height()-(40+(ROWHEIGHT*2)));
}

void
DonutOverviewItem::dragChanged(bool drag)
{
    if (chart) {
        if (drag) chart->hide();
        else chart->show();
    }
}

void
DonutOverviewItem::itemGeometryChanged() {

    QRectF geom = geometry();

    // if we contain charts etc lets update their geom
    if (!drag) chart->show();

    // disable animation when changing geometry
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->setGeometry(20,20+(ROWHEIGHT*2), geom.width()-40, geom.height()-(40+(ROWHEIGHT*2)));
}

void
KPIOverviewItem::itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

    double addy = 0;
    if (units != "") addy = QFontMetrics(parent->smallfont).height();

    // mid is slightly higher to account for space around title, move mid up
    double mid = (ROWHEIGHT*1.5f) + ((geometry().height() - (ROWHEIGHT*2)) / 2.0f) - (addy/2);

    // if we're deep enough to show the sparkline then stop
    if (geometry().height() > (ROWHEIGHT*6)) mid=((ROWHEIGHT*1.5f) + (ROWHEIGHT*3) / 2.0f) - (addy/2);

    // we align centre and mid
    QFontMetrics fm(parent->bigfont);
    QRectF rect = QFontMetrics(parent->bigfont, parent->device()).boundingRect(value);

    painter->setPen(GColor(CPLOTMARKER));
    painter->setFont(parent->bigfont);
    painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                              mid + (fm.ascent() / 3.0f)), value); // divided by 3 to account for "gap" at top of font

    // now units
    if (units != "" && addy > 0) {
        painter->setPen(QColor(100,100,100));
        painter->setFont(parent->smallfont);

        painter->drawText(QPointF((geometry().width() - QFontMetrics(parent->smallfont).width(units)) / 2.0f,
                              mid + (fm.ascent() / 3.0f) + addy), units); // divided by 3 to account for "gap" at top of font
    }


    // paint the range and mean if the chart is shown
    if (progressbar->isVisible()) {
        //sparkline->paint(painter, option, widget);

        // in small font max min at top bottom right of chart
        double bottom = progressbar->geometry().top() + (ROWHEIGHT*2.5);
        double left = ROWHEIGHT*2;
        double right = geometry().width()-(ROWHEIGHT*2);

        painter->setPen(QColor(100,100,100));
        painter->setFont(parent->smallfont);

        QString stoptext = istime ? time_to_string(stop, true) : Utils::removeDP(QString("%1").arg(stop));
        QString starttext = istime ? time_to_string (start, true) : Utils::removeDP(QString("%1").arg(start));
        painter->drawText(QPointF(right - QFontMetrics(parent->smallfont).width(stoptext), bottom), stoptext);
        painter->drawText(QPointF(left, bottom), starttext);

        // percentage in mid font...
        if (geometry().height() >= (ROWHEIGHT*8)) {

            double percent = round((progressbar->getCurrentValue()-start)/(stop-start) * 100.0);
            QString percenttext = Utils::removeDP(QString("%1%").arg(percent));

            QFontMetrics mfm(parent->midfont);
            QRectF mrect = QFontMetrics(parent->midfont, parent->device()).boundingRect(percenttext);

            if (!percenttext.startsWith("nan") && !percenttext.startsWith("inf") && percenttext != "0%") {

                // title color, copied code from chartspace.cpp, should really be a cleaner way to get these
                if (GCColor::luminance(GColor(CCARDBACKGROUND)) < 127) painter->setPen(QColor(200,200,200));
                else painter->setPen(QColor(70,70,70));

                painter->setFont(parent->midfont);
                painter->drawText(QPointF((geometry().width() - mrect.width()) / 2.0f, (ROWHEIGHT * 4.5) + mid + (mfm.ascent() / 3.0f)), percenttext); // divided by 3 to account for "gap" at top of font
            }
        }
    }
}

void
DataOverviewItem::wheelEvent(QGraphicsSceneWheelEvent *w)
{
    if (globalpos != QCursor::pos() && scrollbar->canscroll) {
        scrollbar->movePos(w->delta());
        w->accept();
    }

    return;
}

bool
DataOverviewItem::sceneEvent(QEvent *event)
{
    click = false;

    // do we need to signal hover?
    if (parent->context->currentRideItem() && hoverinterval != "" && hoverinterval != hoversignal) {
        hoversignal = hoverinterval;
        foreach(IntervalItem *it, const_cast<RideItem*>(parent->context->currentRideItem())->intervals()) {
            if (it->name == hoverinterval) {
                parent->context->notifyIntervalHover(it);
                break;
            }
        }
    }

    if (event->type() == QEvent::GraphicsSceneHoverMove) {

        // mouse moved so hover paint anyway
        update();


    }  else if (event->type() == QEvent::GraphicsSceneHoverLeave) {

        update();

    } else if (event->type() == QEvent::GraphicsSceneMousePress) {

        QRectF paintarea = QRectF(20,ROWHEIGHT*2, geometry().width()-40, geometry().height()-20-(ROWHEIGHT*2));

        QPoint vpos = parent->view->mapFromGlobal(QCursor::pos());
        QPointF pos = parent->view->mapToScene(vpos);
        QPointF cpos = pos - geometry().topLeft();

        // grab this before its interpreted as initiate drag
        if (paintarea.contains(cpos)) {
            event->accept();

            // pain will work out if we have something to select
            click = true;
            sortcolumn = -1;
            clickthru = NULL;
            update();

            return true;
        }

    } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {

        if (clickthru) {
            // do we need to select?
            parent->context->notifyRideSelected(clickthru);
            clickthru = NULL;
        }

        if (sortcolumn != -1) {
            // sort !!
            click=false;
            if (lastsort == sortcolumn) sort(sortcolumn, lastorder == Qt::DescendingOrder ? Qt::AscendingOrder : Qt::DescendingOrder);
            else sort(sortcolumn, Qt::DescendingOrder);
            update();
        }

        event->accept();
        return true;

    } else if (event->type() == QEvent::GraphicsSceneHoverEnter) {

        // lets remember to cursor global position
        globalpos = QCursor::pos();
        update();

    }
    return false;
}

QRectF
DataOverviewItem::hotspot()
{
    // use the paint area, regardless of rows as too expensive to compute
    return QRectF(20,ROWHEIGHT*2, geometry().width()-40, geometry().height()-20-(ROWHEIGHT*2));
}

// export the data to a CSV file
void
DataOverviewItem::exportData()
{
    // badly formed or no data to export
    if (names.count() == 0 || values.count() == 0 || values.count() < names.count()) {
        QMessageBox::critical(parent, tr("Export Table Data"), tr("Data malformed or not available."));
        return;
    }

    QString basename = name == "" ? "data" : name;
    QString suffix="csv";

    // get a filename to open
    QString fileName = QFileDialog::getSaveFileName(parent, tr("Export Table Data to CSV"),
                       QDir::homePath()+"/" + basename + ".csv",
                       ("*.csv;;"), &suffix, QFileDialog::DontUseNativeDialog); // native dialog hangs

    if (fileName.isEmpty()) return;

    // open and truncate
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) return;
    file.resize(0);

    // stream, output without a BOM, unlikely to be expected (?)
    QTextStream out(&file);
    // unified codepage and BOM for identification on all platforms
    out.setCodec("UTF-8");
    //out.setGenerateByteOrderMark(true);

    // headers- taking care to protect for csv output
    for (int col=0; col<names.count(); col++) {
        out << Utils::csvprotect(names[col], QChar(','));
        if ((col+1) < names.count()) out << ",";
    }
    out << "\n";

    // line items
    int rows = values.count() / names.count();
    for (int row=0; row<rows; row++) {

        // output a row
        for(int j=0; j<names.count(); j++) {
            int offset = (rows * j) + row;
            out << (values.count() > offset ? Utils::csvprotect(values[offset], QChar(',')) : "");
            if ((j+1) < names.count()) out << ",";
        }
        out << "\n";
    }

    // close
    out.flush();
    file.close();

}

void
DataOverviewItem::itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

    // paint nothing if no values - or a mismatch
    if (names.count() == 0 || values.count() == 0 || values.count() < names.count()) return;

    // don't paint on the edges
    QRectF geom = geometry();
    painter->setClipRect(40,40,geom.width()-80,geom.height()-80);

    // step 1: calculate paint metrics, colors, fonts, margins etc etc ...

    // we use the mid font, so lets get some font metrics
    QFontMetrics fm(multirow ? parent->smallfont : parent->midfont, parent->device());
    double lineheight = fm.boundingRect("XXX").height() * 1.2f;

    // default horizontal spacing, no flex here, is what it is
    double hmargin=ROWHEIGHT;

    // our bounding rectangle
    QRectF paintarea = QRectF(20,ROWHEIGHT*2, geometry().width()-40, geometry().height()-20-(ROWHEIGHT*2));

    // rows of data with column headings- will make paged and interactive in v3.7
    //
    // layout like this:  |hvvvsvvvsvvvsvvvsvvvh|
    // where:
    //     h is hmargin
    //     vvv is metric value
    //     s is hspace
    //
    double content = 0;
    foreach(double width, columnWidths) content += width;
    double hspace = geometry().width() - (content + hmargin + hmargin);
    hspace = (hspace > 0) ? hspace / (columnWidths.count()-1) : hmargin; // minimum space

    // fonts and colors for data
    QFont normal = multirow ? parent->smallfont : parent->midfont;
    QFont bold = normal;
    bold.setBold(true);

    // normal just grey, we highlight with plot marker
    QColor cnormal = (GCColor::luminance(GColor(CCARDBACKGROUND)) < 127) ? QColor(200,200,200) : QColor(70,70,70);


    // step 2: where is the mouse hovering, paint a background etc ....

    int hoverrow = -1; // remember if the mouse is over a particular row.

    // scroll position, only really relevant for multirow
    double scrollareapos = scrollbar->pos();
    int startrow = 0;
    if (scrollareapos) startrow = scrollareapos/lineheight + 1;

    // paint the hover background, only matters when have multiple rows
    if (multirow && underMouse()) {

        QRectF dataarea = paintarea;
        dataarea.setY(dataarea.y() + (lineheight*2) + (lineheight*0.25f)); // 0.2 is the line spacing

        QRectF headingarea = paintarea;
        headingarea.setHeight(lineheight*2);

        // single row just highlight the first data point
        if (!multirow) dataarea.setY(dataarea.y() - (lineheight*2));

        // set value based upon the location of the mouse
        QPoint vpos = parent->view->mapFromGlobal(QCursor::pos());
        QPointF pos = parent->view->mapToScene(vpos);
        QPointF cpos = pos - geometry().topLeft();

        // what row would we be on?
        int row = (cpos.y()-dataarea.topLeft().y())/lineheight;
        QRectF itemarea(dataarea.left(), dataarea.top()+(row*lineheight), dataarea.width(), lineheight);

        // in header or data area?
        if (headingarea.contains(cpos)) {

            // column number and rectangle
            int column = -1, cn=0;
            double xoffset = 0;
            QRectF crect;
            foreach(double width, columnWidths) {

                // bound for column cn heading
                QRectF prect = QRectF(headingarea.x()+xoffset, headingarea.y(), width+hspace, lineheight*2.25f);
                if (prect.contains(cpos)) {
                    column = cn;
                    crect=prect;
                }

                // and on to the next column
                cn++;
                xoffset += width + hspace;
            }

            // lets paint the hover background for the column
            if (column != -1) {
                painter->setPen(Qt::NoPen);
                QColor darkgray(120,120,120,120);
                painter->setBrush(darkgray);
                painter->drawRect(crect);

                if (click) sortcolumn = column;
            }

        } else if (!scrollbar->isDragging() && itemarea.contains(cpos)) {

            if (files.count() && row+startrow < files.count() && files[row+startrow] != "") {
                painter->setPen(Qt::NoPen);
                QColor darkgray(120,120,120,120);
                painter->setBrush(darkgray);
                painter->drawRect(itemarea);

                if (click) {
                    clickthru = parent->context->athlete->rideCache->getRide(files[row+startrow]);
                    click = false;
                }
            } else {
                // clicktru is not available bit lets at least make the
                if (multirow) hoverrow = row+startrow;
            }

            // if its an interval we should signal hovering
            if (intervals.count() && hoverrow >=0 && hoverrow < intervals.count()) {
                // signal hover!
                hoverinterval = intervals[hoverrow];
            }
        }
    }


    // step 3: paint the table from here ....
    painter->setFont(normal);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(cnormal, 4, Qt::SolidLine, Qt::RoundCap));

    if (multirow) {

        // heading row
        double xoffset = hmargin;
        double yoffset = paintarea.topLeft().y()+lineheight;
        for(int i=0; i<names.count(); i++) {

            // column name
            painter->drawText(xoffset, yoffset, names[i]); // todo: centering

            // if we are the sort column we need an indicator
            if (lastsort == i) {

                QFontMetrics fm(normal);
                QRectF tb = fm.boundingRect(names[i]);
                QRectF cb = fm.boundingRect("X");

                double direction = lastorder == Qt::AscendingOrder ? +1 : -1;

                // tick
                QPointF start(xoffset+tb.width()+(cb.width()*1.8), yoffset-(cb.height()/5));
                painter->drawLine(start, start-QPointF(cb.width()/2, direction * cb.height()/5));
                painter->drawLine(start-QPointF(cb.width()/2, direction * cb.height()/5), start-QPointF(cb.width(), 0));

                //painter->setPen(cnormal);
            }
            xoffset += columnWidths[i] + hspace;
        }
        yoffset += lineheight;

        // units row
        xoffset = hmargin;
        for(int i=0; i<names.count(); i++) {
            if (units.count() > i) painter->drawText(xoffset, yoffset, units[i]); // todo: centering
            xoffset += columnWidths[i] + hspace;
        }

        // separator for heading from data
        painter->setPen(QPen(cnormal, 4, Qt::SolidLine, Qt::RoundCap));
        painter->drawLine(paintarea.topLeft() + QPointF(0, lineheight * 2.2),
                          paintarea.topRight() + QPointF(0, lineheight * 2.2));

        // data values
        xoffset = hmargin;

        // lets see if there is a hoverrow signalled from another widget
        int hoveredrow=-1;
        if (parent->scope == OverviewScope::ANALYSIS && hovered != "" && intervals.count())  hoveredrow = intervals.indexOf(hovered);

        int rows = values.count() / names.count();
        for(int i=0; i<names.count(); i++) {

            // start at top
            yoffset = paintarea.topLeft().y()+(lineheight*3);

            // paint values in columns
            int offset = rows * i;
            for (int j=startrow; j<rows && offset+j < values.count(); j++) {
                QString value = values[offset+j];
                double heatvalue = heat.count() > (offset+j) ? heat[offset+j] : 0;

                // if no heat then no brush
                if (heatvalue == 0) painter->setBrush(Qt::NoBrush);
                else {
                    // lets use the heat color, but with a little translucency
                    QColor brushcolor = Utils::heatcolor(heatvalue);
                    brushcolor.setAlpha(127);
                    painter->fillRect(xoffset,yoffset-(lineheight*0.8), columnWidths[i]+(hspace/2), lineheight, QBrush(brushcolor));
                }

                // highlight rows when hovering and click thru not available
                if ((j == hoverrow || j == hoveredrow) && !scrollbar->isDragging()) {
                    painter->setFont(bold);
                    painter->setPen(GColor(CPLOTMARKER));
                } else {
                    painter->setFont(normal);
                    painter->setPen(cnormal);
                }

                painter->drawText(xoffset, yoffset, value);
                yoffset += lineheight;
            }

            // move across to next column
            xoffset += columnWidths[i] + hspace;
        }

    } else {

        // names (units) - left aligned
        double xoffset = hmargin;
        double yoffset = paintarea.topLeft().y()+lineheight;
        for(int i=0; i<names.count() && i<values.count(); i++) {

            QString text = names[i];
            if (i<units.count() && units[i] != "") text += " (" + units[i] + ")";

            painter->drawText(xoffset, yoffset, text);
            yoffset += lineheight;
        }

        // value - right aligned
        yoffset = paintarea.topLeft().y()+lineheight;
        for(int i=0; i<values.count(); i++) {

            QRectF bound = fm.boundingRect(values[i]);
            xoffset = geometry().width() - (bound.width() + hmargin);

            painter->drawText(xoffset, yoffset, values[i]);
            yoffset += lineheight;
        }
    }
}

void
RPEOverviewItem::itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

    // we need the metric units
    double addy = 0;
    if (rperating->isVisible()) addy=ROWHEIGHT*2; // shift up for rperating

    // mid is slightly higher to account for space around title, move mid up
    double mid = (ROWHEIGHT*1.5f) + ((geometry().height() - (ROWHEIGHT*2)) / 2.0f) - (addy/2);

    // if we're deep enough to show the sparkline then stop
    if (geometry().height() > (ROWHEIGHT*6)) mid=((ROWHEIGHT*1.5f) + (ROWHEIGHT*3) / 2.0f) - (addy/2);

    // we align centre and mid
    QFontMetrics fm(parent->bigfont);
    QRectF rect = QFontMetrics(parent->bigfont, parent->device()).boundingRect(value);

    painter->setPen(GColor(CPLOTMARKER));
    painter->setFont(parent->bigfont);
    painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f, mid + (fm.ascent() / 3.0f)), value); // divided by 3 to account for "gap" at top of font
    painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f, mid + (fm.ascent() / 3.0f)), value); // divided by 3 to account for "gap" at top of font

    // paint the range and mean if the chart is shown
    if (showrange && sparkline && sparkline->isVisible()) {

        // in small font max min at top bottom right of chart
        double top = sparkline->geometry().top();
        double bottom = sparkline->geometry().bottom();
        double right = sparkline->geometry().right();

        painter->setPen(QColor(100,100,100));
        painter->setFont(parent->smallfont);

        painter->drawText(QPointF(right - QFontMetrics(parent->smallfont).width(upper) - 80,
                              top - 40 + (fm.ascent() / 2.0f)), upper);
        painter->drawText(QPointF(right - QFontMetrics(parent->smallfont).width(lower) - 80,
                              bottom -40), lower);

        painter->setPen(QColor(50,50,50));
        painter->drawText(QPointF(right - QFontMetrics(parent->smallfont).width(mean) - 80,
                              ((top+bottom)/2) + (fm.tightBoundingRect(mean).height()/2) - 60), mean);
    }

    // regardless we always show up/down/same
    QPointF bl = QPointF((geometry().width() - rect.width()) / 2.0f, mid + (fm.ascent() / 3.0f));
    QRectF trect = fm.tightBoundingRect(value);
    QRectF trirect(bl.x() + trect.width() + ROWHEIGHT,
                   bl.y() - trect.height(), trect.height()*0.66f, trect.height());

    // trend triangle
    QPainterPath triangle;
    painter->setBrush(QBrush(QColor(up ? Qt::darkGreen : Qt::darkRed)));
    painter->setPen(Qt::NoPen);

    triangle.moveTo(trirect.left(), (trirect.top()+trirect.bottom())/2.0f);
    triangle.lineTo((trirect.left() + trirect.right()) / 2.0f, up ? trirect.top() : trirect.bottom());
    triangle.lineTo(trirect.right(), (trirect.top()+trirect.bottom())/2.0f);
    triangle.lineTo(trirect.left(), (trirect.top()+trirect.bottom())/2.0f);

    painter->drawPath(triangle);
}

void
MetricOverviewItem::itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

    // put the medal up so all else paints over it
    if (parent->scope == OverviewScope::ANALYSIS && metric && metric->isDate() == false && rank < 4) {

        // paint a medal
        QPixmap *medal;
        switch(rank) {
        case 1: medal=&gold; break;
        case 2: medal=&silver; break;
        default:
        case 3: medal=&bronze; break;
        }

        // draw unscaled
        painter->setClipRect(0,0,geometry().width(),geometry().height());
        painter->drawPixmap(QPointF(ROWHEIGHT, ROWHEIGHT*2), *medal);

        // rank
        if (beststring == tr("Career"))  painter->setPen(GColor(CPLOTMARKER));
        else painter->setPen(QPen(QColor(150,150,150)));
        painter->setFont(parent->midfont);
        painter->drawText(QRectF(0, (ROWHEIGHT*2)+medal->height()+10, medal->width()+(ROWHEIGHT*2), ROWHEIGHT*2), beststring, Qt::AlignTop|Qt::AlignHCenter);
    }

    double addy = 0;
    if (units != "" && units != tr("seconds")) addy = QFontMetrics(parent->smallfont).height();

    // mid is slightly higher to account for space around title, move mid up
    double mid = (ROWHEIGHT*1.5f) + ((geometry().height() - (ROWHEIGHT*2)) / 2.0f) - (addy/2);

    // if we're deep enough to show the sparkline then stop
    if (geometry().height() > (ROWHEIGHT*6)) mid=((ROWHEIGHT*1.5f) + (ROWHEIGHT*3) / 2.0f) - (addy/2);

    // we align centre and mid
    QFontMetrics fm(parent->bigfont);
    QRectF rect = QFontMetrics(parent->bigfont, parent->device()).boundingRect(value);

    painter->setPen(GColor(CPLOTMARKER));
    painter->setFont(parent->bigfont);
    painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                              mid + (fm.ascent() / 3.0f)), value); // divided by 3 to account for "gap" at top of font
    painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                              mid + (fm.ascent() / 3.0f)), value); // divided by 3 to account for "gap" at top of font

    // now units
    if (units != "" && addy > 0) {
        painter->setPen(QColor(100,100,100));
        painter->setFont(parent->smallfont);

        painter->drawText(QPointF((geometry().width() - QFontMetrics(parent->smallfont).width(units)) / 2.0f,
                              mid + (fm.ascent() / 3.0f) + addy), units); // divided by 3 to account for "gap" at top of font
    }

    // paint the range and mean if the chart is shown
    if (showrange && sparkline && sparkline->isVisible()) {
        //sparkline->paint(painter, option, widget);

        // in small font max min at top bottom right of chart
        double top = sparkline->geometry().top();
        double bottom = sparkline->geometry().bottom();
        double right = sparkline->geometry().right();

        painter->setPen(QColor(100,100,100));
        painter->setFont(parent->smallfont);

        painter->drawText(QPointF(right - QFontMetrics(parent->smallfont).width(upper) - 80,
                              top - 40 + (fm.ascent() / 2.0f)), upper);
        painter->drawText(QPointF(right - QFontMetrics(parent->smallfont).width(lower) - 80,
                              bottom -40), lower);

        painter->setPen(QColor(50,50,50));
        painter->drawText(QPointF(right - QFontMetrics(parent->smallfont).width(mean) - 80,
                              ((top+bottom)/2) + (fm.tightBoundingRect(mean).height()/2) - 60), mean);
    }

    // regardless we always show up/down/same
    QPointF bl = QPointF((geometry().width() - rect.width()) / 2.0f, mid + (fm.ascent() / 3.0f));
    QRectF trect = fm.tightBoundingRect(value);
    QRectF trirect(bl.x() + trect.width() + ROWHEIGHT,
                   bl.y() - trect.height(), trect.height()*0.66f, trect.height());

    // activity show if current one is up or down on trend for last 30 days..
    if (parent->scope == ANALYSIS && metric && !metric->isDate()) {

        // trend triangle
        QPainterPath triangle;
        painter->setBrush(QBrush(QColor(up ? Qt::darkGreen : Qt::darkRed)));
        painter->setPen(Qt::NoPen);

        triangle.moveTo(trirect.left(), (trirect.top()+trirect.bottom())/2.0f);
        triangle.lineTo((trirect.left() + trirect.right()) / 2.0f, up ? trirect.top() : trirect.bottom());
        triangle.lineTo(trirect.right(), (trirect.top()+trirect.bottom())/2.0f);
        triangle.lineTo(trirect.left(), (trirect.top()+trirect.bottom())/2.0f);

        painter->drawPath(triangle);
    }

}

QRectF
TopNOverviewItem::hotspot()
{
    return QRectF(20,ROWHEIGHT*2, geometry().width()-40, geometry().height()-20-(ROWHEIGHT*2));
}

bool
TopNOverviewItem::sceneEvent(QEvent *event)
{

    if (event->type() == QEvent::GraphicsSceneHoverMove) {

        // mouse moved so hover paint anyway
        update();

    }  else if (event->type() == QEvent::GraphicsSceneHoverLeave) {

        update();

    } else if (event->type() == QEvent::GraphicsSceneMousePress) {

        QRectF paintarea = QRectF(20,ROWHEIGHT*2, geometry().width()-40, geometry().height()-20-(ROWHEIGHT*2));

        QPoint vpos = parent->view->mapFromGlobal(QCursor::pos());
        QPointF pos = parent->view->mapToScene(vpos);
        QPointF cpos = pos - geometry().topLeft();

        // grab this before its interpreted as initiate drag
        if (paintarea.contains(cpos)) {
            event->accept();
            click = true;
            clickthru = NULL;
            update();
            return true;
        }

    } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {

        if (clickthru) {
            parent->context->notifyRideSelected(clickthru);
            clickthru = NULL;
        }
        return true;

    } else if (event->type() == QEvent::GraphicsSceneHoverEnter) {

        update();
    }
    return false;
}

void
TopNOverviewItem::itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

    // CALC GEOM / OFFSETS
    QFontMetrics fm(parent->smallfont);
    painter->setFont(parent->smallfont);

    // we paint the table, so lets work out the geometry and count etc
    QRectF paintarea = QRectF(20,ROWHEIGHT*2, geometry().width()-40, geometry().height()-20-(ROWHEIGHT*2));

    // max rows
    double margins = 30;
    double rowheight = fm.ascent() + margins;
    int maxrows = (paintarea.height()-margins) / rowheight;

    // min and max values for what is being painted
    minv=0;
    maxv=0;
    for (int i=0; i<maxrows && i<ranked.count(); i++) {

        double v = ranked[i].v;

        // biggest value?
        if (v > maxv)  maxv = v;

        // strings for rect sizing (remember neg values are longer strings)
        if (ranked[i].value.length() > maxvalue.length()) maxvalue = ranked[i].value;

        // minv should be 0 unless it goes negative
        if (v < minv) minv=v;
    }

    // number rect
    QRectF numrect = QRectF(0,0, fm.boundingRect(QString(" %1.").arg(maxrows)).width(), fm.boundingRect(QString(" %1.").arg(maxrows)).height());

    // date rect
    QRectF daterect = QRectF(0,0, fm.boundingRect("31 May yy").width(), fm.boundingRect("31 May yy").height());

    // value rect
    QRectF valuerect = QRectF(0,0, fm.boundingRect(maxvalue).width(), fm.boundingRect(maxvalue).height());

    // bar rect
    int width = paintarea.width() - (numrect.width() + daterect.width() + valuerect.width() + (margins * 6));
    QRectF barrect = QRectF(0,10, width, 30);

    // text color
    QColor cnormal = (GCColor::luminance(GColor(CCARDBACKGROUND)) < 127) ? QColor(200,200,200) : QColor(70,70,70);

    // PAINT
    for (int i=0; i<maxrows && i<ranked.count(); i++) {

        // containing area
        QRectF itemarea(paintarea.left(), paintarea.top()+margins+(i*rowheight), paintarea.width(), rowheight);

        // set value based upon the location of the mouse
        QPoint vpos = parent->view->mapFromGlobal(QCursor::pos());
        QPointF pos = parent->view->mapToScene(vpos);
        QPointF cpos = pos - geometry().topLeft();

        if (itemarea.contains(cpos)) {
            painter->setPen(Qt::NoPen);
            QColor darkgray(120,120,120,120);
            painter->setBrush(darkgray);
            painter->drawRect(itemarea);

            clickthru = ranked[i].item;
        }

        // rank
        painter->setPen(cnormal);
        painter->drawText(paintarea.topLeft()+QPointF(margins, margins+(i*rowheight)+fm.ascent()), QString("%1.").arg(i+1));

        // date
        QString datestring = ranked[i].date.toString("d MMM yy");
        painter->drawText(paintarea.topLeft()+numrect.topRight()+QPointF(margins*2, margins+(i*rowheight)+fm.ascent()), datestring);

        // bar width bearing in mind range might start at -ve value
        double width = (ranked[i].v / (maxv-minv)) * barrect.width() * (double(transition)/100.00);

        // 0 width is invisible, always at least 5
        if (width == 0) width = 5;

        // push rect across to account for 0 being in middle of bar when -ve values
        // NOTE: minv must NEV£R be > 0 !
        double offset = (barrect.width() / (maxv-minv)) * fabs(minv);

        // rectangles for full and this value
        QRectF fullbar(paintarea.left()+numrect.width()+daterect.width()+(margins*3), paintarea.top()+margins+(i*rowheight)+fm.ascent()-35, barrect.width(), barrect.height());
        QRectF bar(offset+paintarea.left()+numrect.width()+daterect.width()+(margins*3), paintarea.top()+margins+(i*rowheight)+fm.ascent()-35, width, barrect.height());

        // draw rects
        QBrush brush(QColor(100,100,100,100));
        painter->fillRect(fullbar, brush);
        QBrush markerbrush(ranked[i].color);
        painter->fillRect(bar, markerbrush);

        // value
        painter->drawText(paintarea.topLeft()+QPointF(numrect.width()+daterect.width()+fullbar.width()+(margins*4),0)+QPointF(margins, margins+(i*rowheight)+fm.ascent()), ranked[i].value);

    }

    click=false;
}

void
MetaOverviewItem::itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

    if (!sparkline && fieldtype >= 0) { // textual metadata field

        // mid is slightly higher to account for space around title, move mid up
        double mid = (ROWHEIGHT*1.5f) + ((geometry().height() - (ROWHEIGHT*2)) / 2.0f);

        // we align centre and mid
        QFontMetrics fm(parent->bigfont);
        QRectF rect = QFontMetrics(parent->bigfont, parent->device()).boundingRect(value);

        if (fieldtype == FIELD_TEXTBOX) {
            // long texts need to be formatted into a smaller font an word wrapped
            painter->setPen(QColor(150,150,150));
            painter->setFont(parent->smallfont);

            // draw text and wrap / truncate to bounding rectangle
            painter->drawText(QRectF(ROWHEIGHT, ROWHEIGHT*2.5, geometry().width()-(ROWHEIGHT*2),
                                                   geometry().height()-(ROWHEIGHT*4)), value);
        } else {

            // any other kind of metadata just paint it
            painter->setPen(GColor(CPLOTMARKER));
            painter->setFont(parent->bigfont);
            painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                                  mid + (fm.ascent() / 3.0f)), value); // divided by 3 to account for "gap" at top of font
        }


    }

    if (sparkline) { // if its a numeric metadata field

        double addy = 0;

        // mid is slightly higher to account for space around title, move mid up
        double mid = (ROWHEIGHT*1.5f) + ((geometry().height() - (ROWHEIGHT*2)) / 2.0f) - (addy/2);

        // if we're deep enough to show the sparkline then stop
        if (geometry().height() > (ROWHEIGHT*6)) mid=((ROWHEIGHT*1.5f) + (ROWHEIGHT*3) / 2.0f) - (addy/2);

        // we align centre and mid
        QFontMetrics fm(parent->bigfont);
        QRectF rect = QFontMetrics(parent->bigfont, parent->device()).boundingRect(value);

        painter->setPen(GColor(CPLOTMARKER));
        painter->setFont(parent->bigfont);
        painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                                  mid + (fm.ascent() / 3.0f)), value); // divided by 3 to account for "gap" at top of font
        painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                                  mid + (fm.ascent() / 3.0f)), value); // divided by 3 to account for "gap" at top of font

        // paint the range and mean if the chart is shown
        if (showrange && sparkline) {

            if (sparkline->isVisible()) {
            //sparkline->paint(painter, option, widget);

            // in small font max min at top bottom right of chart
            double top = sparkline->geometry().top();
            double bottom = sparkline->geometry().bottom();
            double right = sparkline->geometry().right();

            painter->setPen(QColor(100,100,100));
            painter->setFont(parent->smallfont);

            painter->drawText(QPointF(right - QFontMetrics(parent->smallfont).width(upper) - 80,
                                  top - 40 + (fm.ascent() / 2.0f)), upper);
            painter->drawText(QPointF(right - QFontMetrics(parent->smallfont).width(lower) - 80,
                                  bottom -40), lower);


            painter->setPen(QColor(50,50,50));
            painter->drawText(QPointF(right - QFontMetrics(parent->smallfont).width(mean) - 80,
                                  ((top+bottom)/2) + (fm.tightBoundingRect(mean).height()/2) - 60), mean);
            }

            // regardless we always show up/down/same
            QPointF bl = QPointF((geometry().width() - rect.width()) / 2.0f, mid + (fm.ascent() / 3.0f));
            QRectF trect = fm.tightBoundingRect(value);
            QRectF trirect(bl.x() + trect.width() + ROWHEIGHT,
                           bl.y() - trect.height(), trect.height()*0.66f, trect.height());


            // trend triangle
            QPainterPath triangle;
            painter->setBrush(QBrush(QColor(up ? Qt::darkGreen : Qt::darkRed)));
            painter->setPen(Qt::NoPen);

            triangle.moveTo(trirect.left(), (trirect.top()+trirect.bottom())/2.0f);
            triangle.lineTo((trirect.left() + trirect.right()) / 2.0f, up ? trirect.top() : trirect.bottom());
            triangle.lineTo(trirect.right(), (trirect.top()+trirect.bottom())/2.0f);
            triangle.lineTo(trirect.left(), (trirect.top()+trirect.bottom())/2.0f);

            painter->drawPath(triangle);

        }
    }
}

void
PMCOverviewItem::itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

    // Lets use friendly names for TSB et al, as described on the TrainingPeaks website
    // here: http://home.trainingpeaks.com/blog/article/4-new-mobile-features-you-should-know-about
    // as written by Ben Pryhoda their Senior Director of Product, Device and API Integrations
    // we will make this configurable later anyway, as calling SB 'Form' is rather dodgy.
    QFontMetrics tfm(parent->titlefont, parent->device());
    QFontMetrics bfm(parent->bigfont, parent->device());

    // 4 measures to show, depending upon how much space
    // so prioritise - SB then LTS, STS, RR

    double nexty = ROWHEIGHT;
    //
    // Stress Balance
    //
    painter->setPen(QColor(200,200,200));
    painter->setFont(parent->titlefont);
    QString string = QString(tr("Form"));
    QRectF rect = tfm.boundingRect(string);
    painter->drawText(QPointF(ROWHEIGHT / 2.0f,
                              nexty + (tfm.ascent() / 3.0f)), string); // divided by 3 to account for "gap" at top of font
    nexty += rect.height() + 30;

    painter->setPen(PMCData::sbColor(sb, GColor(CPLOTMARKER)));
    painter->setFont(parent->bigfont);
    string = QString("%1").arg(round(sb));
    rect = bfm.boundingRect(string);
    painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                              nexty + (bfm.ascent() / 3.0f)), string); // divided by 3 to account for "gap" at top of font
    nexty += ROWHEIGHT*2;

    //
    // Long term Stress
    //
    if (deep > 7) {

        painter->setPen(QColor(200,200,200));
        painter->setFont(parent->titlefont);
        QString string = QString(tr("Fitness"));
        QRectF rect = tfm.boundingRect(string);
        painter->drawText(QPointF(ROWHEIGHT / 2.0f,
                                      nexty + (tfm.ascent() / 3.0f)), string); // divided by 3 to account for "gap" at top of font
        nexty += rect.height() + 30;

        painter->setPen(PMCData::ltsColor(lts, GColor(CPLOTMARKER)));
        painter->setFont(parent->bigfont);
        string = QString("%1").arg(round(lts));
        rect = bfm.boundingRect(string);
        painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                                  nexty + (bfm.ascent() / 3.0f)), string); // divided by 3 to account for "gap" at top of font
        nexty += ROWHEIGHT*2;

    }

    //
    // Short term Stress
    //
    if (deep > 11) {

        painter->setPen(QColor(200,200,200));
        painter->setFont(parent->titlefont);
        QString string = QString(tr("Fatigue"));
        QRectF rect = tfm.boundingRect(string);
        painter->drawText(QPointF(ROWHEIGHT / 2.0f,
                                  nexty + (tfm.ascent() / 3.0f)), string); // divided by 3 to account for "gap" at top of font
        nexty += rect.height() + 30;

        painter->setPen(PMCData::stsColor(sts, GColor(CPLOTMARKER)));
        painter->setFont(parent->bigfont);
        string = QString("%1").arg(round(sts));
        rect = bfm.boundingRect(string);
        painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                                  nexty + (bfm.ascent() / 3.0f)), string); // divided by 3 to account for "gap" at top of font
        nexty += ROWHEIGHT*2;

    }

    //
    // Ramp Rate
    //
    if (deep > 14) {

        painter->setPen(QColor(200,200,200));
        painter->setFont(parent->titlefont);
        QString string = QString(tr("Risk"));
        QRectF rect = tfm.boundingRect(string);
        painter->drawText(QPointF(ROWHEIGHT / 2.0f,
                                  nexty + (tfm.ascent() / 3.0f)), string); // divided by 3 to account for "gap" at top of font
        nexty += rect.height() + 30;

        painter->setPen(PMCData::rrColor(rr, GColor(CPLOTMARKER)));
        painter->setFont(parent->bigfont);
        string = QString("%1").arg(round(rr));
        rect = bfm.boundingRect(string);
        painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                                  nexty + (bfm.ascent() / 3.0f)), string); // divided by 3 to account for "gap" at top of font
        nexty += ROWHEIGHT*2;

    }
}

// no custom painting for these guys, they contain widgets only
void RouteOverviewItem::itemPaint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {  }
void IntervalOverviewItem::itemPaint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {  }
void ZoneOverviewItem::itemPaint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) {  }

void DonutOverviewItem::itemPaint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setFont(parent->bigfont);
    painter->setPen(GColor(CPLOTMARKER));
    painter->drawText(chart->geometry(), Qt::AlignHCenter | Qt::AlignVCenter, value);
    painter->setPen(QColor(100,100,100));
    QFontMetrics fm(parent->midfont);
    painter->setFont(parent->midfont);
    painter->drawText(QRectF(0,ROWHEIGHT*2, geometry().width(), fm.ascent()+(ROWHEIGHT*2)), Qt::AlignHCenter | Qt::AlignTop, valuename);
}


//
// OverviewItem Configuration Widget
//
static bool insensitiveLessThan(const QString &a, const QString &b)
{
    return a.toLower() < b.toLower();
}
OverviewItemConfig::OverviewItemConfig(ChartSpaceItem *item) : QWidget(NULL), item(item), block(false)
{
    QVBoxLayout *main = new QVBoxLayout(this);
    QFormLayout *layout = new QFormLayout();
    main->addLayout(layout);

    if (item->type != OverviewItemType::KPI && item->type != OverviewItemType::DATATABLE) main->addStretch();

    // everyone except PMC
    if (item->type != OverviewItemType::PMC) {
        name = new QLineEdit(this);
        connect(name, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->addRow(tr("Name"), name);
    }

    if (item->type == OverviewItemType::DATATABLE) {
        exp = new QPushButton(tr("Export Data"));
        exp->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        legacySelector = new QComboBox(this);
        legacySelector->addItem("User defined", 0);
        legacySelector->addItem("Totals", DATA_TABLE_TOTALS);
        legacySelector->addItem("Averages", DATA_TABLE_AVERAGES);
        legacySelector->addItem("Maximums", DATA_TABLE_MAXIMUMS);
        legacySelector->addItem("Metrics", DATA_TABLE_METRICS);
        legacySelector->addItem("Zones", DATA_TABLE_ZONES);
        if (item->parent->scope & OverviewScope::ANALYSIS) {
            legacySelector->addItem("Intervals", DATA_TABLE_INTERVALS);
        } else {
            legacySelector->addItem("Activities", DATA_TABLE_TRENDS);
        }

        layout->addRow(tr("Legacy"), legacySelector);
        connect(legacySelector, SIGNAL(currentIndexChanged(int)), this, SLOT(setProgram(int)));
        connect(exp, SIGNAL(clicked()), item, SLOT(exportData()));

    }

    // trends view always has a filter
    if (item->parent->scope & OverviewScope::TRENDS) {
        filterEditor = new SearchFilterBox(this, item->parent->context);
        layout->addRow(tr("Filter"), filterEditor);
        connect(filterEditor->searchbox, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
    }

    // single metric names
    if (item->type == OverviewItemType::TOPN || item->type == OverviewItemType::METRIC  ||
        item->type == OverviewItemType::PMC || item->type == OverviewItemType::DONUT) {

        metric1 = new MetricSelect(this, item->parent->context, MetricSelect::Metric);
        layout->addRow(tr("Metric"), metric1);
        connect(metric1, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
    }

    // metric1, metric2 and metric3
    if (item->type == OverviewItemType::INTERVAL || item->type == OverviewItemType::ACTIVITIES) {
        metric1 = new MetricSelect(this, item->parent->context, MetricSelect::Metric);
        connect(metric1, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->addRow(tr("X Axis Metric"), metric1);

        metric2 = new MetricSelect(this, item->parent->context, MetricSelect::Metric);
        connect(metric2, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->addRow(tr("Y Axis Metric"), metric2);

        metric3 = new MetricSelect(this, item->parent->context, MetricSelect::Metric);
        connect(metric3, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->addRow(tr("Bubble Size Metric"), metric3);
    }

    if (item->type == OverviewItemType::META || item->type == OverviewItemType::DONUT) {
        meta1 = new MetricSelect(this, item->parent->context, MetricSelect::Meta);
        connect(meta1, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->addRow(tr("Field Name"), meta1);
    }

    if (item->type == OverviewItemType::ZONE) {
        series1 = new SeriesSelect(this, SeriesSelect::Zones);
        connect(series1, SIGNAL(currentIndexChanged(int)), this, SLOT(dataChanged()));
        layout->addRow(tr("Zone Series"), series1);

        // polarized
        cb1 = new QCheckBox(this);
        layout->addRow(tr("Polarized"), cb1);
        connect(cb1, SIGNAL(stateChanged(int)), this, SLOT(dataChanged()));
    }

    if (item->type == OverviewItemType::KPI) {

        double1 = new QDoubleSpinBox(this);
        double2 = new QDoubleSpinBox(this);
        double1->setMinimum(-9999999);
        double1->setMaximum(9999999);
        double2->setMinimum(-9999999);
        double2->setMaximum(9999999);
        layout->addRow(tr("Start"), double1);
        layout->addRow(tr("Stop"), double2);
        connect(double1, SIGNAL(valueChanged(double)), this, SLOT(dataChanged()));
        connect(double2, SIGNAL(valueChanged(double)), this, SLOT(dataChanged()));
    }


    if (item->type == OverviewItemType::KPI || item->type == OverviewItemType::DATATABLE) {

        //
        // Program editor... bit of a faff needs refactoring!!
        //
        QList<QString> list;
        QString last;
        SpecialFields sp;

        // get sorted list
        QStringList names = item->parent->context->rideNavigator->logicalHeadings;

        // start with just a list of functions
        list = DataFilter::builtins(item->parent->context);

        // ridefile data series symbols
        list += RideFile::symbols();

        // add special functions (older code needs fixing !)
        list << "config(cranklength)";
        list << "config(cp)";
        list << "config(aetp)";
        list << "config(ftp)";
        list << "config(w')";
        list << "config(pmax)";
        list << "config(cv)";
        list << "config(aetv)";
        list << "config(sex)";
        list << "config(dob)";
        list << "config(height)";
        list << "config(weight)";
        list << "config(lthr)";
        list << "config(aethr)";
        list << "config(maxhr)";
        list << "config(rhr)";
        list << "config(units)";
        list << "const(e)";
        list << "const(pi)";
        list << "daterange(start)";
        list << "daterange(stop)";
        list << "ctl";
        list << "tsb";
        list << "atl";
        list << "sb(BikeStress)";
        list << "lts(BikeStress)";
        list << "sts(BikeStress)";
        list << "rr(BikeStress)";
        list << "tiz(power, 1)";
        list << "tiz(hr, 1)";
        list << "best(power, 3600)";
        list << "best(hr, 3600)";
        list << "best(cadence, 3600)";
        list << "best(speed, 3600)";
        list << "best(torque, 3600)";
        list << "best(isopower, 3600)";
        list << "best(xpower, 3600)";
        list << "best(vam, 3600)";
        list << "best(wpk, 3600)";

        std::sort(names.begin(), names.end(), insensitiveLessThan);

        foreach(QString name, names) {

            // handle dups
            if (last == name) continue;
            last = name;

            // Handle bikescore tm
            if (name.startsWith("BikeScore")) name = QString("BikeScore");

            //  Always use the "internalNames" in Filter expressions
            name = sp.internalName(name);

            // we do very little to the name, just space to _ and lower case it for now...
            name.replace(' ', '_');
            list << name;
        }

        // program editor
        QVBoxLayout *pl= new QVBoxLayout();
        editor = new DataFilterEdit(this, item->parent->context);
        editor->setMinimumHeight(250 * dpiXFactor); // give me some space!
        editor->setMinimumWidth(450 * dpiXFactor); // give me some space!
        editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        DataFilterCompleter *completer = new DataFilterCompleter(list, this);
        editor->setCompleter(completer);
        errors = new QLabel(this);
        errors->setWordWrap(true);
        errors->setStyleSheet("color: red;");
        pl->addWidget(editor);
        pl->addWidget(errors);
        layout->addRow(tr("Program"), (QWidget*)NULL);
        layout->addRow(pl);

        connect(editor, SIGNAL(syntaxErrors(QStringList&)), this, SLOT(setErrors(QStringList&)));
        connect(editor, SIGNAL(textChanged()), this, SLOT(dataChanged()));

    }


    if (item->type == OverviewItemType::KPI) {
        // istime
        cb1 = new QCheckBox(this);
        layout->addRow(tr("Time"), cb1);
        connect(cb1, SIGNAL(stateChanged(int)), this, SLOT(dataChanged()));

        // units
        string1 = new QLineEdit(this);
        layout->addRow(tr("Units"), string1);
        connect(string1, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));

    }

    if (item->type != OverviewItemType::USERCHART) {
        // bg color
        bgcolor = new ColorButton(this, tr("Background"), QColor(item->color()), true);
        bgcolor->setSelectAll(true);
        layout->addRow(tr("Color"), bgcolor);
        connect(bgcolor, SIGNAL(colorChosen(QColor)), this, SLOT(dataChanged()));
    }

    // last item to export the data, note in main layout, aligned with bottom buttons
    if (item->type == OverviewItemType::DATATABLE)  {
        main->addWidget(exp);

        // need to align with botton buttons
        QMargins mm = main->contentsMargins();
        mm.setLeft(0);
        main->setContentsMargins(mm);
    }

    // reflect current config
    setWidgets();
}

void
OverviewItemConfig::setProgram(int index)
{
    if (index >= 0) {
        index = legacySelector->itemData(index).toInt();
        // for metric lookup
        DataFilter df(item->parent, item->parent->context);
        editor->setText(DataOverviewItem::getLegacyProgram(index, df.rt, item->parent->scope == OverviewScope::TRENDS));
    }
}

void
OverviewItemConfig::setErrors(QStringList &list)
{
    errors->setText(list.join(";"));
}

OverviewItemConfig::~OverviewItemConfig() {
#if 0
    // everyone except PMC
    if (item->type != OverviewItemType::PMC)  delete name ;

    // trends view always has a filter
    if (item->parent->scope & OverviewScope::TRENDS)  delete filterEditor;

    // single metric names
    if (item->type == OverviewItemType::TOPN || item->type == OverviewItemType::METRIC  ||
        item->type == OverviewItemType::PMC || item->type == OverviewItemType::DONUT) delete metric1;

    // metric1, metric2 and metric3
    if (item->type == OverviewItemType::INTERVAL || item->type == OverviewItemType::ACTIVITIES) {
        delete metric1;
        delete metric2;
        delete metric3;
    }

    if (item->type == OverviewItemType::META || item->type == OverviewItemType::DONUT)  delete meta1;

    if (item->type == OverviewItemType::ZONE) {
        delete series1;
        delete cb1;
    }

    if (item->type == OverviewItemType::KPI) {

        delete double1;
        delete double2;
    }


    if (item->type == OverviewItemType::KPI || item->type == OverviewItemType::DATATABLE) {

        delete editor;
        delete errors;
    }


    if (item->type == OverviewItemType::KPI) {
        delete cb1;
        delete string1;

    }
#endif
}

void
OverviewItemConfig::setWidgets()
{
    block = true;

    // always have a filter on trends view
    if (item->parent->scope & OverviewScope::TRENDS)  filterEditor->setFilter(item->datafilter);

    // set the widget values from the item
    switch(item->type) {
    case OverviewItemType::RPE:
        {
            RPEOverviewItem *mi = dynamic_cast<RPEOverviewItem*>(item);
            name->setText(mi->name);
        }
        break;

    case OverviewItemType::METRIC:
        {
            MetricOverviewItem *mi = dynamic_cast<MetricOverviewItem*>(item);
            name->setText(mi->name);
            metric1->setSymbol(mi->symbol);
        }
        break;

    case OverviewItemType::DONUT:
        {
            DonutOverviewItem *mi = dynamic_cast<DonutOverviewItem*>(item);
            name->setText(mi->name);
            metric1->setSymbol(mi->symbol);
            meta1->setMeta(mi->meta);
        }
        break;


    case OverviewItemType::TOPN:
        {
            TopNOverviewItem *mi = dynamic_cast<TopNOverviewItem*>(item);
            name->setText(mi->name);
            metric1->setSymbol(mi->symbol);
        }
        break;

    case OverviewItemType::META:
        {
            MetaOverviewItem *mi = dynamic_cast<MetaOverviewItem*>(item);
            name->setText(mi->name);
            meta1->setMeta(mi->symbol);
        }
        break;

    case OverviewItemType::ZONE:
        {
            ZoneOverviewItem *mi = dynamic_cast<ZoneOverviewItem*>(item);
            name->setText(mi->name);
            series1->setSeries(mi->series);
            cb1->setChecked(mi->polarized);
        }
        break;

    case OverviewItemType::INTERVAL:
    case OverviewItemType::ACTIVITIES:
        {
            IntervalOverviewItem *mi = dynamic_cast<IntervalOverviewItem*>(item);
            name->setText(mi->name);
            metric1->setSymbol(mi->xsymbol);
            metric2->setSymbol(mi->ysymbol);
            metric3->setSymbol(mi->zsymbol);
        }
        break;

    case OverviewItemType::ROUTE:
        {
            RouteOverviewItem *mi = dynamic_cast<RouteOverviewItem*>(item);
            name->setText(mi->name);
        }
        break;

    case OverviewItemType::PMC:
        {
            PMCOverviewItem *mi = dynamic_cast<PMCOverviewItem*>(item);
            metric1->setSymbol(mi->symbol);
        }
        break;

    case OverviewItemType::DATATABLE:
        {
            DataOverviewItem *mi = dynamic_cast<DataOverviewItem*>(item);
            name->setText(mi->name);
            editor->setText(mi->program);
        }
        break;

    case OverviewItemType::KPI:
        {
            KPIOverviewItem *mi = dynamic_cast<KPIOverviewItem*>(item);
            name->setText(mi->name);
            editor->setText(mi->program);
            double1->setValue(mi->start);
            double2->setValue(mi->stop);
            cb1->setChecked(mi->istime);
            string1->setText(mi->units);
        }
    }
    block = false;
}

void
OverviewItemConfig::dataChanged()
{
    // user edited or programmatically the data was changed
    // so lets update the item to reflect those changes
    // if they are valid. But block set when the widgets
    // are being initialised
    if (block) return;

    // get filter
    if (item->parent->scope & OverviewScope::TRENDS)  item->datafilter = filterEditor->filter();

    // set the widget values from the item
    switch(item->type) {
    case OverviewItemType::RPE:
        {
            RPEOverviewItem *mi = dynamic_cast<RPEOverviewItem*>(item);
            mi->name = name->text();
            mi->bgcolor = bgcolor->getColor().name();
        }
        break;

    case OverviewItemType::METRIC:
        {
            MetricOverviewItem *mi = dynamic_cast<MetricOverviewItem*>(item);
            mi->name = name->text();
            if (metric1->isValid()) {
                mi->symbol = metric1->rideMetric()->symbol();
                mi->units = metric1->rideMetric()->units(GlobalContext::context()->useMetricUnits);
            }
            mi->bgcolor = bgcolor->getColor().name();
        }
        break;

    case OverviewItemType::DONUT:
        {
            DonutOverviewItem *mi = dynamic_cast<DonutOverviewItem*>(item);
            mi->name = name->text();
            if (metric1->isValid())  mi->symbol = metric1->rideMetric()->symbol();
            if (meta1->isValid())  mi->meta = meta1->metaname();
            mi->bgcolor = bgcolor->getColor().name();
        }
        break;

    case OverviewItemType::TOPN:
        {
            TopNOverviewItem *mi = dynamic_cast<TopNOverviewItem*>(item);
            mi->name = name->text();
            if (metric1->isValid()) {
                mi->symbol = metric1->rideMetric()->symbol();
                mi->units = metric1->rideMetric()->units(GlobalContext::context()->useMetricUnits);
            }
            mi->bgcolor = bgcolor->getColor().name();
        }
        break;

    case OverviewItemType::META:
        {
            MetaOverviewItem *mi = dynamic_cast<MetaOverviewItem*>(item);
            mi->name = name->text();
            if (meta1->isValid()) mi->symbol = meta1->metaname();
            mi->bgcolor = bgcolor->getColor().name();
        }
        break;

    case OverviewItemType::ZONE:
        {
            ZoneOverviewItem *mi = dynamic_cast<ZoneOverviewItem*>(item);
            mi->name = name->text();
            if (series1->currentIndex() >= 0) mi->series = static_cast<RideFile::SeriesType>(series1->itemData(series1->currentIndex(), Qt::UserRole).toInt());
            mi->polarized = cb1->isChecked();
            mi->bgcolor = bgcolor->getColor().name();
        }
        break;

    case OverviewItemType::INTERVAL:
    case OverviewItemType::ACTIVITIES:
        {
            IntervalOverviewItem *mi = dynamic_cast<IntervalOverviewItem*>(item);
            mi->name = name->text();
            if (metric1->isValid()) mi->xsymbol = metric1->rideMetric()->symbol();
            if (metric2->isValid()) mi->ysymbol = metric2->rideMetric()->symbol();
            if (metric3->isValid()) mi->zsymbol = metric3->rideMetric()->symbol();
            mi->bgcolor = bgcolor->getColor().name();
        }
        break;

    case OverviewItemType::ROUTE:
        {
            RouteOverviewItem *mi = dynamic_cast<RouteOverviewItem*>(item);
            mi->name = name->text();
            mi->bgcolor = bgcolor->getColor().name();
        }
        break;

    case OverviewItemType::PMC:
        {
            PMCOverviewItem *mi = dynamic_cast<PMCOverviewItem*>(item);
            if (metric1->isValid()) mi->symbol = metric1->rideMetric()->symbol();
        }
        break;

    case OverviewItemType::DATATABLE:
        {
            DataOverviewItem *mi = dynamic_cast<DataOverviewItem*>(item);
            mi->name = name->text();
            mi->program = editor->toPlainText();
            mi->bgcolor = bgcolor->getColor().name();
        }
        break;

    case OverviewItemType::KPI:
        {
            KPIOverviewItem *mi = dynamic_cast<KPIOverviewItem*>(item);
            mi->name = name->text();
            mi->istime = cb1->isChecked();
            mi->units = string1->text();
            mi->program = editor->toPlainText();
            mi->start = double1->value();
            mi->stop = double2->value();
            mi->bgcolor = bgcolor->getColor().name();
        }
    }
}

//
// Below here are all the overviewitem viz
//
RPErating::RPErating(RPEOverviewItem *parent, QString name) : QGraphicsItem(NULL), parent(parent), name(name), hover(false)
{
    setGeometry(20,20,100,100);
    setZValue(11);
    setAcceptHoverEvents(true);
}

static QString FosterDesc[11]={
    QObject::tr("Rest"), // 0
    QObject::tr("Very, very easy"), // 1
    QObject::tr("Easy"), // 2
    QObject::tr("Moderate"), // 3
    QObject::tr("Somewhat hard"), // 4
    QObject::tr("Hard"), // 5
    QObject::tr("Hard+"), // 6
    QObject::tr("Very hard"), // 7
    QObject::tr("Very hard+"), // 8
    QObject::tr("Very hard++"), // 9
    QObject::tr("Maximum")// 10
};

static QColor FosterColors[11]={
    QColor(Qt::lightGray),// 0
    QColor(Qt::lightGray),// 1
    QColor(Qt::darkGreen),// 2
    QColor(Qt::darkGreen),// 3
    QColor(Qt::darkGreen),// 4
    QColor(Qt::darkYellow),// 5
    QColor(Qt::darkYellow),// 6
    QColor(Qt::darkYellow),// 7
    QColor(Qt::darkRed),// 8
    QColor(Qt::darkRed),// 9
    QColor(Qt::red),// 10
};

void
RPErating::setValue(QString value)
{
    // RPE values from other sources (e.g. TodaysPlan) are "double"
    this->value = value;
    int v = qRound(value.toDouble());
    if (v <0 || v>10) {
        color = GColor(CPLOTMARKER);
        description = QObject::tr("Invalid");
    } else {
        description = FosterDesc[v];
        color = FosterColors[v];
    }
}

QVariant RPErating::itemChange(GraphicsItemChange change, const QVariant &value)
{
     if (change == ItemPositionChange && parent->scene())  prepareGeometryChange();
     return QGraphicsItem::itemChange(change, value);
}

void
RPErating::setGeometry(double x, double y, double width, double height)
{
    geom = QRectF(x,y,width,height);

    // we need to go onto the scene !
    if (scene() == NULL && parent->scene()) parent->scene()->addItem(this);

    // set our geom
    prepareGeometryChange();
}

bool
RPErating::sceneEvent(QEvent *event)
{
    // skip whilst dragging and resizing
    if (parent->parent->state != ChartSpace::NONE) return false;

    if (event->type() == QEvent::GraphicsSceneHoverMove) {

        if (hover) {

            // set value based upon the location of the mouse
            QPoint vpos = parent->parent->view->mapFromGlobal(QCursor::pos());
            QPointF pos = parent->parent->view->mapToScene(vpos);
            QPointF cpos = pos - parent->geometry().topLeft() - geom.topLeft();

            // new value should
            double width = geom.width() / 13; // always a block each side for a margin

            double x = round((cpos.x() - width)/width);
            if (x >=0 && x<=10) {

                // set to the new value
                setValue(QString("%1").arg(x));
                parent->value = value;
                parent->update();

            }
        }

        // mouse moved so hover paint anyway
        update();

    } else if (hover && event->type() == QEvent::GraphicsSceneMousePress) {

        applyEdit();
        update();

    }  else if (event->type() == QEvent::GraphicsSceneHoverLeave) {

        cancelEdit();
        update();

    } else if (event->type() == QEvent::GraphicsSceneHoverEnter) {

        // remember what it was
        oldvalue = value;
        hover = true;
        update();
    }
    return false;
}

void
RPErating::cancelEdit()
{
    if (value != oldvalue || parent->value != oldvalue) {
        parent->value=oldvalue;
        value=oldvalue;
        parent->update();
        setValue(oldvalue);
    }
    hover = false;
    update();

}

void
RPErating::applyEdit()
{
    // update the item - if we have one
    RideItem *item = parent->parent->currentRideItem;

    // did it change?
    if (item && item->ride() && item->getText("RPE","") != value) {

        // change it -- this smells, since it should be abstracted in RideItem XXX
        item->ride()->setTag("RPE", value);
        item->notifyRideMetadataChanged();
        item->setDirty(true);

        // now oldvalue is value!
        oldvalue = value;
    }

    hover = false;
    update();
}

void
RPErating::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    painter->setPen(Qt::NoPen);

    // hover?
    if (hover) {
        QColor darkgray(120,120,120,120);
        painter->fillRect(QRectF(parent->x()+geom.x(), parent->y()+geom.y(), geom.width(),geom.height()), QBrush(darkgray));
    }

    painter->setPen(QPen(color));
    QFontMetrics tfm(parent->parent->titlefont);
    QRectF rect = tfm.boundingRect(description);
    painter->setFont(parent->parent->titlefont);
    painter->drawText(QPointF(parent->x()+geom.x()+((geometry().width() - rect.width()) / 2.0f),
                              parent->y()+geom.y()+geom.height()-ROWHEIGHT), description); // divided by 3 to account for "gap" at top of font


    // paint the blocks
    double width = geom.width() / 13; // always a block each side for a margin
    int i=0;
    for(; i<= qRound(value.toDouble()); i++) {

        // draw a rectangle with a 5px gap
        painter->setPen(Qt::NoPen);
        painter->fillRect(geom.x()+parent->x()+(width *(i+1)), parent->y()+geom.y()+ROWHEIGHT*1.5f, width-5, ROWHEIGHT*0.25f, QBrush(FosterColors[i]));
    }

    for(; i<= 10; i++) {

        // draw a rectangle with a 5px gap
        painter->setPen(Qt::NoPen);
        painter->fillRect(geom.x()+parent->x()+(width *(i+1)), parent->y()+geom.y()+ROWHEIGHT*1.5f, width-5, ROWHEIGHT*0.25f, QBrush(GColor(CCARDBACKGROUND).darker(200)));
    }
}

double BPointF::score(BPointF &other)
{
    // match score
    // 100 * n characters that match in label
    // +1000 if color same (class)
    // -(10 * sizediff) size important
    double score = 0;

    // must be the same class
    if (fill != other.fill) return 0;
    else score += 1000;

    // oh, this is a peach
    if (label == other.label) return 10000;

    for(int i=0; i<label.length() && i<other.label.length(); i++) {
        if (label[i] == other.label[i]) score += 100;
        else break;
    }

    // size?
    double diff = fabs(z-other.z);
    if (diff == 0) score += 1000;
    else score += (z/fabs(z - other.z));

    // for now ..
    return score;
}

BubbleViz::BubbleViz(IntervalOverviewItem *parent, QString name) : QGraphicsItem(NULL), parent(parent), name(name), hover(false), click(false)
{
    setGeometry(20,20,100,100);
    setZValue(11);
    setAcceptHoverEvents(true);

    clickthru = NULL;
    intervalsignal=false;
    group = new QSequentialAnimationGroup(this);

    QParallelAnimationGroup *par = new QParallelAnimationGroup(this);
    xaxisAnimation=new QPropertyAnimation(this, "xaxis");
    yaxisAnimation=new QPropertyAnimation(this, "yaxis");
    par->addAnimation(xaxisAnimation);
    par->addAnimation(yaxisAnimation);
    group->addAnimation(par);

    transitionAnimation=new QPropertyAnimation(this, "transition");
    group->addAnimation(transitionAnimation);

    minx=maxx=miny=maxy=-1; // initial values
}

BubbleViz::~BubbleViz()
{
    group->stop();
    delete group;
}

QVariant BubbleViz::itemChange(GraphicsItemChange change, const QVariant &value)
{
     if (change == ItemPositionChange && parent->scene())  prepareGeometryChange();
     return QGraphicsItem::itemChange(change, value);
}

void
BubbleViz::setGeometry(double x, double y, double width, double height)
{
    geom = QRectF(x,y,width,height);

    // we need to go onto the scene !
    if (scene() == NULL && parent->scene()) parent->scene()->addItem(this);

    // set our geom
    prepareGeometryChange();
}

bool
BubbleViz::sceneEvent(QEvent *event)
{
    // skip whilst dragging and resizing
    if (parent->parent->state != ChartSpace::NONE) return false;

    if (event->type() == QEvent::GraphicsSceneHoverMove) {

       // set value based upon the location of the mouse
       QPoint vpos = parent->parent->view->mapFromGlobal(QCursor::pos());
       QPointF pos = parent->parent->view->mapToScene(vpos);

        QRectF canvas= QRectF(parent->x()+geom.x(), parent->y()+geom.y(), geom.width(),geom.height());
        QRectF plotarea = QRectF(canvas.x() + ROWHEIGHT * 2 + 20, canvas.y()+ROWHEIGHT,
                             canvas.width() - ROWHEIGHT * 2 - 20 - ROWHEIGHT,
                             canvas.height() - ROWHEIGHT * 2 - 20 - ROWHEIGHT);
       if (plotarea.contains(pos)) {
            plotpos = QPointF(pos.x()-plotarea.x(), pos.y()-plotarea.y());
            hover=true;
            update();
       } else if (hover == true) {
            hover=false;
            update();
       }
    }

    if (event->type() == QEvent::GraphicsSceneMousePress) {
        click = true;
        clickthru=NULL;
        update();
    }

    if (event->type() == QEvent::GraphicsSceneMouseRelease) {
        if (clickthru) {
            parent->parent->context->notifyRideSelected(clickthru);
            clickthru = NULL;
        }
    }

    // hoversignal?
    if (intervalsignal && intervalhover != "" && intervalhover != lastintervalsignal) {
        lastintervalsignal = intervalhover;
        // lets signal that hover todo
        foreach(IntervalItem *it, const_cast<RideItem*>(parent->parent->context->currentRideItem())->intervals()) {
            if (it->name == intervalhover) {
                parent->parent->context->notifyIntervalHover(it);
                break;
            }
        }
    }

    return false;
}

class BubbleVizTuple {
public:
    double score;
    int newindex, oldindex;

};
bool scoresBiggerThan(const BubbleVizTuple i1, const BubbleVizTuple i2)
{
    return i1.score > i2.score;
}
void
BubbleViz::setPoints(QList<BPointF> p, double minx, double maxx, double miny, double maxy, bool animate)
{
    // initial conditions?
    if (this->miny == -1 && this->maxy == -1 && this->minx == -1 && this->maxx == -1)  {
        this->minx=minx;
        this->miny=miny;
        this->maxy=maxy;
        this->maxx=maxx;
    }

    xaxisAnimation->setStartValue(QPointF(this->minx,this->maxx));
    xaxisAnimation->setEndValue(QPointF(minx,maxx));
    yaxisAnimation->setStartValue(QPointF(this->miny,this->maxy));
    yaxisAnimation->setEndValue(QPointF(miny,maxy));
    xaxisAnimation->setEasingCurve(QEasingCurve::OutQuad);
    xaxisAnimation->setDuration(400);
    yaxisAnimation->setEasingCurve(QEasingCurve::OutQuad);
    yaxisAnimation->setDuration(400);
    transition = -1; // work on axis first

    oldpoints = this->points;
    oldmean = this->mean;

    double sum=0, count=0;
    this->points.clear();
    foreach(BPointF point, p) {

        if (point.x < minx || point.x > maxx || !std::isfinite(point.x) || std::isnan(point.x) || point.x == 0 ||
            point.y < miny || point.y > maxy || !std::isfinite(point.y) || std::isnan(point.y) || point.y == 0 ||
            point.z == 0 || !std::isfinite(point.z) || std::isnan(point.z)) continue;

        this->points << point;
        sum += point.z;
        count++;
    }
    mean = sum/count;

    // so now we need to setup a transition
    // we match the oldpoints to the new points by scoring
    QList<BPointF> matches;
    for(int i=0; i<points.count(); i++) matches << BPointF(); // fill with no matches

    QList<BubbleVizTuple> scores;
    QVector<bool> available(oldpoints.count());
    available.fill(true);

    // get all the scores
    for(int newindex =0; newindex < points.count(); newindex++) {
        for (int oldindex =0; oldindex < oldpoints.count(); oldindex++) {
            BubbleVizTuple add;
            add.newindex = newindex;
            add.oldindex = oldindex;
            add.score = points[newindex].score(oldpoints[oldindex]);
            if (add.score > 0) scores << add;
        }
    }

    // sort scores high to low
    std::sort(scores.begin(), scores.end(), scoresBiggerThan);

    // now assign - from best match to worst
    foreach(BubbleVizTuple score, scores){
        if (available[score.oldindex]) {
            available[score.oldindex]=false; // its now taken
            matches[score.newindex]=oldpoints[score.oldindex];
        }
    }

    // add non-matches to the end
    for(int i=0; i<available.count(); i++) {
        if (available[i]) {
            matches << oldpoints[i];
        }
    }
    oldpoints = matches;

    // stop any transition animation currently running
    group->stop();
    if (animate) {
        transitionAnimation->setStartValue(0);
        transitionAnimation->setEndValue(256);
        transitionAnimation->setEasingCurve(QEasingCurve::OutQuad);
        transitionAnimation->setDuration(400);
        group->start();
    } else {
        transition=256;
        update();
    }
}

static double pointDistance(QPointF a, QPointF b)
{
    double distance = sqrt(pow(b.x()-a.x(),2) + pow(b.y()-a.y(),2));
    return distance;
}

// just draw a rect for now
void
BubbleViz::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    // blank when no points
    if (points.count() == 0 || miny==maxy || minx==maxx) return;

    painter->setPen(GColor(CPLOTMARKER));

    // chart canvas
    QRectF canvas= QRectF(parent->x()+geom.x(), parent->y()+geom.y(), geom.width(),geom.height());
    //DIAG painter->drawRect(canvas);

    // plotting space
    QRectF plotarea = QRectF(canvas.x() + ROWHEIGHT * 2 + 20, canvas.y()+ROWHEIGHT,
                             canvas.width() - ROWHEIGHT * 2 - 20 - ROWHEIGHT,
                             canvas.height() - ROWHEIGHT * 2 - 20 - ROWHEIGHT);
    //DIAG painter->drawRect(plotarea);

    // clip to canvas -- draw points first so all axis etc are overlayed
    painter->save();
    painter->setClipRect(plotarea);

    // scale values to plot area
    double xratio = plotarea.width() / (maxx*1.03);
    double yratio = plotarea.height() / (maxy*1.03); // boundary space

    // old values when transitioning
    double oxratio = plotarea.width() / (maxx*1.03);
    double oyratio = plotarea.height() / (maxy*1.03); // boundary space

    // run through each point
    double area = 10000; // max size
    if (parent->parent->scope == OverviewScope::TRENDS) area /= 2; // smaller on trends so many to see

    // remember the one we are nearest
    BPointF nearest;
    double nearvalue = -1;

    // get xoff and yoff (we assume always the same for now)
    double xoff=0, yoff=0;
    if (points.count() > 0) {
        xoff = points[0].xoff;
        yoff = points[0].yoff;
    }

    if (transition >= 0) {

        int index=0;
        foreach(BPointF point, points) {

            if (point.x < minx || point.x > maxx ||
                point.y < miny || point.y > maxy ||
                !std::isfinite(point.z) || std::isnan(point.z)) {
                index++;
                continue;
            }

            // resize if transitioning
            QPointF center(plotarea.left() + (xratio * point.x), plotarea.bottom() - (yratio * point.y));
            int alpha = 200;
            if (parent->parent->scope == OverviewScope::TRENDS) alpha /= 2;

            double size = (point.z / mean) * area;
            if (size > area * 6) size=area*6;
            if (size < 600) size=600;

            if (transition < 256 && oldpoints.count()) {
                if (oldpoints[index].x != 0 || oldpoints[index].y != 0) {
                    // where it was
                    QPointF oldcenter = QPointF(plotarea.left() + (oxratio * oldpoints[index].x),
                                                plotarea.bottom() - (oyratio * oldpoints[index].y));

                    // transition to new point
                    center.setX(center.x() - (double(255-transition) * ((center.x()-oldcenter.x())/255.0f)));
                    center.setY(center.y() - (double(255-transition) * ((center.y()-oldcenter.y())/255.0f)));

                    // transition bubble size
                    double oldsize = (oldpoints[index].z / oldmean) * area;
                    if (oldsize > area * 6) oldsize=area*6;
                    if (oldsize < 600) oldsize=600;
                    size = size - (double(255-transition) * ((size-oldsize)/255.0f));

                } else {
                    // just make it appear
                    alpha = ((parent->parent->scope == OverviewScope::TRENDS ? 100 : 200.0f)/255.0f) * transition;
                }
            }

            // once transitioned clear them away
            if (transition == 256 && oldpoints.count()) oldpoints.clear();

            QColor color = point.fill;
            color.setAlpha(alpha);
            painter->setBrush(color);
            painter->setPen(QColor(150,150,150));

            double radius = sqrt(size/3.1415927f);
            painter->drawEllipse(center, radius, radius);

            // is the cursor hovering over me?
            double distance;
            if (transition == 256 && hover && (distance=pointDistance(center, plotarea.topLeft()+plotpos)) <= radius) {

                // is this the nearest ?
                if (nearvalue == -1 || distance < nearvalue) {
                    nearest = point;
                    nearvalue = distance;
                }

            }
            index++;
        }

        // if we're transitioning
        while (transition < 256 && index < oldpoints.count()) {
           QPointF oldcenter = QPointF(plotarea.left() + (oxratio * oldpoints[index].x),
                                       plotarea.bottom() - (oyratio * oldpoints[index].y));

            // fade out
            QColor color = oldpoints[index].fill;
            double alpha = ((parent->parent->scope == OverviewScope::TRENDS ? 100 : 200.0f)/255.0f) * transition;
            color.setAlpha(alpha);
            painter->setBrush(color);
            painter->setPen(Qt::NoPen);

            double size = (oldpoints[index].z/oldmean) * area;
            if (size > area * 6) size=area*6;
            if (size < 600) size=600;
            double radius = sqrt(size/3.1415927f);
            painter->drawEllipse(oldcenter, radius, radius);

            // hide the old ones
            index++;
        }

    } else {

        // when transition is -1 we are rescaling the axes first
        int index=0;
        foreach(BPointF point, oldpoints) {

            if (point.x < minx || point.x > maxx ||
                point.y < miny || point.y > maxy ||
                !std::isfinite(point.z) || std::isnan(point.z)) {
                index++;
                continue;
            }

            // resize if transitioning
            QPointF center(plotarea.left() + (xratio * point.x), plotarea.bottom() - (yratio * point.y));
            int alpha = 100;

            double size = (point.z / mean) * area;
            if (size > area * 6) size=area*6;
            if (size < 600) size=600;

            QColor color = point.fill;
            color.setAlpha(alpha);
            painter->setBrush(color);
            painter->setPen(QColor(150,150,150));

            double radius = sqrt(size/3.1415927f);
            painter->drawEllipse(center, radius, radius);

            index++;
        }

    }

    painter->setBrush(Qt::NoBrush);

    // clip to canvas
    painter->setClipRect(canvas);

    // x-axis labels
    QRectF xlabelspace = QRectF(plotarea.x(), plotarea.bottom() + 20, plotarea.width(), ROWHEIGHT);
    painter->setPen(Qt::red);
    //DIAG painter->drawRect(xlabelspace);

    // y-axis labels
    QRectF ylabelspace = QRectF(plotarea.x()-20-ROWHEIGHT, plotarea.y(), ROWHEIGHT, plotarea.height());
    painter->setPen(Qt::red);
    //DIAG painter->drawRect(ylabelspace);

    // y-axis title
    //DIAGQRectF ytitlespace = QRectF(plotarea.x()-20-(ROWHEIGHT*2), plotarea.y(), ROWHEIGHT, plotarea.height());
    //DIAGpainter->setPen(Qt::yellow);
    //DIAGpainter->drawRect(ytitlespace);

    painter->setPen(QColor(150,150,150));
    painter->setFont(parent->parent->smallfont);
    //XXX FIXME XXX painter->drawText(xtitlespace, xlabel, midcenter);

    // draw axis, from minx, to maxx (see tufte for 'range' axis on scatter plots
    QPen axisPen(QColor(150,150,150));
    axisPen.setWidth(5);
    painter->setPen(axisPen);

    // x-axis
    painter->drawLine(QPointF(plotarea.left() + (minx * xratio), plotarea.bottom()),
                      QPointF(plotarea.left() + (maxx * xratio), plotarea.bottom()));

    // x-axis range
    RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *m = factory.rideMetric(parent->xsymbol);
    QString smin, smax;
    if (m) {
        smin = m->toString(round(minx+xoff));
        smax = m->toString(round(maxx+xoff));
    } else {
        smin = QString("%1").arg(round(minx+xoff));
        smax = QString("%1").arg(round(maxx+xoff));
    }

    QFontMetrics sfm(parent->parent->smallfont);
    QRectF bminx = sfm.tightBoundingRect(smin);
    QRectF bmaxx = sfm.tightBoundingRect(smax);
    painter->drawText(xlabelspace.left() + (minx*xratio) - (bminx.width()/2),  xlabelspace.bottom(), smin);
    painter->drawText(xlabelspace.left() + (maxx*xratio) - (bmaxx.width()/2),  xlabelspace.bottom(), smax);

    // x-axis title - offset from minx
    QRectF xtitlespace = QRectF(plotarea.x() + (minx*xratio), xlabelspace.bottom(), plotarea.width() - (minx*xratio), ROWHEIGHT);
    painter->setPen(QColor(150,150,150));
    painter->setFont(parent->parent->smallfont);
    painter->drawText(xtitlespace, xlabel, Qt::AlignCenter|Qt::AlignVCenter);

    // draw minimum value
    painter->drawLine(QPointF(plotarea.left(), plotarea.bottom() - (miny*yratio)),
                      QPointF(plotarea.left(), plotarea.bottom() - (maxy*yratio)));
    // y-axis range
    QRectF bminy = sfm.tightBoundingRect(QString("%1").arg(round(miny+yoff)));
    QRectF bmaxy = sfm.tightBoundingRect(QString("%1").arg(round(maxy+yoff)));
    painter->drawText(ylabelspace.right() - bmaxy.width(),  ylabelspace.bottom()-(maxy*yratio) + (bmaxy.height()/2), QString("%1").arg(round(maxy+yoff)));
    painter->drawText(ylabelspace.right() - bminy.width(),  ylabelspace.bottom()-(miny*yratio) + (bminy.height()/2), QString("%1").arg(round(miny+yoff)));

    // hover point?
    painter->setPen(GColor(CPLOTMARKER));

    if (hover && nearvalue >= 0) {

        painter->setFont(parent->parent->titlefont);
        QFontMetrics tfm(parent->parent->titlefont);

        // where is it?
        QPointF center(plotarea.left() + (xratio * nearest.x), plotarea.bottom() - (yratio * nearest.y));

        // xlabel
        const RideMetric *m = factory.rideMetric(parent->xsymbol);
        QString xlab;
        if (m)  xlab = m->toString(nearest.x+xoff);
        else xlab = Utils::removeDP(QString("%1").arg(nearest.x+xoff,0,'f',parent->xdp));
        bminx = tfm.tightBoundingRect(QString("%1").arg(xlab));
        bminx.moveTo(center.x() - (bminx.width()/2),  xlabelspace.bottom()-bminx.height());
        painter->fillRect(bminx, QBrush(GColor(CCARDBACKGROUND))); // overwrite range labels
        painter->drawText(center.x() - (bminx.width()/2),  xlabelspace.bottom(), xlab);

        // ylabel
        m = factory.rideMetric(parent->ysymbol);
        QString ylab;
        if (m)  ylab = m->toString(nearest.y+yoff);
        else ylab = Utils::removeDP(QString("%1").arg(nearest.y+yoff,0,'f',parent->ydp));
        bminy = tfm.tightBoundingRect(QString("%1").arg(ylab));
        bminy.moveTo(ylabelspace.right() - bminy.width(),  center.y() - (bminy.height()/2));
        painter->fillRect(bminy, QBrush(GColor(CCARDBACKGROUND))); // overwrite range labels
        painter->drawText(ylabelspace.right() - bminy.width(),  center.y() + (bminy.height()/2), ylab);

        // plot marker
        QPen pen(Qt::NoPen);
        painter->setPen(pen);
        painter->setBrush(GColor(CPLOTMARKER));

        // draw  the one we are near with no alpha
        double size = (nearest.z/mean) * area;
        if (size > area * 6) size=area*6;
        if (size < 600) size=600;
        double radius = sqrt(size/3.1415927f) + 20;
        painter->drawEllipse(center, radius, radius);

        // clip to card, but happily write all over the title!
        painter->setClipping(false);

        // now put the label at the top of the canvas
        painter->setPen(QPen(GColor(CPLOTMARKER)));
        bminx = tfm.tightBoundingRect(nearest.label);
        painter->drawText(canvas.center().x()-(bminx.width()/2.0f),
                          canvas.top()+bminx.height()-10, nearest.label);
    }

    // we hovering...
    if (intervalsignal) intervalhover = nearest.label;

    if (click && nearvalue >= 0 && nearest.item != NULL) {
        clickthru = nearest.item;
    }
    click = false;

    painter->restore();
}

Sparkline::Sparkline(QGraphicsWidget *parent, QString name, bool bigdot)
    : QGraphicsItem(NULL), parent(parent), name(name), sparkdays(SPARKDAYS), bigdot(bigdot), fill(false)
{
    min = max = 0.0f;
    setGeometry(20,20,100,100);
    setZValue(11);
}

void
Sparkline::setRange(double min, double max)
{
    this->min = min;
    this->max = max;
}

void
Sparkline::setPoints(QList<QPointF>x)
{
    points = x;
}

QVariant Sparkline::itemChange(GraphicsItemChange change, const QVariant &value)
{
     if (change == ItemPositionChange && parent->scene())  prepareGeometryChange();
     return QGraphicsItem::itemChange(change, value);
}

void
Sparkline::setGeometry(double x, double y, double width, double height)
{
    geom = QRectF(x,y,width,height);

    // we need to go onto the scene !
    if (scene() == NULL && parent->scene()) parent->scene()->addItem(this);

    // set our geom
    prepareGeometryChange();
}

void
Sparkline::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    // if no points just leave blank
    if (points.isEmpty() || (max-min)==0) return;

    // so draw a line connecting the points
    double xfactor = (geom.width() - (ROWHEIGHT*6)) / sparkdays;
    double xoffset = boundingRect().left()+(ROWHEIGHT*3);
    double yfactor = (geom.height()-(ROWHEIGHT)) / (max-min);
    double bottom = boundingRect().bottom()-ROWHEIGHT/2;

    // draw a sparkline -- need more than 1 point !
    if (points.count() > 1) {


        QPainterPath path;
        path.moveTo((points[0].x()*xfactor)+xoffset, bottom-((points[0].y()-min)*yfactor));
        for(int i=1; i<points.count();i++) {
            path.lineTo((points[i].x()*xfactor)+xoffset, bottom-((points[i].y()-min)*yfactor));
        }

        if (fill) {
            QColor fillColor=GColor(CPLOTMARKER);
            fillColor.setAlpha(64);
            QPainterPath fillpath = path;
            fillpath.lineTo((points.last().x()*xfactor)+xoffset,bottom);
            fillpath.lineTo((points.first().x()*xfactor)+xoffset,bottom);
            fillpath.lineTo((points.first().x()*xfactor)+xoffset,bottom);
            fillpath.lineTo((points.first().x()*xfactor)+xoffset, bottom-((points.first().y()-min)*yfactor));
            painter->fillPath(fillpath, QBrush(fillColor));
        }

        // xaxis
        QPainterPath line;
        line.moveTo(xoffset, bottom);
        line.lineTo(xoffset+geom.width()-(ROWHEIGHT*6), bottom);
        QPen lpen(QColor(100,100,100,75));
        lpen.setWidth(4);
        painter->setPen(lpen);
        painter->drawPath(line);

        QPen pen(QColor(150,150,150));
        pen.setWidth(8);
        //pen.setStyle(Qt::DotLine);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->drawPath(path);

        if (bigdot) {
            // and the last one is a dot for this value
            double x = (points.first().x()*xfactor)+xoffset-25;
            double y = bottom-((points.first().y()-min)*yfactor)-25;
            if (std::isfinite(x) && std::isfinite(y)) {
                painter->setBrush(QBrush(GColor(CPLOTMARKER).darker(150)));
                painter->setPen(Qt::NoPen);
                painter->drawEllipse(QRectF(x, y, 50, 50));
            }
        }
    }
}

Routeline::Routeline(QGraphicsWidget *parent, QString name) : QGraphicsItem(NULL), parent(parent), name(name)
{
    setGeometry(20,20,100,100);
    setZValue(11);
    animator=new QPropertyAnimation(this, "transition");
}

Routeline::~Routeline()
{
    animator->stop();
    delete animator;
}

// Spherical pseudo-Mercator projection. Only needs to be applied to the
// latitude of a coordinate.
// See also https://wiki.openstreetmap.org/wiki/Mercator
static inline double mercator_projection(double lat) {
    double y = lat / 90 + 1;
    y = y * M_PI / 4;
    y = log(tan(y));
    return y * 180 / M_PI;
}

void
Routeline::setData(RideItem *item)
{
    // no data, no plot
    if (item == NULL || !item->ride() || item->ride()->areDataPresent()->lat == false) {
        path = QPainterPath();
        return;
    }
    oldpath = path;
    owidth = width;
    oheight = height;

    // step 1 normalise the points

    // set points as ratio from topleft corner
    // and also calculate aspect ratio - to ensure
    // values are mapped to maintain the ratio (!)

    //
    // Find the top left and bottom right extents
    // of the trace and calculate offset, factor
    // and ratios to apply to each data point
    //
    double minlat=999, minlon=999;
    double maxlat=-999, maxlon=-999;

    foreach(RideFilePoint *p, item->ride()->dataPoints()) {

        // ignore zero values and out of bounds
        if (p->lat == 0 || p->lon == 0 ||
            p->lon < -180 || p->lon > 180 ||
            p->lat < -90 || p->lat > 90) continue;

        double projected_lat = mercator_projection(p->lat);

        if (projected_lat > maxlat) maxlat=projected_lat;
        if (projected_lat < minlat) minlat=projected_lat;
        if (p->lon < minlon) minlon=p->lon;
        if (p->lon > maxlon) maxlon=p->lon;
    }

    // calculate aspect ratio
    path = QPainterPath();
    double xdiff = (maxlon - minlon);
    double ydiff = (maxlat - minlat);
    double aspectratio = ydiff/xdiff;
    width = geom.width();

    // create a painterpath that uses a 1x1 aspect ratio
    // based upon the GPS co-ords
    int div = item->ride()->dataPoints().count() / ROUTEPOINTS;
    int count=0;
    height = geom.width() * aspectratio;
    int lines=0;
    foreach(RideFilePoint *p, item->ride()->dataPoints()){

        // ignore zero values and out of bounds
        if (p->lat == 0 || p->lon == 0 ||
            p->lon < -180 || p->lon > 180 ||
            p->lat < -90 || p->lat > 90) continue;

        // filter out most of the points so we end up with ROUTEPOINTS points
        if (--count < 0) { // first

            //path.moveTo(xoff+(geom.width() / (xdiff / (p->lon - minlon))),
            //            yoff+(geom.height()-(geom.height() / (ydiff / (p->lat - minlat)))));

            path.moveTo((geom.width() / (xdiff / (p->lon - minlon))),
                        (height-(height / (ydiff / (mercator_projection(p->lat) - minlat)))));
            count=div;

        } else if (count == 0) {

            //path.lineTo(xoff+(geom.width() / (xdiff / (p->lon - minlon))),
            //            yoff+(geom.height()-(geom.height() / (ydiff / (p->lat - minlat)))));
            path.lineTo((geom.width() / (xdiff / (p->lon - minlon))),
                        (height-(height / (ydiff / (mercator_projection(p->lat) - minlat)))));
            count=div;
            lines++;

        }
    }

    // if we have a transition
    animator->stop();
    if (oldpath.elementCount()) {
        animator->setStartValue(0);
        animator->setEndValue(256);
        animator->setEasingCurve(QEasingCurve::OutQuad);
        animator->setDuration(1000);
        animator->start();
    } else {
        transition = 256;
    }
}

QVariant Routeline::itemChange(GraphicsItemChange change, const QVariant &value)
{
     if (change == ItemPositionChange && parent->scene())  prepareGeometryChange();
     return QGraphicsItem::itemChange(change, value);
}

void
Routeline::setGeometry(double x, double y, double width, double height)
{
    geom = QRectF(x,y,width,height);

    // we need to go onto the scene !
    if (scene() == NULL && parent->scene()) parent->scene()->addItem(this);

    // set our geom
    prepareGeometryChange();
}

void
Routeline::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    painter->save();

    QPen pen(QColor(150,150,150));
    painter->setPen(pen);

    // draw the route, but scale it to fit what we have
    double scale = geom.width() / width;
    double oscale = geom.width() / width;
    if (height * scale > geom.height())  scale = geom.height() / height;
    if (oheight * oscale > geom.height())  oscale = geom.height() / oheight;

    // set clipping before we translate!
    painter->setClipRect(parent->x()+geom.x(), parent->y()+geom.y(), geom.width(), geom.height());

    // and center it too
    double midx=scale*width/2;
    double midy=scale*height/2;
    double omidx=oscale*owidth/2;
    double omidy=oscale*oheight/2;
    QPointF translate(boundingRect().x() + ((geom.width()/2)-midx),
                               boundingRect().y()+((geom.height()/2)-midy));

    QPointF otranslate(boundingRect().x() + ((geom.width()/2)-omidx),
                       boundingRect().y()+((geom.height()/2)-omidy));
    painter->translate(translate);

    // set painter scale - and keep original aspect ratio
    painter->scale(scale,scale);
    pen.setWidth(20.0f);
    painter->setPen(pen);


    // silly little animated morph from old to new
    if(transition < 256) {
        // transition!
        QPainterPath tpath;
        for(int i=0; i<path.elementCount(); i++) {

            // get co-ords - use last over and over if different sizes
            int n=0;
            if (i < oldpath.elementCount()) n=i;
            else n = oldpath.elementCount()-1;

            double x1=((oldpath.elementAt(n).x - translate.x() + otranslate.x()) / scale) * oscale;
            double y1=((oldpath.elementAt(n).y - translate.y() + otranslate.y()) / scale) * oscale;
            double x2=path.elementAt(i).x;
            double y2=path.elementAt(i).y;

            if (!i) tpath.moveTo(x1 + ((x2-x1)/255.0f) * double(transition), y1 + ((y2-y1)/255.0f) * double(transition));
            else tpath.lineTo(x1 + ((x2-x1)/255.0f) * double(transition), y1 + ((y2-y1)/255.0f) * double(transition));
        }
        painter->drawPath(tpath);
    } else {
        painter->drawPath(path);
    }
    painter->restore();
    return;
}

ProgressBar::ProgressBar(QGraphicsWidget *parent, double start, double stop, double value)
    : QGraphicsItem(NULL), parent(parent), start(start), stop(stop), value(value)
{
    setGeometry(20,20,100,100);
    setZValue(11);
    animator=new QPropertyAnimation(this, "value");
}

ProgressBar::~ProgressBar()
{
    animator->stop();
    delete animator;
}

void
ProgressBar::setValue(double start, double stop, double newvalue)
{
    this->start=start;
    this->stop = stop;

    animator->stop();
    animator->setStartValue(this->value);
    animator->setEndValue(newvalue);
    animator->setEasingCurve(QEasingCurve::OutQuad);
    animator->setDuration(1000);
    animator->start();
}

QVariant ProgressBar::itemChange(GraphicsItemChange change, const QVariant &value)
{
     if (change == ItemPositionChange && parent->scene())  prepareGeometryChange();
     return QGraphicsItem::itemChange(change, value);
}

void
ProgressBar::setGeometry(double x, double y, double width, double height)
{
    geom = QRectF(x,y,width,height);

    // we need to go onto the scene !
    if (scene() == NULL && parent->scene()) parent->scene()->addItem(this);

    // set our geom
    prepareGeometryChange();
}

void
ProgressBar::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{

    QRectF box(boundingRect().left() + (ROWHEIGHT*2), boundingRect().top() + ROWHEIGHT, geom.width()-(ROWHEIGHT*4), ROWHEIGHT/3.0);
    painter->fillRect(box, QBrush(QColor(100,100,100,100)));

    // width of bar
    double factor = (value-start) / (stop-start);
    QString percent = Utils::removeDP(QString("%1").arg(factor * 100.0));

    if (factor > 1) factor = 1;
    if (factor < 0) factor = 0;

    QRectF bar(box.left(), box.top(), box.width() * factor, ROWHEIGHT/3.0);
    painter->fillRect(bar, QBrush(GColor(CPLOTMARKER)));

}

Button::Button(QGraphicsItem*parent, QString text) : QGraphicsItem(parent), text(text), state(None)
{
    // not much really
    setZValue(11);
    setAcceptHoverEvents(true);
}

void
Button::setGeometry(double x, double y, double width, double height)
{
    geom = QRectF(x,y,width, height);
}

void
Button::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    static const int gl_border = 2;
    static const int gl_radius = 24;

    // anti aliasing please- none of that sketchy lines
    painter->setRenderHint(QPainter::Antialiasing);

    // button background
    QColor pc = GCColor::invertColor(GColor(CCARDBACKGROUND));
    pc.setAlpha(64);
    QPen line(pc,gl_border, Qt::SolidLine);
    line.setJoinStyle(Qt::RoundJoin);
    painter->setPen(line);
    QPointF pos=mapToParent(geom.x(), geom.y());
    if (isUnderMouse()) {
        QColor hover=GColor(CPLOTMARKER);
        if (state==Clicked) hover.setAlpha(200);
        else hover.setAlpha(100);
        painter->setBrush(QBrush(hover));
    } else painter->setBrush(QBrush(GColor(CCARDBACKGROUND)));
    painter->drawRoundedRect(pos.x()+gl_border, pos.y()+gl_border, geom.width()-(gl_border*2), geom.height()-(gl_border*2), gl_radius, gl_radius);

    // text using large font clipped
    if (isUnderMouse()) {
        QColor tc = GCColor::invertColor(CPLOTMARKER);
        tc.setAlpha(200);
        painter->setPen(tc);
    } else {
        QColor tc = GCColor::invertColor(GColor(CCARDBACKGROUND));
        tc.setAlpha(200);
        painter->setPen(tc);
    }
    painter->setFont(font);
    painter->drawText(geom, text, Qt::AlignHCenter | Qt::AlignVCenter);
}

bool
Button::sceneEvent(QEvent *event)
{

    if (event->type() == QEvent::GraphicsSceneHoverMove) {

        // mouse moved so hover paint anyway
        update();

    }  else if (event->type() == QEvent::GraphicsSceneHoverLeave) {

        update();

    } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {


        if (isUnderMouse() && state == Clicked) {
            state = None;
            update();
            QApplication::processEvents();
            emit clicked();
        } else {
            state = None;
            update();
            QApplication::processEvents();
        }


    } else if (event->type() == QEvent::GraphicsSceneMousePress) {

        if (isUnderMouse()) state = Clicked;
        update();

    } else if (event->type() == QEvent::GraphicsSceneHoverEnter) {

        update();
    }
    return false;
}

VScrollBar::VScrollBar(QGraphicsWidget *parent, ChartSpace *space) : QGraphicsItem(parent), parent(parent), space(space), height(0)
{
    setZValue(11); // slightly higher than host

    // hovering and dragging state set to initial conditions
    origin = QPointF(0,0);
    barhover=hover=false;
    obarpos=barpos=0;
    state=NONE;

    // we need to watch events...
    setAcceptHoverEvents(true);
}

double
VScrollBar::pos() const
{
    if (geom.height() == 0) return 0;

    // return pos as point in scrollarea thats at the top
    return height * (barpos / geom.height());
}
void
VScrollBar::setGeometry(double x, double y, double width, double height)
{
    geom = QRectF(x,y,width,height);
    barpos = 0;
}

void
VScrollBar::setAreaHeight(double n)
{
    height = n;
    barpos = 0;
}

void
VScrollBar::setPos(double x)
{
    // xxx todo
}

void
VScrollBar::movePos(int x)
{
    // just normalise to plus or minus but not natural scrolling
    // which is how the chart space works too
    x = x < 0 ? +1 : -1;

    // move at least .8 of a page down
    double barheight = geom.height() * (geom.height() / height);
    barpos += barheight * 0.8 * x;
    if (barpos < 0) barpos = 0;
    if (barpos + barheight > geom.height()) barpos = geom.height() - barheight;

    parent->update();
    update();
}

// the usual
void
VScrollBar::paint(QPainter*painter, const QStyleOptionGraphicsItem *, QWidget*)
{
    if (isVisible() && geom.height() && geom.height() < height) {

        canscroll=true;
        double barheight = geom.height() * (geom.height() / height);
        QColor barcolor(127,127,127,64);
        if (state == DRAG) {
            barcolor = GColor(CPLOTMARKER);
        } else if (hover) {
            if (barhover) barcolor = QColor(127,127,127,255);
            else barcolor = QColor(127,127,127,127);
        }

        painter->setBrush(barcolor);
        painter->setPen(Qt::NoPen);
        painter->drawRoundedRect(QRectF(geom.x(), geom.y()+barpos, geom.width(), barheight), geom.width()/2, geom.width()/2);
    } else {
        canscroll=false;
    }
}

// spotting mouse events hover, click move and wheel (but only in small area of scrollbar)
bool
VScrollBar::sceneEvent(QEvent *event)
{
    // we are not needed as scrollable area fits anyway
    if (height == 0 || geom.height() >= height) return false;

    // set value based upon the location of the mouse
    QPoint vpos = space->mapFromGlobal(QCursor::pos());
    QPointF spos = space->view->mapToScene(vpos);
    QPointF cpos = spos - parent->geometry().topLeft();

    // remember what it was....
    bool oldhover = hover;
    bool oldbarhover = barhover;

    // scrollbar space
    if (geom.contains(cpos)) hover = true;
    else hover=false;

    // the actual bar
    double barheight = geom.height() * (geom.height() / height);
    QRectF barrect = QRectF(geom.x(), geom.y()+barpos, geom.width(), barheight);
    if (barrect.contains(cpos)) barhover = true;
    else barhover = false;

    // plain old mouse move
    if (oldhover != hover || oldbarhover != barhover) update();

    if (event->type() == QEvent::GraphicsSceneMouseMove && state == DRAG) {

        // mouse moved so hover paint anyway
        barpos = obarpos + cpos.y() - origin.y();
        if (barpos < 0) barpos = 0;
        if (barpos + barheight > geom.height()) barpos = geom.height() - barheight;
        parent->update();
        update();

        return true;

    } else if (barhover && event->type() == QEvent::GraphicsSceneMousePress) {

        state = DRAG;
        origin = cpos;
        obarpos = barpos;
        update();

        return true;

    } else if (event->type() == QEvent::GraphicsSceneMouseRelease) {

        // remember what it was
        state = NONE;
        update();

        return true;
    }
    return false;
}
