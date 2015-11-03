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

#include "ComparePane.h"
#include "Season.h"
#include "Settings.h"
#include "Colors.h"
#include "RideCache.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "RideFile.h"
#include "RideFileCache.h"
#include "RideMetric.h"
#include "ColorButton.h"
#include "TimeUtils.h"
#include "Units.h"
#include "Zones.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QTextEdit>

//
// A selection of distinct colours, user can adjust also
//
static QList<QColor> standardColors;
static bool initStandardColors()
{
    standardColors << QColor(Qt::magenta);
    standardColors << QColor(Qt::cyan);
    standardColors << QColor(Qt::yellow);
    standardColors << QColor(Qt::red);
    standardColors << QColor(Qt::blue);
    standardColors << QColor(Qt::gray);
    standardColors << QColor(Qt::darkCyan);
    standardColors << QColor(Qt::green);
    standardColors << QColor(Qt::darkRed);
    standardColors << QColor(Qt::darkGreen);
    standardColors << QColor(Qt::darkBlue);
    standardColors << QColor(Qt::darkMagenta);

    return true;
}
static bool init = initStandardColors();

QColor standardColor(int num)
{
   return standardColors.at(num % standardColors.count());
}

// we need to fix the sort order!
class CTableWidgetItem : public QTableWidgetItem
{
    public:
        CTableWidgetItem(int type = Type) : QTableWidgetItem(type) {}
        ~CTableWidgetItem() {}

        bool operator<(const QTableWidgetItem&other) const // for sorting our way
        { 
            QStringList list;

            switch(column()) {

                case 2 : return text() < other.text(); // athlete

                case 3 : return QDate::fromString(text(), Qt::ISODate) <
                                QDate::fromString(other.text(), Qt::ISODate); // date

                case 4 : // date or time depending on which view
                         if (text().contains(":")) {

                             return QTime::fromString(text(), "hh:mm:ss") <
                                    QTime::fromString(other.text(), "hh:mm:ss");

                         } else {

                            return //
                                   QDate::fromString(text(), Qt::ISODate) <
                                   QDate::fromString(other.text(), Qt::ISODate); // date

                         }
                default: // work it out ..
                         // first time & duration (considering the fixed format with at least one ":")
                         if (text().contains(":") && other.text().contains(":")) {
                             // time or duration - (for comparison the can be treated equally by converting to "seconds"
                             // QTime only works up to 23:59:59 - but in Trends View Durations will be often higher
                             int factor;
                             double t1 = 0;
                             // split seconds, minutes, hours into a list and compute Seconds (Right to Left)
                             list = text().split(":", QString::SkipEmptyParts, Qt::CaseInsensitive);
                             factor = 1;
                             while (!list.isEmpty()) {
                                 t1 += list.takeLast().toInt() * factor; // start from the end
                                 factor *= 60; // seconds, minutes, hours
                             }
                             double t2 = 0;
                             // split seconds, minutes, hours into a list and compute Seconds (Right to Left)
                             list = other.text().split(":", QString::SkipEmptyParts, Qt::CaseInsensitive);
                             factor = 1;
                             while (!list.isEmpty()) {
                                 t2 += list.takeLast().toInt() * factor; // start from the end
                                 factor *= 60; // seconds, minutes, hours
                             }

                             return t1 < t2;

                         } else if (text().contains(QRegExp("[^0-9.,]")) ||
                                    other.text().contains(QRegExp("[^0-9.,]"))) { // alpha

                              return text() < other.text();

                         } else { // assume numeric

                              return text().toDouble() < other.text().toDouble();
                        }
                        break;
            }
            return false; // keep compiler happy
        }
};

ComparePane::ComparePane(Context *context, QWidget *parent, CompareMode mode) : QWidget(parent), context(context), mode_(mode)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    setAcceptDrops(true);
    setAutoFillBackground(true);
    QPalette pal;
    pal.setBrush(QPalette::Active, QPalette::Window, Qt::white);
    pal.setBrush(QPalette::Inactive, QPalette::Window, Qt::white);
    setPalette(pal);

    scrollArea = new QScrollArea(this);
    scrollArea->setAutoFillBackground(false);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setContentsMargins(0,0,0,0);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(scrollArea);

    table = new QTableWidget(this);
#ifdef Q_OS_MAC
    table->setAttribute(Qt::WA_MacShowFocusRect, 0);
    table->horizontalHeader()->setSortIndicatorShown(false); // blue looks nasty
#endif
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setAcceptDrops(false);
    table->setStyleSheet("QTableWidget { border: none; }");
    table->setFrameStyle(QFrame::NoFrame);
    scrollArea->setWidget(table);

    configChanged(CONFIG_APPEARANCE | CONFIG_METRICS); // set up ready to go...

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(table->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(itemsWereSorted()));
}

void
ComparePane::configChanged(qint32)
{
    // via standard style sheet
    table->setStyleSheet(GCColor::stylesheet());

    // refresh table...
    refreshTable();
}

void
ComparePane::refreshTable()
{
    blockSignals(true); // don't stop me now...

    if (mode_ == interval) { // INTERVALS

        // STEP ONE : SET THE TABLE HEADINGS

        // clear current contents
        table->clearSelection();
        table->clear();
        table->setRowCount(0);
        table->setColumnCount(0);

        // metric summary
        QStringList always;
        always << "workout_time" << "total_distance";
        QString s = appsettings->value(this, GC_SETTINGS_INTERVAL_METRICS, GC_SETTINGS_INTERVAL_METRICS_DEFAULT).toString();
        if (s == "") s = GC_SETTINGS_INTERVAL_METRICS_DEFAULT;
        QStringList metricColumns = always + s.split(","); // always showm metrics plus user defined summary metrics
        metricColumns.removeDuplicates(); // where user has already added workout_time, total_distance

        // called after config is updated typically
        QStringList list;
        list << "" // checkbox
            << "" // color
            << tr("Athlete")
            << tr("Date")
            << tr("Time");

        QStringList worklist; // metrics to compute
        RideMetricFactory &factory = RideMetricFactory::instance();

        foreach(QString metric, metricColumns) {

            // get the metric name
            const RideMetric *m = factory.rideMetric(metric);
            if (m) {
                worklist << metric;
                QString units;
                // check for both original and translated
                if (!(m->units(context->athlete->useMetricUnits) == "seconds" || m->units(context->athlete->useMetricUnits) == tr("seconds")))
                    units = m->units(context->athlete->useMetricUnits);
                QTextEdit name(m->name()); // process html encoding of(TM)
                if (units != "") list << QString("%1 (%2)").arg(name.toPlainText()).arg(units);
                else list << QString("%1").arg(name.toPlainText());
            }
        }

        list << tr("Interval");

        table->setColumnCount(list.count()+1);
        table->horizontalHeader()->setSectionHidden(list.count(), true);
        table->setHorizontalHeaderLabels(list);
        table->setSortingEnabled(false);
        table->verticalHeader()->hide();
        table->setShowGrid(false);
        table->setSelectionMode(QAbstractItemView::MultiSelection);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);

        // STEP TWO : CLEAR AND RE-ADD TO REFLECT CHANGES

        table->setRowCount(context->compareIntervals.count());
        int counter = 0;
        foreach(CompareInterval x, context->compareIntervals) {

            // compute the metrics for this ride
            RideItem metrics;
            QHash<QString, RideMetricPtr> computed = RideMetric::computeMetrics(context, x.data,
                                                     context->athlete->zones(), context->athlete->hrZones(), worklist);

            metrics.setFrom(computed);

            // First few cols always the same
            // check - color - athlete - date - time
            // now create a row on the compare pane

            // Checkbox
            QCheckBox *check = new QCheckBox(this);
            check->setChecked(x.checked);
            if (!counter) check->setEnabled(false);
            table->setCellWidget(counter, 0, check);
            connect(check, SIGNAL(stateChanged(int)), this, SLOT(intervalButtonsChanged()));

            // Color Button
            ColorButton *colorButton = new ColorButton(this, "Color", x.color);
            table->setCellWidget(counter, 1, colorButton);
            connect(colorButton, SIGNAL(colorChosen(QColor)), this, SLOT(intervalButtonsChanged()));

            // athlete
            CTableWidgetItem *t = new CTableWidgetItem;
            t->setText(x.sourceContext->athlete->cyclist);
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(counter, 2, t);

            // date
            t = new CTableWidgetItem;
            t->setText(x.data->startTime().date().toString(Qt::ISODate));
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(counter, 3, t);

            // time
            t = new CTableWidgetItem;
            t->setText(x.data->startTime().time().toString("hh:mm:ss"));
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(counter, 4, t);

            // metrics
            for(int i = 0; i < worklist.count(); i++) {

                RideMetricPtr m = computed.value(worklist[i]);

                QString strValue;

                if (m) {
                    // get value and convert if needed
                    double value = metrics.getForSymbol(worklist[i]) 
                                * (context->athlete->useMetricUnits ? 1 : m->conversion()) 
                                + (context->athlete->useMetricUnits ? 0 : m->conversionSum());

                    // use right precision
                    strValue = QString("%1").arg(value, 0, 'f', m->precision());
    
                    // or maybe its a duration (worry about local lang or translated)
                    if (m->units(true) == "seconds" || m->units(true) == tr("seconds"))
                        strValue = time_to_string(value);

                }

                // add to the table
                t = new CTableWidgetItem;
                t->setText(strValue);
                t->setFlags(t->flags() & (~Qt::ItemIsEditable));
                table->setItem(counter, i + 5, t);
            }

            // Interval name
            t = new CTableWidgetItem;
            t->setText(x.name);
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(counter, worklist.count() + 5, t);

            // INDEX
            t = new CTableWidgetItem;
            t->setText(QString("%1").arg(counter));
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(counter, worklist.count() + 6, t);

            // align center
            for (int i=3; i<(worklist.count()+5); i++)
                table->item(counter,i)->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

            table->setRowHeight(counter, 23);
            counter++;
        }

        table->setRowCount(counter);
        table->setVisible(false);
        table->resizeColumnsToContents(); // set columns to fit
        table->setVisible(true);

#if QT_VERSION > 0x050000 // fix the first two if we can
        for (int i=0; i<list.count(); i++) {
            if (i < 2) {
                table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
            } else {
                table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Interactive);
            }
        }
#else
        table->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
#endif
        table->horizontalHeader()->setStretchLastSection(true);

    } else { //SEASONS

        // STEP ONE : SET THE TABLE HEADINGS

        // clear current contents
        table->clearSelection();
        table->clear();
        table->setRowCount(0);
        table->setColumnCount(0);

        // metric summary
        QStringList always;
        always << "workout_time" << "total_distance";
        QString s = appsettings->value(this, GC_SETTINGS_SUMMARY_METRICS, GC_SETTINGS_SUMMARY_METRICS_DEFAULT).toString();
        if (s == "") s = GC_SETTINGS_SUMMARY_METRICS_DEFAULT;
        QStringList metricColumns = always + s.split(","); // always showm metrics plus user defined summary metrics
        metricColumns.removeDuplicates(); // where user has already added workout_time, total_distance

        // called after config is updated typically
        QStringList list;
        list << "" // checkbox
            << "" // color
            << tr("Athlete")
            << tr("From")
            << tr("To");

        QStringList worklist; // metrics to compute
        RideMetricFactory &factory = RideMetricFactory::instance();

        foreach(QString metric, metricColumns) {

            // get the metric name
            const RideMetric *m = factory.rideMetric(metric);
            if (m) {
                worklist << metric;
                QString units;
                if (!(m->units(context->athlete->useMetricUnits) == "seconds" || m->units(context->athlete->useMetricUnits) == tr("seconds")))
                    units = m->units(context->athlete->useMetricUnits);
                QTextEdit name(m->name()); // process html encoding of(TM)
                if (units != "") list << QString("%1 (%2)").arg(name.toPlainText()).arg(units);
                else list << QString("%1").arg(name.toPlainText());
            }
        }

        list << tr("Date Range");

        table->setColumnCount(list.count()+1);
        table->horizontalHeader()->setSectionHidden(list.count(), true);
        table->setHorizontalHeaderLabels(list);
        table->setSortingEnabled(false);
        table->verticalHeader()->hide();
        table->setShowGrid(false);
        table->setSelectionMode(QAbstractItemView::MultiSelection);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);

        // STEP TWO : CLEAR AND RE-ADD TO REFLECT CHANGES

        table->setRowCount(context->compareDateRanges.count());
        int counter = 0;
        foreach(CompareDateRange x, context->compareDateRanges) {

            // First few cols always the same
            // check - color - athlete - date - time
            // now create a row on the compare pane

            // Checkbox
            QCheckBox *check = new QCheckBox(this);
            check->setChecked(x.checked);
            if (!counter) check->setEnabled(false);
            table->setCellWidget(counter, 0, check);
            connect(check, SIGNAL(stateChanged(int)), this, SLOT(daterangeButtonsChanged()));

            // Color Button
            ColorButton *colorButton = new ColorButton(this, "Color", x.color);
            table->setCellWidget(counter, 1, colorButton);
            connect(colorButton, SIGNAL(colorChosen(QColor)), this, SLOT(daterangeButtonsChanged()));

            // athlete
            CTableWidgetItem *t = new CTableWidgetItem;
            t->setText(x.sourceContext->athlete->cyclist);
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(counter, 2, t);

            // date from
            t = new CTableWidgetItem;
            t->setText(x.start.toString(Qt::ISODate));
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(counter, 3, t);

            // date to
            t = new CTableWidgetItem;
            t->setText(x.end.toString(Qt::ISODate));
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(counter, 4, t);

            // metrics
            for(int i = 0; i < worklist.count(); i++) {

                QString value = x.sourceContext->athlete->rideCache->getAggregate(worklist[i], x.specification, context->athlete->useMetricUnits);

                // add to the table
                t = new CTableWidgetItem;
                t->setText(value);
                t->setFlags(t->flags() & (~Qt::ItemIsEditable));
                table->setItem(counter, i + 5, t);
            }

            // Date Range name
            t = new CTableWidgetItem;
            t->setText(x.name);
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(counter, worklist.count() + 5, t);

            // INDEX
            t = new CTableWidgetItem;
            t->setText(QString("%1").arg(counter));
            t->setFlags(t->flags() & (~Qt::ItemIsEditable));
            table->setItem(counter, worklist.count() + 6, t);


            // align center
            for (int i=3; i<(worklist.count()+5); i++)
                table->item(counter,i)->setTextAlignment(Qt::AlignVCenter | Qt::AlignHCenter);

            table->setRowHeight(counter, 23);
            counter++;
        }

        table->setRowCount(counter);
        table->setVisible(false);
        table->resizeColumnsToContents(); // set columns to fit
        table->setVisible(true);
#if QT_VERSION > 0x050000 // fix the first two if we can
        for (int i=0; i<list.count(); i++) {
            if (i < 2) {
                table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Fixed);
            } else {
                table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::Interactive);
            }
        }
#else
        table->horizontalHeader()->setResizeMode(QHeaderView::Interactive);
#endif
        table->horizontalHeader()->setStretchLastSection(true);
    }
    // sorting has to be disabled as long as table content is updated
    table->setSortingEnabled(true);
    blockSignals(false);
}

void
ComparePane::itemsWereSorted()
{
    if (mode_ == interval) {

        QList<CompareInterval> newOrder;

        for(int i=0;i<table->rowCount(); i++) {
            QCheckBox *check = static_cast<QCheckBox*>(table->cellWidget(i,0));
            if (i) check->setEnabled(true);
            else {
                check->setChecked(true);
                check->setEnabled(false);
            }
            int oldIndex = table->item(i,table->columnCount()-1)->text().toInt();
            table->item(i,table->columnCount()-1)->setText(QString("%1").arg(i));
            newOrder << context->compareIntervals.at(oldIndex);
        }

        context->compareIntervals = newOrder;
        context->notifyCompareIntervalsChanged();

    }
 else {

        QList<CompareDateRange> newOrder;

        for(int i=0;i<table->rowCount(); i++) {
            QCheckBox *check = static_cast<QCheckBox*>(table->cellWidget(i,0));
            if (i) check->setEnabled(true);
            else {
                check->setChecked(true);
                check->setEnabled(false);
            }
            int oldIndex = table->item(i,table->columnCount()-1)->text().toInt();
            table->item(i,table->columnCount()-1)->setText(QString("%1").arg(i));
            newOrder << context->compareDateRanges.at(oldIndex);
        }

        context->compareDateRanges = newOrder;
        context->notifyCompareDateRangesChanged();

    }
}

void
ComparePane::clear()
{
    if (mode_ == interval) { // INTERVALS

        // wipe all away
        foreach(CompareInterval ci, context->compareIntervals) {
            delete ci.data;
        }
        context->compareIntervals.clear();

        // refresh table
        refreshTable();

        // tell the charts
        context->notifyCompareIntervalsChanged();

    } else {

        // wipe em
        context->compareDateRanges.clear();

        // refresh table
        refreshTable();

        // tell the charts
        context->notifyCompareDateRangesChanged();

    }
}

void
ComparePane::intervalButtonsChanged()
{
    // run through the table and see if anything changed
    bool changed = false;
    for (int i=0; i<table->rowCount(); i++) {

        bool isChecked = static_cast<QCheckBox*>(table->cellWidget(i,0))->isChecked();
        QColor color =  static_cast<ColorButton*>(table->cellWidget(i,1))->getColor();

        if (context->compareIntervals[i].checked != isChecked ||
            context->compareIntervals[i].color != color) {

            context->compareIntervals[i].checked = isChecked;
            context->compareIntervals[i].color = color;
            changed = true;
        }
    }
    if (changed) context->notifyCompareIntervalsChanged();
}

void
ComparePane::daterangeButtonsChanged()
{
    // run through the table and see if anything changed
    bool changed = false;
    for (int i=0; i<table->rowCount(); i++) {

        bool isChecked = static_cast<QCheckBox*>(table->cellWidget(i,0))->isChecked();
        QColor color =  static_cast<ColorButton*>(table->cellWidget(i,1))->getColor();

        if (context->compareDateRanges[i].checked != isChecked ||
            context->compareDateRanges[i].color != color) {

            context->compareDateRanges[i].checked = isChecked;
            context->compareDateRanges[i].color = color;
            changed = true;
        }
    }
    if (changed) context->notifyCompareDateRangesChanged();
}

void 
ComparePane::dragEnterEvent(QDragEnterEvent *event)
{
    if ((mode_ == interval && event->mimeData()->formats().contains("application/x-gc-intervals")) ||
        (mode_ == season && event->mimeData()->formats().contains("application/x-gc-seasons"))) {
        event->acceptProposedAction();
    }
}

void 
ComparePane::dragLeaveEvent(QDragLeaveEvent *)
{
    // we might consider hiding on this?
}

// sort intervals most recent first
static bool dateRecentFirst(const IntervalItem *a, const IntervalItem *b) { return a->rideItem_->dateTime > b->rideItem_->dateTime; }

void
ComparePane::dropEvent(QDropEvent *event)
{
    // set action to copy and accept that so the source data
    // is left intact and not wiped or removed
    event->setDropAction(Qt::CopyAction);
    event->accept();

    // here we can unpack and add etc...
    // lets get the context!
    QString fmt = (mode_ == interval) ? "application/x-gc-intervals" : "application/x-gc-seasons";

    // get the context out
    QByteArray rawData = event->mimeData()->data(fmt);
    QDataStream stream(&rawData, QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_6);

    // pack data 
    quint64 from;
    stream >> from; // where did this come from?

    // lets look at the context..
    Context *sourceContext = (Context*)(from);

    // NOW LETS UNPACK
    if (mode_ == interval) { // INTERVALS

        int count;

        QList<CompareInterval> newOnes;

        // lets get the basic data
        stream >> count;
        for (int i=0; i<count; i++) {

            CompareInterval add;

            add.checked = true;                     // UPDATE COMPARE INTERVAL
            add.context = context;                  // UPDATE COMPARE INTERVAL
            add.sourceContext = sourceContext;      // UPDATE COMPARE INTERVAL

            quint64 ridep;
            quint64 start, stop, startKM, stopKM;
            quint64 seq; // not relevant here

            // serialize
            stream >> add.name;                     // UPDATE COMPARE INTERVAL

            stream >> ridep;
            RideItem *rideItem = (RideItem*)ridep;
            RideFile *ride = rideItem->ride();

            // index into ridefile
            stream >> start;
            stream >> stop;
            stream >> startKM;
            stream >> stopKM;
            stream >> seq;
            stream >> add.route;

            // just construct a ridefile for the interval

            // RideFile *data;
            add.data = new RideFile(ride);
            add.data->context = context;


            // manage offsets
            bool first = true;
            double offset = 0.0f, offsetKM = 0.0f;

            foreach(RideFilePoint *p, ride->dataPoints()) {

                if (p->secs > stop) break;

                if (p->secs >= start) {

                    // intervals always start from zero when comparing
                    if (first) {
                        first = false;
                        offset = p->secs;
                        offsetKM = p->km;
                    }

                    add.data->appendPoint(p->secs - offset, p->cad, p->hr, p->km - offsetKM, p->kph, p->nm,
                                          p->watts, p->alt, p->lon, p->lat, p->headwind,
                                          p->slope, p->temp,
                                          p->lrbalance, p->lte, p->rte, p->lps, p->rps,
                                          p->lpco, p->rpco, p->lppb, p->rppb, p->lppe, p->rppe, p->lpppb, p->rpppb, p->lpppe, p->rpppe,
                                          p->smo2, p->thb, p->rvert, p->rcad, p->rcontact, p->tcore, 0);

                    // get derived data calculated
                    RideFilePoint *l = add.data->dataPoints().last();
                    l->np = p->np;
                    l->xp = p->xp;
                    l->apower = p->apower;
                }
            }
            add.data->recalculateDerivedSeries();

            // just use standard colors and cycle round
            // we will of course repeat, but the user can
            // just edit them using the button
            add.color = standardColors.at((i + context->compareIntervals.count()) % standardColors.count());

            // construct a fake RideItem, slightly hacky need to fix this later XXX fixme
            //                            mostly cut and paste from RideItem::refresh
            //                            we do this to support user data series in compare mode
            //                            so UserData can be generated from this rideItem
            add.rideItem = new RideItem(add.data, add.data->context);
            add.rideItem->setFrom(*rideItem);
            add.rideItem->metadata_ = add.data->tags();
            add.rideItem->getWeight();
            add.rideItem->isRun = add.data->isRun();
            add.rideItem->isSwim = add.data->isSwim();
            add.rideItem->present = add.data->getTag("Data", "");
            add.rideItem->samples = add.data->dataPoints().count() > 0;
            const RideMetricFactory &factory = RideMetricFactory::instance();
            QHash<QString,RideMetricPtr> computed= RideMetric::computeMetrics(add.data->context, add.data, add.data->context->athlete->zones(), 
                                               add.data->context->athlete->hrZones(), factory.allMetrics());
            add.rideItem->metrics_.fill(0, factory.metricCount());
            QHashIterator<QString, RideMetricPtr> l(computed);
            while (l.hasNext()) {
                l.next();
                add.rideItem->metrics_[l.value()->index()] = l.value()->value();
            }
            for(int j=0; j<factory.metricCount(); j++)
                if (std::isinf(add.rideItem->metrics_[j]) || std::isnan(add.rideItem->metrics_[j]))
                    add.rideItem->metrics_[j] = 0.00f;
            // end of fake RideItem hack XXX

            // now add but only if not empty
            if (!add.data->dataPoints().empty()) newOnes << add;

        }

        // if we have nothing being compared yet and are only dropping one and it's a route
        // then offer to find all times you traveled along the same route
        if (context->compareIntervals.count() == 0 && newOnes.count() == 1 && newOnes[0].route != QUuid()) {

            // how many across seasons? (there will always be the standard seasons)
            // these are passed to the dialog to setup the combobox
            QVector<int> seasonCount(newOnes[0].sourceContext->athlete->seasons->seasons.count());
            QList<IntervalItem*> matches;

            // loop through rides finding intervals on this route
            foreach(RideItem *ride, newOnes[0].sourceContext->athlete->rideCache->rides()) {
                // find the interval?
                foreach(IntervalItem *interval, ride->intervals(RideFileInterval::ROUTE)) {
                    if (interval->route == newOnes[0].route) {

                        // add to the main list
                        matches << interval;

                        // add to the counts - first is always used as default
                        for(int i=0; i<newOnes[0].sourceContext->athlete->seasons->seasons.count(); i++) {
                            Season current = newOnes[0].sourceContext->athlete->seasons->seasons[i];
                            if (interval->rideItem()->dateTime.date() >= current.start &&
                                interval->rideItem()->dateTime.date() <= current.end) {
                                seasonCount[i]++;
                            }
                        }
                    }
                }
            }

            // if we have some lets ask user if they want to
            // add matches for this route, or just this one
            if (matches.count() > 1 ) {

                // sort matches so most recent first
                qSort(matches.begin(), matches.end(), dateRecentFirst);

                // ok, lets crank up a dialog to ask
                // one only, or the season to use
                // the default should be the first one
                int select=0;

                // pass by reference
                RouteDropDialog dialog(this, newOnes[0].sourceContext, newOnes[0].name, seasonCount, select);

                // did they say yes ?
                if (dialog.exec()) {

                    // augment the one we have with all other intervals
                    // but make sure it does not include ours
                    // all other options mean do nothing so we drop
                    // through to the same code as before
                    Season want = newOnes[0].sourceContext->athlete->seasons->seasons[select];
                    foreach(IntervalItem *matched, matches) {

                        // don't add the dropped one twice!
                        if (matched->rideItem()->dateTime == newOnes[0].data->startTime()) continue;

                        // add each one
                        if (matched->rideItem()->dateTime.date() >= want.start &&
                            matched->rideItem()->dateTime.date() <= want.end) {

                            // create a new interval for this one
                            CompareInterval add;
                            add.checked = seasonCount[select] <= 10; // check if not that many, don't if loads
                            add.context = context;                  // UPDATE COMPARE INTERVAL
                            add.sourceContext = newOnes[0].sourceContext;      // UPDATE COMPARE INTERVAL

                            RideFile *ride = matched->rideItem()->ride();

                            add.name = QString("%1/%2 %3").arg(matched->rideItem()->dateTime.date().day())
                                                          .arg(matched->rideItem()->dateTime.date().month())
                                                          .arg(matched->name);
                            add.route = matched->route;

                            // just construct a ridefile for the interval
                            add.data = new RideFile(ride);
                            add.data->context = context;

                            // manage offsets
                            bool first = true;
                            double offset = 0.0f, offsetKM = 0.0f;

                            foreach(RideFilePoint *p, ride->dataPoints()) {

                                if (p->secs > matched->stop) break;

                                if (p->secs >= matched->start) {

                                    // intervals always start from zero when comparing
                                    if (first) {
                                        first = false;
                                        offset = p->secs;
                                        offsetKM = p->km;
                                    }

                                    add.data->appendPoint(p->secs - offset, p->cad, p->hr, p->km - offsetKM, p->kph, p->nm,
                                                        p->watts, p->alt, p->lon, p->lat, p->headwind,
                                                        p->slope, p->temp,
                                                        p->lrbalance, p->lte, p->rte, p->lps, p->rps,
                                                        p->lpco, p->rpco, p->lppb, p->rppb, 
                                                        p->lppe, p->rppe, p->lpppb, p->rpppb, p->lpppe, p->rpppe,
                                                        p->smo2, p->thb, p->rvert, p->rcad, p->rcontact, p->tcore, 0);

                                    // get derived data calculated
                                    RideFilePoint *l = add.data->dataPoints().last();
                                    l->np = p->np;
                                    l->xp = p->xp;
                                    l->apower = p->apower;
                                }
                            }
                            add.data->recalculateDerivedSeries();

                            // construct a fake RideItem, slightly hacky need to fix this later XXX fixme
                            //                            mostly cut and paste from RideItem::refresh
                            //                            we do this to support user data series in compare mode
                            //                            so UserData can be generated from this rideItem
                            add.rideItem = new RideItem(add.data, add.data->context);
                            add.rideItem->setFrom(*matched->rideItem());
                            add.rideItem->metadata_ = add.data->tags();
                            add.rideItem->getWeight();
                            add.rideItem->isRun = add.data->isRun();
                            add.rideItem->isSwim = add.data->isSwim();
                            add.rideItem->present = add.data->getTag("Data", "");
                            add.rideItem->samples = add.data->dataPoints().count() > 0;
                            const RideMetricFactory &factory = RideMetricFactory::instance();
                            QHash<QString,RideMetricPtr> computed= RideMetric::computeMetrics(add.data->context, add.data, add.data->context->athlete->zones(), 
                                               add.data->context->athlete->hrZones(), factory.allMetrics());
                            add.rideItem->metrics_.fill(0, factory.metricCount());
                            QHashIterator<QString, RideMetricPtr> l(computed);
                            while (l.hasNext()) {
                                l.next();
                                add.rideItem->metrics_[l.value()->index()] = l.value()->value();
                            }
                            for(int j=0; j<factory.metricCount(); j++)
                                if (std::isinf(add.rideItem->metrics_[j]) || std::isnan(add.rideItem->metrics_[j]))
                                    add.rideItem->metrics_[j] = 0.00f;
                            // end of fake RideItem hack XXX

                            // just use standard colors and cycle round
                            // we will of course repeat, but the user can
                            // just edit them using the button
                            add.color = standardColors.at((newOnes.count()) % standardColors.count());

                            // now add but only if not empty
                            if (!add.data->dataPoints().empty()) newOnes << add;
                        }
                    }
                }  
            }
        }

        // how many we get ?
        if (newOnes.count()) {

            context->compareIntervals.append(newOnes);

            // refresh the table to reflect the new list
            refreshTable();

            // let all the charts know
            context->notifyCompareIntervalsChanged();
        }

    } else { // SEASONS

        int count;

        QList<CompareDateRange> newOnes;

        // lets get the basic data
        stream >> count;
        for (int i=0; i<count; i++) {

            CompareDateRange add;

            add.checked = true;                     // UPDATE COMPARE INTERVAL
            add.context = context;                  // UPDATE COMPARE INTERVAL
            add.sourceContext = sourceContext;      // UPDATE COMPARE INTERVAL

            stream >> add.name;
            stream >> add.start;
            stream >> add.end;
            stream >> add.days;

            // for now the specification is just a date range
            add.specification.setDateRange(DateRange(add.start,add.end));

            // just use standard colors and cycle round
            // we will of course repeat, but the user can
            // just edit them using the button
            add.color = standardColors.at((i + context->compareDateRanges.count()) % standardColors.count());

            // even empty date ranges are valid
            newOnes << add;

        }
        // how many we get ?
        if (newOnes.count()) {

            context->compareDateRanges.append(newOnes);

            // refresh the table to reflect the new list
            refreshTable();

            // let all the charts know
            context->notifyCompareDateRangesChanged();
        }

    }
}

/*----------------------------------------------------------------------
 * Route Drag and Drop Dialog
 *--------------------------------------------------------------------*/
RouteDropDialog::RouteDropDialog(QWidget *parent, Context *context, QString segmentName, QVector<int> &seasonCount, int &selected) :
    QDialog(parent, Qt::Dialog), context(context), seasonCount(seasonCount), selected(selected)
{
    setWindowTitle(QString(tr("\"%1\"")).arg(segmentName));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);

    // Grid
    QLabel *season = new QLabel("Season");
    seasonSelector = new QComboBox(this);
    QHBoxLayout *seasonLayout = new QHBoxLayout;
    seasonLayout->addWidget(season);
    seasonLayout->addWidget(seasonSelector);
    seasonLayout->addStretch();
    mainLayout->addLayout(seasonLayout);

    // add seasons to the selector
    for(int i=0; i<seasonCount.count(); i++) {
        if (seasonCount[i] > 1) {
            seasonSelector->addItem(QString("%1 (%2)").arg(context->athlete->seasons->seasons[i].name)
                                                      .arg(seasonCount[i]), i);
        }
    }
    seasonSelector->setCurrentIndex(0);
    selected = seasonSelector->itemData(0).toInt(); // the one to go with.

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    all = new QPushButton(tr("&All Selected"), this);
    buttonLayout->addStretch();
    one = new QPushButton(tr("Just this &One"), this);
    buttonLayout->addStretch();
    buttonLayout->addWidget(all);
    buttonLayout->addStretch();
    buttonLayout->addWidget(one);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // we want Just this one to be the default button
    one->setAutoDefault(true);
    one->setDefault(true);

    // connect up slots
    connect(all, SIGNAL(clicked()), this, SLOT(allClicked()));
    connect(one, SIGNAL(clicked()), this, SLOT(oneClicked()));
    connect(seasonSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(seasonChanged(int)));
}

void
RouteDropDialog::allClicked()
{
    accept(); // lets do it, add lots
}
void
RouteDropDialog::oneClicked()
{
    reject(); // don't add loads, just this one
}

void
RouteDropDialog::seasonChanged(int index)
{
    if (index <0) return;
    else selected = seasonSelector->itemData(index).toInt();
}
