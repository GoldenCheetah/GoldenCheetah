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

#include <QString>
#include <QDateTime>

class SummaryMetrics
{
	public:
        // filename
	    QString getFileName() { return fileName; }
        void    setFileName(QString fileName) { this->fileName = fileName; }

        // ride date
        QDateTime getRideDate() { return rideDate; }
        void setRideDate(QDateTime rideDate) { this->rideDate = rideDate; }

        // metric values
        void setForSymbol(QString symbol, double v) { value.insert(symbol, v); }
        double getForSymbol(QString symbol) { return value.value(symbol, 0.0); }

	private:
	    QString fileName;
        QDateTime rideDate;
        QMap<QString, double> value;
};


#endif /* SUMMARYMETRICS_H_ */
