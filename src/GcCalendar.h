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

#ifndef _GC_GcCalendar_h
#define _GC_GcCalendar_h 1

#include "GoldenCheetah.h"

#include <QtGui>
#include <QWebView>

#include "MainWindow.h"
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

public:
    GcLabel(const QString & text, QWidget * parent = 0) : QLabel(text, parent), xoff(0), yoff(0), bg(false), selected(false), filtered(false), bgColor(Qt::lightGray) {}
    ~GcLabel(){}
 
signals:
    void clicked();
 
public slots:
    void setYOff(int x) { yoff = x; }
    void setXOff(int x) { xoff = x; }
    void setBg(bool x) { bg = x; }
    bool getBg() { return bg; }
    void setBgColor(QColor bg) { bgColor = bg; }
    void setSelected(bool x) { selected = x; }
    void setFiltered(bool x) { filtered = x; }
    bool event(QEvent *e);

protected:
    void mouseReleaseEvent(QMouseEvent *) {
        emit clicked();
    }
    void paintEvent(QPaintEvent *);
    QColor bgColor; 
};

class GcMiniCalendar : public QWidget
{
    Q_OBJECT

    public:

        GcMiniCalendar(MainWindow *, bool master);

        void setDate(int month, int year);
        void getDate(int &_month, int &_year) { _month = month; _year = year; }
        void clearRide();

        void setFilter(QStringList filter);
        void clearFilter();

    public slots:

        void setRide(RideItem *ride);
        void refresh(); 

        void dayClicked(int num); // for when a day is selected
        void next();
        void previous();

        bool event(QEvent *e);

    signals:
        void dateChanged(int month, int year);

    protected:
        MainWindow *main;
        RideItem *_ride;
        int month, year;

        QVBoxLayout *layout;

        GcLabel *left, *right; // < ... >
        GcLabel *monthName; // January 2012
        GcLabel *dayNames[7]; // Mon .. Sun

        QList<GcLabel*> dayLabels; // 1 .. 31

        QSignalMapper *signalMapper; // for mapping dayLabels "clicked"

        QPalette black, grey, white;
        QList<FieldDefinition> fieldDefinitions;
        GcCalendarModel *calendarModel;
        bool master;

        QStringList filters;
};

class GcMultiCalendar : public QScrollArea
{
    Q_OBJECT

    public:

        GcMultiCalendar(MainWindow*);
        void refresh();

    public slots:
        void dateChanged(int month, int year);
        void setRide(RideItem *ride);
        void resizeEvent(QResizeEvent*);
        void setFilter(QStringList filter);
        void clearFilter();


    private:
        GcWindowLayout *layout;
        QVector<GcMiniCalendar*> calendars;
        MainWindow *main;
        int showing;
        QStringList filters;
        bool active;
};

class GcCalendar : public QWidget // not a GcWindow - belongs on sidebar
{
    Q_OBJECT
    G_OBJECT

    public:

        GcCalendar(MainWindow *);

    public slots:

        void setRide(RideItem *ride);
        void refresh(); 
        void setSummary(); // set the summary at the bottom

        void setFilter(QStringList filters) { multiCalendar->setFilter(filters);}
        void clearFilter() { multiCalendar->clearFilter();}

    signals:
        void dateRangeChanged(DateRange);

    protected:
        MainWindow *main;
        RideItem *_ride;
        int month, year;

        QVBoxLayout *layout;
        QGridLayout *dayLayout; // contains the day names and days

        GcMultiCalendar *multiCalendar;

        QPalette black, grey, white;
        GcSplitter *splitter; // calendar vs summary
        GcSplitterItem *calendarItem,
                       *summaryItem;

        QComboBox *summarySelect;
        QWebView *summary;
        QDate from, to;
};
#endif // _GC_GcCalendar_h
