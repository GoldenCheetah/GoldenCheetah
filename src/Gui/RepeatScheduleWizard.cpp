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

#include "RepeatScheduleWizard.h"

#include <QtGui>
#include <QScrollArea>

#include "Context.h"
#include "Athlete.h"
#include "AthleteTab.h"
#include "Seasons.h"
#include "Colors.h"

#define ICON_COLOR QColor("#F79130")
#ifdef Q_OS_MAC
#define ICON_SIZE 250
#define ICON_MARGIN 0
#define ICON_TYPE QWizard::BackgroundPixmap
#else
#define ICON_SIZE 125
#define ICON_MARGIN 5
#define ICON_TYPE QWizard::LogoPixmap
#endif

static QString rideItemName(RideItem const * const rideItem);
static QString rideItemSport(RideItem const * const rideItem);


////////////////////////////////////////////////////////////////////////////////
// RepeatScheduleWizard

RepeatScheduleWizard::RepeatScheduleWizard
(Context *context, const QDate &when, QWidget *parent)
: QWizard(parent), context(context), when(when)
{
    setWindowTitle(tr("Repeat Schedule"));
    setMinimumSize(800 * dpiXFactor, 650 * dpiYFactor);
    setModal(true);

#ifdef Q_OS_MAC
    setWizardStyle(QWizard::MacStyle);
#else
    setWizardStyle(QWizard::ModernStyle);
#endif
    setPixmap(ICON_TYPE, svgAsColoredPixmap(":images/breeze/media-playlist-repeat.svg", QSize(ICON_SIZE * dpiXFactor, ICON_SIZE * dpiYFactor), ICON_MARGIN * dpiXFactor, ICON_COLOR));

    setPage(PageSetup, new RepeatSchedulePageSetup(context, when));
    setPage(PageActivities, new RepeatSchedulePageActivities(context));
    setPage(PageSummary, new RepeatSchedulePageSummary(context, when));
    setStartId(PageSetup);
}


void
RepeatScheduleWizard::done
(int result)
{
    int finalResult = result;
    if (result == QDialog::Accepted) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        RepeatSchedulePageSummary *summaryPage = qobject_cast<RepeatSchedulePageSummary*>(page(PageSummary));
        QList<RideItem*> deletionList = summaryPage->getDeletionList();
        QList<std::pair<RideItem*, QDate>> scheduleList = summaryPage->getScheduleList();
        context->tab->setNoSwitch(true);
        for (RideItem *rideItem : deletionList) {
            context->athlete->rideCache->removeRide(rideItem->fileName);
        }
        for (std::pair<RideItem*, QDate> entry : scheduleList) {
            RideItem *rideItem = entry.first;
            QDate targetDate = entry.second;
            RideFile *rideFile = rideItem->ride();
            QDateTime rideDateTime(targetDate, rideFile->startTime().time());
            rideFile->setStartTime(rideDateTime);
            QString basename = rideDateTime.toString("yyyy_MM_dd_HH_mm_ss");
            QString filename = context->athlete->home->planned().canonicalPath() + "/" + basename + ".json";
            QFile out(filename);
            if (RideFileFactory::instance().writeRideFile(context, rideFile, out, "json")) {
                context->athlete->addRide(basename + ".json", true, true, false, rideItem->planned);
            }
        }
        context->tab->setNoSwitch(false);
        QApplication::restoreOverrideCursor();
    }

    QWizard::done(finalResult);
}


////////////////////////////////////////////////////////////////////////////////
// RepeatSchedulePageSetup

RepeatSchedulePageSetup::RepeatSchedulePageSetup
(Context *context, const QDate &when, QWidget *parent)
: QWizardPage(parent), context(context)
{
    setTitle(tr("Repeat Schedule Setup"));
    setSubTitle(tr("Specify the time range and strategy for repeating the schedule. All planned activities within this range will be copied. You can optionally select a season or phase to prefill the start and end dates."));

    QTreeWidget *seasonTree = new QTreeWidget();
    seasonTree->setColumnCount(1);
    basicTreeWidgetStyle(seasonTree, false);
    seasonTree->setHeaderHidden(true);
    seasonTree->resetIndentation();
    QTreeWidgetItem *currentSeason = nullptr;
    for (const Season &season : context->athlete->seasons->seasons) {
        QTreeWidgetItem *seasonItem = new QTreeWidgetItem();
        seasonItem->setData(0, Qt::DisplayRole, season.getName());
        seasonItem->setData(0, Qt::UserRole, season.getStart());
        seasonItem->setData(0, Qt::UserRole + 1, season.getEnd());
        if (context->currentSeason() != nullptr && context->currentSeason()->id() == season.id()) {
            currentSeason = seasonItem;
        }
        for (const Phase &phase : season.phases) {
            QTreeWidgetItem *phaseItem = new QTreeWidgetItem();
            phaseItem->setData(0, Qt::DisplayRole, phase.getName());
            phaseItem->setData(0, Qt::UserRole, phase.getStart());
            phaseItem->setData(0, Qt::UserRole + 1, phase.getEnd());
            seasonItem->addChild(phaseItem);
            if (context->currentSeason() != nullptr && context->currentSeason()->id() == phase.id()) {
                currentSeason = phaseItem;
            }
        }
        seasonTree->addTopLevelItem(seasonItem);
    }

    QSpinBox *restDayBox = new QSpinBox();
    restDayBox->setSuffix(" " + tr("active days"));
    restDayBox->setValue(3);
    restDayBox->setRange(1, 99);

    QComboBox *conflictBox = new QComboBox();
    conflictBox->addItem("Delete all pre-existing activities");
    conflictBox->addItem("Skip days with pre-existing activities");
    conflictBox->addItem("Fail for all");

    QDateEdit *startDate = new QDateEdit();
    startDate->setMaximumDate(when.addDays(-1));

    QDateEdit *endDate = new QDateEdit();
    endDate->setMaximumDate(when.addDays(-1));

    QCheckBox *sameDayCheck = new QCheckBox(tr("Copy same-day activities to consecutive days"));

    registerField("startDate", startDate);
    registerField("endDate", endDate);
    registerField("restDayHandling", restDayBox);
    registerField("sameDay", sameDayCheck);
    registerField("conflictHandling", conflictBox);

    QFormLayout *form = newQFormLayout();
    form->addRow(seasonTree);
    form->addRow(tr("Start Date"), startDate);
    form->addRow(tr("End Date"), endDate);
    form->addRow(tr("Insert rest day after"), restDayBox);
    form->addRow("", sameDayCheck);
    form->addRow(tr("Conflict Handling"), conflictBox);

    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->addWidget(centerLayoutInWidget(form, false));
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *all = new QVBoxLayout();
    all->addWidget(scrollArea);
    setLayout(all);

    connect(seasonTree, &QTreeWidget::currentItemChanged, [=](QTreeWidgetItem *current) {
        if (current != nullptr) {
            QDate seasonStart(current->data(0, Qt::UserRole).toDate());
            QDate seasonEnd(current->data(0, Qt::UserRole + 1).toDate());
            if (seasonEnd > when) {
                seasonEnd = when;
            }
            if (seasonStart > when) {
                seasonEnd = when;
            }
            startDate->setDate(seasonStart);
            endDate->setDate(seasonEnd);
        }
    });
    connect(startDate, &QDateEdit::dateChanged, [=](QDate date) {
        endDate->setMinimumDate(date);
    });
    connect(endDate, &QDateEdit::dateChanged, [=](QDate date) {
        startDate->setMaximumDate(date);
    });

    if (currentSeason != nullptr) {
        seasonTree->setCurrentItem(currentSeason);
    } else {
        startDate->setDate(QDate::currentDate().addDays(-7));
        endDate->setDate(QDate::currentDate().addDays(-1));
    }
}


int
RepeatSchedulePageSetup::nextId
() const
{
    return RepeatScheduleWizard::PageActivities;
}


////////////////////////////////////////////////////////////////////////////////
// RepeatSchedulePageActivities

RepeatSchedulePageActivities::RepeatSchedulePageActivities
(Context *context, QWidget *parent)
: QWizardPage(parent), context(context)
{
    setTitle(tr("Repeat Schedule Activities"));
    setSubTitle(tr("Review and choose the activities you wish to add to your schedule."));

    setFinalPage(false);

    activityTree = new QTreeWidget();
    activityTree->setColumnCount(4);
    basicTreeWidgetStyle(activityTree, false);
    activityTree->setHeaderHidden(true);

    QWidget *formWidget = new QWidget();
    QVBoxLayout *form = new QVBoxLayout(formWidget);
    form->addWidget(new QLabel("<h4>" + tr("Activities for your new schedule") + "</h4"));
    form->addWidget(activityTree);

    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->addWidget(formWidget);
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *all = new QVBoxLayout();
    all->addWidget(scrollArea);
    setLayout(all);
}


int
RepeatSchedulePageActivities::nextId
() const
{
    return RepeatScheduleWizard::PageSummary;
}


void
RepeatSchedulePageActivities::initializePage
()
{
    activityTree->clear();
    QDate startDate = field("startDate").toDate();
    QDate endDate = field("endDate").toDate();
    QLocale locale;
    numSelected = 0;
    for (RideItem *rideItem : context->athlete->rideCache->rides()) {
        if (   rideItem == nullptr
            || ! rideItem->planned
            || rideItem->dateTime.date() < startDate
            || rideItem->dateTime.date() > endDate) {
            continue;
        }
        if (context->isfiltered && ! context->filters.contains(rideItem->fileName)) {
            continue;
        }
        ++numSelected;
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setData(1, Qt::DisplayRole, locale.toString(rideItem->dateTime.date(), QLocale::ShortFormat));
        item->setData(1, Qt::UserRole, QVariant::fromValue(rideItem));
        item->setData(1, Qt::UserRole + 1, true);
        item->setData(2, Qt::DisplayRole, rideItemSport(rideItem));
        item->setData(3, Qt::DisplayRole, rideItemName(rideItem));
        activityTree->addTopLevelItem(item);

        QCheckBox *selectionBox = new QCheckBox();
        selectionBox->setChecked(true);
        QWidget *selectionWidget = new QWidget(activityTree);
        QVBoxLayout *layout = new QVBoxLayout(selectionWidget);
        layout->addWidget(selectionBox, 0, Qt::AlignCenter);
        activityTree->setItemWidget(item, 0, selectionWidget);
        connect(selectionBox, &QCheckBox::toggled, [=](bool checked) {
            item->setData(1, Qt::UserRole + 1, checked);
            numSelected += checked ? 1 : -1;
            emit completeChanged();
        });
    }
}


bool
RepeatSchedulePageActivities::isComplete
() const
{
    return numSelected > 0;
}


QList<RideItem*>
RepeatSchedulePageActivities::getSelectedRideItems
() const
{
    QList<RideItem*> ret;
    for (int i = 0; i < activityTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = activityTree->topLevelItem(i);
        if (item->data(1, Qt::UserRole + 1).toBool()) {
            ret << item->data(1, Qt::UserRole).value<RideItem*>();
        }
    }
    return ret;
}


////////////////////////////////////////////////////////////////////////////////
// RepeatSchedulePageSummary

RepeatSchedulePageSummary::RepeatSchedulePageSummary
(Context *context, const QDate &when, QWidget *parent)
: QWizardPage(parent), context(context), when(when)
{
    setTitle(tr("Repeat Schedule Summary"));
    setSubTitle(tr("Preview the schedule updates, including planned additions and deletions. No changes will be made until you continue."));

    setFinalPage(true);

    failedLabel = new QLabel("<center><h4>"
                             + tr("Unable to create a new schedule due to conflicts")
                             + "</h4>"
                             + tr("Adjust conflict handling on the first page to proceed.")
                             + "</center>");
    failedLabel->setWordWrap(true);

    scheduleLabel = new QLabel("<h4>" + tr("New Schedule Overview") + "</h4");
    scheduleTree = new QTreeWidget();
    scheduleTree->setColumnCount(5);
    basicTreeWidgetStyle(scheduleTree, false);
    scheduleTree->setHeaderHidden(true);

    deletionLabel = new QLabel("<h4>" + tr("Planned Activities Marked for Deletion") + "</h4>");
    deletionTree = new QTreeWidget();
    deletionTree->setColumnCount(3);
    basicTreeWidgetStyle(deletionTree, false);
    deletionTree->setHeaderHidden(true);

    QWidget *formWidget = new QWidget();
    QVBoxLayout *form = new QVBoxLayout(formWidget);
    form->addWidget(failedLabel);
    form->addWidget(scheduleLabel);
    form->addWidget(scheduleTree);
    form->addWidget(deletionLabel);
    form->addWidget(deletionTree);

    QWidget *scrollWidget = new QWidget();
    QVBoxLayout *scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->addWidget(formWidget);
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *all = new QVBoxLayout();
    all->addWidget(scrollArea);
    setLayout(all);
}


int
RepeatSchedulePageSummary::nextId
() const
{
    return RepeatScheduleWizard::Finalize;
}


void
RepeatSchedulePageSummary::initializePage
()
{
    failed = false;
    scheduleList.clear();
    deletionList.clear();

    scheduleTree->clear();
    deletionTree->clear();

    failedLabel->setVisible(false);
    scheduleLabel->setVisible(false);
    scheduleTree->setVisible(false);
    deletionLabel->setVisible(false);
    deletionTree->setVisible(false);

    int restDayAfter = field("restDayHandling").toInt();
    bool sameDay = field("sameDay").toBool();
    int conflictHandling = field("conflictHandling").toInt();

    QList<RideItem*> preexistingPlanned; // Currently planned activities with date > when
    QHash<QDate, int> preexistingCount; // Number of preexisting activities with date > when per date
    for (RideItem *rideItem : context->athlete->rideCache->rides()) {
        if (   rideItem == nullptr
            || ! rideItem->planned
            || rideItem->dateTime.date() < when) {
            continue;
        }
        if (context->isfiltered && ! context->filters.contains(rideItem->fileName)) {
            continue;
        }
        preexistingPlanned << rideItem;
        preexistingCount.insert(rideItem->dateTime.date(), preexistingCount.value(rideItem->dateTime.date(), 0) + 1);
    }

    RepeatSchedulePageActivities *activitiesPage = qobject_cast<RepeatSchedulePageActivities*>(wizard()->page(RepeatScheduleWizard::PageActivities));
    QList<RideItem*> selectedItems = activitiesPage->getSelectedRideItems(); // list of all selected activities that are to be copied to after when
    QHash<QDate, int> selectedCount; // Number of selected activities per date
    for (RideItem *rideItem : selectedItems) {
        selectedCount.insert(rideItem->dateTime.date(), selectedCount.value(rideItem->dateTime.date(), 0) + 1);
    }

    QDate nextAddDate(when);
    QDate lastSourceDate;
    QDate minDate;
    QDate maxDate;
    int activeDays = 0;
    for (RideItem *rideItem : selectedItems) {
        bool found = sameDay && lastSourceDate.isValid() && lastSourceDate == rideItem->dateTime.date();
        while (! found) {
            bool hasPreexisting = preexistingCount.value(nextAddDate, 0) > 0;
            if (hasPreexisting) {
                if (conflictHandling == 1) { // Skip days with preexisting
                    nextAddDate = nextAddDate.addDays(1);
                    ++activeDays;
                    continue;
                } else if (conflictHandling == 2) { // Fail
                    failed = true;
                    break;
                }
            }
            if (activeDays >= restDayAfter) {
                activeDays = 0;
                found = false;
                nextAddDate = nextAddDate.addDays(1);
                continue;
            }
            found = true;
        }
        if (failed) {
            deletionList.clear();
            scheduleList.clear();
            break;
        }
        scheduleList << std::make_pair(rideItem, nextAddDate);
        if (conflictHandling == 0) {
            if (! minDate.isValid() || minDate > nextAddDate) {
                minDate = nextAddDate;
            }
            if (! maxDate.isValid() || maxDate < nextAddDate) {
                maxDate = nextAddDate;
            }
        }

        int remaining = selectedCount.value(rideItem->dateTime.date(), 1) - 1;
        selectedCount.insert(rideItem->dateTime.date(), remaining);
        if (! (sameDay && remaining > 0)) {
            nextAddDate = nextAddDate.addDays(1);
            ++activeDays;
        }
        lastSourceDate = rideItem->dateTime.date();
    }
    if (! failed) {
        QLocale locale;
        if (minDate.isValid() && maxDate.isValid()) {
            for (RideItem *rideItem : preexistingPlanned) {
                if (rideItem->dateTime.date() >= minDate && rideItem->dateTime.date() <= maxDate) {
                    deletionList << rideItem;
                }
            }
        }
        if (! scheduleList.isEmpty()) {
            scheduleLabel->setVisible(true);
            scheduleTree->setVisible(true);
            for (std::pair<RideItem*, QDate> entry : scheduleList) {
                QTreeWidgetItem *scheduleItem = new QTreeWidgetItem();
                scheduleItem->setData(0, Qt::DisplayRole, locale.toString(entry.first->dateTime.date(), QLocale::ShortFormat));
                scheduleItem->setData(1, Qt::DisplayRole, "→");
                scheduleItem->setData(2, Qt::DisplayRole, locale.toString(entry.second, QLocale::ShortFormat));
                scheduleItem->setData(3, Qt::DisplayRole, rideItemSport(entry.first));
                scheduleItem->setData(4, Qt::DisplayRole, rideItemName(entry.first));
                scheduleTree->addTopLevelItem(scheduleItem);
            }
        }
        if (! deletionList.isEmpty()) {
            deletionLabel->setVisible(true);
            deletionTree->setVisible(true);
            for (RideItem *rideItem : deletionList) {
                QTreeWidgetItem *deletionItem = new QTreeWidgetItem();
                deletionItem->setData(0, Qt::DisplayRole, locale.toString(rideItem->dateTime.date(), QLocale::ShortFormat));
                deletionItem->setData(1, Qt::DisplayRole, rideItemSport(rideItem));
                deletionItem->setData(2, Qt::DisplayRole, rideItemName(rideItem));
                deletionTree->addTopLevelItem(deletionItem);
            }
        }
    } else {
        failedLabel->setVisible(true);
    }
}


bool
RepeatSchedulePageSummary::isComplete
() const
{
    return ! failed;
}


QList<RideItem*>
RepeatSchedulePageSummary::getDeletionList
() const
{
    return deletionList;
}


QList<std::pair<RideItem*, QDate>>
RepeatSchedulePageSummary::getScheduleList
() const
{
    return scheduleList;
}


////////////////////////////////////////////////////////////////////////////////
// Helpers

QString
rideItemName
(RideItem const * const rideItem)
{
    QString rideName = rideItem->getText("Route", "").trimmed();
    if (rideName.isEmpty()) {
        rideName = rideItem->getText("Workout Code", "").trimmed();
        if (rideName.isEmpty()) {
            rideName = QObject::tr("<unnamed>");
        }
    }
    return rideName;
}


QString
rideItemSport
(RideItem const * const rideItem)
{
    QString sport = rideItem->sport;
    if (! rideItem->getText("SubSport", "").isEmpty()) {
        sport += " / " + rideItem->getText("SubSport", "");
    }
    return sport;
}
