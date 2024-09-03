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

#include "ChartSpace.h"
#include "AthleteTab.h"
#include "OverviewEquipment.h"
#include "OverviewEquipmentItems.h"
#include "EquipmentCalculator.h"

EquipCalculator::EquipCalculator(const MainWindow* mainWindow, OverviewEquipmentWindow* eqOverviewWindow) :
    mainWindow_(mainWindow), eqOverviewWindow_(eqOverviewWindow), eqCalculationInProgress_(false)
{
}

EquipCalculator::~EquipCalculator()
{
}

void
EquipCalculator::recalculateEquipTile(ChartSpaceItem* item)
{
    // already recalcuating !
    if (eqCalculationInProgress_) return;

    eqCalculationInProgress_ = true;
    spaceItems_.clear();
    spaceItems_.append(item);
    recalculateTiles();
}

void
EquipCalculator::recalculateEquipSpace(QList<ChartSpaceItem*> items)
{
    // already recalcuating !
    if (eqCalculationInProgress_) return;

    eqCalculationInProgress_ = true;
    spaceItems_ = items;
    recalculateTiles();
}

void
EquipCalculator::recalculateTiles()
{
    // take a copy of the rides through the athlete's rides creating a ride List to process
    for (auto it = mainWindow_->athletetabs.keyValueBegin(); it != mainWindow_->athletetabs.keyValueEnd(); ++it) {
        rideItemList_ += it->second->context->athlete->rideCache->rides();
    }

    // Reset all the tiles distances
    for (ChartSpaceItem* item : spaceItems_) {

        static_cast<CommonEquipmentItem*>(item)->resetEqItem();
    }

    // calculate number of threads and work per thread
    int maxthreads = QThreadPool::globalInstance()->maxThreadCount();
    int threads = maxthreads / 3; // Don't need many threads
    if (threads == 0) threads = 1; // but need at least one!

    // keep launching the threads
    while (threads--) {

        EquipCalculationThread* thread = new EquipCalculationThread(this);
        recalculationThreads_ << thread;
        thread->start();
    }
}

RideItem*
EquipCalculator::nextRideToCheck()
{
    updateMutex_.lock();
    RideItem*  item = rideItemList_.isEmpty() ? nullptr : rideItemList_.takeLast();
    updateMutex_.unlock();

    return(item);
}

void
EquipCalculationThread::run() {

    RideItem* item = eqCalc_->nextRideToCheck();
    while (item != nullptr) {
        eqCalc_->recalculateEq(item);
        item = eqCalc_->nextRideToCheck();
    }
    eqCalc_->threadCompleted(this);
    return;
}

void
EquipCalculator::threadCompleted(EquipCalculationThread* thread)
{
    updateMutex_.lock();
    recalculationThreads_.removeOne(thread);
    updateMutex_.unlock();

    // if the final thread is finished, then update the summary items.
    if (recalculationThreads_.count() == 0) {

        eqOverviewWindow_->calculationComplete();
        eqCalculationInProgress_ = false;
    }
}

void
EquipCalculator::recalculateEq(RideItem* rideItem)
{
    // get the date of the activity 
    QDate actDate(QDate(1900, 01, 01).addDays(rideItem->getText("Start Date", "0").toInt()));

    // using integral type atomics (c++11) but to retain accuracy multiply by EQ_REAL_TO_SCALED, see overviewItems.h
    double rideDistance = rideItem->getForSymbol("total_distance", GlobalContext::context()->useMetricUnits);
    uint64_t rideDistanceScaled = static_cast<uint64_t>(round(rideDistance*EQ_REAL_TO_SCALED));

    double rideElevation = rideItem->getForSymbol("elevation_gain", GlobalContext::context()->useMetricUnits);
    uint64_t eqElevationScaled = static_cast<uint64_t>(round(rideElevation*EQ_REAL_TO_SCALED));

    uint64_t rideTimeInSecs = static_cast<uint64_t>(rideItem->getForSymbol("time_riding"));

    QStringList rideEqLinkNameList = rideItem->getText("EquipmentLink", "abcde").simplified().remove(' ').split(",");

    for (ChartSpaceItem* item : spaceItems_) {

        if (item->type == OverviewItemType::EQ_ITEM)
        {
            if (static_cast<EquipmentItem*>(item)->isWithin(rideEqLinkNameList, actDate)) {

                static_cast<EquipmentItem*>(item)->addActivity(rideDistanceScaled, eqElevationScaled, rideTimeInSecs);
            }
        } else if (item->type == OverviewItemType::EQ_SUMMARY)
        {
            if ((static_cast<EquipmentSummary*>(item)->eqLinkName_ == "") ||
                (rideEqLinkNameList.contains(static_cast<EquipmentSummary*>(item)->eqLinkName_))) {

                static_cast<EquipmentSummary*>(item)->addActivity(rideItem->context->athlete->cyclist,
                                                                    actDate, rideDistanceScaled,
                                                                    eqElevationScaled, rideTimeInSecs);
            }
        }
    }
}
