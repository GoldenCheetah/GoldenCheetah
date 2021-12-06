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


#include "AthleteTab.h"
#include "Views.h"
#include "Athlete.h"
#include "RideCache.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "MainWindow.h"
#include "Colors.h"
#include "NewSideBar.h"
#include "NavigationModel.h"

#include <QPaintEvent>

AthleteTab::AthleteTab(Context *context) : QWidget(context->mainWindow), context(context), noswitch(true)
{
    context->tab = this;
    init = false;

    setContentsMargins(0,0,0,0);
    QVBoxLayout *main = new QVBoxLayout(this);
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);

    views = new QStackedWidget(this);
    views->setContentsMargins(0,0,0,0);
    main->addWidget(views);

    // all the stack views for the controls
    masterControls = new QStackedWidget(this);
    masterControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    masterControls->setCurrentIndex(0);
    masterControls->setContentsMargins(0,0,0,0);

    // Analysis - created first as sidebar is used elsewhere (yuk)
    analysisControls = new QStackedWidget(this);
    analysisControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    analysisControls->setCurrentIndex(0);
    analysisControls->setContentsMargins(0,0,0,0);
    analysisView = new AnalysisView(context, analysisControls);

    // Home
    homeControls = new QStackedWidget(this);
    homeControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    homeControls->setContentsMargins(0,0,0,0);
    homeView = new TrendsView(context, homeControls);

    // Diary
    diaryControls = new QStackedWidget(this);
    diaryControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    diaryControls->setCurrentIndex(0);
    diaryControls->setContentsMargins(0,0,0,0);
    diaryView = new DiaryView(context, diaryControls);

    // Train
    trainControls = new QStackedWidget(this);
    trainControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    trainControls->setCurrentIndex(0);
    trainControls->setContentsMargins(0,0,0,0);
    trainView = new TrainView(context, trainControls);

    // although the views are created with analysis created first
    // we add them to the views and master controls in the old
    // order to make sure we select the right stack index
    // when switching views
    views->addWidget(homeView);
    views->addWidget(analysisView);
    views->addWidget(diaryView);
    views->addWidget(trainView);

    masterControls->addWidget(homeControls);
    masterControls->addWidget(analysisControls);
    masterControls->addWidget(diaryControls);
    masterControls->addWidget(trainControls);

    // the dialog box for the chart settings
    chartSettings = new ChartSettings(this, masterControls);
    chartSettings->setMaximumWidth(650);
    chartSettings->setMaximumHeight(600);
    chartSettings->hide();

    // navigation model after main items as it uses the observer
    // pattern on views etc, so they need to be created first
    // but we need to get setup before ride selection happens
    // below, so we can observe the iniital ride select
    nav = new NavigationModel(this);

    // cpx aggregate cache check
    connect(context,SIGNAL(rideSelected(RideItem*)), this, SLOT(rideSelected(RideItem*)));

    noswitch = false; // we only let it happen when we're initialised
    init = true;
}

AthleteTab::~AthleteTab()
{
    delete analysisView;
    delete homeView;
    delete trainView;
    delete diaryView;
    delete views;
    delete nav;
}

void
AthleteTab::close()
{
    analysisView->saveState();
    homeView->saveState();
    trainView->saveState();
    diaryView->saveState();

    analysisView->close();
    homeView->close();
    trainView->close();
    diaryView->close();
}

/******************************************************************************
 * MainWindow integration with Tab / TabView (mostly pass through)
 *****************************************************************************/

bool AthleteTab::hasBottom() { return view(currentView())->hasBottom(); }
bool AthleteTab::isBottomRequested() { return view(currentView())->isBottomRequested(); }
void AthleteTab::setBottomRequested(bool x) { view(currentView())->setBottomRequested(x); }
void AthleteTab::setSidebarEnabled(bool x) { view(currentView())->setSidebarEnabled(x); }
bool AthleteTab::isSidebarEnabled() { return view(currentView())->sidebarEnabled(); }
void AthleteTab::toggleSidebar() { view(currentView())->setSidebarEnabled(!view(currentView())->sidebarEnabled()); }
void AthleteTab::setTiled(bool x) { view(currentView())->setTiled(x); }
bool AthleteTab::isTiled() { return view(currentView())->isTiled(); }
void AthleteTab::toggleTile() { view(currentView())->setTiled(!view(currentView())->isTiled()); }
void AthleteTab::resetLayout(QComboBox *perspectiveSelector) { view(currentView())->resetLayout(perspectiveSelector); }
void AthleteTab::addChart(GcWinID i) { view(currentView())->addChart(i); }
void AthleteTab::addIntervals() { analysisView->addIntervals(); }

void AthleteTab::setRide(RideItem*ride)
{
    analysisView->setRide(ride);
    homeView->setRide(ride);
    trainView->setRide(ride);
    diaryView->setRide(ride);
}

AbstractView *
AthleteTab::view(int index)
{
    switch(index) {
        case 0 : return homeView;
        default:
        case 1 : return analysisView;
        case 2 : return diaryView;
        case 3 : return trainView;
    }
}

void
AthleteTab::selectView(int index)
{
    if (views->currentIndex() == index) return; // not changing

    emit viewChanged(index);

    // first we deselect the current
    view(views->currentIndex())->setSelected(false);

    // now select the real one
    views->setCurrentIndex(index);
    view(index)->setSelected(true);
    masterControls->setCurrentIndex(index);
    context->setIndex(index);
    context->mainWindow->resetPerspective(index); // set perspective for this view
}

void
AthleteTab::rideSelected(RideItem*)
{
    emit rideItemSelected(context->ride);

    // update the ride property on all widgets
    // to let them know they need to replot new
    // selected ride (now the tree is up to date)
    setRide(context->ride);

    // notify that the intervals have been cleared too
    context->notifyIntervalsChanged();

    // if we selected a ride we should be on the analysis
    // view-- this is new with the overview and click thru
    // coming in other charts but when navigation model is
    // going back we need to stay on the target view
    // so noswitch is set by the nav model whilst it is working
    if (!noswitch && views->currentIndex() != 1) {

        // but we should not switch if the view has not been
        // loaded yet, since a ride may have been imported
        // ahead of selecting a view. We could incorporate this
        // check in the clause above, but want to call it out
        // as important to be aware that perspectives are loaded
        // when the view is selected by the user and no earlier
        if (analysisView->page() != NULL) {
            context->mainWindow->newSidebar()->setItemSelected(3, true);
            selectView(1);
        }
    }
}

ProgressLine::ProgressLine(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    setFixedHeight(6 *dpiYFactor);
    hide();

    connect(context, SIGNAL(refreshStart()), this, SLOT(show()));
    connect(context, SIGNAL(refreshEnd()), this, SLOT(hide()));
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(show())); // we might miss 1st one
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(repaint()));
}

void
ProgressLine::paintEvent(QPaintEvent *)
{

    // nothing for test...
    QColor translucentGray = GColor(CPLOTMARKER);
    translucentGray.setAlpha(240);
    QColor translucentWhite = GColor(CPLOTBACKGROUND);

    // setup a painter and the area to paint
    QPainter painter(this);

    painter.save();
    QRect all(0,0,width(),height());

    // fill
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, translucentWhite);

    // progressbar
    QRectF progress(0, 0, (double(context->athlete->rideCache->progress()) / 100.0f) * double(width()), height());
    painter.fillRect(progress, translucentGray);
    painter.restore();
}
