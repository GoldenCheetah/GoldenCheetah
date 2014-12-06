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
#include "MetricAggregator.h"
#include "CPPlot.h"
#include "Context.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "TimeUtils.h"
#include "IntervalItem.h"
#include "GcOverlayWidget.h"
#include "MUWidget.h"
#include <qwt_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>
#include <qwt_compat.h>
#include <QFile>
#include "Season.h"
#include "SeasonParser.h"
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

    //
    // Chart settings
    //

    // controls widget and layout
    QTabWidget *settingsTabs = new QTabWidget(this);

    QWidget *settingsWidget = new QWidget(this);
    settingsWidget->setContentsMargins(0,0,0,0);
    settingsTabs->addTab(settingsWidget, tr("Basic"));

    QFormLayout *cl = new QFormLayout(settingsWidget);;
    cl->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    QWidget *modelWidget = new QWidget(this);
    modelWidget->setContentsMargins(0,0,0,0);
    settingsTabs->addTab(modelWidget, tr("CP Model"));

    QFormLayout *mcl = new QFormLayout(modelWidget);;
    mcl->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    // add additional menu items before setting
    // controls since the menu is SET from setControls
    QAction *exportData = new QAction(tr("Export Chart Data..."), this);
    addAction(exportData);

    setControls(settingsTabs);

#ifdef GC_HAVE_LUCENE
    // filter / searchbox
    searchBox = new SearchFilterBox(this, context);
    connect(searchBox, SIGNAL(searchClear()), cpPlot, SLOT(clearFilter()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), cpPlot, SLOT(setFilter(QStringList)));
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(filterChanged()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(filterChanged()));
    cl->addRow(new QLabel(tr("Filter")), searchBox);
    cl->addWidget(new QLabel("")); //spacing
#endif

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

    showGridCheck = new QCheckBox(this);
    showGridCheck->setChecked(true); // default on
    QLabel *gridify = new QLabel(tr("Show grid"));
    cl->addRow(gridify, showGridCheck);

    showBestCheck = new QCheckBox(this);
    showBestCheck->setChecked(true); // default off
    QLabel *bestify = new QLabel(tr("Show Bests"));
    cl->addRow(bestify, showBestCheck);

    showPercentCheck = new QCheckBox(this);
    showPercentCheck->setChecked(false); // default off
    QLabel *percentify = new QLabel(tr("Show as percentage"));
    cl->addRow(percentify, showPercentCheck);

    showHeatCheck = new QCheckBox(this);
    showHeatCheck->setChecked(false); // default off
    QLabel *heaties = new QLabel(tr("Show curve heat"));
    cl->addRow(heaties, showHeatCheck);

    showHeatByDateCheck = new QCheckBox(this);
    showHeatByDateCheck->setChecked(false); // default off
    QLabel *heatiesByDate = new QLabel(tr("Show curve heat by date"));
    cl->addRow(heatiesByDate, showHeatByDateCheck);

    shadeIntervalsCheck = new QCheckBox(this);
    shadeIntervalsCheck->setChecked(true); // default on
    QLabel *shadies = new QLabel(tr("Shade Intervals"));
    cl->addRow(shadies, shadeIntervalsCheck);

    ridePlotStyleCombo = new QComboBox(this);
    ridePlotStyleCombo->addItem(tr("Ride Mean Max"));
    ridePlotStyleCombo->addItem(tr("Ride Centile"));
    ridePlotStyleCombo->addItem(tr("No Ride"));

    cl->addWidget(new QLabel("")); //spacing
    cl->addRow(new QLabel(tr("Current Ride")), ridePlotStyleCombo);

    // model config
    // 2 or 3 point model ?
    modelCombo= new QComboBox(this);
    modelCombo->addItem(tr("None"));
    modelCombo->addItem(tr("2 parameter"));
    modelCombo->addItem(tr("3 parameter"));
    modelCombo->addItem(tr("Extended CP"));
    modelCombo->addItem(tr("Multicomponent"));
    modelCombo->setCurrentIndex(1);

    mcl->addRow(new QLabel(tr("CP Model")), modelCombo);

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
    aeI1SpinBox->setValue(1200); // 30 minutes

    aeI2SpinBox = new QDoubleSpinBox(this);
    aeI2SpinBox->setDecimals(0);
    aeI2SpinBox->setMinimum(0.0);
    aeI2SpinBox->setMaximum(3600);
    aeI2SpinBox->setSingleStep(1.0);
    aeI2SpinBox->setAlignment(Qt::AlignRight);
    aeI2SpinBox->setValue(1800); // 60 minutes

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
    grid->enableX(true); // not needed
    grid->enableY(true);
    grid->setZ(-20);
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
    ftpTitle = new QLabel(tr("FTP"), this);
    ftpValue = new QLabel(tr("0 w"), this);
    ftpRank = new QLabel(tr("n/a"), this);
    eiTitle = new QLabel(tr("Endurance Index"), this);
    eiValue = new QLabel(tr("n/a"), this);

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
    ftpTitle->setAutoFillBackground(true);
    ftpValue->setAutoFillBackground(true);
    ftpRank->setAutoFillBackground(true);
    eiTitle->setAutoFillBackground(true);
    eiValue->setAutoFillBackground(true);

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
    ftpTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ftpValue->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    ftpRank->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    eiTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    eiValue->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    // add to grid
    gridLayout->addWidget(titleBlank, 0, 0);
    gridLayout->addWidget(titleValue, 0, 1);
    gridLayout->addWidget(titleRank, 0, 2);
    gridLayout->addWidget(wprimeTitle, 1, 0);
    gridLayout->addWidget(wprimeValue, 1, 1);
    gridLayout->addWidget(wprimeRank, 1, 2);
    gridLayout->addWidget(cpTitle, 2, 0);
    gridLayout->addWidget(cpValue, 2, 1);
    gridLayout->addWidget(cpRank, 2, 2);
    gridLayout->addWidget(pmaxTitle, 3, 0);
    gridLayout->addWidget(pmaxValue, 3, 1);
    gridLayout->addWidget(pmaxRank, 3, 2);
    gridLayout->addWidget(ftpTitle, 4, 0);
    gridLayout->addWidget(ftpValue, 4, 1);
    gridLayout->addWidget(ftpRank, 4, 2);
    gridLayout->addWidget(eiTitle, 5, 0);
    gridLayout->addWidget(eiValue, 5, 1);

#ifdef GC_HAVE_MUMODEL
    addHelper(QString(tr("Motor Unit Model")), new MUWidget(this, context));
#endif
    addHelper(QString(tr("CP Model")), helper);

    if (rangemode) {
        connect(this, SIGNAL(dateRangeChanged(DateRange)), SLOT(dateRangeChanged(DateRange)));

        // Compare
        connect(context, SIGNAL(compareDateRangesStateChanged(bool)), SLOT(forceReplot()));
        connect(context, SIGNAL(compareDateRangesChanged()), SLOT(forceReplot()));
    } else {
        // when working on a ride we can select intervals!
        connect(cComboSeason, SIGNAL(currentIndexChanged(int)), this, SLOT(seasonSelected(int)));
        connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
        connect(context, SIGNAL(intervalsChanged()), this, SLOT(intervalsChanged()));  
        connect(context, SIGNAL(intervalHover(RideFileInterval)), this, SLOT(intervalHover(RideFileInterval)));  

        // Compare
        connect(context, SIGNAL(compareIntervalsStateChanged(bool)), SLOT(forceReplot()));
        connect(context, SIGNAL(compareIntervalsChanged()), SLOT(forceReplot()));
    }

    connect(seriesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setSeries(int)));
    connect(ridePlotStyleCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setPlotType(int)));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(configChanged()), cpPlot, SLOT(configChanged()));
    connect(exportData, SIGNAL(triggered()), this, SLOT(exportData()));

    // model updated?
    connect(modelCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(modelChanged()));
    connect(anI1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(anI2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(aeI1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(aeI2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(sanI1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(sanI2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(laeI1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(laeI2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(velo1, SIGNAL(toggled(bool)), this, SLOT(modelParametersChanged()));
    connect(velo2, SIGNAL(toggled(bool)), this, SLOT(modelParametersChanged()));
    connect(velo3, SIGNAL(toggled(bool)), this, SLOT(modelParametersChanged()));

    // redraw on config change -- this seems the simplest approach
    connect(context, SIGNAL(filterChanged()), this, SLOT(forceReplot()));
    connect(context, SIGNAL(homeFilterChanged()), this, SLOT(forceReplot()));
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(context->athlete->metricDB, SIGNAL(dataChanged()), this, SLOT(refreshRideSaved()));
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(newRideAdded(RideItem*)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(newRideAdded(RideItem*)));
    connect(seasons, SIGNAL(seasonsChanged()), this, SLOT(resetSeasons()));
    connect(shadeCheck, SIGNAL(stateChanged(int)), this, SLOT(shadingSelected(int)));
    connect(shadeIntervalsCheck, SIGNAL(stateChanged(int)), this, SLOT(shadeIntervalsChanged(int)));
    connect(showHeatCheck, SIGNAL(stateChanged(int)), this, SLOT(showHeatChanged(int)));
    connect(rHeat, SIGNAL(stateChanged(int)), this, SLOT(rHeatChanged(int)));
    connect(rDelta, SIGNAL(stateChanged(int)), this, SLOT(rDeltaChanged()));
    connect(rDeltaPercent, SIGNAL(stateChanged(int)), this, SLOT(rDeltaChanged()));
    connect(showHeatByDateCheck, SIGNAL(stateChanged(int)), this, SLOT(showHeatByDateChanged(int)));
    connect(showPercentCheck, SIGNAL(stateChanged(int)), this, SLOT(showPercentChanged(int)));
    connect(showBestCheck, SIGNAL(stateChanged(int)), this, SLOT(showBestChanged(int)));
    connect(showGridCheck, SIGNAL(stateChanged(int)), this, SLOT(showGridChanged(int)));
    connect(rPercent, SIGNAL(stateChanged(int)), this, SLOT(rPercentChanged(int)));
    connect(dateSetting, SIGNAL(useCustomRange(DateRange)), this, SLOT(useCustomRange(DateRange)));
    connect(dateSetting, SIGNAL(useThruToday()), this, SLOT(useThruToday()));
    connect(dateSetting, SIGNAL(useStandardRange()), this, SLOT(useStandardRange()));

    // set date range for bests and model
    if (!rangemode) seasonSelected(cComboSeason->currentIndex());

    // set widgets and model parameters
    modelChanged();

    configChanged(); // get colors set
}

void
CriticalPowerWindow::configChanged()
{
    setProperty("color", GColor(CPLOTBACKGROUND));

    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    setPalette(palette);

    // inverted palette for data etc
    QPalette whitepalette;
    whitepalette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    whitepalette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    whitepalette.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));

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
    ftpTitle->setFont(font);
    ftpValue->setFont(font);
    ftpRank->setFont(font);
    eiTitle->setFont(font);
    eiValue->setFont(font);

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
    ftpTitle->setPalette(palette);
    ftpValue->setPalette(whitepalette);
    ftpRank->setPalette(whitepalette);
    eiTitle->setPalette(palette);
    eiValue->setPalette(whitepalette);

    QPen gridPen(GColor(CPLOTGRID));
    grid->setPen(gridPen);

    // set ride
    rideSelected();
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

    switch (modelCombo->currentIndex()) {

    case 0 : // None

            intervalLabel->hide();
            secondsLabel->hide();
            sanLabel->hide();
            sanI1SpinBox->hide();
            sanI2SpinBox->hide();
            anLabel->hide();
            anI1SpinBox->hide();
            anI2SpinBox->hide();
            aeLabel->hide();
            aeI1SpinBox->hide();
            aeI2SpinBox->hide();
            laeLabel->hide();
            laeI1SpinBox->hide();
            laeI2SpinBox->hide();

            // No default values !
            break;

    case 4 : // Veloclinic Model uses 2 parameter classic but 
             // also lets you select a variation ..
            vlabel->show();
            velo1->show();
            velo2->show();
            velo3->show();

            // and drop through into case 1 below ...

    case 1 : // Classic 2 param model 2-20 default (per literature)

            intervalLabel->show();
            secondsLabel->show();
            anLabel->show();
            sanLabel->hide();
            sanI1SpinBox->hide();
            sanI2SpinBox->hide();
            anLabel->show();
            anI1SpinBox->show();
            anI2SpinBox->show();
            aeLabel->show();
            aeI1SpinBox->show();
            aeI2SpinBox->show();
            laeLabel->hide();
            laeI1SpinBox->hide();
            laeI2SpinBox->hide();

            // Default values: class 2-20 model
            anI1SpinBox->setValue(100);
            anI2SpinBox->setValue(120);
            aeI1SpinBox->setValue(1000);
            aeI2SpinBox->setValue(1200);

            break;

    case 2 : // 3 param model: 3-30 model

            intervalLabel->show();
            secondsLabel->show();
            sanLabel->hide();
            sanI1SpinBox->hide();
            sanI2SpinBox->hide();
            anLabel->show();
            anI1SpinBox->show();
            anI2SpinBox->show();
            aeLabel->show();
            aeI1SpinBox->show();
            aeI2SpinBox->show();
            laeLabel->hide();
            laeI1SpinBox->hide();
            laeI2SpinBox->hide();

            // Default values
            anI1SpinBox->setValue(1800);
            anI2SpinBox->setValue(2400);
            aeI1SpinBox->setValue(2400);
            aeI2SpinBox->setValue(3600);

            break;

    case 3 : // ExtendedCP

            intervalLabel->show();
            secondsLabel->show();
            sanLabel->show();
            sanI1SpinBox->show();
            sanI2SpinBox->show();
            anLabel->show();
            anI1SpinBox->show();
            anI2SpinBox->show();
            aeLabel->show();
            aeI1SpinBox->show();
            aeI2SpinBox->show();
            laeLabel->show();
            laeI1SpinBox->show();
            laeI2SpinBox->show();         

            // Default values
            sanI1SpinBox->setValue(20);
            sanI2SpinBox->setValue(90);
            anI1SpinBox->setValue(120);
            anI2SpinBox->setValue(300);
            aeI1SpinBox->setValue(600);
            aeI2SpinBox->setValue(3000);
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
        if ((series == watts || series == wattsKg) && modelCombo->currentIndex() >= 1) helperWidget()->show();
        else helperWidget()->hide();
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
                        variant());

    // and apply
    if (amVisible() && myRideItem != NULL) {
        cpPlot->setRide(myRideItem);
    }
}

void
CriticalPowerWindow::refreshRideSaved()
{
    const RideItem *current = context->rideItem();
    if (!current) return;

    // if the saved ride is in the aggregated time period
    QDate date = current->dateTime.date();
    if (date >= cpPlot->startDate &&
        date <= cpPlot->endDate) {

        // force a redraw next time visible
        cpPlot->setDateRange(cpPlot->startDate, cpPlot->endDate);
    }
}

void
CriticalPowerWindow::forceReplot()
{
    stale = true; // we must become stale

    if ((rangemode && context->isCompareDateRanges) || (!rangemode && context->isCompareIntervals)) {

        // hide in compare mode
        helperWidget()->hide();

        rPercent->hide();
        rHeat->hide();
        rDelta->show();
        rDeltaPercent->show();

    } else {

        // show helper if we're showing power
        CriticalSeriesType series = static_cast<CriticalSeriesType>(seriesCombo->itemData(seriesCombo->currentIndex()).toInt());
        if ((series == watts || series == wattsKg) && modelCombo->currentIndex() >= 1) helperWidget()->show();
        else helperWidget()->hide();

        // these are allowed outside of compare mode
        rPercent->show();
        rHeat->show();
        rDelta->hide();
        rDeltaPercent->hide();
    }

    if (rangemode) {

        // force replot...
        dateRangeChanged(myDateRange); 

    } else {
        Season season = seasons->seasons.at(cComboSeason->currentIndex());

        // Refresh aggregated curve (ride added/filter changed)
        cpPlot->setDateRange(season.getStart(), season.getEnd());

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
    if (context->athlete->allIntervalItems() == NULL) return; // not inited yet!

    // if the array hasn't been initialised properly then clean it up
    // this is because intervalsChanged gets called when selecting rides
    if (intervalCurves.count() != context->athlete->allIntervalItems()->childCount()) {
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
        if (myRideItem && myRideItem->ride() && myRideItem->ride()->intervals().count())
            for (int i=0; i< myRideItem->ride()->intervals().count(); i++)
                intervalCurves << NULL;
    }

    // which itervals are selected?
    IntervalItem *current=NULL;
    for (int i=0; i<context->athlete->allIntervalItems()->childCount(); i++) {
        current = static_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(i));
        if (current != NULL) {
            if (current->isSelected() == true) {
                showIntervalCurve(current, i); // set it all up
            } else {
                hideIntervalCurve(i); // in case its shown at present
            }
        }
    }
    cpPlot->replot();
}

// user hovered over an interval
void
CriticalPowerWindow::intervalHover(RideFileInterval x)
{
    // ignore in compare mode
    if (!amVisible() || context->isCompareIntervals) return;

    // do we need to fill with nulls ?
    if (intervalCurves.count() == 0 && myRideItem && myRideItem->ride() && myRideItem->ride()->intervals().count())
        for (int i=0; i< myRideItem->ride()->intervals().count(); i++)
            intervalCurves << NULL;

    // only one interval can be hovered at any one time
    // so we always use the same curve to ensure we don't leave
    // any nasty artefacts behind. And its always gray :)

    // first lets see what interval this actually is?
    IntervalItem *current=NULL;
    int index = -1;

    for (int i=0; i<context->athlete->allIntervalItems()->childCount(); i++) {
        current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(i));
        if (current != NULL) {
            // is this the one ?
            if (x.start == current->start && x.stop == current->stop) {
                index = i;
                break;
            }
        }
    }

    if (index >=0 && index < intervalCurves.count()) {

        // lazy for now just reuse existing
        if (intervalCurves[index] == NULL) {

            // get the data setup
            // but if there is no data for the ride series
            // selected they will still be null
            showIntervalCurve(current, index); // set it all up
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
        hoverCurve->setYAxis(QwtPlot::yLeft);
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
                       p->slope, p->temp, p->lrbalance, p->lte, p->rte, p->lps, p->rps, p->smo2, p->thb, 
                       p->rvert, p->rcad, p->rcontact, 0);
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
    int count=context->athlete->allIntervalItems()->childCount();
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
    intervalSelected(); // will refresh
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
    }

    if (!amVisible()) return;
    currentRide = myRideItem;

    if (currentRide) {

        if (context->athlete->zones()) {
            int zoneRange = context->athlete->zones()->whichRange(currentRide->dateTime.date());
            int CP = zoneRange >= 0 ? context->athlete->zones()->getCP(zoneRange) : 0;
            cpPlot->setDateCP(CP);
        } else {
            cpPlot->setDateCP(0);
        }
        if (context->athlete->paceZones()) {
            int paceZoneRange = context->athlete->paceZones()->whichRange(currentRide->dateTime.date());
            double CV = paceZoneRange >= 0.0 ? context->athlete->paceZones()->getCV(paceZoneRange) : 0.0;
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
            helperWidget()->move(mainWidget()->geometry().width()-275, 50);
        }

        // if off the screen move on screen
        if (helperWidget()->geometry().x() > geometry().width()) {
            helperWidget()->move(mainWidget()->geometry().width()-275, 50);
        }
    }
    return QWidget::event(event);
}

void
CriticalPowerWindow::setSeries(int index)
{
    if (index >= 0) {

        // need a helper any more ?
        CriticalSeriesType series = static_cast<CriticalSeriesType>(seriesCombo->itemData(index).toInt());
        if ((series == watts || series == wattsKg) && modelCombo->currentIndex() >= 1) helperWidget()->show();
        else helperWidget()->hide();

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
            for (int i=0; i<= context->athlete->allIntervalItems()->childCount(); i++) intervalCurves << NULL;

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
            unsigned min = 0, mid = 0, max = data->size();
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
        case NP: return QString(tr("Normalized Power"));
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
        case NP: return RideFile::NP;
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
               << NP
               << hr
               << kph
               << cad
               << nm
               << vam
               << aPower
               << kphd
               << wattsd
               << nmd
               << cadd
               << hrd
               << work
               << veloclinicplot;

    foreach (CriticalSeriesType x, seriesList) {
        seriesCombo->addItem(seriesName(x), static_cast<int>(x));
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
        cComboSeason->addItem(season.getName());
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
    if (dateRange.from != cfrom || dateRange.to != cto || stale) {

        cfrom = dateRange.from;
        cto = dateRange.to;

        // lets work out the average CP configure value
        if (context->athlete->zones()) {
            int fromZoneRange = context->athlete->zones()->whichRange(cfrom);
            int toZoneRange = context->athlete->zones()->whichRange(cto);

            int CPfrom = fromZoneRange >= 0 ? context->athlete->zones()->getCP(fromZoneRange) : 0;
            int CPto = toZoneRange >= 0 ? context->athlete->zones()->getCP(toZoneRange) : CPfrom;
            if (CPfrom == 0) CPfrom = CPto;
            int dateCP = (CPfrom + CPto) / 2;

            cpPlot->setDateCP(dateCP);
        }

        // lets work out the average CV configure value
        if (context->athlete->paceZones()) {
            int fromZoneRange = context->athlete->paceZones()->whichRange(cfrom);
            int toZoneRange = context->athlete->paceZones()->whichRange(cto);

            double CVfrom = fromZoneRange >= 0 ? context->athlete->paceZones()->getCV(fromZoneRange) : 0.0;
            double CVto = toZoneRange >= 0 ? context->athlete->paceZones()->getCV(toZoneRange) : CVfrom;
            if (CVfrom == 0.0) CVfrom = CVto;
            double dateCV = (CVfrom + CVto) / 2.0;

            cpPlot->setDateCV(dateCV);
        }

        cpPlot->setDateRange(dateRange.from, dateRange.to);
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

void CriticalPowerWindow::filterChanged()
{
    cpPlot->setRide(currentRide);
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
    cpPlot->setShowPercent(state);
    rPercent->setChecked(state);

    // redraw
    if (rangemode) dateRangeChanged(DateRange());
    else cpPlot->setRide(currentRide);
}

void 
CriticalPowerWindow::rPercentChanged(int check)
{
    showPercentCheck->setChecked(check);
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
CriticalPowerWindow::rHeatChanged(int check)
{
    showHeatCheck->setChecked(check);
}

void
CriticalPowerWindow::rDeltaChanged()
{
    cpPlot->setShowDelta(rDelta->isChecked(), rDeltaPercent->isChecked());

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
