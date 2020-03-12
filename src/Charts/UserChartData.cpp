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

UserChartData::UserChartData(Context *context, QString script) : context(context), script(script)
{
    // compile the program - built in a context that can close.
    program = new DataFilter(NULL, context, script);
    root = program->root();
    rt = &program->rt;

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
    if (!root)  return;

    // clear rt indexes
    rt->indexes.clear();

    // clear previous data
    relevant=x=y=z=d=t=Result(0);

    // always init first
    if (finit) root->eval(rt, finit, 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);

    // is it relevant ?
    if (frelevant) {
        relevant = root->eval(rt, frelevant, 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
        if (relevant.number == 0) return;
    }

    // process samples, if there are any and a function exists
    if (!spec.isEmpty(item->ride()) && fsample) {
        RideFileIterator it(item->ride(), spec);

        while(it.hasNext()) {
            struct RideFilePoint *point = it.next();
            root->eval(rt, fsample, 0, const_cast<RideItem*>(item), point, NULL, spec, dr);
        }
    }

    // finalise computation
    if (ffinalise) {
        root->eval(rt, ffinalise, 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    }

    // values
    if (fx) x = root->eval(rt, fx, 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    if (fy) y = root->eval(rt, fy, 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    if (fz) z = root->eval(rt, fz, 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    if (ft) t = root->eval(rt, ft, 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);
    if (fd) d = root->eval(rt, fd, 0, const_cast<RideItem*>(item), NULL, NULL, spec, dr);

}
