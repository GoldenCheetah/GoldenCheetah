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
#include "TabView.h"
#include "RideFileCommand.h"
#include "Utils.h"
#include "Tab.h"
#include "LTMTool.h"
#include "RideNavigator.h"
#include "ColorButton.h"
#include "MainWindow.h"
#include "UserChartData.h"

#include <limits>
#include <QScrollArea>
#include <QDialog>

UserChart::UserChart(Context *context, bool rangemode) : GcChartWindow(context), context(context), rangemode(rangemode), stale(true), last(NULL)
{
    // the config
    settingsTool = new UserChartSettings(context, rangemode, chartinfo, seriesinfo, axisinfo);
    setControls(settingsTool);

    // layout
    QVBoxLayout *main=new QVBoxLayout();
    setChartLayout(main); // we're a gcchartwindow
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);

    // the chart
    chart = new GenericChart(this, context);
    main->addWidget(chart);

    // when a ride is selected
    if (!rangemode) {
        connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(setRide(RideItem*)));
    } else {
        connect(this, SIGNAL(dateRangeChanged(DateRange)), SLOT(setDateRange(DateRange)));
    }

    // need to refresh when chart settings change
    connect(settingsTool, SIGNAL(chartConfigChanged()), this, SLOT(chartConfigChanged()));

    configChanged(0);
}

void
UserChart::configChanged(qint32)
{
    setUpdatesEnabled(false);

    setProperty("color", GColor(CPLOTBACKGROUND));
    QPalette palette;
    palette.setBrush(QPalette::Background, QBrush(GColor(CRIDEPLOTBACKGROUND)));
    setPalette(palette); // propagates to children

    setUpdatesEnabled(true);
}

void
UserChart::chartConfigChanged()
{
    if (!myRideItem) return;

    stale = true;

    if (rangemode) setDateRange(context->currentDateRange());
    else setRide(myRideItem);
}

//
// Ride selected
//
void
UserChart::setRide(RideItem *item)
{
    // not being shown so just ignore
    if (!amVisible()) { stale=true; return; }

    // make sure its not NULL etc
    if (item == NULL || item->ride() == NULL) return;

    // have we already seen it?
    if (last == item && !stale) return;

    // do we have any series to plot?!
    if (seriesinfo.count() == 0) return;

    ride = last = item;
    stale = false;

    dr=DateRange(); // always set to no range

    refresh();
 }

 void
 UserChart::setDateRange(DateRange d)
 {
    if (!amVisible()) return;

    // we don't really need to worry too much
    // about not refreshing as it doesn't get
    // called so often in trends view.
    dr = d;
    ride = myRideItem; // always current

    refresh();
 }

 void
 UserChart::refresh()
 {
    // ok, we've run out of excuses, looks like we need to plot
    chart->initialiseChart(chartinfo.title, chartinfo.type, chartinfo.animate, chartinfo.legendpos, chartinfo.stack, chartinfo.orientation);

    // now generate the series data
    for (int ii=0; ii<seriesinfo.count(); ii++) {

        // get a reference to it as we update it
        GenericSeriesInfo &series = seriesinfo[ii];

        // clear old annotations for this series
        annotations.clear();

        // re-create program (may be edited)
        if (series.user1 != NULL) delete static_cast<UserChartData*>(series.user1);
        series.user1 = new UserChartData(context, this, series.string1);
        connect(static_cast<UserChartData*>(series.user1)->program, SIGNAL(annotateLabel(QStringList&)), this, SLOT(annotateLabel(QStringList&)));

        // cast so we can work with it
        UserChartData *ucd = static_cast<UserChartData*>(series.user1);
        ucd->compute(ride, Specification(), dr);
        series.xseries = ucd->x.vector;
        series.yseries = ucd->y.vector;

        // data now generated so can add curve
        chart->addCurve(series.name, series.xseries, series.yseries, series.xname, series.yname,
                        series.labels, series.colors,
                        series.line, series.symbol, series.size, series.color, series.opacity, series.opengl, series.legend, series.datalabels);

        // add series annotations
        foreach(QStringList list, annotations) chart->annotateLabel(series.name, list);

    }

    foreach (GenericAxisInfo axis, axisinfo) {

        double min=-1,max=-1;
        if (axis.fixed) {
            min = axis.min();
            max = axis.max();
        }
        // we do NOT set align, this is managed in generic plot on our behalf
        // we also don't hide axes, so visible is always set to true
        chart->configureAxis(axis.name, true, -1, min, max, axis.type,
                             axis.labelcolor.name(), axis.axiscolor.name(), axis.log, axis.categories);
    }

    // all done
    chart->finaliseChart();
}

void
UserChart::annotateLabel(QStringList &list)
{
    annotations << list;
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
    out << "\"title\": \""       << Utils::jsonprotect(chartinfo.title) << "\",\n";
    out << "\"description\": \"" << Utils::jsonprotect(chartinfo.description) << "\",\n";
    out << "\"type\": "          << chartinfo.type << ",\n";
    out << "\"animate\": "       << (chartinfo.animate ? "true" : "false") << ",\n";
    out << "\"legendpos\": "     << chartinfo.legendpos << ",\n";
    out << "\"stack\": "         << (chartinfo.stack ? "true" : "false") << ",\n";
    out << "\"orientation\": "   << chartinfo.orientation; // note no trailing comma

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
        out << "\"name\": \""    << Utils::jsonprotect(series.name) << "\", ";
        out << "\"group\": \""   << Utils::jsonprotect(series.group) << "\", ";
        out << "\"xname\": \""   << Utils::jsonprotect(series.xname) << "\", ";
        out << "\"yname\": \""   << Utils::jsonprotect(series.yname) << "\", ";
        out << "\"program\": \"" << Utils::jsonprotect(series.string1) << "\", ";
        out << "\"line\": "      << series.line << ", ";
        out << "\"symbol\": "    << series.symbol << ", ";
        out << "\"size\": "      << series.size << ", ";
        out << "\"color\": \""   << series.color << "\", ";
        out << "\"opacity\": "   << series.opacity << ", ";
        out << "\"legend\": "    << (series.legend ? "true" : "false") << ", ";
        out << "\"opengl\": "    << (series.opengl ? "true" : "false") << ", ";
        out << "\"datalabels\": "    << (series.datalabels ? "true" : "false"); // noter no trailing comma
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

        out << "\"name\": \""       << Utils::jsonprotect(axis.name) << "\", ";
        out << "\"type\": "         << axis.type << ", ";
        out << "\"orientation\": "  << axis.orientation << ", ";
        out << "\"align\": "        << axis.align << ", ";
        out << "\"minx\": "         << axis.minx << ", ";
        out << "\"maxx\": "         << axis.maxx << ", ";
        out << "\"miny\": "         << axis.miny << ", ";
        out << "\"maxy\": "         << axis.maxy << ", ";
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
    chartinfo.title = Utils::jsonunprotect(obj["title"].toString());
    chartinfo.description = Utils::jsonunprotect(obj["description"].toString());
    chartinfo.type = obj["type"].toInt();
    chartinfo.animate = obj["animate"].toBool();
    chartinfo.legendpos = obj["legendpos"].toInt();
    chartinfo.stack = obj["stack"].toBool();
    chartinfo.orientation = obj["orientation"].toInt();

    // array of series, but userchartdata needs to be deleted
    foreach(GenericSeriesInfo series, seriesinfo)
        if (series.user1 != NULL)
            delete static_cast<UserChartData*>(series.user1);

    seriesinfo.clear();
    foreach(QJsonValue it, obj["SERIES"].toArray()) {

        // should be an array of objects
        QJsonObject series=it.toObject();

        GenericSeriesInfo add;
        add.name = Utils::jsonunprotect(series["name"].toString());
        add.group = Utils::jsonunprotect(series["group"].toString());
        add.xname = Utils::jsonunprotect(series["xname"].toString());
        add.yname = Utils::jsonunprotect(series["yname"].toString());
        add.string1 = Utils::jsonunprotect(series["program"].toString());
        add.line = series["line"].toInt();
        add.symbol = series["symbol"].toInt();
        add.size = series["size"].toDouble();
        add.color = series["color"].toString();
        add.opacity = series["opacity"].toDouble();
        add.legend = series["legend"].toBool();
        add.opengl = series["opengl"].toBool();

        // added later, may be null, if so, unset
        if (!series["datalabels"].isNull())  add.datalabels = series["datalabels"].toBool();
        else add.datalabels = false;

        seriesinfo.append(add);
    }

    // array of axes
    axisinfo.clear();
    foreach(QJsonValue it, obj["AXES"].toArray()) {

        // should be an array of objects
        QJsonObject axis=it.toObject();

        GenericAxisInfo add;
        add.name = Utils::jsonunprotect(axis["name"].toString());
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
        add.labelcolor = QColor(axis["labelcolot"].toString());
        add.axiscolor = QColor(axis["axiscolor"].toString());

        axisinfo.append(add);
    }

    // update configuration widgets to reflect new settings loaded
    settingsTool->refreshChartInfo();
    settingsTool->refreshSeriesTab();
    settingsTool->refreshAxesTab();

    // config changed...
    chartConfigChanged();
}

//
// core user chart settings
//
UserChartSettings::UserChartSettings(Context *context, bool rangemode, GenericChartInfo &chart, QList<GenericSeriesInfo> &series, QList<GenericAxisInfo> &axes) :
  context(context), rangemode(rangemode), chartinfo(chart), seriesinfo(series), axisinfo(axes), updating(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    // Chart tab
    QWidget *cs= new QWidget(this);
    tabs->addTab(cs, "Chart");

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

    cf->addRow("  ", (QWidget*)NULL);
    animate = new QCheckBox(tr("Animate"));
    cf->addRow(" ", animate);
    stack = new QCheckBox(tr("Single series per plot"));
    cf->addRow(" ", stack);

    // Series tab
    QWidget *seriesWidget = new QWidget(this);
    QVBoxLayout *seriesLayout = new QVBoxLayout(seriesWidget);
    tabs->addTab(seriesWidget, "Series");

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
    //connect(upSeriesButton, SIGNAL(clicked()), this, SLOT(moveSeriesUp()));
    //connect(downSeriesButton, SIGNAL(clicked()), this, SLOT(moveSeriesDown()));


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
    tabs->addTab(axisWidget, "Axes");

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
    connect(orientation, SIGNAL(currentIndexChanged(int)), this, SLOT(updateChartInfo()));
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

    updating=false;
}

void
UserChartSettings::updateChartInfo()
{
    // if refresh chart info is updating, just ignore for now...
    if (updating) return;

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

    // we need to refresh whenever stuff changes....
    if (refresh) emit chartConfigChanged();
}

void
UserChartSettings::addSeries()
{
    GenericSeriesInfo add;
    EditUserSeriesDialog dialog(context, rangemode, add);

    if (dialog.exec()) {

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
    QStringList names = context->tab->rideNavigator()->logicalHeadings;

    // start with just a list of functions
    list = DataFilter::builtins();

    // ridefile data series symbols
    list += RideFile::symbols();

    // add special functions (older code needs fixing !)
    list << "config(cranklength)";
    list << "config(cp)";
    list << "config(ftp)";
    list << "config(w')";
    list << "config(pmax)";
    list << "config(cv)";
    list << "config(scv)";
    list << "config(height)";
    list << "config(weight)";
    list << "config(lthr)";
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

    qSort(names.begin(), names.end(), insensitiveLessThan);

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
    cf->addRow(tr("Program"), (QWidget *)NULL);
    cf->addRow(program);
    errors = new QLabel(this);
    errors->setStyleSheet("color: red;");
    cf->addRow(errors); //


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

    color = new ColorButton(this, "Color", Qt::red);
    zz = new QHBoxLayout();
    zz->addWidget(color);
    zz->addStretch();
    zz->addWidget(datalabels);
    cf->addRow(tr("Color"), zz);

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
    setMinimumWidth(550 *dpiXFactor);
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
            "    sample {\n"
            "        # as we iterate over activity data points\n"
            "        count <- count + 1;\n"
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

    // make it wide enough
    setMinimumWidth(350 *dpiXFactor);
    setMinimumHeight(250 *dpiXFactor);

    // update gui items from Axis info
    axisname->setText(original.name);
    log->setChecked(original.log);
    int index=axistype->findData(original.type);
    if (index >=0) axistype->setCurrentIndex(index);
    else axistype->setCurrentIndex(0);
    fixed->setChecked(original.fixed);
    min->setValue(original.min());
    max->setValue(original.max());

    // connect up slots
    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
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

    accept();
}
