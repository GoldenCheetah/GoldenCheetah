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

#include "CalendarItemDelegates.h"

#include <QDate>
#include <QHelpEvent>
#include <QAbstractItemView>
#include <QPainter>
#include <QToolTip>
#include <QPixmap>
#include <QSvgRenderer>
#include <QPainterPath>

#include "CalendarData.h"
#include "Colors.h"


//////////////////////////////////////////////////////////////////////////////
// TimeScaleData

TimeScaleData::TimeScaleData
()
{
}

void
TimeScaleData::setFirstMinute
(int minute)
{
    _firstMinute = minute;
}


int
TimeScaleData::firstMinute
() const
{
    return _firstMinute;
}


void
TimeScaleData::setLastMinute
(int minute)
{
    _lastMinute = minute;
}


int
TimeScaleData::lastMinute
() const
{
    return _lastMinute;
}


void
TimeScaleData::setStepSize
(int step)
{
    _stepSize = step;
}


int
TimeScaleData::stepSize
() const
{
    return _stepSize;
}


double
TimeScaleData::pixelsPerMinute
(int availableHeight) const
{
    int totalMinutes = _lastMinute - _firstMinute;
    if (totalMinutes <= 0) {
        return 0.0;
    }
    return static_cast<double>(availableHeight) / totalMinutes;
}


double
TimeScaleData::pixelsPerMinute
(const QRect& rect) const
{
    return pixelsPerMinute(rect.height());
}


int
TimeScaleData::minuteToY
(int minute, int rectTop, int rectHeight) const
{
    if (minute < _firstMinute) {
        return rectTop;
    }
    if (minute > _lastMinute) {
        return rectTop + rectHeight;
    }

    int totalMinutes = _lastMinute - _firstMinute;
    double pixelsPerMinute = static_cast<double>(rectHeight) / totalMinutes;
    return std::min(rectHeight, rectTop + static_cast<int>((minute - _firstMinute) * pixelsPerMinute));
}


int
TimeScaleData::minuteToY
(int minute, const QRect &rect) const
{
    return minuteToY(minute, rect.top(), rect.height());
}


int
TimeScaleData::minuteToY
(const QTime &time, const QRect& rect) const
{
    return minuteToY(time.hour() * 60 + time.minute(), rect.top(), rect.height());
}


QTime
TimeScaleData::timeFromY
(int y, const QRect &rect, int snap) const
{
    int s = std::max(1, snap);
    int minute = trunc((_firstMinute + y / pixelsPerMinute(rect)) / s) * s;
    return QTime(0, 0).addSecs(60 * minute);
}


//////////////////////////////////////////////////////////////////////////////
// CalendarDayViewDayDelegate

ColumnDelegatingItemDelegate::ColumnDelegatingItemDelegate
(QList<QStyledItemDelegate*> delegates, QObject *parent)
: QStyledItemDelegate(parent), delegates(delegates)
{
}


QStyledItemDelegate*
ColumnDelegatingItemDelegate::getDelegate
(int idx) const
{
    return delegates.value(idx, nullptr);
}


QWidget*
ColumnDelegatingItemDelegate::createEditor
(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate *delegate = delegates.value(index.column(), nullptr);
    if (delegate != nullptr) {
        return delegate->createEditor(parent, option, index);
    } else {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }
}


void
ColumnDelegatingItemDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate *delegate = delegates.value(index.column(), nullptr);
    if (delegate != nullptr) {
        delegate->paint(painter, option, index);
    } else {
        QStyledItemDelegate::paint(painter, option, index);
    }
}


bool
ColumnDelegatingItemDelegate::helpEvent
(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    QStyledItemDelegate *delegate = delegates.value(index.column(), nullptr);
    if (delegate != nullptr) {
        return delegate->helpEvent(event, view, option, index);
    } else {
        return QStyledItemDelegate::helpEvent(event, view, option, index);
    }
}


void
ColumnDelegatingItemDelegate::setEditorData
(QWidget *editor, const QModelIndex &index) const
{
    QStyledItemDelegate *delegate = delegates.value(index.column(), nullptr);
    if (delegate != nullptr) {
        return delegate->setEditorData(editor, index);
    } else {
        return QStyledItemDelegate::setEditorData(editor, index);
    }
}


void
ColumnDelegatingItemDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QStyledItemDelegate *delegate = delegates.value(index.column(), nullptr);
    if (delegate != nullptr) {
        delegate->setModelData(editor, model, index);
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}


QSize
ColumnDelegatingItemDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate *delegate = delegates.value(index.column(), nullptr);
    if (delegate != nullptr) {
        return delegate->sizeHint(option, index);
    } else {
        return QStyledItemDelegate::sizeHint(option, index);
    }
}


void
ColumnDelegatingItemDelegate::updateEditorGeometry
(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate *delegate = delegates.value(index.column(), nullptr);
    if (delegate != nullptr) {
        delegate->updateEditorGeometry(editor, option, index);
    } else {
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }
}


//////////////////////////////////////////////////////////////////////////////
// CalendarDayViewDayDelegate

CalendarDayViewDayDelegate::CalendarDayViewDayDelegate
(TimeScaleData const * const timeScaleData, QObject *parent)
: QStyledItemDelegate(parent), timeScaleData(timeScaleData)
{
}


void
CalendarDayViewDayDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.row() != 0) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    bool ok;
    int pressedEntryIdx = index.data(Qt::UserRole + 2).toInt(&ok);
    if (! ok) {
        pressedEntryIdx = -2;
    }
    QDate date = index.data(Qt::UserRole).toDate();
    CalendarDay calendarDay = index.data(Qt::UserRole + 1).value<CalendarDay>();
    QList<CalendarEntryLayout> layout = index.data(Qt::UserRole + 3).value<QList<CalendarEntryLayout>>();
#if QT_VERSION < 0x060000
    entryRects[index].clear();
    for (int i = 0; i < layout.count(); ++i) {
        entryRects[index] << QRect();
    }
#else
    entryRects[index].resize(layout.count());
#endif
    bool isToday = (date == QDate::currentDate());

    // Horizontal Lines
    painter->save();
    QPen linePen(Qt::lightGray, 1, Qt::DotLine);
    painter->setPen(linePen);
    for (int minute = timeScaleData->firstMinute(); minute <= timeScaleData->lastMinute(); minute += timeScaleData->stepSize()) {
        int y = timeScaleData->minuteToY(minute, option.rect);
        painter->drawLine(option.rect.left(), y, option.rect.right(), y);
    }
    painter->restore();

    // Activities
    const int columnSpacing = 10 * dpiXFactor;
    const int lineSpacing = 2 * dpiYFactor;
    const int iconSpacing = 2 * dpiXFactor;
    const int radius = 4 * dpiXFactor;
    const int horPadding = 4 * dpiXFactor;
    QFont entryFont = painter->font();
    entryFont.setPointSize(entryFont.pointSize() * 0.95);
    QFontMetrics entryFM(entryFont);
    int lineHeight = entryFM.height();
    painter->setFont(entryFont);

    int rectLeft = option.rect.left() + columnSpacing;
    int rectWidth = option.rect.width() - 2 * columnSpacing;
    for (const CalendarEntryLayout &l : layout) {
        if (calendarDay.entries.count() < l.entryIdx) {
            continue;
        }
        bool pressed = (l.entryIdx == pressedEntryIdx);
        const CalendarEntry &entry = calendarDay.entries[l.entryIdx];
        int startMinute = entry.start.hour() * 60 + entry.start.minute();
        int endMinute = startMinute + entry.durationSecs / 60;
        int columnWidth = (rectWidth - (l.columnCount - 1) * columnSpacing) / std::max(1, l.columnCount);
        int left = rectLeft + l.columnIndex * (columnWidth + columnSpacing);
        int top = timeScaleData->minuteToY(startMinute, option.rect);
        int bottom = timeScaleData->minuteToY(endMinute, option.rect);
        int height = std::max(1, bottom - top);
        int numLines = (height + lineSpacing) / (lineHeight + lineSpacing);

        painter->save();
        QRect entryRect(left, top, columnWidth, height);
        entryRects[index][l.entryIdx] = entryRect;
        QColor entryBG = option.palette.base().color();
        QColor entryFrame = entry.color;
        if (pressed) {
            entryBG = option.palette.highlight().color();
            entryFrame = entryBG;
        } else if (entry.type == ENTRY_TYPE_ACTIVITY) {
            QColor overlayBG = entry.color;
            overlayBG = entry.color;
            if (GCColor::isPaletteDark(option.palette)) {
                overlayBG.setAlpha(150);
            } else {
                overlayBG.setAlpha(60);
            }
            entryBG = GCColor::blendedColor(overlayBG, entryBG);
            entryFrame = entryBG;
        }
        painter->save();
        painter->setPen(entryFrame);
        painter->setBrush(entryBG);
        painter->drawRoundedRect(entryRect, radius, radius);
        painter->restore();

        int iconWidth = 0;
        if (height >= lineHeight && columnWidth >= lineHeight) {
            QColor pixmapColor(entry.color);
            int headlineOffset = 0;
            if (height >= 2 * lineHeight + lineSpacing && columnWidth >= 2 * lineHeight + lineSpacing) {
                iconWidth = 2 * lineHeight + lineSpacing;
            } else {
                iconWidth = lineHeight;
            }
            QSize pixmapSize(iconWidth, iconWidth);
            QPixmap pixmap;
            if (entry.type == ENTRY_TYPE_PLANNED_ACTIVITY) {
                if (pressed) {
                    pixmapColor = GCColor::invertColor(entryBG);
                }
                pixmap = svgAsColoredPixmap(entry.iconFile, pixmapSize, lineSpacing, pixmapColor);
            } else {
                pixmap = svgOnBackground(entry.iconFile, pixmapSize, lineSpacing, pixmapColor, radius);
            }
            painter->drawPixmap(left, top, pixmap);
        }
        if (numLines > 0) {
            painter->save();
            QRect textRect(left + iconWidth + horPadding, top, columnWidth - iconWidth - 2 * horPadding, lineHeight);
            painter->setPen(GCColor::invertColor(entryBG));
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, entry.primary);
            --numLines;
            if (numLines > 0) {
                textRect.translate(0, lineHeight + lineSpacing);
                painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, entry.secondary + " (" + entry.secondaryMetric + ")");
            }
            painter->restore();
        }
        painter->restore();
    }

    painter->restore();
}


bool
CalendarDayViewDayDelegate::helpEvent
(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (! event || ! view) {
        return false;
    }

    CalendarDay day = index.data(Qt::UserRole + 1).value<CalendarDay>();
    int entryIdx = hitTestEntry(index, event->pos());
    if (entryIdx >= 0 && entryIdx < day.entries.size()) {
        CalendarEntry calEntry = day.entries[entryIdx];
        QString tooltip = "<center><b>" + calEntry.primary + "</b>";
        tooltip += "<br/>";
        switch (calEntry.type) {
        case ENTRY_TYPE_ACTIVITY:
            tooltip += "[completed]";
            break;
        case ENTRY_TYPE_PLANNED_ACTIVITY:
            tooltip += "[planned]";
            break;
        case ENTRY_TYPE_OTHER:
        default:
            tooltip += "[other]";
            break;
        }
        tooltip += "<br/>";
        if (! calEntry.secondary.isEmpty()) {
            tooltip += calEntry.secondaryMetric + ": " + calEntry.secondary;
            tooltip += "<br/>";
        }
        if (calEntry.start.isValid()) {
            tooltip += calEntry.start.toString();
            if (calEntry.durationSecs > 0) {
                tooltip += " - " + calEntry.start.addSecs(calEntry.durationSecs).toString();
            }
        }
        tooltip += "</center>";
        QToolTip::showText(event->globalPos(), tooltip, view);
        return true;
    }

    return QStyledItemDelegate::helpEvent(event, view, option, index);
}


int
CalendarDayViewDayDelegate::hitTestEntry
(const QModelIndex &index, const QPoint &pos) const
{
    if (! entryRects.contains(index)) {
        return -1;
    }
    const QList<QRect> &rects = entryRects[index];
    for (int i = 0; i < rects.size(); ++i) {
        if (rects[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}


//////////////////////////////////////////////////////////////////////////////
// CalendarTimeScaleDelegate

CalendarTimeScaleDelegate::CalendarTimeScaleDelegate
(TimeScaleData *timeScaleData, QObject *parent)
: QStyledItemDelegate(parent), timeScaleData(timeScaleData)
{
}


void
CalendarTimeScaleDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.row() != 0) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    const int totalRangeMinutes = timeScaleData->lastMinute() - timeScaleData->firstMinute();
    const int cellHeight = option.rect.height();
    const double pixelsPerMinute = static_cast<double>(cellHeight) / totalRangeMinutes;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen linePen(Qt::lightGray, 1, Qt::DotLine);
    QPen textPen(Qt::darkGray);
    painter->setPen(textPen);

    QFont font = painter->font();
    font.setPointSizeF(font.pointSizeF() * 0.9);
    painter->setFont(font);
    QFontMetrics fm(font);

    int labelHeight = fm.height();
    int minPixelSpacing = labelHeight + 4;
    int calculatedMinInterval = std::ceil(minPixelSpacing / pixelsPerMinute);
    int effectiveInterval = selectNiceInterval(calculatedMinInterval);
    timeScaleData->setStepSize(effectiveInterval);

    int previousYBottom = -labelHeight;

    for (int minute = timeScaleData->firstMinute(); minute <= timeScaleData->lastMinute(); minute += timeScaleData->stepSize()) {
        int y = timeScaleData->minuteToY(minute, option.rect);
        QString timeLabel = QTime(0, 0).addSecs(minute * 60).toString("hh:mm");

        QRect textRect;
        if (minute == timeScaleData->firstMinute()) {
            textRect = QRect(option.rect.left() + 4, y, option.rect.width() - 8, labelHeight);
        } else if (minute + effectiveInterval > timeScaleData->lastMinute()) {
            int bottomY = option.rect.bottom();
            textRect = QRect(option.rect.left() + 4, bottomY - labelHeight, option.rect.width() - 8, labelHeight);
            y = bottomY;
        } else {
            textRect = QRect(option.rect.left() + 4, y - labelHeight / 2, option.rect.width() - 8, labelHeight);
        }

        if (textRect.top() < previousYBottom + 2) {
            continue;
        }

        painter->setPen(linePen);
        painter->drawLine(textRect.right(), y, option.rect.right(), y);

        painter->setPen(textPen);
        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, timeLabel);

        previousYBottom = textRect.bottom();
    }
    int blockY = -1;
    QModelIndex scaleIndex = index.sibling(0, 0);
    BlockIndicator blockIndicator = static_cast<BlockIndicator>(scaleIndex.data(Qt::UserRole + 1).toInt());
    if (blockIndicator == BlockIndicator::AllBlock) {
        blockY = option.rect.height();
    } else if (blockIndicator == BlockIndicator::BlockBeforeNow) {
        blockY = timeScaleData->minuteToY(QTime::currentTime(), option.rect);
    }
    if (blockY >= 0) {
        QRect rect = option.rect;
        rect.setHeight(blockY);
        QColor blockColor = option.palette.color(QPalette::Disabled, QPalette::Base);
        blockColor.setAlpha(180);
        painter->fillRect(rect, blockColor);
    }
    int currentY = scaleIndex.data(Qt::UserRole).toInt();
    if (currentY > blockY) {
        painter->save();
        painter->setPen(textPen);
        painter->drawLine(option.rect.left(), currentY, option.rect.width(), currentY);
        painter->restore();
    }

    painter->restore();
}


QSize
CalendarTimeScaleDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    QFont font = option.font;
    font.setPointSizeF(font.pointSizeF() * 0.9);
    QFontMetrics fm(font);
    return QSize(fm.horizontalAdvance("00:00") + 10 * dpiXFactor, option.rect.height());
}


int
CalendarTimeScaleDelegate::selectNiceInterval
(int minInterval) const
{
    const QList<int> allowed = { 60, 120, 180, 240, 360, 720 };
    for (int interval : allowed) {
        if (interval >= minInterval) {
            return interval;
        }
    }
    return 1440;
}


//////////////////////////////////////////////////////////////////////////////
// CalendarDayDelegate

CalendarDayDelegate::CalendarDayDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
CalendarDayDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHints(painter->renderHints() | QPainter::Antialiasing | QPainter::TextAntialiasing);
    entryRects[index].clear();
    headlineEntryRects[index].clear();

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    QDate date = index.data(Qt::UserRole).toDate();
    CalendarDay calendarDay = index.data(Qt::UserRole + 1).value<CalendarDay>();
    bool isToday = (date == QDate::currentDate());

    QColor bgColor;
    QColor selColor = opt.palette.highlight().color();
    QColor dayColor;
    QColor entryColor;

    bool ok;
    int pressedEntryIdx = index.data(Qt::UserRole + 2).toInt(&ok);
    if (! ok) {
        pressedEntryIdx = -2;
    }

    if (pressedEntryIdx < 0 && opt.state & QStyle::State_Selected) {
        bgColor = opt.palette.color(calendarDay.isDimmed == DayDimLevel::Full ? QPalette::Disabled : QPalette::Active, QPalette::Highlight);
    } else if (calendarDay.isDimmed == DayDimLevel::Full) {
        bgColor = opt.palette.color(QPalette::Disabled, QPalette::Base);
    } else {
        bgColor = opt.palette.base().color();
    }
    entryColor = GCColor::invertColor(bgColor);

    painter->fillRect(opt.rect, bgColor);

    const int leftMargin = 4 * dpiXFactor;
    const int rightMargin = 4 * dpiXFactor;
    const int topMargin = 2 * dpiYFactor;
    const int bottomMargin = 4 * dpiYFactor;
    const int lineSpacing = 2 * dpiYFactor;
    const int iconSpacing = 2 * dpiXFactor;
    const int radius = 4 * dpiXFactor;

    // Day number / Headline
    painter->save();
    QFont dayFont = painter->font();
    QFontMetrics dayFM(dayFont);
    dayFont.setWeight(QFont::Bold);
    int lineHeight = dayFM.height();
    QRect dayRect(opt.rect.x() + leftMargin,
                  opt.rect.y() + topMargin,
                  std::max(dayFM.horizontalAdvance(QString::number(date.day())) + leftMargin, lineHeight) + leftMargin,
                  lineHeight);

    int alignFlags = Qt::AlignLeft | Qt::AlignTop;
    if (isToday) {
        dayRect.setX(opt.rect.x() + 1);
        dayRect.setWidth(dayRect.width() - 1);
        painter->save();
        painter->setPen(opt.palette.color(calendarDay.isDimmed == DayDimLevel::Full ? QPalette::Disabled : QPalette::Active, QPalette::Base)),
        painter->setBrush(opt.palette.color(calendarDay.isDimmed == DayDimLevel::Full ? QPalette::Disabled : QPalette::Active, QPalette::Highlight)),
        painter->drawRoundedRect(dayRect, 2 * radius, 2 * radius);
        painter->restore();
        dayColor = opt.palette.color((calendarDay.isDimmed == DayDimLevel::Full || calendarDay.isDimmed == DayDimLevel::Partial) ? QPalette::Disabled : QPalette::Active, QPalette::HighlightedText);
        alignFlags = Qt::AlignHCenter | Qt::AlignTop;
    } else if (pressedEntryIdx < 0 && opt.state & QStyle::State_Selected) {
        dayColor = opt.palette.color((calendarDay.isDimmed == DayDimLevel::Full || calendarDay.isDimmed == DayDimLevel::Partial) ? QPalette::Disabled : QPalette::Active, QPalette::HighlightedText);
    } else {
        dayColor = opt.palette.color((calendarDay.isDimmed == DayDimLevel::Full || calendarDay.isDimmed == DayDimLevel::Partial) ? QPalette::Disabled : QPalette::Active, QPalette::Text);
    }

    painter->setFont(dayFont);
    painter->setPen(dayColor);
    painter->drawText(dayRect, alignFlags, QString::number(date.day()));
    painter->restore();

    for (int i = 0; i < calendarDay.headlineEntries.size(); ++i) {
        CalendarEntry calEntry = calendarDay.headlineEntries[i];
        int x = opt.rect.x() + opt.rect.width() - rightMargin - i * (lineHeight + lineSpacing) - lineHeight;
        if (x < dayRect.x() + dayRect.width() + lineSpacing) {
            break;
        }
        QPixmap pixmap = svgOnBackground(calEntry.iconFile, QSize(lineHeight, lineHeight), 0, calEntry.color, radius);
        QRect headlineEntryRect(x, opt.rect.y() + topMargin, lineHeight, lineHeight);
        painter->drawPixmap(x, opt.rect.y() + topMargin, pixmap);
        headlineEntryRects[index].append(headlineEntryRect);
    }

    // Entries
    QFont entryFont = painter->font();
    dayFont.setWeight(QFont::Normal);
    entryFont.setPointSize(entryFont.pointSize() * 0.95);
    QFontMetrics entryFM(entryFont);
    lineHeight = entryFM.height();
    QSize pixmapSize(2 * lineHeight + lineSpacing, 2 * lineHeight + lineSpacing);
    int entryHeight = pixmapSize.height();
    int entrySpace = opt.rect.height() - dayRect.height() - topMargin - bottomMargin;
    int maxLines = entrySpace / (entryHeight + lineSpacing);
    bool multiLine = (static_cast<int>(calendarDay.entries.size()) <= maxLines);
    if (! multiLine) {
        pixmapSize = QSize(lineHeight, lineHeight);
        entryHeight = pixmapSize.height();
        maxLines = entrySpace / (entryHeight + lineSpacing);
    }

    painter->setPen(entryColor);
    painter->setFont(entryFont);

    int entryStartY = dayRect.y() + dayRect.height() + lineSpacing;
    for (int i = 0; i < std::min(maxLines, static_cast<int>(calendarDay.entries.size())); ++i) {
        CalendarEntry calEntry = calendarDay.entries[i];

        QRect entryRect(opt.rect.x() + leftMargin,
                        entryStartY + i * (entryHeight + lineSpacing),
                        opt.rect.width() - leftMargin - rightMargin - 1,
                        entryHeight - 1);
        QRect titleRect(opt.rect.x() + leftMargin + pixmapSize.width() + iconSpacing,
                        entryStartY + i * (entryHeight + lineSpacing),
                        opt.rect.width() - leftMargin - rightMargin - pixmapSize.width() - 2,
                        lineHeight);
        painter->save();
        if (i == pressedEntryIdx) {
            painter->setBrush(selColor);
            painter->setPen(selColor);
            painter->drawRoundedRect(entryRect, radius, radius);
            painter->setPen(GCColor::invertColor(selColor));
        }
        QPixmap pixmap;
        if (calEntry.type == ENTRY_TYPE_PLANNED_ACTIVITY) {
            QColor pixmapColor(calEntry.color);
            if (   i == pressedEntryIdx
                || (   opt.state & QStyle::State_Selected
                    && pressedEntryIdx < 0)) {
                pixmapColor = GCColor::invertColor(selColor);
            }
            pixmap = svgAsColoredPixmap(calEntry.iconFile, pixmapSize, lineSpacing, pixmapColor);
        } else {
            pixmap = svgOnBackground(calEntry.iconFile, pixmapSize, lineSpacing, calEntry.color, radius);
        }
        painter->drawPixmap(opt.rect.x() + leftMargin,
                            entryStartY + i * (entryHeight + lineSpacing),
                            pixmap);
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop, calEntry.primary);
        if (multiLine && ! calEntry.secondary.isEmpty()) {
            QRect subRect(titleRect.x(),
                          titleRect.y() + lineSpacing + lineHeight,
                          titleRect.width(),
                          titleRect.height());
            painter->drawText(subRect, Qt::AlignLeft | Qt::AlignTop, calEntry.secondary + " (" + calEntry.secondaryMetric + ")");
        }
        painter->restore();

        entryRects[index].append(entryRect);
    }
    int overflow = static_cast<int>(calendarDay.entries.size()) - maxLines;
    if (overflow > 0) {
        QString overflowStr = tr("%1 more...").arg(overflow);
        QFont overflowFont(entryFont);
        overflowFont.setWeight(QFont::DemiBold);
        QFontMetrics overflowFM(overflowFont);
        int overflowWidth = overflowFM.horizontalAdvance(overflowStr) + 2 * rightMargin;
        QRect overflowRect = QRect(opt.rect.x() + opt.rect.width() - overflowWidth,
                                   opt.rect.y() + opt.rect.height() - lineHeight,
                                   overflowWidth,
                                   lineHeight);
        QColor overflowBg(bgColor);
        overflowBg.setAlpha(185);
        painter->save();
        painter->setPen(overflowBg);
        painter->setBrush(overflowBg);
        painter->drawRoundedRect(overflowRect, 2 * radius, 2 * radius);
        painter->restore();
        painter->save();
        painter->setFont(overflowFont);
        painter->drawText(overflowRect, Qt::AlignCenter, overflowStr);
        painter->restore();
        moreRects[index] = overflowRect;
    } else {
        moreRects[index] = QRect();
    }

    painter->restore();
}


bool
CalendarDayDelegate::helpEvent
(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (! event || ! view) {
        return false;
    }

    CalendarDay day = index.data(Qt::UserRole + 1).value<CalendarDay>();
    int entryIdx;
    bool hoverMore = hitTestMore(index, event->pos());
    if (! hoverMore && (entryIdx = hitTestEntry(index, event->pos())) >= 0 && entryIdx < day.entries.size()) {
        CalendarEntry calEntry = day.entries[entryIdx];
        QString tooltip = "<center><b>" + calEntry.primary + "</b>";
        tooltip += "<br/>";
        switch (calEntry.type) {
        case ENTRY_TYPE_ACTIVITY:
            tooltip += "[completed]";
            break;
        case ENTRY_TYPE_PLANNED_ACTIVITY:
            tooltip += "[planned]";
            break;
        case ENTRY_TYPE_OTHER:
        default:
            tooltip += "[other]";
            break;
        }
        tooltip += "<br/>";
        if (! calEntry.secondary.isEmpty()) {
            tooltip += calEntry.secondaryMetric + ": " + calEntry.secondary;
            tooltip += "<br/>";
        }
        if (calEntry.start.isValid()) {
            tooltip += calEntry.start.toString();
            if (calEntry.durationSecs > 0) {
                tooltip += " - " + calEntry.start.addSecs(calEntry.durationSecs).toString();
            }
        }
        tooltip += "</center>";
        QToolTip::showText(event->globalPos(), tooltip, view);
        return true;
    } else if (! hoverMore && (entryIdx = hitTestHeadlineEntry(index, event->pos())) >= 0 && entryIdx < day.headlineEntries.size()) {
        CalendarEntry headlineEntry = day.headlineEntries[entryIdx];
        QString tooltip = "<center><b>" + headlineEntry.primary + "</b></center>";
        tooltip += "<center>";
        switch (headlineEntry.type) {
        case ENTRY_TYPE_PHASE:
            tooltip += "[phase]";
            break;
        case ENTRY_TYPE_EVENT:
        default:
            tooltip += "[event]";
            break;
        }
        tooltip += "</center>";
        if (headlineEntry.spanStart.isValid() && headlineEntry.spanEnd.isValid() && headlineEntry.spanStart < headlineEntry.spanEnd) {
            QLocale locale;
            tooltip += QString("<center>%1 - %2</center>")
                              .arg(locale.toString(headlineEntry.spanStart, QLocale::ShortFormat))
                              .arg(locale.toString(headlineEntry.spanEnd, QLocale::ShortFormat));
        }
        QToolTip::showText(event->globalPos(), tooltip, view);
        return true;
    } else {
        QStringList entries;
        for (CalendarEntry entry : day.entries) {
            entries << entry.primary;
        }
        if (! entries.isEmpty()) {
            QString tooltip = entries.join("\n");
            QToolTip::showText(event->globalPos(), tooltip, view);
            return true;
        }
    }

    return QStyledItemDelegate::helpEvent(event, view, option, index);
}


int
CalendarDayDelegate::hitTestEntry
(const QModelIndex &index, const QPoint &pos) const
{
    if (! entryRects.contains(index)) {
        return -1;
    }
    const QList<QRect> &rects = entryRects[index];
    for (int i = 0; i < rects.size(); ++i) {
        if (rects[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}


int
CalendarDayDelegate::hitTestHeadlineEntry
(const QModelIndex &index, const QPoint &pos) const
{
    if (! headlineEntryRects.contains(index)) {
        return -1;
    }
    const QList<QRect> &rects = headlineEntryRects[index];
    for (int i = 0; i < rects.size(); ++i) {
        if (rects[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}


bool
CalendarDayDelegate::hitTestMore
(const QModelIndex &index, const QPoint &pos) const
{
    return moreRects.contains(index) && moreRects[index].contains(pos);
}


//////////////////////////////////////////////////////////////////////////////
// CalendarSummaryDelegate

CalendarSummaryDelegate::CalendarSummaryDelegate
(int horMargin, QObject *parent)
: QStyledItemDelegate(parent), horMargin(horMargin)
{
}


void
CalendarSummaryDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool hasToolTip = false;
    const QColor bgColor = option.palette.color(QPalette::Active, QPalette::AlternateBase);
    const QColor fgColor = option.palette.color(QPalette::Active, QPalette::Text);
    const CalendarSummary summary = index.data(Qt::UserRole).value<CalendarSummary>();
    QFont valueFont(painter->font());
    valueFont.setWeight(QFont::Normal);
    valueFont.setPointSize(valueFont.pointSize() * 0.95);
    QFont keyFont(valueFont);
    keyFont.setWeight(QFont::DemiBold);
    const QFontMetrics valueFontMetrics(valueFont);
    const QFontMetrics keyFontMetrics(keyFont);
    const int lineHeight = keyFontMetrics.height();
    const int cellWidth = option.rect.width();
    const int cellHeight = option.rect.height();
    const int leftX = option.rect.x();
    const int rightX = option.rect.x() + cellWidth;
    const int availableWidth = cellWidth - 2 * horMargin;
    int lineY = option.rect.y() + vertMargin;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->fillRect(option.rect, bgColor);
    painter->setPen(fgColor);

    for (const std::pair<QString, QString> &p : summary.keyValues) {
        if (lineY + lineHeight > option.rect.y() + cellHeight - vertMargin) {
            hasToolTip = true;
            break;
        }
        QString keyText = p.first;
        QString valueText = p.second;
        QRect keyRect = keyFontMetrics.boundingRect(keyText);
        QRect valueRect = valueFontMetrics.boundingRect(valueText);
        int availableForLeftLabel = availableWidth - valueRect.width() - horMargin;
        if (keyRect.width() > availableForLeftLabel) {
            valueText = valueText.split(' ')[0];
            valueRect = valueFontMetrics.boundingRect(valueText);
            availableForLeftLabel = availableWidth - valueRect.width() - horMargin;
            if (keyRect.width() > availableForLeftLabel) {
                keyText = keyFontMetrics.elidedText(keyText, Qt::ElideMiddle, availableForLeftLabel);
                keyRect = valueFontMetrics.boundingRect(keyText);
            }
            hasToolTip = true;
        }
        keyRect.moveTo(leftX + horMargin, lineY);
        valueRect.moveTo(rightX - horMargin - valueRect.width(), lineY);
        painter->setFont(keyFont);
        painter->drawText(keyRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip, keyText);
        painter->setFont(valueFont);
        painter->drawText(valueRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextDontClip, valueText);

        int lineStartX = leftX + 2 * horMargin + keyRect.width();
        int lineEndX = rightX - 2 * horMargin - valueRect.width();
        if (lineEndX > lineStartX) {
            QPen dottedPen(fgColor, 1 * dpiXFactor, Qt::DotLine);
            painter->save();
            painter->setPen(dottedPen);
            int dotsY = lineY + keyFontMetrics.ascent() - 1;
            painter->drawLine(lineStartX, dotsY, lineEndX, dotsY);
            painter->restore();
        }

        lineY += lineHeight + lineSpacing;
    }

    painter->restore();
    indexHasToolTip[index] = hasToolTip;
}


bool
CalendarSummaryDelegate::helpEvent
(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    Q_UNUSED(option)

    if (! event || ! view || ! indexHasToolTip.value(index, true)) {
        return false;
    }

    CalendarSummary summary = index.data(Qt::UserRole).value<CalendarSummary>();
    QString tooltip = "<table>";
    for (const std::pair<QString, QString> &p : summary.keyValues) {
        tooltip += QString("<tr><td><b>%1</b></td><td>&nbsp;</td><td align='right'>%2</td></tr>").arg(p.first).arg(p.second);
    }
    tooltip += "</table>";
    QToolTip::showText(event->globalPos(), tooltip, view);
    return true;
}


QSize
CalendarSummaryDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const CalendarSummary summary = index.data(Qt::UserRole).value<CalendarSummary>();
    QFont font;
    font.setWeight(QFont::DemiBold);
    const QFontMetrics fontMetrics(font);
    const int lineHeight = fontMetrics.height();

    return QSize(10, vertMargin + (lineHeight + lineSpacing) * summary.keyValues.count() - lineSpacing);
}
