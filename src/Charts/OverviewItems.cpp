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

RPEOverviewItem::RPEOverviewItem(ChartSpace *parent, QString name) : ChartSpaceItem(parent, name)
{

    // a META widget, "RPE" using the FOSTER modified 0-10 scale
    this->type = OverviewItemType::RPE;

    sparkline = new Sparkline(this, SPARKDAYS+1, name);
    rperating = new RPErating(this, name);
}

RouteOverviewItem::RouteOverviewItem(ChartSpace *parent, QString name) : ChartSpaceItem(parent, name)
{
    this->type = OverviewItemType::ROUTE;
    routeline = new Routeline(this, name);
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

ZoneOverviewItem::ZoneOverviewItem(ChartSpace *parent, QString name, RideFile::seriestype series) : ChartSpaceItem(parent, name)
{

    this->type = OverviewItemType::ZONE;
    this->series = series;

    // basic chart setup
    chart = new QChart(this);
    chart->setBackgroundVisible(false); // draw on canvas
    chart->legend()->setVisible(false); // no legends
    chart->setTitle(""); // none wanted
    chart->setAnimationOptions(QChart::NoAnimation);

    // we have a mid sized font for chart labels etc
    chart->setFont(parent->midfont);

    // needs a set of bars
    barset = new QBarSet(tr("Time In Zone"), this);
    barset->setLabelFont(parent->midfont);

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

MetricOverviewItem::MetricOverviewItem(ChartSpace *parent, QString name, QString symbol) : ChartSpaceItem(parent, name)
{
    // metric
    this->type = OverviewItemType::METRIC;
    this->symbol = symbol;

    RideMetricFactory &factory = RideMetricFactory::instance();
    this->metric = const_cast<RideMetric*>(factory.rideMetric(symbol));
    if (metric) units = metric->units(parent->context->athlete->useMetricUnits);

    // we may plot the metric sparkline if the tile is big enough
    sparkline = new Sparkline(this, SPARKDAYS+1, name);

}

PMCOverviewItem::PMCOverviewItem(ChartSpace *parent, QString symbol) : ChartSpaceItem(parent, "")
{
    // PMC doesn't have a title as we show multiple things
    // metric
    this->type = OverviewItemType::PMC;
    this->symbol = symbol;

}

MetaOverviewItem::MetaOverviewItem(ChartSpace *parent, QString name, QString symbol) : ChartSpaceItem(parent, name)
{

    // metric or meta or pmc
    this->type = OverviewItemType::META;
    this->symbol = symbol;

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
    } else {
        sparkline = NULL;
    }
}

IntervalOverviewItem::IntervalOverviewItem(ChartSpace *parent, QString name, QString xsymbol, QString ysymbol, QString zsymbol) : ChartSpaceItem(parent, name)
{
    this->type = OverviewItemType::INTERVAL;
    this->xsymbol = xsymbol;
    this->ysymbol = ysymbol;
    this->zsymbol = zsymbol;

    // we may plot the metric sparkline if the tile is big enough
    bubble = new BubbleViz(this, "intervals");

    RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *xm = factory.rideMetric(xsymbol);
    const RideMetric *ym = factory.rideMetric(ysymbol);
    bubble->setAxisNames(xm ? xm->name() : "NA", ym ? ym->name() : "NA");
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

           v = prior->getText("RPE", "0").toDouble();
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
    value = item->getStringForSymbol(symbol, parent->context->athlete->useMetricUnits);
    if (value == "nan") value ="";
    double v = (units == tr("seconds")) ? item->getForSymbol(symbol, parent->context->athlete->useMetricUnits) : value.toDouble();
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

            if (units == tr("seconds")) v = prior->getForSymbol(symbol, parent->context->athlete->useMetricUnits);
            else {
                QString vs = prior->getStringForSymbol(symbol, parent->context->athlete->useMetricUnits);
                if (vs == "nan") vs="0";
                v = vs.toDouble();
            }

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
    if (units == tr("seconds")) {
        upper = time_to_string(max, true);
        lower = time_to_string(min, true);
        mean = time_to_string(avg, true);
    } else {
        upper = QString("%1").arg(max);
        lower = QString("%1").arg(min);

        // we need the same precision
        const RideMetricFactory &factory = RideMetricFactory::instance();
        const RideMetric *m = factory.rideMetric(symbol);

        if (m) mean = m->toString(parent->context->athlete->useMetricUnits, avg);
        else mean = QString("%1").arg(avg, 0, 'f', 0);
    }
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
IntervalOverviewItem::setData(RideItem *item)
{
    if (item == NULL || item->ride() == NULL) return;

    double minx = 999999999;
    double maxx =-999999999;
    double miny = 999999999;
    double maxy =-999999999;

    //set the x, y series
    QList<BPointF> points;
    foreach(IntervalItem *interval, item->intervals()) {
        // get the x and y VALUE
        double x = interval->getForSymbol(xsymbol, parent->context->athlete->useMetricUnits);
        double y = interval->getForSymbol(ysymbol, parent->context->athlete->useMetricUnits);
        double z = interval->getForSymbol(zsymbol, parent->context->athlete->useMetricUnits);

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
    RideItem *item = parent->parent->current;

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

BubbleViz::BubbleViz(IntervalOverviewItem *parent, QString name) : QGraphicsItem(NULL), parent(parent), name(name), hover(false)
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
