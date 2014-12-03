/*
 * Copyright (c) 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#include "IntervalSidebar.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "Settings.h"
#include "Units.h"
#include <QtGui>

// metadata support
#include "RideMetadata.h"
#include "SpecialFields.h"

// working with intervals
#include "IntervalItem.h"
#include "AddIntervalDialog.h"
#include "BestIntervalDialog.h"

// working with routes
#include "Route.h"

IntervalSidebar::IntervalSidebar(Context *context) : QWidget(context->mainWindow), context(context)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);
    
    splitter = new GcSplitter(Qt::Vertical);
    mainLayout->addWidget(splitter);

    // Route
    routeNavigator = new IntervalNavigator(context, "Route", true);
    routeNavigator->setProperty("nomenu", true);
    groupByMapper = NULL;

    // Bests
    bestNavigator = new IntervalNavigator(context, "Best", true);
    bestNavigator->setProperty("nomenu", true);


    // retrieve settings (properties are saved when we close the window)
    if (appsettings->cvalue(context->athlete->cyclist, GC_ROUTEHEADINGS, "").toString() != "") {
        routeNavigator->setSortByIndex(appsettings->cvalue(context->athlete->cyclist, GC_ROUTESORTBY).toInt());
        routeNavigator->setSortByOrder(appsettings->cvalue(context->athlete->cyclist, GC_ROUTESORTBYORDER).toInt());
        //routeNavigator->setGroupBy(appsettings->cvalue(context->athlete->cyclist, GC_ROUTEGROUPBY).toInt());
        routeNavigator->setColumns(appsettings->cvalue(context->athlete->cyclist, GC_ROUTEHEADINGS).toString());
        routeNavigator->setWidths(appsettings->cvalue(context->athlete->cyclist, GC_ROUTEHEADINGWIDTHS).toString());
    }
    if (appsettings->cvalue(context->athlete->cyclist, GC_BESTHEADINGS, "").toString() != "") {
        bestNavigator->setSortByIndex(appsettings->cvalue(context->athlete->cyclist, GC_BESTSORTBY).toInt());
        bestNavigator->setSortByOrder(appsettings->cvalue(context->athlete->cyclist, GC_BESTSORTBYORDER).toInt());
        bestNavigator->setColumns(appsettings->cvalue(context->athlete->cyclist, GC_BESTHEADINGS).toString());
        bestNavigator->setWidths(appsettings->cvalue(context->athlete->cyclist, GC_BESTHEADINGWIDTHS).toString());
    }

    QWidget *routeWidget = new QWidget(this);
    routeWidget->setContentsMargins(0,0,0,0);
#ifndef Q_OS_MAC // not on mac thanks
    routeWidget->setStyleSheet("padding: 0px; border: 0px; margin: 0px;");
#endif
    QVBoxLayout *routeLayout = new QVBoxLayout(routeWidget);
    routeLayout->setSpacing(0);
    routeLayout->setContentsMargins(0,0,0,0);
    routeLayout->addWidget(routeNavigator);

    routeItem = new GcSplitterItem(tr("Routes"), iconFromPNG(":images/sidebar/folder.png"), this);
    QAction *routeAction = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    routeItem->addAction(routeAction);
    connect(routeAction, SIGNAL(triggered(void)), this, SLOT(routePopup()));
    routeItem->addWidget(routeWidget);




    QWidget *bestWidget = new QWidget(this);
    bestWidget->setContentsMargins(0,0,0,0);
#ifndef Q_OS_MAC // not on mac thanks
    bestWidget->setStyleSheet("padding: 0px; border: 0px; margin: 0px;");
#endif
    QVBoxLayout *bestLayout = new QVBoxLayout(bestWidget);
    bestLayout->setSpacing(0);
    bestLayout->setContentsMargins(0,0,0,0);
    bestLayout->addWidget(bestNavigator);

    bestItem = new GcSplitterItem(tr("Bests"), iconFromPNG(":images/sidebar/folder.png"), this);
    QAction *bestAction = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    bestItem->addAction(bestAction);
    connect(bestAction, SIGNAL(triggered(void)), this, SLOT(bestPopup()));
    bestItem->addWidget(bestWidget);

    splitter->addWidget(routeItem);
    splitter->addWidget(bestItem);

    splitter->prepare(context->athlete->cyclist, "interval");

    // GC signal
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));

    // right click menus...
    connect(routeNavigator,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showRouteMenu(const QPoint &)));
    //connect(context->athlete->intervalWidget,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showIntervalMenu(const QPoint &)));
    connect(bestNavigator,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showBestMenu(const QPoint &)));

    connect (context, SIGNAL(filterChanged()), this, SLOT(filterChanged()));

    configChanged();
}

void
IntervalSidebar::close()
{
    appsettings->setCValue(context->athlete->cyclist, GC_ROUTESORTBY, routeNavigator->sortByIndex());
    appsettings->setCValue(context->athlete->cyclist, GC_ROUTESORTBYORDER, routeNavigator->sortByOrder());
    //appsettings->setCValue(context->athlete->cyclist, GC_ROUTEGROUPBY, routeNavigator->groupBy());
    appsettings->setCValue(context->athlete->cyclist, GC_ROUTEHEADINGS, routeNavigator->columns());
    appsettings->setCValue(context->athlete->cyclist, GC_ROUTEHEADINGWIDTHS, routeNavigator->widths());
}

void
IntervalSidebar::configChanged()
{
    routeNavigator->tableView->viewport()->setPalette(GCColor::palette());
    routeNavigator->tableView->viewport()->setStyleSheet(QString("background: %1;").arg(GColor(CPLOTBACKGROUND).name()));

    bestNavigator->tableView->viewport()->setPalette(GCColor::palette());
    bestNavigator->tableView->viewport()->setStyleSheet(QString("background: %1;").arg(GColor(CPLOTBACKGROUND).name()));

    // interval tree
    context->athlete->intervalWidget->setPalette(GCColor::palette());
    context->athlete->intervalWidget->setStyleSheet(GCColor::stylesheet());

    repaint();
}

void
IntervalSidebar::filterChanged()
{
    if (context->isfiltered) setFilter(context->filters);
    else clearFilter();
}

void
IntervalSidebar::setFilter(QStringList filter)
{
    routeNavigator->searchStrings(filter);
    bestNavigator->searchStrings(filter);
}

void
IntervalSidebar::clearFilter()
{
    routeNavigator->clearSearch();
    bestNavigator->clearSearch();
}

/***********************************************************************
 * Sidebar Menus
 **********************************************************************/
void
IntervalSidebar::routePopup()
{
    // set the point for the menu and call below
    showRouteMenu(this->mapToGlobal(QPoint(routeItem->pos().x()+routeItem->width()-20, routeItem->pos().y())));
}

void
IntervalSidebar::showRouteMenu(const QPoint &pos)
{
    if (context->ride == 0) return; //none selected!

    RideItem *rideItem = (RideItem *)context->ride;

    if (rideItem != NULL) { 

        QMenu menu(routeNavigator);

        // ride navigator stuff
        QAction *colChooser = new QAction(tr("Show Column Chooser"), routeNavigator);
        connect(colChooser, SIGNAL(triggered(void)), routeNavigator, SLOT(showColumnChooser()));
        menu.addAction(colChooser);

        /*if (routeNavigator->groupBy() >= 0) {

            // already grouped lets ungroup
            QAction *nogroups = new QAction(tr("Do Not Show In Groups"), context->athlete->treeWidget);
            connect(nogroups, SIGNAL(triggered(void)), routeNavigator, SLOT(noGroups()));
            menu.addAction(nogroups);

        } else {

            QMenu *groupByMenu = new QMenu(tr("Group"), context->athlete->treeWidget);
            groupByMenu->setEnabled(true);
            menu.addMenu(groupByMenu);
            connect(groupByMenu, SIGNAL(triggered()), routeNavigator, SLOT(setGroupByColumnName()));

        }*/
        // expand / collapse
        QAction *expandAll = new QAction(tr("Expand All"), routeNavigator);
        connect(expandAll, SIGNAL(triggered(void)), routeNavigator->tableView, SLOT(expandAll()));
        menu.addAction(expandAll);

        // expand / collapse
        QAction *collapseAll = new QAction(tr("Collapse All"), routeNavigator);
        connect(collapseAll, SIGNAL(triggered(void)), routeNavigator->tableView, SLOT(collapseAll()));
        menu.addAction(collapseAll);
        menu.exec(pos);
    }
}

void
IntervalSidebar::bestPopup()
{
    // set the point for the menu and call below
    showBestMenu(this->mapToGlobal(QPoint(routeItem->pos().x()+routeItem->width()-20, routeItem->pos().y())));
}

void
IntervalSidebar::showBestMenu(const QPoint &pos)
{
    if (context->ride == 0) return; //none selected!

    RideItem *rideItem = context->ride;

    if (rideItem != NULL) {

        QMenu menu(bestNavigator);

        // ride navigator stuff
        QAction *colChooser = new QAction(tr("Show Column Chooser"), bestNavigator);
        connect(colChooser, SIGNAL(triggered(void)), bestNavigator, SLOT(showColumnChooser()));
        menu.addAction(colChooser);

        // expand / collapse
        QAction *expandAll = new QAction(tr("Expand All"), bestNavigator);
        connect(expandAll, SIGNAL(triggered(void)), bestNavigator->tableView, SLOT(expandAll()));
        menu.addAction(expandAll);

        // expand / collapse
        QAction *collapseAll = new QAction(tr("Collapse All"), bestNavigator);
        connect(collapseAll, SIGNAL(triggered(void)), bestNavigator->tableView, SLOT(collapseAll()));
        menu.addAction(collapseAll);
        menu.exec(pos);
    }
}
