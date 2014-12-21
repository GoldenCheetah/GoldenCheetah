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
#include "Context.h"

#include <stdio.h>

#include <QSharedPointer>
#include <QProgressDialog>

PMCData::PMCData(Context *context, Specification spec, QString metricName, int stsDays, int ltsDays) 
    : context(context), specification_(spec), metricName_(metricName), stsDays_(stsDays), ltsDays_(ltsDays), isstale(true)
{
    refresh();
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(invalidate()));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(invalidate()));
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(invalidate()));
}

void PMCData::invalidate()
{
    isstale=true;
}

void PMCData::refresh()
{
    if (!isstale) return;

    QTime timer;
    timer.start();

    //
    // STEP ONE: What is the date range ?
    //

    // Date range needs to take into account seasons that
    // have a starting LTS/STS potentially before any rides
    QDate seed;
    foreach(Season x, context->athlete->seasons->seasons)
        if (x.getSeed() && (seed == QDate() || x.getStart() < seed))
            seed = x.getStart();
    
    // take into account any rides, some might be before
    // the start of the first defined season
    QDate first, last;
    if (context->athlete->rideCache->rides().count()) {

        // set date range - extend to a year after last ride
        first = context->athlete->rideCache->rides().first()->dateTime.date();
        last = context->athlete->rideCache->rides().last()->dateTime.date();
    }

    // what is earliest date we got ?
    start_ = QDate(9999,12,31);
    if (seed != QDate() && seed < start_) start_ = seed;
    if (first != QDate() && first < start_) start_ = first;

    // whats the latest date we got ? (and add a year for decay)
    end_ = QDate();
    if (last > seed) end_ = last.addDays(365);
    else if (seed != QDate()) end_ = seed.addDays(365);

    // back to null date if not set, just to get round date arithmetic
    if (start_ == QDate(9999,12,31)) start_ = QDate();

    // We got a valid range ?
    if (start_ != QDate() && end_ != QDate() && start_ < end_) {

        // resize arrays
        days_ = start_.daysTo(end_)+1;
        stress_.resize(days_);
        lts_.resize(days_);
        sts_.resize(days_);
        sb_.resize(days_+1); // for SB tomorrow!

    } else {

        // nothing to calculate
        start_= QDate();
        end_ = QDate();
        days_ = 0;
        stress_.resize(0);
        lts_.resize(0);
        sts_.resize(0);
        sb_.resize(0);

        // give up
        return;
    }
    //qDebug()<<"refresh PMC dates:"<<metricName_<<"days="<<days_<<"start="<<start_<<"end="<<end_;

    //
    // STEP TWO What are the seedings and ride values
    //
    bool sbToday = appsettings->cvalue(context->athlete->cyclist, GC_SB_TODAY).toInt();
    double lte = (double)exp(-1.0/ltsDays_);
    double ste = (double)exp(-1.0/stsDays_);

    // clear what's there
    stress_.fill(0);
    lts_.fill(0);
    sts_.fill(0);
    sb_.fill(0);

    // add the seeded values from seasons
    foreach(Season x, context->athlete->seasons->seasons) {
        if (x.getSeed()) {
            int offset = start_.daysTo(x.getStart());
            lts_[offset] = x.getSeed() * -1;
            sts_[offset] = x.getSeed() * -1;
        }
    }

    // add the stress scores
    foreach(RideItem *item, context->athlete->rideCache->rides()) {

        if (!specification_.pass(item)) continue;

        // seed with score for this one
        int offset = start_.daysTo(item->dateTime.date());
        if (offset > 0 && offset < stress_.count())
        stress_[offset] += item->getForSymbol(metricName_);
    }

    //
    // STEP THREE Calculate sts/lts and sb
    //
    double lastLTS=0.0f;
    double lastSTS=0.0f;

    for(int day=0; day < days_; day++) {

        // not seeded
        if (lts_[day] >=0 || sts_[day]>=0) {

            // LTS
            if (day) lastLTS = lts_[day-1];
            lts_[day] = (stress_[day] * (1.0 - lte)) + (lastLTS * lte);

            // STS
            if (day) lastSTS = sts_[day-1];
            sts_[day] = (stress_[day] * (1.0 - ste)) + (lastSTS * ste);

        } else if (lts_[day]< 0|| sts_[day]<0) {

            lts_[day] *= -1;
            sts_[day] *= -1;
        }

        // SB (stress balance)  long term - short term
        // We allow it to be shown today or tomorrow where
        // most (sane/thinking) folks usually show SB on the following day
        sb_[day+(sbToday ? 0 : 1)] =  lts_[day] - sts_[day];
    }

    //qDebug()<<"refresh PMC in="<<timer.elapsed()<<"ms";

    isstale=false;
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
PMCData::lts(QDate date)
{
    refresh();

    int index=indexOf(date);
    if (index == -1) return 0.0f;
    else return lts_[index];
}

double
PMCData::sts(QDate date)
{
    refresh();

    int index=indexOf(date);
    if (index == -1) return 0.0f;
    else return sts_[index];
}

double
PMCData::stress(QDate date)
{
    refresh();

    int index=indexOf(date);
    if (index == -1) return 0.0f;
    else return stress_[index];
}

double
PMCData::sb(QDate date)
{
    refresh();

    int index=indexOf(date);
    if (index == -1) return 0.0f;
    else return sb_[index];
}
