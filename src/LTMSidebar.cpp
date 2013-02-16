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
#include "Settings.h"
#include "Units.h"
#include <assert.h>
#include <QApplication>
#include <QWebView>
#include <QWebFrame>
#include <QtGui>

// seasons support
#include "Season.h"
#include "SeasonParser.h"
#include <QXmlInputSource>
#include <QXmlSimpleReader>

// metadata support
#include "RideMetadata.h"
#include "SpecialFields.h"

#include "MetricAggregator.h"
#include "SummaryMetrics.h"

LTMSidebar::LTMSidebar(MainWindow *parent, const QDir &home) : QWidget(parent), home(home), main(parent), active(false)
{
    setStyleSheet("QFrame { FrameStyle = QFrame::NoFrame };"
                  "QWidget { background = Qt::white; border:0 px; margin: 2px; };");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);

    seasonsWidget = new GcSideBarItem(tr("Date Ranges"), this);

    QAction *addSeasonAct = new QAction(tr("+"), this);
    seasonsWidget->addAction(addSeasonAct);
    connect(addSeasonAct, SIGNAL(triggered(void)), this, SLOT(addRange(void)));
    QAction *removeSeasonAct = new QAction(tr("-"), this);
    seasonsWidget->addAction(removeSeasonAct);
    connect(removeSeasonAct, SIGNAL(triggered(void)), this, SLOT(deleteRange(void)));
    QAction *moreSeasonAct = new QAction(tr(">>"), this);
    seasonsWidget->addAction(moreSeasonAct);
    connect(moreSeasonAct, SIGNAL(triggered(void)), this, SLOT(dateRangePopup(void)));

    dateRangeTree = new SeasonTreeView;
    //allDateRanges = new QTreeWidgetItem(dateRangeTree, ROOT_TYPE);
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


    eventsWidget = new GcSideBarItem(tr("Events"), this);

    QAction *addEventAct = new QAction(tr("+"), this);
    eventsWidget->addAction(addEventAct);
    QAction *removeEventAct = new QAction(tr("-"), this);
    eventsWidget->addAction(removeEventAct);
    QAction *moreEventAct = new QAction(tr(">>"), this);
    eventsWidget->addAction(moreSeasonAct);
    connect(moreEventAct, SIGNAL(triggered(void)), this, SLOT(eventPopup(void)));



    eventTree = new QTreeWidget;
    //allEvents = new QTreeWidgetItem(eventTree, ROOT_TYPE);
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

    seasons = parent->seasons;
    resetSeasons(); // reset the season list

    configChanged(); // will reset the metric tree

    splitter = new QSplitter;
    splitter->setHandleWidth(1);
    splitter->setFrameStyle(QFrame::NoFrame);
    splitter->setContentsMargins(0,0,0,0);
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(seasonsWidget);
    splitter->addWidget(eventsWidget);
    connect(splitter,SIGNAL(splitterMoved(int,int)), this, SLOT(splitterMoved(int,int)));

    GcSideBarItem *summaryWidget = new GcSideBarItem(tr("Sumary"), this);

    summary = new QWebView(this);
    summary->setContentsMargins(0,0,0,0);
    summary->page()->view()->setContentsMargins(0,0,0,0);
    summary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    summary->setAcceptDrops(false);

    summaryWidget->addWidget(summary);

    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply
    summary->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize());
    summary->settings()->setFontFamily(QWebSettings::StandardFont, defaultFont.family());
    splitter->addWidget(summaryWidget);

    mainLayout->addWidget(splitter);

    // restore splitter
    QVariant splitterSizes = appsettings->cvalue(main->cyclist, GC_SETTINGS_LTMSPLITTER_SIZES); 
    if (splitterSizes != QVariant()) {
        splitter->restoreState(splitterSizes.toByteArray());
        splitter->setOpaqueResize(true); // redraw when released, snappier UI
    } else {
        QList<int> sizes;
        sizes.append(400);
        sizes.append(400);
        splitter->setSizes(sizes);
    }

    // our date ranges
    connect(dateRangeTree,SIGNAL(itemSelectionChanged()), this, SLOT(dateRangeTreeWidgetSelectionChanged()));
    connect(dateRangeTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(dateRangePopup(const QPoint &)));
    connect(dateRangeTree,SIGNAL(itemChanged(QTreeWidgetItem *,int)), this, SLOT(dateRangeChanged(QTreeWidgetItem*, int)));
    connect(dateRangeTree,SIGNAL(itemMoved(QTreeWidgetItem *,int, int)), this, SLOT(dateRangeMoved(QTreeWidgetItem*, int, int)));
    connect(eventTree,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(eventPopup(const QPoint &)));
    // GC signal
    connect(main, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(seasons, SIGNAL(seasonsChanged()), this, SLOT(resetSeasons()));

    connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(setSummary(DateRange)));

    // let everyone know what date range we are starting with
    dateRangeTreeWidgetSelectionChanged();

}

void
LTMSidebar::configChanged()
{
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

    QString now;

    // remember currebt
    if (dateRangeTree->selectedItems().count())
        now = dateRangeTree->selectedItems().first()->text(0);

    active = true;
    int i;
    for (i=allDateRanges->childCount(); i > 0; i--) {
        delete allDateRanges->takeChild(0);
    }
    for (i=0; i <seasons->seasons.count(); i++) {
        Season season = seasons->seasons.at(i);
        QTreeWidgetItem *add = new QTreeWidgetItem(allDateRanges, season.getType());
        // No Drag/Drop for temporary  Season
        if (season.getType() == Season::temporary)
            add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        else
            add->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled);
        add->setText(0, season.getName());
    }

    // get current back!
    if (now == "") allDateRanges->child(0)->setSelected(true); // just select first child
    active = false;
}

int
LTMSidebar::newSeason(QString name, QDate start, QDate end, int type)
{
    seasons->newSeason(name, start, end, type);

    QTreeWidgetItem *item = new QTreeWidgetItem(USER_DATE);
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
    if (item != NULL && item->type() != ROOT_TYPE && item->type() != SYS_DATE) {

        // out of bounds or not user defined
        int index = allDateRanges->indexOfChild(item);
        if (index == -1 || index >= seasons->seasons.count()
            || seasons->seasons[index].getType() == Season::temporary) {
            // on system date we just offer to add a Season, since its
            // the only way of doing it when no seasons are defined!!
            activeDateRange = NULL;

            // create context menu
            QMenu menu(dateRangeTree);
            QAction *add = new QAction(tr("Add season"), dateRangeTree);
            menu.addAction(add);

            // connect menu to functions
            connect(add, SIGNAL(triggered(void)), this, SLOT(addRange(void)));

            // execute the menu
            menu.exec(dateRangeTree->mapToGlobal(pos));

        } else {

            // save context
            activeDateRange = item;

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

    if (item != NULL && item->type() != ROOT_TYPE && allDateRanges->indexOfChild(item) != -1) {
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
    menu.exec(dateRangeTree->mapToGlobal(QPoint(dateRangeTree->pos().x()+dateRangeTree->width()-30,seasonsWidget->pos().y()-30)));
}

void
LTMSidebar::eventPopup(QPoint pos)
{
    // no current season selected
    if (dateRangeTree->selectedItems().isEmpty()) return;

    QTreeWidgetItem *item = eventTree->itemAt(pos);

    // save context - which season and event are we working with?
    QTreeWidgetItem *which = dateRangeTree->selectedItems().first();
    if (which && which != allDateRanges) {
        activeDateRange = which;
        activeEvent = item;
    } else return;

    // OK - we are working with a specific event..
    QMenu menu(eventTree);
    if (item != NULL && item->type() != ROOT_TYPE && allEvents->indexOfChild(item) != -1) {

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
    // no current season selected
    if (eventTree->selectedItems().isEmpty()) return;

    QTreeWidgetItem *item = eventTree->selectedItems().at(0);

    // OK - we are working with a specific event..
    QMenu menu(eventTree);
    if (item != NULL && item->type() != ROOT_TYPE && allEvents->indexOfChild(item) != -1) {

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
    menu.exec(eventTree->mapToGlobal(QPoint(eventTree->pos().x()+eventTree->width()-30,eventTree->pos().y()-30)));
}

void
LTMSidebar::dateRangeChanged(QTreeWidgetItem*item, int)
{
    if (item != activeDateRange || active == true) return;

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

    EditSeasonDialog dialog(main, &newOne);

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
    // fields.
    int index = allDateRanges->indexOfChild(activeDateRange);
    EditSeasonDialog dialog(main, &seasons->seasons[index]);

    if (dialog.exec()) {

        active = true;

        // update name
        activeDateRange->setText(0, seasons->seasons[index].getName());

        // save changes away
        seasons->writeSeasons();
        active = false;

        // signal its changed!
        //dateRangeSelected(&seasons->seasons[index]);
    }
}

void
LTMSidebar::deleteRange()
{
    // now delete!
    int index = allDateRanges->indexOfChild(activeDateRange);
    delete allDateRanges->takeChild(index);
    seasons->deleteSeason(index);
}

void
LTMSidebar::addEvent()
{
    SeasonEvent myevent("", QDate());
    int seasonindex = allDateRanges->indexOfChild(activeDateRange);
    EditSeasonEventDialog dialog(main, &myevent);

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

    // now delete!
    int seasonindex = allDateRanges->indexOfChild(activeDateRange);
    int eventindex = allEvents->indexOfChild(activeEvent);

    // wipe them away
    delete allEvents->takeChild(eventindex);
    seasons->seasons[seasonindex].events.removeAt(eventindex);

    // save changes away
    seasons->writeSeasons();

    active = false;
}

void
LTMSidebar::editEvent()
{
    int seasonindex = allDateRanges->indexOfChild(activeDateRange);
    int eventindex = allEvents->indexOfChild(activeEvent);

    EditSeasonEventDialog dialog(main, &seasons->seasons[seasonindex].events[eventindex]);

    if (dialog.exec()) {

        active = true;

        // update name
        activeEvent->setText(0, seasons->seasons[seasonindex].events[eventindex].name);
        activeEvent->setText(1, seasons->seasons[seasonindex].events[eventindex].date.toString("MMM d, yyyy"));

        // save changes away
        seasons->writeSeasons();
        active = false;

        // signal its changed!
        //dateRangeSelected(&seasons->seasons[index]);
    }
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
        QList<SummaryMetrics>results = main->metricDB->getAllMetricsFor(QDateTime(from,QTime(0,0,0)), QDateTime(to, QTime(24,59,59)));

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

                QString value = SummaryMetrics::getAggregated(metricname, results, main->useMetricUnits);

                // Maximum Max and Average Average looks nasty, remove from name for display
                QString s = metric ? metric->name().replace(QRegExp(tr("^(Average|Max) ")), "") : "unknown";

                // don't show units for time values
                if (metric && (metric->units(main->useMetricUnits) == "seconds" ||
                               metric->units(main->useMetricUnits) == tr("seconds") ||
                               metric->units(main->useMetricUnits) == "")) {

                    summaryText += QString("<tr><td>%1:</td><td align=\"right\"> %2</td>")
                                            .arg(s)
                                            .arg(value);

                } else {
                    summaryText += QString("<tr><td>%1(%2):</td><td align=\"right\"> %3</td>")
                                            .arg(s)
                                            .arg(metric ? metric->units(main->useMetricUnits) : "unknown")
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

void
LTMSidebar::splitterMoved(int, int)
{
    appsettings->setCValue(main->cyclist, GC_SETTINGS_LTMSPLITTER_SIZES, splitter->saveState());
}
