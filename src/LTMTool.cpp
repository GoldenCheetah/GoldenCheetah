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
    useMetricUnits = main->useMetricUnits;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);

    QWidget *basicsettings = new QWidget(this);
    mainLayout->addWidget(basicsettings);
    QFormLayout *basicsettingsLayout = new QFormLayout(basicsettings);
    basicsettingsLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

#ifdef GC_HAVE_LUCENE
    searchBox = new SearchFilterBox(this, main);
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
    basicLayout->setSpacing(0);

    QLabel *presetLabel = new QLabel(tr("Chart"));
    QFont sameFont = presetLabel->font();
#ifdef Q_OS_MAC // possibly needed on others too
    sameFont.setPointSize(sameFont.pointSize() + 2);
#endif
    presetPicker = new QComboBox;
    presetPicker->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    QHBoxLayout *presetrow = new QHBoxLayout;
    presetrow->setSpacing(5);
    presetrow->addWidget(presetLabel);
    presetrow->addWidget(presetPicker);
    presetrow->addStretch();
    basicLayout->addLayout(presetrow);
    basicLayout->addStretch();

    dateSetting = new DateSettingsEdit(this);
    basicsettingsLayout->addRow(new QLabel(tr("Date range")), dateSetting);
    basicsettingsLayout->addRow(new QLabel(tr(""))); // spacing

    groupBy = new QComboBox;
    groupBy->addItem(tr("Days"), LTM_DAY);
    groupBy->addItem(tr("Weeks"), LTM_WEEK);
    groupBy->addItem(tr("Months"), LTM_MONTH);
    groupBy->addItem(tr("Years"), LTM_YEAR);
    groupBy->addItem(tr("Time Of Day"), LTM_TOD);
    groupBy->setCurrentIndex(0);
    basicsettingsLayout->addRow(new QLabel(tr("Group by")), groupBy);
    basicsettingsLayout->addRow(new QLabel(tr(""))); // spacing

    shadeZones = new QCheckBox(tr("Shade Zones"));
    basicsettingsLayout->addRow(new QLabel(""), shadeZones);

    showLegend = new QCheckBox(tr("Show Legend"));
    basicsettingsLayout->addRow(new QLabel(""), showLegend);

    showEvents = new QCheckBox(tr("Show Events"));
    basicsettingsLayout->addRow(new QLabel(""), showEvents);

    // controls
    saveButton = new QPushButton(tr("Add"));
    manageButton = new QPushButton(tr("Manage"));
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addWidget(manageButton);
    buttons->addStretch();
    buttons->addWidget(saveButton);
    basicLayout->addStretch();
    basicLayout->addLayout(buttons);

    metricTree = new QTreeWidget;
#ifdef Q_OS_MAC
    metricTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    metricTree->setColumnCount(1);
    if (multi)
        metricTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    else
        metricTree->setSelectionMode(QAbstractItemView::SingleSelection);
    metricTree->header()->hide();
    //metricTree->setFrameStyle(QFrame::NoFrame);
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
        adds.units = "";
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
    skibaSTS.trend = false;
    skibaSTS.topN = 5;
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
    skibaSB.trend = false;
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
    skibaSTR.trend = false;
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
    skibaLTR.trend = false;
    skibaLTR.topN = 1;
    skibaLTR.uname = skibaLTR.name = tr("Skiba LTS Ramp");
    skibaLTR.units = "Ramp";
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
    danielsSTS.trend = false;
    danielsSTS.topN = 5;
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
    danielsSB.trend = false;
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
    danielsSTR.trend = false;
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
    danielsLTR.trend = false;
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
    workLTS.trend = false;
    workLTS.topN = 5;
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
    workSTS.trend = false;
    workSTS.topN = 5;
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
    workSB.trend = false;
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
    workSTR.trend = false;
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
    workLTR.trend = false;
    workLTR.topN = 1;
    workLTR.uname = workLTR.name = tr("Work (Kj) LTS Ramp");
    workLTR.units = "Ramp";
    workLTR.uunits = tr("Ramp");
    metrics.append(workLTR);

    // total distance
    MetricDetail distanceLTS;
    distanceLTS.type = METRIC_PM;
    distanceLTS.symbol = "distance_lts";
    distanceLTS.metric = NULL; // not a factory metric
    distanceLTS.penColor = QColor(Qt::blue);
    distanceLTS.curveStyle = QwtPlotCurve::Lines;
    distanceLTS.symbolStyle = QwtSymbol::NoSymbol;
    distanceLTS.smooth = false;
    distanceLTS.trend = false;
    distanceLTS.topN = 5;
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
    distanceSTS.trend = false;
    distanceSTS.topN = 5;
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
    distanceSB.trend = false;
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
    distanceSTR.trend = false;
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
    distanceLTR.trend = false;
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
    cogganCTL.trend = false;
    cogganCTL.topN = 5;
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
    cogganATL.trend = false;
    cogganATL.topN = 5;
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
    cogganTSB.trend = false;
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
    cogganSTR.trend = false;
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
    cogganLTR.trend = false;
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
    trimpSB.trend = false;
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
    trimpSTR.trend = false;
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
    trimpLTR.trend = false;
    trimpLTR.topN = 1;
    trimpLTR.uname = trimpLTR.name = tr("TRIMP LTS Ramp");
    trimpLTR.units = "Ramp";
    trimpLTR.uunits = tr("Ramp");
    metrics.append(trimpLTR);

    // metadata metrics
    SpecialFields sp;
    foreach (FieldDefinition field, main->rideMetadata()->getFields()) {
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
            metametric.trend = false;
            metametric.topN = 5;
            metametric.uname = metametric.name = sp.displayName(field.name);
            metametric.units = "";
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
            measure.uname = "";
            measure.name = QString("%1 (m)").arg(sp.displayName(field.name));
            measure.units = "";
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

    // read charts.xml and populate the picker
    LTMSettings reader;
    reader.readChartXML(home, presets);
    // translate default chart names
    translateDefaultCharts(presets);
    for(int i=0; i<presets.count(); i++)
        presetPicker->addItem(presets[i].name, i);
    presetPicker->setCurrentIndex(-1);

    configChanged(); // will reset the metric tree

    tabs = new QTabWidget(this);

    mainLayout->addWidget(tabs);
    basic->setContentsMargins(20,20,20,20);

    // metric tree in a container for spacing etc
    QWidget *metricContainer = new QWidget(this);
    metricContainer->setContentsMargins(20,20,20,20);
    QVBoxLayout *metricContainerLayout = new QVBoxLayout(metricContainer);
    metricContainerLayout->setContentsMargins(0,0,0,0);
    metricContainerLayout->setSpacing(0);
    metricContainerLayout->addWidget(metricTree);

    tabs->addTab(basicsettings, tr("Basic"));
    tabs->addTab(basic, tr("Preset"));
    tabs->addTab(metricContainer, tr("Custom"));

    connect(metricTree,SIGNAL(itemSelectionChanged()),
            this, SLOT(metricTreeWidgetSelectionChanged()));
    connect(main, SIGNAL(configChanged()),
            this, SLOT(configChanged()));
    connect(metricTree,SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(metricTreePopup(const QPoint &)));

    // switched between one or other
    connect(dateSetting, SIGNAL(useStandardRange()), this, SIGNAL(useStandardRange()));
    connect(dateSetting, SIGNAL(useCustomRange(DateRange)), this, SIGNAL(useCustomRange(DateRange)));
    connect(dateSetting, SIGNAL(useThruToday()), this, SIGNAL(useThruToday()));
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

    // don't use native dialog, since there is a nasty bug causing focus loss
    // see https://bugreports.qt-project.org/browse/QTBUG-14889
    QColor color = picker.getColor(metrics[index].penColor, this, tr("Choose Metric Color"), QColorDialog::DontUseNativeDialog);

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
    grid->addWidget(stackLabel, 4, 0);
    grid->addWidget(stack, 4, 1);
    grid->addWidget(color, 5,0);
    grid->addWidget(curveColor, 5,1);
    grid->addWidget(fill, 6,0);
    grid->addWidget(fillCurve, 6,1);
    grid->addWidget(topN, 7,0);
    grid->addWidget(showBest, 7,1);
    grid->addWidget(outN, 8,0);
    grid->addWidget(showOut, 8,1);
    grid->addWidget(baseline, 9, 0);
    grid->addWidget(baseLine, 9,1);
    grid->addWidget(curveSmooth, 10,1);
    grid->addWidget(curveTrend, 11,1);

    mainLayout->addLayout(grid);

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
    metricDetail->fillCurve = fillCurve->isChecked();
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

void
LTMTool::translateDefaultCharts(QList<LTMSettings>&charts)
{
    // Map default (english) chart name to external (Localized) name
    // New default charts need to be added to this list to be translated
    QMap<QString, QString> chartNameMap;
	chartNameMap.insert("PMC", tr("PMC"));
	chartNameMap.insert("Track Weight", tr("Track Weight"));
	chartNameMap.insert("Time In Power Zone (Stacked)", tr("Time In Power Zone (Stacked)"));
	chartNameMap.insert("Time In Power Zone (Bar)", tr("Time In Power Zone (Bar)"));
	chartNameMap.insert("Time In HR Zone", tr("Time In HR Zone"));
	chartNameMap.insert("Power Distribution", tr("Power Distribution"));
	chartNameMap.insert("KPI Tracker", tr("KPI Tracker"));
	chartNameMap.insert("Critical Power Trend", tr("Critical Power Trend"));
	chartNameMap.insert("Aerobic Power", tr("Aerobic Power"));
	chartNameMap.insert("Aerobic WPK", tr("Aerobic WPK"));
	chartNameMap.insert("Power Variance", tr("Power Variance"));
	chartNameMap.insert("Power Profile", tr("Power Profile"));
	chartNameMap.insert("Anaerobic Power", tr("Anaerobic Power"));
	chartNameMap.insert("Anaerobic WPK", tr("Anaerobic WPK"));
	chartNameMap.insert("Power & Speed Trend", tr("Power & Speed Trend"));
	chartNameMap.insert("Cardiovascular Response", tr("Cardiovascular Response"));
	chartNameMap.insert("Tempo & Threshold Time", tr("Tempo & Threshold Time"));
	chartNameMap.insert("Training Mix", tr("Training Mix"));
	chartNameMap.insert("Time & Distance", tr("Time & Distance"));
	chartNameMap.insert("Skiba Power", tr("Skiba Power"));
	chartNameMap.insert("Daniels Power", tr("Daniels Power"));
	chartNameMap.insert("PM Ramp & Peak", tr("PM Ramp & Peak"));
	chartNameMap.insert("Skiba PM", tr("Skiba PM"));
	chartNameMap.insert("Daniels PM", tr("Daniels PM"));
	chartNameMap.insert("Device Reliability", tr("Device Reliability"));
	chartNameMap.insert("Withings Weight", tr("Withings Weight"));
	chartNameMap.insert("Stress and Distance", tr("Stress and Distance"));
	chartNameMap.insert("Calories vs Duration", tr("Calories vs Duration"));

    for(int i=0; i<charts.count(); i++) {
        // Replace chart name for localized version, default to english name
        charts[i].name = chartNameMap.value(charts[i].name, charts[i].name);
    }
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
LTMTool::translateMetrics(MainWindow *main, const QDir &home, LTMSettings *settings)
{
    static QMap<QString, QString> unitsMap;
    // LTMTool instance is created to have access to metrics catalog
    LTMTool* ltmTool = new LTMTool(main, home, false);
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
