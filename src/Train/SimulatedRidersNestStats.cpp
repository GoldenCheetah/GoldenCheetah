/*
 * Copyright (c) 2020 Peter Kanatselis (pkanatselis@gmail.com)
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

#include "SimulatedRidersNestStats.h"
#include "BicycleSim.h"

#include "MainWindow.h"
#include "RideItem.h"
#include "RideFile.h"
#include "RideImportWizard.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "SmallPlot.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "TimeUtils.h"
#include "HelpWhatsThis.h"
#include "Library.h"
#include "ErgFile.h"

 // overlay helper
#include "GcOverlayWidget.h"
#include "IntervalSummaryWindow.h"
#include <QDebug>
#include <QFileDialog>


// declared in main, we only want to use it to get QStyle
extern QApplication* application;

SimulatedRidersNestStats::SimulatedRidersNestStats(Context* context) : GcChartWindow(context), context(context)
{
    prevRouteDistance = -1.0;
    prevVP1Distance = -1.0;
    srConfigChanged = true;

    // Create chart settings widget
    settingsWidget = new QWidget(this);
    settingsWidget->setContentsMargins(0, 0, 0, 0);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    // Create the grid layout for all the settings on the page
    settingsGridLayout = new QGridLayout(settingsWidget);

    // CheckBox to globaly enable Virtual Partner and add it to the grid
    enableVPChkBox = new QCheckBox("Enable Virtual Partner");
    QString sValue = "";
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ISENABLED, "").toString();
    if (sValue.isEmpty() || sValue == "0") {
        enableVPChkBox->setCheckState(Qt::Unchecked);
    }
    else {
        enableVPChkBox->setCheckState(Qt::Checked);
    }
    settingsGridLayout->addWidget(enableVPChkBox, 0, 0, 1, 2, Qt::AlignLeft);

    // Create a group box for all the engines and radio buttons for each engine
    VPenginesGrpBox = new QGroupBox(tr("Simulated Rider Engines:"));
    radioIntervalsAI = new QRadioButton(tr("&Intervals AI"));
    // Disable AI otion for now. Need to work on the state machine code
    radioIntervalsAI->setEnabled(false);
    radioPreviousRide = new QRadioButton(tr("&Previous Ride"));

    // Group all radio buttons in the VBox
    QVBoxLayout* VPEnginesVBox = new QVBoxLayout(settingsWidget);
    VPEnginesVBox->addWidget(radioIntervalsAI);
    VPEnginesVBox->addWidget(radioPreviousRide);
    //Add VBox to GroupBox
    VPenginesGrpBox->setLayout(VPEnginesVBox);
    //Add group box to the settings grid
    settingsGridLayout->addWidget(VPenginesGrpBox, 1, 0, 1, 2, Qt::AlignLeft);

    // Connect radio buton click signals
    connect(radioIntervalsAI, SIGNAL(clicked(bool)), this, SLOT(intervalsAIClicked()));
    connect(radioPreviousRide, SIGNAL(clicked(bool)), this, SLOT(previousRideClicked()));
    createPrevRidesection();
    createIntervalsAIsection();

    // Add Save settings button at the bottom of the page
    rButton = new QPushButton(application->style()->standardIcon(QStyle::SP_ArrowRight), "Save settings", this);
    settingsGridLayout->addWidget(rButton, 10, 0, 1, 3, Qt::AlignHCenter);
    connect(rButton, SIGNAL(clicked(bool)), this, SLOT(saveEnginesOptions()));

    // Read SimRider engine type from file and set the form values
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_ENGINETYPE, "").toString();
    if (sValue == "" || sValue == "0") {
        radioIntervalsAI->setChecked(true);
        intervalsAIClicked();
    }
    else if (sValue == "1") {
        radioPreviousRide->setChecked(true);
        previousRideClicked();
    }

    //Set widget as configuration settings
    setControls(settingsWidget);
    setContentsMargins(0, 0, 0, 0);

    //Create a VBox layout to show both widgets (settings and stats) when adding the chart
    initialVBoxlayout = new QVBoxLayout();
    initialVBoxlayout->setSpacing(0);
    initialVBoxlayout->setContentsMargins(2, 0, 2, 2);
    setChartLayout(initialVBoxlayout);

    // Conent signal to recieve updates build the chart.
    connect(context, SIGNAL(telemetryUpdate(RealtimeData)), this, SLOT(telemetryUpdate(RealtimeData)));
    connect(context, SIGNAL(stop()), this, SLOT(stop()));
    connect(context, SIGNAL(start()), this, SLOT(start()));
    connect(context, SIGNAL(pause()), this, SLOT(pause()));
    connect(context, SIGNAL(SimRiderStateUpdate(SimRiderStateData)), this, SLOT(SimRiderStateUpdate(SimRiderStateData)));
    // User picked a new workout signal
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(ergFileSelected(ErgFile*)));

    // New widget to show rider statistics
    nestStatsWidget = new QWidget(this);
    nestStatsWidget->setContentsMargins(0, 0, 0, 0);
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));  //Set chart menu text color
    nestStatsWidget->setWindowTitle("Simulated Riders Nest Stats");

    header = new QLabel("Virtual Partner Simulation");
    header->setStyleSheet("QLabel { color : white; }");
    header->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);

    // Create Grid Layout to show statistics
    QGridLayout* nestGridLayout = new QGridLayout(nestStatsWidget);
    nestGridLayout->addWidget(header, 0, 0, 1, 4, Qt::AlignHCenter);

    // Add stats widget to initial VBox Layout
    initialVBoxlayout->addWidget(nestStatsWidget);

    createNestStatsTableWidget();
    configChanged(CONFIG_APPEARANCE);
    ergFileSelected(context->currentErgFile());
}

void SimulatedRidersNestStats::createNestStatsTableWidget() {

    nestStatsTableWidget = new QTableWidget(this);
    if (localSrData.srCount() == 1) header->setText("No valid routes found!");
    nestStatsTableWidget->setColumnCount(4);
    //Insert labels into the horizontal header
    QStringList headerForTableWidget = { "Athlete Name", "Distance", "Status", "Power" };
    nestStatsTableWidget->setHorizontalHeaderLabels(headerForTableWidget);
    nestStatsTableWidget->setSelectionMode(QAbstractItemView::NoSelection);
    // Header style sheet
    QString styleSheetHeader = "::section {" // "QHeaderView::section {"
        "spacing: 0px;"
        "color: blue;"
        //"background-color: white;"
        "border: 0px;"
        "font-weight: bold;"
        "border-color: black;"
        "margin: 0px;"
        "text-align: center;"
        "font-family: arial;"
        "font-size: 18px; }";
    nestStatsTableWidget->horizontalHeader()->setStyleSheet(styleSheetHeader);
    // Disable editing
    nestStatsTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    nestStatsTableWidget->verticalHeader()->setVisible(false);
    nestStatsTableWidget->horizontalHeader()->setVisible(true);
    nestStatsTableWidget->setShowGrid(true);

    initialVBoxlayout->addWidget(nestStatsTableWidget);
    configChanged(CONFIG_APPEARANCE);
}


void SimulatedRidersNestStats::createIntervalsAIsection() {

    // Create a GroupBox for Intervals AI options
    intervalsAIGrpBox = new QGroupBox(tr("Simulated Rider Intervals AI Engine options:"));
    // Create a vbox layout for Intervals AI Engine options
    QVBoxLayout* intervalsAIOptionsVBox = new QVBoxLayout(settingsWidget);
    // Create grid layout for the engine options inside the settings grid
    QGridLayout* IntervalAIOptionsGridLayout = new QGridLayout(settingsWidget);
    // Add labels and values to the engine options grid
    QString sValue = "";
    IntAITotalAttacksLabel = new QLabel(tr("Total attacks during the ride by the AI: <br><font color=red><i>Zero disables Virtual Partner</i></font>"));
    qleIntAITotalAttacks = new QLineEdit(this);
    qleIntAITotalAttacks->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_TOTALATTACKS, "").toString();
    if (sValue.isEmpty()) qleIntAITotalAttacks->setText("8");
    else qleIntAITotalAttacks->setText(sValue);
    // Add label 1 to the options grid
    IntervalAIOptionsGridLayout->addWidget(IntAITotalAttacksLabel, 0, 0, Qt::AlignLeft);
    // Add value 1 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAITotalAttacks, 0, 1, Qt::AlignLeft);

    sValue = "";
    WarmUpDurationLabel = new QLabel(tr("Warm Up %:"));
    qleIntAIWarmUpDuration = new QLineEdit(this);
    qleIntAIWarmUpDuration->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_WARMUPSTARTSPERCENT, "").toString();
    if (sValue.isEmpty()) qleIntAIWarmUpDuration->setText("5"); //meters, need to convert between meters and feet
    else qleIntAIWarmUpDuration->setText(sValue);
    // Add label 2 to the options grid
    IntervalAIOptionsGridLayout->addWidget(WarmUpDurationLabel, 1, 0, Qt::AlignLeft);
    // Add value 2 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAIWarmUpDuration, 1, 1, Qt::AlignLeft);

    sValue = "";
    CoolDownDurationLabel = new QLabel(tr("Cool down %:"));
    qleIntAICoolDownDuration = new QLineEdit(this);
    qleIntAICoolDownDuration->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_COOLDOWNSTARTSPERCENT, "").toString();
    if (sValue.isEmpty()) qleIntAICoolDownDuration->setText("5"); //meters, need to convert between meters and feet
    else qleIntAICoolDownDuration->setText(sValue);
    // Add label 3 to the options grid
    IntervalAIOptionsGridLayout->addWidget(CoolDownDurationLabel, 2, 0, Qt::AlignLeft);
    // Add value 3 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAICoolDownDuration, 2, 1, Qt::AlignLeft);

    sValue = "";
    AttackPowerIncreaseLabel = new QLabel(tr("AI power increase during attack (WATTS):"));
    qleIntAIAttackPowerIncrease = new QLineEdit(this);
    qleIntAIAttackPowerIncrease->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_ATTACKPOWERINCREASE, "").toString();
    if (sValue.isEmpty()) qleIntAIAttackPowerIncrease->setText("55"); //meters, need to convert between meters and feet
    else qleIntAIAttackPowerIncrease->setText(sValue);
    // Add label 4 to the options grid
    IntervalAIOptionsGridLayout->addWidget(AttackPowerIncreaseLabel, 3, 0, Qt::AlignLeft);
    // Add value 4 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAIAttackPowerIncrease, 3, 1, Qt::AlignLeft);

    QString feetMetersLabel = ((GlobalContext::context()->useMetricUnits) ? "AI attack duration (meters):" : "AI attack duration (feet):");
    sValue = "";
    IntAIAttackDurationLabel = new QLabel(feetMetersLabel);
    qleIntAIAttackDuration = new QLineEdit(this);
    qleIntAIAttackDuration->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_ATTACKDURATION, "").toString();
    // We always write meters so we need to convert between meters and feet
    QVariant feetMeterValue = ((GlobalContext::context()->useMetricUnits) ? sValue.toDouble() : (sValue.toDouble() * FEET_PER_METER));
    if (sValue.isEmpty()) qleIntAIAttackDuration->setText("100");
    else qleIntAIAttackDuration->setText(QString::number(feetMeterValue.toInt()));
    // Add label 5 to the options grid
    IntervalAIOptionsGridLayout->addWidget(IntAIAttackDurationLabel, 4, 0, Qt::AlignLeft);
    // Add value 5 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAIAttackDuration, 4, 1, Qt::AlignLeft);

    QString feetMetersMaxSepLabel = ((GlobalContext::context()->useMetricUnits) ? "Maximum separation from Virtual Partner (meters):" : "Maximum separation from Virtual Partner (feet):");
    feetMetersMaxSepLabel += " <br><font color=red><i>If set to zero, Virtual Partner will not wait.</i></font>";
    sValue = "";
    IntAIMaxSeparationLabel = new QLabel(feetMetersMaxSepLabel);
    qleIntAIMaxSeparation = new QLineEdit(this);
    qleIntAIMaxSeparation->setFixedWidth(300);
    sValue = appsettings->cvalue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_MAXSEPARATION, "").toString();
    // We always write meters so we need to convert between meters and feet
    feetMeterValue = ((GlobalContext::context()->useMetricUnits) ? sValue.toDouble() : (sValue.toDouble() * FEET_PER_METER));
    if (sValue.isEmpty()) qleIntAIMaxSeparation->setText("50");
    else qleIntAIMaxSeparation->setText(QString::number(feetMeterValue.toInt()));
    // Add label 5 to the options grid
    IntervalAIOptionsGridLayout->addWidget(IntAIMaxSeparationLabel, 5, 0, Qt::AlignLeft);
    //Add value 5 to options grid
    IntervalAIOptionsGridLayout->addWidget(qleIntAIMaxSeparation, 5, 1, Qt::AlignLeft);

    // Add Interval AI options groupbox  to the VBox
    intervalsAIOptionsVBox->addWidget(intervalsAIGrpBox);
    // Add Interval AI Options grid  layout to the group box
    intervalsAIGrpBox->setLayout(IntervalAIOptionsGridLayout);
    // Add Intervals AI options groupbox to the settings grid
    settingsGridLayout->addWidget(intervalsAIGrpBox, 2, 0, 1, 3, Qt::AlignLeft);
}

void SimulatedRidersNestStats::createPrevRidesection() {
    QString line1, line2;
    // Create a group box for the previous ride options
    selectPrevRideGrpBox = new QGroupBox(tr(" Ride against previous rides "));

    // Create grid layout 
    QGridLayout* prevRideOptionsGridLayout = new QGridLayout(settingsWidget);

    // Previous ride stats labels
    line1 = QString(
        "This feature is based on the existence of the route tag in the activities folder of the athlete. "
        "The tag value (route name) will be read from the selected workout and used to find matching activities. "
        "If any routes are found, they will be sorted by time so the fastest one is first. "
        "If more than 4 are found, the top 4 will be selected and displayed in the chart. "
        "It is recommended that you zoom in the live map to see all markers. "
        "If 4 or less are found, they will all displayed in the chart. "
        "If no files are found with the specific route tag, only the current user will be displayed.\n");
    QLabel* l1 = new QLabel(line1);
    l1->setWordWrap(true);
    prevRideOptionsGridLayout->addWidget(l1, 2, 0, 1, 1, Qt::AlignLeft);
    line2 = QString(
        "The following steps are required in order for the feature to function as designed:\n\n"
        "1. Simulate Speed from Power must be enabled in the preferences section in options-> training-> Preferences\n"
        "2. Previous actitivies must have power (Watts) data. If they don't, the feature will be disabled\n"
        "3. Review existing activities and add the same route name to all activities created using the same workout.\n"
        "4. Export one of the tagged activities as a GC json file in the athlete workouts folder.\n"
        "5. Add the workouts folder in the search path for workouts, video files and videosync files.\n"
        "\n\n ===> Save settings after changing configurations to apply the changes <===\n");
    QLabel* l2 = new QLabel(line2);
    l1->setWordWrap(true);
    prevRideOptionsGridLayout->addWidget(l2, 3, 0, 1, 1, Qt::AlignLeft);

    // Add grid to the GroupBox
    selectPrevRideGrpBox->setLayout(prevRideOptionsGridLayout);

    // Add group box to the settings grid
    settingsGridLayout->addWidget(selectPrevRideGrpBox, 1, 2, 1, 3, Qt::AlignLeft);
}

SimulatedRidersNestStats::~SimulatedRidersNestStats()
{

}

void SimulatedRidersNestStats::ergFileSelected(ErgFile* f) {
    srConfigChanged = true;
}

void SimulatedRidersNestStats::intervalsAIClicked() {
    intervalsAIGrpBox->show();
    selectPrevRideGrpBox->hide();
}

void SimulatedRidersNestStats::previousRideClicked() {
    intervalsAIGrpBox->hide();
    selectPrevRideGrpBox->show();

}

void SimulatedRidersNestStats::start() {
    if (!this->context->workout) {
        qDebug() << "Error: Route not selected.";
        return;
    }
    displayGrid.resize(0);
}

void SimulatedRidersNestStats::saveEnginesOptions() {
    QString isVPEnabled = "0";
    if (enableVPChkBox->checkState() == Qt::Checked) {
        isVPEnabled = "1";
    }
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_ISENABLED, isVPEnabled);

    if (radioIntervalsAI->isChecked()) {
        appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_ENGINETYPE, "0");
    }
    else if (radioPreviousRide->isChecked()) {
        appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_ENGINETYPE, "1");
    }

    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_TOTALATTACKS, qleIntAITotalAttacks->text());
    //Always save in meters
    QString sValue = qleIntAIAttackDuration->text();
    QVariant val = ((GlobalContext::context()->useMetricUnits) ? sValue.toDouble() : (sValue.toDouble() / FEET_PER_METER));
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_ATTACKDURATION, QString::number(val.toInt()));
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_WARMUPSTARTSPERCENT, qleIntAIWarmUpDuration->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_COOLDOWNSTARTSPERCENT, qleIntAICoolDownDuration->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_ATTACKPOWERINCREASE, qleIntAIAttackPowerIncrease->text());
    //Always save in meters
    sValue = qleIntAIMaxSeparation->text();
    val = ((GlobalContext::context()->useMetricUnits) ? sValue.toDouble() : (sValue.toDouble() / FEET_PER_METER));
    appsettings->setCValue(context->athlete->cyclist, GC_SIMRIDER_INTERVALS_AI_MAXSEPARATION, val.toInt());

    QMessageBox::information(0, QString("Settings Saved"),
        QString("Settings saved succesfully!\n"),
        QMessageBox::Ok);
}

void SimulatedRidersNestStats::stop()
{
    if (localSrData.srFeatureEnabled() && localSrData.srEngineinitialized()) {
        for (int i = 0; i < localSrData.srCount(); i++) {
            nestStatsTableWidget->setItem(i, 2, new QTableWidgetItem("Stopped"));
        }

    }
    srConfigChanged = true;
}

void SimulatedRidersNestStats::pause()
{
}

void SimulatedRidersNestStats::configChanged(qint32)
{
    // tinted palette for headings etc
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);
}

void SimulatedRidersNestStats::telemetryUpdate(RealtimeData rtd)
{
    if (context->isRunning && !context->isPaused && localSrData.srFeatureEnabled())
    {
        if (localSrData.srEngineType() >= 0 && localSrData.srEngineType() < 3) {
            setDisplayData(rtd, localSrData.srEngineType());
        }
        else {
            header->setText("Unknown Engine type!");
        }
    }

    if (context->isRunning && context->isPaused && localSrData.srFeatureEnabled()) {
        header->setText("Virtual Partner feature is paused!");
    }

    if (context->isRunning && !context->isPaused && !localSrData.srFeatureEnabled()) {
        header->setText("Virtual Partner feature is disabled!");
    }
}

void SimulatedRidersNestStats::SimRiderStateUpdate(SimRiderStateData srData) {
    localSrData = srData;
}

void SimulatedRidersNestStats::setDisplayData(RealtimeData rtd, int engineType) {

    if (engineType == 0) {
        int displayAttackCount = localSrData.srAttackCount() + 1;

        if (localSrData.srEngineinitialized()) {
            // Display next attack distance
            if (displayAttackCount == 0) {
                header->setText("No more attacks!");
            }
            else if (!localSrData.srIsAttacking() && displayAttackCount > 0) {
                double attackDistance = localSrData.srNextAttack() - (rtd.getRouteDistance() * 1000); //Using route distance to support skipping
                header->setText("Next attack # " + QString::number(displayAttackCount, 'f', 0) + " in " + QString::number(attackDistance, 'f', 0) +
                    ((GlobalContext::context()->useMetricUnits) ? tr(" m") : tr(" ft")));
            }
            else { // SimRider is attacking
                header->setText("Attack # " + QString::number(displayAttackCount - 1, 'f', 0) + " in progress");
            }
        }
        else {
            header->setText("Engine NOT initialized!");
        }
    }
    else {
        if (engineType == 1) {
            if (localSrData.srCount() == 1) header->setText("No valid routes found!");
            else header->setText("Following previous route: " + localSrData.srRouteName());
        }
    }
    // Set vector size the first time
    if (displayGrid.size() == 0) displayGrid.resize(localSrData.srCount());
    for (int i = 0; i < localSrData.srCount(); i++) {
        if (i == 0) displayGrid[i].srName = context->athlete->cyclist;
        else displayGrid[i].srName = "SimRider " + QString::number(i, 'f', 0);
        displayGrid[i].idx = i;
        if (i > 0) {
            displayGrid[i].srDistDiff = localSrData.srDistanceFor(0) - localSrData.srDistanceFor(i);
        }
        else {
            displayGrid[i].srDistDiff = 0;
        }
        if (!localSrData.srIsWorkoutFinishedFor(i)) {
            //displayGrid[i].srStatus = "Running";
            displayGrid[i].srStatus = (i > 0) ? localSrData.srDisplayStatusFor(i) : " ";
            displayGrid[i].srWatts = localSrData.srWattsFor(i);
            displayGrid[i].srTime = 0; // // Display time diff from previous ride? 
        }
        else {
            displayGrid[i].srStatus = "Finished";
            displayGrid[i].srWatts = 0.;
            displayGrid[i].srTime = 0; // Display time diff from previous ride?
        }
    }
    //sort array by distance difference. Negative numbers are ahead
    std::sort(displayGrid.begin(), displayGrid.end(), [](const displayGridItems& a, const displayGridItems& b) { return a.srDistDiff < b.srDistDiff; });
    showDisplayGrid();
}

void SimulatedRidersNestStats::showDisplayGrid() {
    try {
        if (srConfigChanged) {
            if (nestStatsTableWidget) {
                nestStatsTableWidget->setRowCount(0);
            }
            nestStatsTableWidget->setRowCount(localSrData.srCount());
            //Set table body style
            QString styleSheetRows = "::section {" // "QTableView::section {"
                "spacing: 0px;"
                "color: black;"
                //"background-color: white;"
                "border: 0px;"
                "border-color: black;"
                "margin: 0px;"
                "text-align: center;"
                "font-family: arial;"
                "font-size: 14px; }";
            nestStatsTableWidget->setStyleSheet(styleSheetRows);
            setControls(nestStatsTableWidget);
            setContentsMargins(0, 0, 0, 0);
            // Build initial values
            for (int i = 0; i < localSrData.srCount(); i++) {
                QString sDist = "";
                nestStatsTableWidget->setItem(i, 0, new QTableWidgetItem(displayGrid[i].srName));
                if (displayGrid[i].idx > 0) {
                    double d = ((GlobalContext::context()->useMetricUnits) ? displayGrid[i].srDistDiff * 1000 : (displayGrid[i].srDistDiff * 1000 * FEET_PER_METER));
                    sDist = QString::number(d, 'f', 0) + ((GlobalContext::context()->useMetricUnits) ? tr(" m") : tr(" ft"));
                }
                nestStatsTableWidget->setItem(i, 1, new QTableWidgetItem(sDist));
                nestStatsTableWidget->setItem(i, 2, new QTableWidgetItem(displayGrid[i].srStatus));
                nestStatsTableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(displayGrid[i].srWatts, 'f', 0)));
            }
            initialVBoxlayout->addWidget(nestStatsTableWidget);
            configChanged(CONFIG_APPEARANCE);
            srConfigChanged = false;
        }
        else { // Update existing table widget
            for (int i = 0; i < localSrData.srCount(); i++) {
                QString sDist = "";
                QColor bgColor;
                bgColor.setRgb(0, 0, 255, 255);

                if (displayGrid[i].idx > 0) {
                    double d = ((GlobalContext::context()->useMetricUnits) ? displayGrid[i].srDistDiff * 1000 : (displayGrid[i].srDistDiff * 1000 * FEET_PER_METER));
                    sDist = QString::number(d, 'f', 0) + ((GlobalContext::context()->useMetricUnits) ? tr(" m") : tr(" ft"));
                    bgColor.setRgb(255, 255, 255, 255);
                }
                nestStatsTableWidget->item(i, 0)->setText(displayGrid[i].srName);
                nestStatsTableWidget->item(i, 0)->setBackgroundColor(bgColor);
                nestStatsTableWidget->item(i, 1)->setText(sDist);
                nestStatsTableWidget->item(i, 1)->setBackgroundColor(bgColor);
                nestStatsTableWidget->item(i, 2)->setText(displayGrid[i].srStatus);
                nestStatsTableWidget->item(i, 2)->setBackgroundColor(bgColor);
                nestStatsTableWidget->item(i, 3)->setText(QString::number(displayGrid[i].srWatts, 'f', 0));
                nestStatsTableWidget->item(i, 3)->setBackgroundColor(bgColor);
            }
        }
    }
    catch (const char* message) {
        qDebug() << __FUNCTION__ << "ERROR ===> " << message;
    }


}