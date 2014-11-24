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
#include "Context.h"
#include "Athlete.h"
#include "Settings.h"
#include "Units.h"
#include <QApplication>
#include <QtGui>

// charts.xml support
#include "LTMChartParser.h"

// seasons.xml support
#include "Season.h"
#include "SeasonParser.h"
#include <QXmlInputSource>
#include <QXmlSimpleReader>

// metadata.xml support
#include "RideMetadata.h"
#include "SpecialFields.h"

// PDModel estimate support
#include "PDModel.h"

LTMTool::LTMTool(Context *context, LTMSettings *settings) : QWidget(context->mainWindow), settings(settings), context(context), active(false), _amFiltered(false), editing(false)
{
    setStyleSheet("QFrame { FrameStyle = QFrame::NoFrame };"
                  "QWidget { background = Qt::white; border:0 px; margin: 2px; };");

    // get application settings
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);

    QWidget *basicsettings = new QWidget(this);
    mainLayout->addWidget(basicsettings);
    QFormLayout *basicsettingsLayout = new QFormLayout(basicsettings);
    basicsettingsLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

#ifdef GC_HAVE_LUCENE
    searchBox = new SearchFilterBox(this, context);
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));

    basicsettingsLayout->addRow(new QLabel(tr("Filter")), searchBox);
    basicsettingsLayout->addRow(new QLabel(tr(""))); // spacing
#endif

    // Basic Controls
    QWidget *basic = new QWidget(this);
    basic->setContentsMargins(0,0,0,0);
    QVBoxLayout *basicLayout = new QVBoxLayout(basic);
    basicLayout->setContentsMargins(0,0,0,0);
    basicLayout->setSpacing(5);

    QLabel *presetLabel = new QLabel(tr("Chart"));
    QFont sameFont = presetLabel->font();
#ifdef Q_OS_MAC // possibly needed on others too
    sameFont.setPointSize(sameFont.pointSize() + 2);
#endif

    dateSetting = new DateSettingsEdit(this);
    basicsettingsLayout->addRow(new QLabel(tr("Date range")), dateSetting);
    basicsettingsLayout->addRow(new QLabel(tr(""))); // spacing

    groupBy = new QComboBox;
    groupBy->addItem(tr("Days"), LTM_DAY);
    groupBy->addItem(tr("Weeks"), LTM_WEEK);
    groupBy->addItem(tr("Months"), LTM_MONTH);
    groupBy->addItem(tr("Years"), LTM_YEAR);
    groupBy->addItem(tr("Time Of Day"), LTM_TOD);
    groupBy->addItem(tr("All"), LTM_ALL);
    groupBy->setCurrentIndex(0);
    basicsettingsLayout->addRow(new QLabel(tr("Group by")), groupBy);
    basicsettingsLayout->addRow(new QLabel(tr(""))); // spacing

    showData = new QCheckBox(tr("Data Table"));
    showData->setChecked(false);
    basicsettingsLayout->addRow(new QLabel(""), showData);

    showStack = new QCheckBox(tr("Show Stack"));
    showStack->setChecked(false);
    basicsettingsLayout->addRow(new QLabel(""), showStack);

    shadeZones = new QCheckBox(tr("Shade Zones"));
    basicsettingsLayout->addRow(new QLabel(""), shadeZones);

    showLegend = new QCheckBox(tr("Show Legend"));
    basicsettingsLayout->addRow(new QLabel(""), showLegend);

    showEvents = new QCheckBox(tr("Show Events"));
    basicsettingsLayout->addRow(new QLabel(""), showEvents);

    stackSlider = new QSlider(Qt::Horizontal,this);
    stackSlider->setMinimum(0);
    stackSlider->setMaximum(7);
    stackSlider->setTickInterval(1);
    stackSlider->setValue(3);
    stackSlider->setFixedWidth(100);
    basicsettingsLayout->addRow(new QLabel(tr("Stack Zoom")), stackSlider);

    // controls
    QGridLayout *presetLayout = new QGridLayout;
    basicLayout->addLayout(presetLayout);

    importButton = new QPushButton(tr("Import..."));
    exportButton = new QPushButton(tr("Export..."));
    upButton = new QPushButton(tr("Move up"));
    downButton = new QPushButton(tr("Move down"));
    renameButton = new QPushButton(tr("Rename"));
    deleteButton = new QPushButton(tr("Delete"));
    newButton = new QPushButton(tr("Add Current")); // connected in LTMWindow.cpp
    connect(newButton, SIGNAL(clicked()), this, SLOT(addCurrent()));

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->addWidget(renameButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();
    actionButtons->addWidget(importButton);
    actionButtons->addWidget(exportButton);
    actionButtons->addStretch();
    actionButtons->addWidget(newButton);

    charts = new QTreeWidget;
#ifdef Q_OS_MAC
    charts->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    charts->headerItem()->setText(0, tr("Charts"));
    charts->setColumnCount(1);
    charts->setSelectionMode(QAbstractItemView::SingleSelection);
    charts->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    charts->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    charts->setIndentation(0);

    applyButton = new QPushButton(tr("Apply")); // connected in LTMWindow.cpp
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(applyButton);
    buttons->addStretch();
    basicLayout->addLayout(buttons);

    presetLayout->addWidget(charts, 0,0);
    presetLayout->addLayout(actionButtons, 0,1,1,2);

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(renameButton, SIGNAL(clicked()), this, SLOT(renameClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(importButton, SIGNAL(clicked()), this, SLOT(importClicked()));
    connect(exportButton, SIGNAL(clicked()), this, SLOT(exportClicked()));
    connect(charts, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(editingFinished()));
    connect(charts, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(editingStarted()));

    tabs = new QTabWidget(this);

    mainLayout->addWidget(tabs);
    basic->setContentsMargins(20,20,20,20);

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
        adds.trendtype = 0;
        adds.topN = 1; // show top 1 by default always
        QTextEdit processHTML(adds.metric->name()); // process html encoding of(TM)
        adds.name   = processHTML.toPlainText();

        // set default for the user overiddable fields
        adds.uname  = adds.name;
        adds.units = "";
        adds.uunits = adds.metric->units(context->athlete->useMetricUnits);

        // default units to metric name if it is blank
        if (adds.uunits == "") adds.uunits = adds.name;
        metrics.append(adds);
    }

    //
    // Add PM metrics, which are calculated over the metric dataset
    //

    QList<MetricDetail> pmMetrics = providePMmetrics();

    foreach (MetricDetail m, pmMetrics){
       metrics.append(m);
    }

    // metadata metrics
    SpecialFields sp;
    foreach (FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
        if (!sp.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            MetricDetail metametric;
            metametric.type = METRIC_META;
            QString underscored = field.name;
            metametric.symbol = underscored.replace(" ", "_");
            metametric.metric = NULL; // not a factory metric
            metametric.penColor = QColor(Qt::blue);
            metametric.curveStyle = QwtPlotCurve::Lines;
            metametric.symbolStyle = QwtSymbol::NoSymbol;
            metametric.smooth = false;
            metametric.trendtype = 0;
            metametric.topN = 1;
            metametric.uname = metametric.name = sp.displayName(field.name);
            metametric.units = "";
            metametric.uunits = "";
            metrics.append(metametric);
        }
    }

    // measures
    QList<FieldDefinition> measureDefinitions;
    QList<KeywordDefinition> keywordDefinitions; //NOTE: not used in measures.xml
    QString filename = context->athlete->home->config().absolutePath()+"/measures.xml";
    QString colorfield;
    if (!QFile(filename).exists()) filename = ":/xml/measures.xml";
    RideMetadata::readXML(filename, keywordDefinitions, measureDefinitions, colorfield);

    foreach (FieldDefinition field, measureDefinitions) {
        if (!sp.isMetric(field.name) && (field.type == 3 || field.type == 4)) {
            MetricDetail measure;
            measure.type = METRIC_MEASURE;
            QString underscored = field.name;
            measure.symbol = QString("%1_m").arg(field.name); // we don't bother with '_' for measures
            measure.metric = NULL; // not a factory metric
            measure.penColor = QColor(Qt::blue);
            measure.curveStyle = QwtPlotCurve::Lines;
            measure.symbolStyle = QwtSymbol::NoSymbol;
            measure.smooth = false;
            measure.trendtype = 0;
            measure.topN = 1;
            measure.uname = "";
            measure.name = QString("%1 (m)").arg(sp.displayName(field.name));
            measure.units = "";
            measure.uunits = "";
            metrics.append(measure);
        }
    }

    // sort the list
    qSort(metrics);

    // custom widget
    QWidget *custom = new QWidget(this);
    custom->setContentsMargins(20,20,20,20);
    QVBoxLayout *customLayout = new QVBoxLayout(custom);
    customLayout->setContentsMargins(0,0,0,0);
    customLayout->setSpacing(5);

    // custom table
    customTable = new QTableWidget(this);
#ifdef Q_OS_MAX
    customTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    customTable->setColumnCount(2);
    customTable->horizontalHeader()->setStretchLastSection(true);
    customTable->setSortingEnabled(false);
    customTable->verticalHeader()->hide();
    customTable->setShowGrid(false);
    customTable->setSelectionMode(QAbstractItemView::SingleSelection);
    customTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    customLayout->addWidget(customTable);
    connect(customTable, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(doubleClicked(int, int)));


    // custom buttons
    editCustomButton = new QPushButton(tr("Edit"));
    connect(editCustomButton, SIGNAL(clicked()), this, SLOT(editMetric()));

    addCustomButton = new QPushButton("+");
    connect(addCustomButton, SIGNAL(clicked()), this, SLOT(addMetric()));

    deleteCustomButton = new QPushButton("- ");
    connect(deleteCustomButton, SIGNAL(clicked()), this, SLOT(deleteMetric()));

    usePreset = new QCheckBox(tr("Use sidebar chart settings"));
    usePreset->setChecked(false);

#ifndef Q_OS_MAC
    addCustomButton->setFixedSize(20,20);
    deleteCustomButton->setFixedSize(20,20);
#endif
    QHBoxLayout *customButtons = new QHBoxLayout;
    customButtons->setSpacing(2);
    customButtons->addWidget(usePreset);
    customButtons->addStretch();
    customButtons->addWidget(editCustomButton);
    customButtons->addStretch();
    customButtons->addWidget(addCustomButton);
    customButtons->addWidget(deleteCustomButton);
    customLayout->addLayout(customButtons);

    tabs->addTab(basicsettings, tr("Basic"));
    tabs->addTab(basic, tr("Preset"));
    tabs->addTab(custom, tr("Curves"));

    // switched between one or other
    connect(dateSetting, SIGNAL(useStandardRange()), this, SIGNAL(useStandardRange()));
    connect(dateSetting, SIGNAL(useCustomRange(DateRange)), this, SIGNAL(useCustomRange(DateRange)));
    connect(dateSetting, SIGNAL(useThruToday()), this, SIGNAL(useThruToday()));

    // watch for changes to the preset charts
    connect(context, SIGNAL(presetsChanged()), this, SLOT(presetsChanged()));
    connect(usePreset, SIGNAL(stateChanged(int)), this, SLOT(usePresetChanged()));

    // set the show/hide for preset selection
    usePresetChanged();
    
    // but setup for the first time
    presetsChanged();
}

QList<MetricDetail> LTMTool::providePMmetrics() {

// Add PM metrics, which are calculated over the metric dataset
//

    QList<MetricDetail> metrics;

    // SKIBA LTS
    MetricDetail skibaLTS;
    skibaLTS.type = METRIC_PM;
    skibaLTS.symbol = "skiba_lts";
    skibaLTS.metric = NULL; // not a factory metric
    skibaLTS.penColor = QColor(Qt::blue);
    skibaLTS.curveStyle = QwtPlotCurve::Lines;
    skibaLTS.symbolStyle = QwtSymbol::NoSymbol;
    skibaLTS.smooth = false;
    skibaLTS.trendtype = 0;
    skibaLTS.topN = 1;
    skibaLTS.uname = skibaLTS.name = tr("Skiba Long Term Stress");
    skibaLTS.units = "Stress";
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
    skibaSTS.trendtype = 0;
    skibaSTS.topN = 1;
    skibaSTS.uname = skibaSTS.name = tr("Skiba Short Term Stress");
    skibaSTS.units = "Stress";
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
    skibaSB.trendtype = 0;
    skibaSB.topN = 1;
    skibaSB.uname = skibaSB.name = tr("Skiba Stress Balance");
    skibaSB.units = "Stress Balance";
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
    skibaSTR.trendtype = 0;
    skibaSTR.topN = 1;
    skibaSTR.uname = skibaSTR.name = tr("Skiba STS Ramp");
    skibaSTR.units = "Ramp";
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
    skibaLTR.trendtype = 0;
    skibaLTR.topN = 1;
    skibaLTR.uname = skibaLTR.name = tr("Skiba LTS Ramp");
    skibaLTR.units = "Ramp";
    skibaLTR.uunits = tr("Ramp");
    metrics.append(skibaLTR);

    // SKIBA Aerobic TISS LTS
    MetricDetail atissLTS;
    atissLTS.type = METRIC_PM;
    atissLTS.symbol = "atiss_lts";
    atissLTS.metric = NULL; // not a factory metric
    atissLTS.penColor = QColor(Qt::blue);
    atissLTS.curveStyle = QwtPlotCurve::Lines;
    atissLTS.symbolStyle = QwtSymbol::NoSymbol;
    atissLTS.smooth = false;
    atissLTS.trendtype = 0;
    atissLTS.topN = 1;
    atissLTS.uname = atissLTS.name = tr("Aerobic TISS Long Term Stress");
    atissLTS.units = "Stress";
    atissLTS.uunits = tr("Stress");
    metrics.append(atissLTS);

    MetricDetail atissSTS;
    atissSTS.type = METRIC_PM;
    atissSTS.symbol = "atiss_sts";
    atissSTS.metric = NULL; // not a factory metric
    atissSTS.penColor = QColor(Qt::magenta);
    atissSTS.curveStyle = QwtPlotCurve::Lines;
    atissSTS.symbolStyle = QwtSymbol::NoSymbol;
    atissSTS.smooth = false;
    atissSTS.trendtype = 0;
    atissSTS.topN = 1;
    atissSTS.uname = atissSTS.name = tr("Aerobic TISS Short Term Stress");
    atissSTS.units = "Stress";
    atissSTS.uunits = tr("Stress");
    metrics.append(atissSTS);

    MetricDetail atissSB;
    atissSB.type = METRIC_PM;
    atissSB.symbol = "atiss_sb";
    atissSB.metric = NULL; // not a factory metric
    atissSB.penColor = QColor(Qt::yellow);
    atissSB.curveStyle = QwtPlotCurve::Steps;
    atissSB.symbolStyle = QwtSymbol::NoSymbol;
    atissSB.smooth = false;
    atissSB.trendtype = 0;
    atissSB.topN = 1;
    atissSB.uname = atissSB.name = tr("Aerobic TISS Stress Balance");
    atissSB.units = "Stress Balance";
    atissSB.uunits = tr("Stress Balance");
    metrics.append(atissSB);

    MetricDetail atissSTR;
    atissSTR.type = METRIC_PM;
    atissSTR.symbol = "atiss_sr";
    atissSTR.metric = NULL; // not a factory metric
    atissSTR.penColor = QColor(Qt::darkGreen);
    atissSTR.curveStyle = QwtPlotCurve::Steps;
    atissSTR.symbolStyle = QwtSymbol::NoSymbol;
    atissSTR.smooth = false;
    atissSTR.trendtype = 0;
    atissSTR.topN = 1;
    atissSTR.uname = atissSTR.name = tr("Aerobic TISS STS Ramp");
    atissSTR.units = "Ramp";
    atissSTR.uunits = tr("Ramp");
    metrics.append(atissSTR);

    MetricDetail atissLTR;
    atissLTR.type = METRIC_PM;
    atissLTR.symbol = "atiss_lr";
    atissLTR.metric = NULL; // not a factory metric
    atissLTR.penColor = QColor(Qt::darkBlue);
    atissLTR.curveStyle = QwtPlotCurve::Steps;
    atissLTR.symbolStyle = QwtSymbol::NoSymbol;
    atissLTR.smooth = false;
    atissLTR.trendtype = 0;
    atissLTR.topN = 1;
    atissLTR.uname = atissLTR.name = tr("Aerobic TISS LTS Ramp");
    atissLTR.units = "Ramp";
    atissLTR.uunits = tr("Ramp");
    metrics.append(atissLTR);

    // SKIBA Anerobic TISS LTS
    MetricDetail antissLTS;
    antissLTS.type = METRIC_PM;
    antissLTS.symbol = "antiss_lts";
    antissLTS.metric = NULL; // not a factory metric
    antissLTS.penColor = QColor(Qt::blue);
    antissLTS.curveStyle = QwtPlotCurve::Lines;
    antissLTS.symbolStyle = QwtSymbol::NoSymbol;
    antissLTS.smooth = false;
    antissLTS.trendtype = 0;
    antissLTS.topN = 1;
    antissLTS.uname = antissLTS.name = tr("Anaerobic TISS Long Term Stress");
    antissLTS.units = "Stress";
    antissLTS.uunits = tr("Stress");
    metrics.append(antissLTS);

    MetricDetail antissSTS;
    antissSTS.type = METRIC_PM;
    antissSTS.symbol = "antiss_sts";
    antissSTS.metric = NULL; // not a factory metric
    antissSTS.penColor = QColor(Qt::magenta);
    antissSTS.curveStyle = QwtPlotCurve::Lines;
    antissSTS.symbolStyle = QwtSymbol::NoSymbol;
    antissSTS.smooth = false;
    antissSTS.trendtype = 0;
    antissSTS.topN = 1;
    antissSTS.uname = antissSTS.name = tr("Anaerobic TISS Short Term Stress");
    antissSTS.units = "Stress";
    antissSTS.uunits = tr("Stress");
    metrics.append(antissSTS);

    MetricDetail antissSB;
    antissSB.type = METRIC_PM;
    antissSB.symbol = "antiss_sb";
    antissSB.metric = NULL; // not a factory metric
    antissSB.penColor = QColor(Qt::yellow);
    antissSB.curveStyle = QwtPlotCurve::Steps;
    antissSB.symbolStyle = QwtSymbol::NoSymbol;
    antissSB.smooth = false;
    antissSB.trendtype = 0;
    antissSB.topN = 1;
    antissSB.uname = antissSB.name = tr("Anaerobic TISS Stress Balance");
    antissSB.units = "Stress Balance";
    antissSB.uunits = tr("Stress Balance");
    metrics.append(antissSB);

    MetricDetail antissSTR;
    antissSTR.type = METRIC_PM;
    antissSTR.symbol = "antiss_sr";
    antissSTR.metric = NULL; // not a factory metric
    antissSTR.penColor = QColor(Qt::darkGreen);
    antissSTR.curveStyle = QwtPlotCurve::Steps;
    antissSTR.symbolStyle = QwtSymbol::NoSymbol;
    antissSTR.smooth = false;
    antissSTR.trendtype = 0;
    antissSTR.topN = 1;
    antissSTR.uname = antissSTR.name = tr("Anaerobic TISS STS Ramp");
    antissSTR.units = "Ramp";
    antissSTR.uunits = tr("Ramp");
    metrics.append(antissSTR);

    MetricDetail antissLTR;
    antissLTR.type = METRIC_PM;
    antissLTR.symbol = "antiss_lr";
    antissLTR.metric = NULL; // not a factory metric
    antissLTR.penColor = QColor(Qt::darkBlue);
    antissLTR.curveStyle = QwtPlotCurve::Steps;
    antissLTR.symbolStyle = QwtSymbol::NoSymbol;
    antissLTR.smooth = false;
    antissLTR.trendtype = 0;
    antissLTR.topN = 1;
    antissLTR.uname = antissLTR.name = tr("Anaerobic TISS LTS Ramp");
    antissLTR.units = "Ramp";
    antissLTR.uunits = tr("Ramp");
    metrics.append(antissLTR);
    // DANIELS LTS
    MetricDetail danielsLTS;
    danielsLTS.type = METRIC_PM;
    danielsLTS.symbol = "daniels_lts";
    danielsLTS.metric = NULL; // not a factory metric
    danielsLTS.penColor = QColor(Qt::blue);
    danielsLTS.curveStyle = QwtPlotCurve::Lines;
    danielsLTS.symbolStyle = QwtSymbol::NoSymbol;
    danielsLTS.smooth = false;
    danielsLTS.trendtype = 0;
    danielsLTS.topN = 1;
    danielsLTS.uname = danielsLTS.name = tr("Daniels Long Term Stress");
    danielsLTS.units = "Stress";
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
    danielsSTS.trendtype = 0;
    danielsSTS.topN = 1;
    danielsSTS.uname = danielsSTS.name = tr("Daniels Short Term Stress");
    danielsSTS.units = "Stress";
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
    danielsSB.trendtype = 0;
    danielsSB.topN = 1;
    danielsSB.uname = danielsSB.name = tr("Daniels Stress Balance");
    danielsSB.units = "Stress Balance";
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
    danielsSTR.trendtype = 0;
    danielsSTR.topN = 1;
    danielsSTR.uname = danielsSTR.name = tr("Daniels STS Ramp");
    danielsSTR.units = "Ramp";
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
    danielsLTR.trendtype = 0;
    danielsLTR.topN = 1;
    danielsLTR.uname = danielsLTR.name = tr("Daniels LTS Ramp");
    danielsLTR.units = "Ramp";
    danielsLTR.uunits = tr("Ramp");
    metrics.append(danielsLTR);

    // total work
    MetricDetail workLTS;
    workLTS.type = METRIC_PM;
    workLTS.symbol = "work_lts";
    workLTS.metric = NULL; // not a factory metric
    workLTS.penColor = QColor(Qt::blue);
    workLTS.curveStyle = QwtPlotCurve::Lines;
    workLTS.symbolStyle = QwtSymbol::NoSymbol;
    workLTS.smooth = false;
    workLTS.trendtype = 0;
    workLTS.topN = 1;
    workLTS.uname = workLTS.name = tr("Work (Kj) Long Term Stress");
    workLTS.units = "Stress (Kj)";
    workLTS.uunits = tr("Stress (Kj)");
    metrics.append(workLTS);

    MetricDetail workSTS;
    workSTS.type = METRIC_PM;
    workSTS.symbol = "work_sts";
    workSTS.metric = NULL; // not a factory metric
    workSTS.penColor = QColor(Qt::magenta);
    workSTS.curveStyle = QwtPlotCurve::Lines;
    workSTS.symbolStyle = QwtSymbol::NoSymbol;
    workSTS.smooth = false;
    workSTS.trendtype = 0;
    workSTS.topN = 1;
    workSTS.uname = workSTS.name = tr("Work (Kj) Short Term Stress");
    workSTS.units = "Stress (Kj)";
    workSTS.uunits = tr("Stress (Kj)");
    metrics.append(workSTS);

    MetricDetail workSB;
    workSB.type = METRIC_PM;
    workSB.symbol = "work_sb";
    workSB.metric = NULL; // not a factory metric
    workSB.penColor = QColor(Qt::yellow);
    workSB.curveStyle = QwtPlotCurve::Steps;
    workSB.symbolStyle = QwtSymbol::NoSymbol;
    workSB.smooth = false;
    workSB.trendtype = 0;
    workSB.topN = 1;
    workSB.uname = workSB.name = tr("Work (Kj) Stress Balance");
    workSB.units = "Stress Balance";
    workSB.uunits = tr("Stress Balance");
    metrics.append(workSB);

    MetricDetail workSTR;
    workSTR.type = METRIC_PM;
    workSTR.symbol = "work_sr";
    workSTR.metric = NULL; // not a factory metric
    workSTR.penColor = QColor(Qt::darkGreen);
    workSTR.curveStyle = QwtPlotCurve::Steps;
    workSTR.symbolStyle = QwtSymbol::NoSymbol;
    workSTR.smooth = false;
    workSTR.trendtype = 0;
    workSTR.topN = 1;
    workSTR.uname = workSTR.name = tr("Work (Kj) STS Ramp");
    workSTR.units = "Ramp";
    workSTR.uunits = tr("Ramp");
    metrics.append(workSTR);

    MetricDetail workLTR;
    workLTR.type = METRIC_PM;
    workLTR.symbol = "work_lr";
    workLTR.metric = NULL; // not a factory metric
    workLTR.penColor = QColor(Qt::darkBlue);
    workLTR.curveStyle = QwtPlotCurve::Steps;
    workLTR.symbolStyle = QwtSymbol::NoSymbol;
    workLTR.smooth = false;
    workLTR.trendtype = 0;
    workLTR.topN = 1;
    workLTR.uname = workLTR.name = tr("Work (Kj) LTS Ramp");
    workLTR.units = "Ramp";
    workLTR.uunits = tr("Ramp");
    metrics.append(workLTR);

    // total wprime work
    MetricDetail wPrimeWorkLTS;
    wPrimeWorkLTS.type = METRIC_PM;
    wPrimeWorkLTS.symbol = "wprime_lts";
    wPrimeWorkLTS.metric = NULL; // not a factory metric
    wPrimeWorkLTS.penColor = QColor(Qt::blue);
    wPrimeWorkLTS.curveStyle = QwtPlotCurve::Lines;
    wPrimeWorkLTS.symbolStyle = QwtSymbol::NoSymbol;
    wPrimeWorkLTS.smooth = false;
    wPrimeWorkLTS.trendtype = 0;
    wPrimeWorkLTS.topN = 1;
    wPrimeWorkLTS.uname = wPrimeWorkLTS.name = tr("W' Work (Kj) Long Term Stress");
    wPrimeWorkLTS.units = "Stress (Kj)";
    wPrimeWorkLTS.uunits = tr("Stress (Kj)");
    metrics.append(wPrimeWorkLTS);

    MetricDetail wPrimeWorkSTS;
    wPrimeWorkSTS.type = METRIC_PM;
    wPrimeWorkSTS.symbol = "wprime_sts";
    wPrimeWorkSTS.metric = NULL; // not a factory metric
    wPrimeWorkSTS.penColor = QColor(Qt::magenta);
    wPrimeWorkSTS.curveStyle = QwtPlotCurve::Lines;
    wPrimeWorkSTS.symbolStyle = QwtSymbol::NoSymbol;
    wPrimeWorkSTS.smooth = false;
    wPrimeWorkSTS.trendtype = 0;
    wPrimeWorkSTS.topN = 1;
    wPrimeWorkSTS.uname = wPrimeWorkSTS.name = tr("W' Work (Kj) Short Term Stress");
    wPrimeWorkSTS.units = "Stress (Kj)";
    wPrimeWorkSTS.uunits = tr("Stress (Kj)");
    metrics.append(wPrimeWorkSTS);

    MetricDetail wPrimeWorkSB;
    wPrimeWorkSB.type = METRIC_PM;
    wPrimeWorkSB.symbol = "wprime_sb";
    wPrimeWorkSB.metric = NULL; // not a factory metric
    wPrimeWorkSB.penColor = QColor(Qt::yellow);
    wPrimeWorkSB.curveStyle = QwtPlotCurve::Steps;
    wPrimeWorkSB.symbolStyle = QwtSymbol::NoSymbol;
    wPrimeWorkSB.smooth = false;
    wPrimeWorkSB.trendtype = 0;
    wPrimeWorkSB.topN = 1;
    wPrimeWorkSB.uname = wPrimeWorkSB.name = tr("W' Work (Kj) Stress Balance");
    wPrimeWorkSB.units = "Stress Balance";
    wPrimeWorkSB.uunits = tr("Stress Balance");
    metrics.append(wPrimeWorkSB);

    MetricDetail wPrimeWorkSTR;
    wPrimeWorkSTR.type = METRIC_PM;
    wPrimeWorkSTR.symbol = "wprime_sr";
    wPrimeWorkSTR.metric = NULL; // not a factory metric
    wPrimeWorkSTR.penColor = QColor(Qt::darkGreen);
    wPrimeWorkSTR.curveStyle = QwtPlotCurve::Steps;
    wPrimeWorkSTR.symbolStyle = QwtSymbol::NoSymbol;
    wPrimeWorkSTR.smooth = false;
    wPrimeWorkSTR.trendtype = 0;
    wPrimeWorkSTR.topN = 1;
    wPrimeWorkSTR.uname = wPrimeWorkSTR.name = tr("W' Work (Kj) STS Ramp");
    wPrimeWorkSTR.units = "Ramp";
    wPrimeWorkSTR.uunits = tr("Ramp");
    metrics.append(wPrimeWorkSTR);

    MetricDetail wPrimeWorkLTR;
    wPrimeWorkLTR.type = METRIC_PM;
    wPrimeWorkLTR.symbol = "wprime_lr";
    wPrimeWorkLTR.metric = NULL; // not a factory metric
    wPrimeWorkLTR.penColor = QColor(Qt::darkBlue);
    wPrimeWorkLTR.curveStyle = QwtPlotCurve::Steps;
    wPrimeWorkLTR.symbolStyle = QwtSymbol::NoSymbol;
    wPrimeWorkLTR.smooth = false;
    wPrimeWorkLTR.trendtype = 0;
    wPrimeWorkLTR.topN = 1;
    wPrimeWorkLTR.uname = wPrimeWorkLTR.name = tr("W' Work (Kj) LTS Ramp");
    wPrimeWorkLTR.units = "Ramp";
    wPrimeWorkLTR.uunits = tr("Ramp");
    metrics.append(wPrimeWorkLTR);

    // total below CP work
    MetricDetail cpWorkLTS;
    cpWorkLTS.type = METRIC_PM;
    cpWorkLTS.symbol = "cp_lts";
    cpWorkLTS.metric = NULL; // not a factory metric
    cpWorkLTS.penColor = QColor(Qt::blue);
    cpWorkLTS.curveStyle = QwtPlotCurve::Lines;
    cpWorkLTS.symbolStyle = QwtSymbol::NoSymbol;
    cpWorkLTS.smooth = false;
    cpWorkLTS.trendtype = 0;
    cpWorkLTS.topN = 1;
    cpWorkLTS.uname = cpWorkLTS.name = tr("Below CP Work (Kj) Long Term Stress");
    cpWorkLTS.units = "Stress (Kj)";
    cpWorkLTS.uunits = tr("Stress (Kj)");
    metrics.append(cpWorkLTS);

    MetricDetail cpWorkSTS;
    cpWorkSTS.type = METRIC_PM;
    cpWorkSTS.symbol = "cp_sts";
    cpWorkSTS.metric = NULL; // not a factory metric
    cpWorkSTS.penColor = QColor(Qt::magenta);
    cpWorkSTS.curveStyle = QwtPlotCurve::Lines;
    cpWorkSTS.symbolStyle = QwtSymbol::NoSymbol;
    cpWorkSTS.smooth = false;
    cpWorkSTS.trendtype = 0;
    cpWorkSTS.topN = 1;
    cpWorkSTS.uname = cpWorkSTS.name = tr("Below CP Work (Kj) Short Term Stress");
    cpWorkSTS.units = "Stress (Kj)";
    cpWorkSTS.uunits = tr("Stress (Kj)");
    metrics.append(cpWorkSTS);

    MetricDetail cpWorkSB;
    cpWorkSB.type = METRIC_PM;
    cpWorkSB.symbol = "cp_sb";
    cpWorkSB.metric = NULL; // not a factory metric
    cpWorkSB.penColor = QColor(Qt::yellow);
    cpWorkSB.curveStyle = QwtPlotCurve::Steps;
    cpWorkSB.symbolStyle = QwtSymbol::NoSymbol;
    cpWorkSB.smooth = false;
    cpWorkSB.trendtype = 0;
    cpWorkSB.topN = 1;
    cpWorkSB.uname = cpWorkSB.name = tr("Below CP Work (Kj) Stress Balance");
    cpWorkSB.units = "Stress Balance";
    cpWorkSB.uunits = tr("Stress Balance");
    metrics.append(cpWorkSB);

    MetricDetail cpWorkSTR;
    cpWorkSTR.type = METRIC_PM;
    cpWorkSTR.symbol = "cp_sr";
    cpWorkSTR.metric = NULL; // not a factory metric
    cpWorkSTR.penColor = QColor(Qt::darkGreen);
    cpWorkSTR.curveStyle = QwtPlotCurve::Steps;
    cpWorkSTR.symbolStyle = QwtSymbol::NoSymbol;
    cpWorkSTR.smooth = false;
    cpWorkSTR.trendtype = 0;
    cpWorkSTR.topN = 1;
    cpWorkSTR.uname = cpWorkSTR.name = tr("Below CP Work (Kj) STS Ramp");
    cpWorkSTR.units = "Ramp";
    cpWorkSTR.uunits = tr("Ramp");
    metrics.append(cpWorkSTR);

    MetricDetail cpWorkLTR;
    cpWorkLTR.type = METRIC_PM;
    cpWorkLTR.symbol = "cp_lr";
    cpWorkLTR.metric = NULL; // not a factory metric
    cpWorkLTR.penColor = QColor(Qt::darkBlue);
    cpWorkLTR.curveStyle = QwtPlotCurve::Steps;
    cpWorkLTR.symbolStyle = QwtSymbol::NoSymbol;
    cpWorkLTR.smooth = false;
    cpWorkLTR.trendtype = 0;
    cpWorkLTR.topN = 1;
    cpWorkLTR.uname = cpWorkLTR.name = tr("Below CP Work (Kj) LTS Ramp");
    cpWorkLTR.units = "Ramp";
    cpWorkLTR.uunits = tr("Ramp");
    metrics.append(cpWorkLTR);

    // total distance
    MetricDetail distanceLTS;
    distanceLTS.type = METRIC_PM;
    distanceLTS.symbol = "distance_lts";
    distanceLTS.metric = NULL; // not a factory metric
    distanceLTS.penColor = QColor(Qt::blue);
    distanceLTS.curveStyle = QwtPlotCurve::Lines;
    distanceLTS.symbolStyle = QwtSymbol::NoSymbol;
    distanceLTS.smooth = false;
    distanceLTS.trendtype = 0;
    distanceLTS.topN = 1;
    distanceLTS.uname = distanceLTS.name = tr("Distance (km|mi) Long Term Stress");
    distanceLTS.units = "Stress (km|mi)";
    distanceLTS.uunits = tr("Stress (km|mi)");
    metrics.append(distanceLTS);

    MetricDetail distanceSTS;
    distanceSTS.type = METRIC_PM;
    distanceSTS.symbol = "distance_sts";
    distanceSTS.metric = NULL; // not a factory metric
    distanceSTS.penColor = QColor(Qt::magenta);
    distanceSTS.curveStyle = QwtPlotCurve::Lines;
    distanceSTS.symbolStyle = QwtSymbol::NoSymbol;
    distanceSTS.smooth = false;
    distanceSTS.trendtype = 0;
    distanceSTS.topN = 1;
    distanceSTS.uname = distanceSTS.name = tr("Distance (km|mi) Short Term Stress");
    distanceSTS.units = "Stress (km|mi)";
    distanceSTS.uunits = tr("Stress (km|mi)");
    metrics.append(distanceSTS);

    MetricDetail distanceSB;
    distanceSB.type = METRIC_PM;
    distanceSB.symbol = "distance_sb";
    distanceSB.metric = NULL; // not a factory metric
    distanceSB.penColor = QColor(Qt::yellow);
    distanceSB.curveStyle = QwtPlotCurve::Steps;
    distanceSB.symbolStyle = QwtSymbol::NoSymbol;
    distanceSB.smooth = false;
    distanceSB.trendtype = 0;
    distanceSB.topN = 1;
    distanceSB.uname = distanceSB.name = tr("Distance (km|mi) Stress Balance");
    distanceSB.units = "Stress Balance";
    distanceSB.uunits = tr("Stress Balance");
    metrics.append(distanceSB);

    MetricDetail distanceSTR;
    distanceSTR.type = METRIC_PM;
    distanceSTR.symbol = "distance_sr";
    distanceSTR.metric = NULL; // not a factory metric
    distanceSTR.penColor = QColor(Qt::darkGreen);
    distanceSTR.curveStyle = QwtPlotCurve::Steps;
    distanceSTR.symbolStyle = QwtSymbol::NoSymbol;
    distanceSTR.smooth = false;
    distanceSTR.trendtype = 0;
    distanceSTR.topN = 1;
    distanceSTR.uname = distanceSTR.name = tr("Distance (km|mi) STS Ramp");
    distanceSTR.units = "Ramp";
    distanceSTR.uunits = tr("Ramp");
    metrics.append(distanceSTR);

    MetricDetail distanceLTR;
    distanceLTR.type = METRIC_PM;
    distanceLTR.symbol = "distance_lr";
    distanceLTR.metric = NULL; // not a factory metric
    distanceLTR.penColor = QColor(Qt::darkBlue);
    distanceLTR.curveStyle = QwtPlotCurve::Steps;
    distanceLTR.symbolStyle = QwtSymbol::NoSymbol;
    distanceLTR.smooth = false;
    distanceLTR.trendtype = 0;
    distanceLTR.topN = 1;
    distanceLTR.uname = distanceLTR.name = tr("Distance (km|mi) LTS Ramp");
    distanceLTR.units = "Ramp";
    distanceLTR.uunits = tr("Ramp");
    metrics.append(distanceLTR);

    // COGGAN LTS
    MetricDetail cogganCTL;
    cogganCTL.type = METRIC_PM;
    cogganCTL.symbol = "coggan_ctl";
    cogganCTL.metric = NULL; // not a factory metric
    cogganCTL.penColor = QColor(Qt::blue);
    cogganCTL.curveStyle = QwtPlotCurve::Lines;
    cogganCTL.symbolStyle = QwtSymbol::NoSymbol;
    cogganCTL.smooth = false;
    cogganCTL.trendtype = 0;
    cogganCTL.topN = 1;
    cogganCTL.uname = cogganCTL.name = tr("Coggan Chronic Training Load");
    cogganCTL.units = "CTL";
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
    cogganATL.trendtype = 0;
    cogganATL.topN = 1;
    cogganATL.uname = cogganATL.name = tr("Coggan Acute Training Load");
    cogganATL.units = "ATL";
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
    cogganTSB.trendtype = 0;
    cogganTSB.topN = 1;
    cogganTSB.uname = cogganTSB.name = tr("Coggan Training Stress Balance");
    cogganTSB.units = "TSB";
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
    cogganSTR.trendtype = 0;
    cogganSTR.topN = 1;
    cogganSTR.uname = cogganSTR.name = tr("Coggan STS Ramp");
    cogganSTR.units = "Ramp";
    cogganSTR.uunits = tr("Ramp");
    metrics.append(cogganSTR);

    MetricDetail cogganLTR;
    cogganLTR.type = METRIC_PM;
    cogganLTR.symbol = "coggan_lr";
    cogganLTR.metric = NULL; // not a factory metric
    cogganLTR.penColor = QColor(Qt::darkBlue);
    cogganLTR.curveStyle = QwtPlotCurve::Steps;
    cogganLTR.symbolStyle = QwtSymbol::NoSymbol;
    cogganLTR.smooth = false;
    cogganLTR.trendtype = 0;
    cogganLTR.topN = 1;
    cogganLTR.uname = cogganLTR.name = tr("Coggan LTS Ramp");
    cogganLTR.units = "Ramp";
    cogganLTR.uunits = tr("Ramp");
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
    trimpLTS.trendtype = 0;
    trimpLTS.topN = 1;
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
    trimpSTS.trendtype = 0;
    trimpSTS.topN = 1;
    trimpSTS.uname = trimpSTS.name = tr("TRIMP Short Term Stress");
    trimpSTS.units = "Stress";
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
    trimpSB.trendtype = 0;
    trimpSB.topN = 1;
    trimpSB.uname = trimpSB.name = tr("TRIMP Stress Balance");
    trimpSB.units = "Stress Balance";
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
    trimpSTR.trendtype = 0;
    trimpSTR.topN = 1;
    trimpSTR.uname = trimpSTR.name = tr("TRIMP STS Ramp");
    trimpSTR.units = "Ramp";
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
    trimpLTR.trendtype = 0;
    trimpLTR.topN = 1;
    trimpLTR.uname = trimpLTR.name = tr("TRIMP LTS Ramp");
    trimpLTR.units = "Ramp";
    trimpLTR.uunits = tr("Ramp");
    metrics.append(trimpLTR);

    // GOVSS LTS
    MetricDetail govssLTS;
    govssLTS.type = METRIC_PM;
    govssLTS.symbol = "govss_lts";
    govssLTS.metric = NULL; // not a factory metric
    govssLTS.penColor = QColor(Qt::blue);
    govssLTS.curveStyle = QwtPlotCurve::Lines;
    govssLTS.symbolStyle = QwtSymbol::NoSymbol;
    govssLTS.smooth = false;
    govssLTS.trendtype = 0;
    govssLTS.topN = 1;
    govssLTS.uname = govssLTS.name = tr("GOVSS Long Term Stress");
    govssLTS.units = "Stress";
    govssLTS.uunits = tr("Stress");
    metrics.append(govssLTS);

    MetricDetail govssSTS;
    govssSTS.type = METRIC_PM;
    govssSTS.symbol = "govss_sts";
    govssSTS.metric = NULL; // not a factory metric
    govssSTS.penColor = QColor(Qt::magenta);
    govssSTS.curveStyle = QwtPlotCurve::Lines;
    govssSTS.symbolStyle = QwtSymbol::NoSymbol;
    govssSTS.smooth = false;
    govssSTS.trendtype = 0;
    govssSTS.topN = 1;
    govssSTS.uname = govssSTS.name = tr("GOVSS Short Term Stress");
    govssSTS.units = "Stress";
    govssSTS.uunits = tr("Stress");
    metrics.append(govssSTS);

    MetricDetail govssSB;
    govssSB.type = METRIC_PM;
    govssSB.symbol = "govss_sb";
    govssSB.metric = NULL; // not a factory metric
    govssSB.penColor = QColor(Qt::yellow);
    govssSB.curveStyle = QwtPlotCurve::Steps;
    govssSB.symbolStyle = QwtSymbol::NoSymbol;
    govssSB.smooth = false;
    govssSB.trendtype = 0;
    govssSB.topN = 1;
    govssSB.uname = govssSB.name = tr("GOVSS Stress Balance");
    govssSB.units = "Stress Balance";
    govssSB.uunits = tr("Stress Balance");
    metrics.append(govssSB);

    MetricDetail govssSTR;
    govssSTR.type = METRIC_PM;
    govssSTR.symbol = "govss_sr";
    govssSTR.metric = NULL; // not a factory metric
    govssSTR.penColor = QColor(Qt::darkGreen);
    govssSTR.curveStyle = QwtPlotCurve::Steps;
    govssSTR.symbolStyle = QwtSymbol::NoSymbol;
    govssSTR.smooth = false;
    govssSTR.trendtype = 0;
    govssSTR.topN = 1;
    govssSTR.uname = govssSTR.name = tr("GOVSS STS Ramp");
    govssSTR.units = "Ramp";
    govssSTR.uunits = tr("Ramp");
    metrics.append(govssSTR);

    MetricDetail govssLTR;
    govssLTR.type = METRIC_PM;
    govssLTR.symbol = "govss_lr";
    govssLTR.metric = NULL; // not a factory metric
    govssLTR.penColor = QColor(Qt::darkBlue);
    govssLTR.curveStyle = QwtPlotCurve::Steps;
    govssLTR.symbolStyle = QwtSymbol::NoSymbol;
    govssLTR.smooth = false;
    govssLTR.trendtype = 0;
    govssLTR.topN = 1;
    govssLTR.uname = govssLTR.name = tr("GOVSS LTS Ramp");
    govssLTR.units = "Ramp";
    govssLTR.uunits = tr("Ramp");
    metrics.append(govssLTR);

    // done

    return metrics;

}


void
LTMTool::usePresetChanged()
{
    customTable->setEnabled(!usePreset->isChecked());
    editCustomButton->setEnabled(!usePreset->isChecked());
    addCustomButton->setEnabled(!usePreset->isChecked());
    deleteCustomButton->setEnabled(!usePreset->isChecked());
}

void
LTMTool::presetsChanged()
{
    // rebuild the preset chart list as the presets have changed
    charts->clear();
    foreach(LTMSettings chart, context->athlete->presets) {
        QTreeWidgetItem *add;
        add = new QTreeWidgetItem(charts->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        add->setText(0, chart.name);
    }

    // select the first one, if there are any
    if (context->athlete->presets.count())
        charts->setCurrentItem(charts->invisibleRootItem()->child(0));
}

void
LTMTool::refreshCustomTable()
{
    // clear then repopulate custom table settings to reflect
    // the current LTMSettings.
    customTable->clear();

    // get headers back
    QStringList header;
    header << tr("Type") << tr("Details"); 
    customTable->setHorizontalHeaderLabels(header);

    // now lets add a row for each metric
    customTable->setRowCount(settings->metrics.count());
    int i=0;
    foreach (MetricDetail metricDetail, settings->metrics) {

        QTableWidgetItem *t = new QTableWidgetItem();
        if (metricDetail.type != 5 && metricDetail.type != 6)
            t->setText(tr("Metric")); // only metrics .. for now ..
        else if (metricDetail.type == 5)
            t->setText(tr("Peak"));
        else
            t->setText(tr("Estimate"));
        
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        customTable->setItem(i,0,t);

        t = new QTableWidgetItem();
        if (metricDetail.type != 5 && metricDetail.type != 6)
            t->setText(metricDetail.name);
        else {
            // text description for peak
            t->setText(metricDetail.uname);
        }

        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        customTable->setItem(i,1,t);

        i++;
    }
}

void
LTMTool::editMetric()
{
    QList<QTableWidgetItem*> items = customTable->selectedItems();
    if (items.count() < 1) return;

    int index = customTable->row(items.first());

    MetricDetail edit = settings->metrics[index];
    EditMetricDetailDialog dialog(context, this, &edit);

    if (dialog.exec()) {

        // apply!
        settings->metrics[index] = edit;

        // update
        refreshCustomTable();
        curvesChanged();
    }
}

void
LTMTool::doubleClicked( int row, int column )
{
    (void) column; // ignore, calm down

    MetricDetail edit = settings->metrics[row];
    EditMetricDetailDialog dialog(context, this, &edit);

    if (dialog.exec()) {

        // apply!
        settings->metrics[row] = edit;

        // update
        refreshCustomTable();
        curvesChanged();
    }
}

void
LTMTool::deleteMetric()
{
    QList<QTableWidgetItem*> items = customTable->selectedItems();
    if (items.count() < 1) return;
    
    int index = customTable->row(items.first());
    settings->metrics.removeAt(index);
    refreshCustomTable();
    curvesChanged();
}

void
LTMTool::addMetric()
{
    MetricDetail add;
    EditMetricDetailDialog dialog(context, this, &add);

    if (dialog.exec()) {
        // apply
        settings->metrics.append(add);

        // refresh
        refreshCustomTable();
        curvesChanged();
    }
}

void
LTMTool::applySettings()
{
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
                    metrics[i].uunits.contains(saved->units(!context->athlete->useMetricUnits)))
                    metrics[i].uunits.replace(saved->units(!context->athlete->useMetricUnits), saved->units(context->athlete->useMetricUnits));


                break;
            }
        }
    }

    refreshCustomTable();

    curvesChanged();
}

void
LTMTool::addCurrent()
{
    // give the chart a name
    if (settings->name == "") settings->name = QString(tr("Chart %1")).arg(context->athlete->presets.count()+1);

    // add the current chart to the presets with a name using the chart title
    context->athlete->presets.append(*settings);

    // tree will now be refreshed
    editing = false;
    context->notifyPresetsChanged();
}

void
LTMTool::getMetricsTranslationMap (QMap<QString, QString> &nMap, QMap<QString, QString> &uMap, bool useMetricUnits) {

    // build up translation maps
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++) {
        const RideMetric *add = factory.rideMetric(factory.metricName(i));
        QTextEdit processHTMLname(add->name());
        // use the .symbol() as key - since only CHART.XML is mapped
        nMap.insert(add->symbol(), processHTMLname.toPlainText());
        uMap.insert(add->symbol(), add->units(useMetricUnits));

    }
    // add mapping for PM metrics (name and unit)
    QList<MetricDetail> pmMetrics = LTMTool::providePMmetrics();
    for (int i=0; i<pmMetrics.count(); i++)
    {
        nMap.insert(pmMetrics[i].symbol, pmMetrics[i].uname);
        uMap.insert(pmMetrics[i].symbol, pmMetrics[i].uunits);
    }

}


// set the estimateSelection based upon what is available
void 
EditMetricDetailDialog::modelChanged()
{
    int currentIndex=modelSelect->currentIndex();
    int ce = estimateSelect->currentIndex();

    // pooey this smells ! -- enable of disable each option 
    //                        based upon the current model
    qobject_cast<QStandardItemModel *>(estimateSelect->model())->item(0)->setEnabled(models[currentIndex]->hasWPrime());
    qobject_cast<QStandardItemModel *>(estimateSelect->model())->item(1)->setEnabled(models[currentIndex]->hasCP());
    qobject_cast<QStandardItemModel *>(estimateSelect->model())->item(2)->setEnabled(models[currentIndex]->hasFTP());
    qobject_cast<QStandardItemModel *>(estimateSelect->model())->item(3)->setEnabled(models[currentIndex]->hasPMax());
    qobject_cast<QStandardItemModel *>(estimateSelect->model())->item(4)->setEnabled(true);
    qobject_cast<QStandardItemModel *>(estimateSelect->model())->item(5)->setEnabled(true);

    // switch to other estimate if wanted estimate is not selected
    if (ce < 0 || !qobject_cast<QStandardItemModel *>(estimateSelect->model())->item(ce)->isEnabled())
        estimateSelect->setCurrentIndex(0);

    estimateName();
}

void
EditMetricDetailDialog::estimateChanged()
{
    estimateName();
}

void
EditMetricDetailDialog::estimateName()
{
    if (chooseEstimate->isChecked() == false) return; // only if we have estimate selected

    // do we need to see the best duration ?
    if (estimateSelect->currentIndex() == 4) {
        estimateDuration->show();
        estimateDurationUnits->show();
    } else {
        estimateDuration->hide();
        estimateDurationUnits->hide();
    }

    // set the estimate name from model and estimate type
    QString name;

    // first do the type if estimate
    switch(estimateSelect->currentIndex()) {
        case 0 : name = "W'"; break;
        case 1 : name = "CP"; break;
        case 2 : name = "FTP"; break;
        case 3 : name = "p-Max"; break;
        case 4 : 
            {
                name = QString(tr("Estimate %1 %2 Power")).arg(estimateDuration->value())
                                                  .arg(estimateDurationUnits->currentText());
            }
            break;
        case 5 : name = tr("Endurance Index"); break;
    }

    // now the model
    name += " (" + models[modelSelect->currentIndex()]->code() + ")";
    userName->setText(name);
    metricDetail->symbol = name.replace(" ", "_");
}

/*----------------------------------------------------------------------
 * EDIT METRIC DETAIL DIALOG
 *--------------------------------------------------------------------*/
EditMetricDetailDialog::EditMetricDetailDialog(Context *context, LTMTool *ltmTool, MetricDetail *metricDetail) :
    QDialog(context->mainWindow, Qt::Dialog), context(context), ltmTool(ltmTool), metricDetail(metricDetail)
{
    setWindowTitle(tr("Curve Settings"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // choose the type
    chooseMetric = new QRadioButton(tr("Metric"), this);
    chooseBest = new QRadioButton(tr("Best"), this);
    chooseEstimate = new QRadioButton(tr("Estimate"), this);

    // put them into a button group because we
    // also have radio buttons for watts per kilo / absolute
    group = new QButtonGroup(this);
    group->addButton(chooseMetric);
    group->addButton(chooseBest);
    group->addButton(chooseEstimate);

    // uncheck them all
    chooseMetric->setChecked(false);
    chooseBest->setChecked(false);
    chooseEstimate->setChecked(false);

    // which one ?
    switch (metricDetail->type) {
    default:
        chooseMetric->setChecked(true);
        break;
    case 5:
        chooseBest->setChecked(true);
        break;
    case 6:
        chooseEstimate->setChecked(true);
        break;
    }

    QVBoxLayout *radioButtons = new QVBoxLayout;
    radioButtons->addStretch();
    radioButtons->addWidget(chooseMetric);
    radioButtons->addWidget(chooseBest);
    radioButtons->addWidget(chooseEstimate);
    radioButtons->addStretch();

    // bests selection
    duration = new QDoubleSpinBox(this);
    duration->setDecimals(0);
    duration->setMinimum(0);
    duration->setMaximum(999);
    duration->setSingleStep(1.0);
    duration->setValue(metricDetail->duration); // default to 60 minutes

    durationUnits = new QComboBox(this);
    durationUnits->addItem(tr("seconds"));
    durationUnits->addItem(tr("minutes"));
    durationUnits->addItem(tr("hours"));
    switch(metricDetail->duration_units) {
        case 1 : durationUnits->setCurrentIndex(0); break;
        case 60 : durationUnits->setCurrentIndex(1); break;
        default :
        case 3600 : durationUnits->setCurrentIndex(2); break;
    }

    dataSeries = new QComboBox(this);

    // add all the different series supported
    seriesList << RideFile::watts
               << RideFile::wattsKg
               << RideFile::xPower
               << RideFile::aPower
               << RideFile::NP
               << RideFile::hr
               << RideFile::kph
               << RideFile::cad
               << RideFile::nm
               << RideFile::vam;

    foreach (RideFile::SeriesType x, seriesList) {
            dataSeries->addItem(RideFile::seriesName(x), static_cast<int>(x));
    }

    int index = seriesList.indexOf(metricDetail->series);
    if (index < 0) index = 0;
    dataSeries->setCurrentIndex(index);

    bestWidget = new QWidget(this);
    QVBoxLayout *alignLayout = new QVBoxLayout(bestWidget);
    QGridLayout *bestLayout = new QGridLayout();
    alignLayout->addLayout(bestLayout);

    bestLayout->addWidget(duration, 0,0);
    bestLayout->addWidget(durationUnits, 0,1);
    bestLayout->addWidget(new QLabel(tr("Peak"), this), 1,0);
    bestLayout->addWidget(dataSeries, 1,1);

    // estimate selection
    estimateWidget = new QWidget(this);
    QVBoxLayout *estimateLayout = new QVBoxLayout(estimateWidget);

    modelSelect = new QComboBox(this);
    estimateSelect = new QComboBox(this);

    // working with estimates, local utility functions
    models << new CP2Model(context);
    models << new CP3Model(context);
    models << new MultiModel(context);
    models << new ExtendedModel(context);
    foreach(PDModel *model, models) {
        modelSelect->addItem(model->name(), model->code());
    }

    estimateSelect->addItem("W'");
    estimateSelect->addItem("CP");
    estimateSelect->addItem("FTP");
    estimateSelect->addItem("p-Max");
    estimateSelect->addItem("Best Power");
    estimateSelect->addItem("Endurance Index");

    int n=0;
    modelSelect->setCurrentIndex(0); // default to 2parm model
    foreach(PDModel *model, models) {
        if (model->code() == metricDetail->model) modelSelect->setCurrentIndex(n);
        else n++;
    }
    estimateSelect->setCurrentIndex(metricDetail->estimate);

    estimateDuration = new QDoubleSpinBox(this);
    estimateDuration->setDecimals(0);
    estimateDuration->setMinimum(0);
    estimateDuration->setMaximum(999);
    estimateDuration->setSingleStep(1.0);
    estimateDuration->setValue(metricDetail->estimateDuration); // default to 60 minutes

    estimateDurationUnits = new QComboBox(this);
    estimateDurationUnits->addItem(tr("seconds"));
    estimateDurationUnits->addItem(tr("minutes"));
    estimateDurationUnits->addItem(tr("hours"));
    switch(metricDetail->estimateDuration_units) {
        case 1 : estimateDurationUnits->setCurrentIndex(0); break;
        case 60 : estimateDurationUnits->setCurrentIndex(1); break;
        default :
        case 3600 : estimateDurationUnits->setCurrentIndex(2); break;
    }
    QHBoxLayout *estbestLayout = new QHBoxLayout();
    estbestLayout->addWidget(estimateDuration);
    estbestLayout->addWidget(estimateDurationUnits);

    // estimate as absolute or watts per kilo ?
    abs = new QRadioButton(tr("Absolute"), this);
    wpk = new QRadioButton(tr("Per Kilogram"), this);
    wpk->setChecked(metricDetail->wpk);
    abs->setChecked(!metricDetail->wpk);

    QHBoxLayout *estwpk = new QHBoxLayout;
    estwpk->addStretch();
    estwpk->addWidget(abs);
    estwpk->addWidget(wpk);
    estwpk->addStretch();

    estimateLayout->addStretch();
    estimateLayout->addWidget(modelSelect);
    estimateLayout->addWidget(estimateSelect);
    estimateLayout->addLayout(estbestLayout);
    estimateLayout->addLayout(estwpk);
    estimateLayout->addStretch();

    // metric selection tree
    metricTree = new QTreeWidget;

#ifdef Q_OS_MAC
    metricTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    metricTree->setColumnCount(1);
    metricTree->setSelectionMode(QAbstractItemView::SingleSelection);
    metricTree->header()->hide();
    metricTree->setIndentation(5);

    foreach(MetricDetail metric, ltmTool->metrics) {
        QTreeWidgetItem *add;
        add = new QTreeWidgetItem(metricTree->invisibleRootItem(), METRIC_TYPE);
        add->setText(0, metric.name);
    }
    metricTree->expandItem(metricTree->invisibleRootItem());

    index = indexMetric(metricDetail);
    if (index > 0) {

        // which item to select?
        QTreeWidgetItem *item = metricTree->invisibleRootItem()->child(index);

        // select the current
        metricTree->clearSelection();
        metricTree->setCurrentItem(item, QItemSelectionModel::Select);
    }

    // contains all the different ways of defining
    // a curve, one foreach type. currently just
    // metric and bests, but will add formula and
    // measure at some point
    typeStack = new QStackedWidget(this);
    typeStack->addWidget(metricTree);
    typeStack->addWidget(bestWidget);
    typeStack->addWidget(estimateWidget);
    typeStack->setCurrentIndex(chooseMetric->isChecked() ? 0 : (chooseBest->isChecked() ? 1 : 2));

    // Grid
    QGridLayout *grid = new QGridLayout;

    QLabel *name = new QLabel(tr("Name"));
    QLabel *units = new QLabel(tr("Axis Label / Units"));
    userName = new QLineEdit(this);
    userName->setText(metricDetail->uname);
    userUnits = new QLineEdit(this);
    userUnits->setText(metricDetail->uunits);

    QLabel *style = new QLabel(tr("Style"));
    curveStyle = new QComboBox(this);
    curveStyle->addItem(tr("Bar"), QwtPlotCurve::Steps);
    curveStyle->addItem(tr("Line"), QwtPlotCurve::Lines);
    curveStyle->addItem(tr("Sticks"), QwtPlotCurve::Sticks);
    curveStyle->addItem(tr("Dots"), QwtPlotCurve::Dots);
    curveStyle->setCurrentIndex(curveStyle->findData(metricDetail->curveStyle));

    QLabel *stackLabel = new QLabel(tr("Stack"));
    stack = new QCheckBox("", this);
    stack->setChecked(metricDetail->stack);


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

    QLabel *fill = new QLabel(tr("Fill curve"));
    fillCurve = new QCheckBox("", this);
    fillCurve->setChecked(metricDetail->fillCurve);
 
    labels = new QCheckBox(tr("Data labels"), this);
    labels->setChecked(metricDetail->labels);
 
    // color background...
    penColor = metricDetail->penColor;
    setButtonIcon(penColor);

    QLabel *topN = new QLabel(tr("Highlight Highest"));
    showBest = new QDoubleSpinBox(this);
    showBest->setDecimals(0);
    showBest->setMinimum(0);
    showBest->setMaximum(999);
    showBest->setSingleStep(1.0);
    showBest->setValue(metricDetail->topN);

    QLabel *bottomN = new QLabel(tr("Highlight Lowest"));
    showLowest = new QDoubleSpinBox(this);
    showLowest->setDecimals(0);
    showLowest->setMinimum(0);
    showLowest->setMaximum(999);
    showLowest->setSingleStep(1.0);
    showLowest->setValue(metricDetail->lowestN);

    QLabel *outN = new QLabel(tr("Highlight Outliers"));
    showOut = new QDoubleSpinBox(this);
    showOut->setDecimals(0);
    showOut->setMinimum(0);
    showOut->setMaximum(999);
    showOut->setSingleStep(1.0);
    showOut->setValue(metricDetail->topOut);

    QLabel *baseline = new QLabel(tr("Baseline"));
    baseLine = new QDoubleSpinBox(this);
    baseLine->setDecimals(0);
    baseLine->setMinimum(-999999);
    baseLine->setMaximum(999999);
    baseLine->setSingleStep(1.0);
    baseLine->setValue(metricDetail->baseline);

    curveSmooth = new QCheckBox(tr("Smooth Curve"), this);
    curveSmooth->setChecked(metricDetail->smooth);

    trendType = new QComboBox(this);
    trendType->addItem(tr("No trend Line"));
    trendType->addItem(tr("Linear Trend"));
    trendType->addItem(tr("Quadratic Trend"));
    trendType->setCurrentIndex(metricDetail->trendtype);

    // add to grid
    grid->addLayout(radioButtons, 0, 0, 1, 1, Qt::AlignTop|Qt::AlignLeft);
    grid->addWidget(typeStack, 0, 1, 1, 3);
    QWidget *spacer1 = new QWidget(this);
    spacer1->setFixedHeight(10);
    grid->addWidget(spacer1, 1,0);
    grid->addWidget(name, 2,0);
    grid->addWidget(userName, 2, 1, 1, 3);
    grid->addWidget(units, 3,0);
    grid->addWidget(userUnits, 3,1);
    grid->addWidget(style, 4,0);
    grid->addWidget(curveStyle, 4,1);
    grid->addWidget(symbol, 5,0);
    grid->addWidget(curveSymbol, 5,1);
    QWidget *spacer2 = new QWidget(this);
    spacer2->setFixedHeight(10);
    grid->addWidget(spacer2, 6,0);
    grid->addWidget(stackLabel, 7, 0);
    grid->addWidget(stack, 7, 1);
    grid->addWidget(color, 8,0);
    grid->addWidget(curveColor, 8,1);
    grid->addWidget(fill, 9,0);
    grid->addWidget(fillCurve, 9,1);
    grid->addWidget(topN, 3,2);
    grid->addWidget(showBest, 3,3);
    grid->addWidget(bottomN, 4,2);
    grid->addWidget(showLowest, 4,3);
    grid->addWidget(outN, 5,2);
    grid->addWidget(showOut, 5,3);
    grid->addWidget(baseline, 6, 2);
    grid->addWidget(baseLine, 6,3);
    grid->addWidget(trendType, 7,2);
    grid->addWidget(curveSmooth, 8,2);
    grid->addWidget(labels, 9,2);

    mainLayout->addLayout(grid);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    applyButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);
    mainLayout->addLayout(buttonLayout);

    // clean up the estimate widgets
    modelChanged();

    // connect up slots
    connect(metricTree, SIGNAL(itemSelectionChanged()), this, SLOT(metricSelected()));
    connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(curveColor, SIGNAL(clicked()), this, SLOT(colorClicked()));
    connect(chooseMetric, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(chooseBest, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(chooseEstimate, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(modelSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(modelChanged()));
    connect(estimateSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(estimateChanged()));
    connect(estimateDuration, SIGNAL(valueChanged(double)), this, SLOT(estimateName()));
    connect(estimateDurationUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(estimateName()));

    // when stuff changes rebuild name
    connect(chooseBest, SIGNAL(toggled(bool)), this, SLOT(bestName()));
    connect(chooseEstimate, SIGNAL(toggled(bool)), this, SLOT(estimateName()));
    connect(chooseMetric, SIGNAL(toggled(bool)), this, SLOT(metricSelected()));
    connect(duration, SIGNAL(valueChanged(double)), this, SLOT(bestName()));
    connect(durationUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(bestName()));
    connect(dataSeries, SIGNAL(currentIndexChanged(int)), this, SLOT(bestName()));
}

int
EditMetricDetailDialog::indexMetric(MetricDetail *metricDetail)
{
    for (int i=0; i < ltmTool->metrics.count(); i++) {
        if (ltmTool->metrics.at(i).symbol == metricDetail->symbol) return i;
    }
    return -1;
}

void
EditMetricDetailDialog::typeChanged()
{
    // switch stack and hide other
    if (chooseMetric->isChecked()) {
        bestWidget->hide();
        metricTree->show();
        estimateWidget->hide();
        typeStack->setCurrentIndex(0);
    }

    if (chooseBest->isChecked()) {
        bestWidget->show();
        metricTree->hide();
        estimateWidget->hide();
        typeStack->setCurrentIndex(1);
    }

    if (chooseEstimate->isChecked()) {
        bestWidget->hide();
        metricTree->hide();
        estimateWidget->show();
        typeStack->setCurrentIndex(2);
    }
    adjustSize();
}

void
EditMetricDetailDialog::bestName()
{
    // only for bests!
    if (chooseBest->isChecked() == false) return;

    // when widget destroyed we get negative indexes so ignore
    if (durationUnits->currentIndex() < 0 || dataSeries->currentIndex() < 0) return;

    // set uname from current parms
    QString desc = QString(tr("Peak %1")).arg(duration->value());
    switch (durationUnits->currentIndex()) {
    case 0 : desc += tr(" second "); break;
    case 1 : desc += tr(" minute "); break;
    default:
    case 2 : desc += tr(" hour "); break;
    }
    desc += RideFile::seriesName(seriesList.at(dataSeries->currentIndex()));
    userName->setText(desc);
    metricDetail->bestSymbol = desc.replace(" ", "_");
}

void
EditMetricDetailDialog::metricSelected()
{
    // only in metric mode
    if (!chooseMetric->isChecked()) return;

    // user selected a different metric
    // so update accordingly
    int index = metricTree->invisibleRootItem()->indexOfChild(metricTree->currentItem());

    // out of bounds !
    if (index < 0 || index >= ltmTool->metrics.count()) return;

    userName->setText(ltmTool->metrics[index].uname);
    userUnits->setText(ltmTool->metrics[index].uunits);
    curveSmooth->setChecked(ltmTool->metrics[index].smooth);
    fillCurve->setChecked(ltmTool->metrics[index].fillCurve);
    labels->setChecked(ltmTool->metrics[index].labels);
    stack->setChecked(ltmTool->metrics[index].stack);
    showBest->setValue(ltmTool->metrics[index].topN);
    showOut->setValue(ltmTool->metrics[index].topOut);
    baseLine->setValue(ltmTool->metrics[index].baseline);
    penColor = ltmTool->metrics[index].penColor;
    trendType->setCurrentIndex(ltmTool->metrics[index].trendtype);
    setButtonIcon(penColor);

    // curve style
    switch (ltmTool->metrics[index].curveStyle) {
      
    case QwtPlotCurve::Steps:
        curveStyle->setCurrentIndex(0);
        break;
    case QwtPlotCurve::Lines:
        curveStyle->setCurrentIndex(1);
        break;
    case QwtPlotCurve::Sticks:
        curveStyle->setCurrentIndex(2);
        break;
    case QwtPlotCurve::Dots:
    default:
        curveStyle->setCurrentIndex(3);
        break;

    }

    // curveSymbol
    switch (ltmTool->metrics[index].symbolStyle) {
      
    case QwtSymbol::NoSymbol:
        curveSymbol->setCurrentIndex(0);
        break;
    case QwtSymbol::Ellipse:
        curveSymbol->setCurrentIndex(1);
        break;
    case QwtSymbol::Rect:
        curveSymbol->setCurrentIndex(2);
        break;
    case QwtSymbol::Diamond:
        curveSymbol->setCurrentIndex(3);
        break;
    case QwtSymbol::Triangle:
        curveSymbol->setCurrentIndex(4);
        break;
    case QwtSymbol::XCross:
        curveSymbol->setCurrentIndex(5);
        break;
    case QwtSymbol::Hexagon:
        curveSymbol->setCurrentIndex(6);
        break;
    case QwtSymbol::Star1:
    default:
        curveSymbol->setCurrentIndex(7);
        break;

    }

    (*metricDetail) = ltmTool->metrics[index]; // overwrite!
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
    if (chooseMetric->isChecked()) { // is a metric, but what type?
        int index = indexMetric(metricDetail);
        if (index >= 0) metricDetail->type = ltmTool->metrics.at(index).type;
        else metricDetail->type = 1;
    }

    if (chooseBest->isChecked()) metricDetail->type = 5; // is a best
    else if (chooseEstimate->isChecked()) metricDetail->type = 6; // estimate

    metricDetail->estimateDuration = estimateDuration->value();
    switch (estimateDurationUnits->currentIndex()) {
        case 0 : metricDetail->estimateDuration_units = 1; break;
        case 1 : metricDetail->estimateDuration_units = 60; break;
        case 2 :
        default: metricDetail->estimateDuration_units = 3600; break;
    }

    metricDetail->duration = duration->value();
    switch (durationUnits->currentIndex()) {
        case 0 : metricDetail->duration_units = 1; break;
        case 1 : metricDetail->duration_units = 60; break;
        case 2 :
        default: metricDetail->duration_units = 3600; break;
    }
    metricDetail->wpk = wpk->isChecked();
    metricDetail->series = seriesList.at(dataSeries->currentIndex());
    metricDetail->model = models[modelSelect->currentIndex()]->code();
    metricDetail->estimate = estimateSelect->currentIndex(); // 0 - 5
    metricDetail->smooth = curveSmooth->isChecked();
    metricDetail->topN = showBest->value();
    metricDetail->lowestN = showLowest->value();
    metricDetail->topOut = showOut->value();
    metricDetail->baseline = baseLine->value();
    metricDetail->curveStyle = styleMap[curveStyle->currentIndex()];
    metricDetail->symbolStyle = symbolMap[curveSymbol->currentIndex()];
    metricDetail->penColor = penColor;
    metricDetail->fillCurve = fillCurve->isChecked();
    metricDetail->labels = labels->isChecked();
    metricDetail->uname = userName->text();
    metricDetail->uunits = userUnits->text();
    metricDetail->stack = stack->isChecked();
    metricDetail->trendtype = trendType->currentIndex();
    accept();
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
EditMetricDetailDialog::cancelClicked()
{
    reject();
}

void
EditMetricDetailDialog::colorClicked()
{
    QColorDialog picker(context->mainWindow);
    picker.setCurrentColor(penColor);

    // don't use native dialog, since there is a nasty bug causing focus loss
    // see https://bugreports.qt-project.org/browse/QTBUG-14889
    QColor color = picker.getColor(metricDetail->penColor, this, tr("Choose Metric Color"), QColorDialog::DontUseNativeDialog);

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


// metricDetails gives access to the metric details catalog by symbol
MetricDetail*
LTMTool::metricDetails(QString symbol)
{
    for(int i = 0; i < metrics.count(); i++)
        if (metrics[i].symbol == symbol)
            return &metrics[i];
    return NULL;
}

void
LTMTool::translateMetrics(Context *context, LTMSettings *settings) // settings override local scope (static function)!!
{
    static QMap<QString, QString> unitsMap;
    // LTMTool instance is created to have access to metrics catalog
    LTMTool* ltmTool = new LTMTool(context, settings);
    if (unitsMap.isEmpty()) {
        foreach(MetricDetail metric, ltmTool->metrics) {
            if (metric.units != "")  // translate units
	            unitsMap.insert(metric.units, metric.uunits);
            if (metric.uunits != "") // keep already translated the same
	            unitsMap.insert(metric.uunits, metric.uunits);
        }
    }
    for (int j=0; j < settings->metrics.count(); j++) {
        if (settings->metrics[j].uname == settings->metrics[j].name) {
            MetricDetail* mdp = ltmTool->metricDetails(settings->metrics[j].symbol);
            if (mdp != NULL) {
                // Replace with default translated name
                settings->metrics[j].name = mdp->name;
                settings->metrics[j].uname = mdp->uname;
                // replace with translated units, if available
                if (settings->metrics[j].uunits != "")
                    settings->metrics[j].uunits = unitsMap.value(settings->metrics[j].uunits,
                                                                 mdp->uunits);
            }
        }
    }
    delete ltmTool;
}

void
LTMTool::editingStarted()
{
    editing = true; // also set from renameClicked
}

void
LTMTool::editingFinished()
{
    if (!editing) return;

    // take the edited versions of the name first
    for(int i=0; i<charts->invisibleRootItem()->childCount(); i++)
        (context->athlete->presets)[i].name = charts->invisibleRootItem()->child(i)->text(0);

    // let everyone know once we're done
    editing = false;
    context->notifyPresetsChanged();
}

void
LTMTool::importClicked()
{
    QFileDialog existing(this);
    existing.setFileMode(QFileDialog::ExistingFile);
    existing.setNameFilter(tr("Chart File (*.xml)"));
    if (existing.exec()){
        // we will only get one (ExistingFile not ExistingFiles)
        QStringList filenames = existing.selectedFiles();

        if (QFileInfo(filenames[0]).exists()) {

            QList<LTMSettings> imported;
            QFile chartsFile(filenames[0]);

            // setup XML processor
            QXmlInputSource source( &chartsFile );
            QXmlSimpleReader xmlReader;
            LTMChartParser handler;
            xmlReader.setContentHandler(&handler);
            xmlReader.setErrorHandler(&handler);

            // parse and get return values
            xmlReader.parse(source);
            imported = handler.getSettings();

            // now append to the QList and QTreeWidget
            context->athlete->presets += imported;

            // notify we changed and tree updates
            editing = false;
            context->notifyPresetsChanged();

        } else {
            // oops non existent - does this ever happen?
            QMessageBox::warning( 0, tr("Entry Error"), QString(tr("Selected file (%1) does not exist")).arg(filenames[0]));
        }
    }
}

void
LTMTool::exportClicked()
{
    QFileDialog newone(this);
    newone.setFileMode(QFileDialog::AnyFile);
    newone.setNameFilter(tr("Chart File (*.xml)"));
    if (newone.exec()){
        // we will only get one (ExistingFile not ExistingFiles)
        QStringList filenames = newone.selectedFiles();

        // if exists confirm overwrite
        if (QFileInfo(filenames[0]).exists()) {
            QMessageBox msgBox;
            msgBox.setText(QString(tr("The selected file (%1) exists.")).arg(filenames[0]));
            msgBox.setInformativeText(tr("Do you want to overwrite it?"));
            msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            msgBox.setDefaultButton(QMessageBox::Cancel);
            msgBox.setIcon(QMessageBox::Warning);
            if (msgBox.exec() != QMessageBox::Ok)
                return;
        }
        LTMChartParser::serialize(filenames[0], context->athlete->presets);
    }
}

void
LTMTool::upClicked()
{
    if (charts->currentItem()) {
        int index = charts->invisibleRootItem()->indexOfChild(charts->currentItem());
        if (index == 0) return; // its at the top already

        // movin on up!
        LTMSettings save = (context->athlete->presets)[index];
        context->athlete->presets.removeAt(index);
        context->athlete->presets.insert(index-1, save);

        // notify we changed
        editing = false;
        context->notifyPresetsChanged();

        // reselect
        charts->setCurrentItem(charts->invisibleRootItem()->child(index-1));
    }
}

void
LTMTool::downClicked()
{
    if (charts->currentItem()) {
        int index = charts->invisibleRootItem()->indexOfChild(charts->currentItem());
        if (index == (charts->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        // movin on up!
        LTMSettings save = (context->athlete->presets)[index];
        context->athlete->presets.removeAt(index);
        context->athlete->presets.insert(index+1, save);

        // notify we changed
        editing = false;
        context->notifyPresetsChanged();

        // reselect
        charts->setCurrentItem(charts->invisibleRootItem()->child(index+1));
    }
}

void
LTMTool::renameClicked()
{
    // which one is selected?
    if (charts->currentItem()) {
        editing = true;
        charts->editItem(charts->currentItem(), 0);
    }
}

void
LTMTool::deleteClicked()
{
    // must have at least 1 child
    if (charts->invisibleRootItem()->childCount() == 1) {
        QMessageBox::warning(0, tr("Error"), tr("You must have at least one chart"));
        return;

    } else if (charts->currentItem()) {

        int index = charts->invisibleRootItem()->indexOfChild(charts->currentItem());

        // zap!
        context->athlete->presets.removeAt(index);

        // notify we changed
        editing = false;
        context->notifyPresetsChanged();
    }
}
