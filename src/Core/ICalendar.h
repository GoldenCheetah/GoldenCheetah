/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_ICalendar_h
#define _GC_ICalendar_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include "Context.h"
#include "RideMetric.h"
#include <libical/ical.h>

class ICalendar : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:

        ICalendar(Context *context);

        // to get access to from the Calendar
        QVariant data(QDate date, int role = Qt::DisplayRole);
        void clearCalendar(QMap<QDate, QList<icalcomponent*>*>&calendar, bool signal=true);
        void refreshRemote(QString);

    signals:
        void dataChanged(); // when we get updated or refreshed

    private slots:
        void parse(QString fulltext, QMap<QDate, QList<icalcomponent*>*>&calendar);

    private:

        Context *context;

        // We store the calendar as an associative array
        // For each date we have a list of events that
        // occur on that day. One for the local calendar
        // stored in cyclisthome/calendar.ics and one
        // for a remote calendar we configure in options
        //
        // XXX Todo: support for multiple calendars instead
        //           of only one local, one remote.
        QMap<QDate, QList<icalcomponent*>*> localCalendar;
        QMap<QDate, QList<icalcomponent*>*> remoteCalendar;
        static void setICalendarProperties();
};

#endif // _GC_ICalendar_h

