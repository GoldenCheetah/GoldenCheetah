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
    dayFont.setBold(true);
    int lineHeight = dayFM.height();
    QRect dayRect(opt.rect.x() + leftMargin,
                  opt.rect.y() + topMargin,
                  std::max(dayFM.horizontalAdvance(QString::number(date.day())) + leftMargin, lineHeight) + leftMargin,
                  lineHeight);

    if (isToday) {
        QRect dayHLRect(opt.rect.x() + 1 , opt.rect.y() + 1, dayRect.width() - 1, dayRect.height() + topMargin - 1);
        painter->save();
        painter->setPen(opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::Base)),
        painter->setBrush(opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::Highlight)),
        painter->drawRoundedRect(dayHLRect, 2 * radius, 2 * radius);
        painter->restore();
        dayColor = opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::HighlightedText);
    } else if (pressedEntryIdx < 0 && opt.state & QStyle::State_Selected) {
        dayColor = opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::HighlightedText);
    } else {
        dayColor = opt.palette.color(calendarDay.isDimmed ? QPalette::Disabled : QPalette::Active, QPalette::Text);
    }

    painter->setFont(dayFont);
    painter->setPen(dayColor);
    painter->drawText(dayRect, Qt::AlignTop | Qt::AlignLeft, QString::number(date.day()));
    painter->restore();

    for (int i = 0; i < calendarDay.headlineEntries.size(); ++i) {
        CalendarEntry calEntry = calendarDay.headlineEntries[i];
        int x = opt.rect.x() + opt.rect.width() - rightMargin - i * (lineHeight + lineSpacing) - lineHeight;
        if (x < dayRect.x() + dayRect.width() + lineSpacing) {
            break;
        }
        QPixmap pixmap = svgOnBackground(calEntry.iconFile, QSize(lineHeight, lineHeight), lineSpacing, calEntry.color, radius);
        QRect headlineEntryRect(x, opt.rect.y() + topMargin, lineHeight, lineHeight);
        painter->drawPixmap(x, opt.rect.y() + topMargin, pixmap);
        headlineEntryRects[index].append(headlineEntryRect);
    }

    // Entries
    QFont entryFont = painter->font();
    entryFont.setBold(false);
    entryFont.setPointSize(entryFont.pointSize() - 1);

    QFontMetrics entryFM(entryFont);
    lineHeight = entryFM.height();
    QSize pixmapSize(2 * lineHeight + lineSpacing, 2 * lineHeight + lineSpacing);
    int entryHeight = pixmapSize.height();
    int entrySpace = opt.rect.height() - dayRect.height() - topMargin - bottomMargin;
    int maxLines = entrySpace / (entryHeight + lineSpacing);
    bool multiLine  = (static_cast<int>(calendarDay.entries.size()) <= maxLines);
    if (! multiLine) {
        pixmapSize = QSize(lineHeight, lineHeight);
        entryHeight = pixmapSize.height();
        maxLines = entrySpace / (entryHeight + lineSpacing);
    }

    painter->setPen(entryColor);

    int entryStartY = dayRect.y() + dayRect.height() + lineSpacing;
    for (int i = 0; i < std::min(maxLines, static_cast<int>(calendarDay.entries.size())); ++i) {
        CalendarEntry calEntry = calendarDay.entries[i];

        entryFont.setWeight(calEntry.isRelocatable ? QFont::ExtraLight : QFont::Normal);
        painter->setFont(entryFont);

        QPixmap pixmap = svgOnBackground(calEntry.iconFile, pixmapSize, lineSpacing, calEntry.color, radius);

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
        painter->drawPixmap(opt.rect.x() + leftMargin,
                            entryStartY + i * (entryHeight + lineSpacing),
                            pixmap);
        painter->drawText(titleRect, Qt::AlignLeft | Qt::AlignTop, calEntry.name);
        if (multiLine && calEntry.durationSecs > 0) {
            QRect subRect(titleRect.x(),
                          titleRect.y() + lineSpacing + lineHeight,
                          titleRect.width(),
                          titleRect.height());
            QTime dur(0, 0, 0);
            dur = dur.addSecs(calEntry.durationSecs);
            painter->drawText(subRect, Qt::AlignLeft | Qt::AlignTop, dur.toString("H:mm:ss"));
        }
        painter->restore();

        entryRects[index].append(entryRect);
    }
    if (maxLines < static_cast<int>(calendarDay.entries.size())) {
        QPixmap pixmap = svgOnBackground(":/images/material/plus.svg", QSize(lineHeight, lineHeight), lineSpacing, bgColor, 2 * radius);
        painter->drawPixmap(opt.rect.x() + opt.rect.width() - lineHeight,
                            opt.rect.y() + opt.rect.height() - lineHeight,
                            pixmap);
        moreRects[index] = QRect(opt.rect.x() + opt.rect.width() - lineHeight,
                                 opt.rect.y() + opt.rect.height() - lineHeight,
                                 lineHeight,
                                 lineHeight);
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
        QString tooltip = "<center><b>" + calEntry.name + "</b></center>";
        tooltip += "<center>";
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
        tooltip += "</center>";
        if (calEntry.start.isValid()) {
            tooltip += calEntry.start.toString();
            if (calEntry.durationSecs > 0) {
                tooltip += " - " + calEntry.start.addSecs(calEntry.durationSecs).toString();
            }
        }
        QToolTip::showText(event->globalPos(), tooltip, view);
        return true;
    } else if (! hoverMore && (entryIdx = hitTestHeadlineEntry(index, event->pos())) >= 0 && entryIdx < day.headlineEntries.size()) {
        CalendarEntry headlineEntry = day.headlineEntries[entryIdx];
        QString tooltip = "<center><b>" + headlineEntry.name + "</b></center>";
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
            entries << entry.name;
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
    QColor bgColor = option.palette.color(QPalette::Disabled, QPalette::Base);
    QColor fgColor = option.palette.color(QPalette::Disabled, QPalette::Text);

    CalendarWeeklySummary summary = index.data(Qt::UserRole).value<CalendarWeeklySummary>();
    painter->save();
    painter->fillRect(option.rect, bgColor);
    painter->setPen(fgColor);

    painter->setFont(QFont(option.font.family(), option.font.pointSize(), QFont::Bold));
    painter->drawText(option.rect, Qt::AlignCenter, QString("%1 - %2 - %3").arg(summary.entriesByType[ENTRY_TYPE_ACTIVITY], 0)
                                                                           .arg(summary.entriesByType[ENTRY_TYPE_PLANNED_ACTIVITY], 0)
                                                                           .arg(summary.entriesByType[ENTRY_TYPE_OTHER], 0));

    painter->restore();
}


bool
CalendarSummaryDelegate::helpEvent
(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (! event || ! view) {
        return false;
    }

    CalendarWeeklySummary summary = index.data(Qt::UserRole).value<CalendarWeeklySummary>();
    QStringList weekEntries;
    for (CalendarEntry entry : summary.entries) {
        weekEntries << entry.name;
    }
    if (! weekEntries.isEmpty()) {
        QToolTip::showText(event->globalPos(), weekEntries.join("\n"), view);
        return true;
    }

    return QStyledItemDelegate::helpEvent(event, view, option, index);
}
