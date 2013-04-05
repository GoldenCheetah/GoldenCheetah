
#include "StressCalculator.h"
#include "MetricAggregator.h"
#include "RideMetric.h"
#include "RideItem.h"
#include "MainWindow.h"

#include <stdio.h>

#include <QSharedPointer>
#include <QProgressDialog>

StressCalculator::StressCalculator (
    QString cyclist,
	QDateTime startDate,
	QDateTime endDate,
        double initialSTS = 0,
	double initialLTS = 0,
	int shortTermDays = 7,
	int longTermDays = 42) :
	startDate(startDate), endDate(endDate), shortTermDays(shortTermDays),
	longTermDays(longTermDays),
	initialSTS(initialSTS), initialLTS(initialLTS), lastDaysIndex(-1)
{
    // calc SB for today or tomorrow?
    showSBToday = appsettings->cvalue(cyclist, GC_SB_TODAY).toInt();

    days = startDate.daysTo(endDate);

    // make vectors 1 larger in case there is a ride for today.
    // see calculateStress()
    stsvalues.resize(days+2);
    ltsvalues.resize(days+2);
    sbvalues.resize(days+2);
    xdays.resize(days+2);
    list.resize(days+2);
    ltsramp.resize(days+2);
    stsramp.resize(days+2);

    lte = (double)exp(-1.0/longTermDays);
    ste = (double)exp(-1.0/shortTermDays);
}


double StressCalculator::max(void) {
    double max = 0.0;
    for(int i = 0; i < days; i++) {
	if (stsvalues[i] > max)  max = stsvalues[i];
	if (ltsvalues[i] > max)  max = ltsvalues[i]; // unlikely..
	if (sbvalues[i] > max)  max = sbvalues[i]; // really unlikely.
    }
    return max;
}


double StressCalculator::min(void) {
    double min = 100.0;
    for(int i = 0; i < days; i++) {
	if (sbvalues[i] < min)  min = sbvalues[i];
	if (ltsvalues[i] < min)  min = ltsvalues[i]; // unlikely..
	if (stsvalues[i] < min)  min = stsvalues[i]; // really unlikely
    }
    return min;
}



void StressCalculator::calculateStress(MainWindow *main, QString, const QString &metric, bool isfilter, QStringList filter)
{
    // get all metric data from the year 1900 - 3000
    QList<SummaryMetrics> results;

    // get metrics
    results = main->metricDB->getAllMetricsFor(QDateTime(QDate(1900,1,1)), QDateTime(QDate(3000,1,1)));

    if (isfilter) {
        // remove any we don't have filtered
        QList<SummaryMetrics> filteredresults;
        foreach (SummaryMetrics x, results) {
            if (filter.contains(x.getFileName()))
                filteredresults << x;
        }
        results = filteredresults;
    }

    // and honour the global one too!
    if (main->isfiltered) {
        // remove any we don't have filtered
        QList<SummaryMetrics> filteredresults;
        foreach (SummaryMetrics x, results) {
            if (main->filters.contains(x.getFileName()))
                filteredresults << x;
        }
        results = filteredresults;
    }

    if (results.count() == 0) return; // no ride files found

    // set start and enddate to maximum maximum required date range
    // remember the date range required so we can truncate afterwards
    QDateTime startDateNeeded = startDate;
    QDateTime endDateNeeded   = endDate;
    startDate = startDate < results[0].getRideDate() ? startDate : results[0].getRideDate();
    endDate   = endDate > results[results.count()-1].getRideDate() ? endDate : results[results.count()-1].getRideDate();

    int maxarray = startDate.daysTo(endDate) +2; // from zero plus tomorrows SB!
    stsvalues.resize(maxarray);
    ltsvalues.resize(maxarray);
    sbvalues.resize(maxarray);
    xdays.resize(maxarray);
    list.resize(maxarray);
    ltsramp.resize(maxarray);
    stsramp.resize(maxarray);

    // clear data add in the seeds
    ltsvalues.fill(0);
    stsvalues.fill(0);
    foreach(Season x, main->seasons->seasons) {
        if (x.getSeed()) {
            int offset = startDate.date().daysTo(x.getStart());
            ltsvalues[offset] = x.getSeed() * -1;
            stsvalues[offset] = x.getSeed() * -1;
        }
    }

    
    for (int i=0; i<results.count(); i++)
        addRideData(results[i].getForSymbol(metric), results[i].getRideDate());

    // ensure the last day is covered ...
    addRideData(0.0, endDate);

    // now truncate the data series to the requested date range
    int firstindex = startDate.daysTo(startDateNeeded);
    int lastindex  = startDate.daysTo(endDateNeeded)+2; // for today and tomorrow SB

    // zap the back
    if (lastindex < maxarray) {
        stsvalues.remove(lastindex, maxarray-lastindex);
        ltsvalues.remove(lastindex, maxarray-lastindex);
        sbvalues.remove(lastindex, maxarray-lastindex);
        xdays.remove(lastindex, maxarray-lastindex);
        list.remove(lastindex, maxarray-lastindex);
        stsramp.remove(lastindex, maxarray-lastindex);
        ltsramp.remove(lastindex, maxarray-lastindex);
    }
    // now zap the front
    if (firstindex) {
        stsvalues.remove(0, firstindex);
        ltsvalues.remove(0, firstindex);
        ltsramp.remove(0, firstindex);
        stsramp.remove(0, firstindex);
        sbvalues.remove(0, firstindex);
        xdays.remove(0, firstindex);
        list.remove(0, firstindex);
    }

    // reapply the requested date range
    startDate = startDateNeeded;
    endDate = endDateNeeded;

    days = startDate.daysTo(endDate) + 1; // include today

}

/*
 * calculate each day's STS and LTS.  The daily BS values are in
 * the list.  if there aren't enough days in the list yet, we fake
 * the missing days using the supplied initial value for each day.
 * STS and LTS are calculated up to but not including todays' ride.
 * if there are two rides per day the second one is added to the first
 * so the BS/day is correct.
 */
void StressCalculator::addRideData(double BS, QDateTime rideDate) {
    int daysIndex = startDate.daysTo(rideDate);

    // fill in any missing days before today
    int d;
    for (d = lastDaysIndex + 1; d < daysIndex ; d++) {
        list[d] = 0.0; // no ride
        calculate(d);
    }

    // ignore stuff from before start date
    if(daysIndex < 0) return;

    // today
    list[daysIndex] += BS;
    calculate(daysIndex);
    lastDaysIndex = daysIndex;
}


/*
 * calculate stress (in Bike Score units) using
 * stress = today's BS * (1 - exp(-1/days)) + yesterday's stress * exp(-1/days)
 * where days is the time period of concern- 7 for STS and 42 for LTS.
 *
 * exp(-1/days) for short and long term is calculated when the
 * class is instantiated.
 *
 */
void StressCalculator::calculate(int daysIndex) {
    double lastLTS, lastSTS;

    // if its seeded leave it alone
    if (ltsvalues[daysIndex] >=0 || stsvalues[daysIndex]>=0) {
        // LTS
        if (daysIndex == 0)
            lastLTS = initialLTS;
        else
            lastLTS = ltsvalues[daysIndex-1];

        ltsvalues[daysIndex] = (list[daysIndex] * (1.0 - lte)) + (lastLTS * lte);

        // STS
        if (daysIndex == 0)
            lastSTS = initialSTS;
        else
            lastSTS = stsvalues[daysIndex-1];

        stsvalues[daysIndex] = (list[daysIndex] * (1.0 - ste)) + (lastSTS * ste);
    } else if (ltsvalues[daysIndex]< 0|| stsvalues[daysIndex]<0) {

        ltsvalues[daysIndex] *= -1;
        stsvalues[daysIndex] *= -1;
    }

    // SB (stress balance)  long term - short term
    // FIXED BUG WHERE SB WAS NOT SHOWN ON THE NEXT DAY!
    if (daysIndex == 0) sbvalues[daysIndex]=0;
    sbvalues[daysIndex+(showSBToday ? 0 : 1)] =  ltsvalues[daysIndex] - stsvalues[daysIndex] ;

    // xdays
    xdays[daysIndex] = daysIndex+1;

    // ramp
    if (daysIndex > 0) {
        stsramp[daysIndex] = stsvalues[daysIndex] - stsvalues[daysIndex-1];
        ltsramp[daysIndex] = ltsvalues[daysIndex] - ltsvalues[daysIndex-1];
    }
}
