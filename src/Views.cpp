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

#include "Views.h"
#include "AnalysisSidebar.h"
#include "DiarySidebar.h"
#include "TrainSidebar.h"
#include "LTMSidebar.h"
#include "BlankState.h"
#include "TrainDB.h"

AnalysisView::AnalysisView(Context *context, QStackedWidget *controls) : TabView(context, VIEW_ANALYSIS)
{
    AnalysisSidebar *s = new AnalysisSidebar(context);
    HomeWindow *a = new HomeWindow(context, "analysis", "Analysis");
    controls->addWidget(a->controls());
    controls->setCurrentIndex(0);
    BlankStateAnalysisPage *b = new BlankStateAnalysisPage(context);

    setSidebar(s);
    setPage(a);
    setBlank(b);
}

AnalysisView::~AnalysisView()
{
}

void
AnalysisView::setRide(RideItem *ride)
{
    static_cast<AnalysisSidebar*>(sidebar())->setRide(ride); // save settings
    page()->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
}

void
AnalysisView::addIntervals()
{
    static_cast<AnalysisSidebar*>(sidebar())->addIntervals(); // save settings
}

void AnalysisView::close()
{
    static_cast<AnalysisSidebar*>(sidebar())->close(); // save settings
}

bool
AnalysisView::isBlank()
{
    if (context->athlete->allRides->childCount() > 0) return false;
    else return true;
}

DiaryView::DiaryView(Context *context, QStackedWidget *controls) : TabView(context, VIEW_DIARY)
{
    DiarySidebar *s = new DiarySidebar(context);
    HomeWindow *d = new HomeWindow(context, "diary", "Diary");
    controls->addWidget(d->controls());
    controls->setCurrentIndex(0);
    BlankStateDiaryPage *b = new BlankStateDiaryPage(context);

    setSidebar(s);
    setPage(d);
    setBlank(b);

    connect(s, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
}

DiaryView::~DiaryView()
{
}

void
DiaryView::setRide(RideItem*ride)
{
    static_cast<DiarySidebar*>(sidebar())->setRide(ride);
    page()->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
}

void
DiaryView::dateRangeChanged(DateRange dr)
{
    context->dr_ = dr;
    page()->setProperty("dateRange", QVariant::fromValue<DateRange>(dr));
}

bool
DiaryView::isBlank()
{
    if (context->athlete->allRides->childCount() > 0) return false;
    else return true;
}

HomeView::HomeView(Context *context, QStackedWidget *controls) : TabView(context, VIEW_HOME)
{
    LTMSidebar *s = new LTMSidebar(context);
    HomeWindow *h = new HomeWindow(context, "home", "Home");
    controls->addWidget(h->controls());
    controls->setCurrentIndex(0);
    BlankStateHomePage *b = new BlankStateHomePage(context);

    setSidebar(s);
    setPage(h);
    setBlank(b);

    connect(s, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    connect(this, SIGNAL(onSelected()), this, SLOT(justSelected()));
}

HomeView::~HomeView()
{
}

void
HomeView::dateRangeChanged(DateRange dr)
{
    context->dr_ = dr;
    page()->setProperty("dateRange", QVariant::fromValue<DateRange>(dr));
}
bool
HomeView::isBlank()
{
    if (context->athlete->allRides->childCount() > 0) return false;
    else return true;
}

void
HomeView::justSelected()
{
    // force date range refresh
    static_cast<LTMSidebar*>(sidebar())->dateRangeTreeWidgetSelectionChanged();
}

TrainView::TrainView(Context *context, QStackedWidget *controls) : TabView(context, VIEW_TRAIN)
{
    TrainSidebar *s = new TrainSidebar(context);
    s->hide();
    s->getToolbarButtons()->hide(); // no show yet

    HomeWindow *t = new HomeWindow(context, "train", "train");
    controls->addWidget(t->controls());
    controls->setCurrentIndex(0);
    BlankStateTrainPage *b = new BlankStateTrainPage(context);

    setSidebar(s->controls());
    setPage(t);
    setBlank(b);

}

TrainView::~TrainView()
{
}

void
TrainView::close()
{
    static_cast<TrainSidebar*>(sidebar())->Stop();
}

bool
TrainView::isBlank()
{
    if (appsettings->value(this, GC_DEV_COUNT).toInt() > 0 && trainDB->getCount() > 2) return false;
    else return true;
}
