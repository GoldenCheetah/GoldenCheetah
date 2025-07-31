#include "Calendar.h"

#include <QHeaderView>
#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QMenu>
#include <QHash>
#include <QDrag>
#include <QMimeData>
#include <QDebug>
#if QT_VERSION < 0x060000
#include <QAbstractItemDelegate>
#endif

#include "CalendarItemDelegates.h"


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
    setItemDelegateForColumn(7, new CalendarSummaryDelegate(this));
    for (int i = 0; i < 7; ++i) {
        setItemDelegateForColumn(i, new CalendarDayDelegate(this));
    }

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &CalendarMonthTable::customContextMenuRequested, this, &CalendarMonthTable::showContextMenu);
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
CalendarMonthTable::addMonths
(int months)
{
    if (canAddMonths(months)) {
        return setMonth(firstOfMonth.addMonths(months));
    }
    return false;
}


bool
CalendarMonthTable::addYears
(int years)
{
    if (canAddYears(years)) {
        return setMonth(firstOfMonth.addYears(years));
    }
    return false;
}


bool
CalendarMonthTable::canAddMonths
(int months) const
{
    QDate fom = firstOfMonth;
    QDate lom(fom.year(), fom.month(), fom.daysInMonth());
    fom = fom.addMonths(months);
    lom = lom.addMonths(months);
    return isInDateRange(fom) || isInDateRange(lom);
}


bool
CalendarMonthTable::canAddYears
(int years) const
{
    QDate fom = firstOfMonth;
    QDate lom(fom.year(), fom.month(), fom.daysInMonth());
    fom = fom.addYears(years);
    lom = lom.addYears(years);
    return isInDateRange(fom) || isInDateRange(lom);
}


bool
CalendarMonthTable::isInDateRange
(const QDate &date) const
{
    return dr.pass(date);
}


void
CalendarMonthTable::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries)
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

    // Summary column
    for (int row = 0; row < rowCount(); ++row) {
        CalendarWeeklySummary summary;
        summary.entriesByType.clear();
        for (int col = 0; col < 7; ++col) {
            QTableWidgetItem *cell = item(row, col);
            if (! cell) {
                continue;
            }
            CalendarDay day = cell->data(Qt::UserRole + 1).value<CalendarDay>();
            if (col == 0) {
                summary.firstDayOfWeek = day.date;
            }
            for (CalendarEntry calEntry : day.entries) {
                summary.entriesByType[calEntry.type] = summary.entriesByType.value(calEntry.type, 0) + 1;
                summary.entries << calEntry;
            }
        }

        QTableWidgetItem *summaryItem = new QTableWidgetItem();
        summaryItem->setData(Qt::UserRole, QVariant::fromValue(summary));
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
    QDate selectedDate = firstOfMonth;
    QTableWidgetItem *item = currentItem();
    if (item != nullptr) {
        selectedDate = item->data(Qt::UserRole).toDate();
    }
    if (isInDateRange(selectedDate)) {
        setMonth(selectedDate, allowKeepMonth);
    } else if (isInDateRange(QDate::currentDate())) {
        setMonth(QDate::currentDate());
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
    if (firstOfMonth.isValid()) {
        setMonth(firstOfMonth, true);
    }
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
    QTableWidget::mousePressEvent(event);
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

    QList<int> entryCoords = { pressedIndex.row(), pressedIndex.column(), entryIdx };
    QString entryStr = QString::fromStdString(std::accumulate(entryCoords.begin(),
                                                              entryCoords.end(),
                                                              std::string(),
                                                              [](const std::string &a, int b) {
                                                                  return a + (a.length() ? "," : "") + std::to_string(b);
                                                              }));

    QByteArray entryBytes = entryStr.toUtf8();
    mimeData->setData("application/x-month-grid-entry", entryBytes);
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
    if (event->mimeData()->hasFormat("application/x-month-grid-entry") && event->source() == this) {
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
    if (event->mimeData()->hasFormat("application/x-month-grid-entry")) {
        QByteArray entryBytes = event->mimeData()->data("application/x-month-grid-entry");
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
                    emit entryMoved(srcDay.entries[entryIdx], srcDay.date, destDay.date);
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
            contextMenu.addAction("View activity", this, [=]() {
                emit viewActivity(calEntry);
            });
            contextMenu.addAction("Delete activity", this, [=]() {
                emit delActivity(calEntry);
            });
            break;
        case ENTRY_TYPE_PLANNED_ACTIVITY:
            contextMenu.addAction("View planned activity", this, [=]() {
                emit viewActivity(calEntry);
            });
            contextMenu.addAction("Delete planned activity", this, [=]() {
                emit delActivity(calEntry);
            });
            break;
        case ENTRY_TYPE_OTHER:
            contextMenu.addAction(calEntry.name);
        default:
            break;
        }
    } else {
        if (day.date <= QDate::currentDate()) {
            contextMenu.addAction("Add activity", this, [=]() {
                QTime time = QTime::currentTime();
                if (day.date == QDate::currentDate()) {
                    time = time.addSecs(std::max(-4 * 3600, time.secsTo(QTime(0, 0))));
                }
                emit addActivity(false, day.date, time);
            });
        }
        if (day.date >= QDate::currentDate()) {
            contextMenu.addAction("Add planned activity", this, [=]() {
                QTime time = QTime::currentTime();
                if (day.date == QDate::currentDate()) {
                    time = time.addSecs(std::min(4 * 3600, time.secsTo(QTime(23, 59, 59))));
                }
                emit addActivity(true, day.date, time);
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


int
CalendarMonthTable::findEntry
(const QModelIndex &index, const QPoint &pos) const
{
    if (index.column() >= 7) {
        return -1;
    }
    QAbstractItemDelegate *abstractDelegate = itemDelegateForIndex(index);
    CalendarDayDelegate *delegate = static_cast<CalendarDayDelegate*>(abstractDelegate);
    return delegate->hitTestEntry(index, pos);
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
    qRegisterMetaType<CalendarWeeklySummary>("CalendarWeeklySummary");

    monthTable = new CalendarMonthTable(dateInMonth, firstDayOfWeek);

    prevYButton = new QPushButton("<<");
    prevYButton->setFlat(true);
    prevMButton = new QPushButton("<");
    prevMButton->setFlat(true);
    nextMButton = new QPushButton(">");
    nextMButton->setFlat(true);
    nextYButton = new QPushButton(">>");
    nextYButton->setFlat(true);
    dateLabel = new QLabel();
    todayButton = new QPushButton("Today");

    connect(monthTable, &CalendarMonthTable::dayClicked, [=](const CalendarDay &day) {
        qDebug() << __func__ << "dayClicked:" << day.date;
    });
    connect(monthTable, &CalendarMonthTable::moreClicked, [=](const CalendarDay &day) {
        qDebug() << __func__ << "moreClicked:" << day.date;
    });
    connect(monthTable, &CalendarMonthTable::entryClicked, [=](const CalendarDay &day, int entryIdx) {
        qDebug() << __func__ << "entryClicked:" << day.date << day.entries[entryIdx].name;
    });
    connect(monthTable, &CalendarMonthTable::dayRightClicked, [=](const CalendarDay &day) {
        qDebug() << __func__ << "dayRightClicked:" << day.date;
    });
    connect(monthTable, &CalendarMonthTable::entryRightClicked, [=](const CalendarDay &day, int entryIdx) {
        qDebug() << __func__ << "entryRightClicked:" << day.date << day.entries[entryIdx].name;
    });
    connect(monthTable, &CalendarMonthTable::dayDblClicked, [=](const CalendarDay &day) {
        qDebug() << __func__ << "dayDblClicked:" << day.date;
    });
    connect(monthTable, &CalendarMonthTable::moreDblClicked, [=](const CalendarDay &day) {
        qDebug() << __func__ << "moreDblClicked:" << day.date;
    });
    connect(monthTable, &CalendarMonthTable::entryDblClicked, [=](const CalendarDay &day, int entryIdx) {
        viewActivity(day.entries[entryIdx]);
    });

    connect(monthTable, &CalendarMonthTable::summaryClicked, [=](QModelIndex index) {
        qDebug() << __func__ << "summaryClicked:" << index.data(Qt::UserRole);
    });
    connect(monthTable, &CalendarMonthTable::summaryRightClicked, [=](QModelIndex index) {
        qDebug() << __func__ << "summaryRightClicked:" << index.data(Qt::UserRole);
    });
    connect(monthTable, &CalendarMonthTable::summaryDblClicked, [=](QModelIndex index) {
        qDebug() << __func__ << "summaryDblClicked:" << index.data(Qt::UserRole);
    });
    connect(monthTable, &CalendarMonthTable::viewActivity, this, &Calendar::viewActivity);
    connect(monthTable, &CalendarMonthTable::addActivity, this, &Calendar::addActivity);
    connect(monthTable, &CalendarMonthTable::delActivity, this, &Calendar::delActivity);
    connect(monthTable, &CalendarMonthTable::entryMoved, this, &Calendar::moveActivity);
    connect(monthTable, &CalendarMonthTable::insertRestday, this, &Calendar::insertRestday);
    connect(monthTable, &CalendarMonthTable::delRestday, this, &Calendar::delRestday);
    connect(monthTable, &CalendarMonthTable::monthChanged, [=](const QDate &month, const QDate &firstVisible, const QDate &lastVisible) {
        setNavButtonState();
        dateLabel->setText(QString("<h2>%1</h2>").arg(monthTable->firstOfCurrentMonth().toString("MMMM yyyy")));
        emit monthChanged(month, firstVisible, lastVisible);
    });
    connect(prevYButton, &QPushButton::clicked, [=]() {
        QDate lom(monthTable->firstOfCurrentMonth().year(), monthTable->firstOfCurrentMonth().month(), monthTable->firstOfCurrentMonth().daysInMonth());
        setMonth(lom.addYears(-1));
    });
    connect(prevMButton, &QPushButton::clicked, [=]() {
        QDate lom(monthTable->firstOfCurrentMonth().year(), monthTable->firstOfCurrentMonth().month(), monthTable->firstOfCurrentMonth().daysInMonth());
        setMonth(lom.addMonths(-1));
    });
    connect(nextMButton, &QPushButton::clicked, [=]() {
        setMonth(monthTable->firstOfCurrentMonth().addMonths(1));
    });
    connect(nextYButton, &QPushButton::clicked, [=]() {
        setMonth(monthTable->firstOfCurrentMonth().addYears(1));
    });
    connect(todayButton, &QPushButton::clicked, [=]() {
        setMonth(QDate::currentDate());
    });

    QHBoxLayout *navLayout = new QHBoxLayout();
    navLayout->addWidget(prevYButton);
    navLayout->addWidget(prevMButton);
    navLayout->addWidget(nextMButton);
    navLayout->addWidget(nextYButton);
    navLayout->addWidget(dateLabel);
    navLayout->addStretch();

    QHBoxLayout *subnavLayout = new QHBoxLayout();
    subnavLayout->addWidget(todayButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(navLayout);
    mainLayout->addLayout(subnavLayout);
    mainLayout->addWidget(monthTable);

    setMonth(dateInMonth);
}


void
Calendar::setMonth
(const QDate &dateInMonth, bool allowKeepMonth)
{
    if (monthTable->isInDateRange(dateInMonth)) {
        monthTable->setMonth(dateInMonth, allowKeepMonth);
    }
}


void
Calendar::fillEntries
(const QHash<QDate, QList<CalendarEntry>> &activityEntries, const QHash<QDate, QList<CalendarEntry>> &headlineEntries)
{
    QHash<QDate, QList<CalendarEntry>> activities = activityEntries;
    QDate firstVisible = monthTable->firstVisibleDay();
    QDate lastVisible = monthTable->lastVisibleDay();
    for (auto dayIt = activities.begin(); dayIt != activities.end(); ++dayIt) {
        if (dayIt.key() >= firstVisible && dayIt.key() <= lastVisible) {
            std::sort(dayIt.value().begin(), dayIt.value().end(), [](const CalendarEntry &a, const CalendarEntry &b) {
                if (a.start == b.start) {
                    return a.name < b.name;
                } else {
                    return a.start < b.start;
                }
            });
        }
    }
    this->headlineEntries = headlineEntries;
    monthTable->fillEntries(activities, headlineEntries);
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
    return monthTable->firstVisibleDay();
}


QDate
Calendar::lastVisibleDay
() const
{
    return monthTable->lastVisibleDay();
}


QDate
Calendar::selectedDate
() const
{
    return monthTable->selectedDate();
}


void
Calendar::activateDateRange
(const DateRange &dr, bool allowKeepMonth)
{
    headlineEntries.clear();
    for (auto dayIt = headlineEntries.begin(); dayIt != headlineEntries.end(); ++dayIt) {
        std::sort(dayIt.value().begin(), dayIt.value().end(), [](const CalendarEntry &a, const CalendarEntry &b) {
            if (a.start == b.start) {
                return a.name < b.name;
            } else {
                return a.start < b.start;
            }
        });
    }
    monthTable->limitDateRange(dr, allowKeepMonth);
    setNavButtonState();
    emit dateRangeActivated(dr.name);
}


void
Calendar::setFirstDayOfWeek
(Qt::DayOfWeek firstDayOfWeek)
{
    monthTable->setFirstDayOfWeek(firstDayOfWeek);
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
    QDate lom(monthTable->firstOfCurrentMonth().year(), monthTable->firstOfCurrentMonth().month(), monthTable->firstOfCurrentMonth().daysInMonth());
    prevYButton->setEnabled(monthTable->isInDateRange(lom.addYears(-1)));
    prevMButton->setEnabled(monthTable->isInDateRange(lom.addMonths(-1)));
    nextMButton->setEnabled(monthTable->isInDateRange(monthTable->firstOfCurrentMonth().addMonths(1)));
    nextYButton->setEnabled(monthTable->isInDateRange(monthTable->firstOfCurrentMonth().addYears(1)));
    todayButton->setEnabled(monthTable->isInDateRange(QDate::currentDate()));
}
