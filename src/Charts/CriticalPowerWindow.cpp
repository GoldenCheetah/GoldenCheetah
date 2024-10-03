/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
 * Copyright (c) 2009 Dan Connelly (@djconnel)
 * Copyright (c) 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "CriticalPowerWindow.h"
#include "Settings.h"
#include "SearchFilterBox.h"
#include "CPPlot.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "TimeUtils.h"
#include "IntervalItem.h"
#include "GcOverlayWidget.h"
#include "MUWidget.h"
#include "HelpWhatsThis.h"
#include "AbstractView.h" // stylesheet
#include "RideCache.h"
#include <qwt_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>
#include <qwt_scale_div.h>
#include <QFile>
#include "Season.h"
#include "Seasons.h"
#include "Colors.h"
#include "Zones.h"
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QFileDialog>

CriticalPowerWindow::CriticalPowerWindow(Context *context, bool rangemode) :
    GcChartWindow(context), _dateRange("{00000000-0000-0000-0000-000000000001}"), context(context), currentRide(NULL), rangemode(rangemode), isfiltered(false), stale(true), useCustom(false), useToToday(false), active(false), hoverCurve(NULL), firstShow(true)
{
    //
    // reveal controls widget
    //

    // layout reveal controls
    QHBoxLayout *revealLayout = new QHBoxLayout;
    revealLayout->setContentsMargins(0,0,0,0);

    QLabel *rSeriesLabel = new QLabel(tr("Data Series"), this);
    rSeriesSelector = new QxtStringSpinBox();
    rPercent = new QCheckBox(this);
    rPercent->setText(tr("Percentage of Best"));
    rHeat = new QCheckBox(this);
    rHeat->setText(tr("Show Heat"));
    rDelta = new QCheckBox(this);
    rDelta->setText(tr("Delta compare"));
    rDelta->hide();
    rDeltaPercent = new QCheckBox(this);
    rDeltaPercent->setText(tr("as percentage"));
    rDeltaPercent->hide();

    QVBoxLayout *checks = new QVBoxLayout;
    checks->addStretch();
    checks->addWidget(rPercent);
    checks->addWidget(rHeat);
    checks->addWidget(rDelta);
    checks->addWidget(rDeltaPercent);
    checks->addStretch();

    revealLayout->addStretch();
    revealLayout->addWidget(rSeriesLabel);
    revealLayout->addWidget(rSeriesSelector);
    revealLayout->addSpacing(20);
    revealLayout->addLayout(checks);
    revealLayout->addStretch();

    setRevealLayout(revealLayout);

    //
    // main plot area
    //
    QVBoxLayout *mainLayout = new QVBoxLayout();
    setChartLayout(mainLayout);

    cpPlot = new CPPlot(this, context, rangemode);
    mainLayout->addWidget(cpPlot);
    HelpWhatsThis *help = new HelpWhatsThis(cpPlot);
    if (rangemode) cpPlot->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartTrends_Critical_MM));
    else cpPlot->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Critical_MM));

    // if we're plotting a veloclinic plot we can adjust CP to see what happens
    CPLabel = new QLabel(tr("Critical Power "), this);
    CPEdit = new QLineEdit(this);
    CPEdit->setFixedWidth(50);
    CPSlider = new QSlider(Qt::Horizontal);
    CPSlider->setTickInterval(50);
    CPSlider->setMinimum(100);
    CPSlider->setMaximum(500);
    CPSlider->setFocusPolicy(Qt::NoFocus);
    QHBoxLayout *cpediting = new QHBoxLayout();

    cpediting->addStretch();
    cpediting->addWidget(CPLabel);
    cpediting->addWidget(CPEdit);
    cpediting->addWidget(CPSlider);
    cpediting->addStretch();

    CPEdit->hide();
    CPSlider->hide();
    CPLabel->hide();

    mainLayout->addLayout(cpediting);
    connect(CPEdit, SIGNAL(textChanged(QString)), this, SLOT(setSliderFromEdit()));
    connect(CPSlider, SIGNAL(valueChanged(int)), this, SLOT(setEditFromSlider()));

    //
    // Chart settings
    //

    // controls widget and layout
    QTabWidget *settingsTabs = new QTabWidget(this);
    HelpWhatsThis *helpTabs = new HelpWhatsThis(settingsTabs);
    if (rangemode) settingsTabs->setWhatsThis(helpTabs->getWhatsThisText(HelpWhatsThis::ChartTrends_Critical_MM));
    else settingsTabs->setWhatsThis(helpTabs->getWhatsThisText(HelpWhatsThis::ChartRides_Critical_MM));


    QWidget *settingsWidget = new QWidget(this);
    settingsWidget->setContentsMargins(0,0,0,0);
    settingsTabs->addTab(settingsWidget, tr("Basic"));
    HelpWhatsThis *helpSettings = new HelpWhatsThis(settingsWidget);
    if (rangemode) settingsWidget->setWhatsThis(helpSettings->getWhatsThisText(HelpWhatsThis::ChartTrends_Critical_MM_Config_Settings));
    else settingsWidget->setWhatsThis(helpSettings->getWhatsThisText(HelpWhatsThis::ChartRides_Critical_MM_Config_Settings));

    QFormLayout *cl = new QFormLayout(settingsWidget);;
    cl->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    QWidget *modelWidget = new QWidget(this);
    modelWidget->setContentsMargins(0,0,0,0);
    settingsTabs->addTab(modelWidget, tr("Model"));
    HelpWhatsThis *helpModel = new HelpWhatsThis(modelWidget);
    if (rangemode) modelWidget->setWhatsThis(helpModel->getWhatsThisText(HelpWhatsThis::ChartTrends_Critical_MM_Config_Model));
    else modelWidget->setWhatsThis(helpModel->getWhatsThisText(HelpWhatsThis::ChartRides_Critical_MM_Config_Model));

    QFormLayout *mcl = new QFormLayout(modelWidget);;
    mcl->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    // add additional menu items before setting
    // controls since the menu is SET from setControls
    QAction *showsettings = new QAction(tr("Chart Settings..."));
    addAction(showsettings);
    QAction *exportData = new QAction(tr("Export Chart Data..."), this);
    addAction(exportData);

    setControls(settingsTabs);

    // filter / searchbox
    searchBox = new SearchFilterBox(this, context);
    connect(searchBox, SIGNAL(searchClear()), cpPlot, SLOT(clearFilter()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), cpPlot, SLOT(setFilter(QStringList)));
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(filterChanged()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(filterChanged()));
    cl->addRow(new QLabel(tr("Filter")), searchBox);
    cl->addWidget(new QLabel("")); //spacing
    HelpWhatsThis *searchHelp = new HelpWhatsThis(searchBox);
    searchBox->setWhatsThis(searchHelp->getWhatsThisText(HelpWhatsThis::SearchFilterBox));

    // series
    seriesCombo = new QComboBox(this);
    addSeries();

    // data -- season / daterange edit
    cComboSeason = new QComboBox(this);
    seasons = context->athlete->seasons;
    resetSeasons();
    QLabel *label = new QLabel(tr("Date range"));
    QLabel *label2 = new QLabel(tr("Date range"));
    if (rangemode) {
        cComboSeason->hide();
        label2->hide();
    }
    cl->addRow(label2, cComboSeason);
    dateSetting = new DateSettingsEdit(this);
    HelpWhatsThis *dateSettingHelp = new HelpWhatsThis(dateSetting);
    dateSetting->setWhatsThis(dateSettingHelp->getWhatsThisText(HelpWhatsThis::ChartTrends_DateRange));
    cl->addRow(label, dateSetting);
    if (rangemode == false) {
        dateSetting->hide();
        label->hide();
    }

    cl->addWidget(new QLabel("")); //spacing
    cl->addRow(new QLabel(tr("Data series")), seriesCombo);

    // shading
    shadeCheck = new QCheckBox(this);
    QLabel *shading = new QLabel(tr("Zone Shading"));
    shadeCheck->setChecked(true);
    cl->addRow(shading, shadeCheck);

    showPPCheck = new QCheckBox(this);
    showPPCheck->setChecked(false); // default off
    QLabel *pp = new QLabel(tr("Show Power Profile"));
    cl->addRow(pp, showPPCheck);

    showGridCheck = new QCheckBox(this);
    showGridCheck->setChecked(true); // default on
    QLabel *gridify = new QLabel(tr("Show grid"));
    cl->addRow(gridify, showGridCheck);

    showTestCheck = new QCheckBox(this);
    showTestCheck->setChecked(true); // default on
    QLabel *testify = new QLabel(tr("Show Performance Tests"));
    cl->addRow(testify, showTestCheck);

    showBestCheck = new QCheckBox(this);
    showBestCheck->setChecked(true); // default on
    QLabel *bestify = new QLabel(tr("Show Bests"));
    cl->addRow(bestify, showBestCheck);

    filterBestCheck = new QCheckBox(this);
    filterBestCheck->setChecked(false); // default off
    QLabel *filterify = new QLabel(tr("Filter Unique Bests"));
    cl->addRow(filterify, filterBestCheck);

    showEffortCheck = new QCheckBox(this);
    showEffortCheck->setChecked(false); // default off
    QLabel *heaties = new QLabel(tr("Show Sustained Efforts"));
    cl->addRow(heaties, showEffortCheck);

    showPowerIndexCheck = new QCheckBox(this);
    showPowerIndexCheck->setChecked(false); // default off
    QLabel *indexify = new QLabel(tr("Show as Power Index"));
    cl->addRow(indexify, showPowerIndexCheck);

    showPercentCheck = new QCheckBox(this);
    showPercentCheck->setChecked(false); // default off
    QLabel *percentify = new QLabel(tr("Show as percentage"));
    cl->addRow(percentify, showPercentCheck);

    showHeatCheck = new QCheckBox(this);
    showHeatCheck->setChecked(false); // default off
    QLabel *efforts = new QLabel(tr("Show curve heat"));
    cl->addRow(efforts, showHeatCheck);

    showHeatByDateCheck = new QCheckBox(this);
    showHeatByDateCheck->setChecked(false); // default off
    QLabel *heatiesByDate = new QLabel(tr("Show curve heat by date"));
    cl->addRow(heatiesByDate, showHeatByDateCheck);

    shadeIntervalsCheck = new QCheckBox(this);
    shadeIntervalsCheck->setChecked(true); // default on
    QLabel *shadies = new QLabel(tr("Shade Intervals"));
    cl->addRow(shadies, shadeIntervalsCheck);

    showCSLinearCheck = new QCheckBox(this);
    showCSLinearCheck->setChecked(true); // default on
    showCSLinearLabel = new QLabel(tr("Show time scale linear"));
    cl->addRow(showCSLinearLabel, showCSLinearCheck);

    showDeltaCheck = new QCheckBox(this);
    showDeltaCheck->setText(tr("Delta compare"));
    showDeltaPercentCheck = new QCheckBox(this);
    showDeltaPercentCheck->setText(tr("as percentage"));
    cl->addRow(showDeltaCheck, showDeltaPercentCheck);

    ridePlotStyleCombo = new QComboBox(this);
    ridePlotStyleCombo->addItem(tr("Activity Mean Max"));
    ridePlotStyleCombo->addItem(tr("Activity Centile"));
    ridePlotStyleCombo->addItem(tr("No Activity"));

    cl->addWidget(new QLabel("")); //spacing
    cl->addRow(new QLabel(tr("Current Activity")), ridePlotStyleCombo);

    // model config
    // 2 or 3 point model ?
    modelCombo= new QComboBox(this);
    modelCombo->addItem(tr("None"));
    modelCombo->addItem(tr("2 parameter"));
    modelCombo->addItem(tr("3 parameter"));
    modelCombo->addItem(tr("Extended CP"));
#if 0 // disable until model fitting errors are fixed (!!!)
    modelCombo->addItem(tr("Multicomponent"));
    modelCombo->addItem(tr("Ward-Smith"));
#endif
    modelCombo->setCurrentIndex(1);

    mcl->addRow(new QLabel(tr("Model")), modelCombo);

    fitCombo= new QComboBox(this);
    fitCombo->addItem(tr("Envelope"));
    fitCombo->addItem(tr("Least Squares (LMA)"));
    fitCombo->addItem(tr("Linear Regression (Work)"));
    fitCombo->setCurrentIndex(0); // default to envelope, backwards compatibility
    mcl->addRow(new QLabel(tr("Curve Fit")), fitCombo);

    fitdataCombo= new QComboBox(this);
    fitdataCombo->addItem(tr("MMP bests"));
    fitdataCombo->addItem(tr("Performance tests"));
    fitdataCombo->setCurrentIndex(0); // default to MMP, backwards compatibility
    mcl->addRow(new QLabel(tr("Data to fit")), fitdataCombo);

    mcl->addRow(new QLabel(tr(" ")));

    modelDecayLabel = new QLabel(tr("CP and W' Decay"));
    modelDecayCheck = new QCheckBox(this);
    mcl->addRow(modelDecayLabel, modelDecayCheck);
    mcl->addRow(new QLabel(tr(" ")));

    intervalLabel = new QLabel(tr("Search Interval"));
    secondsLabel = new QLabel(tr("(seconds)"));
    mcl->addRow(intervalLabel, secondsLabel);

    anLabel = new QLabel(tr("Anaerobic"));

    anI1SpinBox = new QDoubleSpinBox(this);
    anI1SpinBox->setDecimals(0);
    anI1SpinBox->setMinimum(0);
    anI1SpinBox->setMaximum(3600);
    anI1SpinBox->setSingleStep(1.0);
    anI1SpinBox->setAlignment(Qt::AlignRight);
    anI1SpinBox->setValue(180); // 3 minutes

    anI2SpinBox = new QDoubleSpinBox(this);
    anI2SpinBox->setDecimals(0);
    anI2SpinBox->setMinimum(0);
    anI2SpinBox->setMaximum(3600);
    anI2SpinBox->setSingleStep(1.0);
    anI2SpinBox->setAlignment(Qt::AlignRight);
    anI2SpinBox->setValue(360); // 6 minutes

    QHBoxLayout *anLayout = new QHBoxLayout;
    anLayout->addWidget(anI1SpinBox);
    anLayout->addWidget(anI2SpinBox);
    mcl->addRow(anLabel, anLayout);

    aeLabel = new QLabel(tr("Aerobic"));

    aeI1SpinBox = new QDoubleSpinBox(this);
    aeI1SpinBox->setDecimals(0);
    aeI1SpinBox->setMinimum(0.0);
    aeI1SpinBox->setMaximum(3600);
    aeI1SpinBox->setSingleStep(1.0);
    aeI1SpinBox->setAlignment(Qt::AlignRight);
    aeI1SpinBox->setValue(420); // 7 minutes

    aeI2SpinBox = new QDoubleSpinBox(this);
    aeI2SpinBox->setDecimals(0);
    aeI2SpinBox->setMinimum(0.0);
    aeI2SpinBox->setMaximum(3600);
    aeI2SpinBox->setSingleStep(1.0);
    aeI2SpinBox->setAlignment(Qt::AlignRight);
    aeI2SpinBox->setValue(1800); // 30 minutes

    QHBoxLayout *aeLayout = new QHBoxLayout;
    aeLayout->addWidget(aeI1SpinBox);
    aeLayout->addWidget(aeI2SpinBox);
    mcl->addRow(aeLabel, aeLayout);

    sanI1SpinBox = new QDoubleSpinBox(this);
    sanI1SpinBox->setDecimals(0);
    sanI1SpinBox->setMinimum(15);
    sanI1SpinBox->setMaximum(45);
    sanI1SpinBox->setSingleStep(1.0);
    sanI1SpinBox->setAlignment(Qt::AlignRight);
    sanI1SpinBox->setValue(30); // 30 secs

    sanI2SpinBox = new QDoubleSpinBox(this);
    sanI2SpinBox->setDecimals(0);
    sanI2SpinBox->setMinimum(15);
    sanI2SpinBox->setMaximum(300);
    sanI2SpinBox->setSingleStep(1.0);
    sanI2SpinBox->setAlignment(Qt::AlignRight);
    sanI2SpinBox->setValue(60); // 100 secs

    sanLabel = new QLabel(tr("Short anaerobic"));

    QHBoxLayout *sanLayout = new QHBoxLayout();
    sanLayout->addWidget(sanI1SpinBox);
    sanLayout->addWidget(sanI2SpinBox);
    mcl->addRow(sanLabel, sanLayout);

    laeI1SpinBox = new QDoubleSpinBox(this);
    laeI1SpinBox->setDecimals(0);
    laeI1SpinBox->setMinimum(3000);
    laeI1SpinBox->setMaximum(9000);
    laeI1SpinBox->setSingleStep(1.0);
    laeI1SpinBox->setAlignment(Qt::AlignRight);
    laeI1SpinBox->setValue(3000);

    laeI2SpinBox = new QDoubleSpinBox();
    laeI2SpinBox->setDecimals(0);
    laeI2SpinBox->setMinimum(4000);
    laeI2SpinBox->setMaximum(30000);
    laeI2SpinBox->setSingleStep(1.0);
    laeI2SpinBox->setAlignment(Qt::AlignRight);
    laeI2SpinBox->setValue(30000);

    laeLabel = new QLabel(tr("Long aerobic"));

    QHBoxLayout *laeLayout = new QHBoxLayout();
    laeLayout->addWidget(laeI1SpinBox);
    laeLayout->addWidget(laeI2SpinBox);
    mcl->addRow(laeLabel, laeLayout);

    mcl->addRow(new QLabel(""), new QLabel(""));
    velo1 = new QRadioButton(tr("Exponential"));
    velo1->setChecked(true);
    mcl->addRow(vlabel = new QLabel(tr("Variant")), velo1);
    velo2 = new QRadioButton(tr("Linear feedback"));
    mcl->addRow(new QLabel(""), velo2);
    velo3 = new QRadioButton(tr("Regeneration"));
    mcl->addRow(new QLabel(""), velo3);

    // point 2 + 3 -or- point 1 + 2 in a 2 point model

    grid = new QwtPlotGrid();
    grid->enableX(false); // not needed
    grid->enableY(true);
    grid->setZ(-20);
    QList<double> ytick[QwtScaleDiv::NTickTypes];
    for (double i=0.0; i<=2500; i+= 100) ytick[QwtScaleDiv::MajorTick]<<i;
    cpPlot->setAxisScaleDiv(QwtAxis::YLeft,QwtScaleDiv(0.0,2500.0,ytick));
    grid->attach(cpPlot);

    // the model helper -- showing model parameters etc
    helper = new QWidget(this);
    helper->setAutoFillBackground(true);

    QGridLayout *gridLayout = new QGridLayout(helper);
    gridLayout->setColumnStretch(0, 40);
    gridLayout->setColumnStretch(1, 30);
    gridLayout->setColumnStretch(2, 20);

    // create the labels
    titleBlank = new QLabel(tr(""), this);
    titleValue = new QLabel(tr("Value"), this);
    titleRank = new QLabel(tr("Rank"), this);
    wprimeTitle = new QLabel(tr("W'"), this);
    wprimeValue = new QLabel(tr("0 kJ"), this);
    wprimeRank = new QLabel(tr("n/a"), this);
    cpTitle = new QLabel(tr("CP"), this);
    cpValue = new QLabel(tr("0 w"), this);
    cpRank = new QLabel(tr("n/a"), this);
    pmaxTitle = new QLabel(tr("Pmax"), this);
    pmaxValue = new QLabel(tr("0 w"), this);
    pmaxRank = new QLabel(tr("n/a"), this);
    eiTitle = new QLabel(tr("Endurance Index"), this);
    eiValue = new QLabel(tr("n/a"), this);
    summary = new QLabel(tr(""), this);

    // autofill
    titleBlank->setAutoFillBackground(true);
    titleValue->setAutoFillBackground(true);
    titleRank->setAutoFillBackground(true);
    wprimeTitle->setAutoFillBackground(true);
    wprimeValue->setAutoFillBackground(true);
    wprimeRank->setAutoFillBackground(true);
    cpTitle->setAutoFillBackground(true);
    cpValue->setAutoFillBackground(true);
    cpRank->setAutoFillBackground(true);
    pmaxTitle->setAutoFillBackground(true);
    pmaxValue->setAutoFillBackground(true);
    pmaxRank->setAutoFillBackground(true);
    eiTitle->setAutoFillBackground(true);
    eiValue->setAutoFillBackground(true);
    summary->setAutoFillBackground(true);

    // align all centered
    titleBlank->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    titleValue->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    titleRank->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    wprimeTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    wprimeValue->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    wprimeRank->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    cpTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    cpValue->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    cpRank->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    pmaxTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    pmaxValue->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    pmaxRank->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    eiTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    eiValue->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    // add to grid
    gridLayout->addWidget(titleBlank, 0, 0);
    gridLayout->addWidget(titleValue, 0, 1);
    gridLayout->addWidget(titleRank, 0, 2);
    gridLayout->addWidget(cpTitle, 1, 0);
    gridLayout->addWidget(cpValue, 1, 1);
    gridLayout->addWidget(cpRank, 1, 2);
    gridLayout->addWidget(wprimeTitle, 2, 0);
    gridLayout->addWidget(wprimeValue, 2, 1);
    gridLayout->addWidget(wprimeRank, 2, 2);
    gridLayout->addWidget(pmaxTitle, 3, 0);
    gridLayout->addWidget(pmaxValue, 3, 1);
    gridLayout->addWidget(pmaxRank, 3, 2);
    gridLayout->addWidget(eiTitle, 4, 0);
    gridLayout->addWidget(eiValue, 4, 1);
    gridLayout->addWidget(summary, 5, 0, 1, 3);

#ifdef GC_HAVE_MUMODEL
    addHelper(QString(tr("Motor Unit Model")), new MUWidget(this, context));
#endif
    addHelper(QString(tr("Model")), helper);
    GcChartWindow::overlayWidget->move(100,100);

    if (rangemode) {
        connect(this, SIGNAL(dateRangeChanged(DateRange)), SLOT(dateRangeChanged(DateRange)));

        // Compare
        connect(context, SIGNAL(compareDateRangesStateChanged(bool)), SLOT(forceReplot()));
        connect(context, SIGNAL(compareDateRangesChanged()), SLOT(forceReplot()));

        connect(this, SIGNAL(perspectiveFilterChanged(QString)), this, SLOT(perspectiveFilterChanged()));
        connect(this, SIGNAL(perspectiveChanged(Perspective*)), this, SLOT(perspectiveFilterChanged()));

    } else {
        // when working on a ride we can select intervals!
        connect(cComboSeason, SIGNAL(currentIndexChanged(int)), this, SLOT(seasonSelected(int)));
        connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
        connect(context, SIGNAL(intervalsChanged()), this, SLOT(intervalsChanged()));
        connect(context, SIGNAL(intervalHover(IntervalItem*)), this, SLOT(intervalHover(IntervalItem*)));

        // Compare
        connect(context, SIGNAL(compareIntervalsStateChanged(bool)), SLOT(forceReplot()));
        connect(context, SIGNAL(compareIntervalsChanged()), SLOT(forceReplot()));

    }

    connect(seriesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setSeries(int)));
    connect(ridePlotStyleCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setPlotType(int)));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(configChanged(qint32)), cpPlot, SLOT(configChanged(qint32)));
    connect(exportData, SIGNAL(triggered()), this, SLOT(exportData()));
    connect(showsettings, SIGNAL(triggered()), this, SIGNAL(showControls()));

    // model updated?
    connect(modelCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(modelChanged()));
    connect(fitCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(fitChanged()));
    connect(fitdataCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(fitChanged()));
    connect(seriesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(modelParametersChanged()));
    connect(anI1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(anI2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(aeI1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(aeI2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(sanI1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(sanI2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(laeI1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(laeI2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(modelDecayCheck, SIGNAL(toggled(bool)), this, SLOT(modelParametersChanged()));
    connect(velo1, SIGNAL(toggled(bool)), this, SLOT(modelParametersChanged()));
    connect(velo2, SIGNAL(toggled(bool)), this, SLOT(modelParametersChanged()));
    connect(velo3, SIGNAL(toggled(bool)), this, SLOT(modelParametersChanged()));

    // redraw on config change -- this seems the simplest approach
    connect(context, SIGNAL(filterChanged()), this, SLOT(forceReplot()));
    connect(context, SIGNAL(homeFilterChanged()), this, SLOT(forceReplot()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(rideSaved(RideItem*)), this, SLOT(refreshRideSaved(RideItem*)));
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(newRideAdded(RideItem*)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(newRideAdded(RideItem*)));
    connect(seasons, SIGNAL(seasonsChanged()), this, SLOT(resetSeasons()));
    connect(shadeCheck, SIGNAL(stateChanged(int)), this, SLOT(shadingSelected(int)));
    connect(shadeIntervalsCheck, SIGNAL(stateChanged(int)), this, SLOT(shadeIntervalsChanged(int)));
    connect(showEffortCheck, SIGNAL(stateChanged(int)), this, SLOT(showEffortChanged(int)));
    connect(showPPCheck, SIGNAL(stateChanged(int)), this, SLOT(showPPChanged(int)));
    connect(showHeatCheck, SIGNAL(stateChanged(int)), this, SLOT(showHeatChanged(int)));
    connect(showCSLinearCheck, SIGNAL(stateChanged(int)), this, SLOT(showCSLinearChanged(int)));
    connect(rSeriesSelector, SIGNAL(valueChanged(int)), this, SLOT(rSeriesSelectorChanged(int)));
    connect(rHeat, SIGNAL(stateChanged(int)), this, SLOT(rHeatChanged(int)));
    connect(rDelta, SIGNAL(stateChanged(int)), this, SLOT(rDeltaChanged()));
    connect(rDeltaPercent, SIGNAL(stateChanged(int)), this, SLOT(rDeltaChanged()));
    connect(showHeatByDateCheck, SIGNAL(stateChanged(int)), this, SLOT(showHeatByDateChanged(int)));
    connect(showPercentCheck, SIGNAL(stateChanged(int)), this, SLOT(showPercentChanged(int)));
    connect(showDeltaCheck, SIGNAL(stateChanged(int)), this, SLOT(showDeltaChanged()));
    connect(showDeltaPercentCheck, SIGNAL(stateChanged(int)), this, SLOT(showDeltaChanged()));
    connect(showPowerIndexCheck, SIGNAL(stateChanged(int)), this, SLOT(showPowerIndexChanged(int)));
    connect(showTestCheck, SIGNAL(stateChanged(int)), this, SLOT(showTestChanged(int)));
    connect(showBestCheck, SIGNAL(stateChanged(int)), this, SLOT(showBestChanged(int)));
    connect(filterBestCheck, SIGNAL(stateChanged(int)), this, SLOT(filterBestChanged(int)));
    connect(showGridCheck, SIGNAL(stateChanged(int)), this, SLOT(showGridChanged(int)));
    connect(rPercent, SIGNAL(stateChanged(int)), this, SLOT(rPercentChanged(int)));
    connect(dateSetting, SIGNAL(useCustomRange(DateRange)), this, SLOT(useCustomRange(DateRange)));
    connect(dateSetting, SIGNAL(useThruToday()), this, SLOT(useThruToday()));
    connect(dateSetting, SIGNAL(useStandardRange()), this, SLOT(useStandardRange()));

    // if refresh is in progress give CPPlot a chance to redraw when CPX updating
    connect(context, SIGNAL(refreshUpdate(QDate)), cpPlot, SLOT(refreshUpdate(QDate)));
    connect(context, SIGNAL(refreshEnd()), cpPlot, SLOT(refreshEnd()));

    // set date range for bests and model
    if (!rangemode) seasonSelected(cComboSeason->currentIndex());

    // set widgets and model parameters
    modelChanged();

    configChanged(CONFIG_APPEARANCE); // get colors set
}

// veloclinic stuff
void
CriticalPowerWindow::setSliderFromEdit()
{
    int value = CPEdit->text().toInt();
    CPSlider->setValue(value);
}

void
CriticalPowerWindow::setEditFromSlider()
{
    CPEdit->setText(QString("%1").arg(CPSlider->value()));

    // replot with this value if we're a velo plot
    if (series() == veloclinicplot) {

        // replot the charts using this new value
        cpPlot->setVeloCP(CPSlider->value());

        // force replot...
        if (rangemode) {

            // Refresh aggregated curve
            stale = true;
            dateRangeChanged(myDateRange);

        } else {
            Season season = seasons->seasons.at(cComboSeason->currentIndex());

            // Refresh aggregated curve (ride added/filter changed)
            cpPlot->setDateRange(season.getStart(), season.getEnd());

            // if visible make the changes visible
            if (amVisible() && myRideItem) cpPlot->setRide(myRideItem);
        }
    }
}

void
CriticalPowerWindow::configChanged(qint32)
{
    if (rangemode) setProperty("color", GColor(CTRENDPLOTBACKGROUND));
    else setProperty("color", GColor(CPLOTBACKGROUND));

    // tinted palette for headings etc
    QPalette palette;
    if (rangemode) palette.setBrush(QPalette::Window, QBrush(GColor(CTRENDPLOTBACKGROUND)));
    else palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);

    // inverted palette for data etc
    QPalette whitepalette;
    if (rangemode) {
        whitepalette.setBrush(QPalette::Window, QBrush(GColor(CTRENDPLOTBACKGROUND)));
        whitepalette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CTRENDPLOTBACKGROUND)));
        whitepalette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
        whitepalette.setColor(QPalette::Text, GCColor::invertColor(GColor(CTRENDPLOTBACKGROUND)));
    } else {
        whitepalette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
        whitepalette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
        whitepalette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
        whitepalette.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    }

    QFont font;
    font.setPointSize(12); // reasonably big
    titleBlank->setFont(font);
    titleValue->setFont(font);
    titleRank->setFont(font);
    wprimeTitle->setFont(font);
    wprimeValue->setFont(font);
    wprimeRank->setFont(font);
    cpTitle->setFont(font);
    cpValue->setFont(font);
    cpRank->setFont(font);
    pmaxTitle->setFont(font);
    pmaxValue->setFont(font);
    pmaxRank->setFont(font);
    eiTitle->setFont(font);
    eiValue->setFont(font);
    summary->setFont(font);

    helper->setPalette(palette);
    titleBlank->setPalette(palette);
    titleValue->setPalette(palette);
    titleRank->setPalette(palette);
    wprimeTitle->setPalette(palette);
    wprimeValue->setPalette(whitepalette);
    wprimeRank->setPalette(whitepalette);
    cpTitle->setPalette(palette);
    cpValue->setPalette(whitepalette);
    cpRank->setPalette(whitepalette);
    pmaxTitle->setPalette(palette);
    pmaxValue->setPalette(whitepalette);
    pmaxRank->setPalette(whitepalette);
    eiTitle->setPalette(palette);
    eiValue->setPalette(whitepalette);
    summary->setPalette(whitepalette);
    CPEdit->setPalette(whitepalette);
    CPLabel->setPalette(whitepalette);
    CPSlider->setPalette(whitepalette);


#ifndef Q_OS_MAC
    QString style = QString("QSpinBox { background: %1; }").arg(GCColor::alternateColor(GColor(CPLOTBACKGROUND)).name());
    CPEdit->setStyleSheet(style);
    //CPLabel->setStyleSheet(style);
    //CPSlider->setStyleSheet(style);
    if (dpiXFactor > 1) {
        helper->setStyleSheet(QString("background: %1; color: %2;").arg(GColor(CPLOTBACKGROUND).name())
                                                                      .arg(GColor(CPLOTMARKER).name()));
    }

    // do after cascade above
    summary->setStyleSheet(QString("background-color: %1; color: %2;").arg(GColor(CPLOTBACKGROUND).name()).arg(QColor(Qt::gray).name()));
#endif


    QPen gridPen(GColor(CPLOTGRID));
    grid->setPen(gridPen);

    // set ride
    rideSelected();
}

void
CriticalPowerWindow::fitChanged()
{
    if (active == true) return;

    // gets a replot
    modelParametersChanged();
}

void
CriticalPowerWindow::showAnaerobicIntervals(bool show)
{
    if (show) {
        anLabel->show();
        anI1SpinBox->show();
        anI2SpinBox->show();
    } else {
        anLabel->hide();
        anI1SpinBox->hide();
        anI2SpinBox->hide();
    }
}

void
CriticalPowerWindow::showAerobicIntervals(bool show)
{
    if (show) {
        aeLabel->show();
        aeI1SpinBox->show();
        aeI2SpinBox->show();
    } else {
        aeLabel->hide();
        aeI1SpinBox->hide();
        aeI2SpinBox->hide();
    }
}
void
CriticalPowerWindow::showShortAnaerobicIntervals(bool show)
{
    if (show) {
        sanLabel->show();
        sanI1SpinBox->show();
        sanI2SpinBox->show();
    } else {
        sanLabel->hide();
        sanI1SpinBox->hide();
        sanI2SpinBox->hide();
    }
}

void
CriticalPowerWindow::showLongAerobicIntervals(bool show)
{
    if (show) {
        laeLabel->show();
        laeI1SpinBox->show();
        laeI2SpinBox->show();
    } else {
        laeLabel->hide();
        laeI1SpinBox->hide();
        laeI2SpinBox->hide();
    }
}

/** 
 * Shows or hides the interval spinboxes relevant for the current selection.
 *
 * The decision depends on both, the model and the fitting method.
 */
void
CriticalPowerWindow::showRelevantIntervals()
{
    // interval selection has no effect for fits other than envelope. In this
    // case hide them all.
    if (fitCombo->currentIndex() != 0) {
        intervalLabel->hide();
        secondsLabel->hide();
        showShortAnaerobicIntervals(false);
        showAnaerobicIntervals(false);
        showAerobicIntervals(false);
        showLongAerobicIntervals(false);
        return;
    }
    // if we have come here, we have envelope fits and the required
    // intervals depend on the chosen model.
    intervalLabel->show();
    secondsLabel->show();
    switch(modelCombo->currentIndex()) {
    // no model
    case 0:
        intervalLabel->hide();
        secondsLabel->hide();
        showShortAnaerobicIntervals(false);
        showAnaerobicIntervals(false);
        showAerobicIntervals(false);
        showLongAerobicIntervals(false);
        break;
    // 2 parameter model
    case 1:
    // 3 parameter model
    case 2:
    // WS-model
    case 5:
        showShortAnaerobicIntervals(false);
        showAnaerobicIntervals(true);
        showAerobicIntervals(true);
        showLongAerobicIntervals(false);
        break;
    case 3:
        showShortAnaerobicIntervals(true);
        showAnaerobicIntervals(true);
        showAerobicIntervals(true);
        showLongAerobicIntervals(true);
        break;
    default:
        // we should not reach this point, since models >= 3 have been handled
        // before.
        break;


    }
}

void
CriticalPowerWindow::modelChanged()
{
    // we changed from/to a 2 or 3 parameter model
    // so lets set some sensible defaults, these are
    // based on advice from our exercise physiologist friends
    // for best results in predicting both W' and CP and providing
    // a reasonable fit for durations < 2mins.
    active = true;

    // hide veloclinic's variation for everyone
    // it will get shown for model 4 below
    vlabel->hide();
    velo1->hide();
    velo2->hide();
    velo3->hide();

    // multimodels use envelope, always
    // others should use LM, but user can choose
    if (modelCombo->currentIndex() > 2) {
        fitCombo->setCurrentIndex(0);
        fitdataCombo->setCurrentIndex(0);
        fitCombo->setEnabled(false);
        fitdataCombo->setEnabled(false);
    } else {
        fitCombo->setCurrentIndex(1);
        fitdataCombo->setCurrentIndex(1);
        fitCombo->setEnabled(true);
        fitdataCombo->setEnabled(true);
    }

    // disable linear regression fit for all models
    // except CP2, this is a bit of a hack, but works
    QStandardItem *item=qobject_cast<QStandardItemModel*>(fitCombo->model())->item(2);
    item->setFlags(modelCombo->currentIndex() != 1 ? item->flags() & ~Qt::ItemIsEnabled: item->flags() | Qt::ItemIsEnabled);

    switch (modelCombo->currentIndex()) {

    case 0 : // None
            modelDecayCheck->hide();
            modelDecayLabel->hide();

            // No default values !
            break;

    case 4 : // Veloclinic Model uses 2 parameter classic but
             // also lets you select a variation ..
            vlabel->show();
            velo1->show();
            velo2->show();
            velo3->show();
            modelDecayCheck->hide();
            modelDecayLabel->hide();

            // intentional fallthrough
            // and drop through into case 1 below ...

    case 1 : // Classic 2 param model 2-20 default (per literature)
            modelDecayCheck->hide();
            modelDecayLabel->hide();

            // Default values: class 2-3mins 10-20 model
            anI1SpinBox->setValue(120);
            anI2SpinBox->setValue(200);
            aeI1SpinBox->setValue(720);
            aeI2SpinBox->setValue(1200);

            break;

    case 2 : // 3 param model: 30-60 model
    case 5 : // WS model: 30-60 model
            modelDecayLabel->show();
            modelDecayCheck->show();

            // Default values
            anI1SpinBox->setValue(180);
            anI2SpinBox->setValue(240);
            aeI1SpinBox->setValue(600);
            aeI2SpinBox->setValue(1200);

            break;

    case 3 : // ExtendedCP
            modelDecayCheck->hide();
            modelDecayLabel->hide();

            // Default values
            sanI1SpinBox->setValue(20);
            sanI2SpinBox->setValue(90);
            anI1SpinBox->setValue(120);
            anI2SpinBox->setValue(300);
            aeI1SpinBox->setValue(420);
            aeI2SpinBox->setValue(1800);
            laeI1SpinBox->setValue(4000);
            laeI2SpinBox->setValue(30000);

            break;
    }
    active = false;

    // update the plot.
    modelParametersChanged();
}

// kind of tedious but return index into a radio button group
// for the button that is actually checked.
int
CriticalPowerWindow::variant() const
{
    if (velo1->isChecked()) return 0;
    if (velo2->isChecked()) return 1;
    if (velo3->isChecked()) return 2;
    return 0; // default
}

void
CriticalPowerWindow::setVariant(int index)
{
    switch (index) {
    case 0 : velo1->setChecked(true); velo2->setChecked(false); velo3->setChecked(false); break;
    case 1 : velo1->setChecked(false); velo2->setChecked(true); velo3->setChecked(false); break;
    case 2 : velo1->setChecked(false); velo2->setChecked(false); velo3->setChecked(true); break;
    }
}

void
CriticalPowerWindow::modelParametersChanged()
{
    if (active == true) return;

    // need a helper any more ?
    if (seriesCombo->currentIndex() >= 0) {
        CriticalSeriesType series = static_cast<CriticalSeriesType>(seriesCombo->itemData(seriesCombo->currentIndex()).toInt());
        updateOptions(series);
    }

    // tell the plot
    cpPlot->setModel(sanI1SpinBox->value(),
                        sanI2SpinBox->value(),
                        anI1SpinBox->value(),
                        anI2SpinBox->value(),
                        aeI1SpinBox->value(),
                        aeI2SpinBox->value(),
                        laeI1SpinBox->value(),
                        laeI2SpinBox->value(),
                        modelCombo->currentIndex(),
                        variant(),
                        fitCombo->currentIndex(),
                        fitdataCombo->currentIndex(),
                        modelDecay());

    // and apply
    if (amVisible() && myRideItem != NULL) {
        cpPlot->setRide(myRideItem);
    }
    showRelevantIntervals();
    // disable data selection for envelope fits, since it has no effect for
    // these.
    if (fitCombo->currentIndex() == 0) {
        fitdataCombo->setEnabled(false);
    } else {
        fitdataCombo->setEnabled(true);
    }
}

void
CriticalPowerWindow::refreshRideSaved(RideItem *item)
{
    if (!item) return;

    // if the saved ride is in the aggregated time period
    QDate date = item->dateTime.date();

    // in rangemode ?
    if (rangemode && date >= cpPlot->startDate &&
        date <= cpPlot->endDate) {

        // force a redraw next time visible
        cpPlot->setDateRange(cpPlot->startDate, cpPlot->endDate);
    }

    if (!rangemode) {
        // reset date range for bests and model
        seasonSelected(cComboSeason->currentIndex());
    }
}

void
CriticalPowerWindow::forceReplot()
{
    stale = true; // we must become stale

    CriticalSeriesType series = static_cast<CriticalSeriesType>(seriesCombo->itemData(seriesCombo->currentIndex()).toInt());
    if ((rangemode && context->isCompareDateRanges) || (!rangemode && context->isCompareIntervals)) {

        // hide in compare mode
        helperWidget()->hide();

        rPercent->hide();
        rHeat->hide();
        rDelta->show();
        rDeltaPercent->show();

    } else {

        // show helper if we're showing power
        updateOptions(series);

        // these are allowed outside of compare mode
        rPercent->show();
        rHeat->show();
        rDelta->hide();
        rDeltaPercent->hide();
    }
    cpPlot->setSeries(series); // Update y-axis

    if (rangemode) {

        // force replot...
        dateRangeChanged(myDateRange);

    } else {

        // if visible make the changes visible
        // rideSelected is easiest way
        if (amVisible()) rideSelected();
    }
    repaint();
}

void
CriticalPowerWindow::newRideAdded(RideItem *here)
{
    // mine just got Zapped, a new rideitem would not be my current item
    if (here == currentRide) currentRide = NULL;

    // any plots we already have are now stale
    if (!rangemode) {

        Season season = seasons->seasons.at(cComboSeason->currentIndex());
        stale = true;

        if ((here->dateTime.date() >= season.getStart() || season.getStart() == QDate())
                && (here->dateTime.date() <= season.getEnd() || season.getEnd() == QDate())) {
            // replot
            forceReplot();
        }

    } else {

        forceReplot();

    }
}

void
CriticalPowerWindow::intervalSelected()
{
    if (rangemode) return; // do nothing for ranges!

    // in compare mode we don't plot intervals from the sidebar
    if (!rangemode && context->isCompareIntervals) {

        // wipe away any we might have
        foreach(QwtPlotCurve *p, intervalCurves) {
            if (p) {
                p->detach();
                delete p;
            }
        }
        return;
    }

    // nothing to plot
    if (!amVisible() || myRideItem == NULL) return;

    // if the array hasn't been initialised properly then clean it up
    // this is because intervalsChanged gets called when selecting rides
    if (intervalCurves.count() != myRideItem->intervals().count()) {
        // wipe away what we got, even if not visible
        // clear any interval curves -- even if we are not visible
        foreach(QwtPlotCurve *p, intervalCurves) {
            if (p) {
                p->detach();
                delete p;
            }
        }

        // clear, resize to interval count and set to null
        intervalCurves.clear();
        if (myRideItem && myRideItem->ride() && myRideItem->intervals().count())
            for (int i=0; i< myRideItem->intervals().count(); i++)
                intervalCurves << NULL;
    }

    // which itervals are selected?
    int i=0;
    foreach (IntervalItem*p, myRideItem->intervals()) {
        if (p != NULL) {
            if (p->selected == true) {
                showIntervalCurve(p, i); // set it all up
            } else {
                hideIntervalCurve(i); // in case its shown at present
            }
        }
        i++;
    }
    cpPlot->replot();
}

// user hovered over an interval
void
CriticalPowerWindow::intervalHover(IntervalItem* x)
{
    if (myRideItem == NULL) return;

    // ignore in compare mode
    if (!amVisible() || context->isCompareIntervals) return;

    // do we need to fill with nulls ?
    if (intervalCurves.count() == 0 && myRideItem && myRideItem->ride() && myRideItem->intervals().count())
        for (int i=0; i< myRideItem->intervals().count(); i++)
            intervalCurves << NULL;

    // only one interval can be hovered at any one time
    // so we always use the same curve to ensure we don't leave
    // any nasty artefacts behind. And its always gray :)

    // first lets see what interval this actually is?
    int index = myRideItem->intervals().indexOf(x);

    if (index >=0 && index < intervalCurves.count()) {

        // lazy for now just reuse existing
        if (intervalCurves[index] == NULL) {

            // get the data setup
            // but if there is no data for the ride series
            // selected they will still be null
            showIntervalCurve(x, index); // set it all up
            hideIntervalCurve(index); // in case its shown at present
        }

        // wipe what we have
        if (hoverCurve != NULL) {
            hoverCurve->detach();
            delete hoverCurve;
            hoverCurve = NULL;
        }

        // still NULL so they have no data
        if (intervalCurves[index] == NULL) return;

        // clone the data
        QVector<QPointF> array;
        for (size_t i=0; i<intervalCurves[index]->data()->size(); i++) array << intervalCurves[index]->data()->sample(i);

        QPen pen(Qt::gray);
        double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
        pen.setWidth(width);

        // create the hover curve
        hoverCurve = new QwtPlotCurve("Interval");
        hoverCurve->setPen(pen);
        if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true) hoverCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
        hoverCurve->setYAxis(QwtAxis::YLeft);
        hoverCurve->setSamples(array);
        hoverCurve->setVisible(true);
        hoverCurve->setZ(100);
        hoverCurve->attach(cpPlot);
        cpPlot->replot();
    }
}

void
CriticalPowerWindow::hideIntervalCurve(int index)
{
    if (rangemode) return; // do nothing for ranges!

    if (intervalCurves[index] != NULL) {
        intervalCurves[index]->detach(); // in case
    }
}

void
CriticalPowerWindow::showIntervalCurve(IntervalItem *current, int index)
{
    if (rangemode) return; // do nothing for ranges!

    // is this interval being created as we plot ?
    if (index == intervalCurves.count()) intervalCurves << NULL;

    // we already made it?
    if (intervalCurves[index] != NULL) {
        intervalCurves[index]->detach(); // in case
        intervalCurves[index]->attach(cpPlot);

        return;
    }

    // ack, we need to create the curve for this interval

    // make a ridefile
    RideFile f(myRideItem->ride());

    foreach(RideFilePoint *p, myRideItem->ride()->dataPoints()) {
       if ((p->secs+f.recIntSecs()) >= current->start && p->secs <= (current->stop+f.recIntSecs())) {
           f.appendPoint(p->secs, p->cad, p->hr, p->km, p->kph, p->nm,
                       p->watts, p->alt, p->lon, p->lat, p->headwind,
                       p->slope, p->temp, p->lrbalance,
                       p->lte, p->rte, p->lps, p->rps,
                       p->lpco, p->rpco, p->lppb, p->rppb, p->lppe, p->rppe, p->lpppb, p->rpppb, p->lpppe, p->rpppe,
                       p->smo2, p->thb,
                       p->rvert, p->rcad, p->rcontact, p->tcore, 0);
       }
    }
    // for xpower and acceleration et al
    f.recalculateDerivedSeries();

    // compute the mean max, this is BLAZINGLY fast, thanks to Mark Rages'
    // mean-max computer. Does a 11hr ride in 150ms
    QVector<float>vector;
    MeanMaxComputer thread1(&f, vector, getRideSeries(series())); thread1.run();
    thread1.wait();

    // no data!
    if (vector.count() == 0) return;

    // create curve data arrays
    QVector<double>y;
    RideFileCache::doubleArray(y, vector, getRideSeries(series()));

    QVector<double>x;
    for (int i=0; i<vector.count(); i++) {
        x << double(i)/60.00f;
    }

    // create a curve!
    QwtPlotCurve *curve = new QwtPlotCurve();
    if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true)
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    // set its color - based upon index in intervals!
    QColor intervalColor;
    int count=myRideItem->intervals().count();
    intervalColor.setHsv(index * (255/count), 255,255);
    QPen pen(intervalColor);
    double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
    pen.setWidth(width);
    //pen.setStyle(Qt::DotLine);
    intervalColor.setAlpha(64);
    QBrush brush = QBrush(intervalColor);
    if (shadeIntervalsCheck->isChecked()) curve->setBrush(brush);
    else curve->setBrush(Qt::NoBrush);
    curve->setPen(pen);
    curve->setSamples(x.data()+1, y.data()+1, x.count()-2); // ignore he first 0,0 point

    // attach and register
    curve->attach(cpPlot);
    intervalCurves[index] = curve;
}

void
CriticalPowerWindow::intervalsChanged()
{
    if (rangemode) return; // do nothing for ranges!

    // wipe away what we got, even if not visible
    // clear any interval curves -- even if we are not visible
    foreach(QwtPlotCurve *p, intervalCurves) {
        if (p) {
            p->detach();
            delete p;
        }
    }

    // clear, resize to interval count and set to null
    intervalCurves.clear();

    if (!amVisible()) return;

    // replot if needed
    forceReplot();
}

void
CriticalPowerWindow::rideSelected()
{
    if (!rangemode) { // we only highlight intervals in normal mode

        // clear any interval curves -- even if we are not visible
        foreach(QwtPlotCurve *p, intervalCurves) {
            if (p) {
                p->detach();
                delete p;
            }
        }

        // clear, resize to interval count and set to null
        intervalCurves.clear();

        // clear the hover curve
        if (hoverCurve) {
            hoverCurve->detach();
            delete hoverCurve;
            hoverCurve = NULL;
        }

        Season season = seasons->seasons.at(cComboSeason->currentIndex());

        if (myRideItem) {
            // if the range selected is relative (e.g. "This Year" or
            // "Last 28 days", compute it with respect to the ride's
            // date
            cpPlot->setDateRange(season.getStart(myRideItem->dateTime.date()), season.getEnd(myRideItem->dateTime.date()));
        } else {
            cpPlot->setDateRange(season.getStart(), season.getEnd());
        }
    }

    if (!amVisible()) return;
    currentRide = myRideItem;

    if (currentRide) {

        if (context->athlete->zones(currentRide->sport)) {
            int zoneRange = context->athlete->zones(currentRide->sport)->whichRange(currentRide->dateTime.date());
            int CP = zoneRange >= 0 ? context->athlete->zones(currentRide->sport)->getCP(zoneRange) : 0;
            CPEdit->setText(QString("%1").arg(CP));
            cpPlot->setDateCP(CP);
        } else {
            cpPlot->setDateCP(0);
        }
        if ((currentRide->isRun || currentRide->isSwim) && context->athlete->paceZones(currentRide->isSwim)) {
            int paceZoneRange = context->athlete->paceZones(currentRide->isSwim)->whichRange(currentRide->dateTime.date());
            double CV = paceZoneRange >= 0.0 ? context->athlete->paceZones(currentRide->isSwim)->getCV(paceZoneRange) : 0.0;
            cpPlot->setDateCV(CV);
        } else {
            cpPlot->setDateCV(0.0);
        }
        cpPlot->setRide(currentRide);

        if (!rangemode && currentRide->ride() && currentRide->ride()->dataPoints().count() == 0)
            setIsBlank(true);
        else
            setIsBlank(false);

    } else if (!rangemode) {

        setIsBlank(true);
    }

    // refresh intervals
    intervalSelected();
}

bool
CriticalPowerWindow::event(QEvent *event)
{
    // nasty nasty nasty hack to move widgets as soon as the widget geometry
    // is set properly by the layout system, by default the width is 100 and
    // we wait for it to be set properly then put our helper widget on the RHS
    if (event->type() == QEvent::Resize && geometry().width() != 100) {

        // put somewhere nice on first show
        if (firstShow) {
            firstShow = false;
            helperWidget()->move(mainWidget()->geometry().width()-(275*dpiXFactor), 50*dpiYFactor);
        }

        // if off the screen move on screen
        if (helperWidget()->geometry().x() > geometry().width() || helperWidget()->geometry().x() < geometry().x()) {
            helperWidget()->move(mainWidget()->geometry().width()-(275*dpiXFactor), 50*dpiYFactor);
        }
    }
    return QWidget::event(event);
}

void
CriticalPowerWindow::setSeries(int index)
{
    // update reveal control
    rSeriesSelector->setValue(index);

    if (index >= 0) {

        // need a helper any more ?
        CriticalSeriesType series = static_cast<CriticalSeriesType>(seriesCombo->itemData(index).toInt());

        // hide velo cp editing
        if (series == veloclinicplot) {
            // || ((series == watts || series == wattsKg ) && modelCombo->currentIndex() >= 1)) {
            CPEdit->show();
            CPSlider->show();
            CPLabel->show();
            cpPlot->setVeloCP(CPEdit->text().toInt());
        } else {
            CPEdit->hide();
            CPSlider->hide();
            CPLabel->hide();
        }

        updateOptions(series);

        if (rangemode) {

            cpPlot->setSeries(series);
            cpPlot->setRide(currentRide);

        } else {

            // clear any interval curves -- even if we are not visible
            foreach(QwtPlotCurve *p, intervalCurves) {
                if (p) {
                    p->detach();
                    delete p;
                }
            }

            // clear, resize to interval count and set to null
            intervalCurves.clear();
            if (myRideItem) for (int i=0; i<= myRideItem->intervals().count(); i++) intervalCurves << NULL;

            cpPlot->setSeries(series);
            cpPlot->setRide(currentRide);

            // refresh intervals
            intervalSelected();
        }
    }
}

double
CriticalPowerWindow::curve_to_point(double x, const QwtPlotCurve *curve, CriticalSeriesType serie)
{
    double result = 0;
    if (curve) {
        const QwtSeriesData<QPointF> *data = curve->data();

        if (data->size() > 0) {
            if (x < data->sample(0).x() || x > data->sample(data->size() - 1).x())
                return 0;
            size_t min = 0, mid = 0, max = data->size();
            while (min < max - 1) {
                mid = (max - min) / 2 + min;
                if (x < data->sample(mid).x()) {
                    double a = pow(10,RideFileCache::decimalsFor(getRideSeries(serie)));

                    result = ((int)((0.5/a + data->sample(mid).y()) * a))/a;
                    //result = (unsigned) round(data->sample(mid).y());
                    max = mid;
                }
                else {
                    min = mid;
                }
            }
        }
    }
    return result;
}

void
CriticalPowerWindow::updateCpint(double minutes)
{
    QString units;

    switch (series()) {

        case veloclinicplot:
        case work:
            units = "kJ";
            break;

        case cad:
            units = "rpm";
            break;

        case kphd:
            units = "metres/s/s";
            break;

        case wattsd:
            units = "watts/s";
            break;

        case cadd:
            units = "rpm/s";
            break;

        case hrd:
            units = "bpm/s";
            break;

        case nmd:
            units = "nm/s";
            break;

        case kph:
            units = "kph";
            break;

        case hr:
            units = "bpm";
            break;

        case nm:
            units = "nm";
            break;

        case vam:
            units = "metres/hour";
            break;

        case aPowerKg:
        case wattsKg:
            units = "Watts/kg";
            break;

        default:
        case aPower:
        case watts:
            units = "Watts";
            break;

    }

    // current ride
    {
      double value = curve_to_point(minutes, cpPlot->getThisCurve(), series());
      QString label;
      if (value > 0)
          label = QString("%1 %2").arg(value).arg(units);
      else
          label = tr("no data");
          //XXXcpintTodayValue->setText(label);
    }

    // cp line
    if (cpPlot->getModelCurve()) {
      double value = curve_to_point(minutes, cpPlot->getModelCurve(), series());
      QString label;
      if (value > 0)
        label = QString("%1 %2").arg(value).arg(units);
      else
        label = tr("no data");
      cpintCPValue->setText(label);
    }

    // global ride
    {
      QString label;
      int index = (int) ceil(minutes * 60);
      if (index >= 0 && cpPlot->getBests().count() > index) {
          QDate date = cpPlot->getBestDates()[index];
          double value = cpPlot->getBests()[index];

          double a = pow(10,RideFileCache::decimalsFor(getRideSeries(series())));
          value = ((int)((0.5/a + value) * a))/a;

              label = QString("%1 %2 (%3)").arg(value).arg(units)
                      .arg(date.isValid() ? date.toString(tr("MM/dd/yyyy")) : tr("no date"));
      }
      else {
        label = tr("no data");
      }
      cpintAllValue->setText(label);
    }
}

QString
CriticalPowerWindow::seriesName(CriticalSeriesType series)
{
    switch (series) {
        case watts: return QString(tr("Power"));
        case wattsKg: return QString(tr("Watts per Kilogram"));
        case xPower: return QString(tr("xPower"));
        case IsoPower: return QString(tr("Iso Power"));
        case hr: return QString(tr("Heartrate"));
        case kph: return QString(tr("Speed"));
        case kphd: return QString(tr("Acceleration"));
        case wattsd: return QString(tr("Power %1").arg(deltaChar));
        case cadd: return QString(tr("Cadence %1").arg(deltaChar));
        case nmd: return QString(tr("Torque %1").arg(deltaChar));
        case hrd: return QString(tr("Heartrate %1").arg(deltaChar));
        case cad: return QString(tr("Cadence"));
        case nm: return QString(tr("Torque"));
        case vam: return QString(tr("VAM"));
        case aPower: return QString(tr("aPower"));
        case aPowerKg: return QString(tr("aPower per Kilogram"));
        case work: return QString(tr("Work"));
        case veloclinicplot: return QString(tr("Veloclinic Plot"));

        default: return QString(tr("Unknown"));
    }
}

RideFile::SeriesType
CriticalPowerWindow::getRideSeries(CriticalSeriesType series)
{
    switch (series) {
        case watts: return RideFile::watts;
        case wattsKg: return RideFile::wattsKg;
        case xPower: return RideFile::xPower;
        case IsoPower: return RideFile::IsoPower;
        case hr: return RideFile::hr;
        case kph: return RideFile::kph;
        case kphd: return RideFile::kphd;
        case wattsd: return RideFile::wattsd;
        case cadd: return RideFile::cadd;
        case nmd: return RideFile::nmd;
        case hrd: return RideFile::hrd;
        case cad: return RideFile::cad;
        case nm: return RideFile::nm;
        case vam: return RideFile::vam;
        case aPower: return RideFile::aPower;
        case aPowerKg: return RideFile::aPowerKg;

        case veloclinicplot: return RideFile::watts;

        // non RideFile series
        case work: return RideFile::none;

        default: return RideFile::none;
    }
}

void
CriticalPowerWindow::addSeries()
{
    // setup series list
    seriesList << watts
               << wattsKg
               << xPower
               << IsoPower
               << hr
               << kph
               << cad
               << nm
               << vam
               << aPower
               << aPowerKg
               << kphd
               << wattsd
               << nmd
               << cadd
               << hrd
               << work
               << veloclinicplot;

    QStringList seriesNames;

    foreach (CriticalSeriesType x, seriesList) {
        seriesCombo->addItem(seriesName(x), static_cast<int>(x));
        seriesNames << seriesName(x);
    }

    rSeriesSelector->setStrings(seriesNames);
}

void
CriticalPowerWindow::updateOptions(CriticalSeriesType series)
{
    if ((series == work || series == watts || series == wattsKg || series == kph || series == aPower || series == aPowerKg) && modelCombo->currentIndex() >= 1) {
        helperWidget()->show();
    } else {
        helperWidget()->hide();
    }

    if (series == kph) {
        // speed series have option to display x-axis linear or log, others are defined fix in CPPlot::setSeries()
        showCSLinearCheck->show();
        showCSLinearLabel->show();
    } else {
        showCSLinearCheck->hide();
        showCSLinearLabel->hide();
    }
}

/*----------------------------------------------------------------------
 * Seasons stuff
 *--------------------------------------------------------------------*/


void
CriticalPowerWindow::resetSeasons()
{
    if (rangemode) return;

    QString prev = cComboSeason->itemText(cComboSeason->currentIndex());

    // remove seasons
    cComboSeason->clear();

    //Store current selection
    QString previousDateRange = _dateRange;
    // insert seasons
    for (int i=0; i <seasons->seasons.count(); i++) {
        Season season = seasons->seasons.at(i);
        // we add the id as user data since there are some 'fixed' ids
        // that represent last 28 days etc that we can then use to offset
        // from the date of the ride
        cComboSeason->addItem(season.getName(), QVariant(season.id()));
    }
    // restore previous selection
    int index = cComboSeason->findText(prev);
    if (index != -1)  {
        cComboSeason->setCurrentIndex(index);
    }
}

void
CriticalPowerWindow::useCustomRange(DateRange range)
{
    // plot using the supplied range
    useCustom = true;
    useToToday = false;
    custom = range;
    dateRangeChanged(custom);
}

void
CriticalPowerWindow::useStandardRange()
{
    useToToday = useCustom = false;
    dateRangeChanged(myDateRange);
}

void
CriticalPowerWindow::useThruToday()
{
    // plot using the supplied range
    useCustom = false;
    useToToday = true;
    custom = myDateRange;
    if (custom.to > QDate::currentDate()) custom.to = QDate::currentDate();
    dateRangeChanged(custom);
}

void
CriticalPowerWindow::dateRangeChanged(DateRange dateRange)
{
    if (!amVisible()) return;

    // it will either be sidebar or custom...
    if (useCustom) dateRange = custom;
    else if (useToToday) {

        dateRange = myDateRange;
        QDate today = QDate::currentDate();
        if (dateRange.to > today) dateRange.to = today;

    } else dateRange = myDateRange;

    // only change date range if its actually changed!
    if (series() == veloclinicplot || dateRange.from != cfrom || dateRange.to != cto || stale) {

        cfrom = dateRange.from;
        cto = dateRange.to;

        FilterSet fs;
        fs.addFilter(searchBox->isFiltered(), SearchFilterBox::matches(context, filter()));
        fs.addFilter(context->isfiltered, context->filters);
        fs.addFilter(context->ishomefiltered, context->homeFilters);
        if (myPerspective) fs.addFilter(myPerspective->isFiltered(), myPerspective->filterlist(dateRange));
        int nActivities, nRides, nRuns, nSwims;
        QString sport;
        context->athlete->rideCache->getRideTypeCounts(
                                        Specification(dateRange, fs),
                                        nActivities, nRides, nRuns, nSwims, sport);

        // lets work out the average CP configure value
        if (series() != veloclinicplot && context->athlete->zones(sport)) {
            int fromZoneRange = context->athlete->zones(sport)->whichRange(cfrom);
            int toZoneRange = context->athlete->zones(sport)->whichRange(cto);

            int CPfrom = fromZoneRange >= 0 ? context->athlete->zones(sport)->getCP(fromZoneRange) : 0;
            int CPto = toZoneRange >= 0 ? context->athlete->zones(sport)->getCP(toZoneRange) : CPfrom;
            if (CPfrom == 0) CPfrom = CPto;
            int dateCP = (CPfrom + CPto) / 2;

            cpPlot->setDateCP(dateCP);
        } else {
            cpPlot->setDateCP(CPEdit->text().toInt());
        }

        // lets work out the average CV configure value
        if (((nActivities == nRuns) || (nActivities == nSwims)) &&
             context->athlete->paceZones(nActivities == nSwims)) {
            int fromZoneRange = context->athlete->paceZones(nActivities == nSwims)->whichRange(cfrom);
            int toZoneRange = context->athlete->paceZones(nActivities == nSwims)->whichRange(cto);

            double CVfrom = fromZoneRange >= 0 ? context->athlete->paceZones(nActivities == nSwims)->getCV(fromZoneRange) : 0.0;
            double CVto = toZoneRange >= 0 ? context->athlete->paceZones(nActivities == nSwims)->getCV(toZoneRange) : CVfrom;
            if (CVfrom == 0.0) CVfrom = CVto;
            double dateCV = (CVfrom + CVto) / 2.0;

            cpPlot->setDateCV(dateCV);
            cpPlot->setSport(sport);
        } else {
            cpPlot->setDateCV(0.0);
            cpPlot->setSport(sport);
        }

        cpPlot->setDateRange(dateRange.from, dateRange.to, stale);
    }

    // always refresh though
    cpPlot->setRide(currentRide);

    stale = false;
}

void CriticalPowerWindow::seasonSelected(int iSeason)
{
    if (iSeason >= seasons->seasons.count() || iSeason < 0) return;
    Season season = seasons->seasons.at(iSeason);
    //XXX BROKEM CODE IN 5.1 PORT // _dateRange = season.id();
    cpPlot->setDateRange(season.getStart(), season.getEnd());
    cpPlot->setRide(currentRide);
}

void CriticalPowerWindow::perspectiveFilterChanged()
{
    if (rangemode) {
        cpPlot->perspectiveFilterChanged();
        forceReplot();
    }
}

void CriticalPowerWindow::filterChanged()
{
    cpPlot->setRide(currentRide);
    // Pace Zones Shading and Pace Units needs updating if sport(s) changed
    forceReplot();
}

void
CriticalPowerWindow::shadingSelected(int shading)
{
    cpPlot->setShadeMode(shading);
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::showGridChanged(int state)
{
    // redraw
    if (state) grid->setVisible(true);
    else grid->setVisible(false);
    cpPlot->replot();
}

void
CriticalPowerWindow::filterBestChanged(int state)
{
    cpPlot->setFilterBest(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}
void
CriticalPowerWindow::showTestChanged(int state)
{
    cpPlot->setShowTest(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::showBestChanged(int state)
{
    cpPlot->setShowBest(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::showPercentChanged(int state)
{
    if (state) showPowerIndexCheck->setChecked(false);
    cpPlot->setShowPercent(state);
    rPercent->setChecked(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::showPowerIndexChanged(int state)
{
    if (state) showPercentCheck->setChecked(false);
    cpPlot->setShowPowerIndex(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::rSeriesSelectorChanged(int value)
{
    seriesCombo->setCurrentIndex(value);
    setSeries(value);
}

void
CriticalPowerWindow::rPercentChanged(int check)
{
    showPercentCheck->setChecked(check);
}

void
CriticalPowerWindow::showEffortChanged(int state)
{
    cpPlot->setShowEffort(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::showPPChanged(int state)
{
    cpPlot->setShowPP(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::showHeatChanged(int state)
{
    cpPlot->setShowHeat(state);
    rHeat->setChecked(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::showCSLinearChanged(int state)
{
    cpPlot->showXAxisLinear(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
}

void
CriticalPowerWindow::rHeatChanged(int check)
{
    showHeatCheck->setChecked(check);
}

void
CriticalPowerWindow::rDeltaChanged()
{
    showDeltaCheck->setChecked(rDelta->isChecked());
    showDeltaPercentCheck->setChecked(rDeltaPercent->isChecked());
}

void
CriticalPowerWindow::showDeltaChanged()
{
    rDelta->setChecked(showDeltaCheck->isChecked());
    rDeltaPercent->setChecked(showDeltaPercentCheck->isChecked());
    cpPlot->setShowDelta(showDeltaCheck->isChecked(), showDeltaPercentCheck->isChecked());

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::showHeatByDateChanged(int state)
{
    cpPlot->setShowHeatByDate(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::shadeIntervalsChanged(int state)
{
    cpPlot->setShadeIntervals(state);

    // any existing interval curves need brush or no brush
    foreach(QwtPlotCurve *p, intervalCurves) {
    if (p) {
            if (state) {
                QColor curveColor = p->pen().color();
                curveColor.setAlpha(64);
                QBrush brush(curveColor);
                p->setBrush(brush);
            }
            else p->setBrush(Qt::NoBrush);
        }
    }
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::setPlotType(int index)
{
    cpPlot->setPlotType(index);
    cpPlot->setRide(currentRide);
}

void
CriticalPowerWindow::exportData()
{
    QString fileName = title()+".csv";
    fileName = QFileDialog::getSaveFileName(this, tr("Save Best Data as CSV"),  QString(), title()+".csv (*.csv)");

    if (!fileName.isEmpty()) {

        // open and write bests data to the csv file
        cpPlot->exportBests(fileName);
    }
}
