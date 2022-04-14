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

#ifndef _GC_SimulatedRidersNestStats_h
#define _GC_SimulatedRidersNestStats_h
#include "GoldenCheetah.h"

#include <QWidget>
#include <QDialog>

#include <string>
#include <iostream>
#include <sstream>
#include <string>
#include "RideFile.h"
#include "IntervalItem.h"
#include "Context.h"
#include "TrainSidebar.h"
#include "BicycleSim.h"
#include <QDialog>
#include <QSslSocket>
#include <QGroupBox>

class QMouseEvent;
class RideItem;
class Context;
class QColor;
class QVBoxLayout;
class QTabWidget;
class LSimulatedRidersNestStats;
class IntervalSummaryWindow;
class SmallPlot;

struct displayGridItems {
    QString srName;
    int idx;  // original index before sort
    double srWatts;
    int srTime;  // Time diff from others TBD
    double srDistDiff;  // Distance diff from others in the grid
    QString srStatus;  // What is the SR doing? running, finished, attacking, following, pacing.
};

class SimulatedRidersNestStats : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    public:
        SimulatedRidersNestStats(Context *);
        void createNestStatsTableWidget();
        void createIntervalsAIsection();
        void createPrevRidesection();
        ~SimulatedRidersNestStats();
        SimulatedRidersNestStats();  // default ctor

        //Setters
        double& vrSpeedFor(int idx) { return this->validRoutes[idx].avgSpeed; };
        double& vrSecsFor(int idx) { return this->validRoutes[idx].totalSecs; };
        QString& vrfileFullPathFor(int idx) { return this->validRoutes[idx].fileFullPath; };
        QString& vrfileRouteTagFor(int idx) { return this->validRoutes[idx].fileRouteTag; };

        //Getters
        double vrAvgSpeedFor(int idx) const { return this->validRoutes[idx].avgSpeed; };
        double vrTotalSecsFor(int idx) const { return this->validRoutes[idx].totalSecs; };
        QString vrFileFullPathFor(int idx) const { return this->validRoutes[idx].fileFullPath; };
        QString vrFileRouteTagFor(int idx) const { return this->validRoutes[idx].fileRouteTag; };

    public slots:
        void configChanged(qint32);
        void ergFileSelected(ErgFile*);

    private:
        QWidget* nestStatsWidget;
        Context *context;
        QVBoxLayout *initialVBoxlayout;
        std::vector<validRouteInfo> validRoutes;    // Allow up to 4 additional simRiders
        QString attackStatus;
        std::vector<displayGridItems> displayGrid;
        QTableWidget* nestStatsTableWidget;
        QCheckBox *enableVPChkBox;
        QLineEdit *qleIntAITotalAttacks, *qleIntAIAttackDuration, *qleIntAIWarmUpDuration, *qleIntAICoolDownDuration, *qleIntAIAttackPowerIncrease, *qleIntAIMaxSeparation;
        QLabel *IntAITotalAttacksLabel, *IntAIAttackDurationLabel, *WarmUpDurationLabel, *CoolDownDurationLabel, *AttackPowerIncreaseLabel, *IntAIMaxSeparationLabel;
        QPushButton *rButton;
        QPushButton *applyButton, *selectERGFileButton, *selectRideFileButton;
        QString workoutERGFileName, gcRideFileName;
        QLabel *header;
        QLabel* position2Name, * position2Status, * position2Position, *position2Watts;
        QLabel *position1Name, *position1Status, *position1Position, *position1Watts;
        QLabel *position0Name, *position0Status, *position0Position, *position0Watts; // Current Athlete
        QRadioButton* radioIntervalsAI, * radioERGworkout, *radioPreviousRide;
        QLabel* fileNameLabel, *selectERGFileLabel, *selectRideFileLabel;
        QGroupBox *VPenginesGrpBox, *intervalsAIGrpBox, * selectPrevRideGrpBox;
        QGridLayout *settingsGridLayout;
        double prevRouteDistance, prevVP1Distance;
        QLabel* routeNameLbl, *routeDistanceLbl, *routeAvgSpeedLbl;
        QWidget *settingsWidget;
        SimRiderStateData localSrData;
        bool srConfigChanged; 

    private slots:
        void telemetryUpdate(RealtimeData rtd);
        void start();
        void saveEnginesOptions();
        void stop();
        void pause();
        void setDisplayData(RealtimeData rtd, int engineType);
        void showDisplayGrid();
        void intervalsAIClicked();
        void previousRideClicked();
        void SimRiderStateUpdate(SimRiderStateData srData);

    protected:
 
};
#endif
