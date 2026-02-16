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
#include <QDialogButtonBox>

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
#include "FilterSimilarDialog.h"
#include "SeasonDialogs.h"
#include "SaveDialogs.h"

#define HLO "<h4>"
#define HLC "</h4>"


//////////////////////////////////////////////////////////////////////////////
// LinkDialog

LinkDialog::LinkDialog
(const LinkEntry &entry, QList<LinkEntry> &candidates, QWidget *parent)
: QDialog(parent), entry(entry), candidates(candidates)
{
    setModal(true);
    setWindowTitle(tr("Link Activity"));
    setMinimumSize(720 * dpiXFactor, 550 * dpiYFactor);
    resize(720 * dpiXFactor, 550 * dpiYFactor);
    QLocale locale;

    QLabel *introLabel = new QLabel();
    if (entry.planned) {
        introLabel->setText(tr("Find an actual activity for:"));
    } else {
        introLabel->setText(tr("Find a planned activity for:"));
    }

    QLabel *primaryLabel = new QLabel(entry.primary);
    QFont primaryFont = primaryLabel->font();
    primaryFont.setPointSize(primaryFont.pointSize() * 1.2);
    primaryFont.setWeight(QFont::Bold);
    primaryLabel->setFont(primaryFont);

    QLabel *secondaryLabel = new QLabel();
    if (entry.secondary.isEmpty() || entry.secondaryMetric.isEmpty()) {
        secondaryLabel->setText(QString("%1 • %2")
                                       .arg(locale.toString(entry.date, QLocale::NarrowFormat))
                                       .arg(locale.toString(entry.time, QLocale::NarrowFormat)));
    } else {
        secondaryLabel->setText(QString("%1 • %2 • %3 <font style='font-weight: 250'>%4</font>")
                                       .arg(locale.toString(entry.date, QLocale::NarrowFormat))
                                       .arg(locale.toString(entry.time, QLocale::NarrowFormat))
                                       .arg(entry.secondary)
                                       .arg(entry.secondaryMetric));
    }

    QVBoxLayout *entryLabelLayout = new QVBoxLayout();
    entryLabelLayout->addWidget(primaryLabel);
    entryLabelLayout->addWidget(secondaryLabel);

    int entryHeight = primaryLabel->sizeHint().height() + secondaryLabel->sizeHint().height() + std::max(0, entryLabelLayout->spacing());
    QPixmap pixmap;
    if (entry.planned) {
        pixmap = svgAsColoredPixmap(entry.iconFile, QSize(entryHeight, entryHeight), 0, entry.iconColor);
    } else {
        pixmap = svgOnBackground(entry.iconFile, QSize(entryHeight, entryHeight), 3 * dpiXFactor, entry.iconColor, 5 * dpiXFactor);
    }

    QLabel *iconLabel = new QLabel();
    iconLabel->setPixmap(pixmap);
    iconLabel->setFixedSize(pixmap.size());
    iconLabel->setAlignment(Qt::AlignCenter);

    QFrame *entryFrame = new QFrame();
    entryFrame->setFrameShape(QFrame::StyledPanel);
    QPalette palette = entryFrame->palette();
    QColor tint = entry.iconColor;
    tint.setAlpha(25);
    QColor entryBG = GCColor::blendedColor(tint, palette.window().color());
    entryFrame->setStyleSheet(QString("QFrame { background-color: %1; }").arg(entryBG.name()));

    QHBoxLayout *entryLayout = new QHBoxLayout(entryFrame);
    entryLayout->setContentsMargins(8 * dpiXFactor, 8 * dpiYFactor, 8 * dpiXFactor, 8 * dpiYFactor);
    entryLayout->addWidget(iconLabel);
    entryLayout->addSpacing(10 * dpiXFactor);
    entryLayout->addLayout(entryLabelLayout);
    entryLayout->addStretch();

    QLabel *rangeLabel = new QLabel();
    if (entry.planned) {
        rangeLabel->setText(tr("Show <b>actual %2</b> activities").arg(entry.sport));
    } else {
        rangeLabel->setText(tr("Show <b>planned %2</b> activities").arg(entry.sport));
    }

    QComboBox *rangeCombo = new QComboBox();
    rangeCombo->addItem(tr("14 days backward"));
    rangeCombo->addItem(tr("7 days backward"));
    rangeCombo->addItem(tr("±7 days"));
    rangeCombo->addItem(tr("7 days forward"));
    rangeCombo->addItem(tr("14 days forward"));

    QHBoxLayout *rangeLayout = new QHBoxLayout();
    rangeLayout->addWidget(rangeLabel);
    rangeLayout->addWidget(rangeCombo);
    rangeLayout->addStretch();

    QFrame *emptyFrame = new QFrame();
    emptyFrame->setFrameShape(QFrame::StyledPanel);
    emptyFrame->setStyleSheet(QString("QFrame { background-color: %1; }").arg(palette.base().color().name()));


    QString emptyHeadline = tr("No matching activities found");
    QString emptyInfoline;
    if (entry.planned) {
        emptyInfoline = tr("There are no unlinked actual %1 activities in the selected date range.").arg(entry.sport);
    } else {
        emptyInfoline = tr("There are no unlinked planned %1 activities in the selected date range.").arg(entry.sport);
    }
    QString emptyActionline = tr("Try adjusting the date range filter or create a new activity.");
    QString emptyMsg = QString("<h3>%1</h3>%2<br/>%3").arg(emptyHeadline).arg(emptyInfoline).arg(emptyActionline);
    QLabel *emptyLabel = new QLabel(emptyMsg);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setWordWrap(true);

    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyFrame);
    emptyLayout->setContentsMargins(8 * dpiXFactor, 8 * dpiYFactor, 8 * dpiXFactor, 8 * dpiYFactor);
    emptyLayout->addWidget(emptyLabel);

    QStringList columnLabels = {
        "",
        tr("Activity"),
        tr("Date"),
        tr("Days"),
        tr("Time"),
        entry.secondaryMetric
    };

    candidateTree = new QTreeWidget();
    candidateTree->setColumnCount(6);
    candidateTree->setHeaderLabels(columnLabels);
    candidateTree->setRootIsDecorated(false);
    candidateTree->setAlternatingRowColors(true);
    candidateTree->setSelectionMode(QTreeWidget::SingleSelection);
    candidateTree->setStyleSheet(QString("QTreeView::item { padding: %1px; }").arg(4 * dpiYFactor));
    candidateTree->header()->setStretchLastSection(false);
    candidateTree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    candidateTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    candidateTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    candidateTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    candidateTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    candidateTree->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);

    candidateStack = new QStackedWidget();
    candidateStack->addWidget(emptyFrame);
    candidateStack->addWidget(candidateTree);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    acceptButton = buttonBox->button(QDialogButtonBox::Ok);
    acceptButton->setText(tr("Link"));
    acceptButton->setEnabled(false);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(introLabel);
    mainLayout->addSpacing(5 * dpiYFactor);
    mainLayout->addWidget(entryFrame);
    mainLayout->addSpacing(18 * dpiYFactor);
    mainLayout->addLayout(rangeLayout);
    mainLayout->addSpacing(12 * dpiYFactor);
    mainLayout->addWidget(candidateStack, 1);
    mainLayout->addSpacing(20 * dpiYFactor);
    mainLayout->addWidget(buttonBox);

    connect(rangeCombo, &QComboBox::currentIndexChanged, this, &LinkDialog::updateCandidates);
    connect(candidateTree, &QTreeWidget::itemSelectionChanged, this, [this]() {
        acceptButton->setEnabled(! candidateTree->selectedItems().isEmpty());
    });
    connect(candidateTree, &QTreeWidget::itemDoubleClicked, this, [this]() {
        accept();
    });

    rangeCombo->setCurrentIndex(2);
}


QString
LinkDialog::getSelectedReference
() const
{
    QList<QTreeWidgetItem*> selectedItems = candidateTree->selectedItems();
    if (! selectedItems.isEmpty()) {
        return selectedItems.first()->data(0, Qt::UserRole).toString();
    }
    return QString();
}


void
LinkDialog::updateCandidates
(int range)
{
    int from[5] = {-14, -7, -7, 0, 0};
    int to[5] = {0, 0, 7, 7, 14};
    QDate minDate = entry.date.addDays(from[range]);
    QDate maxDate = entry.date.addDays(to[range]);
    candidateTree->clear();
    int stackIdx = 0;
    int selectRow = 0;
    int i = 0;
    double selectScore = 1;
    QLocale locale;
    for (const LinkEntry &candidate : candidates) {
        if (candidate.date >= minDate && candidate.date <= maxDate) {
            stackIdx = 1;
            QString quality;
            if (candidate.matchScore <= 0.2) {
                quality = "★★★";
            } else if (candidate.matchScore <= 0.45) {
                quality = "★★☆";
            } else {
                quality = "★☆☆";
            }
            if (candidate.matchScore < selectScore) {
                selectRow = i;
                selectScore = candidate.matchScore;
            }
            QString days;
            int dayDiff = entry.date.daysTo(candidate.date);
            if (dayDiff < -1) {
                days = tr("%1 days before").arg(-dayDiff);
            } else if (dayDiff == -1) {
                days = tr("1 day before");
            } else if (dayDiff == 0) {
                days = tr("same day");
            } else if (dayDiff == 1) {
                days = tr("1 day after");
            } else if (dayDiff > 1) {
                days = tr("%1 days after").arg(dayDiff);
            }
            QTreeWidgetItem *item = new QTreeWidgetItem(candidateTree);
            item->setText(0, quality);
            item->setData(0, Qt::UserRole, candidate.reference);
            item->setText(1, candidate.primary);
            item->setText(2, locale.toString(candidate.date, QLocale::NarrowFormat));
            item->setText(3, days);
            item->setText(4, locale.toString(candidate.time, QLocale::NarrowFormat));
            item->setText(5, candidate.secondary);

            ++i;
        }
    }
    QTreeWidgetItem *item = candidateTree->topLevelItem(selectRow);
    if (item != nullptr) {
        candidateTree->setCurrentItem(item);
    }
    candidateStack->setCurrentIndex(stackIdx);
}


//////////////////////////////////////////////////////////////////////////////
// CalendarWindow

CalendarWindow::CalendarWindow(Context *context)
: GcChartWindow(context), context(context)
{
    mkControls();

    calendar = new Calendar(QDate::currentDate(), static_cast<Qt::DayOfWeek>(getFirstDayOfWeek()), context->athlete->measures);

    setStartHour(8);
    setEndHour(21);
    setShowSecondaryLabel(true);

    QVBoxLayout *mainLayout = new QVBoxLayout();
    setChartLayout(mainLayout);
    mainLayout->addWidget(calendar);

    connect(context->athlete->rideCache, QOverload<RideItem*>::of(&RideCache::itemChanged), this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context->athlete->rideCache, &RideCache::itemSaved, this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context->athlete->seasons, &Seasons::seasonsChanged, this, [this]() {
        updateSeason(this->context->currentSeason(), true);
    });
    connect(context, &Context::seasonSelected, this, [this](Season const *season, bool changed) {
        if (changed || first) {
            first = false;
            updateSeason(season, false);
        }
    });
    connect(context, &Context::filterChanged, this, &CalendarWindow::updateActivities);
    connect(context, &Context::homeFilterChanged, this, &CalendarWindow::updateActivities);
    connect(context, &Context::autoImportCompleted, this, &CalendarWindow::updateActivities);
    connect(context, &Context::rideAdded, this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideDeleted, this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideChanged, this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideSaved, this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideDirty, this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::rideClean, this, &CalendarWindow::updateActivitiesIfInRange);
    connect(context, &Context::configChanged, this, &CalendarWindow::configChanged);
    connect(calendar, &Calendar::showInTrainMode, this, [this](CalendarEntry activity) {
        RideItem *rideItem = getRideItem(activity);
        if (rideItem != nullptr) {
            QString filter = buildWorkoutFilter(rideItem);
            if (! filter.isEmpty()) {
                this->context->mainWindow->fillinWorkoutFilterBox(filter);
                this->context->mainWindow->selectTrain();
                this->context->notifySelectWorkout(0);
            }
        }
    });
    connect(calendar, &Calendar::filterSimilar, this, [this](CalendarEntry activity) {
        for (RideItem *rideItem : this->context->athlete->rideCache->rides()) {
            if (rideItem != nullptr && rideItem->fileName == activity.reference) {
                FilterSimilarDialog dlg(this->context, rideItem, this);
                dlg.exec();
                break;
            }
        }
    });
    connect(calendar, &Calendar::linkActivity, this, &CalendarWindow::linkActivities);
    connect(calendar, &Calendar::unlinkActivity, this, &CalendarWindow::unlinkActivities);
    connect(calendar, &Calendar::viewActivity, this, [this](CalendarEntry activity) {
        RideItem *rideItem = getRideItem(activity);
        if (rideItem != nullptr) {
            this->context->notifyRideSelected(rideItem);
            this->context->mainWindow->selectAnalysis();
        }
    });
    connect(calendar, &Calendar::viewLinkedActivity, this, [this](CalendarEntry entry) {
        RideItem *rideItem = getRideItem(entry, true);
        if (rideItem != nullptr) {
            this->context->notifyRideSelected(rideItem);
            this->context->mainWindow->selectAnalysis();
        }
    });
    connect(calendar, &Calendar::copyPlannedActivity, this, &CalendarWindow::copyPlannedActivity);
    connect(calendar, &Calendar::pastePlannedActivity, this, &CalendarWindow::pastePlannedActivity);
    connect(calendar, &Calendar::addActivity, this, [this](bool plan, const QDate &day, const QTime &time) {
        this->context->tab->setNoSwitch(true);
        ManualActivityWizard wizard(this->context, plan, QDateTime(day, time));
        wizard.exec();
        this->context->tab->setNoSwitch(false);
    });
    connect(calendar, &Calendar::repeatSchedule, this, [this](const QDate &day) {
        this->context->tab->setNoSwitch(true);
        RepeatScheduleWizard wizard(this->context, day);
        if (wizard.exec() == QDialog::Accepted) {
            // Context::rideDeleted is not always emitted, therefore forcing the update
            updateActivities();
        }
        this->context->tab->setNoSwitch(false);
    });
    connect(calendar, &Calendar::delActivity, this, [this](CalendarEntry activity) {
        QMessageBox::StandardButton res = QMessageBox::question(this, tr("Delete Activity"), tr("Are you sure you want to delete %1?").arg(activity.reference));
        if (res == QMessageBox::Yes) {
            if (unlinkActivities(activity)) {
                this->context->tab->setNoSwitch(true);
                this->context->athlete->rideCache->removeRide(activity.reference);
                this->context->tab->setNoSwitch(false);
            }

            // Context::rideDeleted is not always emitted, therefore forcing the update
            updateActivities();
        }
    });
    connect(calendar, &Calendar::moveActivity, this, [this](CalendarEntry activity, const QDate &srcDay, const QDate &destDay, const QTime &destTime) {
        Q_UNUSED(srcDay)

        QApplication::setOverrideCursor(Qt::WaitCursor);
        RideItem *rideItem = getRideItem(activity);
        if (rideItem != nullptr) {
            movePlannedActivity(rideItem, destDay, destTime);
        }
        QApplication::restoreOverrideCursor();
    });
    connect(calendar, &Calendar::insertRestday, this, [this](const QDate &day) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        shiftPlannedActivities(day, 1);
        updateActivities();
        QApplication::restoreOverrideCursor();
    });
    connect(calendar, &Calendar::delRestday, this, [this](const QDate &day) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        shiftPlannedActivities(day, -1);
        updateActivities();
        QApplication::restoreOverrideCursor();
    });
    connect(calendar, &Calendar::dayChanged, this, &CalendarWindow::updateActivities);
    connect(calendar, &Calendar::monthChanged, this, &CalendarWindow::updateActivities);
    connect(calendar, &Calendar::viewChanged, this, &CalendarWindow::updateActivities);
    connect(calendar, &Calendar::addEvent, this, &CalendarWindow::addEvent);
    connect(calendar, &Calendar::editEvent, this, &CalendarWindow::editEvent);
    connect(calendar, &Calendar::delEvent, this, &CalendarWindow::delEvent);
    connect(calendar, &Calendar::addPhase, this, &CalendarWindow::addPhase);
    connect(calendar, &Calendar::editPhase, this, &CalendarWindow::editPhase);
    connect(calendar, &Calendar::delPhase, this, &CalendarWindow::delPhase);
    connect(calendar, &Calendar::saveChanges, this, [this](const CalendarEntry &entry) {
        RideItem *item = getRideItem(entry, false);
        if (item != nullptr) {
            QString error;
            QList<RideItem*> activities;
            activities << item;
            relinkRideItems(this->context, item, activities);
            this->context->athlete->rideCache->saveActivities(activities, error);
        }
    });
    connect(calendar, &Calendar::discardChanges, this, [this](const CalendarEntry &entry) {
        RideItem *item = getRideItem(entry, false);
        if (item != nullptr) {
            item->close();
            item->ride();
            item->setStartTime(item->ride()->startTime());
        }
    });

    QTimer::singleShot(0, this, [this]() {
        configChanged(CONFIG_APPEARANCE);
    });
}


void
CalendarWindow::showEvent(QShowEvent*)
{
    // When the chart is added to the Perspective's QStackedWidget, the Perspective's style sheets
    // can conflict with the local calendar palette settings causing the appearance to be lost.
    // Therefore re-apply the palette upon showEvent to ensure correct calendar appearance.
    PaletteApplier::setPaletteRecursively(this, palette, true);
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


bool
CalendarWindow::isShowSecondaryLabel
() const
{
    return showSecondaryLabelCheck->isChecked();
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
CalendarWindow::setShowSecondaryLabel
(bool showSecondaryLabel)
{
    showSecondaryLabelCheck->setChecked(showSecondaryLabel);
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
        updateTertiaryConfigCombo();
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
    summaryDayCheck = new QCheckBox(tr("Day View"));
    summaryDayCheck->setChecked(true);
    summaryWeekCheck = new QCheckBox(tr("Week View"));
    summaryWeekCheck->setChecked(true);
    summaryMonthCheck = new QCheckBox(tr("Month View"));
    summaryMonthCheck->setChecked(true);
    primaryMainCombo = new QComboBox();
    primaryFallbackCombo = new QComboBox();
    secondaryCombo = new QComboBox();
    showSecondaryLabelCheck = new QCheckBox(tr("Show Label"));
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
    generalForm->addRow(new QLabel(HLO + tr("Calendar Basics") + HLC));
    generalForm->addRow(tr("Startup View"), defaultViewCombo);
    generalForm->addRow(tr("First Day of Week"), firstDayOfWeekCombo);
    generalForm->addItem(new QSpacerItem(0, 20 * dpiYFactor));
    generalForm->addRow(new QLabel(HLO + tr("Default Times") + HLC));
    generalForm->addRow(tr("Default Start Time"), startHourSpin);
    generalForm->addRow(tr("Default End Time"), endHourSpin);
    generalForm->addItem(new QSpacerItem(0, 20 * dpiYFactor));
    generalForm->addRow(new QLabel(HLO + tr("Summary Options") + HLC));
    generalForm->addRow(tr("Show Summary In"), summaryDayCheck);
    generalForm->addRow("", summaryWeekCheck);
    generalForm->addRow("", summaryMonthCheck);

    QFormLayout *entriesForm = newQFormLayout();
    entriesForm->setContentsMargins(0, 10 * dpiYFactor, 0, 10 * dpiYFactor);
    entriesForm->addRow(new QLabel(HLO + tr("Main Line") + HLC));
    entriesForm->addRow(tr("Field"), primaryMainCombo);
    entriesForm->addRow(tr("Fallback Field"), primaryFallbackCombo);
    entriesForm->addItem(new QSpacerItem(0, 20 * dpiYFactor));
    entriesForm->addRow(new QLabel(HLO + tr("Metric Line") + HLC));
    entriesForm->addRow(tr("Metric"), secondaryCombo);
    entriesForm->addRow("", showSecondaryLabelCheck);
    entriesForm->addItem(new QSpacerItem(0, 20 * dpiYFactor));
    entriesForm->addRow(new QLabel(HLO + tr("Detail Line (Day and Week View only)") + HLC));
    entriesForm->addRow(tr("Field"), tertiaryCombo);

    QTabWidget *controlsTabs = new QTabWidget();
    controlsTabs->addTab(centerLayoutInWidget(generalForm, false), tr("General"));
    controlsTabs->addTab(centerLayoutInWidget(entriesForm, false), tr("Calendar Entries"));
    controlsTabs->addTab(multiMetricSelector, tr("Summary"));

    connect(startHourSpin, &QSpinBox::valueChanged, this, &CalendarWindow::setStartHour);
    connect(endHourSpin, &QSpinBox::valueChanged, this, &CalendarWindow::setEndHour);
    connect(defaultViewCombo, &QComboBox::currentIndexChanged, this, &CalendarWindow::setDefaultView);
    connect(firstDayOfWeekCombo, &QComboBox::currentIndexChanged, this, [this](int idx) { setFirstDayOfWeek(idx + 1); });
    connect(primaryMainCombo, &QComboBox::currentIndexChanged, this, &CalendarWindow::updateActivities);
    connect(primaryFallbackCombo, &QComboBox::currentIndexChanged, this, &CalendarWindow::updateActivities);
    connect(secondaryCombo, &QComboBox::currentIndexChanged, this, &CalendarWindow::updateActivities);
    connect(tertiaryCombo, &QComboBox::currentIndexChanged, this, &CalendarWindow::updateActivities);
    connect(summaryDayCheck, &QCheckBox::toggled, this, &CalendarWindow::setSummaryVisibleDay);
    connect(summaryWeekCheck, &QCheckBox::toggled, this, &CalendarWindow::setSummaryVisibleWeek);
    connect(summaryMonthCheck, &QCheckBox::toggled, this, &CalendarWindow::setSummaryVisibleMonth);
    connect(showSecondaryLabelCheck, &QCheckBox::toggled, this, &CalendarWindow::updateActivities);
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

        activity.primary = getPrimary(rideItem);
        if (rideMetric != nullptr && rideMetric->isRelevantForRide(rideItem)) {
            activity.secondary = rideItem->getStringForSymbol(getSecondaryMetric(), GlobalContext::context()->useMetricUnits);
            if (! rideMetricUnit.isEmpty()) {
                activity.secondary += " " + rideMetricUnit;
            }
            if (isShowSecondaryLabel()) {
                activity.secondaryMetric = rideMetricName;
            }
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
        activity.type = rideItem->planned ? ENTRY_TYPE_PLANNED_ACTIVITY : ENTRY_TYPE_ACTUAL_ACTIVITY;
        activity.isRelocatable = rideItem->planned;
        activity.hasTrainMode = rideItem->planned && sport == "Bike" && ! buildWorkoutFilter(rideItem).isEmpty();
        activity.dirty = rideItem->isDirty();
        if (rideItem->planned) {
            activity.originalPlanLabel = buildOriginalLabel(rideItem);
        }

        RideItem *linkedRide = context->athlete->rideCache->getLinkedActivity(rideItem);
        if (linkedRide != nullptr) {
            activity.linkedReference = linkedRide->fileName;
            activity.linkedPrimary = getPrimary(linkedRide);
            activity.linkedStartDT = linkedRide->dateTime;
            if (linkedRide->planned) {
                activity.originalPlanLabel = buildOriginalLabel(linkedRide);
            }
        }

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

    Specification spec;
    const RideMetricFactory &factory = RideMetricFactory::instance();
    if (context->isfiltered && context->ishomefiltered) {

        // take the intersection of the two filter sets
        auto mergedSet =
                QSet<QString>(
                        context->filters.begin(),
                        context->filters.end()).intersect(
                QSet<QString>(
                        context->homeFilters.begin(),
                        context->homeFilters.end()
                        )
                );
        spec.setFilterSet(FilterSet(true, QStringList(mergedSet.begin(), mergedSet.end())));

    } else if (context->isfiltered) {
        spec.setFilterSet(FilterSet(true, context->filters));
    } else if (context->ishomefiltered) {
        spec.setFilterSet(FilterSet(true, context->homeFilters));
    }

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
                if (event.id.isEmpty()) {
                    entry.reference = QString("0x%1").arg(reinterpret_cast<quintptr>(&event), 0, 16);
                } else {
                    entry.reference = event.id;
                }
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


RideItem*
CalendarWindow::getRideItem
(const CalendarEntry &entry, bool linked)
{
    bool thisIsPlanned = (entry.type == ENTRY_TYPE_PLANNED_ACTIVITY);
    for (RideItem *rideItem : this->context->athlete->rideCache->rides()) {
        if (rideItem == nullptr || rideItem->fileName.isEmpty()) {
            continue;
        }
        if (   (   linked
                && rideItem->fileName == entry.linkedReference
                && rideItem->planned != thisIsPlanned)
            || (   ! linked
                && rideItem->fileName == entry.reference
                && rideItem->planned == thisIsPlanned)) {
            return rideItem;
        }
    }
    return nullptr;
}


QString
CalendarWindow::getPrimary
(RideItem const * const rideItem) const
{
    QString primary = rideItem->getText(getPrimaryMainField(), "").trimmed();
    if (primary.isEmpty()) {
        primary = rideItem->getText(getPrimaryFallbackField(), "").trimmed();
        if (primary.isEmpty()) {
            QString sport = rideItem->sport;
            if (! sport.isEmpty()) {
                primary = tr("Unnamed %1").arg(sport);
            } else {
                primary = tr("<unknown>");
            }
        }
    }
    return primary;
}


QTime
CalendarWindow::findFreeSlot
(RideItem *sourceItem, QDate newDate, QTime targetTime)
{
    if (sourceItem == nullptr) {
        return QTime();
    }
    QList<std::pair<QTime, int>> busySlots;
    busySlots.append(std::make_pair(QTime(0, 0), getStartHour() * 60 * 60));
    for (RideItem *rideItem : context->athlete->rideCache->rides()) {
        if (rideItem != nullptr && rideItem->planned == sourceItem->planned && rideItem->dateTime.date() == newDate) {
            busySlots.append(std::make_pair(rideItem->dateTime.time(), static_cast<int>(rideItem->getForSymbol("workout_time"))));
        }
    }
    busySlots.append(std::make_pair(QTime(getEndHour(), 0), (24 - getEndHour()) * 60 * 60 - 1));
    if (! targetTime.isValid()) {
        targetTime = sourceItem->dateTime.time();
    }
    QTime newTime = findFreeSlot(busySlots, targetTime, static_cast<int>(sourceItem->getForSymbol("workout_time")));
    return newTime;
}


QTime
CalendarWindow::findFreeSlot
(QList<std::pair<QTime, int>> busySlots, QTime targetTime, int requiredDurationSeconds) const
{
    std::sort(busySlots.begin(), busySlots.end(), [](const std::pair<QTime, int> &a, const std::pair<QTime, int> &b) {
        return a.first < b.first;
    });
    QSet<QTime> forbiddenTimes;
    for (const std::pair<QTime, int> &busySlot : busySlots) {
        forbiddenTimes.insert(busySlot.first);
    }
    QTime bestSlot;
    int bestDistance = 24 * 60 * 60;
    bool foundFullSlot = false;
    QTime dayStart = QTime(0, 0, 0);
    QTime dayEnd = QTime(23, 59, 59);
    QList<std::pair<QTime, QTime>> busyPeriods;
    for (const std::pair<QTime, int> &busySlot : busySlots) {
        QTime start = busySlot.first;
        QTime end = busySlot.first.addSecs(busySlot.second);
        if (busyPeriods.isEmpty()) {
            busyPeriods.append({start, end});
        } else {
            std::pair<QTime, QTime> &last = busyPeriods.last();
            if (start <= last.second) {
                last.second = std::max(last.second, end);
            } else {
                busyPeriods.append({start, end});
            }
        }
    }
    QList<std::pair<QTime, QTime>> freeIntervals;
    if (busyPeriods.isEmpty()) {
        freeIntervals.append({dayStart, dayEnd});
    } else {
        if (busyPeriods.first().first > dayStart) {
            freeIntervals.append({dayStart, busyPeriods.first().first.addSecs(-1)});
        }
        for (int i = 0; i < busyPeriods.size() - 1; ++i) {
            QTime currentEnd = busyPeriods[i].second;
            QTime nextStart = busyPeriods[i + 1].first;
            if (currentEnd < nextStart) {
                freeIntervals.append({currentEnd, nextStart.addSecs(-1)});
            }
        }
        QTime lastEnd = busyPeriods.last().second;
        if (lastEnd <= dayEnd) {
            freeIntervals.append({lastEnd, dayEnd});
        }
    }
    for (const std::pair<QTime, QTime> &freeInterval : freeIntervals) {
        QTime intervalStart = freeInterval.first;
        QTime intervalEnd = freeInterval.second;
        int intervalDuration = intervalStart.secsTo(intervalEnd) + 1;
        if (intervalDuration >= requiredDurationSeconds) {
            QTime candidate = intervalStart;
            if (! forbiddenTimes.contains(candidate)) {
                int distance = qAbs(candidate.secsTo(targetTime));
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestSlot = candidate;
                    foundFullSlot = true;
                }
            }
            if (targetTime >= intervalStart && targetTime <= intervalEnd) {
                if (targetTime.secsTo(intervalEnd) + 1 >= requiredDurationSeconds) {
                    if (! forbiddenTimes.contains(targetTime)) {
                        bestSlot = targetTime;
                        foundFullSlot = true;
                        break;
                    }
                }
            }
            candidate = intervalEnd.addSecs(-(requiredDurationSeconds - 1));
            if (candidate >= intervalStart && ! forbiddenTimes.contains(candidate)) {
                int distance = std::abs(candidate.secsTo(targetTime));
                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestSlot = candidate;
                    foundFullSlot = true;
                }
            }
        }
    }
    if (foundFullSlot) {
        return bestSlot;
    }
    if (! forbiddenTimes.contains(targetTime)) {
        return targetTime;
    }
    const int increment = 15 * 60;
    int targetSecs = QTime(0, 0, 0).secsTo(targetTime);
    int maxSecs = QTime(0, 0, 0).secsTo(QTime(23, 59, 59));
    int maxSteps = (maxSecs / increment) + 1;
    for (int step = 1; step <= maxSteps; ++step) {
        int offset = step * increment;
        if (targetSecs + offset <= maxSecs) {
            QTime candidate = QTime(0, 0, 0).addSecs(targetSecs + offset);
            if (! forbiddenTimes.contains(candidate)) {
                return candidate;
            }
        }
        if (targetSecs - offset >= 0) {
            QTime candidate = QTime(0, 0, 0).addSecs(targetSecs - offset);
            if (! forbiddenTimes.contains(candidate)) {
                return candidate;
            }
        }
    }
    return QTime();
}


QString
CalendarWindow::buildOriginalLabel
(RideItem const * const item) const
{
    QDate originalPlan = QDate::fromString(item->getText("Original Date", ""), "yyyy/MM/dd");
    if (! originalPlan.isValid() || originalPlan == item->dateTime.date()) {
        return "";
    }

    QLocale locale;
    QString unitLabel;
    int days = originalPlan.daysTo(item->dateTime.date());
    QChar sign = days > 0 ? '+' : '-';
    ShowDaysAsUnit unit = showDaysAs(days);
    int c = 0;
    if (unit == ShowDaysAsUnit::Days) {
        c = std::abs(days);
        if (c == 1) {
            unitLabel = tr("day");
        } else {
            unitLabel = tr("days");
        }
    } else if (unit == ShowDaysAsUnit::Weeks) {
        c = daysToWeeks(days);
        if (c == 1) {
            unitLabel = tr("week");
        } else {
            unitLabel = tr("weeks");
        }
    } else if (unit == ShowDaysAsUnit::Months) {
        c = daysToMonths(days);
        if (c == 1) {
            unitLabel = tr("month");
        } else {
            unitLabel = tr("months");
        }
    }
    return QString("%1 (%2%3 %4)")
                  .arg(locale.toString(originalPlan, QLocale::NarrowFormat))
                  .arg(sign)
                  .arg(c)
                  .arg(unitLabel);
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
        calendar->activateDateRange(dr, allowKeepMonth, false);
    } else {
        DateRange dr(DateRange(season->getStart(), season->getEnd(), season->getName()));
        calendar->activateDateRange(dr, allowKeepMonth, season->canHavePhasesOrEvents());
    }
}


void
CalendarWindow::movePlannedActivity
(RideItem *rideItem, const QDate &destDay, const QTime &destTime)
{
    RideCache::OperationPreCheck check = context->athlete->rideCache->checkMoveActivity(rideItem, QDateTime(destDay, destTime));
    if (check.canProceed && proceedDialog(context, check)) {
        context->tab->setNoSwitch(true);
        RideCache::OperationResult result = context->athlete->rideCache->moveActivity(rideItem, QDateTime(destDay, destTime));
        if (result.success) {
            QString error;
            context->athlete->rideCache->saveActivities(check.affectedItems, error);
        } else {
            QMessageBox::warning(this, tr("Failed"), result.error);
        }
        context->tab->setNoSwitch(false);
    }
}


void
CalendarWindow::shiftPlannedActivities
(const QDate &destDay, int offset)
{
    RideCache::OperationPreCheck check = context->athlete->rideCache->checkShiftPlannedActivities(destDay, offset);
    if (check.canProceed && proceedDialog(context, check)) {
        context->tab->setNoSwitch(true);
        RideCache::OperationResult result = context->athlete->rideCache->shiftPlannedActivities(destDay, offset);
        if (result.success) {
            QString error;
            context->athlete->rideCache->saveActivities(check.affectedItems, error);
        } else {
            QMessageBox::warning(this, tr("Failed"), result.error);
        }
        context->tab->setNoSwitch(false);
    }
}


void
CalendarWindow::linkActivities
(const CalendarEntry &entry, bool autoLink)
{
    RideItem *rideItem = getRideItem(entry);
    if (rideItem == nullptr) {
        return;
    }
    RideItem *other = nullptr;
    if (autoLink) {
        if ((other = context->athlete->rideCache->findSuggestion(rideItem)) == nullptr) {
            QMessageBox::warning(this, tr("Failed"), tr("No matching activity found"));
        }
    } else {
        LinkEntry linkEntry;
        linkEntry.reference = entry.reference;
        linkEntry.planned = entry.type == ENTRY_TYPE_PLANNED_ACTIVITY;
        linkEntry.sport = rideItem->sport;
        linkEntry.iconFile = entry.iconFile;
        linkEntry.iconColor = entry.color;
        linkEntry.primary = entry.primary;
        linkEntry.secondary = entry.secondary;
        linkEntry.secondaryMetric = entry.secondaryMetric;
        linkEntry.date = rideItem->dateTime.date();
        linkEntry.time = entry.start;
        linkEntry.matchScore = 1;

        QDate minDate = linkEntry.date.addDays(-14);
        QDate maxDate = linkEntry.date.addDays(14);

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
        double rideMetricValue = rideItem->getForSymbol(getSecondaryMetric(), GlobalContext::context()->useMetricUnits);
        QList<LinkEntry> candidates;
        for (RideItem *candidateItem : context->athlete->rideCache->rides()) {
            if (   candidateItem->dateTime.date() >= minDate
                && candidateItem->dateTime.date() <= maxDate
                && candidateItem->planned != linkEntry.planned
                && candidateItem->sport == rideItem->sport
                && candidateItem->getLinkedFileName().isEmpty()) {
                LinkEntry candidate;
                candidate.reference = candidateItem->fileName;
                candidate.planned = candidateItem->planned;
                candidate.primary = getPrimary(candidateItem);
                candidate.secondary = candidateItem->getStringForSymbol(getSecondaryMetric(), GlobalContext::context()->useMetricUnits);
                if (! rideMetricUnit.isEmpty()) {
                    candidate.secondary += " " + rideMetricUnit;
                }
                candidate.secondaryMetric = rideMetricName;
                candidate.date = candidateItem->dateTime.date();
                candidate.time = candidateItem->dateTime.time();

                double candidateMetricValue = candidateItem->getForSymbol(getSecondaryMetric(), GlobalContext::context()->useMetricUnits);
                double dayScore = std::min(std::abs(linkEntry.date.daysTo(candidate.date)) / 7.0, 1.0);
                double metricScore = 1.0;
                if (rideMetricValue > 0 && candidateMetricValue > 0) {
                    double maxMetric = std::max(rideMetricValue, candidateMetricValue);
                    double metricDiff = std::abs(rideMetricValue - candidateMetricValue) / maxMetric;
                    metricScore = std::min(metricDiff, 1.0);
                }

                candidate.matchScore = (0.2 * dayScore) + (0.8 * metricScore);
                candidates << candidate;
            }
        }

        LinkDialog linkDialog(linkEntry, candidates, this);
        if (linkDialog.exec() == QDialog::Accepted) {
            other = context->athlete->rideCache->getRide(linkDialog.getSelectedReference(), ! linkEntry.planned);
        }
    }

    if (rideItem != nullptr && other != nullptr) {
        RideCache::OperationPreCheck check = context->athlete->rideCache->checkLinkActivities(rideItem, other);
        if (check.canProceed && proceedDialog(context, check)) {
            context->tab->setNoSwitch(true);
            RideCache::OperationResult result = context->athlete->rideCache->linkActivities(rideItem, other);
            if (result.success) {
                QString error;
                context->athlete->rideCache->saveActivities(check.affectedItems, error);
            } else {
                QMessageBox::warning(this, tr("Failed"), result.error);
            }
            context->tab->setNoSwitch(false);
        }
    }
}


bool
CalendarWindow::unlinkActivities
(const CalendarEntry &entry)
{
    bool ret = false;
    RideItem *item = context->athlete->rideCache->getRide(entry.reference, entry.type == ENTRY_TYPE_PLANNED_ACTIVITY);
    if (item->getLinkedFileName().isEmpty()) {
        return true;
    }
    RideCache::OperationPreCheck check = context->athlete->rideCache->checkUnlinkActivity(item);
    if (check.canProceed && proceedDialog(context, check)) {
        context->tab->setNoSwitch(true);
        RideCache::OperationResult result = context->athlete->rideCache->unlinkActivity(item);
        if (result.success) {
            QString error;
            context->athlete->rideCache->saveActivities(check.affectedItems, error);
            ret = true;
        } else {
            QMessageBox::warning(this, tr("Failed"), result.error);
        }
        context->tab->setNoSwitch(false);
    }
    return ret;
}


void
CalendarWindow::copyPlannedActivity
(CalendarEntry activity)
{
    RideItem *rideItem = getRideItem(activity);
    if (rideItem != nullptr) {
        QClipboard *clipboard = QGuiApplication::clipboard();
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        stream << activity.primary << activity.reference;
        QMimeData *mimeData = new QMimeData();
        mimeData->setData(PLANNED_MIME_TYPE, data);
        clipboard->setMimeData(mimeData);
    }
}


void
CalendarWindow::pastePlannedActivity
(QDate day, QTime time)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    if (mimeData && mimeData->hasFormat(PLANNED_MIME_TYPE)) {
        QByteArray data = mimeData->data(PLANNED_MIME_TYPE);
        QDataStream stream(data);
        QString primary;
        QString reference;
        stream >> primary >> reference;

        RideItem *sourceItem = nullptr;
        for (RideItem *rideItem : context->athlete->rideCache->rides()) {
            if (rideItem != nullptr && rideItem->planned && rideItem->fileName == reference) {
                sourceItem = rideItem;
                break;
            }
        }
        time = findFreeSlot(sourceItem, day, time);
        RideCache::OperationPreCheck check = context->athlete->rideCache->checkCopyPlannedActivity(sourceItem, day, time);
        if (check.canProceed) {
            if (proceedDialog(context, check)) {
                context->tab->setNoSwitch(true);
                RideCache::OperationResult result = context->athlete->rideCache->copyPlannedActivity(sourceItem, day, time);
                if (result.success) {
                    QString error;
                    context->athlete->rideCache->saveActivities(check.affectedItems, error);
                    // Context::rideDeleted is not always emitted, therefore forcing the update
                    updateActivities();
                } else {
                    QMessageBox::warning(this, tr("Paste failed: %1").arg(primary), result.error);
                }
                context->tab->setNoSwitch(false);
            }
        } else {
            QMessageBox::warning(this, tr("Paste failed: %1").arg(primary), check.blockingReason);
        }
    }
}


void
CalendarWindow::addEvent
(const QDate &date)
{
    Season const *currentSeason = context->currentSeason();
    if (currentSeason == nullptr) {
        return;
    }
    if (! currentSeason->canHavePhasesOrEvents()) {
        return;
    }
    Season *season = nullptr;
    for (Season &s : context->athlete->seasons->seasons) {
        if (&s == currentSeason) {
            season = &s;
            break;
        }
    }
    if (season == nullptr) {
        return;
    }

    SeasonEvent myevent("", date);
    EditSeasonEventDialog dialog(context, &myevent, *season);
    if (dialog.exec()) {
        season->events.append(myevent);
        context->athlete->seasons->writeSeasons();
    }
}


void
CalendarWindow::editEvent
(const CalendarEntry &entry)
{
    if (entry.type != ENTRY_TYPE_EVENT) {
        return;
    }
    Season *season = nullptr;
    SeasonEvent *seasonEvent = nullptr;
    for (Season &s : context->athlete->seasons->seasons) {
        for (SeasonEvent &event : s.events) {
            QString evId = event.id;
            if (evId.isEmpty()) {
                evId = QString("0x%1").arg(reinterpret_cast<quintptr>(&event), 0, 16);
            }
            if (entry.reference == evId) {
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
    }
}


void
CalendarWindow::delEvent
(const CalendarEntry &entry)
{
    if (entry.type != ENTRY_TYPE_EVENT) {
        return;
    }
    bool done = false;
    for (Season &s : context->athlete->seasons->seasons) {
        int idx = 0;
        for (SeasonEvent &event : s.events) {
            QString evId = event.id;
            if (evId.isEmpty()) {
                evId = QString("0x%1").arg(reinterpret_cast<quintptr>(&event), 0, 16);
            }
            if (entry.reference == evId) {
                s.events.removeAt(idx);
                context->athlete->seasons->writeSeasons();
                done = true;
                break;
            }
            ++idx;
        }
        if (done) {
            break;
        }
    }
}


void
CalendarWindow::addPhase
(const QDate &date)
{
    Season const *currentSeason = context->currentSeason();
    if (currentSeason == nullptr) {
        return;
    }
    if (! currentSeason->canHavePhasesOrEvents()) {
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
    QDate seasonStart = phaseSeason->getStart();
    QDate seasonEnd = phaseSeason->getEnd();
    if (seasonStart == seasonEnd) {
        return;
    }
    qint64 daysBefore = std::min(static_cast<qint64>(6), seasonStart.daysTo(date));
    qint64 daysAfter = std::min(static_cast<qint64>(6), date.daysTo(seasonEnd));
    QDate start(date);
    QDate end(date);
    if (daysAfter > 0) {
        end = end.addDays(daysAfter);
    } else {
        start = start.addDays(-daysBefore);
    }
    Phase myphase("", start, end);
    EditPhaseDialog dialog(context, &myphase, *phaseSeason);
    if (dialog.exec()) {
        phaseSeason->phases.append(myphase);
        context->athlete->seasons->writeSeasons();
    }
}


void
CalendarWindow::editPhase
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
            }
            break;
        }
    }
}


void
CalendarWindow::delPhase
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
    int idx = 0;
    for (Phase &editPhase : phaseSeason->phases) {
        if (entry.reference == editPhase.id().toString()) {
            phaseSeason->phases.removeAt(idx);
            context->athlete->seasons->writeSeasons();
            break;
        }
        ++idx;
    }
}
