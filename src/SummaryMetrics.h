/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#ifndef SUMMARYMETRICS_H_
#define SUMMARYMETRICS_H_
#include "GoldenCheetah.h"

#include <QString>
#include <QMap>
#include <QDateTime>
#include <QApplication>

class Context;
class SummaryBest
{
    public:
    double nvalue;
    QString value; // formatted value
    QDate date;
#ifdef GC_HAVE_INTERVALS
    QString fileName;
#endif

    // for qsort
    bool operator< (SummaryBest right) const { return (nvalue < right.nvalue); }
};

class SummaryMetrics
{
    Q_DECLARE_TR_FUNCTIONS(SummaryMetrics)

	public:
        // filename
	    QString getFileName() const { return fileName; }
        void    setFileName(QString fileName) { this->fileName = fileName; }

        // Identifier
        QString getId() const { return id; }
        void setId(QString id) { this->id = id; }

        // ride date
        QDateTime getRideDate() const { return rideDate; }
        void setRideDate(QDateTime rideDate) { this->rideDate = rideDate; }

        void setIsRun(bool x) { isrun = x; }
        bool isRun() { return isrun; }

        // for non-rides, ie. measures use same field but overload
        QDateTime getDateTime() const { return rideDate; }
        void setDateTime(QDateTime dateTime) { this->rideDate = dateTime; }

        // metric values
        void setForSymbol(QString symbol, double v) { value.insert(symbol, v); }
        double getForSymbol(QString symbol, bool metric=true) const;

        void setText(QString name, QString v) { text.insert(name, v); }
        QString getText(QString name, QString fallback) { return text.value(name, fallback); }

        // convert to string, using format supplied
        // replaces ${...:units} or ${...} with unit string
        // or value respectively
        QString toString(QString, bool units) const;

        // get a metric formatted properly and apply metric/imperial conversion
        QString getStringForSymbol(QString symbol, bool UseMetric) const;

        // get unit string to use for this symbol
        QString getUnitsForSymbol(QString symbol, bool UseMetric) const;

        // when passed a list of summary metrics and a name return aggregated value as a string
        static QString getAggregated(Context *context, QString name, 
                                     const QList<SummaryMetrics> &results,
                                     const QStringList &filters, bool filtered,
                                     bool useMetricUnits, bool nofmt = false);

        // get an ordered list pf bests for that symbol
        static QList<SummaryBest> getBests(Context *context, QString symbol, int n, 
                                            const QList<SummaryMetrics> &results, 
                                            const QStringList &filters, bool filtered, 
                                            bool useMetricUnits);

        QMap<QString, double> &values() { return value; }
        QMap<QString, QString> &texts() { return text; }

	private:
	    QString fileName;
        QString id;
        QDateTime rideDate;
        bool isrun;
        QMap<QString, double> value;
        QMap<QString, QString> text;
};


#endif /* SUMMARYMETRICS_H_ */
