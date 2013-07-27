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
#include "Context.h"
#include "Athlete.h"
#include "BlankState.h"
#include "HomeWindow.h"
#include "GcWindowRegistry.h"

#include "Settings.h"
#include "MainWindow.h" // temp - will become Tab when its ready

TabView::TabView(Context *context, int type) : 
    QWidget(context->mainWindow), context(context), type(type),
    _sidebar(true), _tiled(false), _selected(false), 
    stack(NULL), splitter(NULL), sidebar_(NULL), page_(NULL), blank_(NULL)
{
    // setup the basic widget
    QVBoxLayout *layout = new QVBoxLayout(this);
    setContentsMargins(0,0,0,0);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    stack = new QStackedWidget(this);
    stack->setContentsMargins(0,0,0,0);
    layout->addWidget(stack);

    // the splitter
    splitter = new QSplitter(this);
    splitter->setStretchFactor(0,0);
    splitter->setStretchFactor(1,1);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(" QSplitter::handle { background-color: rgb(120,120,120); color: darkGray; }");
    splitter->setFrameStyle(QFrame::NoFrame);
    splitter->setContentsMargins(0, 0, 0, 0); // attempting to follow some UI guides
    splitter->setOpaqueResize(true); // redraw when released, snappier UI
    stack->insertWidget(0, splitter); // splitter always at index 0

    connect(splitter,SIGNAL(splitterMoved(int,int)), this, SLOT(splitterMoved(int,int)));
}

TabView::~TabView()
{
    if (page_) page_->saveState();
}

void
TabView::setRide(RideItem*ride)
{
    page()->setProperty("ride", QVariant::fromValue<RideItem*>(dynamic_cast<RideItem*>(ride)));
}

void
TabView::splitterMoved(int pos,int)
{
    // show / hide sidebar as dragged..
    if ((pos == 0  && sidebarEnabled())) setSidebarEnabled(false);

    //XXX ? analysisSidebar should handle resizeEvents better, we shouldn't have
    //      to babysit it when the sidebar sizes change
    //analysisSidebar->setWidth(pos);

    // we now have splitter settings for each view
    QString setting = QString("%1/%2").arg(GC_SETTINGS_SPLITTER_SIZES).arg(type);
    appsettings->setCValue(context->athlete->cyclist, setting, splitter->saveState());
}

void
TabView::setSidebar(QWidget *sidebar)
{
    sidebar_ = sidebar;
    splitter->insertWidget(0, sidebar);
}

void
TabView::setPage(HomeWindow *page)
{
    page_ = page;

    // now reset the splitter
    splitter->insertWidget(-1, page);
    splitter->setCollapsible(0, true);
    splitter->setCollapsible(1, false);
    QString setting = QString("%1/%2").arg(GC_SETTINGS_SPLITTER_SIZES).arg(type);
    QVariant splitterSizes = appsettings->cvalue(context->athlete->cyclist, setting); 
    if (splitterSizes.toByteArray().size() > 1 ) {
        splitter->restoreState(splitterSizes.toByteArray());
    }

}

void
TabView::setBlank(BlankStatePage *blank)
{
    blank_ = blank;
    stack->insertWidget(1, blank); // blank state always at index 1
}


void
TabView::sidebarChanged()
{
    if (sidebarEnabled()) {

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
            int size = width() - 200;
            if (size>200) size = 200;

            QList<int> sizes;
            sizes.append(size);
            sizes.append(width()-size);
            splitter->setSizes(sizes);
        }

    } else sidebar_->hide();
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

        emit onSelected(); // give view a change to prepare
        page()->selected(); // select the view

        // or do we need to show blankness?
        if (isBlank() && blank_ && page_) stack->setCurrentIndex(1);
        if (!isBlank() && blank_ && page_) stack->setCurrentIndex(0);
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
