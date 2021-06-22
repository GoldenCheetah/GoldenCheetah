/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#include "RideMetric.h"
#include "UserChartData.h"
#include "DataFilter.h"
#include "Athlete.h"

UserChartData::UserChartData(Context *context, UserChart *parent, QString script, bool rangemode) : context(context), script(script), rangemode(rangemode)
{
    // compile the program - built in a context that can close.
    program = new DataFilter(NULL, context, script);
    root = program->root();
    rt = &program->rt;
    rt->chart = parent;

    // lookup functions we need
    finit = rt->functions.contains("init") ? rt->functions.value("init") : NULL;
    frelevant = rt->functions.contains("relevant") ? rt->functions.value("relevant") : NULL;
    fsample = rt->functions.contains("sample") ? rt->functions.value("sample") : NULL;
    factivity = rt->functions.contains("activity") ? rt->functions.value("activity") : NULL;
    ffinalise = rt->functions.contains("finalise") ? rt->functions.value("finalise") : NULL;
    fx = rt->functions.contains("x") ? rt->functions.value("x") : NULL;
    fy = rt->functions.contains("y") ? rt->functions.value("y") : NULL;
    fz = rt->functions.contains("z") ? rt->functions.value("z") : NULL;
    ft = rt->functions.contains("t") ? rt->functions.value("t") : NULL;
    fd = rt->functions.contains("d") ? rt->functions.value("d") : NULL;
    ff = rt->functions.contains("f") ? rt->functions.value("f") : NULL;

}

UserChartData::~UserChartData()
{
    // program is shared, only delete when last is destroyed
    delete program;
}

// compute when working with an activity / interval
void
UserChartData::compute(RideItem *item, Specification spec, DateRange dr)
{
    if (!root || item == NULL)  return;

    // clear rt indexes
    rt->indexes.clear();

    // clear previous data
    relevant=x=y=z=d=t=Result(0);

    // always init first
    if (finit) root->eval(rt, finit, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);

    // is it relevant ?
    if (frelevant) {
        relevant = root->eval(rt, frelevant, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
        if (relevant.number() == 0) return;
    }

    // process samples, if there are any and a function exists
    if (rangemode) {
        if (factivity) {

            FilterSet fs;
            fs.addFilter(context->isfiltered, context->filters);
            fs.addFilter(context->ishomefiltered, context->homeFilters);
            Specification spec;
            spec.setFilterSet(fs);

            // loop through rides for daterange
            foreach(RideItem *ride, context->athlete->rideCache->rides()) {

                if (!dr.pass(ride->dateTime.date())) continue; // relies upon the daterange being passed to eval...
                if (!spec.pass(ride)) continue; // relies upon the daterange being passed to eval...


                root->eval(rt, factivity, Result(0), 0, const_cast<RideItem*>(ride), NULL, NULL, spec, dr);
            }
        }

    } else {
        if (!spec.isEmpty(item->ride()) && fsample) {
            RideFileIterator it(item->ride(), spec);

            while(it.hasNext()) {
                struct RideFilePoint *point = it.next();
                root->eval(rt, fsample, Result(0), 0, const_cast<RideItem*>(item), point, NULL, spec, dr);
            }
        }

    }

    // finalise computation
    if (ffinalise) {
        root->eval(rt, ffinalise, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    }

    // values
    if (fx) x = root->eval(rt, fx, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    if (fy) y = root->eval(rt, fy, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    if (fz) z = root->eval(rt, fz, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    if (ft) t = root->eval(rt, ft, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    if (fd) d = root->eval(rt, fd, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    if (ff) f = root->eval(rt, ff, Result(0), 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);

}
