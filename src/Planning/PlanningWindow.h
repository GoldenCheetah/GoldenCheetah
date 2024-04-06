/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_PlanningWindow_h
#define _GC_PlanningWindow_h 1

// basics
#include "GoldenCheetah.h"
#include "Settings.h"
#include "Units.h"
#include "Colors.h"
#include "Context.h"

// trends view
#include "Season.h" // for data series types
#include "AbstractView.h"  // stylesheet for scroller

// qt
#include <QtGui>

class PlanningWindow : public GcChartWindow
{
    Q_OBJECT

    public:

        PlanningWindow(Context *context);


   public slots:

        // trap signals
        void configChanged(qint32);

        // show hide toolbar if too small
        void resizeEvent(QResizeEvent * event);

    protected:
        bool eventFilter(QObject *obj, QEvent *event);

    private:

        Context *context;

};

#endif // _GC_PlanningWindow_h
