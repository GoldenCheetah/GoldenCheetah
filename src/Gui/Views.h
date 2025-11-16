/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
 * LTMSidebarView Copyright (c) 2025 Paul Johnson (paulj49457@gmail.com)
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
class LTMSidebar;
class IntervalSidebar;
class QDialog;
class RideNavigator;
class TrainBottom;

// the LTMSidebarView class manages the sharing of the Long Term Metrics (LTM) sidebar
// between the trends and plan views, each LTMSidebar instance is shared between the
// views for the same context/athlete.
class LTMSidebarView : public AbstractView
{
    Q_OBJECT

    public:

        static void selectDateRange(Context *sbContext, DateRange dr);

    signals:
        void dateChanged(DateRange);

    protected slots:

        void dateRangeChanged(DateRange);

    protected:

        LTMSidebarView(Context *context, int type, const QString& view, const QString& heading);
        virtual ~LTMSidebarView();

        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;

        LTMSidebar* getLTMSidebar(Context *sbContext);
        void removeLTMSidebar(Context *sbContext);

    private:

        // each athlete has their own LTMSidebar shared by the plan & trends views.
        static QMap<Context*, LTMSidebar*> LTMSidebars_;
};

class AnalysisView : public AbstractView
{
    Q_OBJECT

    public:

        AnalysisView(Context *context, QStackedWidget *controls);
        virtual ~AnalysisView();
        void close() override;
        void setRide(RideItem*ride) override;
        void addIntervals();

        RideNavigator *rideNavigator();
        AnalysisSidebar *analSidebar;

    public slots:

        bool isBlank() override;
        void compareChanged(bool);

    protected:

        void notifyViewSidebarChanged() override;
        int getViewSpecificPerspective() override;
        void notifyViewSplitterMoved() override;

    private:

        int findRidesPerspective(RideItem* ride);
};

class PlanView : public LTMSidebarView
{
    Q_OBJECT

    public:

        PlanView(Context *context, QStackedWidget *controls);
        virtual ~PlanView();

    public slots:

        bool isBlank() override;
        void justSelected();
};

class TrainView : public AbstractView
{
    Q_OBJECT

    public:

        TrainView(Context *context, QStackedWidget *controls);
        virtual ~TrainView();
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

class TrendsView : public LTMSidebarView
{
    Q_OBJECT

    public:

        TrendsView(Context *context, QStackedWidget *controls);
        virtual ~TrendsView();

        int countActivities(Perspective *, DateRange dr);

    public slots:

        bool isBlank() override;
        void justSelected();
        void compareChanged(bool);
};

#endif // _GC_Views_h
