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
#include "RideCache.h"
#include "AnalysisSidebar.h"
#include "DiarySidebar.h"
#include "TrainSidebar.h"
#include "LTMSidebar.h"
#include "BlankState.h"
#include "TrainDB.h"
#include "ComparePane.h"
#include "TrainBottom.h"
#include "Specification.h"

#include <QDesktopWidget>
extern QDesktopWidget *desktop;

AnalysisView::AnalysisView(Context *context, QStackedWidget *controls) : AbstractView(context, VIEW_ANALYSIS)
{
    analSidebar = new AnalysisSidebar(context);
    BlankStateAnalysisPage *b = new BlankStateAnalysisPage(context);

    setSidebar(analSidebar);

    // perspectives are stacked
    pstack = new QStackedWidget(this);
    setPages(pstack);

    // each perspective has a stack of controls
    cstack = new QStackedWidget(this);
    controls->addWidget(cstack);
    controls->setCurrentIndex(0);

    setBlank(b);
    setBottom(new ComparePane(context, this, ComparePane::interval));

    setSidebarEnabled(appsettings->value(this, GC_SETTINGS_MAIN_SIDEBAR "analysis", true).toBool());

    connect(bottomSplitter(), SIGNAL(compareChanged(bool)), this, SLOT(compareChanged(bool)));
    connect(bottomSplitter(), SIGNAL(compareClear()), bottom(), SLOT(clear()));
}

RideNavigator *AnalysisView::rideNavigator()
{
    return context->rideNavigator;
}

AnalysisView::~AnalysisView()
{
    appsettings->setValue(GC_SETTINGS_MAIN_SIDEBAR "analysis", _sidebar);
    delete analSidebar;
    //delete hw; tabview deletes after save state
}

void
AnalysisView::setRide(RideItem *ride)
{
    if (!loaded) return; // not loaded yet, all bets are off till then.

    // when ride selected, but not from the sidebar.
    static_cast<AnalysisSidebar*>(sidebar())->setRide(ride); // save settings

    // if we are the current view and the current perspective is no longer relevant
    // then lets go find one to switch to..
    if (context->mainWindow->athleteTab()->currentView() == 1 && page()->relevant(ride) != true) {

        // lets find one to switch to
        Perspective *found=page();
        foreach(Perspective *p, perspectives_){
            if (p->relevant(ride)) {
                found =p;
                break;
            }
        }

        // none of them want to be selected, so we can stay on the current one
        // so long as it doesn't have an expression that failed...
        if (found == page() && page()->expression() != "")
            found = perspectives_[0];

        // if we need to switch, i.e. not already on it
        if (found != page())  {
            context->mainWindow->switchPerspective(perspectives_.indexOf(found));
        }
    }

    // tell the perspective we have selected a ride
    page()->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
}

void
AnalysisView::addIntervals()
{
    static_cast<AnalysisSidebar*>(sidebar())->addIntervals(); // save settings
}

void
AnalysisView::compareChanged(bool state)
{
    // we turned compare on / off
    context->notifyCompareIntervals(state);
}

void AnalysisView::close()
{
    static_cast<AnalysisSidebar*>(sidebar())->close(); // save settings
}

bool
AnalysisView::isBlank()
{
    if (context->athlete->rideCache->rides().count() > 0) return false;
    else return true;
}

DiaryView::DiaryView(Context *context, QStackedWidget *controls) : AbstractView(context, VIEW_DIARY)
{
    diarySidebar = new DiarySidebar(context);
    BlankStateDiaryPage *b = new BlankStateDiaryPage(context);

    setSidebar(diarySidebar);

    // each perspective has a stack of controls
    cstack = new QStackedWidget(this);
    controls->addWidget(cstack);
    controls->setCurrentIndex(0);

    pstack = new QStackedWidget(this);
    setPages(pstack);
    setBlank(b);

    setSidebarEnabled(appsettings->value(this,  GC_SETTINGS_MAIN_SIDEBAR "diary", true).toBool());
    connect(diarySidebar, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
}

DiaryView::~DiaryView()
{
    appsettings->setValue(GC_SETTINGS_MAIN_SIDEBAR "diary", _sidebar);
    delete diarySidebar;
    //delete hw; tabview deletes after save state
}

void
DiaryView::setRide(RideItem*ride)
{
    if (loaded) {
        static_cast<DiarySidebar*>(sidebar())->setRide(ride);
        page()->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
    }
}

void
DiaryView::dateRangeChanged(DateRange dr)
{
    //context->notifyDateRangeChanged(dr); // diary view deprecated and not part of navigation model
    if (loaded) page()->setProperty("dateRange", QVariant::fromValue<DateRange>(dr));
}

bool
DiaryView::isBlank()
{
    if (context->athlete->rideCache->rides().count() > 0) return false;
    else return true;
}

TrendsView::TrendsView(Context *context, QStackedWidget *controls) : AbstractView(context, VIEW_TRENDS)
{
    sidebar = new LTMSidebar(context);
    BlankStateHomePage *b = new BlankStateHomePage(context);

    setSidebar(sidebar);

    // each perspective has a stack of controls
    cstack = new QStackedWidget(this);
    controls->addWidget(cstack);
    controls->setCurrentIndex(0);

    pstack = new QStackedWidget(this);
    setPages(pstack);
    setBlank(b);
    setBottom(new ComparePane(context, this, ComparePane::season));

    setSidebarEnabled(appsettings->value(this,  GC_SETTINGS_MAIN_SIDEBAR "trend", true).toBool());
    connect(sidebar, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    connect(this, SIGNAL(onSelectionChanged()), this, SLOT(justSelected()));
    connect(bottomSplitter(), SIGNAL(compareChanged(bool)), this, SLOT(compareChanged(bool)));
    connect(bottomSplitter(), SIGNAL(compareClear()), bottom(), SLOT(clear()));
}

TrendsView::~TrendsView()
{
    appsettings->setValue(GC_SETTINGS_MAIN_SIDEBAR "trend", _sidebar);
    delete sidebar;
    //delete hw; tabview deletes after save state
}

void
TrendsView::compareChanged(bool state)
{
    // we turned compare on / off
    context->notifyCompareDateRanges(state);
}

int
TrendsView::countActivities(Perspective *perspective, DateRange dr)
{
    // get the filterset for the current daterange
    // using the data filter expression
    int returning=0;
    bool filtered= perspective->df != NULL;

    Specification spec;
    FilterSet fs;
    fs.addFilter(context->isfiltered, context->filters);
    fs.addFilter(context->ishomefiltered, context->homeFilters);
    spec.setDateRange(dr);
    spec.setFilterSet(fs);

    foreach(RideItem *item, context->athlete->rideCache->rides()) {
        if (!spec.pass(item)) continue;

        // if no filter, or the filter passes add to count
        if (!filtered || perspective->df->evaluate(item, NULL).number() != 0)
            returning++;
    }
    return returning;

}

void
TrendsView::dateRangeChanged(DateRange dr)
{
#if 0 // commented out auto switching perspectives on trends because it was annoying...
    // if there are no activities for the current perspective
    // lets switch to one that has the most
    if (countActivities(page(), dr) == 0) {

        int max=0;
        int index=perspectives_.indexOf(page());
        int switchto=index;
        for (int i=0; i<perspectives_.count(); i++) {
            int count = countActivities(perspectives_[i], dr);
            if (count > max) { max=count; switchto=i; }
        }

        // if we found a better one
        if (index != switchto)  context->mainWindow->switchPerspective(switchto);
    }
#endif

    // Once the right perspective is set, we can go ahead and update everyone
    emit dateChanged(dr);
    context->notifyDateRangeChanged(dr);
    if (loaded) page()->setProperty("dateRange", QVariant::fromValue<DateRange>(dr));
}
bool
TrendsView::isBlank()
{
    if (context->athlete->rideCache->rides().count() > 0) return false;
    else return true;
}

void
TrendsView::justSelected()
{
    if (isSelected()) {
        // force date range refresh
        static_cast<LTMSidebar*>(sidebar)->dateRangeTreeWidgetSelectionChanged();
    }
}

TrainView::TrainView(Context *context, QStackedWidget *controls) : AbstractView(context, VIEW_TRAIN)
{
    trainTool = new TrainSidebar(context);
    trainTool->setTrainView(this);
    trainTool->hide();
    BlankStateTrainPage *b = new BlankStateTrainPage(context);

    setSidebar(trainTool->controls());

    // each perspective has a stack of controls
    cstack = new QStackedWidget(this);
    controls->addWidget(cstack);
    controls->setCurrentIndex(0);

    pstack = new QStackedWidget(this);
    setPages(pstack);
    setBlank(b);

    trainBottom = new TrainBottom(trainTool, this);
    setBottom(trainBottom);
    setHideBottomOnIdle(false);

    setSidebarEnabled(appsettings->value(NULL,  GC_SETTINGS_MAIN_SIDEBAR "train").toBool());
    connect(this, SIGNAL(onSelectionChanged()), this, SLOT(onSelectionChanged()));
    connect(trainBottom, SIGNAL(autoHideChanged(bool)), this, SLOT(onAutoHideChanged(bool)));
}

void TrainView::onAutoHideChanged(bool enabled)
{
    setHideBottomOnIdle(enabled);
}

TrainView::~TrainView()
{
    appsettings->setValue(GC_SETTINGS_MAIN_SIDEBAR "train", _sidebar);
    delete trainTool;
    //delete hw; tabview deletes after save state
}

void
TrainView::close()
{
    trainTool->Stop();
}

bool
TrainView::isBlank()
{
    if (appsettings->value(this, GC_DEV_COUNT).toInt() > 0 && trainDB->getCount() > 2) return false;
    else return true;
}

void
TrainView::onSelectionChanged()
{
    if (isSelected()) {
        setBottomRequested(true);
    }
}
