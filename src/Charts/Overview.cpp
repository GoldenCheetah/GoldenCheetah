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
#include "AddChartWizard.h"

static QIcon grayConfig, whiteConfig, accentConfig;

OverviewWindow::OverviewWindow(Context *context) : GcChartWindow(context), context(context), configured(false)
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

    space = new ChartSpace(context);
    main->addWidget(space);

    // all the widgets
    setChartLayout(main);

    // tell space when a ride is selected
    connect(this, SIGNAL(rideItemChanged(RideItem*)), space, SLOT(rideSelected(RideItem*)));
    connect(addTile, SIGNAL(triggered(bool)), this, SLOT(addTile()));
    connect(space, SIGNAL(itemConfigRequested(ChartSpaceItem*)), this, SLOT(configItem(ChartSpaceItem*)));
}

void
OverviewWindow::addTile()
{
    AddChartWizard *p = new AddChartWizard(context, space);
    p->exec(); // no mem leak delete on close dialog
}

void
OverviewWindow::configItem(ChartSpaceItem *)
{
    //fprintf(stderr, "config item...\n"); fflush(stderr);
    //XXX TODO
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
        config += "\"deep\":" + QString("%1").arg(item->deep) + ",";
        config += "\"column\":" + QString("%1").arg(item->column) + ",";
        config += "\"order\":" + QString("%1").arg(item->order) + ",";

        // now the actual card settings
        switch(item->type) {
        case OverviewItemType::RPE:
            {
                RPEOverviewItem *rpe = reinterpret_cast<RPEOverviewItem*>(item);
            }
            break;
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
                RouteOverviewItem *route = reinterpret_cast<RouteOverviewItem*>(item);
            }
            break;
        case OverviewItemType::INTERVAL:
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
            }
            break;
        }

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

        // column 0
        ChartSpaceItem *add;
        add = new PMCOverviewItem(space, "coggan_tss");
        space->addItem(1,0,9, add);

        add = new MetaOverviewItem(space, "Sport", "Sport");
        space->addItem(2,0,5, add);

        add = new MetaOverviewItem(space, "Workout Code", "Workout Code");
        space->addItem(3,0,5, add);

        add = new MetricOverviewItem(space, "Duration", "workout_time");
        space->addItem(4,0,9, add);

        add = new MetaOverviewItem(space, "Notes", "Notes");
        space->addItem(5,0,13, add);

        // column 1
        add = new MetricOverviewItem(space, "HRV rMSSD", "rMSSD");
        space->addItem(1,1,9, add);

        add = new MetricOverviewItem(space, "Heartrate", "average_hr");
        space->addItem(2,1,5, add);

        add = new ZoneOverviewItem(space, "Heartrate Zones", RideFile::hr);
        space->addItem(3,1,11, add);

        add = new MetricOverviewItem(space, "Climbing", "elevation_gain");
        space->addItem(4,1,5, add);

        add = new MetricOverviewItem(space, "Cadence", "average_cad");
        space->addItem(5,1,5, add);

        add = new MetricOverviewItem(space, "Work", "total_work");
        space->addItem(6,1,5, add);

        // column 2
        add = new RPEOverviewItem(space, "RPE");
        space->addItem(1,2,9, add);

        add = new MetricOverviewItem(space, "Stress", "coggan_tss");
        space->addItem(2,2,5, add);

        add = new ZoneOverviewItem(space, "Fatigue Zones", RideFile::wbal);
        space->addItem(3,2,11, add);

        add = new IntervalOverviewItem(space, "Intervals", "elapsed_time", "average_power", "workout_time");
        space->addItem(4,2,17, add);

        // column 3
        add = new MetricOverviewItem(space, "Power", "average_power");
        space->addItem(1,3,9, add);

        add = new MetricOverviewItem(space, "IsoPower", "coggan_np");
        space->addItem(2,3,5, add);

        add = new ZoneOverviewItem(space, "Power Zones", RideFile::watts);
        space->addItem(3,3,11, add);

        add = new MetricOverviewItem(space, "Peak Power Index", "peak_power_index");
        space->addItem(4,3,8, add);

        add = new MetricOverviewItem(space, "Variability", "coggam_variability_index");
        space->addItem(5,3,8, add);

        // column 4
        add = new MetricOverviewItem(space, "Distance", "total_distance");
        space->addItem(1,4,9, add);

        add = new MetricOverviewItem(space, "Speed", "average_speed");
        space->addItem(2,4,5, add);

        add = new ZoneOverviewItem(space, "Pace Zones", RideFile::kph);
        space->addItem(3,4,11, add);

        add = new RouteOverviewItem(space, "Route");
        space->addItem(4,4,17, add);

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
            int column = obj["column"].toInt();
            int order = obj["order"].toInt();
            int deep = obj["deep"].toInt();
            int type = obj["type"].toInt();


            // lets create the cards
            ChartSpaceItem *add=NULL;
            switch(type) {

            case OverviewItemType::RPE :
                {
                    add = new RPEOverviewItem(space, name);
                    space->addItem(order,column,deep, add);
                }
                break;

            case OverviewItemType::METRIC :
                {
                    QString symbol=obj["symbol"].toString();
                    add = new MetricOverviewItem(space, name,symbol);
                    space->addItem(order,column,deep, add);
                }
                break;

            case OverviewItemType::META :
                {
                    QString symbol=obj["symbol"].toString();
                    add = new MetaOverviewItem(space, name,symbol);
                    space->addItem(order,column,deep, add);
                }
                break;

            case OverviewItemType::PMC :
                {
                    QString symbol=obj["symbol"].toString();
                    add = new PMCOverviewItem(space, symbol); // doesn't have a title
                    space->addItem(order,column,deep, add);
                }
                break;

            case OverviewItemType::ZONE :
                {
                    RideFile::SeriesType series = static_cast<RideFile::SeriesType>(obj["series"].toInt());
                    add = new ZoneOverviewItem(space, name, series);
                    space->addItem(order,column,deep, add);

                }
                break;

            case OverviewItemType::ROUTE :
                {
                    add = new RouteOverviewItem(space, name); // doesn't have a title
                    space->addItem(order,column,deep, add);
                }
                break;

            case OverviewItemType::INTERVAL :
                {
                    QString xsymbol=obj["xsymbol"].toString();
                    QString ysymbol=obj["ysymbol"].toString();
                    QString zsymbol=obj["zsymbol"].toString();

                    add = new IntervalOverviewItem(space, name, xsymbol, ysymbol, zsymbol); // doesn't have a title
                    space->addItem(order,column,deep, add);
                }
                break;
            }
        }
    }

    // put in place
    space->updateGeometry();
}
