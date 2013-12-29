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

LTMPopup::LTMPopup(Context *context) : QWidget(context->mainWindow), context(context)
{
    // get application settings
    setAutoFillBackground(false);
    setContentsMargins(0,0,0,0);
    setFixedWidth(800);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
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
    title = new QLabel("No Ride Selected");
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
                     "font-size: 8px; }";
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
    mainLayout->addWidget(rides);

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
}

void
LTMPopup::setTitle(QString s)
{
    title->setText(s);
}

void
LTMPopup::setData(QList<SummaryMetrics>data, const RideMetric *metric, QString title)
{
    // create the ride list
    int count = 0;
    rides->clear();
    selected.clear();

    // set headings
    rides->setColumnCount(2);

    // when
    QTableWidgetItem *h = new QTableWidgetItem("Date & Time", QTableWidgetItem::Type);
    rides->setHorizontalHeaderItem(0,h);

    // value
    h = new QTableWidgetItem(metric->name(), QTableWidgetItem::Type);
    rides->setHorizontalHeaderItem(1,h);

    // now add rows to the table for each entry
    foreach(SummaryMetrics x, data) {

        QDateTime rideDate = x.getRideDate();

        // we'll select it for summary aggregation
        selected << x;

        // date/time
        QTableWidgetItem *t = new QTableWidgetItem(rideDate.toString("ddd, dd MMM yy hh:mmA"));
        t->setFlags(t->flags() & (~Qt::ItemIsEditable));
        rides->setRowCount(count+1);
        rides->setItem(count, 0, t);
        rides->setRowHeight(count, 14);

        // metrics
        QString value = x.getStringForSymbol(metric->symbol(), context->athlete->useMetricUnits);
        h = new QTableWidgetItem(value,QTableWidgetItem::Type);
        h->setFlags(t->flags() & (~Qt::ItemIsEditable));
        h->setTextAlignment(Qt::AlignHCenter);
        rides->setItem(count, 1, h);


        count++;
    }

    // make em all visible!
    rides->resizeColumnsToContents();
    if (count > 1) {
        rides->setFixedHeight((count > 10 ? 10 : count) * 14 + rides->horizontalHeader()->height());
    }

    // select the first one
    rides->setRangeSelected(QTableWidgetSelectionRange(0,0,0,1), true);

    // for now at least, if multiple rides then show the table
    // if single ride show the summary, show all if we're grouping by
    // days tho, since we're interested in specific rides...
    if (count > 1) {
        //int size = ((count+1)*14) + rides->horizontalHeader()->height() + 4;
        //rides->setFixedHeight(size > 100 ? 100 : size);

        rides->show();
        metrics->show();
        notes->show();

    } else {

        rides->hide();
        metrics->show();
        notes->show();
    }

    // Metric summary
    QString filename = context->athlete->home.absolutePath()+"/ltm-summary.html";
    if (!QFile(filename).exists()) filename = ":/html/ltm-summary.html";

    // read it in...
    QFile summaryFile(filename);
    if (summaryFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&summaryFile);
        summary = in.readAll();
        summaryFile.close();
    }
            setTitle(title);

    rideSelected();
}

void
LTMPopup::setData(LTMSettings &settings, QDate start, QDate end)
{
    // set the title
    QString _title;
    switch (settings.groupBy) {
        case LTM_DAY:
            _title = start.toString("dddd, d MMMM yyyy");
            break;
        case LTM_MONTH:
            _title = start.toString("MMMM yyyy");
            break;
        case LTM_YEAR:
            _title = start.toString("yyyy");
            break;
        case LTM_WEEK:
            _title = QString("%1 to %2")
                    .arg(start.toString("dd MMMM yyyy"))
                    .arg(end.toString("dd MMMM yyyy"));
            break;
        case LTM_TOD:
            _title = QString("%1 to %2")
                    .arg(settings.start.date().toString("dd MMMM yy"))
                    .arg(settings.end.date().toString("dd MMMM yy"));
            break;
    }

    // create the ride list
    int count = 0;
    rides->clear();
    selected.clear();

    // set headings
    rides->setColumnCount(settings.metrics.count()+1);
    QTableWidgetItem *h = new QTableWidgetItem("Date & Time", QTableWidgetItem::Type);
    rides->setHorizontalHeaderItem(0,h);
    int column = 1;
    foreach(MetricDetail d, settings.metrics) {
         h = new QTableWidgetItem(d.name,QTableWidgetItem::Type);
         rides->setHorizontalHeaderItem(column++,h);
    }

    foreach(SummaryMetrics x, (*settings.data)) {
        QDateTime rideDate = x.getRideDate();
        if (rideDate.date() >= start && rideDate.date() <= end) {

            // we'll select it for summary aggregation
            selected << x;

            // date/time
            QTableWidgetItem *t = new QTableWidgetItem(rideDate.toString("ddd, dd MMM yy hh:mmA"));
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            rides->setRowCount(count+1);
            rides->setItem(count, 0, t);
            rides->setRowHeight(count, 14);

            // metrics
            int column = 1;
            foreach(MetricDetail d, settings.metrics) {
                QString value = x.getStringForSymbol(d.symbol, context->athlete->useMetricUnits);
                h = new QTableWidgetItem(value,QTableWidgetItem::Type);
                h->setFlags(t->flags() & (~Qt::ItemIsEditable));
                h->setTextAlignment(Qt::AlignHCenter);
                rides->setItem(count, column++, h);
            }
            count++;
        }
    }

    // make em all visible!
    rides->resizeColumnsToContents();
    if (count > 1) {
        rides->setFixedHeight((count > 10 ? 10 : count) * 14 +
                        rides->horizontalHeader()->height());
    }

    // select the first one
    rides->setRangeSelected(QTableWidgetSelectionRange(0,0,0,settings.metrics.count()), true);

    // for now at least, if multiple rides then show the table
    // if single ride show the summary, show all if we're grouping by
    // days tho, since we're interested in specific rides...
    if (count > 1) {
        //int size = ((count+1)*14) + rides->horizontalHeader()->height() + 4;
        //rides->setFixedHeight(size > 100 ? 100 : size);

        if (settings.groupBy != LTM_DAY) {
            rides->show();
            metrics->show();
            notes->show();
        } else {
            rides->show();
            metrics->show();
            notes->show();
        }
        _title += QString(" (%1 rides)").arg(count);
    } else {
        rides->hide();
        metrics->show();
        notes->show();
    }

    // Metric summary
    QString filename = context->athlete->home.absolutePath()+"/ltm-summary.html";
    if (!QFile(filename).exists()) filename = ":/html/ltm-summary.html";

    // read it in...
    QFile summaryFile(filename);
    if (summaryFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream in(&summaryFile);
        summary = in.readAll();
        summaryFile.close();
    }
    setTitle(_title);

    rideSelected();
}

void
LTMPopup::rideSelected()
{
    int index = 0;
    foreach (QTableWidgetItem *item, rides->selectedItems())
        index = item->row();

    if (selected.count() > index) {

        // update summary
        metrics->setText(selected[index].toString(summary, context->athlete->useMetricUnits));

        notes->setText(selected[index].getText("Notes", ""));
    }
    resizeEvent(NULL);
}

void
LTMPopup::resizeEvent(QResizeEvent *)
{
    int _width = width()-10;
    int _firstWidth = 150;
    int _scrollbarWidth = 20;
    int _sectionWidth = ((_width-_scrollbarWidth-_firstWidth) / (rides->horizontalHeader()->count()-1)) - 4;

    rides->setFixedWidth(_width);
    rides->horizontalHeader()->resizeSection(0, _firstWidth);

    for (int i=1; i < rides->horizontalHeader()->count(); i++)
        rides->horizontalHeader()->resizeSection(i, _sectionWidth);
}

bool
LTMPopup::eventFilter(QObject * /*object*/, QEvent *e)
{

    //if (object != (QObject *)plot()->canvas() )
        //return false;
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
