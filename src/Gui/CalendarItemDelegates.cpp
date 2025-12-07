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
#include <QHeaderView>

#include "CalendarData.h"
#include "Colors.h"
#include "Agenda.h"

static bool toolTipHeadlineEntry(const QPoint &pos, QAbstractItemView *view, const CalendarDay &day, int idx);
static bool toolTipDayEntry(const QPoint &pos, QAbstractItemView *view, const CalendarDay &day, int idx);
static bool toolTipMore(const QPoint &pos, QAbstractItemView *view, const CalendarDay &day);
static QRect paintHeadline(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index, HitTester &headlineTester, const QString &dateFormat, int pressedEntryIdx, int leftMargin, int rightMargin, int topMargin, int lineSpacing, int radius);


//////////////////////////////////////////////////////////////////////////////
// TimeScaleData

HitTester::HitTester
()
{
}

void
HitTester::clear
(const QModelIndex &index)
{
    rects[index].clear();
}


void
HitTester::resize
(const QModelIndex &index, qsizetype size)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    rects[index].clear();
    for (int i = 0; i < size; ++i) {
        rects[index] << QRect();
    }
#else
    rects[index].resize(size);
#endif
}


void
HitTester::append
(const QModelIndex &index, const QRect &rect)
{
    rects[index].append(rect);
}


bool
HitTester::set
(const QModelIndex &index, qsizetype i, const QRect &rect)
{
    bool ret = rects[index].count() > i;
    if (ret) {
        rects[index][i] = rect;
    }
    return ret;
}


int
HitTester::hitTest
(const QModelIndex &index, const QPoint &pos) const
{
    if (! rects.contains(index)) {
        return -1;
    }
    const QList<QRect> &indexRects = rects[index];
    for (int i = 0; i < indexRects.size(); ++i) {
        if (indexRects[i].contains(pos)) {
            return i;
        }
    }
    return -1;
}


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
TimeScaleData::minuteToYInTable
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
    return rectTop + std::min(rectHeight, static_cast<int>((minute - _firstMinute) * pixelsPerMinute));
}


int
TimeScaleData::minuteToYInTable
(int minute, const QRect &rect) const
{
    return minuteToYInTable(minute, rect.top(), rect.height());
}


int
TimeScaleData::minuteToYInTable
(const QTime &time, const QRect& rect) const
{
    return minuteToYInTable(time.hour() * 60 + time.minute(), rect.top(), rect.height());
}


QTime
TimeScaleData::timeFromYInTable
(int y, const QRect &rect, int snap) const
{
    int s = std::max(1, snap);
    int minute = trunc((_firstMinute + (y - rect.top()) / pixelsPerMinute(rect)) / s) * s;
    return QTime(0, 0).addSecs(60 * minute);
}


//////////////////////////////////////////////////////////////////////////////
// ColumnDelegatingItemDelegate

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
    }
    return QStyledItemDelegate::helpEvent(event, view, option, index);
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
// CalendarDetailedDayDelegate

CalendarDetailedDayDelegate::CalendarDetailedDayDelegate
(TimeScaleData const * const timeScaleData, QObject *parent)
: QStyledItemDelegate(parent), timeScaleData(timeScaleData)
{
}


void
CalendarDetailedDayDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    bool ok;
    int pressedEntryIdx = index.data(PressedEntryRole).toInt(&ok);
    if (! ok) {
        pressedEntryIdx = -2;
    }
    CalendarDay calendarDay = index.data(DayRole).value<CalendarDay>();
    QList<CalendarEntryLayout> layout = index.data(LayoutRole).value<QList<CalendarEntryLayout>>();
    entryTester.resize(index, layout.count());

    // Background
    QColor bgColor;
    if (calendarDay.isDimmed == DayDimLevel::Full) {
        bgColor = opt.palette.color(QPalette::Disabled, QPalette::Base);
    } else {
        bgColor = opt.palette.base().color();
    }
    painter->fillRect(opt.rect, bgColor);

    // Horizontal Lines
    painter->save();
    QPen linePen(Qt::lightGray, 1, Qt::DotLine);
    painter->setPen(linePen);
    for (int minute = timeScaleData->firstMinute(); minute <= timeScaleData->lastMinute(); minute += timeScaleData->stepSize()) {
        int y = timeScaleData->minuteToYInTable(minute, option.rect);
        painter->drawLine(option.rect.left(), y, option.rect.right(), y);
    }
    painter->restore();

    // Activities
    QFont entryFont = painter->font();
    entryFont.setPointSize(entryFont.pointSize() * 0.95);
    QFontMetrics entryFM(entryFont);
    const int columnSpacing = 10 * dpiXFactor;
    const int horPadding = 4 * dpiXFactor;
    const int radius = 4 * dpiXFactor;
    const int iconSpacing = 2 * dpiXFactor;
    const int priSecSpacing = 2 * dpiXFactor;
    const int lineHeight = entryFM.height();
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
        int top = timeScaleData->minuteToYInTable(startMinute, option.rect);
        int bottom = timeScaleData->minuteToYInTable(endMinute, option.rect);
        int height = std::max(1, bottom - top);
        int numLines = (height + priSecSpacing) / (lineHeight + priSecSpacing);

        painter->save();
        QRect entryRect(left, top, columnWidth, height);
        entryTester.set(index, l.entryIdx, entryRect);
        QColor entryBG = bgColor;
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
            if (height >= 2 * lineHeight + priSecSpacing && columnWidth >= 2 * lineHeight + priSecSpacing) {
                iconWidth = 2 * lineHeight + priSecSpacing;
            } else {
                iconWidth = lineHeight;
            }
            QSize pixmapSize(iconWidth, iconWidth);
            QPixmap pixmap;
            if (entry.type == ENTRY_TYPE_PLANNED_ACTIVITY) {
                if (pressed) {
                    pixmapColor = GCColor::invertColor(entryBG);
                }
                pixmap = svgAsColoredPixmap(entry.iconFile, pixmapSize, iconSpacing, pixmapColor);
            } else {
                pixmap = svgOnBackground(entry.iconFile, pixmapSize, iconSpacing, pixmapColor, radius);
            }
            painter->drawPixmap(left, top, pixmap);
        }
        if (numLines > 0) {
            painter->save();
            QRect textRect(left + iconWidth + horPadding, top, columnWidth - iconWidth - 2 * horPadding, lineHeight);
            painter->setPen(GCColor::invertColor(entryBG));
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, entry.primary);
            if (--numLines > 0 && ! entry.secondary.isEmpty()) {
                textRect.translate(0, lineHeight + priSecSpacing);
                if (! entry.secondaryMetric.isEmpty()) {
                    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, entry.secondary + " (" + entry.secondaryMetric + ")");
                } else {
                    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop, entry.secondary);
                }
            }
            if (--numLines > 0 && ! entry.tertiary.isEmpty()) {
                QRect tertiaryRect(left + horPadding,
                                   top + iconWidth + priSecSpacing,
                                   columnWidth - 2 * horPadding,
                                   entryRect.height() - iconWidth - 2 * priSecSpacing);
                QFont tertiaryFont = painter->font();
                tertiaryFont.setWeight(QFont::Light);
                painter->save();
                painter->setFont(tertiaryFont);
                drawWrappingText(*painter, tertiaryRect, entry.tertiary);
                painter->restore();
            }
            painter->restore();
        }
        painter->restore();
    }

    painter->restore();
}


bool
CalendarDetailedDayDelegate::helpEvent
(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (! event || ! view) {
        return false;
    }
    CalendarDay day = index.data(DayRole).value<CalendarDay>();
    if (toolTipDayEntry(event->globalPos(), view, day, entryTester.hitTest(index, event->pos()))) {
        return true;
    }
    return QStyledItemDelegate::helpEvent(event, view, option, index);
}


void
CalendarDetailedDayDelegate::drawWrappingText
(QPainter &painter, const QRect &rect, const QString &text) const
{
    painter.save();
    const QFont font = painter.font();
    QFontMetrics fm(font);
    const int maxWidth = rect.width();
    const qreal maxY = rect.bottom() + 0.5;
    qreal y = rect.top();
    const QStringList paragraphs = text.split('\n');
    QList<QPair<QPoint, QString>> linesToDraw;
    QString lastLineText;
    QPoint lastLinePos;
    bool stopDrawing = false;
    bool moreTextFollowing = false;
    for (int p = 0; p < paragraphs.size() && ! stopDrawing; ++p) {
        const QString &para = paragraphs[p];
        QTextLayout layout(para, font);
        layout.beginLayout();
        while (! stopDrawing) {
            QTextLine line = layout.createLine();
            if (! line.isValid()) {
                break;
            }
            line.setLineWidth(maxWidth);
            const qreal ascent = line.ascent();
            const qreal descent = line.descent();
            const qreal lineHeight = ascent + descent;
            const qreal lineBottom = y + lineHeight;
            if (lineBottom > maxY) {
                stopDrawing = true;
                moreTextFollowing = true;
                break;
            }
            const QString lineText = para.mid(line.textStart(), line.textLength());
            if (line.naturalTextWidth() > maxWidth) {
                linesToDraw.clear();
                stopDrawing = true;
                break;
            }
            QPoint pos(rect.left(), std::round(y + ascent));
            linesToDraw.append({ pos, lineText });
            lastLineText = lineText;
            lastLinePos = pos;
            y += lineHeight;
        }
        layout.endLayout();
        if (stopDrawing) {
            break;
        }
        if (p < paragraphs.size() - 1) {
            qreal nextLineHeight = fm.ascent() + fm.descent();
            if (y + nextLineHeight > maxY) {
                stopDrawing = true;
                moreTextFollowing = true;
                break;
            }
        }
    }
    if (stopDrawing && ! linesToDraw.isEmpty() && moreTextFollowing) {
        QString textToElide = lastLineText;
        const QString ellipsis = QStringLiteral("…");
        while (! textToElide.isEmpty() && fm.horizontalAdvance(textToElide + ellipsis) > maxWidth - 1) {
            textToElide.chop(1);
        }
        textToElide += ellipsis;
        linesToDraw.last() = { lastLinePos, textToElide };
    }
    for (const auto &line : linesToDraw) {
        painter.drawText(line.first, line.second);
    }
    painter.restore();
}


//////////////////////////////////////////////////////////////////////////////
// CalendarHeadlineDelegate

CalendarHeadlineDelegate::CalendarHeadlineDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
CalendarHeadlineDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHints(painter->renderHints() | QPainter::Antialiasing | QPainter::TextAntialiasing);
    headlineTester.clear(index);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    const int leftMargin = 4 * dpiXFactor;
    const int rightMargin = 4 * dpiXFactor;
    const int topMargin = 2 * dpiYFactor;
    const int bottomMargin = 4 * dpiYFactor;
    const int lineSpacing = 2 * dpiYFactor;
    const int iconSpacing = 2 * dpiXFactor;
    const int radius = 4 * dpiXFactor;

    // Headline: Date, Phases and Events
    QRect dayRect = paintHeadline(painter, opt, index, headlineTester, tr("ddd, dd.MM."), -1, leftMargin, rightMargin, topMargin, lineSpacing, radius);
    heightHint = dayRect.height();

    painter->restore();
}


bool
CalendarHeadlineDelegate::helpEvent
(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (! event || ! view) {
        return false;
    }
    CalendarDay day = index.data(DayRole).value<CalendarDay>();
    if (toolTipHeadlineEntry(event->globalPos(), view, day, headlineTester.hitTest(index, event->pos()))) {
        return true;
    }
    return QStyledItemDelegate::helpEvent(event, view, option, index);
}


QSize
CalendarHeadlineDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return QSize(0, heightHint);
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
        int y = timeScaleData->minuteToYInTable(minute, option.rect);
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
    QModelIndex scaleIndex = index.siblingAtColumn(0);
    BlockIndicator blockIndicator = static_cast<BlockIndicator>(scaleIndex.data(BlockRole).toInt());
    if (blockIndicator == BlockIndicator::AllBlock) {
        blockY = option.rect.height();
    } else if (blockIndicator == BlockIndicator::BlockBeforeNow) {
        blockY = timeScaleData->minuteToYInTable(QTime::currentTime(), option.rect) - option.rect.top();
    }
    if (blockY >= 0) {
        QRect rect = option.rect;
        rect.setHeight(blockY);
        QColor blockColor = option.palette.color(QPalette::Disabled, QPalette::Base);
        blockColor.setAlpha(180);
        painter->fillRect(rect, blockColor);
    }
    int currentY = scaleIndex.data(CurrentYRole).toInt();
    if (currentY - option.rect.y() > blockY) {
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
// CalendarCompactDayDelegate

CalendarCompactDayDelegate::CalendarCompactDayDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
CalendarCompactDayDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHints(painter->renderHints() | QPainter::Antialiasing | QPainter::TextAntialiasing);
    entryTester.clear(index);
    headlineTester.clear(index);
    moreTester.clear(index);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    CalendarDay calendarDay = index.data(DayRole).value<CalendarDay>();
    QColor bgColor;
    QColor selColor = opt.palette.highlight().color();
    QColor dayColor;
    QColor entryColor;

    bool ok;
    int pressedEntryIdx = index.data(PressedEntryRole).toInt(&ok);
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

    // Headline: Date, Phases and Events
    QRect dayRect = paintHeadline(painter, opt, index, headlineTester, tr("dd"), pressedEntryIdx, leftMargin, rightMargin, topMargin, lineSpacing, radius);

    // Entries
    QFont entryFont = painter->font();
    entryFont.setPointSize(entryFont.pointSize() * 0.95);
    QFontMetrics entryFM(entryFont);
    int lineHeight = entryFM.height();
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

        entryTester.append(index, entryRect);
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
        moreTester.append(index, overflowRect);
    }

    painter->restore();
}


bool
CalendarCompactDayDelegate::helpEvent
(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (! event || ! view) {
        return false;
    }
    CalendarDay day = index.data(DayRole).value<CalendarDay>();
    if (   (moreTester.hitTest(index, event->pos()) != -1 && toolTipMore(event->globalPos(), view, day))
        || (toolTipDayEntry(event->globalPos(), view, day, entryTester.hitTest(index, event->pos())))
        || (toolTipHeadlineEntry(event->globalPos(), view, day, headlineTester.hitTest(index, event->pos())))) {
        return true;
    }
    return QStyledItemDelegate::helpEvent(event, view, option, index);
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
    const CalendarSummary summary = index.data(SummaryRole).value<CalendarSummary>();
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

    CalendarSummary summary = index.data(SummaryRole).value<CalendarSummary>();
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
    Q_UNUSED(option)

    const CalendarSummary summary = index.data(SummaryRole).value<CalendarSummary>();
    QFont font;
    font.setWeight(QFont::DemiBold);
    const QFontMetrics fontMetrics(font);
    const int lineHeight = fontMetrics.height();

    return QSize(10, vertMargin + (lineHeight + lineSpacing) * summary.keyValues.count() - lineSpacing);
}


//////////////////////////////////////////////////////////////////////////////
// AgendaMultiDelegate

AgendaMultiDelegate::AgendaMultiDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
AgendaMultiDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    int type = index.data(TypeRole).toInt();
    bool hover = (type == 0) && opt.state & QStyle::State_MouseOver;
    opt.state &= ~QStyle::State_Selected;
    opt.state &= ~QStyle::State_HasFocus;
    opt.state &= ~QStyle::State_MouseOver;

    if (type == 0) { // Dual Line Text
        opt.displayAlignment = Qt::AlignLeft | Qt::AlignTop;

        const QString secondaryText = index.data(SecondaryDisplayRole).toString();
        const QString secondaryHoverText = index.data(SecondaryHoverTextRole).toString();

        if (hover) {
            const QString hoverText = index.data(HoverTextRole).toString();
            const QFont hoverFont = index.data(HoverFontRole).value<QFont>();
            if (! hoverText.isEmpty()) {
                opt.text = hoverText;
            }
            if (hoverFont != QFont()) {
                opt.font = hoverFont;
            }
        }

        QFontMetrics upperFM(opt.font);
        QRect upperRect(opt.rect.x() + attributes.paddingDL.left(),
                        opt.rect.y() + attributes.paddingDL.top(),
                        std::max(0, opt.rect.width() - attributes.paddingDL.left() - attributes.paddingDL.right()),
                        std::min(opt.rect.height(), upperFM.height()));

        const QWidget *widget = opt.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        opt.rect = upperRect;
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

        if (   (   ! hover
                && ! secondaryText.isEmpty())
            || (   hover
                && ! secondaryHoverText.isEmpty())) {
            opt.text = secondaryText;
            if (! hover) {
                const QFont secondaryFont = index.data(SecondaryFontRole).value<QFont>();
                if (secondaryFont != QFont()) {
                    opt.font = secondaryFont;
                }
            } else {
                const QFont secondaryHoverFont = index.data(SecondaryHoverFontRole).value<QFont>();
                if (secondaryHoverFont != QFont()) {
                    opt.font = secondaryHoverFont;
                }
                if (! secondaryHoverText.isEmpty()) {
                    opt.text = secondaryHoverText;
                }
            }
            QFontMetrics lowerFM(opt.font);
            opt.rect.translate(0, opt.rect.height());
            opt.rect.setHeight(lowerFM.height());
            style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
        }
    } else if (type == 1) { // Headline
        opt.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
        opt.rect = QRect(opt.rect.x() + attributes.paddingHL.left(),
                         opt.rect.y() + attributes.paddingHL.top(),
                         std::max(0, opt.rect.width() - attributes.paddingHL.left() - attributes.paddingHL.right()),
                         std::min(opt.rect.height(), opt.fontMetrics.height()));

        const QWidget *widget = opt.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    }
}


QSize
AgendaMultiDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    int type = index.data(TypeRole).toInt();
    if (type == 0) { // Dual Line Text
        QString text = index.data(Qt::DisplayRole).toString();
        QFont normalFont = index.data(Qt::FontRole).value<QFont>();
        if (! normalFont.family().isEmpty()) {
            opt.font = normalFont;
        }

        QFontMetrics normalFM(opt.font);
        QSize normalSize = normalFM.size(Qt::TextSingleLine, text);

        const QString hoverText = index.data(HoverTextRole).toString();
        QFont hoverFont = index.data(HoverFontRole).value<QFont>();

        QSize hoverSize = QSize(0, 0);
        if (! hoverText.isEmpty()) {
            if (hoverFont == QFont()) {
                hoverFont = opt.font;
            }
            QFontMetrics hoverFM(hoverFont);
            hoverSize = hoverFM.size(Qt::TextSingleLine, hoverText.isEmpty() ? text : hoverText);
        }
        QSize maxSize(std::max(normalSize.width(), hoverSize.width()), std::max(normalSize.height(), hoverSize.height()));

        QSize secondarySize(0, 0);
        const QString secondaryText = index.data(SecondaryDisplayRole).toString();
        if (! secondaryText.isEmpty()) {
            QFont secondaryFont = index.data(SecondaryFontRole).value<QFont>();
            if (secondaryFont == QFont()) {
                secondaryFont = normalFont;
            }
            QFontMetrics secondaryFM(secondaryFont);
            secondarySize = secondaryFM.size(Qt::TextSingleLine, secondaryText);
        }
        const QString secondaryHoverText = index.data(SecondaryHoverTextRole).toString();
        if (! secondaryHoverText.isEmpty()) {
            QFont secondaryHoverFont = index.data(SecondaryHoverFontRole).value<QFont>();
            if (secondaryHoverFont == QFont()) {
                secondaryHoverFont = hoverFont;
            }
            QFontMetrics secondaryHoverFM(secondaryHoverFont);
            QSize secondaryHoverSize = secondaryHoverFM.size(Qt::TextSingleLine, secondaryText);
            secondarySize.setWidth(std::max(secondarySize.width(), secondaryHoverSize.width()));
            secondarySize.setHeight(std::max(secondarySize.height(), secondaryHoverSize.height()));
        }
        maxSize.setWidth(std::max(maxSize.width(), secondarySize.width()));
        maxSize.setHeight(maxSize.height() + secondarySize.height());

        maxSize += QSize(8 + attributes.paddingDL.left() + attributes.paddingDL.right(), 4 + attributes.paddingDL.top() + attributes.paddingDL.bottom());

        return maxSize;
    } else if (type == 1) { // Headline
        const QWidget *widget = opt.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        int height = style->sizeFromContents(QStyle::CT_ItemViewItem, &opt, QSize(), opt.widget).height();
        return QSize(option.rect.width(), height + attributes.paddingHL.top() + attributes.paddingHL.bottom());
    }
    return QStyledItemDelegate::sizeHint(option, index);
}


AgendaMultiDelegate::Attributes
AgendaMultiDelegate::getAttributes
() const
{
    return attributes;
}


void
AgendaMultiDelegate::setAttributes
(const Attributes &attributes)
{
    this->attributes = attributes;
}


void
AgendaMultiDelegate::initStyleOption
(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

    if (AgendaTree *tree = qobject_cast<AgendaTree*>(parent())) {
        if (tree->isRowHovered(index)) {
            option->state |= QStyle::State_MouseOver;
        } else {
            option->state &= ~QStyle::State_MouseOver;
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// AgendaEntryDelegate

AgendaEntryDelegate::AgendaEntryDelegate
(QTreeWidget *parent)
: QStyledItemDelegate(parent), treeWidget(parent)
{
    connect(parent, &QTreeWidget::itemChanged, this, &AgendaEntryDelegate::updateColumnWidth, Qt::QueuedConnection);
    connect(parent->header(), &QHeaderView::sectionResized, this, &AgendaEntryDelegate::onSectionResized);
    QTimer::singleShot(0, this, &AgendaEntryDelegate::updateColumnWidth);
}



void
AgendaEntryDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (column < 0) {
        column = index.column();
    }
    CalendarEntry entry = index.data(EntryRole).value<CalendarEntry>();

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    bool hover = opt.state & QStyle::State_MouseOver;

    opt.state &= ~QStyle::State_Selected;
    opt.state &= ~QStyle::State_HasFocus;
    opt.state &= ~QStyle::State_MouseOver;
    opt.text.clear();

    const QWidget *widget = opt.widget;
    QStyle *style = widget ? widget->style() : QApplication::style();
    style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

    QFont line1Font = painter->font();

    QFont::Weight primaryWeight = attributes.primaryWeight;
    QFont::Weight secondaryWeight = attributes.secondaryWeight;
    QFont::Weight secondaryMetricWeight = attributes.secondaryMetricWeight;
    if (hover) {
        primaryWeight = attributes.primaryHoverWeight;
        secondaryWeight = attributes.secondaryHoverWeight;
        secondaryMetricWeight = attributes.secondaryMetricHoverWeight;
    }
    if (primaryWeight == 0) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        primaryWeight = static_cast<QFont::Weight>(line1Font.weight());
#else
        primaryWeight = line1Font.weight();
#endif
    }
    if (secondaryWeight == 0) {
        secondaryWeight = primaryWeight;
    }
    if (secondaryMetricWeight == 0) {
        secondaryMetricWeight = secondaryWeight;
    }

    line1Font.setWeight(primaryWeight);
    QFontMetrics line1FM(line1Font);
    const int lineSpacing = attributes.lineSpacing * dpiYFactor;
    const int lineHeight = line1FM.height();
    const int radius = 4 * dpiXFactor;
    const int iconInnerSpacing = 4 * dpiXFactor;
    const int iconTextSpacing = attributes.iconTextSpacing * dpiXFactor;
    const int iconWidth = 2 * lineHeight + lineSpacing;
    const QSize pixmapSize(iconWidth, iconWidth);
    QColor textColor = opt.palette.color(QPalette::Active, QPalette::Text);
    QColor iconColor = entry.color;

    const int textX = opt.rect.x() + iconWidth + iconTextSpacing + attributes.padding.left();
    const int textWidth = opt.rect.width() - iconWidth - iconTextSpacing - attributes.padding.left() - attributes.padding.right();

    if (hover) {
        QColor bgColor = opt.palette.color(QPalette::Active, QPalette::Highlight);
        bgColor.setAlphaF(0.2);
        QColor bgBorder = bgColor;
        bgBorder.setAlphaF(0.6);
        iconColor = GCColor::invertColor(GCColor::blendedColor(bgColor, opt.palette.color(QPalette::Active, QPalette::AlternateBase)));
        textColor = iconColor;
    }

    QPixmap pixmap = svgAsColoredPixmap(entry.iconFile, pixmapSize, iconInnerSpacing, iconColor);
    painter->drawPixmap(opt.rect.x() + attributes.padding.left(), opt.rect.y() + attributes.padding.top(), pixmap);

    painter->setPen(textColor);
    QRect textRect(textX, opt.rect.y() + attributes.padding.top(), textWidth, lineHeight);

    QString primary(entry.primary);
    QRect boundingRect = line1FM.boundingRect(primary);
    if (boundingRect.width() > textRect.width()) {
        primary = line1FM.elidedText(primary, Qt::ElideRight, textRect.width());
    }

    painter->save();
    painter->setFont(line1Font);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip, primary);
    painter->restore();
    if (! entry.secondary.isEmpty()) {
        painter->save();
        QFont metricValueFont = painter->font();
        metricValueFont.setWeight(secondaryWeight);
        QFontMetrics metricValueFM(metricValueFont);
        painter->setFont(metricValueFont);
        textRect.translate(0, lineHeight + lineSpacing);
        QRect secRect = metricValueFM.boundingRect(entry.secondary);
        secRect.setRect(textRect.x(), textRect.y(), secRect.width(), secRect.height());
        painter->drawText(secRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextDontClip, entry.secondary);
        painter->restore();
        if (! entry.secondaryMetric.isEmpty()) {
            painter->save();
            QFont metricLabelFont = painter->font();
            metricLabelFont.setWeight(secondaryMetricWeight);
            QFontMetrics metricLabelFM(metricLabelFont);
            painter->setFont(metricLabelFont);
            int remainingWidth = opt.rect.x() + opt.rect.width() - secRect.right() - attributes.padding.left() - attributes.padding.right();

            QString txt = " " + entry.secondaryMetric;
            QRect secMetricRect = metricLabelFM.boundingRect(txt);
            if (secMetricRect.width() > remainingWidth) {
                txt = metricLabelFM.elidedText(txt, Qt::ElideRight, remainingWidth);
            }
            secMetricRect.setRect(secRect.x() + secRect.width() + 2 * dpiXFactor,
                                  secRect.y(),
                                  remainingWidth,
                                  secMetricRect.height());
            painter->drawText(secMetricRect, Qt::AlignLeft | Qt::AlignTop, txt);
            painter->restore();
        }
        if (! entry.tertiary.isEmpty()) {
            painter->save();
            painter->setPen(GCColor::inactiveColor(painter->pen().color(), attributes.tertiaryDimLevel));
            int y = opt.rect.y() + attributes.padding.top() + pixmapSize.height() + 2 * lineSpacing;
            QStringList tertiaryLines;
            int numLines;
            int tertiaryHeight;
            layoutTertiaryText(entry.tertiary, opt.font, textWidth, numLines, tertiaryHeight, &tertiaryLines, true);
            QRect tertiaryRect(textX, y, textWidth, lineHeight);
            for (const QString &tertiaryLine : tertiaryLines) {
                painter->drawText(tertiaryRect, Qt::AlignLeft | Qt::AlignTop, tertiaryLine);
                tertiaryRect.translate(0, lineHeight);
            }
            painter->restore();
        }
    }

    painter->restore();
}


QSize
AgendaEntryDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (column < 0) {
        column = index.column();
    }
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    int width = columnWidth;
    if (width <= 0) {
        const QTreeWidget *tw = qobject_cast<const QTreeWidget*>(option.widget);
        if (tw) {
            width = tw->columnWidth(column);
            if (width <= 0) {
                width = tw->viewport()->width() / 2;
            }
        } else {
            width = 200;
        }
    }

    QVariant data = index.data(EntryRole);
    if (! data.isNull()) {
        const int lineSpacing = attributes.lineSpacing * dpiYFactor;
        const int lineHeight = option.fontMetrics.height();
        CalendarEntry entry = data.value<CalendarEntry>();
        int tertiaryHeight = 0;
        if (! entry.tertiary.isEmpty()) {
            const int iconWidth = 2 * lineHeight + lineSpacing;
            int tertiaryWidth = width - iconWidth - attributes.iconTextSpacing * dpiXFactor - attributes.padding.left() - attributes.padding.right();
            int numLines;
            int tertiary;
            layoutTertiaryText(entry.tertiary, option.font, tertiaryWidth, numLines, tertiaryHeight, nullptr, true);
            tertiaryHeight += 2 * lineSpacing;
        }
        return QSize(width, 2 * lineHeight + lineSpacing + attributes.padding.top() + attributes.padding.bottom() + tertiaryHeight);
    }
    return QSize(width, attributes.padding.top() + attributes.padding.bottom());
}


AgendaEntryDelegate::Attributes
AgendaEntryDelegate::getAttributes
() const
{
    return attributes;
}


void
AgendaEntryDelegate::setAttributes
(const Attributes &attributes)
{
    this->attributes = attributes;
}


bool
AgendaEntryDelegate::hasToolTip
(const QModelIndex &index, QString *toolTipText) const
{
    if (! index.isValid()) {
        return false;
    }
    CalendarEntry entry = index.data(EntryRole).value<CalendarEntry>();
    QString text(entry.tertiary.trimmed());
    if (text.isEmpty()) {
        return false;
    }
    QStyleOptionViewItem opt;
    initStyleOption(&opt, index);
    const int lineSpacing = attributes.lineSpacing * dpiYFactor;
    const int lineHeight = opt.fontMetrics.height();
    const int iconTextSpacing = attributes.iconTextSpacing * dpiXFactor;
    const int iconWidth = 2 * lineHeight + lineSpacing;
    int availableWidth = columnWidth - iconWidth - iconTextSpacing - attributes.padding.left() - attributes.padding.right();
    int numLines;
    int height;
    layoutTertiaryText(entry.tertiary, opt.font, availableWidth, numLines, height, nullptr, false);
    bool show = numLines > attributes.maxTertiaryLines;
    if (show) {
        QStringList ttlines = entry.tertiary.split('\n');
        *toolTipText = QString("<h4>%1</h4>").arg(entry.primary.trimmed());
        for (const QString &line : ttlines) {
            *toolTipText += QString("<div style='max-width: %1px; white-space: normal;'>%2</div>")
                                   .arg(600 * dpiXFactor)
                                   .arg(line);
        }
    }

    return show;
}


void
AgendaEntryDelegate::initStyleOption
(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

    if (AgendaTree *tree = qobject_cast<AgendaTree*>(parent())) {
        if (tree->isRowHovered(index)) {
            option->state |= QStyle::State_MouseOver;
        } else {
            option->state &= ~QStyle::State_MouseOver;
        }
    }
}


AgendaEntryDelegate::LayoutResult
AgendaEntryDelegate::processLayoutText(const QString &text, const QFont &font, int availableWidth, bool limitHeight, bool needLines) const
{
    LayoutResult result;
    if (text.isEmpty() || availableWidth <= 0) {
        return result;
    }

    if (needLines) {
        result.lines.reserve(limitHeight ? attributes.maxTertiaryLines : 20);
    }

    QStringList paragraphs = text.split('\n');

    int calcHeight = 0;
    int calcLines = 0;
    QFontMetrics fm(font);
    const int lineHeight = fm.height();

    for (int p = 0; p < paragraphs.size(); ++p) {
        const QString &paragraph = paragraphs[p];

        if (paragraph.isEmpty()) {
            ++calcLines;
            if (needLines) {
                result.lines.append(QString());
            }
            if (! limitHeight || calcLines <= attributes.maxTertiaryLines) {
                calcHeight += lineHeight;
            }
            if (limitHeight && calcLines >= attributes.maxTertiaryLines) {
                result.numLines = calcLines;
                result.height = calcHeight;
                return result;
            }
            continue;
        }

        QTextLayout layout(paragraph, font);
        layout.setCacheEnabled(true);
        layout.beginLayout();

        while (true) {
            QTextLine line = layout.createLine();
            if (! line.isValid()) {
                break;
            }
            line.setLineWidth(availableWidth);
            ++calcLines;

            bool isLastDisplayLine = limitHeight && calcLines >= attributes.maxTertiaryLines;
            if (isLastDisplayLine && needLines) {
                bool hasMoreLinesInParagraph = (line.textStart() + line.textLength() < paragraph.length());
                bool hasMoreParagraphs = (p < paragraphs.size() - 1);
                bool hasMoreText = hasMoreLinesInParagraph || hasMoreParagraphs;
                if (hasMoreText) {
                    const QString ellipsis = "…";
                    const int ellipsisWidth = fm.horizontalAdvance(ellipsis);
                    QString lineText = paragraph.mid(line.textStart(), line.textLength());

                    int fullWidth = fm.horizontalAdvance(lineText);
                    if (fullWidth + ellipsisWidth <= availableWidth) {
                        result.lines.append(lineText + ellipsis);
                    } else {
                        int left = 0;
                        int right = lineText.length();
                        while (left < right) {
                            int mid = (left + right + 1) / 2;
                            QString testText = lineText.left(mid);
                            if (fm.horizontalAdvance(testText + ellipsis) <= availableWidth) {
                                left = mid;
                            } else {
                                right = mid - 1;
                            }
                        }
                        QString truncatedText = lineText.left(left).trimmed();

                        int wordBoundary = -1;
                        for (int i = truncatedText.length() - 1; i >= 0; --i) {
                            if (truncatedText[i].isSpace()) {
                                wordBoundary = i;
                                break;
                            }
                        }

                        if (wordBoundary > 0 && wordBoundary >= truncatedText.length() * 0.8) {
                            QString wordBrokenText = truncatedText.left(wordBoundary).trimmed();
                            if (fm.horizontalAdvance(wordBrokenText + ellipsis) <= availableWidth) {
                                truncatedText = wordBrokenText;
                            }
                        }
                        result.lines.append(truncatedText + ellipsis);
                    }
                    calcHeight += line.height();
                    layout.endLayout();
                    result.numLines = calcLines;
                    result.height = calcHeight;
                    return result;
                } else {
                    result.lines.append(paragraph.mid(line.textStart(), line.textLength()));
                    calcHeight += line.height();
                    layout.endLayout();
                    result.numLines = calcLines;
                    result.height = calcHeight;
                    return result;
                }
            }

            if (needLines) {
                result.lines.append(paragraph.mid(line.textStart(), line.textLength()));
            }

            if (! limitHeight || calcLines <= attributes.maxTertiaryLines) {
                calcHeight += line.height();
            }

            if (isLastDisplayLine) {
                layout.endLayout();
                result.numLines = calcLines;
                result.height = calcHeight;
                return result;
            }
        }
        layout.endLayout();
    }
    result.numLines = calcLines;
    result.height = calcHeight;

    return result;
}


void
AgendaEntryDelegate::layoutTertiaryText
(const QString &text, const QFont &font, int availableWidth, int &numLines, int &height, QStringList *lines, bool limitHeight) const
{
    LayoutResult result = processLayoutText(text, font, availableWidth, limitHeight, lines != nullptr);
    numLines = result.numLines;
    height = result.height;
    if (lines) {
        *lines = result.lines;
    }
}


void
AgendaEntryDelegate::updateColumnWidth
()
{
    if (column >= 0) {
        int newWidth = treeWidget->columnWidth(column);
        if (newWidth > 50 && newWidth != columnWidth) {
            columnWidth = newWidth;
            treeWidget->setItemDelegateForColumn(column, nullptr);
            treeWidget->setItemDelegateForColumn(column, this);
        }
    }
}


void
AgendaEntryDelegate::onSectionResized
(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(oldSize)
    Q_UNUSED(newSize)

    if (column >= 0 && column <= logicalIndex) {
        QTimer::singleShot(100, this, &AgendaEntryDelegate::updateColumnWidth);
    }
}


//////////////////////////////////////////////////////////////////////////////
// Local helpers

static bool
toolTipHeadlineEntry
(const QPoint &pos, QAbstractItemView *view, const CalendarDay &day, int idx)
{
    if (idx >= 0 && idx < day.headlineEntries.size()) {
        CalendarEntry headlineEntry = day.headlineEntries[idx];
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
        QToolTip::showText(pos, tooltip, view);
        return true;
    }
    return false;
}


static bool
toolTipDayEntry(const QPoint &pos, QAbstractItemView *view, const CalendarDay &day, int idx)
{
    if (idx >= 0 && idx < day.entries.size()) {
        CalendarEntry calEntry = day.entries[idx];
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
        QToolTip::showText(pos, tooltip, view);
        return true;
    }
    return false;
}


static bool
toolTipMore
(const QPoint &pos, QAbstractItemView *view, const CalendarDay &day)
{
    QStringList entries;
    for (CalendarEntry entry : day.entries) {
        entries << entry.primary;
    }
    if (! entries.isEmpty()) {
        QString tooltip = entries.join("\n");
        QToolTip::showText(pos, tooltip, view);
        return true;
    }
    return false;
}


static QRect
paintHeadline
(QPainter *painter, const QStyleOptionViewItem &opt, const QModelIndex &index, HitTester &headlineTester, const QString &dateFormat, int pressedEntryIdx, int leftMargin, int rightMargin, int topMargin, int lineSpacing, int radius)
{
    CalendarDay calendarDay = index.data(CalendarHeadlineDelegate::DayRole).value<CalendarDay>();
    QColor dayColor;
    bool isToday = (calendarDay.date == QDate::currentDate());
    QLocale locale;
    QString dateText = locale.toString(calendarDay.date, dateFormat);

    // Day number / Headline
    painter->save();
    QFont dayFont = painter->font();
    QFontMetrics dayFM(dayFont);
    dayFont.setWeight(QFont::Bold);
    int lineHeight = dayFM.height();
    QRect dayRect(opt.rect.x() + leftMargin,
                  opt.rect.y() + topMargin,
                  std::max(dayFM.horizontalAdvance(dateText) + leftMargin, lineHeight) + leftMargin,
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
    painter->drawText(dayRect, alignFlags, dateText);
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
        headlineTester.append(index, headlineEntryRect);
    }

    return dayRect;
}
