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

    dateRangeTree = new QTreeWidget;
    allDateRanges = new QTreeWidgetItem(dateRangeTree, ROOT_TYPE);
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

    seasons = parent->seasons;
    resetSeasons(); // reset the season list

    configChanged(); // will reset the metric tree

    splitter = new QSplitter;
    splitter->setHandleWidth(1);
    splitter->setFrameStyle(QFrame::NoFrame);
    splitter->setContentsMargins(0,0,0,0);
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(dateRangeTree);
    connect(splitter,SIGNAL(splitterMoved(int,int)), this, SLOT(splitterMoved(int,int)));

    summary = new QWebView(this);
    summary->setContentsMargins(0,0,0,0);
    summary->page()->view()->setContentsMargins(0,0,0,0);
    summary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    summary->setAcceptDrops(false);

    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply
    summary->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize());
    summary->settings()->setFontFamily(QWebSettings::StandardFont, defaultFont.family());
    splitter->addWidget(summary);

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

    // GC signal
    connect(main, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(seasons, SIGNAL(seasonsChanged()), this, SLOT(resetSeasons()));

    connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(setSummary(DateRange)));
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
        add->setText(0, season.getName());
    }

    // get current back!
    if (now != "") ; //XXX 
    else {
        allDateRanges->child(0)->setSelected(true); // just select first child
    }
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
            || seasons->seasons[index].getType() == Season::temporary)
            return;

        // save context
        activeDateRange = item;

        // create context menu
        QMenu menu(dateRangeTree);
        QAction *rename = new QAction(tr("Rename range"), dateRangeTree);
        QAction *edit = new QAction(tr("Edit details"), dateRangeTree);
        QAction *del = new QAction(tr("Delete range"), dateRangeTree);
        menu.addAction(rename);
        menu.addAction(edit);
        menu.addAction(del);

        // connect menu to functions
        connect(rename, SIGNAL(triggered(void)), this, SLOT(renameRange(void)));
        connect(edit, SIGNAL(triggered(void)), this, SLOT(editRange(void)));
        connect(del, SIGNAL(triggered(void)), this, SLOT(deleteRange(void)));

        // execute the menu
        menu.exec(dateRangeTree->mapToGlobal(pos));
    }
}

void
LTMSidebar::renameRange()
{
    // go edit the name
    activeDateRange->setFlags(activeDateRange->flags() | Qt::ItemIsEditable);
    dateRangeTree->editItem(activeDateRange, 0);
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
LTMSidebar::editRange()
{
    // throw up modal dialog box to edit all the season
    // fields.
    int index = allDateRanges->indexOfChild(activeDateRange);
    EditSeasonDialog dialog(main, &seasons->seasons[index]);

    if (dialog.exec()) {
        // update name
        activeDateRange->setText(0, seasons->seasons[index].getName());

        // save changes away
        active = true;
        seasons->writeSeasons();
        active = false;

        // signal its changed!
        //XXX dateRangeSelected(&seasons->seasons[index]);
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
LTMSidebar::setSummary(DateRange dateRange)
{
    // are we metric?
    bool useMetricUnits = (appsettings->value(this, GC_UNIT).toString() == "Metric");

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

        for (int i=0; i<4; i++) { //XXX taken out maximums -- too much info -- looks ugly

            QString aggname;
            QStringList list;

            switch(i) {
                case 0 : // Totals
                    aggname = "Totals";
                    list = totalColumn;
                    break;

                case 1 :  // Averages
                    aggname = "Averages";
                    list = averageColumn;
                    break;

                case 3 :  // Maximums
                    aggname = "Maximums";
                    list = maximumColumn;
                    break;

                case 2 :  // User defined.. 
                    aggname = "Metrics";
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

                QString value = SummaryMetrics::getAggregated(metricname, results, useMetricUnits);


                // don't show units for time values
                if (metric && (metric->units(useMetricUnits) == "seconds" ||
                               metric->units(useMetricUnits) == "")) {

                    summaryText += QString("<tr><td>%1:</td><td align=\"right\"> %2</td>")
                                            .arg(metric ? metric->name() : "unknown")
                                            .arg(value);

                } else {
                    summaryText += QString("<tr><td>%1(%2):</td><td align=\"right\"> %3</td>")
                                            .arg(metric ? metric->name() : "unknown")
                                            .arg(metric ? metric->units(useMetricUnits) : "unknown")
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
