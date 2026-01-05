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

#ifndef CALENDARITEMDELEGATES_H
#define CALENDARITEMDELEGATES_H

#include <QStyledItemDelegate>
#include <QTreeWidget>


class HitTester {
public:
    explicit HitTester();

    void clear(const QModelIndex &index);
    void resize(const QModelIndex &index, qsizetype size);
    void append(const QModelIndex &index, const QRect &rect);
    bool set(const QModelIndex &index, qsizetype i, const QRect &rect);
    int hitTest(const QModelIndex &index, const QPoint &pos) const;

private:
    QHash<QModelIndex, QList<QRect>> rects;
};


enum class BlockIndicator {
    NoBlock = 0,
    AllBlock = 1,
    BlockBeforeNow = 2
};


class TimeScaleData {
public:
    explicit TimeScaleData();

    void setFirstMinute(int minute);
    int firstMinute() const;

    void setLastMinute(int minute);
    int lastMinute() const;

    void setStepSize(int step);
    int stepSize() const;

    double pixelsPerMinute(int availableHeight) const;
    double pixelsPerMinute(const QRect& rect) const;

    int minuteToYInTable(int minute, int rectTop, int rectHeight) const;
    int minuteToYInTable(int minute, const QRect& rect) const;
    int minuteToYInTable(const QTime &time, const QRect& rect) const;

    QTime timeFromYInTable(int y, const QRect &rect, int snap = 15) const;

private:
    int _firstMinute = 8 * 60;
    int _lastMinute = 21 * 60;
    int _stepSize = 60;
};


// Workaround for Qt5 not having QAbstractItemView::itemDelegateForIndex (Qt6 only)
class ColumnDelegatingItemDelegate : public QStyledItemDelegate {
public:
    explicit ColumnDelegatingItemDelegate(QList<QStyledItemDelegate*> delegates, QObject *parent = nullptr);

    QStyledItemDelegate *getDelegate(int idx) const;
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QList<QStyledItemDelegate*> delegates;
};


class CalendarDetailedDayDelegate : public QStyledItemDelegate {
public:
    enum Roles {
        DayRole = Qt::UserRole + 1, // [CalendarDay] Calendar day
        PressedEntryRole,           // [int] Index of the currently pressed CalendarDay (see DayRole >> entries)
        RelatedEntryRole,           // [int] Index of the entry related to the currently active one (see
                                    //       PressedEntryRole, DayRole >> entries)
        LayoutRole                  // [QList<CalendarEntryLayout>] Layout of the activities in one column
    };

    explicit CalendarDetailedDayDelegate(TimeScaleData const * const timeScaleData, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    mutable HitTester entryTester;

private:
    TimeScaleData const * const timeScaleData;
    void drawWrappingText(QPainter &painter, const QRect &rect, const QString &text) const;
};


class CalendarHeadlineDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    enum Roles {
        DayRole = Qt::UserRole + 1 // [CalendarDay] Calendar day
    };

    explicit CalendarHeadlineDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    mutable HitTester headlineTester;

private:
    mutable int heightHint = 0;
};


class CalendarTimeScaleDelegate : public QStyledItemDelegate {
public:
    enum Roles {
        CurrentYRole = Qt::UserRole, // [int] Current Y value of the mousepointer
        BlockRole                    // [int / BlockIndicator] How the timescale should be marked as blocked
    };

    explicit CalendarTimeScaleDelegate(TimeScaleData *timeScaleData, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    TimeScaleData *timeScaleData;

    int selectNiceInterval(int minInterval) const;
};


class CalendarCompactDayDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    enum Roles {
        DayRole = Qt::UserRole + 1, // [CalendarDay] Calendar day
        PressedEntryRole,           // [int] Index of the currently pressed CalendarDay (see DayRole >> entries)
        RelatedEntryRole            // [int] Index of the entry related to the currently active one (see
                                    //       PressedEntryRole, DayRole >> entries)
    };

    explicit CalendarCompactDayDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;

    mutable HitTester headlineTester;
    mutable HitTester entryTester;
    mutable HitTester moreTester;
};


class CalendarSummaryDelegate : public QStyledItemDelegate {
public:
    enum Roles {
        SummaryRole = Qt::UserRole  // [CalendarSummary] Summary data
    };

    explicit CalendarSummaryDelegate(int horMargin = 4, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index) override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    mutable QHash<QModelIndex, bool> indexHasToolTip;
    const int horMargin;
    const int vertMargin = 4;
    const int lineSpacing = 2;
};


class AgendaMultiDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    enum Roles {
        // Qt::DisplayRole            // [QString] The default text to display
        // Qt::FontRole               // [QFont] The default font to use for text
        HoverTextRole = Qt::UserRole, // [QString] Hover text to display when hovered. If empty, the normal DisplayRole text is used
        HoverFontRole,                // [QFont] Hover font to use when hovered. If default-constructed, the normal font is used
        SecondaryDisplayRole,         // [QString] The default text to display in the second row
        SecondaryHoverTextRole,       // [QString] Hover text for the secondary row. Only used when SecondaryTextRole is filled
        SecondaryFontRole,            // [QFont] Font to be use for the secondary row. Use Qt::FontRole if not filled
        SecondaryHoverFontRole,       // [QFont] Hover font for secondary row to use when hovered. If unset, HoverFontRole is used
        TypeRole                      // [int] 0: Dual line text
                                      //       1: Headline
    };

    struct Attributes {
        QMargins paddingDL;  // Padding of the element (TypeRole == 0)
        QMargins paddingHL;  // Padding of the element (TypeRole == 1)
        int lineSpacing = 2; // Vertical spacing between primary and secondary row (dpiYFactor not applied)
    };


    explicit AgendaMultiDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    Attributes getAttributes() const;
    void setAttributes(const Attributes &attributes);

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

private:
    Attributes attributes;
};


class AgendaEntryDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    enum Roles {
        EntryRole = Qt::UserRole, // [CalendarEntry] Entry to be displayed
        EntryDateRole             // [bool] Date of the CalendarEntry
    };

    struct Attributes {
        QMargins padding;                                         // Padding of the element
        QFont::Weight primaryWeight = QFont::Medium;              // Primary row
        QFont::Weight primaryHoverWeight = QFont::DemiBold;       // Primary row (hovered)
        QFont::Weight secondaryWeight = QFont::Light;             // Secondary row
        QFont::Weight secondaryHoverWeight = QFont::DemiBold;     // Secondary row (hovered)
        QFont::Weight secondaryMetricWeight = QFont::ExtraLight;  // Metric in the secondary row
        QFont::Weight secondaryMetricHoverWeight = QFont::Normal; // Metric in the secondary row (hovered)
        int lineSpacing = 2;                                      // Vertical spacing between primary and secondary row (dpiYFactor not applied)
        int iconTextSpacing = 10;                                 // Horizontal spacing between icon and text (dpiXFactor not applied)
        float tertiaryDimLevel = 0.5;                             // Dimming amount for tertiary row
        int maxTertiaryLines = 3;                                 // Max number of displayed lines of tertiary text. Show a tooltip if exceeded
    };

    explicit AgendaEntryDelegate(QTreeWidget *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    Attributes getAttributes() const;
    void setAttributes(const Attributes &attributes);

    bool hasToolTip(const QModelIndex &index, QString *toolTipText = nullptr) const;

protected:
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

private:
    Attributes attributes;
    QTreeWidget *treeWidget;
    mutable int column = -1;
    int columnWidth = -1;

    void layoutTertiaryText(const QString &text, const QFont &font, int availableWidth, int &numLines, int &height, QStringList *lines, bool limitHeight) const;

    struct LayoutResult {
        int numLines;
        int height;
        QStringList lines;
    };
    LayoutResult processLayoutText(const QString &text, const QFont &font, int availableWidth, bool limitHeight, bool needLines) const;

private slots:
    void updateColumnWidth();
    void onSectionResized(int logicalIndex, int oldSize, int newSize);
};

#endif
