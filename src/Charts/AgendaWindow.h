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

#ifndef _GC_AgendaWindow_h
#define _GC_AgendaWindow_h

#include "GoldenCheetah.h"

#include <QtGui>
#include <QCheckBox>
#include <QHash>
#include <QList>
#include <QDate>

#include "MetricSelect.h"
#include "Context.h"
#include "Season.h"
#include "Agenda.h"
#include "CalendarData.h"


class AgendaWindow : public GcChartWindow
{
    Q_OBJECT

    Q_PROPERTY(int agendaPastDays READ getAgendaPastDays WRITE setAgendaPastDays USER true)
    Q_PROPERTY(int agendaFutureDays READ getAgendaFutureDays WRITE setAgendaFutureDays USER true)
    Q_PROPERTY(QString primaryMainField READ getPrimaryMainField WRITE setPrimaryMainField USER true)
    Q_PROPERTY(QString primaryFallbackField READ getPrimaryFallbackField WRITE setPrimaryFallbackField USER true)
    Q_PROPERTY(QString secondaryMetric READ getSecondaryMetric WRITE setSecondaryMetric USER true)
    Q_PROPERTY(bool showSecondaryLabel READ isShowSecondaryLabel WRITE setShowSecondaryLabel USER true)
    Q_PROPERTY(int showTertiaryFor READ getShowTertiaryFor WRITE setShowTertiaryFor USER true)
    Q_PROPERTY(QString tertiaryField READ getTertiaryField WRITE setTertiaryField USER true)
    Q_PROPERTY(int activityMaxTertiaryLines READ getActivityMaxTertiaryLines WRITE setActivityMaxTertiaryLines USER true)
    Q_PROPERTY(int eventMaxTertiaryLines READ getEventMaxTertiaryLines WRITE setEventMaxTertiaryLines USER true)

    public:
        AgendaWindow(Context *context);

        int getAgendaPastDays() const;
        int getAgendaFutureDays() const;

        bool isFiltered() const;

        QString getPrimaryMainField() const;
        QString getPrimaryFallbackField() const;
        QString getSecondaryMetric() const;
        bool isShowSecondaryLabel() const;
        int getShowTertiaryFor() const;
        QString getTertiaryField() const;
        int getActivityMaxTertiaryLines() const;
        int getEventMaxTertiaryLines() const;

    public slots:
        void setAgendaPastDays(int days);
        void setAgendaFutureDays(int days);
        void setPrimaryMainField(const QString &name);
        void setPrimaryFallbackField(const QString &name);
        void setSecondaryMetric(const QString &name);
        void setShowSecondaryLabel(bool showSecondaryLabel);
        void setShowTertiaryFor(int showFor);
        void setTertiaryField(const QString &name);
        void setActivityMaxTertiaryLines(int maxTertiaryLines);
        void setEventMaxTertiaryLines(int maxTertiaryLines);
        void configChanged(qint32);

    protected:
        void showEvent(QShowEvent *event) override;

    private:
        Context *context;
        QPalette palette;

        QSpinBox *agendaPastDaysSpin;
        QSpinBox *agendaFutureDaysSpin;
        QComboBox *primaryMainCombo;
        QComboBox *primaryFallbackCombo;
        QComboBox *secondaryCombo;
        QCheckBox *showSecondaryLabelCheck;
        QComboBox *showTertiaryForCombo;
        QComboBox *tertiaryCombo;
        QSpinBox *activityMaxTertiaryLinesSpin;
        QSpinBox *eventMaxTertiaryLinesSpin;
        AgendaView *agendaView = nullptr;

        void mkControls();
        void updatePrimaryConfigCombos();
        void updateSecondaryConfigCombo();
        void updateTertiaryConfigCombo();
        QHash<QDate, QList<CalendarEntry>> getActivities(const QDate &firstDay, const QDate &today, const QDate &lastDay) const;
        std::pair<QList<CalendarEntry>, QList<CalendarEntry>> getPhases(const Season &season, const QDate &firstDay) const;
        QHash<QDate, QList<CalendarEntry>> getEvents(const QDate &firstDay) const;

    private slots:
        void updateActivities();
        void updateActivitiesIfInRange(RideItem *rideItem);
        void editPhaseEntry(const CalendarEntry &entry);
        void editEventEntry(const CalendarEntry &entry);
};

#endif
