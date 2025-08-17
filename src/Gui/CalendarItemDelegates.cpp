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
        bgColor = opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::Highlight);
    } else if (calendarDay.isDimmed) {
        bgColor = opt.palette.color(QPalette::Disabled, QPalette::Base);
    } else {
        bgColor = opt.palette.base().color();
    }
    entryColor = GCColor::invertColor(bgColor);

    painter->fillRect(opt.rect, bgColor);

    int leftMargin = 4;
    int rightMargin = 4;
    int topMargin = 2;
    int bottomMargin = 4;
    int lineSpacing = 2;
    int iconSpacing = 2;
    int radius = 4;

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
        painter->setPen(opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::Base)),
        painter->setBrush(opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::Highlight)),
        painter->drawRoundedRect(dayRect, 2 * radius, 2 * radius);
        painter->restore();
        dayColor = opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::HighlightedText);
        alignFlags = Qt::AlignHCenter | Qt::AlignTop;
    } else if (pressedEntryIdx < 0 && opt.state & QStyle::State_Selected) {
        dayColor = opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::HighlightedText);
    } else {
        dayColor = opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::Text);
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
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
CalendarSummaryDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool hasToolTip = false;
    const QColor bgColor = GCColor::inactiveColor(option.palette.color(QPalette::Active, QPalette::Base));
    const QColor fgColor = option.palette.color(QPalette::Active, QPalette::Text);
    const CalendarSummary summary = index.data(Qt::UserRole).value<CalendarSummary>();
    QFont valueFont(painter->font());
    valueFont.setWeight(QFont::Normal);
    valueFont.setPointSize(valueFont.pointSize() * 0.95);
    QFont keyFont(valueFont);
    keyFont.setWeight(QFont::DemiBold);
    const QFontMetrics valueFontMetrics(valueFont);
    const QFontMetrics keyFontMetrics(keyFont);
    const int lineSpacing = 2;
    const int horMargin = 4;
    const int vertMargin = 4;
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
