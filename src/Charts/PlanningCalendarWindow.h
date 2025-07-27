/*
 * Copyright (c) 2025 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _GC_PlanningCalendarWindow_h
#define _GC_PlanningCalendarWindow_h

#include "GoldenCheetah.h"

#include <QtGui>
#include <QHash>
#include <QList>
#include <QDate>

#include "Context.h"
#include "Season.h"
#include "Calendar.h"
#include "CalendarData.h"


class PlanningCalendarWindow : public GcChartWindow
{
    Q_OBJECT

    Q_PROPERTY(int weekStartsOn READ getWeekStartsOn WRITE setWeekStartsOn USER true)

    public:
        PlanningCalendarWindow(Context *context);

        int getWeekStartsOn() const;

        bool isFiltered() const;

    public slots:
        void setWeekStartsOn(int wso);
        void configChanged(qint32);

    private:
        Context *context;
        int weekStartsOn = 1; // 0: Sunday, 1: Monday
        bool first = true;

        QComboBox *weekStartCombo;
        Calendar *calendar;

        QHash<QDate, QList<CalendarEntry>> getActivities(const QDate &firstDay, const QDate &lastDay) const;
        QHash<QDate, QList<CalendarEntry>> getPhasesEvents(const Season &season, const QDate &firstDay, const QDate &lastDay) const;

    private slots:
        void updateActivities();
        void updateActivitiesIfVisible(RideItem *rideItem);
        void updateSeason(Season const *season, bool allowKeepMonth = false);
        bool movePlannedActivity(RideItem *rideItem, const QDate &destDay, bool force = false);
};

#endif
