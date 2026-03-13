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

#include "PlanAdherenceWindow.h"

#include "Athlete.h"
#include "RideMetadata.h"
#include "Seasons.h"
#include "RideItem.h"
#include "Colors.h"
#include "IconManager.h"
#include "PlanAdherence.h"

#define HLO "<h4>"
#define HLC "</h4>"


PlanAdherenceWindow::PlanAdherenceWindow(Context *context)
: GcChartWindow(context), context(context)
{
    mkControls();

    adherenceView = new PlanAdherenceView();

    titleMainCombo->setCurrentText("Route");
    titleFallbackCombo->setCurrentText("Workout Code");
    setMaxDaysBefore(1);
    setMaxDaysAfter(5);
    setPreferredStatisticsDisplay(1);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setChartLayout(mainLayout);
    mainLayout->addWidget(adherenceView);

    connect(adherenceView, &PlanAdherenceView::monthChanged, this, &PlanAdherenceWindow::updateActivities);
    connect(adherenceView, &PlanAdherenceView::viewActivity, this, [this](QString reference, bool planned) {
        for (RideItem *rideItem : this->context->athlete->rideCache->rides()) {
            if (rideItem == nullptr || rideItem->fileName.isEmpty()) {
                continue;
            }
            if (rideItem->fileName == reference && rideItem->planned == planned) {
                this->context->notifyRideSelected(rideItem);
                this->context->mainWindow->selectAnalysis();
                break;
            }
        }
    });
    connect(context->athlete->rideCache, QOverload<RideItem*>::of(&RideCache::itemChanged), this, &PlanAdherenceWindow::updateActivitiesIfInRange);
    connect(context->athlete->seasons, &Seasons::seasonsChanged, this, [this]() {
        Season const * current = this->context->currentSeason();
        if (current != nullptr) {
            DateRange dr(current->getStart(), current->getEnd(), current->getName());
            adherenceView->setDateRange(dr);
        }
    });
    connect(context, &Context::seasonSelected, this, [this](Season const *season, bool changed) {
        if (season != nullptr) {
            if (changed || first) {
                first = false;
                DateRange dr(season->getStart(), season->getEnd(), season->getName());
                adherenceView->setDateRange(dr);
            }
        }
        updateActivities();
    });
    connect(context, &Context::filterChanged, this, &PlanAdherenceWindow::updateActivities);
    connect(context, &Context::homeFilterChanged, this, &PlanAdherenceWindow::updateActivities);
    connect(context, &Context::rideAdded, this, &PlanAdherenceWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideDeleted, this, &PlanAdherenceWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideChanged, this, &PlanAdherenceWindow::updateActivitiesIfInRange);
    connect(context, &Context::configChanged, this, &PlanAdherenceWindow::configChanged);

    QTimer::singleShot(0, this, [this]() {
        configChanged(CONFIG_APPEARANCE);
        if (this->context->currentSeason() != nullptr) {
            DateRange dr(this->context->currentSeason()->getStart(), this->context->currentSeason()->getEnd(), this->context->currentSeason()->getName());
            adherenceView->setDateRange(dr);
        }
    });
}


QString
PlanAdherenceWindow::getTitleMainField
() const
{
    return titleMainCombo->currentText();
}


void
PlanAdherenceWindow::setTitleMainField
(const QString &name)
{
    titleMainCombo->setCurrentText(name);
}


QString
PlanAdherenceWindow::getTitleFallbackField
() const
{
    return titleFallbackCombo->currentText();
}


void
PlanAdherenceWindow::setTitleFallbackField
(const QString &name)
{
    titleFallbackCombo->setCurrentText(name);
}


int
PlanAdherenceWindow::getMaxDaysBefore
() const
{
    return maxDaysBeforeSpin->value();
}


void
PlanAdherenceWindow::setMaxDaysBefore
(int days)
{
    QSignalBlocker blocker(maxDaysBeforeSpin);
    maxDaysBeforeSpin->setValue(days);
    if (adherenceView != nullptr) {
        adherenceView->setMinAllowedOffset(-maxDaysBeforeSpin->value());
        updateActivities();
    }
}


int
PlanAdherenceWindow::getMaxDaysAfter
() const
{
    return maxDaysAfterSpin->value();
}


void
PlanAdherenceWindow::setMaxDaysAfter
(int days)
{
    QSignalBlocker blocker(maxDaysAfterSpin);
    maxDaysAfterSpin->setValue(days);
    if (adherenceView != nullptr) {
        adherenceView->setMaxAllowedOffset(maxDaysAfterSpin->value());
        updateActivities();
    }
}


int
PlanAdherenceWindow::getPreferredStatisticsDisplay
() const
{
    return preferredStatisticsDisplay->currentIndex();
}


void
PlanAdherenceWindow::setPreferredStatisticsDisplay
(int mode)
{
    QSignalBlocker blocker(preferredStatisticsDisplay);
    preferredStatisticsDisplay->setCurrentIndex(mode);
    if (adherenceView != nullptr) {
        adherenceView->setPreferredStatisticsDisplay(mode);
    }
}


void
PlanAdherenceWindow::configChanged
(qint32 what)
{
    bool refreshActivities = false;
    if (   (what & CONFIG_FIELDS)
        || (what & CONFIG_USERMETRICS)) {
        updateTitleConfigCombos();
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
        adherenceView->applyNavIcons();
    }

    if (refreshActivities) {
        updateActivities();
    }
}


bool
PlanAdherenceWindow::isFiltered
() const
{
    return (context->ishomefiltered || context->isfiltered);
}


void
PlanAdherenceWindow::mkControls
()
{
    titleMainCombo = new QComboBox();
    titleFallbackCombo = new QComboBox();
    updateTitleConfigCombos();

    maxDaysBeforeSpin = new QSpinBox();
    maxDaysBeforeSpin->setMinimum(1);
    maxDaysBeforeSpin->setMaximum(14);

    maxDaysAfterSpin = new QSpinBox();
    maxDaysAfterSpin->setMinimum(1);
    maxDaysAfterSpin->setMaximum(14);

    preferredStatisticsDisplay = new QComboBox();
    preferredStatisticsDisplay->addItem(tr("Absolute"));
    preferredStatisticsDisplay->addItem(tr("Percentage"));

    QFormLayout *controlsForm = newQFormLayout();
    controlsForm->setContentsMargins(0, 10 * dpiYFactor, 0, 10 * dpiYFactor);

    controlsForm->addRow(new QLabel(HLO + tr("Activity title") + HLC));
    controlsForm->addRow(tr("Field"), titleMainCombo);
    controlsForm->addRow(tr("Fallback Field"), titleFallbackCombo);
    controlsForm->addItem(new QSpacerItem(0, 20 * dpiYFactor));
    controlsForm->addRow(new QLabel(HLO + tr("Adherence window (days)") + HLC));
    controlsForm->addRow(tr("Before"), maxDaysBeforeSpin);
    controlsForm->addRow(tr("After"), maxDaysAfterSpin);
    controlsForm->addItem(new QSpacerItem(0, 20 * dpiYFactor));
    controlsForm->addRow(tr("Statistics display"), preferredStatisticsDisplay);

    connect(titleMainCombo, &QComboBox::currentIndexChanged, this, &PlanAdherenceWindow::updateActivities);
    connect(titleFallbackCombo, &QComboBox::currentIndexChanged, this, &PlanAdherenceWindow::updateActivities);
    connect(maxDaysBeforeSpin, &QSpinBox::valueChanged, this, &PlanAdherenceWindow::setMaxDaysBefore);
    connect(maxDaysAfterSpin, &QSpinBox::valueChanged, this, &PlanAdherenceWindow::setMaxDaysAfter);
    connect(preferredStatisticsDisplay, &QComboBox::currentIndexChanged, this, &PlanAdherenceWindow::setPreferredStatisticsDisplay);

    setControls(centerLayoutInWidget(controlsForm));
}


void
PlanAdherenceWindow::updateTitleConfigCombos
()
{
    QString mainField = getTitleMainField();
    QString fallbackField = getTitleFallbackField();

    QSignalBlocker blocker1(titleMainCombo);
    QSignalBlocker blocker2(titleFallbackCombo);

    titleMainCombo->clear();
    titleFallbackCombo->clear();
    QList<FieldDefinition> fieldsDefs = GlobalContext::context()->rideMetadata->getFields();
    for (const FieldDefinition &fieldDef : fieldsDefs) {
        if (fieldDef.isTextField()) {
            titleMainCombo->addItem(fieldDef.name);
            titleFallbackCombo->addItem(fieldDef.name);
        }
    }

    setTitleMainField(mainField);
    setTitleFallbackField(fallbackField);
}


QString
PlanAdherenceWindow::getRideItemTitle
(RideItem const * const rideItem) const
{
    QString title = rideItem->getText(getTitleMainField(), rideItem->getText(getTitleFallbackField(), "")).trimmed();
    if (title.isEmpty()) {
        if (! rideItem->sport.isEmpty()) {
            title = tr("Unnamed %1").arg(rideItem->sport);
        } else {
            title = tr("<unknown>");
        }
    }
    return title;
}


void
PlanAdherenceWindow::updateActivities
()
{
    QList<PlanAdherenceEntry> entries;
    PlanAdherenceStatistics statistics;
    PlanAdherenceOffsetRange offsetRange;
    QDate firstVisible = adherenceView->firstVisibleDay();
    QDate lastVisible = adherenceView->lastVisibleDay();
    QDate today = QDate::currentDate();
    for (RideItem *rideItem : context->athlete->rideCache->rides()) {
        if (   rideItem == nullptr
            || (! rideItem->planned && rideItem->hasLinkedActivity())
            || (context->isfiltered && ! context->filters.contains(rideItem->fileName))
            || (context->ishomefiltered && ! context->homeFilters.contains(rideItem->fileName))) {
            continue;
        }
        QDate rideDate = rideItem->dateTime.date();
        QString originalDateString = rideItem->getText("Original Date", "");
        QDate originalDate(rideDate);
        if (! originalDateString.isEmpty()) {
            originalDate = QDate::fromString(originalDateString, "yyyy/MM/dd");
            if (! originalDate.isValid()) {
                originalDate = rideDate;
            }
        }
        if (   (firstVisible.isValid() && originalDate < firstVisible)
            || (lastVisible.isValid() && originalDate > lastVisible)) {
            continue;
        }

        PlanAdherenceEntry entry;

        entry.titlePrimary = getRideItemTitle(rideItem);
        entry.iconFile = IconManager::instance().getFilepath(rideItem);

        entry.date = originalDate;
        entry.isPlanned = rideItem->planned;
        if (entry.isPlanned) {
            entry.plannedReference = rideItem->fileName;
            entry.color = GColor(CCALPLANNED);
        } else {
            entry.actualReference = rideItem->fileName;
            if (rideItem->color.alpha() < 255) {
                entry.color = GCColor::invertColor(GColor(CPLOTBACKGROUND));
            } else {
                entry.color = rideItem->color;
            }
        }

        if (entry.isPlanned && originalDate != rideDate) {
            entry.shiftOffset = originalDate.daysTo(rideDate);
            offsetRange.min = std::min(offsetRange.min, entry.shiftOffset.value());
            offsetRange.max = std::max(offsetRange.max, entry.shiftOffset.value());
        } else {
            entry.shiftOffset.reset();
        }

        RideItem *linkedItem = nullptr;
        if (! rideItem->getLinkedFileName().isEmpty()) {
            linkedItem = context->athlete->rideCache->getRide(rideItem->getLinkedFileName());
        }
        if (entry.isPlanned && linkedItem != nullptr) {
            if (linkedItem->color.alpha() < 255) {
                entry.color = GCColor::invertColor(GColor(CPLOTBACKGROUND));
            } else {
                entry.color = linkedItem->color;
            }
            entry.titleSecondary = getRideItemTitle(linkedItem);
            entry.actualReference = linkedItem->fileName;
            entry.actualOffset = originalDate.daysTo(linkedItem->dateTime.date());
            offsetRange.min = std::min(offsetRange.min, entry.actualOffset.value());
            offsetRange.max = std::max(offsetRange.max, entry.actualOffset.value());
        } else {
            entry.actualOffset.reset();
        }

        entries << entry;

        ++statistics.totalAbs;
        if (entry.isPlanned) {
            ++statistics.plannedAbs;
            if (entry.shiftOffset != std::nullopt) {
                ++statistics.shiftedAbs;
                statistics.totalShiftDaysAbs += std::abs(entry.shiftOffset.value());
            }
            if (entry.actualOffset != std::nullopt) {
                if (entry.actualOffset.value() == 0) {
                    ++statistics.onTimeAbs;
                }
            } else if (entry.date < today) {
                ++statistics.missedAbs;
            }
        } else {
            ++statistics.unplannedAbs;
        }
    }
    std::sort(entries.begin(), entries.end(), [](const PlanAdherenceEntry &a, const PlanAdherenceEntry &b) {
        return a.date < b.date;
    });
    if (statistics.totalAbs > 0) {
        statistics.plannedRel = 100.0 * statistics.plannedAbs / statistics.totalAbs;
        statistics.unplannedRel = 100.0 * statistics.unplannedAbs / statistics.totalAbs;
    }
    if (statistics.plannedAbs > 0) {
        statistics.onTimeRel = 100.0 * statistics.onTimeAbs / statistics.plannedAbs;
        statistics.missedRel = 100.0 * statistics.missedAbs / statistics.plannedAbs;
        statistics.shiftedRel = 100.0 * statistics.shiftedAbs / statistics.plannedAbs;
    }
    if (statistics.shiftedAbs > 0) {
        statistics.avgShift = statistics.totalShiftDaysAbs / static_cast<float>(statistics.shiftedAbs);
    }

    adherenceView->fillEntries(entries, statistics, offsetRange, isFiltered());
}


void
PlanAdherenceWindow::updateActivitiesIfInRange
(RideItem *rideItem)
{
    if (   rideItem != nullptr
        && rideItem->dateTime.date() >= adherenceView->firstVisibleDay()
        && rideItem->dateTime.date() <= adherenceView->lastVisibleDay()) {
        updateActivities();
    }
}
