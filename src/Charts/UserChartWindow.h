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

#ifndef _GC_UserChartWindow_h
#define _GC_UserChartWindow_h 1

#include "GoldenCheetah.h"
#include "UserChart.h"
#include "Context.h"
#include "Athlete.h"
#include "Colors.h"

class UserChartWindow : public GcChartWindow {

    Q_OBJECT

    // settings are maintained by the userchart widget, we are just a wrapper to include in a perspective.
    Q_PROPERTY(QString settings READ settings WRITE applySettings USER true)
    Q_PROPERTY(QString color READ readColor USER false)

    public:

        UserChartWindow(Context *context, bool rangemode);

        // for read and write of settings via chart properties
        QString settings() const { return chart->settings(); }
        void applySettings(QString x) { chart->applySettings(x); }
        QString readColor() const { return chart->chartinfo.bgcolor.name(); }

    public slots:

        // runtime - ride item changed
        void setRide(RideItem*);

        // runtime - date range was selected
        void setDateRange(DateRange);

        // watch color changes
        void configChanged();

        // redraw
        void refresh();

    private:

        Context *context;
        bool rangemode;
        bool stale;
        RideItem *last; // the last ride we plotted

        RideItem *ride;
        DateRange dr;

        UserChart *chart;
        UserChartSettings *settingsTool_;
};
#endif
