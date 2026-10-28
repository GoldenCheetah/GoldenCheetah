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


#include "PMCData.h"

#include "Athlete.h"
#include "RideCache.h"
#include "RideMetric.h"
#include "RideItem.h"
#include "Specification.h"
#include "Season.h"
#include "Seasons.h"
#include "Context.h"

#include <stdio.h>
#include <cmath>
#include <algorithm>
#include <utility>


#undef PMCDATA_DEBUG

#if defined(PMCDATA_DEBUG)
#define PMC_TIMER_START QElapsedTimer timer; timer.start();
#define PMC_TIMER_LOG qDebug() << (context->currentSeason() != nullptr ? context->currentSeason()->getName() : "NO SELECTED SEASON") \
                               << "took" << timer.elapsed() << "ms";
#else
#define PMC_TIMER_START
#define PMC_TIMER_LOG
#endif


PMCData::PMCData(Context *context, Specification spec, QString metricName, int stsDays, int ltsDays, bool fullScope)
    : context(context), specification_(spec), fullScope(fullScope), metricName_(metricName), stsDays_(stsDays), ltsDays_(ltsDays)
{
    // get defaults if not passed
    useDefaults = false;

    // we're not from a datafilter
    fromDataFilter = false;
    expr = nullptr;

    if (ltsDays < 0) {
        QVariant lts = appsettings->cvalue(context->athlete->cyclist, GC_LTS_DAYS);
        if (lts.isNull() || lts.toInt() == 0) ltsDays_ = 42;
        else ltsDays_ = lts.toInt();
        useDefaults=true;
    }
    if (stsDays < 0) {
        QVariant sts = appsettings->cvalue(context->athlete->cyclist, GC_STS_DAYS);
        if (sts.isNull() || sts.toInt() == 0) stsDays_ = 7;
        else stsDays_ = sts.toInt();
        useDefaults=true;
    }


    refresh();
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(invalidate()));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(invalidate()));
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(invalidate()));
    connect(context->athlete->rideCache, SIGNAL(itemChanged(RideItem*)), this, SLOT(invalidate()));
    connect(context->athlete->seasons, SIGNAL(seasonsChanged()), this, SLOT(invalidate()));
    connectMacroSignals();
}

PMCData::PMCData(Context *context, Specification spec, Leaf *expr, DataFilterRuntime *df, int stsDays, int ltsDays, bool fullScope)
    : context(context), specification_(spec), fullScope(fullScope), metricName_(""), stsDays_(stsDays), ltsDays_(ltsDays)
{
    // get defaults if not passed
    useDefaults = false;

    // If expr is just a metric name, we can use the faster and specific case
    QString metricName = df->lookupMap.value(expr->toString(),"");
    RideMetricFactory &factory = RideMetricFactory::instance();
    if (factory.haveMetric(metricName)) {
        // use a metric name
        metricName_ = metricName;
        fromDataFilter = false;
        this->expr = nullptr;
    } else {
        // use an expression
        fromDataFilter = true;
        this->expr = expr;
    }

    if (ltsDays < 0) {
        QVariant lts = appsettings->cvalue(context->athlete->cyclist, GC_LTS_DAYS);
        if (lts.isNull() || lts.toInt() == 0) ltsDays_ = 42;
        else ltsDays_ = lts.toInt();
        useDefaults=true;
    }
    if (stsDays < 0) {
        QVariant sts = appsettings->cvalue(context->athlete->cyclist, GC_STS_DAYS);
        if (sts.isNull() || sts.toInt() == 0) stsDays_ = 7;
        else stsDays_ = sts.toInt();
        useDefaults=true;
    }


    refresh();
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(invalidate()));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(invalidate()));
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(invalidate()));
    connect(context->athlete->rideCache, SIGNAL(itemChanged(RideItem*)), this, SLOT(invalidate()));
    connect(context->athlete->seasons, SIGNAL(seasonsChanged()), this, SLOT(invalidate()));
    connectMacroSignals();
}


void PMCData::invalidate()
{
    isstale = true;
}


void PMCData::updateDecay()
{
    lte_ = static_cast<double>(exp(-1.0 / ltsDays_));
    ste_ = static_cast<double>(exp(-1.0 / stsDays_));
    sbToday_ = appsettings->cvalue(context->athlete->cyclist, GC_SB_TODAY).toInt() != 0;
}


bool
PMCData::macroMetricMatches
(Season const * const season) const
{
    if (fromDataFilter) {
        return false;
    } else if (season->getModelType() == Season::ModelType::coggan) {
        return metricName_ == "coggan_tss";
    } else if (season->getModelType() == Season::ModelType::skiba) {
        return    metricName_ == "skiba_bike_score"
               || metricName_ == "govss"
               || metricName_ == "swimscore"
               || metricName_ == "triscore";
    }
    return false;
}


QString
PMCData::normalizeSport
(const QString &sport) const
{
    if (! fullScope) {
        return sport;
    }
    return sport.isEmpty() ? otherSport() : sport;
}


void PMCData::refresh()
{
    if (! isstale) {
        if (macroStale) {
            fillMacro();
        }
        return;
    }

    // we need to reread config if refreshing (it might have changed)
    if (useDefaults) {

        QVariant lts = appsettings->cvalue(context->athlete->cyclist, GC_LTS_DAYS);
        if (lts.isNull() || lts.toInt() == 0) ltsDays_ = 42;
        else ltsDays_ = lts.toInt();

        QVariant sts = appsettings->cvalue(context->athlete->cyclist, GC_STS_DAYS);
        if (sts.isNull() || sts.toInt() == 0) stsDays_ = 7;
        else stsDays_ = sts.toInt();
    }
    updateDecay();

    PMC_TIMER_START

    //
    // STEP ONE: What is the date range ?
    //

    // Date range needs to take into account seasons that
    // have a starting LTS/STS potentially before any rides
    QDate extraStart;
    QDate extraEnd;
    for (const Season &season : context->athlete->seasons->seasons) {
        if (season.getSeed()) {
            extraStart = minValid(extraStart, season.getStart());
            extraEnd = maxValid(extraEnd, season.getStart());
        }
        if (season.isMacrocycle()) {
            extraStart = minValid(extraStart, season.getStart());
            extraEnd = maxValid(extraEnd, season.getEnd());
        }
    }

    // take into account any rides, some might be before
    // the start of the first defined season
    QDate first, last;
    if (context->athlete->rideCache->rides().count()) {

        // set date range - extend to a year after last ride
        first = context->athlete->rideCache->rides().first()->dateTime.date();
        last = context->athlete->rideCache->rides().last()->dateTime.date();
    }

    // what is earliest date we got ? (substract 1 day to include first ride)
    start_ = QDate(9999,12,31);
    if (extraStart.isValid() && extraStart < start_) {
        start_ = extraStart;
    }
    if (first.isValid() && first <= start_) {
        start_ = first.addDays(-1);
    }

    // whats the latest date we got ? (and add a year for decay)
    end_ = QDate();
    if (last > extraEnd) {
        end_ = last.addDays(365);
    } else if (extraEnd.isValid()) {
        end_ = extraEnd.addDays(365);
    }

    // back to null date if not set, just to get round date arithmetic
    if (start_ == QDate(9999,12,31)) start_ = QDate();

    // We got a valid range ?
    if (start_ != QDate() && end_ != QDate() && start_ < end_) {

        // resize arrays
        days_ = start_.daysTo(end_)+1;
    } else {

        // nothing to calculate
        start_= QDate();
        end_ = QDate();
        days_ = 0;
    }

    // Throw away everything: all series depend on start_, days_, the time
    // constants and the specification, and the macro series depend on them too.
    seriesData.clear();
    macroStale = true;
    if (days_ == 0) {
        // give up
        return;
    }

    //
    // STEP TWO What are the seedings and ride values
    //


    // Everything is accumulated into local containers and only moved into
    // seriesData afterwards, to avoid changing references in hashes
    SeriesT actualAll = buildEmptySeries(days_);
    SeriesT plannedAll = buildEmptySeries(days_);
    SeriesT expectedAll = buildEmptySeries(days_);
    SeriesGroupT actualBy;
    SeriesGroupT plannedBy;
    SeriesGroupT expectedBy;

    // a sport's series is created when it receives its first value;
    // the returned reference must be used only before the next insert
    auto sportSeries = [this](SeriesGroupT &by, const QString &sport) -> SeriesT& {
        SeriesGroupT::iterator it = by.find(sport);
        if (it == by.end()) {
            it = by.insert(sport, buildEmptySeries(days_));
        }
        return it.value();
    };

    DataFilter* df = new DataFilter(this, context);

    const int todayIdx = start_.daysTo(QDate::currentDate());
    bool haveToday = false;
    double todayActualStress = 0;
    double todayPlannedStress = 0;
    QHash<QString, double> todayActualBySport;
    QHash<QString, double> todayPlannedBySport;

    // add the stress scores
    for (RideItem *item : context->athlete->rideCache->rides()) {

        if (!specification_.pass(item)) continue;

        // seed with score for this one
        const int offset = start_.daysTo(item->dateTime.date());
        if (offset <= 0 || offset >= days_) {
            continue;
        }

        // although metrics are cleansed, we check here because development
        // builds have a rideDB.json that has nan and inf values in it.
        double value = 0;
        if (fromDataFilter) {
            value = expr->eval(&df->rt, expr, Result(0), 0, item).number();
        } else {
            value = item->getForSymbol(metricName_);
        }
        if (std::isinf(value) || std::isnan(value)) {
            continue;
        }

        const QString sport = normalizeSport(item->sport);

        if (item->planned) {
            plannedAll[offset] += value;
            if (fullScope) {
                sportSeries(plannedBy, sport)[offset] += value;
            }
        } else {
            actualAll[offset] += value;
            if (fullScope) {
                sportSeries(actualBy, sport)[offset] += value;
            }
        }

        if (offset == todayIdx) {
            // Collect todays stress separately to decide later whether to use planned or actual stress
            haveToday = true;
            if (item->planned) {
                todayPlannedStress += value;
                if (fullScope) {
                    todayPlannedBySport[sport] += value;
                }
            } else {
                todayActualStress += value;
                if (fullScope) {
                    todayActualBySport[sport] += value;
                }
            }
        } else if (offset > todayIdx) {
            // future: planned activities that are not already done
            if (item->planned && ! item->hasLinkedActivity()) {
                expectedAll[offset] += value;
                if (fullScope) {
                    sportSeries(expectedBy, sport)[offset] += value;
                }
            }
        } else {
            // past: what really happened
            if (! item->planned) {
                expectedAll[offset] += value;
                if (fullScope) {
                    sportSeries(expectedBy, sport)[offset] += value;
                }
            }
        }
    }

    // Special case today: Use actual stress if available, otherwise planned.
    if (haveToday) {
        const bool useActual = todayActualStress > 0;
        expectedAll[todayIdx] = useActual ? todayActualStress : todayPlannedStress;
        if (fullScope) {
            const QHash<QString, double> &todayBySport = useActual ? todayActualBySport : todayPlannedBySport;
            for (QHash<QString, double>::const_iterator it = todayBySport.cbegin(); it != todayBySport.cend(); ++it) {
                sportSeries(expectedBy, it.key())[todayIdx] = it.value();
            }
        }
    }

    delete df;

    commitSeries(SeriesType::Actual, std::move(actualAll), actualBy);
    commitSeries(SeriesType::Planned, std::move(plannedAll), plannedBy);
    commitSeries(SeriesType::Expected, std::move(expectedAll), expectedBy);

    macroStale = true;
    bool macroFilled = fillMacro();

    isstale=false;

    PMC_TIMER_LOG

#if defined(DEV_GNUPLOT)
    const QString gpBaseDir = "/tmp/gp/";
    if (macroFilled && context->currentSeason() != nullptr) {
        writeGnuplot(gpBaseDir + "actual.gnuplot", SeriesType::Actual, context->currentSeason()->getStart(), context->currentSeason()->getEnd());
        writeGnuplot(gpBaseDir + "planned.gnuplot", SeriesType::Planned, context->currentSeason()->getStart(), context->currentSeason()->getEnd());
        writeGnuplot(gpBaseDir + "expected.gnuplot", SeriesType::Expected, context->currentSeason()->getStart(), context->currentSeason()->getEnd());
        writeGnuplot(gpBaseDir + "macroExpectedTarget.gnuplot", SeriesType::MacroExpectedTarget, context->currentSeason()->getStart(), context->currentSeason()->getEnd());
        writeGnuplot(gpBaseDir + "macroExpectedSlotted.gnuplot", SeriesType::MacroExpectedSlotted, context->currentSeason()->getStart(), context->currentSeason()->getEnd());

        writeGnuplot(gpBaseDir + "actual-bike.gnuplot", SeriesType::Actual, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), "Bike");
        writeGnuplot(gpBaseDir + "planned-bike.gnuplot", SeriesType::Planned, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), "Bike");
        writeGnuplot(gpBaseDir + "expected-bike.gnuplot", SeriesType::Expected, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), "Bike");
        writeGnuplot(gpBaseDir + "macroExpectedSlotted-bike.gnuplot", SeriesType::MacroExpectedSlotted, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), "Bike");

        writeGnuplot(gpBaseDir + "actual-hiking.gnuplot", SeriesType::Actual, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), "Hiking");
        writeGnuplot(gpBaseDir + "planned-hiking.gnuplot", SeriesType::Planned, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), "Hiking");
        writeGnuplot(gpBaseDir + "expected-hiking.gnuplot", SeriesType::Expected, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), "Hiking");
        writeGnuplot(gpBaseDir + "macroExpectedSlotted-hiking.gnuplot", SeriesType::MacroExpectedSlotted, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), "Hiking");

        writeGnuplot(gpBaseDir + "actual-other.gnuplot", SeriesType::Actual, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), otherSport());
        writeGnuplot(gpBaseDir + "planned-other.gnuplot", SeriesType::Planned, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), otherSport());
        writeGnuplot(gpBaseDir + "expected-other.gnuplot", SeriesType::Expected, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), otherSport());
        writeGnuplot(gpBaseDir + "macroExpectedSlotted-other.gnuplot", SeriesType::MacroExpectedSlotted, context->currentSeason()->getStart(), context->currentSeason()->getEnd(), otherSport());
    }
#endif
}


PMCData::SeriesT
PMCData::buildEmptySeries
(int size) const
{
    SeriesT series;
    series.resize(size);
    series.fill(0);
    return series;
}


void
PMCData::eraseSeries
(SeriesType series)
{
    AllSeriesT::iterator it = seriesData.begin();
    while (it != seriesData.end()) {
        if (it.key().series == series) {
            it = seriesData.erase(it);
        } else {
            ++it;
        }
    }
}


int
PMCData::seedOffset
(const Season &season) const
{
    if (! season.getSeed()) {
        return -1;
    }
    const int offset = start_.daysTo(season.getStart());
    return (offset >= 0 && offset < days_) ? offset : -1;
}


PMCData::SeedMap
PMCData::seedsFor
(SeriesType series) const
{
    // actual and planned: every seed; expected and macro: only seeds that have started
    const bool pastOnly = series != SeriesType::Actual && series != SeriesType::Planned;
    const int todayIdx = start_.daysTo(QDate::currentDate());
    SeedMap seeds;
    for (Season const *season : context->athlete->seasons->seededSeasons()) {
        const int offset = seedOffset(*season);
        if (offset < 0 || (pastOnly && offset > todayIdx)) {
            continue;
        }
        seeds.insert(offset, season->getSeed());
    }
    return seeds;
}


void
PMCData::commitSeries
(SeriesType series, SeriesT globalStress, SeriesGroupT &sportStress)
{
    // first all inserts...
    seriesData.insert(SeriesKey(series, ValueType::Stress), std::move(globalStress));
    for (SeriesGroupT::iterator it = sportStress.begin(); it != sportStress.end(); ++it) {
        seriesData.insert(SeriesKey(series, ValueType::Stress, it.key()), std::move(it.value()));
    }

    // ...then derive lts/sts/sb/rr
    deriveSeries(series, QString(), seedsFor(series));
    for (SeriesGroupT::const_iterator it = sportStress.cbegin(); it != sportStress.cend(); ++it) {
        deriveSeries(series, it.key(), SeedMap());
    }
}


void
PMCData::deriveSeries
(SeriesType series, const QString &sport, const SeedMap &seeds)
{
    const SeriesKey stressKey(series, ValueType::Stress, sport);
    const AllSeriesT::const_iterator found = seriesData.constFind(stressKey);
    if (days_ <= 0 || found == seriesData.constEnd()) {
        return;
    }
    const SeriesT stress = found.value();

    SeriesT lts = buildEmptySeries(days_);
    SeriesT sts = buildEmptySeries(days_);
    SeriesT sb = buildEmptySeries(days_ + 1);
    SeriesT rr = buildEmptySeries(days_);
    calculateMetrics(days_, stress, seeds, lts, sts, sb, rr);

    seriesData.insert(stressKey.withValue(ValueType::Lts), std::move(lts));
    seriesData.insert(stressKey.withValue(ValueType::Sts), std::move(sts));
    seriesData.insert(stressKey.withValue(ValueType::Sb), std::move(sb));
    seriesData.insert(stressKey.withValue(ValueType::Rr), std::move(rr));
}


void
PMCData::calculateMetrics
(int days, const SeriesT &stress, const SeedMap &seeds, SeriesT &lts, SeriesT &sts, SeriesT &sb, SeriesT &rr) const
{
    double rollingStress = 0;

    for (int day = 0; day < days; day++) {
        const SeedMap::const_iterator seed = seeds.constFind(day);
        if (seed != seeds.constEnd()) {
            // seeded: the series restarts with the seed value (0 is a valid seed)
            lts[day] = seed.value();
            sts[day] = seed.value();
        } else {
            // LTS
            const double lastLTS = day ? lts[day-1] : 0.0;
            lts[day] = (stress[day] * (1.0 - lte_)) + (lastLTS * lte_);

            // STS
            const double lastSTS = day ? sts[day-1] : 0.0;
            sts[day] = (stress[day] * (1.0 - ste_)) + (lastSTS * ste_);
        }

        // rolling stress for STS days
        if (day && day <= stsDays_) {
            // just starting out
            rollingStress += lts[day] - lts[day-1];
            rr[day] = rollingStress;
        } else if (day) {
            rollingStress += lts[day] - lts[day-1];
            rollingStress -= lts[day-stsDays_] - lts[day-stsDays_-1];
            rr[day] = rollingStress;
        }

        // SB (stress balance)  long term - short term
        // We allow it to be shown today or tomorrow where
        // most (sane/thinking) folks usually show SB on the following day
        sb[day + (sbToday_ ? 0 : 1)] =  lts[day] - sts[day];
    }
}


bool
PMCData::fillMacro
()
{
    if (! macroStale || ! fullScope) {
        return false;
    }
    PMC_TIMER_START
    macroStale = false;
    updateDecay();

    eraseSeries(SeriesType::MacroExpectedTarget);
    eraseSeries(SeriesType::MacroExpectedSlotted);

    Season const *season = context->currentSeason();
    if (   days_ == 0
        || season == nullptr
        || ! season->isMacrocycle()
        || specification_.isFiltered()
        || ! macroMetricMatches(season)) {
        return false;
    }

    // Sports that need a slotted series:
    // - sports with slots
    // - sports with expected history, so their (decaying) past is kept
    QSet<QString> sports;
    for (const Phase &phase : season->phases) {
        const Mesocycle *meso = phase.getMesocycle();
        if (! meso) {
            continue;
        }
        for (const QString &sport : meso->getSlottedSports()) {
            sports.insert(normalizeSport(sport));
        }
        for (auto [date, daySports] : meso->getSlotDateSports().asKeyValueRange()) {
            Q_UNUSED(date)
            for (const QString &sport : daySports) {
                sports.insert(normalizeSport(sport));
            }
        }
    }
    for (AllSeriesT::const_iterator it = seriesData.constBegin(); it != seriesData.constEnd(); ++it) {
        const SeriesKey &key = it.key();
        if (   key.series == SeriesType::Expected
            && key.value == ValueType::Stress
            && ! key.sport.isEmpty()) {
            sports.insert(key.sport);
        }
    }

    // Local series, moved into seriesData at the end
    SeriesT target = buildEmptySeries(days_);
    SeriesT slotted = buildEmptySeries(days_);
    SeriesGroupT slottedBy;
    for (const QString &sport : sports) {
        slottedBy.insert(sport, buildEmptySeries(days_));
    }

    // Copy past stress: target + slotted + slotted sports
    const int todayIdx = start_.daysTo(QDate::currentDate());
    const int copyCount = std::min(todayIdx, days_ - 1) + 1;
    if (copyCount > 0) {
        const SeriesT expected = seriesData.value(SeriesKey(SeriesType::Expected, ValueType::Stress));
        if (expected.size() >= copyCount) {
            std::copy_n(expected.constData(), copyCount, target.data());
            std::copy_n(expected.constData(), copyCount, slotted.data());
        }
        for (SeriesGroupT::iterator it = slottedBy.begin(); it != slottedBy.end(); ++it) {
            // a sport without expected series has no history, its past stays 0
            const AllSeriesT::const_iterator sportExpected = seriesData.constFind(SeriesKey(SeriesType::Expected, ValueType::Stress, it.key()));
            if (sportExpected != seriesData.constEnd() && sportExpected->size() >= copyCount) {
                std::copy_n(sportExpected->constData(), copyCount, it.value().data());
            }
        }
    }

    // Copy macro-planned stress
    for (const Phase &phase : season->phases) {
        const Mesocycle *meso = phase.getMesocycle();
        if (! meso) {
            continue;
        }

        // Target stress
        for (int mc = 0; mc < meso->getMaxMicrocycles(); ++mc) {
            if (! meso->hasMicrocycle(mc)) {
                continue;
            }
            const double dailyMicroLoad = meso->getMicrocycle(mc)->getLoad() / 7.0;
            const int first = start_.daysTo(phase.getStart().addDays(mc * 7));
            for (int d = 0; d < 7; ++d) {
                const int idx = first + d;
                if (idx > todayIdx && idx >= 0 && idx < days_) {
                    target[idx] += dailyMicroLoad;
                }
            }
        }

        // Slotted stress (global + relevant sports)
        for (auto [date, daySports] : meso->getSlotDateSports().asKeyValueRange()) {
            const int idx = start_.daysTo(date);
            if (idx <= todayIdx || idx < 0 || idx >= days_) {
                continue;
            }
            for (const QString &sport : daySports) {
                const Slot *slot = meso->getSlot(date, sport); // the meso knows the original spelling
                if (! slot) {
                    continue;
                }
                const int slotLoad = slot->getLoad();
                if (slotLoad <= 0) {
                    continue;
                }
                SeriesGroupT::iterator dst = slottedBy.find(normalizeSport(sport));
                Q_ASSERT(dst != slottedBy.end());
                if (dst == slottedBy.end()) {
                    continue;
                }
                slotted[idx] += slotLoad;
                dst.value()[idx] += slotLoad;
            }
        }
    }

    // First all inserts, then derive lts/sts/sb/rr (macro series are never seeded)
    seriesData.insert(SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Stress), std::move(target));
    seriesData.insert(SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Stress), std::move(slotted));
    for (SeriesGroupT::iterator it = slottedBy.begin(); it != slottedBy.end(); ++it) {
        seriesData.insert(SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Stress, it.key()), std::move(it.value()));
    }
    deriveSeries(SeriesType::MacroExpectedTarget, QString(), seedsFor(SeriesType::MacroExpectedTarget));
    deriveSeries(SeriesType::MacroExpectedSlotted, QString(), seedsFor(SeriesType::MacroExpectedSlotted));
    for (SeriesGroupT::const_iterator it = slottedBy.cbegin(); it != slottedBy.cend(); ++it) {
        deriveSeries(SeriesType::MacroExpectedSlotted, it.key(), SeedMap());
    }

    PMC_TIMER_LOG

    return true;
}


void
PMCData::connectMacroSignals
()
{
    connect(context->athlete->seasons, &Seasons::macroPlanChanged, this, [this](QString seasonId) {
        Season const *season = this->context->currentSeason();
        if (season != nullptr && seasonId == season->idString()) {
            macroStale = true;
        }
    });
    connect(context, &Context::seasonSelected, this, [this](Season const *season, bool changed) {
        Q_UNUSED(season)
        if (changed) {
            macroStale = true;
        }
    });
}


QStringList
PMCData::sports
()
{
    refresh();

    QSet<QString> found;
    for (AllSeriesT::const_iterator it = seriesData.constBegin(); it != seriesData.constEnd(); ++it) {
        if (it.key().value == ValueType::Stress && ! it.key().sport.isEmpty()) {
            found.insert(it.key().sport);
        }
    }
    QStringList list = found.values();
    list.sort();
    return list;
}


PMCData::SeriesT
PMCData::data
(const SeriesKey &key)
{
    refresh();

    // return a temporary all-zero series for missing sport / series
    const AllSeriesT::const_iterator it = seriesData.constFind(key);
    if (it != seriesData.constEnd()) {
        return it.value();
    }
    if (days_ == 0) {
        return SeriesT();
    }
    return buildEmptySeries(key.value == ValueType::Sb ? days_ + 1 : days_);
}


int
PMCData::indexOf(QDate date)
{
    refresh();

    // offset into arrays or -1 if invalid
    int returning = start_.daysTo(date);
    if (returning < 0 || returning >= days_)
        returning = -1;

    return returning;

}


double
PMCData::stress(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Actual, ValueType::Stress, sport));
}

double
PMCData::lts(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Actual, ValueType::Lts, sport));
}

double
PMCData::sts(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Actual, ValueType::Sts, sport));
}

double
PMCData::sb(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Actual, ValueType::Sb, sport));
}

double
PMCData::rr(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Actual, ValueType::Rr, sport));
}


double
PMCData::plannedStress(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Planned, ValueType::Stress, sport));
}

double
PMCData::plannedLts(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Planned, ValueType::Lts, sport));
}

double
PMCData::plannedSts(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Planned, ValueType::Sts, sport));
}

double
PMCData::plannedSb(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Planned, ValueType::Sb, sport));
}

double
PMCData::plannedRr(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Planned, ValueType::Rr, sport));
}


double
PMCData::expectedStress(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Expected, ValueType::Stress, sport));
}

double
PMCData::expectedLts(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Expected, ValueType::Lts, sport));
}

double
PMCData::expectedSts(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Expected, ValueType::Sts, sport));
}

double
PMCData::expectedSb(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Expected, ValueType::Sb, sport));
}

double
PMCData::expectedRr(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::Expected, ValueType::Rr, sport));
}


double
PMCData::macroExpectedTargetStress(QDate date)
{
    return data(date, SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Stress));
}

double
PMCData::macroExpectedTargetLts(QDate date)
{
    return data(date, SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Lts));
}

double
PMCData::macroExpectedTargetSts(QDate date)
{
    return data(date, SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Sts));
}

double
PMCData::macroExpectedTargetSb(QDate date)
{
    return data(date, SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Sb));
}

double
PMCData::macroExpectedTargetRr(QDate date)
{
    return data(date, SeriesKey(SeriesType::MacroExpectedTarget, ValueType::Rr));
}


double
PMCData::macroExpectedSlottedStress(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Stress, sport));
}

double
PMCData::macroExpectedSlottedLts(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Lts, sport));
}

double
PMCData::macroExpectedSlottedSts(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Sts, sport));
}

double
PMCData::macroExpectedSlottedSb(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Sb, sport));
}

double
PMCData::macroExpectedSlottedRr(QDate date, const QString &sport)
{
    return data(date, SeriesKey(SeriesType::MacroExpectedSlotted, ValueType::Rr, sport));
}


double
PMCData::data
(const QDate &date, const SeriesKey &key)
{
    // indexOf() refreshes if needed
    const int index = indexOf(date);
    if (index == -1) {
        return 0.0;
    }
    const AllSeriesT::const_iterator it = seriesData.constFind(key);
    if (it == seriesData.constEnd() || index >= it->size()) {
        return 0.0; // fallback in case of no series
    }
    return (*it)[index];
}


// rag reporting according to wattage type groupthink
QColor PMCData::ltsColor(double value, QColor defaultColor)
{
    //if (value < 30) return QColor(Qt::red);
    //if (value < 80) return QColor(Qt::yellow);
    if (value > 75) return QColor(Qt::green);
    if (value > 100) return QColor(Qt::blue);
    return defaultColor;
}

QColor PMCData::stsColor(double, QColor defaultColor)
{
    return defaultColor; // nowt is wrong, rest or peak who can tell ?
}

QColor PMCData::sbColor(double value, QColor defaultColor)
{
    if (value < -40) return QColor(Qt::red); // injury risk
    if (value > 15) return QColor(Qt::yellow); // detraining risk
    if (value >= -5 && value < 15) return QColor(Qt::green); // ideal
    return defaultColor;
}

QColor PMCData::rrColor(double value, QColor defaultColor)
{
    if (value < -4 || value > 8) return QColor(Qt::red); // too fast or detraining
    //if (value < 0) return QColor(Qt::yellow); // risk of losing fitness
    return defaultColor;
}

// User descriptions for the 4 series
QString PMCData::ltsDescription()
{
    return tr("CTL/LTS : Chronic Training Load/Long Term Stress. The dose of training you accumulated over a longer period of time, computed as an exponentially weighted moving average of the selected Training Load metric typically from 4-8 weeks, 42 days by default. It is claimed to relate to your fitness.");
}

QString PMCData::stsDescription()
{
    return tr("ATL/STS : Acute Training Load/Short Term Stress. The dose of training that you accumulated over a short period of time, computed as an exponentially weighted moving average of the selected Training Load metric from 3 to 10 days in general, 7 by default. It is claimed to relate to your fatigue.");
}

QString PMCData::sbDescription()
{
    return tr("TSB/SB : Training Stress Balance/Stress Balance. It's the result of subtracting yesterday's Acute Training Load/Short Term Stress from yesterday's Chronic Training Load/Long Term Stress. It is claimed to relate to your freshness.");
}

QString PMCData::rrDescription()
{
    return tr("RR : Ramp Rate. The rate at which CTL/LTS increases over a given time period. Large values up and down indicate a risk of injury and aggressive taper respectively.");
}


QDate
PMCData::minValid
(const QDate &date1, const QDate &date2) const
{
    if (date1.isValid() && date2.isValid()) {
        return std::min(date1, date2);
    } else if (date1.isValid()) {
        return date1;
    } else {
        return date2;
    }
}


QDate
PMCData::maxValid
(const QDate &date1, const QDate &date2) const
{
    if (date1.isValid() && date2.isValid()) {
        return std::max(date1, date2);
    } else if (date1.isValid()) {
        return date1;
    } else {
        return date2;
    }
}


#if defined(DEV_GNUPLOT)

namespace {
QString
seriesTitle
(PMCData::SeriesType series)
{
    switch (series) {
    case PMCData::SeriesType::Actual:
        return QStringLiteral("Actual");
    case PMCData::SeriesType::Planned:
        return QStringLiteral("Planned");
    case PMCData::SeriesType::Expected:
        return QStringLiteral("Expected");
    case PMCData::SeriesType::MacroExpectedTarget:
        return QStringLiteral("MacroExpectedTarget");
    case PMCData::SeriesType::MacroExpectedSlotted:
        return QStringLiteral("MacroExpectedSlotted");
    }
    return QStringLiteral("Unknown");
}
}

QString
PMCData::gnuplotScript
(SeriesType series, const QDate &from, const QDate &to, const QString &sport)
{
    if (days_ == 0) {
        return QString();
    }

    const QDate lo = from.isValid() ? std::max(from, start_) : start_;
    const QDate hi = to.isValid() ? std::min(to, end_) : end_;
    if (hi < lo) {
        return QString();
    }
    const int first = start_.daysTo(lo);
    const int last = start_.daysTo(hi);

    const QVector<double> stress = data(SeriesKey(series, ValueType::Stress, sport));
    const QVector<double> lts = data(SeriesKey(series, ValueType::Lts, sport));
    const QVector<double> sts = data(SeriesKey(series, ValueType::Sts, sport));
    const QVector<double> sb = data(SeriesKey(series, ValueType::Sb, sport));
    const QVector<double> rr = data(SeriesKey(series, ValueType::Rr, sport));

    QString sportLabel = sport.isEmpty() ? QStringLiteral("all sports") : sport;
    sportLabel.replace('"', QStringLiteral("\\\""));
    const QString title = QStringLiteral("PMC %1 / %2 (%3 .. %4, STS %5 d, LTS %6 d)")
                              .arg(seriesTitle(series), sportLabel, lo.toString(Qt::ISODate), hi.toString(Qt::ISODate))
                              .arg(stsDays_)
                              .arg(ltsDays_);

    auto num = [](double v) { return QString::number(v, 'f', 3); };

    QString out;
    QTextStream s(&out);
    s << "# " << title << "\n";
    s << "$pmc << EOD\n";
    s << "# date stress lts sts sb rr\n";
    for (int i = first; i <= last; ++i) {
        s << start_.addDays(i).toString(Qt::ISODate) << ' '
          << num(stress.value(i)) << ' '
          << num(lts.value(i)) << ' '
          << num(sts.value(i)) << ' '
          << num(sb.value(i)) << ' '
          << num(rr.value(i)) << "\n";
    }
    s << "EOD\n";

    s << "set title \"" << title << "\"\n";
    s << "set xdata time\n";
    s << "set timefmt \"%Y-%m-%d\"\n";
    s << "set format x \"%m-%d\"\n";
    s << "set xtics rotate by -45\n";
    s << "set grid\n";
    s << "set ytics nomirror\n";
    s << "set y2tics\n";
    s << "set ylabel \"LTS / STS / SB / RR\"\n";
    s << "set y2label \"Stress\"\n";
    s << "set key outside bottom horizontal\n";
    s << "plot $pmc using 1:2 axes x1y2 with impulses lw 2 lc rgb \"#bbbbbb\" title \"Stress\", \\\n";
    s << "     $pmc using 1:3 with lines lw 2 title \"LTS\", \\\n";
    s << "     $pmc using 1:4 with lines lw 2 title \"STS\", \\\n";
    s << "     $pmc using 1:5 with lines lw 2 title \"SB\", \\\n";
    s << "     $pmc using 1:6 with lines lw 1 dashtype 2 title \"RR\"\n";
    return out;
}


void
PMCData::printGnuplot
(SeriesType series, const QDate &from, const QDate &to, const QString &sport)
{
    QTextStream(stdout) << gnuplotScript(series, from, to, sport) << Qt::flush;
}


bool
PMCData::writeGnuplot
(const QString &fileName, SeriesType series, const QDate &from, const QDate &to, const QString &sport)
{
    const QString script = gnuplotScript(series, from, to, sport);
    if (script.isEmpty()) {
        qWarning() << "PMCData: nothing to write for" << fileName << "(empty range or no data)";
        return false;
    }

    QSaveFile file(fileName);
    if (! file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "PMCData: cannot open" << fileName << ":" << file.errorString();
        return false;
    }
    file.write(script.toUtf8());
    if (! file.commit()) {
        qWarning() << "PMCData: cannot write" << fileName << ":" << file.errorString();
        return false;
    }
    return true;
}

#endif
