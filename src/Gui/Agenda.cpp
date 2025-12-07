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

#include "Agenda.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QAbstractItemDelegate>
#endif
#include <QHeaderView>
#include <QGridLayout>
#include <QToolTip>

#include "Colors.h"
#include "Settings.h"
#include "Context.h"


//////////////////////////////////////////////////////////////////////////////
// AgendaTree

AgendaTree::AgendaTree
(QWidget *parent)
: QTreeWidget(parent)
{
    setColumnCount(2);
    setUniformRowHeights(false);
    setItemsExpandable(false);
    setRootIsDecorated(false);
    setIndentation(0);

    multiDelegate = new AgendaMultiDelegate(this);
    AgendaMultiDelegate::Attributes multiAttributes = multiDelegate->getAttributes();
    multiAttributes.paddingDL = QMargins(20 * dpiXFactor, 9 * dpiYFactor, 4 * dpiXFactor, 9 * dpiYFactor);
    multiAttributes.paddingHL = QMargins(20 * dpiXFactor, 20 * dpiYFactor, 20 * dpiXFactor, 14 * dpiYFactor);
    multiDelegate->setAttributes(multiAttributes);
    setItemDelegateForColumn(0, multiDelegate);

    entryDelegate = new AgendaEntryDelegate(this);
    AgendaEntryDelegate::Attributes entryAttributes = entryDelegate->getAttributes();
    entryAttributes.padding = QMargins(0 * dpiXFactor, 9 * dpiYFactor, 20 * dpiXFactor, 9 * dpiYFactor);
    entryDelegate->setAttributes(entryAttributes);
    setItemDelegateForColumn(1, entryDelegate);

    setFocusPolicy(Qt::NoFocus);
    setFrameShape(QFrame::NoFrame);
    header()->setVisible(false);
    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(1, QHeaderView::Stretch);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
}


void
AgendaTree::updateDate
()
{
    currentDate = QDate::currentDate();
    emit dayChanged(currentDate);
}


QDate
AgendaTree::selectedDate
() const
{
    return currentDate;
}


bool
AgendaTree::isRowHovered
(const QModelIndex &index) const
{
    return    (   hoveredIndex.isValid()
               && hoveredIndex.model() == index.model()
               && hoveredIndex == index.siblingAtColumn(0))
           || (   contextMenuIndex.isValid()
               && contextMenuIndex.model() == index.model()
               && contextMenuIndex == index.siblingAtColumn(0));
}


void
AgendaTree::setMaxTertiaryLines
(int maxTertiaryLines)
{
    AgendaEntryDelegate::Attributes attributes = entryDelegate->getAttributes();
    attributes.maxTertiaryLines = maxTertiaryLines;
    entryDelegate->setAttributes(attributes);
    doItemsLayout();
}


void
AgendaTree::changeEvent
(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange) {
        clearHover();
        QTimer::singleShot(0, this, [this]() {
            QPalette treePal = palette();
            treePal.setColor(QPalette::Base, treePal.color(QPalette::Base));
            setPalette(treePal);
            viewport()->setPalette(treePal);
            setAutoFillBackground(false);
        });
    }
    QWidget::changeEvent(event);
}


bool
AgendaTree::viewportEvent
(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
        QModelIndex hoverIndex = indexAt(helpEvent->pos());

        if (! hoverIndex.isValid()) {
            return QTreeWidget::viewportEvent(event);
        }

        QModelIndex col1 = hoverIndex.siblingAtColumn(1);
        if (! col1.isValid()) {
            return QTreeWidget::viewportEvent(event);
        }

        QString toolTipText;
        if (entryDelegate->hasToolTip(col1, &toolTipText)) {
            QToolTip::showText(helpEvent->globalPos(), toolTipText, this);
            return true;
        }
        QToolTip::hideText();
        event->ignore();
        return true;
    }
    return QTreeWidget::viewportEvent(event);
}


void
AgendaTree::mouseMoveEvent
(QMouseEvent *event)
{
    if (! contextMenuIndex.isValid()) {
        updateHoveredIndex(event->pos());
    }
    QTreeWidget::mouseMoveEvent(event);
}


void
AgendaTree::wheelEvent
(QWheelEvent *event)
{
    if (! contextMenuIndex.isValid()) {
        updateHoveredIndex(event->position().toPoint());
    }
    QTreeWidget::wheelEvent(event);
}


void
AgendaTree::leaveEvent
(QEvent *event)
{
    if (! contextMenuIndex.isValid()) {
        clearHover();
    }
    QTreeWidget::leaveEvent(event);
}


#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void
AgendaTree::enterEvent
(QEvent *event)
{
    if (! contextMenuIndex.isValid()) {
        mapFromGlobal(QCursor::pos());
    }
#else
void
AgendaTree::enterEvent
(QEnterEvent *event)
{
    if (! contextMenuIndex.isValid()) {
        updateHoveredIndex(event->position().toPoint());
    }
#endif
    QTreeWidget::enterEvent(event);
}


void
AgendaTree::updateHoveredIndex
(const QPoint &pos)
{
    QModelIndex index = indexAt(pos);

    if (index.isValid()) {
        index = index.siblingAtColumn(0);
    }

    if (index != hoveredIndex) {
        QToolTip::hideText();
        QPersistentModelIndex oldIndex = hoveredIndex;
        hoveredIndex = QPersistentModelIndex(index);

        if (oldIndex.isValid()) {

            QRect oldRect = visualRect(oldIndex);
            oldRect.setLeft(0);
            oldRect.setRight(viewport()->width());
            viewport()->update(oldRect);
        }

        if (hoveredIndex.isValid()) {
            QRect newRect = visualRect(hoveredIndex);
            newRect.setLeft(0);
            newRect.setRight(viewport()->width());
            viewport()->update(newRect);
        }
    }
}


void
AgendaTree::clearHover
()
{
    if (hoveredIndex.isValid()) {
        QRect oldRect = visualRect(hoveredIndex);
        oldRect.setLeft(0);
        oldRect.setRight(viewport()->width());
        viewport()->update(oldRect);
        QToolTip::hideText();

        hoveredIndex = QPersistentModelIndex();
    }
}


void
AgendaTree::contextMenuEvent
(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (! index.isValid()) {
        return;
    }

    index = index.siblingAtColumn(0);
    QMenu *contextMenu = createContextMenu(index);
    if (contextMenu == nullptr) {
        return;
    }

    contextMenuIndex = QPersistentModelIndex(index);
    hoveredIndex = contextMenuIndex;

    QRect rowRect = visualRect(index);
    rowRect.setLeft(0);
    rowRect.setRight(viewport()->width());
    viewport()->update(rowRect);

    contextMenu->exec(event->globalPos());

    delete contextMenu;

    QPersistentModelIndex oldContextIndex = contextMenuIndex;
    contextMenuIndex = QPersistentModelIndex();

    QPoint currentPos = viewport()->mapFromGlobal(QCursor::pos());
    if (viewport()->rect().contains(currentPos)) {
        updateHoveredIndex(currentPos);
    } else {
        if (oldContextIndex.isValid()) {
            QRect oldRowRect = visualRect(oldContextIndex);
            oldRowRect.setLeft(0);
            oldRowRect.setRight(viewport()->width());
            viewport()->update(oldRowRect);
        }
        hoveredIndex = QPersistentModelIndex();
    }
}


void
AgendaTree::drawRow
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool isTopLevel = ! index.parent().isValid();
    int type = index.siblingAtColumn(0).data(AgendaMultiDelegate::TypeRole).toInt();
    bool hover = isRowHovered(index);
    QStyleOptionViewItem opt = option;
    if (! isTopLevel && type == 0 && hover) {
        painter->save();
        QColor bgColor = option.palette.color(QPalette::Active, QPalette::Highlight);
        bgColor.setAlphaF(0.2);
        QRect hoverRect(option.rect);
        hoverRect.setX(hoverRect.x() + 4 * dpiXFactor);
        painter->fillRect(hoverRect, bgColor);
        bgColor.setAlphaF(0.6);
        QRect markerRect(option.rect);
        markerRect.setWidth(4 * dpiXFactor);
        painter->fillRect(markerRect, bgColor);
        painter->restore();
        opt.state |= QStyle::State_MouseOver;
    } else {
        opt.state &= ~QStyle::State_MouseOver;
    }
    QTreeWidget::drawRow(painter, opt, index);
}


QMenu*
AgendaTree::createContextMenu
(const QModelIndex &index)
{
    Q_UNUSED(index)

    return nullptr;
}


void
AgendaTree::fillFonts
(Fonts &fonts) const
{
    fonts.defaultFont = font();
    fonts.relativeFont = font();
    fonts.relativeFont.setWeight(QFont::ExtraLight);
    fonts.hoverFont = font();
    fonts.hoverFont.setWeight(QFont::DemiBold);
    fonts.headlineDefaultFont = font();
    fonts.headlineTodayFont = font();
    fonts.headlineEmptyFont = font();
    fonts.headlineSmallEmptyFont = font();
    if (fonts.headlineDefaultFont.pointSizeF() > 0) {
        fonts.headlineDefaultFont.setPointSizeF(fonts.headlineDefaultFont.pointSizeF() * 1.3);
        fonts.headlineTodayFont.setPointSizeF(fonts.headlineTodayFont.pointSizeF() * 1.35);
        fonts.headlineEmptyFont.setPointSizeF(fonts.headlineEmptyFont.pointSizeF() * 1.3);
        fonts.headlineSmallEmptyFont.setPointSizeF(fonts.headlineSmallEmptyFont.pointSizeF() * 1.1);
    } else {
        fonts.headlineDefaultFont.setPixelSize(fonts.headlineDefaultFont.pixelSize() * 1.3);
        fonts.headlineTodayFont.setPixelSize(fonts.headlineTodayFont.pixelSize() * 1.35);
        fonts.headlineEmptyFont.setPixelSize(fonts.headlineEmptyFont.pixelSize() * 1.3);
        fonts.headlineSmallEmptyFont.setPixelSize(fonts.headlineSmallEmptyFont.pixelSize() * 1.1);
    }
    fonts.headlineTodayFont.setWeight(QFont::DemiBold);
    fonts.headlineDefaultFont.setWeight(QFont::DemiBold);
    fonts.headlineEmptyFont.setWeight(QFont::DemiBold);
    fonts.headlineEmptyFont.setItalic(true);
    fonts.headlineSmallEmptyFont.setWeight(QFont::DemiBold);
    fonts.headlineSmallEmptyFont.setItalic(true);
}


//////////////////////////////////////////////////////////////////////////////
// ActivityTree

ActivityTree::ActivityTree
(QWidget *parent)
: AgendaTree(parent)
{
    AgendaEntryDelegate::Attributes attributes = entryDelegate->getAttributes();
    attributes.secondaryWeight = attributes.primaryWeight;
    entryDelegate->setAttributes(attributes);

    connect(this, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int column) {
        Q_UNUSED(column)

        QVariant data = item->data(1, AgendaEntryDelegate::EntryRole);
        if (! data.isNull()) {
            CalendarEntry entry = data.value<CalendarEntry>();
            if (entry.type == ENTRY_TYPE_PLANNED_ACTIVITY) {
                emit viewActivity(entry);
            }
        }
    });
}


void
ActivityTree::setPastDays
(int days)
{
    pastDays = days;
    emit dayChanged(selectedDate());
}


void
ActivityTree::setFutureDays
(int days)
{
    futureDays = days;
    emit dayChanged(selectedDate());
}


void
ActivityTree::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activities)
{
    QDate today = selectedDate();
    QDate pastFirst = today.addDays(-pastDays);
    QDate futureLast = today.addDays(futureDays);
    if (! pastFirst.isValid() || ! futureLast.isValid()) {
        return;
    }
    bool todayHasPlanned = false;
    QList<QDate> missedDays;
    QList<QDate> upcomingDays;
    QHash<QDate, QList<CalendarEntry>> missedEntries;
    QList<CalendarEntry> todayEntries;
    QHash<QDate, QList<CalendarEntry>> upcomingEntries;
    for (QDate date = pastFirst; date <= futureLast; date = date.addDays(1)) {
        QList<CalendarEntry> dateEntries;
        dateEntries = activities.value(date);
        for (const CalendarEntry &entry : dateEntries) {
            if (entry.type == ENTRY_TYPE_PLANNED_ACTIVITY) {
                if (date < today) {
                    if (! missedDays.contains(date)) {
                        missedDays << date;
                    }
                    missedEntries[date].append(entry);
                } else if (date > today) {
                    if (! upcomingDays.contains(date)) {
                        upcomingDays << date;
                    }
                    upcomingEntries[date].append(entry);
                } else {
                    todayHasPlanned = true;
                    todayEntries.append(entry);
                }
            }
        }
    }
    std::sort(missedDays.begin(), missedDays.end());
    std::sort(upcomingDays.begin(), upcomingDays.end());

    clearHover();
    clear();
    QString label;

    Fonts fonts;
    fillFonts(fonts);

    if (pastDays > 0) {
        QTreeWidgetItem *pastItem = new QTreeWidgetItem();
        addTopLevelItem(pastItem);
        QFont hlFont(fonts.headlineDefaultFont);
        if (missedDays.count() > 0) {
            label = tr("Missed Planned Activities");
            for (const QDate &date : missedDays) {
                addEntries(today, date, missedEntries[date], pastItem, fonts);
            }
        } else {
            hlFont = fonts.headlineSmallEmptyFont;
            label = tr("No missed planned activities");
        }
        if (pastDays > 1) {
            label += " " + tr("(Past %1 days)").arg(pastDays);
        } else {
            label += " " + tr("(Yesterday)");
        }
        pastItem->setData(0, Qt::DisplayRole, label);
        pastItem->setData(0, Qt::FontRole, hlFont);
        pastItem->setData(0, AgendaMultiDelegate::TypeRole, 1);
        pastItem->setFirstColumnSpanned(true);
    }

    QTreeWidgetItem *todayItem = new QTreeWidgetItem();
    addTopLevelItem(todayItem);
    QFont hlFont(fonts.headlineTodayFont);
    if (todayHasPlanned) {
        label = tr("Today’s Planned Activities");
        addEntries(today, today, todayEntries, todayItem, fonts);
    } else {
        hlFont = fonts.headlineEmptyFont;
        label = tr("No planned activities for today");
    }
    todayItem->setData(0, Qt::DisplayRole, label);
    todayItem->setData(0, Qt::FontRole, hlFont);
    todayItem->setData(0, AgendaMultiDelegate::TypeRole, 1);
    todayItem->setFirstColumnSpanned(true);

    if (futureDays > 0) {
        QTreeWidgetItem *futureItem = new QTreeWidgetItem();
        addTopLevelItem(futureItem);
        QFont hlFont(fonts.headlineDefaultFont);
        if (upcomingDays.count() > 0) {
            label = tr("Upcoming Planned Activities");
            for (const QDate &date : upcomingDays) {
                addEntries(today, date, upcomingEntries[date], futureItem, fonts);
            }
        } else {
            hlFont = fonts.headlineSmallEmptyFont;
            label = tr("No upcoming planned activities");
        }
        if (futureDays > 1) {
            label += " " + tr("(Next %1 days)").arg(futureDays);
        } else {
            label += " " + tr("(Tomorrow)");
        }
        futureItem->setData(0, Qt::DisplayRole, label);
        futureItem->setData(0, Qt::FontRole, hlFont);
        futureItem->setData(0, AgendaMultiDelegate::TypeRole, 1);
        futureItem->setFirstColumnSpanned(true);
    }

    expandAll();
}


QDate
ActivityTree::firstVisibleDay
() const
{
    return selectedDate().addDays(-pastDays);
}


QDate
ActivityTree::lastVisibleDay
() const
{
    return selectedDate().addDays(futureDays);
}


QMenu*
ActivityTree::createContextMenu
(const QModelIndex &index)
{
    QTreeWidgetItem *item = itemFromIndex(index);
    if (! item) {
        return nullptr;
    }
    QVariant entryData = item->data(1, AgendaEntryDelegate::EntryRole);
    if (entryData.isNull()) {
        return nullptr;
    }
    CalendarEntry entry = entryData.value<CalendarEntry>();
    if (entry.type != ENTRY_TYPE_PLANNED_ACTIVITY) {
        return nullptr;
    }
    QMenu *contextMenu = new QMenu(this);
    if (entry.hasTrainMode) {
        contextMenu->addAction(tr("Show in train mode..."), this, [this, entry]() {
            emit showInTrainMode(entry);
        });
    }
    contextMenu->addAction(tr("View planned activity..."), this, [this, entry]() {
        emit viewActivity(entry);
    });
    return contextMenu;
}


void
ActivityTree::addEntries
(const QDate &today, const QDate &date, const QList<CalendarEntry> &activities, QTreeWidgetItem *parent, const Fonts &fonts)
{
    int activityIdx = 0;
    for (const CalendarEntry &activity : activities) {
        QTreeWidgetItem *entryItem = new QTreeWidgetItem();
        QString diffStr;
        int diff = date.daysTo(today);
        if (diff > 1) {
            diffStr = tr("%1 days ago").arg(diff);
        } else if (diff == 1) {
            diffStr = tr("yesterday");
        } else if (diff == 0) {
            diffStr = tr("today");
        } else if (diff == -1) {
            diffStr = tr("tomorrow");
        } else {
            diffStr = tr("in %1 days").arg(std::abs(diff));
        }
        if (activityIdx == 0) {
            entryItem->setData(0, Qt::DisplayRole, date.toString(tr("ddd, d.M.")));
            entryItem->setData(0, Qt::FontRole, fonts.defaultFont);
            entryItem->setData(0, AgendaMultiDelegate::SecondaryDisplayRole, diffStr);
            entryItem->setData(0, AgendaMultiDelegate::SecondaryFontRole, fonts.relativeFont);
        }
        entryItem->setData(0, AgendaMultiDelegate::HoverTextRole, date.toString(tr("ddd, d.M.")));
        entryItem->setData(0, AgendaMultiDelegate::HoverFontRole, fonts.hoverFont);
        entryItem->setData(0, AgendaMultiDelegate::SecondaryHoverTextRole, diffStr);
        entryItem->setData(0, AgendaMultiDelegate::SecondaryHoverFontRole, fonts.hoverFont);
        entryItem->setData(1, AgendaEntryDelegate::EntryRole, QVariant::fromValue(activity));
        entryItem->setData(1, AgendaEntryDelegate::EntryDateRole, date);
        parent->addChild(entryItem);
        ++activityIdx;
    }
}


//////////////////////////////////////////////////////////////////////////////
// PhaseTree

void
PhaseTree::fillEntries
(const std::pair<QList<CalendarEntry>, QList<CalendarEntry>> &phases)
{
    QDate today = selectedDate();
    if (! today.isValid()) {
        return;
    }
    clear();

    Fonts fonts;
    fillFonts(fonts);

    QTreeWidgetItem *ongoingItem = new QTreeWidgetItem();
    addTopLevelItem(ongoingItem);
    QFont hlFont(fonts.headlineDefaultFont);
    QString label;
    if (phases.first.count() > 0) {
        label = tr("Ongoing Phases");
        addEntries(today, phases.first, ongoingItem, fonts);
    } else {
        hlFont = fonts.headlineSmallEmptyFont;
        label = tr("No ongoing phases");
    }
    ongoingItem->setData(0, Qt::DisplayRole, label);
    ongoingItem->setData(0, Qt::FontRole, hlFont);
    ongoingItem->setData(0, AgendaMultiDelegate::TypeRole, 1);
    ongoingItem->setFirstColumnSpanned(true);

    QTreeWidgetItem *upcomingItem = new QTreeWidgetItem();
    addTopLevelItem(upcomingItem);
    hlFont = fonts.headlineDefaultFont;
    if (phases.second.count() > 0) {
        label = tr("Upcoming Phases");
        addEntries(today, phases.second, upcomingItem, fonts);
    } else {
        hlFont = fonts.headlineSmallEmptyFont;
        label = tr("No upcoming phases");
    }
    upcomingItem->setData(0, Qt::DisplayRole, label);
    upcomingItem->setData(0, Qt::FontRole, hlFont);
    upcomingItem->setData(0, AgendaMultiDelegate::TypeRole, 1);
    upcomingItem->setFirstColumnSpanned(true);

    expandAll();
}


QMenu*
PhaseTree::createContextMenu
(const QModelIndex &index)
{
    QTreeWidgetItem *item = itemFromIndex(index);
    if (! item) {
        return nullptr;
    }
    QVariant entryData = item->data(1, AgendaEntryDelegate::EntryRole);
    if (entryData.isNull()) {
        return nullptr;
    }
    CalendarEntry entry = entryData.value<CalendarEntry>();
    if (entry.type != ENTRY_TYPE_PHASE) {
        return nullptr;
    }
    QMenu *contextMenu = new QMenu(this);
    contextMenu->addAction(tr("Edit phase..."), this, [this, entry]() {
        emit editPhaseEntry(entry);
    });
    return contextMenu;
}


void
PhaseTree::addEntries
(const QDate &today, const QList<CalendarEntry> &phases, QTreeWidgetItem *parent, const Fonts &fonts)
{
    for (const CalendarEntry &phase : phases) {
        QTreeWidgetItem *entryItem = new QTreeWidgetItem();
        QString diffStr;
        int diffStart = today.daysTo(phase.spanStart);
        int diffEnd = today.daysTo(phase.spanEnd);
        if (diffStart >= 0) {
            if (diffStart == 0) {
                diffStr = tr("begins today");
            } else if (diffStart == 1) {
                diffStr = tr("begins tomorrow");
            } else {
                ShowDaysAsUnit unit = showDaysAs(diffStart);
                if (unit == ShowDaysAsUnit::Days) {
                    diffStr = tr("begins in %1 days").arg(diffStart);
                } else if (unit == ShowDaysAsUnit::Weeks) {
                    int weeks = daysToWeeks(diffStart);
                    if (weeks > 1) {
                        diffStr = tr("begins in %1 weeks").arg(weeks);
                    } else {
                        diffStr = tr("begins in %1 week").arg(weeks);
                    }
                } else {
                    int months = daysToMonths(diffStart);
                    if (months > 1) {
                        diffStr = tr("begins in %1 months").arg(months);
                    } else {
                        diffStr = tr("begins in %1 month").arg(months);
                    }
                }
            }
        } else {
            if (diffEnd == 0) {
                diffStr = tr("ends today");
            } else if (diffEnd == 1) {
                diffStr = tr("ends tomorrow");
            } else {
                ShowDaysAsUnit unit = showDaysAs(diffEnd);
                if (unit == ShowDaysAsUnit::Days) {
                    diffStr = tr("ends in %1 days").arg(diffEnd);
                } else if (unit == ShowDaysAsUnit::Weeks) {
                    int weeks = daysToWeeks(diffEnd);
                    if (weeks > 1) {
                        diffStr = tr("ends in %1 weeks").arg(weeks);
                    } else {
                        diffStr = tr("ends in %1 week").arg(weeks);
                    }
                } else {
                    int months = daysToMonths(diffEnd);
                    if (months > 1) {
                        diffStr = tr("ends in %1 months").arg(months);
                    } else {
                        diffStr = tr("ends in %1 month").arg(months);
                    }
                }
            }
        }
        QString dateRangeStr = QString("%1 - %2")
                                      .arg(phase.spanStart.toString(tr("ddd, d.M.")))
                                      .arg(phase.spanEnd.toString(tr("ddd, d.M.")));
        entryItem->setData(0, Qt::DisplayRole, dateRangeStr);
        entryItem->setData(0, Qt::FontRole, fonts.defaultFont);
        entryItem->setData(0, AgendaMultiDelegate::SecondaryDisplayRole, diffStr);
        entryItem->setData(0, AgendaMultiDelegate::SecondaryFontRole, fonts.relativeFont);
        entryItem->setData(0, AgendaMultiDelegate::HoverTextRole, dateRangeStr);
        entryItem->setData(0, AgendaMultiDelegate::HoverFontRole, fonts.hoverFont);
        entryItem->setData(0, AgendaMultiDelegate::SecondaryHoverTextRole, diffStr);
        entryItem->setData(0, AgendaMultiDelegate::SecondaryHoverFontRole, fonts.hoverFont);
        entryItem->setData(1, AgendaEntryDelegate::EntryRole, QVariant::fromValue(phase));
        entryItem->setData(1, AgendaEntryDelegate::EntryDateRole, phase.spanEnd);
        parent->addChild(entryItem);
    }
}


//////////////////////////////////////////////////////////////////////////////
// EventTree

void
EventTree::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &events)
{
    QDate today = selectedDate();
    if (! today.isValid()) {
        return;
    }
    clear();

    Fonts fonts;
    fillFonts(fonts);

    QTreeWidgetItem *eventItem = new QTreeWidgetItem();
    addTopLevelItem(eventItem);
    QFont hlFont(fonts.headlineDefaultFont);
    QString label;
    if (events.count() > 0) {
        label = tr("Upcoming Events");
        QList<QDate> dates = events.keys();
        std::sort(dates.begin(), dates.end());
        for (const QDate &date : dates) {
            addEntries(today, events[date], eventItem, fonts);
        }
    } else {
        hlFont = fonts.headlineSmallEmptyFont;
        label = tr("No upcoming events");
    }
    eventItem->setData(0, Qt::DisplayRole, label);
    eventItem->setData(0, Qt::FontRole, hlFont);
    eventItem->setData(0, AgendaMultiDelegate::TypeRole, 1);
    eventItem->setFirstColumnSpanned(true);

    expandAll();
}


QMenu*
EventTree::createContextMenu
(const QModelIndex &index)
{
    QTreeWidgetItem *item = itemFromIndex(index);
    if (! item) {
        return nullptr;
    }
    QVariant entryData = item->data(1, AgendaEntryDelegate::EntryRole);
    if (entryData.isNull()) {
        return nullptr;
    }
    CalendarEntry entry = entryData.value<CalendarEntry>();
    if (entry.type != ENTRY_TYPE_EVENT) {
        return nullptr;
    }
    QMenu *contextMenu = new QMenu(this);
    contextMenu->addAction(tr("Edit event..."), this, [this, entry]() {
        emit editEventEntry(entry);
    });
    return contextMenu;
}


void
EventTree::addEntries
(const QDate &today, const QList<CalendarEntry> &events, QTreeWidgetItem *parent, const Fonts &fonts)
{
    bool firstEntry = true;
    for (const CalendarEntry &event : events) {
        QTreeWidgetItem *entryItem = new QTreeWidgetItem();
        int diffStart = today.daysTo(event.spanStart);
        QString diffStr;
        if (diffStart == 0) {
            diffStr = tr("today");
        } else if (diffStart == 1) {
            diffStr = tr("tomorrow");
        } else {
            ShowDaysAsUnit unit = showDaysAs(diffStart);
            if (unit == ShowDaysAsUnit::Days) {
                diffStr = tr("in %1 days").arg(diffStart);
            } else if (unit == ShowDaysAsUnit::Weeks) {
                if (diffStart > 1) {
                    diffStr = tr("in %1 weeks").arg(daysToWeeks(diffStart));
                } else {
                    diffStr = tr("in %1 week").arg(daysToWeeks(diffStart));
                }
            } else {
                if (diffStart > 1) {
                    diffStr = tr("in %1 months").arg(daysToMonths(diffStart));
                } else {
                    diffStr = tr("in %1 month").arg(daysToMonths(diffStart));
                }
            }
        }
        if (firstEntry) {
            entryItem->setData(0, Qt::DisplayRole, event.spanStart.toString(tr("ddd, d.M.")));
            entryItem->setData(0, Qt::FontRole, fonts.defaultFont);
            entryItem->setData(0, AgendaMultiDelegate::SecondaryDisplayRole, diffStr);
            entryItem->setData(0, AgendaMultiDelegate::SecondaryFontRole, fonts.relativeFont);
        }
        entryItem->setData(0, AgendaMultiDelegate::HoverTextRole, event.spanStart.toString(tr("ddd, d.M.")));
        entryItem->setData(0, AgendaMultiDelegate::HoverFontRole, fonts.hoverFont);
        entryItem->setData(0, AgendaMultiDelegate::SecondaryHoverTextRole, diffStr);
        entryItem->setData(0, AgendaMultiDelegate::SecondaryHoverFontRole, fonts.hoverFont);
        entryItem->setData(1, AgendaEntryDelegate::EntryRole, QVariant::fromValue(event));
        entryItem->setData(1, AgendaEntryDelegate::EntryDateRole, event.spanStart);
        parent->addChild(entryItem);
        firstEntry = false;
    }
}


//////////////////////////////////////////////////////////////////////////////
// AgendaView

AgendaView::AgendaView
(QWidget *parent)
: QWidget(parent)
{
    QFont baseFont;

    QLabel *headline = new QLabel(tr("Your Training Agenda"));
    headline->setAlignment(Qt::AlignCenter);
    QFont hlFont = headline->font();
    hlFont.setPointSizeF(baseFont.pointSizeF() * 1.4);
    hlFont.setWeight(QFont::DemiBold);
    headline->setFont(hlFont);

    filterLabel = new QLabel("<i>" + tr("Filters applied") + "</i>");
    filterLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QSizePolicy sp = filterLabel->sizePolicy();
    sp.setRetainSizeWhenHidden(true);
    filterLabel->setSizePolicy(sp);

    seasonLabel = new QLabel();
    seasonLabel->setAlignment(Qt::AlignCenter);
    QFont seasonFont = headline->font();
    seasonFont.setPointSize(baseFont.pointSizeF() * 1.1);
    seasonFont.setWeight(QFont::Normal);
    seasonLabel->setFont(seasonFont);

    activityTree = new ActivityTree();
    connect(activityTree, &ActivityTree::dayChanged, [this](const QDate &date) { emit dayChanged(date); });
    connect(activityTree, &ActivityTree::showInTrainMode, [this](const CalendarEntry &activity) { emit showInTrainMode(activity); });
    connect(activityTree, &ActivityTree::viewActivity, [this](const CalendarEntry &activity) { emit viewActivity(activity); });

    phaseTree = new PhaseTree();
    connect(phaseTree, &PhaseTree::dayChanged, [this](const QDate &date) { emit dayChanged(date); });
    connect(phaseTree, &PhaseTree::editPhaseEntry, [this](const CalendarEntry &phase) { emit editPhaseEntry(phase); });

    eventTree = new EventTree();
    connect(eventTree, &EventTree::dayChanged, [this](const QDate &date) { emit dayChanged(date); });
    connect(eventTree, &EventTree::editEventEntry, [this](const CalendarEntry &event) { emit editEventEntry(event); });

    QGridLayout* headLayout = new QGridLayout();
    headLayout->setColumnStretch(0, 1);
    headLayout->setColumnStretch(2, 1);
    headLayout->addWidget(headline, 0, 1, Qt::AlignCenter);
    headLayout->addWidget(filterLabel, 0, 2, Qt::AlignRight | Qt::AlignVCenter);

    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(phaseTree);
    rightLayout->addSpacing(16 * dpiXFactor);
    rightLayout->addWidget(eventTree);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->addSpacing(16 * dpiXFactor);
    contentLayout->addWidget(activityTree, 1);
    contentLayout->addSpacing(16 * dpiXFactor);
    contentLayout->addLayout(rightLayout, 1);
    contentLayout->addSpacing(16 * dpiXFactor);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(headLayout);
    layout->addWidget(seasonLabel);
    layout->addSpacing(16 * dpiYFactor);
    layout->addLayout(contentLayout);
    layout->addSpacing(16 * dpiYFactor);
}


void
AgendaView::updateDate
()
{
    activityTree->updateDate();
    phaseTree->updateDate();
    eventTree->updateDate();
}


void
AgendaView::setPastDays
(int days)
{
    activityTree->setPastDays(days);
}


void
AgendaView::setFutureDays
(int days)
{
    activityTree->setFutureDays(days);
}


void
AgendaView::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activities, std::pair<QList<CalendarEntry>, QList<CalendarEntry>> &phases , const QHash<QDate, QList<CalendarEntry>> &events, const QString &seasonName, bool isFiltered)
{
    if (! seasonName.isNull()) {
        seasonLabel->setText(tr("Season: %1").arg(seasonName));
    } else {
        seasonLabel->setText(tr("No season selected"));
    }
    activityTree->fillEntries(activities);
    phaseTree->fillEntries(phases);
    eventTree->fillEntries(events);
    filterLabel->setVisible(isFiltered);
}


QDate
AgendaView::firstVisibleDay
() const
{
    return activityTree->firstVisibleDay();
}


QDate
AgendaView::lastVisibleDay
() const
{
    return activityTree->lastVisibleDay();
}


QDate
AgendaView::selectedDate
() const
{
    return activityTree->selectedDate();
}


void
AgendaView::setActivityMaxTertiaryLines
(int maxTertiaryLines)
{
    activityTree->setMaxTertiaryLines(maxTertiaryLines);
}


void
AgendaView::setEventMaxTertiaryLines
(int maxTertiaryLines)
{
    eventTree->setMaxTertiaryLines(maxTertiaryLines);
}


void
AgendaView::changeEvent
(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange) {
        QTimer::singleShot(0, this, [this]() {
            QPalette parentPal = palette();
            parentPal.setColor(QPalette::Window, parentPal.color(QPalette::AlternateBase));
            setAutoFillBackground(true);
            setPalette(parentPal);
        });
    }
    QWidget::changeEvent(event);
}
