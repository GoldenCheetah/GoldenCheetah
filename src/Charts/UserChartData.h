/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_UserChartData_h
#define _GC_UserChartData_h 1

#include "Context.h"
#include "DataFilter.h"
#include "Specification.h"

class UserChart;
class UserChartData : public QObject {

    Q_OBJECT

public:

    // new program
    UserChartData(Context *context, UserChart *parent, QString script, bool rangemode);
    ~UserChartData();

    // Compute from samples
    void compute(RideItem *item, Specification spec, DateRange=DateRange());

    // the results ...
    Result relevant, x, y, z, t, d, f;

    Context *context;

    // script and "compiled" program
    QString script;
    bool rangemode;
    DataFilter *program;
    Leaf *root;

    // functions, to save lots of lookups
    Leaf *frelevant, *finit, *fsample, *factivity, *ffinalise, *fx, *fy, *fz, *ft, *fd, *ff;

    // our runtime
    DataFilterRuntime *rt;

    // our owner
    UserChart *parent;

};

#endif // _GC_UserChartData_h

