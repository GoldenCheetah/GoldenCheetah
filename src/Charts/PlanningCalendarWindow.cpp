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


#include "PlanningCalendarWindow.h"

#include <QComboBox>

#include "MainWindow.h"
#include "AthleteTab.h"
#include "Seasons.h"
#include "Athlete.h"
#include "Colors.h"
#include "ManualActivityWizard.h"

PlanningCalendarWindow::PlanningCalendarWindow(Context *context)
: GcChartWindow(context), context(context)
{
    QWidget *controlsWidget = new QWidget();
    weekStartCombo = new QComboBox();
    weekStartCombo->addItem(tr("Sunday"));
    weekStartCombo->addItem(tr("Monday"));
    QFormLayout *controlsLayout = newQFormLayout(controlsWidget);
    controlsLayout->addRow(tr("Week starts on"), weekStartCombo);
    setControls(controlsWidget);

#if QT_VERSION >= 0x060000
    connect(weekStartCombo, &QComboBox::currentIndexChanged, this, &PlanningCalendarWindow::setWeekStartsOn);
#else
    connect(weekStartCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlanningCalendarWindow::setWeekStartsOn);
#endif

    calendar = new Calendar(QDate::currentDate());

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setChartLayout(mainLayout);
    mainLayout->addWidget(calendar);

    connect(context->athlete->seasons, &Seasons::seasonsChanged, [=]() {
        updateSeason(context->currentSeason(), true);
    });
    connect(context, &Context::seasonSelected, [=](Season const *season, bool changed) {
        if (changed || first) {
            first = false;
            updateSeason(season, false);
        }
    });
    connect(context, &Context::filterChanged, this, &PlanningCalendarWindow::updateActivities);
    connect(context, &Context::homeFilterChanged, this, &PlanningCalendarWindow::updateActivities);
    connect(context, &Context::rideAdded, this, &PlanningCalendarWindow::updateActivitiesIfVisible);
    connect(context, &Context::rideDeleted, this, &PlanningCalendarWindow::updateActivitiesIfVisible);
    connect(context, &Context::configChanged, this, &PlanningCalendarWindow::configChanged);
    connect(calendar, &Calendar::viewActivity, [=](CalendarEntry activity) {
        for (RideItem *rideItem : context->athlete->rideCache->rides()) {
            if (rideItem != nullptr && rideItem->fileName == activity.reference) {
                context->notifyRideSelected(rideItem);
                context->mainWindow->selectAnalysis();
                break;
            }
        }
    });
    connect(calendar, &Calendar::addActivity, [=](bool plan, const QDate &day, const QTime &time) {
        context->tab->setNoSwitch(true);
        ManualActivityWizard wizard(context, plan, QDateTime(day, time));
        wizard.exec();
        context->tab->setNoSwitch(false);
    });
    connect(calendar, &Calendar::delActivity, [=](CalendarEntry activity) {
        context->tab->setNoSwitch(true);
        context->athlete->rideCache->removeRide(activity.reference);
        context->tab->setNoSwitch(false);

        // Context::rideDeleted is not always emitted, therefore forcing the update
        updateActivities();
    });
    connect(calendar, &Calendar::moveActivity, [=](CalendarEntry activity, const QDate &srcDay, const QDate &destDay) {
        Q_UNUSED(srcDay)

        QApplication::setOverrideCursor(Qt::WaitCursor);
        for (RideItem *rideItem : context->athlete->rideCache->rides()) {
            if (rideItem != nullptr && rideItem->fileName == activity.reference) {
                movePlannedActivity(rideItem, destDay);
                break;
            }
        }
        QApplication::restoreOverrideCursor();
    });
    connect(calendar, &Calendar::insertRestday, [=](const QDate &day) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QList<RideItem*> plannedRides;
        for (RideItem *rideItem : context->athlete->rideCache->rides()) {
            if (rideItem != nullptr && rideItem->planned && rideItem->dateTime.date() >= day) {
                plannedRides << rideItem;
            }
        }
        for (int i = plannedRides.size() - 1; i >= 0; --i) {
            QDate destDay = plannedRides[i]->dateTime.date().addDays(1);
            movePlannedActivity(plannedRides[i], destDay);
        }
        updateActivities();
        QApplication::restoreOverrideCursor();
    });
    connect(calendar, &Calendar::delRestday, [=](const QDate &day) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        QList<RideItem*> plannedRides;
        for (RideItem *rideItem : context->athlete->rideCache->rides()) {
            if (rideItem != nullptr && rideItem->planned && rideItem->dateTime.date() >= day) {
                QDate destDay = rideItem->dateTime.date().addDays(-1);
                movePlannedActivity(rideItem, destDay);
            }
        }
        QApplication::restoreOverrideCursor();
    });
    connect(calendar, &Calendar::monthChanged, this, &PlanningCalendarWindow::updateActivities);

    configChanged(CONFIG_FIELDS | CONFIG_APPEARANCE);
}


int
PlanningCalendarWindow::getWeekStartsOn
() const
{
    return weekStartsOn;
}


void
PlanningCalendarWindow::setWeekStartsOn
(int wso)
{
    weekStartsOn = wso;
}


bool
PlanningCalendarWindow::isFiltered
() const
{
    return (context->ishomefiltered || context->isfiltered);
}


void
PlanningCalendarWindow::configChanged(qint32)
{
    // change colors to reflect preferences
    setProperty("color", GColor(CPLOTBACKGROUND));

    QColor activeBg = GColor(CPLOTBACKGROUND);
    QColor activeText = GCColor::invertColor(activeBg);
    QColor activeHl = GColor(CCALCURRENT);
    QColor activeHlText = GCColor::invertColor(activeHl);

    QColor alternateBg = GCColor::inactiveColor(activeBg, 0.3);
    QColor alternateText = GCColor::inactiveColor(activeText, 1.5);

    QPalette palette;

    palette.setColor(QPalette::Active, QPalette::Window, activeBg);
    palette.setColor(QPalette::Active, QPalette::WindowText, activeText);
    palette.setColor(QPalette::Active, QPalette::Base, activeBg);
    palette.setColor(QPalette::Active, QPalette::Text, activeText);
    palette.setColor(QPalette::Active, QPalette::Highlight, activeHl);
    palette.setColor(QPalette::Active, QPalette::HighlightedText, activeHlText);
    palette.setColor(QPalette::Active, QPalette::Button, activeBg);
    palette.setColor(QPalette::Active, QPalette::ButtonText, activeText);

    palette.setColor(QPalette::Disabled, QPalette::Window, alternateBg);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, alternateText);
    palette.setColor(QPalette::Disabled, QPalette::Base, alternateBg);
    palette.setColor(QPalette::Disabled, QPalette::Text, alternateText);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, activeHl);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, activeHlText);
    palette.setColor(QPalette::Disabled, QPalette::Button, alternateBg);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, alternateText);

    PaletteApplier::setPaletteRecursively(this, palette, true);
}


QHash<QDate, QList<CalendarEntry>>
PlanningCalendarWindow::getActivities
(const QDate &firstDay, const QDate &lastDay) const
{
    QHash<QDate, QList<CalendarEntry>> activities;
    for (RideItem *rideItem : context->athlete->rideCache->rides()) {
        if (   rideItem->dateTime.date() < firstDay
            || rideItem->dateTime.date() > lastDay
            || rideItem == nullptr) {
            continue;
        }
        if (context->isfiltered && ! context->filters.contains(rideItem->fileName)) {
            continue;
        }

        QString route = rideItem->getText("Route", "").trimmed();
        QString workoutCode = rideItem->getText("Workout Code", "").trimmed();
        QString sport = rideItem->getText("Sport", "").trimmed();
        CalendarEntry activity;
        if (! route.isEmpty()) {
            activity.name = route;
        } else if (! workoutCode.isEmpty()) {
            activity.name = workoutCode;
        } else if (! sport.isEmpty()) {
            activity.name = tr("Unnamed %1").arg(sport);
        } else {
            activity.name = tr("<unknown>");
        }

        if (sport == "Bike") {
            activity.iconFile = ":images/material/bike.svg";
        } else if (sport == "Run") {
            activity.iconFile = ":images/material/run.svg";
        } else if (sport == "Swim") {
            activity.iconFile = ":images/material/swim.svg";
        } else if (sport == "Row") {
            activity.iconFile = ":images/material/rowing.svg";
        } else if (sport == "Ski") {
            activity.iconFile = ":images/material/ski.svg";
        } else if (sport == "Gym") {
            activity.iconFile = ":images/material/weight-lifter.svg";
        } else {
            activity.iconFile = ":images/material/torch.svg";
        }
        if (rideItem->color.alpha() < 255 || rideItem->planned) {
            activity.color = QColor("#F79130");
        } else {
            activity.color = rideItem->color;
        }
        activity.reference = rideItem->fileName;
        activity.start = rideItem->dateTime.time();
        activity.type = rideItem->planned ? ENTRY_TYPE_PLANNED_ACTIVITY : ENTRY_TYPE_ACTIVITY;
        activity.isRelocatable = rideItem->planned;
        activity.durationSecs = rideItem->getForSymbol("workout_time", GlobalContext::context()->useMetricUnits);
        activities[rideItem->dateTime.date()] << activity;
    }
    return activities;
}


QHash<QDate, QList<CalendarEntry>>
PlanningCalendarWindow::getPhasesEvents
(const Season &season, const QDate &firstDay, const QDate &lastDay) const
{
    QHash<QDate, QList<CalendarEntry>> phasesEvents;
    for (const Phase &phase : season.phases) {
        if (phase.getAbsoluteStart().isValid() && phase.getAbsoluteEnd().isValid()) {
            for (QDate date = phase.getAbsoluteStart(); date <= phase.getAbsoluteEnd(); date = date.addDays(1)) {
                if (   (   (   firstDay.isValid()
                            && date >= firstDay)
                        || ! firstDay.isValid())
                    && (   (   lastDay.isValid()
                            && date <= lastDay)
                        || ! lastDay.isValid())) {
                    CalendarEntry entry;
                    entry.name = phase.getName();
                    entry.iconFile = ":images/material/arrow-left-right.svg";
                    entry.color = Qt::red;
                    entry.reference = phase.id().toString();
                    entry.start = QTime(0, 0, 1);
                    entry.durationSecs = 0;
                    entry.type = ENTRY_TYPE_PHASE;
                    entry.isRelocatable = false;
                    entry.spanStart = phase.getStart();
                    entry.spanEnd = phase.getEnd();
                    phasesEvents[date] << entry;
                }
            }
        }
    }
    for (const SeasonEvent &event : season.events) {
        if (   (   (   firstDay.isValid()
                    && event.date >= firstDay)
                || ! firstDay.isValid())
            && (   (   lastDay.isValid()
                    && event.date <= lastDay)
                || ! lastDay.isValid())) {
            CalendarEntry entry;
            entry.name = event.name;
            if (event.priority == 0 || event.priority == 1) {
                entry.iconFile = ":images/material/podium.svg";
            } else if (event.priority == 2) {
                entry.iconFile = ":images/material/podium-gold.svg";
            } else if (event.priority == 3) {
                entry.iconFile = ":images/material/podium-silver.svg";
            } else {
                entry.iconFile = ":images/material/podium-bronze.svg";
            }
            entry.color = Qt::yellow;
            entry.reference = event.id;
            entry.start = QTime(0, 0, 0);
            entry.durationSecs = 0;
            entry.type = ENTRY_TYPE_EVENT;
            entry.isRelocatable = false;
            phasesEvents[event.date] << entry;
        }
    }

    return phasesEvents;
}


void
PlanningCalendarWindow::updateActivities
()
{
    Season const *season = context->currentSeason();
    QHash<QDate, QList<CalendarEntry>> activities = getActivities(calendar->firstVisibleDay(), calendar->lastVisibleDay());
    QHash<QDate, QList<CalendarEntry>> phasesEvents = getPhasesEvents(*season, calendar->firstVisibleDay(), calendar->lastVisibleDay());
    calendar->fillEntries(activities, phasesEvents);
}


void
PlanningCalendarWindow::updateActivitiesIfVisible
(RideItem *rideItem)
{
    if (   rideItem->dateTime.date() >= calendar->firstVisibleDay()
        && rideItem->dateTime.date() <= calendar->lastVisibleDay()) {
        updateActivities();
    }
}


void
PlanningCalendarWindow::updateSeason
(Season const *season, bool allowKeepMonth)
{
    if (season == nullptr) {
        DateRange dr(QDate(), QDate(), "");
        calendar->activateDateRange(dr, allowKeepMonth);
    } else {
        DateRange dr(DateRange(season->getStart(), season->getEnd(), season->getName()));
        calendar->activateDateRange(dr, allowKeepMonth);
    }
}


bool
PlanningCalendarWindow::movePlannedActivity
(RideItem *rideItem, const QDate &destDay, bool force)
{
    bool ret = false;
    RideFile *rideFile = rideItem->ride();

    QDateTime rideDateTime(destDay, rideFile->startTime().time());
    rideFile->setStartTime(rideDateTime);
    QString basename = rideDateTime.toString("yyyy_MM_dd_HH_mm_ss");

    QString filename;
    if (rideItem->planned) {
        filename = context->athlete->home->planned().canonicalPath() + "/" + basename + ".json";
    } else {
        filename = context->athlete->home->activities().canonicalPath() + "/" + basename + ".json";
    }
    QFile out(filename);
    if (   (   force
            || (! force && ! out.exists()))
        && RideFileFactory::instance().writeRideFile(context, rideFile, out, "json")) {
        context->tab->setNoSwitch(true);
        context->athlete->rideCache->removeRide(rideItem->fileName);
        context->athlete->addRide(basename + ".json", true, true, false, rideItem->planned);
        context->tab->setNoSwitch(false);
        ret = true;
    } else {
        QMessageBox oops(QMessageBox::Critical, tr("Unable to save"),
                         tr("There is already an activity with the same start time or you do not have permissions to save a file."));
        oops.exec();
    }
    return ret;
}
