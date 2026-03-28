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
#include "SaveDialogs.h"

#define HLO "<span style='font-weight:600;'>"
#define HLC "</span>"

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

static constexpr int IndexRole = Qt::UserRole + 100;


////////////////////////////////////////////////////////////////////////////////
// TargetRangeBar

TargetRangeBar::TargetRangeBar
(QString errorMsg, QWidget *parent)
: QFrame(parent), currentState(State::Neutral), errorMsg(errorMsg)
{
    setObjectName("TargetRangeBar");

    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Plain);
    setLineWidth(1 * dpiXFactor);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8 * dpiXFactor, 4 * dpiYFactor, 8 * dpiXFactor, 4 * dpiYFactor);
    layout->setSpacing(6 * dpiXFactor);

    iconLabel = new QLabel(this);
    iconLabel->setFixedWidth(16 * dpiXFactor);
    iconLabel->setAlignment(Qt::AlignCenter);

    textLabel = new QLabel();
    textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    textLabel->setWordWrap(false);
    textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);

    applyStateStyle(State::Neutral);
}


void
TargetRangeBar::setResult
(const QDate &start, const QDate &end, int activityCount, int deletedCount)
{
    QString text;
    if (activityCount == 0) {
        currentState = State::Error;
        text = errorMsg;
    } else {
        QLocale locale;
        QString localFormat = locale.dateFormat(QLocale::ShortFormat);
        QString customFormat = "ddd, " + localFormat;

        currentState = State::Neutral;
        QString duration = formatDuration(start, end);
        text = QString("%1 - %2 (%3) • ")
                      .arg(locale.toString(start, customFormat))
                      .arg(locale.toString(end, customFormat))
                      .arg(duration);
        text += tr("%1 to copy")
                  .arg(activityCount);

        if (deletedCount > 0) {
            text += " • " + tr("%1 to remove").arg(deletedCount);
            currentState = State::Warning;
        }
    }
    if (text != textLabel->text()) {
        textLabel->setText(text);
        applyStateStyle(currentState);
        if (flashEnabled) {
            flash();
        }
    }
}


void
TargetRangeBar::setFlashEnabled
(bool enabled)
{
    flashEnabled = enabled;
}


QString
TargetRangeBar::formatDuration
(const QDate &start, const QDate &end) const
{
    if (! start.isValid() || ! end.isValid()) {
        return "";
    }
    int days = start.daysTo(end) + 1;
    ShowDaysAsUnit unit = showDaysAs(days);
    if (unit == ShowDaysAsUnit::Months) {
        return tr("%1 mo").arg(daysToMonths(days));
    } else if (unit == ShowDaysAsUnit::Weeks) {
        return tr("%1 w").arg(daysToWeeks(days));
    } else {
        return tr("%1 d").arg(days);
    }
}


void
TargetRangeBar::applyStateStyle
(State state)
{
    QIcon icon;
    QColor accentColor;
    switch (state) {
    case State::Neutral:
        icon = style()->standardIcon(QStyle::SP_MessageBoxInformation);
        accentColor = Qt::gray;
        break;
    case State::Warning:
        icon = style()->standardIcon(QStyle::SP_MessageBoxWarning);
        accentColor = QColor(255, 193, 7);
        break;
    case State::Error:
        icon = style()->standardIcon(QStyle::SP_MessageBoxCritical);
        accentColor = Qt::red;
        break;
    }
    QColor bg = QApplication::palette().window().color();
    accentColor.setAlpha(20);
    baseColor = GCColor::blendedColor(accentColor, bg);
    accentColor.setAlpha(200);
    borderColor = GCColor::blendedColor(accentColor, bg);
    iconLabel->setPixmap(icon.pixmap(14 * dpiXFactor, 14 * dpiYFactor));
    setStyleSheet(QString("#TargetRangeBar { background-color: %1; border: 1px solid %2; }")
                         .arg(baseColor.name())
                         .arg(borderColor.name()));
}


QColor
TargetRangeBar::highlightColor
() const
{
    return hlColor;
}


void
TargetRangeBar::setHighlightColor
(const QColor &color)
{
    hlColor = color;
    QColor blended;
    blended.setRed((baseColor.red() + color.red()) / 2);
    blended.setGreen((baseColor.green() + color.green()) / 2);
    blended.setBlue((baseColor.blue() + color.blue()) / 2);
    setStyleSheet(QString("#TargetRangeBar { background-color: %1; border: 1px solid %2; }")
                         .arg(blended.name())
                         .arg(borderColor.name()));
}


void
TargetRangeBar::flash
()
{
    QPropertyAnimation *anim = new QPropertyAnimation(this, "highlightColor");
    anim->setDuration(350);
    anim->setStartValue(GColor(CCALPLANNED));
    anim->setEndValue(baseColor);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}


////////////////////////////////////////////////////////////////////////////////
// IndicatorDelegate

IndicatorDelegate::IndicatorDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
IndicatorDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() != 0) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const bool isRadio = index.data(IndicatorTypeRole).toInt() == RadioIndicator;
    const bool isCheck = index.data(IndicatorTypeRole).toInt() == CheckIndicator;
    if (! isRadio && ! isCheck) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    QStyle *style = opt.widget ? opt.widget->style() : QApplication::style();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget);

    const int size = style->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, opt.widget);
    const int spacing = 4 * dpiXFactor;
    QRect indicatorRect(opt.rect.left() + spacing, opt.rect.top() + (opt.rect.height() - size) / 2, size, size);

    QStyleOptionButton buttonOpt;
    buttonOpt.rect = indicatorRect;
    buttonOpt.state = QStyle::State_Enabled;
    buttonOpt.state |= index.data(IndicatorStateRole).toBool() ? QStyle::State_On : QStyle::State_Off;
    if (opt.state & QStyle::State_MouseOver) {
        buttonOpt.state |= QStyle::State_MouseOver;
    }

    style->drawPrimitive(isRadio ? QStyle::PE_IndicatorRadioButton : QStyle::PE_IndicatorCheckBox, &buttonOpt, painter, opt.widget);

    QRect textRect = opt.rect;
    textRect.setLeft(indicatorRect.right() + spacing);
    painter->save();
    painter->setClipRect(textRect);
    QColor textColor = (opt.state & QStyle::State_Selected) ? opt.palette.highlightedText().color() : opt.palette.text().color();
    painter->setPen(textColor);
    painter->drawText(textRect, opt.displayAlignment, index.data(Qt::DisplayRole).toString());
    painter->restore();
}


bool
IndicatorDelegate::editorEvent
(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    const bool isRadio = index.data(IndicatorTypeRole).toInt() == RadioIndicator;
    const bool isCheck = index.data(IndicatorTypeRole).toInt() == CheckIndicator;
    if (! isRadio && ! isCheck) {
        return QStyledItemDelegate::editorEvent(event, model, option, index);
    } else if (event->type() != QEvent::MouseButtonRelease) {
        return false;
    } else if (isRadio) {
        model->setData(index, true, IndicatorStateRole);
        QModelIndex parent = index.parent();
        for (int row = 0; row < model->rowCount(parent); ++row) {
            QModelIndex sibling = model->index(row, index.column(), parent);
            if (sibling != index && sibling.data(IndicatorTypeRole).toInt() == RadioIndicator) {
                model->setData(sibling, false, IndicatorStateRole);
            }
        }
        return true;
    } else if (isCheck) {
        model->setData(index, ! index.data(IndicatorStateRole).toBool(), IndicatorStateRole);
        return true;
    }
    return false;
}


QSize
IndicatorDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const bool isRadio = index.data(IndicatorTypeRole).toInt() == RadioIndicator;
    const bool isCheck = index.data(IndicatorTypeRole).toInt() == CheckIndicator;
    if ((! isRadio && ! isCheck) || index.column() != 0) {
        return QStyledItemDelegate::sizeHint(option, index);
    }
    QStyle *style = option.widget ? option.widget->style() : QApplication::style();
    const int indicatorWidth = style->pixelMetric(QStyle::PM_IndicatorWidth, nullptr, option.widget);
    const int spacing = 4 * dpiXFactor;
    QFontMetrics fm(option.font);
    int textWidth = fm.horizontalAdvance(index.data(Qt::DisplayRole).toString());
    return QSize(indicatorWidth + spacing + textWidth + spacing, QStyledItemDelegate::sizeHint(option, index).height());
}


////////////////////////////////////////////////////////////////////////////////
// RepeatScheduleWizard

RepeatScheduleWizard::RepeatScheduleWizard
(Context *context, const QDate &when, QWidget *parent)
: QWizard(parent), context(context), targetRangeStart(when), targetRangeEnd(when)
{
    setWindowTitle(tr("Repeat Schedule"));
    setMinimumSize(800 * dpiXFactor, 750 * dpiYFactor);
    setModal(true);

#ifdef Q_OS_MAC
    setWizardStyle(QWizard::MacStyle);
#else
    setWizardStyle(QWizard::ModernStyle);
#endif
    setPixmap(ICON_TYPE, svgAsColoredPixmap(":images/breeze/media-playlist-repeat.svg", QSize(ICON_SIZE * dpiXFactor, ICON_SIZE * dpiYFactor), ICON_MARGIN * dpiXFactor, ICON_COLOR));

    setPage(PageSetup, new RepeatSchedulePageSetup(context, when));
    setPage(PageActivities, new RepeatSchedulePageActivities(context));
    setPage(PageSummary, new RepeatSchedulePageSummary(context));
    setStartId(PageSetup);
}


QDate
RepeatScheduleWizard::getTargetRangeStart
() const
{
    return targetRangeStart;
}


QDate
RepeatScheduleWizard::getTargetRangeEnd
() const
{
    return targetRangeEnd;
}


const QList<RideItem*>&
RepeatScheduleWizard::getDeletionList
() const
{
    return deletionList;
}


void
RepeatScheduleWizard::updateTargetRange
()
{
    updateTargetRange(sourceRangeStart, sourceRangeEnd, keepGap, preferOriginal);
}


void
RepeatScheduleWizard::updateTargetRange
(QDate sourceStart, QDate sourceEnd, bool keepGap, bool preferOriginal)
{
    if (   sourceRangeStart != sourceStart
        || sourceRangeEnd != sourceEnd
        || keepGap != this->keepGap
        || preferOriginal != this->preferOriginal) {

        sourceRangeStart = sourceStart;
        sourceRangeEnd = sourceEnd;
        this->keepGap = keepGap;
        this->preferOriginal = preferOriginal;

        sourceRides.clear();
        for (RideItem *rideItem : context->athlete->rideCache->rides()) {
            if (   rideItem == nullptr
                || ! rideItem->planned) {
                continue;
            }
            QDate rideDate = getDate(rideItem, preferOriginal);
            if (   rideDate < sourceStart
                || rideDate > sourceEnd) {
                continue;
            }
            sourceRides << SourceRide { rideItem, rideDate, QDate(), true, -1, false };
        }
        std::sort(sourceRides.begin(), sourceRides.end(),
            [](const SourceRide &a, const SourceRide &b) { return a.sourceDate < b.sourceDate; });

        // Assume all planned activities from the range will be copied
        int prelimFrontGap = 0;
        if (! keepGap && ! sourceRides.isEmpty()) {
            prelimFrontGap = sourceStart.daysTo(sourceRides.first().sourceDate);
        }
        for (SourceRide &sourceRide : sourceRides) {
            sourceRide.targetDate = targetRangeStart.addDays(
                sourceRangeStart.daysTo(sourceRide.sourceDate) - prelimFrontGap);
        }

        // QDateTime of any planned RideItem must be unique
        QHash<QDateTime, int> targetKeyCount;
        for (const SourceRide &sourceRide : sourceRides) {
            QDateTime key(sourceRide.targetDate, sourceRide.rideItem->dateTime.time());
            targetKeyCount[key]++;
        }
        QHash<QDateTime, int> keyToGroup;
        int nextGroup = 0;
        for (SourceRide &sourceRide : sourceRides) {
            QDateTime key(sourceRide.targetDate, sourceRide.rideItem->dateTime.time());
            if (targetKeyCount[key] > 1) {
                if (! keyToGroup.contains(key)) {
                    keyToGroup[key] = nextGroup++;
                }
                sourceRide.conflictGroup = keyToGroup[key];
            } else {
                sourceRide.conflictGroup = -1;
            }
        }

        QHash<int, bool> groupHasSelection;
        for (SourceRide &sourceRide : sourceRides) {
            if (sourceRide.conflictGroup < 0) {
                continue;
            }
            if (! groupHasSelection.value(sourceRide.conflictGroup, false)) {
                sourceRide.selected = true;
                groupHasSelection[sourceRide.conflictGroup] = true;
            } else {
                sourceRide.selected = false;
            }
        }
    }

    // Calculate frontGap and rangeLength
    frontGap = 0;
    int rangeLength = 0;
    if (! sourceRides.isEmpty()) {
        QDate firstSelectedDate;
        QDate lastSelectedDate;
        for (const SourceRide &sourceRide : sourceRides) {
            if (! sourceRide.selected) {
                continue;
            }
            if (firstSelectedDate.isNull()) {
                firstSelectedDate = sourceRide.sourceDate;
                if (! keepGap) {
                    frontGap = sourceStart.daysTo(firstSelectedDate);
                }
            }
            lastSelectedDate = sourceRide.sourceDate;
        }

        for (SourceRide &sourceRide : sourceRides) {
            sourceRide.targetDate = targetRangeStart.addDays(
                sourceRangeStart.daysTo(sourceRide.sourceDate) - frontGap);
        }

        if (firstSelectedDate.isValid()) {
            rangeLength = sourceStart.daysTo(sourceEnd);
            if (! keepGap) {
                rangeLength -= (frontGap + lastSelectedDate.daysTo(sourceEnd));
            }
        }
    }
    targetRangeEnd = targetRangeStart.addDays(rangeLength);

    // Find conflicting planned and linked activities (they wont be autodeleted)
    deletionList.clear();
    QSet<QDateTime> blockedKeys;
    for (RideItem *rideItem : context->athlete->rideCache->rides()) {
        if (   rideItem == nullptr
            || ! rideItem->planned) {
            continue;
        }
        QDate rideDate = rideItem->dateTime.date();
        if (   rideDate < targetRangeStart
            || rideDate > targetRangeEnd) {
            continue;
        }
        if (rideItem->hasLinkedActivity()) {
            blockedKeys.insert(QDateTime(rideDate, rideItem->dateTime.time()));
        } else {
            deletionList << rideItem;
        }
    }
    for (SourceRide &sourceRide : sourceRides) {
        QDateTime key(sourceRide.targetDate, sourceRide.rideItem->dateTime.time());
        sourceRide.targetBlocked = blockedKeys.contains(key);
    }

    emit targetRangeChanged();
}


void
RepeatScheduleWizard::done
(int result)
{
    if (result == QDialog::Accepted) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        const QList<RideItem*> &deletionList = getDeletionList();
        QList<std::pair<RideItem*, QDate>> scheduleList;
        for (const SourceRide &sourceRide : sourceRides) {
            if (! sourceRide.selected || sourceRide.targetBlocked) {
                continue;
            }
            scheduleList << std::pair<RideItem*, QDate> { sourceRide.rideItem, sourceRide.targetDate };
        }
        context->tab->setNoSwitch(true);
        for (RideItem *rideItem : deletionList) {
            context->athlete->rideCache->removeRide(rideItem->fileName);
        }
        RideCache::OperationPreCheck check = context->athlete->rideCache->checkCopyPlannedActivities(scheduleList);
        if (check.canProceed) {
            RideCache::OperationResult result = context->athlete->rideCache->copyPlannedActivities(scheduleList);
            if (! result.success) {
                QMessageBox::warning(this, "Failed", result.error);
            }
        }
        context->tab->setNoSwitch(false);
        QApplication::restoreOverrideCursor();
    }
    QWizard::done(result);
}


QDate
RepeatScheduleWizard::getDate
(RideItem const * const rideItem, bool preferOriginal) const
{
    QDate date = rideItem->dateTime.date();
    if (preferOriginal) {
        QString originalDateString = rideItem->getText("Original Date", "");
        if (! originalDateString.isEmpty()) {
            QDate originalDate = QDate::fromString(originalDateString, "yyyy/MM/dd");
            if (originalDate.isValid()) {
                date = originalDate;
            }
        }
    }
    return date;
}


////////////////////////////////////////////////////////////////////////////////
// RepeatSchedulePageSetup

RepeatSchedulePageSetup::RepeatSchedulePageSetup
(Context *context, const QDate &when, QWidget *parent)
: QWizardPage(parent), context(context)
{
    QLocale locale;
    QString localFormat = locale.dateFormat(QLocale::ShortFormat);
    QString customFormat = "ddd, " + localFormat;

    setTitle(tr("Repeat Schedule Setup"));
    setSubTitle(tr("Define the time range and repetition strategy for copying activities. Optionally select a season or phase to prefill the dates; only those ending before the target date <b>%1</b> can be selected.")
                  .arg(locale.toString(when, customFormat)));

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
        seasonItem->setDisabled(DateRange(season.getStart(), season.getEnd()).pass(when) || season.getStart() > when);
        if (context->currentSeason() != nullptr && context->currentSeason()->id() == season.id()) {
            currentSeason = seasonItem;
        }
        for (const Phase &phase : season.phases) {
            QTreeWidgetItem *phaseItem = new QTreeWidgetItem();
            phaseItem->setData(0, Qt::DisplayRole, phase.getName());
            phaseItem->setData(0, Qt::UserRole, phase.getStart());
            phaseItem->setData(0, Qt::UserRole + 1, phase.getEnd());
            phaseItem->setDisabled(DateRange(phase.getStart(), phase.getEnd()).pass(when) || phase.getStart() > when);
            seasonItem->addChild(phaseItem);
            if (context->currentSeason() != nullptr && context->currentSeason()->id() == phase.id()) {
                currentSeason = phaseItem;
            }
        }
        seasonTree->addTopLevelItem(seasonItem);
    }

    startDate = new QDateEdit();
    startDate->setMaximumDate(when.addDays(-2));
    startDate->setCalendarPopup(true);
    startDate->setDisplayFormat(customFormat);

    endDate = new QDateEdit(when.addDays(-1));
    endDate->setMaximumDate(when.addDays(-1));
    endDate->setCalendarPopup(true);
    endDate->setDisplayFormat(customFormat);

    QHBoxLayout *dateRangeLayout = new QHBoxLayout();
    dateRangeLayout->addWidget(startDate);
    dateRangeLayout->addWidget(new QLabel(" - "));
    dateRangeLayout->addWidget(endDate);
    dateRangeLayout->addStretch();

    originalRadio = new QRadioButton(tr("As originally planned"));
    currentRadio = new QRadioButton(tr("As currently scheduled"));
    QButtonGroup *sourceDateGroup = new QButtonGroup(this);
    sourceDateGroup->setExclusive(true);
    sourceDateGroup->addButton(originalRadio);
    sourceDateGroup->addButton(currentRadio);
    originalRadio->setChecked(true);

    keepGapCheck = new QCheckBox(tr("Keep leading and trailing gaps"));
    keepGapCheck->setChecked(true);

    targetRangeBar = new TargetRangeBar(tr("No planned activities in source period"));
    targetRangeBar->setMinimumWidth(650 * dpiXFactor);

    QVBoxLayout *form = new QVBoxLayout();
    form->addWidget(new QLabel(HLO + tr("Source Period") + HLC));
    form->addWidget(seasonTree);
    form->addLayout(dateRangeLayout);
    form->addSpacing(10 * dpiYFactor);
    form->addWidget(new QLabel(HLO + tr("Repetition Strategy") + HLC));
    form->addWidget(originalRadio);
    form->addWidget(currentRadio);
    form->addWidget(keepGapCheck);
    form->addSpacing(10 * dpiYFactor);
    form->addWidget(targetRangeBar);

    QWidget *scrollWidget = new QWidget();
    scrollWidget->setLayout(form);
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);

    QVBoxLayout *all = new QVBoxLayout();
    all->addWidget(scrollArea);
    setLayout(all);

    connect(seasonTree, &QTreeWidget::currentItemChanged, this, [this, when](QTreeWidgetItem *current) {
        if (current != nullptr) {
            QDate seasonStart(current->data(0, Qt::UserRole).toDate());
            QDate seasonEnd(current->data(0, Qt::UserRole + 1).toDate());
            if (seasonEnd > when) {
                seasonEnd = when;
            }
            if (seasonStart > when) {
                seasonEnd = when;
            }

            QSignalBlocker b1(startDate);
            QSignalBlocker b2(endDate);
            startDate->setDate(seasonStart);
            endDate->setDate(seasonEnd);
            refresh();
        }
    });
    connect(startDate, &QDateEdit::dateChanged, this, [this, seasonTree](QDate date) {
        endDate->setMinimumDate(date);
        seasonTree->setCurrentItem(nullptr);
        refresh();
    });
    connect(endDate, &QDateEdit::dateChanged, this, [this, seasonTree](QDate date) {
        startDate->setMaximumDate(date);
        seasonTree->setCurrentItem(nullptr);
        refresh();
    });
    connect(keepGapCheck, &QCheckBox::toggled, this, &RepeatSchedulePageSetup::refresh);
    connect(sourceDateGroup, &QButtonGroup::idClicked, this, &RepeatSchedulePageSetup::refresh);

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


void
RepeatSchedulePageSetup::initializePage
()
{
    targetRangeBar->setFlashEnabled(false);
    refresh();
    targetRangeBar->setFlashEnabled(true);
}


bool
RepeatSchedulePageSetup::isComplete
() const
{
    RepeatScheduleWizard *rsw = qobject_cast<RepeatScheduleWizard*>(wizard());
    return rsw->sourceRides.count() > 0;
}


void
RepeatSchedulePageSetup::refresh
()
{
    RepeatScheduleWizard *rsw = qobject_cast<RepeatScheduleWizard*>(wizard());
    if (rsw == nullptr) {
        return;
    }

    rsw->updateTargetRange(startDate->date(), endDate->date(), keepGapCheck->isChecked(), originalRadio->isChecked());

    targetRangeBar->setResult(rsw->getTargetRangeStart(), rsw->getTargetRangeEnd(), rsw->sourceRides.count(), rsw->getDeletionList().count());

    emit completeChanged();
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
    activityTree->setHeaderHidden(true);
    activityTree->setColumnCount(3);
    activityTree->setRootIsDecorated(false);
    activityTree->setSelectionMode(QAbstractItemView::SingleSelection);
    activityTree->setItemDelegate(new IndicatorDelegate());
    activityTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    targetRangeBar = new TargetRangeBar(tr("No selected activities"));
    targetRangeBar->setMinimumWidth(650 * dpiXFactor);

    QWidget *formWidget = new QWidget();
    QVBoxLayout *form = new QVBoxLayout(formWidget);
    form->addWidget(new QLabel(HLO + tr("Activities for your new schedule") + HLC));
    form->addWidget(activityTree);
    form->addSpacing(10 * dpiYFactor);
    form->addWidget(targetRangeBar);

    QWidget *scrollWidget = new QWidget();
    scrollWidget->setLayout(form);
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
    disconnect(dataChangedConnection);
    numSelected = 0;
    activityTree->clear();

    RepeatScheduleWizard *rsw = qobject_cast<RepeatScheduleWizard*>(wizard());
    if (rsw == nullptr) {
        return;
    }
    targetRangeBar->setFlashEnabled(false);
    rsw->updateTargetRange();

    QLocale locale;
    int i = 0;
    while (i < rsw->sourceRides.count()) {
        const SourceRide &sourceRide = rsw->sourceRides[i];

        if (sourceRide.conflictGroup < 0) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setData(0, Qt::DisplayRole, locale.toString(sourceRide.sourceDate, QLocale::ShortFormat));
            item->setData(0, IndicatorDelegate::IndicatorTypeRole, IndicatorDelegate::CheckIndicator);
            item->setData(0, IndicatorDelegate::IndicatorStateRole, sourceRide.selected);
            item->setData(0, IndexRole, i);
            item->setData(1, Qt::DisplayRole, rideItemSport(sourceRide.rideItem));
            item->setData(2, Qt::DisplayRole, rideItemName(sourceRide.rideItem));
            activityTree->addTopLevelItem(item);

            if (sourceRide.selected) {
                ++numSelected;
            }
            ++i;
        } else {
            int group = sourceRide.conflictGroup;
            QList<int> groupIndices;
            while (i < rsw->sourceRides.count() && rsw->sourceRides[i].conflictGroup == group) {
                groupIndices << i;
                ++i;
            }

            bool linkedConflict = std::any_of(groupIndices.begin(), groupIndices.end(), [rsw](int idx) { return rsw->sourceRides[idx].targetBlocked; });

            // Parent node — warning icon, shared target date, no checkbox
            QTreeWidgetItem *groupItem = new QTreeWidgetItem(activityTree);
            groupItem->setFirstColumnSpanned(true);
            groupItem->setIcon(0, style()->standardIcon(QStyle::SP_MessageBoxWarning));
            groupItem->setData(0, Qt::DisplayRole, tr("Choose one to schedule"));
            if (linkedConflict) {
                groupItem->setData(0, Qt::DisplayRole, tr("Blocked by linked activity"));
            }
            groupItem->setExpanded(true);

            for (int idx : groupIndices) {
                const SourceRide &groupRide = rsw->sourceRides[idx];
                QTreeWidgetItem *child = new QTreeWidgetItem(groupItem);
                child->setData(0, Qt::DisplayRole, locale.toString(groupRide.sourceDate, QLocale::ShortFormat));
                child->setData(0, IndicatorDelegate::IndicatorTypeRole, IndicatorDelegate::RadioIndicator);
                child->setData(0, IndicatorDelegate::IndicatorStateRole, groupRide.selected);
                child->setData(0, IndexRole, idx);
                child->setData(1, Qt::DisplayRole, rideItemSport(groupRide.rideItem));
                child->setData(2, Qt::DisplayRole, rideItemName(groupRide.rideItem));
            }
            ++numSelected;
        }
    }

    dataChangedConnection = connect(activityTree->model(), &QAbstractItemModel::dataChanged, this, [this, rsw](const QModelIndex &index) {
        QModelIndex col0Index = index.siblingAtColumn(0);
        int indicatorType = col0Index.data(IndicatorDelegate::IndicatorTypeRole).toInt();
        if (indicatorType == IndicatorDelegate::NoIndicator) {
            return;
        }
        int i = col0Index.data(IndexRole).toInt();
        if (i < 0) {
            return;
        }
        bool checked = col0Index.data(IndicatorDelegate::IndicatorStateRole).toBool();
        rsw->sourceRides[i].selected = checked;
        if (indicatorType == IndicatorDelegate::CheckIndicator) {
            numSelected += checked ? 1 : -1;
            emit completeChanged();
        }
        rsw->updateTargetRange();
        targetRangeBar->setResult(rsw->getTargetRangeStart(), rsw->getTargetRangeEnd(), numSelected, rsw->getDeletionList().count());
    });
    targetRangeBar->setResult(rsw->getTargetRangeStart(), rsw->getTargetRangeEnd(), numSelected, rsw->getDeletionList().count());
    targetRangeBar->setFlashEnabled(true);
}


bool
RepeatSchedulePageActivities::isComplete
() const
{
    return numSelected > 0;
}


////////////////////////////////////////////////////////////////////////////////
// RepeatSchedulePageSummary

RepeatSchedulePageSummary::RepeatSchedulePageSummary
(Context *context, QWidget *parent)
: QWizardPage(parent), context(context)
{
    setTitle(tr("Repeat Schedule Summary"));
    setSubTitle(tr("Preview the schedule updates, including planned additions and deletions. No changes will be made until you continue."));

    setFinalPage(true);

    scheduleLabel = new QLabel(HLO + tr("New Schedule Overview") + HLC);
    scheduleTree = new QTreeWidget();
    scheduleTree->setColumnCount(5);
    basicTreeWidgetStyle(scheduleTree, false);
    scheduleTree->setHeaderHidden(true);

    deletionLabel = new QLabel(HLO + tr("Planned Activities Marked for Deletion") + HLC);
    deletionTree = new QTreeWidget();
    deletionTree->setColumnCount(3);
    basicTreeWidgetStyle(deletionTree, false);
    deletionTree->setHeaderHidden(true);

    targetRangeBar = new TargetRangeBar(tr("No selected activities"));
    targetRangeBar->setMinimumWidth(650 * dpiXFactor);
    targetRangeBar->setFlashEnabled(false);

    QWidget *formWidget = new QWidget();
    QVBoxLayout *form = new QVBoxLayout(formWidget);
    form->addWidget(scheduleLabel);
    form->addWidget(scheduleTree);
    form->addSpacing(10 * dpiYFactor);
    form->addWidget(deletionLabel);
    form->addWidget(deletionTree);
    form->addSpacing(10 * dpiYFactor);
    form->addWidget(targetRangeBar);

    QWidget *scrollWidget = new QWidget();
    scrollWidget->setLayout(form);
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
    scheduleTree->clear();
    deletionTree->clear();

    scheduleLabel->setVisible(false);
    scheduleTree->setVisible(false);
    deletionLabel->setVisible(false);
    deletionTree->setVisible(false);

    RepeatScheduleWizard *rsw = qobject_cast<RepeatScheduleWizard*>(wizard());
    if (rsw == nullptr) {
        return;
    }

    QLocale locale;
    int numSelected = 0;
    for (const SourceRide &sourceRide : rsw->sourceRides) {
        if (! sourceRide.selected) {
            continue;
        }
        QTreeWidgetItem *scheduleItem = new QTreeWidgetItem(scheduleTree);
        scheduleItem->setData(0, Qt::DisplayRole, locale.toString(sourceRide.sourceDate, QLocale::ShortFormat));
        scheduleItem->setData(1, Qt::DisplayRole, "→");
        if (! sourceRide.targetBlocked) {
            scheduleItem->setData(2, Qt::DisplayRole, locale.toString(sourceRide.targetDate, QLocale::ShortFormat));
            ++numSelected;
        } else {
            QFont font;
            font.setItalic(true);
            scheduleItem->setData(0, Qt::ForegroundRole, palette().color(QPalette::Disabled, QPalette::Text));
            scheduleItem->setData(1, Qt::ForegroundRole, palette().color(QPalette::Disabled, QPalette::Text));
            scheduleItem->setData(2, Qt::ForegroundRole, palette().color(QPalette::Disabled, QPalette::Text));
            scheduleItem->setData(2, Qt::FontRole, font);
            scheduleItem->setData(2, Qt::DisplayRole, tr("skipped"));
            scheduleItem->setData(3, Qt::ForegroundRole, palette().color(QPalette::Disabled, QPalette::Text));
            scheduleItem->setData(4, Qt::ForegroundRole, palette().color(QPalette::Disabled, QPalette::Text));
        }
        scheduleItem->setData(3, Qt::DisplayRole, rideItemSport(sourceRide.rideItem));
        scheduleItem->setData(4, Qt::DisplayRole, rideItemName(sourceRide.rideItem));
    }
    scheduleLabel->setVisible(true);
    scheduleTree->setVisible(true);
    const QList<RideItem*> &deletionList = rsw->getDeletionList();
    if (deletionList.count() > 0) {
        for (RideItem *rideItem : deletionList) {
            QTreeWidgetItem *deletionItem = new QTreeWidgetItem(deletionTree);
            deletionItem->setData(0, Qt::DisplayRole, locale.toString(rideItem->dateTime.date(), QLocale::ShortFormat));
            deletionItem->setData(1, Qt::DisplayRole, rideItemSport(rideItem));
            deletionItem->setData(2, Qt::DisplayRole, rideItemName(rideItem));
        }
        deletionLabel->setVisible(true);
        deletionTree->setVisible(true);
    }
    targetRangeBar->setResult(rsw->getTargetRangeStart(), rsw->getTargetRangeEnd(), numSelected, rsw->getDeletionList().count());
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
