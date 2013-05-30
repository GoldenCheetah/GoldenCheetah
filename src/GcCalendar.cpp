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

#include "GcCalendar.h"
#include "GcWindowLayout.h"
#include "Settings.h"
#include <QWebSettings>
#include <QWebFrame>
#include "TimeUtils.h"

//********************************************************************************
// CALENDAR SIDEBAR (GcCalendar)
//********************************************************************************
GcCalendar::GcCalendar(MainWindow *main) : main(main)
{
    setContentsMargins(0,0,0,0);
    setAutoFillBackground(true);

    month = year = 0;
    _ride = NULL;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);

    // Splitter - cal at top, summary at bottom
    splitter = new GcSplitter(Qt::Vertical);
    mainLayout->addWidget(splitter);

    // calendar
    calendarItem = new GcSplitterItem(tr("Calendar"), iconFromPNG(":images/sidebar/calendar.png"), this);
    summaryItem = new GcSplitterItem(tr("Summary"), iconFromPNG(":images/sidebar/dashboard.png"), this);

    QPalette pal;
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    // cal widget
    QWidget *cal = new QWidget(this);
    cal->setPalette(pal);
    cal->setContentsMargins(0,0,0,0);
    cal->setStyleSheet("QLabel { color: gray; }");
    layout = new QVBoxLayout(cal);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    calendarItem->addWidget(cal);

    // summary widget
    QWidget *sum = new QWidget(this);
    sum->setPalette(pal);
    sum->setContentsMargins(0,0,0,0);
    QVBoxLayout *slayout = new QVBoxLayout(sum);
    slayout->setSpacing(0);
    summaryItem->addWidget(sum);

    splitter->addWidget(calendarItem);
    splitter->addWidget(summaryItem);
    splitter->prepare(main->cyclist, "diary");

    black.setColor(QPalette::WindowText, Qt::gray);
    white.setColor(QPalette::WindowText, Qt::white);
    grey.setColor(QPalette::WindowText, Qt::gray);

    multiCalendar = new GcMultiCalendar(main);
    layout->addWidget(multiCalendar);

    // Summary level selector
    QHBoxLayout *h = new QHBoxLayout();
    h->setSpacing(5);
    summarySelect = new QComboBox(this);
    summarySelect->setFixedWidth(150); // is it impossible to size constrain qcombos?
    summarySelect->addItem(tr("Day Summary"));
    summarySelect->addItem(tr("Weekly Summary"));
    summarySelect->addItem(tr("Monthly Summary"));
    summarySelect->setCurrentIndex(1); // default to weekly
    h->addStretch();
    h->addWidget(summarySelect, Qt::AlignCenter);
    h->addStretch();
    slayout->addLayout(h);

    summary = new QWebView(this);
    summary->setContentsMargins(0,0,0,0);
    summary->page()->view()->setContentsMargins(0,0,0,0);
    summary->page()->mainFrame()->setScrollBarPolicy(Qt::Vertical, Qt::ScrollBarAlwaysOff);
    summary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    summary->setAcceptDrops(false);

    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply
    summary->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize());
    summary->settings()->setFontFamily(QWebSettings::StandardFont, defaultFont.family());
    slayout->addWidget(summary);
    slayout->addStretch();

    // summary mode changed
    connect(summarySelect, SIGNAL(currentIndexChanged(int)), this, SLOT(refresh()));

    // refresh on these events...
    connect(main, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh()));
    connect(main, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh()));
    connect(main, SIGNAL(configChanged()), this, SLOT(refresh()));

    // set up for current selections
    refresh();
}

void
GcCalendar::refresh()
{
    if (!isHidden()) {
        multiCalendar->refresh();
        setSummary();
        repaint();
    }
}

void
GcCalendar::setRide(RideItem *ride)
{
    _ride = ride;

    multiCalendar->setRide(ride);
    setSummary();
}

bool
GcLabel::event(QEvent *e)
{
    // entry / exit event repaint for hover color
    if (e->type() == QEvent::Leave || e->type() == QEvent::Enter) repaint();
    return QWidget::event(e);
}

void
GcLabel::paintEvent(QPaintEvent *)
{
    static QIcon left = iconFromPNG(":images/mac/left.png");
    static QIcon right = iconFromPNG(":images/mac/right.png");

    QPainter painter(this);
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing, true);

    // grr. some want a rect others want a rectf
    QRectF norm(0,0,width(),height());
    QRect all(0,0,width(),height());

    if (bg) {

        // setup a painter and the area to paint
        if (!underMouse()) painter.fillRect(all, bgColor);
        else {
            if (filtered) painter.fillRect(all, GColor(CCALCURRENT));
            else painter.fillRect(all, Qt::lightGray);
        }

        painter.setPen(Qt::gray);
        painter.drawRect(QRect(0,0,width(),height()));
    }

    if (selected) {
        painter.fillRect(all, GColor(CCALCURRENT));
    }

    if (text() != "<" && text() != ">") {
        painter.setFont(this->font());

        if (xoff || yoff) {

            // draw text in white behind...
            QRectF off(xoff,yoff,width(),height());
            painter.setPen(QColor(255,255,255,200));
            painter.drawText(off, alignment(), text());
        }

        if (filtered && !selected && !underMouse()) painter.setPen(GColor(CCALCURRENT));
        else painter.setPen(QColor(0,0,0,170));

        painter.drawText(norm, alignment(), text());

        if (highlighted) {
            QColor over = GColor(CCALCURRENT);
            over.setAlpha(180);
            painter.setPen(over);

            painter.drawText(norm, alignment(), text());
        }
    } else {

        // use standard icons
        QIcon &icon = text() == "<" ?  left : right;
        Qt::AlignmentFlag alignment = text() == "<" ? Qt::AlignLeft : Qt::AlignRight;

        icon.paint(&painter, all, alignment|Qt::AlignVCenter);
    }

    if (text() != ""  && filtered) {
        QPen pen;
        pen.setColor(GColor(CCALCURRENT));
        pen.setWidth(3);
        painter.setPen(pen);
        painter.drawRect(QRect(0,0,width(),height()));
    }
    painter.restore();
}

void
GcCalendar::setSummary()
{
    // are we metric?
    bool useMetricUnits = main->useMetricUnits;

    // where we construct the text
    QString summaryText("");

    QDate when;
    if (_ride && _ride->ride()) when = _ride->dateTime.date();
    else when = QDate::currentDate();

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
    QDate newFrom, newTo;

    switch(summarySelect->currentIndex()) {

        case 0 :
            // DAILY - just the date of the ride
            newFrom = newTo = when;
            break;
        case 1 :
            // WEEKLY - from Mon to Sun
            newFrom = when.addDays((when.dayOfWeek()-1)*-1);
            newTo = newFrom.addDays(6);
            break;

        default:
        case 2 : 
            // MONTHLY - all days in month
            newFrom = QDate(when.year(), when.month(), 1);
            newTo = newFrom.addMonths(1).addDays(-1);
            break;
    }

    if (newFrom != from || newTo != to) {

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

        for (int i=0; i<4; i++) { //taken out maximums -- too much info -- looks ugly

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

                QStringList empty; // usually for filters, but we don't do that
                QString value = SummaryMetrics::getAggregated(main, metricname, results, empty, false, useMetricUnits);


                // Maximum Max and Average Average looks nasty, remove from name for display
                QString s = metric ? metric->name().replace(QRegExp(tr("^(Average|Max) ")), "") : "unknown";

                // don't show units for time values
                if (metric && (metric->units(useMetricUnits) == "seconds" ||
                               metric->units(useMetricUnits) == tr("seconds") ||
                               metric->units(useMetricUnits) == "")) {

                    summaryText += QString("<tr><td>%1:</td><td align=\"right\"> %2</td>")
                                            .arg(s)
                                            .arg(value);

                } else {
                    summaryText += QString("<tr><td>%1(%2):</td><td align=\"right\"> %3</td>")
                                            .arg(s)
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

    // we always tell everyone the date range changed
    QString name;

    if (summarySelect->currentIndex() == 0)
        name = tr("Day of ") + from.toString("dddd MMMM d");
    else if (summarySelect->currentIndex() == 1)
        name = QString(tr("Week Commencing %1")).arg(from.toString("dddd MMMM d"));
    else if (summarySelect->currentIndex() == 2)
        name = from.toString(tr("MMMM yyyy"));

    emit dateRangeChanged(DateRange(from, to, name));
}

//********************************************************************************
// MINI CALENDAR (GcMiniCalendar)
//********************************************************************************
GcMiniCalendar::GcMiniCalendar(MainWindow *main, bool master) : main(main), master(master)
{
    setContentsMargins(0,0,0,0);
    setAutoFillBackground(true);

    month = year = 0;
    _ride = NULL;

    setStyleSheet("QLabel { color: gray; }");
    layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    black.setColor(QPalette::WindowText, Qt::gray);
    white.setColor(QPalette::WindowText, Qt::white);
    grey.setColor(QPalette::WindowText, Qt::gray);

    // get the model
    fieldDefinitions = main->rideMetadata()->getFields();
    calendarModel = new GcCalendarModel(this, &fieldDefinitions, main);
    calendarModel->setSourceModel(main->listView->sqlModel);

    QHBoxLayout *line = new QHBoxLayout;
    line->setSpacing(0);
    line->setContentsMargins(0,0,0,0);

    QFont font;
#ifdef WIN32
    font.setPointSize(10);
#else
    font.setPointSize(12);
#endif

    if (master) {
        left = new GcLabel("<",this);
        left->setAutoFillBackground(false);
        left->setPalette(white);
        left->setFont(font);
        left->setAlignment(Qt::AlignLeft);
        line->addWidget(left);
        connect (left, SIGNAL(clicked()), this, SLOT(previous()));
    }

    monthName = new GcLabel("January 2012", this);
    monthName->setAutoFillBackground(false);
    monthName->setPalette(white);
    monthName->setFont(font);
    monthName->setAlignment(Qt::AlignCenter);
    line->addWidget(monthName);

    if (master) {
        right = new GcLabel(">", this);
        right->setAutoFillBackground(false);
        right->setPalette(white);
        right->setFont(font);
        right->setAlignment(Qt::AlignRight);
        line->addWidget(right);
        connect (right, SIGNAL(clicked()), this, SLOT(next()));
    }

    QWidget *month = new QWidget(this);
    month->setContentsMargins(0,0,0,0);
    month->setFixedWidth(180);
    month->setFixedHeight(180);
    QGridLayout *dayLayout = new QGridLayout(month);
    dayLayout->setSpacing(1);
    dayLayout->setContentsMargins(0,0,0,0);
    dayLayout->addLayout(line, 0,0,1,7);
    layout->addWidget(month, Qt::AlignCenter);

    font.setWeight(QFont::Normal);

    // Mon
#ifdef WIN32
    font.setPointSize(8);
#else
    font.setPointSize(9);
#endif
    GcLabel *day = new GcLabel("Mon", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 0);

    // Tue
    day = new GcLabel("Tue", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 1);

    // Wed
    day = new GcLabel("Wed", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 2);

    // Thu
    day = new GcLabel("Thu", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 3);

    // Fri
    day = new GcLabel("Fri", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 4);

    // Sat
    day = new GcLabel("Sat", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 5);

    // Sun
    day = new GcLabel("Sun", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 6);

    signalMapper = new QSignalMapper(this);
#ifdef WIN32
    font.setPointSize(8);
#else
    font.setPointSize(11);
#endif
    for (int row=2; row<8; row++) {

        for (int col=0; col < 7; col++) {

            GcLabel *d = new GcLabel(QString("%1").arg((row-2)*7+col), this);
            d->setFont(font);
            d->setAutoFillBackground(false);
            d->setPalette(grey);
            d->setStyleSheet("color: gray;");
            d->setAlignment(Qt::AlignCenter);
            d->setXOff(0);
            d->setYOff(0);
            dayLayout->addWidget(d,row,col);

            // we like squares
            d->setFixedHeight(22);
            d->setFixedWidth(22);

            dayLabels << d;

            if (row== 3 && col == 4) d->setSelected(true);

            connect (d, SIGNAL(clicked()), signalMapper, SLOT(map()));
            signalMapper->setMapping(d, (row-2)*7+col);
        }
    }
    layout->addStretch();

    // day clicked
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(dayClicked(int)));

    // set up for current selections
    refresh();
}

void
GcMiniCalendar::refresh()
{
    if (month && year) setDate(month, year);
}

bool
GcMiniCalendar::event(QEvent *e)
{
    if (e->type() != QEvent::ToolTip && e->type() != QEvent::Paint && e->type() != QEvent::Destroy &&
        e->type() != QEvent::LayoutRequest) {
        main->setBubble("");
    }

    if (e->type() == QEvent::Paint) {
        // fill the background
        QPainter painter(this);
        QRect all(0,0,width(),height());
        //painter.fillRect(all, QColor("#B3B4BA"));
        painter.fillRect(all, QColor(Qt::white));
    }

    int n=0;
    if (e->type() == QEvent::ToolTip) {

        // are we hovering over a label?
        foreach(GcLabel *label, dayLabels) {
            if (label->underMouse()) {
                if (dayLabels.at(n)->text() == "") return false;

                // select a ride if there is one for this one?
                int row = n / 7;
                int col = n % 7;

                QModelIndex p = calendarModel->index(row,col);
                QStringList files = calendarModel->data(p, GcCalendarModel::FilenamesRole).toStringList();

                QPoint pos = dynamic_cast<QHelpEvent*>(e)->pos();

                // Popup bubble for ride
                if (files.count()) {
                    if (files[0] == "calendar") ; // handle planned rides
                    else main->setBubble(files.at(0), mapToGlobal(pos+QPoint(+2,+2)));
                }
            }
            n++;
        }
    }
    return QWidget::event(e);
}

void
GcMiniCalendar::dayClicked(int i)
{
    if (dayLabels.at(i)->text() == "") return;

    // select a ride if there is one for this one?
    int row = i / 7;
    int col = i % 7;


    QModelIndex p = calendarModel->index(row,col);
    QStringList files = calendarModel->data(p, GcCalendarModel::FilenamesRole).toStringList();

    if (files.count()) // if more than one file cycle through them?
        main->selectRideFile(QFileInfo(files[0]).fileName());

}

void
GcMiniCalendar::previous()
{
    QList<QDateTime> allDates = main->metricDB->db()->getAllDates();
    qSort(allDates);

    // begin of month
    QDateTime bom(QDate(year,month,01), QTime(0,0,0));

    for(int i=allDates.count()-1; i>=0; i--) {
        if (allDates.at(i) < bom) {

            QDate date = allDates.at(i).date();
            month = date.month();
            year = date.year();
            calendarModel->setMonth(date.month(), date.year());

            // find the day in the calendar...
            for (int day=42; day>0;day--) {

                QModelIndex p = calendarModel->index(day/7,day%7);
                QDate heredate = calendarModel->date(p);
                if (date == heredate) {
                    // select this ride...
                    QStringList files = calendarModel->data(p, GcCalendarModel::FilenamesRole).toStringList();
                    if (files.count()) main->selectRideFile(QFileInfo(files[0]).fileName());
                }
            }
            emit dateChanged(month,year);
            break;
        }
    }
}

void
GcMiniCalendar::next()
{
    QList<QDateTime> allDates = main->metricDB->db()->getAllDates();
    qSort(allDates);

    // end of month
    QDateTime eom(QDate(year,month,01).addMonths(1), QTime(00,00,00));
    for(int i=0; i<allDates.count(); i++) {
        if (allDates.at(i) >= eom) {

            QDate date = allDates.at(i).date();
            month = date.month();
            year = date.year();
            calendarModel->setMonth(date.month(), date.year());

            // find the day in the calendar...
            for (int day=0; day<42;day++) {

                QModelIndex p = calendarModel->index(day/7,day%7);
                QDate heredate = calendarModel->date(p);
                if (date == heredate) {
                    // select this ride...
                    QStringList files = calendarModel->data(p, GcCalendarModel::FilenamesRole).toStringList();
                    if (files.count()) main->selectRideFile(QFileInfo(files[0]).fileName());
                }
            }
            emit dateChanged(month,year);
            break;
        }
    }
}

void
GcMiniCalendar::setRide(RideItem *ride)
{

    if (ride != _ride) {
        _ride = ride;


        QDate when;
        if (_ride && _ride->ride()) when = _ride->dateTime.date();
        else when = QDate::currentDate();

        // refresh the model
        setDate(when.month(), when.year());
    }
}

void
GcMiniCalendar::clearRide()
{
    if (_ride) {
        _ride = NULL;
        setDate(month,year);
    }
}

void
GcMiniCalendar::setFilter(QStringList filter)
{
    filters = filter;
}

void
GcMiniCalendar::clearFilter()
{
    filters.clear();
}

void
GcMiniCalendar::setDate(int _month, int _year)
{

    // don't refresh the model if we're already showing this month
    if (month != _month || year != _year) calendarModel->setMonth(_month, _year);
    monthName->setText(QDate(_year,_month,01).toString("MMMM yyyy"));

    // update state
    month = _month;
    year = _year;

    // now reapply for each row/col of calendar...
    for (int row=0; row<6; row++) {
        for (int col=0; col < 7; col++) {

            GcLabel *d = dayLabels.at(row*7+col);
            QModelIndex p = calendarModel->index(row,col);
            QDate date = calendarModel->date(p);

            // filtered dates surrounded in light blue
            d->setFiltered(false);
            foreach (QString file, calendarModel->data(p, GcCalendarModel::FilenamesRole).toStringList()) {
                if (filters.contains(file)) d->setFiltered(true);
            }

            if (date.month() != month || date.year() != year) {
                d->setText("");
                d->setBg(false);
                d->setSelected(false);
            } else {
                d->setText(QString("%1").arg(date.day()));

                // what color should it be?
                // for taste we /currently/ just set to bg to
                // highlight there is a ride there, colors /will/ come
                // back later when worked out a way of making it look
                // nice and not garish 
                QList<QColor> colors = p.data(Qt::BackgroundRole).value<QList<QColor> >();
                if (colors.count()) {
                    d->setBg(true);
                    d->setPalette(black);
                    d->setBgColor(colors.at(0)); // use first always
                } else {
                    d->setBg(false);
                    d->setPalette(white);
                }

                // is this the current ride?
                QDate when;
                if (_ride && _ride->ride()) when = _ride->dateTime.date();

                if (date == when) {
                    d->setSelected(true);
                    d->setPalette(white);
                } else d->setSelected(false);
            }
        }
    }
}

//********************************************************************************
// MULTI CALENDAR (GcMultiCalendar)
//********************************************************************************
GcMultiCalendar::GcMultiCalendar(MainWindow *main) : QScrollArea(main), main(main), active(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(0,0,0,0);

    setFrameStyle(QFrame::NoFrame);

    layout = new GcWindowLayout(this, 0, 4, 0);
    layout->setSpacing(0);
    layout->setContentsMargins(10,10,0,0);

    QPalette pal;
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    GcMiniCalendar *mini = new GcMiniCalendar(main, true);
    calendars.append(mini);
    mini->setDate(QDate::currentDate().month(), QDate::currentDate().year()); // default to this month
    layout->addWidget(mini);

    // only 1 months are visible right now
    showing = 1;

    connect(mini, SIGNAL(dateChanged(int,int)), this, SLOT(dateChanged(int,int)));
}

void
GcMultiCalendar::setFilter(QStringList filter)
{
    this->filters = filter;

    if (!isVisible()) {
        stale = true;
        return;
    }

    for (int i=0; i<calendars.count();i++) {
        calendars.at(i)->setFilter(filter);
    }

    // refresh
    int month, year;
    calendars.at(0)->getDate(month,year);
    dateChanged(month,year);
}

void
GcMultiCalendar::clearFilter()
{
    this->filters.clear();

    if (!isVisible()) {
        stale = true;
        return;
    }

    for (int i=0; i<calendars.count();i++) {
        calendars.at(i)->clearFilter();
    }

    // refresh
    int month, year;
    calendars.at(0)->getDate(month,year);
    dateChanged(month,year);
}

void
GcMultiCalendar::dateChanged(int month, int year)
{
    if (!isVisible()) {
        stale = true;
        return;
    }

    setUpdatesEnabled(false);

    // master changed make all the others change too
    QDate date(year,month,01);
    calendars.at(0)->setDate(month, year);
    for (int i=1; i<calendars.count();i++) {
        QDate ours = date.addMonths(i);
        calendars.at(i)->setDate(ours.month(), ours.year());
    }
    setUpdatesEnabled(true);
}

void
GcMultiCalendar::resizeEvent(QResizeEvent*)
{
    if (!isVisible()) {
        stale = true;
        return;
    }

    // we expand x and y
    int oldshowing = showing;
    showing = height() < 180 ? 1 : (int)(height() / 180);
    showing *= width() < 180 ? 1 : (int)(width() / 180);

    int have = calendars.count();

    int m,y;
    calendars.at(0)->getDate(m,y);
    QDate first(y, m, 01);

    if (showing > calendars.count()) {

        for (int i=have; i<showing; i++) {
            GcMiniCalendar *mini = new GcMiniCalendar(main, false);
            mini->setFilter(this->filters);
            calendars.append(mini);
            layout->insert(i, mini);
            calendars.at(i)->setDate(first.addMonths(i).month(), first.addMonths(i).year());
        }
        
    } else {

        for (int i=0; i<calendars.count(); i++) {

            if (i>=oldshowing && i<showing) {

                calendars.at(i)->setFilter(this->filters);
                calendars.at(i)->setDate(first.addMonths(i).month(), first.addMonths(i).year());
                calendars.at(i)->show();
 
            // We no longer delete unused calendars we just hide them
            } else if (i && i>=showing) {

                GcMiniCalendar *p = calendars.at(i);
                p->hide();
            }
        }
    }
}

void
GcMultiCalendar::setRide(RideItem *ride)
{
    _ride = ride;

    if (active) return;
    if (!isVisible()) {
        stale = true;

        // notify of ride gone though as not repeated
        if (!_ride) {
            for (int i=0; i<showing; i++) {
                calendars.at(i)->setRide(_ride); // current ride is on this one
            }
        }
        return;
    }

    setUpdatesEnabled(false);
    active = true; // avoid multiple calls

    // whats the date on the first calendar?
    int month, year;
    calendars.at(0)->getDate(month,year);

    QDate when;
    if (!ride || !ride->ride()) when = QDate::currentDate();
    else when = ride->dateTime.date();

    // lets see which calendar to inform..
    bool done = false;
    for (int i=0; i<showing; i++) {
        QDate from = QDate(year,month,01).addMonths(i);
        QDate to = from.addMonths(1);

        if (when >= from && when < to) {
            done = true;
            calendars.at(i)->setRide(ride); // current ride is on this one
        } else {
            calendars.at(i)->clearRide(); // no longer highlight  on that mini calendar
        }
    }

    // the ride selected is not visible.. move via the master (first) mini calendar
    if (done == false) {

        calendars.at(0)->setRide(ride);
        // set the calendars to the right month etc
        // they will ignore if they already have that date
        int m,y;
        calendars.at(0)->getDate(m,y);
        QDate first(y, m, 01);
        for (int i=1; i<calendars.count();i++) {
            calendars.at(i)->setDate(first.addMonths(i).month(), first.addMonths(i).year());
        }
    }

    setUpdatesEnabled(true);
    active = false;
}

void
GcMultiCalendar::refresh()
{
    setUpdatesEnabled(false);
    for(int i=0; i<showing; i++) {
        calendars.at(i)->refresh();
    }
    setUpdatesEnabled(true);
}

void
GcMultiCalendar::showEvent(QShowEvent*)
{
    setRide(_ride);
    setFilter(filters);
    resizeEvent(NULL);
}
