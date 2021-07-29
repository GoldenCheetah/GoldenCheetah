/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#include "AnalysisSidebar.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include <QtGui>

// metadata support
#include "RideMetadata.h"
#include "SpecialFields.h"

// working with intervals
#include "IntervalItem.h"
#include "AddIntervalDialog.h"

// working with routes
#include "Route.h"

// the ride cache
#include "RideCache.h"

AnalysisSidebar::AnalysisSidebar(Context *context) : QWidget(context->mainWindow), context(context)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);
    
    splitter = new GcSplitter(Qt::Vertical);
    mainLayout->addWidget(splitter);

    autowidget = new CloudServiceAutoDownloadWidget(context, this);
    mainLayout->addWidget(autowidget);

    // calendar
    calendarWidget = new GcMultiCalendar(context);
    calendarItem = new GcSplitterItem(tr("Calendar"), iconFromPNG(":images/sidebar/calendar.png"), this);
    calendarItem->addWidget(calendarWidget);

    HelpWhatsThis *helpCalendar = new HelpWhatsThis(calendarWidget);
    calendarWidget->setWhatsThis(helpCalendar->getWhatsThisText(HelpWhatsThis::SideBarRidesView_Calendar));

    // Activity History
    context->rideNavigator = rideNavigator = new RideNavigator(context, true);
    rideNavigator->showMore(false);
    groupByMapper = NULL;

    // retrieve settings (properties are saved when we close the window)
    if (appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGS, "").toString() != "") {
        rideNavigator->setSortByIndex(appsettings->cvalue(context->athlete->cyclist, GC_SORTBY).toInt());
        rideNavigator->setSortByOrder(appsettings->cvalue(context->athlete->cyclist, GC_SORTBYORDER).toInt());
        rideNavigator->setGroupBy(appsettings->cvalue(context->athlete->cyclist, GC_NAVGROUPBY).toInt());
        rideNavigator->setColumns(appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGS).toString());
        rideNavigator->setWidths(appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGWIDTHS).toString());
    }

    activityHistory = new QWidget(this);
    activityHistory->setContentsMargins(0,0,0,0);
#ifndef Q_OS_MAC // not on mac thanks
    activityHistory->setStyleSheet("padding: 0px; border: 0px; margin: 0px;");
#endif
    QVBoxLayout *activityLayout = new QVBoxLayout(activityHistory);
    activityLayout->setSpacing(0);
    activityLayout->setContentsMargins(0,0,0,0);
    activityLayout->addWidget(rideNavigator);

    activityItem = new GcSplitterItem(tr("Activities"), iconFromPNG(":images/sidebar/folder.png"), this);
    QAction *activityAction = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    activityItem->addAction(activityAction);
    connect(activityAction, SIGNAL(triggered(void)), this, SLOT(analysisPopup()));
    activityItem->addWidget(activityHistory);

    // INTERVALS
    intervalTree = new IntervalTreeView(context);
    intervalTree->setAnimated(true);
    intervalTree->setColumnCount(1);
    intervalTree->setIndentation(5);
    intervalTree->setSortingEnabled(false);
    intervalTree->header()->hide();
    intervalTree->setAlternatingRowColors (false);
    intervalTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    intervalTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    intervalTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    intervalTree->setContextMenuPolicy(Qt::CustomContextMenu);
    intervalTree->setFrameStyle(QFrame::NoFrame);

    // create tree for user intervals, so its always at the top
    QTreeWidgetItem *tree = new QTreeWidgetItem(intervalTree->invisibleRootItem(), RideFileInterval::USER);
    tree->setData(0, Qt::UserRole, qVariantFromValue((void *)NULL)); // no intervalitem related
    tree->setText(0, RideFileInterval::typeDescription(RideFileInterval::USER));
    tree->setForeground(0, GColor(CPLOTMARKER));
    QFont bold;
    bold.setWeight(QFont::Bold);
    tree->setFont(0, bold);
    tree->setExpanded(true);
    tree->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled);
    tree->setHidden(true);
    trees.insert(RideFileInterval::USER, tree);

    intervalSummaryWindow = new IntervalSummaryWindow(context);

    HelpWhatsThis *helpSummaryWindow = new HelpWhatsThis(intervalSummaryWindow);
    intervalSummaryWindow->setWhatsThis(helpSummaryWindow->getWhatsThisText(HelpWhatsThis::SideBarRidesView_Intervals));

    intervalSplitter = new QSplitter(this);
    intervalSplitter->setHandleWidth(1);
    intervalSplitter->setOrientation(Qt::Vertical);
    intervalSplitter->addWidget(intervalTree);
    activeInterval = NULL;
    intervalSplitter->addWidget(intervalSummaryWindow);
    intervalSplitter->setFrameStyle(QFrame::NoFrame);
    intervalSplitter->setCollapsible(0, false);
    intervalSplitter->setCollapsible(1, false);

    HelpWhatsThis *helpIntervalSplitter = new HelpWhatsThis(intervalSplitter);
    intervalSplitter->setWhatsThis(helpIntervalSplitter->getWhatsThisText(HelpWhatsThis::SideBarRidesView_Intervals));

    intervalItem = new GcSplitterItem(tr("Intervals"), iconFromPNG(":images/mac/stop.png"), this);
    QAction *moreIntervalAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    intervalItem->addAction(moreIntervalAct);
    connect(moreIntervalAct, SIGNAL(triggered(void)), this, SLOT(intervalPopup()));
    intervalItem->addWidget(intervalSplitter);

    splitter->addWidget(calendarItem);
    splitter->addWidget(activityItem);
    splitter->addWidget(intervalItem);

    splitter->prepare(context->athlete->cyclist, "analysis");

    // GC signal
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(intervalsUpdate(RideItem*)), this, SLOT(intervalsUpdate(RideItem*)));
    connect(context, SIGNAL(intervalItemSelectionChanged(IntervalItem*)), this, SLOT(intervalItemSelectionChanged(IntervalItem*)));

    // right click menus...
    connect(rideNavigator,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showActivityMenu(const QPoint &)));
    connect(intervalTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showIntervalMenu(const QPoint &)));
    connect(intervalTree,SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(clickZoomInterval(QTreeWidgetItem*)));
    connect(intervalTree,SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

    connect (context, SIGNAL(filterChanged()), this, SLOT(filterChanged()));

    configChanged(CONFIG_APPEARANCE);
}

void
AnalysisSidebar::intervalItemSelectionChanged(IntervalItem*x)
{
    if (!x) return; // no null please.

    // find it and select/deselect
    QMapIterator<RideFileInterval::intervaltype, QTreeWidgetItem*> i(trees);
    i.toFront();
    while(i.hasNext()) {

        i.next();

        // loop through the intervals for this tree
        for(int j=0; j<i.value()->childCount(); j++) {

            // get pointer to the IntervalItem for this item
            QVariant v = i.value()->child(j)->data(0, Qt::UserRole);

            if (static_cast<IntervalItem*>(v.value<void*>()) == x) {
                i.value()->child(j)->setSelected(x->selected);
                return;
            }
        }
    }
}

void
AnalysisSidebar::intervalsUpdate(RideItem *changed)
{
    // refresh the sidebar for ours
    if (changed && changed == context->currentRideItem()) {
        setRide(const_cast<RideItem*>(context->currentRideItem()));
        context->notifyIntervalsChanged();
    }
}

void
AnalysisSidebar::setRide(RideItem*ride)
{
    // FIRST REFRESH THE INTERVAL TREE
    // stop SEGV in widgets watching for intervals being
    // selected whilst we are deleting them from the tree
    intervalTree->blockSignals(true);

    // refresh interval list for bottom left
    // clear each tree and hide it until items
    // are added. That way we get to keep the state
    // for expanded (users can unexpand it and that
    // is kept as they choose different activities)
    QMapIterator<RideFileInterval::intervaltype, QTreeWidgetItem*> i(trees);
    i.toFront();
    while(i.hasNext()) {

        i.next();
        i.value()->takeChildren();

        // now clean and hide the tree
        QList<QTreeWidgetItem *> items = i.value()->takeChildren();
        for (int j=0; j<items.count(); j++) delete items.at(j);
        i.value()->setHidden(true);
    }

    // now add the intervals for the current ride
    if (ride) { // only if we have a ride pointer

        // tree has bold font
        QFont bold;
        bold.setWeight(QFont::Bold);

        // for each type add a tree structure
        foreach(IntervalItem *interval, ride->intervals()) {

            // create trees on demand, not all will be needed by all
            // users and just as useful to create here than at startup
            // also means we don't need to know what all the types are
            // in advance, we create when we see one
            // BUT the USER tree is always created
            QTreeWidgetItem *tree = trees.value(interval->type, NULL);
            if (tree == NULL) {
                tree = new QTreeWidgetItem(intervalTree->invisibleRootItem(), interval->type);
                tree->setData(0, Qt::UserRole, qVariantFromValue((void *)NULL)); // no intervalitem related
                tree->setText(0, RideFileInterval::typeDescription(interval->type));
                tree->setForeground(0, GColor(CPLOTMARKER));
                tree->setFont(0, bold);
                tree->setExpanded(true);
                tree->setFlags(Qt::ItemIsEnabled);

                trees.insert(interval->type, tree);
            }
            tree->setHidden(false); // we have items, so make sure it is visible

            // add this interval to the tree
            QTreeWidgetItem *add = new QTreeWidgetItem(tree, interval->type);
            add->setText(0, interval->name);
            add->setData(0, Qt::UserRole, qVariantFromValue((void*)interval));
            add->setData(0, Qt::UserRole+1, QVariant(interval->color));
            add->setData(0, Qt::UserRole+2, QVariant(interval->test));
            add->setFlags(Qt::ItemIsEnabled
                          | Qt::ItemNeverHasChildren
                          | Qt::ItemIsSelectable 
                          | Qt::ItemIsDragEnabled 
                          | Qt::ItemIsEditable);

            // set interval selected to current state since it is
            // being maintained and we are called on resequencing etc
            add->setSelected(interval->selected);
        }

    }

    // all done, so connected widgets can receive signals now
    intervalTree->blockSignals(false);

    // now the other widgets
    calendarWidget->setRide(ride);
    rideNavigator->setRide(ride);
}

void
AnalysisSidebar::itemSelectionChanged()
{
    // update RideItem::intervals to reflect user selection
    QMapIterator<RideFileInterval::intervaltype, QTreeWidgetItem*> i(trees);
    i.toFront();
    while(i.hasNext()) {
        i.next();

        // loop through the intervals for this tree
        for(int j=0; j<i.value()->childCount(); j++) {

            // get pointer to the IntervalItem for this item
            QVariant v = i.value()->child(j)->data(0, Qt::UserRole);

            // make the IntervalItem selected flag reflect the current selection state
            static_cast<IntervalItem*>(v.value<void*>())->selected = i.value()->child(j)->isSelected();
        }
    }

    // clear hover now
    context->notifyIntervalHover(NULL);

    // notify the world
    context->notifyIntervalSelected();
}

void
AnalysisSidebar::close()
{
    appsettings->setCValue(context->athlete->cyclist, GC_SORTBY, rideNavigator->sortByIndex());
    appsettings->setCValue(context->athlete->cyclist, GC_SORTBYORDER, rideNavigator->sortByOrder());
    appsettings->setCValue(context->athlete->cyclist, GC_NAVGROUPBY, rideNavigator->groupBy());
    appsettings->setCValue(context->athlete->cyclist, GC_NAVHEADINGS, rideNavigator->columns());
    appsettings->setCValue(context->athlete->cyclist, GC_NAVHEADINGWIDTHS, rideNavigator->widths());
}

void
AnalysisSidebar::configChanged(qint32)
{
    //calendarWidget->setPalette(GCColor::palette());
    //intervalSummaryWindow->setPalette(GCColor::palette());
    //intervalSummaryWindow->setStyleSheet(GCColor::stylesheet());

    splitter->setPalette(GCColor::palette());
    activityHistory->setStyleSheet(QString("background: %1;").arg(GColor(CPLOTBACKGROUND).name()));
    rideNavigator->tableView->viewport()->setPalette(GCColor::palette());
    rideNavigator->tableView->viewport()->setStyleSheet(QString("background: %1;").arg(GColor(CPLOTBACKGROUND).name()));

    // interval tree
    intervalTree->setPalette(GCColor::palette());
    intervalTree->setStyleSheet(GCColor::stylesheet());
    QMapIterator<RideFileInterval::intervaltype, QTreeWidgetItem*> i(trees);
    i.toFront();
    while(i.hasNext()) {
        i.next();
        i.value()->setForeground(0, GColor(CPLOTMARKER));
    }

    repaint();
}

void
AnalysisSidebar::filterChanged()
{
    if (context->isfiltered) setFilter(context->filters);
    else clearFilter();
}

void
AnalysisSidebar::setFilter(QStringList filter)
{
    rideNavigator->searchStrings(filter);
    calendarWidget->setFilter(filter);
}

void
AnalysisSidebar::clearFilter()
{
    rideNavigator->clearSearch();
    calendarWidget->clearFilter();
}

/***********************************************************************
 * Sidebar Menus
 **********************************************************************/
void
AnalysisSidebar::analysisPopup()
{
    // set the point for the menu and call below
    showActivityMenu(this->mapToGlobal(QPoint(activityItem->pos().x()+activityItem->width()-20, activityItem->pos().y())));
}

void
AnalysisSidebar::showActivityMenu(const QPoint &pos)
{
    if (context->ride == 0) return; //none selected!

    RideItem *rideItem = context->ride;

    if (rideItem != NULL) { 
        QMenu menu(this);


        QAction *actSaveRide = new QAction(tr("Save Changes"), rideNavigator);
        connect(actSaveRide, SIGNAL(triggered(void)), context->mainWindow, SLOT(saveRide()));

        QAction *revertRide = new QAction(tr("Revert to Saved version"), rideNavigator);
        connect(revertRide, SIGNAL(triggered(void)), context->mainWindow, SLOT(revertRide()));

        QAction *actDeleteRide = new QAction(tr("Delete Activity"), rideNavigator);
        connect(actDeleteRide, SIGNAL(triggered(void)), context->mainWindow, SLOT(deleteRide()));

        QAction *actSplitRide = new QAction(tr("Split Activity"), rideNavigator);
        connect(actSplitRide, SIGNAL(triggered(void)), context->mainWindow, SLOT(splitRide()));

        if (rideItem->isDirty() == true) {
          menu.addAction(actSaveRide);
          menu.addAction(revertRide);
        }

        menu.addAction(actDeleteRide);
        menu.addAction(actSplitRide);

        QAction *actFindBest = new QAction(tr("Find Intervals..."), intervalItem);
        connect(actFindBest, SIGNAL(triggered(void)), this, SLOT(addIntervals(void)));
        menu.addAction(actFindBest);
        menu.addSeparator();

        // ride navigator stuff
        QAction *colChooser = new QAction(tr("Show Column Chooser"), rideNavigator);
        connect(colChooser, SIGNAL(triggered(void)), rideNavigator, SLOT(showColumnChooser()));
        menu.addAction(colChooser);

        if (rideNavigator->groupBy() >= 0) {

            // already grouped lets ungroup
            QAction *nogroups = new QAction(tr("Do Not Show In Groups"), rideNavigator);
            connect(nogroups, SIGNAL(triggered(void)), rideNavigator, SLOT(noGroups()));
            menu.addAction(nogroups);

        } else {

            QMenu *groupByMenu = new QMenu(tr("Group By"), rideNavigator);
            groupByMenu->setEnabled(true);
            menu.addMenu(groupByMenu);

            // add menu options for each column
            if (groupByMapper) delete groupByMapper;
            groupByMapper = new QSignalMapper(this);
            connect(groupByMapper, &QSignalMapper::mappedString, rideNavigator, &RideNavigator::setGroupByColumnName);

            foreach(QString heading, rideNavigator->columnNames()) {
                if (heading == "*") continue; // special hidden column

                QAction *groupByAct = new QAction(heading, rideNavigator);
                connect(groupByAct, SIGNAL(triggered()), groupByMapper, SLOT(map()));
                groupByMenu->addAction(groupByAct);

                // map action to column heading
                groupByMapper->setMapping(groupByAct, heading);
            }
        }
        // expand / collapse
        QAction *expandAll = new QAction(tr("Expand All"), rideNavigator);
        connect(expandAll, SIGNAL(triggered(void)), rideNavigator->tableView, SLOT(expandAll()));
        menu.addAction(expandAll);

        // expand / collapse
        QAction *collapseAll = new QAction(tr("Collapse All"), rideNavigator);
        connect(collapseAll, SIGNAL(triggered(void)), rideNavigator->tableView, SLOT(collapseAll()));
        menu.addAction(collapseAll);

        // reset to default
        QAction *resetToDefault = new QAction(tr("Reset to default"), rideNavigator);
        connect(resetToDefault, SIGNAL(triggered(void)), rideNavigator, SLOT(setWidths()));
        menu.addAction(resetToDefault);

        menu.exec(pos);
    }
}

void
AnalysisSidebar::intervalPopup()
{
    // Hamburger menu appears on the sidebar widget
    // to manipulate the interval tree

    // always show the 'find best' 'find peaks' options
    QMenu menu(this);

    RideItem *rideItem = context->ride;

    if (rideItem && rideItem->samples) {
        QAction *actFindBest = new QAction(tr("Find Intervals..."), intervalItem);
        connect(actFindBest, SIGNAL(triggered(void)), this, SLOT(addIntervals(void)));
        menu.addAction(actFindBest);

        // sort but only if 2 or more intervals
        if (rideItem->intervals(RideFileInterval::USER).count()>1) {
            QAction *actSort = new QAction(tr("Sort User Intervals"), intervalItem);
            connect(actSort, SIGNAL(triggered(void)), this, SLOT(sortIntervals(void)));
            menu.addAction(actSort);
        }

        if (rideItem->intervalsSelected().count()) menu.addSeparator();
    }

    QAction *actZoomOut = new QAction(tr("Zoom out"), intervalItem);
    connect(actZoomOut, SIGNAL(triggered(void)), this, SLOT(zoomOut(void)));
    menu.addAction(actZoomOut);

    if (rideItem && rideItem->intervalsSelected().count() == 1) {

        // we can zoom, rename etc if only 1 interval is selected
        QAction *actZoomInt = new QAction(tr("Zoom to interval"), intervalTree);
        connect(actZoomInt, SIGNAL(triggered(void)), this, SLOT(zoomIntervalSelected(void)));
        menu.addAction(actZoomInt);
    }

    // EDIT / DELETE SINGLE USER INTERVAL
    if (rideItem && rideItem->intervalsSelected(RideFileInterval::USER).count() == 1) {
        menu.addSeparator();
        QAction *actEditInt = new QAction(tr("Edit interval"), intervalTree);
        QAction *actDeleteInt = new QAction(tr("Delete interval"), intervalTree);
        connect(actEditInt, SIGNAL(triggered(void)), this, SLOT(editIntervalSelected(void)));
        connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteIntervalSelected(void)));
        menu.addAction(actEditInt);
        menu.addAction(actDeleteInt);
    }

    // RENAME DELETE LOTS OF USER INTERVALS
    if (rideItem && rideItem->intervalsSelected(RideFileInterval::USER).count() > 1) {
        menu.addSeparator();
        QAction *actRenameInt = new QAction(tr("Rename selected intervals"), intervalTree);
        connect(actRenameInt, SIGNAL(triggered(void)), this, SLOT(renameIntervalsSelected(void)));
        QAction *actDeleteInt = new QAction(tr("Delete selected intervals"), intervalTree);
        connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteIntervalSelected(void)));

        menu.addAction(actRenameInt);
        menu.addAction(actDeleteInt);
    }

    menu.exec(mapToGlobal((QPoint(intervalItem->pos().x()+intervalItem->width()-20, intervalItem->pos().y()))));
}

void
AnalysisSidebar::showIntervalMenu(const QPoint &pos)
{
    // Right-Click 'Context' menu appears on an interval item
    // to manipulate the specific interval, but we need to take
    // care as we should only really operate on user intervals

    QTreeWidgetItem *trItem = intervalTree->itemAt(pos);
    QTreeWidgetItem *root = trItem ? trItem->parent() : NULL;

    QVariant v = trItem ? trItem->data(0, Qt::UserRole) : QVariant();
    IntervalItem *interval = static_cast<IntervalItem*>(v.value<void*>());

    RideItem *rideItem = context->ride;

    if (trItem != NULL && root && interval) {

        // what kind if interval are we looking at ?
        RideFileInterval::IntervalType type = trees.key(root, static_cast<RideFileInterval::IntervalType>(-1));
        //bool isUser = interval->rideInterval != NULL;

        activeInterval = interval;
        QMenu menu(this);

        // ZOOM IN AND OUT FOR ALL
        QAction *actZoomOut = new QAction(tr("Zoom Out"), intervalTree);
        QAction *actZoomInt = new QAction(tr("Zoom to interval"), intervalTree);
        connect(actZoomOut, SIGNAL(triggered(void)), this, SLOT(zoomOut(void)));
        connect(actZoomInt, SIGNAL(triggered(void)), this, SLOT(zoomInterval(void)));
        menu.addAction(actZoomOut);
        menu.addAction(actZoomInt);

        // EDIT / DELETE SINGLE USER INTERVAL
        if (rideItem && rideItem->intervalsSelected(RideFileInterval::USER).count() == 1) {
            menu.addSeparator();
            QAction *actEditInt = new QAction(tr("Edit interval"), intervalTree);
            connect(actEditInt, SIGNAL(triggered(void)), this, SLOT(editIntervalSelected(void)));
            menu.addAction(actEditInt);

            // only add if not a performance test already !
            if (rideItem->intervalsSelected(RideFileInterval::USER).at(0)->test == false) {
                QAction *actPerfInt = new QAction(tr("Mark as a performance test"), intervalTree);
                connect(actPerfInt, SIGNAL(triggered(void)), this, SLOT(perfTestIntervalSelected(void)));
                menu.addAction(actPerfInt);
            }

            QAction *actDeleteInt = new QAction(tr("Delete interval"), intervalTree);
            connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteIntervalSelected(void)));
            menu.addAction(actDeleteInt);
        }

        // RENAME DELETE LOTS OF USER INTERVALS
        if (rideItem && rideItem->intervalsSelected(RideFileInterval::USER).count() > 1) {
            menu.addSeparator();
            QAction *actRenameInt = new QAction(tr("Rename selected intervals"), intervalTree);
            connect(actRenameInt, SIGNAL(triggered(void)), this, SLOT(renameIntervalsSelected(void)));
            QAction *actDeleteInt = new QAction(tr("Delete selected intervals"), intervalTree);
            connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteIntervalSelected(void)));

            menu.addAction(actRenameInt);
            menu.addAction(actDeleteInt);
        }


        if (type == RideFileInterval::ROUTE) {
            QAction *actEditInt = new QAction(tr("Rename route"), intervalTree);
            connect(actEditInt, SIGNAL(triggered(void)), this, SLOT(editRoute(void)));
            menu.addAction(actEditInt);

            // stop identifying this segment
            QAction *actDeleteRoute = new QAction(tr("Stop tracking this segment"), intervalTree);
            connect(actDeleteRoute, SIGNAL(triggered(void)), this, SLOT(deleteRoute(void)));
            menu.addAction(actDeleteRoute);

        }

        // BACK / FRONT NOT AVAILABLE YET
        //QAction *actFrontInt = new QAction(tr("Bring to Front"), intervalTree);
        //QAction *actBackInt = new QAction(tr("Send to back"), intervalTree);
        //connect(actFrontInt, SIGNAL(triggered(void)), this, SLOT(frontInterval(void)));
        //connect(actBackInt, SIGNAL(triggered(void)), this, SLOT(backInterval(void)));

        menu.addSeparator();

        // CREATE NEW ROUTE SEGMENT IF GPS PRESENT (AND NOT ALREADY A ROUTE!)
        if (type != RideFileInterval::USER) {

            QAction *actPerf = new QAction(tr("Create a performance test"), intervalTree);
            connect(actPerf, SIGNAL(triggered(void)), this, SLOT(createPerfTestIntervalSelected(void)));
            menu.addAction(actPerf);
        }
        // CREATE NEW ROUTE SEGMENT IF GPS PRESENT (AND NOT ALREADY A ROUTE!)
        if (type != RideFileInterval::ROUTE && 
            context->currentRideItem() && context->currentRideItem()->present.contains("G")) {

            QAction *actRoute = new QAction(tr("Create a route segment"), intervalTree);
            connect(actRoute, SIGNAL(triggered(void)), this, SLOT(createRouteIntervalSelected(void)));
            menu.addAction(actRoute);
        }

        menu.exec(intervalTree->mapToGlobal(pos));
    }
}

/*----------------------------------------------------------------------
 * Intervals
 *--------------------------------------------------------------------*/

void
AnalysisSidebar::addIntervals()
{
    if (context->ride && context->ride->ride() && context->ride->ride()->dataPoints().count()) {

        AddIntervalDialog *p = new AddIntervalDialog(context);
        p->setWindowModality(Qt::ApplicationModal); // don't allow select other ride or it all goes wrong!
        p->exec();

    } else {

        if (!context->ride || !context->ride->ride())
            QMessageBox::critical(this, tr("Find Intervals"), tr("No activity selected"));
        else
            QMessageBox::critical(this, tr("Find Intervals"), tr("Current activity contains no data"));
    }
}

bool
lessItem(const IntervalItem *s1, const IntervalItem *s2) {
    return s1->start < s2->start;
}

void
AnalysisSidebar::sortIntervals()
{
    // sort them chronologically
    QList<IntervalItem*> userIntervals = context->rideItem()->intervals(RideFileInterval::USER);
    QList<IntervalItem*> intervals = context->rideItem()->intervals();

    bool change = false;

    for (int i=0; i<userIntervals.count()-1;i++) {
        int min = i;

        for (int j=i+1; j<userIntervals.count();j++) {
            if (userIntervals.at(j)->start < userIntervals.at(min)->start) {
                min = j;
            }
        }

        if (min != i) {
            int indexFrom = intervals.indexOf(userIntervals.at(min));
            int indexTo = intervals.indexOf(userIntervals.at(i));

            userIntervals.move(min, i);
            intervals.move(indexFrom, indexTo);

            context->rideItem()->moveInterval(indexFrom, indexTo);
            change = true;
        }
    }

    if (change) {
        context->notifyIntervalsUpdate(context->rideItem());
        context->rideItem()->setDirty(true);
    }
}

// mark *first selected* USER intervals as a performance test
void
AnalysisSidebar::perfTestIntervalSelected()
{
    QTreeWidgetItem *userIntervals = trees.value(RideFileInterval::USER, NULL);

    // what are we renaming?
    if (context->currentRideItem() && userIntervals && userIntervals->childCount() &&
        context->currentRideItem()->intervalsSelected(RideFileInterval::USER).count()) {

        // make the IntervalItem selected flag reflect the current selection state
        IntervalItem *item = context->currentRideItem()->intervalsSelected(RideFileInterval::USER).first();

        // is it selected and linked ?
        if (item && item->rideInterval) {

            // set item and ride
            item->rideInterval->test = item->test = true;

            // mark dirty and tell the charts it changed
            const_cast<RideItem*>(context->currentRideItem())->setDirty(true);
            context->notifyIntervalsChanged();
        }
    }
}

// create a USER interval as a performance test copying the highlighted interval
void
AnalysisSidebar::createPerfTestIntervalSelected()
{
    if (activeInterval && context->currentRideItem()) {

        // just clone it as a user interval
        const_cast<RideItem*>(context->currentRideItem())->newInterval(tr("Performance Test"),
                                                                       activeInterval->start, activeInterval->stop,
                                                                       activeInterval->startKM, activeInterval->stopKM,
                                                                       Qt::red, true);

        // refresh etc
        context->notifyIntervalsUpdate(context->rideItem());
    }
}

// rename multiple intervals
void
AnalysisSidebar::renameIntervalsSelected()
{
    QTreeWidgetItem *userIntervals = trees.value(RideFileInterval::USER, NULL);

    // what are we renaming?
    if (context->currentRideItem() && userIntervals && userIntervals->childCount() && 
        context->currentRideItem()->intervalsSelected(RideFileInterval::USER).count()) {

        QString string = context->currentRideItem()->intervalsSelected(RideFileInterval::USER).first()->name;

        // type in a name and we will renumber all the intervals
        // in the same fashion -- esp if the last characters are
        RenameIntervalDialog dialog(string, this);
        dialog.setFixedWidth(320);

        if (dialog.exec()) {

            int number = 1;

            // does it end in a number?
            // if so we use that to renumber from
            QRegExp ends("^(.*)([0-9]+)$");
            if (ends.exactMatch(string)) {

                string = ends.cap(1);
                number = ends.cap(2).toInt();

            } else if (!string.endsWith(" ")) string += " ";

            // now go and renumber from 'number' with prefix 'string'
            for(int j=0; j<userIntervals->childCount(); j++) {

                // get pointer to the IntervalItem for this item
                QVariant v = userIntervals->child(j)->data(0, Qt::UserRole);

                // make the IntervalItem selected flag reflect the current selection state
                IntervalItem *item = static_cast<IntervalItem*>(v.value<void*>());

                // is it selected and linked ?
                if (item && item->selected && item->rideInterval) {

                    // set item and ride
                    item->rideInterval->name = item->name = 
                    QString("%1%2").arg(string).arg(number++);

                    // update tree to reflect changes!
                    userIntervals->child(j)->setText(0, item->name);
                }
            }

            // mark dirty and tell the charts it changed
            const_cast<RideItem*>(context->currentRideItem())->setDirty(true);
            context->notifyIntervalsChanged();
        }
    }
}

void
AnalysisSidebar::deleteIntervalSelected()
{
    // run down the USER tree looking for the selected intervals
    // and delete them. If none selected we do nothing.
    QTreeWidgetItem *userIntervals = trees.value(RideFileInterval::USER, NULL);

    if (userIntervals) {
        // Are you sure ?
        QMessageBox msgBox;
        msgBox.setText(tr("Are you sure you want to delete selected interval?"));
        QPushButton *deleteButton = msgBox.addButton(tr("Remove"),QMessageBox::YesRole);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        // nope, don't want to
        if(msgBox.clickedButton() != deleteButton) return;

        QList <QTreeWidgetItem*> deleteList;

        // loop through the intervals for this tree
        for(int j=0; j<userIntervals->childCount(); j++) {

            // get pointer to the IntervalItem for this item
            QVariant v = userIntervals->child(j)->data(0, Qt::UserRole);

            // make the IntervalItem selected flag reflect the current selection state
            IntervalItem *item = static_cast<IntervalItem*>(v.value<void*>());

            // is it selected and linked ?
            if (item->selected && item->rideInterval) {
                RideItem *rideItem = context->ride;
                if (rideItem->removeInterval(item) == true) {
                    deleteList << userIntervals->child(j);
                } else {
                    QMessageBox::warning(this, tr("Delete Interval"), tr("Unable to delete interval"));
                }
            }
        } 

        // now wipe the trees
        foreach (QTreeWidgetItem*item, deleteList) {
            userIntervals->removeChild(item);
            delete item;
        }
        context->notifyIntervalsChanged();
    }
}

void
AnalysisSidebar::deleteRoute()
{
    // stop tracking this route across rides
    if (activeInterval) {

        // Are you sure ?
        QMessageBox msgBox;
        msgBox.setText(tr("Are you sure you want to stop tracking this segment?"));
        QPushButton *deleteButton = msgBox.addButton(tr("Remove"),QMessageBox::YesRole);
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();

        // nope, don't want to
        if(msgBox.clickedButton() != deleteButton) return;

        // if refresh is running cancel it !
        context->athlete->rideCache->cancel();

        // zap it
        context->athlete->routes->deleteRoute(activeInterval->route);
        activeInterval = NULL; // it goes below.

        // the fingerprint for routes has changed, refresh is inevitable sadly
        context->athlete->rideCache->refresh();
    }
}

void
AnalysisSidebar::editRoute()
{
    // stop tracking this route across rides
    if (activeInterval) {
        // if refresh is running cancel it !
        context->athlete->rideCache->cancel();

        QString name = activeInterval->name;
        RenameIntervalDialog dialog(name, this);
        dialog.setFixedWidth(320);

        if (dialog.exec() && name != activeInterval->name) {
            context->athlete->routes->renameRoute(activeInterval->route, name);

            // loop through rides finding intervals on this route
            foreach(RideItem *ride, context->athlete->rideCache->rides()) {
                // find the interval?
                foreach(IntervalItem *interval, ride->intervals(RideFileInterval::ROUTE)) {
                    if (interval->route == activeInterval->route) {
                        //Make stale
                        ride->isstale = true;
                    }
                }
            }
            context->athlete->rideCache->refresh();
        }
    }
}

void
AnalysisSidebar::editIntervalSelected()
{
    // run down the USER tree looking for it and delete it
    QTreeWidgetItem *userIntervals = trees.value(RideFileInterval::USER, NULL);

    if (userIntervals) {

        // loop through the intervals for this tree
        for(int j=0; j<userIntervals->childCount(); j++) {

            // get pointer to the IntervalItem for this item
            QVariant v = userIntervals->child(j)->data(0, Qt::UserRole);

            // make the IntervalItem selected flag reflect the current selection state
            IntervalItem *item = static_cast<IntervalItem*>(v.value<void*>());

            // is it selected and linked ?
            if (item && item->selected && item->rideInterval) {

                activeInterval = item;
                editInterval();

                // update tree to reflect changes!
                userIntervals->child(j)->setText(0, activeInterval->name);
                userIntervals->child(j)->setData(0, Qt::UserRole+1, activeInterval->color);
                userIntervals->child(j)->setData(0, Qt::UserRole+2, activeInterval->test ? 255 : 0);

                // tell the charts !
                context->notifyIntervalsChanged();
                return;
            }
        }
    }
}

void
AnalysisSidebar::editInterval()
{
    IntervalItem temp(*activeInterval);
    EditIntervalDialog dialog(this, temp); // pass by reference

    if (dialog.exec()) {

        // update the interval item
        activeInterval->name = temp.name;
        activeInterval->color = temp.color;
        activeInterval->start = temp.start;
        activeInterval->stop = temp.stop;
        activeInterval->test = temp.test;
        activeInterval->startKM = activeInterval->rideItem()->ride()->timeToDistance(temp.start);
        activeInterval->stopKM = activeInterval->rideItem()->ride()->timeToDistance(temp.stop);

        if (activeInterval->rideInterval != NULL) { // User interval
            // update the ridefile interval
            activeInterval->rideInterval->name = activeInterval->name;
            activeInterval->rideInterval->start = activeInterval->start;
            activeInterval->rideInterval->stop = activeInterval->stop;
            activeInterval->rideInterval->test = activeInterval->test;
            activeInterval->rideInterval->color = activeInterval->color;
            activeInterval->rideItem()->setDirty(true);

            // refresh metrics!
            activeInterval->refresh();

            // now refresh charts
            context->notifyIntervalsChanged();
        }
    }
}

void
AnalysisSidebar::clickZoomInterval(QTreeWidgetItem*item)
{
    if (item) {

        // get interval reference for this tree item
        QVariant v = item->data(0, Qt::UserRole);
        IntervalItem *interval = static_cast<IntervalItem*>(v.value<void*>());
        if (interval) context->notifyIntervalZoom(interval);
    }
}

void
AnalysisSidebar::zoomIntervalSelected()
{
    RideItem *rideItem = context->ride;
    if (rideItem && rideItem->intervalsSelected().count()) {
        // zoom the one interval that is selected via popup menu
        context->notifyIntervalZoom(rideItem->intervalsSelected().first());
    }
}

void
AnalysisSidebar::zoomOut()
{
    context->notifyZoomOut(); // only really used by ride plot
}

void
AnalysisSidebar::zoomInterval() 
{
    // zoom into this interval on allPlot
    context->notifyIntervalZoom(activeInterval);
}

void
AnalysisSidebar::createRouteIntervalSelected() 
{
    // create a new route for this interval
    context->athlete->routes->createRouteFromInterval(activeInterval);

    // this will create a segment for the interval added
    if (context->currentRideItem()) 
        const_cast<RideItem*>(context->currentRideItem())->refresh();
}

void
AnalysisSidebar::frontInterval()
{
#if 0
    int oindex = activeInterval->displaySequence;
    for (int i=0; i<context->athlete->allIntervals->childCount(); i++) {
        IntervalItem *it = (IntervalItem *)context->athlete->allIntervals->child(i);
        int ds = it->displaySequence;
        if (ds > oindex)
            it->setDisplaySequence(ds-1);
    }
    activeInterval->setDisplaySequence(context->athlete->allIntervals->childCount());

    // signal!
    context->notifyIntervalsChanged();
#endif
}

void
AnalysisSidebar::backInterval()
{
#if 0
    int oindex = activeInterval->displaySequence;
    for (int i=0; i<context->athlete->allIntervals->childCount(); i++) {
        IntervalItem *it = (IntervalItem *)context->athlete->allIntervals->child(i);
        int ds = it->displaySequence;
        if (ds < oindex)
            it->setDisplaySequence(ds+1);
    }
    activeInterval->setDisplaySequence(1);

    // signal!
    context->notifyIntervalsChanged();

#endif
}


