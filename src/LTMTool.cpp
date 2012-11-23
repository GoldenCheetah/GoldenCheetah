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

LTMTool::LTMTool(MainWindow *parent, const QDir &home, bool multi) : QWidget(parent), home(home), main(parent), active(false), _amFiltered(false)
{
    setStyleSheet("QFrame { FrameStyle = QFrame::NoFrame };"
                  "QWidget { background = Qt::white; border:0 px; margin: 2px; };");

    // get application settings
    useMetricUnits = appsettings->value(this, GC_UNIT).toString() == "Metric";

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);

#ifdef GC_HAVE_LUCENE
    searchBox = new SearchFilterBox(this, main);
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));

    mainLayout->addWidget(searchBox);
#endif

    dateRangeTree = new QTreeWidget;
    dateRangeTree->setFrameStyle(QFrame::NoFrame);
    dateRangeTree->setColumnCount(1);
    dateRangeTree->setSelectionMode(QAbstractItemView::SingleSelection);
    dateRangeTree->header()->hide();
    //dateRangeTree->setAlternatingRowColors (true);
    dateRangeTree->setIndentation(5);
    allDateRanges = new QTreeWidgetItem(dateRangeTree, ROOT_TYPE);
    allDateRanges->setText(0, tr("Date Range"));

    dateRangeTree->expandItem(allDateRanges);
    dateRangeTree->setContextMenuPolicy(Qt::CustomContextMenu);

    seasons = parent->seasons;
    resetSeasons(); // reset the season list

    metricTree = new QTreeWidget;
    metricTree->setColumnCount(1);
    if (multi)
        metricTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    else
        metricTree->setSelectionMode(QAbstractItemView::SingleSelection);
    metricTree->header()->hide();
    metricTree->setFrameStyle(QFrame::NoFrame);
    //metricTree->setAlternatingRowColors (true);
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
    skibaLTS.uname = skibaLTS.name = "Skiba Long Term Stress";
    skibaLTS.uunits = "Stress";
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
    skibaSTS.uname = skibaSTS.name = "Skiba Short Term Stress";
    skibaSTS.uunits = "Stress";
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
    skibaSB.uname = skibaSB.name = "Skiba Stress Balance";
    skibaSB.uunits = "Stress Balance";
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
    skibaSTR.uname = skibaSTR.name = "Skiba STS Ramp";
    skibaSTR.uunits = "Ramp";
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
    skibaLTR.uname = skibaLTR.name = "Skiba LTS Ramp";
    skibaLTR.uunits = "Ramp";
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
    danielsLTS.uname = danielsLTS.name = "Daniels Long Term Stress";
    danielsLTS.uunits = "Stress";
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
    danielsSTS.uname = danielsSTS.name = "Daniels Short Term Stress";
    danielsSTS.uunits = "Stress";
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
    danielsSB.uname = danielsSB.name = "Daniels Stress Balance";
    danielsSB.uunits = "Stress Balance";
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
    danielsSTR.uname = danielsSTR.name = "Daniels STS Ramp";
    danielsSTR.uunits = "Ramp";
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
    danielsLTR.uname = danielsLTR.name = "Daniels LTS Ramp";
    danielsLTR.uunits = "Ramp";
    metrics.append(danielsLTR);

    // COGGAN LTS
    MetricDetail cogganCTL;
    cogganCTL.type = METRIC_PM;
    cogganCTL.symbol = "coggan_ctl";
    cogganCTL.metric = NULL; // not a factory metric
    cogganCTL.penColor = QColor(Qt::blue);
    cogganCTL.curveStyle = QwtPlotCurve::Lines;
    cogganCTL.symbolStyle = QwtSymbol::NoSymbol;
    cogganCTL.smooth = false;
    cogganCTL.trend = false;
    cogganCTL.topN = 5;
    cogganCTL.uname = cogganCTL.name = "Coggan Chronic Training Load";
    cogganCTL.uunits = "CTL";
    metrics.append(cogganCTL);

    MetricDetail cogganATL;
    cogganATL.type = METRIC_PM;
    cogganATL.symbol = "coggan_atl";
    cogganATL.metric = NULL; // not a factory metric
    cogganATL.penColor = QColor(Qt::magenta);
    cogganATL.curveStyle = QwtPlotCurve::Lines;
    cogganATL.symbolStyle = QwtSymbol::NoSymbol;
    cogganATL.smooth = false;
    cogganATL.trend = false;
    cogganATL.topN = 5;
    cogganATL.uname = cogganATL.name = "Coggan Acute Training Load";
    cogganATL.uunits = "ATL";
    metrics.append(cogganATL);

    MetricDetail cogganTSB;
    cogganTSB.type = METRIC_PM;
    cogganTSB.symbol = "coggan_tsb";
    cogganTSB.metric = NULL; // not a factory metric
    cogganTSB.penColor = QColor(Qt::yellow);
    cogganTSB.curveStyle = QwtPlotCurve::Steps;
    cogganTSB.symbolStyle = QwtSymbol::NoSymbol;
    cogganTSB.smooth = false;
    cogganTSB.trend = false;
    cogganTSB.topN = 1;
    cogganTSB.uname = cogganTSB.name = "Coggan Training Stress Balance";
    cogganTSB.uunits = "TSB";
    metrics.append(cogganTSB);

    MetricDetail cogganSTR;
    cogganSTR.type = METRIC_PM;
    cogganSTR.symbol = "coggan_sr";
    cogganSTR.metric = NULL; // not a factory metric
    cogganSTR.penColor = QColor(Qt::darkGreen);
    cogganSTR.curveStyle = QwtPlotCurve::Steps;
    cogganSTR.symbolStyle = QwtSymbol::NoSymbol;
    cogganSTR.smooth = false;
    cogganSTR.trend = false;
    cogganSTR.topN = 1;
    cogganSTR.uname = cogganSTR.name = "Coggan STS Ramp";
    cogganSTR.uunits = "Ramp";
    metrics.append(cogganSTR);

    MetricDetail cogganLTR;
    cogganLTR.type = METRIC_PM;
    cogganLTR.symbol = "coggan_lr";
    cogganLTR.metric = NULL; // not a factory metric
    cogganLTR.penColor = QColor(Qt::darkBlue);
    cogganLTR.curveStyle = QwtPlotCurve::Steps;
    cogganLTR.symbolStyle = QwtSymbol::NoSymbol;
    cogganLTR.smooth = false;
    cogganLTR.trend = false;
    cogganLTR.topN = 1;
    cogganLTR.uname = cogganLTR.name = "Coggan LTS Ramp";
    cogganLTR.uunits = "Ramp";
    metrics.append(cogganLTR);

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
    trimpLTS.uname = trimpLTS.name = "TRIMP Long Term Stress";
    trimpLTS.uunits = "Stress";
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
    trimpSTS.uname = trimpSTS.name = "TRIMP Short Term Stress";
    trimpSTS.uunits = "Stress";
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
    trimpSB.uname = trimpSB.name = "TRIMP Stress Balance";
    trimpSB.uunits = "Stress Balance";
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
    trimpSTR.uname = trimpSTR.name = "TRIMP STS Ramp";
    trimpSTR.uunits = "Ramp";
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
    trimpLTR.uname = trimpLTR.name = "TRIMP LTS Ramp";
    trimpLTR.uunits = "Ramp";
    metrics.append(trimpLTR);

    // metadata metrics
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
            metametric.uname = metametric.name = field.name;
            metametric.uunits = "";
            metrics.append(metametric);
        }
    }

    // measures
    QList<FieldDefinition> measureDefinitions;
    QList<KeywordDefinition> keywordDefinitions; //NOTE: not used in measures.xml
    QString filename = main->home.absolutePath()+"/measures.xml";
    QString colorfield;
    if (!QFile(filename).exists()) filename = ":/xml/measures.xml";
    RideMetadata::readXML(filename, keywordDefinitions, measureDefinitions, colorfield);

    foreach (FieldDefinition field, measureDefinitions) {
        if (!sp.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            MetricDetail measure;
            measure.type = METRIC_MEASURE;
            QString underscored = field.name;
            measure.symbol = field.name; // we don't bother with '_' for measures
            measure.metric = NULL; // not a factory metric
            measure.penColor = QColor(Qt::blue);
            measure.curveStyle = QwtPlotCurve::Lines;
            measure.symbolStyle = QwtSymbol::NoSymbol;
            measure.smooth = false;
            measure.trend = false;
            measure.topN = 5;
            measure.uname = measure.name = field.name;
            measure.uunits = "";
            metrics.append(measure);
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
    ltmSplitter->setHandleWidth(1);
    ltmSplitter->setFrameStyle(QFrame::NoFrame);
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
    connect(seasons, SIGNAL(seasonsChanged()), this, SLOT(resetSeasons()));
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

// get/set date range
QString
LTMTool::_dateRange() const
{
    if (dateRangeTree->selectedItems().isEmpty()) return QString("");
    else {
        QTreeWidgetItem *which = dateRangeTree->selectedItems().first();
        return seasons->seasons[allDateRanges->indexOfChild(which)].id().toString();
    }
}

void
LTMTool::setDateRange(QString s)
{
    // clear current selection
    dateRangeTree->clearSelection();

    QUuid find(s);

    for(int i=0; i<seasons->seasons.count(); i++) {
        if (seasons->seasons[i].id() == find) {
            allDateRanges->child(i)->setSelected(true);
            return;
        }
    }
}

/*----------------------------------------------------------------------
 * Selections Made
 *----------------------------------------------------------------------*/

void
LTMTool::dateRangeTreeWidgetSelectionChanged()
{
    if (active == true) return;

    if (dateRangeTree->selectedItems().isEmpty()) dateRange = NULL;
    else {
        QTreeWidgetItem *which = dateRangeTree->selectedItems().first();
        if (which != allDateRanges) {
            dateRange = &seasons->seasons.at(allDateRanges->indexOfChild(which));
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
 * Metric settings
 *--------------------------------------------------------------------*/
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
LTMTool::selectMetric(QString symbol)
{
    for (int i=0; i<metrics.count(); i++) {
        if (metrics[i].symbol == symbol) {
            allMetrics->child(i)->setSelected(true);
        }
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

    QLabel *name = new QLabel("Name");
    QLabel *units = new QLabel("Axis Label / Units");
    userName = new QLineEdit(this);
    userName->setText(metricDetail->uname);
    userUnits = new QLineEdit(this);
    userUnits->setText(metricDetail->uunits);

    QLabel *filterlabel = new QLabel(tr("Filter"));
    filter = new QComboBox(this);         // no filter / include / exclude
    filter->addItem(tr("No filter"));
    filter->addItem(tr("Include"));
    filter->addItem(tr("Exclude"));

    QLabel *fromlabel = new QLabel(tr("From"));
    from = new QDoubleSpinBox(this);
    from->setMinimum(-9999999.99);
    from->setMaximum(9999999.99);

    QLabel *tolabel = new QLabel(tr("To"));
    to = new QDoubleSpinBox(this);
    to->setMinimum(-9999999.99);
    to->setMaximum(9999999.99);

    showOnPlot = new QCheckBox(tr("Show on plot"), this);

    QLabel *style = new QLabel("Curve");
    curveStyle = new QComboBox(this);
    curveStyle->addItem("Bar", QwtPlotCurve::Steps);
    curveStyle->addItem("Line", QwtPlotCurve::Lines);
    curveStyle->addItem("Sticks", QwtPlotCurve::Sticks);
    curveStyle->addItem("Dots", QwtPlotCurve::Dots);
    curveStyle->setCurrentIndex(curveStyle->findData(metricDetail->curveStyle));

    QLabel *stackLabel = new QLabel("Stack");
    stack = new QCheckBox("", this);
    stack->setChecked(metricDetail->stack);


    QLabel *symbol = new QLabel("Symbol");
    curveSymbol = new QComboBox(this);
    curveSymbol->addItem("None", QwtSymbol::NoSymbol);
    curveSymbol->addItem("Circle", QwtSymbol::Ellipse);
    curveSymbol->addItem("Square", QwtSymbol::Rect);
    curveSymbol->addItem("Diamond", QwtSymbol::Diamond);
    curveSymbol->addItem("Triangle", QwtSymbol::Triangle);
    curveSymbol->addItem("Cross", QwtSymbol::XCross);
    curveSymbol->addItem("Hexagon", QwtSymbol::Hexagon);
    curveSymbol->addItem("Star", QwtSymbol::Star1);
    curveSymbol->setCurrentIndex(curveSymbol->findData(metricDetail->symbolStyle));

    QLabel *color = new QLabel("Color");
    curveColor = new QPushButton(this);

    // color background...
    penColor = metricDetail->penColor;
    setButtonIcon(penColor);

    QLabel *topN = new QLabel("Highlight Best");
    showBest = new QDoubleSpinBox(this);
    showBest->setDecimals(0);
    showBest->setMinimum(0);
    showBest->setMaximum(999);
    showBest->setSingleStep(1.0);
    showBest->setValue(metricDetail->topN);

    QLabel *outN = new QLabel("Highlight Outliers");
    showOut = new QDoubleSpinBox(this);
    showOut->setDecimals(0);
    showOut->setMinimum(0);
    showOut->setMaximum(999);
    showOut->setSingleStep(1.0);
    showOut->setValue(metricDetail->topOut);

    QLabel *baseline = new QLabel("Baseline");
    baseLine = new QDoubleSpinBox(this);
    baseLine->setDecimals(0);
    baseLine->setMinimum(-999999);
    baseLine->setMaximum(999999);
    baseLine->setSingleStep(1.0);
    baseLine->setValue(metricDetail->baseline);

    curveSmooth = new QCheckBox("Smooth Curve", this);
    curveSmooth->setChecked(metricDetail->smooth);

    curveTrend = new QCheckBox("Trend Line", this);
    curveTrend->setChecked(metricDetail->trend);

    // add to grid
    grid->addWidget(name, 0,0);
    grid->addWidget(userName, 0,1);
    grid->addWidget(units, 1,0);
    grid->addWidget(userUnits, 1,1);


    grid->addWidget(filterlabel, 2,0);
    grid->addWidget(filter, 2,1);
    grid->addWidget(fromlabel, 3,0);
    grid->addWidget(from, 3,1);
    grid->addWidget(tolabel, 4,0);
    grid->addWidget(to, 4,1);
    grid->addWidget(showOnPlot, 5,1);

    grid->addWidget(style, 6,0);
    grid->addWidget(curveStyle, 6,1);
    grid->addWidget(symbol, 7,0);
    grid->addWidget(curveSymbol, 7,1);
    grid->addWidget(stackLabel, 8, 0);
    grid->addWidget(stack, 8, 1);
    grid->addWidget(color, 9,0);
    grid->addWidget(curveColor, 9,1);
    grid->addWidget(topN, 10,0);
    grid->addWidget(showBest, 10,1);
    grid->addWidget(outN, 11,0);
    grid->addWidget(showOut, 11,1);
    grid->addWidget(baseline, 12, 0);
    grid->addWidget(baseLine, 12,1);
    grid->addWidget(curveSmooth, 13,1);
    grid->addWidget(curveTrend, 14,1);

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
    metricDetail->topOut = showOut->value();
    metricDetail->baseline = baseLine->value();
    metricDetail->curveStyle = styleMap[curveStyle->currentIndex()];
    metricDetail->symbolStyle = symbolMap[curveSymbol->currentIndex()];
    metricDetail->penColor = penColor;
    metricDetail->uname = userName->text();
    metricDetail->uunits = userUnits->text();
    metricDetail->stack = stack->isChecked();
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

/*----------------------------------------------------------------------
 * Seasons stuff
 *--------------------------------------------------------------------*/

void
LTMTool::resetSeasons()
{
    if (active) return;

    QString now = _dateRange(); // remeber now

    active = true;
    int i;
    for (i=allDateRanges->childCount(); i > 0; i--) {
        delete allDateRanges->takeChild(0);
    }
    for (i=0; i <seasons->seasons.count(); i++) {
        Season season = seasons->seasons.at(i);
        QTreeWidgetItem *add = new QTreeWidgetItem(allDateRanges, season.getType());
        add->setText(0, season.getName());
    }
    setDateRange(now); // reselect now
    active = false;
}

int
LTMTool::newSeason(QString name, QDate start, QDate end, int type)
{
    seasons->newSeason(name, start, end, type);

    QTreeWidgetItem *item = new QTreeWidgetItem(USER_DATE);
    item->setText(0, name);
    allDateRanges->insertChild(0, item);
    return 0; // always add at the top
}

void
LTMTool::updateSeason(int index, QString name, QDate start, QDate end, int type)
{
    seasons->updateSeason(index, name, start, end, type);
    allDateRanges->child(index)->setText(0, name);
}

void
LTMTool::dateRangePopup(QPoint pos)
{
    QTreeWidgetItem *item = dateRangeTree->itemAt(pos);
    if (item != NULL && item->type() != ROOT_TYPE && item->type() != SYS_DATE) {

        // out of bounds or not user defined
        int index = allDateRanges->indexOfChild(item);
        if (index == -1 || index >= seasons->seasons.count()
            || seasons->seasons[index].getType() == Season::temporary)
            return;

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
    if (item != activeDateRange || active == true) return;

    int index = allDateRanges->indexOfChild(item);
    seasons->seasons[index].setName(item->text(0));

    // save changes away
    active = true;
    seasons->writeSeasons();
    active = false;

    // signal date selected changed
    //dateRangeSelected(&seasons->seasons[index]);
}

void
LTMTool::editRange()
{
    // throw up modal dialog box to edit all the season
    // fields.
    int index = allDateRanges->indexOfChild(activeDateRange);
    EditSeasonDialog dialog(main, &seasons->seasons[index]);

    if (dialog.exec()) {
        // update name
        activeDateRange->setText(0, seasons->seasons[index].getName());

        // save changes away
        active = true;
        seasons->writeSeasons();
        active = false;

        // signal its changed!
        dateRangeSelected(&seasons->seasons[index]);
    }
}

void
LTMTool::deleteRange()
{
    // now delete!
    int index = allDateRanges->indexOfChild(activeDateRange);
    delete allDateRanges->takeChild(index);
    seasons->deleteSeason(index);
}

void
LTMTool::clearFilter()
{
    filenames.clear();
    _amFiltered = false;

    emit filterChanged();
}

void
LTMTool::setFilter(QStringList files)
{
        _amFiltered = true;
        filenames = files;

        emit filterChanged();
}
