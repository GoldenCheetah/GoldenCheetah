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

    QStringList headers;
    headers << ""
            << tr("Activities");
    setHorizontalHeaderLabels(headers);

    horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
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
// Calendar


Calendar::Calendar
(Qt::DayOfWeek firstDayOfWeek, QWidget *parent)
: Calendar(QDate::currentDate(), firstDayOfWeek, parent)
{
}


Calendar::Calendar
(const QDate &dateInMonth, Qt::DayOfWeek firstDayOfWeek, QWidget *parent)
: QWidget(parent)
{
    qRegisterMetaType<CalendarDay>("CalendarDay");
    qRegisterMetaType<CalendarSummary>("CalendarSummary");

    dayDateSelector = new CalendarOverview();

    dayPhaseLabel = new QLabel(tr("No Phase"));
    dayPhaseLabel->setWordWrap(true);
    dayEventLabel = new QLabel(tr("No Event"));
    dayEventLabel->setWordWrap(true);

    QWidget *dayLeftPane = new QWidget();
    QVBoxLayout *leftPaneLayout = new QVBoxLayout(dayLeftPane);
    leftPaneLayout->addWidget(dayDateSelector);
    leftPaneLayout->addWidget(dayPhaseLabel);
    leftPaneLayout->addWidget(dayEventLabel);
    leftPaneLayout->addStretch();

    dayTable = new CalendarDayTable(dateInMonth);

    dayCalendar = new QWidget();
    QHBoxLayout *dayLayout = new QHBoxLayout(dayCalendar);
    dayLayout->addWidget(dayLeftPane, 1);
    dayLayout->addWidget(dayTable, 3);

    monthTable = new CalendarMonthTable(dateInMonth, firstDayOfWeek);

    tableStack = new QStackedWidget();
    tableStack->addWidget(dayCalendar);
    tableStack->addWidget(monthTable);

    QToolBar *toolbar = new QToolBar();

    prevYAction = toolbar->addAction("Previous Year");
    prevMAction = toolbar->addAction("Previous Month");
    nextMAction = toolbar->addAction("Next Month");
    nextYAction = toolbar->addAction("Next Year");
    todayAction = toolbar->addAction("Today");
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

    connect(dayDateSelector, &QCalendarWidget::selectionChanged, [=]() {
        if (dayTable->selectedDate() != dayDateSelector->selectedDate()) {
            dayTable->setDay(dayDateSelector->selectedDate());
        }
    });
    connect(dayTable, &CalendarDayTable::dayChanged, [=](const QDate &date) {
        QLocale locale;
        dateLabel->setText(locale.toString(dayTable->selectedDate(), QLocale::LongFormat));
        dayDateSelector->setSelectedDate(date);
        emit dayChanged(date);
    });
    connect(dayTable, &CalendarDayTable::viewActivity, this, &Calendar::viewActivity);
    connect(dayTable, &CalendarDayTable::addActivity, this, &Calendar::addActivity);
    connect(dayTable, &CalendarDayTable::showInTrainMode, this, &Calendar::showInTrainMode);
    connect(dayTable, &CalendarDayTable::delActivity, this, &Calendar::delActivity);
    connect(dayTable, &CalendarDayTable::entryMoved, this, &Calendar::moveActivity);
    connect(monthTable, &CalendarMonthTable::entryDblClicked, [=](const CalendarDay &day, int entryIdx) {
        viewActivity(day.entries[entryIdx]);
    });
    connect(monthTable, &CalendarMonthTable::daySelected, [=](const CalendarDay &day) {
        emit daySelected(day.date);
    });
    connect(monthTable, &CalendarMonthTable::moreClicked, [=]() {
        setView(CalendarView::Day);
    });
    connect(monthTable, &CalendarMonthTable::dayDblClicked, [=]() {
        setView(CalendarView::Day);
    });
    connect(monthTable, &CalendarMonthTable::showInTrainMode, this, &Calendar::showInTrainMode);
    connect(monthTable, &CalendarMonthTable::viewActivity, this, &Calendar::viewActivity);
    connect(monthTable, &CalendarMonthTable::addActivity, this, &Calendar::addActivity);
    connect(monthTable, &CalendarMonthTable::repeatSchedule, this, &Calendar::repeatSchedule);
    connect(monthTable, &CalendarMonthTable::delActivity, this, &Calendar::delActivity);
    connect(monthTable, &CalendarMonthTable::entryMoved, this, &Calendar::moveActivity);
    connect(monthTable, &CalendarMonthTable::insertRestday, this, &Calendar::insertRestday);
    connect(monthTable, &CalendarMonthTable::delRestday, this, &Calendar::delRestday);
    connect(monthTable, &CalendarMonthTable::monthChanged, [=](const QDate &month, const QDate &firstVisible, const QDate &lastVisible) {
        setNavButtonState();
        QLocale locale;
        dateLabel->setText(locale.toString(monthTable->firstOfCurrentMonth(), ("MMMM yyyy")));
        emit monthChanged(month, firstVisible, lastVisible);
    });
    connect(prevYAction, &QAction::triggered, [=]() { addYears(-1); });
    connect(prevMAction, &QAction::triggered, [=]() { addMonths(-1); });
    connect(nextMAction, &QAction::triggered, [=]() { addMonths(1); });
    connect(nextYAction, &QAction::triggered, [=]() { addYears(1); });
    connect(todayAction, &QAction::triggered, [=]() { setDate(QDate::currentDate()); });

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(tableStack);

    setView(CalendarView::Month);
    setDate(dateInMonth);
}


void
Calendar::setDate
(const QDate &date, bool allowKeepMonth)
{
    if (currentView() == CalendarView::Day) {
        dayTable->setDay(date);
    } else {
        if (monthTable->isInDateRange(date)) {
            monthTable->setMonth(date, allowKeepMonth);
        }
    }
}


void
Calendar::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QList<CalendarSummary> &summaries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries)
{
    if (currentView() == CalendarView::Day) {
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
    } else {
        monthTable->fillEntries(activityEntries, summaries, headlineEntries);
    }
}


QDate
Calendar::firstOfCurrentMonth
() const
{
    return monthTable->firstOfCurrentMonth();
}


QDate
Calendar::firstVisibleDay
() const
{
    if (currentView() == CalendarView::Day) {
        return dayDateSelector->firstVisibleDay();
    } else {
        return monthTable->firstVisibleDay();
    }
}


QDate
Calendar::lastVisibleDay
() const
{
    if (currentView() == CalendarView::Day) {
        return dayDateSelector->lastVisibleDay();
    } else {
        return monthTable->lastVisibleDay();
    }
}


QDate
Calendar::selectedDate
() const
{
    if (currentView() == CalendarView::Day) {
        return dayTable->selectedDate();
    } else {
        return monthTable->selectedDate();
    }
}


CalendarView
Calendar::currentView
() const
{
    return static_cast<CalendarView>(tableStack->currentIndex());
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
        return isInDateRange(dayTable->selectedDate().addMonths(months));
    } else {
        QDate fom = monthTable->firstOfCurrentMonth();
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
        return isInDateRange(dayTable->selectedDate().addYears(years));
    } else {
        QDate fom = monthTable->firstOfCurrentMonth();
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
    monthTable->limitDateRange(dr, allowKeepMonth);
    dayDateSelector->limitDateRange(dr);
    setNavButtonState();
    setDate(fitDate(selectedDate(), true));
    emit dateRangeActivated(dr.name);
}


void
Calendar::setFirstDayOfWeek
(Qt::DayOfWeek firstDayOfWeek)
{
    QDate currentDate = selectedDate();
    monthTable->setFirstDayOfWeek(firstDayOfWeek);
    dayDateSelector->setFirstDayOfWeek(firstDayOfWeek);
    if (currentView() == CalendarView::Month) {
        setDate(fitDate(currentDate, false), true);
    }
}


void
Calendar::setSummaryMonthVisible
(bool visible)
{
    monthTable->setColumnHidden(7, ! visible);
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
Calendar::setView
(CalendarView view)
{
    int idx = static_cast<int>(view);
    int oldIdx = tableStack->currentIndex();
    if (idx != oldIdx) {
        if (view == CalendarView::Day) {
            dayAction->setChecked(true);
            dayTable->setDay(monthTable->selectedDate());
        } else {
            monthAction->setChecked(true);
            monthTable->setMonth(fitDate(dayTable->selectedDate(), false), true);
        }
        tableStack->setCurrentIndex(idx);
        emit viewChanged(view, static_cast<CalendarView>(oldIdx));
    }
}
