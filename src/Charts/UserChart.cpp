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

#include "UserChart.h"

#include "Colors.h"
#include "AbstractView.h"
#include "RideFileCommand.h"
#include "Utils.h"
#include "AthleteTab.h"
#include "Views.h"
#include "AnalysisSidebar.h"
#include "LTMTool.h"
#include "RideNavigator.h"
#include "ColorButton.h"
#include "MainWindow.h"
#include "UserChartData.h"
#include "TimeUtils.h"
#include "HelpWhatsThis.h"
#include "RideItem.h"

#include <limits>
#include <QScrollArea>
#include <QDialog>

UserChart::UserChart(QWidget *parent, Context *context, bool rangemode, QString bg)
    : QWidget(parent), context(context), rangemode(rangemode), stale(true), last(NULL), ride(NULL), intervals(0), item(NULL)
{
    HelpWhatsThis *helpContents = new HelpWhatsThis(this);
    this->setWhatsThis(helpContents->getWhatsThisText(HelpWhatsThis::Chart_User));

    // the config
    settingsTool_ = new UserChartSettings(context, rangemode, chartinfo, seriesinfo, axisinfo);
    settingsTool_->hide();

    // layout
    QVBoxLayout *main=new QVBoxLayout();
    setLayout(main);
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);

    // we don't know our perspective yet...
    setPerspective(NULL);

    // the chart
    chart = new GenericChart(this, context);
    main->addWidget(chart);

    // when a ride is selected, etc we get notified by our parent
    // which is a UserChartWindow or a UserChartOverviewItem

    // but we do need to refresh when chart settings change
    connect(settingsTool_, SIGNAL(chartConfigChanged()), this, SLOT(chartConfigChanged()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalRefresh()));
    connect(context, SIGNAL(intervalsChanged()), this, SLOT(intervalRefresh()));

    // defaults, can be overriden via setBackgroundColor()
    if (bg != "") chartinfo.bgcolor = bg;
    else if (rangemode) chartinfo.bgcolor = StandardColor(CTRENDPLOTBACKGROUND).name();
    else chartinfo.bgcolor = StandardColor(CPLOTBACKGROUND).name();

    // set default background color
    configChanged(0);
}

void
UserChart::configChanged(qint32)
{
    setUpdatesEnabled(false);

    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, RGBColor(chartinfo.bgcolor));
    palette.setBrush(QPalette::Background, RGBColor(chartinfo.bgcolor));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, RGBColor(chartinfo.bgcolor) /*GCColor::alternateColor(bgcolor)*/);
    setPalette(palette);

    setAutoFillBackground(true);

    // redraw
    chartConfigChanged();

    setUpdatesEnabled(true);
}

void
UserChart::setGraphicsItem(QGraphicsItem *item)
{
    this->item = item;
    chart->setGraphicsItem(item);
}

void
UserChart::chartConfigChanged()
{
    emit userChartConfigChanged();

    if (!ride) return;

    stale = true;

    if (rangemode) setDateRange(context->currentDateRange());
    else  setRide(ride);
}

//
// Ride selected
//
void
UserChart::setRide(const RideItem *item)
{
    // not being shown so just ignore
    if (!isVisible()) { stale=true; return; }

    // make sure its not NULL etc
    if (item == NULL || const_cast<RideItem*>(item)->ride() == NULL) return;

    // have we already seen it?
    if (last == item && !stale) return;

    // do we have any series to plot?!
    if (seriesinfo.count() == 0) return;

    ride = last = item;
    stale = false;

    //dr=DateRange(); // always set to no range

    refresh();
 }

 void
 UserChart::setDateRange(DateRange d)
 {
    if (!isVisible()) return;

    // we don't really need to worry too much
    // about not refreshing as it doesn't get
    // called so often in trends view.
    dr = d;
    ride = context->currentRideItem(); // always current

    refresh();
 }

void
UserChart::intervalRefresh()
{
    // refresh on intervals change is user configurable
    if (!rangemode && chartinfo.intervalrefresh && context->currentRideItem()) {

        // are there any intervals selected?
        int ints = 0;
        foreach (IntervalItem*p, const_cast<RideItem*>(context->currentRideItem())->intervals()) {
            if (p != NULL && p->selected == true) ints++;
        }

        // if the number of intervals is selected is 0 and
        // when we last refreshed it was also 0 then just ignore
        // this signal (on ride change the ride changed signal
        // will trigger a refresh, lets not duplicate it)
        if (ints !=0 || intervals !=0) {
            refresh();
        }
    }
}

void
UserChart::refresh()
{
    if (context->currentRideItem() == NULL) return;

    if (!isVisible()) { stale=true; return; }

    // remember how many interval were selected when we refreshed
    intervals = 0;
    foreach (IntervalItem*p, const_cast<RideItem*>(context->currentRideItem())->intervals()) {
        if (p != NULL && p->selected == true) intervals++;
    }

    // ok, we've run out of excuses, looks like we need to plot
    chart->setBackgroundColor(RGBColor(chartinfo.bgcolor));
    chart->initialiseChart(chartinfo.title, chartinfo.type, chartinfo.animate, chartinfo.legendpos, chartinfo.stack, chartinfo.orientation, chartinfo.scale);

    // now generate the series data
    for (int ii=0; ii<seriesinfo.count(); ii++) {

        // get a reference to it as we update it
        GenericSeriesInfo &series = seriesinfo[ii];

        // clear old annotations for this series
        series.annotations.clear();

        // re-create program (may be edited)
        if (series.user1 != NULL) delete static_cast<UserChartData*>(series.user1);
        series.user1 = new UserChartData(context, this, series.string1, rangemode);
        connect(static_cast<UserChartData*>(series.user1)->program, SIGNAL(annotate(GenericAnnotationInfo&)), this, SLOT(annotate(GenericAnnotationInfo&)));

        // cast so we can work with it
        UserChartData *ucd = static_cast<UserChartData*>(series.user1);
        // NOTE: specification is blank so doesn't honor perspective or filters, use activity {} in program for that (!!)
        ucd->compute(const_cast<RideItem*>(ride), Specification(), dr);
        series.xseries = ucd->x.asNumeric();
        series.yseries = ucd->y.asNumeric();
        series.fseries = ucd->f.asString();

        // lookup axis info to get groupby or smoothing
        // we need to preprocess data as axis management
        // resolves for the series data
        //
        // this means group and smooth applies to user data
        // charts but not R and Python where it will need
        // to be managed within the script by the user
        int ay=GenericAxisInfo::findAxis(axisinfo, series.yname);
        int ax=GenericAxisInfo::findAxis(axisinfo, series.xname);

        // TIME smoothing (applies to y axis)
        double xsmooth=0, ysmooth=0;
        if (ax != -1 && axisinfo[ax].smooth != 0 && axisinfo[ax].type == GenericAxisInfo::TIME) ysmooth=axisinfo[ax].smooth;
        if (ay != -1 && axisinfo[ay].smooth != 0 && axisinfo[ay].type == GenericAxisInfo::TIME) xsmooth=axisinfo[ay].smooth;

        // lets pre-process the data- and might as well use sampling if losing resolution- some performance benefits here
        if (xsmooth >= 2) {
            series.xseries = Utils::smooth_sma(series.xseries, GC_SMOOTH_CENTERED, xsmooth, 3);
            series.yseries = Utils::sample(series.yseries, 3);
        }
        if (ysmooth >= 2) {
            series.yseries = Utils::smooth_sma(series.yseries, GC_SMOOTH_CENTERED, ysmooth, 3);
            series.xseries = Utils::sample(series.xseries, 3);
        }

        // DATE groupby (applies to date axis)
        int xgroupby=0, ygroupby=0;
        if (ax != -1 && axisinfo[ax].groupby != 0 && axisinfo[ax].type == GenericAxisInfo::DATERANGE) xgroupby=axisinfo[ax].groupby;
        if (ay != -1 && axisinfo[ay].groupby != 0 && axisinfo[ay].type == GenericAxisInfo::DATERANGE) ygroupby=axisinfo[ay].groupby;


        // groupBy uses pass by reference and will update what is passed
        // we update the ucd result as its used elsewhere
        if (xgroupby > 0) groupBy(xgroupby, series.aggregateby, series.xseries, series.yseries, chartinfo.type == GC_CHART_BAR || chartinfo.type == GC_CHART_STACK || chartinfo.type == GC_CHART_PERCENT);
        if (ygroupby > 0) groupBy(ygroupby, series.aggregateby, series.yseries, series.xseries, chartinfo.type == GC_CHART_BAR || chartinfo.type == GC_CHART_STACK || chartinfo.type == GC_CHART_PERCENT);

        // this is a bit of a hack, but later processing references ucd->x and y, so we
        // update them since they have been smoothed/aggregated.
        if (xsmooth >= 2 || ysmooth >= 2 || ygroupby > 0 || xgroupby > 0) {
            ucd->x.asNumeric() = series.xseries;
            ucd->y.asNumeric() = series.yseries;
        }

        // pie charts need labels
        if (chartinfo.type == GC_CHART_PIE) {
            series.labels.clear();
            foreach (GenericAxisInfo axis, axisinfo) {
                if (series.xname == axis.name) {
                    // DATERANGE values are days from 01-01-1900
                    QDateTime earliest(QDate(1900,01,01), QTime(0,0,0), Qt::LocalTime);
                    switch (axis.type) {
                        case GenericAxisInfo::TIME:
                            for(int i=0; i<ucd->x.asNumeric().count(); i++) series.labels << time_to_string(ucd->x.asNumeric()[i], true);
                            break;
                        case GenericAxisInfo::DATERANGE:
                            for(int i=0; i<ucd->x.asNumeric().count(); i++) series.labels << earliest.addDays(ucd->x.asNumeric()[i]).toString("dd MMM yy");
                            break;
                        default:
                            for(int i=0; i<ucd->x.asString().count(); i++) series.labels << ucd->x.asString()[i];
                            break;
                    }
                    break;
                }
            }
            series.colors.clear();
            QColor min=QColor(series.color);
            QColor max=GCColor::invertColor(GColor(CPLOTBACKGROUND));
            for(int i=0; i<series.labels.count(); i++) {
                QColor color = QColor(min.red() + (double(max.red()-min.red()) * (i/double(series.labels.count()))),
                              min.green() + (double(max.green()-min.green()) * (i/double(series.labels.count()))),
                              min.blue() + (double(max.blue()-min.blue()) * (i/double(series.labels.count()))));

                series.colors << color.name();
            }
        }

        // data now generated so can add curve
        chart->addCurve(series.name, series.xseries, series.yseries, series.fseries, series.xname, series.yname,
                        series.labels, series.colors, series.line, series.symbol, series.size, series.color, series.opacity,
                        series.opengl, series.legend, series.datalabels, series.fill, series.aggregateby, series.annotations);

    }

    foreach (GenericAxisInfo axis, axisinfo) {

        double min=-1,max=-1;
        if (axis.fixed) {
            min = axis.min();
            max = axis.max();
        }

        // force plot marker color for x-axis, helps to refresh after
        // config has changed. might be a better way to handle this but
        // it works for now.
        if (axis.orientation == Qt::Horizontal)  axis.labelcolor = axis.axiscolor = GColor(CPLOTMARKER);

        // on a user chart the series sets the categories for a bar chart
        // find the first series for this axis and set the categories
        // to the x series values.
        if ((chartinfo.type == GC_CHART_BAR || chartinfo.type == GC_CHART_STACK || chartinfo.type == GC_CHART_PERCENT) && axis.orientation == Qt::Horizontal) {
            // DATERANGE values are days from 01-01-1900
            QDateTime earliest(QDate(1900,01,01), QTime(0,0,0), Qt::LocalTime);

            // find the first series for axis.name
            foreach(GenericSeriesInfo s, seriesinfo) {
                if (s.xname == axis.name && s.user1) {

                    axis.categories.clear();


                    UserChartData *ucd = static_cast<UserChartData*>(s.user1);
                    switch (axis.type) {
                        case GenericAxisInfo::TIME:
                            for(int i=0; i<ucd->x.asNumeric().count(); i++) axis.categories << time_to_string(ucd->x.asNumeric()[i], true);
                            break;
                        case GenericAxisInfo::DATERANGE: {

                                // date labels, date, week #, month name, year
                                QString dateformat= "dd MMM yy";
                                int ax=GenericAxisInfo::findAxis(axisinfo, s.xname); // lookup axisinfo
                                if (ax != -1 && axisinfo[ax].groupby != 0 && axisinfo[ax].type == GenericAxisInfo::DATERANGE)  {

                                    // date format depends on how data was grouped
                                    switch(axisinfo[ax].groupby) {
                                    default:
                                    case GenericAxisInfo::NONE:
                                    case GenericAxisInfo::DAY:
                                        dateformat= "dd MMM yy";
                                        break;

                                    case GenericAxisInfo::WEEK:
                                        dateformat = "d/M"; // week commencing

                                        break;
                                    case GenericAxisInfo::MONTH:
                                        dateformat = "MMM yy";
                                        break;

                                    case GenericAxisInfo::YEAR:
                                        dateformat = "yyyy";
                                        break;
                                    }
                                }
                                // create the labels
                                for(int i=0; i<ucd->x.asNumeric().count(); i++)
                                    axis.categories << earliest.addDays(ucd->x.asNumeric()[i]).toString(dateformat);
                            }
                            break;
                        default:
                            for(int i=0; i<ucd->x.asString().count(); i++) axis.categories << ucd->x.asString()[i];
                            break;
                    }
                    axis.type =  GenericAxisInfo::CATEGORY;
                    break;
                }
            }
        }

        // we need to set max and min based upon the barsets for bar charts since
        // the generic plot only looks are series associated with an axis and we have 0 of those
        if (min==-1 && max==-1 && (chartinfo.type == GC_CHART_BAR || chartinfo.type == GC_CHART_STACK || chartinfo.type == GC_CHART_PERCENT) && axis.orientation == Qt::Vertical) {

            // loop through all the series and look at max and min y values
            bool first=true;
            foreach(GenericSeriesInfo s, seriesinfo) {
                if (s.yname == axis.name && s.user1) {
                    UserChartData *ucd = static_cast<UserChartData*>(s.user1);
                    for(int i=0; i<ucd->y.asNumeric().count(); i++) {
                        double yy = ucd->y.asNumeric()[i];
                        if (first || yy > max) max = yy;
                        if (first || yy < min) min = yy;
                        first = false;
                    }
                    break;
                }
            }
            if (min >0 && max >0) min=0;
        }

        // we do NOT set align, this is managed in generic plot on our behalf
        // we also don't hide axes, so visible is always set to true
        chart->configureAxis(axis.name, true, -1, min, max, axis.type,
                             axis.labelcolor.name(), axis.axiscolor.name(), axis.log, axis.categories);
    }

    // all done
    chart->finaliseChart();
}


// dates are always days since 1900,1,1 at this point as they
// were returned by the datafilter. later on (notably after the
// genericchart has intervened) they are converted to MSsincetheEpoch
// but at this point, we have days since Jan 1 1900
void
UserChart::groupBy(int groupby, int aggregateby, QVector<double> &xseries, QVector<double> &yseries, bool fillzero)
{
    // used to aggregate
    double aggregate=0;
    long lastgroup=0;
    long groupcount=0;

    QVector<double> newx, newy;

    QDate epoch(1900,1,1);

    for(int i=0; i<xseries.count() && i <yseries.count(); i++) {

        // value
        double value = yseries[i];

        // date
        QDate date = epoch.addDays(xseries[i]);
        long group=groupForDate(groupby, date);

        // first entry needs to set group
        if (lastgroup == 0) lastgroup = group;

        // when the group changes we save last seen value
        // assumes in date order, we could sort first (?)
        if (group != lastgroup && groupcount > 0) {

            newx << epoch.daysTo(dateForGroup(groupby, lastgroup));
            newy << aggregate;

            if (fillzero) {

                // we fill gaps with zero for some chart types
                // notably category based charts e.g. bar chart
                for(int j=lastgroup+1; j<group;j++) {
                    newx << epoch.daysTo(dateForGroup(groupby, j));
                    newy << 0;
                }
            }

            aggregate = 0;
            lastgroup = group;
            groupcount = 0;
        }

        // lets aggregate for this group
        switch (aggregateby) {
        case RideMetric::Total:
        case RideMetric::RunningTotal:
            aggregate += value;
            break;
        case RideMetric::Average:
            {
            // simple mean, no accounting for duration of ride etc
            aggregate = ((aggregate * groupcount) + value) / (groupcount+1);
            break;
            }
        case RideMetric::Low:
            if (value < aggregate) aggregate = value;
            break;
        case RideMetric::Peak:
            if (value > aggregate) aggregate = value;
            break;
        case RideMetric::MeanSquareRoot:
            if (value) aggregate = sqrt((pow(aggregate,2)*groupcount + pow(value,2)*value)/(groupcount+1));
            break;
        }

        groupcount++;
    }

    if (groupcount >0) {

        // pick up on last one
        newx << epoch.daysTo(dateForGroup(groupby, lastgroup));
        newy << aggregate;
    }

    // replace
    xseries = newx;
    yseries = newy;
}

long
UserChart::groupForDate(int groupby, QDate date)
{
    switch(groupby) {
    case GenericAxisInfo::WEEK:
        {
        // must start from 1 not zero!
        return date.toJulianDay() / 7;
        }
    case GenericAxisInfo::MONTH: return (date.year()*12) + (date.month()-1);
    case GenericAxisInfo::YEAR:  return date.year();
    case GenericAxisInfo::DAY:
    default:
        return date.toJulianDay();
    }
}

QDate
UserChart::dateForGroup(int groupby, long group)
{
    switch(groupby) {
    case GenericAxisInfo::WEEK:
        {
        // must start from 1 not zero!
        return QDate::fromJulianDay(group*7);
        }
    case GenericAxisInfo::MONTH:
        {
            int year = group/12;
            int month = group - (year*12);
            return QDate(year, month+1, 1);
        }
    case GenericAxisInfo::YEAR:
        {
            return QDate(group, 1, 1);
        }
    case GenericAxisInfo::DAY:
    default:
        return QDate::fromJulianDay(group);
    }
}

void
UserChart::annotate(GenericAnnotationInfo &annotation)
{
    QObject *from = sender();

    for(int i=0; i<seriesinfo.count(); i++) {
        if (seriesinfo[i].user1) {
            UserChartData *ucd = static_cast<UserChartData*>(seriesinfo[i].user1);
            if (ucd->program == from) {
                seriesinfo[i].annotations << annotation;
            }
        }
    }
}

//
// Read amd Write settings from a JSON document
//
QString
UserChart::settings() const
{
    QString returning;

    QTextStream out(&returning);
    out.setCodec("UTF-8");
    out << "{ ";

    // chartinfo
    out << "\"title\": \""       << Utils::jsonprotect2(chartinfo.title) << "\",\n";
    out << "\"description\": \"" << Utils::jsonprotect2(chartinfo.description) << "\",\n";
    out << "\"type\": "          << chartinfo.type << ",\n";
    out << "\"animate\": "       << (chartinfo.animate ? "true" : "false") << ",\n";
    out << "\"intervalrefresh\": "  << (chartinfo.intervalrefresh ? "true" : "false") << ",\n";
    out << "\"legendpos\": "     << chartinfo.legendpos << ",\n";
    out << "\"stack\": "         << (chartinfo.stack ? "true" : "false") << ",\n";
    out << "\"orientation\": "   << chartinfo.orientation << ",\n";
    out << "\"bgcolor\": \""       << chartinfo.bgcolor.name() << "\", \n";
    out << "\"scale\": "         << QString("%1").arg(chartinfo.scale); // note no trailing comma

    // seriesinfos
    if (seriesinfo.count()) out << ",\n\"SERIES\": [\n"; // that trailing comma
    bool first=true;
    foreach(GenericSeriesInfo series, seriesinfo) {

        // commas from last, before next one, not
        // needed for first item of course.
        if (!first) out << ",\n";
        first=false;

        // out as a json object in the "SERIES" array
        out << "{ ";
        out << "\"name\": \""    << Utils::jsonprotect2(series.name) << "\", ";
        out << "\"group\": \""   << Utils::jsonprotect2(series.group) << "\", ";
        out << "\"xname\": \""   << Utils::jsonprotect2(series.xname) << "\", ";
        out << "\"yname\": \""   << Utils::jsonprotect2(series.yname) << "\", ";
        out << "\"program\": \"" << Utils::jsonprotect2(series.string1) << "\", ";
        out << "\"line\": "      << series.line << ", ";
        out << "\"symbol\": "    << series.symbol << ", ";
        out << "\"size\": "      << series.size << ", ";
        out << "\"color\": \""   << series.color << "\", ";
        out << "\"opacity\": "   << series.opacity << ", ";
        out << "\"legend\": "    << (series.legend ? "true" : "false") << ", ";
        out << "\"opengl\": "    << (series.opengl ? "true" : "false") << ", ";
        out << "\"datalabels\": "    << (series.datalabels ? "true" : "false") << ", ";
        out << "\"aggregate\": " << static_cast<int>(series.aggregateby) << ", ";
        out << "\"fill\": "    << (series.fill ? "true" : "false"); // NOTE: no trailing comma- when adding something new
        out << "}"; // note no trailing comman
    }
    if (seriesinfo.count()) out << " ]\n"; // end of array

    // axesinfos
    if (axisinfo.count()) out << ",\n\"AXES\": [\n"; // that trailing comma
    first=true;
    foreach(GenericAxisInfo axis, axisinfo) {

        // commas from last, before next one, not
        // needed for first item of course.
        if (!first) out << ",\n";
        first=false;

        // out as a json object in the AXES array
        out << "{ ";

        out << "\"name\": \""       << Utils::jsonprotect2(axis.name) << "\", ";
        out << "\"type\": "         << axis.type << ", ";
        out << "\"orientation\": "  << axis.orientation << ", ";
        out << "\"align\": "        << axis.align << ", ";
        out << "\"minx\": "         << axis.minx << ", ";
        out << "\"maxx\": "         << axis.maxx << ", ";
        out << "\"miny\": "         << axis.miny << ", ";
        out << "\"maxy\": "         << axis.maxy << ", ";
        out << "\"smooth\": "       << axis.smooth << ", ";
        out << "\"groupby\": "      << static_cast<int>(axis.groupby) << ", ";
        out << "\"visible\": "      << (axis.visible ? "true" : "false") << ", ";
        out << "\"fixed\": "        << (axis.fixed ? "true" : "false") << ", ";
        out << "\"log\": "          << (axis.log ? "true" : "false") << ", ";
        out << "\"minorgrid\": "    << (axis.minorgrid ? "true" : "false") << ", ";
        out << "\"majorgrid\": "    << (axis.majorgrid ? "true" : "false") << ", ";
        out << "\"labelcolor\": \"" << axis.labelcolor.name() << "\", ";
        out << "\"axiscolor\": \""  << axis.axiscolor.name() << "\""; // note no trailing comma
        out << "}"; // note not trailing comma
    }
    if (axisinfo.count()) out << " ]\n"; // end of array

    // all done
    out << "}";
    out.flush();

    return returning;
}

void
UserChart::applySettings(QString x)
{
    // parse into a JSON
    QJsonDocument doc = QJsonDocument::fromJson(x.toUtf8());
    QJsonObject obj = doc.object();

    // chartinfo
    chartinfo.title = Utils::jsonunprotect2(obj["title"].toString());
    chartinfo.description = Utils::jsonunprotect2(obj["description"].toString());
    chartinfo.type = obj["type"].toInt();
    chartinfo.animate = obj["animate"].toBool();
    chartinfo.legendpos = obj["legendpos"].toInt();
    chartinfo.stack = obj["stack"].toBool();
    chartinfo.orientation = obj["orientation"].toInt();
    if (obj.contains("bgcolor")) chartinfo.bgcolor = obj["bgcolor"].toString();
    if (obj.contains("scale")) chartinfo.scale = obj["scale"].toDouble();
    else chartinfo.scale = 1.0f;
    if (obj.contains("intervalrefresh")) chartinfo.intervalrefresh = obj["intervalrefresh"].toBool();
    else chartinfo.intervalrefresh = false;

    // array of series, but userchartdata needs to be deleted
    foreach(GenericSeriesInfo series, seriesinfo)
        if (series.user1 != NULL)
            delete static_cast<UserChartData*>(series.user1);

    seriesinfo.clear();
    foreach(QJsonValue it, obj["SERIES"].toArray()) {

        // should be an array of objects
        QJsonObject series=it.toObject();

        GenericSeriesInfo add;
        add.name = Utils::jsonunprotect2(series["name"].toString());
        add.group = Utils::jsonunprotect2(series["group"].toString());
        add.xname = Utils::jsonunprotect2(series["xname"].toString());
        add.yname = Utils::jsonunprotect2(series["yname"].toString());
        add.string1 = Utils::jsonunprotect2(series["program"].toString());
        add.line = series["line"].toInt();
        add.symbol = series["symbol"].toInt();
        add.size = series["size"].toDouble();
        add.color = series["color"].toString();
        add.opacity = series["opacity"].toDouble();
        add.legend = series["legend"].toBool();
        add.opengl = series["opengl"].toBool();
        if (series.contains("aggregate")) add.aggregateby = static_cast<RideMetric::MetricType>(series["aggregate"].toInt());
        else add.aggregateby = RideMetric::Average;

        // added later, may be null, if so, unset
        if (!series["datalabels"].isNull())  add.datalabels = series["datalabels"].toBool();
        else add.datalabels = false;
        if (!series["fill"].isNull())  add.fill = series["fill"].toBool();
        else add.fill = false;

        seriesinfo.append(add);
    }

    // array of axes
    axisinfo.clear();
    foreach(QJsonValue it, obj["AXES"].toArray()) {

        // should be an array of objects
        QJsonObject axis=it.toObject();

        GenericAxisInfo add;
        add.name = Utils::jsonunprotect2(axis["name"].toString());
        add.type = static_cast<GenericAxisInfo::AxisInfoType>(axis["type"].toInt());
        add.orientation = static_cast<Qt::Orientation>(axis["orientation"].toInt());
        add.align = static_cast<Qt::AlignmentFlag>(axis["align"].toInt());
        add.minx = axis["minx"].toDouble();
        add.maxx = axis["maxx"].toDouble();
        add.miny = axis["miny"].toDouble();
        add.maxy = axis["maxy"].toDouble();
        add.visible = axis["visible"].toBool();
        add.fixed = axis["fixed"].toBool();
        add.log = axis["log"].toBool();
        add.minorgrid = axis["minorgrid"].toBool();
        add.majorgrid = axis["majorgrid"].toBool();
        add.labelcolor = QColor(axis["labelcolor"].toString());
        add.axiscolor = QColor(axis["axiscolor"].toString());
        if (axis.contains("smooth")) add.smooth = axis["smooth"].toDouble();
        else add.smooth = 0;
        if (axis.contains("groupby")) add.groupby = static_cast<GenericAxisInfo::AxisGroupBy>(axis["groupby"].toInt());
        else add.groupby = GenericAxisInfo::NONE;

        axisinfo.append(add);
    }

    // update configuration widgets to reflect new settings loaded
    settingsTool_->refreshChartInfo();
    settingsTool_->refreshSeriesTab();
    settingsTool_->refreshAxesTab();

    // config changed...
    chartConfigChanged();
}

//
// core user chart settings
//
UserChartSettings::UserChartSettings(Context *context, bool rangemode, GenericChartInfo &chart, QList<GenericSeriesInfo> &series, QList<GenericAxisInfo> &axes) :
  QWidget(NULL), context(context), rangemode(rangemode), chartinfo(chart), seriesinfo(series), axisinfo(axes), updating(false), blocked(false)
{
    HelpWhatsThis *helpConfig = new HelpWhatsThis(this);
    this->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::Chart_User));

    setMinimumHeight(500*dpiYFactor);
    setMinimumWidth(450*dpiXFactor);

    layout = new QVBoxLayout(this);
    tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    // Chart tab
    QWidget *cs= new QWidget(this);
    tabs->addTab(cs, tr("Chart"));

    QVBoxLayout *vf = new QVBoxLayout(cs);
    QFormLayout *cf = new QFormLayout();
    vf->addLayout(cf);
    vf->addStretch();

    // laying out the chart tab
    cf->addRow("  ", (QWidget*)NULL);
    title = new QLineEdit(this);
    cf->addRow(tr("Sub-title"), title);
    description = new QTextEdit(this);
    description->setAcceptRichText(false);
    cf->addRow(tr("Description"), description);

    cf->addRow("  ", (QWidget*)NULL);

    QHBoxLayout *zz = new QHBoxLayout();
    type=new QComboBox(this);
    type->addItem(tr("Line Chart"), GC_CHART_LINE);
    type->addItem(tr("Scatter Chart"), GC_CHART_SCATTER);
    type->addItem(tr("Bar Chart"), GC_CHART_BAR);
    type->addItem(tr("Stacked Bar Chart"), GC_CHART_STACK);
    type->addItem(tr("Stacked Percent Chart"), GC_CHART_PERCENT);
    type->addItem(tr("Pie Chart"), GC_CHART_PIE);
    type->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Preferred);
    zz->addWidget(type);
    zz->addStretch();
    cf->addRow(tr("Type"), zz);

    zz = new QHBoxLayout();
    orientation = new QComboBox(this);
    orientation->addItem(tr("Vertical"), Qt::Vertical);
    orientation->addItem(tr("Horizontal"), Qt::Horizontal);
    zz->addWidget(orientation);
    zz->addStretch();
    cf->addRow(tr("Layout"), zz);

    zz = new QHBoxLayout();
    legpos = new QComboBox(this);
    legpos->addItem(tr("None"), 4);
    legpos->addItem(tr("Top"), 2);
    legpos->addItem(tr("Left"), 1);
    legpos->addItem(tr("Right"), 3);
    legpos->addItem(tr("Bottom"), 0);
    legpos->setCurrentIndex(1); // top is default
    zz->addWidget(legpos);
    zz->addStretch();
    cf->addRow(tr("Legend"), zz);

    scale = new QSlider(Qt::Horizontal, this);
    scale->setTickInterval(1);
    scale->setMinimum(1);
    scale->setMaximum(18);
    scale->setSingleStep(1);
    scale->setValue(1 + ((chart.scale-1)*2)); // scale is in increments of 0.5
    cf->addRow("  ", (QWidget*)NULL);
    cf->addRow(tr("Font scaling"), scale);

    bgcolor = new ColorButton(this, tr("Background"), QColor(chartinfo.bgcolor), true);
    bgcolor->setSelectAll(true);
    cf->addRow("Background", bgcolor);
    cf->addRow("  ", (QWidget*)NULL);

    animate = new QCheckBox(tr("Animate"));
    cf->addRow(" ", animate);
    stack = new QCheckBox(tr("Single series per plot"));
    cf->addRow(" ", stack);

    intervalrefresh = new QCheckBox(tr("Refresh for intervals"), this);
    intervalrefresh->setChecked(chart.intervalrefresh);
    cf->addRow(" ", intervalrefresh);

    // Series tab
    QWidget *seriesWidget = new QWidget(this);
    QVBoxLayout *seriesLayout = new QVBoxLayout(seriesWidget);
    tabs->addTab(seriesWidget, tr("Series"));

    seriesTable = new QTableWidget(this);
#ifdef Q_OS_MAX
    seriesTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    seriesTable->setColumnCount(3);
    seriesTable->horizontalHeader()->setStretchLastSection(true);
    seriesTable->setSortingEnabled(false);
    seriesTable->verticalHeader()->hide();
    seriesTable->setShowGrid(false);
    seriesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    seriesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    seriesLayout->addWidget(seriesTable);

    QStringList header;
    header << tr("Name") << tr("Group") << tr("Y Formula");
    seriesTable->setHorizontalHeaderLabels(header);

    connect(seriesTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(seriesClicked(int, int)));

    // custom buttons
    editSeriesButton = new QPushButton(tr("Edit"));
    connect(editSeriesButton, SIGNAL(clicked()), this, SLOT(editSeries()));

    addSeriesButton = new QPushButton("+");
    connect(addSeriesButton, SIGNAL(clicked()), this, SLOT(addSeries()));

    deleteSeriesButton = new QPushButton("-");
    connect(deleteSeriesButton, SIGNAL(clicked()), this, SLOT(deleteSeries()));

#ifndef Q_OS_MAC
    upSeriesButton = new QToolButton(this);
    downSeriesButton = new QToolButton(this);
    upSeriesButton->setArrowType(Qt::UpArrow);
    downSeriesButton->setArrowType(Qt::DownArrow);
    upSeriesButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    downSeriesButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    addSeriesButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteSeriesButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    upSeriesButton = new QPushButton(tr("Up"));
    downSeriesButton = new QPushButton(tr("Down"));
#endif
    connect(upSeriesButton, SIGNAL(clicked()), this, SLOT(moveSeriesUp()));
    connect(downSeriesButton, SIGNAL(clicked()), this, SLOT(moveSeriesDown()));


    QHBoxLayout *seriesButtons = new QHBoxLayout;
    seriesButtons->setSpacing(2 *dpiXFactor);
    seriesButtons->addWidget(upSeriesButton);
    seriesButtons->addWidget(downSeriesButton);
    seriesButtons->addStretch();
    seriesButtons->addWidget(editSeriesButton);
    seriesButtons->addStretch();
    seriesButtons->addWidget(addSeriesButton);
    seriesButtons->addWidget(deleteSeriesButton);
    seriesLayout->addLayout(seriesButtons);

    // Axes tab
    // axis tab
    QWidget *axisWidget = new QWidget(this);
    QVBoxLayout *axisLayout = new QVBoxLayout(axisWidget);
    tabs->addTab(axisWidget, tr("Axes"));

    axisTable = new QTableWidget(this);
#ifdef Q_OS_MAX
    axisTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    axisTable->setColumnCount(2);
    axisTable->horizontalHeader()->setStretchLastSection(true);
    axisTable->setSortingEnabled(false);
    axisTable->verticalHeader()->hide();
    axisTable->setShowGrid(false);
    axisTable->setSelectionMode(QAbstractItemView::SingleSelection);
    axisTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    axisLayout->addWidget(axisTable);
    header.clear();
    header << tr("Name") << tr("Type");
    axisTable->setHorizontalHeaderLabels(header);

    connect(axisTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(axisClicked(int, int)));

    // custom buttons
    editAxisButton = new QPushButton(tr("Edit"));
    connect(editAxisButton, SIGNAL(clicked()), this, SLOT(editAxis()));

    // we don't allow axes to be created, since they are
    // implied by the data series that we add
    // this might change in the future

    //addAxisButton = new QPushButton("+");
    //connect(addAxisButton, SIGNAL(clicked()), this, SLOT(addAxis()));

    //deleteAxisButton = new QPushButton("-");
    //connect(deleteAxisButton, SIGNAL(clicked()), this, SLOT(deleteAxis()));

    //addAxisButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    //deleteAxisButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);


    QHBoxLayout *axisButtons = new QHBoxLayout;
    axisButtons->setSpacing(2 *dpiXFactor);
    axisButtons->addStretch();
    axisButtons->addWidget(editAxisButton);
    axisButtons->addStretch();
    //axisButtons->addWidget(addAxisButton);
    //axisButtons->addWidget(deleteAxisButton);
    axisLayout->addLayout(axisButtons);

    // watch for chartinfo edits (the series/axis stuff is managed by separate dialogs)
    connect(title, SIGNAL(textChanged(QString)), this, SLOT(updateChartInfo()));
    connect(description, SIGNAL(textChanged()), this, SLOT(updateChartInfo()));
    connect(type, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateChartInfo()));
    connect(animate, SIGNAL(stateChanged(int)), this, SLOT(updateChartInfo()));
    connect(legpos, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateChartInfo()));
    connect(stack, SIGNAL(stateChanged(int)), this, SLOT(updateChartInfo()));
    connect(intervalrefresh, SIGNAL(stateChanged(int)), this, SLOT(updateChartInfo()));
    connect(orientation, SIGNAL(currentIndexChanged(int)), this, SLOT(updateChartInfo()));
    connect(scale, SIGNAL(valueChanged(int)), this, SLOT(updateChartInfo()));
    connect(bgcolor, SIGNAL(colorChosen(QColor)), this, SLOT(updateChartInfo()));
}

void
UserChartSettings::insertLayout(QLayout *p)
{
    // e.g. UserChartOverviewItem adds a layout for editing
    //      the chart name
    layout->insertLayout(0, p);
}

void
UserChartSettings::refreshChartInfo()
{
    updating=true;

    title->setText(chartinfo.title);
    description->setText(chartinfo.description);
    int index=type->findData(chartinfo.type);
    if (index >=0) type->setCurrentIndex(index);
    else type->setCurrentIndex(0);
    animate->setChecked(chartinfo.animate);
    index=legpos->findData(chartinfo.legendpos);
    if (index >=0) legpos->setCurrentIndex(index);
    else legpos->setCurrentIndex(1);
    stack->setChecked(chartinfo.stack);
    index=orientation->findData(chartinfo.orientation);
    if (index >= 0) orientation->setCurrentIndex(index);
    else orientation->setCurrentIndex(0);
    scale->setValue(1 + ((chartinfo.scale-1)*2)); // 1-5 mapped to 1-7, where scale is 1,1.5,2,2.5,3,3.5,4,4.5,5
    intervalrefresh->setChecked(chartinfo.intervalrefresh);
    bgcolor->setColor(QColor(chartinfo.bgcolor));
    updating=false;
}

void
UserChartSettings::updateChartInfo()
{
    // if refresh chart info is updating, just ignore for now...
    if (blocked || updating) return;

    // don't interrupt as charts get zapped too soon...
    blocked = true;

    bool refresh=true;

    // keep the chart info data up to date as gui edits take place
    chartinfo.title = title->text();
    if (chartinfo.description != description->toPlainText()) refresh=false;
    chartinfo.description = description->toPlainText();
    chartinfo.type = type->itemData(type->currentIndex()).toInt();
    chartinfo.animate = animate->isChecked();
    chartinfo.legendpos = legpos->itemData(legpos->currentIndex()).toInt();
    chartinfo.stack = stack->isChecked();
    chartinfo.orientation = orientation->itemData(orientation->currentIndex()).toInt();
    chartinfo.scale = 1 + ((scale->value() - 1) * 0.5);
    chartinfo.intervalrefresh = intervalrefresh->isChecked();
    chartinfo.bgcolor = bgcolor->getColor().name();

    // we need to refresh whenever stuff changes....
    if (refresh) emit chartConfigChanged();

    blocked = false; // now we can process another...
}

void
UserChartSettings::addSeries()
{
    GenericSeriesInfo add;
    EditUserSeriesDialog dialog(context, rangemode, add);

    if (dialog.exec()) {

        // check it isn't a duplicate
        bool duplicate = false;
        QString name = add.name;
        int dup=1;
        do {
            duplicate = false;
            foreach(GenericSeriesInfo info, seriesinfo) {
                if (info.name == add.name) {
                    duplicate=true;
                    add.name= name + QString("_%1").arg(dup);
                    dup++;
                    break;
                }
            }
        } while (duplicate);

        // apply
        seriesinfo.append(add);

        // refresh
        refreshSeriesTab();
        emit chartConfigChanged();
    }
}

void
UserChartSettings::seriesClicked(int row,int)
{

    GenericSeriesInfo edit = seriesinfo[row];
    EditUserSeriesDialog dialog(context, rangemode, edit);

    if (dialog.exec()) {

        // check it isn't a duplicate
        bool duplicate = false;
        QString name = edit.name;
        int dup=1;
        do {
            duplicate = false;
            for(int i=0; i<seriesinfo.count(); i++) {

                // don't check against the one we are editing!
                if (i == row) continue;

                if (seriesinfo.at(i).name == edit.name) {
                    duplicate=true;
                    edit.name= name + QString("_%1").arg(dup);
                    dup++;
                    break;
                }
            }
        } while (duplicate);

        // apply!
        seriesinfo[row] = edit;

        // update
        refreshSeriesTab();
        emit chartConfigChanged();
    }
}

void
UserChartSettings::editSeries()
{
    // make sure something is selected
    QList<QTableWidgetItem*> items = seriesTable->selectedItems();
    if (items.count() < 1) return;
    int index = seriesTable->row(items.first());

    GenericSeriesInfo edit = seriesinfo[index];
    EditUserSeriesDialog dialog(context, rangemode, edit);

    if (dialog.exec()) {

        // check it isn't a duplicate
        bool duplicate = false;
        QString name = edit.name;
        int dup=1;
        do {
            duplicate = false;
            for(int i=0; i<seriesinfo.count(); i++) {

                // don't check against the one we are editing!
                if (i == index) continue;

                if (seriesinfo.at(i).name == edit.name) {
                    duplicate=true;
                    edit.name= name + QString("_%1").arg(dup);
                    dup++;
                    break;
                }
            }
        } while (duplicate);

        // apply!
        seriesinfo[index] = edit;

        // update
        refreshSeriesTab();
        chartConfigChanged();
    }
}

void
UserChartSettings::deleteSeries()
{
    QList<QTableWidgetItem*> items = seriesTable->selectedItems();
    if (items.count() < 1) return;

    int index = seriesTable->row(items.first());
    seriesinfo.removeAt(index);
    refreshSeriesTab();
    emit chartConfigChanged();
}

void
UserChartSettings::moveSeriesUp()
{
    QList<QTableWidgetItem*> items = seriesTable->selectedItems();
    if (items.count() < 1) return; // nothing selected

    int index = seriesTable->row(items.first());
    if (index == 0) return; // already at the top

    // move and update the table preserving selection
    seriesinfo.move(index, index-1);
    refreshSeriesTab();
    seriesTable->setCurrentCell(index-1, 0);
    emit chartConfigChanged();
}

void
UserChartSettings::moveSeriesDown()
{
    QList<QTableWidgetItem*> items = seriesTable->selectedItems();
    if (items.count() < 1) return; // nothing selected

    int index = seriesTable->row(items.first());
    if (index == seriesinfo.count()-1) return; // already at the bottom

    // move and update the table preserving selection
    seriesinfo.move(index, index+1);
    refreshSeriesTab();
    seriesTable->setCurrentCell(index+1, 0);
    emit chartConfigChanged();
}

void
UserChartSettings::refreshSeriesTab()
{

    // clear then repopulate custom table settings to reflect
    seriesTable->clear();

    // get headers back
    QStringList header;
    header << tr("Name") << tr("Group") << tr("Y Formula");
    seriesTable->setHorizontalHeaderLabels(header);

    //QTableWidgetItem *selected = NULL;

    // now lets add a row for each metric
    seriesTable->setRowCount(seriesinfo.count());
    int i=0;
    foreach (GenericSeriesInfo series, seriesinfo) {

        QTableWidgetItem *t = new QTableWidgetItem();
        t->setText(series.name);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        seriesTable->setItem(i,0,t);

        t = new QTableWidgetItem();
        t->setText(series.group);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        seriesTable->setItem(i,1,t);

        t = new QTableWidgetItem();
        t->setText(series.string1); // yformula
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        seriesTable->setItem(i,2,t);

        // keep the selected item from previous step (relevant for moving up/down)
        //if (indexSelectedItem == i) {
        //    selected = t;
        //}

        i++;
    }

    // axes are implied from series setup
    refreshAxesTab();

    //if (selected) seriesTable->setCurrentItem(selected);
}

void
UserChartSettings::addAxis()
{
    GenericAxisInfo add;
    EditUserAxisDialog dialog(context, add);

    if (dialog.exec()) {

        // apply
        axisinfo.append(add);

        // refresh
        refreshAxesTab();
        emit chartConfigChanged();
    }
}

void
UserChartSettings::axisClicked(int row,int)
{

    GenericAxisInfo edit = axisinfo[row];
    EditUserAxisDialog dialog(context, edit);

    if (dialog.exec()) {

        // apply!
        axisinfo[row] = edit;

        // update
        refreshAxesTab();
        emit chartConfigChanged();
    }
}

void
UserChartSettings::editAxis()
{
    // make sure something is selected
    QList<QTableWidgetItem*> items = axisTable->selectedItems();
    if (items.count() < 1) return;
    int index = axisTable->row(items.first());

    GenericAxisInfo edit = axisinfo[index];
    EditUserAxisDialog dialog(context, edit);

    if (dialog.exec()) {

        // apply!
        axisinfo[index] = edit;

        // update
        refreshAxesTab();
        emit chartConfigChanged();
    }
}
void
UserChartSettings::deleteAxis()
{
// might want to check if this is sensible..... XXX todo

    QList<QTableWidgetItem*> items = axisTable->selectedItems();
    if (items.count() < 1) return;

    int index = axisTable->row(items.first());
    axisinfo.removeAt(index);
    refreshAxesTab();
    emit chartConfigChanged();
}

void
UserChartSettings::refreshAxesTab()
{
    // first lets create a list of what we think we need
    QList<GenericAxisInfo> want;
    QStringList have;
    foreach(GenericSeriesInfo series, seriesinfo) {

        // xaxis
        if (!have.contains(series.xname)) {
            GenericAxisInfo found;
            found.name = series.xname;
            found.orientation = Qt::Horizontal;
            foreach(GenericAxisInfo axis, axisinfo) {
                if (axis.name == series.xname) {
                    found = axis;
                    break;
                }
            }
            found.axiscolor = GColor(CPLOTMARKER);
            found.labelcolor = GColor(CPLOTMARKER);
            want << found;
            have << series.xname;
        }

        // yaxis
        if (!have.contains(series.yname)) {
            GenericAxisInfo found;
            found.name = series.yname;
            found.orientation = Qt::Vertical;
            foreach(GenericAxisInfo axis, axisinfo) {
                if (axis.name == series.yname) {
                    found = axis;
                    break;
                }
            }
            found.axiscolor = QColor(series.color);// string vs qcolor .. but why? xxx fixme
            found.labelcolor = QColor(series.color);// string vs qcolor .. but why? xxx fixme
            want << found;
            have << series.yname;
        }
    }
    axisinfo = want;

    // just refresh based upon axisinfo
    axisTable->clear();

    // get headers back
    QStringList header;
    header << tr("Name") << tr("Type");
    axisTable->setHorizontalHeaderLabels(header);

    //QTableWidgetItem *selected = NULL;

    // now lets add a row for each metric
    axisTable->setRowCount(axisinfo.count());
    int i=0;
    foreach (GenericAxisInfo axis, axisinfo) {

        QTableWidgetItem *t = new QTableWidgetItem();
        t->setText(axis.name);
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        axisTable->setItem(i,0,t);

        t = new QTableWidgetItem();
        t->setText(GenericAxisInfo::axisTypeDescription(axis.type));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        axisTable->setItem(i,1,t);

        // keep the selected item from previous step (relevant for moving up/down)
        //if (indexSelectedItem == i) {
        //    selected = t;
        //}

        i++;
    }

    //if (selected) seriesTable->setCurrentItem(selected);
}

static bool insensitiveLessThan(const QString &a, const QString &b)
{
    return a.toLower() < b.toLower();
}
//
// Data series settings
//
EditUserSeriesDialog::EditUserSeriesDialog(Context *context, bool rangemode, GenericSeriesInfo &info)
    : QDialog(context->mainWindow, Qt::Dialog), context(context), original(info)
{
    setWindowTitle(tr("Edit Data Series"));

    // set the dialog
    QVBoxLayout *main = new QVBoxLayout(this);
    main->setMargin(10*dpiXFactor);
    QHBoxLayout *hf = new QHBoxLayout();
    QVBoxLayout *pl = new QVBoxLayout();
    QFormLayout *cf = new QFormLayout();
    hf->addLayout(pl);
    hf->addLayout(cf);
    hf->setStretchFactor(pl, 2);
    hf->setStretchFactor(cf, 1);
    main->addLayout(hf);
    //main->addStretch();
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    okButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);
    main->addLayout(buttonLayout);

    cf->addRow(" ", (QWidget *)NULL);
    name = new QLineEdit(this);
    QLabel *glabel=new QLabel("Group");
    groupname = new QLineEdit(this);
    QHBoxLayout *zz = new QHBoxLayout();
    zz->addWidget(name);
    zz->addStretch();
    zz->addWidget(glabel);
    zz->addWidget(groupname);

    cf->addRow(tr("Series Name"), zz);

    QList<QString> list;
    QString last;
    SpecialFields sp;

    // get sorted list
    QStringList names = context->rideNavigator->logicalHeadings;

    // start with just a list of functions
    list = DataFilter::builtins(context);

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
    program = new DataFilterEdit(this, context);
    program->setMinimumHeight(250 * dpiXFactor); // give me some space!
    DataFilterCompleter *completer = new DataFilterCompleter(list, this);
    program->setCompleter(completer);
    pl->addWidget(program);
    errors = new QLabel(this);
    errors->setWordWrap(true);
    errors->setStyleSheet("color: red;");
    pl->addWidget(errors); //


    yname = new QLineEdit(this);
    zz = new QHBoxLayout();
    zz->addWidget(yname);
    zz->addStretch();
    cf->addRow("Y units", zz);
    xname = new QLineEdit(this);
    zz = new QHBoxLayout();
    zz->addWidget(xname);
    zz->addStretch();
    cf->addRow("X units", zz);

    cf->addRow(" ", (QWidget *)NULL);
    aggregate = new QComboBox(this);
    aggregate->addItem("Sum");
    aggregate->addItem("Average");
    aggregate->addItem("Peak");
    aggregate->addItem("Low");
    aggregate->addItem("Running Total");
    aggregate->addItem("Mean Square Root");
    aggregate->addItem("Std Deviation");
    aggregate->setCurrentIndex(info.aggregateby);
    aggregate->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    cf->addRow("Aggregate", aggregate);
    cf->addRow(" ", (QWidget *)NULL);

    line = new QComboBox(this);
    line->addItem(tr("None"), static_cast<int>(Qt::PenStyle::NoPen));
    line->addItem(tr("Solid"), static_cast<int>(Qt::PenStyle::SolidLine));
    line->addItem(tr("Dashed"), static_cast<int>(Qt::PenStyle::DashLine));
    line->addItem(tr("Dot"), static_cast<int>(Qt::PenStyle::DotLine));
    line->addItem(tr("Dash Dot"), static_cast<int>(Qt::PenStyle::DashDotLine));
    opengl = new QCheckBox(tr("Fast Graphics"));
    legend = new QCheckBox(tr("Show on Legend"));
    datalabels = new QCheckBox(tr("Show Data Labels"));

    zz = new QHBoxLayout();
    zz->addWidget(line);
    zz->addStretch();
    zz->addWidget(opengl);
    cf->addRow(tr("Line Style"), zz);

    symbol = new QComboBox(this);
    symbol->addItem(tr("None"), 0);
    symbol->addItem(tr("Circle"), 1);
    symbol->addItem(tr("Rectangle"), 2);
    zz = new QHBoxLayout();
    zz->addWidget(symbol);
    zz->addStretch();
    zz->addWidget(legend);
    cf->addRow(tr("Symbol"), zz);

    // we allow colors using the GC palette and default to power colors
    color = new ColorButton(this, "Color", QColor(1,1,CPOWER), true, false);
    zz = new QHBoxLayout();
    zz->addWidget(color);
    zz->addStretch();
    zz->addWidget(datalabels);
    cf->addRow(tr("Color"), zz);

    fill = new QCheckBox(this);
    cf->addRow(tr("Fill curve"), fill);

    opacity = new QSpinBox(this);
    opacity->setRange(1,100);
    opacity->setValue(100);// default
    zz = new QHBoxLayout();
    zz->addWidget(opacity);
    zz->addStretch();
    cf->addRow(tr("Opacity (%)"), zz);

    size = new QDoubleSpinBox(this);
    zz = new QHBoxLayout();
    zz->addWidget(size);
    zz->addStretch();
    cf->addRow(tr("Size/Width"), zz);

    //QLineEdit *labels, *colors; XXX TODO

    // make it wide enough
    setMinimumWidth(850 *dpiXFactor);
    setMinimumHeight(600 *dpiXFactor);

    // update gui items from series info
    name->setText(original.name);
    groupname->setText(original.group);

    // lets put a sensible default in to guide the user
    if (original.string1 == "") {

        if (rangemode) {

            // working for a date range
            original.string1 =
            "{\n"
            "    init {\n"
            "        xx<-c();\n"
            "        yy<-c();\n"
            "        activities<-0;\n"
            "    }\n"
            "\n"
            "    activity {\n"
            "        # as we iterate over activities\n"
            "        activities <- activities + 1;\n"
            "    }\n"
            "\n"
            "    finalise {\n"
            "        # we just fetch metrics at the end\n"
            "        xx <- metrics(date);\n"
            "        yy <- metrics(BikeStress);\n"
            "    }\n"
            "\n"
            "    x { xx; }\n"
            "    y { yy; }\n"
            "}\n";

        } else {

            original.string1=
            "{\n"
            "    init {\n"
            "        xx<-c();\n"
            "        yy<-c();\n"
            "        count<-0;\n"
            "    }\n"
            "\n"
            "    relevant {\n"
            "        Data contains \"P\";\n"
            "    }\n"
            "\n"
            "    finalise {\n"
            "        # we just fetch samples at end\n"
            "        xx <- samples(SECS);\n"
            "        yy <- samples(POWER);\n"
            "    }\n"
            "\n"
            "    x { xx; }\n"
            "    y { yy; }\n"
            "}\n";
        }
    }
    program->setText(original.string1);
    xname->setText(original.xname);
    yname->setText(original.yname);

    //labels;
    //colors;

    int index = line->findData(original.line);
    if (index >= 0) line->setCurrentIndex(index);
    else line->setCurrentIndex(0);

    index = symbol->findData(original.symbol);
    if (index >= 0) symbol->setCurrentIndex(index);
    else symbol->setCurrentIndex(1); // circle if not set

    size->setValue(original.size);
    color->setColor(QColor(original.color));
    //original.opacity = opacity->value();
    opengl->setChecked(original.opengl != 0);
    legend->setChecked(original.legend != 0);
    datalabels->setChecked(original.datalabels != 0);
    opacity->setValue(original.opacity);
    fill->setChecked(original.fill);
    // update the source

    // connect up slots
    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(program, SIGNAL(syntaxErrors(QStringList&)), this, SLOT(setErrors(QStringList&)));
}

void
EditUserSeriesDialog::setErrors(QStringList &list)
{
    errors->setText(list.join(";"));

}

void
EditUserSeriesDialog::cancelClicked()
{
    reject();
}

void
EditUserSeriesDialog::okClicked()
{
    // update seriesinfo from gui components
    original.name = name->text();
    original.group = groupname->text();
    original.string1 = program->toPlainText();
    original.xname = xname->text();
    original.yname = yname->text();
    //labels;
    //colors;
    original.line = line->itemData(line->currentIndex()).toInt();
    original.symbol = symbol->itemData(symbol->currentIndex()).toInt();
    original.size = size->value();
    original.color = color->getColor().name();
    original.opengl = opengl->isChecked();
    original.opacity = opacity->value();
    original.legend = legend->isChecked();
    original.datalabels = datalabels->isChecked();
    original.fill = fill->isChecked();
    original.aggregateby = static_cast<RideMetric::MetricType>(aggregate->currentIndex());
    // update the source

    accept();
}

//
// Axis settings
//
EditUserAxisDialog::EditUserAxisDialog(Context *context, GenericAxisInfo &info)
    : QDialog(context->mainWindow, Qt::Dialog), context(context), original(info)
{
    setWindowTitle(tr("Edit Axis"));

    // set the dialog
    QVBoxLayout *main = new QVBoxLayout(this);
    main->setMargin(10*dpiXFactor);
    QFormLayout *cf = new QFormLayout();
    main->addLayout(cf);
    main->addStretch();
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    okButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(okButton);
    main->addLayout(buttonLayout);

    cf->addRow(" ", (QWidget *)NULL);
    axisname = new QLineEdit(this);
    axisname->setEnabled(false); // can't edit- these come from the series!
    cf->addRow(tr("Units"), axisname);
    axistype = new QComboBox();
    for(int i=0; i< GenericAxisInfo::LAST; i++)
        axistype->addItem(GenericAxisInfo::axisTypeDescription(static_cast<GenericAxisInfo::AxisInfoType>(i)), i);
    log = new QCheckBox(tr("Logarithmic Scale"));
    QHBoxLayout *zz=new QHBoxLayout();
    zz->addWidget(axistype);
    zz->addStretch();
    cf->addRow(tr("Type"), zz);
    cf->addRow(tr(" "), log);

    fixed = new QCheckBox(tr("Fixed"));
    min = new QDoubleSpinBox();
    min->setDecimals(2);
    min->setMinimum(-99999);
    min->setMaximum(99999);
    max = new QDoubleSpinBox();
    max->setDecimals(2);
    max->setMinimum(-99999);
    max->setMaximum(99999);
    zz=new QHBoxLayout();
    zz->addWidget(fixed);
    zz->addWidget(new QLabel(tr(" from ")));
    zz->addWidget(min);
    zz->addWidget(new QLabel(tr(" to ")));
    zz->addWidget(max);
    zz->addStretch();
    cf->addRow(tr("Range"),zz);
    cf->addRow(tr(" "), new QWidget(this));

    // smoothing of series with a time axis - otherwise not shown
    smooth = new QSlider(this);
    smooth->setRange(0,60); // 0-60s smoothing
    smooth->setSingleStep(1);
    smooth->setOrientation(Qt::Horizontal);
    smoothlabel = new QLabel("Smoothing", this);
    cf->addRow(smoothlabel, smooth);

    // group by of series with a date axis - otherwise not shown
    groupby = new QComboBox(this);
    groupby->addItem("None");
    groupby->addItem("Day");
    groupby->addItem("Week");
    groupby->addItem("Month");
    groupby->addItem("Year");
    groupby->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    groupbylabel = new QLabel("Group By", this);
    cf->addRow(groupbylabel, groupby);

    // make it wide enough
    setMinimumWidth(350 *dpiXFactor);
    setMinimumHeight(250 *dpiXFactor);

    // update gui items from Axis info
    int index=axistype->findData(original.type);
    if (index >=0) axistype->setCurrentIndex(index);
    else axistype->setCurrentIndex(0);

    // show / hide widgets on current config, setting defaults
    setWidgets();

    axisname->setText(original.name);
    log->setChecked(original.log);
    fixed->setChecked(original.fixed);
    min->setValue(original.min());
    max->setValue(original.max());
    smooth->setValue(original.smooth);
    groupby->setCurrentIndex(original.groupby);

    // connect axis type selection to widget selector
    connect(axistype, SIGNAL(currentIndexChanged(int)), this, SLOT(setWidgets()));

    // connect up slots
    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));

}

void
EditUserAxisDialog::setWidgets()
{
    // set widgets shown/hidden on the basis of controlling config

    // first- axistype determines the use of smoothing or groupby
    switch(axistype->currentIndex()) {
        case GenericAxisInfo::DATERANGE:
            groupby->show();
            groupbylabel->show();
            smooth->setValue(0);
            smooth->hide();
            smoothlabel->hide();
            break;

        case GenericAxisInfo::TIME:
            smooth->setValue(5); // default to 5 seconds smoothing user can override
            smooth->show();
            smoothlabel->show();
            groupby->setCurrentIndex(0);
            groupby->hide();
            groupbylabel->hide();
            break;

        default:
            groupby->setCurrentIndex(0);
            smooth->setValue(0);
            smooth->hide();
            smoothlabel->hide();
            groupby->hide();
            groupbylabel->hide();
            break; // do nothing
    }
}

void
EditUserAxisDialog::cancelClicked()
{
    reject();
}

void
EditUserAxisDialog::okClicked()
{
    // update Axisinfo from gui components
    original.type= static_cast<GenericAxisInfo::AxisInfoType>(axistype->itemData(axistype->currentIndex()).toInt());
    original.log= log->isChecked();
    original.fixed = fixed->isChecked();
    if (original.orientation==Qt::Horizontal){
        original.minx= min->value();
        original.maxx= max->value();
    } else {
        original.miny= min->value();
        original.maxy= max->value();
    }
    original.smooth = smooth->value();
    original.groupby = static_cast<GenericAxisInfo::AxisGroupBy>(groupby->currentIndex());

    accept();
}
