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
#include <QMap>
#include <QHash>
#include <QSet>


#undef DEV_GNUPLOT

class Context;
class Season;

class PMCData : public QObject {

    Q_OBJECT

    public:
        using SeriesT = QVector<double>;

        enum class SeriesType : char {
            Actual,
            Planned,
            Expected,
            MacroExpectedTarget,
            MacroExpectedSlotted
        };
        enum class ValueType : char {
            Stress,
            Lts,
            Sts,
            Sb,
            Rr
        };
        struct SeriesKey {
            SeriesType series;
            ValueType value;
            QString sport;

            explicit SeriesKey(SeriesType s, ValueType v, QString sport = "") noexcept
                : series(s), value(v), sport(std::move(sport)) {}

            [[nodiscard]] SeriesKey withSeries(SeriesType newSeries) const {
                return SeriesKey(newSeries, value, sport);
            }
            [[nodiscard]] SeriesKey withValue(ValueType newValue) const {
                return SeriesKey(series, newValue, sport);
            }
            [[nodiscard]] SeriesKey withSport(const QString &newSport) const {
                return SeriesKey(series, value, newSport);
            }
            bool operator==(const SeriesKey &other) const noexcept {
                return    series == other.series
                       && value == other.value
                       && sport == other.sport;
            }
            friend inline size_t qHash(const SeriesKey& key, size_t seed = 0) noexcept {
                return qHashMulti(seed, key.series, key.value, key.sport);
            }
        };


        // create a PMC data series for the athlete
        // for ALL date ranges
        // fullScope: individual series per sports and macroSeries
        PMCData(Context *, Specification specification, QString metricName, int stsDays=-1, int ltsDays=-1, bool fullScope = false);
        PMCData(Context *, Specification specification, Leaf *expr, DataFilterRuntime *df, int stsDays=-1, int ltsDays=-1, bool fullScope = false);

        // set parameters
        void setStsDays(int x) { stsDays_ = x; invalidate(); }
        void setLtsDays(int x) { ltsDays_ = x; invalidate(); }
        void setSpecification(Specification x) { specification_ = x; invalidate(); }

        QString &metricName() { return metricName_; }
        const RideMetric *metric() { return metric_; }
        int &stsDays() { return stsDays_; }
        int &ltsDays() { return ltsDays_; }
        Specification &specification() { return specification_; }

        QDate &start() { return start_; }
        QDate &end() { return end_; }
        int &days() { return days_; }

        // name used for activities without a sport
        static QString otherSport() { return tr("Other"); }

        // sports for which at least one series with data exists (sorted)
        QStringList sports();

        // get arrays
        SeriesT stress(const QString &sport = "") { return data(SeriesKey(SeriesType::Actual, ValueType::Stress, sport)); }
        SeriesT lts(const QString &sport = "") {  return data(SeriesKey(SeriesType::Actual, ValueType::Lts, sport)); }
        SeriesT sts(const QString &sport = "") {  return data(SeriesKey(SeriesType::Actual, ValueType::Sts, sport)); }
        SeriesT sb(const QString &sport = "") {  return data(SeriesKey(SeriesType::Actual, ValueType::Sb, sport)); }
        SeriesT rr(const QString &sport = "") {  return data(SeriesKey(SeriesType::Actual, ValueType::Rr, sport)); }

        SeriesT plannedStress(const QString &sport = "") { return data(SeriesKey(SeriesType::Planned, ValueType::Stress, sport)); }
        SeriesT plannedLts(const QString &sport = "") {  return data(SeriesKey(SeriesType::Planned, ValueType::Lts, sport)); }
        SeriesT plannedSts(const QString &sport = "") {  return data(SeriesKey(SeriesType::Planned, ValueType::Sts, sport)); }
        SeriesT plannedSb(const QString &sport = "") {  return data(SeriesKey(SeriesType::Planned, ValueType::Sb, sport)); }
        SeriesT plannedRr(const QString &sport = "") {  return data(SeriesKey(SeriesType::Planned, ValueType::Rr, sport)); }

        SeriesT expectedStress(const QString &sport = "") { return data(SeriesKey(SeriesType::Expected, ValueType::Stress, sport)); }
        SeriesT expectedLts(const QString &sport = "") { return data(SeriesKey(SeriesType::Expected, ValueType::Lts, sport)); }
        SeriesT expectedSts(const QString &sport = "") { return data(SeriesKey(SeriesType::Expected, ValueType::Sts, sport)); }
        SeriesT expectedSb(const QString &sport = "") { return data(SeriesKey(SeriesType::Expected, ValueType::Sb, sport)); }
        SeriesT expectedRr(const QString &sport = "") { return data(SeriesKey(SeriesType::Expected, ValueType::Rr, sport)); }

        SeriesT macroExpectedTargetStress() { return data(SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Stress)); }
        SeriesT macroExpectedTargetLts() { return data(SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Lts)); }
        SeriesT macroExpectedTargetSts() { return data(SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Sts)); }
        SeriesT macroExpectedTargetSb() { return data(SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Sb)); }
        SeriesT macroExpectedTargetRr() { return data(SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Rr)); }

        SeriesT macroExpectedSlottedStress(const QString &sport = "") { return data(SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Stress, sport)); }
        SeriesT macroExpectedSlottedLts(const QString &sport = "") { return data(SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Lts, sport)); }
        SeriesT macroExpectedSlottedSts(const QString &sport = "") { return data(SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Sts, sport)); }
        SeriesT macroExpectedSlottedSb(const QString &sport = "") { return data(SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Sb, sport)); }
        SeriesT macroExpectedSlottedRr(const QString &sport = "") { return data(SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Rr, sport)); }

        // returns the series, or a temporary all-zero series if there is none
        SeriesT data(const SeriesKey &key);

        // index into the arrays
        int indexOf(QDate) ;

        // get value for date (refresh if needed)
        double stress(QDate, const QString &sport = "");
        double lts(QDate, const QString &sport = "");
        double sts(QDate, const QString &sport = "");
        double sb(QDate, const QString &sport = "");
        double rr(QDate, const QString &sport = "");

        double plannedStress(QDate, const QString &sport = "");
        double plannedLts(QDate, const QString &sport = "");
        double plannedSts(QDate, const QString &sport = "");
        double plannedSb(QDate, const QString &sport = "");
        double plannedRr(QDate, const QString &sport = "");

        double expectedStress(QDate, const QString &sport = "");
        double expectedLts(QDate, const QString &sport = "");
        double expectedSts(QDate, const QString &sport = "");
        double expectedSb(QDate, const QString &sport = "");
        double expectedRr(QDate, const QString &sport = "");

        double macroExpectedTargetStress(QDate);
        double macroExpectedTargetLts(QDate);
        double macroExpectedTargetSts(QDate);
        double macroExpectedTargetSb(QDate);
        double macroExpectedTargetRr(QDate);

        double macroExpectedSlottedStress(QDate, const QString &sport = "");
        double macroExpectedSlottedLts(QDate, const QString &sport = "");
        double macroExpectedSlottedSts(QDate, const QString &sport = "");
        double macroExpectedSlottedSb(QDate, const QString &sport = "");
        double macroExpectedSlottedRr(QDate, const QString &sport = "");

        double data(const QDate &date, const SeriesKey &key);

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

#if defined(DEV_GNUPLOT)
        QString gnuplotScript(SeriesType series, const QDate &from, const QDate &to, const QString &sport = QString());
        void printGnuplot(SeriesType series, const QDate &from, const QDate &to, const QString &sport = QString());
        bool writeGnuplot(const QString &fileName, SeriesType series, const QDate &from, const QDate &to, const QString &sport = QString());
#endif

    public slots:

        // as underlying ride data changes the
        // contents are invalidated and refreshed
        void invalidate();
        void refresh();

    private:

        // day index -> seed value; a seed resets LTS and STS of the series on that day
        using SeedMap = QMap<int, double>;

        // who we for ?
        Context *context;
        Specification specification_;

        bool fullScope = false;

        bool fromDataFilter = false;
        Leaf *expr = nullptr;

        // parameters
        QString metricName_;
        const RideMetric *metric_ = nullptr; // the input parameter
        int stsDays_, ltsDays_;
        bool useDefaults = false;

        // data
        QDate start_, end_;
        int days_ = 0;
        using SeriesGroupT = QHash<QString, SeriesT>;
        using AllSeriesT = QHash<SeriesKey, SeriesT>;
        AllSeriesT seriesData;

        bool isstale = true;      // needs refreshing
        bool macroStale = true;   // macro series need refreshing

        double lte_ = 0.0;
        double ste_ = 0.0;
        bool sbToday_ = false;
        void updateDecay();

        bool macroMetricMatches(const Season *season) const;

        // activities and slots without a sport are accounted to otherSport()
        QString normalizeSport(const QString &sport) const;

        SeriesT buildEmptySeries(int size) const;
        void eraseSeries(SeriesType series);

        // day index of the seed of a season, -1 if it has no seed or lies outside the range
        int seedOffset(const Season &season) const;

        SeedMap seedsFor(SeriesType series) const;

        // moves the stress series into seriesData and derives lts/sts/sb/rr
        void commitSeries(SeriesType series, SeriesT globalStress, SeriesGroupT &sportStress);

        // derives lts/sts/sb/rr for an existing stress series
        void deriveSeries(SeriesType series, const QString &sport, const SeedMap &seeds);

        void calculateMetrics(int days, const SeriesT &stress, const SeedMap &seeds, SeriesT &lts, SeriesT &sts, SeriesT &sb, SeriesT &rr) const;

        bool fillMacro();
        void connectMacroSignals();

        QDate minValid(const QDate &date1, const QDate &date2) const;
        QDate maxValid(const QDate &date1, const QDate &date2) const;
};

#endif // _GC_PMCData_h
