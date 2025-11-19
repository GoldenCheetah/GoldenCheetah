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

#include "CalendarWindow.h"

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


CalendarWindow::CalendarWindow(Context *context)
: GcChartWindow(context), context(context)
{
    mkControls();

    calendar = new Calendar(QDate::currentDate(), static_cast<Qt::DayOfWeek>(getFirstDayOfWeek()), context->athlete->measures);

    setStartHour(8);
    setEndHour(21);
    setAgendaPastDays(7);
    setAgendaFutureDays(7);

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
    connect(context, &Context::filterChanged, this, &CalendarWindow::updateActivities);
    connect(context, &Context::homeFilterChanged, this, &CalendarWindow::updateActivities);
    connect(context, &Context::rideAdded, this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideDeleted, this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideChanged, this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::configChanged, this, &CalendarWindow::configChanged);
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
    connect(calendar, &Calendar::moveActivity, [=](CalendarEntry activity, const QDate &srcDay, const QDate &destDay, const QTime &destTime) {
        Q_UNUSED(srcDay)

        QApplication::setOverrideCursor(Qt::WaitCursor);
        for (RideItem *rideItem : context->athlete->rideCache->rides()) {
            if (rideItem != nullptr && rideItem->fileName == activity.reference) {
                movePlannedActivity(rideItem, destDay, destTime);
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
            movePlannedActivity(plannedRides[i], destDay, plannedRides[i]->dateTime.time());
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
                movePlannedActivity(rideItem, destDay, rideItem->dateTime.time());
            }
        }
        QApplication::restoreOverrideCursor();
    });
    connect(calendar, &Calendar::dayChanged, this, &CalendarWindow::updateActivities);
    connect(calendar, &Calendar::monthChanged, this, &CalendarWindow::updateActivities);
    connect(calendar, &Calendar::viewChanged, this, &CalendarWindow::updateActivities);

    QTimer::singleShot(0, this, [this]() {
        configChanged(CONFIG_APPEARANCE);
    });
}


int
CalendarWindow::getDefaultView
() const
{
    return defaultViewCombo->currentIndex();
}


void
CalendarWindow::setDefaultView
(int view)
{
    defaultViewCombo->setCurrentIndex(view);
    calendar->setView(static_cast<CalendarView>(view));
}


int
CalendarWindow::getFirstDayOfWeek
() const
{
    return firstDayOfWeekCombo->currentIndex() + 1;
}


void
CalendarWindow::setFirstDayOfWeek
(int fdw)
{
    firstDayOfWeekCombo->setCurrentIndex(std::min(static_cast<int>(Qt::Sunday), std::max(static_cast<int>(Qt::Monday), fdw)) - 1);
    calendar->setFirstDayOfWeek(static_cast<Qt::DayOfWeek>(fdw));
}


int
CalendarWindow::getStartHour
() const
{
    return startHourSpin->value();
}


void
CalendarWindow::setStartHour
(int hour)
{
    startHourSpin->setValue(hour);
    endHourSpin->setMinimum(hour + 1);
    if (calendar != nullptr) {
        calendar->setStartHour(hour);
        updateActivities();
    }
}


int
CalendarWindow::getEndHour
() const
{
    return endHourSpin->value();
}


void
CalendarWindow::setEndHour
(int hour)
{
    endHourSpin->setValue(hour);
    startHourSpin->setMaximum(hour - 1);
    if (calendar != nullptr) {
        calendar->setEndHour(hour);
        updateActivities();
    }
}


int
CalendarWindow::getAgendaPastDays
() const
{
    return agendaPastDaysSpin->value();
}


void
CalendarWindow::setAgendaPastDays
(int days)
{
    agendaPastDaysSpin->setValue(days);
    if (calendar != nullptr) {
        calendar->setAgendaPastDays(days);
        updateActivities();
    }
}


int
CalendarWindow::getAgendaFutureDays
() const
{
    return agendaFutureDaysSpin->value();
}


void
CalendarWindow::setAgendaFutureDays
(int days)
{
    agendaFutureDaysSpin->setValue(days);
    if (calendar != nullptr) {
        calendar->setAgendaFutureDays(days);
        updateActivities();
    }
}


bool
CalendarWindow::isSummaryVisibleDay
() const
{
    return summaryDayCheck->isChecked();
}


void
CalendarWindow::setSummaryVisibleDay
(bool visible)
{
    summaryDayCheck->setChecked(visible);
    calendar->setSummaryDayVisible(visible);
}


bool
CalendarWindow::isSummaryVisibleWeek
() const
{
    return summaryWeekCheck->isChecked();
}


void
CalendarWindow::setSummaryVisibleWeek
(bool visible)
{
    summaryWeekCheck->setChecked(visible);
    calendar->setSummaryWeekVisible(visible);
}


bool
CalendarWindow::isSummaryVisibleMonth
() const
{
    return summaryMonthCheck->isChecked();
}


void
CalendarWindow::setSummaryVisibleMonth
(bool visible)
{
    summaryMonthCheck->setChecked(visible);
    calendar->setSummaryMonthVisible(visible);
}


bool
CalendarWindow::isFiltered
() const
{
    return (context->ishomefiltered || context->isfiltered);
}


QString
CalendarWindow::getPrimaryMainField
() const
{
    return primaryMainCombo->currentText();
}


void
CalendarWindow::setPrimaryMainField
(const QString &name)
{
    primaryMainCombo->setCurrentText(name);
}


QString
CalendarWindow::getPrimaryFallbackField
() const
{
    return primaryFallbackCombo->currentText();
}


void
CalendarWindow::setPrimaryFallbackField
(const QString &name)
{
    primaryFallbackCombo->setCurrentText(name);
}


QString
CalendarWindow::getSecondaryMetric
() const
{
    return secondaryCombo->currentData(Qt::UserRole).toString();
}


QString
CalendarWindow::getTertiaryField
() const
{
    return tertiaryCombo->currentText();
}


void
CalendarWindow::setTertiaryField
(const QString &name)
{
    tertiaryCombo->setCurrentText(name);
}


QString
CalendarWindow::getSummaryMetrics
() const
{
    return multiMetricSelector->getSymbols().join(',');
}


QStringList
CalendarWindow::getSummaryMetricsList
() const
{
    return multiMetricSelector->getSymbols();
}


void
CalendarWindow::setSecondaryMetric
(const QString &name)
{
    secondaryCombo->setCurrentIndex(std::max(0, secondaryCombo->findData(name)));
}


void
CalendarWindow::setSummaryMetrics
(const QString &summaryMetrics)
{
    multiMetricSelector->setSymbols(summaryMetrics.split(',', Qt::SkipEmptyParts));
}


void
CalendarWindow::setSummaryMetrics
(const QStringList &summaryMetrics)
{
    multiMetricSelector->setSymbols(summaryMetrics);
}


void
CalendarWindow::configChanged
(qint32 what)
{
    bool refreshActivities = false;
    if (   (what & CONFIG_FIELDS)
        || (what & CONFIG_USERMETRICS)) {
        updatePrimaryConfigCombos();
        updateSecondaryConfigCombo();
        multiMetricSelector->updateMetrics();
    }
    if (what & CONFIG_ATHLETE) {
        calendar->updateMeasures();
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

        QPalette palette;
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
        calendar->applyNavIcons();
        refreshActivities = true;
    }

    if (refreshActivities) {
        updateActivities();
    }
}


void
CalendarWindow::mkControls
()
{
    QLocale locale;
    defaultViewCombo = new QComboBox();
    defaultViewCombo->addItem(tr("Day"));
    defaultViewCombo->addItem(tr("Week"));
    defaultViewCombo->addItem(tr("Month"));
    defaultViewCombo->addItem(tr("Agenda"));
    defaultViewCombo->setCurrentIndex(static_cast<int>(CalendarView::Month));
    firstDayOfWeekCombo = new QComboBox();
    for (int i = Qt::Monday; i <= Qt::Sunday; ++i) {
        firstDayOfWeekCombo->addItem(locale.dayName(i, QLocale::LongFormat));
    }
    firstDayOfWeekCombo->setCurrentIndex(locale.firstDayOfWeek() - 1);
    startHourSpin = new QSpinBox();
    startHourSpin->setSuffix(":00");
    startHourSpin->setMinimum(0);
    endHourSpin = new QSpinBox();
    endHourSpin->setSuffix(":00");
    endHourSpin->setMaximum(24);
    agendaPastDaysSpin = new QSpinBox();
    agendaPastDaysSpin->setMaximum(31);
    agendaPastDaysSpin->setSuffix(" " + tr("day(s)"));
    agendaFutureDaysSpin = new QSpinBox();
    agendaFutureDaysSpin->setMaximum(31);
    agendaFutureDaysSpin->setSuffix(" " + tr("day(s)"));
    summaryDayCheck = new QCheckBox(tr("Day View"));
    summaryDayCheck->setChecked(true);
    summaryWeekCheck = new QCheckBox(tr("Week View"));
    summaryWeekCheck->setChecked(true);
    summaryMonthCheck = new QCheckBox(tr("Month View"));
    summaryMonthCheck->setChecked(true);
    primaryMainCombo = new QComboBox();
    primaryFallbackCombo = new QComboBox();
    secondaryCombo = new QComboBox();
    tertiaryCombo = new QComboBox();
    updatePrimaryConfigCombos();
    updateSecondaryConfigCombo();
    updateTertiaryConfigCombo();
    primaryMainCombo->setCurrentText("Route");
    primaryFallbackCombo->setCurrentText("Workout Code");
    int secondaryIndex = secondaryCombo->findData("workout_time");
    if (secondaryIndex >= 0) {
        secondaryCombo->setCurrentIndex(secondaryIndex);
    }
    tertiaryCombo->setCurrentText("Notes");
    QStringList summaryMetrics { "ride_count", "total_distance", "coggan_tss", "workout_time" };
    multiMetricSelector = new MultiMetricSelector(tr("Available Metrics"), tr("Selected Metrics"), summaryMetrics);
    multiMetricSelector->setContentsMargins(10 * dpiXFactor, 10 * dpiYFactor, 10 * dpiXFactor, 10 * dpiYFactor);
    multiMetricSelector->setMinimumHeight(300 * dpiYFactor);

    QFormLayout *generalForm = newQFormLayout();
    generalForm->setContentsMargins(0, 10 * dpiYFactor, 0, 10 * dpiYFactor);
    generalForm->addRow(tr("Startup View"), defaultViewCombo);
    generalForm->addRow(tr("First Day of Week"), firstDayOfWeekCombo);
    generalForm->addRow(tr("Default Start Time"), startHourSpin);
    generalForm->addRow(tr("Default End Time"), endHourSpin);
    generalForm->addRow(tr("Agenda: Look Back"), agendaPastDaysSpin);
    generalForm->addRow(tr("Agenda: Look Forward"), agendaFutureDaysSpin);
    generalForm->addRow(tr("Show Summary In"), summaryDayCheck);
    generalForm->addRow("", summaryWeekCheck);
    generalForm->addRow("", summaryMonthCheck);

    QFormLayout *entriesForm = newQFormLayout();
    entriesForm->setContentsMargins(0, 10 * dpiYFactor, 0, 10 * dpiYFactor);
    entriesForm->addRow(tr("Field for Primary Line"), primaryMainCombo);
    entriesForm->addRow(tr("Fallback Field for Primary Line"), primaryFallbackCombo);
    entriesForm->addRow(tr("Metric for Secondary Line"), secondaryCombo);
    entriesForm->addRow(tr("Field for Tertiary Line (Day and Week View)"), tertiaryCombo);

    QTabWidget *controlsTabs = new QTabWidget();
    controlsTabs->addTab(centerLayoutInWidget(generalForm, false), tr("General"));
    controlsTabs->addTab(centerLayoutInWidget(entriesForm, false), tr("Calendar Entries"));
    controlsTabs->addTab(multiMetricSelector, tr("Summary"));

#if QT_VERSION < 0x060000
    connect(startHourSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CalendarWindow::setStartHour);
    connect(endHourSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CalendarWindow::setEndHour);
    connect(agendaPastDaysSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CalendarWindow::setAgendaPastDays);
    connect(agendaFutureDaysSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &CalendarWindow::setAgendaFutureDays);
    connect(defaultViewCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CalendarWindow::setDefaultView);
    connect(firstDayOfWeekCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int idx) { setFirstDayOfWeek(idx + 1); });
    connect(primaryMainCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CalendarWindow::updateActivities);
    connect(primaryFallbackCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CalendarWindow::updateActivities);
    connect(secondaryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CalendarWindow::updateActivities);
    connect(tertiaryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CalendarWindow::updateActivities);
#else
    connect(startHourSpin, &QSpinBox::valueChanged, this, &CalendarWindow::setStartHour);
    connect(endHourSpin, &QSpinBox::valueChanged, this, &CalendarWindow::setEndHour);
    connect(agendaPastDaysSpin, &QSpinBox::valueChanged, this, &CalendarWindow::setAgendaPastDays);
    connect(agendaFutureDaysSpin, &QSpinBox::valueChanged, this, &CalendarWindow::setAgendaFutureDays);
    connect(defaultViewCombo, &QComboBox::currentIndexChanged, this, &CalendarWindow::setDefaultView);
    connect(firstDayOfWeekCombo, &QComboBox::currentIndexChanged, [=](int idx) { setFirstDayOfWeek(idx + 1); });
    connect(primaryMainCombo, &QComboBox::currentIndexChanged, this, &CalendarWindow::updateActivities);
    connect(primaryFallbackCombo, &QComboBox::currentIndexChanged, this, &CalendarWindow::updateActivities);
    connect(secondaryCombo, &QComboBox::currentIndexChanged, this, &CalendarWindow::updateActivities);
    connect(tertiaryCombo, &QComboBox::currentIndexChanged, this, &CalendarWindow::updateActivities);
#endif
    connect(summaryDayCheck, &QCheckBox::toggled, this, &CalendarWindow::setSummaryVisibleDay);
    connect(summaryWeekCheck, &QCheckBox::toggled, this, &CalendarWindow::setSummaryVisibleWeek);
    connect(summaryMonthCheck, &QCheckBox::toggled, this, &CalendarWindow::setSummaryVisibleMonth);
    connect(multiMetricSelector, &MultiMetricSelector::selectedChanged, this, &CalendarWindow::updateActivities);

    setControls(controlsTabs);
}


void
CalendarWindow::updatePrimaryConfigCombos
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
CalendarWindow::updateSecondaryConfigCombo
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
CalendarWindow::updateTertiaryConfigCombo
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
CalendarWindow::getActivities
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
            activity.secondaryMetric = rideMetricName;
        } else {
            activity.secondary = "";
            activity.secondaryMetric = "";
        }
        activity.tertiary = rideItem->getText(getTertiaryField(), "").trimmed();
        activity.primary = Utils::unprotect(activity.primary);
        activity.secondary = Utils::unprotect(activity.secondary);
        activity.secondaryMetric = Utils::unprotect(activity.secondaryMetric);
        activity.tertiary = Utils::unprotect(activity.tertiary);

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


QList<CalendarSummary>
CalendarWindow::getSummaries
(const QDate &firstDay, const QDate &lastDay, int timeBucketSize) const
{
    QStringList symbols = getSummaryMetricsList();
    QList<CalendarSummary> summaries;
    int numTimeBuckets = firstDay.daysTo(lastDay) / timeBucketSize + 1;
    bool useMetricUnits = GlobalContext::context()->useMetricUnits;

    const RideMetricFactory &factory = RideMetricFactory::instance();
    FilterSet filterSet(context->isfiltered, context->filters);
    Specification spec;
    spec.setFilterSet(filterSet);
    for (int timeBucket = 0; timeBucket < numTimeBuckets; ++timeBucket) {
        QDate firstDayOfTimeBucket = firstDay.addDays(timeBucket * timeBucketSize);
        QDate lastDayOfTimeBucket = firstDayOfTimeBucket.addDays(timeBucketSize - 1);
        spec.setDateRange(DateRange(firstDayOfTimeBucket, lastDayOfTimeBucket));
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
CalendarWindow::getPhasesEvents
(const Season &season, const QDate &firstDay, const QDate &lastDay) const
{
    QHash<QDate, QList<CalendarEntry>> phasesEvents;
    QList<Season> tmpSeasons = context->athlete->seasons->seasons;
    std::sort(tmpSeasons.begin(), tmpSeasons.end(), Season::LessThanForStarts);
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
                entry.color = GColor(CCALEVENT);
                entry.reference = event.id;
                entry.start = QTime(0, 0, 0);
                entry.durationSecs = 0;
                entry.type = ENTRY_TYPE_EVENT;
                entry.isRelocatable = false;
                phasesEvents[event.date] << entry;
            }
        }
    }
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
                    entry.color = GColor(CCALPHASE);
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

    return phasesEvents;
}


void
CalendarWindow::updateActivities
()
{
    Season const *season = context->currentSeason();
    if (!season) return; // avoid crash if no season selected

    QList<CalendarSummary> summaries;
    QHash<QDate, QList<CalendarEntry>> activities = getActivities(calendar->firstVisibleDay(), calendar->lastVisibleDay());
    QHash<QDate, QList<CalendarEntry>> phasesEvents = getPhasesEvents(*season, calendar->firstVisibleDay(), calendar->lastVisibleDay());
    if (calendar->currentView() == CalendarView::Day) {
        QDate selectedDate = calendar->selectedDate();
        summaries = getSummaries(selectedDate, selectedDate, 1);
    } else if (calendar->currentView() == CalendarView::Week) {
        summaries = getSummaries(calendar->firstVisibleDay(), calendar->lastVisibleDay(), 1);
    } else {
        summaries = getSummaries(calendar->firstVisibleDay(), calendar->lastVisibleDay(), 7);
    }
    calendar->fillEntries(activities, summaries, phasesEvents, isFiltered());
}


void
CalendarWindow::updateActivitiesIfInRange
(RideItem *rideItem)
{
    if (calendar->currentView() == CalendarView::Day) {
        if (rideItem->dateTime.date() == calendar->selectedDate()) {
            updateActivities();
        }
    } else {
        if (   rideItem->dateTime.date() >= calendar->firstVisibleDay()
            && rideItem->dateTime.date() <= calendar->lastVisibleDay()) {
            updateActivities();
        }
    }
}


void
CalendarWindow::updateSeason
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
CalendarWindow::movePlannedActivity
(RideItem *rideItem, const QDate &destDay, const QTime &destTime)
{
    bool ret = false;
    RideFile *rideFile = rideItem->ride();

    QDateTime rideDateTime(destDay, destTime);
    rideFile->setStartTime(rideDateTime);
    QString basename = rideDateTime.toString("yyyy_MM_dd_HH_mm_ss");

    QString filename;
    if (rideItem->planned) {
        filename = context->athlete->home->planned().canonicalPath() + "/" + basename + ".json";
    } else {
        filename = context->athlete->home->activities().canonicalPath() + "/" + basename + ".json";
    }
    QFile out(filename);
    if (   ! out.exists()
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
