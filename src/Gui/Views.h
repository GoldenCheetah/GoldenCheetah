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

#include "AbstractView.h"
class TrainSidebar;
class AnalysisSidebar;
class IntervalSidebar;
class QDialog;
class RideNavigator;
class TrainBottom;

class AnalysisView : public AbstractView
{
    Q_OBJECT

    public:

        AnalysisView(Context *context, QStackedWidget *controls);
        ~AnalysisView();
        void close() override;
        void setRide(RideItem*ride) override;
        void addIntervals();

        RideNavigator *rideNavigator();
        AnalysisSidebar *analSidebar;

    public slots:

        bool isBlank() override;
        void compareChanged(bool);

    protected:

        void notifyViewStateRestored() override;
        void notifyViewSidebarChanged() override;
        void setViewSpecificPerspective() override;
        void notifyViewSplitterMoved() override;

};

class PlanSidebar;
class PlanView : public AbstractView
{
    Q_OBJECT

    public:

        PlanView(Context *context, QStackedWidget *controls);
        ~PlanView();
        void setRide(RideItem*ride) override;

    public slots:

        bool isBlank() override;
        void dateRangeChanged(DateRange);

    private:
        PlanSidebar *planSidebar;

};

class TrainView : public AbstractView
{
    Q_OBJECT

    public:

        TrainView(Context *context, QStackedWidget *controls);
        ~TrainView();
        void close() override;

    public slots:

        bool isBlank() override;
        void onSelectionChanged();

    protected:

        void notifyViewPerspectiveAdded(Perspective* page) override;

    private:

        TrainSidebar *trainTool;
        TrainBottom *trainBottom;

private slots:
        void onAutoHideChanged(bool enabled);
};

class LTMSidebar;
class TrendsView : public AbstractView
{
    Q_OBJECT

    public:

        TrendsView(Context *context, QStackedWidget *controls);
        ~TrendsView();

        LTMSidebar *sidebar;

        int countActivities(Perspective *, DateRange dr);

    signals:
        void dateChanged(DateRange);

    public slots:

        bool isBlank() override;
        void justSelected();
        void dateRangeChanged(DateRange);
        void compareChanged(bool);
};

#endif // _GC_Views_h
