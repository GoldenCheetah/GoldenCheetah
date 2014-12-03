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

AnalysisSidebar::AnalysisSidebar(Context *context) : QWidget(context->mainWindow), context(context)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);
    
    splitter = new GcSplitter(Qt::Vertical);
    mainLayout->addWidget(splitter);

    // calendar
    calendarWidget = new GcMultiCalendar(context);
    calendarItem = new GcSplitterItem(tr("Calendar"), iconFromPNG(":images/sidebar/calendar.png"), this);
    calendarItem->addWidget(calendarWidget);

    // Activity History
    rideNavigator = new RideNavigator(context, true);
    rideNavigator->setProperty("nomenu", true);
    groupByMapper = NULL;

    // retrieve settings (properties are saved when we close the window)
    if (appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGS, "").toString() != "") {
        rideNavigator->setSortByIndex(appsettings->cvalue(context->athlete->cyclist, GC_SORTBY).toInt());
        rideNavigator->setSortByOrder(appsettings->cvalue(context->athlete->cyclist, GC_SORTBYORDER).toInt());
        rideNavigator->setGroupBy(appsettings->cvalue(context->athlete->cyclist, GC_NAVGROUPBY).toInt());
        rideNavigator->setColumns(appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGS).toString());
        rideNavigator->setWidths(appsettings->cvalue(context->athlete->cyclist, GC_NAVHEADINGWIDTHS).toString());
    }

    QWidget *activityHistory = new QWidget(this);
    activityHistory->setContentsMargins(0,0,0,0);
#ifndef Q_OS_MAC // not on mac thanks
    activityHistory->setStyleSheet("padding: 0px; border: 0px; margin: 0px;");
#endif
    QVBoxLayout *activityLayout = new QVBoxLayout(activityHistory);
    activityLayout->setSpacing(0);
    activityLayout->setContentsMargins(0,0,0,0);
    activityLayout->addWidget(rideNavigator);

    activityItem = new GcSplitterItem(tr("Rides"), iconFromPNG(":images/sidebar/folder.png"), this);
    QAction *activityAction = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    activityItem->addAction(activityAction);
    connect(activityAction, SIGNAL(triggered(void)), this, SLOT(analysisPopup()));
    activityItem->addWidget(activityHistory);

    // INTERVALS
    intervalSummaryWindow = new IntervalSummaryWindow(context);

    intervalSplitter = new QSplitter(this);
    intervalSplitter->setHandleWidth(1);
    intervalSplitter->setOrientation(Qt::Vertical);
    intervalSplitter->addWidget(context->athlete->intervalWidget);
    activeInterval = NULL;
    intervalSplitter->addWidget(intervalSummaryWindow);
    intervalSplitter->setFrameStyle(QFrame::NoFrame);
    intervalSplitter->setCollapsible(0, false);
    intervalSplitter->setCollapsible(1, false);

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
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));

    // right click menus...
    connect(rideNavigator,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showActivityMenu(const QPoint &)));
    connect(context->athlete->intervalWidget,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showIntervalMenu(const QPoint &)));

    connect (context, SIGNAL(filterChanged()), this, SLOT(filterChanged()));

    configChanged();
}

void
AnalysisSidebar::setRide(RideItem*ride)
{
    calendarWidget->setRide(ride);
    rideNavigator->setRide(ride);
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
AnalysisSidebar::configChanged()
{
    //calendarWidget->setPalette(GCColor::palette());
    //intervalSummaryWindow->setPalette(GCColor::palette());
    //intervalSummaryWindow->setStyleSheet(GCColor::stylesheet());

    rideNavigator->tableView->viewport()->setPalette(GCColor::palette());
    rideNavigator->tableView->viewport()->setStyleSheet(QString("background: %1;").arg(GColor(CPLOTBACKGROUND).name()));

    // interval tree
    context->athlete->intervalWidget->setPalette(GCColor::palette());
    context->athlete->intervalWidget->setStyleSheet(GCColor::stylesheet());

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
        QMenu menu(rideNavigator);


        QAction *actSaveRide = new QAction(tr("Save Changes"), rideNavigator);
        connect(actSaveRide, SIGNAL(triggered(void)), context->mainWindow, SLOT(saveRide()));

        QAction *revertRide = new QAction(tr("Revert to Saved version"), rideNavigator);
        connect(revertRide, SIGNAL(triggered(void)), context->mainWindow, SLOT(revertRide()));

        QAction *actDeleteRide = new QAction(tr("Delete Ride"), rideNavigator);
        connect(actDeleteRide, SIGNAL(triggered(void)), context->mainWindow, SLOT(deleteRide()));

        QAction *actSplitRide = new QAction(tr("Split Ride"), rideNavigator);
        connect(actSplitRide, SIGNAL(triggered(void)), context->mainWindow, SLOT(splitRide()));

        if (rideItem->isDirty() == true) {
          menu.addAction(actSaveRide);
          menu.addAction(revertRide);
        }

        menu.addAction(actDeleteRide);
        menu.addAction(actSplitRide);
#ifdef GC_HAVE_ICAL
        QAction *actUploadCalendar = new QAction(tr("Upload Ride to Calendar"), rideNavigator);
        connect(actUploadCalendar, SIGNAL(triggered(void)), context->mainWindow, SLOT(uploadCalendar()));
        menu.addAction(actUploadCalendar);
#endif
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
            connect(groupByMapper, SIGNAL(mapped(const QString &)), rideNavigator, SLOT(setGroupByColumnName(QString)));

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
        menu.exec(pos);
    }
}

void
AnalysisSidebar::intervalPopup()
{
    // always show the 'find best' 'find peaks' options
    QMenu menu(intervalItem);

    RideItem *rideItem = context->ride;

    if (rideItem != NULL && rideItem->ride() && rideItem->ride()->dataPoints().count()) {
        QAction *actFindBest = new QAction(tr("Find Intervals..."), intervalItem);
        connect(actFindBest, SIGNAL(triggered(void)), this, SLOT(addIntervals(void)));
        menu.addAction(actFindBest);

        // sort but only if 2 or more intervals
        if (context->athlete->allIntervals->childCount() > 1) {
            QAction *actSort = new QAction(tr("Sort Intervals"), intervalItem);
            connect(actSort, SIGNAL(triggered(void)), this, SLOT(sortIntervals(void)));
            menu.addAction(actSort);
        }

        if (context->athlete->intervalWidget->selectedItems().count()) menu.addSeparator();
    }

    QAction *actZoomOut = new QAction(tr("Zoom out"), intervalItem);
    connect(actZoomOut, SIGNAL(triggered(void)), this, SLOT(zoomOut(void)));
    menu.addAction(actZoomOut);

    if (context->athlete->intervalWidget->selectedItems().count() == 1) {

        // we can zoom, rename etc if only 1 interval is selected
        QAction *actZoomInt = new QAction(tr("Zoom to interval"), context->athlete->intervalWidget);
        QAction *actEditInt = new QAction(tr("Edit interval"), context->athlete->intervalWidget);
        QAction *actDeleteInt = new QAction(tr("Delete interval"), context->athlete->intervalWidget);
        QAction *actCreateRouteInt = new QAction(tr("Create route for interval"), context->athlete->intervalWidget);

        connect(actZoomInt, SIGNAL(triggered(void)), this, SLOT(zoomIntervalSelected(void)));
        connect(actEditInt, SIGNAL(triggered(void)), this, SLOT(editIntervalSelected(void)));
        connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteIntervalSelected(void)));
        connect(actCreateRouteInt, SIGNAL(triggered(void)), this, SLOT(createRouteIntervalSelected(void)));

        menu.addAction(actZoomInt);
        menu.addAction(actEditInt);
        menu.addAction(actDeleteInt);
        menu.addSeparator();
        menu.addAction(actCreateRouteInt);
    }

    if (context->athlete->intervalWidget->selectedItems().count() > 1) {
        QAction *actRenameInt = new QAction(tr("Rename selected intervals"), context->athlete->intervalWidget);
        connect(actRenameInt, SIGNAL(triggered(void)), this, SLOT(renameIntervalsSelected(void)));
        QAction *actDeleteInt = new QAction(tr("Delete selected intervals"), context->athlete->intervalWidget);
        connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteIntervalSelected(void)));

        menu.addAction(actRenameInt);
        menu.addAction(actDeleteInt);
    }

    menu.exec(mapToGlobal((QPoint(intervalItem->pos().x()+intervalItem->width()-20, intervalItem->pos().y()))));
}

void
AnalysisSidebar::showIntervalMenu(const QPoint &pos)
{
    QTreeWidgetItem *trItem = context->athlete->intervalWidget->itemAt(pos);
    if (trItem != NULL && trItem->text(0) != tr("Intervals")) {

        activeInterval = (IntervalItem *)trItem;
        QMenu menu(context->athlete->intervalWidget);

        QAction *actEditInt = new QAction(tr("Edit interval"), context->athlete->intervalWidget);
        QAction *actDeleteInt = new QAction(tr("Delete interval"), context->athlete->intervalWidget);
        QAction *actZoomOut = new QAction(tr("Zoom Out"), context->athlete->intervalWidget);
        QAction *actZoomInt = new QAction(tr("Zoom to interval"), context->athlete->intervalWidget);
        QAction *actFrontInt = new QAction(tr("Bring to Front"), context->athlete->intervalWidget);
        QAction *actBackInt = new QAction(tr("Send to back"), context->athlete->intervalWidget);
        QAction *actCreateRouteInt = new QAction(tr("Create route for interval"), context->athlete->intervalWidget);

        connect(actEditInt, SIGNAL(triggered(void)), this, SLOT(editInterval(void)));
        connect(actDeleteInt, SIGNAL(triggered(void)), this, SLOT(deleteInterval(void)));
        connect(actZoomOut, SIGNAL(triggered(void)), this, SLOT(zoomOut(void)));
        connect(actZoomInt, SIGNAL(triggered(void)), this, SLOT(zoomInterval(void)));
        connect(actFrontInt, SIGNAL(triggered(void)), this, SLOT(frontInterval(void)));
        connect(actBackInt, SIGNAL(triggered(void)), this, SLOT(backInterval(void)));
        connect(actCreateRouteInt, SIGNAL(triggered(void)), this, SLOT(createRouteIntervalSelected(void)));


        menu.addAction(actZoomOut);
        menu.addAction(actZoomInt);
        menu.addAction(actEditInt);
        menu.addAction(actDeleteInt);
        menu.addSeparator();
        menu.addAction(actCreateRouteInt);

        menu.exec(context->athlete->intervalWidget->mapToGlobal(pos));
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
            QMessageBox::critical(this, tr("Find Intervals"), tr("No ride selected"));
        else
            QMessageBox::critical(this, tr("Find Intervals"), tr("Current ride contains no data"));
    }
}

void
AnalysisSidebar::addIntervalForPowerPeaksForSecs(RideFile *ride, int windowSizeSecs, QString name)
{
    QList<BestIntervalDialog::BestInterval> results;
    BestIntervalDialog::findBests(ride, windowSizeSecs, 1, results);
    if (results.isEmpty()) return;
    const BestIntervalDialog::BestInterval &i = results.first();
    QTreeWidgetItem *peak =
        new IntervalItem(ride, name+tr(" (%1 watts)").arg((int) round(i.avg)),
                         i.start, i.stop,
                         ride->timeToDistance(i.start),
                         ride->timeToDistance(i.stop),
                         context->athlete->allIntervals->childCount()+1);
    peak->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
    context->athlete->allIntervals->addChild(peak);
}

void
AnalysisSidebar::findPowerPeaks()
{

    if (context->ride && context->ride->ride() && context->ride->ride()->dataPoints().count()) {

        addIntervalForPowerPeaksForSecs(context->ride->ride(), 5, "Peak 5s");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 10, "Peak 10s");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 20, "Peak 20s");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 30, "Peak 30s");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 60, "Peak 1min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 120, "Peak 2min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 300, "Peak 5min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 600, "Peak 10min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 1200, "Peak 20min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 1800, "Peak 30min");
        addIntervalForPowerPeaksForSecs(context->ride->ride(), 3600, "Peak 60min");

        // now update the RideFileIntervals
        context->athlete->updateRideFileIntervals();

    } else {

        if (!context->ride || !context->ride->ride())
            QMessageBox::critical(this, tr("Find Power Peaks"), tr("No ride selected"));
        else
            QMessageBox::critical(this, tr("Find Power Peaks"), tr("Current ride contains no data"));
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
    QList<IntervalItem*> intervals;

    // set string to first interval selected
    for (int i=0; i<context->athlete->allIntervals->childCount();i++)
        intervals.append((IntervalItem*)(context->athlete->allIntervals->child(i)));

    // now sort them into start time order
    qStableSort(intervals.begin(), intervals.end(), lessItem);

    // empty context->athlete->allIntervals
    context->athlete->allIntervals->takeChildren();

    // and put em back in chronological sequence
    foreach(IntervalItem* item, intervals) {
        context->athlete->allIntervals->addChild(item);
    }

    // now update the ridefile
    context->athlete->updateRideFileIntervals(); // will emit intervalChanged() signal
}

// rename multiple intervals
void
AnalysisSidebar::renameIntervalsSelected()
{
    QString string;

    // set string to first interval selected
    for (int i=0; i<context->athlete->allIntervals->childCount();i++) {
        if (context->athlete->allIntervals->child(i)->isSelected()) {
            string = context->athlete->allIntervals->child(i)->text(0);
            break;
        }
    }

    // type in a name and we will renumber all the intervals
    // in the same fashion -- esp if the last characters are
    RenameIntervalDialog dialog(string, this);
    dialog.setFixedWidth(320);

    if (dialog.exec()) {

        int number = 1;

        // does it end in a number?
        // if so we use that to renumber from
        QRegExp ends("^(.*[^0-9])([0-9]+)$");
        if (ends.exactMatch(string)) {

            string = ends.cap(1);
            number = ends.cap(2).toInt();

        } else if (!string.endsWith(" ")) string += " ";

        // now go and renumber from 'number' with prefix 'string'
        for (int i=0; i<context->athlete->allIntervals->childCount();i++) {
            if (context->athlete->allIntervals->child(i)->isSelected())
                context->athlete->allIntervals->child(i)->setText(0, QString("%1%2").arg(string).arg(number++));
        }

        context->athlete->updateRideFileIntervals(); // will emit intervalChanged() signal
    }
}

void
AnalysisSidebar::deleteIntervalSelected()
{
    // delete the intervals that are selected (from the menu)
    // the normal delete intervals does that already
    deleteInterval();
}

void
AnalysisSidebar::deleteInterval()
{
    // now delete highlighted!
    for (int i=0; i<context->athlete->allIntervals->childCount();) {
        if (context->athlete->allIntervals->child(i)->isSelected()) delete context->athlete->allIntervals->takeChild(i);
        else i++;
    }

    context->athlete->updateRideFileIntervals(); // will emit intervalChanged() signal
}

void
AnalysisSidebar::renameIntervalSelected()
{
    // go edit the name
    for (int i=0; i<context->athlete->allIntervals->childCount();) {
        if (context->athlete->allIntervals->child(i)->isSelected()) {
            context->athlete->allIntervals->child(i)->setFlags(context->athlete->allIntervals->child(i)->flags() | Qt::ItemIsEditable);
            context->athlete->intervalWidget->editItem(context->athlete->allIntervals->child(i), 0);
            break;
        } else i++;
    }
    context->athlete->updateRideFileIntervals(); // will emit intervalChanged() signal
}

void
AnalysisSidebar::renameInterval() {
    // go edit the name
    activeInterval->setFlags(activeInterval->flags() | Qt::ItemIsEditable);
    context->athlete->intervalWidget->editItem(activeInterval, 0);
}

void
AnalysisSidebar::editIntervalSelected()
{
    // go edit the interval
    for (int i=0; i<context->athlete->allIntervals->childCount();) {
        if (context->athlete->allIntervals->child(i)->isSelected()) {
            activeInterval = (IntervalItem*)context->athlete->allIntervals->child(i);
            editInterval();
            break;
        } else i++;
    }
}

void
AnalysisSidebar::editInterval()
{
    IntervalItem temp = *activeInterval;
    EditIntervalDialog dialog(this, temp);

    if (dialog.exec()) {
        *activeInterval = temp;
        context->athlete->updateRideFileIntervals(); // will emit intervalChanged() signal
        context->athlete->intervalWidget->update();
    }
}

void
AnalysisSidebar::zoomIntervalSelected()
{
    // zoom the one interval that is selected via popup menu
    for (int i=0; i<context->athlete->allIntervals->childCount();) {
        if (context->athlete->allIntervals->child(i)->isSelected()) {
            context->notifyIntervalZoom((IntervalItem*)(context->athlete->allIntervals->child(i)));
            break;
        } else i++;
    }
}

void
AnalysisSidebar::zoomOut()
{
    context->notifyZoomOut(); // only really used by ride plot
}

void
AnalysisSidebar::zoomInterval() {
    // zoom into this interval on allPlot
    context->notifyIntervalZoom(activeInterval);
}

void
AnalysisSidebar::createRouteIntervalSelected() {
    // create a new route for this interval
    context->athlete->routes->createRouteFromInterval(activeInterval);
}

void
AnalysisSidebar::frontInterval()
{
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
}

void
AnalysisSidebar::backInterval()
{
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

}


