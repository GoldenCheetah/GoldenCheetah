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

#include "OverviewWindow.h"

#include "TabView.h"
#include "Athlete.h"
#include "RideCache.h"
#include "IntervalItem.h"

#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"

#include "PMCData.h"
#include "RideMetadata.h"

#include <cmath>
#include <QGraphicsSceneMouseEvent>
#include <QGLWidget>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

static QIcon grayConfig, whiteConfig, accentConfig;

OverviewWindow::OverviewWindow(Context *context) :
    GcChartWindow(context), mode(CONFIG), state(NONE), context(context), group(NULL), _viewY(0),
                            yresizecursor(false), xresizecursor(false), block(false), scrolling(false),
                            setscrollbar(false), lasty(-1)
{
    setContentsMargins(0,0,0,0);
    setProperty("color", GColor(COVERVIEWBACKGROUND));
    //setProperty("nomenu", true);
    setShowTitle(false);
    setControls(NULL);

    QHBoxLayout *main = new QHBoxLayout;

    // add a view and scene and centre
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(this);
    view->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, false); // stops it stealing focus on mouseover
    scrollbar = new QScrollBar(Qt::Vertical, this);

    // how to move etc
    //view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setFrameStyle(QFrame::NoFrame);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setScene(scene);

    // layout
    main->addWidget(view);
    main->addWidget(scrollbar);

    // all the widgets
    setChartLayout(main);

    // by default these are the column sizes (user can adjust)
    columns << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200;

    // for changing the view
    group = new QParallelAnimationGroup(this);
    viewchange = new QPropertyAnimation(this, "viewRect");
    viewchange->setEasingCurve(QEasingCurve(QEasingCurve::OutQuint));

    // for scrolling the view
    scroller = new QPropertyAnimation(this, "viewY");
    scroller->setEasingCurve(QEasingCurve(QEasingCurve::Linear));

    // watch the view for mouse events
    view->setMouseTracking(true);
    scene->installEventFilter(this);

    // once all widgets created we can connect the signals
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(scroller, SIGNAL(finished()), this, SLOT(scrollFinished()));
    connect(scrollbar, SIGNAL(valueChanged(int)), this, SLOT(scrollbarMoved(int)));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));

    // set the widgets etc
    configChanged(CONFIG_APPEARANCE);

    // we're ready to plot, but not configured
    configured=false;
    stale=true;
    current=NULL;
}

QString
OverviewWindow::getConfiguration() const
{
    // return a JSON snippet to represent the entire config
    QString config;

    // setup
    config = "{\n  \"version\":\"1.0\",\n  \"columns\":[";
    for(int i=0; i<columns.count(); i++) {
        config += QString("%1%2").arg(columns[i]).arg(i+1<columns.count() ? "," : "");
    }
    config += "],\n  \"CARDS\":[\n";

    // do cards
    foreach(Card *card, cards) {
        // basic stuff first - name, type etc
        config += "    { ";
        config += "\"type\":" + QString("%1").arg(static_cast<int>(card->type)) + ",";
        config += "\"name\":\"" + card->name + "\",";
        config += "\"deep\":" + QString("%1").arg(card->deep) + ",";
        config += "\"column\":" + QString("%1").arg(card->column) + ",";
        config += "\"order\":" + QString("%1").arg(card->order) + ",";

        // now the actual card settings
        config += "\"series\":" + QString("%1").arg(static_cast<int>(card->settings.series)) + ",";
        config += "\"symbol\":\"" + QString("%1").arg(card->settings.symbol) + "\",";
        config += "\"xsymbol\":\"" + QString("%1").arg(card->settings.xsymbol) + "\",";
        config += "\"ysymbol\":\"" + QString("%1").arg(card->settings.ysymbol) + "\",";
        config += "\"zsymbol\":\"" + QString("%1").arg(card->settings.zsymbol) + "\"";

        config += " }";

        if (cards.last() != card) config += ",";
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

        columns.clear();
        columns << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200;

        // XXX lets hack in some tiles to start (will load from config later) XXX

        // column 0
        newCard(tr("PMC"), 0, 0, 9, Card::PMC, "coggan_tss");
        newCard(tr("Sport"), 0, 1, 5, Card::META, "Sport");
        newCard(tr("Workout Code"), 0, 2, 5, Card::META, "Workout Code");
        newCard(tr("Duration"), 0, 3, 9, Card::METRIC, "workout_time");
        newCard(tr("Notes"), 0, 4, 13, Card::META, "Notes");

        // column 1
        newCard(tr("HRV rMSSD"), 1, 0, 9, Card::METRIC, "rMSSD");
        newCard(tr("Heartrate"), 1, 1, 5, Card::METRIC, "average_hr");
        newCard(tr("Heartrate Zones"), 1, 2, 11, Card::ZONE, RideFile::hr);
        newCard(tr("Climbing"), 1, 3, 5, Card::METRIC, "elevation_gain");
        newCard(tr("Cadence"), 1, 4, 5, Card::METRIC, "average_cad");
        newCard(tr("Work"), 1, 6, 5, Card::METRIC, "total_work");

        // column 2
        newCard(tr("RPE"), 2, 0, 9, Card::RPE);
        newCard(tr("Stress"), 2, 1, 5, Card::METRIC, "coggan_tss");
        newCard(tr("Fatigue Zones"), 2, 2, 11, Card::ZONE, RideFile::wbal);
        newCard(tr("Intervals"), 2, 3, 17, Card::INTERVAL, "elapsed_time", "average_power", "workout_time");

        // column 3
        newCard(tr("Intensity"), 3, 0, 9, Card::METRIC, "coggan_if");
        newCard(tr("Power"), 3, 1, 5, Card::METRIC, "average_power");
        newCard(tr("Power Zones"), 3, 2, 11, Card::ZONE, RideFile::watts);
        newCard(tr("Equivalent Power"), 3, 3, 5, Card::METRIC, "coggan_np");
        newCard(tr("Peak Power Index"), 3, 4, 5, Card::METRIC, "peak_power_index");
        newCard(tr("Variability"), 3, 5, 5, Card::METRIC, "coggam_variability_index");

        // column 4
        newCard(tr("Distance"), 4, 0, 9, Card::METRIC, "total_distance");
        newCard(tr("Speed"), 4, 1, 5, Card::METRIC, "average_speed");
        newCard(tr("Pace Zones"), 4, 2, 11, Card::ZONE, RideFile::kph);
        newCard(tr("Route"), 4, 3, 17, Card::ROUTE);

        updateGeometry();
        return;
    }

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
    if (version != "1.0") {
        config="";
        goto defaultsetup;
    }

    // set columns
    columns.clear();
    QJsonArray cols = root["columns"].toArray();
    foreach (const QVariant &value, cols.toVariantList()) {
        columns << value.toInt();
    }

    // cards
    QJsonArray CARDS = root["CARDS"].toArray();
    foreach(const QJsonValue val, CARDS) {

        // convert so we can inspect
        QJsonObject obj = val.toObject();

        // get the basics
        QString name = obj["name"].toString();
        int column = obj["column"].toInt();
        int order = obj["order"].toInt();
        int deep = obj["deep"].toInt();
        Card::CardType type = static_cast<Card::CardType>(obj["type"].toInt());

        // get the settings
        RideFile::SeriesType series = static_cast<RideFile::SeriesType>(obj["series"].toInt());
        QString symbol=obj["symbol"].toString();
        QString xsymbol=obj["xsymbol"].toString();
        QString ysymbol=obj["ysymbol"].toString();
        QString zsymbol=obj["zsymbol"].toString();

        // lets create the cards
        switch(type) {
        case Card::NONE :
        case Card::MODEL :
        case Card::RPE :
        case Card::ROUTE : newCard(name, column, order, deep, type); break;
        case Card::METRIC :
        case Card::PMC :
        case Card::META : newCard(name, column, order, deep, type, symbol); break;
        case Card::INTERVAL : newCard(name, column, order, deep, type, xsymbol, ysymbol, zsymbol); break;
        case Card::SERIES :
        case Card::ZONE : newCard(name, column, order, deep, type, series); break;
        }
    }

    // put in place
    updateGeometry();
}

// when a ride is selected we need to notify all the cards
void
OverviewWindow::rideSelected()
{
    // don't plot when we're not visible, unless we have nothing plotted yet
    if (!isVisible() && current != NULL && myRideItem != NULL) {
        stale=true;
        return;
    }

    // don't replot .. we already did this one
    if (current == myRideItem && stale == false) {
        return;
    }

    // lets make sure its configured the first time thru
    setConfiguration(""); // sets to default config

// profiling the code
//QTime timer;
//timer.start();

    // ride item changed
    foreach(Card *card, cards) card->setData(myRideItem);

// profiling the code
//qDebug()<<"took:"<<timer.elapsed();

    // update
    updateView();

    // ok, remember we did this one
    current = myRideItem;
    stale=false;
}

// empty card
void
Card::setType(CardType type)
{
    if (type == NONE) {
        this->type = type;
    }

    if (type == RPE) {

        // a META widget, "RPE" using the FOSTER modified 0-10 scale
        this->type = type;
        settings.symbol = "RPE";
        fieldtype = FIELD_INTEGER;

        sparkline = new Sparkline(this, SPARKDAYS+1, name);
        rperating = new RPErating(this, name);
    }

    // create a map (for now, add profile later
    if (type == ROUTE) {

        this->type = type;
        routeline = new Routeline(this, name);
    }
}

// configure the cards
void
Card::setType(CardType type, RideFile::SeriesType series)
{
    this->type = type;
    settings.series = series;

    // basic chart setup
    chart = new QChart(this);
    chart->setBackgroundVisible(false); // draw on canvas
    chart->legend()->setVisible(false); // no legends
    chart->setTitle(""); // none wanted
    chart->setAnimationOptions(QChart::NoAnimation);

    // we have a mid sized font for chart labels etc
    chart->setFont(parent->midfont);

    if (type == Card::ZONE) {

        // needs a set of bars
        barset = new QBarSet(tr("Time In Zone"), this);
        barset->setLabelFont(parent->midfont);

        if (settings.series == RideFile::hr) {
            barset->setLabelColor(GColor(CHEARTRATE));
            barset->setBorderColor(GColor(CHEARTRATE));
            barset->setBrush(GColor(CHEARTRATE));
        } else if (settings.series == RideFile::watts) {
            barset->setLabelColor(GColor(CPOWER));
            barset->setBorderColor(GColor(CPOWER));
            barset->setBrush(GColor(CPOWER));
        } else if (settings.series == RideFile::wbal) {
            barset->setLabelColor(GColor(CWBAL));
            barset->setBorderColor(GColor(CWBAL));
            barset->setBrush(GColor(CWBAL));
        } else if (settings.series == RideFile::kph) {
            barset->setLabelColor(GColor(CSPEED));
            barset->setBorderColor(GColor(CSPEED));
            barset->setBrush(GColor(CSPEED));
        }


        //
        // HEARTRATE
        //
        if (series == RideFile::hr && parent->context->athlete->hrZones(false)) {
            // set the zero values
            for(int i=0; i<parent->context->athlete->hrZones(false)->getScheme().nzones_default; i++) {
                *barset << 0;
                categories << parent->context->athlete->hrZones(false)->getScheme().zone_default_name[i];
            }
        }

        //
        // POWER
        //
        if (series == RideFile::watts && parent->context->athlete->zones(false)) {
            // set the zero values
            for(int i=0; i<parent->context->athlete->zones(false)->getScheme().nzones_default; i++) {
                *barset << 0;
                categories << parent->context->athlete->zones(false)->getScheme().zone_default_name[i];
            }
        }

        //
        // PACE
        //
        if (series == RideFile::kph && parent->context->athlete->paceZones(false)) {
            // set the zero values
            for(int i=0; i<parent->context->athlete->paceZones(false)->getScheme().nzones_default; i++) {
                *barset << 0;
                categories << parent->context->athlete->paceZones(false)->getScheme().zone_default_name[i];
            }
        }

        //
        // W'BAL
        //
        if (series == RideFile::wbal) {
            categories << "Low" << "Med" << "High" << "Ext";
            *barset << 0 << 0 << 0 << 0;
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
}

void
Card::setType(CardType type, QString symbol)
{
    // metric or meta or pmc
    this->type = type;
    settings.symbol = symbol;

    // we may plot the metric sparkline if the tile is big enough
    if (type == METRIC) {
        sparkline = new Sparkline(this, SPARKDAYS+1, name);
    }

    // meta fields type?
    if (type == META) {

        this->settings.symbol = symbol;

        //  Get the field type
        fieldtype = -1;
        foreach(FieldDefinition p, parent->context->athlete->rideMetadata()->getFields()) {
            if (p.name == symbol) {
                fieldtype = p.type;
                break;
             }
        }

        // sparkline if are we numeric?
        if (fieldtype == FIELD_INTEGER || fieldtype == FIELD_DOUBLE) {

            sparkline = new Sparkline(this, SPARKDAYS+1, name);

        }
    }
}

// interval
void
Card::setType(CardType type, QString xsymbol, QString ysymbol, QString zsymbol)
{
    // metric or meta
    this->type = type;
    settings.xsymbol = xsymbol;
    settings.ysymbol = ysymbol;
    settings.zsymbol = zsymbol;

    // we may plot the metric sparkline if the tile is big enough
    if (type == INTERVAL) {
        bubble = new BubbleViz(this, "intervals");

        RideMetricFactory &factory = RideMetricFactory::instance();
        const RideMetric *xm = factory.rideMetric(xsymbol);
        const RideMetric *ym = factory.rideMetric(ysymbol);
        bubble->setAxisNames(xm ? xm->name() : "NA",
                             ym ? ym->name() : "NA");
    }
}

static const QStringList timeInZones = QStringList()
        << "percent_in_zone_L1"
        << "percent_in_zone_L2"
        << "percent_in_zone_L3"
        << "percent_in_zone_L4"
        << "percent_in_zone_L5"
        << "percent_in_zone_L6"
        << "percent_in_zone_L7"
        << "percent_in_zone_L8"
        << "percent_in_zone_L9"
        << "percent_in_zone_L10";

static const QStringList paceTimeInZones = QStringList()
        << "percent_in_zone_P1"
        << "percent_in_zone_P2"
        << "percent_in_zone_P3"
        << "percent_in_zone_P4"
        << "percent_in_zone_P5"
        << "percent_in_zone_P6"
        << "percent_in_zone_P7"
        << "percent_in_zone_P8"
        << "percent_in_zone_P9"
        << "percent_in_zone_P10";

static const QStringList timeInZonesHR = QStringList()
        << "percent_in_zone_H1"
        << "percent_in_zone_H2"
        << "percent_in_zone_H3"
        << "percent_in_zone_H4"
        << "percent_in_zone_H5"
        << "percent_in_zone_H6"
        << "percent_in_zone_H7"
        << "percent_in_zone_H8"
        << "percent_in_zone_H9"
        << "percent_in_zone_H10";

static const QStringList timeInZonesWBAL = QStringList()
        << "wtime_in_zone_L1"
        << "wtime_in_zone_L2"
        << "wtime_in_zone_L3"
        << "wtime_in_zone_L4";

void
Card::setData(RideItem *item)
{
    if (item == NULL || item->ride() == NULL) return;

    // use ride colors in painting?
    ridecolor = item->color;

    // stop any animation before starting, just in case- stops a crash
    // when we update a chart in the middle of its animation
    if (chart) chart->setAnimationOptions(QChart::NoAnimation);;

    if (type == ROUTE && scene() != NULL) {

        // only if we're place on the scene
        if (item->ride() && item->ride()->areDataPresent()->lat) {
            if (!routeline->isVisible()) routeline->show();
            routeline->setData(item);
        } else routeline->hide();
    }

    // non-numeric META
    if (!sparkline && type == META) {
        value = item->getText(settings.symbol, "");
    }

    // set the sparkline for numeric meta fields, metrics
    if (sparkline && (type == METRIC || type == META || type == RPE)) {

        // get last 30 days, if they exist
        QList<QPointF> points;

        // include current activity value
        double v;
        if (type == METRIC) {

            // get the metric value
            value = item->getStringForSymbol(settings.symbol, parent->context->athlete->useMetricUnits);
            v = (units == tr("seconds")) ?
            item->getForSymbol(settings.symbol, parent->context->athlete->useMetricUnits)
            : item->getStringForSymbol(settings.symbol, parent->context->athlete->useMetricUnits).toDouble();

        } else {

            // get the metadata value
            value = item->getText(settings.symbol, "0");
            if (fieldtype == FIELD_DOUBLE) v = value.toDouble();
            else v = value.toInt();

            // set the RPErating if needed
            if (rperating) rperating->setValue(value);
        }
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

                if (type == METRIC) {
                    v = (units == tr("seconds")) ?
                    prior->getForSymbol(settings.symbol, parent->context->athlete->useMetricUnits)
                    : prior->getStringForSymbol(settings.symbol, parent->context->athlete->useMetricUnits).toDouble();
                } else {

                    if (fieldtype == FIELD_DOUBLE)  v = prior->getText(settings.symbol, "").toDouble();
                    else v = prior->getText(settings.symbol, "").toInt();
                }

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
        if (units == tr("seconds")) {
            upper = time_to_string(max, true);
            lower = time_to_string(min, true);
            mean = time_to_string(avg, true);
        } else {
            upper = QString("%1").arg(max);
            lower = QString("%1").arg(min);

            // we need the same precision
            const RideMetricFactory &factory = RideMetricFactory::instance();
            const RideMetric *m = factory.rideMetric(settings.symbol);

            if (m) mean = m->toString(parent->context->athlete->useMetricUnits, avg);
            else mean = QString("%1").arg(avg, 0, 'f', 0);
        }
    }

    if (type == PMC) {
        // get lts, sts, sb, rr for the input metric
        PMCData *pmc = parent->context->athlete->getPMCFor(settings.symbol);

        QDate date = item ? item->dateTime.date() : QDate();
        lts = pmc->lts(date);
        sts = pmc->sts(date);
        stress = pmc->stress(date);
        sb = pmc->sb(date);
        rr = pmc->rr(date);
    }

    if (type == ZONE) {

        // enable animation when setting values (disabled at all other times)
        if (chart) chart->setAnimationOptions(QChart::SeriesAnimations);

        switch(settings.series) {

        //
        // HEARTRATE
        //
        case RideFile::hr:
        {
            if (parent->context->athlete->hrZones(item->isRun)) {

                int numhrzones;
                int hrrange = parent->context->athlete->hrZones(item->isRun)->whichRange(item->dateTime.date());

                if (hrrange > -1) {

                    numhrzones = parent->context->athlete->hrZones(item->isRun)->numZones(hrrange);
                    for(int i=0; i<categories.count() && i < numhrzones;i++) {
                        barset->replace(i, round(item->getForSymbol(timeInZonesHR[i])));
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
            if (parent->context->athlete->zones(item->isRun)) {

                int numzones;
                int range = parent->context->athlete->zones(item->isRun)->whichRange(item->dateTime.date());

                if (range > -1) {

                    numzones = parent->context->athlete->zones(item->isRun)->numZones(range);
                    for(int i=0; i<categories.count() && i < numzones;i++) {
                        barset->replace(i, round(item->getForSymbol(timeInZones[i])));
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
            if ((item->isRun || item->isSwim) && parent->context->athlete->paceZones(item->isSwim)) {

                int numzones;
                int range = parent->context->athlete->paceZones(item->isSwim)->whichRange(item->dateTime.date());

                if (range > -1) {

                    numzones = parent->context->athlete->paceZones(item->isSwim)->numZones(range);
                    for(int i=0; i<categories.count() && i < numzones;i++) {
                        barset->replace(i, round(item->getForSymbol(paceTimeInZones[i])));
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

    if (type == INTERVAL) {

        double minx = 999999999;
        double maxx =-999999999;
        double miny = 999999999;
        double maxy =-999999999;

        //set the x, y series
        QList<BPointF> points;
        foreach(IntervalItem *interval, item->intervals()) {
            // get the x and y VALUE
            double x = interval->getForSymbol(settings.xsymbol, parent->context->athlete->useMetricUnits);
            double y = interval->getForSymbol(settings.ysymbol, parent->context->athlete->useMetricUnits);
            double z = interval->getForSymbol(settings.zsymbol, parent->context->athlete->useMetricUnits);

            BPointF add;
            add.x = x;
            add.y = y;
            add.z = z;
            add.fill = interval->color;
            add.label = interval->name;
            points << add;

            if (x<minx) minx=x;
            if (y<miny) miny=y;
            if (x>maxx) maxx=x;
            if (y>maxy) maxy=y;
        }


        // set scale
        double ydiff = (maxy-miny) / 10.0f;
        if (miny >= 0 && ydiff > miny) miny = ydiff;
        double xdiff = (maxx-minx) / 10.0f;
        if (minx >= 0 && xdiff > minx) minx = xdiff;
        maxx=round(maxx); minx=round(minx);
        maxy=round(maxy); miny=round(miny);

        // set range before points to filter
        bubble->setRange(minx,maxx,miny,maxy);
        bubble->setPoints(points);
    }
}

void
Card::setDrag(bool x)
{
    drag = x;

    // hide stuff
    if (drag && chart) chart->hide();
    if (!drag) geometryChanged();
}

void
Card::geometryChanged() {

    QRectF geom = geometry();

    // route map needs adding to scene etc
    if (type == ROUTE) {

        if (!drag) {
            if (myRideItem && myRideItem->ride() && myRideItem->ride()->areDataPresent()->lat) routeline->show();
            routeline->setGeometry(20,ROWHEIGHT+40, geom.width()-40, geom.height()-(60+ROWHEIGHT));

        } else routeline->hide();
    }

    if (type == INTERVAL) {

        if (!drag) bubble->show();
        // disable animation when changing geometry
        bubble->setGeometry(20,20+(ROWHEIGHT*2), geom.width()-40, geom.height()-(40+(ROWHEIGHT*2)));
    }

    // if we contain charts etc lets update their geom
    if ((type == ZONE || type == SERIES) && chart)  {

        if (!drag) chart->show();
        // disable animation when changing geometry
        chart->setAnimationOptions(QChart::NoAnimation);
        chart->setGeometry(20,20+(ROWHEIGHT*2), geom.width()-40, geom.height()-(40+(ROWHEIGHT*2)));
    }

    if (sparkline && (type == METRIC || type == META || type == RPE)) {

        // make space for the rpe rating widget if needed
        int minh=6;
        if (type == RPE) {
            if (!drag && geom.height() > (ROWHEIGHT*5)+20) {
                rperating->show();
                rperating->setGeometry(20+(ROWHEIGHT*2), ROWHEIGHT*3, geom.width()-40-(ROWHEIGHT*4), ROWHEIGHT*2);
            } else { // not set for meta or metric
                rperating->hide();
            }
            minh=7;
        }

        // space enough?
        if (!drag && geom.height() > (ROWHEIGHT*minh)) {
            sparkline->show();
            sparkline->setGeometry(20, ROWHEIGHT*(minh-2), geom.width()-40, geom.height()-20-(ROWHEIGHT*(minh-2)));
        } else {
            sparkline->hide();
        }
    }
}

bool
Card::sceneEvent(QEvent *event)
{
    // skip whilst dragging and resizing
    if (parent->state != OverviewWindow::NONE) return false;

    // repaint when mouse enters and leaves
    if (event->type() == QEvent::GraphicsSceneHoverLeave ||
        event->type() == QEvent::GraphicsSceneHoverEnter) {

        // force repaint
        update();
        scene()->update();

    // repaint when in the corner
    } else if (event->type() == QEvent::GraphicsSceneHoverMove && inCorner() != incorner) {

        incorner = inCorner();
        update();
        scene()->update();
    }
    return false;
}

bool
Card::inCorner()
{
    QPoint vpos = parent->view->mapFromGlobal(QCursor::pos());
    QPointF spos = parent->view->mapToScene(vpos);

    if (geometry().contains(spos.x(), spos.y())) {
        if (spos.y() - geometry().top() < (ROWHEIGHT+40) &&
            geometry().width() - (spos.x() - geometry().x()) < (ROWHEIGHT+40))
            return true;
    }
    return false;

}

bool
Card::underMouse()
{
    QPoint vpos = parent->view->mapFromGlobal(QCursor::pos());
    QPointF spos = parent->view->mapToScene(vpos);

    if (geometry().contains(spos.x(), spos.y())) return true;
    return false;
}

// cards need to show they are in config mode
void
Card::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {

    if (drag) painter->setBrush(QBrush(GColor(CPLOTMARKER)));
    else painter->setBrush(GColor(CCARDBACKGROUND));

    QPainterPath path;
    path.addRoundedRect(QRectF(0,0,geometry().width(),geometry().height()), ROWHEIGHT/5, ROWHEIGHT/5);
    painter->setPen(Qt::NoPen);
    //painter->fillPath(path, brush.color());
    painter->drawPath(path);
    painter->setPen(GColor(CPLOTGRID));
    //XXXpainter->drawLine(QLineF(0,ROWHEIGHT*2,geometry().width(),ROWHEIGHT*2));
    //painter->fillRect(QRectF(0,0,geometry().width()+1,geometry().height()+1), brush);
    //titlefont.setWeight(QFont::Bold);
    if (GCColor::luminance(GColor(CCARDBACKGROUND)) < 127) painter->setPen(QColor(200,200,200));
    else painter->setPen(QColor(70,70,70));

    painter->setFont(parent->titlefont);
    if (type != PMC || drag) painter->drawText(QPointF(ROWHEIGHT /2.0f, QFontMetrics(parent->titlefont, parent->device()).height()), name);

    // only paint contents if not dragging
    if (drag) return;

    // not dragging so we can get to work painting the rest
    if (parent->state != OverviewWindow::DRAG && underMouse()) {

        if (inCorner()) {

            // if hovering over the button show a background to indicate
            // that pressing a button is good
            QPainterPath path;
            path.addRoundedRect(QRectF(geometry().width()-40-ROWHEIGHT,0,
                                ROWHEIGHT+40, ROWHEIGHT+40), ROWHEIGHT/5, ROWHEIGHT/5);
            painter->setPen(Qt::NoPen);
            QColor darkgray(GColor(CCARDBACKGROUND).lighter(200));
            painter->setBrush(darkgray);
            painter->drawPath(path);
            painter->fillRect(QRectF(geometry().width()-40-ROWHEIGHT, 0, ROWHEIGHT+40-(ROWHEIGHT/5), ROWHEIGHT+40), QBrush(darkgray));
            painter->fillRect(QRectF(geometry().width()-40-ROWHEIGHT, ROWHEIGHT/5, ROWHEIGHT+40, ROWHEIGHT+40-(ROWHEIGHT/5)), QBrush(darkgray));

            // draw the config button and make it more obvious
            // when hovering over the card
            painter->drawPixmap(geometry().width()-20-(ROWHEIGHT*1), 20, ROWHEIGHT*1, ROWHEIGHT*1, accentConfig.pixmap(QSize(ROWHEIGHT*1, ROWHEIGHT*1)));

        } else {

            // hover on card - make it more obvious there is a config button
            painter->drawPixmap(geometry().width()-20-(ROWHEIGHT*1), 20, ROWHEIGHT*1, ROWHEIGHT*1, whiteConfig.pixmap(QSize(ROWHEIGHT*1, ROWHEIGHT*1)));
        }

    } else painter->drawPixmap(geometry().width()-20-(ROWHEIGHT*1), 20, ROWHEIGHT*1, ROWHEIGHT*1, grayConfig.pixmap(QSize(ROWHEIGHT*1, ROWHEIGHT*1)));

    if (!sparkline && type == META && fieldtype >= 0) {

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

    if (sparkline && (type == METRIC || type == META || type == RPE)) {

        // we need the metric units
        if (type == METRIC && metric == NULL) {
            // get the metric details
            RideMetricFactory &factory = RideMetricFactory::instance();
            metric = const_cast<RideMetric*>(factory.rideMetric(settings.symbol));
            if (metric) units = metric->units(parent->context->athlete->useMetricUnits);
        }

        double addy = 0;
        if (type == RPE && rperating->isVisible()) addy=ROWHEIGHT*2; // shift up for rperating
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

    if (type == PMC) {

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
}

RPErating::RPErating(Card *parent, QString name) : QGraphicsItem(NULL), parent(parent), name(name), hover(false)
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
    if (parent->parent->state != OverviewWindow::NONE) return false;

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
    RideItem *item = parent->parent->property("ride").value<RideItem*>();

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

BubbleViz::BubbleViz(Card *parent, QString name) : QGraphicsItem(NULL), parent(parent), name(name), hover(false)
{
    setGeometry(20,20,100,100);
    setZValue(11);
    setAcceptHoverEvents(true);
    animator=new QPropertyAnimation(this, "transition");
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
    if (parent->parent->state != OverviewWindow::NONE) return false;

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
BubbleViz::setPoints(QList<BPointF> p)
{
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
    qSort(scores.begin(), scores.end(), scoresBiggerThan);

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
    animator->stop();
    animator->setStartValue(0);
    animator->setEndValue(256);
    animator->setEasingCurve(QEasingCurve::OutQuad);
    animator->setDuration(1000);
    animator->start();
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
    double oxratio = plotarea.width() / (oldmaxx*1.03);
    double oyratio = plotarea.height() / (oldmaxy*1.03); // boundary space

    // run through each point
    double area = 10000; // max size

    // remember the one we are nearest
    BPointF nearest;
    double nearvalue = -1;

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
                alpha = (200.0f/255.0f) * transition;
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
        color.setAlpha(200 - (200.0f/255.0f) * double(transition));
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

    painter->setBrush(Qt::NoBrush);

    // clip to canvas
    painter->setClipRect(canvas);

    // x-axis labels
    QRectF xlabelspace = QRectF(plotarea.x(), plotarea.bottom() + 20, plotarea.width(), ROWHEIGHT);
    painter->setPen(Qt::red);
    //DIAG painter->drawRect(xlabelspace);

    // x-axis title
    QRectF xtitlespace = QRectF(plotarea.x(), xlabelspace.bottom(), plotarea.width(), ROWHEIGHT);
    painter->setPen(Qt::yellow);
    //DIAG painter->drawRect(xtitlespace);

    QTextOption midcenter;
    midcenter.setAlignment(Qt::AlignVCenter|Qt::AlignCenter);

    painter->setPen(QColor(150,150,150));
    painter->setFont(parent->parent->smallfont);
    painter->drawText(xtitlespace, xlabel, midcenter);


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
    QFontMetrics sfm(parent->parent->smallfont);
    QRectF bminx = sfm.tightBoundingRect(QString("%1").arg(minx));
    QRectF bmaxx = sfm.tightBoundingRect(QString("%1").arg(maxx));
    painter->drawText(xlabelspace.left() + (minx*xratio) - (bminx.width()/2),  xlabelspace.bottom(), QString("%1").arg(minx));
    painter->drawText(xlabelspace.left() + (maxx*xratio) - (bmaxx.width()/2),  xlabelspace.bottom(), QString("%1").arg(maxx));

    // draw minimum value
    painter->drawLine(QPointF(plotarea.left(), plotarea.bottom() - (miny*yratio)),
                      QPointF(plotarea.left(), plotarea.bottom() - (maxy*yratio)));
    // y-axis range
    QRectF bminy = sfm.tightBoundingRect(QString("%1").arg(miny));
    QRectF bmaxy = sfm.tightBoundingRect(QString("%1").arg(maxy));
    painter->drawText(ylabelspace.right() - bmaxy.width(),  ylabelspace.bottom()-(maxy*yratio) + (bmaxy.height()/2), QString("%1").arg(maxy));
    painter->drawText(ylabelspace.right() - bminy.width(),  ylabelspace.bottom()-(miny*yratio) + (bminy.height()/2), QString("%1").arg(miny));

    // hover point?
    painter->setPen(GColor(CPLOTMARKER));

    if (hover && nearvalue >= 0) {

        painter->setFont(parent->parent->titlefont);
        QFontMetrics tfm(parent->parent->titlefont);

        // where is it?
        QPointF center(plotarea.left() + (xratio * nearest.x), plotarea.bottom() - (yratio * nearest.y));

        // xlabel
        QString xlab = QString("%1").arg(nearest.x, 0, 'f', 0);
        bminx = tfm.tightBoundingRect(QString("%1").arg(xlab));
        bminx.moveTo(center.x() - (bminx.width()/2),  xlabelspace.bottom()-bminx.height());
        painter->fillRect(bminx, QBrush(GColor(CCARDBACKGROUND))); // overwrite range labels
        painter->drawText(center.x() - (bminx.width()/2),  xlabelspace.bottom(), xlab);

        // ylabel
        QString ylab = QString("%1").arg(nearest.y, 0, 'f', 0);
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

    painter->restore();
}

Sparkline::Sparkline(QGraphicsWidget *parent, int count, QString name)
    : QGraphicsItem(NULL), parent(parent), name(name)
{
    Q_UNUSED(count)
    
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
    double xfactor = (geom.width() - (ROWHEIGHT*6)) / SPARKDAYS;
    double xoffset = boundingRect().left()+(ROWHEIGHT*2);
    double yfactor = (geom.height()-(ROWHEIGHT)) / (max-min);
    double bottom = boundingRect().bottom()-ROWHEIGHT/2;

    // draw a sparkline -- need more than 1 point !
    if (points.count() > 1) {

        QPainterPath path;
        path.moveTo((points[0].x()*xfactor)+xoffset, bottom-((points[0].y()-min)*yfactor));
        for(int i=1; i<points.count();i++) {
            path.lineTo((points[i].x()*xfactor)+xoffset, bottom-((points[i].y()-min)*yfactor));
        }

        QPen pen(QColor(150,150,150));
        pen.setWidth(8);
        //pen.setStyle(Qt::DotLine);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->drawPath(path);

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

Routeline::Routeline(QGraphicsWidget *parent, QString name) : QGraphicsItem(NULL), parent(parent), name(name)
{
    setGeometry(20,20,100,100);
    setZValue(11);
    animator=new QPropertyAnimation(this, "transition");
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

        if (p->lat > maxlat) maxlat=p->lat;
        if (p->lat < minlat) minlat=p->lat;
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
                        (height-(height / (ydiff / (p->lat - minlat)))));
            count=div;

        } else if (count == 0) {

            //path.lineTo(xoff+(geom.width() / (xdiff / (p->lon - minlon))),
            //            yoff+(geom.height()-(geom.height() / (ydiff / (p->lat - minlat)))));
            path.lineTo((geom.width() / (xdiff / (p->lon - minlon))),
                        (height-(height / (ydiff / (p->lat - minlat)))));
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

static bool cardSort(const Card* left, const Card* right)
{
    return (left->column < right->column ? true : (left->column == right->column && left->order < right->order ? true : false));
}

void
OverviewWindow::updateGeometry()
{
    bool animated=false;

    // prevent a memory leak
    group->stop();
    delete group;
    group = new QParallelAnimationGroup(this);

    // order the items to their positions
    qSort(cards.begin(), cards.end(), cardSort);

    int y=SPACING;
    int maxy = y;
    int column=-1;

    int x=SPACING;

    // just set their geometry for now, no interaction
    for(int i=0; i<cards.count(); i++) {

        // don't show hidden
        if (!cards[i]->isVisible()) continue;

        // move on to next column, check if first item too
        if (cards[i]->column > column) {

            // once past the first column we need to update x
            if (column >= 0) x+= columns[column] + SPACING;

            int diff = cards[i]->column - column - 1;
            if (diff > 0) {

                // there are empty columns so shift the cols to the right
                // to the left to fill  the gap left and all  the column
                // widths also need to move down too
                for(int j=cards[i]->column-1; j < 8; j++) columns[j]=columns[j+1];
                for(int j=i; j<cards.count();j++) cards[j]->column -= diff;
            }
            y=SPACING; column = cards[i]->column;

        }

        // set geometry
        int ty = y;
        int tx = x;
        int twidth = columns[column];
        int theight = cards[i]->deep * ROWHEIGHT;

        // make em smaller when configuring visual cue stolen from Windows Start Menu
        int add = 0; //XXX PERFORMANCE ISSSE XXX (state == DRAG) ? (ROWHEIGHT/2) : 0;


        // for setting the scene rectangle - but ignore a card if we are dragging it
        if (maxy < ty+theight+SPACING) maxy = ty+theight+SPACING;

        // add to scene if new
        if (!cards[i]->onscene) {
            scene->addItem(cards[i]);
            cards[i]->setGeometry(tx, ty, twidth, theight);
            cards[i]->onscene = true;

        } else if (cards[i]->invisible == false &&
                   (cards[i]->geometry().x() != tx+add ||
                    cards[i]->geometry().y() != ty+add ||
                    cards[i]->geometry().width() != twidth-(add*2) ||
                    cards[i]->geometry().height() != theight-(add*2))) {

            // we've got an animation to perform
            animated = true;

            // add an animation for this movement
            QPropertyAnimation *animation = new QPropertyAnimation(cards[i], "geometry");
            animation->setDuration(300);
            animation->setStartValue(cards[i]->geometry());
            animation->setEndValue(QRect(tx+add,ty+add,twidth-(add*2),theight-(add*2)));

            // when placing a little feedback helps
            if (cards[i]->placing) {
                animation->setEasingCurve(QEasingCurve(QEasingCurve::OutBack));
                cards[i]->placing = false;
            } else animation->setEasingCurve(QEasingCurve(QEasingCurve::OutQuint));

            group->addAnimation(animation);
        }

        // set spot for next tile
        y += theight + SPACING;
    }

    // set the scene rectangle, columns start at 0
    sceneRect = QRectF(0, 0, columns[column] + x + SPACING, maxy);

    if (animated) group->start();
}

void
OverviewWindow::configChanged(qint32)
{
    grayConfig = colouredIconFromPNG(":images/configure.png", GColor(CCARDBACKGROUND).lighter(75));
    whiteConfig = colouredIconFromPNG(":images/configure.png", QColor(100,100,100));
    accentConfig = colouredIconFromPNG(":images/configure.png", QColor(150,150,150));

    // set fonts
    bigfont.setPixelSize(pixelSizeForFont(bigfont, ROWHEIGHT *2.5f));
    titlefont.setPixelSize(pixelSizeForFont(titlefont, ROWHEIGHT)); // need a bit of space
    midfont.setPixelSize(pixelSizeForFont(midfont, ROWHEIGHT *0.8f));
    smallfont.setPixelSize(pixelSizeForFont(smallfont, ROWHEIGHT*0.7f));

    setProperty("color", GColor(COVERVIEWBACKGROUND));
    view->setBackgroundBrush(QBrush(GColor(COVERVIEWBACKGROUND)));
    scene->setBackgroundBrush(QBrush(GColor(COVERVIEWBACKGROUND)));
    scrollbar->setStyleSheet(TabView::ourStyleSheet());

    // text edit colors
    QPalette palette;
    palette.setColor(QPalette::Window, GColor(COVERVIEWBACKGROUND));

    // only change base if moved away from white plots
    // which is a Mac thing
#ifndef Q_OS_MAC
    if (GColor(COVERVIEWBACKGROUND) != Qt::white)
#endif
    {
        //palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CTRAINPLOTBACKGROUND)));
        palette.setColor(QPalette::Base, GColor(COVERVIEWBACKGROUND));
        palette.setColor(QPalette::Window, GColor(COVERVIEWBACKGROUND));
    }

#ifndef Q_OS_MAC // the scrollers appear when needed on Mac, we'll keep that
    //code->setStyleSheet(TabView::ourStyleSheet());
#endif

    palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(COVERVIEWBACKGROUND)));
    palette.setColor(QPalette::Text, GCColor::invertColor(GColor(COVERVIEWBACKGROUND)));
    //code->setPalette(palette);
    repaint();
}

void
OverviewWindow::updateView()
{
    scene->setSceneRect(sceneRect);
    scene->update();

    // don'r scale whilst resizing on x?
    if (scrolling || (state != YRESIZE && state != XRESIZE && state != DRAG)) {

        // much of a resize / change ?
        double dx = fabs(viewRect.x() - sceneRect.x());
        double dy = fabs(viewRect.y() - sceneRect.y());
        double vy = fabs(viewRect.y()-double(_viewY));
        double dwidth = fabs(viewRect.width() - sceneRect.width());
        double dheight = fabs(viewRect.height() - sceneRect.height());

        // scale immediately if not a bit change
        // otherwise it feels unresponsive
        if (viewRect.width() == 0 || (vy < 20 && dx < 20 && dy < 20 && dwidth < 20 && dheight < 20)) {
            setViewRect(sceneRect);
        } else {

            // tempting to make this longer but feels ponderous at longer durations
            viewchange->setDuration(400);
            viewchange->setStartValue(viewRect);
            viewchange->setEndValue(sceneRect);
            viewchange->start();
        }
    }

    if (view->sceneRect().height() >= scene->sceneRect().height()) {
        scrollbar->setEnabled(false);
    } else {

        // now set scrollbar
        setscrollbar = true;
        scrollbar->setMinimum(0);
        scrollbar->setMaximum(scene->sceneRect().height()-view->sceneRect().height());
        scrollbar->setValue(_viewY);
        scrollbar->setPageStep(view->sceneRect().height());
        scrollbar->setEnabled(true);
        setscrollbar = false;
    }
}

void
OverviewWindow::edgeScroll(bool down)
{
    // already scrolling, so don't move
    if (scrolling) return;
    // we basically scroll the view if the cursor is at or above
    // the top of the view, or at or below the bottom and the mouse
    // is moving away. Needs to work in normal and full screen.
    if (state == DRAG || state == YRESIZE) {

        QPointF pos =this->mapFromGlobal(QCursor::pos());

        if (!down && pos.y() <= 0) {

            // at the top of the screen, go up a qtr of a screen
            scrollTo(_viewY - (view->sceneRect().height()/4));

        } else if (down && (geometry().height()-pos.y()) <= 0) {

            // at the bottom of the screen, go down a qtr of a screen
            scrollTo(_viewY + (view->sceneRect().height()/4));

        }
    }
}

void
OverviewWindow::scrollTo(int newY)
{

    // bound the target to the top or a screenful from the bottom, except when we're
    // resizing on Y as we are expanding the scene by increasing the size of an object
    if ((state != YRESIZE) && (newY +view->sceneRect().height()) > sceneRect.bottom())
        newY = sceneRect.bottom() - view->sceneRect().height();
    if (newY < 0)
        newY = 0;

    if (_viewY != newY) {

        if (abs(_viewY - newY) < 20) {

            // for small scroll increments just do it, its tedious to wait for animations
            _viewY = newY;
            updateView();

        } else {

            // disable other view updates whilst scrolling
            scrolling = true;

            // make it snappy for short distances - ponderous for drag scroll
            // and vaguely snappy for page by page scrolling
            if (state == DRAG || state == YRESIZE) scroller->setDuration(300);
            else if (abs(_viewY-newY) < 100) scroller->setDuration(150);
            else scroller->setDuration(250);
            scroller->setStartValue(_viewY);
            scroller->setEndValue(newY);
            scroller->start();
        }
    }
}

void
OverviewWindow::setViewRect(QRectF rect)
{
    viewRect = rect;

    // fit to scene width XXX need to fix scrollbars.
    double scale = view->frameGeometry().width() / viewRect.width();
    QRectF scaledRect(0,_viewY, viewRect.width(), view->frameGeometry().height() / scale);

    // scale to selection
    view->scale(scale,scale);
    view->setSceneRect(scaledRect);
    view->fitInView(scaledRect, Qt::KeepAspectRatio);

    // if we're dragging, as the view changes it can be really jarring
    // as the dragged item is not under the mouse then snaps back
    // this might need to be cleaned up as a little too much of spooky
    // action at a distance going on here !
    if (state == DRAG) {

        // update drag point
        QPoint vpos = view->mapFromGlobal(QCursor::pos());
        QPointF pos = view->mapToScene(vpos);

        // move the card being dragged
        stateData.drag.card->setPos(pos.x()-stateData.drag.offx, pos.y()-stateData.drag.offy);
    }

    view->update();

}

bool
OverviewWindow::eventFilter(QObject *, QEvent *event)
{
    if (block || (event->type() != QEvent::KeyPress && event->type() != QEvent::GraphicsSceneWheel &&
                  event->type() != QEvent::GraphicsSceneMousePress && event->type() != QEvent::GraphicsSceneMouseRelease &&
                  event->type() != QEvent::GraphicsSceneMouseMove)) {
        return false;
    }
    block = true;
    bool returning = false;

    // we only filter out keyboard shortcuts for undo redo etc
    // in the qwkcode editor, anything else is of no interest.
    if (event->type() == QEvent::KeyPress) {

        // we care about cmd / ctrl
        Qt::KeyboardModifiers kmod = static_cast<QInputEvent*>(event)->modifiers();
        bool ctrl = (kmod & Qt::ControlModifier) != 0;

        switch(static_cast<QKeyEvent*>(event)->key()) {

        case Qt::Key_Y:
            if (ctrl) {
                //workout->redo();
                returning = true; // we grab all key events
            }
            break;

        case Qt::Key_Z:
            if (ctrl) {
                //workout->undo();
                returning=true;
            }
            break;

        case Qt::Key_Home:
            scrollTo(0);
            break;

        case Qt::Key_End:
            scrollTo(scene->sceneRect().bottom());
            break;

        case Qt::Key_PageDown:
            scrollTo(_viewY + view->sceneRect().height());
            break;

        case Qt::Key_PageUp:
            scrollTo(_viewY - view->sceneRect().height());
            break;

        case Qt::Key_Down:
            scrollTo(_viewY + ROWHEIGHT);
            break;

        case Qt::Key_Up:
            scrollTo(_viewY - ROWHEIGHT);
            break;
        }

    } else  if (event->type() == QEvent::GraphicsSceneWheel) {

        // take it as applied
        QGraphicsSceneWheelEvent *w = static_cast<QGraphicsSceneWheelEvent*>(event);
        scrollTo(_viewY - (w->delta()*2));
        event->accept();
        returning = true;

    } else  if (event->type() == QEvent::GraphicsSceneMousePress) {

        // we will process clicks when configuring so long as we're
        // not in the middle of something else - this is to start
        // dragging a card around
        if (mode == CONFIG && state == NONE) {

            // where am i ?
            QPointF pos = static_cast<QGraphicsSceneMouseEvent*>(event)->scenePos();
            QGraphicsItem *item = scene->itemAt(pos, view->transform());
            Card *card = static_cast<Card*>(item);

            // ignore other scene elements (e.g. charts)
            if (!cards.contains(card)) card=NULL;

            // only respond to clicks not in config corner button
            if (card && ! card->inCorner()) {

               // are we on the boundary of the card?
               double offx = pos.x()-card->geometry().x();
               double offy = pos.y()-card->geometry().y();


               if (card->geometry().height()-offy < 10) {

                    state = YRESIZE;

                    stateData.yresize.card = card;
                    stateData.yresize.deep = card->deep;
                    stateData.yresize.posy = pos.y();

                    // thanks we'll take that
                    event->accept();
                    returning = true;

               } else if (card->geometry().width()-offx < 10) {

                    state = XRESIZE;

                    stateData.xresize.column = card->column;
                    stateData.xresize.width = columns[card->column];
                    stateData.xresize.posx = pos.x();

                    // thanks we'll take that
                    event->accept();
                    returning = true;

               } else {

                    // we're grabbing a card, so lets
                    // work out the offset so we can move
                    // it around when we start dragging
                    state = DRAG;
                    card->invisible = true;
                    card->setDrag(true);
                    card->setZValue(100);

                    stateData.drag.card = card;
                    stateData.drag.offx = offx;
                    stateData.drag.offy = offy;
                    stateData.drag.width = columns[card->column];

                    // thanks we'll take that
                    event->accept();
                    returning = true;

                    // what is the offset?
                    //updateGeometry();
                    scene->update();
                    view->update();
                }
            }
        }

    } else  if (event->type() == QEvent::GraphicsSceneMouseRelease) {

        // stop dragging
        if (mode == CONFIG && (state == DRAG || state == YRESIZE || state == XRESIZE)) {

            // we want this one
            event->accept();
            returning = true;

            // set back to visible if dragging
            if (state == DRAG) {
                stateData.drag.card->invisible = false;
                stateData.drag.card->setZValue(10);
                stateData.drag.card->placing = true;
                stateData.drag.card->setDrag(false);
            }

            // end state;
            state = NONE;

            // drop it down
            updateGeometry();
            updateView();
        }

    } else if (event->type() == QEvent::GraphicsSceneMouseMove) {

        // where is the mouse now?
        QPointF pos = static_cast<QGraphicsSceneMouseEvent*>(event)->scenePos();

        // check for autoscrolling at edges
        if (state == DRAG || state == YRESIZE) edgeScroll(lasty < pos.y());

        // remember pos
        lasty = pos.y();

        if (mode == CONFIG && state == NONE) {                 // hovering

            // where am i ?
            QGraphicsItem *item = scene->itemAt(pos, view->transform());
            Card *card = static_cast<Card*>(item);

            // ignore other scene elements (e.g. charts)
            if (!cards.contains(card)) card=NULL;

            if (card) {

                // are we on the boundary of the card?
                double offx = pos.x()-card->geometry().x();
                double offy = pos.y()-card->geometry().y();

                if (yresizecursor == false && card->geometry().height()-offy < 10) {

                    yresizecursor = true;
                    setCursor(QCursor(Qt::SizeVerCursor));

                } else if (yresizecursor == true && card->geometry().height()-offy > 10) {

                    yresizecursor = false;
                    setCursor(QCursor(Qt::ArrowCursor));

                }

                if (xresizecursor == false && card->geometry().width()-offx < 10) {

                    xresizecursor = true;
                    setCursor(QCursor(Qt::SizeHorCursor));

                } else if (xresizecursor == true && card->geometry().width()-offx > 10) {

                    xresizecursor = false;
                    setCursor(QCursor(Qt::ArrowCursor));

                }

            } else {

                // not hovering over tile, so if still have a resize cursor
                // set it back to the normal arrow pointer
                if (yresizecursor || xresizecursor || cursor().shape() != Qt::ArrowCursor) {
                    xresizecursor = yresizecursor = false;
                    setCursor(QCursor(Qt::ArrowCursor));
                }
            }

        } else if (mode == CONFIG && state == DRAG && !scrolling) {          // dragging?

            // whilst mouse moves, only update geom when changed
            bool changed = false;

            // move the card being dragged
            stateData.drag.card->setPos(pos.x()-stateData.drag.offx, pos.y()-stateData.drag.offy);

            // should I move?
            QList<QGraphicsItem *> overlaps;
            foreach(QGraphicsItem *p, scene->items(pos))
                if(cards.contains(static_cast<Card*>(p)))
                    overlaps << p;

            // we always overlap with ourself, so see if more
            if (overlaps.count() > 1) {

                Card *over = static_cast<Card*>(overlaps[1]);
                if (pos.y()-over->geometry().y() > over->geometry().height()/2) {

                    // place below the one its over
                    stateData.drag.card->column = over->column;
                    stateData.drag.card->order = over->order+1;
                    for(int i=cards.indexOf(over); i< cards.count(); i++) {
                        if (i>=0 && cards[i]->column == over->column && cards[i]->order > over->order && cards[i] != stateData.drag.card) {
                            cards[i]->order += 1;
                            changed = true;
                        }
                    }

                } else {

                    // place above the one its over
                    stateData.drag.card->column = over->column;
                    stateData.drag.card->order = over->order;
                    for(int i=0; i< cards.count(); i++) {
                        if (i>=0 && cards[i]->column == over->column && cards[i]->order >= (over->order) && cards[i] != stateData.drag.card) {
                            cards[i]->order += 1;
                            changed = true;
                        }
                    }
                }

            } else {

                // columns are now variable width
                // create a new column to the right?
                int x=SPACING;
                int targetcol = -1;
                for(int i=0; i<10; i++) {
                    if (pos.x() > x && pos.x() < (x+columns[i]+SPACING)) {
                        targetcol = i;
                        break;
                    }
                    x += columns[i]+SPACING;
                }

                if (cards.last()->column < 9 && targetcol < 0) {

                    // don't keep moving - if we're already alone in column 0 then no move is needed
                    if (stateData.drag.card->column != 0 || (cards.count()>1 && cards[1]->column == 0)) {

                        // new col to left
                        for(int i=0; i< cards.count(); i++) cards[i]->column += 1;
                        stateData.drag.card->column = 0;
                        stateData.drag.card->order = 0;

                        // shift columns widths to the right
                        for(int i=9; i>0; i--) columns[i] = columns[i-1];
                        columns[0] = stateData.drag.width;

                        changed = true;
                    }

                } else if (cards.last()->column < 9 && cards.last() && cards.last()->column < targetcol) {

                    // new col to the right
                    stateData.drag.card->column = cards.last()->column + 1;
                    stateData.drag.card->order = 0;

                    // make column width same as source width
                    columns[stateData.drag.card->column] = stateData.drag.width;

                    changed = true;

                } else {

                    // add to the end of the column
                    int last = -1;
                    for(int i=0; i<cards.count() && cards[i]->column <= targetcol; i++) {
                        if (cards[i]->column == targetcol) last=i;
                    }

                    // so long as its been dragged below the last entry on the column !
                    if (last >= 0 && pos.y() > cards[last]->geometry().bottom()) {
                        stateData.drag.card->column = targetcol;
                        stateData.drag.card->order = cards[last]->order+1;
                    }

                    changed = true;
                }

            }

            if (changed) {
                // drop it down
                updateGeometry();
                updateView();
            }

        } else if (mode == CONFIG && state == YRESIZE) {

            // resize in rows, so in 75px units
            int addrows = (pos.y() - stateData.yresize.posy) / ROWHEIGHT;
            int setdeep = stateData.yresize.deep + addrows;

            //min height
            if (setdeep < 5) setdeep=5; // min of 5 rows

            stateData.yresize.card->deep = setdeep;

            // drop it down
            updateGeometry();
            updateView();

        } else if (mode == CONFIG && state == XRESIZE) {

            // multiples of 50 (smaller than margin)
            int addblocks = (pos.x() - stateData.xresize.posx) / 50;
            int setcolumn = stateData.xresize.width + (addblocks * 50);

            // min max width
            if (setcolumn < 800) setcolumn = 800;
            if (setcolumn > 4400) setcolumn = 4400;

            columns[stateData.xresize.column] = setcolumn;

            // animate
            updateGeometry();
            updateView();
        }
    }

    block = false;
    return returning;
}


void
Card::clicked()
{
    if (isVisible()) hide();
    else show();

    //if (brush.color() == GColor(CCARDBACKGROUND)) brush.setColor(Qt::red);
    //else brush.setColor(GColor(CCARDBACKGROUND));

    update(geometry());
}
