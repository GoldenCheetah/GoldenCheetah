/*
 * Copyright (c) 2026 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef PLANADHERENCE_H
#define PLANADHERENCE_H

#include <QtGui>
#include <QLabel>
#include <QTreeWidget>
#include <QHeaderView>
#include <QStackedWidget>

#include "TimeUtils.h"
#include "Colors.h"


class StatisticBox : public QFrame {
    Q_OBJECT

public:
    enum class DisplayState {
        Primary,
        Secondary
    };

    explicit StatisticBox(const QString &title, const QString &description, DisplayState startupPreferred = DisplayState::Primary, QWidget* parent = nullptr);

    void setValue(const QString &primaryValue, const QString &secondaryValue = QString());
    void setPreferredState(const DisplayState &state);

protected:
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    QLabel *valueLabel;
    QLabel *titleLabel;
    QString primaryValue;
    QString secondaryValue;
    QString description;
    DisplayState startupPreferredState;
    DisplayState currentPreferredState;
    DisplayState currentState;
};


class JointLineTree : public QTreeWidget {
    Q_OBJECT

public:
    explicit JointLineTree(QWidget *parent = nullptr);

    bool isRowHovered(const QModelIndex &index) const;
    virtual QColor getHoverBG() const;
    virtual QColor getHoverHL() const;

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool viewportEvent(QEvent *event) override;
    void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    virtual QMenu *createContextMenu(const QModelIndex &index) = 0;
    virtual QString createToolTipText(const QModelIndex &index) const = 0;

    void clearHover();

private:
    QPersistentModelIndex hoveredIndex;
    QPersistentModelIndex contextMenuIndex;

    void updateHoveredIndex(const QPoint &pos);
};


struct PlanAdherenceStatistics {
    int totalAbs = 0;
    int plannedAbs = 0;
    float plannedRel = 0;
    int onTimeAbs = 0;
    float onTimeRel = 0;
    int shiftedAbs = 0;
    float shiftedRel = 0;
    int missedAbs = 0;
    float missedRel = 0;
    float avgShift = 0;
    int unplannedAbs = 0;
    float unplannedRel = 0;
    int totalShiftDaysAbs = 0;
};


struct PlanAdherenceEntry {
    QString titlePrimary;
    QString titleSecondary;
    QString iconFile;
    QColor color;
    QDate date;
    bool isPlanned;
    std::optional<qint64> shiftOffset;
    std::optional<qint64> actualOffset;
    QString plannedReference;
    QString actualReference;
};
Q_DECLARE_METATYPE(PlanAdherenceEntry)


struct PlanAdherenceOffsetRange {
    qint64 min = -1;
    qint64 max = 1;
};


class PlanAdherenceOffsetHandler {
public:
    void setMinAllowedOffset(int minOffset);
    void setMaxAllowedOffset(int maxOffset);
    void setMinEntryOffset(int minOffset);
    void setMaxEntryOffset(int maxOffset);

    bool hasMinOverflow() const;
    bool hasMaxOverflow() const;

    int minVisibleOffset() const;
    int maxVisibleOffset() const;

    bool isMinOverflow(int offset) const;
    bool isMaxOverflow(int offset) const;
    int indexFromOffset(int offset) const;
    int numCells() const;

protected:
    int minAllowedOffset = -1;
    int maxAllowedOffset = 1;
    int minEntryOffset = -1;
    int maxEntryOffset = 1;
};


namespace PlanAdherence {
    enum Roles {
        DataRole = Qt::UserRole + 1   // [PlanAdherenceEntry] Data to be visualized
    };
}


class PlanAdherenceTitleDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit PlanAdherenceTitleDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    const int leftPadding = 10 * dpiXFactor;
    const int horMargin = 4 * dpiXFactor;
    const int vertMargin = 6 * dpiYFactor;
    const int lineSpacing = 2 * dpiYFactor;
    const int iconInnerSpacing = 2 * dpiXFactor;
    const int iconTextSpacing = 8 * dpiXFactor;
    const int maxWidth = 300 * dpiXFactor;
};


class PlanAdherenceOffsetDelegate : public QStyledItemDelegate, public PlanAdherenceOffsetHandler {
    Q_OBJECT

public:
    explicit PlanAdherenceOffsetDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    const int horMargin = 4 * dpiXFactor;
    const int vertMargin = 6 * dpiYFactor;
    const int lineSpacing = 2 * dpiYFactor;
};


class PlanAdherenceStatusDelegate : public QStyledItemDelegate {
public:
    enum Roles {
        Line1Role = Qt::UserRole + 1, // [QString] First line to be shown
        Line2Role,                    // [QString] Second line to be shown (optional, first line will be vertically centered if this is not available)
        Line2WeightRole               // [QFont::Weight] Font weight to be used for line 2
    };

    explicit PlanAdherenceStatusDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    const int rightPadding = 10 * dpiXFactor;
    const int horMargin = 4 * dpiXFactor;
    const int vertMargin = 6 * dpiYFactor;
    const int lineSpacing = 2 * dpiYFactor;
};


class PlanAdherenceHeaderView : public QHeaderView, public PlanAdherenceOffsetHandler {
    Q_OBJECT

public:
    explicit PlanAdherenceHeaderView(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;

private:
    const int horMargin = 4 * dpiXFactor;
};


class PlanAdherenceTree : public JointLineTree {
    Q_OBJECT

public:
    explicit PlanAdherenceTree(QWidget *parent = nullptr);

    void fillEntries(const QList<PlanAdherenceEntry> &entries, const PlanAdherenceOffsetRange &offsets);

    QColor getHoverBG() const;
    QColor getHoverHL() const;

    void setMinAllowedOffset(int offset);
    void setMaxAllowedOffset(int offset);

signals:
    void viewActivity(QString reference, bool planned);

protected:
    QMenu *createContextMenu(const QModelIndex &index) override;
    QString createToolTipText(const QModelIndex &index) const override;

private:
    PlanAdherenceOffsetDelegate *planAdherenceOffsetDelegate;
    PlanAdherenceHeaderView *headerView;
    int minAllowedOffset = -1;
    int maxAllowedOffset = 5;
};


class PlanAdherenceView : public QWidget {
    Q_OBJECT

public:
    explicit PlanAdherenceView(QWidget *parent = nullptr);

    QDate firstVisibleDay() const;
    QDate lastVisibleDay() const;
    void fillEntries(const QList<PlanAdherenceEntry> &entries, const PlanAdherenceStatistics &statistics, const PlanAdherenceOffsetRange &offsets, bool isFiltered);

public slots:
    void setDateRange(const DateRange &dateRange);
    void setMinAllowedOffset(int offset);
    void setMaxAllowedOffset(int offset);
    void setPreferredStatisticsDisplay(int mode);
    void applyNavIcons();

signals:
    void monthChanged(QDate month);
    void viewActivity(QString reference, bool planned);

private:
    QToolBar *toolbar;
    QAction *prevAction;
    QAction *nextAction;
    QAction *separator;
    QAction *todayAction;
    QAction *dateNavigatorAction;
    QAction *filterLabelAction;
    QToolButton *dateNavigator;
    QMenu *dateMenu;
    QStackedWidget *viewStack;
    PlanAdherenceTree *timeline;
    StatisticBox *totalBox;
    StatisticBox *plannedBox;
    StatisticBox *unplannedBox;
    StatisticBox *onTimeBox;
    StatisticBox *shiftedBox;
    StatisticBox *missedBox;
    StatisticBox *avgShiftBox;
    StatisticBox *totalShiftDaysBox;
    DateRange dateRange;
    QDate dateInMonth;

    void setNavButtonState();
    void updateHeader();

private slots:
    void populateDateMenu();
};

#endif
