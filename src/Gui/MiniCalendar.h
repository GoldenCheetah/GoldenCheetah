/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_MiniCalendar_h
#define _GC_MiniCalendar_h 1

#include "GoldenCheetah.h"

#include <QtGui>

#include "Context.h"
#include "Athlete.h"
#include "TimeUtils.h"
#include "GcCalendarModel.h"
#include "RideItem.h"
#include "RideNavigator.h"

#include "GcSideBarItem.h"

class GcWindowLayout;

// Catch signal, no background and do embossed text
class GcLabel : public QLabel
{
    Q_OBJECT

    int xoff, yoff;
    bool bg, selected, filtered; // bg = highlighted, selected = user selected too
    bool highlighted;            // highlighed uses highlighter color overlay when painting
    QColor bgColor; 
    bool isChrome;

public:
    GcLabel(const QString & text, QWidget * parent = 0) : QLabel(text, parent), xoff(0), yoff(0), bg(false), selected(false), filtered(false), highlighted(false), bgColor(Qt::lightGray), isChrome(false) {}
    ~GcLabel(){}
 
signals:
    void clicked();
 
public slots:
    void setYOff(int x) { yoff = x; }
    void setXOff(int x) { xoff = x; }
    void setChrome(bool x) { isChrome = x; }
    void setBg(bool x) { bg = x; }
    bool getBg() { return bg; }
    void setBgColor(QColor bg) { bgColor = bg; }
    void setSelected(bool x) { selected = x; }
    void setFiltered(bool x) { filtered = x; }
    void setHighlighted(bool x) { highlighted = x; }
    bool event(QEvent *e);

protected:
    void mouseReleaseEvent(QMouseEvent *) {
        emit clicked();
    }
    void paintEvent(QPaintEvent *);
};

class GcMiniCalendar : public QWidget
{
    Q_OBJECT

    public:

        GcMiniCalendar(Context *, bool master);

        void setDate(int month, int year);
        void getDate(int &_month, int &_year) { _month = month; _year = year; }
        void clearRide();

        void setFilter(QStringList filter);
        void clearFilter();

    public slots:

        void configChanged(qint32);

        void setRide(RideItem *ride);
        void refresh(); 

        void dayClicked(int num); // for when a day is selected
        void next();
        void previous();

        bool event(QEvent *e);

    signals:
        void dateChanged(int month, int year);

    protected:
        Context *context;
        RideItem *_ride;
        int month, year;

        QVBoxLayout *layout;

        GcLabel *left, *right; // < ... >
        GcLabel *monthName; // January 2012
        GcLabel *dayNames[7]; // Mon .. Sun

        QList<GcLabel*> dayLabels; // 1 .. 31

        QSignalMapper *signalMapper; // for mapping dayLabels "clicked"

        QPalette tint, grey, white;
        QList<FieldDefinition> fieldDefinitions;
        GcCalendarModel *calendarModel;
        bool master;

        QStringList filters;
        QWidget *monthWidget;
};

class GcMultiCalendar : public QScrollArea
{
    Q_OBJECT

    public:

        GcMultiCalendar(Context *);
        void refresh();

    public slots:
        void dateChanged(int month, int year);
        void setRide(RideItem *ride);
        void resizeEvent(QResizeEvent*);
        void filterChanged();
        void setFilter(QStringList filter);
        void clearFilter();
        void showEvent(QShowEvent*);
        void configChanged(qint32);

    private:
        GcWindowLayout *layout;
        QVector<GcMiniCalendar*> calendars;
        Context *context;
        int showing;
        QStringList filters;
        bool active;
        bool stale; // we need to redraw when shown
        RideItem *_ride;
};

#endif // _GC_MiniCalendar_h
