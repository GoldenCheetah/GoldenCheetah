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
#include "Settings.h"
#include "Units.h"

#include <QApplication>
#include <QWebView>
#include <QWebFrame>
#include <QScrollBar>
#include <QtGui>

// seasons support
#include "Season.h"
#include "SeasonParser.h"
#include <QXmlInputSource>
#include <QXmlSimpleReader>

// named searchs
#include "NamedSearch.h"
#include "DataFilter.h"
#ifdef GC_HAVE_LUCENE
#include "Lucene.h"
#endif

// metadata support
#include "RideMetadata.h"
#include "SpecialFields.h"

#include "MetricAggregator.h"
#include "SummaryMetrics.h"

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
    dateRangeTree->setIndentation(5);
    dateRangeTree->expandItem(allDateRanges);
    dateRangeTree->setContextMenuPolicy(Qt::CustomContextMenu);
#ifdef Q_OS_MAC
    dateRangeTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    seasonsWidget->addWidget(dateRangeTree);

    // events
    eventsWidget = new GcSplitterItem(tr("Events"), iconFromPNG(":images/sidebar/bookmark.png"), this);
    QAction *moreEventAct = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    eventsWidget->addAction(moreEventAct);
    connect(moreEventAct, SIGNAL(triggered(void)), this, SLOT(eventPopup(void)));

    eventTree = new QTreeWidget;
    eventTree->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    allEvents = eventTree->invisibleRootItem();
    allEvents->setText(0, tr("Events"));
    eventTree->setFrameStyle(QFrame::NoFrame);
    eventTree->setColumnCount(2);
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
    eventsWidget->addWidget(eventTree);

    // filters
#ifdef GC_HAVE_LUCENE
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
    // we cast the filter tree and this because we use the same constructor XXX fix this!!!
    filterSplitter = new GcSubSplitter(Qt::Vertical, (GcSplitterControl*)filterTree, (GcSplitter*)this, true);
    filtersWidget->addWidget(filterSplitter);
#endif

    seasons = context->athlete->seasons;
    resetSeasons(); // reset the season list
    resetFilters(); // reset the filters list

    autoFilterMenu = new QMenu(tr("Autofilter"),this);
    configChanged(); // will reset the metric tree and the autofilters

    splitter = new GcSplitter(Qt::Vertical);
    splitter->addWidget(seasonsWidget); // goes alongside events
    splitter->addWidget(eventsWidget); // goes alongside date ranges
#ifdef GC_HAVE_LUCENE
    splitter->addWidget(filtersWidget);
#endif

    GcSplitterItem *summaryWidget = new GcSplitterItem(tr("Summary"), iconFromPNG(":images/sidebar/dashboard.png"), this);

    summary = new QWebView(this);
    summary->setContentsMargins(0,0,0,0);
    summary->page()->view()->setContentsMargins(0,0,0,0);
    summary->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
    summary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    summary->setAcceptDrops(false);

    summaryWidget->addWidget(summary);

    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply
    summary->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize());
    summary->settings()->setFontFamily(QWebSettings::StandardFont, defaultFont.family());
    splitter->addWidget(summaryWidget);

    mainLayout->addWidget(splitter);

    splitter->prepare(context->athlete->cyclist, "LTM");

    // our date ranges
    connect(dateRangeTree,SIGNAL(itemSelectionChanged()), this, SLOT(dateRangeTreeWidgetSelectionChanged()));
    connect(dateRangeTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(dateRangePopup(const QPoint &)));
    connect(dateRangeTree,SIGNAL(itemChanged(QTreeWidgetItem *,int)), this, SLOT(dateRangeChanged(QTreeWidgetItem*, int)));
    connect(dateRangeTree,SIGNAL(itemMoved(QTreeWidgetItem *,int, int)), this, SLOT(dateRangeMoved(QTreeWidgetItem*, int, int)));
#ifdef GC_HAVE_LUCENE
    connect(filterTree,SIGNAL(itemSelectionChanged()), this, SLOT(filterTreeWidgetSelectionChanged()));
#endif
    connect(eventTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(eventPopup(const QPoint &)));

    // GC signal
    connect(context->athlete->metricDB, SIGNAL(dataChanged()), this, SLOT(autoFilterRefresh()));
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(seasons, SIGNAL(seasonsChanged()), this, SLOT(resetSeasons()));
    connect(context->athlete, SIGNAL(namedSearchesChanged()), this, SLOT(resetFilters()));

    connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(setSummary(DateRange)));

    // let everyone know what date range we are starting with
    dateRangeTreeWidgetSelectionChanged();

}

void
LTMSidebar::configChanged()
{
    setAutoFilterMenu();

    // set or reset the autofilter widgets
    autoFilterChanged();

}

/*----------------------------------------------------------------------
 * Selections Made
 *----------------------------------------------------------------------*/

void
LTMSidebar::dateRangeTreeWidgetSelectionChanged()
{
    if (active == true) return;

    const Season *dateRange = NULL;

    if (dateRangeTree->selectedItems().isEmpty()) dateRange = NULL;
    else {
        QTreeWidgetItem *which = dateRangeTree->selectedItems().first();
        if (which != allDateRanges) {
            dateRange = &seasons->seasons.at(allDateRanges->indexOfChild(which));
        } else {
            dateRange = NULL;
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
            add->setText(0, event.name);
            add->setText(1, event.date.toString("MMM d, yyyy"));
        }

        // make sure they fit
        eventTree->header()->resizeSections(QHeaderView::ResizeToContents);
        appsettings->setCValue(context->athlete->cyclist, GC_LTM_LAST_DATE_RANGE, dateRange->id().toString());

    }

    // Let the view know its changed....
    if (dateRange) emit dateRangeChanged(DateRange(dateRange->start, dateRange->end, dateRange->name));
    else emit dateRangeChanged(DateRange());

}

/*----------------------------------------------------------------------
 * Seasons stuff
 *--------------------------------------------------------------------*/

void
LTMSidebar::resetSeasons()
{
    if (active == true) return;

    active = true;
    int i;
    for (i=allDateRanges->childCount(); i > 0; i--) {
        delete allDateRanges->takeChild(0);
    }
    QString id = appsettings->cvalue(context->athlete->cyclist, GC_LTM_LAST_DATE_RANGE, seasons->seasons.at(0).id().toString()).toString();
    for (i=0; i <seasons->seasons.count(); i++) {
        Season season = seasons->seasons.at(i);
        QTreeWidgetItem *add = new QTreeWidgetItem(allDateRanges, season.getType());
        if (season.id().toString()==id)
            add->setSelected(true);

        // Drag and Drop is FINE for temporary seasons -- IT IS JUST A DATE RANGE!
        add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
        add->setText(0, season.getName());
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
        int index = allDateRanges->indexOfChild(item);
        if (index == -1 || index >= seasons->seasons.count()
            || seasons->seasons[index].getType() == Season::temporary) {
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

        } else {

            // create context menu
            QMenu menu(dateRangeTree);
            QAction *add = new QAction(tr("Add season"), dateRangeTree);
            QAction *edit = new QAction(tr("Edit season"), dateRangeTree);
            QAction *del = new QAction(tr("Delete season"), dateRangeTree);
            QAction *event = new QAction(tr("Add Event"), dateRangeTree);
            menu.addAction(add);
            menu.addAction(edit);
            menu.addAction(del);
            menu.addAction(event);

            // connect menu to functions
            connect(add, SIGNAL(triggered(void)), this, SLOT(addRange(void)));
            connect(edit, SIGNAL(triggered(void)), this, SLOT(editRange(void)));
            connect(del, SIGNAL(triggered(void)), this, SLOT(deleteRange(void)));
            connect(event, SIGNAL(triggered(void)), this, SLOT(addEvent(void)));

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

    // OK - we are working with a specific event..
    QMenu menu(dateRangeTree);
    QAction *add = new QAction(tr("Add season"), dateRangeTree);
    menu.addAction(add);
    connect(add, SIGNAL(triggered(void)), this, SLOT(addRange(void)));

    if (item != NULL && allDateRanges->indexOfChild(item) != -1) {
        QAction *edit = new QAction(tr("Edit season"), dateRangeTree);
        QAction *del = new QAction(tr("Delete season"), dateRangeTree);
        QAction *event = new QAction(tr("Add Event"), dateRangeTree);

        menu.addAction(edit);
        menu.addAction(del);
        menu.addAction(event);

        // connect menu to functions

        connect(edit, SIGNAL(triggered(void)), this, SLOT(editRange(void)));
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteRange(void)));
        connect(event, SIGNAL(triggered(void)), this, SLOT(addEvent(void)));
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

    // and the season must be user defined not temporary
    int seasonindex = allDateRanges->indexOfChild(dateRangeTree->selectedItems().first());
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
#ifdef GC_HAVE_LUCENE
    EditNamedSearches *editor = new EditNamedSearches(this, context);
    editor->move(QCursor::pos()+QPoint(10,-200));
    editor->show();
#endif
}

void
LTMSidebar::setAutoFilterMenu()
{
#ifdef GC_HAVE_LUCENE
    QStringList on = appsettings->cvalue(context->athlete->cyclist, GC_LTM_AUTOFILTERS, tr("Workout Code|Sport")).toString().split("|");
    autoFilterMenu->clear();
    autoFilterState.clear();

    foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {

        if (field.tab != "" && (field.type == 0 || field.type == 2)) { // we only do text or shorttext fields

            QAction *action = new QAction(field.name, this);
            action->setCheckable(true);

            if (on.contains(field.name)) action->setChecked(true);
            connect(action, SIGNAL(triggered()), this, SLOT(autoFilterChanged()));

            autoFilterMenu->addAction(action);
            autoFilterState << false;
        }
    }
#endif
}

void 
LTMSidebar::autoFilterChanged()
{

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
            item->addWidget(tree);
            filterSplitter->addWidget(item);

            // update the values available in the tree
            foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
                if (field.name == action->text()) {
                    foreach (QString value, context->athlete->metricDB->db()->getDistinctValues(field)) {
                        if (value == "") value = "(blank)";
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
#ifdef GC_HAVE_LUCENE
    int selected = filterTree->selectedItems().count();

    if (selected) {

        QStringList errors, files; // results of all the selections
        bool first = true;
        int index=0;

        foreach (QTreeWidgetItem *item, filterTree->selectedItems()) {

            int index = filterTree->invisibleRootItem()->indexOfChild(item);

            NamedSearch ns = context->athlete->namedSearches->get(index);
            QStringList errors, results;

            switch(ns.type) {

            case NamedSearch::filter :
                {
                    // use a data filter
                    DataFilter f(this, context);
                    errors = f.parseFilter(ns.text, &results);
                }
                break;

            case NamedSearch::search :
                {
                    // use clucence
                    Lucene s(this, context);
                    results = s.search(ns.text);
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
#endif
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
            QStringList merged = autoFilterFiles.toSet().intersect(queryFilterFiles.toSet()).toList();
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

        // what is the field?
        QString fieldname = item->splitterHandle->title();

            // update the values available in the tree
        foreach(FieldDefinition field, context->athlete->rideMetadata()->getFields()) {
            if (field.name == fieldname) {
                foreach (QString value, context->athlete->metricDB->db()->getDistinctValues(field)) {
                    if (value == "") value = "(blank)";
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
    QList<SummaryMetrics> allRides;
    QSet<QString> matched;

    // assume nothing to do...
    isautofilter = false;

    // are any auto filters applied?
    for (int i=1; i<filterSplitter->count(); i++) {

        GcSplitterItem *item = static_cast<GcSplitterItem*>(filterSplitter->widget(i));
        QTreeWidget *tree = static_cast<QTreeWidget*>(item->content);

        // are some values selected?
        if (tree->selectedItems().count() > 0) {

            // we have a selection!
            if (isautofilter == false) {
                isautofilter = true;
                allRides = context->athlete->metricDB->getAllMetricsFor(QDateTime(), QDateTime());
                foreach(SummaryMetrics x, allRides) matched << x.getFileName();
            }

            // what is the field?
            QString fieldname = item->splitterHandle->title();

            // what values are highlighted
            QStringList values;
            foreach (QTreeWidgetItem *wi, tree->selectedItems()) values << wi->text(0);

            // get a set of filenames that match
            QSet<QString> matches;
            foreach(SummaryMetrics x, allRides) {

                // we use XXX___XXX___XXX because it is not likely to exist
                QString value = x.getText(fieldname, "XXX___XXX___XXX");
                if (value == "") value = "(blank)"; // match blanks!

                if (values.contains(value)) matches << x.getFileName();
            }

            // now remove items from the matched list that
            // are not in my list of matches
            matched = matched.intersect(matches);
        }
    }

    // all done
    autoFilterFiles = matched.toList();

    // tell the world
    filterNotify();
}

void
LTMSidebar::resetFilters()
{
#ifdef GC_HAVE_LUCENE
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
#endif
}

void
LTMSidebar::filterPopup()
{
#ifdef GC_HAVE_LUCENE
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
#endif
}

void
LTMSidebar::deleteFilter()
{
#ifdef GC_HAVE_LUCENE
    if (filterTree->selectedItems().count() <= 0) return;

    active = true; // no need to reset tree when items deleted from model!
    while (filterTree->selectedItems().count()) {
        int index = allFilters->indexOfChild(filterTree->selectedItems().first());

        // now delete!
        delete allFilters->takeChild(index);
        context->athlete->namedSearches->deleteNamedSearch(index);
    }
    active = false;
#endif
}

void
LTMSidebar::dateRangeChanged(QTreeWidgetItem*item, int)
{
    if (active == true) return;

    int index = allDateRanges->indexOfChild(item);
    seasons->seasons[index].setName(item->text(0));

    // save changes away
    active = true;
    seasons->writeSeasons();
    active = false;

    // signal date selected changed
    //dateRangeSelected(&seasons->seasons[index]);
}

void
LTMSidebar::dateRangeMoved(QTreeWidgetItem*item, int oldposition, int newposition)
{
    // no drop in the temporary seasons
    if (newposition>allDateRanges->childCount()-12) {
        newposition = allDateRanges->childCount()-12;
        allDateRanges->removeChild(item);
        allDateRanges->insertChild(newposition, item);
    }

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

    int index = allDateRanges->indexOfChild(dateRangeTree->selectedItems().first());
    EditSeasonDialog dialog(context, &seasons->seasons[index]);

    if (dialog.exec()) {

        active = true;

        // update name
        dateRangeTree->selectedItems().first()->setText(0, seasons->seasons[index].getName());

        // save changes away
        seasons->writeSeasons();
        active = false;

    }
}

void
LTMSidebar::deleteRange()
{
    if (dateRangeTree->selectedItems().count() != 1) return;
    int index = allDateRanges->indexOfChild(dateRangeTree->selectedItems().first());

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

    int seasonindex = allDateRanges->indexOfChild(dateRangeTree->selectedItems().first());

    if (seasons->seasons[seasonindex].getType() == Season::temporary) {
        QMessageBox::warning(this, tr("Add Event"), tr("You can only add events to user defined seasons. Please select a season you have created before adding an event."));
        return; // must be a user season
    }

    SeasonEvent myevent("", QDate());
    EditSeasonEventDialog dialog(context, &myevent);

    if (dialog.exec()) {

        active = true;
        seasons->seasons[seasonindex].events.append(myevent);

        QTreeWidgetItem *add = new QTreeWidgetItem(allEvents);
        add->setText(0, myevent.name);
        add->setText(1, myevent.date.toString("MMM d, yyyy"));

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

        int seasonindex = allDateRanges->indexOfChild(dateRangeTree->selectedItems().first());

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

        int seasonindex = allDateRanges->indexOfChild(dateRangeTree->selectedItems().first());

        // only delete those that are selected
        if (eventTree->selectedItems().count() == 1) {

            QTreeWidgetItem *ours = eventTree->selectedItems().first();
            int index = allEvents->indexOfChild(ours);

            EditSeasonEventDialog dialog(context, &seasons->seasons[seasonindex].events[index]);

            if (dialog.exec()) {

                // update name
                ours->setText(0, seasons->seasons[seasonindex].events[index].name);
                ours->setText(1, seasons->seasons[seasonindex].events[index].date.toString("MMM d, yyyy"));

                // save changes away
                seasons->writeSeasons();
            }
        }
    }
    active = false;
}

void
LTMSidebar::setSummary(DateRange dateRange)
{
    // where we construct the text
    QString summaryText("");

    // main totals
    static const QStringList totalColumn = QStringList()
        << "workout_time"
        << "time_riding"
        << "total_distance"
        << "total_work"
        << "elevation_gain";

    static const QStringList averageColumn = QStringList()
        << "average_speed"
        << "average_power"
        << "average_hr"
        << "average_cad";

    static const QStringList maximumColumn = QStringList()
        << "max_speed"
        << "max_power"
        << "max_heartrate"
        << "max_cadence";

    // user defined
    QString s = appsettings->value(this, GC_SETTINGS_SUMMARY_METRICS, GC_SETTINGS_SUMMARY_METRICS_DEFAULT).toString();

    // in case they were set tand then unset
    if (s == "") s = GC_SETTINGS_SUMMARY_METRICS_DEFAULT;
    QStringList metricColumn = s.split(",");

    // what date range should we use?
    QDate newFrom = dateRange.from;
    QDate newTo = dateRange.to;

    if (newFrom == from && newTo == to) return;
    else {

        // date range changed lets refresh
        from = newFrom;
        to = newTo;

        // lets get the metrics
        QList<SummaryMetrics>results = context->athlete->metricDB->getAllMetricsFor(QDateTime(from,QTime(0,0,0)), QDateTime(to, QTime(24,59,59)));

        // foreach of the metrics get an aggregated value
        // header of summary
        summaryText = QString("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2//EN\">"
                              "<html>"
                              "<head>"
                              "<title></title>"
                              "</head>"
                              "<body>"
                              "<center>");

        for (int i=0; i<4; i++) {

            QString aggname;
            QStringList list;

            switch(i) {
                case 0 : // Totals
                    aggname = tr("Totals");
                    list = totalColumn;
                    break;

                case 1 :  // Averages
                    aggname = tr("Averages");
                    list = averageColumn;
                    break;

                case 3 :  // Maximums
                    aggname = tr("Maximums");
                    list = maximumColumn;
                    break;

                case 2 :  // User defined.. 
                    aggname = tr("Metrics");
                    list = metricColumn;
                    break;

            }

            summaryText += QString("<p><table width=\"85%\">"
                                   "<tr>"
                                   "<td align=\"center\" colspan=\"2\">"
                                   "<b>%1</b>"
                                   "</td>"
                                   "</tr>").arg(aggname);

            foreach(QString metricname, list) {

                const RideMetric *metric = RideMetricFactory::instance().rideMetric(metricname);

                QStringList empty; // filter list not used at present
                QString value = SummaryMetrics::getAggregated(context, metricname, results, empty, false, context->athlete->useMetricUnits);

                // Maximum Max and Average Average looks nasty, remove from name for display
                QString s = metric ? metric->name().replace(QRegExp(tr("^(Average|Max) ")), "") : "unknown";

                // don't show units for time values
                if (metric && (metric->units(context->athlete->useMetricUnits) == "seconds" ||
                               metric->units(context->athlete->useMetricUnits) == tr("seconds") ||
                               metric->units(context->athlete->useMetricUnits) == "")) {

                    summaryText += QString("<tr><td>%1:</td><td align=\"right\"> %2</td>")
                                            .arg(s)
                                            .arg(value);

                } else {
                    summaryText += QString("<tr><td>%1(%2):</td><td align=\"right\"> %3</td>")
                                            .arg(s)
                                            .arg(metric ? metric->units(context->athlete->useMetricUnits) : "unknown")
                                            .arg(value);
                }
            }
            summaryText += QString("</tr>" "</table>");

        }

        // finish off the html document
        summaryText += QString("</center>"
                               "</body>"
                               "</html>");

        // set webview contents
        summary->page()->mainFrame()->setHtml(summaryText);

    }
}
