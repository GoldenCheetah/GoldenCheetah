

#ifndef _GC_StressCalculator_h
#define _GC_StressCalculator_h 1


/* STS = Short Term Stress.. default 7 days
 * LTS = Long Term Stress.. default 42 days
 * SB = stress balance... negative is stressed
 */

#include <QtCore>

#include <QList>
#include <QDateTime>
#include <QTreeWidgetItem>
#include "Settings.h"

class StressCalculator:public QObject {

    Q_OBJECT

    private:
	int days; // number of days to calculate stress for
	QDateTime startDate, endDate;  // start date
	int shortTermDays;
	int longTermDays;
	double initialSTS;
	double initialLTS;
        double ste, lte;

	int lastDaysIndex;

	// graph axis arrays
	QVector<double> stsvalues;
	QVector<double> ltsvalues;
	QVector<double> sbvalues;
	QVector<double> xdays;
	// averaging array
	QVector<double> list;

	void calculate(int daysIndex);
	void addRideData(double BS, QDateTime rideDate);

    boost::shared_ptr<QSettings> settings;

    public:

	StressCalculator(QDateTime startDate, QDateTime endDate,
		double initialSTS, double initialLTS,
		int shortTermDays, int longTermDays);

	void calculateStress(QWidget *mw, QString homePath,
		const QTreeWidgetItem * rides, const QString &metric);

	// x axes:
	double *getSTSvalues() { return stsvalues.data(); }
	double *getLTSvalues() { return ltsvalues.data(); }
	double *getSBvalues() { return sbvalues.data(); }
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
