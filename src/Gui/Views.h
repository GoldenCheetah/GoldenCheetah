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

#ifndef _GC_Views_h
#define _GC_Views_h 1

#include "TabView.h"
class TrainSidebar;
class AnalysisSidebar;
class IntervalSidebar;
class QDialog;
class RideNavigator;
class TrainBottom;

class AnalysisView : public TabView
{
    Q_OBJECT

    public:

        AnalysisView(Context *context, QStackedWidget *controls);
        ~AnalysisView();
        void close();
        void setRide(RideItem*ride);
        void addIntervals();

        RideNavigator *rideNavigator();

    public slots:

        bool isBlank();
        void compareChanged(bool);

    private:
        AnalysisSidebar *analSidebar;
        Perspective *hw;

};

class DiarySidebar;
class DiaryView : public TabView
{
    Q_OBJECT

    public:

        DiaryView(Context *context, QStackedWidget *controls);
        ~DiaryView();
        void setRide(RideItem*ride);

    public slots:

        bool isBlank();
        void dateRangeChanged(DateRange);

    private:
        DiarySidebar *diarySidebar;
        Perspective *hw;

};

class TrainView : public TabView
{
    Q_OBJECT

    public:

        TrainView(Context *context, QStackedWidget *controls);
        ~TrainView();
        void close();

    public slots:

        bool isBlank();
        void onSelectionChanged();

    private:

        TrainSidebar *trainTool;
        TrainBottom *trainBottom;
        Perspective *hw;

private slots:
        void onAutoHideChanged(bool enabled);
};

class LTMSidebar;
class HomeView : public TabView
{
    Q_OBJECT

    public:

        HomeView(Context *context, QStackedWidget *controls);
        ~HomeView();

        LTMSidebar *sidebar;
        Perspective *hw;

    signals:
        void dateChanged(DateRange);

    public slots:

        bool isBlank();
        void justSelected();
        void dateRangeChanged(DateRange);
        void compareChanged(bool);
};

#endif // _GC_Views_h
