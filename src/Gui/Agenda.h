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

#ifndef AGENDA_H
#define AGENDA_H

#include <QtGui>
#include <QLabel>
#include <QTreeWidget>

#include "CalendarItemDelegates.h"
#include "CalendarData.h"
#include "Measures.h"


class AgendaTree : public QTreeWidget {
    Q_OBJECT

public:
    explicit AgendaTree(QWidget *parent = nullptr);

    void updateDate();
    QDate selectedDate() const;

    bool isRowHovered(const QModelIndex &index) const;

public slots:
    void setMaxTertiaryLines(int maxTertiaryLines);

signals:
    void dayChanged(const QDate &date);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void leaveEvent(QEvent *event) override;
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEvent *event) override;
#else
    void enterEvent(QEnterEvent *event) override;
#endif
    void contextMenuEvent(QContextMenuEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool viewportEvent(QEvent *event) override;
    void drawRow(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    virtual QMenu *createContextMenu(const QModelIndex &index);

    void clearHover();

protected:
    struct Fonts {
        QFont defaultFont;
        QFont relativeFont;
        QFont hoverFont;
        QFont headlineDefaultFont;
        QFont headlineTodayFont;
        QFont headlineEmptyFont;
        QFont headlineSmallEmptyFont;
    };

    void fillFonts(Fonts &fonts) const;

    AgendaMultiDelegate *multiDelegate;
    AgendaEntryDelegate *entryDelegate;

private:
    QDate currentDate;
    QPersistentModelIndex hoveredIndex;
    QPersistentModelIndex contextMenuIndex;

    void updateHoveredIndex(const QPoint &pos);
};


class ActivityTree : public AgendaTree {
    Q_OBJECT

public:
    explicit ActivityTree(QWidget *parent = nullptr);

    void setPastDays(int days);
    void setFutureDays(int days);
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activities);
    QDate firstVisibleDay() const;
    QDate lastVisibleDay() const;

signals:
    void showInTrainMode(const CalendarEntry &activity);
    void viewActivity(const CalendarEntry &activity);

protected:
    QMenu *createContextMenu(const QModelIndex &index) override;

private:
    int pastDays = 7;
    int futureDays = 7;

    void addEntries(const QDate &today, const QDate &date, const QList<CalendarEntry> &activities, QTreeWidgetItem *parent, const Fonts &fonts);
};


class PhaseTree : public AgendaTree {
    Q_OBJECT

public:
    void fillEntries(const std::pair<QList<CalendarEntry>, QList<CalendarEntry>> &phases);

signals:
    void editPhaseEntry(const CalendarEntry &entry);

protected:
    QMenu *createContextMenu(const QModelIndex &index) override;

private:
    void addEntries(const QDate &today, const QList<CalendarEntry> &phases, QTreeWidgetItem *parent, const Fonts &fonts);
};


class EventTree : public AgendaTree {
    Q_OBJECT

public:
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &events);

signals:
    void editEventEntry(const CalendarEntry &entry);

protected:
    virtual QMenu *createContextMenu(const QModelIndex &index);

private:
    void addEntries(const QDate &today, const QList<CalendarEntry> &phases, QTreeWidgetItem *parent, const Fonts &fonts);
};


class AgendaView : public QWidget {
    Q_OBJECT

public:
    explicit AgendaView(QWidget *parent = nullptr);

    void updateDate();
    void setDateRange(const DateRange &dateRange);
    void setPastDays(int days);
    void setFutureDays(int days);
    void fillEntries(const QHash<QDate, QList<CalendarEntry>> &activities, std::pair<QList<CalendarEntry>, QList<CalendarEntry>> &phases, const QHash<QDate, QList<CalendarEntry>> &events, const QString &seasonName, bool isFiltered);

    QDate firstVisibleDay() const;
    QDate lastVisibleDay() const;
    QDate selectedDate() const;

public slots:
    void setActivityMaxTertiaryLines(int maxTertiaryLines);
    void setEventMaxTertiaryLines(int maxTertiaryLines);

signals:
    void showInTrainMode(const CalendarEntry &activity);
    void viewActivity(const CalendarEntry &activity);
    void editPhaseEntry(const CalendarEntry &entry);
    void editEventEntry(const CalendarEntry &entry);
    void dayChanged(const QDate &date);

protected:
    void changeEvent(QEvent *event) override;

private:
    QLabel *filterLabel;
    QLabel *seasonLabel;
    ActivityTree *activityTree;
    PhaseTree *phaseTree;
    EventTree *eventTree;
};

#endif
