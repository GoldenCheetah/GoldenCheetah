/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "LTMPopup.h"
#include "MainWindow.h"
#include "Athlete.h"
#include "Specification.h"
#include "RideCache.h"
#include "RideFileCache.h" // for RideBest
#include "Utils.h"
#include "Colors.h"

LTMPopup::LTMPopup(Context *context) : QWidget(context->mainWindow), context(context)
{
    // get application settings
    setAutoFillBackground(false);
    setContentsMargins(0,0,0,0);
    setMinimumWidth(800);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(10 *dpiXFactor); // some space between the vertical Scroll bars
    setStyleSheet(QString::fromUtf8(
        "border-width: 0px; \
         border-color: rgba(255,255,255,0); \
         color: white; \
         background-color: rgba(255, 255, 255, 0); \
         selection-color: rgba(255,255,255,60); \
         selection-background-color: rgba(0,0,255,60);"));

    // title
    QFont titleFont;
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    title = new QLabel(tr("No activity Selected"));
    title->setFont(titleFont);
    title->setFixedHeight(30);
    mainLayout->addWidget(title);

    // ride list...
    rides = new QTableWidget(this);
#ifdef Q_OS_LINUX // QT 4.7 bug on Linux, selection-background-color is ignored (no transparency)
#if QT_VERSION > 0x5000
    QStyle *style = QStyleFactory::create("fusion");
#else
    QStyle *style = QStyleFactory::create("Cleanlooks");
#endif
    rides->setStyle(style);
#endif
    rides->setFrameStyle(QFrame::NoFrame);
    rides->viewport()->setAutoFillBackground(false);
    QString styleSheet = "::section {"
                     "spacing: 2px;"
                     "color: white;"
                     "background-color: rgba(255,255,255,0);"
                     "border: 0px;"
                     "margin: 0px;"
                     "font-size: 10px; }";
    rides->horizontalHeader()->setStyleSheet(styleSheet);
    rides->verticalHeader()->hide();
    rides->setShowGrid(false);
    rides->setSelectionMode(QAbstractItemView::SingleSelection);
    rides->setSelectionBehavior(QAbstractItemView::SelectRows);
    rides->viewport()->setMouseTracking(true);
    rides->viewport()->installEventFilter(this);
    QFont rideFont;
    rideFont.setPointSize(SMALLFONT);
    rides->setFont(rideFont);
    mainLayout->addWidget(rides, 0, Qt::AlignRight);

    // display the metrics summary here
    metrics = new QTextEdit(this);
    metrics->setReadOnly(true);
    metrics->setFrameStyle(QFrame::NoFrame);
    metrics->setFixedHeight(175);
    metrics->viewport()->setAutoFillBackground(false);
    QFont metricFont;
    metricFont.setPointSize(SMALLFONT);
    metrics->setFont(metricFont);
    mainLayout->addWidget(metrics);

    // display the ride notes here
    notes = new QTextEdit;
    notes->setReadOnly(true);
    notes->setFont(metricFont);
    notes->setFrameStyle(QFrame::NoFrame);
    notes->viewport()->setAutoFillBackground(false);
    mainLayout->addWidget(notes);

    connect(rides, SIGNAL(itemSelectionChanged()), this, SLOT(rideSelected()));
    connect(rides, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(rideOpen()));

}

void
LTMPopup::setTitle(QString s)
{
    title->setText(s);
}

void
LTMPopup::setData(Specification spec, const RideMetric *metric, QString title)
{
    // create the ride list
    int count = 0;
    rides->clear();
    selected.clear();

    // set headings
    rides->setColumnCount(2);

    // when
    QTableWidgetItem *h = new QTableWidgetItem(tr("Date & Time"), QTableWidgetItem::Type);
    rides->setHorizontalHeaderItem(0,h);

    // value & process html encoding of(TM)
    QString name(Utils::unprotect(metric->name()));
    h = new QTableWidgetItem(name, QTableWidgetItem::Type);

    rides->setHorizontalHeaderItem(1,h);

    // now add rows to the table for each entry
    foreach(RideItem *item, context->athlete->rideCache->rides()) {

        if (!spec.pass(item)) continue;

        // what rides were selected ?
        selected << item->fileName;

        // date/time
        QTableWidgetItem *t = new QTableWidgetItem(item->dateTime.toString(tr("ddd, dd MMM yy hh:mm")));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        t->setTextAlignment(Qt::AlignHCenter);
        rides->setRowCount(count+1);
        rides->setItem(count, 0, t);
        rides->setRowHeight(count, 14);

        // metrics
        double d = item->getForSymbol(metric->symbol());
        const_cast<RideMetric *>(metric)->setValue(d);
        QString value = metric->toString(context->athlete->useMetricUnits);

        h = new QTableWidgetItem(value,QTableWidgetItem::Type);
        h->setFlags(t->flags() & (~Qt::ItemIsEditable));
        h->setTextAlignment(Qt::AlignHCenter);
        rides->setItem(count, 1, h);

        count++;
    }

    // make em all visible! - also show rides if only 1 ride selected
    rides->resizeColumnsToContents();
    rides->setFixedHeight((count > 10 ? 10 : count) * 14 + rides->horizontalHeader()->height());
    // select the first one only if more than 1 rows exist !
    if (count > 1) {
        rides->setRangeSelected(QTableWidgetSelectionRange(0,0,0,1), true);
    }
    // show rides and metrics if at least 1 rides is selected -
    // and even if only 1 Rides (since without the Ride we have not date/time)
    // and the metrics for which the ride was selected
    if (count == 0) {
       // no ride
       rides->hide();
       metrics->hide();
       notes->hide();
       title = tr("No activity selected");
    } else {
       // one or more rides
       rides->show();
       metrics->show();
       notes->show();
    }

    setTitle(title);

    rideSelected();
}

void
LTMPopup::setData(LTMSettings &settings, QDate start, QDate end, QTime time)
{
    // set the title
    QString _title;
    switch (settings.groupBy) {
        case LTM_DAY:
            _title = start.toString(tr("dddd, d MMMM yyyy"));
            break;
        case LTM_MONTH:
            _title = start.toString(tr("MMMM yyyy"));
            break;
        case LTM_YEAR:
            _title = start.toString("yyyy");
            break;
        case LTM_WEEK:
            _title = QString(tr("%1 to %2"))
                    .arg(start.toString(tr("dd MMMM yyyy")))
                    .arg(end.toString(tr("dd MMMM yyyy")));
            break;
        case LTM_TOD:
        case LTM_ALL:
            _title = QString(tr("%1 to %2"))
                    .arg(settings.start.date().toString(tr("dd MMMM yy")))
                    .arg(settings.end.date().toString(tr("dd MMMM yy")));
            break;

    }

    // add Search/Filter info to Title
    if (context->isfiltered || context->ishomefiltered)
       _title = tr("Search/Filter: ") + _title;


    // For LTM_ALL show all Rides which are done in the active/selected Date Range
    if (settings.groupBy == LTM_ALL) {
        start = settings.start.date();
        end = settings.end.date();
    }

    // For LTM_TOD show all Rides which are done in the active/selected Date Range considering the Ride Time
    QTime end_time;
    if (settings.groupBy == LTM_TOD) {
        end_time = time.addSecs(3599); // full hour to 1 second before next (this also avoids reaching 00:00:00)
    }

    // create the ride list
    int count = 0;
    rides->clear();
    selected.clear();

    // set headings
    rides->setColumnCount(1);
    QTableWidgetItem *h = new QTableWidgetItem(tr("Date & Time"), QTableWidgetItem::Type);
    rides->setHorizontalHeaderItem(0,h);
    bool nonRideMetrics = false; // a non Ride specific metrics is shown on the Chart
    // collect the header texts (to know the number of columns)
    int column = 1;
    QList<QString> headerList;
    foreach(MetricDetail d, settings.metrics) {
         // only add columns for ride related metrics (DB, META and BEST)
         if (d.type == METRIC_DB || d.type == METRIC_META || d.type == METRIC_BEST) {
             headerList << d.uname;
             column++;
         } else {
             // there is at least one other metrics in the chart, so add info to title
             nonRideMetrics = true;
         }
    }
    // set the header texts of relevant columns exist
    if (column > 1) {
        rides->setColumnCount(column);
        int i = 1;
        foreach (QString header, headerList) {
            h = new QTableWidgetItem(header,QTableWidgetItem::Type);
            rides->setHorizontalHeaderItem(i++,h);
        }
    }

    // Summary Metrics.data is always available and thus perfect find the rides eligible for the list
    int i = 0;
    foreach(RideItem *item, context->athlete->rideCache->rides()) {

        if (!settings.specification.pass(item)) continue;

        QDate rideDate = item->dateTime.date();

          // check either RideDate (for all Date related groupBy's) or RideTime (for LTM_TOD only)
          if (((settings.groupBy != LTM_TOD) && (rideDate >= start && rideDate <= end))
            ||
            // group by time of day ?
            ((settings.groupBy == LTM_TOD) && (item->dateTime.time() >= time && item->dateTime.time() <= end_time))) {

             // we'll select it for summary display
             selected << item->fileName;

             // date/time
             QTableWidgetItem *t = new QTableWidgetItem(item->dateTime.toString(tr("ddd, dd MMM yy hh:mm")));
             t->setFlags(t->flags() & (~Qt::ItemIsEditable));
             t->setTextAlignment(Qt::AlignHCenter);
             rides->setRowCount(count+1);
             rides->setItem(count, 0, t);
             rides->setRowHeight(count, 14);

             // the column data has to be determined depending on the metrics type
             int column = 1;
             // per selected Ride find values for each Metrics
             foreach(MetricDetail d, settings.metrics) {
                 QString value;
                 if (d.type == METRIC_DB || d.type == METRIC_META) {

                     // get the value to show as a string
                     value = item->getStringForSymbol(d.symbol, context->athlete->useMetricUnits);

                 } else if (d.type == METRIC_BEST) {

                     // bests are only available if a METRIC_BEST exists
                     RideBest bests = settings.bests->at(i);

                     // and are not considered as standard metrics for rounding, so do it here with precision 0
                     double v = bests.getForSymbol(d.bestSymbol);
                     value = QString("%1").arg(v, 0, 'f', 0);
                 } else {
                     // for any other type, rides have no data / and no header column is prepared
                     continue;
                 }
                 h = new QTableWidgetItem(value,QTableWidgetItem::Type);
                 h->setTextAlignment(Qt::AlignHCenter);
                 rides->setItem(count, column++, h);
             }
             count++;
         }
         i++;
    }

    // make em all visible!
    rides->resizeColumnsToContents();
    rides->setFixedHeight((count > 10 ? 10 : count) * 14 + rides->horizontalHeader()->height());

    // select the first one only if more than 1 rows exist !
    if (count > 1) {
        rides->setRangeSelected(QTableWidgetSelectionRange(0,0,0,settings.metrics.count()), true);
    }

    // show rides and metrics if at least 1 rides is selected -
    // and even if only 1 Rides (since without the Ride we have not date/time)
    // and the metrics for which the ride was selected
    if (count == 0) {
       // no ride
       rides->hide();
       metrics->hide();
       notes->hide();
       _title = tr("No activity selected");
    } else {
       // one or more rides
       rides->show();
       metrics->show();
       notes->show();
       // give some extrat info
       if (count > 1) _title += QString(tr(" (%1 activities)")).arg(count);
    }

    if (nonRideMetrics) _title += QString(tr(" / non activity-related metrics skipped"));

    setTitle(_title);
    rideSelected();
}

QString
LTMPopup::setSummaryHTML(RideItem *item)
{
    // where we construct the text
    QString summaryText("");

    // main totals
    const QStringList totalColumn = QStringList()
        << "workout_time"
        << "time_riding"
        << (item->isSwim ? "distance_swim" : "total_distance")
        << "total_work"
        << "skiba_wprime_exp"
        << "elevation_gain";

    const QStringList averageColumn = QStringList()
        << "average_speed"
        << "average_power"
        << "average_hr"
        << "average_cad"
        << (item->isSwim ? "pace_swim" : "pace");

    static const QStringList maximumColumn = QStringList()
        << "max_speed"
        << "max_power"
        << "max_heartrate"
        << "max_cadence"
        << "skiba_wprime_max";


    // user defined
    QString s = appsettings->value(this, GC_SETTINGS_SUMMARY_METRICS, GC_SETTINGS_SUMMARY_METRICS_DEFAULT).toString();

    // in case they were set tand then unset
    if (s == "") s = GC_SETTINGS_SUMMARY_METRICS_DEFAULT;
    QStringList metricColumn = s.split(",");

    // foreach of the metrics get the ride value
    // header of summary

    summaryText = QString("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 3.2//EN\">"
                          "<html>"
                          "<head>"
                          "<title></title>"
                          "</head>"
                          "<body>"
                          "<table border= \"0\" cellspacing=\"10\">"
                          "<tr>");


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

      case 2 :  // Maximums
              aggname = tr("Maximums");
              list = maximumColumn;
              break;

      case 3 :  // User defined..
              aggname = tr("Metrics");
              list = metricColumn;
      break;

      }

      summaryText += QString("<td align=\"center\" width=\"25%\">"
                             "<table>"
                             "<tr>"
                             "<td align=\"center\" colspan=\"2\">"
                             "<b>%1</b>"
                             "</td>"
                             "</tr>").arg(aggname);

      foreach(QString metricname, list) {

        const RideMetric *metric = RideMetricFactory::instance().rideMetric(metricname);

        // use getAggregate even if it's only 1 file to have consistent value treatment
        double d = item->getForSymbol(metricname, true);
        QString value;
        if (metric) {
            const_cast<RideMetric*>(metric)->setValue(d);
            value = metric->toString(context->athlete->useMetricUnits);
        }

        // Maximum Max and Average Average looks nasty, remove from name for display
        QString s = metric ? metric->name().replace(QRegExp(tr("^(Average|Max) ")), "") : "unknown";

        // don't show units for time values
        if (metric && (metric->units(context->athlete->useMetricUnits) == "seconds" ||
                       metric->units(context->athlete->useMetricUnits) == tr("seconds") ||
                       metric->units(context->athlete->useMetricUnits) == "")) {

          summaryText += QString("<tr><td>%1:</td><td align=\"right\"> %2</td></tr>")
                                 .arg(s)
                                 .arg(value);

        } else {
          summaryText += QString("<tr><td>%1(%2):</td><td align=\"right\"> %3</td></tr>")
                                 .arg(s)
                                 .arg(metric ? metric->units(context->athlete->useMetricUnits) : "unknown")
                                 .arg(value);
        }
      }
      summaryText += QString("</table>"
                             "</td>");

   }

   // finish off the html document
   summaryText += QString("</tr>"
                          "</table>"
                          "</center>"
                          "</body>"
                          "</html>");


   return summaryText;
}

void
LTMPopup::rideSelected()
{
    // which ride is selected
    int index = 0;
    if (rides->selectedItems().count())
        index = rides->selectedItems().first()->row();

    // do we have any rides and is the index within bounds
    if (selected.count() > index) {

        RideItem *have = context->athlete->rideCache->getRide(selected[index]);

        if (have) {

            // update summary
            metrics->setText(""); //! stop crash (?)
            metrics->setText(setSummaryHTML(have));

            notes->setText(""); //! stop crash (?)
            // Use calendar text for more information and to allow customization
            notes->setText(have->getText("Calendar Text", ""));
        }
    }
    resizeEvent(NULL);
}

void
LTMPopup::rideOpen()
{
    // which ride is selected
    int index = 0;
    if (rides->selectedItems().count())
        index = rides->selectedItems().first()->row();

    // do we have any rides and is the index within bounds
    if (selected.count() > index) {

        RideItem *have = context->athlete->rideCache->getRide(selected[index]);

        if (have) {

            // Select Activity in Activities view
            context->notifyRideSelected(have);
            // Select Activities view
            context->mainWindow->selectAnalysis();
        }
    }
}

void
LTMPopup::resizeEvent(QResizeEvent *)
{
    int _width = width()-10;
    int _firstWidth = 150;
    int _scrollbarWidth = 20;

    rides->setFixedWidth(_width);
    rides->horizontalHeader()->resizeSection(0, _firstWidth);
    // only resize if more than Date/Time column exists
    if (rides->horizontalHeader()->count() > 1) {
        int _sectionWidth = ((_width-_scrollbarWidth-_firstWidth) / (rides->horizontalHeader()->count()-1)) - 4;
        rides->horizontalHeader()->resizeSection(0, _firstWidth);
        for (int i=1; i < rides->horizontalHeader()->count(); i++)
           rides->horizontalHeader()->resizeSection(i, _sectionWidth);

    }
}
bool
LTMPopup::eventFilter(QObject * /*object*/, QEvent *e)
{

    //if (object != (QObject *)plot()->canvas() )
        //return false;
    //if only 1 Ride in list, ignore any mouse activities
    if (rides->rowCount() == 1) return false;

    // process mouse
    switch (e->type()) {
    case QEvent::MouseMove:
        {
            int row = rides->indexAt((static_cast<QMouseEvent*>(e))->pos()).row();

            if (row >= 0) {
                 QList<QModelIndex> selection = rides->selectionModel()->selection().indexes();
                 if (selection.count() == 0 || selection[0].row() != row) {
                    rides->clearSelection();
                    rides->setRangeSelected(QTableWidgetSelectionRange(row,0,row,rides->columnCount()-1), true);
                }
            }
            return false;
        }
        break;
    default:
        return false;
    }
    return false;
}
