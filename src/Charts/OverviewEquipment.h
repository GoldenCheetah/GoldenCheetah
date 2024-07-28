/*
 * Copyright (c) 2024 Paul Johnson (paulj49457@gmail.com)
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

#ifndef _GC_OverviewEquipmentWindow_h
#define _GC_OverviewEquipmentWindow_h 1

#include "Overview.h"
#include "ChartSpace.h"

class EquipCalculator;

class EquipCalculationThread : public QThread
{
public:
    EquipCalculationThread(EquipCalculator* eqCalc) : eqCalc_(eqCalc) {}

protected:

    // recalculate distances
    virtual void run() override;

private:
    EquipCalculator* eqCalc_;
};

class EquipCalculator
{
    public:
        EquipCalculator(const MainWindow* mainWindow);
        virtual ~EquipCalculator();

        void recalculateEquipTile(const QString& eqLinkName, ChartSpaceItem* item);
        void recalculateEquipSpace(const QString& eqLinkName, QList<ChartSpaceItem*> items);

    protected:

        friend class ::EquipCalculationThread;

        void recalculateTiles(const QString& eqLinkName);

        // distance calculation
        void recalculateEq(RideItem* rideItem);
        RideItem* nextRideToCheck();
        void threadCompleted(EquipCalculationThread* thread);

        // Equipment distance recalculation
        QMutex updateMutex_;
        QVector<EquipCalculationThread*> recalculationThreads_;
        QVector<RideItem*>  rideItemList_;
        QDate eqLinkEarliestDate_, eqLinkLatestDate_;
        std::atomic<uint64_t> eqLinkTotalTimeInSecs_;
        std::atomic<uint64_t> eqLinkTotalDistanceScaled_;
        std::atomic<uint64_t> eqLinkTotalElevationScaled_;
        std::atomic<uint64_t> eqLinkNumActivities_;

        QString eqLinkName_;
        QList<ChartSpaceItem*> spaceItems_;
        const MainWindow* mainWindow_;

};

class OverviewEquipmentWindow : public OverviewWindow
{
    Q_OBJECT

    public:

        OverviewEquipmentWindow(Context* context, int scope = OverviewScope::EQUIPMENT, bool blank = false);
        virtual ~OverviewEquipmentWindow();

    public slots:

        virtual ChartSpaceItem* addTile() override;
        virtual void configItem(ChartSpaceItem*) override;

        void showEvent(QShowEvent*);
        void hideEvent(QHideEvent*);

        // athlete opening
        void openingAthlete(QString, Context*);
        void loadDone(QString, Context*);

        void configChanged(qint32 cfg);

    private:
        bool reCalcOnVisible;
        EquipCalculator* eqCalc;

};

#endif // _GC_OverviewEquipmentWindow_h
