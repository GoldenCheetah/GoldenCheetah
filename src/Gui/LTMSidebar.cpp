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

#include "LTMSidebar.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "RideCache.h"
#include "Settings.h"
#include "GcUpgrade.h" // for URL to config at www.goldencheetah.org/
#include "Colors.h"
#include "Units.h"
#include "HelpWhatsThis.h"

#include "GcWindowRegistry.h" // for GcWinID types
#include "Perspective.h" // for GcWindowDialog
#include "LTMWindow.h" // for LTMWindow::settings()
#include "LTMChartParser.h" // for LTMChartParser::serialize && ChartTreeView

#include <QApplication>
#include <QScrollBar>
#include <QtGui>
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

// seasons support
#include "Season.h"
#include "Seasons.h"
#ifdef GC_HAVE_ICAL
#include "CalDAV.h" // upload Events to remote calendar
#endif

// named searchs
#include "FreeSearch.h"
#include "NamedSearch.h"
#include "DataFilter.h"

// ride cache
#include "RideCache.h"
#include "RideItem.h"
#include "Specification.h"

// metadata support
#include "RideMetadata.h"
#include "SpecialFields.h"


LTMSidebar::LTMSidebar(Context *context) : QWidget(context->mainWindow), context(context), active(false),
                                           isqueryfilter(false), isautofilter(false)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);

    seasonsWidget = new GcSplitterItem(tr("Date Ranges"), iconFromPNG(":images/sidebar/calendar.png"), this);
    QAction *moreSeasonAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    seasonsWidget->addAction(moreSeasonAct);
    connect(moreSeasonAct, SIGNAL(triggered(void)), this, SLOT(dateRangePopup(void)));

    dateRangeTree = new SeasonTreeView(context); // context needed for drag/drop across contexts
    allDateRanges=dateRangeTree->invisibleRootItem();
    // Drop for Seasons
    allDateRanges->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    allDateRanges->setText(0, tr("Date Ranges"));
    dateRangeTree->setFrameStyle(QFrame::NoFrame);
    dateRangeTree->setColumnCount(1);
    dateRangeTree->setSelectionMode(QAbstractItemView::SingleSelection);
    dateRangeTree->header()->hide();
    dateRangeTree->setIndentation(10);
    dateRangeTree->expandItem(allDateRanges);
    dateRangeTree->setContextMenuPolicy(Qt::CustomContextMenu);
#ifdef Q_OS_MAC
    dateRangeTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
    QStyle *cde = QStyleFactory::create(OS_STYLE);
    dateRangeTree->verticalScrollBar()->setStyle(cde);
#endif
    seasonsWidget->addWidget(dateRangeTree);

    HelpWhatsThis *helpDateRange = new HelpWhatsThis(dateRangeTree);
    dateRangeTree->setWhatsThis(helpDateRange->getWhatsThisText(HelpWhatsThis::SideBarTrendsView_DateRanges));

     // events
    eventsWidget = new GcSplitterItem(tr("Events"), iconFromPNG(":images/sidebar/bookmark.png"), this);
    QAction *moreEventAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    eventsWidget->addAction(moreEventAct);
    connect(moreEventAct, SIGNAL(triggered(void)), this, SLOT(eventPopup(void)));

    eventTree = new SeasonEventTreeView;
    eventTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    allEvents = eventTree->invisibleRootItem();
    // Drop for Events
    allEvents->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    allEvents->setText(0, tr("Events"));
    eventTree->setFrameStyle(QFrame::NoFrame);
    eventTree->setColumnCount(3);
    eventTree->setSelectionMode(QAbstractItemView::SingleSelection);
    eventTree->header()->hide();
    eventTree->setIndentation(5);
    eventTree->expandItem(allEvents);
    eventTree->setContextMenuPolicy(Qt::CustomContextMenu);
    eventTree->horizontalScrollBar()->setDisabled(true);
    eventTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#ifdef Q_OS_MAC
    eventTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
    cde = QStyleFactory::create(OS_STYLE);
    eventTree->verticalScrollBar()->setStyle(cde);
#endif

    eventsWidget->addWidget(eventTree);

    HelpWhatsThis *helpEventsTree = new HelpWhatsThis(eventTree);
    eventTree->setWhatsThis(helpEventsTree->getWhatsThisText(HelpWhatsThis::SideBarTrendsView_Events));

    // charts
    chartsWidget = new GcSplitterItem(tr("Charts"), iconFromPNG(":images/sidebar/charts.png"), this);

    // Chart Widget Actions
    QAction *moreChartAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    chartsWidget->addAction(moreChartAct);
    connect(moreChartAct, SIGNAL(triggered(void)), this, SLOT(presetPopup(void)));

    chartTree = new ChartTreeView(context);
    chartTree->setFrameStyle(QFrame::NoFrame);
    allCharts = chartTree->invisibleRootItem();
    allCharts->setText(0, tr("Events"));
    allCharts->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    chartTree->setColumnCount(1);
    chartTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    chartTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    chartTree->header()->hide();
    chartTree->setIndentation(5);
    chartTree->expandItem(allCharts);
    chartTree->setContextMenuPolicy(Qt::CustomContextMenu);
    chartTree->horizontalScrollBar()->setDisabled(true);
    chartTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#ifdef Q_OS_MAC
    chartTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
    cde = QStyleFactory::create(OS_STYLE);
    chartTree->verticalScrollBar()->setStyle(cde);
#endif
    chartsWidget->addWidget(chartTree);

    HelpWhatsThis *helpChartsTree = new HelpWhatsThis(chartTree);
    chartTree->setWhatsThis(helpChartsTree->getWhatsThisText(HelpWhatsThis::SideBarTrendsView_Charts));

    // setup for first time
    presetsChanged();

    // filters
    filtersWidget = new GcSplitterItem(tr("Filters"), iconFromPNG(":images/toolbar/filter3.png"), this);
    QAction *moreFilterAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    filtersWidget->addAction(moreFilterAct);
    connect(moreFilterAct, SIGNAL(triggered(void)), this, SLOT(filterPopup(void)));

    filterTree = new QTreeWidget;
    filterTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    allFilters = filterTree->invisibleRootItem();
    allFilters->setText(0, tr("Filters"));
    allFilters->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    filterTree->setFrameStyle(QFrame::NoFrame);
    filterTree->setColumnCount(1);
    filterTree->setSelectionBehavior(QAbstractItemView::SelectRows);
    filterTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    filterTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    filterTree->header()->hide();
    filterTree->setIndentation(5);
    filterTree->expandItem(allFilters);
    filterTree->setContextMenuPolicy(Qt::CustomContextMenu);
    filterTree->horizontalScrollBar()->setDisabled(true);
    filterTree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#ifdef Q_OS_MAC
    filterTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
    cde = QStyleFactory::create(OS_STYLE);
    filterTree->verticalScrollBar()->setStyle(cde);
#endif
    // we cast the filter tree and this because we use the same constructor XXX fix this!!!
    filterSplitter = new GcSubSplitter(Qt::Vertical, (GcSplitterControl*)filterTree, (GcSplitter*)this, true);
    filtersWidget->addWidget(filterSplitter);

    HelpWhatsThis *helpFilterTree = new HelpWhatsThis(filterTree);
    filterTree->setWhatsThis(helpFilterTree->getWhatsThisText(HelpWhatsThis::SideBarTrendsView_Filter));

    seasons = context->athlete->seasons;
    resetSeasons(); // reset the season list
    resetFilters(); // reset the filters list

    autoFilterMenu = new QMenu(tr("Autofilter"),this);
    configChanged(CONFIG_APPEARANCE); // will reset the metric tree and the autofilters

    splitter = new GcSplitter(Qt::Vertical);
    splitter->addWidget(seasonsWidget); // goes alongside events
    splitter->addWidget(eventsWidget); // goes alongside date ranges
    splitter->addWidget(filtersWidget);
    splitter->addWidget(chartsWidget); // for charts that 'use sidebar chart' charts ! (confusing or what?!)
    mainLayout->addWidget(splitter);

    splitter->prepare(context->athlete->cyclist, "LTM");

    // our date ranges
    connect(dateRangeTree,SIGNAL(itemSelectionChanged()), this, SLOT(dateRangeTreeWidgetSelectionChanged()));
    connect(dateRangeTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(dateRangePopup(const QPoint &)));
    connect(dateRangeTree,SIGNAL(itemMoved(QTreeWidgetItem *,int, int)), this, SLOT(dateRangeMoved(QTreeWidgetItem*, int, int)));

    // filters
    connect(filterTree,SIGNAL(itemSelectionChanged()), this, SLOT(filterTreeWidgetSelectionChanged()));

    // events
    connect(eventTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(eventPopup(const QPoint &)));
    connect(eventTree,SIGNAL(itemMoved(QTreeWidgetItem *,int, int)), this, SLOT(eventMoved(QTreeWidgetItem*, int, int)));

    // presets
    connect(chartTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(presetPopup(const QPoint &)));
    connect(chartTree,SIGNAL(itemSelectionChanged()), this, SLOT(presetSelectionChanged()));

    // GC signal
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(seasons, SIGNAL(seasonsChanged()), this, SLOT(resetSeasons()));
    connect(context->athlete, SIGNAL(namedSearchesChanged()), this, SLOT(resetFilters()));
    connect(context, SIGNAL(presetsChanged()), this, SLOT(presetsChanged()));
    connect(context, SIGNAL(presetSelected(int)), this, SLOT(presetSelected(int)));

    // setup colors
    configChanged(CONFIG_APPEARANCE);
}

void
LTMSidebar::presetsChanged()
{
    // rebuild the preset chart list as the presets have changed
    chartTree->clear();
    foreach(LTMSettings chart, context->athlete->presets) {
        QTreeWidgetItem *add;
        add = new QTreeWidgetItem(chartTree->invisibleRootItem());
        add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
        add->setText(0, chart.name);
    }
    chartTree->setCurrentItem(chartTree->invisibleRootItem()->child(0));
}

void
LTMSidebar::presetSelected(int index)
{
    // select it if it isn't selected now
    if (index < allCharts->childCount() && allCharts->child(index)->isSelected() == false) {

        // lets clear the current selections and select the one we want 
        // blocking signals to prevent endless loops
        chartTree->blockSignals(true);

        // clear
        foreach(QTreeWidgetItem *p, chartTree->selectedItems()) p->setSelected(false);

        // now select!
        allCharts->child(index)->setSelected(true);

        // back to normal
        chartTree->blockSignals(false);
    }
}

void
LTMSidebar::presetSelectionChanged()
{
    if (!chartTree->selectedItems().isEmpty()) {
        QTreeWidgetItem *which = chartTree->selectedItems().first();
        if (which != allCharts) {
            int index = allCharts->indexOfChild(which);
            if (index >=0 && index < context->athlete->presets.count())
                context->notifyPresetSelected(index);
        }
    }
}

void
LTMSidebar::configChanged(qint32)
{
    seasonsWidget->setStyleSheet(GCColor::stylesheet());
    eventsWidget->setStyleSheet(GCColor::stylesheet());
    chartsWidget->setStyleSheet(GCColor::stylesheet());
    filtersWidget->setStyleSheet(GCColor::stylesheet());
    setAutoFilterMenu();

    // set or reset the autofilter widgets
    autoFilterChanged();

    // forget what we just used...
    from = to = QDate();

    // let everyone know what date range we are starting with
    dateRangeTreeWidgetSelectionChanged();

}

/*----------------------------------------------------------------------
 * Selections Made
 *----------------------------------------------------------------------*/

void
LTMSidebar::selectDateRange(DateRange dr)
{
    for(int i=0; i<seasons->seasons.count(); i++) {
        Season s = seasons->seasons.at(i);
        if (s.getStart() == dr.from && s.getEnd() == dr.to && s.getName() == dr.name) {
            // bingo
            dateRangeTree->selectionModel()->clearSelection(); // clear selection
            allDateRanges->child(i)->setSelected(true); // select ours
            return;
        } else {
            QStringList names=dr.name.split("/");
            if (names.count() == 2 && s.getName() == names.at(0)) {
                for (int j=0; j<s.phases.count(); j++) {
                    Phase p = s.phases.at(j);
                    if (p.getStart() == dr.from && p.getEnd() == dr.to && p.getName() == names.at(1)) {
                        // bingo
                        dateRangeTree->selectionModel()->clearSelection(); // clear selection
                        allDateRanges->child(i)->child(j)->setSelected(true); // select ours
                        return;
                    }
                }
            }
        }
    }
}

void
LTMSidebar::dateRangeTreeWidgetSelectionChanged()
{
    if (active == true) return;

    const Season *dateRange = NULL;
    const Phase  *phase = NULL;

    if (dateRangeTree->selectedItems().isEmpty()) dateRange = NULL;
    else {
        QTreeWidgetItem *which = dateRangeTree->selectedItems().first();
        if (which->type() >= Season::season && which->type() < Phase::phase) {
            if (which != allDateRanges) {
                dateRange = &seasons->seasons.at(allDateRanges->indexOfChild(which));
                phase = NULL;
            } else {
                dateRange = NULL;
                phase = NULL;
            }
        } else if (which->type() >= Phase::phase) {
            int seasonIdx = allDateRanges->indexOfChild(which->parent());
            int phaseIdx = which->parent()->indexOfChild(which);
            if (which != allDateRanges) {
                dateRange = &seasons->seasons.at(seasonIdx);
                phase = &seasons->seasons.at(seasonIdx).phases[phaseIdx];
            } else {
                dateRange = NULL;
                phase = NULL;
            }
        }
    }

    if (dateRange) {
        int i;
        // clear events - we need to add for currently selected season
        for (i=allEvents->childCount(); i > 0; i--) {
            delete allEvents->takeChild(0);
        }

        // add this seasons events
        for (i=0; i <dateRange->events.count(); i++) {
            SeasonEvent event = dateRange->events.at(i);
            QTreeWidgetItem *add = new QTreeWidgetItem(allEvents);
            add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
            add->setText(0, event.name);
            add->setText(1, event.date.toString("MMM d, yyyy"));
            add->setText(2, SeasonEvent::priorityList().at(event.priority));
        }

        // make sure they fit
        eventTree->header()->resizeSections(QHeaderView::ResizeToContents);

        QString lastDateSel = phase ? phase->id().toString() : dateRange->id().toString();
        appsettings->setCValue(context->athlete->cyclist, GC_LTM_LAST_DATE_RANGE, lastDateSel);

    }

    // Let the view know its changed....
    if (phase) {
        emit dateRangeChanged(DateRange(phase->getStart(), phase->getEnd(), dateRange->getName() + "/" + phase->getName()));
        context->notifySeasonChanged(phase);
    } else if (dateRange) {
        emit dateRangeChanged(DateRange(dateRange->getStart(), dateRange->getEnd(), dateRange->getName()));
        context->notifySeasonChanged(dateRange);
    } else {
        emit dateRangeChanged(DateRange());
        context->notifySeasonChanged(nullptr);
    }
}

/*----------------------------------------------------------------------
 * Seasons stuff
 *--------------------------------------------------------------------*/

void
LTMSidebar::resetSeasons()
{
    if (active == true) return;

    active = true;

    // delete it and its children
    dateRangeTree->clear();

    // by default choose last 3 months not first one, since the first one is all dates
    // and that means aggregating all data when first starting...
    QString id = appsettings->cvalue(context->athlete->cyclist, GC_LTM_LAST_DATE_RANGE, "{00000000-0000-0000-0000-000000000012}").toString();
    for (int i=0; i <seasons->seasons.count(); i++) {
        Season season = seasons->seasons.at(i);
        QTreeWidgetItem *addSeason = new QTreeWidgetItem(allDateRanges, season.getType());
        if (season.id().toString()==id) {
            addSeason->setSelected(true);
        }

        // Drag and Drop is FINE for temporary seasons -- IT IS JUST A DATE RANGE!
        addSeason->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
        addSeason->setText(0, season.getName());


        // Phases
        for (int j=0; j <season.phases.count(); j++) {
            Phase phase = season.phases.at(j);
            QTreeWidgetItem *addPhase = new QTreeWidgetItem(addSeason, phase.getType());
            if (phase.id().toString() == id) {
                addSeason->setExpanded(true);
                addPhase->setSelected(true);
            }
            addPhase->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
            addPhase->setText(0, phase.getName());
        }
    }

    active = false;
}

int
LTMSidebar::newSeason(QString name, QDate start, QDate end, int type)
{
    seasons->newSeason(name, start, end, type);

    QTreeWidgetItem *item = new QTreeWidgetItem(Season::season);
    item->setText(0, name);
    allDateRanges->insertChild(0, item);
    return 0; // always add at the top
}

void
LTMSidebar::updateSeason(int index, QString name, QDate start, QDate end, int type)
{
    seasons->updateSeason(index, name, start, end, type);
    allDateRanges->child(index)->setText(0, name);
}

void
LTMSidebar::dateRangePopup(QPoint pos)
{
    QTreeWidgetItem *item = dateRangeTree->itemAt(pos);
    if (item != NULL) {

        // out of bounds or not user defined
        int seasonIdx = -1;
        int phaseIdx = -1;

        if (item->type() >= Season::season && item->type() < Phase::phase) {
            seasonIdx = allDateRanges->indexOfChild(item);
        } else if (item->type() >= Phase::phase) {
            seasonIdx = allDateRanges->indexOfChild(item->parent());
            phaseIdx = item->parent()->indexOfChild(item);
        }

        if (phaseIdx == -1 &&
                (seasonIdx == -1 || seasonIdx >= seasons->seasons.count() ||
                 seasons->seasons[seasonIdx].getType() == Season::temporary)) {
            // on system date we just offer to add a Season, since its
            // the only way of doing it when no seasons are defined!!

            // create context menu
            QMenu menu(dateRangeTree);
            QAction *add = new QAction(tr("Add season"), dateRangeTree);
            menu.addAction(add);

            // connect menu to functions
            connect(add, SIGNAL(triggered(void)), this, SLOT(addRange(void)));

            // execute the menu
            menu.exec(dateRangeTree->mapToGlobal(pos));

        } else if (phaseIdx > -1) {

            // create context menu
            QMenu menu(dateRangeTree);

            QAction *edit = new QAction(tr("Edit phase"), dateRangeTree);
            QAction *del = new QAction(tr("Delete phase"), dateRangeTree);
            QAction *season = new QAction(tr("Add season"), dateRangeTree);
            QAction *event = new QAction(tr("Add Event"), dateRangeTree);
            QAction *phase = new QAction(tr("Add Phase"), dateRangeTree);

            menu.addAction(edit);
            menu.addAction(del);
            menu.addSeparator();
            menu.addAction(season);
            menu.addAction(event);
            menu.addAction(phase);

            // connect menu to functions
            connect(edit, SIGNAL(triggered(void)), this, SLOT(editRange(void)));
            connect(del, SIGNAL(triggered(void)), this, SLOT(deleteRange(void)));
            connect(season, SIGNAL(triggered(void)), this, SLOT(addRange(void)));
            connect(event, SIGNAL(triggered(void)), this, SLOT(addEvent(void)));
            connect(phase, SIGNAL(triggered(void)), this, SLOT(addPhase(void)));

            // execute the menu
            menu.exec(dateRangeTree->mapToGlobal(pos));
        } else {

            // create context menu
            QMenu menu(dateRangeTree);

            QAction *edit = new QAction(tr("Edit season"), dateRangeTree);
            QAction *del = new QAction(tr("Delete season"), dateRangeTree);
            QAction *season = new QAction(tr("Add season"), dateRangeTree);
            QAction *event = new QAction(tr("Add Event"), dateRangeTree);
            QAction *phase = new QAction(tr("Add Phase"), dateRangeTree);

            if (! seasons->seasons[seasonIdx].isAbsolute()) {
                event->setEnabled(false);
                phase->setEnabled(false);
            }

            menu.addAction(edit);
            menu.addAction(del);
            menu.addSeparator();
            menu.addAction(season);
            menu.addAction(event);
            menu.addAction(phase);

            // connect menu to functions
            connect(edit, SIGNAL(triggered(void)), this, SLOT(editRange(void)));
            connect(del, SIGNAL(triggered(void)), this, SLOT(deleteRange(void)));
            connect(season, SIGNAL(triggered(void)), this, SLOT(addRange(void)));
            connect(event, SIGNAL(triggered(void)), this, SLOT(addEvent(void)));
            connect(phase, SIGNAL(triggered(void)), this, SLOT(addPhase(void)));

            // execute the menu
            menu.exec(dateRangeTree->mapToGlobal(pos));
        }
    }
}

void
LTMSidebar::dateRangePopup()
{
    // no current season selected
    if (dateRangeTree->selectedItems().isEmpty()) return;

    QTreeWidgetItem *item = dateRangeTree->selectedItems().at(0);
    int seasonIdx = -1;
    int phaseIdx = -1;

    if (item->type() >= Season::season && item->type() < Phase::phase) {
        seasonIdx = allDateRanges->indexOfChild(item);
    } else if (item->type() >= Phase::phase) {
        seasonIdx = allDateRanges->indexOfChild(item->parent());
        phaseIdx = item->parent()->indexOfChild(item);
    }

    // OK - we are working with a specific event..
    QMenu menu(dateRangeTree);

    if (phaseIdx == -1 &&
            (seasonIdx == -1 || seasonIdx >= seasons->seasons.count() ||
             seasons->seasons[seasonIdx].getType() == Season::temporary)) {
        QAction *season = new QAction(tr("Add season"), dateRangeTree);

        menu.addAction(season);

        // connect menu to functions
        connect(season, SIGNAL(triggered(void)), this, SLOT(addRange(void)));
    } else if (phaseIdx > -1) {
        QAction *edit = new QAction(tr("Edit phase"), dateRangeTree);
        QAction *del = new QAction(tr("Delete phase"), dateRangeTree);
        QAction *season = new QAction(tr("Add season"), dateRangeTree);
        QAction *event = new QAction(tr("Add Event"), dateRangeTree);
        QAction *phase = new QAction(tr("Add Phase"), dateRangeTree);

        menu.addAction(edit);
        menu.addAction(del);
        menu.addSeparator();
        menu.addAction(season);
        menu.addAction(event);
        menu.addAction(phase);

        // connect menu to functions
        connect(edit, SIGNAL(triggered(void)), this, SLOT(editRange(void)));
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteRange(void)));
        connect(season, SIGNAL(triggered(void)), this, SLOT(addRange(void)));
        connect(event, SIGNAL(triggered(void)), this, SLOT(addEvent(void)));
        connect(phase, SIGNAL(triggered(void)), this, SLOT(addPhase(void)));
    } else {
        QAction *edit = new QAction(tr("Edit season"), dateRangeTree);
        QAction *del = new QAction(tr("Delete season"), dateRangeTree);
        QAction *season = new QAction(tr("Add season"), dateRangeTree);
        QAction *event = new QAction(tr("Add Event"), dateRangeTree);
        QAction *phase = new QAction(tr("Add Phase"), dateRangeTree);

        menu.addAction(edit);
        menu.addAction(del);
        menu.addSeparator();
        menu.addAction(season);
        menu.addAction(event);
        menu.addAction(phase);

        // connect menu to functions
        connect(edit, SIGNAL(triggered(void)), this, SLOT(editRange(void)));
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteRange(void)));
        connect(season, SIGNAL(triggered(void)), this, SLOT(addRange(void)));
        connect(event, SIGNAL(triggered(void)), this, SLOT(addEvent(void)));
        connect(phase, SIGNAL(triggered(void)), this, SLOT(addPhase(void)));
    }
    // execute the menu
    menu.exec(splitter->mapToGlobal(QPoint(seasonsWidget->pos().x()+seasonsWidget->width()-20,
                                           seasonsWidget->pos().y())));
}

void
LTMSidebar::eventPopup(QPoint pos)
{
    // no current season selected
    if (dateRangeTree->selectedItems().isEmpty()) return;

    QTreeWidgetItem *item = eventTree->itemAt(pos);

    // save context - which season and event are we working with?
    QTreeWidgetItem *which = dateRangeTree->selectedItems().first();
    if (!which || which == allDateRanges) return;

    // OK - we are working with a specific event..
    QMenu menu(eventTree);
    if (item != NULL && allEvents->indexOfChild(item) != -1) {

        QAction *edit = new QAction(tr("Edit details"), eventTree);
        QAction *del = new QAction(tr("Delete event"), eventTree);
        menu.addAction(edit);
        menu.addAction(del);

        // connect menu to functions
        connect(edit, SIGNAL(triggered(void)), this, SLOT(editEvent(void)));
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteEvent(void)));
    }

    // we can always add, regardless of any event being selected...
    QAction *addEvent = new QAction(tr("Add event"), eventTree);
    menu.addAction(addEvent);
    connect(addEvent, SIGNAL(triggered(void)), this, SLOT(addEvent(void)));

    // execute the menu
    menu.exec(eventTree->mapToGlobal(pos));
}

void
LTMSidebar::eventPopup()
{
    // events are against a selected season
    if (dateRangeTree->selectedItems().count() == 0) return; // need a season selected!

    // if a phase is selected (rather than a season), get the season this phase belongs to
    QTreeWidgetItem *selectedDateRange = dateRangeTree->selectedItems().first();
    if (selectedDateRange->parent() != NULL) {
        selectedDateRange = selectedDateRange->parent();
    }

    // and the season must be user defined not temporary
    int seasonindex = allDateRanges->indexOfChild(selectedDateRange);
    if (seasons->seasons[seasonindex].getType() == Season::temporary) return;

    // have we selected an event?
    QTreeWidgetItem *item = NULL;
    if (!eventTree->selectedItems().isEmpty()) item = eventTree->selectedItems().at(0);

    QMenu menu(eventTree);

    // we can always add, regardless of any event being selected...
    QAction *addEvent = new QAction(tr("Add event"), eventTree);
    menu.addAction(addEvent);
    connect(addEvent, SIGNAL(triggered(void)), this, SLOT(addEvent(void)));

    if (item != NULL && allEvents->indexOfChild(item) != -1) {

        QAction *edit = new QAction(tr("Edit details"), eventTree);
        QAction *del = new QAction(tr("Delete event"), eventTree);
        menu.addAction(edit);
        menu.addAction(del);

        // connect menu to functions
        connect(edit, SIGNAL(triggered(void)), this, SLOT(editEvent(void)));
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteEvent(void)));
    }

    // execute the menu
    menu.exec(splitter->mapToGlobal(QPoint(eventsWidget->pos().x()+eventsWidget->width()-20, eventsWidget->pos().y())));
}

void
LTMSidebar::manageFilters()
{
    EditNamedSearches *editor = new EditNamedSearches(context->mainWindow, context);
    editor->move(QCursor::pos()+QPoint(10,-200));
    editor->show();
}

void
LTMSidebar::setAutoFilterMenu()
{
    active = true;

    QStringList on = appsettings->cvalue(context->athlete->cyclist, GC_LTM_AUTOFILTERS, tr("Workout Code|Sport")).toString().split("|");
    autoFilterMenu->clear();
    autoFilterState.clear();

    // Convert field names for Internal to Display (to work with the translated values)
    SpecialFields& sp = SpecialFields::getInstance();
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {

        if (field.tab != "" && (field.type == GcFieldType::FIELD_TEXT || field.type == GcFieldType::FIELD_SHORTTEXT)) { // we only do text or shorttext fields

            QAction *action = new QAction(sp.displayName(field.name), this);
            action->setCheckable(true);

            if (on.contains(sp.displayName(field.name))) action->setChecked(true);
            else action->setChecked(false);

            connect(action, SIGNAL(triggered()), this, SLOT(autoFilterChanged()));

            // remove from tree if its already there
            GcSplitterItem *item = filterSplitter->removeItem(action->text());
            if (item) delete item; // will be removed from splitter too

            // if you crash on this line compile with QT5.3 or higher
            // or at least avoid the 5.3 RC1 release (see QTBUG-38685)
            autoFilterMenu->addAction(action);
            autoFilterState << false;
        }
    }
    active = false;
}

void 
LTMSidebar::autoFilterChanged()
{
    if (active) return;

    QString on;

    // boop
    int i=0;
    foreach (QAction *action, autoFilterMenu->actions()) {

        // activate
        if (action->isChecked() && autoFilterState.at(i) == false) {
            autoFilterState[i] = true;

            GcSplitterItem *item = new GcSplitterItem(action->text(), QIcon(), this);

            // get a tree of values
            QTreeWidget *tree = new QTreeWidget(item);
            tree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            tree->setFrameStyle(QFrame::NoFrame);
            tree->setColumnCount(1);
            tree->setSelectionBehavior(QAbstractItemView::SelectRows);
            tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
            tree->setSelectionMode(QAbstractItemView::MultiSelection);
            tree->header()->hide();
            tree->setIndentation(5);
            tree->expandItem(tree->invisibleRootItem());
            tree->setContextMenuPolicy(Qt::CustomContextMenu);
            tree->horizontalScrollBar()->setDisabled(true);
            tree->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
#ifdef Q_OS_MAC
            tree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
#ifdef Q_OS_WIN
            QStyle *cde = QStyleFactory::create(OS_STYLE);
            tree->verticalScrollBar()->setStyle(cde);
#endif
            item->addWidget(tree);
            filterSplitter->addWidget(item);

            HelpWhatsThis *helpFilterTree = new HelpWhatsThis(tree);
            tree->setWhatsThis(helpFilterTree->getWhatsThisText(HelpWhatsThis::SideBarTrendsView_Filter));

            // Convert field names for Internal to Display (to work with the translated values)
            SpecialFields& sp = SpecialFields::getInstance();
            // update the values available in the tree
            foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
                if (sp.displayName(field.name) == action->text()) {
                    foreach (QString value, context->athlete->rideCache->getDistinctValues(field.name)) {
                        if (value == "") value = tr("(blank)");
                        QTreeWidgetItem *add = new QTreeWidgetItem(tree->invisibleRootItem(), 0);

                        // No Drag/Drop for autofilters
                        add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                        add->setText(0, value);
                    }
                }
            }
            connect(tree,SIGNAL(itemSelectionChanged()), this, SLOT(autoFilterSelectionChanged()));

        }

        // deactivate
        if (!action->isChecked() && autoFilterState.at(i) == true) {
            autoFilterState[i] = false;

            // remove from tree
            GcSplitterItem *item = filterSplitter->removeItem(action->text());
            if (item) {
                // if there were items selected in the tree and we 
                // remove it we need to clear the filter
                bool selected = static_cast<QTreeWidget*>(item->content)->selectedItems().count() > 0;

                // will remove from the splitter (is only way to do this!)
                delete item; // splitter deletes handle too !

                // clear any results of the selection
                if (selected) autoFilterSelectionChanged();
            }
        }

        // reset the selected fields
        if (action->isChecked())  {
            if (on != "") on += "|";
            on += action->text();
        }

        i++;
    }
    appsettings->setCValue(context->athlete->cyclist, GC_LTM_AUTOFILTERS, on);
}

void 
LTMSidebar::filterTreeWidgetSelectionChanged()
{
    int selected = filterTree->selectedItems().count();

    if (selected) {

        QStringList errors, files; // results of all the selections
        bool first = true;

        foreach (QTreeWidgetItem *item, filterTree->selectedItems()) {

            int index = filterTree->invisibleRootItem()->indexOfChild(item);

            NamedSearch ns = context->athlete->namedSearches->get(index);
            QStringList errors, results;

            switch(ns.type) {

            case NamedSearch::filter :
                {
                    // use a data filter
                    DataFilter f(this, context);
                    errors = f.parseFilter(context, ns.text, &results);
                }
                break;

            case NamedSearch::search :
                {
                    // use RideCache
                    FreeSearch s;
                    results = s.search(context, ns.text);
                }

            }

            // lets filter the results!
            if (first) files = results;
            else {
                QStringList filtered;
                foreach(QString file, files)
                    if (results.contains(file))
                        filtered << file;
                files = filtered;
            }

            first = false;
        }

        queryFilterFiles = files;
        isqueryfilter = true;

    } else {

        queryFilterFiles.clear();
        isqueryfilter = false;
    }

    // tell the world
    filterNotify();
}

void
LTMSidebar::filterNotify()
{
    // either the auto filter or query filter
    // has been updated, so we need to merge the results
    // of both and then notify via context

    // no filter so clear
    if (isqueryfilter == false && isautofilter == false) {
        context->clearHomeFilter();
    } else {

        if (isqueryfilter == false) {

            // autofilter only
            context->setHomeFilter(autoFilterFiles);

        } else if (isautofilter == false) {

            // queryfilter only
            context->setHomeFilter(queryFilterFiles);

        } else {

            // both are set, so merge results
            auto mergedSet =
                    QSet<QString>(
                            autoFilterFiles.begin(),
                            autoFilterFiles.end()).intersect(
                    QSet<QString>(
                            queryFilterFiles.begin(),
                            queryFilterFiles.end()
                            )
                    );
            QStringList merged = QStringList(mergedSet.begin(), mergedSet.end());
            context->setHomeFilter(merged);
        }

    }
}

void
LTMSidebar::autoFilterRefresh()
{
    // the data has changed so refresh the trees
    for (int i=1; i<filterSplitter->count(); i++) {

        GcSplitterItem *item = static_cast<GcSplitterItem*>(filterSplitter->widget(i));
        QTreeWidget *tree = static_cast<QTreeWidget*>(item->content);

        qDeleteAll(tree->invisibleRootItem()->takeChildren());

        // translate fields back from Display Name to internal Name !
        SpecialFields& sp = SpecialFields::getInstance();

        // what is the field?
        QString fieldname = sp.internalName(item->splitterHandle->title());

        // update the values available in the tree
        foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
            if (field.name == fieldname) {
                foreach (QString value, context->athlete->rideCache->getDistinctValues(field.name)) {
                    if (value == "") value = tr("(blank)");
                    QTreeWidgetItem *add = new QTreeWidgetItem(tree->invisibleRootItem(), 0);

                    // No Drag/Drop for autofilters
                    add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
                    add->setText(0, value);
                }
            }
        }
    }
}

void
LTMSidebar::autoFilterSelectionChanged()
{
    // only fetch when we know we need to filter ..
    QSet<QString> matched;

    // assume nothing to do...
    isautofilter = false;

    // Convert field names from Display to Internal (to work with translations)
    SpecialFields& sp = SpecialFields::getInstance();

    // are any auto filters applied?
    for (int i=1; i<filterSplitter->count(); i++) {

        GcSplitterItem *item = static_cast<GcSplitterItem*>(filterSplitter->widget(i));
        QTreeWidget *tree = static_cast<QTreeWidget*>(item->content);

        // are some values selected?
        if (tree->selectedItems().count() > 0) {

            // we have a selection!
            if (isautofilter == false) {
                isautofilter = true;
                foreach(RideItem *x, context->athlete->rideCache->rides()) matched << x->fileName;
            }

            // what is the field?
            QString fieldname = sp.internalName(item->splitterHandle->title());

            // what values are highlighted
            QStringList values;
            foreach (QTreeWidgetItem *wi, tree->selectedItems()) 
                values << wi->text(0);

            // get a set of filenames that match
            QSet<QString> matches;
            foreach(RideItem *x, context->athlete->rideCache->rides()) {

                // we use XXX___XXX___XXX because it is not likely to exist
                QString value = x->getText(fieldname, "XXX___XXX___XXX");
                if (value == "") value = tr("(blank)"); // match blanks!

                if (values.contains(value)) matches << x->fileName;
            }

            // now remove items from the matched list that
            // are not in my list of matches
            matched = matched.intersect(matches);
        }
    }

    // all done
    autoFilterFiles = matched.values();

    // tell the world
    filterNotify();
}

void
LTMSidebar::resetFilters()
{
    if (active == true) return;

    active = true;
    int i;
    for (i=allFilters->childCount(); i > 0; i--) {
        delete allFilters->takeChild(0);
    }

    foreach(NamedSearch ns, context->athlete->namedSearches->getList()) {
        
        QTreeWidgetItem *add = new QTreeWidgetItem(allFilters, 0);

        // No Drag/Drop for filters
        add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        add->setText(0, ns.name);
    }

    active = false;
}

void
LTMSidebar::filterPopup()
{
    // is one selected for deletion?
    int selected = filterTree->selectedItems().count();

    QMenu menu(filterTree);

    // we can always add, regardless of any event being selected...
    QAction *addEvent = new QAction(tr("Manage Filters"), filterTree);
    menu.addAction(addEvent);
    connect(addEvent, SIGNAL(triggered(void)), this, SLOT(manageFilters(void)));

    if (selected) {

        QAction *del = new QAction(QString(tr("Delete Filter%1")).arg(selected>1 ? "s" : ""), filterTree);
        menu.addAction(del);

        // connect menu to functions
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteFilter(void)));
    }

    menu.addSeparator();
    menu.addMenu(autoFilterMenu);

    // execute the menu
    menu.exec(splitter->mapToGlobal(QPoint(filtersWidget->pos().x()+filtersWidget->width()-20, filtersWidget->pos().y())));
}

void
LTMSidebar::deleteFilter()
{
    if (filterTree->selectedItems().count() <= 0) return;

    active = true; // no need to reset tree when items deleted from model!
    while (filterTree->selectedItems().count()) {
        int index = allFilters->indexOfChild(filterTree->selectedItems().first());

        // now delete!
        delete allFilters->takeChild(index);
        context->athlete->namedSearches->deleteNamedSearch(index);
    }
    active = false;
}

void
LTMSidebar::dateRangeMoved(QTreeWidgetItem*item, int oldposition, int newposition)
{
    // report the move in the seasons
    seasons->seasons.move(oldposition, newposition);

    // save changes away
    active = true;
    seasons->writeSeasons();
    active = false;

    // deselect actual selection
    dateRangeTree->selectedItems().first()->setSelected(false);
    // select the move/drop item
    item->setSelected(true);
}

void
LTMSidebar::addRange()
{
    Season newOne;

    EditSeasonDialog dialog(context, &newOne);

    if (dialog.exec()) {

        active = true;

        // check dates are right way round...
        if (newOne.getStart() > newOne.getEnd()) {
            QDate temp = newOne.getStart();
            newOne.setAbsoluteStart(newOne.getEnd());
            newOne.setAbsoluteEnd(temp);
        }

        // save
        seasons->seasons.insert(0, newOne);
        seasons->writeSeasons();
        active = false;

        // signal its changed!
        resetSeasons();
    }
}

void
LTMSidebar::editRange()
{
    // throw up modal dialog box to edit all the season
    if (dateRangeTree->selectedItems().count() != 1) return;

    int seasonIdx = -1;
    int phaseIdx = -1;

    QTreeWidgetItem* item = dateRangeTree->selectedItems().first();
    if (item->type() >= Season::season && item->type() < Phase::phase) {
        seasonIdx = allDateRanges->indexOfChild(item);
    } else if (item->type() >= Phase::phase) {
        seasonIdx = allDateRanges->indexOfChild(item->parent());
        phaseIdx = item->parent()->indexOfChild(item);
    }

    if (seasons->seasons[seasonIdx].getType() == Season::temporary) {
        QMessageBox::warning(this, tr("Edit Season"), tr("You can only edit user defined seasons. Please select a season you have created for editing."));
        return; // must be a user season
    }

    QDialog* dialog;
    if (phaseIdx> -1) {
        dialog = new EditPhaseDialog(context, &seasons->seasons[seasonIdx].phases[phaseIdx], seasons->seasons[seasonIdx]);
    } else {
        dialog = new EditSeasonDialog(context, &seasons->seasons[seasonIdx]);
    }

    if (dialog->exec()) {

        active = true;

        if (phaseIdx> -1) {
            // check dates are right way round...
            if (seasons->seasons[seasonIdx].phases[phaseIdx].getStart() > seasons->seasons[seasonIdx].phases[phaseIdx].getEnd()) {
                QDate temp = seasons->seasons[seasonIdx].phases[phaseIdx].getStart();
                seasons->seasons[seasonIdx].phases[phaseIdx].setAbsoluteStart(seasons->seasons[seasonIdx].phases[phaseIdx].getEnd());
                seasons->seasons[seasonIdx].phases[phaseIdx].setAbsoluteEnd(temp);
            }

            // update name
            dateRangeTree->selectedItems().first()->setText(0, seasons->seasons[seasonIdx].phases[phaseIdx].getName());
        } else {
            // check dates are right way round...
            if (seasons->seasons[seasonIdx].getStart() > seasons->seasons[seasonIdx].getEnd()) {
                QDate temp = seasons->seasons[seasonIdx].getStart();
                seasons->seasons[seasonIdx].setAbsoluteStart(seasons->seasons[seasonIdx].getEnd());
                seasons->seasons[seasonIdx].setAbsoluteEnd(temp);
            }

            // update name
            dateRangeTree->selectedItems().first()->setText(0, seasons->seasons[seasonIdx].getName());
        }

        // save changes away
        seasons->writeSeasons();
        active = false;

    }
}

void
LTMSidebar::deleteRange()
{
    if (dateRangeTree->selectedItems().count() != 1) return;

    // what is selected?
    QTreeWidgetItem *selected = dateRangeTree->selectedItems().first();
    int index = allDateRanges->indexOfChild(selected);

    // its a phase, not a season
    if (index == -1) {

        // deleting a phase, selected item is not a child of root
        // so lets get parent (we know .first() returns non-null).
        index = allDateRanges->indexOfChild(selected->parent());
        if (index >= 0) {

            // delete the nth phase !
            // XXX there is no method to do this, so FOR NOW we
            // XXX will implement here, the season/phase code needs
            // XXX to be cleaned up for general use in planning anyway
            int phase = selected->parent()->indexOfChild(selected);

            delete selected->parent()->takeChild(phase);
            seasons->seasons[index].phases.removeAt(phase);

            // save change
            seasons->writeSeasons();
        }
        return;
    }

    // if we get here its a season
    if (seasons->seasons[index].getType() == Season::temporary) {
        QMessageBox::warning(this, tr("Delete Season"), tr("You can only delete user defined seasons. Please select a season you have created for deletion."));
        return; // must be a user season
    }

    // now delete!
    delete allDateRanges->takeChild(index);
    seasons->deleteSeason(index);
}

void
LTMSidebar::addEvent()
{
    if (dateRangeTree->selectedItems().count() == 0) {
        QMessageBox::warning(this, tr("Add Event"), tr("You can only add events to user defined seasons. Please select a season you have created before adding an event."));
        return; // need a season selected!
    }

    // if a phase is selected (rather than a season), get the season this phase belongs to
    QTreeWidgetItem *selectedDateRange = dateRangeTree->selectedItems().first();
    if (selectedDateRange->parent() != NULL) {
        selectedDateRange = selectedDateRange->parent();
    }

    int seasonindex = allDateRanges->indexOfChild(selectedDateRange);

    if (seasonindex == -1 || seasons->seasons[seasonindex].getType() == Season::temporary) {
        QMessageBox::warning(this, tr("Add Event"), tr("You can only add events to user defined seasons. Please select a season you have created before adding an event."));
        return; // must be a user season
    }

    SeasonEvent myevent("", seasons->seasons[seasonindex].getEnd());
    EditSeasonEventDialog dialog(context, &myevent, seasons->seasons[seasonindex]);

    if (dialog.exec()) {

        active = true;

#ifdef GC_HAVE_ICAL
        // upload to remote calendar if configured
        if (context->athlete->davCalendar->getConfig())
            if (!context->athlete->davCalendar->upload(&myevent))
                QMessageBox::warning(this, tr("Add Event"), tr("The new event could not be uploaded to your remote calendar."));
#endif

        seasons->seasons[seasonindex].events.append(myevent);

        QTreeWidgetItem *add = new QTreeWidgetItem(allEvents);
        add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
        add->setText(0, myevent.name);
        add->setText(1, myevent.date.toString("MMM d, yyyy"));
        add->setText(2, SeasonEvent::priorityList().at(myevent.priority));

        // make sure they fit
        eventTree->header()->resizeSections(QHeaderView::ResizeToContents);

        // save changes away
        seasons->writeSeasons();
        active = false;
    }
}

void
LTMSidebar::deleteEvent()
{
    active = true;

    if (dateRangeTree->selectedItems().count()) {

        // if a phase is selected (rather than a season), get the season this phase belongs to
        QTreeWidgetItem *selectedDateRange = dateRangeTree->selectedItems().first();
        if (selectedDateRange->parent() != NULL) {
            selectedDateRange = selectedDateRange->parent();
        }

        int seasonindex = allDateRanges->indexOfChild(selectedDateRange);

        // only delete those that are selected
        if (eventTree->selectedItems().count() > 0) {

            // wipe them away
            foreach(QTreeWidgetItem *d, eventTree->selectedItems()) {
                int index = allEvents->indexOfChild(d);

                delete allEvents->takeChild(index);
                seasons->seasons[seasonindex].events.removeAt(index);
            }
        }

        // save changes away
        seasons->writeSeasons();

    }
    active = false;
}

void
LTMSidebar::editEvent()
{
    active = true;

    if (dateRangeTree->selectedItems().count()) {

        // if a phase is selected (rather than a season), get the season this phase belongs to
        QTreeWidgetItem *selectedDateRange = dateRangeTree->selectedItems().first();
        if (selectedDateRange->parent() != NULL) {
            selectedDateRange = selectedDateRange->parent();
        }

        int seasonindex = allDateRanges->indexOfChild(selectedDateRange);

        // only delete those that are selected
        if (eventTree->selectedItems().count() == 1) {

            QTreeWidgetItem *ours = eventTree->selectedItems().first();
            int index = allEvents->indexOfChild(ours);

            EditSeasonEventDialog dialog(context, &seasons->seasons[seasonindex].events[index], seasons->seasons[seasonindex]);

            if (dialog.exec()) {

                // update event data
                ours->setText(0, seasons->seasons[seasonindex].events[index].name);
                ours->setText(1, seasons->seasons[seasonindex].events[index].date.toString("MMM d, yyyy"));
                ours->setText(2, SeasonEvent::priorityList().at(seasons->seasons[seasonindex].events[index].priority));

                // make sure they fit
                eventTree->header()->resizeSections(QHeaderView::ResizeToContents);

                // save changes away
                seasons->writeSeasons();
            }
        }
    }
    active = false;
}

void
LTMSidebar::eventMoved(QTreeWidgetItem*item, int oldposition, int newposition)
{
    active = true;

    if (dateRangeTree->selectedItems().count()) {

        // if a phase is selected (rather than a season), get the season this phase belongs to
        QTreeWidgetItem *selectedDateRange = dateRangeTree->selectedItems().first();
        if (selectedDateRange->parent() != NULL) {
            selectedDateRange = selectedDateRange->parent();
        }

        int seasonindex = allDateRanges->indexOfChild(selectedDateRange);

        // report the move in the seasons
        seasons->seasons[seasonindex].events.move(oldposition, newposition);

        // save changes away
        seasons->writeSeasons();

        // deselect actual selection
        eventTree->selectedItems().first()->setSelected(false);
        // select the move/drop item
        item->setSelected(true);
    }

    active = false;
}

void
LTMSidebar::addPhase()
{
    if (dateRangeTree->selectedItems().count() == 0) {
        QMessageBox::warning(this, tr("Add Phase"), tr("You can only add phases to user defined seasons. Please select a season you have created before adding a phase."));
        return; // need a season selected!
    }

    // if a phase is selected (rather than a season), get the season this phase belongs to
    QTreeWidgetItem *selectedDateRange = dateRangeTree->selectedItems().first();
    if (selectedDateRange->parent() != NULL) {
        selectedDateRange = selectedDateRange->parent();
    }

    int seasonindex = allDateRanges->indexOfChild(selectedDateRange);

    if (seasonindex == -1 || seasons->seasons[seasonindex].getType() == Season::temporary) {
        QMessageBox::warning(this, tr("Add Phase"), tr("You can only add phases to user defined seasons. Please select a season you have created before adding a phase."));
        return; // must be a user season
    }

    Phase myphase("", seasons->seasons[seasonindex].getStart(), seasons->seasons[seasonindex].getEnd());
    EditPhaseDialog dialog(context, &myphase, seasons->seasons[seasonindex]);

    if (dialog.exec()) {

        active = true;
        seasons->seasons[seasonindex].phases.append(myphase);

        QTreeWidgetItem *addPhase = new QTreeWidgetItem(selectedDateRange, myphase.getType());
        addPhase->setSelected(true);
        addPhase->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
        addPhase->setText(0, myphase.getName());

        // save changes away
        seasons->writeSeasons();
        active = false;
    }
}

//
// Preset chart functions
//
void
LTMSidebar::presetPopup(QPoint)
{
}

void
LTMSidebar::presetPopup()
{
    // item points to selection, if it exists
    QTreeWidgetItem *item = chartTree->selectedItems().count() ? chartTree->selectedItems().at(0) : NULL;

    // OK - we are working with a specific event..
    QMenu menu(chartTree);
    QAction *add = new QAction(tr("Add Chart"), chartTree);
    menu.addAction(add);
    connect(add, SIGNAL(triggered(void)), this, SLOT(addPreset(void)));

    if (item != NULL) {

        if (chartTree->selectedItems().count() == 1) {
            QAction *edit = new QAction(tr("Edit Chart"), chartTree);
            QAction *del = new QAction(tr("Delete Chart"), chartTree);

            menu.addAction(edit);
            menu.addAction(del);

            connect(edit, SIGNAL(triggered(void)), this, SLOT(editPreset(void)));
            connect(del, SIGNAL(triggered(void)), this, SLOT(deletePreset(void)));

        } else {
            QAction *del = new QAction(tr("Delete Selected Charts"), chartTree);
            menu.addAction(del);
            connect(del, SIGNAL(triggered(void)), this, SLOT(deletePreset(void)));
        }

        menu.addSeparator();
        QAction *exp = NULL;
        if (chartTree->selectedItems().count() == 1) {
            exp = new QAction(tr("Export Chart"), chartTree);
        } else {
            exp = new QAction(tr("Export Selected Charts"), chartTree);
        }
        // connect menu to functions
        menu.addAction(exp);
        connect(exp, SIGNAL(triggered(void)), this, SLOT(exportPreset(void)));

    } else {

        // for the import/reset options
        menu.addSeparator();
    }

    QAction *import = new QAction(tr("Import Charts"), chartTree);
    menu.addAction(import);
    connect(import, SIGNAL(triggered(void)), this, SLOT(importPreset(void)));

    // this one needs to be in a corner away from the crowd
    menu.addSeparator();

    QAction *reset = new QAction(tr("Reset to default"), chartTree);
    menu.addAction(reset);
    connect(reset, SIGNAL(triggered(void)), this, SLOT(resetPreset(void)));

    // execute the menu
    menu.exec(splitter->mapToGlobal(QPoint(chartsWidget->pos().x()+chartsWidget->width()-20,
                                           chartsWidget->pos().y())));
}

void
LTMSidebar::presetMoved(QTreeWidgetItem *, int, int)
{
}

void
LTMSidebar::addPreset()
{
    GcChartWindow *newone = NULL;

    // GcWindowDialog is delete on close, so no need to delete
    GcWindowDialog *f = new GcWindowDialog(GcWindowTypes::LTM, context, &newone, true);
    f->exec();

    // returns null if cancelled or closed
    if (newone) {
        // append to the chart list ...
        LTMSettings set = static_cast<LTMWindow*>(newone)->getSettings();
        set.name = set.title = newone->property("title").toString();
        context->athlete->presets.append(set);

        // newone
        newone->close();
        newone->deleteLater();

        // now wipe it
        f->hide();
        //XXX memory leak XXX causes crash on QT5.8 XXX f->deleteLater();

        // tell the world
        context->notifyPresetsChanged();
    }
}

void
LTMSidebar::editPreset()
{
    GcChartWindow *newone = NULL;

    int index = allCharts->indexOfChild(chartTree->selectedItems()[0]);

    // clear bests, it won't be there any more.
    context->athlete->presets[index].bests = NULL;

    // GcWindowDialog is delete on close, so no need to delete
    GcWindowDialog *f = new GcWindowDialog(GcWindowTypes::LTM, context, &newone, true, &context->athlete->presets[index]);
    f->exec();

    // returns null if cancelled or closed
    if (newone) {

        // append to the chart list ...
        LTMSettings set = static_cast<LTMWindow*>(newone)->getSettings();
        set.name = set.title = newone->property("title").toString();
        context->athlete->presets[index] = set;

        // newone
        newone->close();
        newone->deleteLater();

        // now wipe it
        f->hide();
        //XXX memory leak XXX causes crash on QT5.8 XXX f->deleteLater();

        // tell the world
        context->notifyPresetsChanged();
    }
}

void
LTMSidebar::deletePreset()
{
    // zap all selected
    for(int index=allCharts->childCount()-1; index>0; index--) 
        if (allCharts->child(index)->isSelected())
            context->athlete->presets.removeAt(index);

    context->notifyPresetsChanged();
}

void
LTMSidebar::exportPreset()
{
    // get a filename to export to...
    QString filename = QFileDialog::getSaveFileName(this, tr("Export Charts"), QDir::homePath() + "/charts.xml", tr("Chart File (*.xml)"));

    // nothing given
    if (filename.length() == 0) return;

    // get a list
    QList<LTMSettings> these;

    for(int index=0; index<allCharts->childCount(); index++)
        if (allCharts->child(index)->isSelected())
            these << context->athlete->presets[index];

    LTMChartParser::serialize(filename, these);
}

void
LTMSidebar::importPreset()
{
    QFileDialog existing(this);
    existing.setFileMode(QFileDialog::ExistingFile);
    existing.setNameFilter(tr("Chart File (*.xml)"));

    if (existing.exec()){

        // we will only get one (ExistingFile not ExistingFiles)
        QStringList filenames = existing.selectedFiles();

        if (QFileInfo(filenames[0]).exists()) {

            QList<LTMSettings> imported;
            QFile chartsFile(filenames[0]);

            // setup XML processor
            QXmlInputSource source( &chartsFile );
            QXmlSimpleReader xmlReader;
            LTMChartParser handler;
            xmlReader.setContentHandler(&handler);
            xmlReader.setErrorHandler(&handler);

            // parse and get return values
            xmlReader.parse(source);
            imported = handler.getSettings();

            // now append to the QList and QTreeWidget
            context->athlete->presets += imported;

            // notify we changed and tree updates
            context->notifyPresetsChanged();

        } else {
            // oops non existent - does this ever happen?
            QMessageBox::warning( 0, tr("Entry Error"), QString(tr("Selected file (%1) does not exist")).arg(filenames[0]));
        }
    }
}


void
LTMSidebar::resetPreset()
{
    // confirm user is committed to this !
    QMessageBox msgBox;
    msgBox.setText(tr("You are about to reset the chart sidebar to the default setup"));
    msgBox.setInformativeText(tr("Do you want to continue?"));
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.exec();

    if(msgBox.clickedButton() != msgBox.button(QMessageBox::Ok)) return;

    // for getting config
    QNetworkAccessManager nam;
    QString content;

    // remove the current saved version
    QFile::remove(context->athlete->home->config().canonicalPath() + "/charts.xml");

    // fetch from the goldencheetah.org website
    QString request = QString("%1/charts.xml").arg(VERSION_CONFIG_PREFIX);
    QNetworkReply *reply = nam.get(QNetworkRequest(QUrl(request)));

    // request submitted ok
    if (reply->error() == QNetworkReply::NoError) {

        // lets wait for a response with an event loop
        // it quits on a 5s timeout or reply coming back
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
        connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        timer.start(5000);

        // lets block until signal received
        loop.exec(QEventLoop::WaitForMoreEvents);

        // all good?
        if (reply->error() == QNetworkReply::NoError) {
            content = reply->readAll();
        }
    }

    //  if we don't have content read from resource
    if (content == "") {

        QFile file(":xml/charts.xml");
        if (file.open(QIODevice::ReadOnly)) {
            content = file.readAll();
            file.close();
        }
    }

    // still nowt? give up.
    if (content == "") return;

    // we should have content now !
    QFile chartsxml(context->athlete->home->config().canonicalPath() + "/charts.xml");
    chartsxml.open(QIODevice::WriteOnly);
    QTextStream out(&chartsxml);
    out << content;
    chartsxml.close();

    // following 2 lines could be managed by athlete, but its just a container really
    // load and tell the world to reset
    context->athlete->loadCharts();
    context->notifyPresetsChanged(); 
}
