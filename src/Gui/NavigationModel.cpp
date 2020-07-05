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

#include "NavigationModel.h"

#include "RideCache.h"
// a little too intertwined with these two,
// probably needs refactoring out at some point.
#include "LTMSidebar.h"
#include "NewSideBar.h"
#include "MainWindow.h"


NavigationModel::NavigationModel(Tab *tab) : tab(tab), block(false), viewinit(false), iteminit(false), drinit(false)
{
    connect(tab, SIGNAL(viewChanged(int)), this, SLOT(viewChanged(int)));
    connect(tab, SIGNAL(rideItemSelected(RideItem*)), this, SLOT(rideChanged(RideItem*)));
    connect(static_cast<HomeView*>(tab->view(0))->sidebar, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateChanged(DateRange)));
    connect(tab->context->mainWindow, SIGNAL(backClicked()), this, SLOT(back()));
    connect(tab->context->mainWindow, SIGNAL(forwardClicked()), this, SLOT(forward()));

    stackpointer=-1;
}

NavigationModel::~NavigationModel()
{
}

void
NavigationModel::addToStack(NavigationEvent &event)
{
    if (stackpointer == -1) {
        // empty
        stack.clear();
        stack << event;
        stackpointer++;

    } else if (stackpointer == (stack.count()-1)) {

        // end, just append
        stack << event;
        stackpointer++;

    } else if (stackpointer < (stack.count()-1)) {

        // middle, truncate
        stack.remove(stackpointer+1, stack.count() - (stackpointer+1));
        stack << event;
        stackpointer++;

    }
}

void
NavigationModel::rideChanged(RideItem *x)
{
    if (block) return;

    // initial values
    if (iteminit == false) {
        iteminit = true;
        item = x;
        return;
    }

    if (item != x) {

        // add to recent
        if (!recent.contains(x)) recent << x;

        // add to the stack -- truncate if needed
        NavigationEvent add(NavigationEvent::RIDE, x);
        add.before.setValue(item);
        addToStack(add);

        item = x;
    }
}

void
NavigationModel::dateChanged(DateRange x)
{
    if (block) return;

    // initial values
    if (drinit == false) {
        drinit = true;
        dr = x;
        return;
    }

    if (dr != x) {

        // add to the stack
        NavigationEvent add(NavigationEvent::DATERANGE, x);
        add.before.setValue(dr);
        addToStack(add);

        dr = x;
    }
}

void
NavigationModel::viewChanged(int x)
{
    if (block) return;

    // initial values
    if (viewinit == false) {
        viewinit = true;
        view = x;
        return;
    }

    if (view != x) {

        // add to the stack
        NavigationEvent add(NavigationEvent::VIEW, x);
        add.before.setValue(view);
        addToStack(add);

        view = x;
    }
}

void
NavigationModel::action(bool redo, NavigationEvent event)
{
    block = true; // don't observe events during redo/undo

    switch(event.type) {
    case NavigationEvent::VIEW:
    {
        int view = redo ? event.after.toInt() : event.before.toInt();
        tab->selectView(view);

        // new side bar uses a different id, which will
        // eventually be refactored to be the only id
        // but for now we need to map this
        int id=0;
        switch(view) {
        case 0: id=2; break; // trends
        case 1: id=3; break; // analysis
        case 2: id=0; break; // diary
        case 3: id=5; break; // train
        }
        tab->context->mainWindow->newSidebar()->setItemSelected(id, true);
    }
    break;

    case NavigationEvent::RIDE:
    {
        RideItem *item = redo ? event.after.value<RideItem*>() : event.before.value<RideItem*>();

        // don't select deleted rides (!!)
        if (!tab->context->athlete->rideCache->deletelist.contains(item))
            tab->context->athlete->selectRideFile(item->fileName);
    }
    break;

    case NavigationEvent::DATERANGE:
    {
        DateRange dr = redo ? event.after.value<DateRange>() : event.before.value<DateRange>();
        static_cast<HomeView*>(tab->view(0))->sidebar->selectDateRange(dr);
    }
    break;
    }

    block = false;
}

void
NavigationModel::back()
{
    // are we the current tab?
    if (tab->context->mainWindow->athleteTab() == tab) {
        if (stackpointer >= 0) {
            action(false, stack[stackpointer]);
            stackpointer--;
        }
    }
}

void
NavigationModel::forward()
{
    // are we the current tab?
    if (tab->context->mainWindow->athleteTab() == tab) {
        if ((stackpointer+1) < stack.count() && stack.count()) {
            stackpointer++;
            action(true, stack[stackpointer]);
        }
    }
}
