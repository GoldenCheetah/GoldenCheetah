

#ifndef _GC_StressCalculator_h
#define _GC_StressCalculator_h 1
#include "GoldenCheetah.h"


/* STS = Short Term Stress.. default 7 days
 * LTS = Long Term Stress.. default 42 days
 * SB = stress balance... negative is stressed
 */

#include <QtCore>

#include <QList>
#include <QDateTime>
#include <QTreeWidgetItem>
#include "Settings.h"
#include "MetricAggregator.h"

class StressCalculator:public QObject {

    Q_OBJECT
    G_OBJECT


    private:
	int days; // number of days to calculate stress for
	QDateTime startDate, endDate;  // start date
	int shortTermDays;
	int longTermDays;
    double ste, lte;

	int lastDaysIndex;
    bool showSBToday;

	// graph axis arrays
	QVector<double> stsvalues;
	QVector<double> ltsvalues;
    QVector<double> ltsramp;
    QVector<double> stsramp;
	QVector<double> sbvalues;
	QVector<double> xdays;
	// averaging array
	QVector<double> list;

	void calculate(int daysIndex);
	void addRideData(double BS, QDateTime rideDate);

    QSharedPointer<QSettings> settings;

    public:

	StressCalculator(QString cyclist, QDateTime startDate, QDateTime endDate, int shortTermDays, int longTermDays);

	void calculateStress(MainWindow *, QString, const QString &metric, bool filter = false, QStringList files = QStringList());

	// x axes:
	double *getSTSvalues() { return stsvalues.data(); }
	double *getLTSvalues() { return ltsvalues.data(); }
	double *getSBvalues() { return sbvalues.data(); }
    double *getDAYvalues() { return list.data(); }
    double *getLRvalues() { return ltsramp.data(); }
    double *getSRvalues() { return stsramp.data(); }

	// y axis
	double *getDays() { return xdays.data(); }
	int n() { return days; }

	// for scaling
	double min(void);
	double max(void);

	// for scale
	QDateTime getStartDate(void) { return startDate; }
	QDateTime getEndDate(void) { return startDate.addDays(days); }

};



#endif // _GC_StressCalculator_h
