/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

	void calculateStress(Context *, QString, const QString &metric, bool filter = false, QStringList files = QStringList(), bool onhome=true);

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
