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

#ifndef _GC_Tab_h
#define _GC_Tab_h 1

#include "TabView.h"
#include "Views.h"

class RideNavigator;
class MainWindow;
class ProgressLine;
class QPaintEvent;

class Tab: public QWidget
{
    Q_OBJECT

    public:

        Tab(Context *context);
        ~Tab();
        void close();

        ChartSettings *chartsettings() { return chartSettings; } // by HomeWindow
        int currentView() { return views->currentIndex(); }
        TabView *view(int index);

        RideNavigator *rideNavigator(); // to get logical headings
        IntervalNavigator *routeNavigator(); // to get logical headings
        IntervalNavigator *bestNavigator(); // to get logical headings

    protected:

        friend class ::MainWindow;
        Context *context;

    signals:

    public slots:

        void rideSelected(RideItem*);

        // set Ride
        void setRide(RideItem*);

        // tile mode
        void setTiled(bool);
        bool isTiled();
        void toggleTile();

        // sidebar
        void toggleSidebar();
        void setSidebarEnabled(bool);
        bool isSidebarEnabled();

        // bottom
        void setShowBottom(bool);
        bool isShowBottom();
        bool hasBottom();

        // layout
        void resetLayout();
        void addChart(GcWinID);

        // switch views
        void selectView(int);

        // specific to analysis view
        void addIntervals();

    private:


        // Each of the views
        QStackedWidget *views;
        AnalysisView *analysisView;
        HomeView *homeView;
        TrainView *trainView;
        DiaryView *diaryView;
        IntervalView *intervalView;

        // Chart Settings Dialog
        ChartSettings *chartSettings;
        QStackedWidget *masterControls,
                       *analysisControls,
                       *trainControls,
                       *diaryControls,
                       *homeControls,
                       *intervalControls;

};

// 1px high progressline only visible when refreshing ..
class ProgressLine : public QWidget
{
    Q_OBJECT

    public:
        ProgressLine(QWidget *parent, Context *context);

    public slots:
        void paintEvent (QPaintEvent *event);

    private:
        Context *context;
};
#endif // _GC_TabView_h
