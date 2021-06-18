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

#include "TabView.h"
#include "Tab.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "BlankState.h"
#include "HomeWindow.h"
#include "GcWindowRegistry.h"
#include "TrainDB.h"
#include "RideNavigator.h"
#include "MainWindow.h"
#include "IdleTimer.h"

#include "Settings.h"

TabView::TabView(Context *context, int type) : 
    QWidget(context->tab), context(context), type(type),
    _sidebar(true), _tiled(false), _selected(false), lastHeight(130*dpiYFactor), sidewidth(0),
    active(false), bottomRequested(false), bottomHideOnIdle(false), perspectiveactive(false),
    stack(NULL), splitter(NULL), mainSplitter(NULL), 
    sidebar_(NULL), bottom_(NULL), page_(NULL), blank_(NULL)
{
    // setup the basic widget
    QVBoxLayout *layout = new QVBoxLayout(this);
    setContentsMargins(0,0,0,0);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    stack = new QStackedWidget(this);
    stack->setContentsMargins(0,0,0,0);
    stack->setFrameStyle(QFrame::Plain | QFrame::NoFrame);
    stack->setMinimumWidth(500*dpiXFactor);
    stack->setMinimumHeight(500*dpiYFactor);

    layout->addWidget(stack);

    // the splitter
    splitter = new QSplitter(this);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(" QSplitter::handle { background-color: rgb(120,120,120); color: darkGray; }");
    splitter->setFrameStyle(QFrame::NoFrame);
    splitter->setContentsMargins(0, 0, 0, 0); // attempting to follow some UI guides
    splitter->setOpaqueResize(true); // redraw when released, snappier UI
    stack->insertWidget(0, splitter); // splitter always at index 0

    QString heading = tr("Compare Activities and Intervals");
    if (type == VIEW_HOME) heading = tr("Compare Date Ranges");
    else if (type == VIEW_TRAIN) heading = tr("Intensity Adjustments and Workout Control");

    mainSplitter = new ViewSplitter(Qt::Vertical, heading, this);
    mainSplitter->setHandleWidth(23 *dpiXFactor);
    mainSplitter->setFrameStyle(QFrame::NoFrame);
    mainSplitter->setContentsMargins(0, 0, 0, 0); // attempting to follow some UI guides
    mainSplitter->setOpaqueResize(true); // redraw when released, snappier UI

    // the animator
    anim = new QPropertyAnimation(mainSplitter, "hpos");

    connect(splitter,SIGNAL(splitterMoved(int,int)), this, SLOT(splitterMoved(int,int)));
    connect(context,SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(&IdleTimer::getInstance(), SIGNAL(userIdle()), this, SLOT(onIdle()));
    connect(&IdleTimer::getInstance(), SIGNAL(userActive()), this, SLOT(onActive()));
}

TabView::~TabView()
{
    if (page_) {
        page_->saveState();
        delete page_;
    }
}

void
TabView::setRide(RideItem*ride)
{
    page()->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
}

void
TabView::splitterMoved(int pos,int)
{
    if (sidebar_ == NULL) return; // we haven't set sidebar yet.

    // show / hide sidebar as dragged..
    if ((pos == 0  && sidebarEnabled())) setSidebarEnabled(false);

    // remember the size the user selected
    sidewidth = splitter->sizes()[0];

    // we now have splitter settings for each view
    QString setting = QString("%1/%2").arg(GC_SETTINGS_SPLITTER_SIZES).arg(type);
    appsettings->setCValue(context->athlete->cyclist, setting, splitter->saveState());

    // if user moved us then tell ride navigator if
    // we are the analysis view
    // all a bit of a hack to stop the column widths from
    // being adjusted as the splitter gets resized and reset
    if (type == VIEW_ANALYSIS && active == false && context->tab->rideNavigator()->geometry().width() != 100)
        context->tab->rideNavigator()->setWidth(context->tab->rideNavigator()->geometry().width());
}

void
TabView::resizeEvent(QResizeEvent *)
{
    active = true; // we're mucking about, so ignore in splitterMoved ...

    if (sidewidth < 200) {
        sidewidth = splitter->sizes()[0];
    } else {

        // splitter sizes will have changed, so lets get the new
        // total width and set the handle back to where it was
        QList<int> csizes = splitter->sizes();
        if (csizes.count() == 2) {

            // total will have changed
            int tot = csizes[0] + csizes[1];
            if (tot >= sidewidth) {

                // set left back to where it was before!
                csizes[0] = sidewidth;
                csizes[1] = tot-sidewidth;
                splitter->setSizes(csizes);
                sidewidth = splitter->sizes()[0];
            }
        }
    }

    active = false;
}

void
TabView::setSidebar(QWidget *sidebar)
{
    sidebar_ = sidebar;
    splitter->insertWidget(0, sidebar);

    configChanged(CONFIG_APPEARANCE);
}

QString
TabView::ourStyleSheet()
{
    return QString::fromUtf8("QScrollBar { background-color: %1; border: 0px; }"
#ifndef Q_OS_MAC
           "QTabWidget { background: %1; }"
           "QTabWidget::pane { border: 1px solid %2; } "
           "QTextEdit { background: %1; }"
           "QTextEdit#metadata { background: %3; }"
#endif
           "QTreeView { background: %1; }"
           "QScrollBar:vertical {"
           "    border: 0px solid darkGray; "
           "    background:%1;"
           "    width: %4px;    "
           "    margin: 0px 0px 0px 0px;"
           "}"
           "QScrollBar::handle:vertical:enabled:hover {"
           "    background: lightGray; "
           "    min-height: %4px;"
           ""
           "}"
           "QScrollBar::handle:vertical:enabled {"
           "    background: darkGray; "
           "    min-height: %4px;"
           ""
           "}"
           "QScrollBar::handle:vertical {"
           "    background: %1; "
           "    min-height: %4px;"
           ""
           "}"
           "QScrollBar::sub-page:vertical { background: %1; }"
           "QScrollBar::add-page:vertical { background: %1; }"
           "QScrollBar::add-line:vertical {"
           "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
           "    stop: 0  rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130),  stop:1 rgb(32, 47, 130));"
           "    height: px;"
           "    subcontrol-position: bottom;"
           "    subcontrol-origin: margin;"
           "}"
           "QScrollBar::sub-line:vertical {"
           "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
           "    stop: 0  rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130),  stop:1 rgb(32, 47, 130));"
           "    height: 0px;"
           "    subcontrol-position: top;"
           "    subcontrol-origin: margin;"
           "}"
           "QScrollBar:horizontal {"
           "    border: 0px solid darkGray; "
           "    background-color:%1;"
           "    height: %4px;    "
           "    margin: 1px 1px 1px 1px;"
           "}"
           "QScrollBar::handle:horizontal {"
           "    background: darkGray; "
           "    min-width: 0px;"
           ""
           "}"
           "QScrollBar::sub-page:horizontal { background: %1; }"
           "QScrollBar::add-page:horizontal { background: %1; }"
           "QScrollBar::add-line:horizontal {"
           "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
           "    stop: 0  rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130),  stop:1 rgb(32, 47, 130));"
           "    width: 0px;"
           "    subcontrol-position: bottom;"
           "    subcontrol-origin: margin;"
           "}"
           "QScrollBar::sub-line:horizontal {"
           "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,"
           "    stop: 0  rgb(32, 47, 130), stop: 0.5 rgb(32, 47, 130),  stop:1 rgb(32, 47, 130));"
           "    width: 0px;"
           "    subcontrol-position: top;"
           "    subcontrol-origin: margin;"
           "}"
           "QTableWidget::item:hover { color: black; background: lightGray; }"
           "QTreeView::item:hover { color: black; background: lightGray; }"
           "").arg(GColor(CPLOTBACKGROUND).name())
            .arg(GColor(CPLOTGRID).name())
            .arg(GCColor::alternateColor(GColor(CPLOTBACKGROUND)).name())
            .arg(8 * dpiXFactor)
            ;
}

void
TabView::configChanged(qint32)
{
#if (defined Q_OS_LINUX) || (defined Q_OS_WIN)
    // style that sucker
    if (sidebar_) {
        sidebar_->setStyleSheet(ourStyleSheet());
    }
#endif
}

void
TabView::setPage(HomeWindow *page)
{
    page_ = page;

    // add to mainSplitter
    // now reset the splitter
    mainSplitter->insertWidget(-1, page);
    mainSplitter->setStretchFactor(0,0);
    mainSplitter->setCollapsible(0, false);
    splitter->insertWidget(-1, mainSplitter);

    // restore sizes
    QString setting = QString("%1/%2").arg(GC_SETTINGS_SPLITTER_SIZES).arg(type);
    QVariant splitterSizes = appsettings->cvalue(context->athlete->cyclist, setting); 

    // new (3.1) mechanism 
    if (splitterSizes.toByteArray().size() > 1 ) {
        splitter->restoreState(splitterSizes.toByteArray());
    } else {

        // use old (v3 or earlier) mechanism
        QVariant splitterSizes = appsettings->cvalue(context->athlete->cyclist, GC_SETTINGS_SPLITTER_SIZES); 
        if (splitterSizes.toByteArray().size() > 1 ) {

            splitter->restoreState(splitterSizes.toByteArray());

        } else {

            // sensible default as never run before!
            QList<int> sizes;

            sizes.append(200);
            sizes.append(context->mainWindow->width()-200);
            splitter->setSizes(sizes);
            
        }
    }
}

void
TabView::setBottom(QWidget *widget)
{
    bottom_ = widget;
    bottom_->hide();
    mainSplitter->insertWidget(-1, bottom_);
    mainSplitter->setCollapsible(1, true); // XXX we need a ComparePane widget ...
    mainSplitter->setStretchFactor(1,1);
}

void 
TabView::dragEvent(bool x)
{
    setBottomRequested(x);
    context->mainWindow->setToolButtons(); // toolbuttons reflect show/hide status
}

// hide and show bottom - but with a little animation ...
void
TabView::setShowBottom(bool x) 
{
    if (x == isShowBottom())
    {
        return;
    }

    // remember last height used when hidind
    if (!x && bottom_) lastHeight = bottom_->height();

    // basic version for now .. remembers and sets horizontal position precisely
    // adding animation should be easy from here


    if (bottom_) {
        if (x) {

            // set to the last value....
            bottom_->show();

            anim->setDuration(lastHeight * 3);
            anim->setEasingCurve(QEasingCurve(QEasingCurve::Linear));
            anim->setKeyValueAt(0,mainSplitter->maxhpos()-22);
            anim->setKeyValueAt(1,mainSplitter->maxhpos()-(lastHeight+22));
            anim->start();

        } else {

            // need a hide animator to hide on timeout
            //anim->setDuration(200);
            //anim->setEasingCurve(QEasingCurve(QEasingCurve::Linear));
            //anim->setKeyValueAt(0,mainSplitter->maxhpos()-(lastHeight+22));
            //anim->setKeyValueAt(1,mainSplitter->maxhpos()-22);
            //anim->start();

            bottom_->hide();
        }
    }
}

void
TabView::setBlank(BlankStatePage *blank)
{
    blank_ = blank;
    blank->hide();
    stack->insertWidget(1, blank); // blank state always at index 1

    // and when stuff happens lets check
    connect(blank, SIGNAL(closeClicked()), this, SLOT(checkBlank()));
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(checkBlank()));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(checkBlank()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(checkBlank()));
    connect(trainDB, SIGNAL(dataChanged()), this, SLOT(checkBlank()));

}


void
TabView::sidebarChanged()
{
    // wait for main window to catch up
    if (sidebar_ == NULL) return;

    // tell main window qmenu we changed
    if (context->mainWindow->init) context->mainWindow->showhideSidebar->setChecked(_sidebar);

    if (sidebarEnabled()) {

        setUpdatesEnabled(false);

        sidebar_->show();

        // Restore sizes
        QString setting = QString("%1/%2").arg(GC_SETTINGS_SPLITTER_SIZES).arg(type);
        QVariant splitterSizes = appsettings->cvalue(context->athlete->cyclist, setting);
        if (splitterSizes.toByteArray().size() > 1 ) {
            splitter->restoreState(splitterSizes.toByteArray());
            splitter->setOpaqueResize(true); // redraw when released, snappier UI
        }

        // if it was collapsed we need set to at least 200
        // unless the mainwindow isn't big enough
        if (sidebar_->width()<10) {
            int size = width() - 200 * dpiXFactor;
            if (size>(200* dpiXFactor)) size = 200* dpiXFactor;

            QList<int> sizes;
            sizes.append(size);
            sizes.append(width()-size);
            splitter->setSizes(sizes);
        }

        // if user moved us then tell ride navigator if
        // we are the analysis view
        // all a bit of a hack to stop the column widths from
        // being adjusted as the splitter gets resized and reset
        if (context->mainWindow->init && context->tab->init && type == VIEW_ANALYSIS && active == false && context->tab->rideNavigator()->geometry().width() != 100)
            context->tab->rideNavigator()->setWidth(context->tab->rideNavigator()->geometry().width());
        setUpdatesEnabled(true);

    } else sidebar_->hide();
}

void
TabView::setPerspectives(QComboBox *perspectiveSelector)
{
    perspectiveactive=true;
    perspectiveSelector->clear();
    perspectiveSelector->addItem("General");
    perspectiveSelector->addItem("Bike");
    perspectiveSelector->addItem("Run");
    perspectiveSelector->addItem("Swim");
    perspectiveSelector->addItem("Add New Perspective...");
    perspectiveSelector->addItem("Manage Perspectives...");
    perspectiveSelector->insertSeparator(4);
    perspectiveactive=false;
}


void
TabView::perspectiveSelected(int index)
{
    if (perspectiveactive || index <0) return;
    fprintf(stderr, "Selected perspective: %d\n", index); fflush(stderr);
}


void
TabView::tileModeChanged()
{
    if (page_) page_->setStyle(isTiled() ? 2 : 0);
}

void
TabView::selectionChanged()
{
    // we got selected..
    if (isSelected()) {

        // makes sure menu now reflects our setting
        context->mainWindow->showhideSidebar->setChecked(_sidebar);

        // or do we need to show blankness?
        if (isBlank() && blank_ && page_ && blank_->canShow()) {

            splitter->hide();
            blank()->show();

            stack->setCurrentIndex(1);

        } else if (blank_ && page_) {

            blank()->hide();
            splitter->show();

            emit onSelectionChanged(); // give view a change to prepare
            page()->selected(); // select the view

            stack->setCurrentIndex(0);
        }
    } else {

        emit onSelectionChanged(); // give view a change to clear

    }
}

void
TabView::resetLayout()
{
    if (page_) page_->resetLayout();
}

void
TabView::addChart(GcWinID id)
{
    if (page_) page_->appendChart(id);
}

void
TabView::checkBlank()
{
    selectionChanged(); // run through the code again
}

void
TabView::setBottomRequested(bool x)
{
    bottomRequested = x;
    setShowBottom(x);
}

void
TabView::setHideBottomOnIdle(bool x)
{
    bottomHideOnIdle = x;
}

void
TabView::onIdle()
{
    if (bottomHideOnIdle)
    {
        setShowBottom(false);
    }
}

void
TabView::onActive()
{
    if (bottomRequested)
    {
        setShowBottom(true);
    }
}
