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
#include "RideMetadata.h"
#include "Colors.h"
#include "ManualActivityWizard.h"
#include "RepeatScheduleWizard.h"
#include "WorkoutFilter.h"
#include "IconManager.h"

#define HLO "<h4>"
#define HLC "</h4>"


PlanningCalendarWindow::PlanningCalendarWindow(Context *context)
: GcChartWindow(context), context(context)
{
    mkControls();

    calendar = new Calendar(QDate::currentDate(), static_cast<Qt::DayOfWeek>(getFirstDayOfWeek()));

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
    connect(context, &Context::rideAdded, this, &PlanningCalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideDeleted, this, &PlanningCalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::configChanged, this, &PlanningCalendarWindow::configChanged);
    connect(calendar, &Calendar::showInTrainMode, [=](CalendarEntry activity) {
        for (RideItem *rideItem : context->athlete->rideCache->rides()) {
            if (rideItem != nullptr && rideItem->fileName == activity.reference) {
                QString filter = buildWorkoutFilter(rideItem);
                if (! filter.isEmpty()) {
                    context->mainWindow->fillinWorkoutFilterBox(filter);
                    context->mainWindow->selectTrain();
                    context->notifySelectWorkout(0);
                }
                break;
            }
        }
    });
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
    connect(calendar, &Calendar::repeatSchedule, [=](const QDate &day) {
        context->tab->setNoSwitch(true);
        RepeatScheduleWizard wizard(context, day);
        if (wizard.exec() == QDialog::Accepted) {
            // Context::rideDeleted is not always emitted, therefore forcing the update
            updateActivities();
        }
        context->tab->setNoSwitch(false);
    });
    connect(calendar, &Calendar::delActivity, [=](CalendarEntry activity) {
        QMessageBox::StandardButton res = QMessageBox::question(this, tr("Delete Activity"), tr("Are you sure you want to delete %1?").arg(activity.reference));
        if (res == QMessageBox::Yes) {
            context->tab->setNoSwitch(true);
            context->athlete->rideCache->removeRide(activity.reference);
            context->tab->setNoSwitch(false);

            // Context::rideDeleted is not always emitted, therefore forcing the update
            updateActivities();
        }
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

    configChanged(CONFIG_APPEARANCE);
}


int
PlanningCalendarWindow::getFirstDayOfWeek
() const
{
    return firstDayOfWeekCombo->currentIndex() + 1;
}


bool
PlanningCalendarWindow::isSummaryVisibleMonth
() const
{
    return summaryMonthCheck->isChecked();
}


void
PlanningCalendarWindow::setFirstDayOfWeek
(int fdw)
{
    firstDayOfWeekCombo->setCurrentIndex(std::min(static_cast<int>(Qt::Sunday), std::max(static_cast<int>(Qt::Monday), fdw)) - 1);
    calendar->setFirstDayOfWeek(static_cast<Qt::DayOfWeek>(fdw));
}


void
PlanningCalendarWindow::setSummaryVisibleMonth
(bool svm)
{
    summaryMonthCheck->setChecked(svm);
    calendar->setSummaryMonthVisible(svm);
}


bool
PlanningCalendarWindow::isFiltered
() const
{
    return (context->ishomefiltered || context->isfiltered);
}


QString
PlanningCalendarWindow::getPrimaryMainField
() const
{
    return primaryMainCombo->currentText();
}


void
PlanningCalendarWindow::setPrimaryMainField
(const QString &name)
{
    primaryMainCombo->setCurrentText(name);
}


QString
PlanningCalendarWindow::getPrimaryFallbackField
() const
{
    return primaryFallbackCombo->currentText();
}


void
PlanningCalendarWindow::setPrimaryFallbackField
(const QString &name)
{
    primaryFallbackCombo->setCurrentText(name);
}


QString
PlanningCalendarWindow::getSecondaryMetric
() const
{
    return secondaryCombo->currentData(Qt::UserRole).toString();
}


QString
PlanningCalendarWindow::getSummaryMetrics
() const
{
    return multiMetricSelector->getSymbols().join(',');
}


QStringList
PlanningCalendarWindow::getSummaryMetricsList
() const
{
    return multiMetricSelector->getSymbols();
}


void
PlanningCalendarWindow::setSecondaryMetric
(const QString &name)
{
    secondaryCombo->setCurrentIndex(std::max(0, secondaryCombo->findData(name)));
}


void
PlanningCalendarWindow::setSummaryMetrics
(const QString &summaryMetrics)
{
    multiMetricSelector->setSymbols(summaryMetrics.split(',', Qt::SkipEmptyParts));
}


void
PlanningCalendarWindow::setSummaryMetrics
(const QStringList &summaryMetrics)
{
    multiMetricSelector->setSymbols(summaryMetrics);
}


void
PlanningCalendarWindow::configChanged(qint32 what)
{
    bool refreshActivities = false;
    if (   (what & CONFIG_FIELDS)
        || (what & CONFIG_USERMETRICS)) {
        updatePrimaryConfigCombos();
        updateSecondaryConfigCombo();
        multiMetricSelector->updateMetrics();
    }
    if (what & CONFIG_APPEARANCE) {
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

        calendar->applyNavIcons();

        refreshActivities = true;
    }

    if (refreshActivities) {
        updateActivities();
    }
}


void
PlanningCalendarWindow::mkControls
()
{
    QLocale locale;
    firstDayOfWeekCombo = new QComboBox();
    for (int i = Qt::Monday; i <= Qt::Sunday; ++i) {
        firstDayOfWeekCombo->addItem(locale.dayName(i, QLocale::LongFormat));
    }
    firstDayOfWeekCombo->setCurrentIndex(locale.firstDayOfWeek() - 1);
    summaryMonthCheck = new QCheckBox(tr("Show weekly summary on month view"));
    summaryMonthCheck->setChecked(true);
    primaryMainCombo = new QComboBox();
    primaryFallbackCombo = new QComboBox();
    secondaryCombo = new QComboBox();
    updatePrimaryConfigCombos();
    updateSecondaryConfigCombo();
    primaryMainCombo->setCurrentText("Route");
    primaryFallbackCombo->setCurrentText("Workout Code");
    int secondaryIndex = secondaryCombo->findData("workout_time");
    if (secondaryIndex >= 0) {
        secondaryCombo->setCurrentIndex(secondaryIndex);
    }
    QStringList summaryMetrics { "ride_count", "total_distance", "coggan_tss", "workout_time" };
    multiMetricSelector = new MultiMetricSelector(tr("Available Metrics"), tr("Selected Metrics"), summaryMetrics);

    QFormLayout *formLayout = newQFormLayout();
    formLayout->addRow(tr("First day of week"), firstDayOfWeekCombo);
    formLayout->addRow("", summaryMonthCheck);
    formLayout->addRow(new QLabel(HLO + tr("Calendar Entries") + HLC));
    formLayout->addRow(tr("Field for Primary Line"), primaryMainCombo);
    formLayout->addRow(tr("Fallback Field for Primary Line"), primaryFallbackCombo);
    formLayout->addRow(tr("Metric for Secondary Line"), secondaryCombo);
    formLayout->addRow(new QLabel(HLO + tr("Summary") + HLC));

    QWidget *controlsWidget = new QWidget();

    QVBoxLayout *controlsLayout = new QVBoxLayout(controlsWidget);
    controlsLayout->addWidget(centerLayoutInWidget(formLayout, false));
    controlsLayout->addWidget(multiMetricSelector, 2);
    controlsLayout->addStretch(1);

#if QT_VERSION < 0x060000
    connect(firstDayOfWeekCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx) { setFirstDayOfWeek(idx + 1); });
    connect(primaryMainCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlanningCalendarWindow::updateActivities);
    connect(primaryFallbackCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlanningCalendarWindow::updateActivities);
    connect(secondaryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PlanningCalendarWindow::updateActivities);
#else
    connect(firstDayOfWeekCombo, &QComboBox::currentIndexChanged, [=](int idx) { setFirstDayOfWeek(idx + 1); });
    connect(primaryMainCombo, &QComboBox::currentIndexChanged, this, &PlanningCalendarWindow::updateActivities);
    connect(primaryFallbackCombo, &QComboBox::currentIndexChanged, this, &PlanningCalendarWindow::updateActivities);
    connect(secondaryCombo, &QComboBox::currentIndexChanged, this, &PlanningCalendarWindow::updateActivities);
#endif
    connect(summaryMonthCheck, &QCheckBox::toggled, this, &PlanningCalendarWindow::setSummaryVisibleMonth);
    connect(multiMetricSelector, &MultiMetricSelector::selectedChanged, this, &PlanningCalendarWindow::updateActivities);

    setControls(controlsWidget);
}


void
PlanningCalendarWindow::updatePrimaryConfigCombos
()
{
    QString mainField = getPrimaryMainField();
    QString fallbackField = getPrimaryFallbackField();

    primaryMainCombo->blockSignals(true);
    primaryFallbackCombo->blockSignals(true);
    primaryMainCombo->clear();
    primaryFallbackCombo->clear();
    QList<FieldDefinition> fieldsDefs = GlobalContext::context()->rideMetadata->getFields();
    for (const FieldDefinition &fieldDef : fieldsDefs) {
        if (fieldDef.isTextField()) {
            primaryMainCombo->addItem(fieldDef.name);
            primaryFallbackCombo->addItem(fieldDef.name);
        }
    }

    primaryMainCombo->blockSignals(false);
    primaryFallbackCombo->blockSignals(false);
    setPrimaryMainField(mainField);
    setPrimaryFallbackField(fallbackField);
}


void
PlanningCalendarWindow::updateSecondaryConfigCombo
()
{
    QString symbol = getSecondaryMetric();

    secondaryCombo->blockSignals(true);
    secondaryCombo->clear();
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (const QString &metricSymbol : factory.allMetrics()) {
        if (metricSymbol.startsWith("compatibility_")) {
            continue;
        }
        secondaryCombo->addItem(Utils::unprotect(factory.rideMetric(metricSymbol)->name()), metricSymbol);
    }

    secondaryCombo->blockSignals(false);
    setSecondaryMetric(symbol);
}


QHash<QDate, QList<CalendarEntry>>
PlanningCalendarWindow::getActivities
(const QDate &firstDay, const QDate &lastDay) const
{
    QHash<QDate, QList<CalendarEntry>> activities;
    const RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *rideMetric = factory.rideMetric(getSecondaryMetric());
    QString rideMetricName;
    QString rideMetricUnit;
    if (rideMetric != nullptr) {
        rideMetricName = rideMetric->name();
        if (   ! rideMetric->isTime()
            && ! rideMetric->isDate()) {
            rideMetricUnit = rideMetric->units(GlobalContext::context()->useMetricUnits);
        }
    }

    for (RideItem *rideItem : context->athlete->rideCache->rides()) {
        if (   rideItem->dateTime.date() < firstDay
            || rideItem->dateTime.date() > lastDay
            || rideItem == nullptr) {
            continue;
        }
        if (context->isfiltered && ! context->filters.contains(rideItem->fileName)) {
            continue;
        }

        QString sport = rideItem->sport;
        CalendarEntry activity;

        QString primaryMain = rideItem->getText(getPrimaryMainField(), "").trimmed();
        if (! primaryMain.isEmpty()) {
            activity.primary = primaryMain;
        } else {
            QString primaryFallback = rideItem->getText(getPrimaryFallbackField(), "").trimmed();
            if (! primaryFallback.isEmpty()) {
                activity.primary = primaryFallback;
            } else if (! sport.isEmpty()) {
                activity.primary = tr("Unnamed %1").arg(sport);
            } else {
                activity.primary = tr("<unknown>");
            }
        }
        if (rideMetric != nullptr && rideMetric->isRelevantForRide(rideItem)) {
            activity.secondary = rideItem->getStringForSymbol(getSecondaryMetric(), GlobalContext::context()->useMetricUnits);
            if (! rideMetricUnit.isEmpty()) {
                activity.secondary += " " + rideMetricUnit;
            }
            activity.secondaryMetric = rideMetricName;
        } else {
            activity.secondary = "";
            activity.secondaryMetric = "";
        }

        activity.iconFile = IconManager::instance().getFilepath(rideItem);
        if (rideItem->color.alpha() < 255 || rideItem->planned) {
            activity.color = QColor("#F79130");
        } else {
            activity.color = rideItem->color;
        }
        activity.reference = rideItem->fileName;
        activity.start = rideItem->dateTime.time();
        activity.durationSecs = rideItem->getForSymbol("workout_time", GlobalContext::context()->useMetricUnits);
        activity.type = rideItem->planned ? ENTRY_TYPE_PLANNED_ACTIVITY : ENTRY_TYPE_ACTIVITY;
        activity.isRelocatable = rideItem->planned;
        activity.hasTrainMode = rideItem->planned && sport == "Bike" && ! buildWorkoutFilter(rideItem).isEmpty();
        activities[rideItem->dateTime.date()] << activity;
    }
    return activities;
}


QList<CalendarSummary>
PlanningCalendarWindow::getWeeklySummaries
(const QDate &firstDay, const QDate &lastDay) const
{
    QStringList symbols = getSummaryMetricsList();
    QList<CalendarSummary> summaries;
    int numWeeks = firstDay.daysTo(lastDay) / 7 + 1;
    bool useMetricUnits = GlobalContext::context()->useMetricUnits;

    const RideMetricFactory &factory = RideMetricFactory::instance();
    FilterSet filterSet(context->isfiltered, context->filters);
    Specification spec;
    spec.setFilterSet(filterSet);
    for (int week = 0; week < numWeeks; ++week) {
        QDate firstDayOfWeek = firstDay.addDays(week * 7);
        QDate lastDayOfWeek = firstDayOfWeek.addDays(6);
        spec.setDateRange(DateRange(firstDayOfWeek, lastDayOfWeek));
        CalendarSummary summary;
        summary.keyValues.clear();
        for (const QString &symbol : symbols) {
            const RideMetric *metric = factory.rideMetric(symbol);
            if (metric == nullptr) {
                continue;
            }
            QString value = context->athlete->rideCache->getAggregate(symbol, spec, useMetricUnits);
            if (! metric->isDate() && ! metric->isTime()) {
                if (value.contains('.')) {
                    while (value.endsWith('0')) {
                        value.chop(1);
                    }
                    if (value.endsWith('.')) {
                        value.chop(1);
                    }
                }
                if (! metric->units(useMetricUnits).isEmpty()) {
                    value += " " + metric->units(useMetricUnits);
                }
            }
            summary.keyValues << std::make_pair(Utils::unprotect(metric->name()), value);
        }
        summaries << summary;
    }
    return summaries;
}


QHash<QDate, QList<CalendarEntry>>
PlanningCalendarWindow::getPhasesEvents
(const Season &season, const QDate &firstDay, const QDate &lastDay) const
{
    QHash<QDate, QList<CalendarEntry>> phasesEvents;
    for (const Phase &phase : season.phases) {
        if (phase.getAbsoluteStart().isValid() && phase.getAbsoluteEnd().isValid()) {
            int duration = std::max(qint64(1), phase.getAbsoluteStart().daysTo(phase.getAbsoluteEnd()));
            for (QDate date = phase.getAbsoluteStart(); date <= phase.getAbsoluteEnd(); date = date.addDays(1)) {
                if (   (   (   firstDay.isValid()
                            && date >= firstDay)
                        || ! firstDay.isValid())
                    && (   (   lastDay.isValid()
                            && date <= lastDay)
                        || ! lastDay.isValid())) {
                    int progress = int(phase.getAbsoluteStart().daysTo(date) / double(duration) * 5.0) * 20;
                    CalendarEntry entry;
                    entry.primary = phase.getName();
                    entry.secondary = "";
                    entry.iconFile = QString(":images/breeze/network-mobile-%1.svg").arg(progress);
                    entry.color = Qt::red;
                    entry.reference = phase.id().toString();
                    entry.start = QTime(0, 0, 1);
                    entry.type = ENTRY_TYPE_PHASE;
                    entry.isRelocatable = false;
                    entry.spanStart = phase.getStart();
                    entry.spanEnd = phase.getEnd();
                    phasesEvents[date] << entry;
                }
            }
        }
    }
    QList<Season> tmpSeasons = context->athlete->seasons->seasons;
    std::sort(tmpSeasons.begin(),tmpSeasons.end(),Season::LessThanForStarts);
    for (const Season &s : tmpSeasons) {
        for (const SeasonEvent &event : s.events) {
            if (   (   (   firstDay.isValid()
                        && event.date >= firstDay)
                    || ! firstDay.isValid())
                && (   (   lastDay.isValid()
                        && event.date <= lastDay)
                    || ! lastDay.isValid())) {
                CalendarEntry entry;
                entry.primary = event.name;
                entry.secondary = "";
                if (event.priority == 0 || event.priority == 1) {
                    entry.iconFile = ":images/breeze/task-process-4.svg";
                } else if (event.priority == 2) {
                    entry.iconFile = ":images/breeze/task-process-3.svg";
                } else if (event.priority == 3) {
                    entry.iconFile = ":images/breeze/task-process-2.svg";
                } else if (event.priority == 4) {
                    entry.iconFile = ":images/breeze/task-process-1.svg";
                } else {
                    entry.iconFile = ":images/breeze/task-process-0.svg";
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
    }

    return phasesEvents;
}


void
PlanningCalendarWindow::updateActivities
()
{
    Season const *season = context->currentSeason();
    if (!season) return; // avoid crash if no season selected

    QHash<QDate, QList<CalendarEntry>> activities = getActivities(calendar->firstVisibleDay(), calendar->lastVisibleDay());
    QList<CalendarSummary> summaries = getWeeklySummaries(calendar->firstVisibleDay(), calendar->lastVisibleDay());
    QHash<QDate, QList<CalendarEntry>> phasesEvents = getPhasesEvents(*season, calendar->firstVisibleDay(), calendar->lastVisibleDay());
    calendar->fillEntries(activities, summaries, phasesEvents);
}


void
PlanningCalendarWindow::updateActivitiesIfInRange
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
        QMessageBox oops(QMessageBox::Critical,
                         tr("Unable to save"),
                         tr("There is already an activity with the same start time or you do not have permissions to save a file."));
        oops.exec();
    }
    return ret;
}
