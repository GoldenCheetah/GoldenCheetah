/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#include "Overview.h"
#include "ChartSpace.h"
#include "OverviewItems.h"
#include "UserChartOverviewItem.h"
#include "AddChartWizard.h"
#include "Utils.h"
#include "HelpWhatsThis.h"

static QIcon grayConfig, whiteConfig, accentConfig;

OverviewWindow::OverviewWindow(Context *context, int scope) : GcChartWindow(context), context(context), configured(false), scope(scope)
{
    setContentsMargins(0,0,0,0);
    setProperty("color", GColor(COVERVIEWBACKGROUND));
    setShowTitle(false);

    // actions...
    QAction *addTile= new QAction(tr("Add Tile..."));
    addAction(addTile);
    setControls(NULL);

    QHBoxLayout *main = new QHBoxLayout;
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);

    space = new ChartSpace(context, scope, this);
    main->addWidget(space);

    HelpWhatsThis *help = new HelpWhatsThis(space);
    if (scope & OverviewScope::ANALYSIS) space->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Overview));
    else space->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Chart_Overview));

    // all the widgets
    setChartLayout(main);

    // tell space when a ride is selected
    if (scope & OverviewScope::ANALYSIS) {
        connect(this, SIGNAL(rideItemChanged(RideItem*)), space, SLOT(rideSelected(RideItem*)));
    }

    if (scope & OverviewScope::TRENDS) {
        connect(this, SIGNAL(dateRangeChanged(DateRange)), space, SLOT(dateRangeChanged(DateRange)));
        connect(context, SIGNAL(filterChanged()), space, SLOT(filterChanged()));
        connect(context, SIGNAL(homeFilterChanged()), space, SLOT(filterChanged()));
        connect(this, SIGNAL(perspectiveFilterChanged(QString)), space, SLOT(filterChanged()));
    }
    connect(this, SIGNAL(perspectiveChanged(Perspective*)), space, SLOT(refresh()));
    connect(context, SIGNAL(refreshEnd()), space, SLOT(refresh()));
    connect(context, SIGNAL(estimatesRefreshed()), space, SLOT(refresh()));

    // menu items
    connect(addTile, SIGNAL(triggered(bool)), this, SLOT(addTile()));
    connect(space, SIGNAL(itemConfigRequested(ChartSpaceItem*)), this, SLOT(configItem(ChartSpaceItem*)));
}

void
OverviewWindow::addTile()
{
    ChartSpaceItem *added = NULL; // tell us what you added...

    AddChartWizard *p = new AddChartWizard(context, space, scope, added);
    p->exec(); // no mem leak delete on close dialog

    // set ride / date range if we added one.....
    if (added) {

        // update after config changed
        if (added->parent->scope & OverviewScope::ANALYSIS && added->parent->currentRideItem) added->setData(added->parent->currentRideItem);
        if (added->parent->scope & OverviewScope::TRENDS ) added->setDateRange(added->parent->currentDateRange);

    }
}

void
OverviewWindow::configItem(ChartSpaceItem *item)
{
    OverviewConfigDialog *p = new OverviewConfigDialog(item);
    p->exec(); // no mem leak as delete on close
}

QString
OverviewWindow::getConfiguration() const
{
    // return a JSON snippet to represent the entire config
    QString config;

    // setup
    config = "{\n  \"version\":\"2.0\",\n";
    config += "  \"CHARTS\":[\n";

    // do cards
    foreach(ChartSpaceItem *item, space->allItems()) {

        // basic stuff first - name, type etc
        config += "    { ";
        config += "\"type\":" + QString("%1").arg(static_cast<int>(item->type)) + ",";
        config += "\"span\":" + QString("%1").arg(item->span) + ",";
        config += "\"deep\":" + QString("%1").arg(item->deep) + ",";
        config += "\"column\":" + QString("%1").arg(item->column) + ",";
        config += "\"order\":" + QString("%1").arg(item->order) + ",";

        // now the actual card settings
        switch(item->type) {
        case OverviewItemType::RPE:
            {
                //UNUSED RPEOverviewItem *rpe = reinterpret_cast<RPEOverviewItem*>(item);
            }
            break;

        case OverviewItemType::DONUT:
            {
                DonutOverviewItem *donut = reinterpret_cast<DonutOverviewItem*>(item);
                config += "\"symbol\":\"" + QString("%1").arg(donut->symbol) + "\",";
                config += "\"meta\":\"" + QString("%1").arg(donut->meta) + "\",";
            }
            break;

        case OverviewItemType::TOPN:
        case OverviewItemType::METRIC:
            {
                MetricOverviewItem *metric = reinterpret_cast<MetricOverviewItem*>(item);
                config += "\"symbol\":\"" + QString("%1").arg(metric->symbol) + "\",";
            }
            break;
        case OverviewItemType::META:
            {
                MetaOverviewItem *meta = reinterpret_cast<MetaOverviewItem*>(item);
                config += "\"symbol\":\"" + QString("%1").arg(meta->symbol) + "\",";
            }
            break;
        case OverviewItemType::PMC:
            {
                PMCOverviewItem *pmc = reinterpret_cast<PMCOverviewItem*>(item);
                config += "\"symbol\":\"" + QString("%1").arg(pmc->symbol) + "\",";
            }
            break;
        case OverviewItemType::ROUTE:
            {
                // UNUSED RouteOverviewItem *route = reinterpret_cast<RouteOverviewItem*>(item);
            }
            break;
        case OverviewItemType::INTERVAL:
        case OverviewItemType::ACTIVITIES:
            {
                IntervalOverviewItem *interval = reinterpret_cast<IntervalOverviewItem*>(item);
                config += "\"xsymbol\":\"" + QString("%1").arg(interval->xsymbol) + "\",";
                config += "\"ysymbol\":\"" + QString("%1").arg(interval->ysymbol) + "\",";
                config += "\"zsymbol\":\"" + QString("%1").arg(interval->zsymbol) + "\"" + ",";
            }
            break;
        case OverviewItemType::ZONE:
            {
                ZoneOverviewItem *zone = reinterpret_cast<ZoneOverviewItem*>(item);
                config += "\"series\":" + QString("%1").arg(static_cast<int>(zone->series)) + ",";
                config += "\"polarized\":" + QString("%1").arg(zone->polarized) + ",";
            }
            break;
        case OverviewItemType::DATATABLE:
            {
                DataOverviewItem *data = reinterpret_cast<DataOverviewItem*>(item);
                config += "\"program\":\"" + QString("%1").arg(Utils::jsonprotect(data->program)) + "\",";
            }
            break;
        case OverviewItemType::KPI:
            {
                KPIOverviewItem *kpi = reinterpret_cast<KPIOverviewItem*>(item);
                config += "\"program\":\"" + QString("%1").arg(Utils::jsonprotect(kpi->program)) + "\",";
                config += "\"units\":\"" + QString("%1").arg(kpi->units) + "\",";
                config += "\"istime\":" + QString("%1").arg(kpi->istime) + ",";
                config += "\"start\":" + QString("%1").arg(kpi->start) + ",";
                config += "\"stop\":" + QString("%1").arg(kpi->stop) + ",";
            }
            break;
        case OverviewItemType::USERCHART:
            {
                UserChartOverviewItem *uc = reinterpret_cast<UserChartOverviewItem*>(item);
                config += "\"settings\":\"" + QString("%1").arg(Utils::jsonprotect(uc->getConfig())) + "\",";
            }
            break;
        }

        config += "\"datafilter\":\"" + Utils::jsonprotect(item->datafilter) + "\",";
        config += "\"name\":\"" + item->name + "\"";

        config += " }";

        if (space->allItems().last() != item) config += ",";

        config += "\n";
    }

    config += "  ]\n}\n";

    return config;
}

void
OverviewWindow::setConfiguration(QString config)
{
    // XXX hack because we're not in the default layout and don't want to
    // XXX this is just to handle setup for the very first time its run !
    if (configured == true) return;
    configured = true;

    // DEFAULT CONFIG (FOR NOW WHEN NOT IN THE DEFAULT LAYOUT)
    //
    // default column widths - max 10 columns;
    // note the sizing is such that each card is the equivalent of a full screen
    // so we can embed charts etc without compromising what they can display

 defaultsetup: // I know, but its easier than lots of nested if clauses above

    if (config == "") {

        if (scope == OverviewScope::ANALYSIS) {

            // column 0
            ChartSpaceItem *add;
            add = new PMCOverviewItem(space, "coggan_tss");
            space->addItem(1,0,1,9, add);

            add = new MetaOverviewItem(space, tr("Sport"), "Sport");
            space->addItem(2,0,1,5, add);

            add = new MetaOverviewItem(space, tr("Workout Code"), "Workout Code");
            space->addItem(3,0,1,5, add);

            add = new MetricOverviewItem(space, tr("Duration"), "workout_time");
            space->addItem(4,0,1,9, add);

            add = new MetaOverviewItem(space, tr("Notes"), "Notes");
            space->addItem(5,0,1,13, add);

            // column 1
            add = new MetricOverviewItem(space, tr("HRV rMSSD"), "rMSSD");
            space->addItem(1,1,1,9, add);

            add = new MetricOverviewItem(space, tr("Heartrate"), "average_hr");
            space->addItem(2,1,1,5, add);

            add = new ZoneOverviewItem(space, tr("Heartrate Zones"), RideFile::hr, false);
            space->addItem(3,1,1,11, add);

            add = new MetricOverviewItem(space, tr("Climbing"), "elevation_gain");
            space->addItem(4,1,1,5, add);

            add = new MetricOverviewItem(space, tr("Cadence"), "average_cad");
            space->addItem(5,1,1,5, add);

            add = new MetricOverviewItem(space, tr("Work"), "total_work");
            space->addItem(6,1,1,5, add);

            // column 2
            add = new RPEOverviewItem(space, tr("RPE"));
            space->addItem(1,2,1,9, add);

            add = new MetricOverviewItem(space, tr("Stress"), "coggan_tss");
            space->addItem(2,2,1,5, add);

            add = new ZoneOverviewItem(space, tr("Fatigue Zones"), RideFile::wbal, false);
            space->addItem(3,2,1,11, add);

            add = new IntervalOverviewItem(space, tr("Intervals"), "elapsed_time", "average_power", "workout_time");
            space->addItem(4,2,1,17, add);

            // column 3
            add = new MetricOverviewItem(space, tr("Power"), "average_power");
            space->addItem(1,3,1,9, add);

            add = new MetricOverviewItem(space, tr("IsoPower"), "coggan_np");
            space->addItem(2,3,1,5, add);

            add = new ZoneOverviewItem(space, tr("Power Zones"), RideFile::watts, false);
            space->addItem(3,3,1,11, add);

            add = new MetricOverviewItem(space, tr("Peak Power Index"), "peak_power_index");
            space->addItem(4,3,1,8, add);

            add = new MetricOverviewItem(space, tr("Variability"), "coggam_variability_index");
            space->addItem(5,3,1,8, add);

            // column 4
            add = new MetricOverviewItem(space, tr("Distance"), "total_distance");
            space->addItem(1,4,1,9, add);

            add = new MetricOverviewItem(space, tr("Speed"), "average_speed");
            space->addItem(2,4,1,5, add);

            add = new ZoneOverviewItem(space, tr("Pace Zones"), RideFile::kph, false);
            space->addItem(3,4,1,11, add);

            add = new RouteOverviewItem(space, tr("Route"));
            space->addItem(4,4,1,17, add);

        }

        if (scope == OverviewScope::TRENDS) {

            ChartSpaceItem *add;

            // column 0
            add = new KPIOverviewItem(space, tr("Distance"), 0, 10000, "{ round(sum(metrics(Distance))); }", "km", false);
            space->addItem(0,0,1,8, add);

            add = new TopNOverviewItem(space, tr("Going Long"), "total_distance");
            space->addItem(1,0,1,25, add);

            add = new KPIOverviewItem(space, tr("Weekly Hours"), 0, 15*3600, "{ weeks <- (daterange(stop)-daterange(start))/7; sum(metrics(Duration))/weeks; }", tr("hh:mm:ss"), true);
            space->addItem(2,0,1,7, add);

            // column 1
            add = new KPIOverviewItem(space, tr("Peak Power Index"), 0, 150, "{ round(sort(descend, metrics(Power_Index))[0]); }", "%", false);
            space->addItem(0,1,1,8, add);

            add = new MetricOverviewItem(space, tr("Max Power"), "max_power");
            space->addItem(1,1,1,7, add);

            add = new MetricOverviewItem(space, tr("Average Power"), "average_power");
            space->addItem(2,1,1,7, add);

            add = new ZoneOverviewItem(space, tr("Power Zones"), RideFile::watts, false);
            space->addItem(3,1,1,9, add);

            add = new MetricOverviewItem(space, tr("Total TSS"), "coggan_tss");
            space->addItem(4,1,1,7, add);

            // column 2
            add = new KPIOverviewItem(space, tr("Total Hours"), 0, 0, "{ sum(metrics(Duration)); }", "hh:mm:ss", true);
            space->addItem(0,2,1,8, add);

            add = new TopNOverviewItem(space, tr("Going Hard"), "skiba_wprime_exp");
            space->addItem(1,2,1,25, add);

            add = new MetricOverviewItem(space, tr("Total W' Work"), "skiba_wprime_exp");
            space->addItem(2,2,1,7, add);

            // column 3
            add = new KPIOverviewItem(space, tr("W' Ratio"), 0, 100, "{ round((sum(metrics(W'_Work)) / sum(metrics(Work))) * 100); }", "%", false);
            space->addItem(0,3,1,8, add);

            add = new KPIOverviewItem(space, tr("Peak CP Estimate "), 0, 360, "{ round(max(estimates(cp3,cp))); }", "watts", false);
            space->addItem(1,3,1,7, add);

            add = new KPIOverviewItem(space, tr("Peak W' Estimate "), 0, 25, "{ round(max(estimates(cp3,w')/1000)*10)/10; }", "kJ", false);
            space->addItem(2,3,1,7, add);


            add = new ZoneOverviewItem(space, tr("Fatigue Zones"), RideFile::wbal, false);
            space->addItem(3,3,1,9, add);

            add = new MetricOverviewItem(space, tr("Total Work"), "total_work");
            space->addItem(4,3,1,7, add);

            // column 4
            add = new MetricOverviewItem(space, tr("Intensity Factor"), "coggan_if");
            space->addItem(0,4,1,8, add);

            add = new TopNOverviewItem(space, tr("Going Deep"), "skiba_wprime_low");
            space->addItem(1,4,1,25, add);

            add = new KPIOverviewItem(space, tr("IF > 0.85"), 0, 0, "{ count(metrics(IF)[x>0.85]); }", "activities", false);
            space->addItem(2,4,1,7, add);

        }

    } else {

        //
        // But by default we parse and apply (dropping back to default setup on error)
        //
        // parse
        QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8());
        if (doc.isEmpty() || doc.isNull()) {
            config="";
            goto defaultsetup;
        }

        // parsed so lets work through it and setup the overview
        QJsonObject root = doc.object();

        // check version
        QString version = root["version"].toString();
        if (version != "2.0") {
            config="";
            goto defaultsetup;
        }

        // cards
        QJsonArray CHARTS = root["CHARTS"].toArray();
        foreach(const QJsonValue val, CHARTS) {

            // convert so we can inspect
            QJsonObject obj = val.toObject();

            // get the basics
            QString name = obj["name"].toString();
            QString datafilter = Utils::jsonunprotect(obj["datafilter"].toString());
            int column = obj["column"].toInt();
            int order = obj["order"].toInt();
            int span = obj.contains("span") ? obj["span"].toInt() : 1;
            int deep = obj["deep"].toInt();
            int type = obj["type"].toInt();


            // lets create the cards
            ChartSpaceItem *add=NULL;
            switch(type) {

            case OverviewItemType::RPE :
                {
                    add = new RPEOverviewItem(space, name);
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;

            case OverviewItemType::TOPN :
                {
                    QString symbol=obj["symbol"].toString();
                    add = new TopNOverviewItem(space, name,symbol);
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;

            case OverviewItemType::DONUT :
                {
                    QString symbol=obj["symbol"].toString();
                    QString meta=obj["meta"].toString();
                    add = new DonutOverviewItem(space, name,symbol,meta);
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;

            case OverviewItemType::METRIC :
                {
                    QString symbol=obj["symbol"].toString();
                    add = new MetricOverviewItem(space, name,symbol);
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;

            case OverviewItemType::META :
                {
                    QString symbol=obj["symbol"].toString();
                    add = new MetaOverviewItem(space, name,symbol);
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;

            case OverviewItemType::PMC :
                {
                    QString symbol=obj["symbol"].toString();
                    add = new PMCOverviewItem(space, symbol); // doesn't have a title
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;

            case OverviewItemType::ZONE :
                {
                    RideFile::SeriesType series = static_cast<RideFile::SeriesType>(obj["series"].toInt());
                    bool polarized = obj["polarized"].toInt();
                    add = new ZoneOverviewItem(space, name, series, polarized);
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);

                }
                break;

            case OverviewItemType::ROUTE :
                {
                    add = new RouteOverviewItem(space, name); // doesn't have a title
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;

            case OverviewItemType::INTERVAL :
            case OverviewItemType::ACTIVITIES:
                {
                    QString xsymbol=obj["xsymbol"].toString();
                    QString ysymbol=obj["ysymbol"].toString();
                    QString zsymbol=obj["zsymbol"].toString();

                    add = new IntervalOverviewItem(space, name, xsymbol, ysymbol, zsymbol); // doesn't have a title
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;

            case OverviewItemType::DATATABLE:
                {
                    QString program=Utils::jsonunprotect(obj["program"].toString());
                    add = new DataOverviewItem(space, name, program);
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;

            case OverviewItemType::KPI :
                {
                    QString program=Utils::jsonunprotect(obj["program"].toString());
                    double start=obj["start"].toDouble();
                    double stop =obj["stop"].toDouble();
                    QString units =obj["units"].toString();
                    bool istime =obj["istime"].toInt();
                    add = new KPIOverviewItem(space, name, start, stop, program, units, istime);
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;

            case OverviewItemType::USERCHART :
                {
                    QString settings=Utils::jsonunprotect(obj["settings"].toString());
                    add = new UserChartOverviewItem(space, name, settings);
                    add->datafilter = datafilter;
                    space->addItem(order,column,span,deep, add);
                }
                break;
            }
        }
    }

    // put in place
    space->updateGeometry();
}

//
// Config dialog that pops up when you click on the config button
//
OverviewConfigDialog::OverviewConfigDialog(ChartSpaceItem*item) : QDialog(NULL), item(item)
{
    setWindowTitle(tr("Chart Settings"));
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowCloseButtonHint);
    setModal(true);

    main = new QVBoxLayout(this);
    main->addWidget(item->config());
    item->config()->show();

    // buttons
    QHBoxLayout *buttons = new QHBoxLayout();
    remove = new QPushButton("Remove", this);
    ok = new QPushButton("Close", this);
    ok->setDefault(true);

    buttons->addWidget(remove);
    buttons->addStretch();
    buttons->addWidget(ok);

    main->addLayout(buttons);

    connect(ok, SIGNAL(clicked()), this, SLOT(close()));
    connect(remove, SIGNAL(clicked()), this, SLOT(removeItem()));
}

OverviewConfigDialog::~OverviewConfigDialog()
{
    if (item) {

        main->removeWidget(item->config());
        item->config()->setParent(NULL);
        item->config()->hide();
    }
}

void
OverviewConfigDialog::close()
{
    // remove from the layout- unless we just deleted it !
    if (item) {

        main->removeWidget(item->config());
        item->config()->setParent(NULL);
        item->config()->hide();

        // update geometry to show hide elements
        item->itemGeometryChanged();

        // update after config changed
        if (item->parent->scope & OverviewScope::ANALYSIS && item->parent->currentRideItem) item->setData(item->parent->currentRideItem);
        if (item->parent->scope & OverviewScope::TRENDS ) item->setDateRange(item->parent->currentDateRange);

        item=NULL;
    }

    accept();
}

void
OverviewConfigDialog::removeItem()
{
    hide(); // don't show the ugliness

    // remove config from our layout as about to be deleted
    main->takeAt(0);

    // remove from the space
    ChartSpace *space = item->parent;
    space->removeItem(item);

    // update geometry
    space->updateGeometry();
    space->updateView();

    item=NULL;

    // and we're done
    close();
}
