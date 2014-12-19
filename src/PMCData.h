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

#ifndef _GC_PMCData_h
#define _GC_PMCData_h 1
#include "GoldenCheetah.h"
#include "Settings.h"

#include "RideMetric.h"
#include "Specification.h"

#include <QtCore>
#include <QList>
#include <QDateTime>
#include <QTreeWidgetItem>

class Context;

class PMCData : public QObject {

    Q_OBJECT

    public:

        // create a PMC data series for the athlete
        // for ALL date ranges
        PMCData(Context *, Specification specification, QString metricName, int stsDays=7, int ltsDays=42);

        // set parameters
        void setStsDays(int x) { stsDays_ = x; invalidate(); }
        void setLtsDays(int x) { ltsDays_ = x; invalidate(); }
        void setSpecification(Specification x) { specification_ = x; invalidate(); }

        // get parameters
        QString &metricName() { return metricName_; }
        const RideMetric *metric() { return metric_; }
        int &stsDays() { return stsDays_; }
        int &ltsDays() { return ltsDays_; }
        Specification &specification() { return specification_; }

        // data accessors
        QDate &start() { return start_; }
        QDate &end() { return end_; }
        int &days() { return days_; }

        // index into the arrays
        int indexOf(QDate) ;

        // get value for date (refresh if needed)
        double lts(QDate);
        double sts(QDate);
        double stress(QDate);
        double sb(QDate);

    public slots:

        // as underlying ride data changes the
        // contents are invalidated and refreshed
        void invalidate();
        void refresh();

    private:

        // who we for ?
        Context *context;
        Specification specification_;

        // parameters
        QString metricName_;
        const RideMetric *metric_; // the input parameter
        int stsDays_, ltsDays_;

        // data
        QDate start_, end_;
        int days_;
        QVector<double> stress_, lts_, sts_, sb_;

        bool isstale; // needs refreshing
};

#endif // _GC_StressCalculator_h
