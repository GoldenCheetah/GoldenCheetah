/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#include <QObject>
#include <QUuid>
#include <QString>
#include <QVariant>

#include "AthleteTab.h"
#include "TimeUtils.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"

class NavigationEvent {

public:

    // events we observe and replay is a limited subset mostly
    // about changing views, dates and ride selections
    enum NavigationEventType { VIEW=0, RIDE, DATERANGE } type;

    NavigationEvent(NavigationEventType t, int v) : type(t) { after.setValue(v); }
    NavigationEvent(NavigationEventType t, DateRange v) : type(t) { after.setValue(v); }
    NavigationEvent(NavigationEventType t, RideItem* v) : type(t) { after.setValue(v); }
    NavigationEvent(NavigationEventType t, QString v) : type(t) { after.setValue(v); }

    // before is updated by the navigation model
    QVariant before, after;
};

//
// Navigation is for the athlete tab -- not mainwindow.
// Remember: multiple athletes can be opened at once.
//
class NavigationModel : public QObject
{
    Q_OBJECT

public:

    NavigationModel(AthleteTab *tab);
    ~NavigationModel();

    // keep a track of the rides we've looked
    // at recently-- might want to quick link
    // to these in the future ...
    QVector<RideItem*> recent;

public slots:

    void viewChanged(int);
    void rideChanged(RideItem*);
    void dateChanged(DateRange dr);

    void back();
    void forward();
    void action(bool redo, NavigationEvent); // redo/undo the event

private:

    AthleteTab *tab;

    // block observer when undo/redo is active
    bool block;

    // current state, before an event arrives
    int view; // which view is selected
    DateRange dr;
    RideItem *item;

    // have we seen first values for:
    bool viewinit, drinit, iteminit;

    // the stack
    void addToStack(NavigationEvent&); // truncates stack if needed
    int stackpointer;
    QVector<NavigationEvent> stack;
};
