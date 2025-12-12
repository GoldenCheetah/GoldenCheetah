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

#include "Views.h"
#include "RideCache.h"
#include "AnalysisSidebar.h"
#include "MiniCalendar.h"
#include "TrainSidebar.h"
#include "LTMSidebar.h"
#include "BlankState.h"
#include "TrainDB.h"
#include "ComparePane.h"
#include "TrainBottom.h"
#include "Specification.h"

QMap<Context*, LTMSidebar*> LTMSidebarView::LTMSidebars_;

LTMSidebarView::LTMSidebarView(Context *context, int type, const QString& view, const QString& heading) :
    AbstractView(context, type, view, heading)
{
    // get or create the LTMSidebar shared between the views
    getLTMSidebar(context);

    // each view's constructor needs to register these signals.
    connect(LTMSidebars_[context], SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
    connect(this, SIGNAL(onSelectionChanged()), this, SLOT(justSelected()));
}

LTMSidebarView::~LTMSidebarView()
{
    // each destructor removes its own context related sidebar
    removeLTMSidebar(context);
}

LTMSidebar*
LTMSidebarView::getLTMSidebar(Context *sbContext)
{
    QMap<Context*, LTMSidebar*>::const_iterator itr = LTMSidebars_.find(sbContext);
    if (itr == LTMSidebars_.end()) {

        // need to create a sidebar for this context
        LTMSidebars_[sbContext] = new LTMSidebar(sbContext);
    }
    return LTMSidebars_[sbContext];
}

void LTMSidebarView::showEvent(QShowEvent*)
{
    // the show event always follows the hide event, so set the sidebar
    setSidebar(LTMSidebars_[context]);

    // update the sidebar's preset chart visibility
    LTMSidebars_[context]->updatePresetChartsOnShow(type);

    // update sidebar for the new view
    sidebarChanged();
}

void LTMSidebarView::hideEvent(QHideEvent*)
{
    // the hide event always precedes the show event, so release the sidebar
    setSidebar(nullptr);
}

void
LTMSidebarView::removeLTMSidebar(Context *sbContext)
{
    QMap<Context*, LTMSidebar*>::const_iterator itr = LTMSidebars_.find(sbContext);
    if (itr != LTMSidebars_.end()) {
        delete LTMSidebars_[sbContext];
        LTMSidebars_.erase(itr);
    }
}

void
LTMSidebarView::selectDateRange(Context *sbContext, DateRange dr)
{
    LTMSidebars_[sbContext]->selectDateRange(dr);
}

void
LTMSidebarView::justSelected()
{
    if (isSelected()) {
        // force date range refresh
        LTMSidebars_[context]->dateRangeTreeWidgetSelectionChanged();
    }
}

void
LTMSidebarView::dateRangeChanged(DateRange dr)
{
    emit dateChanged(dr);
    context->notifyDateRangeChanged(dr);
    if (loaded) page()->setProperty("dateRange", QVariant::fromValue<DateRange>(dr));
}

AnalysisView::AnalysisView(Context *context, QStackedWidget *controls) :
        AbstractView(context, VIEW_ANALYSIS, "analysis", tr("Compare Activities and Intervals"))
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

    setSidebarEnabled(appsettings->value(this, GC_SETTINGS_MAIN_SIDEBAR "analysis", defaultAppearance.sideanalysis).toBool());

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
}

void
AnalysisView::setRide(RideItem *ride)
{
    // when ride selected, but not from the sidebar.
    static_cast<AnalysisSidebar*>(sidebar())->setRide(ride); // save settings

    if (!loaded) return; // not loaded yet, all bets are off until the perspectives are loaded.

    // if we are the current view and the current perspective is no longer relevant
    // then lets go find one to switch to..
    if (context->mainWindow->athleteTab()->currentView() == 1 && page()->relevant(ride) != true) {

        // lets find a perspective to switch to
        int ridePerspectiveIdx = findRidesPerspective(ride);

        // if we need to switch, i.e. not already on it
        if (ridePerspectiveIdx != perspectives_.indexOf(page()))  {
            context->mainWindow->switchPerspective(ridePerspectiveIdx);
        }
    }

    // tell the perspective we have selected a ride
    page()->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
}

int
AnalysisView::findRidesPerspective(RideItem* ride)
{
    // lets find one to switch to
    foreach(Perspective *p, perspectives_) {
        if (p->relevant(ride)) {
            return perspectives_.indexOf(p);
        }
    }

    // none of them want to be selected, so we can stay on the current one
    // so long as it doesn't have an expression that failed...
    if (page()->expression() != "") return 0;

    // we can just return the current one
    return perspectives_.indexOf(page());
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

void
AnalysisView::notifyViewSidebarChanged() {

    // if user moved us then tell ride navigator
    // all a bit of a hack to stop the column widths from
    // being adjusted as the splitter gets resized and reset
    if (context->mainWindow->init && context->tab->init && active == false && context->rideNavigator->geometry().width() != 100) {
        context->rideNavigator->setWidth(context->rideNavigator->geometry().width());
    }
}

int
AnalysisView::getViewSpecificPerspective() {

    // Setting the ride for analysis view also sets the perspective.
    return findRidesPerspective(const_cast<RideItem*>(context->currentRideItem()));
}

void
AnalysisView::notifyViewSplitterMoved() {

    // if user moved us then tell ride navigator if
    // all a bit of a hack to stop the column widths from
    // being adjusted as the splitter gets resized and reset

    if (active == false && context->rideNavigator->geometry().width() != 100) {
        context->rideNavigator->setWidth(context->rideNavigator->geometry().width());
    }
}

PlanView::PlanView(Context *context, QStackedWidget *controls) :
        LTMSidebarView(context, VIEW_PLAN, "plan", tr("Plan future activities"))
{
    BlankStatePlanPage *b = new BlankStatePlanPage(context);

    // each perspective has a stack of controls
    cstack = new QStackedWidget(this);
    controls->addWidget(cstack);
    controls->setCurrentIndex(0);

    setSidebarEnabled(appsettings->value(this,  GC_SETTINGS_MAIN_SIDEBAR "plan", defaultAppearance.sideplan).toBool());

    pstack = new QStackedWidget(this);
    setPages(pstack);
    setBlank(b);
}

PlanView::~PlanView()
{
    appsettings->setValue(GC_SETTINGS_MAIN_SIDEBAR "plan", _sidebar);
}

bool
PlanView::isBlank()
{
    if (context->athlete->rideCache->rides().count() > 0) return false;
    else return true;
}

TrendsView::TrendsView(Context *context, QStackedWidget *controls) :
        LTMSidebarView(context, VIEW_TRENDS, "home", tr("Compare Date Ranges"))
{
    BlankStateHomePage *b = new BlankStateHomePage(context);

    // each perspective has a stack of controls
    cstack = new QStackedWidget(this);
    controls->addWidget(cstack);
    controls->setCurrentIndex(0);

    setSidebarEnabled(appsettings->value(this,  GC_SETTINGS_MAIN_SIDEBAR "trend", defaultAppearance.sidetrend).toBool());

    pstack = new QStackedWidget(this);
    setPages(pstack);
    setBlank(b);
    setBottom(new ComparePane(context, this, ComparePane::season));

    connect(bottomSplitter(), SIGNAL(compareChanged(bool)), this, SLOT(compareChanged(bool)));
    connect(bottomSplitter(), SIGNAL(compareClear()), bottom(), SLOT(clear()));
}

TrendsView::~TrendsView()
{
    appsettings->setValue(GC_SETTINGS_MAIN_SIDEBAR "trend", _sidebar);
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

bool
TrendsView::isBlank()
{
    if (context->athlete->rideCache->rides().count() > 0) return false;
    else return true;
}

TrainView::TrainView(Context *context, QStackedWidget *controls) :
        AbstractView(context, VIEW_TRAIN, "train", tr("Intensity Adjustments and Workout Control"))
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

    setSidebarEnabled(appsettings->value(NULL,  GC_SETTINGS_MAIN_SIDEBAR "train", defaultAppearance.sidetrain).toBool());
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

void
TrainView::notifyViewPerspectiveAdded(Perspective* page) {
    page->styleChanged(2);
}

