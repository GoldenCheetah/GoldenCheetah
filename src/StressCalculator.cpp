
#include "StressCalculator.h"
#include "RideMetric.h"
#include "RideItem.h"
#include <stdio.h>

#include <QSharedPointer>
#include <QProgressDialog>

StressCalculator::StressCalculator (
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
    days = startDate.daysTo(endDate);
    // make vectors 1 larger in case there is a ride for today.
    // see calculateStress()
    stsvalues.resize(days+1);
    ltsvalues.resize(days+1);
    sbvalues.resize(days+1);
    xdays.resize(days+1);
    list.resize(days+1);

    lte = (double)exp(-1.0/longTermDays);
    ste = (double)exp(-1.0/shortTermDays);
    
    settings = GetApplicationSettings();
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



void StressCalculator::calculateStress(QWidget *mw,
        QString homePath, const QTreeWidgetItem * rides,
        const QString &metric)
{
    QSharedPointer<QProgressDialog> progress;
    int endingOffset = 0;
    bool aborted = false;
    bool showProgress = false;
    RideItem *item;


    // set up cache file
    QString cachePath = homePath + "/" + "stress.cache";
    QFile cacheFile(cachePath);
    QMap<QString,QMap<QString,float> > cache;

    const QString bs_name = "skiba_bike_score";
    const QString dp_name = "daniels_points";
    assert(metric == bs_name || metric == dp_name);

    if (cacheFile.exists() && cacheFile.size() > 0) {
	if (cacheFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
	    fprintf(stderr,"reading stress cache file\n");
	    QTextStream in(&cacheFile);
            bool first = true;
            QMap<int,QString> columnToMetric;
	    while(! in.atEnd()) {
                QString line = in.readLine();
                QStringList fields = line.split(",");
                if (first) {
                    first = false;
                    if (fields[0] != "Date")
                        break; // rescan to get DanielsPoints
                    for (int i = 1; i < fields.size(); ++i)
                        columnToMetric.insert(i, fields[i]);
                    continue;
                }
                else {
                    QString date = fields[0];
                    for (int i = 1; i < fields.size(); ++i)
                        cache[date][columnToMetric[i]] = fields[i].toFloat();
                }
	    }
	    cacheFile.close();
	}
    }
    if (cache.isEmpty()) {
	// set up progress bar only if no cache file
	progress = QSharedPointer<QProgressDialog>(new
		QProgressDialog(QString(tr("Computing stress.\n")),
		tr("Abort"),0,days,mw));
	endingOffset = progress->labelText().size();
	showProgress = true;
    }

    QVariant isAscending = settings->value(GC_ALLRIDES_ASCENDING,Qt::Checked);
    
    if(isAscending.toInt() > 0 ){
        item = (RideItem*) rides->child(0);
    } else {
        item = (RideItem*) rides->child(rides->childCount()-1);
    }

    for (int i = 0; i < rides->childCount(); ++i) {
        if(isAscending.toInt() > 0 ){
            item = (RideItem*) rides->child(i);
        } else {
            item = (RideItem*) rides->child(rides->childCount()-1-i);
        }

	// calculate using rides within date range
	if (item->dateTime.daysTo(startDate) <= 0 &&
		item->dateTime.daysTo(endDate) >= 0) { // inclusive of end date

	    QString ridedatestring = item->dateTime.toString();

	    double bs = 0.0, dp = 0.0;

	    if (showProgress) {
		QString existing = progress->labelText();
		existing.chop(progress->labelText().size() - endingOffset);
		progress->setLabelText( existing +
			QString(tr("Processing %1...")).arg(item->fileName));
	    }

	    // get new value if not in cache
	    if (cache.contains(ridedatestring)) {
		bs = cache[ridedatestring][bs_name];
		dp = cache[ridedatestring][dp_name];
	    }
	    else {
		item->computeMetrics();

                RideMetricPtr m;
                if ((m = item->metrics.value(bs_name)) && m->value(true))
		    bs = m->value(true);
		if ((m = item->metrics.value(dp_name)) && m->value(true))
		    dp = m->value(true);
		cache[ridedatestring][bs_name] = bs;
		cache[ridedatestring][dp_name] = dp;

                // only delete if the ride is clean (i.e. no pending ave)
                if (item->isDirty() == false) item->freeMemory();
	    }

	    addRideData(metric == bs_name ? bs : dp,item->dateTime);


	    // check progress
	    if (showProgress) {
		QCoreApplication::processEvents();
		if (progress->wasCanceled()) {
		    aborted = true;
		    goto done;
		}
		// set progress from 0 to days
		progress->setValue(startDate.daysTo(item->dateTime));
	    }
	}
    }
    // fill in any days from last ride up to YESTERDAY but not today.
    // we want to show todays ride if there is a ride but don't fill in
    // a zero for it if there is no ride
    if (item->dateTime.daysTo(endDate) > 0)
    {
	/*
	fprintf(stderr,"filling in up to date = %s\n",
		endDate.toString().toAscii().data());
	*/

	addRideData(0.0,endDate.addDays(-1));
    }
    else
    {
	// there was a ride for today, increment the count so
	// we will show it:
	days++;
    }

done:
    if (!aborted) {
	// write cache file
	if (cacheFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
	    cacheFile.resize(0); // truncate
	    QTextStream out(&cacheFile);
            out << "Date," << bs_name << "," << dp_name << endl;
            QMap<QString,QMap<QString,float> >::const_iterator i = cache.constBegin();
            while (i != cache.constEnd()) {
                out << i.key() << "," << i.value()[bs_name]
                    << "," << i.value()[dp_name] << endl;
		++i;
	    }
	    cacheFile.close();
	}
    }


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

    /*
       fprintf(stderr,"addRideData  date = %s\n",
       rideDate.toString().toAscii().data());
       */


    // fill in any missing days before today
    int d;
    for (d = lastDaysIndex + 1; d < daysIndex ; d++) {
	list[d] = 0.0; // no ride
	calculate(d);
    }
    // do this ride (may be more than one ride in a day)
    if(daysIndex < 0) return;
    list[daysIndex] += BS;
    calculate(daysIndex);
    lastDaysIndex = daysIndex;

    // fprintf(stderr,"addRideData (%.2f, %d)\n",BS,daysIndex);
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

    // SB (stress balance)  long term - short term
    sbvalues[daysIndex] =  ltsvalues[daysIndex] - stsvalues[daysIndex] ;

    // xdays
    xdays[daysIndex] = daysIndex+1;
}

