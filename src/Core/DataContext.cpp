#include "DataContext.h"
#include "RideItem.h"
#include "RealtimeData.h"
#include "ErgFile.h"

DataContext::DataContext(QObject *parent) : QObject(parent)
{
    athlete = nullptr;
    ride = nullptr;
    workout = nullptr;
    videosync = nullptr;
    season = nullptr;
    isCompareIntervals = false;
    isCompareDateRanges = false;
    isRunning = false;
    isPaused = false;
    now = 0;
}

DataContext::~DataContext()
{
}

void 
DataContext::notifyCompareIntervals(bool state) 
{ 
    isCompareIntervals = state; 
    emit compareIntervalsStateChanged(state); 
}

void 
DataContext::notifyCompareIntervalsChanged() 
{
    if (isCompareIntervals) {
        emit compareIntervalsChanged(); 
    }
}

void 
DataContext::notifyCompareDateRanges(bool state)
{
    isCompareDateRanges = state;
    emit compareDateRangesStateChanged(state); 
}

void 
DataContext::notifyCompareDateRangesChanged()
{ 
    if (isCompareDateRanges) {
        emit compareDateRangesChanged(); 
    }
}

void DataContext::notifyTelemetryUpdate(const RealtimeData &rtData) 
{ 
    emit telemetryUpdate(rtData); 
}

void DataContext::notifyErgFileSelected(ErgFile *x) 
{ 
    workout=x; 
    emit ergFileSelected(x); 
    emit ergFileSelected((ErgFileBase*)(x));
}
