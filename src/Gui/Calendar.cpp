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
#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
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
(const QDate &date, QWidget *parent)
: QTableWidget(parent)
{
    dragTimer.setSingleShot(true);
    setAcceptDrops(true);
    setColumnCount(2);
    setRowCount(2);
    setFrameShape(QFrame::NoFrame);

    QList<QStyledItemDelegate*> row0Delegates;
    row0Delegates << new CalendarTimeScaleDelegate(&timeScaleData, this);
    row0Delegates << new CalendarDayViewDayDelegate(&timeScaleData, this);
    setItemDelegateForRow(0, new ColumnDelegatingItemDelegate(row0Delegates, this));
    setItemDelegateForRow(1, new CalendarSummaryDelegate(20, this));

    horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    horizontalHeader()->setVisible(false);
    verticalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    verticalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    verticalHeader()->setVisible(false);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &CalendarDayTable::customContextMenuRequested, this, &CalendarDayTable::showContextMenu);
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
    timeScaleItem->setData(Qt::UserRole, -1);
    setItem(0, 0, timeScaleItem);
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setData(Qt::UserRole, date);
    CalendarDay day;
    day.date = date;
    day.isDimmed = false;
    item->setData(Qt::UserRole + 1, QVariant::fromValue(day));
    setItem(0, 1, item);
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);

    emit dayChanged(date);

    return true;
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
(const QList<CalendarEntry> &activityEntries, const CalendarSummary &summary, const QList<CalendarEntry> &headlineEntries)
{
    Q_UNUSED(headlineEntries)

    QTableWidgetItem *item = this->item(0, 1);
    CalendarDay day = item->data(Qt::UserRole + 1).value<CalendarDay>();
    day.entries = activityEntries;
    day.headlineEntries.clear();
    item->setData(Qt::UserRole + 1, QVariant::fromValue(day));

    int start = 8;
    int end = 21;
    for (const CalendarEntry &entry : activityEntries) {
        start = std::min(start, entry.start.hour());
        QTime endTime = entry.start.addSecs(entry.durationSecs);
        if (endTime < entry.start) {
            end = 24;
        } else {
            end = std::max(end, endTime.hour() + 1);
        }
    }
    end = std::min(24, end);
    timeScaleData.setFirstMinute(start * 60);
    timeScaleData.setLastMinute(end * 60);

    CalendarEntryLayouter layouter;
    QList<CalendarEntryLayout> layout = layouter.layout(activityEntries);
    item->setData(Qt::UserRole + 3, QVariant::fromValue(layout));

    QTableWidgetItem *summaryItem = new QTableWidgetItem();
    summaryItem->setData(Qt::UserRole, QVariant::fromValue(summary));
    summaryItem->setFlags(Qt::ItemIsEnabled);
    setItem(1, 1, summaryItem);
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
    if (row == 0 && column > 0) {
        ColumnDelegatingItemDelegate *delegatingDelegate = static_cast<ColumnDelegatingItemDelegate*>(itemDelegateForRow(row));
        CalendarDayViewDayDelegate *delegate = static_cast<CalendarDayViewDayDelegate*>(delegatingDelegate->getDelegate(column));

        CalendarDay day = index.data(Qt::UserRole + 1).value<CalendarDay>();
        int entryIdx = delegate->hitTestEntry(index, event->pos());
        if (entryIdx >= 0) {
            emit viewActivity(day.entries[entryIdx]);
        } else {
            QTime time = timeScaleData.timeFromY(pos.y(), visualRect(index));
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
    if (pressedIndex.row() == 0 && pressedIndex.column() > 0) {
        ColumnDelegatingItemDelegate *delegatingDelegate = static_cast<ColumnDelegatingItemDelegate*>(itemDelegateForRow(pressedIndex.row()));
        CalendarDayViewDayDelegate *delegate = static_cast<CalendarDayViewDayDelegate*>(delegatingDelegate->getDelegate(pressedIndex.column()));
        int entryIdx = delegate->hitTestEntry(pressedIndex, pressedPos);
        if (pressedIndex.isValid()) {
            QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
            item->setData(Qt::UserRole + 2, entryIdx);
        }
        CalendarDay day = pressedIndex.data(Qt::UserRole + 1).value<CalendarDay>();
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
            if (pressedIndex.row() == 0 && pressedIndex.column() > 0) {
                QTime time = timeScaleData.timeFromY(pos.y(), visualRect(releasedIndex));
                CalendarDay day = pressedIndex.data(Qt::UserRole + 1).value<CalendarDay>();
                ColumnDelegatingItemDelegate *delegatingDelegate = static_cast<ColumnDelegatingItemDelegate*>(itemDelegateForRow(pressedIndex.row()));
                CalendarDayViewDayDelegate *delegate = static_cast<CalendarDayViewDayDelegate*>(delegatingDelegate->getDelegate(pressedIndex.column()));
                int entryIdx = delegate->hitTestEntry(pressedIndex, pressedPos);
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
        if (pressedIndex.isValid()) {
            QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
            item->setData(Qt::UserRole + 2, QVariant());
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
    if (row != 0 || column == 0) {
        return;
    }

    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData();

    ColumnDelegatingItemDelegate *delegatingDelegate = static_cast<ColumnDelegatingItemDelegate*>(itemDelegateForRow(row));
    CalendarDayViewDayDelegate *delegate = static_cast<CalendarDayViewDayDelegate*>(delegatingDelegate->getDelegate(column));
    int entryIdx = delegate->hitTestEntry(pressedIndex, pressedPos);

    CalendarEntry calEntry = pressedIndex.data(Qt::UserRole + 1).value<CalendarDay>().entries[entryIdx];
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
    item->setData(Qt::UserRole + 2, QVariant());
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
        && hoverIndex.row() == 0) {
        CalendarDay day = pressedIndex.data(Qt::UserRole + 1).value<CalendarDay>();
        QTime time = timeScaleData.timeFromY(pos.y(), visualRect(hoverIndex));
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
        if (   entryCoords[0] == 0
            && entryCoords[1] > 0
            && entryCoords[1] < columnCount()) {
            QModelIndex srcIndex = model()->index(entryCoords[0], entryCoords[1]);
            int entryIdx = entryCoords[2];
            if (srcIndex.isValid() && entryIdx >= 0) {
                CalendarDay srcDay = srcIndex.data(Qt::UserRole + 1).value<CalendarDay>();
                if (entryIdx < srcDay.entries.count()) {
#if QT_VERSION < 0x060000
                    QPoint pos = event->pos();
#else
                    QPoint pos = event->position().toPoint();
#endif
                    QModelIndex destIndex = indexAt(pos);
                    QTime time = timeScaleData.timeFromY(pos.y(), visualRect(destIndex));
                    CalendarDay destDay = destIndex.data(Qt::UserRole + 1).value<CalendarDay>();
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
    int row = index.row();
    int column = index.column();
    if (   ! index.isValid()
        || row != 0
        || column == 0) {
        if (pressedIndex.isValid()) {
            QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
            item->setData(Qt::UserRole + 2, QVariant());
        }
        return;
    }
    ColumnDelegatingItemDelegate *delegatingDelegate = static_cast<ColumnDelegatingItemDelegate*>(itemDelegateForRow(row));
    CalendarDayViewDayDelegate *delegate = static_cast<CalendarDayViewDayDelegate*>(delegatingDelegate->getDelegate(column));
    int entryIdx = delegate->hitTestEntry(index, pos);
    CalendarDay day = index.data(Qt::UserRole + 1).value<CalendarDay>();

    QMenu contextMenu(this);
    if (entryIdx >= 0) {
        CalendarEntry calEntry = day.entries[entryIdx];
        switch (calEntry.type) {
        case ENTRY_TYPE_ACTIVITY:
            contextMenu.addAction("View activity...", this, [=]() {
                emit viewActivity(calEntry);
            });
            contextMenu.addAction("Delete activity", this, [=]() {
                emit delActivity(calEntry);
            });
            break;
        case ENTRY_TYPE_PLANNED_ACTIVITY:
            if (calEntry.hasTrainMode) {
                contextMenu.addAction("Show in Train Mode...", this, [=]() {
                    emit showInTrainMode(calEntry);
                });
            }
            contextMenu.addAction("View planned activity...", this, [=]() {
                emit viewActivity(calEntry);
            });
            contextMenu.addAction("Delete planned activity", this, [=]() {
                emit delActivity(calEntry);
            });
            break;
        case ENTRY_TYPE_OTHER:
        default:
            break;
        }
    } else {
        QTime time = timeScaleData.timeFromY(pos.y(), visualRect(index));
        if (   day.date < QDate::currentDate()
            || (   day.date == QDate::currentDate()
                && time < QTime::currentTime())) {
            contextMenu.addAction("Add activity...", this, [=]() {
                emit addActivity(false, day.date, time);
            });
        } else {
            contextMenu.addAction("Add planned activity...", this, [=]() {
                emit addActivity(true, day.date, time);
            });
        }
    }
    contextMenu.exec(viewport()->mapToGlobal(pos));
    if (pressedIndex.isValid()) {
        QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
        item->setData(Qt::UserRole + 2, QVariant());
    }
}


void
CalendarDayTable::setDropIndicator
(int y, BlockIndicator block)
{
    QTableWidgetItem *scaleItem = item(0, 0);
    scaleItem->setData(Qt::UserRole, y);
    scaleItem->setData(Qt::UserRole + 1, static_cast<int>(block));
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
        setItemDelegateForColumn(i, new CalendarDayDelegate(this));
    }

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &CalendarMonthTable::customContextMenuRequested, this, &CalendarMonthTable::showContextMenu);
    connect(this, &QTableWidget::itemSelectionChanged, [=]() {
        QList<QTableWidgetItem*> selection = selectedItems();
        if (selection.count() > 0) {
            QTableWidgetItem *item = selection[0];
            if (item->column() < 7) {
                emit daySelected(item->data(Qt::UserRole + 1).value<CalendarDay>());
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
    if (! isInDateRange(dateInMonth) && ! allowKeepMonth) {
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
        item->setData(Qt::UserRole, date);
        bool isDimmed = ! dr.pass(date);
        CalendarDay day;
        day.date = date;
        day.isDimmed = isDimmed;
        item->setData(Qt::UserRole + 1, QVariant::fromValue(day));

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
        CalendarDay day = item->data(Qt::UserRole + 1).value<CalendarDay>();
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
        item->setData(Qt::UserRole + 1, QVariant::fromValue(day));
    }

    for (int row = 0; row < rowCount() && row < summaries.count(); ++row) {
        QTableWidgetItem *summaryItem = new QTableWidgetItem();
        summaryItem->setData(Qt::UserRole, QVariant::fromValue(summaries[row]));
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
        return item->data(Qt::UserRole).toDate();
    }
    return firstOfMonth;
}


void
CalendarMonthTable::limitDateRange
(const DateRange &dr, bool allowKeepMonth)
{
    if (dr.from.isValid() && dr.to.isValid() && dr.from > dr.to) {
        return;
    }
    this->dr = dr;
    if (currentItem() != nullptr && isInDateRange(currentItem()->data(Qt::UserRole).toDate())) {
        setMonth(currentItem()->data(Qt::UserRole).toDate());
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
        CalendarDayDelegate *delegate = static_cast<CalendarDayDelegate*>(abstractDelegate);
        CalendarDay day = index.data(Qt::UserRole + 1).value<CalendarDay>();
        if (delegate->hitTestMore(index, event->pos())) {
            emit moreDblClicked(day);
        } else {
            int entryIdx = delegate->hitTestEntry(index, event->pos());
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
        CalendarDayDelegate *delegate = static_cast<CalendarDayDelegate*>(abstractDelegate);
        if (! delegate->hitTestMore(pressedIndex, pressedPos)) {
            int entryIdx = delegate->hitTestEntry(pressedIndex, pressedPos);
            if (pressedIndex.isValid()) {
                QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
                item->setData(Qt::UserRole + 2, entryIdx);
            }
            CalendarDay day = pressedIndex.data(Qt::UserRole + 1).value<CalendarDay>();
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
                CalendarDay day = pressedIndex.data(Qt::UserRole + 1).value<CalendarDay>();
                QAbstractItemDelegate *abstractDelegate = itemDelegateForIndex(releasedIndex);
                CalendarDayDelegate *delegate = static_cast<CalendarDayDelegate*>(abstractDelegate);
                if (delegate->hitTestMore(releasedIndex, event->pos())) {
                    emit moreClicked(day);
                } else {
                    int entryIdx = delegate->hitTestEntry(releasedIndex, event->pos());
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
        selectDay(pressedIndex.data(Qt::UserRole).toDate());
        if (pressedIndex.isValid()) {
            QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
            item->setData(Qt::UserRole + 2, QVariant());
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
    CalendarDayDelegate *delegate = static_cast<CalendarDayDelegate*>(abstractDelegate);
    int entryIdx = delegate->hitTestEntry(pressedIndex, pressedPos);

    CalendarEntry calEntry = pressedIndex.data(Qt::UserRole + 1).value<CalendarDay>().entries[entryIdx];
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
        item->setData(Qt::UserRole + 2, QVariant());
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
                CalendarDay srcDay = srcIndex.data(Qt::UserRole + 1).value<CalendarDay>();
                if (entryIdx < srcDay.entries.count()) {
#if QT_VERSION < 0x060000
                    QModelIndex destIndex = indexAt(event->pos());
#else
                    QModelIndex destIndex = indexAt(event->position().toPoint());
#endif
                    CalendarDay destDay = destIndex.data(Qt::UserRole + 1).value<CalendarDay>();
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
            item->setData(Qt::UserRole + 2, QVariant());
        }
        return;
    }
    QAbstractItemDelegate *abstractDelegate = itemDelegateForIndex(index);
    CalendarDayDelegate *delegate = static_cast<CalendarDayDelegate*>(abstractDelegate);
    if (delegate->hitTestMore(index, pos)) {
        return;
    }
    int entryIdx = delegate->hitTestEntry(index, pos);
    CalendarDay day = index.data(Qt::UserRole + 1).value<CalendarDay>();

    QMenu contextMenu(this);
    if (entryIdx >= 0) {
        CalendarEntry calEntry = day.entries[entryIdx];
        switch (calEntry.type) {
        case ENTRY_TYPE_ACTIVITY:
            contextMenu.addAction("View activity...", this, [=]() {
                emit viewActivity(calEntry);
            });
            contextMenu.addAction("Delete activity", this, [=]() {
                emit delActivity(calEntry);
            });
            break;
        case ENTRY_TYPE_PLANNED_ACTIVITY:
            if (calEntry.hasTrainMode) {
                contextMenu.addAction("Show in Train Mode...", this, [=]() {
                    emit showInTrainMode(calEntry);
                });
            }
            contextMenu.addAction("View planned activity...", this, [=]() {
                emit viewActivity(calEntry);
            });
            contextMenu.addAction("Delete planned activity", this, [=]() {
                emit delActivity(calEntry);
            });
            break;
        case ENTRY_TYPE_OTHER:
        default:
            break;
        }
    } else {
        if (day.date <= QDate::currentDate()) {
            contextMenu.addAction("Add activity...", this, [=]() {
                QTime time = QTime::currentTime();
                if (day.date == QDate::currentDate()) {
                    time = time.addSecs(std::max(-4 * 3600, time.secsTo(QTime(0, 0))));
                }
                emit addActivity(false, day.date, time);
            });
        }
        if (day.date >= QDate::currentDate()) {
            contextMenu.addAction("Add planned activity...", this, [=]() {
                QTime time = QTime::currentTime();
                if (day.date == QDate::currentDate()) {
                    time = time.addSecs(std::min(4 * 3600, time.secsTo(QTime(23, 59, 59))));
                }
                emit addActivity(true, day.date, time);
            });
            contextMenu.addAction("Repeat schedule...", this, [=]() {
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
                contextMenu.addAction("Insert restday", this, [=]() {
                    emit insertRestday(day.date);
                });
            } else {
                contextMenu.addAction("Delete restday", this, [=]() {
                    emit delRestday(day.date);
                });
            }
        }
    }
    contextMenu.exec(viewport()->mapToGlobal(pos));
    if (pressedIndex.isValid()) {
        QTableWidgetItem *item = this->item(pressedIndex.row(), pressedIndex.column());
        item->setData(Qt::UserRole + 2, QVariant());
    }
}


//////////////////////////////////////////////////////////////////////////////
// CalendarDayView

CalendarDayView::CalendarDayView
(const QDate &dateInMonth, Qt::DayOfWeek firstDayOfWeek, Measures * const athleteMeasures, QWidget *parent)
: QWidget(parent), athleteMeasures(athleteMeasures)
{
    dayDateSelector = new CalendarOverview();

    dayPhaseLabel = new QLabel(tr("No Phase"));
    dayPhaseLabel->setWordWrap(true);
    dayEventLabel = new QLabel(tr("No Event"));
    dayEventLabel->setWordWrap(true);
    measureTabs = new QTabWidget();

    QWidget *dayLeftPane = new QWidget();
    QVBoxLayout *leftPaneLayout = new QVBoxLayout(dayLeftPane);
    leftPaneLayout->addWidget(dayDateSelector);
    leftPaneLayout->addWidget(dayPhaseLabel);
    leftPaneLayout->addWidget(dayEventLabel);
    leftPaneLayout->addWidget(measureTabs, 1);

    dayTable = new CalendarDayTable(dateInMonth);

    QHBoxLayout *dayLayout = new QHBoxLayout(this);
    dayLayout->addWidget(dayLeftPane, 1);
    dayLayout->addWidget(dayTable, 3);

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
CalendarDayView::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries)
{
    QDate dayViewDate = selectedDate();
    dayDateSelector->fillEntries(activityEntries, headlineEntries);
    QList<CalendarEntry> todayHeadline = headlineEntries.value(dayViewDate);
    QStringList phaseList;
    QStringList eventList;
    for (const CalendarEntry &entry : todayHeadline) {
        if (entry.type == ENTRY_TYPE_EVENT) {
            eventList << entry.primary;
        } else if (entry.type == ENTRY_TYPE_PHASE) {
            phaseList << entry.primary;
        }
    }
    if (phaseList.count() == 0) {
        dayPhaseLabel->setText(tr("No Phase"));
    } else if (phaseList.count() == 1) {
        dayPhaseLabel->setText(tr("Phase: %1").arg(phaseList.join(", ")));
    } else {
        dayPhaseLabel->setText(tr("Phases: %1").arg(phaseList.join(", ")));
    }
    if (eventList.count() == 0) {
        dayEventLabel->setText(tr("No Event"));
    } else if (eventList.count() == 1) {
        dayEventLabel->setText(tr("Event: %1").arg(eventList.join(", ")));
    } else {
        dayEventLabel->setText(tr("Events: %1").arg(eventList.join(", ")));
    }
    dayTable->fillEntries(activityEntries.value(dayViewDate), summaries.value(0), headlineEntries.value(dayViewDate));
}


void
CalendarDayView::limitDateRange
(const DateRange &dr)
{
    dayDateSelector->limitDateRange(dr);
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
            QTextEdit *commentField = new QTextEdit();
            commentField->setAcceptRichText(false);
            commentField->setReadOnly(true);
            commentField->setText(measure.comment);
            form->addRow(tr("Comment"), commentField);
            QLocale locale;
            QString validText = locale.toString(measure.when, QLocale::ShortFormat);
            int validDays = measure.when.date().daysTo(date);
            if (validDays > 1) {
                validText.append(tr(" (%1 days earlier)").arg(validDays));
            } else if (validDays > 0) {
                validText.append(tr(" (%1 day earlier)").arg(validDays));
            }
            form->addRow(tr("Valid since"), new QLabel(validText));
            measureLayout->addLayout(form);
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
// Calendar

Calendar::Calendar
(const QDate &dateInMonth, Qt::DayOfWeek firstDayOfWeek, Measures * const athleteMeasures, QWidget *parent)
: QWidget(parent)
{
    qRegisterMetaType<CalendarDay>("CalendarDay");
    qRegisterMetaType<CalendarSummary>("CalendarSummary");

    dayView = new CalendarDayView(dateInMonth, firstDayOfWeek, athleteMeasures);
    monthView = new CalendarMonthTable(dateInMonth, firstDayOfWeek);

    viewStack = new QStackedWidget();
    viewStack->addWidget(dayView);
    viewStack->addWidget(monthView);

    QToolBar *toolbar = new QToolBar();

    prevYAction = toolbar->addAction(tr("Previous Year"));
    prevMAction = toolbar->addAction(tr("Previous Month"));
    nextMAction = toolbar->addAction(tr("Next Month"));
    nextYAction = toolbar->addAction(tr("Next Year"));
    todayAction = toolbar->addAction(tr("Today"));
    toolbar->addSeparator();

    dateLabel = new QLabel();
    dateLabel->setAlignment(Qt::AlignRight);
    toolbar->addWidget(dateLabel);

    QWidget *stretch = new QWidget();
    stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(stretch);

    QActionGroup *viewGroup = new QActionGroup(toolbar);
    dayAction = toolbar->addAction(tr("Day"));
    dayAction->setCheckable(true);
    dayAction->setActionGroup(viewGroup);
    connect(dayAction, &QAction::triggered, [=]() { setView(CalendarView::Day); });

    monthAction = toolbar->addAction(tr("Month"));
    monthAction->setCheckable(true);
    monthAction->setActionGroup(viewGroup);
    connect(monthAction, &QAction::triggered, [=]() { setView(CalendarView::Month); });

    applyNavIcons();

    connect(dayView, &CalendarDayView::dayChanged, [=](const QDate &date) {
        QLocale locale;
        dateLabel->setText(locale.toString(dayView->selectedDate(), QLocale::LongFormat));
        emit dayChanged(date);
    });
    connect(dayView, &CalendarDayView::viewActivity, this, &Calendar::viewActivity);
    connect(dayView, &CalendarDayView::addActivity, this, &Calendar::addActivity);
    connect(dayView, &CalendarDayView::showInTrainMode, this, &Calendar::showInTrainMode);
    connect(dayView, &CalendarDayView::delActivity, this, &Calendar::delActivity);
    connect(dayView, &CalendarDayView::entryMoved, this, &Calendar::moveActivity);
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
    connect(monthView, &CalendarMonthTable::insertRestday, this, &Calendar::insertRestday);
    connect(monthView, &CalendarMonthTable::delRestday, this, &Calendar::delRestday);
    connect(monthView, &CalendarMonthTable::monthChanged, [=](const QDate &month, const QDate &firstVisible, const QDate &lastVisible) {
        setNavButtonState();
        QLocale locale;
        dateLabel->setText(locale.toString(monthView->firstOfCurrentMonth(), ("MMMM yyyy")));
        emit monthChanged(month, firstVisible, lastVisible);
    });
    connect(prevYAction, &QAction::triggered, [=]() { addYears(-1); });
    connect(prevMAction, &QAction::triggered, [=]() { addMonths(-1); });
    connect(nextMAction, &QAction::triggered, [=]() { addMonths(1); });
    connect(nextYAction, &QAction::triggered, [=]() { addYears(1); });
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
        dayView->setDay(date);
    } else {
        if (monthView->isInDateRange(date)) {
            monthView->setMonth(date, allowKeepMonth);
        }
    }
}


void
Calendar::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries)
{
    if (currentView() == CalendarView::Day) {
        dayView->fillEntries(activityEntries, summaries, headlineEntries);
    } else {
        monthView->fillEntries(activityEntries, summaries, headlineEntries);
    }
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
    } else {
        return monthView->firstVisibleDay();
    }
}


QDate
Calendar::lastVisibleDay
() const
{
    if (currentView() == CalendarView::Day) {
        return dayView->lastVisibleDay();
    } else {
        return monthView->lastVisibleDay();
    }
}


QDate
Calendar::selectedDate
() const
{
    if (currentView() == CalendarView::Day) {
        return dayView->selectedDate();
    } else {
        return monthView->selectedDate();
    }
}


CalendarView
Calendar::currentView
() const
{
    return static_cast<CalendarView>(viewStack->currentIndex());
}


bool
Calendar::addMonths
(int months)
{
    QDate newDate = fitDate(selectedDate().addMonths(months), currentView() == CalendarView::Month);
    if (newDate.isValid()) {
        setDate(newDate);
        return true;
    }
    return false;
}


bool
Calendar::addYears
(int years)
{
    QDate newDate = fitDate(selectedDate().addYears(years), currentView() == CalendarView::Month);
    if (newDate.isValid()) {
        setDate(newDate);
        return true;
    }
    return false;
}


QDate
Calendar::fitDate
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
Calendar::canAddMonths
(int months) const
{
    if (currentView() == CalendarView::Day) {
        return isInDateRange(dayView->selectedDate().addMonths(months));
    } else {
        QDate fom = monthView->firstOfCurrentMonth();
        QDate lom(fom.year(), fom.month(), fom.daysInMonth());
        fom = fom.addMonths(months);
        lom = lom.addMonths(months);
        return isInDateRange(fom) || isInDateRange(lom);
    }
}


bool
Calendar::canAddYears
(int years) const
{
    if (currentView() == CalendarView::Day) {
        return isInDateRange(dayView->selectedDate().addYears(years));
    } else {
        QDate fom = monthView->firstOfCurrentMonth();
        QDate lom(fom.year(), fom.month(), fom.daysInMonth());
        fom = fom.addYears(years);
        lom = lom.addYears(years);
        return isInDateRange(fom) || isInDateRange(lom);
    }
}


bool
Calendar::isInDateRange
(const QDate &date) const
{
    return date.isValid() && dateRange.pass(date);
}


void
Calendar::activateDateRange
(const DateRange &dr, bool allowKeepMonth)
{
    dateRange = dr;
    monthView->limitDateRange(dr, allowKeepMonth);
    dayView->limitDateRange(dr);
    setNavButtonState();
    setDate(fitDate(selectedDate(), true));
    emit dateRangeActivated(dr.name);
}


void
Calendar::setFirstDayOfWeek
(Qt::DayOfWeek firstDayOfWeek)
{
    QDate currentDate = selectedDate();
    monthView->setFirstDayOfWeek(firstDayOfWeek);
    dayView->setFirstDayOfWeek(firstDayOfWeek);
    if (currentView() == CalendarView::Month) {
        setDate(fitDate(currentDate, false), true);
    }
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
    prevYAction->setEnabled(canAddYears(-1));
    prevMAction->setEnabled(canAddMonths(-1));
    nextMAction->setEnabled(canAddMonths(1));
    nextYAction->setEnabled(canAddYears(1));
    todayAction->setEnabled(isInDateRange(QDate::currentDate()));
}


void
Calendar::applyNavIcons
()
{
    double scale = appsettings->value(this, GC_FONT_SCALE, 1.0).toDouble();
    QFont font;
    font.setPointSize(font.pointSizeF() * scale * 1.3);
    font.setWeight(QFont::Bold);
    dateLabel->setFont(font);
    QString mode = GCColor::isPaletteDark(palette()) ? "dark" : "light";

    prevYAction->setIcon(QIcon(QString(":images/breeze/%1/go-previous-skip.svg").arg(mode)));
    prevMAction->setIcon(QIcon(QString(":images/breeze/%1/go-previous.svg").arg(mode)));
    nextMAction->setIcon(QIcon(QString(":images/breeze/%1/go-next.svg").arg(mode)));
    nextYAction->setIcon(QIcon(QString(":images/breeze/%1/go-next-skip.svg").arg(mode)));
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
        if (view == CalendarView::Day) {
            dayAction->setChecked(true);
            dayView->setDay(monthView->selectedDate());
        } else {
            monthAction->setChecked(true);
            monthView->setMonth(fitDate(dayView->selectedDate(), false), true);
        }
        viewStack->setCurrentIndex(idx);
        emit viewChanged(view, static_cast<CalendarView>(oldIdx));
    }
}
