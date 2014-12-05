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


#include "Tab.h"
#include "Views.h"
#include "Athlete.h"
#include "RideCache.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "MainWindow.h"
#include "Colors.h"

#include <QPaintEvent>

Tab::Tab(Context *context) : QWidget(context->mainWindow), context(context)
{
    context->tab = this;

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

    // Home
    homeControls = new QStackedWidget(this);
    homeControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    homeControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(homeControls);
    homeView = new HomeView(context, homeControls);
    views->addWidget(homeView);

    // Analysis
    analysisControls = new QStackedWidget(this);
    analysisControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    analysisControls->setCurrentIndex(0);
    analysisControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(analysisControls);
    analysisView = new AnalysisView(context, analysisControls);
    views->addWidget(analysisView);

    // Diary
    diaryControls = new QStackedWidget(this);
    diaryControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    diaryControls->setCurrentIndex(0);
    diaryControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(diaryControls);
    diaryView = new DiaryView(context, diaryControls);
    views->addWidget(diaryView);

    // Train
    trainControls = new QStackedWidget(this);
    trainControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    trainControls->setCurrentIndex(0);
    trainControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(trainControls);
    trainView = new TrainView(context, trainControls);
    views->addWidget(trainView);

#ifdef GC_HAVE_INTERVALS
    // Interval
    intervalControls = new QStackedWidget(this);
    intervalControls->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    intervalControls->setCurrentIndex(0);
    intervalControls->setContentsMargins(0,0,0,0);
    masterControls->addWidget(intervalControls);
    intervalView = new IntervalView(context, intervalControls);
    views->addWidget(intervalView);
#endif

    // the dialog box for the chart settings
    chartSettings = new ChartSettings(this, masterControls);
    chartSettings->setMaximumWidth(450);
    chartSettings->setMaximumHeight(600);
    chartSettings->hide();

    // cpx aggregate cache check
    connect(context,SIGNAL(rideSelected(RideItem*)), this, SLOT(rideSelected(RideItem*)));

// for testing
context->athlete->rideCache->refresh();

    // selects the latest ride in the list:
    if (context->athlete->rideCache->rides().count() != 0) 
        context->athlete->selectRideFile(context->athlete->rideCache->rides().last()->fileName);
}

Tab::~Tab()
{
    delete analysisView;
    delete homeView;
    delete trainView;
    delete diaryView;
    delete views;
}

RideNavigator *
Tab::rideNavigator()
{
    return analysisView->rideNavigator();
}

IntervalNavigator *
Tab::routeNavigator()
{
    return intervalView->routeNavigator();
}

IntervalNavigator *
Tab::bestNavigator()
{
    return intervalView->bestNavigator();
}

void
Tab::close()
{
    analysisView->saveState();
    homeView->saveState();
    trainView->saveState();
    diaryView->saveState();
#ifdef GC_HAVE_INTERVALS
    intervalView->saveState();
#endif

    analysisView->close();
    homeView->close();
    trainView->close();
    diaryView->close();
#ifdef GC_HAVE_INTERVALS
    intervalView->close();
#endif
}

/******************************************************************************
 * MainWindow integration with Tab / TabView (mostly pass through)
 *****************************************************************************/

void Tab::setShowBottom(bool x) { view(currentView())->setShowBottom(x); }
bool Tab::isShowBottom() { return view(currentView())->isShowBottom(); }
bool Tab::hasBottom() { return view(currentView())->hasBottom(); }
void Tab::setSidebarEnabled(bool x) { view(currentView())->setSidebarEnabled(x); }
bool Tab::isSidebarEnabled() { return view(currentView())->sidebarEnabled(); }
void Tab::toggleSidebar() { view(currentView())->setSidebarEnabled(!view(currentView())->sidebarEnabled()); }
void Tab::setTiled(bool x) { view(currentView())->setTiled(x); }
bool Tab::isTiled() { return view(currentView())->isTiled(); }
void Tab::toggleTile() { view(currentView())->setTiled(!view(currentView())->isTiled()); }
void Tab::resetLayout() { view(currentView())->resetLayout(); }
void Tab::addChart(GcWinID i) { view(currentView())->addChart(i); }
void Tab::addIntervals() { analysisView->addIntervals(); }

void Tab::setRide(RideItem*ride) 
{ 
    analysisView->setRide(ride);
    homeView->setRide(ride);
    trainView->setRide(ride);
    diaryView->setRide(ride);
#ifdef GC_HAVE_INTERVALS
    intervalView->setRide(ride);
#endif
}

TabView *
Tab::view(int index)
{
    switch(index) {
        case 0 : return homeView;
        default:
        case 1 : return analysisView;
        case 2 : return diaryView;
        case 3 : return trainView;
#ifdef GC_HAVE_INTERVALS
        case 4 : return intervalView;
#endif
    }
}

void
Tab::selectView(int index)
{
    // first we deselect the current
    view(views->currentIndex())->setSelected(false);

    // now select the real one
    views->setCurrentIndex(index);
    view(index)->setSelected(true);
    masterControls->setCurrentIndex(index);
}

void
Tab::rideSelected(RideItem*)
{
    // stop SEGV in widgets watching for intervals being
    // selected whilst we are deleting them from the tree
    context->athlete->intervalWidget->blockSignals(true);

    // refresh interval list for bottom left
    // first lets wipe away the existing intervals
    QList<QTreeWidgetItem *> intervals = context->athlete->allIntervals->takeChildren();
    for (int i=0; i<intervals.count(); i++) delete intervals.at(i);

    // now add the intervals for the current ride
    if (context->ride) { // only if we have a ride pointer
        RideFile *selected = context->ride->ride();
        if (selected) {
            // get all the intervals in the currently selected RideFile
            QList<RideFileInterval> intervals = selected->intervals();
            for (int i=0; i < intervals.count(); i++) {
                // add as a child to context->athlete->allIntervals
                IntervalItem *add = new IntervalItem(selected,
                                                        intervals.at(i).name,
                                                        intervals.at(i).start,
                                                        intervals.at(i).stop,
                                                        selected->timeToDistance(intervals.at(i).start),
                                                        selected->timeToDistance(intervals.at(i).stop),
                                                        context->athlete->allIntervals->childCount()+1);
                add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
                context->athlete->allIntervals->addChild(add);
            }
        }
    }
    // all done, so connected widgets can receive signals now
    context->athlete->intervalWidget->blockSignals(false);

    // update the ride property on all widgets
    // to let them know they need to replot new
    // selected ride (now the tree is up to date)
    setRide(context->ride);

    // notify that the intervals have been cleared too
    context->notifyIntervalsChanged();
}

ProgressLine::ProgressLine(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    setFixedHeight(1);
    //hide();

    connect(context, SIGNAL(refreshStart()), this, SLOT(show()));
    connect(context, SIGNAL(refreshEnd()), this, SLOT(hide()));
    connect(context, SIGNAL(refreshUpdate()), this, SLOT(repaint()));
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
    QRectF progress(0, 0, (double(context->athlete->rideCache->progress()) / 100.0f) * double(width()), 1);
    painter.fillRect(progress, translucentGray);
    painter.restore();
}
