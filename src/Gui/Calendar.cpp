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

#include "Calendar.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QScrollArea>
#include <QEvent>
#include <QMouseEvent>
#include <QToolBar>
#include <QDialog>
#include <QDialogButtonBox>
#include <QActionGroup>
#include <QHash>
#include <QDrag>
#include <QMimeData>
#include <QDebug>
#if QT_VERSION < 0x060000
#include <QAbstractItemDelegate>
#endif

#include "Colors.h"
#include "Settings.h"
#include "Context.h"


//////////////////////////////////////////////////////////////////////////////
// CalendarOverview

CalendarOverview::CalendarOverview
(QWidget *parent)
: QCalendarWidget(parent)
{
    setGridVisible(false);
    setNavigationBarVisible(false);
    setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
}


QDate
CalendarOverview::firstVisibleDay
() const
{
    int year = yearShown();
    int month = monthShown();

    QDate firstOfMonth(year, month, 1);
    int dayOfWeek = firstOfMonth.dayOfWeek();
    int offset = (dayOfWeek - firstDayOfWeek() + 7) % 7 + 7;
    QDate firstVisibleDate = firstOfMonth.addDays(-offset);
    return firstVisibleDate;
}


QDate
CalendarOverview::lastVisibleDay
() const
{
    return firstVisibleDay().addDays(48);
}


void
CalendarOverview::limitDateRange
(const DateRange &dr)
{
    setMinimumDate(dr.from);
    setMaximumDate(dr.to);
}


void
CalendarOverview::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries)
{
    this->activityEntries = activityEntries;
    this->headlineEntries = headlineEntries;
}


void
CalendarOverview::paintCell
#if QT_VERSION < 0x060000
(QPainter *painter, const QRect &rect, const QDate &date) const
#else
(QPainter *painter, const QRect &rect, QDate date) const
#endif
{
    QCalendarWidget::paintCell(painter, rect, date);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    int w = 10 * dpiXFactor;
    int r = 2 * dpiXFactor;
    int maxEntries = (rect.width() - 2 * r) / (w + r);

    QPolygon triangle;
    triangle << QPoint(w / 2, w / 1.4)
             << QPoint(0, 0)
             << QPoint(w, 0);
    triangle.translate(rect.x() + r, rect.y() + r);
    drawEntries(painter, headlineEntries.value(date), triangle, maxEntries, w + r);

    triangle.clear();
    triangle << QPoint(w / 2, 0)
             << QPoint(0, w / 1.4)
             << QPoint(w, w / 1.4);
    triangle.translate(rect.x() + r, rect.y() + rect.height() - w);
    drawEntries(painter, activityEntries.value(date), triangle, maxEntries, w + r);

    painter->restore();
}


void
CalendarOverview::drawEntries
(QPainter *painter, const QList<CalendarEntry> &entries, QPolygon polygon, int maxEntries, int shiftX) const
{
    painter->save();
    int i = 0;
    for (const CalendarEntry &entry : entries) {
        painter->setPen(entry.color);
        if (entry.type == ENTRY_TYPE_PLANNED_ACTIVITY) {
            painter->setBrush(Qt::transparent);
        } else {
            painter->setBrush(entry.color);
        }
        painter->drawPolygon(polygon);
        polygon.translate(shiftX, 0);
        if (++i >= maxEntries) {
            break;
        }
    }
    painter->restore();
}


//////////////////////////////////////////////////////////////////////////////
// CalendarDayTable

CalendarDayTable::CalendarDayTable
(const QDate &date, CalendarDayTableType type, Qt::DayOfWeek firstDayOfWeek, QWidget *parent)
: QTableWidget(parent), type(type)
{
    int numDays = type == CalendarDayTableType::Day ? 1 : 7;
    dragTimer.setSingleShot(true);
    setAcceptDrops(true);
    setColumnCount(numDays + 1);
    setRowCount(3);
    setFrameShape(QFrame::NoFrame);

    QList<QStyledItemDelegate*> row0Delegates;
    row0Delegates << new CalendarTimeScaleDelegate(&timeScaleData, this);
    for (int i = 0; i < numDays; ++i) {
        row0Delegates << new CalendarDetailedDayDelegate(&timeScaleData, this);
    }
    setItemDelegateForRow(0, new CalendarHeadlineDelegate(this));
    setItemDelegateForRow(1, new ColumnDelegatingItemDelegate(row0Delegates, this));
    setItemDelegateForRow(2, new CalendarSummaryDelegate(numDays > 1 ? 4 : 20, this));

    horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    for (int i = 0; i < numDays; ++i) {
        horizontalHeader()->setSectionResizeMode(i + 1, QHeaderView::Stretch);
    }
    horizontalHeader()->setVisible(false);
    verticalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    verticalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    verticalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    verticalHeader()->setVisible(false);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &CalendarDayTable::customContextMenuRequested, this, &CalendarDayTable::showContextMenu);

    setFirstDayOfWeek(firstDayOfWeek);
    setDay(date);
}


bool
CalendarDayTable::setDay
(const QDate &date)
{
    if (! isInDateRange(date)) {
        return false;
    }
    this->date = date;
    clearContents();
    QTableWidgetItem *timeScaleItem = new QTableWidgetItem();
    timeScaleItem->setData(CalendarTimeScaleDelegate::CurrentYRole, -1);
    setItem(1, 0, timeScaleItem);
    if (type == CalendarDayTableType::Day) {
        QTableWidgetItem *headlineItem = new QTableWidgetItem();
        CalendarDay headlineDay;
        headlineDay.date = date;
        headlineDay.isDimmed = DayDimLevel::None;
        headlineItem->setData(CalendarHeadlineDelegate::DayRole, QVariant::fromValue(headlineDay));
        setItem(0, 1, headlineItem);

        QTableWidgetItem *item = new QTableWidgetItem();
        CalendarDay day;
        day.date = date;
        day.isDimmed = DayDimLevel::None;
        item->setData(CalendarDetailedDayDelegate::DayRole, QVariant::fromValue(day));
        setItem(1, 1, item);
    } else {
        QDate dayDate = firstVisibleDay();
        for (int i = 0; i < 7; ++i) {
            QTableWidgetItem *headlineItem = new QTableWidgetItem();
            CalendarDay headlineDay;
            headlineDay.date = dayDate;
            headlineDay.isDimmed = DayDimLevel::None;
            headlineItem->setData(CalendarHeadlineDelegate::DayRole, QVariant::fromValue(headlineDay));
            setItem(0, i + 1, headlineItem);

            QTableWidgetItem *item = new QTableWidgetItem();
            CalendarDay day;
            day.date = dayDate;
            day.isDimmed = dr.pass(dayDate) ? DayDimLevel::None : DayDimLevel::Full;
            item->setData(CalendarDetailedDayDelegate::DayRole, QVariant::fromValue(day));
            setItem(1, i + 1, item);
            dayDate = dayDate.addDays(1);
        }
    }
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    emit dayChanged(date);

    return true;
}


QDate
CalendarDayTable::firstVisibleDay
() const
{
    return firstVisibleDay(date);
}


QDate
CalendarDayTable::firstVisibleDay
(const QDate &d) const
{
    if (type == CalendarDayTableType::Day) {
        return d;
    } else {
        int currentDayOfWeek = d.dayOfWeek();
        int offset = currentDayOfWeek - firstDayOfWeek;
        if (offset < 0) {
            offset += 7;
        }
        return d.addDays(-offset);
    }
}


QDate
CalendarDayTable::lastVisibleDay
() const
{
    return lastVisibleDay(date);
}


QDate
CalendarDayTable::lastVisibleDay
(const QDate &d) const
{
    if (type == CalendarDayTableType::Day) {
        return d;
    } else {
        return firstVisibleDay(d).addDays(6);
    }
}


QDate
CalendarDayTable::selectedDate
() const
{
    return date;
}


bool
CalendarDayTable::isInDateRange
(const QDate &date) const
{
    return date.isValid() && dr.pass(date);
}


void
CalendarDayTable::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries)
{
    int startHour = defaultStartHour;
    int endHour = defaultEndHour;
    int numDays = type == CalendarDayTableType::Day ? 1 : 7;
    for (int i = 0; i < numDays; ++i) {
        QTableWidgetItem *headlineItem = this->item(0, i + 1);
        CalendarDay headlineDay = headlineItem->data(CalendarHeadlineDelegate::DayRole).value<CalendarDay>();
        headlineDay.entries.clear();
        headlineDay.headlineEntries = headlineEntries.value(headlineDay.date);
        headlineItem->setData(CalendarHeadlineDelegate::DayRole, QVariant::fromValue(headlineDay));

        QTableWidgetItem *item = this->item(1, i + 1);
        CalendarDay day = item->data(CalendarDetailedDayDelegate::DayRole).value<CalendarDay>();
        day.entries = activityEntries.value(day.date);
        day.headlineEntries.clear();
        item->setData(CalendarDetailedDayDelegate::DayRole, QVariant::fromValue(day));

        for (const CalendarEntry &entry : day.entries) {
            startHour = std::min(startHour, entry.start.hour());
            QTime endHourTime = entry.start.addSecs(entry.durationSecs);
            if (endHourTime < entry.start) {
                endHour = 24;
            } else {
                endHour = std::max(endHour, endHourTime.hour() + 1);
            }
        }
        endHour = std::min(24, endHour);

        CalendarEntryLayouter layouter;
        QList<CalendarEntryLayout> layout = layouter.layout(day.entries);
        item->setData(CalendarDetailedDayDelegate::LayoutRole, QVariant::fromValue(layout));

        QTableWidgetItem *summaryItem = new QTableWidgetItem();
        summaryItem->setData(CalendarSummaryDelegate::SummaryRole, QVariant::fromValue(summaries.value(i)));
        summaryItem->setFlags(Qt::ItemIsEnabled);
        setItem(2, i + 1, summaryItem);
    }
    timeScaleData.setFirstMinute(startHour * 60);
    timeScaleData.setLastMinute(endHour * 60);
}


void
CalendarDayTable::limitDateRange
(const DateRange &dr, bool canHavePhasesOrEvents)
{
    if (dr.from.isValid() && dr.to.isValid() && dr.from > dr.to) {
        return;
    }
    this->canHavePhasesOrEvents = canHavePhasesOrEvents;
    this->dr = dr;
    if (! dr.pass(selectedDate())) {
        if (dr.pass(lastVisibleDay())) {
            setDay(lastVisibleDay());
        } else if (dr.pass(firstVisibleDay())) {
            setDay(firstVisibleDay());
        } else if (dr.pass(QDate::currentDate())) {
            setDay(QDate::currentDate());
        } else if (dr.to.isValid() && dr.to < QDate::currentDate()) {
            setDay(dr.to);
        } else if (dr.from.isValid() && dr.from > QDate::currentDate()) {
            setDay(dr.from);
        } else if (dr.to.isValid()) {
            setDay(dr.to);
        } else {
            setDay(dr.from);
        }
    }
}


void
CalendarDayTable::setFirstDayOfWeek
(Qt::DayOfWeek firstDayOfWeek)
{
    this->firstDayOfWeek = firstDayOfWeek;
    if (type == CalendarDayTableType::Week) {
        setDay(selectedDate());
    }
}


void
CalendarDayTable::setStartHour
(int hour)
{
    defaultStartHour = hour;
}


void
CalendarDayTable::setEndHour
(int hour)
{
    defaultEndHour = hour;
}


void
CalendarDayTable::changeEvent
(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange) {
        horizontalHeader()->setStyleSheet(QString("QHeaderView::section { border: none; background-color: %1; color: %2 }")
                                                 .arg(palette().color(QPalette::Active, QPalette::Window).name())
                                                 .arg(palette().color(QPalette::Active, QPalette::WindowText).name()));
    }
    QTableWidget::changeEvent(event);
}


void
CalendarDayTable::mouseDoubleClickEvent
(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);

    int row = index.row();
    int column = index.column();
    if (row == 1 && column > 0) {
        ColumnDelegatingItemDelegate *delegatingDelegate = static_cast<ColumnDelegatingItemDelegate*>(itemDelegateForRow(row));
        CalendarDetailedDayDelegate *delegate = static_cast<CalendarDetailedDayDelegate*>(delegatingDelegate->getDelegate(column));

        CalendarDay day = index.data(CalendarDetailedDayDelegate::DayRole).value<CalendarDay>();
        int entryIdx = delegate->entryTester.hitTest(index, event->pos());
        if (entryIdx >= 0) {
            emit viewActivity(day.entries[entryIdx]);
        } else {
            QTime time = timeScaleData.timeFromYInTable(pos.y(), visualRect(index));
            bool past =    day.date < QDate::currentDate()
                        || (   day.date == QDate::currentDate()
                            && time < QTime::currentTime());
            emit addActivity(! past, day.date, time);
        }
    }
}


void
CalendarDayTable::mousePressEvent
(QMouseEvent *event)
{
    isDraggable = false;
    pressedPos = event->pos();
    pressedIndex = indexAt(pressedPos);
    if (pressedIndex.row() == 1 && pressedIndex.column() > 0) {
        ColumnDelegatingItemDelegate *delegatingDelegate = static_cast<ColumnDelegatingItemDelegate*>(itemDelegateForRow(pressedIndex.row()));
        CalendarDetailedDayDelegate *delegate = static_cast<CalendarDetailedDayDelegate*>(delegatingDelegate->getDelegate(pressedIndex.column()));
        int entryIdx = delegate->entryTester.hitTest(pressedIndex, pressedPos);
        if (pressedIndex.isValid()) {
            QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
            if (item != nullptr) {
                item->setData(CalendarDetailedDayDelegate::PressedEntryRole, entryIdx);
            }
        }
        CalendarDay day = pressedIndex.data(CalendarDetailedDayDelegate::DayRole).value<CalendarDay>();
        if (entryIdx >= 0) {
            CalendarEntry calEntry = day.entries[entryIdx];
            isDraggable = day.entries[entryIdx].isRelocatable;
            if (event->button() == Qt::LeftButton && isDraggable) {
                dragTimer.start(QApplication::startDragTime());
            }
        }
    }
}


void
CalendarDayTable::mouseReleaseEvent
(QMouseEvent *event)
{
    if (dragTimer.isActive()) {
        dragTimer.stop();
    }
    if (pressedIndex.isValid()) {
        QPoint pos = event->pos();
        QModelIndex releasedIndex = indexAt(pos);
        if (releasedIndex == pressedIndex) {
            if (pressedIndex.row() == 1 && pressedIndex.column() > 0) {
                QTime time = timeScaleData.timeFromYInTable(pos.y(), visualRect(releasedIndex));
                CalendarDay day = pressedIndex.data(CalendarDetailedDayDelegate::DayRole).value<CalendarDay>();
                ColumnDelegatingItemDelegate *delegatingDelegate = static_cast<ColumnDelegatingItemDelegate*>(itemDelegateForRow(pressedIndex.row()));
                CalendarDetailedDayDelegate *delegate = static_cast<CalendarDetailedDayDelegate*>(delegatingDelegate->getDelegate(pressedIndex.column()));
                int entryIdx = delegate->entryTester.hitTest(pressedIndex, pressedPos);
                if (event->button() == Qt::LeftButton) {
                    if (entryIdx >= 0) {
                        emit entryClicked(day, entryIdx);
                    } else {
                        emit dayClicked(day, time);
                    }
                } else if (event->button() == Qt::RightButton) {
                    if (entryIdx >= 0) {
                        emit entryRightClicked(day, entryIdx);
                    } else {
                        emit dayRightClicked(day, time);
                    }
                }
            }
        }
        if (pressedIndex.isValid() && pressedIndex.row() == 1) {
            QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
            if (item != nullptr) {
                item->setData(CalendarDetailedDayDelegate::PressedEntryRole, QVariant());
            }
        }
        pressedPos = QPoint();
        pressedIndex = QModelIndex();
    }
}


void
CalendarDayTable::mouseMoveEvent
(QMouseEvent *event)
{
    if (   ! (event->buttons() & Qt::LeftButton)
        || ! isDraggable
        || ! pressedIndex.isValid()
        || dragTimer.isActive()
        || (event->pos() - pressedPos).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }
    int row = pressedIndex.row();
    int column = pressedIndex.column();
    if (row != 1 || column == 0) {
        return;
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData();

    ColumnDelegatingItemDelegate *delegatingDelegate = static_cast<ColumnDelegatingItemDelegate*>(itemDelegateForRow(row));
    CalendarDetailedDayDelegate *delegate = static_cast<CalendarDetailedDayDelegate*>(delegatingDelegate->getDelegate(column));
    int entryIdx = delegate->entryTester.hitTest(pressedIndex, pressedPos);

    CalendarEntry calEntry = pressedIndex.data(CalendarDetailedDayDelegate::DayRole).value<CalendarDay>().entries[entryIdx];
    QSize pixmapSize(40 * dpiXFactor, 40 * dpiYFactor);
    QPixmap pixmap = svgAsColoredPixmap(calEntry.iconFile, pixmapSize, 0, calEntry.color);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmapSize.width() / 2, pixmapSize.height() / 2));

    QList<int> entryCoords = { row, column, entryIdx };
    QString entryStr = QString::fromStdString(std::accumulate(entryCoords.begin(),
                                                              entryCoords.end(),
                                                              std::string(),
                                                              [](const std::string &a, int b) {
                                                                  return a + (a.length() ? "," : "") + std::to_string(b);
                                                              }));

    QByteArray entryBytes = entryStr.toUtf8();
    mimeData->setData("application/x-hour-grid-entry", entryBytes);
    drag->setMimeData(mimeData);
    drag->exec(Qt::MoveAction);
    QTableWidgetItem *item = this->item(row, column);
    item->setData(CalendarDetailedDayDelegate::PressedEntryRole, QVariant());
    pressedPos = QPoint();
    pressedIndex = QModelIndex();
}


void
CalendarDayTable::dragEnterEvent
(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-hour-grid-entry") && event->source() == this) {
        event->acceptProposedAction();
    }
}


void
CalendarDayTable::dragMoveEvent
(QDragMoveEvent *event)
{
#if QT_VERSION < 0x060000
    QPoint pos = event->pos();
#else
    QPoint pos = event->position().toPoint();
#endif
    QModelIndex hoverIndex = indexAt(pos);
    if (   hoverIndex.isValid()
        && hoverIndex.column() > 0
        && hoverIndex.row() == 1) {
        CalendarDay day = hoverIndex.data(CalendarDetailedDayDelegate::DayRole).value<CalendarDay>();
        QRect indexRect = visualRect(hoverIndex);
        QTime time = timeScaleData.timeFromYInTable(pos.y(), indexRect);
        bool past =    day.date < QDate::currentDate()
                    || (   day.date == QDate::currentDate()
                        && time < QTime::currentTime());
        bool conflict = false;
        BlockIndicator blockIndicator = BlockIndicator::NoBlock;
        if (day.date < QDate::currentDate()) {
            blockIndicator = BlockIndicator::AllBlock;
        } else if (day.date == QDate::currentDate()) {
            blockIndicator = BlockIndicator::BlockBeforeNow;
        }
        setDropIndicator(pos.y(), blockIndicator);
        for (const CalendarEntry &entry : day.entries) {
            if (entry.start == time) {
                conflict = true;
                break;
            }
        }
        if (past || conflict) {
            event->ignore();
            return;
        }
        event->accept();
    } else {
        setDropIndicator(-1, BlockIndicator::NoBlock);
        event->ignore();
    }
}


void
CalendarDayTable::dragLeaveEvent
(QDragLeaveEvent *event)
{
    Q_UNUSED(event)

    setDropIndicator(-1, BlockIndicator::NoBlock);
}


void
CalendarDayTable::dropEvent
(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-hour-grid-entry")) {
        setDropIndicator(-1, BlockIndicator::NoBlock);

        QByteArray entryBytes = event->mimeData()->data("application/x-hour-grid-entry");
        QString entryStr = QString::fromUtf8(entryBytes);
        QStringList entryStrList = entryStr.split(',');
        QList<int> entryCoords;
        for (const QString &str : entryStrList) {
            entryCoords.append(str.toInt());
        }
        if (entryCoords.size() != 3) {
            return;
        }
        if (   entryCoords[0] == 1
            && entryCoords[1] > 0
            && entryCoords[1] < columnCount()) {
            QModelIndex srcIndex = model()->index(entryCoords[0], entryCoords[1]);
            int entryIdx = entryCoords[2];
            if (srcIndex.isValid() && entryIdx >= 0) {
                CalendarDay srcDay = srcIndex.data(CalendarDetailedDayDelegate::DayRole).value<CalendarDay>();
                if (entryIdx < srcDay.entries.count()) {
#if QT_VERSION < 0x060000
                    QPoint pos = event->pos();
#else
                    QPoint pos = event->position().toPoint();
#endif
                    QModelIndex destIndex = indexAt(pos);
                    QTime time = timeScaleData.timeFromYInTable(pos.y(), visualRect(destIndex));
                    CalendarDay destDay = destIndex.data(CalendarDetailedDayDelegate::DayRole).value<CalendarDay>();
                    emit entryMoved(srcDay.entries[entryIdx], srcDay.date, destDay.date, time);
                }
            }
        }
    }
}


void
CalendarDayTable::showContextMenu
(const QPoint &pos)
{
    QModelIndex index = indexAt(pos);
    if (! index.isValid()) {
        return;
    }
    int row = index.row();
    int column = index.column();
    if (pressedIndex.isValid() && (row != 1 || column == 0)) {
        QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
        if (item != nullptr) {
            item->setData(CalendarDetailedDayDelegate::PressedEntryRole, QVariant());
        }
    }

    QMenu *contextMenu = nullptr;
    if (row == 0 && column > 0) {
        contextMenu = makeHeaderMenu(index, pos);
    } else if (row == 1 && column > 0) {
        contextMenu = makeActivityMenu(index, pos);
    }
    if (contextMenu != nullptr) {
        contextMenu->exec(viewport()->mapToGlobal(pos));
        delete contextMenu;
        if (pressedIndex.isValid()) {
            QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
            if (item != nullptr) {
                item->setData(CalendarDetailedDayDelegate::PressedEntryRole, QVariant());
            }
        }
    }
}


void
CalendarDayTable::setDropIndicator
(int y, BlockIndicator block)
{
    QTableWidgetItem *scaleItem = item(1, 0);
    scaleItem->setData(CalendarTimeScaleDelegate::CurrentYRole, y);
    scaleItem->setData(CalendarTimeScaleDelegate::BlockRole, static_cast<int>(block));
}


QMenu*
CalendarDayTable::makeHeaderMenu
(const QModelIndex &index, const QPoint &pos)
{
    CalendarHeadlineDelegate *delegate = static_cast<CalendarHeadlineDelegate*>(itemDelegateForRow(index.row()));
    int entryIdx = delegate->headlineTester.hitTest(index, pos);
    CalendarDay day = index.data(CalendarHeadlineDelegate::DayRole).value<CalendarDay>();
    QMenu *contextMenu = new QMenu(this);
    if (entryIdx >= 0) {
        const CalendarEntry &entry = day.headlineEntries[entryIdx];
        switch (entry.type) {
        case ENTRY_TYPE_EVENT: {
            QAction *editEventAction = contextMenu->addAction(tr("Edit event..."), this, [=]() { emit editEvent(entry); });
            QAction *delEventAction = contextMenu->addAction(tr("Delete event"), this, [=]() { emit delEvent(entry); });
            if (! isInDateRange(day.date) || ! canHavePhasesOrEvents) {
                editEventAction->setEnabled(false);
                delEventAction->setEnabled(false);
            }
            break;
        }
        case ENTRY_TYPE_PHASE: {
            QAction *editPhaseAction = contextMenu->addAction(tr("Edit phase..."), this, [=]() { emit editPhase(entry); });
            QAction *delPhaseAction = contextMenu->addAction(tr("Delete phase..."), this, [=]() { emit delPhase(entry); });
            if (! isInDateRange(day.date) || ! canHavePhasesOrEvents) {
                editPhaseAction->setEnabled(false);
                delPhaseAction->setEnabled(false);
            }
            break;
        }
        default:
            break;
        }
    } else {
        QAction *addPhaseAction = contextMenu->addAction(tr("Add phase..."), this, [=]() { emit addPhase(day.date); });
        QAction *addEventAction = contextMenu->addAction(tr("Add event..."), this, [=]() { emit addEvent(day.date); });
        if (! isInDateRange(day.date) || ! canHavePhasesOrEvents) {
            addPhaseAction->setEnabled(false);
            addEventAction->setEnabled(false);
        }
    }
    return contextMenu;
}


QMenu*
CalendarDayTable::makeActivityMenu
(const QModelIndex &index, const QPoint &pos)
{
    ColumnDelegatingItemDelegate *delegatingDelegate = static_cast<ColumnDelegatingItemDelegate*>(itemDelegateForRow(index.row()));
    CalendarDetailedDayDelegate *delegate = static_cast<CalendarDetailedDayDelegate*>(delegatingDelegate->getDelegate(index.column()));
    int entryIdx = delegate->entryTester.hitTest(index, pos);
    CalendarDay day = index.data(CalendarDetailedDayDelegate::DayRole).value<CalendarDay>();
    QMenu *contextMenu = new QMenu(this);
    if (entryIdx >= 0) {
        CalendarEntry calEntry = day.entries[entryIdx];
        switch (calEntry.type) {
        case ENTRY_TYPE_ACTIVITY:
            contextMenu->addAction(tr("View activity..."), this, [=]() {
                emit viewActivity(calEntry);
            });
            contextMenu->addAction(tr("Delete activity"), this, [=]() {
                emit delActivity(calEntry);
            });
            break;
        case ENTRY_TYPE_PLANNED_ACTIVITY:
            if (calEntry.hasTrainMode) {
                contextMenu->addAction(tr("Show in train node..."), this, [=]() {
                    emit showInTrainMode(calEntry);
                });
            }
            contextMenu->addAction(tr("View planned activity..."), this, [=]() {
                emit viewActivity(calEntry);
            });
            contextMenu->addAction(tr("Delete planned activity"), this, [=]() {
                emit delActivity(calEntry);
            });
            break;
        default:
            break;
        }
    } else {
        QTime time = timeScaleData.timeFromYInTable(pos.y(), visualRect(index));
        if (   day.date < QDate::currentDate()
            || (   day.date == QDate::currentDate()
                && time < QTime::currentTime())) {
            contextMenu->addAction(tr("Add activity..."), this, [=]() {
                emit addActivity(false, day.date, time);
            });
        } else {
            contextMenu->addAction(tr("Add planned activity..."), this, [=]() {
                emit addActivity(true, day.date, time);
            });
        }
        QAction *addPhaseAction = contextMenu->addAction(tr("Add phase..."), this, [=]() { emit addPhase(day.date); });
        QAction *addEventAction = contextMenu->addAction(tr("Add event..."), this, [=]() { emit addEvent(day.date); });
        if (! isInDateRange(day.date) || ! canHavePhasesOrEvents) {
            addPhaseAction->setEnabled(false);
            addEventAction->setEnabled(false);
        }
    }
    return contextMenu;
}


//////////////////////////////////////////////////////////////////////////////
// CalendarMonthTable

CalendarMonthTable::CalendarMonthTable
(Qt::DayOfWeek firstDayOfWeek, QWidget *parent)
: CalendarMonthTable(QDate::currentDate(), firstDayOfWeek, parent)
{
}


CalendarMonthTable::CalendarMonthTable
(const QDate &dateInMonth, Qt::DayOfWeek firstDayOfWeek, QWidget *parent)
: QTableWidget(parent)
{
    dragTimer.setSingleShot(true);
    setAcceptDrops(true);
    setColumnCount(8);
    setFrameShape(QFrame::NoFrame);
    setItemDelegateForColumn(7, new CalendarSummaryDelegate(4, this));
    for (int i = 0; i < 7; ++i) {
        setItemDelegateForColumn(i, new CalendarCompactDayDelegate(this));
    }

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &CalendarMonthTable::customContextMenuRequested, this, &CalendarMonthTable::showContextMenu);
    connect(this, &QTableWidget::itemSelectionChanged, [=]() {
        QList<QTableWidgetItem*> selection = selectedItems();
        if (selection.count() > 0) {
            QTableWidgetItem *item = selection[0];
            if (item->column() < 7) {
                emit daySelected(item->data(CalendarCompactDayDelegate::DayRole).value<CalendarDay>());
            }
        }
    });
    setFirstDayOfWeek(firstDayOfWeek);
    setMonth(dateInMonth);
}


bool
CalendarMonthTable::selectDay
(const QDate &day)
{
    if (day < startDate || day > endDate) {
        return false;
    }
    int daysAfterFirst = startDate.daysTo(day);
    int row = daysAfterFirst / 7;
    int col = daysAfterFirst % 7;
    setCurrentCell(row, col);
    return true;
}


bool
CalendarMonthTable::setMonth
(const QDate &dateInMonth, bool allowKeepMonth)
{
    if (   ! dateInMonth.isValid()
        || (   ! isInDateRange(dateInMonth)
            && ! allowKeepMonth)) {
        return false;
    }
    if (! (allowKeepMonth && dateInMonth >= startDate && dateInMonth <= endDate) || ! firstOfMonth.isValid()) {
        firstOfMonth = QDate(dateInMonth.year(), dateInMonth.month(), 1);
    }
    clearContents();

    int startDayOfWeek = firstOfMonth.dayOfWeek();
    int offset = (startDayOfWeek - firstDayOfWeek + 7) % 7;
    startDate = firstOfMonth.addDays(-offset);
    int daysInMonth = dateInMonth.daysInMonth();
    int totalDisplayedDays = offset + daysInMonth;
    int totalRows = (totalDisplayedDays + 6) / 7;
    endDate = startDate.addDays(totalRows * 7 - 1);

    setRowCount(totalRows);
    for (int i = 0; i < totalRows * 7; ++i) {
        QDate date = startDate.addDays(i);
        int row = i / 7;
        int col = (date.dayOfWeek() - firstDayOfWeek + 7) % 7;
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setData(DateRole, date);
        DayDimLevel isDimmed = DayDimLevel::None;
        if (! dr.pass(date)) {
            isDimmed = DayDimLevel::Full;
        } else if (date.month() != dateInMonth.month()) {
            isDimmed = DayDimLevel::Partial;
        }
        CalendarDay day;
        day.date = date;
        day.isDimmed = isDimmed;
        item->setData(CalendarCompactDayDelegate::DayRole, QVariant::fromValue(day));

        setItem(row, col, item);
    }

    for (int i = 0; i < 7; ++i) {
        horizontalHeader()->setSectionResizeMode(i, QHeaderView::Stretch);
    }
    horizontalHeader()->setSectionResizeMode(7, QHeaderView::Stretch);
    verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    selectDay(dateInMonth);

    emit monthChanged(firstOfMonth, startDate, endDate);

    return true;
}


bool
CalendarMonthTable::isInDateRange
(const QDate &date) const
{
    return date.isValid() && dr.pass(date);
}


void
CalendarMonthTable::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries)
{
    for (int i = 0; i < rowCount() * 7; ++i) {
        QDate date = startDate.addDays(i);
        int row = i / 7;
        int col = (date.dayOfWeek() - firstDayOfWeek + 7) % 7;
        QTableWidgetItem *item = this->item(row, col);
        CalendarDay day = item->data(CalendarCompactDayDelegate::DayRole).value<CalendarDay>();
        if (activityEntries.contains(date)) {
            day.entries = activityEntries[date];
        } else {
            day.entries.clear();
        }
        if (headlineEntries.contains(date)) {
            day.headlineEntries = headlineEntries[date];
        } else {
            day.headlineEntries.clear();
        }
        item->setData(CalendarCompactDayDelegate::DayRole, QVariant::fromValue(day));
    }

    for (int row = 0; row < rowCount() && row < summaries.count(); ++row) {
        QTableWidgetItem *summaryItem = new QTableWidgetItem();
        summaryItem->setData(CalendarSummaryDelegate::SummaryRole, QVariant::fromValue(summaries[row]));
        summaryItem->setFlags(Qt::ItemIsEnabled);
        setItem(row, 7, summaryItem);
    }
}


QDate
CalendarMonthTable::firstOfCurrentMonth
() const
{
    return firstOfMonth;
}


QDate
CalendarMonthTable::firstVisibleDay
() const
{
    return startDate;
}


QDate
CalendarMonthTable::lastVisibleDay
() const
{
    return endDate;
}


QDate
CalendarMonthTable::selectedDate
() const
{
    QTableWidgetItem *item = currentItem();
    if (item != nullptr) {
        return item->data(DateRole).toDate();
    }
    return firstOfMonth;
}


void
CalendarMonthTable::limitDateRange
(const DateRange &dr, bool allowKeepMonth, bool canHavePhasesOrEvents)
{
    if (dr.from.isValid() && dr.to.isValid() && dr.from > dr.to) {
        return;
    }
    this->dr = dr;
    this->canHavePhasesOrEvents = canHavePhasesOrEvents;
    if (currentItem() != nullptr && isInDateRange(currentItem()->data(DateRole).toDate())) {
        setMonth(currentItem()->data(DateRole).toDate());
    } else if (isInDateRange(QDate::currentDate())) {
        setMonth(QDate::currentDate());
    } else if (isInDateRange(firstOfMonth)) {
        setMonth(firstOfMonth, allowKeepMonth);
    } else if (dr.to.isValid() && dr.to < QDate::currentDate()) {
        setMonth(dr.to);
    } else if (dr.from.isValid() && dr.from > QDate::currentDate()) {
        setMonth(dr.from);
    } else if (dr.to.isValid()) {
        setMonth(dr.to);
    } else {
        setMonth(dr.from);
    }
}


void
CalendarMonthTable::setFirstDayOfWeek
(Qt::DayOfWeek firstDayOfWeek)
{
    clear();
    QLocale locale;
    QStringList headers;
    this->firstDayOfWeek = firstDayOfWeek;
    for (int i = Qt::Monday - 1; i < Qt::Sunday; ++i) {
        headers << locale.dayName((i + firstDayOfWeek - 1) % 7 + 1, QLocale::ShortFormat);
    }
    headers << tr("Summary");
    setHorizontalHeaderLabels(headers);
    verticalHeader()->setVisible(false);
}


void
CalendarMonthTable::changeEvent
(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange) {
        horizontalHeader()->setStyleSheet(QString("QHeaderView::section { border: none; background-color: %1; color: %2 }")
                                                 .arg(palette().color(QPalette::Active, QPalette::Window).name())
                                                 .arg(palette().color(QPalette::Active, QPalette::WindowText).name()));
    }
    QTableWidget::changeEvent(event);
}


void
CalendarMonthTable::mouseDoubleClickEvent
(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    QPoint pos = event->pos();
    QModelIndex index = indexAt(pos);

    int column = index.column();
    if (column < 7) {
        QAbstractItemDelegate *abstractDelegate = itemDelegateForIndex(index);
        CalendarCompactDayDelegate *delegate = static_cast<CalendarCompactDayDelegate*>(abstractDelegate);
        CalendarDay day = index.data(CalendarCompactDayDelegate::DayRole).value<CalendarDay>();
        if (delegate->moreTester.hitTest(index, event->pos()) != -1) {
            emit moreDblClicked(day);
        } else {
            int entryIdx = delegate->entryTester.hitTest(index, event->pos());
            if (entryIdx >= 0) {
                emit entryDblClicked(day, entryIdx);
            } else {
                emit dayDblClicked(day);
            }
        }
    } else {
        emit summaryDblClicked(index);
    }
}


void
CalendarMonthTable::mousePressEvent
(QMouseEvent *event)
{
    isDraggable = false;
    pressedPos = event->pos();
    pressedIndex = indexAt(pressedPos);
    QTableWidget::mousePressEvent(event);
    if (pressedIndex.column() < 7) {
        QAbstractItemDelegate *abstractDelegate = itemDelegateForIndex(pressedIndex);
        CalendarCompactDayDelegate *delegate = static_cast<CalendarCompactDayDelegate*>(abstractDelegate);
        if (delegate->moreTester.hitTest(pressedIndex, pressedPos) == -1) {
            int entryIdx = delegate->entryTester.hitTest(pressedIndex, pressedPos);
            if (pressedIndex.isValid()) {
                QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
                if (item != nullptr) {
                    item->setData(CalendarCompactDayDelegate::PressedEntryRole, entryIdx);
                }
            }
            CalendarDay day = pressedIndex.data(CalendarCompactDayDelegate::DayRole).value<CalendarDay>();
            if (entryIdx >= 0) {
                CalendarEntry calEntry = day.entries[entryIdx];
                isDraggable = day.entries[entryIdx].isRelocatable;
                if (event->button() == Qt::LeftButton && isDraggable) {
                    dragTimer.start(QApplication::startDragTime());
                }
            }
        }
    }
}


void
CalendarMonthTable::mouseReleaseEvent
(QMouseEvent *event)
{
    if (dragTimer.isActive()) {
        dragTimer.stop();
    }
    if (pressedIndex.isValid()) {
        QPoint pos = event->pos();
        QModelIndex releasedIndex = indexAt(pos);
        if (releasedIndex == pressedIndex) {
            if (pressedIndex.column() < 7) {
                CalendarDay day = pressedIndex.data(CalendarCompactDayDelegate::DayRole).value<CalendarDay>();
                QAbstractItemDelegate *abstractDelegate = itemDelegateForIndex(releasedIndex);
                CalendarCompactDayDelegate *delegate = static_cast<CalendarCompactDayDelegate*>(abstractDelegate);
                if (delegate->moreTester.hitTest(releasedIndex, event->pos()) != -1) {
                    emit moreClicked(day);
                } else {
                    int entryIdx = delegate->entryTester.hitTest(releasedIndex, event->pos());
                    if (event->button() == Qt::LeftButton) {
                        if (entryIdx >= 0) {
                            emit entryClicked(day, entryIdx);
                        } else {
                            emit dayClicked(day);
                        }
                    } else if (event->button() == Qt::RightButton) {
                        if (entryIdx >= 0) {
                            emit entryRightClicked(day, entryIdx);
                        } else {
                            emit dayRightClicked(day);
                        }
                    }
                }
            } else {
                if (event->button() == Qt::LeftButton) {
                    emit summaryClicked(pressedIndex);
                } else if (event->button() == Qt::RightButton) {
                    emit summaryRightClicked(pressedIndex);
                }
            }
        }
        selectDay(pressedIndex.data(DateRole).toDate());
        if (pressedIndex.isValid()) {
            QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
            if (item != nullptr) {
                item->setData(CalendarCompactDayDelegate::PressedEntryRole, QVariant());
            }
        }
        pressedPos = QPoint();
        pressedIndex = QModelIndex();
    }
    QTableWidget::mouseReleaseEvent(event);
}


void
CalendarMonthTable::mouseMoveEvent
(QMouseEvent *event)
{
    if (   ! (event->buttons() & Qt::LeftButton)
        || ! isDraggable
        || dragTimer.isActive()
        || (event->pos() - pressedPos).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData();

    QAbstractItemDelegate *abstractDelegate = itemDelegateForIndex(pressedIndex);
    CalendarCompactDayDelegate *delegate = static_cast<CalendarCompactDayDelegate*>(abstractDelegate);
    int entryIdx = delegate->entryTester.hitTest(pressedIndex, pressedPos);

    CalendarEntry calEntry = pressedIndex.data(CalendarCompactDayDelegate::DayRole).value<CalendarDay>().entries[entryIdx];
    QSize pixmapSize(40 * dpiXFactor, 40 * dpiYFactor);
    QPixmap pixmap = svgAsColoredPixmap(calEntry.iconFile, pixmapSize, 0, calEntry.color);
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmapSize.width() / 2, pixmapSize.height() / 2));

    QList<int> entryCoords = { pressedIndex.row(), pressedIndex.column(), entryIdx };
    QString entryStr = QString::fromStdString(std::accumulate(entryCoords.begin(),
                                                              entryCoords.end(),
                                                              std::string(),
                                                              [](const std::string &a, int b) {
                                                                  return a + (a.length() ? "," : "") + std::to_string(b);
                                                              }));

    QByteArray entryBytes = entryStr.toUtf8();
    mimeData->setData("application/x-day-grid-entry", entryBytes);
    drag->setMimeData(mimeData);
    drag->exec(Qt::MoveAction);
    if (pressedIndex.isValid()) {
        QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
        if (item != nullptr) {
            item->setData(CalendarCompactDayDelegate::PressedEntryRole, QVariant());
        }
    }
    pressedPos = QPoint();
    pressedIndex = QModelIndex();
}


void
CalendarMonthTable::dragEnterEvent
(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-day-grid-entry") && event->source() == this) {
        event->acceptProposedAction();
    }
}


void
CalendarMonthTable::dragMoveEvent
(QDragMoveEvent *event)
{
#if QT_VERSION < 0x060000
    QModelIndex hoverIndex = indexAt(event->pos());
#else
    QModelIndex hoverIndex = indexAt(event->position().toPoint());
#endif
    if (   hoverIndex.isValid()
        && hoverIndex.column() < 7
        && hoverIndex != pressedIndex) {
        QTableWidgetItem *item = this->item(hoverIndex.row(), hoverIndex.column());
        if (item != nullptr) {
            setCurrentItem(item);
        }
        event->accept();
    } else {
        setCurrentItem(nullptr);
        event->ignore();
    }
}


void
CalendarMonthTable::dragLeaveEvent
(QDragLeaveEvent *event)
{
    Q_UNUSED(event)

    if (pressedIndex.isValid()) {
        QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
        if (item != nullptr) {
            setCurrentItem(item);
        }
    }
}


void
CalendarMonthTable::dropEvent
(QDropEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-day-grid-entry")) {
        QByteArray entryBytes = event->mimeData()->data("application/x-day-grid-entry");
        QString entryStr = QString::fromUtf8(entryBytes);
        QStringList entryStrList = entryStr.split(',');
        QList<int> entryCoords;
        for (const QString &str : entryStrList) {
            entryCoords.append(str.toInt());
        }
        if (entryCoords.size() != 3) {
            return;
        }
        if (   entryCoords[0] >= 0
            && entryCoords[0] < rowCount()
            && entryCoords[1] >= 0
            && entryCoords[1] < 7) {
            QModelIndex srcIndex = model()->index(entryCoords[0], entryCoords[1]);
            int entryIdx = entryCoords[2];
            if (srcIndex.isValid() && entryIdx >= 0) {
                CalendarDay srcDay = srcIndex.data(CalendarCompactDayDelegate::DayRole).value<CalendarDay>();
                if (entryIdx < srcDay.entries.count()) {
#if QT_VERSION < 0x060000
                    QModelIndex destIndex = indexAt(event->pos());
#else
                    QModelIndex destIndex = indexAt(event->position().toPoint());
#endif
                    CalendarDay destDay = destIndex.data(CalendarCompactDayDelegate::DayRole).value<CalendarDay>();
                    emit entryMoved(srcDay.entries[entryIdx], srcDay.date, destDay.date, srcDay.entries[entryIdx].start);
                }
            }
        }
    }
}


#if QT_VERSION < 0x060000
QAbstractItemDelegate*
CalendarMonthTable::itemDelegateForIndex
(const QModelIndex &index) const
{
    return itemDelegateForColumn(index.column());
}
#endif


void
CalendarMonthTable::showContextMenu
(const QPoint &pos)
{
    QModelIndex index = indexAt(pos);
    if (   ! index.isValid()
        || index.column() > 6) {
        if (pressedIndex.isValid()) {
            QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
            if (item != nullptr) {
                item->setData(CalendarCompactDayDelegate::PressedEntryRole, QVariant());
            }
        }
        return;
    }
    QAbstractItemDelegate *abstractDelegate = itemDelegateForIndex(index);
    CalendarCompactDayDelegate *delegate = static_cast<CalendarCompactDayDelegate*>(abstractDelegate);
    if (delegate->moreTester.hitTest(index, pos) != -1) {
        return;
    }
    int entryIdx = delegate->entryTester.hitTest(index, pos);
    int headlineEntryIdx = delegate->headlineTester.hitTest(index, pos);
    CalendarDay day = index.data(CalendarCompactDayDelegate::DayRole).value<CalendarDay>();

    QMenu contextMenu(this);
    if (entryIdx >= 0) {
        CalendarEntry calEntry = day.entries[entryIdx];
        switch (calEntry.type) {
        case ENTRY_TYPE_ACTIVITY:
            contextMenu.addAction(tr("View activity..."), this, [=]() {
                emit viewActivity(calEntry);
            });
            contextMenu.addAction(tr("Delete activity"), this, [=]() {
                emit delActivity(calEntry);
            });
            break;
        case ENTRY_TYPE_PLANNED_ACTIVITY:
            if (calEntry.hasTrainMode) {
                contextMenu.addAction(tr("Show in train mode..."), this, [=]() {
                    emit showInTrainMode(calEntry);
                });
            }
            contextMenu.addAction(tr("View planned activity..."), this, [=]() {
                emit viewActivity(calEntry);
            });
            contextMenu.addAction(tr("Delete planned activity"), this, [=]() {
                emit delActivity(calEntry);
            });
            break;
        default:
            break;
        }
    } else if (headlineEntryIdx >= 0) {
        const CalendarEntry &entry = day.headlineEntries[headlineEntryIdx];
        switch (entry.type) {
        case ENTRY_TYPE_EVENT: {
            QAction *editEventAction = contextMenu.addAction(tr("Edit event..."), this, [=]() { emit editEvent(entry); });
            QAction *delEventAction = contextMenu.addAction(tr("Delete event"), this, [=]() { emit delEvent(entry); });
            if (! isInDateRange(day.date) || ! canHavePhasesOrEvents) {
                editEventAction->setEnabled(false);
                delEventAction->setEnabled(false);
            }
            break;
        }
        case ENTRY_TYPE_PHASE: {
            QAction *editPhaseAction = contextMenu.addAction(tr("Edit phase..."), this, [=]() { emit editPhase(entry); });
            QAction *delPhaseAction = contextMenu.addAction(tr("Delete phase"), this, [=]() { emit delPhase(entry); });
            if (! isInDateRange(day.date) || ! canHavePhasesOrEvents) {
                editPhaseAction->setEnabled(false);
                delPhaseAction->setEnabled(false);
            }
            break;
        }
        default:
            break;
        }
    } else {
        if (day.date <= QDate::currentDate()) {
            contextMenu.addAction(tr("Add activity..."), this, [=]() {
                QTime time = QTime::currentTime();
                if (day.date == QDate::currentDate()) {
                    time = time.addSecs(std::max(-4 * 3600, time.secsTo(QTime(0, 0))));
                }
                emit addActivity(false, day.date, time);
            });
        }
        if (day.date >= QDate::currentDate()) {
            contextMenu.addAction(tr("Add planned activity..."), this, [=]() {
                QTime time = QTime::currentTime();
                if (day.date == QDate::currentDate()) {
                    time = time.addSecs(std::min(4 * 3600, time.secsTo(QTime(23, 59, 59))));
                }
                emit addActivity(true, day.date, time);
            });
        }
        QAction *addPhaseAction = contextMenu.addAction(tr("Add phase..."), this, [=]() { emit addPhase(day.date); });
        QAction *addEventAction = contextMenu.addAction(tr("Add event..."), this, [=]() { emit addEvent(day.date); });
        if (! isInDateRange(day.date) || ! canHavePhasesOrEvents) {
            addPhaseAction->setEnabled(false);
            addEventAction->setEnabled(false);
        }
        if (day.date >= QDate::currentDate()) {
            contextMenu.addSeparator();
            contextMenu.addAction(tr("Repeat schedule..."), this, [=]() {
                emit repeatSchedule(day.date);
            });
            bool hasPlannedActivity = false;
            for (const CalendarEntry &calEntry : day.entries) {
                if (calEntry.type == ENTRY_TYPE_PLANNED_ACTIVITY) {
                    hasPlannedActivity = true;
                    break;
                }
            }
            if (hasPlannedActivity) {
                contextMenu.addAction(tr("Insert restday"), this, [=]() {
                    emit insertRestday(day.date);
                });
            } else {
                contextMenu.addAction(tr("Delete restday"), this, [=]() {
                    emit delRestday(day.date);
                });
            }
        }
    }
    contextMenu.exec(viewport()->mapToGlobal(pos));
    if (pressedIndex.isValid()) {
        QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
        if (item != nullptr) {
            item->setData(CalendarCompactDayDelegate::PressedEntryRole, QVariant());
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// CalendarDayView

CalendarDayView::CalendarDayView
(const QDate &dateInMonth, Measures * const athleteMeasures, QWidget *parent)
: QWidget(parent), athleteMeasures(athleteMeasures)
{
    dayDateSelector = new CalendarOverview();
    dayDateSelector->setFixedHeight(std::max(static_cast<int>(280 * dpiYFactor), dayDateSelector->sizeHint().height()));

    measureTabs = new QTabWidget();

    QWidget *dayLeftPane = new QWidget();
    QVBoxLayout *leftPaneLayout = new QVBoxLayout(dayLeftPane);
    leftPaneLayout->addWidget(dayDateSelector);
    leftPaneLayout->addSpacing(10 * dpiYFactor);
    leftPaneLayout->addWidget(measureTabs, 1);
    dayLeftPane->setFixedWidth(dayDateSelector->sizeHint().width() + leftPaneLayout->contentsMargins().left() + leftPaneLayout->contentsMargins().right());

    dayTable = new CalendarDayTable(dateInMonth);

    QHBoxLayout *dayLayout = new QHBoxLayout(this);
    dayLayout->addWidget(dayLeftPane);
    dayLayout->addWidget(dayTable);

    connect(dayDateSelector, &QCalendarWidget::selectionChanged, [=]() {
        if (dayTable->selectedDate() != dayDateSelector->selectedDate()) {
            setDay(dayDateSelector->selectedDate());
        }
    });
    connect(dayTable, &CalendarDayTable::dayChanged, [=](const QDate &date) {
        dayDateSelector->setSelectedDate(date);
        emit dayChanged(date);
    });
    connect(dayTable, &CalendarDayTable::viewActivity, this, &CalendarDayView::viewActivity);
    connect(dayTable, &CalendarDayTable::addActivity, this, &CalendarDayView::addActivity);
    connect(dayTable, &CalendarDayTable::showInTrainMode, this, &CalendarDayView::showInTrainMode);
    connect(dayTable, &CalendarDayTable::delActivity, this, &CalendarDayView::delActivity);
    connect(dayTable, &CalendarDayTable::entryMoved, this, &CalendarDayView::entryMoved);
    connect(dayTable, &CalendarDayTable::addPhase, this, &CalendarDayView::addPhase);
    connect(dayTable, &CalendarDayTable::editPhase, this, &CalendarDayView::editPhase);
    connect(dayTable, &CalendarDayTable::delPhase, this, &CalendarDayView::delPhase);
    connect(dayTable, &CalendarDayTable::addEvent, this, &CalendarDayView::addEvent);
    connect(dayTable, &CalendarDayTable::editEvent, this, &CalendarDayView::editEvent);
    connect(dayTable, &CalendarDayTable::delEvent, this, &CalendarDayView::delEvent);
}


bool
CalendarDayView::setDay
(const QDate &date)
{
    updateMeasures(date);
    return dayTable->setDay(date);
}


void
CalendarDayView::setFirstDayOfWeek
(Qt::DayOfWeek firstDayOfWeek)
{
    dayDateSelector->setFirstDayOfWeek(firstDayOfWeek);
}


void
CalendarDayView::setStartHour
(int hour)
{
    dayTable->setStartHour(hour);
}


void
CalendarDayView::setEndHour
(int hour)
{
    dayTable->setEndHour(hour);
}


void
CalendarDayView::setSummaryVisible
(bool visible)
{
    dayTable->setRowHidden(2, ! visible);
}


void
CalendarDayView::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries)
{
    dayDateSelector->fillEntries(activityEntries, headlineEntries);
    dayTable->fillEntries(activityEntries, summaries, headlineEntries);
}


void
CalendarDayView::limitDateRange
(const DateRange &dr, bool canHavePhasesOrEvents)
{
    dayDateSelector->limitDateRange(dr);
    dayTable->limitDateRange(dr, canHavePhasesOrEvents);
}


QDate
CalendarDayView::firstVisibleDay
() const
{
    return dayDateSelector->firstVisibleDay();
}


QDate
CalendarDayView::lastVisibleDay
() const
{
    return dayDateSelector->lastVisibleDay();
}


QDate
CalendarDayView::selectedDate
() const
{
    return dayTable->selectedDate();
}


void
CalendarDayView::updateMeasures
()
{
    updateMeasures(selectedDate());
}


void
CalendarDayView::updateMeasures
(const QDate &date)
{
    int currentIndex = measureTabs->currentIndex();
    while (measureTabs->count() > 0) {
        QWidget *page = measureTabs->widget(0);
        measureTabs->removeTab(0);
        delete page;
    }
    bool metricUnits = GlobalContext::context()->useMetricUnits;
    for (MeasuresGroup * const measuresGroup : athleteMeasures->getGroups()) {
        QWidget *measureWidget = new QWidget();
        Measure measure;
        measuresGroup->getMeasure(date, measure);
        QVBoxLayout *measureLayout = new QVBoxLayout();
        int buttonType = 0;
        if (measure.when.isValid()) {
            QFormLayout *form = newQFormLayout();
            for (int i = 0; i < measuresGroup->getFieldNames().count(); ++i) {
                QString measureText;
                double fieldValue = measuresGroup->getFieldValue(date, i, metricUnits);
                if (fieldValue > 0) {
                    if (measuresGroup->getFieldUnits(i, metricUnits).size() > 0) {
                        measureText = QString("%1 %2").arg(fieldValue).arg(measuresGroup->getFieldUnits(i, metricUnits));
                    } else {
                        measureText = QString("%1").arg(fieldValue);
                    }
                    form->addRow(measuresGroup->getFieldNames()[i], new QLabel(measureText));
                }
            }
            if (! measure.comment.isEmpty()) {
                QTextEdit *commentField = new QTextEdit();
                commentField->setAcceptRichText(false);
                commentField->setReadOnly(true);
                commentField->setText(measure.comment);
                commentField->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                form->addRow(commentField);
            }
            QLocale locale;
            QString validText = locale.toString(measure.when, QLocale::ShortFormat);
            int validDays = measure.when.date().daysTo(date);
            if (validDays > 1) {
                validText.append(tr("\n(%1 days earlier)").arg(validDays));
            } else if (validDays > 0) {
                validText.append(tr("\n(%1 day earlier)").arg(validDays));
            }
            form->addRow(tr("Valid since"), new QLabel(validText));
            QWidget *scrollWidget = new QWidget();
            scrollWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            scrollWidget->setLayout(form);
            QScrollArea *scrollArea = new QScrollArea();
            scrollArea->setWidget(scrollWidget);
            scrollArea->setWidgetResizable(true);
            measureLayout->addWidget(scrollArea);
            if (validDays == 0) {
                buttonType = 1;
            }
        } else {
            QLabel *noMeasureLabel = new QLabel(tr("No measure available"));
            noMeasureLabel->setAlignment(Qt::AlignCenter);
            measureLayout->addStretch();
            measureLayout->addWidget(noMeasureLabel, Qt::AlignCenter);
            measureLayout->addStretch();
        }
        if (buttonType == 0) {
            QPushButton *addButton = new QPushButton(tr("Add Measure"));
            connect(addButton, &QPushButton::clicked, [=]() {
                if (measureDialog(QDateTime(date, QTime::currentTime()), measuresGroup, false)) {
                    QTimer::singleShot(0, this, [=]() {
                        updateMeasures(date);
                    });
                }
            });
            measureLayout->addWidget(addButton);
        } else {
            QPushButton *editButton = new QPushButton(tr("Edit Measure"));
            connect(editButton, &QPushButton::clicked, [=]() {
                if (measureDialog(measure.when, measuresGroup, true)) {
                    QTimer::singleShot(0, this, [=]() {
                        updateMeasures(date);
                    });
                }
            });
            measureLayout->addWidget(editButton);
        }
        measureWidget->setLayout(measureLayout);
        measureTabs->addTab(measureWidget, measuresGroup->getName());
    }
    if (measureTabs->count() > currentIndex) {
        measureTabs->setCurrentIndex(currentIndex);
    }
    PaletteApplier::setPaletteRecursively(measureTabs, this->palette(), true);
}


bool
CalendarDayView::measureDialog
(const QDateTime &when, MeasuresGroup * const measuresGroup, bool update)
{
    QDialog dialog;
    dialog.setWindowTitle(update ? tr("Edit Measure") : tr("Add Measure"));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(  QDialogButtonBox::Apply
                                                       | QDialogButtonBox::Discard);
    connect(buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), &dialog, SLOT(accept()));
    connect(buttonBox->button(QDialogButtonBox::Discard), SIGNAL(clicked()), &dialog, SLOT(reject()));

    QFormLayout *form = newQFormLayout(&dialog);

    QLocale locale;
    form->addRow(tr("Start Date"), new QLabel(locale.toString(when, QLocale::ShortFormat)));
    QList<double> unitsFactors = measuresGroup->getFieldUnitsFactors();

    QStringList fieldNames = measuresGroup->getFieldNames();
    QList<QLabel*> valuesLabel;
    QList<QDoubleSpinBox*> valuesEdit;
    bool metricUnits = GlobalContext::context()->useMetricUnits;
    int i = 0;
    for (QString &fieldName : fieldNames) {
        valuesLabel << new QLabel(fieldName);
        valuesEdit << new QDoubleSpinBox(this);
        valuesEdit[i]->setMaximum(9999.99);
        valuesEdit[i]->setMinimum(0.0);
        valuesEdit[i]->setDecimals(2);
        valuesEdit[i]->setValue(measuresGroup->getFieldValue(when.date(), i, metricUnits));
        valuesEdit[i]->setSuffix(QString(" %1").arg(measuresGroup->getFieldUnits(i, metricUnits)));

        form->addRow(valuesLabel[i], valuesEdit[i]);

        ++i;
    }

    Measure measure;
    measuresGroup->getMeasure(when.date(), measure);
    QTextEdit *commentEdit = new QTextEdit(measure.comment);
    commentEdit->setAcceptRichText(false);
    form->addRow(tr("Comment"), commentEdit);

    form->addRow(buttonBox);

    int dialogRet = dialog.exec();
    if (dialogRet != QDialog::Accepted) {
        return false;
    }

    for (i = 0; i < valuesEdit.count(); ++i) {
        measure.values[i] = valuesEdit[i]->value();
    }
    measure.when = when;
    measure.comment = commentEdit->toPlainText();

    QList<Measure> measures = measuresGroup->measures();
    bool found = false;
    for (int measureIdx = 0; measureIdx < measures.count(); ++measureIdx) {
        if (measures[measureIdx].when == measure.when) {
            measures[measureIdx] = measure;
            found = true;
            break;
        }
    }
    if (! found) {
        measures << measure;
    }

    std::reverse(measures.begin(), measures.end());
    measuresGroup->setMeasures(measures);
    measuresGroup->write();

    return true;
}


//////////////////////////////////////////////////////////////////////////////
// CalendarWeekView

CalendarWeekView::CalendarWeekView
(const QDate &date, QWidget *parent)
: QWidget(parent)
{
    weekTable = new CalendarDayTable(date, CalendarDayTableType::Week);

    QHBoxLayout *weekLayout = new QHBoxLayout(this);
    weekLayout->addWidget(weekTable);

    connect(weekTable, &CalendarDayTable::dayChanged, this, &CalendarWeekView::dayChanged);
    connect(weekTable, &CalendarDayTable::viewActivity, this, &CalendarWeekView::viewActivity);
    connect(weekTable, &CalendarDayTable::addActivity, this, &CalendarWeekView::addActivity);
    connect(weekTable, &CalendarDayTable::showInTrainMode, this, &CalendarWeekView::showInTrainMode);
    connect(weekTable, &CalendarDayTable::delActivity, this, &CalendarWeekView::delActivity);
    connect(weekTable, &CalendarDayTable::entryMoved, this, &CalendarWeekView::entryMoved);
    connect(weekTable, &CalendarDayTable::addPhase, this, &CalendarWeekView::addPhase);
    connect(weekTable, &CalendarDayTable::editPhase, this, &CalendarWeekView::editPhase);
    connect(weekTable, &CalendarDayTable::delPhase, this, &CalendarWeekView::delPhase);
    connect(weekTable, &CalendarDayTable::addEvent, this, &CalendarWeekView::addEvent);
    connect(weekTable, &CalendarDayTable::editEvent, this, &CalendarWeekView::editEvent);
    connect(weekTable, &CalendarDayTable::delEvent, this, &CalendarWeekView::delEvent);

    setDay(date);
}


bool
CalendarWeekView::setDay
(const QDate &date)
{
    return weekTable->setDay(date);
}


void
CalendarWeekView::setFirstDayOfWeek
(Qt::DayOfWeek firstDayOfWeek)
{
    weekTable->setFirstDayOfWeek(firstDayOfWeek);
}


void
CalendarWeekView::setStartHour
(int hour)
{
    weekTable->setStartHour(hour);
}


void
CalendarWeekView::setEndHour
(int hour)
{
    weekTable->setEndHour(hour);
}


void
CalendarWeekView::setSummaryVisible
(bool visible)
{
    weekTable->setRowHidden(2, ! visible);
}


void
CalendarWeekView::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries)
{
    weekTable->fillEntries(activityEntries, summaries, headlineEntries);
}


void
CalendarWeekView::limitDateRange
(const DateRange &dr, bool canHavePhasesOrEvents)
{
    weekTable->limitDateRange(dr, canHavePhasesOrEvents);
}


QDate
CalendarWeekView::firstVisibleDay
() const
{
    return weekTable->firstVisibleDay();
}


QDate
CalendarWeekView::firstVisibleDay
(const QDate &date) const
{
    return weekTable->firstVisibleDay(date);
}


QDate
CalendarWeekView::lastVisibleDay
() const
{
    return weekTable->lastVisibleDay();
}


QDate
CalendarWeekView::lastVisibleDay
(const QDate &date) const
{
    return weekTable->lastVisibleDay(date);
}


QDate
CalendarWeekView::selectedDate
() const
{
    return weekTable->selectedDate();
}


//////////////////////////////////////////////////////////////////////////////
// Calendar

Calendar::Calendar
(const QDate &dateInMonth, Qt::DayOfWeek firstDayOfWeek, Measures * const athleteMeasures, QWidget *parent)
: QWidget(parent)
{
    qRegisterMetaType<CalendarDay>("CalendarDay");
    qRegisterMetaType<CalendarSummary>("CalendarSummary");

    dayView = new CalendarDayView(dateInMonth, athleteMeasures);
    weekView = new CalendarWeekView(dateInMonth);
    monthView = new CalendarMonthTable(dateInMonth, firstDayOfWeek);

    viewStack = new QStackedWidget();
    viewStack->addWidget(dayView);
    viewStack->addWidget(weekView);
    viewStack->addWidget(monthView);

    toolbar = new QToolBar();

    prevAction = toolbar->addAction(tr("Previous Month"));
    nextAction = toolbar->addAction(tr("Next Month"));
    todayAction = toolbar->addAction(tr("Today"));
    separator = toolbar->addSeparator();

    dateNavigator = new QToolButton();
    dateNavigator->setPopupMode(QToolButton::InstantPopup);
    dateNavigatorAction = toolbar->addWidget(dateNavigator);

    dateMenu = new QMenu(this);
    connect(dateMenu, &QMenu::aboutToShow, this, &Calendar::populateDateMenu);

    dateNavigator->setMenu(dateMenu);

    seasonLabel = new QLabel();
    seasonLabelAction = toolbar->addWidget(seasonLabel);

    QWidget *spacer = new QWidget(toolbar);
    spacer->setFixedWidth(10 * dpiXFactor);
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    filterSpacerAction = toolbar->addWidget(spacer);

    filterLabel = new QLabel("<i>" + tr("Filters applied") + "</i>");
    filterLabelAction = toolbar->addWidget(filterLabel);

    QWidget *stretch = new QWidget();
    stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(stretch);

    QActionGroup *viewGroup = new QActionGroup(toolbar);
    dayAction = toolbar->addAction(tr("Day"));
    dayAction->setCheckable(true);
    dayAction->setActionGroup(viewGroup);

    connect(dayAction, &QAction::triggered, [=]() { setView(CalendarView::Day); });

    weekAction = toolbar->addAction(tr("Week"));
    weekAction->setCheckable(true);
    weekAction->setActionGroup(viewGroup);
    connect(weekAction, &QAction::triggered, [=]() { setView(CalendarView::Week); });

    monthAction = toolbar->addAction(tr("Month"));
    monthAction->setCheckable(true);
    monthAction->setActionGroup(viewGroup);
    connect(monthAction, &QAction::triggered, [=]() { setView(CalendarView::Month); });

    applyNavIcons();

    connect(dayView, &CalendarDayView::dayChanged, [=](const QDate &date) {
        if (currentView() == CalendarView::Day) {
            emit dayChanged(date);
            updateHeader();
            setNavButtonState();
        }
    });
    connect(dayView, &CalendarDayView::viewActivity, this, &Calendar::viewActivity);
    connect(dayView, &CalendarDayView::addActivity, this, &Calendar::addActivity);
    connect(dayView, &CalendarDayView::showInTrainMode, this, &Calendar::showInTrainMode);
    connect(dayView, &CalendarDayView::delActivity, this, &Calendar::delActivity);
    connect(dayView, &CalendarDayView::entryMoved, this, &Calendar::moveActivity);
    connect(dayView, &CalendarDayView::addPhase, this, &Calendar::addPhase);
    connect(dayView, &CalendarDayView::editPhase, this, &Calendar::editPhase);
    connect(dayView, &CalendarDayView::delPhase, this, &Calendar::delPhase);
    connect(dayView, &CalendarDayView::addEvent, this, &Calendar::addEvent);
    connect(dayView, &CalendarDayView::editEvent, this, &Calendar::editEvent);
    connect(dayView, &CalendarDayView::delEvent, this, &Calendar::delEvent);

    connect(weekView, &CalendarWeekView::dayChanged, [=](const QDate &date) {
        if (currentView() == CalendarView::Week) {
            emit dayChanged(date);
            updateHeader();
            setNavButtonState();
        }
    });
    connect(weekView, &CalendarWeekView::viewActivity, this, &Calendar::viewActivity);
    connect(weekView, &CalendarWeekView::addActivity, this, &Calendar::addActivity);
    connect(weekView, &CalendarWeekView::showInTrainMode, this, &Calendar::showInTrainMode);
    connect(weekView, &CalendarWeekView::delActivity, this, &Calendar::delActivity);
    connect(weekView, &CalendarWeekView::entryMoved, this, &Calendar::moveActivity);
    connect(weekView, &CalendarWeekView::addPhase, this, &Calendar::addPhase);
    connect(weekView, &CalendarWeekView::editPhase, this, &Calendar::editPhase);
    connect(weekView, &CalendarWeekView::delPhase, this, &Calendar::delPhase);
    connect(weekView, &CalendarWeekView::addEvent, this, &Calendar::addEvent);
    connect(weekView, &CalendarWeekView::editEvent, this, &Calendar::editEvent);
    connect(weekView, &CalendarWeekView::delEvent, this, &Calendar::delEvent);

    connect(monthView, &CalendarMonthTable::entryDblClicked, [=](const CalendarDay &day, int entryIdx) {
        viewActivity(day.entries[entryIdx]);
    });
    connect(monthView, &CalendarMonthTable::daySelected, [=](const CalendarDay &day) {
        emit daySelected(day.date);
    });
    connect(monthView, &CalendarMonthTable::moreClicked, [=]() {
        setView(CalendarView::Day);
    });
    connect(monthView, &CalendarMonthTable::dayDblClicked, [=]() {
        setView(CalendarView::Day);
    });
    connect(monthView, &CalendarMonthTable::showInTrainMode, this, &Calendar::showInTrainMode);
    connect(monthView, &CalendarMonthTable::viewActivity, this, &Calendar::viewActivity);
    connect(monthView, &CalendarMonthTable::addActivity, this, &Calendar::addActivity);
    connect(monthView, &CalendarMonthTable::repeatSchedule, this, &Calendar::repeatSchedule);
    connect(monthView, &CalendarMonthTable::delActivity, this, &Calendar::delActivity);
    connect(monthView, &CalendarMonthTable::entryMoved, this, &Calendar::moveActivity);
    connect(monthView, &CalendarMonthTable::addPhase, this, &Calendar::addPhase);
    connect(monthView, &CalendarMonthTable::editPhase, this, &Calendar::editPhase);
    connect(monthView, &CalendarMonthTable::delPhase, this, &Calendar::delPhase);
    connect(monthView, &CalendarMonthTable::addEvent, this, &Calendar::addEvent);
    connect(monthView, &CalendarMonthTable::editEvent, this, &Calendar::editEvent);
    connect(monthView, &CalendarMonthTable::delEvent, this, &Calendar::delEvent);
    connect(monthView, &CalendarMonthTable::insertRestday, this, &Calendar::insertRestday);
    connect(monthView, &CalendarMonthTable::delRestday, this, &Calendar::delRestday);
    connect(monthView, &CalendarMonthTable::monthChanged, [=](const QDate &month, const QDate &firstVisible, const QDate &lastVisible) {
        if (currentView() == CalendarView::Month) {
            emit monthChanged(month, firstVisible, lastVisible);
            updateHeader();
            setNavButtonState();
        }
    });

    connect(prevAction, &QAction::triggered, [=]() { goNext(-1); });
    connect(nextAction, &QAction::triggered, [=]() { goNext(1); });
    connect(todayAction, &QAction::triggered, [=]() { setDate(QDate::currentDate()); });

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(viewStack);

    setView(CalendarView::Month);
    setDate(dateInMonth);
}


void
Calendar::setDate
(const QDate &date, bool allowKeepMonth)
{
    if (currentView() == CalendarView::Day) {
        if (isInDateRange(date)) {
            dayView->setDay(date);
        }
    } else if (currentView() == CalendarView::Week) {
        if (isInDateRange(date)) {
            weekView->setDay(date);
        }
    } else if (currentView() == CalendarView::Month) {
        if (monthView->isInDateRange(date)) {
            monthView->setMonth(date, allowKeepMonth);
        }
    }
}


void
Calendar::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries, bool isFiltered)
{
    if (currentView() == CalendarView::Day) {
        dayView->fillEntries(activityEntries, summaries, headlineEntries);
    } else if (currentView() == CalendarView::Week) {
        weekView->fillEntries(activityEntries, summaries, headlineEntries);
    } else if (currentView() == CalendarView::Month) {
        monthView->fillEntries(activityEntries, summaries, headlineEntries);
    }
    filterSpacerAction->setVisible(isFiltered);
    filterLabelAction->setVisible(isFiltered);
}


QDate
Calendar::firstOfCurrentMonth
() const
{
    return monthView->firstOfCurrentMonth();
}


QDate
Calendar::firstVisibleDay
() const
{
    if (currentView() == CalendarView::Day) {
        return dayView->firstVisibleDay();
    } else if (currentView() == CalendarView::Week) {
        return weekView->firstVisibleDay();
    } else if (currentView() == CalendarView::Month) {
        return monthView->firstVisibleDay();
    }
    return QDate();
}


QDate
Calendar::lastVisibleDay
() const
{
    if (currentView() == CalendarView::Day) {
        return dayView->lastVisibleDay();
    } else if (currentView() == CalendarView::Week) {
        return weekView->lastVisibleDay();
    } else if (currentView() == CalendarView::Month) {
        return monthView->lastVisibleDay();
    }
    return QDate();
}


QDate
Calendar::selectedDate
() const
{
    if (currentView() == CalendarView::Day) {
        return dayView->selectedDate();
    } else if (currentView() == CalendarView::Week) {
        return weekView->selectedDate();
    } else if (currentView() == CalendarView::Month) {
        return monthView->selectedDate();
    }
    return QDate();
}


CalendarView
Calendar::currentView
() const
{
    return static_cast<CalendarView>(viewStack->currentIndex());
}


bool
Calendar::goNext
(int amount)
{
    bool ret = true;
    if (currentView() == CalendarView::Day) {
        if ((ret = canGoNext(amount))) {
            setDate(dayView->selectedDate().addDays(amount));
        }
    } else if (currentView() == CalendarView::Week) {
        QDate newDate = selectedDate().addDays(7 * amount);
        if ((ret = newDate.isValid())) {
            setDate(newDate);
        }
    } else if (currentView() == CalendarView::Month) {
        QDate newDate = fitToMonth(selectedDate().addMonths(amount), true);
        if ((ret = newDate.isValid())) {
            setDate(newDate);
        }
    }
    return ret;
}


QDate
Calendar::fitToMonth
(const QDate &date, bool preferToday) const
{
    QDate newDate(date);
    QDate today = QDate::currentDate();
    if (! newDate.isValid()) {
        newDate = today;
    }
    if (   preferToday
        && newDate.year() == today.year()
        && newDate.month() == today.month()
        && isInDateRange(today)) {
        newDate = today;
    } else if (! isInDateRange(newDate)) {
        if (newDate < dateRange.to) {
            newDate = QDate(newDate.year(), newDate.month(), newDate.daysInMonth());
        } else {
            newDate = QDate(newDate.year(), newDate.month(), 1);
        }
    }
    return isInDateRange(newDate) ? newDate : QDate();
}


bool
Calendar::canGoNext
(int amount) const
{
    if (currentView() == CalendarView::Day) {
        return isInDateRange(dayView->selectedDate().addDays(amount));
    } else if (currentView() == CalendarView::Week) {
        return isInDateRange(weekView->selectedDate().addDays(7 * amount));
    } else if (currentView() == CalendarView::Month) {
        QDate fom = monthView->firstOfCurrentMonth();
        QDate lom(fom.year(), fom.month(), fom.daysInMonth());
        fom = fom.addMonths(amount);
        lom = lom.addMonths(amount);
        return isInDateRange(fom) || isInDateRange(lom);
    }
    return false;
}


int
Calendar::weekNumber
(const QDate &date) const
{
    int isoWeekNumber = date.weekNumber();
    int dayOfWeek = date.dayOfWeek();
    int offset = (dayOfWeek - firstDayOfWeek + 7) % 7;
    if (offset != 0) {
        isoWeekNumber++;
    }
    return isoWeekNumber;
}


bool
Calendar::isInDateRange
(const QDate &date) const
{
    return date.isValid() && dateRange.pass(date);
}


void
Calendar::activateDateRange
(const DateRange &dr, bool allowKeepMonth, bool canHavePhasesOrEvents)
{
    QDate currentDate = selectedDate();
    dateRange = dr;
    monthView->limitDateRange(dr, allowKeepMonth, canHavePhasesOrEvents);
    weekView->limitDateRange(dr, canHavePhasesOrEvents);
    dayView->limitDateRange(dr, canHavePhasesOrEvents);
    if (currentView() == CalendarView::Day || currentView() == CalendarView::Week) {
        setDate(currentDate, false);
    } else if (currentView() == CalendarView::Month) {
        setDate(fitToMonth(currentDate, false), true);
    }
    seasonLabel->setText(tr("Season: %1").arg(dateRange.name));
    emit dateRangeActivated(dr.name);
}


void
Calendar::setFirstDayOfWeek
(Qt::DayOfWeek firstDayOfWeek)
{
    QDate currentDate = selectedDate();
    this->firstDayOfWeek = firstDayOfWeek;
    monthView->setFirstDayOfWeek(firstDayOfWeek);
    weekView->setFirstDayOfWeek(firstDayOfWeek);
    dayView->setFirstDayOfWeek(firstDayOfWeek);
    if (currentView() == CalendarView::Week) {
        setDate(currentDate, false);
    } else if (currentView() == CalendarView::Month) {
        setDate(fitToMonth(currentDate, false), true);
    }
}


void
Calendar::setStartHour
(int hour)
{
    weekView->setStartHour(hour);
    dayView->setStartHour(hour);
}


void
Calendar::setEndHour
(int hour)
{
    weekView->setEndHour(hour);
    dayView->setEndHour(hour);
}


void
Calendar::setSummaryDayVisible
(bool visible)
{
    dayView->setSummaryVisible(visible);
}


void
Calendar::setSummaryWeekVisible
(bool visible)
{
    weekView->setSummaryVisible(visible);
}


void
Calendar::setSummaryMonthVisible
(bool visible)
{
    monthView->setColumnHidden(7, ! visible);
}


void
Calendar::setNavButtonState
()
{
    prevAction->setEnabled(canGoNext(-1));
    nextAction->setEnabled(canGoNext(1));
    todayAction->setEnabled(isInDateRange(QDate::currentDate()));
}


void
Calendar::updateHeader
()
{
    QLocale locale;
    if (currentView() == CalendarView::Day) {
        dateNavigator->setText(locale.toString(dayView->selectedDate(), QLocale::LongFormat));
        prevAction->setVisible(true);
        nextAction->setVisible(true);
        todayAction->setVisible(true);
        separator->setVisible(true);
        dateNavigatorAction->setVisible(true);
        seasonLabelAction->setVisible(false);
    } else if (currentView() == CalendarView::Week) {
        dateNavigator->setText(tr("Week %1 (%2 - %3)")
                                 .arg(weekNumber(weekView->selectedDate()))
                                 .arg(locale.toString(weekView->firstVisibleDay(), QLocale::ShortFormat))
                                 .arg(locale.toString(weekView->lastVisibleDay(), QLocale::ShortFormat)));
        prevAction->setVisible(true);
        nextAction->setVisible(true);
        todayAction->setVisible(true);
        separator->setVisible(true);
        dateNavigatorAction->setVisible(true);
        seasonLabelAction->setVisible(false);
    } else if (currentView() == CalendarView::Month) {
        dateNavigator->setText(locale.toString(monthView->firstOfCurrentMonth(), "MMMM yyyy"));
        prevAction->setVisible(true);
        nextAction->setVisible(true);
        todayAction->setVisible(true);
        separator->setVisible(true);
        dateNavigatorAction->setVisible(true);
        seasonLabelAction->setVisible(false);
    }
}


void
Calendar::applyNavIcons
()
{
    double scale = appsettings->value(this, GC_FONT_SCALE, 1.0).toDouble();
    QFont font;
    font.setPointSize(font.pointSizeF() * scale * 1.3);
    font.setWeight(QFont::Bold);
    dateNavigator->setFont(font);
    seasonLabel->setFont(font);
    QString mode = GCColor::isPaletteDark(palette()) ? "dark" : "light";
    toolbar->setMinimumHeight(dateNavigator->sizeHint().height() + 12 * dpiYFactor);

    prevAction->setIcon(QIcon(QString(":images/breeze/%1/go-previous.svg").arg(mode)));
    nextAction->setIcon(QIcon(QString(":images/breeze/%1/go-next.svg").arg(mode)));
}


void
Calendar::updateMeasures
()
{
    if (currentView() == CalendarView::Day) {
        dayView->updateMeasures();
    }
}


void
Calendar::setView
(CalendarView view)
{
    int idx = static_cast<int>(view);
    int oldIdx = viewStack->currentIndex();
    if (idx != oldIdx) {
        QDate useDate = selectedDate();
        if (view == CalendarView::Day) {
            dayAction->setChecked(true);
            dayView->setDay(useDate);
        } else if (view == CalendarView::Week) {
            weekAction->setChecked(true);
            weekView->setDay(useDate);
        } else if (view == CalendarView::Month) {
            monthAction->setChecked(true);
            monthView->setMonth(fitToMonth(selectedDate(), false), true);
        }
        viewStack->setCurrentIndex(idx);
        emit viewChanged(view, static_cast<CalendarView>(oldIdx));
        updateHeader();
        setNavButtonState();
    }
}


void
Calendar::populateDateMenu
()
{
    dateMenu->clear();
    dateMenu->addSection(tr("Season: %1").arg(dateRange.name));
    dateMenu->setEnabled(true);
    if (currentView() == CalendarView::Day || currentView() == CalendarView::Week) {
        int currentYear = selectedDate().year();
        int currentMonth = selectedDate().month();
        int firstMonth = 1;
        int lastMonth = 12;
        if (dateRange.from.isValid() && dateRange.from.year() == currentYear) {
            firstMonth = dateRange.from.month();
        }
        if (dateRange.to.isValid() && dateRange.to.year() == currentYear) {
            lastMonth = dateRange.to.month();
        }
        QDate firstDate(currentYear, firstMonth, 1);
        QDate lastDate(currentYear, lastMonth, 1);
        QLocale locale;
        for (QDate date = firstDate; date <= lastDate; date = date.addMonths(1)) {
            QAction *action = dateMenu->addAction(locale.toString(date, "MMMM yyyy"));
            if (currentMonth == date.month()) {
                action->setEnabled(false);
            } else {
                QDate actualDate = date;
                while (! dateRange.pass(actualDate)) {
                    actualDate = actualDate.addDays(1);
                    if (actualDate.month() != date.month()) {
                        break;
                    }
                }
                if (actualDate.month() == date.month()) {
                    connect(action, &QAction::triggered, [=]() {
                        setDate(actualDate);
                    });
                }
            }
        }
    } else if (currentView() == CalendarView::Month) {
        int yearFrom = dateRange.from.isValid() ? dateRange.from.year() : 2020;
        int yearTo = dateRange.to.isValid() ? dateRange.to.year() : 2030;
        for (int year = yearFrom; year <= yearTo; ++year) {
            QAction *action = dateMenu->addAction(QString::number(year));
            if (year == selectedDate().year()) {
                action->setEnabled(false);
            } else {
                QDate date(year, 1, 1);
                if (! dateRange.pass(date)) {
                    if (date.year() == dateRange.from.year()) {
                        date = dateRange.from;
                    } else {
                        date = dateRange.to;
                    }
                }
                connect(action, &QAction::triggered, [=]() {
                    setDate(date);
                });
            }
        }
    }
}
