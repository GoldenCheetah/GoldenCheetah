
#include "BasicRideMetrics.h"

bool totalDistanceAdded =
    RideMetricFactory::instance().addMetric(TotalDistanceRideMetric());
bool totalWorkAdded =
    RideMetricFactory::instance().addMetric(TotalWorkRideMetric());

