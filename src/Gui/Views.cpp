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
#include "EquipmentSidebar.h"
#include "DiarySidebar.h"
#include "TrainSidebar.h"
#include "LTMSidebar.h"
#include "BlankState.h"
#include "TrainDB.h"
#include "ComparePane.h"
#include "TrainBottom.h"
#include "Specification.h"

AnalysisView::AnalysisView(Context *context, QStackedWidget *controls) :
        AbstractView(context, VIEW_ANALYSIS, "analysis", "Compare Activities and Intervals")
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
    //delete hw; tabview deletes after save state

    saveState(); // writes analysis-perspectives.xml
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
AnalysisView::restoreState(bool useDefault)
{
    AbstractView::restoreState(useDefault);

    // analysis view then lets select the first ride now
    // used to happen in Tab.cpp on create, but we do it later now
    QDateTime now = QDateTime::currentDateTime();
    for (int i = context->athlete->rideCache->rides().count(); i > 0; --i) {
        if (context->athlete->rideCache->rides()[i - 1]->dateTime <= now) {
            context->athlete->selectRideFile(context->athlete->rideCache->rides()[i - 1]->fileName);
            break;
        }
    }

    // otherwise just the latest
    if (context->currentRideItem() == NULL && context->athlete->rideCache->rides().count() != 0) {
        context->athlete->selectRideFile(context->athlete->rideCache->rides().last()->fileName);
    }
}

void
AnalysisView::splitterMoved(int pos, int)
{
    AbstractView::splitterMoved(pos, 0);

    // if user moved us then tell ride navigator if
    // we are the analysis view
    // all a bit of a hack to stop the column widths from
    // being adjusted as the splitter gets resized and reset
    if (active == false && context->rideNavigator->geometry().width() != 100)
        context->rideNavigator->setWidth(context->rideNavigator->geometry().width());
}

void
AnalysisView::sidebarChanged()
{
    // wait for main window to catch up
    if (sidebar_ == NULL) return;

    AbstractView::sidebarChanged();

    if (sidebarEnabled()) {

        // if user moved us then tell ride navigator if
        // we are the analysis view
        // all a bit of a hack to stop the column widths from
        // being adjusted as the splitter gets resized and reset
        if (context->mainWindow->init && context->tab->init && active == false && context->rideNavigator->geometry().width() != 100)
            context->rideNavigator->setWidth(context->rideNavigator->geometry().width());
        setUpdatesEnabled(true);

    }
}

void
AnalysisView::setPerspectives(QComboBox* perspectiveSelector, bool selectChart = false)
{
    AbstractView::setPerspectives(perspectiveSelector, selectChart);

    if (loaded) {
        // On analysis view the perspective is selected on the basis
        // of the currently selected ride
        RideItem* notconst = (RideItem*)context->currentRideItem();
        if (notconst != NULL) setRide(notconst);

        // due to visibility optimisation we need to force the first tab to be selected in tab mode
        if (perspective_->currentStyle == 0 && perspective_->charts.count()) perspective_->tabSelected(0);
    }
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

DiaryView::DiaryView(Context *context, QStackedWidget *controls) :
        AbstractView(context, VIEW_DIARY, "diary", "Compare Activities and Intervals")
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

    setSidebarEnabled(appsettings->value(this,  GC_SETTINGS_MAIN_SIDEBAR "diary", false).toBool());
    connect(diarySidebar, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
}

DiaryView::~DiaryView()
{
    appsettings->setValue(GC_SETTINGS_MAIN_SIDEBAR "diary", _sidebar);
    delete diarySidebar;
    //delete hw; tabview deletes after save state

    saveState(); // writes diary-perspectives.xml
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

TrendsView::TrendsView(Context *context, QStackedWidget *controls) :
        AbstractView(context, VIEW_TRENDS, "home", "Compare Date Ranges")
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

    setSidebarEnabled(appsettings->value(this,  GC_SETTINGS_MAIN_SIDEBAR "trend", defaultAppearance.sidetrend).toBool());
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

    saveState(); // writes home-perspectives.xml
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

TrainView::TrainView(Context *context, QStackedWidget *controls) :
        AbstractView(context, VIEW_TRAIN, "train", "Intensity Adjustments and Workout Control")
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
    //delete hw; tabview deletes after save state

    saveState(); // writes train-perspectives.xml
}

void
TrainView::close()
{
    trainTool->Stop();
}

Perspective*
TrainView::addPerspective(QString name)
{
    Perspective* page = new Perspective(context, name, type);

    // no tabs on train view
    page->styleChanged(2);

    // append
    appendPerspective(page);
    return page;
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

EquipView::EquipView(Context* context, QStackedWidget* controls) :
        AbstractView(context, VIEW_EQUIPMENT, "equipment", "Equipment Management")
{
    equipmentSidebar_ = new EquipmentSidebar(context);

    setSidebar(equipmentSidebar_);

    // collapsing the equipment navigators doesn't make sense in the
    // equipment view, so disable the QSplitters collaspsing capability.
    splitter->setCollapsible(0, false);

    // perspectives are stacked
    pstack = new QStackedWidget(this);
    setPages(pstack);

    // each perspective has a stack of controls
    cstack = new QStackedWidget(this);
    controls->addWidget(cstack);
    controls->setCurrentIndex(0);

    setSidebarEnabled(true);
}

EquipView::~EquipView()
{
    delete equipmentSidebar_;

    // equipment view has a single persistent perspective and is always
    // loaded from the "baked" in configuration, so is never saved.
}

void
EquipView::restoreState(bool useDefault)
{
    // always load the "baked" in qt resources configuration :xml/equipment-perspectives.xml file.
    AbstractView::restoreState(false);
}

bool
EquipView::isBlank()
{
    return true;
}
