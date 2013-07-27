/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_TabView_h
#define _GC_TabView_h 1

#include <QWidget>
#include <QSplitter>
#include <QMetaObject>
#include <QStackedWidget>

#include "HomeWindow.h"
#include "GcWindowRegistry.h"

class Tab;
class Context;
class BlankStatePage;

class TabView : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(bool sidebar READ sidebarEnabled WRITE setSidebarEnabled USER true)
    Q_PROPERTY(bool tiled READ isTiled WRITE setTiled USER true)
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected USER true) // make this last always

    public:

        TabView(Context *context, int type);
        virtual ~TabView();

        // add the widgets to the view
        void setSidebar(QWidget *sidebar);
        void setPage(HomeWindow *page);
        void setBlank(BlankStatePage *blank);

        // sidebar
        void setSidebarEnabled(bool x) { _sidebar=x; sidebarChanged(); }
        bool sidebarEnabled() const { return _sidebar; }

        // tiled
        void setTiled(bool x) { _tiled=x; tileModeChanged(); }
        bool isTiled() const { return _tiled; }

        // select / deselect view
        void setSelected(bool x) { _selected=x; selectionChanged(); }
        bool isSelected() const { return _selected; }

    signals:

        void sidebarClosed(); // the user dragged the sidebar closed.

    public slots:

        // interface used by the Tab class - must be implemented by a TabView
        virtual bool isBlank() = 0;

        // can be overriden by the derived class but we provide a working version
        virtual void sidebarChanged();
        virtual void tileModeChanged();
        virtual void selectionChanged();
        virtual void resetLayout();
        //virtual void setChartMenu(); // need to be in Tab 
        //virtual void setSubChartMenu(); // need to be in Tab and scopebar by the looks of it 
        virtual void addChart(GcWinID id);

        // Let the base class handle the splitter movement and
        // hiding the sidebar by dragging it closed.
        void splitterMoved(int, int);

        //void dateRangeChanged(DateRange);
        //void rideSelected(RideItem*ride);
        //void mediaSelected(QString filename);
        //void ergSelected(ErgFile *erg);

    private:

        Context *context;
        int type; // used by windowregistry; e.g VIEW_TRAIN VIEW_ANALYSIS VIEW_DIARY VIEW_HOME
                  // we don't care what values are pass through to the GcWindowRegistry to decide
                  // what charts are relevant for this view.

        // properties
        bool _filtered;
        QStringList _filter;
        bool _sidebar;
        bool _tiled;
        bool _selected;

        QStackedWidget *stack;
        QSplitter *splitter;
        QWidget *sidebar;
        HomeWindow *page;
        BlankStatePage *blank;

};

#endif // _GC_TabView_h
