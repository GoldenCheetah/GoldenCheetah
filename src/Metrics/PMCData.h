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

#include "DataFilter.h"

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
        PMCData(Context *, Specification specification, QString metricName, int stsDays=-1, int ltsDays=-1);
        PMCData(Context *, Specification specification, Leaf *expr, DataFilterRuntime *df, int stsDays=-1, int ltsDays=-1);

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

        // get arrays
        QVector<double> &stress() { return stress_; }
        QVector<double> &lts() { return lts_; }
        QVector<double> &sts() { return sts_; }
        QVector<double> &sb() { return sb_; }
        QVector<double> &rr() { return rr_; }

        QVector<double> &plannedStress() { return planned_stress_; }
        QVector<double> &plannedLts() { return planned_lts_; }
        QVector<double> &plannedSts() { return planned_sts_; }
        QVector<double> &plannedSb() { return planned_sb_; }
        QVector<double> &plannedRr() { return planned_rr_; }

        QVector<double> &expectedStress() { return expected_stress_; }
        QVector<double> &expectedLts() { return expected_lts_; }
        QVector<double> &expectedSts() { return expected_sts_; }
        QVector<double> &expectedSb() { return expected_sb_; }
        QVector<double> &expectedRr() { return expected_rr_; }

        // index into the arrays
        int indexOf(QDate) ;

        // get value for date (refresh if needed)
        double lts(QDate);
        double sts(QDate);
        double stress(QDate);
        double sb(QDate);
        double rr(QDate);

        double plannedLts(QDate);
        double plannedSts(QDate);
        double plannedStress(QDate);
        double plannedSb(QDate);
        double plannedRr(QDate);

        double expectedLts(QDate);
        double expectedSts(QDate);
        double expectedStress(QDate);
        double expectedSb(QDate);
        double expectedRr(QDate);

        // colour coding the 4 series for RAG reporting
        static QColor ltsColor(double, QColor defaultColor);
        static QColor stsColor(double, QColor defaultColor);
        static QColor sbColor(double, QColor defaultColor);
        static QColor rrColor(double, QColor defaultColor);

        // user description for the 4 series
        static QString ltsDescription();
        static QString stsDescription();
        static QString sbDescription();
        static QString rrDescription();

    public slots:

        // as underlying ride data changes the
        // contents are invalidated and refreshed
        void invalidate();
        void refresh();

    private:

        // who we for ?
        Context *context;
        Specification specification_;

        bool fromDataFilter;
        Leaf *expr;

        // parameters
        QString metricName_;
        const RideMetric *metric_; // the input parameter
        int stsDays_, ltsDays_;
        bool useDefaults;

        // data
        QDate start_, end_;
        int days_;
        QVector<double> stress_, lts_, sts_, sb_, rr_;
        QVector<double> planned_stress_, planned_lts_, planned_sts_, planned_sb_, planned_rr_;
        QVector<double> expected_stress_, expected_lts_, expected_sts_, expected_sb_, expected_rr_;

        bool isstale; // needs refreshing

        void calculateMetrics(int days, const QVector<double> &stress, QVector<double> &lts, QVector<double> &sts, QVector<double> &sb, QVector<double> &rr) const;
};

#endif // _GC_StressCalculator_h
