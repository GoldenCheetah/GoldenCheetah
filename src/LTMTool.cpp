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

#include "LTMTool.h"
#include "MainWindow.h"
#include "Settings.h"
#include "Units.h"
#include <assert.h>
#include <QApplication>
#include <QtGui>

// seasons support
#include "Season.h"
#include "SeasonParser.h"
#include <QXmlInputSource>
#include <QXmlSimpleReader>

// metadata support
#include "RideMetadata.h"
#include "SpecialFields.h"

LTMTool::LTMTool(MainWindow *parent, const QDir &home) : QWidget(parent), home(home), main(parent)
{
    // get application settings
    boost::shared_ptr<QSettings> appsettings = GetApplicationSettings();
    useMetricUnits = appsettings->value(GC_UNIT).toString() == "Metric";

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);

    dateRangeTree = new QTreeWidget;
    dateRangeTree->setColumnCount(1);
    dateRangeTree->setSelectionMode(QAbstractItemView::SingleSelection);
    dateRangeTree->header()->hide();
    dateRangeTree->setAlternatingRowColors (true);
    dateRangeTree->setIndentation(5);
    allDateRanges = new QTreeWidgetItem(dateRangeTree, ROOT_TYPE);
    allDateRanges->setText(0, tr("Date Range"));
    readSeasons();
    dateRangeTree->expandItem(allDateRanges);
    dateRangeTree->setContextMenuPolicy(Qt::CustomContextMenu);

    metricTree = new QTreeWidget;
    metricTree->setColumnCount(1);
    metricTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    metricTree->header()->hide();
    metricTree->setAlternatingRowColors (true);
    metricTree->setIndentation(5);
    allMetrics = new QTreeWidgetItem(metricTree, ROOT_TYPE);
    allMetrics->setText(0, tr("Metric"));
    metricTree->setContextMenuPolicy(Qt::CustomContextMenu);

    // initialise the metrics catalogue and user selector
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i = 0; i < factory.metricCount(); ++i) {

        // metrics catalogue and settings
        MetricDetail adds;
        QColor cHSV;

        adds.symbol = factory.metricName(i);
        adds.metric = factory.rideMetric(factory.metricName(i));
        qsrand(QTime::currentTime().msec());
        cHSV.setHsv((i%6)*(255/(factory.metricCount()/5)), 255, 255);
        adds.penColor = cHSV.convertTo(QColor::Rgb);
        adds.curveStyle = curveStyle(factory.metricType(i));
        adds.symbolStyle = symbolStyle(factory.metricType(i));
        adds.smooth = false;
        adds.trend = false;
        adds.topN = 5; // show top 5 by default always
        QTextEdit processHTML(adds.metric->name()); // process html encoding of(TM)
        adds.name   = processHTML.toPlainText();

        // set default for the user overiddable fields
        adds.uname  = adds.name;
        adds.uunits = adds.metric->units(useMetricUnits);

        // default units to metric name if it is blank
        if (adds.uunits == "") adds.uunits = adds.name;
        metrics.append(adds);
    }

    //
    // Add PM metrics, which are calculated over the metric dataset
    //

    // SKIBA LTS
    MetricDetail skibaLTS;
    skibaLTS.type = METRIC_PM;
    skibaLTS.symbol = "skiba_lts";
    skibaLTS.metric = NULL; // not a factory metric
    skibaLTS.penColor = QColor(Qt::blue);
    skibaLTS.curveStyle = QwtPlotCurve::Lines;
    skibaLTS.symbolStyle = QwtSymbol::NoSymbol;
    skibaLTS.smooth = false;
    skibaLTS.trend = false;
    skibaLTS.topN = 5;
    skibaLTS.uname = skibaLTS.name = tr("Skiba Long Term Stress");
    skibaLTS.uunits = tr("Stress");
    metrics.append(skibaLTS);

    MetricDetail skibaSTS;
    skibaSTS.type = METRIC_PM;
    skibaSTS.symbol = "skiba_sts";
    skibaSTS.metric = NULL; // not a factory metric
    skibaSTS.penColor = QColor(Qt::magenta);
    skibaSTS.curveStyle = QwtPlotCurve::Lines;
    skibaSTS.symbolStyle = QwtSymbol::NoSymbol;
    skibaSTS.smooth = false;
    skibaSTS.trend = false;
    skibaSTS.topN = 5;
    skibaSTS.uname = skibaSTS.name = tr("Skiba Short Term Stress");
    skibaSTS.uunits = tr("Stress");
    metrics.append(skibaSTS);

    MetricDetail skibaSB;
    skibaSB.type = METRIC_PM;
    skibaSB.symbol = "skiba_sb";
    skibaSB.metric = NULL; // not a factory metric
    skibaSB.penColor = QColor(Qt::yellow);
    skibaSB.curveStyle = QwtPlotCurve::Steps;
    skibaSB.symbolStyle = QwtSymbol::NoSymbol;
    skibaSB.smooth = false;
    skibaSB.trend = false;
    skibaSB.topN = 1;
    skibaSB.uname = skibaSB.name = tr("Skiba Stress Balance");
    skibaSB.uunits = tr("Stress Balance");
    metrics.append(skibaSB);

    MetricDetail skibaSTR;
    skibaSTR.type = METRIC_PM;
    skibaSTR.symbol = "skiba_sr";
    skibaSTR.metric = NULL; // not a factory metric
    skibaSTR.penColor = QColor(Qt::darkGreen);
    skibaSTR.curveStyle = QwtPlotCurve::Steps;
    skibaSTR.symbolStyle = QwtSymbol::NoSymbol;
    skibaSTR.smooth = false;
    skibaSTR.trend = false;
    skibaSTR.topN = 1;
    skibaSTR.uname = skibaSTR.name = tr("Skiba STS Ramp");
    skibaSTR.uunits = tr("Ramp");
    metrics.append(skibaSTR);

    MetricDetail skibaLTR;
    skibaLTR.type = METRIC_PM;
    skibaLTR.symbol = "skiba_lr";
    skibaLTR.metric = NULL; // not a factory metric
    skibaLTR.penColor = QColor(Qt::darkBlue);
    skibaLTR.curveStyle = QwtPlotCurve::Steps;
    skibaLTR.symbolStyle = QwtSymbol::NoSymbol;
    skibaLTR.smooth = false;
    skibaLTR.trend = false;
    skibaLTR.topN = 1;
    skibaLTR.uname = skibaLTR.name = tr("Skiba LTS Ramp");
    skibaLTR.uunits = tr("Ramp");
    metrics.append(skibaLTR);

    // DANIELS LTS
    MetricDetail danielsLTS;
    danielsLTS.type = METRIC_PM;
    danielsLTS.symbol = "daniels_lts";
    danielsLTS.metric = NULL; // not a factory metric
    danielsLTS.penColor = QColor(Qt::blue);
    danielsLTS.curveStyle = QwtPlotCurve::Lines;
    danielsLTS.symbolStyle = QwtSymbol::NoSymbol;
    danielsLTS.smooth = false;
    danielsLTS.trend = false;
    danielsLTS.topN = 5;
    danielsLTS.uname = danielsLTS.name = tr("Daniels Long Term Stress");
    danielsLTS.uunits = tr("Stress");
    metrics.append(danielsLTS);

    MetricDetail danielsSTS;
    danielsSTS.type = METRIC_PM;
    danielsSTS.symbol = "daniels_sts";
    danielsSTS.metric = NULL; // not a factory metric
    danielsSTS.penColor = QColor(Qt::magenta);
    danielsSTS.curveStyle = QwtPlotCurve::Lines;
    danielsSTS.symbolStyle = QwtSymbol::NoSymbol;
    danielsSTS.smooth = false;
    danielsSTS.trend = false;
    danielsSTS.topN = 5;
    danielsSTS.uname = danielsSTS.name = tr("Daniels Short Term Stress");
    danielsSTS.uunits = tr("Stress");
    metrics.append(danielsSTS);

    MetricDetail danielsSB;
    danielsSB.type = METRIC_PM;
    danielsSB.symbol = "daniels_sb";
    danielsSB.metric = NULL; // not a factory metric
    danielsSB.penColor = QColor(Qt::yellow);
    danielsSB.curveStyle = QwtPlotCurve::Steps;
    danielsSB.symbolStyle = QwtSymbol::NoSymbol;
    danielsSB.smooth = false;
    danielsSB.trend = false;
    danielsSB.topN = 1;
    danielsSB.uname = danielsSB.name = tr("Daniels Stress Balance");
    danielsSB.uunits = tr("Stress Balance");
    metrics.append(danielsSB);

    MetricDetail danielsSTR;
    danielsSTR.type = METRIC_PM;
    danielsSTR.symbol = "daniels_sr";
    danielsSTR.metric = NULL; // not a factory metric
    danielsSTR.penColor = QColor(Qt::darkGreen);
    danielsSTR.curveStyle = QwtPlotCurve::Steps;
    danielsSTR.symbolStyle = QwtSymbol::NoSymbol;
    danielsSTR.smooth = false;
    danielsSTR.trend = false;
    danielsSTR.topN = 1;
    danielsSTR.uname = danielsSTR.name = tr("Daniels STS Ramp");
    danielsSTR.uunits = tr("Ramp");
    metrics.append(danielsSTR);

    MetricDetail danielsLTR;
    danielsLTR.type = METRIC_PM;
    danielsLTR.symbol = "daniels_lr";
    danielsLTR.metric = NULL; // not a factory metric
    danielsLTR.penColor = QColor(Qt::darkBlue);
    danielsLTR.curveStyle = QwtPlotCurve::Steps;
    danielsLTR.symbolStyle = QwtSymbol::NoSymbol;
    danielsLTR.smooth = false;
    danielsLTR.trend = false;
    danielsLTR.topN = 1;
    danielsLTR.uname = danielsLTR.name = tr("Daniels LTS Ramp");
    danielsLTR.uunits = tr("Ramp");
    metrics.append(danielsLTR);

    // TRIMP LTS
    MetricDetail trimpLTS;
    trimpLTS.type = METRIC_PM;
    trimpLTS.symbol = "trimp_lts";
    trimpLTS.metric = NULL; // not a factory metric
    trimpLTS.penColor = QColor(Qt::blue);
    trimpLTS.curveStyle = QwtPlotCurve::Lines;
    trimpLTS.symbolStyle = QwtSymbol::NoSymbol;
    trimpLTS.smooth = false;
    trimpLTS.trend = false;
    trimpLTS.topN = 5;
    trimpLTS.uname = trimpLTS.name = tr("TRIMP Long Term Stress");
    trimpLTS.uunits = tr("Stress");
    metrics.append(trimpLTS);

    MetricDetail trimpSTS;
    trimpSTS.type = METRIC_PM;
    trimpSTS.symbol = "trimp_sts";
    trimpSTS.metric = NULL; // not a factory metric
    trimpSTS.penColor = QColor(Qt::magenta);
    trimpSTS.curveStyle = QwtPlotCurve::Lines;
    trimpSTS.symbolStyle = QwtSymbol::NoSymbol;
    trimpSTS.smooth = false;
    trimpSTS.trend = false;
    trimpSTS.topN = 5;
    trimpSTS.uname = trimpSTS.name = tr("TRIMP Short Term Stress");
    trimpSTS.uunits = tr("Stress");
    metrics.append(trimpSTS);

    MetricDetail trimpSB;
    trimpSB.type = METRIC_PM;
    trimpSB.symbol = "trimp_sb";
    trimpSB.metric = NULL; // not a factory metric
    trimpSB.penColor = QColor(Qt::yellow);
    trimpSB.curveStyle = QwtPlotCurve::Steps;
    trimpSB.symbolStyle = QwtSymbol::NoSymbol;
    trimpSB.smooth = false;
    trimpSB.trend = false;
    trimpSB.topN = 1;
    trimpSB.uname = trimpSB.name = tr("TRIMP Stress Balance");
    trimpSB.uunits = tr("Stress Balance");
    metrics.append(trimpSB);

    MetricDetail trimpSTR;
    trimpSTR.type = METRIC_PM;
    trimpSTR.symbol = "trimp_sr";
    trimpSTR.metric = NULL; // not a factory metric
    trimpSTR.penColor = QColor(Qt::darkGreen);
    trimpSTR.curveStyle = QwtPlotCurve::Steps;
    trimpSTR.symbolStyle = QwtSymbol::NoSymbol;
    trimpSTR.smooth = false;
    trimpSTR.trend = false;
    trimpSTR.topN = 1;
    trimpSTR.uname = trimpSTR.name = tr("TRIMP STS Ramp");
    trimpSTR.uunits = tr("Ramp");
    metrics.append(trimpSTR);

    MetricDetail trimpLTR;
    trimpLTR.type = METRIC_PM;
    trimpLTR.symbol = "trimp_lr";
    trimpLTR.metric = NULL; // not a factory metric
    trimpLTR.penColor = QColor(Qt::darkBlue);
    trimpLTR.curveStyle = QwtPlotCurve::Steps;
    trimpLTR.symbolStyle = QwtSymbol::NoSymbol;
    trimpLTR.smooth = false;
    trimpLTR.trend = false;
    trimpLTR.topN = 1;
    trimpLTR.uname = trimpLTR.name = tr("TRIMP LTS Ramp");
    trimpLTR.uunits = tr("Ramp");
    metrics.append(trimpLTR);


    SpecialFields sp;
    foreach (FieldDefinition field, main->rideMetadata()->getFields()) {
        if (!sp.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            MetricDetail metametric;
            metametric.type = METRIC_META;
            QString underscored = field.name;
            metametric.symbol = underscored.replace(" ", "_"); //XXX other special chars!!!
            metametric.metric = NULL; // not a factory metric
            metametric.penColor = QColor(Qt::blue);
            metametric.curveStyle = QwtPlotCurve::Lines;
            metametric.symbolStyle = QwtSymbol::NoSymbol;
            metametric.smooth = false;
            metametric.trend = false;
            metametric.topN = 5;
#ifdef ENABLE_METRICS_TRANSLATION
            metametric.uname = metametric.name = sp.displayName(field.name);
#else
            metametric.uname = metametric.name = field.name;
#endif
            metametric.uunits = "";
            metrics.append(metametric);
        }
    }

    // sort the list
    qSort(metrics);

    foreach(MetricDetail metric, metrics) {
        QTreeWidgetItem *add;
        add = new QTreeWidgetItem(allMetrics, METRIC_TYPE);
        add->setText(0, metric.name);
    }
    metricTree->expandItem(allMetrics);

    configChanged(); // will reset the metric tree

    ltmSplitter = new QSplitter;
    ltmSplitter->setContentsMargins(0,0,0,0);
    ltmSplitter->setOrientation(Qt::Vertical);

    mainLayout->addWidget(ltmSplitter);
    ltmSplitter->addWidget(dateRangeTree);
    ltmSplitter->setCollapsible(0, true);
    ltmSplitter->addWidget(metricTree);
    ltmSplitter->setCollapsible(1, true);

    connect(dateRangeTree,SIGNAL(itemSelectionChanged()),
            this, SLOT(dateRangeTreeWidgetSelectionChanged()));
    connect(metricTree,SIGNAL(itemSelectionChanged()),
            this, SLOT(metricTreeWidgetSelectionChanged()));
    connect(main, SIGNAL(configChanged()),
            this, SLOT(configChanged()));
    connect(dateRangeTree,SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(dateRangePopup(const QPoint &)));
    connect(metricTree,SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(metricTreePopup(const QPoint &)));
    connect(dateRangeTree,SIGNAL(itemChanged(QTreeWidgetItem *,int)),
            this, SLOT(dateRangeChanged(QTreeWidgetItem*, int)));
}

void
LTMTool::selectDateRange(int index)
{
    allDateRanges->child(index)->setSelected(true);
}

QwtPlotCurve::CurveStyle
LTMTool::curveStyle(RideMetric::MetricType type)
{
    switch (type) {

    case RideMetric::Average : return QwtPlotCurve::Lines;
    case RideMetric::Total : return QwtPlotCurve::Steps;
    case RideMetric::Peak : return QwtPlotCurve::Lines;
    default : return QwtPlotCurve::Lines;

    }
}

QwtSymbol::Style
LTMTool::symbolStyle(RideMetric::MetricType type)
{
    switch (type) {

    case RideMetric::Average : return QwtSymbol::Ellipse;
    case RideMetric::Total : return QwtSymbol::Ellipse;
    case RideMetric::Peak : return QwtSymbol::Rect;
    default : return QwtSymbol::XCross;
    }
}

void
LTMTool::configChanged()
{
}

/*----------------------------------------------------------------------
 * Selections Made
 *----------------------------------------------------------------------*/

void
LTMTool::dateRangeTreeWidgetSelectionChanged()
{
    if (dateRangeTree->selectedItems().isEmpty()) dateRange = NULL;
    else {
        QTreeWidgetItem *which = dateRangeTree->selectedItems().first();
        if (which != allDateRanges) {
            dateRange = &seasons.at(allDateRanges->indexOfChild(which));
        } else {
            dateRange = NULL;
        }
    }
    dateRangeSelected(dateRange);
}

void
LTMTool::metricTreeWidgetSelectionChanged()
{
    metricSelected();
}

/*----------------------------------------------------------------------
 * Date ranges from Seasons.xml
 *--------------------------------------------------------------------*/
void LTMTool::readSeasons()
{
    QFile seasonFile(home.absolutePath() + "/seasons.xml");
    QXmlInputSource source( &seasonFile );
    QXmlSimpleReader xmlReader;
    SeasonParser( handler );
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );
    seasons = handler.getSeasons();

    int i;
    for (i=0; i <seasons.count(); i++) {
        Season season = seasons.at(i);
        QTreeWidgetItem *add = new QTreeWidgetItem(allDateRanges, USER_DATE);
        add->setText(0, season.getName());
    }
    Season season;
    QDate today = QDate::currentDate();
    QDate eom = QDate(today.year(), today.month(), today.daysInMonth());

    // add Default Date Ranges
    season.setName(tr("All Dates"));
    season.setType(Season::temporary);
    season.setStart(QDate::currentDate().addYears(-50));
    season.setEnd(QDate::currentDate().addYears(50));
    seasons.append(season);

    season.setName(tr("This Year"));
    season.setType(Season::temporary);
    season.setStart(QDate(today.year(), 1,1));
    season.setEnd(QDate(today.year(), 12, 31));
    seasons.append(season);

    season.setName(tr("This Month"));
    season.setType(Season::temporary);
    season.setStart(QDate(today.year(), today.month(),1));
    season.setEnd(eom);
    seasons.append(season);

    season.setName(tr("This Week"));
    season.setType(Season::temporary);
    // from Mon-Sun
    QDate wstart = QDate::currentDate();
    wstart = wstart.addDays(Qt::Monday - wstart.dayOfWeek());
    QDate wend = wstart.addDays(6); // first day + 6 more
    season.setStart(wstart);
    season.setEnd(wend);
    seasons.append(season);

    season.setName(tr("Last 7 days"));
    season.setType(Season::temporary);
    season.setStart(today.addDays(-6)); // today plus previous 6
    season.setEnd(today);
    seasons.append(season);

    season.setName(tr("Last 14 days"));
    season.setType(Season::temporary);
    season.setStart(today.addDays(-13));
    season.setEnd(today);
    seasons.append(season);

    season.setName(tr("Last 28 days"));
    season.setType(Season::temporary);
    season.setStart(today.addDays(-27));
    season.setEnd(today);
    seasons.append(season);

    season.setName(tr("Last 3 months"));
    season.setType(Season::temporary);
    season.setEnd(today);
    season.setStart(today.addMonths(-3));
    seasons.append(season);

    season.setName(tr("Last 6 months"));
    season.setType(Season::temporary);
    season.setEnd(today);
    season.setStart(today.addMonths(-6));
    seasons.append(season);

    season.setName(tr("Last 12 months"));
    season.setType(Season::temporary);
    season.setEnd(today);
    season.setStart(today.addMonths(-12));
    seasons.append(season);

    for (;i <seasons.count(); i++) {
        Season season = seasons.at(i);
        QTreeWidgetItem *add = new QTreeWidgetItem(allDateRanges, SYS_DATE);
        add->setText(0, season.getName());
    }
    dateRangeTree->expandItem(allDateRanges);
}

QString
LTMTool::metricName(QTreeWidgetItem *item)
{
    int idx = allMetrics->indexOfChild(item);
    if (idx >= 0) return metrics[idx].name;
    else return tr("Unknown Metric");
}

QString
LTMTool::metricSymbol(QTreeWidgetItem *item)
{
    int idx = allMetrics->indexOfChild(item);
    if (idx >= 0) return metrics[idx].symbol;
    else return tr("Unknown Metric");
}

MetricDetail
LTMTool::metricDetails(QTreeWidgetItem *item)
{
    MetricDetail empty;
    int idx = allMetrics->indexOfChild(item);
    if (idx >= 0) return metrics[idx];
    else return empty;
}


int
LTMTool::newSeason(QString name, QDate start, QDate end, int type)
{
    Season add;
    add.setName(name);
    add.setStart(start);
    add.setEnd(end);
    add.setType(type);
    seasons.insert(0, add);

    // save changes away
    writeSeasons();

    QTreeWidgetItem *item = new QTreeWidgetItem(USER_DATE);
    item->setText(0, add.getName());
    allDateRanges->insertChild(0, item);
    return 0; // always add at the top
}

void
LTMTool::updateSeason(int index, QString name, QDate start, QDate end, int type)
{
    seasons[index].setName(name);
    seasons[index].setStart(start);
    seasons[index].setEnd(end);
    seasons[index].setType(type);
    allDateRanges->child(index)->setText(0, name);

    // save changes away
    writeSeasons();

}

void
LTMTool::dateRangePopup(QPoint pos)
{
    QTreeWidgetItem *item = dateRangeTree->itemAt(pos);
    if (item != NULL && item->type() != ROOT_TYPE && item->type() != SYS_DATE) {

        // save context
        activeDateRange = item;

        // create context menu
        QMenu menu(dateRangeTree);
        QAction *rename = new QAction(tr("Rename range"), dateRangeTree);
        QAction *edit = new QAction(tr("Edit details"), dateRangeTree);
        QAction *del = new QAction(tr("Delete range"), dateRangeTree);
        menu.addAction(rename);
        menu.addAction(edit);
        menu.addAction(del);

        // connect menu to functions
        connect(rename, SIGNAL(triggered(void)), this, SLOT(renameRange(void)));
        connect(edit, SIGNAL(triggered(void)), this, SLOT(editRange(void)));
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteRange(void)));

        // execute the menu
        menu.exec(dateRangeTree->mapToGlobal(pos));
    }
}

void
LTMTool::renameRange()
{
    // go edit the name
    activeDateRange->setFlags(activeDateRange->flags() | Qt::ItemIsEditable);
    dateRangeTree->editItem(activeDateRange, 0);
}

void
LTMTool::dateRangeChanged(QTreeWidgetItem*item, int)
{
    if (item != activeDateRange) return;

    int index = allDateRanges->indexOfChild(item);
    seasons[index].setName(item->text(0));

    // save changes away
    writeSeasons();

    // signal date selected changed
    dateRangeSelected(&seasons[index]);
}

void
LTMTool::editRange()
{
    // throw up modal dialog box to edit all the season
    // fields.
    int index = allDateRanges->indexOfChild(activeDateRange);
    EditSeasonDialog dialog(main, &seasons[index]);

    if (dialog.exec()) {
        // update name
        activeDateRange->setText(0, seasons[index].getName());

        // save changes away
        writeSeasons();

        // signal its changed!
        dateRangeSelected(&seasons[index]);
    }
}

void
LTMTool::deleteRange()
{
    // now delete!
    int index = allDateRanges->indexOfChild(activeDateRange);
    delete allDateRanges->takeChild(index);
    seasons.removeAt(index);

    // now update season.xml
    writeSeasons();
}

void
LTMTool::writeSeasons()
{
    // update seasons.xml
    QString file = QString(home.absolutePath() + "/seasons.xml");
    SeasonParser::serialize(file, seasons);
}

void
LTMTool::metricTreePopup(QPoint pos)
{
    QTreeWidgetItem *item = metricTree->itemAt(pos);
    if (item != NULL && item->type() != ROOT_TYPE) {

        // save context
        activeMetric = item;

        // create context menu
        QMenu menu(metricTree);
        QAction *color = new QAction(tr("Pick Color"), metricTree);
        QAction *edit = new QAction(tr("Settings"), metricTree);
        menu.addAction(color);
        menu.addAction(edit);

        // connect menu to functions
        connect(color, SIGNAL(triggered(void)), this, SLOT(colorPicker(void)));
        connect(edit, SIGNAL(triggered(void)), this, SLOT(editMetric(void)));

        // execute the menu
        menu.exec(metricTree->mapToGlobal(pos));
    }
}

void
LTMTool::editMetric()
{
    int index = allMetrics->indexOfChild(activeMetric);
    EditMetricDetailDialog dialog(main, &metrics[index]);

    if (dialog.exec()) {
        // notify of change
        metricSelected();
    }
}

void
LTMTool::colorPicker()
{
    int index = allMetrics->indexOfChild(activeMetric);
    QColorDialog picker(main);
    picker.setCurrentColor(metrics[index].penColor);
    QColor color = picker.getColor();

    // if we got a good color use it and notify others
    if (color.isValid()) {
        metrics[index].penColor = color;
        metricSelected();
    }
}

void
LTMTool::applySettings(LTMSettings *settings)
{
    disconnect(metricTree,SIGNAL(itemSelectionChanged()), this, SLOT(metricTreeWidgetSelectionChanged()));
    metricTree->clearSelection(); // de-select everything
    foreach (MetricDetail metricDetail, settings->metrics) {
        // get index for the symbol
        for (int i=0; i<metrics.count(); i++) {

            if (metrics[i].symbol == metricDetail.symbol) {

                // rather than copy each member one by one
                // we save the ridemetric pointer and metric type
                // copy across them all then re-instate the saved point
                RideMetric *saved = (RideMetric*)metrics[i].metric;
                int type = metrics[i].type;

                metrics[i] = metricDetail;
                metrics[i].metric = saved;
                metrics[i].type = type;

                // units may need to be adjusted if
                // usemetricUnits changed since charts.xml was
                // written
                if (saved && saved->conversion() != 1.0 &&
                    metrics[i].uunits.contains(saved->units(!useMetricUnits)))
                    metrics[i].uunits.replace(saved->units(!useMetricUnits), saved->units(useMetricUnits));
                // select it on the tool
                allMetrics->child(i)->setSelected(true);
                break;
            }
        }
    }
    connect(metricTree,SIGNAL(itemSelectionChanged()), this, SLOT(metricTreeWidgetSelectionChanged()));
    metricTreeWidgetSelectionChanged();
}

/*----------------------------------------------------------------------
 * EDIT METRIC DETAIL DIALOG
 *--------------------------------------------------------------------*/
EditMetricDetailDialog::EditMetricDetailDialog(MainWindow *mainWindow, MetricDetail *metricDetail) :
    QDialog(mainWindow, Qt::Dialog), mainWindow(mainWindow), metricDetail(metricDetail)
{
    setWindowTitle(tr("Settings"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Metric Name
    mainLayout->addSpacing(5);
    QLabel *metricName = new QLabel(metricDetail->name, this);
    metricName->setAlignment(Qt::AlignHCenter);
    QFont def;
    def.setBold(true);
    metricName->setFont(def);
    mainLayout->addWidget(metricName);
    mainLayout->addSpacing(5);

    // Grid
    QGridLayout *grid = new QGridLayout;

    QLabel *name = new QLabel(tr("Name"));
    QLabel *units = new QLabel(tr("Axis Label / Units"));
    userName = new QLineEdit(this);
    userName->setText(metricDetail->uname);
    userUnits = new QLineEdit(this);
    userUnits->setText(metricDetail->uunits);

    QLabel *style = new QLabel(tr("Curve"));
    curveStyle = new QComboBox(this);
    curveStyle->addItem(tr("Bar"), QwtPlotCurve::Steps);
    curveStyle->addItem(tr("Line"), QwtPlotCurve::Lines);
    curveStyle->addItem(tr("Sticks"), QwtPlotCurve::Sticks);
    curveStyle->addItem(tr("Dots"), QwtPlotCurve::Dots);
    curveStyle->setCurrentIndex(curveStyle->findData(metricDetail->curveStyle));


    QLabel *symbol = new QLabel(tr("Symbol"));
    curveSymbol = new QComboBox(this);
    curveSymbol->addItem(tr("None"), QwtSymbol::NoSymbol);
    curveSymbol->addItem(tr("Circle"), QwtSymbol::Ellipse);
    curveSymbol->addItem(tr("Square"), QwtSymbol::Rect);
    curveSymbol->addItem(tr("Diamond"), QwtSymbol::Diamond);
    curveSymbol->addItem(tr("Triangle"), QwtSymbol::Triangle);
    curveSymbol->addItem(tr("Cross"), QwtSymbol::XCross);
    curveSymbol->addItem(tr("Hexagon"), QwtSymbol::Hexagon);
    curveSymbol->addItem(tr("Star"), QwtSymbol::Star1);
    curveSymbol->setCurrentIndex(curveSymbol->findData(metricDetail->symbolStyle));

    QLabel *color = new QLabel(tr("Color"));
    curveColor = new QPushButton(this);

    // color background...
    penColor = metricDetail->penColor;
    setButtonIcon(penColor);

    QLabel *topN = new QLabel(tr("Highlight Best"));
    showBest = new QDoubleSpinBox(this);
    showBest->setDecimals(0);
    showBest->setMinimum(0);
    showBest->setMaximum(999);
    showBest->setSingleStep(1.0);
    showBest->setValue(metricDetail->topN);

    QLabel *baseline = new QLabel(tr("Baseline"));
    baseLine = new QDoubleSpinBox(this);
    baseLine->setDecimals(0);
    baseLine->setMinimum(-999999);
    baseLine->setMaximum(999999);
    baseLine->setSingleStep(1.0);
    baseLine->setValue(metricDetail->baseline);

    curveSmooth = new QCheckBox(tr("Smooth Curve"), this);
    curveSmooth->setChecked(metricDetail->smooth);

    curveTrend = new QCheckBox(tr("Trend Line"), this);
    curveTrend->setChecked(metricDetail->trend);

    // add to grid
    grid->addWidget(name, 0,0);
    grid->addWidget(userName, 0,1);
    grid->addWidget(units, 1,0);
    grid->addWidget(userUnits, 1,1);
    grid->addWidget(style, 2,0);
    grid->addWidget(curveStyle, 2,1);
    grid->addWidget(symbol, 3,0);
    grid->addWidget(curveSymbol, 3,1);
    grid->addWidget(color, 4,0);
    grid->addWidget(curveColor, 4,1);
    grid->addWidget(topN, 5,0);
    grid->addWidget(showBest, 5,1);
    grid->addWidget(baseline, 6, 0);
    grid->addWidget(baseLine, 6,1);
    grid->addWidget(curveSmooth, 7,1);
    grid->addWidget(curveTrend, 8,1);

    mainLayout->addLayout(grid);
    mainLayout->addStretch();

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    applyButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);
    mainLayout->addLayout(buttonLayout);

    // connect up slots
    connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(curveColor, SIGNAL(clicked()), this, SLOT(colorClicked()));
}

// uh. i hate enums when you need to modify from ints
// this is fugly and prone to error. Tied directly to the
// combo box above. all better solutions gratefully received
// but wanna get this code running for now
static QwtPlotCurve::CurveStyle styleMap[] = { QwtPlotCurve::Steps, QwtPlotCurve::Lines,
                                               QwtPlotCurve::Sticks, QwtPlotCurve::Dots };
static QwtSymbol::Style symbolMap[] = { QwtSymbol::NoSymbol, QwtSymbol::Ellipse, QwtSymbol::Rect,
                                        QwtSymbol::Diamond, QwtSymbol::Triangle, QwtSymbol::XCross,
                                        QwtSymbol::Hexagon, QwtSymbol::Star1 };
void
EditMetricDetailDialog::applyClicked()
{
    // get the values back
    metricDetail->smooth = curveSmooth->isChecked();
    metricDetail->trend = curveTrend->isChecked();
    metricDetail->topN = showBest->value();
    metricDetail->baseline = baseLine->value();
    metricDetail->curveStyle = styleMap[curveStyle->currentIndex()];
    metricDetail->symbolStyle = symbolMap[curveSymbol->currentIndex()];
    metricDetail->penColor = penColor;
    metricDetail->uname = userName->text();
    metricDetail->uunits = userUnits->text();
    accept();
}
void
EditMetricDetailDialog::cancelClicked()
{
    reject();
}

void
EditMetricDetailDialog::colorClicked()
{
    QColorDialog picker(mainWindow);
    picker.setCurrentColor(penColor);
    QColor color = picker.getColor();

    if (color.isValid()) {
        setButtonIcon(penColor=color);
    }
}

void
EditMetricDetailDialog::setButtonIcon(QColor color)
{

    // create an icon
    QPixmap pix(24, 24);
    QPainter painter(&pix);
    if (color.isValid()) {
    painter.setPen(Qt::gray);
    painter.setBrush(QBrush(color));
    painter.drawRect(0, 0, 24, 24);
    }
    QIcon icon;
    icon.addPixmap(pix);
    curveColor->setIcon(icon);
    curveColor->setContentsMargins(2,2,2,2);
    curveColor->setFixedWidth(34);
}

void
LTMTool::translateDefaultCharts(QList<LTMSettings>&charts)
{
    // Map default (english) chart name to external (Localized) name
    // New default charts need to be added to this list to be translated
    QMap<QString, QString> chartNameMap;
    chartNameMap.insert("Aerobic Power", tr("Aerobic Power"));
    chartNameMap.insert("Anaerobic Power", tr("Anaerobic Power"));
    chartNameMap.insert("Critical Power Trend", tr("Critical Power Trend"));
    chartNameMap.insert("Power & Speed Trend", tr("Power & Speed Trend"));
    chartNameMap.insert("Cardiovascular Response", tr("Cardiovascular Response"));
    chartNameMap.insert("Training Mix", tr("Training Mix"));
    chartNameMap.insert("Tempo & Threshold Time", tr("Tempo & Threshold Time"));
    chartNameMap.insert("Time & Distance", tr("Time & Distance"));
    chartNameMap.insert("Daniels Power", tr("Daniels Power"));
    chartNameMap.insert("Skiba Power", tr("Skiba Power"));
    chartNameMap.insert("Skiba PM", tr("Skiba PM"));
    chartNameMap.insert("Daniels PM", tr("Daniels PM"));

    for(int i=0; i<charts.count(); i++) {
        // Replace chart name for localized version, default to english name
        charts[i].name = chartNameMap.value(charts[i].name, charts[i].name);
        // For each metric in chart
        for (int j=0; j<charts[i].metrics.count(); j++) {
            if (charts[i].metrics[j].uname == charts[i].metrics[j].name) { // Default uname
                for (int k=0; k<metrics.count(); k++) // Look in metrics list
                    if (metrics[k].symbol == charts[i].metrics[j].symbol) { // Replace with default translated values
                        charts[i].metrics[j].name = metrics[k].name;
                        charts[i].metrics[j].uname = metrics[k].uname;
                        charts[i].metrics[j].uunits = metrics[k].uunits;
                        break;
                    }
            }
        }
    }
}
