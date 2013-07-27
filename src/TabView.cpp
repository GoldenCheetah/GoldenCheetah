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
    QWidget(context->mainWindow), type(type),
    _sidebar(true), _tiled(false), _selected(false), 
    stack(NULL), splitter(NULL), sidebar(NULL), page(NULL), blank(NULL)
{
    // setup the basic widget
    QVBoxLayout *layout = new QVBoxLayout(this);
    setContentsMargins(0,0,0,0);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    stack = new QStackedWidget(this);
    layout->addWidget(stack);

    // the splitter
    splitter = new QSplitter(this);
    splitter->setStretchFactor(0,0);
    splitter->setStretchFactor(1,1);
    splitter->setCollapsible(0, true);
    splitter->setCollapsible(1, false);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet(" QSplitter::handle { background-color: rgb(120,120,120); color: darkGray; }");
    splitter->setFrameStyle(QFrame::NoFrame);
    splitter->setContentsMargins(0, 0, 0, 0); // attempting to follow some UI guides
    splitter->setOpaqueResize(true); // redraw when released, snappier UI
    stack->insertWidget(0, splitter); // splitter always at index 0

    QString setting = QString("%1/%2").arg(GC_SETTINGS_SPLITTER_SIZES).arg(type);
    QVariant splitterSizes = appsettings->cvalue(context->athlete->cyclist, setting); 
    if (splitterSizes.toByteArray().size() > 1 ) {
        splitter->restoreState(splitterSizes.toByteArray());
    }
}

TabView::~TabView()
{
    if (page) page->saveState();
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
    this->sidebar = sidebar;
    splitter->insertWidget(0, sidebar);
}

void
TabView::setPage(HomeWindow *page)
{
    this->page = page;
    splitter->insertWidget(-1, page);
}

void
TabView::setBlank(BlankStatePage *blank)
{
    this->blank = blank;
    stack->insertWidget(1, blank); // blank state always at index 1
}


void
TabView::sidebarChanged()
{
    if (sidebarEnabled()) sidebar->show();
    else sidebar->hide();
}

void
TabView::tileModeChanged()
{
    if (page) page->setStyle(isTiled() ? 0 : 2);
}

void
TabView::selectionChanged()
{
    // we got selected..
    if (isSelected()) {
        if (isBlank() && blank && page) stack->setCurrentIndex(1);
        if (!isBlank() && blank && page) stack->setCurrentIndex(0);
    }
}

void
TabView::resetLayout()
{
    if (page) page->resetLayout();
}

void
TabView::addChart(GcWinID id)
{
    if (page) page->appendChart(id);
}
