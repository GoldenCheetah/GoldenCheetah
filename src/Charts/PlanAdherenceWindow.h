/*
 * Copyright (c) 2026 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _GC_PlanAdherenceWindow_h
#define _GC_PlanAdherenceWindow_h

#include "GoldenCheetah.h"

#include <QtGui>

#include "Context.h"
#include "PlanAdherence.h"


class PlanAdherenceWindow : public GcChartWindow
{
    Q_OBJECT

    Q_PROPERTY(QString titleMainField READ getTitleMainField WRITE setTitleMainField USER true)
    Q_PROPERTY(QString titleFallbackField READ getTitleFallbackField WRITE setTitleFallbackField USER true)
    Q_PROPERTY(int maxDaysBefore READ getMaxDaysBefore WRITE setMaxDaysBefore USER true)
    Q_PROPERTY(int mayDaysAfter READ getMaxDaysAfter WRITE setMaxDaysAfter USER true)
    Q_PROPERTY(int preferredStatisticsDisplay READ getPreferredStatisticsDisplay WRITE setPreferredStatisticsDisplay USER true)

    public:
        PlanAdherenceWindow(Context *context);

        QString getTitleMainField() const;
        QString getTitleFallbackField() const;
        int getMaxDaysBefore() const;
        int getMaxDaysAfter() const;
        int getPreferredStatisticsDisplay() const;

    public slots:
        void setTitleMainField(const QString &name);
        void setTitleFallbackField(const QString &name);
        void setMaxDaysBefore(int days);
        void setMaxDaysAfter(int days);
        void setPreferredStatisticsDisplay(int mode);

        void configChanged(qint32);

    private:
        Context *context;
        PlanAdherenceView *adherenceView = nullptr;
        bool first = true;

        QPalette palette;

        QComboBox *titleMainCombo;
        QComboBox *titleFallbackCombo;
        QSpinBox *maxDaysBeforeSpin;
        QSpinBox *maxDaysAfterSpin;
        QComboBox *preferredStatisticsDisplay;

        bool isFiltered() const;
        void mkControls();
        void updateTitleConfigCombos();
        QString getRideItemTitle(RideItem const * const rideItem) const;

    private slots:
        void updateActivities();
        void updateActivitiesIfInRange(RideItem *rideItem);
};

#endif
