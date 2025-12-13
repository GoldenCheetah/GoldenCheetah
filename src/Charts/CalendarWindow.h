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

#ifndef _GC_CalendarWindow_h
#define _GC_CalendarWindow_h

#include "GoldenCheetah.h"

#include <QtGui>
#include <QCheckBox>
#include <QHash>
#include <QList>
#include <QDate>

#include "MetricSelect.h"
#include "Context.h"
#include "Season.h"
#include "Calendar.h"
#include "CalendarData.h"


class CalendarWindow : public GcChartWindow
{
    Q_OBJECT

    Q_PROPERTY(int defaultView READ getDefaultView WRITE setDefaultView USER true)
    Q_PROPERTY(int firstDayOfWeek READ getFirstDayOfWeek WRITE setFirstDayOfWeek USER true)
    Q_PROPERTY(int startHour READ getStartHour WRITE setStartHour USER true)
    Q_PROPERTY(int endHour READ getEndHour WRITE setEndHour USER true)
    Q_PROPERTY(bool summaryVisibleDay READ isSummaryVisibleDay WRITE setSummaryVisibleDay USER true)
    Q_PROPERTY(bool summaryVisibleWeek READ isSummaryVisibleWeek WRITE setSummaryVisibleWeek USER true)
    Q_PROPERTY(bool summaryVisibleMonth READ isSummaryVisibleMonth WRITE setSummaryVisibleMonth USER true)
    Q_PROPERTY(QString primaryMainField READ getPrimaryMainField WRITE setPrimaryMainField USER true)
    Q_PROPERTY(QString primaryFallbackField READ getPrimaryFallbackField WRITE setPrimaryFallbackField USER true)
    Q_PROPERTY(QString secondaryMetric READ getSecondaryMetric WRITE setSecondaryMetric USER true)
    Q_PROPERTY(bool showSecondaryLabel READ isShowSecondaryLabel WRITE setShowSecondaryLabel USER true)
    Q_PROPERTY(QString tertiaryField READ getTertiaryField WRITE setTertiaryField USER true)
    Q_PROPERTY(QString summaryMetrics READ getSummaryMetrics WRITE setSummaryMetrics USER true)

    public:
        CalendarWindow(Context *context);

        int getDefaultView() const;
        int getFirstDayOfWeek() const;
        int getStartHour() const;
        int getEndHour() const;
        bool isSummaryVisibleDay() const;
        bool isSummaryVisibleWeek() const;
        bool isSummaryVisibleMonth() const;

        bool isFiltered() const;

        QString getPrimaryMainField() const;
        QString getPrimaryFallbackField() const;
        QString getSecondaryMetric() const;
        bool isShowSecondaryLabel() const;
        QString getTertiaryField() const;
        QString getSummaryMetrics() const;
        QStringList getSummaryMetricsList() const;

    public slots:
        void setDefaultView(int view);
        void setFirstDayOfWeek(int fdw);
        void setStartHour(int hour);
        void setEndHour(int hour);
        void setSummaryVisibleDay(bool visible);
        void setSummaryVisibleWeek(bool visible);
        void setSummaryVisibleMonth(bool svm);
        void setPrimaryMainField(const QString &name);
        void setPrimaryFallbackField(const QString &name);
        void setSecondaryMetric(const QString &name);
        void setShowSecondaryLabel(bool showSecondaryLabel);
        void setTertiaryField(const QString &name);
        void setSummaryMetrics(const QString &summaryMetrics);
        void setSummaryMetrics(const QStringList &summaryMetrics);
        void configChanged(qint32);

    protected:

        void showEvent(QShowEvent*) override;

    private:
        Context *context;
        bool first = true;

        QPalette palette;
        QComboBox *defaultViewCombo;
        QComboBox *firstDayOfWeekCombo;
        QSpinBox *startHourSpin;
        QSpinBox *endHourSpin;
        QCheckBox *summaryDayCheck;
        QCheckBox *summaryWeekCheck;
        QCheckBox *summaryMonthCheck;
        QComboBox *primaryMainCombo;
        QComboBox *primaryFallbackCombo;
        QComboBox *secondaryCombo;
        QCheckBox *showSecondaryLabelCheck;
        QComboBox *tertiaryCombo;
        MultiMetricSelector *multiMetricSelector;
        Calendar *calendar = nullptr;

        void mkControls();
        void updatePrimaryConfigCombos();
        void updateSecondaryConfigCombo();
        void updateTertiaryConfigCombo();
        QHash<QDate, QList<CalendarEntry>> getActivities(const QDate &firstDay, const QDate &lastDay) const;
        QList<CalendarSummary> getSummaries(const QDate &firstDay, const QDate &lastDay, int timeBucketSize = 7) const;
        QHash<QDate, QList<CalendarEntry>> getPhasesEvents(const Season &season, const QDate &firstDay, const QDate &lastDay) const;

    private slots:
        void updateActivities();
        void updateActivitiesIfInRange(RideItem *rideItem);
        void updateSeason(Season const *season, bool allowKeepMonth = false);
        bool movePlannedActivity(RideItem *rideItem, const QDate &destDay, const QTime &destTime = QTime());
};

#endif
