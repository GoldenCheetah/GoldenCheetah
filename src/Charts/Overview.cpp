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

OverviewWindow::OverviewWindow(Context *context, int scope, bool blank) : GcChartWindow(context), context(context), configured(false), scope(scope), blank(blank)
{
    setContentsMargins(0,0,0,0);
    setProperty("color", GColor(COVERVIEWBACKGROUND));
    setShowTitle(false);

    // actions...
    QAction *addTile= new QAction(tr("Add Tile..."));
    addAction(addTile);

    QAction *importChart= new QAction(tr("Import Chart..."));
    addAction(importChart);

    QAction *settings= new QAction(tr("Settings..."));
    addAction(settings);

    // settings
    QWidget *controls=new QWidget(this);
    QFormLayout *formlayout = new QFormLayout(controls);
    mincolsEdit= new QSpinBox(this);
    mincolsEdit->setMinimum(1);
    mincolsEdit->setMaximum(10);
    mincolsEdit->setValue(5);
    mincolsEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    formlayout->addRow(new QLabel(tr("Minimum Columns")), mincolsEdit);

    setControls(controls);

    QHBoxLayout *main = new QHBoxLayout;
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);

    space = new ChartSpace(context, scope, this);
    space->setMinimumColumns(minimumColumns());
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
    connect(importChart, SIGNAL(triggered(bool)), this, SLOT(importChart()));
    connect(settings, SIGNAL(triggered(bool)), this, SLOT(settings()));
    connect(mincolsEdit, SIGNAL(valueChanged(int)), this, SLOT(setMinimumColumns(int)));
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

        // update geometry
        space->updateGeometry();
        space->updateView();

    }
}

void
OverviewWindow::settings()
{
    emit showControls();
}

void
OverviewWindow::importChart()
{
    // first lets choose a .gchartfile
    QString suffix="csv";

    // get a filename to open
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import user chart from file"),
                       QDir::homePath()+"/",
                       ("*.gchart;;"), &suffix, QFileDialog::DontUseNativeDialog); // native dialog hangs

    if (fileName.isEmpty()) return;

    // read the file and parse into properties
    QList<QMap<QString,QString> > props = GcChartWindow::chartPropertiesFromFile(fileName);

    // we only support user charts, and they must be relevant to the current view
    QString want = QString("%1").arg(scope == OverviewScope::ANALYSIS ? GcWindowTypes::UserAnalysis : GcWindowTypes::UserTrends);

    // we look through all charts, but only import the first relevant one
    for (int i=0; i<props.count(); i++) {

        // make sure its one we want...
        if (props[i].value("TYPE", "") == want) {

            // get the chart settings (user chart has one main property)
            QString name = Utils::jsonunprotect(props[i].value("title","User Chart"));
            QString settings = Utils::jsonunprotect(props[i].value("settings",""));
            if (settings == "") goto nodice;

            // create new tile
            UserChartOverviewItem *add = new UserChartOverviewItem(space, name, settings);
            add->datafilter = "";
            space->addItem(0,0,3,25, add);

            // and update
            space->updateGeometry();

            // initialise to current selected daterange / activity as appropriate
            // need to do after geometry as it won't be visible till added to space
            if (scope == OverviewScope::ANALYSIS && space->currentRideItem) add->setData(space->currentRideItem);
            if (scope == OverviewScope::TRENDS ) add->setDateRange(space->currentDateRange);

            // and update- sometimes a little wonky
            space->updateGeometry();

            return;
        }
    }

nodice:
    QMessageBox::critical(this, tr("Not imported"), tr("We only support importing valid User Charts built for this view at present."));
    return;
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
    config += "  \"widths\":[";

    // column widths
    bool first=true;
    foreach(int n, space->columnWidths()) {

        // last one doesn't have a comma
        if (!first) config += ",";
        else first=false;

        config += QString(" %1").arg(n);
    }
    config += " ],\n";

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

        if (item->type != OverviewItemType::USERCHART) {
            config += "\"color\":\"" + item->bgcolor + "\",";
        }

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

                // although they aren't config we remember the last sort order
                // so it is retained over restarts, config of dynamic data is too hard
                config += "\"sortcolumn\":" + QString("%1").arg(data->lastsort) + ",";
                config += "\"sortorder\":" + QString("%1").arg(data->lastorder) + ",";
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
    if (blank == true || configured == true) return;
    configured = true;

    // DEFAULT CONFIG (FOR NOW WHEN NOT IN THE DEFAULT LAYOUT)
    //
    // default column widths - max 10 columns;
    // note the sizing is such that each card is the equivalent of a full screen
    // so we can embed charts etc without compromising what they can display
    //
    // we drop back here is the config is an old and unsupported format from pre v3.5

defaultsetup:

    if (config == "") {

        // to make life simpler we place a .gchart export into the resources
        // this is so we can design them and export rather than doing anything
        // special with the contents.
        //
        // so we open the json doc and extract the config element
        //
        QString source;
        if (scope == OverviewScope::ANALYSIS) source = ":charts/overview-analysis.gchart";
        if (scope == OverviewScope::TRENDS) source = ":charts/overview-trends.gchart";

        QFile file(source);
        if (file.open(QIODevice::ReadOnly)) {
            config = file.readAll();
            file.close();
        }

        QJsonDocument chart = QJsonDocument::fromJson(config.toUtf8());
        if (chart.isEmpty() || chart.isNull()) {

badconfig:
            fprintf(stderr, "bad config: %s\n", source.toStdString().c_str());
            return;
        }

        // root is "CHART"
        QJsonObject gchartroot = chart.object();
        if (!gchartroot.contains("CHART")) goto badconfig;
        QJsonObject ochart = gchartroot["CHART"].toObject();

        // CHART PROPERTIES
        if (!ochart.contains("PROPERTIES")) goto badconfig;
        QJsonObject properties = ochart["PROPERTIES"].toObject();

        // set from the config property
        if (!properties.contains("config")) goto badconfig;
        config = properties["config"].toString();
    }

    //
    // But by default we parse and apply (dropping back to default setup on error)
    //
    // parse
    QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8());
    if (doc.isEmpty() || doc.isNull()) {
        config="";
        return;
    }

    // parsed so lets work through it and setup the overview
    QJsonObject root = doc.object();

    // check version
    QString version = root["version"].toString();
    if (version != "2.0") {
        config="";
        goto defaultsetup;
    }

    // does it contain widths (we added this later)
    if (root.contains("widths")) {

        QVector<int> widths;
        QJsonArray w = root["widths"].toArray();
        foreach(const QJsonValue width, w) widths << width.toInt();
        space->setColumnWidths(widths);
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

                // added later, so not guaranteed to be there
                // we remember the sorting so we can reinstate after restart
                if (obj.contains("sortorder")) {
                    int column = obj["sortcolumn"].toInt();
                    int order = obj["sortorder"].toInt();
                    static_cast<DataOverviewItem*>(add)->lastsort = column;
                    static_cast<DataOverviewItem*>(add)->lastorder = static_cast<Qt::SortOrder>(order);
                }
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

        // color is common- if we actuall added one...
        if (add) {
            if (obj.contains("color") && type != OverviewItemType::USERCHART)  add->bgcolor = obj["color"].toString();
            else add->bgcolor = StandardColor(CCARDBACKGROUND).name();
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
    if (item->type == OverviewItemType::USERCHART) setWindowTitle(tr("Chart Settings"));
    else setWindowTitle(tr("Tile Settings"));

    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowCloseButtonHint);
    setModal(true);

    // get the factory and set what's this string according to type
    ChartSpaceItemRegistry &registry = ChartSpaceItemRegistry::instance();
    ChartSpaceItemDetail itemDetail = registry.detailForType(item->type);
    itemDetail.quick.replace(" ", "-"); // Blanks are replaced by - in Wiki URLs
    HelpWhatsThis *help = new HelpWhatsThis(this);
    if (item->parent->scope & OverviewScope::ANALYSIS) {
        this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Overview_Config).arg(itemDetail.quick, itemDetail.description));
    } else {
        this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Chart_Overview_Config).arg(itemDetail.quick, itemDetail.description));
    }

    main = new QVBoxLayout(this);
    main->addWidget(item->config());
    item->config()->show();

    // buttons
    QHBoxLayout *buttons = new QHBoxLayout();
    remove = new QPushButton(tr("Remove"), this);
    ok = new QPushButton(tr("Close"), this);
    ok->setDefault(true);

    buttons->addWidget(remove);
    buttons->addStretch();

    if (item->type == OverviewItemType::USERCHART) {
        exp = new QPushButton(tr("Export"), this);
        buttons->addWidget(exp);
        buttons->addStretch();
        connect(exp, SIGNAL(clicked()), this, SLOT(exportChart()));
    }
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

        // tell item its config changed
        item->configChanged(0);

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

void
OverviewConfigDialog::exportChart()
{
    // Overview tiles that contain user charts can be exported
    // as .gchart files, but bear in mind these tiles are not
    // chart windows, so we need to emulate the normal property
    // export.

    UserChartOverviewItem *chart = static_cast<UserChartOverviewItem*>(item);

    // get the filename
    QString basename = chart->name;
    QString suffix=".gchart";

    // get a filename to open
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export User Chart"),
                       QDir::homePath()+"/" + basename + ".gchart",
                       ("*.gchart;;"), &suffix, QFileDialog::DontUseNativeDialog); // native dialog hangs

    if (fileName.isEmpty()) return;

    // open, truncate and setup a text stream to output via
    QFile outfile(fileName);
    if (!outfile.open(QFile::WriteOnly)) {
        // couldn't open
        QMessageBox::critical(this, tr("Export User Chart"), tr("Open file for export failed."));
        return;
    }

    // truncate and start a stream
    outfile.resize(0);
    QTextStream out(&outfile);
    out.setCodec ("UTF-8");

    // serialise (mimicing real exporter in GoldenCheetah.cpp
    out <<"{\n\t\"CHART\":{\n";
    out <<"\t\t\"VERSION\":\"1\",\n";
    out <<"\t\t\"VIEW\":\"" << (item->parent->scope == OverviewScope::ANALYSIS ? "analysis" : "home") << "\",\n";
    out <<"\t\t\"TYPE\":\"" << (item->parent->scope == OverviewScope::ANALYSIS ? "46" : "45") << "\",\n";

    // PROPERTIES
    out <<"\t\t\"PROPERTIES\":{\n";

    // title and settings are the only two we can create basically
    out<<"\t\t\t\""<<"title"<<"\":\""<<Utils::jsonprotect(chart->name)<<" \",\n";
    out<<"\t\t\t\""<<"settings"<<"\":\""<<Utils::jsonprotect(chart->getConfig())<<" \",\n";

    out <<"\t\t\t\"__LAST__\":\"1\"\n";

    // end here, only one chart
    out<<"\t\t}\n\t}\n}";

    // all done
    outfile.close();


}
