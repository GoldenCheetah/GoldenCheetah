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

#include "AgendaWindow.h"

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
#include "SeasonDialogs.h"

#define HLO "<h4>"
#define HLC "</h4>"


AgendaWindow::AgendaWindow(Context *context)
: GcChartWindow(context), context(context)
{
    mkControls();

    agendaView = new AgendaView();
    agendaView->updateDate();

    setAgendaPastDays(7);
    setAgendaFutureDays(7);
    setPrimaryMainField("Route");
    setPrimaryFallbackField("Workout Code");
    setSecondaryMetric("workout_time");
    setShowSecondaryLabel(true);
    setTertiaryField("Notes");
    setShowTertiaryFor(0);
    setActivityMaxTertiaryLines(3);
    setEventMaxTertiaryLines(3);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setChartLayout(mainLayout);
    mainLayout->addWidget(agendaView);

    connect(context->athlete->seasons, &Seasons::seasonsChanged, this, &AgendaWindow::updateActivities);
    connect(context, &Context::seasonSelected, this, &AgendaWindow::updateActivities);
    connect(context, &Context::filterChanged, this, &AgendaWindow::updateActivities);
    connect(context, &Context::homeFilterChanged, this, &AgendaWindow::updateActivities);
    connect(context, &Context::rideAdded, this, &AgendaWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideDeleted, this, &AgendaWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideChanged, this, &AgendaWindow::updateActivitiesIfInRange);
    connect(context, &Context::configChanged, this, &AgendaWindow::configChanged);
    connect(agendaView, &AgendaView::showInTrainMode, this, [context](const CalendarEntry &activity) {
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
    connect(agendaView, &AgendaView::viewActivity, this, [context](const CalendarEntry &activity) {
        for (RideItem *rideItem : context->athlete->rideCache->rides()) {
            if (rideItem != nullptr && rideItem->fileName == activity.reference) {
                context->notifyRideSelected(rideItem);
                context->mainWindow->selectAnalysis();
                break;
            }
        }
    });
    connect(agendaView, &AgendaView::editPhaseEntry, this, &AgendaWindow::editPhaseEntry);
    connect(agendaView, &AgendaView::editEventEntry, this, &AgendaWindow::editEventEntry);

    QTimer::singleShot(0, this, [this]() {
        configChanged(CONFIG_APPEARANCE);
    });
}


int
AgendaWindow::getAgendaPastDays
() const
{
    return agendaPastDaysSpin->value();
}


void
AgendaWindow::setAgendaPastDays
(int days)
{
    agendaPastDaysSpin->setValue(days);
    if (agendaView != nullptr) {
        agendaView->setPastDays(days);
        updateActivities();
    }
}


int
AgendaWindow::getAgendaFutureDays
() const
{
    return agendaFutureDaysSpin->value();
}


void
AgendaWindow::setAgendaFutureDays
(int days)
{
    agendaFutureDaysSpin->setValue(days);
    if (agendaView != nullptr) {
        agendaView->setFutureDays(days);
        updateActivities();
    }
}


bool
AgendaWindow::isFiltered
() const
{
    return (context->ishomefiltered || context->isfiltered);
}


QString
AgendaWindow::getPrimaryMainField
() const
{
    return primaryMainCombo->currentText();
}


void
AgendaWindow::setPrimaryMainField
(const QString &name)
{
    primaryMainCombo->setCurrentText(name);
}


QString
AgendaWindow::getPrimaryFallbackField
() const
{
    return primaryFallbackCombo->currentText();
}


void
AgendaWindow::setPrimaryFallbackField
(const QString &name)
{
    primaryFallbackCombo->setCurrentText(name);
}


void
AgendaWindow::setSecondaryMetric
(const QString &name)
{
    secondaryCombo->setCurrentIndex(std::max(0, secondaryCombo->findData(name)));
}


QString
AgendaWindow::getSecondaryMetric
() const
{
    return secondaryCombo->currentData(Qt::UserRole).toString();
}


void
AgendaWindow::setShowSecondaryLabel
(bool showSecondaryLabel)
{
    showSecondaryLabelCheck->setChecked(showSecondaryLabel);
}


bool
AgendaWindow::isShowSecondaryLabel
() const
{
    return showSecondaryLabelCheck->isChecked();
}


void
AgendaWindow::setShowTertiaryFor
(int showFor)
{
    showTertiaryForCombo->setCurrentIndex(showFor);
}


int
AgendaWindow::getShowTertiaryFor
() const
{
    return showTertiaryForCombo->currentIndex();
}


QString
AgendaWindow::getTertiaryField
() const
{
    return tertiaryCombo->currentText();
}


void
AgendaWindow::setTertiaryField
(const QString &name)
{
    tertiaryCombo->setCurrentText(name);
}


int
AgendaWindow::getActivityMaxTertiaryLines
() const
{
    return activityMaxTertiaryLinesSpin->value();
}


void
AgendaWindow::setActivityMaxTertiaryLines
(int maxTertiaryLines)
{
    activityMaxTertiaryLinesSpin->setValue(maxTertiaryLines);
    if (agendaView != nullptr) {
        agendaView->setActivityMaxTertiaryLines(maxTertiaryLines);
    }
}


int
AgendaWindow::getEventMaxTertiaryLines
() const
{
    return eventMaxTertiaryLinesSpin->value();
}


void
AgendaWindow::setEventMaxTertiaryLines
(int maxTertiaryLines)
{
    eventMaxTertiaryLinesSpin->setValue(maxTertiaryLines);
    if (agendaView != nullptr) {
        agendaView->setEventMaxTertiaryLines(maxTertiaryLines);
    }
}


void
AgendaWindow::configChanged
(qint32 what)
{
    bool refreshActivities = false;
    if (   (what & CONFIG_FIELDS)
        || (what & CONFIG_USERMETRICS)) {
        updatePrimaryConfigCombos();
        updateSecondaryConfigCombo();
        updateTertiaryConfigCombo();
    }
    if (what & CONFIG_APPEARANCE) {
        // change colors to reflect preferences
        setProperty("color", GColor(CPLOTBACKGROUND));

        QColor activeBase = GColor(CPLOTBACKGROUND);
        QColor activeWindow = activeBase;
        QColor activeText = GCColor::invertColor(activeBase);
        QColor activeHl = GColor(CCALCURRENT);
        QColor activeHlText = GCColor::invertColor(activeHl);
        QColor alternateBg = GCColor::inactiveColor(activeBase, 0.2);
        QColor inactiveText = GCColor::inactiveColor(activeText, 1.5);
        QColor activeButtonBg = activeBase;
        QColor disabledButtonBg = alternateBg;
        if (activeBase.lightness() < 20) {
            activeWindow = GCColor::inactiveColor(activeWindow, 0.2);
            activeButtonBg = alternateBg;
            disabledButtonBg = GCColor::inactiveColor(activeButtonBg, 0.3);
            inactiveText = GCColor::inactiveColor(activeText, 2.5);
        }

        palette.setColor(QPalette::Active, QPalette::Window, activeWindow);
        palette.setColor(QPalette::Active, QPalette::WindowText, activeText);
        palette.setColor(QPalette::Active, QPalette::Base, activeBase);
        palette.setColor(QPalette::Active, QPalette::AlternateBase, alternateBg);
        palette.setColor(QPalette::Active, QPalette::Text, activeText);
        palette.setColor(QPalette::Active, QPalette::Highlight, activeHl);
        palette.setColor(QPalette::Active, QPalette::HighlightedText, activeHlText);
        palette.setColor(QPalette::Active, QPalette::Button, activeButtonBg);
        palette.setColor(QPalette::Active, QPalette::ButtonText, activeText);

        palette.setColor(QPalette::Inactive, QPalette::Window, activeWindow);
        palette.setColor(QPalette::Inactive, QPalette::WindowText, activeText);
        palette.setColor(QPalette::Inactive, QPalette::Base, activeBase);
        palette.setColor(QPalette::Inactive, QPalette::AlternateBase, alternateBg);
        palette.setColor(QPalette::Inactive, QPalette::Text, activeText);
        palette.setColor(QPalette::Inactive, QPalette::Highlight, activeHl);
        palette.setColor(QPalette::Inactive, QPalette::HighlightedText, activeHlText);
        palette.setColor(QPalette::Inactive, QPalette::Button, activeButtonBg);
        palette.setColor(QPalette::Inactive, QPalette::ButtonText, activeText);

        palette.setColor(QPalette::Disabled, QPalette::Window, alternateBg);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, inactiveText);
        palette.setColor(QPalette::Disabled, QPalette::Base, alternateBg);
        palette.setColor(QPalette::Disabled, QPalette::AlternateBase, alternateBg);
        palette.setColor(QPalette::Disabled, QPalette::Text, inactiveText);
        palette.setColor(QPalette::Disabled, QPalette::Highlight, activeHl);
        palette.setColor(QPalette::Disabled, QPalette::HighlightedText, activeHlText);
        palette.setColor(QPalette::Disabled, QPalette::Button, disabledButtonBg);
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, inactiveText);

        PaletteApplier::setPaletteRecursively(this, palette, true);
        refreshActivities = true;
    }

    if (refreshActivities) {
        updateActivities();
    }
}


void
AgendaWindow::showEvent
(QShowEvent *event)
{
    Q_UNUSED(event)

    PaletteApplier::setPaletteRecursively(this, palette, true);
}


void
AgendaWindow::mkControls
()
{
    agendaPastDaysSpin = new QSpinBox();
    agendaPastDaysSpin->setMaximum(31);
    agendaPastDaysSpin->setSuffix(" " + tr("day(s)"));
    agendaFutureDaysSpin = new QSpinBox();
    agendaFutureDaysSpin->setMaximum(31);
    agendaFutureDaysSpin->setSuffix(" " + tr("day(s)"));
    primaryMainCombo = new QComboBox();
    primaryFallbackCombo = new QComboBox();
    secondaryCombo = new QComboBox();
    showSecondaryLabelCheck = new QCheckBox(tr("Show Label"));
    showTertiaryForCombo = new QComboBox();
    tertiaryCombo = new QComboBox();
    updatePrimaryConfigCombos();
    updateSecondaryConfigCombo();
    showTertiaryForCombo->addItem(tr("all dates"));
    showTertiaryForCombo->addItem(tr("today"));
    showTertiaryForCombo->addItem(tr("no dates"));
    updateTertiaryConfigCombo();
    primaryMainCombo->setCurrentText("Route");
    primaryFallbackCombo->setCurrentText("Workout Code");
    int secondaryIndex = secondaryCombo->findData("workout_time");
    if (secondaryIndex >= 0) {
        secondaryCombo->setCurrentIndex(secondaryIndex);
    }
    activityMaxTertiaryLinesSpin = new QSpinBox();
    activityMaxTertiaryLinesSpin->setRange(1, 5);
    eventMaxTertiaryLinesSpin = new QSpinBox();
    eventMaxTertiaryLinesSpin->setRange(1, 5);

    QFormLayout *activityForm = newQFormLayout();
    activityForm->setContentsMargins(0, 10 * dpiYFactor, 0, 10 * dpiYFactor);
    activityForm->addRow(new QLabel(HLO + tr("Agenda Basics") + HLC));
    activityForm->addRow(tr("Back to Include"), agendaPastDaysSpin);
    activityForm->addRow(tr("Ahead to Include"), agendaFutureDaysSpin);
    activityForm->addItem(new QSpacerItem(0, 20 * dpiYFactor));
    activityForm->addRow(new QLabel(HLO + tr("Main Line") + HLC));
    activityForm->addRow(tr("Field"), primaryMainCombo);
    activityForm->addRow(tr("Fallback Field"), primaryFallbackCombo);
    activityForm->addItem(new QSpacerItem(0, 20 * dpiYFactor));
    activityForm->addRow(new QLabel(HLO + tr("Metric Line") + HLC));
    activityForm->addRow(tr("Metric"), secondaryCombo);
    activityForm->addRow("", showSecondaryLabelCheck);
    activityForm->addItem(new QSpacerItem(0, 20 * dpiYFactor));
    activityForm->addRow(new QLabel(HLO + tr("Detail Line") + HLC));
    activityForm->addRow(tr("Show Line for"), showTertiaryForCombo);
    activityForm->addRow(tr("Field"), tertiaryCombo);
    activityForm->addRow(tr("Visible Lines"), activityMaxTertiaryLinesSpin);

    QScrollArea *activityScroller = new QScrollArea();
    activityScroller->setWidgetResizable(true);
    activityScroller->setWidget(centerLayoutInWidget(activityForm, false));

    QFormLayout *eventForm = newQFormLayout();
    eventForm->setContentsMargins(0, 10 * dpiYFactor, 0, 10 * dpiYFactor);
    eventForm->addRow(new QLabel(HLO + tr("Detail Line") + HLC));
    eventForm->addRow(tr("Visible Lines"), eventMaxTertiaryLinesSpin);

    QScrollArea *eventScroller = new QScrollArea();
    eventScroller->setWidgetResizable(true);
    eventScroller->setWidget(centerLayoutInWidget(eventForm, false));

    QTabWidget *controlsTabs = new QTabWidget();
    controlsTabs->addTab(activityScroller, tr("Activities"));
    controlsTabs->addTab(eventScroller, tr("Events"));

    connect(agendaPastDaysSpin, &QSpinBox::valueChanged, this, &AgendaWindow::setAgendaPastDays);
    connect(agendaFutureDaysSpin, &QSpinBox::valueChanged, this, &AgendaWindow::setAgendaFutureDays);
    connect(primaryMainCombo, &QComboBox::currentIndexChanged, this, &AgendaWindow::updateActivities);
    connect(primaryFallbackCombo, &QComboBox::currentIndexChanged, this, &AgendaWindow::updateActivities);
    connect(secondaryCombo, &QComboBox::currentIndexChanged, this, &AgendaWindow::updateActivities);
    connect(showTertiaryForCombo, &QComboBox::currentIndexChanged, this, &AgendaWindow::updateActivities);
    connect(tertiaryCombo, &QComboBox::currentIndexChanged, this, &AgendaWindow::updateActivities);
    connect(activityMaxTertiaryLinesSpin, &QSpinBox::valueChanged, this, &AgendaWindow::setActivityMaxTertiaryLines);
    connect(eventMaxTertiaryLinesSpin, &QSpinBox::valueChanged, this, &AgendaWindow::setEventMaxTertiaryLines);
    connect(showSecondaryLabelCheck, &QCheckBox::toggled, this, &AgendaWindow::updateActivities);

    setControls(controlsTabs);
}


void
AgendaWindow::updatePrimaryConfigCombos
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
AgendaWindow::updateSecondaryConfigCombo
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


void
AgendaWindow::updateTertiaryConfigCombo
()
{
    QString field = getTertiaryField();

    tertiaryCombo->blockSignals(true);
    tertiaryCombo->clear();
    QList<FieldDefinition> fieldsDefs = GlobalContext::context()->rideMetadata->getFields();
    for (const FieldDefinition &fieldDef : fieldsDefs) {
        if (fieldDef.isTextField()) {
            tertiaryCombo->addItem(fieldDef.name);
        }
    }

    tertiaryCombo->blockSignals(false);
    setTertiaryField(field);
}


QHash<QDate, QList<CalendarEntry>>
AgendaWindow::getActivities
(const QDate &firstDay, const QDate &today, const QDate &lastDay) const
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

    int showTertiaryFor = getShowTertiaryFor();
    for (RideItem *rideItem : context->athlete->rideCache->rides()) {
        if (   rideItem == nullptr
            || rideItem->dateTime.date() < firstDay
            || rideItem->dateTime.date() > lastDay) {
            continue;
        }
        if (   (context->isfiltered && ! context->filters.contains(rideItem->fileName))
            || (context->ishomefiltered && ! context->homeFilters.contains(rideItem->fileName))) {
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
            if (isShowSecondaryLabel()) {
                activity.secondaryMetric = rideMetricName;
            }
        } else {
            activity.secondary = tr("N/A");
            activity.secondaryMetric = "";
        }
        if (showTertiaryFor == 0 || (showTertiaryFor == 1 && rideItem->dateTime.date() == today)) {
            activity.tertiary = rideItem->getText(getTertiaryField(), "").trimmed();
            activity.tertiary = Utils::unprotect(activity.tertiary);
        }
        activity.primary = Utils::unprotect(activity.primary);
        activity.secondary = Utils::unprotect(activity.secondary);
        activity.secondaryMetric = Utils::unprotect(activity.secondaryMetric);

        activity.iconFile = IconManager::instance().getFilepath(rideItem);
        if (rideItem->color.alpha() < 255 || rideItem->planned) {
            activity.color = GColor(CCALPLANNED);
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
    for (auto dayIt = activities.begin(); dayIt != activities.end(); ++dayIt) {
        std::sort(dayIt.value().begin(), dayIt.value().end(), [](const CalendarEntry &a, const CalendarEntry &b) {
            if (a.start == b.start) {
                return a.primary < b.primary;
            } else {
                return a.start < b.start;
            }
        });
    }
    return activities;
}


std::pair<QList<CalendarEntry>, QList<CalendarEntry>>
AgendaWindow::getPhases
(const Season &season, const QDate &firstDay) const
{
    QList<CalendarEntry> ongoingPhases;
    QList<CalendarEntry> futurePhases;
    for (const Phase &phase : season.phases) {
        if (phase.getAbsoluteStart().isValid() && phase.getAbsoluteEnd().isValid()) {
            QString phaseType;
            switch (phase.getType()) {
            case Phase::camp:
                phaseType = Phase::types[5];
                break;
            default:
                phaseType = Phase::types[static_cast<int>(phase.getType()) - static_cast<int>(Phase::phase)];
            }
            CalendarEntry entry;
            entry.primary = phase.getName();
            entry.iconFile = ":images/breeze/network-mobile-100.svg";
            entry.color = GColor(CCALPHASE);
            entry.reference = phase.id().toString();
            entry.start = QTime(0, 0, 1);
            entry.type = ENTRY_TYPE_PHASE;
            entry.isRelocatable = false;
            entry.spanStart = phase.getAbsoluteStart();
            entry.spanEnd = phase.getAbsoluteEnd();
            int duration = entry.spanStart.daysTo(entry.spanEnd);
            ShowDaysAsUnit unit = showDaysAs(duration);
            if (unit == ShowDaysAsUnit::Days) {
                if (duration > 1) {
                    entry.secondary = tr("%1 • %2 days").arg(phaseType).arg(duration);
                } else {
                    entry.secondary = tr("%1 • %2 day").arg(phaseType).arg(duration);
                }
            } else if (unit == ShowDaysAsUnit::Weeks) {
                duration = daysToWeeks(duration);
                if (duration > 1) {
                    entry.secondary = tr("%1 • %2 weeks").arg(phaseType).arg(duration);
                } else {
                    entry.secondary = tr("%1 • %2 week").arg(phaseType).arg(duration);
                }
            } else {
                duration = daysToMonths(duration);
                if (duration > 1) {
                    entry.secondary = tr("%1 • %2 months").arg(phaseType).arg(duration);
                } else {
                    entry.secondary = tr("%1 • %2 month").arg(phaseType).arg(duration);
                }
            }
            if (phase.getAbsoluteStart() <= firstDay && phase.getAbsoluteEnd() >= firstDay) {
                ongoingPhases << entry;
            } else if (phase.getAbsoluteStart() >= firstDay) {
                futurePhases << entry;
            }
        }
    }
    std::sort(ongoingPhases.begin(), ongoingPhases.end(), [](const CalendarEntry &a, const CalendarEntry &b) {
        return a.spanEnd < b.spanEnd;
    });
    std::sort(futurePhases.begin(), futurePhases.end(), [](const CalendarEntry &a, const CalendarEntry &b) {
        return a.spanStart < b.spanStart;
    });

    return std::pair(ongoingPhases, futurePhases);
}


QHash<QDate, QList<CalendarEntry>>
AgendaWindow::getEvents
(const QDate &firstDay) const
{
    QHash<QDate, QList<CalendarEntry>> events;
    QList<Season> tmpSeasons = context->athlete->seasons->seasons;
    std::sort(tmpSeasons.begin(), tmpSeasons.end(), Season::LessThanForStarts);
    for (const Season &s : tmpSeasons) {
        for (const SeasonEvent &event : s.events) {
            if (   (   (   firstDay.isValid()
                        && event.date >= firstDay)
                    || ! firstDay.isValid())) {
                CalendarEntry entry;
                entry.primary = event.name;
                if (event.priority == 0) {
                    entry.iconFile = ":images/breeze/task-process-4.svg";
                    entry.secondary = tr("Uncategorized");
                } else if (event.priority == 1) {
                    entry.iconFile = ":images/breeze/task-process-4.svg";
                    entry.secondary = tr("Category A");
                } else if (event.priority == 2) {
                    entry.iconFile = ":images/breeze/task-process-3.svg";
                    entry.secondary = tr("Category B");
                } else if (event.priority == 3) {
                    entry.iconFile = ":images/breeze/task-process-2.svg";
                    entry.secondary = tr("Category C");
                } else if (event.priority == 4) {
                    entry.iconFile = ":images/breeze/task-process-1.svg";
                    entry.secondary = tr("Category D");
                } else {
                    entry.iconFile = ":images/breeze/task-process-0.svg";
                    entry.secondary = tr("Category E");
                }
                entry.tertiary = event.description.trimmed();
                entry.color = GColor(CCALEVENT);
                entry.reference = event.id;
                entry.start = QTime(0, 0, 0);
                entry.durationSecs = 0;
                entry.type = ENTRY_TYPE_EVENT;
                entry.spanStart = event.date;
                entry.spanEnd = event.date;
                entry.isRelocatable = false;
                events[event.date] << entry;
            }
        }
    }

    return events;
}


void
AgendaWindow::updateActivities
()
{
    if (agendaView->selectedDate() != QDate::currentDate()) {
        agendaView->updateDate();
        return;
    }
    QHash<QDate, QList<CalendarEntry>> activities = getActivities(agendaView->firstVisibleDay(), agendaView->selectedDate(), agendaView->lastVisibleDay());
    std::pair<QList<CalendarEntry>, QList<CalendarEntry>> phases;
    QHash<QDate, QList<CalendarEntry>> events;
    QString seasonName;

    tertiaryCombo->setEnabled(getShowTertiaryFor() != 2);
    activityMaxTertiaryLinesSpin->setEnabled(getShowTertiaryFor() != 2);

    Season const *season = context->currentSeason();
    if (season) {
        phases = getPhases(*season, agendaView->selectedDate());
        events = getEvents(agendaView->selectedDate());
        seasonName = season->getName();
    }

    agendaView->fillEntries(activities, phases, events, seasonName, isFiltered());
}


void
AgendaWindow::updateActivitiesIfInRange
(RideItem *rideItem)
{
    if (   rideItem->dateTime.date() >= agendaView->firstVisibleDay()
        && rideItem->dateTime.date() <= agendaView->lastVisibleDay()) {
        updateActivities();
    }
}


void
AgendaWindow::editPhaseEntry
(const CalendarEntry &entry)
{
    if (entry.type != ENTRY_TYPE_PHASE) {
        return;
    }
    Season const *currentSeason = context->currentSeason();
    if (currentSeason == nullptr) {
        return;
    }
    Season *phaseSeason = nullptr;
    for (Season &s : context->athlete->seasons->seasons) {
        if (s.id() == currentSeason->id()) {
            phaseSeason = &s;
            break;
        }
    }
    if (phaseSeason == nullptr) {
        return;
    }
    for (Phase &editPhase : phaseSeason->phases) {
        if (entry.reference == editPhase.id().toString()) {
            EditPhaseDialog dialog(context, &editPhase, *phaseSeason);
            if (dialog.exec()) {
                context->athlete->seasons->writeSeasons();
                updateActivities();
            }
            break;
        }
    }
}


void
AgendaWindow::editEventEntry
(const CalendarEntry &entry)
{
    if (entry.type != ENTRY_TYPE_EVENT) {
        return;
    }
    Season *season = nullptr;
    SeasonEvent *seasonEvent = nullptr;
    for (Season &s : context->athlete->seasons->seasons) {
        for (SeasonEvent &event : s.events) {
            // FIXME: Ugly comparison required because SeasonEvent::id is not populated
            if (   event.name == entry.primary
                && (   (event.priority == 0 && entry.secondary == tr("Uncategorized"))
                    || (event.priority == 1 && entry.secondary == tr("Category A"))
                    || (event.priority == 2 && entry.secondary == tr("Category B"))
                    || (event.priority == 3 && entry.secondary == tr("Category C"))
                    || (event.priority == 4 && entry.secondary == tr("Category D"))
                    || (   (event.priority < 0 || event.priority > 4)
                        && entry.secondary == tr("Category E")))
                && event.description.trimmed() == entry.tertiary
                && event.id == entry.reference
                && event.date == entry.spanStart
                && event.date == entry.spanEnd) {
                season = &s;
                seasonEvent = &event;
                break;
            }
        }
        if (seasonEvent != nullptr) {
            break;
        }
    }
    if (seasonEvent == nullptr) {
        return;
    }
    EditSeasonEventDialog dialog(context, seasonEvent, *season);
    if (dialog.exec()) {
        context->athlete->seasons->writeSeasons();
        updateActivities();
    }
}
