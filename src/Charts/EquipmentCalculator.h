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

#ifndef _GC_EquipmentCalculator_h
#define _GC_EquipmentCalculator_h 1

class EquipCalculator;

class EquipCalculationThread : public QThread
{
public:
    EquipCalculationThread(EquipCalculator* equipCalculator) : eqCalc_(equipCalculator) {}

protected:

    // recalculate distances
    virtual void run() override;

private:
    EquipCalculator* eqCalc_;
};

class OverviewEquipmentWindow;

class EquipCalculator
{
    public:
        EquipCalculator(const MainWindow* mainWindow, OverviewEquipmentWindow* eqOverviewWindow);
        virtual ~EquipCalculator();

        void recalculateEquipTile(ChartSpaceItem* item);
        void recalculateEquipSpace(QList<ChartSpaceItem*> items);

    protected:

        friend class ::EquipCalculationThread;

        void recalculateTiles();

        // distance calculation
        void recalculateEq(RideItem* rideItem);
        RideItem* nextRideToCheck();
        void threadCompleted(EquipCalculationThread* thread);
        void addToSummary(const QDate& actDate, uint64_t rideDistanceScaled,
                           uint64_t eqElevationScaled, uint64_t rideTimeInSecs);

        // Equipment distance recalculation
        QMutex updateMutex_;
        QVector<EquipCalculationThread*> recalculationThreads_;
        QVector<RideItem*>  rideItemList_;

        std::atomic<bool> eqCalculationInProgress_;

        QList<ChartSpaceItem*> spaceItems_;
        const MainWindow* mainWindow_;
        OverviewEquipmentWindow* eqOverviewWindow_;
};

#endif // _GC_EquipmentCalculator_h
