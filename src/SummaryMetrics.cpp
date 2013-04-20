/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "SummaryMetrics.h"
#include "MainWindow.h"
#include "RideMetric.h"
#include <QRegExp>
#include <QStringList>
#include <QDebug>

QString
SummaryMetrics::toString(QString format, bool UseMetric) const
{
    QRegExp tokens("\\$\\$\\{[^}]*\\}");
    QRegExp unit("^\\$\\$\\{([^:]*):units[^}]*\\}$");
    QRegExp value("^\\$\\$\\{([^}]*)\\}$");

    while(tokens.indexIn(format) != -1) {
        // what we looking for?
        if (unit.exactMatch(tokens.cap(0))) {

            format.replace(tokens.cap(0), getUnitsForSymbol(unit.cap(1), UseMetric));

        } else if (value.exactMatch(tokens.cap(0))) {

            format.replace(tokens.cap(0), getStringForSymbol(value.cap(1), UseMetric));

        } else {
            format.replace(tokens.cap(0), "unknown"); // unknown?

        }

    }

    return format;
}

static
const RideMetric *metricForSymbol(QString symbol)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();
    return factory.rideMetric(symbol);
}

double
SummaryMetrics::getForSymbol(QString symbol, bool metric) const
{
    if (metric) return value.value(symbol, 0.0);
    else {
        const RideMetric *m = metricForSymbol(symbol);
        double metricValue = value.value(symbol, 0.0);
        metricValue *= m->conversion();
        metricValue += m->conversionSum();
        return metricValue;
    }
}

QString
SummaryMetrics::getStringForSymbol(QString symbol, bool UseMetric) const
{
    // get the value honoouring metric/imperial and with the right
    // number of decimal places.

    const RideMetric *m = metricForSymbol(symbol);

    if (m) {

        // get the value
        double value = getForSymbol(symbol);

        // metric imperial conversion
        if (UseMetric == false) {
            value *= m->conversion();
            value += m->conversionSum();
        }

        if (m->units(true) == "seconds" || m->units(true) == tr("seconds")) {
            // format time values...
            QChar zero = QLatin1Char('0');
            int secs = value;
            return QString("%1:%2:%3").arg(secs/3600,2,10,zero)
                                          .arg(secs%3600/60,2,10,zero)
                                          .arg(secs%60,2,10,zero);

        } else {
            // format numeric values with right precision
            return QString("%1").arg(value, 0, 'f', m->precision());
        }

    } else return QString("%1").arg(getForSymbol(symbol));
}

QString
SummaryMetrics::getUnitsForSymbol(QString symbol, bool UseMetric) const
{
    const RideMetric *m = metricForSymbol(symbol);

    if (m) return m->units(UseMetric);
    else return QString("units");
}

QString SummaryMetrics::getAggregated(MainWindow *main, QString name, const QList<SummaryMetrics> &results, const QStringList &filters, 
                                      bool filtered, bool useMetricUnits, bool nofmt)
{
    // get the metric details, so we can convert etc
    const RideMetric *metric = RideMetricFactory::instance().rideMetric(name);
    if (!metric) return QString("%1 unknown").arg(name);

    // what we will return
    double rvalue = 0;
    double rcount = 0; // using double to avoid rounding issues with int when dividing

    // loop through and aggregate
    foreach (SummaryMetrics rideMetrics, results) {

        // skip filtered rides
        if (filtered && !filters.contains(rideMetrics.getFileName())) continue;
        if (main->isfiltered && !main->filters.contains(rideMetrics.getFileName())) continue;

        // get this value
        double value = rideMetrics.getForSymbol(name);
        double count = rideMetrics.getForSymbol("workout_time"); // for averaging

        
        // check values are bounded, just in case
        if (isnan(value) || isinf(value)) value = 0;

        // imperial / metric conversion
        if (useMetricUnits == false) {
            value *= metric->conversion();
            value += metric->conversionSum();
        }

        switch (metric->type()) {
        case RideMetric::Total:
            rvalue += value;
            break;
        case RideMetric::Average:
            {
            // average should be calculated taking into account
            // the duration of the ride, otherwise high value but
            // short rides will skew the overall average
            rvalue += value*count;
            rcount += count;
            break;
            }
        case RideMetric::Peak:
            {
            if (value > rvalue) rvalue = value;
            break;
            }
        }
    }

    // now compute the average
    if (metric->type() == RideMetric::Average) {
        if (rcount) rvalue = rvalue / rcount;
    }


    // Format appropriately
    QString result;
    if (metric->units(useMetricUnits) == "seconds" ||
        metric->units(useMetricUnits) == tr("seconds")) {
        if (nofmt) result = QString("%1").arg(rvalue);
        else result = time_to_string(rvalue);

    } else result = QString("%1").arg(rvalue, 0, 'f', metric->precision());

    return result;
}
