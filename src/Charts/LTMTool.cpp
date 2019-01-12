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
#include "Tab.h"
#include "RideNavigator.h"
#include "HelpWhatsThis.h"
#include "Utils.h"

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

// Filter / formula
#include "DataFilter.h"

LTMTool::LTMTool(Context *context, LTMSettings *settings) : QWidget(context->mainWindow), settings(settings), context(context), active(false), _amFiltered(false)
{
    setStyleSheet("QFrame { FrameStyle = QFrame::NoFrame };"
                  "QWidget { background = Qt::white; border:0 px; margin: 2px; };");

    // get application settings
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);

    //----------------------------------------------------------------------------------------------------------
    // Basic Settings (First TAB)
    //----------------------------------------------------------------------------------------------------------
    basicsettings = new QWidget(this);
    HelpWhatsThis *basicHelp = new HelpWhatsThis(basicsettings);
    basicsettings->setWhatsThis(basicHelp->getWhatsThisText(HelpWhatsThis::ChartTrends_MetricTrends_Config_Basic));

    QFormLayout *basicsettingsLayout = new QFormLayout(basicsettings);

    searchBox = new SearchFilterBox(this, context);
    HelpWhatsThis *searchHelp = new HelpWhatsThis(searchBox);
    searchBox->setWhatsThis(searchHelp->getWhatsThisText(HelpWhatsThis::SearchFilterBox));
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));

    basicsettingsLayout->addRow(new QLabel(tr("Filter")), searchBox);
    basicsettingsLayout->addRow(new QLabel(tr(""))); // spacing

    dateSetting = new DateSettingsEdit(this);
    HelpWhatsThis *dateSettingHelp = new HelpWhatsThis(dateSetting);
    dateSetting->setWhatsThis(dateSettingHelp->getWhatsThisText(HelpWhatsThis::ChartTrends_DateRange));
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

    showBanister = new QCheckBox(tr("Show Banister Helper"));
    showBanister->setChecked(true); //enable by default
    basicsettingsLayout->addRow(new QLabel(""), showBanister);

    stackSlider = new QSlider(Qt::Horizontal,this);
    stackSlider->setMinimum(0);
    stackSlider->setMaximum(7);
    stackSlider->setTickInterval(1);
    stackSlider->setValue(3);
    stackSlider->setFixedWidth(100);
    basicsettingsLayout->addRow(new QLabel(tr("Stack Zoom")), stackSlider);
    // use separate line to distinguish from the operational buttons for the Table View

    usePreset = new QCheckBox(tr("Use sidebar chart settings"));
    usePreset->setChecked(false);
    basicsettingsLayout->addRow(new QLabel(""), new QLabel());
    basicsettingsLayout->addRow(new QLabel(""), usePreset);

    //----------------------------------------------------------------------------------------------------------
    // Preset List (2nd TAB)
    //----------------------------------------------------------------------------------------------------------

    presets = new QWidget(this);

    presets->setContentsMargins(20,20,20,20);
    HelpWhatsThis *presetHelp = new HelpWhatsThis(presets);
    presets->setWhatsThis(presetHelp->getWhatsThisText(HelpWhatsThis::ChartTrends_MetricTrends_Config_Preset));
    QVBoxLayout *presetLayout = new QVBoxLayout(presets);
    presetLayout->setContentsMargins(0,0,0,0);
    presetLayout->setSpacing(5 *dpiXFactor);

    charts = new QTreeWidget;
#ifdef Q_OS_MAC
    charts->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    charts->headerItem()->setText(0, tr("Charts"));
    charts->setColumnCount(1);
    charts->setSelectionMode(QAbstractItemView::SingleSelection);
    charts->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    charts->setIndentation(0);

    presetLayout->addWidget(charts, 0,0);

    applyButton = new QPushButton(tr("Apply")); // connected in LTMWindow.cpp (weird!?)
    newButton = new QPushButton(tr("Add Current"));
    connect(newButton, SIGNAL(clicked()), this, SLOT(addCurrent()));

    QHBoxLayout *presetButtons = new QHBoxLayout;
    presetButtons->addWidget(applyButton);
    presetButtons->addStretch();
    presetButtons->addWidget(newButton);

    presetLayout->addLayout(presetButtons);



    //----------------------------------------------------------------------------------------------------------
    // initialise the metrics catalogue and user selector (for Custom Curves - 4th TAB)
    //----------------------------------------------------------------------------------------------------------

    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i = 0; i < factory.metricCount(); ++i) {

        // don't add compatibility metrics to config tool, we support them
        // for previously configured charts, but not for creating new ones.
        if (factory.metricName(i).startsWith("compatibility_")) continue;

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

        adds.name   = Utils::unprotect(adds.metric->name());

        // set default for the user overiddable fields
        adds.uname  = adds.name;
        adds.units = adds.metric->units(context->athlete->useMetricUnits);
        adds.uunits = adds.units;

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
            metametric.name = field.name; // used to retrieve metadata value
            metametric.uname = sp.displayName(field.name);
            metametric.units = "";
            metametric.uunits = "";
            metrics.append(metametric);
        }
    }

    // sort the list
    qSort(metrics);

    //----------------------------------------------------------------------------------------------------------
    // Custom Curves (4th TAB)
    //----------------------------------------------------------------------------------------------------------

    custom = new QWidget(this);
    custom->setContentsMargins(20,20,20,20);
    HelpWhatsThis *curvesHelp = new HelpWhatsThis(custom);
    custom->setWhatsThis(curvesHelp->getWhatsThisText(HelpWhatsThis::ChartTrends_MetricTrends_Config_Curves));
    QVBoxLayout *customLayout = new QVBoxLayout(custom);
    customLayout->setContentsMargins(0,0,0,0);
    customLayout->setSpacing(5 *dpiXFactor);

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

    deleteCustomButton = new QPushButton("-");
    connect(deleteCustomButton, SIGNAL(clicked()), this, SLOT(deleteMetric()));

#ifndef Q_OS_MAC
    upCustomButton = new QToolButton(this);
    downCustomButton = new QToolButton(this);
    upCustomButton->setArrowType(Qt::UpArrow);
    downCustomButton->setArrowType(Qt::DownArrow);
    upCustomButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    downCustomButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    addCustomButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    deleteCustomButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
#else
    upCustomButton = new QPushButton(tr("Up"));
    downCustomButton = new QPushButton(tr("Down"));
#endif
    connect(upCustomButton, SIGNAL(clicked()), this, SLOT(moveMetricUp()));
    connect(downCustomButton, SIGNAL(clicked()), this, SLOT(moveMetricDown()));


    QHBoxLayout *customButtons = new QHBoxLayout;
    customButtons->setSpacing(2 *dpiXFactor);
    customButtons->addWidget(upCustomButton);
    customButtons->addWidget(downCustomButton);
    customButtons->addStretch();
    customButtons->addWidget(editCustomButton);
    customButtons->addStretch();
    customButtons->addWidget(addCustomButton);
    customButtons->addWidget(deleteCustomButton);
    customLayout->addLayout(customButtons);

    //----------------------------------------------------------------------------------------------------------
    // setup the Tabs
    //----------------------------------------------------------------------------------------------------------

    tabs = new QTabWidget(this);
    mainLayout->addWidget(tabs);
    tabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    tabs->addTab(basicsettings, tr("Basic"));
    tabs->addTab(presets, tr("Preset"));
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

    MetricDetail cogganExpectedCTL;
    cogganExpectedCTL.type = METRIC_PM;
    cogganExpectedCTL.symbol = "expected_coggan_ctl";
    cogganExpectedCTL.metric = NULL; // not a factory metric
    cogganExpectedCTL.penColor = QColor(Qt::blue);
    cogganExpectedCTL.curveStyle = QwtPlotCurve::Dots;
    cogganExpectedCTL.symbolStyle = QwtSymbol::NoSymbol;
    cogganExpectedCTL.smooth = false;
    cogganExpectedCTL.trendtype = 0;
    cogganExpectedCTL.topN = 1;
    cogganExpectedCTL.uname = cogganExpectedCTL.name = tr("Coggan Expected Chronic Training Load");
    cogganExpectedCTL.units = "CTL";
    cogganExpectedCTL.uunits = "CTL";
    metrics.append(cogganExpectedCTL);

    MetricDetail cogganExpectedATL;
    cogganExpectedATL.type = METRIC_PM;
    cogganExpectedATL.symbol = "expected_coggan_atl";
    cogganExpectedATL.metric = NULL; // not a factory metric
    cogganExpectedATL.penColor = QColor(Qt::magenta);
    cogganExpectedATL.curveStyle = QwtPlotCurve::Dots;
    cogganExpectedATL.symbolStyle = QwtSymbol::NoSymbol;
    cogganExpectedATL.smooth = false;
    cogganExpectedATL.trendtype = 0;
    cogganExpectedATL.topN = 1;
    cogganExpectedATL.uname = cogganExpectedATL.name = tr("Coggan Expected Acute Training Load");
    cogganExpectedATL.units = "ATL";
    cogganExpectedATL.uunits = "ATL";
    metrics.append(cogganExpectedATL);

    MetricDetail cogganExpectedTSB;
    cogganExpectedTSB.type = METRIC_PM;
    cogganExpectedTSB.symbol = "expected_coggan_tsb";
    cogganExpectedTSB.metric = NULL; // not a factory metric
    cogganExpectedTSB.penColor = QColor(Qt::yellow);
    cogganExpectedTSB.curveStyle = QwtPlotCurve::Dots;
    cogganExpectedTSB.symbolStyle = QwtSymbol::NoSymbol;
    cogganExpectedTSB.smooth = false;
    cogganExpectedTSB.trendtype = 0;
    cogganExpectedTSB.topN = 1;
    cogganExpectedTSB.uname = cogganExpectedTSB.name = tr("Coggan Expected Training Stress Balance");
    cogganExpectedTSB.units = "TSB";
    cogganExpectedTSB.uunits = "TSB";
    metrics.append(cogganExpectedTSB);

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

    // TriScore LTS
    MetricDetail triscoreLTS;
    triscoreLTS.type = METRIC_PM;
    triscoreLTS.symbol = "triscore_lts";
    triscoreLTS.metric = NULL; // not a factory metric
    triscoreLTS.penColor = QColor(Qt::blue);
    triscoreLTS.curveStyle = QwtPlotCurve::Lines;
    triscoreLTS.symbolStyle = QwtSymbol::NoSymbol;
    triscoreLTS.smooth = false;
    triscoreLTS.trendtype = 0;
    triscoreLTS.topN = 1;
    triscoreLTS.uname = triscoreLTS.name = tr("TriScore Long Term Stress");
    triscoreLTS.units = "Stress";
    triscoreLTS.uunits = tr("Stress");
    metrics.append(triscoreLTS);

    MetricDetail triscoreSTS;
    triscoreSTS.type = METRIC_PM;
    triscoreSTS.symbol = "triscore_sts";
    triscoreSTS.metric = NULL; // not a factory metric
    triscoreSTS.penColor = QColor(Qt::magenta);
    triscoreSTS.curveStyle = QwtPlotCurve::Lines;
    triscoreSTS.symbolStyle = QwtSymbol::NoSymbol;
    triscoreSTS.smooth = false;
    triscoreSTS.trendtype = 0;
    triscoreSTS.topN = 1;
    triscoreSTS.uname = triscoreSTS.name = tr("TriScore Short Term Stress");
    triscoreSTS.units = "Stress";
    triscoreSTS.uunits = tr("Stress");
    metrics.append(triscoreSTS);

    MetricDetail triscoreSB;
    triscoreSB.type = METRIC_PM;
    triscoreSB.symbol = "triscore_sb";
    triscoreSB.metric = NULL; // not a factory metric
    triscoreSB.penColor = QColor(Qt::yellow);
    triscoreSB.curveStyle = QwtPlotCurve::Steps;
    triscoreSB.symbolStyle = QwtSymbol::NoSymbol;
    triscoreSB.smooth = false;
    triscoreSB.trendtype = 0;
    triscoreSB.topN = 1;
    triscoreSB.uname = triscoreSB.name = tr("TriScore Stress Balance");
    triscoreSB.units = "Stress Balance";
    triscoreSB.uunits = tr("Stress Balance");
    metrics.append(triscoreSB);

    MetricDetail triscoreSTR;
    triscoreSTR.type = METRIC_PM;
    triscoreSTR.symbol = "triscore_sr";
    triscoreSTR.metric = NULL; // not a factory metric
    triscoreSTR.penColor = QColor(Qt::darkGreen);
    triscoreSTR.curveStyle = QwtPlotCurve::Steps;
    triscoreSTR.symbolStyle = QwtSymbol::NoSymbol;
    triscoreSTR.smooth = false;
    triscoreSTR.trendtype = 0;
    triscoreSTR.topN = 1;
    triscoreSTR.uname = triscoreSTR.name = tr("TriScore STS Ramp");
    triscoreSTR.units = "Ramp";
    triscoreSTR.uunits = tr("Ramp");
    metrics.append(triscoreSTR);

    MetricDetail triscoreLTR;
    triscoreLTR.type = METRIC_PM;
    triscoreLTR.symbol = "triscore_lr";
    triscoreLTR.metric = NULL; // not a factory metric
    triscoreLTR.penColor = QColor(Qt::darkBlue);
    triscoreLTR.curveStyle = QwtPlotCurve::Steps;
    triscoreLTR.symbolStyle = QwtSymbol::NoSymbol;
    triscoreLTR.smooth = false;
    triscoreLTR.trendtype = 0;
    triscoreLTR.topN = 1;
    triscoreLTR.uname = triscoreLTR.name = tr("TriScore LTS Ramp");
    triscoreLTR.units = "Ramp";
    triscoreLTR.uunits = tr("Ramp");
    metrics.append(triscoreLTR);

    // done

    return metrics;

}

void
LTMTool::hideBasic()
{
    // first make sure use sidebar is false
    usePreset->setChecked(false);
    if (tabs->count() == 3) {

        tabs->removeTab(0);
        basicsettings->hide(); // it doesn't get deleted

        // resize etc
        tabs->updateGeometry();
        presets->updateGeometry();
        custom->updateGeometry();

        // choose curves tab
        tabs->setCurrentIndex(1);
    }
}

void
LTMTool::usePresetChanged()
{
    customTable->setEnabled(!usePreset->isChecked());
    editCustomButton->setEnabled(!usePreset->isChecked());
    addCustomButton->setEnabled(!usePreset->isChecked());
    deleteCustomButton->setEnabled(!usePreset->isChecked());
    upCustomButton->setEnabled(!usePreset->isChecked());
    downCustomButton->setEnabled(!usePreset->isChecked());

    // yuck .. this doesn't work nicely !
    //basic->setHidden(usePreset->isChecked());
    //custom->setHidden(usePreset->isChecked());
    // so instead we disable
    charts->setEnabled(!usePreset->isChecked());
    newButton->setEnabled(!usePreset->isChecked());
    applyButton->setEnabled(!usePreset->isChecked());

}

void
LTMTool::presetsChanged()
{
    // rebuild the preset chart list as the presets have changed
    charts->clear();
    foreach(LTMSettings chart, context->athlete->presets) {
        QTreeWidgetItem *add;
        add = new QTreeWidgetItem(charts->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);
        add->setText(0, chart.name);
    }

    // select the first one, if there are any
    if (context->athlete->presets.count())
        charts->setCurrentItem(charts->invisibleRootItem()->child(0));
}


void
LTMTool::refreshCustomTable(int indexSelectedItem)
{
    // clear then repopulate custom table settings to reflect
    // the current LTMSettings.
    customTable->clear();

    // get headers back
    QStringList header;
    header << tr("Type") << tr("Details"); 
    customTable->setHorizontalHeaderLabels(header);

    QTableWidgetItem *selected = new QTableWidgetItem();
    // now lets add a row for each metric
    customTable->setRowCount(settings->metrics.count());
    int i=0;
    foreach (MetricDetail metricDetail, settings->metrics) {

        QTableWidgetItem *t = new QTableWidgetItem();
        if (metricDetail.type < 5)
            t->setText(tr("Metric")); // only metrics .. for now ..
        else if (metricDetail.type == 5)
            t->setText(tr("Peak"));
        else if (metricDetail.type == 6)
            t->setText(tr("Estimate"));
        else if (metricDetail.type == 7)
            t->setText(tr("Stress"));
        else if (metricDetail.type == 8)
            t->setText(tr("Formula"));
        else if (metricDetail.type == 9)
            t->setText(tr("Measure"));
        else if (metricDetail.type == 10)
            t->setText(tr("Performance"));
        else if (metricDetail.type == 11)
            t->setText(tr("Banister"));


        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        customTable->setItem(i,0,t);

        t = new QTableWidgetItem();
        if (metricDetail.type == 8) {
            t->setText(metricDetail.formula);
        } else if (metricDetail.type != 5 && metricDetail.type != 6 && metricDetail.type != 9  && metricDetail.type != 10 && metricDetail.type != 11)
            t->setText(metricDetail.name);
        else {
            // text description for peak && measure
            t->setText(metricDetail.uname);
        }

        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        customTable->setItem(i,1,t);

        // keep the selected item from previous step (relevant for moving up/down)
        if (indexSelectedItem == i) {
            selected = t;
        }

        i++;
    }

    if (selected) {
      customTable->setCurrentItem(selected);
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
LTMTool::moveMetricUp()
{
    QList<QTableWidgetItem*> items = customTable->selectedItems();
    if (items.count() < 1) return;

    int index = customTable->row(items.first());

    if (index > 0) {
        settings->metrics.swap(index, index-1);
         // refresh
        refreshCustomTable(index-1);
        curvesChanged();
    }
}

void
LTMTool::moveMetricDown()
{
    QList<QTableWidgetItem*> items = customTable->selectedItems();
    if (items.count() < 1) return;

    int index = customTable->row(items.first());

    if (index+1 <  settings->metrics.size()) {
        settings->metrics.swap(index, index+1);
         // refresh
        refreshCustomTable(index+1);
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
    context->notifyPresetsChanged();
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
    qobject_cast<QStandardItemModel *>(estimateSelect->model())->item(6)->setEnabled(true);

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
        case 6 : name = tr("Vo2Max Estimate"); break;
    }

    // now the model
    name += " (" + models[modelSelect->currentIndex()]->code() + ")";
    userName->setText(name);
    metricDetail->symbol = name.replace(" ", "_");
}

// set measureFieldSelect contents based on current measureGroupSelect
void
EditMetricDetailDialog::measureGroupChanged()
{
    measureFieldSelect->clear();
    int currentIndex=measureGroupSelect->currentIndex();
    foreach(QString field, context->athlete->measures->getFieldNames(currentIndex)) {
        measureFieldSelect->addItem(field);
    }
    measureFieldSelect->setCurrentIndex(metricDetail->measureField);
    measureName();
}

/*----------------------------------------------------------------------
 * EDIT METRIC DETAIL DIALOG
 *--------------------------------------------------------------------*/

static bool insensitiveLessThan(const QString &a, const QString &b)
{
    return a.toLower() < b.toLower();
}

EditMetricDetailDialog::EditMetricDetailDialog(Context *context, LTMTool *ltmTool, MetricDetail *metricDetail) :
    QDialog(context->mainWindow, Qt::Dialog), context(context), ltmTool(ltmTool), metricDetail(metricDetail)
{
    setWindowTitle(tr("Curve Settings"));

    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartTrends_MetricTrends_Curves_Settings));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // choose the type
    chooseMetric = new QRadioButton(tr("Metric"), this);
    chooseBest = new QRadioButton(tr("Best"), this);
    chooseEstimate = new QRadioButton(tr("Estimate"), this);
    chooseStress = new QRadioButton(tr("PMC"), this);
    chooseFormula = new QRadioButton(tr("Formula"), this);
    chooseMeasure = new QRadioButton(tr("Measure"), this);
    choosePerformance = new QRadioButton(tr("Performance"), this);
    chooseBanister = new QRadioButton(tr("Banister"), this);

    // put them into a button group because we
    // also have radio buttons for watts per kilo / absolute
    group = new QButtonGroup(this);
    group->addButton(chooseMetric);
    group->addButton(chooseBest);
    group->addButton(chooseEstimate);
    group->addButton(chooseStress);
    group->addButton(chooseBanister);
    group->addButton(choosePerformance);
    group->addButton(chooseFormula);
    group->addButton(chooseMeasure);

    // uncheck them all
    chooseMetric->setChecked(false);
    chooseBest->setChecked(false);
    chooseEstimate->setChecked(false);
    chooseStress->setChecked(false);
    chooseFormula->setChecked(false);
    chooseMeasure->setChecked(false);
    choosePerformance->setChecked(false);
    chooseBanister->setChecked(false);

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
    case 7:
        chooseStress->setChecked(true);
        break;
    case 8:
        chooseFormula->setChecked(true);
        break;
    case 9:
        chooseMeasure->setChecked(true);
        break;
    case 10:
        choosePerformance->setChecked(true);
        break;
    case 11:
        chooseBanister->setChecked(true);
    }

    QVBoxLayout *radioButtons = new QVBoxLayout;
    radioButtons->addStretch();
    radioButtons->addWidget(chooseMetric);
    radioButtons->addWidget(chooseBest);
    radioButtons->addWidget(chooseEstimate);
    radioButtons->addWidget(chooseStress);
    radioButtons->addWidget(chooseBanister);
    radioButtons->addWidget(choosePerformance);
    radioButtons->addWidget(chooseFormula);
    radioButtons->addWidget(chooseMeasure);
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
               << RideFile::IsoPower
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
    models << new WSModel(context);
    foreach(PDModel *model, models) {
        modelSelect->addItem(model->name(), model->code());
    }

    estimateSelect->addItem("W'");
    estimateSelect->addItem("CP");
    estimateSelect->addItem("FTP");
    estimateSelect->addItem("p-Max");
    estimateSelect->addItem("Best Power");
    estimateSelect->addItem("Endurance Index");
    estimateSelect->addItem("Vo2Max Estimate");

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

    // estimate selection
    formulaWidget = new QWidget(this);
    //formulaWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *formulaLayout = new QVBoxLayout(formulaWidget);

    // courier font
    formulaEdit = new DataFilterEdit(this, context);
    QFont courier("Courier", QFont().pointSize());
    QFontMetrics fm(courier);

    formulaEdit->setFont(courier);
    formulaEdit->setTabStopWidth(4 * fm.width(' ')); // 4 char tabstop
    //formulaEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    formulaType = new QComboBox(this);
    formulaType->addItem(tr("Total"), static_cast<int>(RideMetric::Total));
    formulaType->addItem(tr("Running Total"), static_cast<int>(RideMetric::RunningTotal));
    formulaType->addItem(tr("Average"), static_cast<int>(RideMetric::Average));
    formulaType->addItem(tr("Peak"), static_cast<int>(RideMetric::Peak));
    formulaType->addItem(tr("Low"), static_cast<int>(RideMetric::Low));
    formulaLayout->addWidget(formulaEdit);
    QHBoxLayout *ftype = new QHBoxLayout;
    ftype->addWidget(new QLabel(tr("Aggregate:")));
    ftype->addWidget(formulaType);
    ftype->addStretch();
    formulaLayout->addLayout(ftype);

    // set to the value...
    if (metricDetail->formula == "") {
        // lets put a template in there
        metricDetail->formula = tr("# type in a formula to use\n" 
                                   "# for e.g. BikeStress / Duration\n"
                                   "# as you type the available metrics\n"
                                   "# will be offered by autocomplete\n");
    }
    formulaEdit->setText(metricDetail->formula);
    formulaType->setCurrentIndex(formulaType->findData(metricDetail->formulaType));

    // performance settings
    performanceWidget=new QWidget(this);
    QVBoxLayout *perfLayout = new QVBoxLayout(performanceWidget);
    weeklyPerfCheck = new QCheckBox(tr("Weekly Best Performances"));
    submaxWeeklyPerfCheck = new QCheckBox(tr("Submaximal Weekly Best"));
    performanceTestCheck = new QCheckBox(tr("Performance Tests"));
    perfLayout->addStretch();
    perfLayout->addWidget(performanceTestCheck);
    perfLayout->addWidget(weeklyPerfCheck);
    perfLayout->addWidget(submaxWeeklyPerfCheck);
    perfLayout->addStretch();

    performanceTestCheck->setChecked(metricDetail->tests);
    weeklyPerfCheck->setChecked(metricDetail->perfs);
    submaxWeeklyPerfCheck->setChecked(metricDetail->submax);

    // get suitably formated list
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
    list << "best(np, 3600)";
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

    // set new list
    // create an empty completer, configchanged will fix it
    DataFilterCompleter *completer = new DataFilterCompleter(list, this);
    formulaEdit->setCompleter(completer);

    // stress selection
    stressTypeSelect = new QComboBox(this);
    stressTypeSelect->addItem(tr("Short Term Stress (STS/ATL)"), STRESS_STS);
    stressTypeSelect->addItem(tr("Long Term Stress  (LTS/CTL)"), STRESS_LTS);
    stressTypeSelect->addItem(tr("Stress Balance    (SB/TSB)"),  STRESS_SB);
    stressTypeSelect->addItem(tr("Stress Ramp Rate  (RR)"),      STRESS_RR);
    stressTypeSelect->setCurrentIndex(metricDetail->stressType);

    stressWidget = new QWidget(this);
    stressWidget->setContentsMargins(0,0,0,0);
    QHBoxLayout *stressLayout = new QHBoxLayout(stressWidget);
    stressLayout->setContentsMargins(0,0,0,0);
    stressLayout->setSpacing(5 *dpiXFactor);
    stressLayout->addWidget(new QLabel(tr("Stress Type"), this));
    stressLayout->addWidget(stressTypeSelect);

    // banister selection
    banisterTypeSelect = new QComboBox(this);
    banisterTypeSelect->addItem(tr("Negative Training Effect (NTE)"), BANISTER_NTE);
    banisterTypeSelect->addItem(tr("Positive Training Effect (PTE)"), BANISTER_PTE);
    banisterTypeSelect->addItem(tr("Performance (Power Index)"),  BANISTER_PERFORMANCE);
    banisterTypeSelect->addItem(tr("Predicted CP (Watts)"),  BANISTER_CP);
    banisterTypeSelect->setCurrentIndex(metricDetail->stressType < 4 ? metricDetail->stressType : 2);

    banisterWidget = new QWidget(this);
    banisterWidget->setContentsMargins(0,0,0,0);
    QHBoxLayout *banisterLayout = new QHBoxLayout(banisterWidget);
    banisterLayout->setContentsMargins(0,0,0,0);
    banisterLayout->setSpacing(5 *dpiXFactor);
    banisterLayout->addWidget(new QLabel(tr("Curve Type"), this));
    banisterLayout->addWidget(banisterTypeSelect);


    metricWidget = new QWidget(this);
    metricWidget->setContentsMargins(0,0,0,0);
    QVBoxLayout *metricLayout = new QVBoxLayout(metricWidget);

    // metric selection tree
    metricTree = new QTreeWidget;
    metricLayout->addWidget(metricTree);

    // and add the stress selector to this widget
    // too as we reuse it for stress selection
    metricLayout->addWidget(stressWidget);
    metricLayout->addWidget(banisterWidget);

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
        if (metric.metric != NULL) add->setToolTip(0, metric.metric->description());
        else if (metric.type == METRIC_META) add->setToolTip(0, tr("Metadata Field"));
        else if (metric.type == METRIC_PM) add->setToolTip(0, tr("PMC metric"));
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

    // measure selection
    measureWidget = new QWidget(this);
    QVBoxLayout *measureLayout = new QVBoxLayout(measureWidget);

    measureGroupSelect = new QComboBox(this);
    foreach(QString group, context->athlete->measures->getGroupNames()) {
        measureGroupSelect->addItem(group);
    }
    measureGroupSelect->setCurrentIndex(metricDetail->measureGroup);
    measureLayout->addStretch();
    measureLayout->addWidget(new QLabel(tr("Measure Group"), this));
    measureLayout->addWidget(measureGroupSelect);

    measureFieldSelect = new QComboBox(this);
    foreach(QString field, context->athlete->measures->getFieldNames(metricDetail->measureGroup)) {
        measureFieldSelect->addItem(field);
    }
    measureFieldSelect->setCurrentIndex(metricDetail->measureField);
    measureLayout->addWidget(new QLabel(tr("Measure Field"), this));
    measureLayout->addWidget(measureFieldSelect);
    measureLayout->addStretch();

    // contains all the different ways of defining
    // a curve, one foreach type. currently just
    // metric and bests, but will add formula and
    // measure at some point
    typeStack = new QStackedWidget(this);
    typeStack->addWidget(metricWidget);
    typeStack->addWidget(bestWidget);
    typeStack->addWidget(estimateWidget);
    typeStack->addWidget(formulaWidget);
    typeStack->addWidget(measureWidget);
    typeStack->addWidget(performanceWidget);
    typeStack->setCurrentIndex(chooseMetric->isChecked() ? 0 : (chooseBest->isChecked() ? 1 : 2));

    // Grid
    QGridLayout *grid = new QGridLayout;

    QLabel *filter = new QLabel(tr("Filter"));
    dataFilter = new SearchFilterBox(this, context);
    dataFilter->setFilter(metricDetail->datafilter);

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
    trendType->addItem(tr("Moving Average"));
    trendType->addItem(tr("Simple Average"));
    trendType->setCurrentIndex(metricDetail->trendtype);

    // add to grid
    grid->addWidget(filter, 0,0);
    grid->addWidget(dataFilter, 0,1,1,3);
    grid->addLayout(radioButtons, 1, 0, 1, 1, Qt::AlignTop|Qt::AlignLeft);
    grid->addWidget(typeStack, 1, 1, 1, 3);
    QWidget *spacer1 = new QWidget(this);
    spacer1->setFixedHeight(10);
    grid->addWidget(spacer1, 2,0);
    grid->addWidget(name, 3,0);
    grid->addWidget(userName, 3, 1, 1, 3);
    grid->addWidget(units, 4,0);
    grid->addWidget(userUnits, 4,1);
    grid->addWidget(style, 5,0);
    grid->addWidget(curveStyle, 5,1);
    grid->addWidget(symbol, 6,0);
    grid->addWidget(curveSymbol, 6,1);
    QWidget *spacer2 = new QWidget(this);
    spacer2->setFixedHeight(10);
    grid->addWidget(spacer2, 7,0);
    grid->addWidget(stackLabel, 8, 0);
    grid->addWidget(stack, 8, 1);
    grid->addWidget(color, 9,0);
    grid->addWidget(curveColor, 9,1);
    grid->addWidget(fill, 10,0);
    grid->addWidget(fillCurve, 10,1);
    grid->addWidget(topN, 4,2);
    grid->addWidget(showBest, 4,3);
    grid->addWidget(bottomN, 5,2);
    grid->addWidget(showLowest, 5,3);
    grid->addWidget(outN, 6,2);
    grid->addWidget(showOut, 6,3);
    grid->addWidget(baseline, 7, 2);
    grid->addWidget(baseLine, 7,3);
    grid->addWidget(trendType, 8,2);
    grid->addWidget(curveSmooth, 9,2);
    grid->addWidget(labels, 10,2);

    mainLayout->addLayout(grid);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    applyButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);
    mainLayout->addLayout(buttonLayout);

    // clean up the widgets
    typeChanged();
    modelChanged();
    measureGroupChanged();

    // connect up slots
    connect(metricTree, SIGNAL(itemSelectionChanged()), this, SLOT(metricSelected()));
    connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(curveColor, SIGNAL(clicked()), this, SLOT(colorClicked()));
    connect(chooseMetric, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(chooseBest, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(chooseEstimate, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(chooseStress, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(chooseFormula, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(chooseMeasure, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(choosePerformance, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(chooseBanister, SIGNAL(toggled(bool)), this, SLOT(typeChanged()));
    connect(modelSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(modelChanged()));
    connect(estimateSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(estimateChanged()));
    connect(estimateDuration, SIGNAL(valueChanged(double)), this, SLOT(estimateName()));
    connect(estimateDurationUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(estimateName()));
    connect(measureGroupSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(measureGroupChanged()));

    // when stuff changes rebuild name
    connect(chooseBest, SIGNAL(toggled(bool)), this, SLOT(bestName()));
    connect(chooseStress, SIGNAL(toggled(bool)), this, SLOT(stressName()));
    connect(chooseEstimate, SIGNAL(toggled(bool)), this, SLOT(estimateName()));
    connect(chooseMeasure, SIGNAL(toggled(bool)), this, SLOT(measureName()));
    connect(choosePerformance, SIGNAL(toggled(bool)), this, SLOT(performanceName()));
    connect(stressTypeSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(stressName()));
    connect(banisterTypeSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(banisterName()));
    connect(chooseMetric, SIGNAL(toggled(bool)), this, SLOT(metricSelected()));
    connect(duration, SIGNAL(valueChanged(double)), this, SLOT(bestName()));
    connect(durationUnits, SIGNAL(currentIndexChanged(int)), this, SLOT(bestName()));
    connect(dataSeries, SIGNAL(currentIndexChanged(int)), this, SLOT(bestName()));
    connect(measureGroupSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(measureName()));
    connect(measureFieldSelect, SIGNAL(currentIndexChanged(int)), this, SLOT(measureName()));
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
        metricWidget->show();
        estimateWidget->hide();
        stressWidget->hide();
        formulaWidget->hide();
        measureWidget->hide();
        performanceWidget->hide();
        banisterWidget->hide();
        typeStack->setCurrentIndex(0);
    }

    if (chooseBest->isChecked()) {
        bestWidget->show();
        metricWidget->hide();
        estimateWidget->hide();
        stressWidget->hide();
        formulaWidget->hide();
        measureWidget->hide();
        performanceWidget->hide();
        banisterWidget->hide();
        typeStack->setCurrentIndex(1);
    }

    if (chooseEstimate->isChecked()) {
        bestWidget->hide();
        metricWidget->hide();
        estimateWidget->show();
        stressWidget->hide();
        formulaWidget->hide();
        measureWidget->hide();
        performanceWidget->hide();
        banisterWidget->hide();
        typeStack->setCurrentIndex(2);
    }

    if (chooseStress->isChecked()) {
        bestWidget->hide();
        metricWidget->show();
        estimateWidget->hide();
        stressWidget->show();
        formulaWidget->hide();
        measureWidget->hide();
        performanceWidget->hide();
        banisterWidget->hide();
        typeStack->setCurrentIndex(0);
    }

    if (chooseFormula->isChecked()) {
        formulaWidget->show();
        bestWidget->hide();
        metricWidget->hide();
        estimateWidget->hide();
        stressWidget->hide();
        measureWidget->hide();
        performanceWidget->hide();
        banisterWidget->hide();
        typeStack->setCurrentIndex(3);
    }

    if (chooseMeasure->isChecked()) {
        measureWidget->show();
        formulaWidget->hide();
        bestWidget->hide();
        metricWidget->hide();
        estimateWidget->hide();
        stressWidget->hide();
        performanceWidget->hide();
        banisterWidget->hide();
        typeStack->setCurrentIndex(4);
    }

    if (choosePerformance->isChecked()) {
        performanceWidget->show();
        formulaWidget->hide();
        bestWidget->hide();
        metricWidget->hide();
        estimateWidget->hide();
        stressWidget->hide();
        banisterWidget->hide();
        typeStack->setCurrentIndex(5);
    }

    if (chooseBanister->isChecked()) {
        bestWidget->hide();
        metricWidget->show();
        estimateWidget->hide();
        stressWidget->hide();
        formulaWidget->hide();
        measureWidget->hide();
        performanceWidget->hide();
        banisterWidget->show();
        typeStack->setCurrentIndex(0);
    }
    adjustSize();
}

void
EditMetricDetailDialog::stressName()
{
    // used when adding the generated curve to the curves
    // map in LTMPlot, we need to be able to differentiate
    // between adding the metric to a chart and adding
    // a stress series to a chart

    // only for bests!
    if (chooseStress->isChecked() == false) return;

    // re-use bestSymbol
    metricDetail->bestSymbol = metricDetail->symbol;

    // append type
    switch(stressTypeSelect->currentIndex()) {
    case 0: metricDetail->bestSymbol += "_lts"; break;
    case 1: metricDetail->bestSymbol += "_sts"; break;
    case 2: metricDetail->bestSymbol += "_sb"; break;
    case 3: metricDetail->bestSymbol += "_rr"; break;
    }

}

void
EditMetricDetailDialog::banisterName()
{
    // used when adding the generated curve to the curves
    // map in LTMPlot, we need to be able to differentiate
    // between adding the metric to a chart and adding
    // a stress series to a chart

    // only for bests!
    if (chooseBanister->isChecked() == false) return;

    // re-use bestSymbol like PMC does
    metricDetail->bestSymbol = metricDetail->symbol;

    // append type
    switch(banisterTypeSelect->currentIndex()) {
    case 0: metricDetail->bestSymbol += "_nte"; break;
    case 1: metricDetail->bestSymbol += "_pte"; break;
    case 2: metricDetail->bestSymbol += "_perf"; break;
    case 3: metricDetail->bestSymbol += "_cp"; break;
    }

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
EditMetricDetailDialog::measureName()
{
    // only for measures!
    if (chooseMeasure->isChecked() == false) return;

    int measureGroup = measureGroupSelect->currentIndex();
    int measureField = measureFieldSelect->currentIndex();

    // when widget destroyed we get negative indexes so ignore
    if (measureGroup < 0 || measureField < 0) return;

    // set uname from current parms
    QString desc = QString(tr("%1 - %2"))
        .arg(context->athlete->measures->getGroupNames().value(measureGroup))
        .arg(context->athlete->measures->getFieldNames(measureGroup).value(measureField));
    userName->setText(desc);
    userUnits->setText(context->athlete->measures->getFieldUnits(measureGroup, measureField));
    metricDetail->symbol = desc.replace(" ", "_");
}

void
EditMetricDetailDialog::performanceName()
{
    // only for performances!
    if (choosePerformance->isChecked() == false) return;

    if (userName->text() == "")  userName->setText(tr("Performances"));
    if (userUnits->text() == "") userUnits->setText(tr("Power Index"));
}

void
EditMetricDetailDialog::metricSelected()
{
    // only in metric mode
    if (!chooseMetric->isChecked() && !chooseStress->isChecked() && !chooseBanister->isChecked()) return;

    // user selected a different metric
    // so update accordingly
    int index = metricTree->invisibleRootItem()->indexOfChild(metricTree->currentItem());

    // out of bounds !
    if (index < 0 || index >= ltmTool->metrics.count()) return;

    if (!chooseBanister->isChecked()) {

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
    }

    (*metricDetail) = ltmTool->metrics[index]; // overwrite!

    // make the banister name
    if (chooseBanister->isChecked()) banisterName();
    // make the stress name
    if (chooseStress->isChecked()) stressName();
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
    else if (chooseStress->isChecked()) metricDetail->type = 7; // stress
    else if (chooseFormula->isChecked()) metricDetail->type = 8; // stress
    else if (chooseMeasure->isChecked()) metricDetail->type = 9; // measure
    else if (choosePerformance->isChecked()) metricDetail->type = 10; // measure
    else if (chooseBanister->isChecked()) metricDetail->type = 11; // banister

    if (choosePerformance->isChecked()) {
        metricDetail->symbol=QString(tr("Performances_%1_%2_%3"))
                            .arg(performanceTestCheck->isChecked() ? 'Y' : 'N')
                            .arg(submaxWeeklyPerfCheck->isChecked() ? 'Y' : 'N')
                            .arg(weeklyPerfCheck->isChecked() ? 'Y' : 'N');
    }
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

    metricDetail->datafilter = dataFilter->filter();
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
    if (chooseStress->isChecked()) metricDetail->stressType = stressTypeSelect->currentIndex();
    if (chooseBanister->isChecked()) metricDetail->stressType = banisterTypeSelect->currentIndex();
    metricDetail->formula = formulaEdit->toPlainText();
    metricDetail->formulaType = static_cast<RideMetric::MetricType>(formulaType->itemData(formulaType->currentIndex()).toInt());
    metricDetail->measureGroup = measureGroupSelect->currentIndex();
    metricDetail->measureField = measureFieldSelect->currentIndex();
    metricDetail->tests = performanceTestCheck->isChecked();
    metricDetail->perfs = weeklyPerfCheck->isChecked();
    metricDetail->submax = submaxWeeklyPerfCheck->isChecked();
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

DataFilterEdit::DataFilterEdit(QWidget *parent, Context *context)
: QTextEdit(parent), c(0), context(context)
{
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(checkErrors()));
}

DataFilterEdit::~DataFilterEdit()
{
}

void
DataFilterEdit::checkErrors()
{
    // parse and present errors to user
    DataFilter checker(this, context);
    QStringList errors = checker.check(toPlainText());
    checker.colorSyntax(document(), textCursor().position()); // syntax + error highlighting

    // need to fixup for errors!
    // XXX next commit
}

bool
DataFilterEdit::event(QEvent *e)
{
    // intercept all events
    if (e->type() == QEvent::ToolTip) {
       // XXX error reporting when mouse over error
    }

    // call standard event handler
    return QTextEdit::event(e);
}

void DataFilterEdit::setCompleter(QCompleter *completer)
{
    if (c)
        QObject::disconnect(c, 0, this, 0);

    c = completer;

    if (!c)
        return;

    c->setWidget(this);
    c->setCompletionMode(QCompleter::PopupCompletion);
    c->setCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(c, SIGNAL(activated(QString)),
                     this, SLOT(insertCompletion(QString)));
}

QCompleter *DataFilterEdit::completer() const
{
    return c;
}

void DataFilterEdit::insertCompletion(const QString& completion)
{
    if (c->widget() != this)
        return;
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, c->completionPrefix().length());
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    tc.insertText(completion);
    setTextCursor(tc);

    checkErrors();
}

void
DataFilterEdit::setText(const QString &text)
{
    // set text..
    QTextEdit::setText(text);
    checkErrors();
}

QString DataFilterEdit::textUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.select(QTextCursor::WordUnderCursor);
    return tc.selectedText();
}

void DataFilterEdit::focusInEvent(QFocusEvent *e)
{
    if (c) c->setWidget(this);
    QTextEdit::focusInEvent(e);
}

void DataFilterEdit::keyPressEvent(QKeyEvent *e)
{
    // wait a couple of seconds before checking the changes....
    if (c && c->popup()->isVisible()) {
        // The following keys are forwarded by the completer to the widget
       switch (e->key()) {
       case Qt::Key_Enter:
       case Qt::Key_Return:
       case Qt::Key_Escape:
       case Qt::Key_Tab:
       case Qt::Key_Backtab:
            e->ignore();
            return; // let the completer do default behavior
       default:
           break;
       }
    }

    bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
    if (!c || !isShortcut) // do not process the shortcut when we have a completer
        QTextEdit::keyPressEvent(e);

    // check
    checkErrors();

    const bool ctrlOrShift = e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier);
    if (!c || (ctrlOrShift && e->text().isEmpty()))
        return;

    static QString eow("~!@#$%^&*()_+{}|:\"<>?,./;'[]\\-="); // end of word
    bool hasModifier = (e->modifiers() != Qt::NoModifier) && !ctrlOrShift;
    QString completionPrefix = textUnderCursor();

    // are we in a comment ?
    QString line = textCursor().block().text().trimmed();
    for(int i=textCursor().positionInBlock(); i>=0; i--)
        if (line[i]=='#') return;

    if (!isShortcut && (hasModifier || e->text().isEmpty()|| completionPrefix.length() < 1
                      || eow.contains(e->text().right(1)))) {
        c->popup()->hide();
        return;
    }

    if (completionPrefix != c->completionPrefix()) {
        c->setCompletionPrefix(completionPrefix);
        c->popup()->setCurrentIndex(c->completionModel()->index(0, 0));
    }
    QRect cr = cursorRect();
    cr.setWidth(c->popup()->sizeHintForColumn(0)
                + c->popup()->verticalScrollBar()->sizeHint().width());
    c->complete(cr); // popup it up!
}
