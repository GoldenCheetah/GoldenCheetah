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

#include "OverviewEquipment.h"
#include "ChartSpace.h"
#include "OverviewItems.h"
#include "AthleteTab.h"

OverviewEquipmentWindow::OverviewEquipmentWindow(Context* context, int scope, bool blank) :
    OverviewWindow(context, scope, blank), reCalcOnVisible(false)
{
    eqCalc = new EquipCalculator(context->mainWindow);

    // cannot use athlete specific signals, as there is only one equipment view.
    connect(GlobalContext::context(), SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context->mainWindow, SIGNAL(openingAthlete(QString, Context*)), this, SLOT(openingAthlete(QString, Context*)));
}

OverviewEquipmentWindow::~OverviewEquipmentWindow()
{
    delete eqCalc;
}

ChartSpaceItem*
OverviewEquipmentWindow::addTile()
{
    ChartSpaceItem* item = OverviewWindow::addTile();

    if ((item != nullptr) && reCalcOnVisible) eqCalc->recalculateEquipTile(title(), item);
    return item;
}

void
OverviewEquipmentWindow::configItem(ChartSpaceItem* item)
{
    OverviewWindow::configItem(item);

    // Called for both item updates and deletion, if the item is still in the chart space list then
    // its an update, otherwise it's a tile deletion which requires no recaculation.
    if (space->allItems().indexOf(item) != -1) {

        // recalculate the affected tile
        if (reCalcOnVisible) eqCalc->recalculateEquipTile(title(), item);
    }
}

void
OverviewEquipmentWindow::configChanged(qint32 cfg) {

    // Update in case the metric/imperial units have changed
    if (cfg & CONFIG_UNITS) {

        // need to update all the nonGCDistances in all the tiles before recalc
        for (ChartSpaceItem* item : space->allItems()) {

            if (item->type == OverviewItemType::EQ_ITEM)
            {
                static_cast<EquipmentItem*>(item)->unitsChanged();
            }
        }

        if (reCalcOnVisible) eqCalc->recalculateEquipSpace(title(), space->allItems());
    }
}

void
OverviewEquipmentWindow::openingAthlete(QString, Context* context)
{
    if (reCalcOnVisible) {

        // If a new athlete is opened whilst the equipment window is displayed then this openingAthlete event
        // for the new athelete is too early, so need to wait for their activities to be loaded so register
        // the new athlete for loadDone temporarily and update then (if necessary)
        connect(context, SIGNAL(loadDone(QString, Context*)), this, SLOT(loadDone(QString, Context*)));
    }
}

void
OverviewEquipmentWindow::loadDone(QString, Context* context)
{
    // de-register the athlete's load event, and recalculate if currently visible.
    disconnect(context, SIGNAL(loadDone(QString, Context*)), this, SLOT(loadDone(QString, Context*)));
    if (reCalcOnVisible) eqCalc->recalculateEquipSpace(title(), space->allItems());
}

void
OverviewEquipmentWindow::hideEvent(QHideEvent*)
{
    reCalcOnVisible = false;
}

void
OverviewEquipmentWindow::showEvent(QShowEvent*)
{
    reCalcOnVisible = true;
    if (reCalcOnVisible) eqCalc->recalculateEquipSpace(title(), space->allItems());
}


EquipCalculator::EquipCalculator(const MainWindow* mainWindow) : mainWindow_(mainWindow)
{
}

EquipCalculator::~EquipCalculator()
{
}

void
EquipCalculator::recalculateEquipTile(const QString& eqLinkName, ChartSpaceItem* item)
{
    spaceItems_.clear();
    spaceItems_.append(item);
    recalculateTiles(eqLinkName);
}

void
EquipCalculator::recalculateEquipSpace(const QString& eqLinkName, QList<ChartSpaceItem*> items)
{
    spaceItems_ = items;
    recalculateTiles(eqLinkName);
}

void
EquipCalculator::recalculateTiles(const QString& eqLinkName)
{
    // already recalcuating !
    if (recalculationThreads_.count()) return;

    eqLinkName_ = eqLinkName;
    eqLinkNumActivities_ = 0;
    eqLinkTotalDistanceScaled_ = 0;
    eqLinkTotalElevationScaled_ = 0;
    eqLinkTotalTimeInSecs_ = 0;
    eqLinkEarliestDate_ = QDate(1900, 01, 01);
    eqLinkLatestDate_ = eqLinkEarliestDate_;

    // take a copy of the rides through the athlete's rides creating a ride List to process
    for (auto it = mainWindow_->athletetabs.keyValueBegin(); it != mainWindow_->athletetabs.keyValueEnd(); ++it) {
        rideItemList_ += it->second->context->athlete->rideCache->rides();
    }

    // Reset all the tiles distances
    for (ChartSpaceItem* item : spaceItems_) {
        
        if (item->type == OverviewItemType::EQ_ITEM)
        {
            static_cast<EquipmentItem*>(item)->resetEqItem();
        }
        else if (item->type == OverviewItemType::EQ_SUMMARY)
        {
            static_cast<EquipmentSummary*>(item)->resetEqSummary();
        }
    }

    // calculate number of threads and work per thread
    int maxthreads = QThreadPool::globalInstance()->maxThreadCount();
    int threads = maxthreads / 4; // Don't need many threads
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

        // update any summary instances
        for (ChartSpaceItem* item : spaceItems_) {

            if (item->type == OverviewItemType::EQ_SUMMARY) {

                static_cast<EquipmentSummary*>(item)->updateSummaryItem(eqLinkNumActivities_, eqLinkTotalTimeInSecs_,
                                                                        eqLinkTotalDistanceScaled_, eqLinkTotalElevationScaled_,
                                                                        eqLinkEarliestDate_, eqLinkLatestDate_);
            }
        }
    }
}

void
EquipCalculator::recalculateEq(RideItem* rideItem)
{
    if (rideItem->getText("EquipmentLink", "abcde") == eqLinkName_) {

        // get the date of the activity 
        QDate actDate(QDate(1900, 01, 01).addDays(rideItem->getText("Start Date", "0").toInt()));

        // using integral type atomics (c++11) but to retain accuracy multiply by EQ_REAL_TO_SCALED_ACC, see overviewItems.h
        double rideDistance = rideItem->getForSymbol("total_distance", GlobalContext::context()->useMetricUnits);
        uint64_t rideDistanceScaled = static_cast<uint64_t>(round(rideDistance*EQ_REAL_TO_SCALED_ACC));
        eqLinkTotalDistanceScaled_ += rideDistanceScaled;

        double rideElevation = rideItem->getForSymbol("elevation_gain", GlobalContext::context()->useMetricUnits);
        uint64_t eqElevationScaled_ = static_cast<uint64_t>(round(rideElevation*EQ_REAL_TO_SCALED_ACC));
        eqLinkTotalElevationScaled_ += eqElevationScaled_;

        uint64_t rideTimeInSecs = static_cast<uint64_t>(rideItem->getForSymbol("time_riding"));
        eqLinkTotalTimeInSecs_ += rideTimeInSecs;

        if (eqLinkNumActivities_++) {
            if (actDate < eqLinkEarliestDate_) eqLinkEarliestDate_ = actDate;
            if (actDate > eqLinkLatestDate_) eqLinkLatestDate_ = actDate;
        } else {
             eqLinkEarliestDate_ = actDate;
            eqLinkLatestDate_ = actDate;
        }

        for (ChartSpaceItem* item : spaceItems_) {

            if (item->type == OverviewItemType::EQ_ITEM)
            {
                if (static_cast<EquipmentItem*>(item)->isWithin(actDate)) {

                    static_cast<EquipmentItem*>(item)->addActivity(rideDistanceScaled, eqElevationScaled_, rideTimeInSecs);
                }
            } else if (item->type == OverviewItemType::EQ_SUMMARY)
            {
                static_cast<EquipmentSummary*>(item)->addAthleteActivity(rideItem->context->athlete->cyclist);
            }
        }
    }
}