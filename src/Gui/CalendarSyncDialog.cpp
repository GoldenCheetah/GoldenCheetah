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

#include "CalendarSyncDialog.h"

#include <QDialogButtonBox>
#include <QHeaderView>

#include "Colors.h"
#include "StyledItemDelegates.h"
#include "RideMetadata.h"


CalendarSyncDialog::CalendarSyncDialog
(Context *context, const CalendarSync::SyncObjects &syncObjects, QString cloudServiceName, QWidget *parent)
: QDialog(parent), context(context), syncObjects(syncObjects), cloudServiceName(cloudServiceName)
{
    setWindowTitle(tr("Synchronize to '%1'").arg(cloudServiceName));
    setMinimumSize(800 * dpiXFactor, 600 * dpiYFactor);
    resize(800 * dpiXFactor, 600 * dpiYFactor);

    QLabel *titleLabel = new QLabel("<h3>" + tr("Synchronize to %1").arg(cloudServiceName) + "</h3>");
    titleLabel->setAlignment(Qt::AlignCenter);
    QLabel *whatLabel = new QLabel();
    whatLabel->setAlignment(Qt::AlignCenter);
    if (syncObjects.root == CalDAV::EntryType::Season) {
        whatLabel->setText("<h5>" + tr("Season %1").arg(syncObjects.seasons[0]->getName()) + "</h5>");
    } else if (syncObjects.root == CalDAV::EntryType::Phase) {
        whatLabel->setText("<h5>" + tr("Phase %1").arg(syncObjects.phases[0]->getName()) + "</h5>");
    } else if (syncObjects.root == CalDAV::EntryType::Event) {
        whatLabel->setText("<h5>" + tr("Event %1").arg(syncObjects.events[0]->name) + "</h5>");
    } else if (syncObjects.root == CalDAV::EntryType::PlannedActivity) {
        QLocale locale;
        QList<RideItem*> activities = syncObjects.plannedActivities;
        whatLabel->setText("<h5>" + tr("Planned Activity %1").arg(locale.toString(activities[0]->dateTime), QLocale::ShortFormat) + "</h5>");
    } else if (syncObjects.root == CalDAV::EntryType::ActualActivity) {
        QLocale locale;
        QList<RideItem*> activities = syncObjects.actualActivities;
        whatLabel->setText("<h5>" + tr("Actual Activity %1").arg(locale.toString(activities[0]->dateTime), QLocale::ShortFormat) + "</h5>");
    }

    titleCombo = new QComboBox();
    descriptionCombo = new QComboBox();
    QList<FieldDefinition> fieldsDefs = GlobalContext::context()->rideMetadata->getFields();
    for (const FieldDefinition &fieldDef : fieldsDefs) {
        if (fieldDef.isTextField()) {
            titleCombo->addItem(fieldDef.name);
            descriptionCombo->addItem(fieldDef.name);
        }
    }
    QString titleField = appsettings->cvalue(context->athlete->cyclist, GC_CALDAV_TITLE, "Workout Code").toString();
    QString descriptionField = appsettings->cvalue(context->athlete->cyclist, GC_CALDAV_DESCRIPTION, "Calendar Text").toString();
    titleCombo->setCurrentText(titleField);
    descriptionCombo->setCurrentText(descriptionField);
    QLabel *infoLabel = new QLabel(tr("Syncing only affects your CalDAV calendar - no data in GoldenCheetah is changed or deleted."));
    infoLabel->setWordWrap(true);
    infoLabel->setFrameStyle(QFrame::StyledPanel);
    infoLabel->setFrameShadow(QFrame::Sunken);

    QFormLayout *configFormLayout = newQFormLayout();
    configFormLayout->addRow(new QLabel("<b>" + tr("Fields for activities") + "</b"));
    configFormLayout->addRow(tr("Title"), titleCombo);
    configFormLayout->addRow(tr("Description"), descriptionCombo);
    configFormLayout->addItem(new QSpacerItem(0, 10 * dpiYFactor, QSizePolicy::Minimum, QSizePolicy::Minimum));
    configFormLayout->addRow(new QLabel("<b>" + tr("Sync strategy") + "</b"));
    configFormLayout->addRow(tr("Seasons"), strategyWidget(&syncSelections.seasons, CalDAV::EntryType::Season, syncObjects.seasons.count()));
    configFormLayout->addRow(tr("Phases"), strategyWidget(&syncSelections.phases, CalDAV::EntryType::Phase, syncObjects.phases.count()));
    configFormLayout->addRow(tr("Events"), strategyWidget(&syncSelections.events, CalDAV::EntryType::Event, syncObjects.events.count()));
    configFormLayout->addRow(tr("Planned Activities"), strategyWidget(&syncSelections.plannedActivities, CalDAV::EntryType::PlannedActivity, syncObjects.plannedActivities.count()));
    configFormLayout->addRow(tr("Actual Activities"), strategyWidget(&syncSelections.actualActivities, CalDAV::EntryType::ActualActivity, syncObjects.actualActivities.count()));

    progressLabel = new QLabel();
    progressLabel->setAlignment(Qt::AlignCenter);
    cleanupBar = new QProgressBar();
    cleanupBar->setMinimum(0);
    cleanupBar->setMaximum(1);
    cleanupBar->setValue(0);
    syncBar = new QProgressBar();
    syncBar->setMinimum(0);
    syncBar->setMaximum(1);
    syncBar->setValue(0);

    resultLabel = new QLabel();
    overview = new QTreeWidget();
    overview->setColumnCount(4);
    overview->setHeaderLabels({ tr("Category"), tr("Successes"), tr("Failures"), tr("Skipped") });
    overview->setRootIsDecorated(true);
    overview->setAlternatingRowColors(true);
    overview->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    overview->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    overview->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    overview->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    QWidget *configPage = new QWidget();
    QVBoxLayout *configLayout = new QVBoxLayout(configPage);
    configLayout->addWidget(centerLayoutInWidget(configFormLayout));
    configLayout->addStretch(1);
    configLayout->addWidget(infoLabel);

    QWidget *syncPage = new QWidget();
    QVBoxLayout *syncLayout = new QVBoxLayout(syncPage);
    syncLayout->addStretch(1);
    syncLayout->addWidget(progressLabel);
    syncLayout->addWidget(cleanupBar);
    syncLayout->addWidget(syncBar);
    syncLayout->addStretch(2);

    QWidget *resultPage = new QWidget();
    QVBoxLayout *resultLayout = new QVBoxLayout(resultPage);
    resultLayout->addWidget(resultLabel);
    resultLayout->addSpacing(10 * dpiYFactor);
    resultLayout->addWidget(overview, 1);

    stackedWidget = new QStackedWidget();
    stackedWidget->addWidget(configPage);
    stackedWidget->addWidget(syncPage);
    stackedWidget->addWidget(resultPage);

    buttons = new QDialogButtonBox();

    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(titleLabel);
    dialogLayout->addWidget(whatLabel);
    dialogLayout->addSpacing(20 * dpiYFactor);
    dialogLayout->addWidget(stackedWidget, 1);
    dialogLayout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::clicked, this, &CalendarSyncDialog::buttonClicked);
    connect(context->athlete->calendarSync, &CalendarSync::cleanupState, this, &CalendarSyncDialog::cleanupStateChange);
    connect(context->athlete->calendarSync, &CalendarSync::cleanupProgress, this, &CalendarSyncDialog::cleanupProgress);
    connect(context->athlete->calendarSync, &CalendarSync::cleanupFinished, this, &CalendarSyncDialog::cleanupFinished);
    connect(context->athlete->calendarSync, &CalendarSync::syncObjectType, this, &CalendarSyncDialog::syncTypeChange);
    connect(context->athlete->calendarSync, &CalendarSync::syncProgress, this, &CalendarSyncDialog::syncProgress);
    connect(context->athlete->calendarSync, &CalendarSync::syncFinished, this, &CalendarSyncDialog::syncFinished);
    connect(titleCombo, &QComboBox::currentTextChanged, this, [this](const QString &entry) {
        appsettings->setCValue(this->context->athlete->cyclist, GC_CALDAV_TITLE, entry);
    });
    connect(descriptionCombo, &QComboBox::currentTextChanged, this, [this](const QString &entry) {
        appsettings->setCValue(this->context->athlete->cyclist, GC_CALDAV_DESCRIPTION, entry);
    });

    setState(State::Configuration);
}


void
CalendarSyncDialog::setState
(State state)
{
    buttons->setEnabled(true);
    this->state = state;

    switch (state) {
    case State::Configuration:
        stackedWidget->setCurrentIndex(0);
        buttons->clear();
        buttons->setStandardButtons(QDialogButtonBox::Cancel);
        buttons->addButton(tr("Sync"), QDialogButtonBox::AcceptRole);
        break;
    case State::Running:
        stackedWidget->setCurrentIndex(1);
        buttons->clear();
        buttons->setStandardButtons(QDialogButtonBox::Cancel);
        break;
    case State::Finished:
        resultLabel->setText("<h4>" + (canceled ? tr("Synchronisation canceled") : tr("Synchronisation completed")) + "</h4>");
        stackedWidget->setCurrentIndex(2);
        buttons->clear();
        buttons->setStandardButtons(QDialogButtonBox::Close);
        break;
    }
}


QWidget*
CalendarSyncDialog::strategyWidget
(CalendarSync::SyncSelections::SyncMode *mode, CalDAV::EntryType type, int available)
{
    QWidget *ret;
    bool isRelevant = syncObjects.isRelevantForType(type);
    if (isRelevant) {
        QComboBox *combo = new QComboBox();
        combo->addItem(tr("Sync"));
        combo->addItem(tr("Sync & cleanup"));
        combo->addItem(tr("Skip"));
        combo->addItem(tr("Remove all synced entries"));
        QString configKey;
        if (type == CalDAV::EntryType::Season) {
            configKey = GC_CALDAV_STRATEGY_SEASON;
        } else if (type == CalDAV::EntryType::Phase) {
            configKey = GC_CALDAV_STRATEGY_PHASE;
        } else if (type == CalDAV::EntryType::Event) {
            configKey = GC_CALDAV_STRATEGY_EVENT;
        } else if (type == CalDAV::EntryType::PlannedActivity) {
            configKey = GC_CALDAV_STRATEGY_PLANNED_ACT;
        } else if (type == CalDAV::EntryType::ActualActivity) {
            configKey = GC_CALDAV_STRATEGY_ACTUAL_ACT;
        }
        int strategyIdx = appsettings->cvalue(context->athlete->cyclist, configKey, 1).toInt();
        *mode = static_cast<CalendarSync::SyncSelections::SyncMode>(strategyIdx);
        combo->setCurrentIndex(strategyIdx);
        connect(combo, &QComboBox::currentIndexChanged, this, [this, type, configKey](int idx) {
            if (configKey.isEmpty()) {
                return;
            }
            appsettings->setCValue(this->context->athlete->cyclist, configKey, idx);
            if (type == CalDAV::EntryType::Season) {
                syncSelections.seasons = static_cast<CalendarSync::SyncSelections::SyncMode>(idx);
            } else if (type == CalDAV::EntryType::Phase) {
                syncSelections.phases = static_cast<CalendarSync::SyncSelections::SyncMode>(idx);
            } else if (type == CalDAV::EntryType::Event) {
                syncSelections.events = static_cast<CalendarSync::SyncSelections::SyncMode>(idx);
            } else if (type == CalDAV::EntryType::PlannedActivity) {
                syncSelections.plannedActivities = static_cast<CalendarSync::SyncSelections::SyncMode>(idx);
            } else if (type == CalDAV::EntryType::ActualActivity) {
                syncSelections.actualActivities = static_cast<CalendarSync::SyncSelections::SyncMode>(idx);
            }
        });
        ret = wrapStrategyCombo(combo, available);
        ret->setEnabled(true);
    } else {
        ret = new QLabel(tr("---"));
        ret->setEnabled(false);
    }
    return ret;
}


QWidget*
CalendarSyncDialog::wrapStrategyCombo
(QComboBox *combo, int count) const
{
    QWidget *fieldWrapper = new QWidget();
    QHBoxLayout *row = new QHBoxLayout(fieldWrapper);
    row->setContentsMargins(0, 0, 0, 0);
    row->addWidget(combo);
    row->addWidget(new QLabel(tr(" / %1 available").arg(count)));
    return fieldWrapper;
}


void
CalendarSyncDialog::addResult
(const QString &category, const CalendarSync::SyncResult &result, bool bold)
{
    QTreeWidgetItem *categoryItem = new QTreeWidgetItem(overview);
    QFont font = categoryItem->font(0);
    font.setBold(bold);
    categoryItem->setData(0, Qt::DisplayRole, category);
    categoryItem->setData(0, Qt::FontRole, font);
    categoryItem->setData(1, Qt::DisplayRole, QString::number(result.success));
    categoryItem->setData(1, Qt::FontRole, font);
    categoryItem->setData(2, Qt::DisplayRole, QString::number(result.fail));
    categoryItem->setData(2, Qt::FontRole, font);
    categoryItem->setData(3, Qt::DisplayRole, QString::number(result.skip));
    categoryItem->setData(3, Qt::FontRole, font);
    for (const QString &msg : result.msg) {
        QTreeWidgetItem *msgItem = new QTreeWidgetItem(categoryItem);
        msgItem->setData(0, Qt::DisplayRole, msg);
        msgItem->setFirstColumnSpanned(true);
    }
    categoryItem->setExpanded(true);
}


void
CalendarSyncDialog::buttonClicked
(QAbstractButton *button)
{
    switch (state) {
    case State::Configuration:
        if (buttons->buttonRole(button) == QDialogButtonBox::AcceptRole) {
            // Defer to not call setState (deleting buttons) from its own clicked handelr
            QTimer::singleShot(0, this, [this]() { startSync(); });
        } else {
            reject();
        }
        break;
    case State::Running:
        if (buttons->buttonRole(button) == QDialogButtonBox::RejectRole) {
            context->athlete->calendarSync->cancel();
            buttons->setEnabled(false);
        }
        break;
    case State::Finished:
    default:
        accept();
        break;
    }
}


void
CalendarSyncDialog::startSync
()
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    setState(State::Running);
    progressLabel->setText(tr("Cleanup"));
    context->athlete->calendarSync->resetCancel();
    context->athlete->calendarSync->cleanup(syncObjects, syncSelections, cloudServiceName);
    context->athlete->calendarSync->syncObjects(syncObjects, syncSelections, cloudServiceName);
    context->athlete->calendarSync->resetCancel();
    QApplication::restoreOverrideCursor();
}


void
CalendarSyncDialog::cleanupStateChange
(bool prepare)
{
    if (prepare) {
        progressLabel->setText(tr("Reading remote calendar"));
    } else {
        progressLabel->setText(tr("Cleanup"));
    }
}


void
CalendarSyncDialog::cleanupProgress
(int current, int count)
{
    cleanupBar->setMaximum(count);
    cleanupBar->setValue(current);
}


void
CalendarSyncDialog::syncTypeChange
(CalDAV::EntryType type)
{
    if (type == CalDAV::EntryType::Season) {
        progressLabel->setText(tr("Syncing Seasons"));
    } else if (type == CalDAV::EntryType::Phase) {
        progressLabel->setText(tr("Syncing Phases"));
    } else if (type == CalDAV::EntryType::Event) {
        progressLabel->setText(tr("Syncing Events"));
    } else if (type == CalDAV::EntryType::PlannedActivity) {
        progressLabel->setText(tr("Syncing planned Activities"));
    } else if (type == CalDAV::EntryType::ActualActivity) {
        progressLabel->setText(tr("Syncing actual Activities"));
    } else {
        progressLabel->setText("");
    }
}


void
CalendarSyncDialog::syncProgress
(int current, int count)
{
    syncBar->setMaximum(count);
    syncBar->setValue(current);
}


void
CalendarSyncDialog::cleanupFinished
(CalendarSync::CleanupResults results, bool canceled)
{
    cleanupBar->setMaximum(1);
    cleanupBar->setValue(1);

    if (results.tech.fail == 0) {
        addResult(tr("Cleanup"), results.cleanup, true);
    } else {
        addResult(tr("Sync Technical"), results.tech);
    }
    this->canceled = canceled;
}


void
CalendarSyncDialog::syncFinished
(CalendarSync::SyncResults results, bool canceled)
{
    this->canceled = canceled;
    if (results.tech.fail == 0) {
        addResult(tr("Sync Overall"), results.overall(), true);
        addResult(tr("Seasons"), results.seasons);
        addResult(tr("Phases"), results.phases);
        addResult(tr("Events"), results.events);
        addResult(tr("Planned Activities"), results.plannedActivities);
        addResult(tr("Actual Activities"), results.actualActivities);
    } else {
        addResult(tr("Sync Technical"), results.tech);
    }
    setState(State::Finished);
}
