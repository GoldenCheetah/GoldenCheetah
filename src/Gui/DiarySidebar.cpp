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

#include "DiarySidebar.h"

#include "Context.h"
#include "Athlete.h"
#include "RideCache.h"
#include "RideCacheModel.h"
#include "TimeUtils.h"
#include "Specification.h"
#include "RideItem.h"

#include "GcWindowLayout.h"
#include "Settings.h"
#include "HelpWhatsThis.h"

//********************************************************************************
// CALENDAR SIDEBAR (DiarySidebar)
//********************************************************************************
DiarySidebar::DiarySidebar(Context *context) : context(context)
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

    // cal widget
    calWidget = new QWidget(this);
    calWidget->setObjectName("calWidget");
    calWidget->setContentsMargins(0,0,0,0);
    calWidget->setAutoFillBackground(true);
    layout = new QVBoxLayout(calWidget);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    calendarItem->addWidget(calWidget);

    HelpWhatsThis *helpCalendarItem = new HelpWhatsThis(calendarItem);
    calendarItem->setWhatsThis(helpCalendarItem->getWhatsThisText(HelpWhatsThis::SideBarDiaryView_Calendar));

    splitter->addWidget(calendarItem);
    splitter->prepare(context->athlete->cyclist, "diary");

    multiCalendar = new GcMultiCalendar(context);
    layout->addWidget(multiCalendar);

    // refresh on these events...
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh()));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh()));
    connect(context, SIGNAL(refreshUpdate(QDate)), this, SLOT(refresh()));
    connect(context, SIGNAL(refreshEnd()), this, SLOT(refresh()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // set up for current selections
    configChanged(CONFIG_APPEARANCE);
}

void
DiarySidebar::configChanged(qint32)
{
    // apply
    multiCalendar->refresh();

    // and summary .. forgetting what we already prepared
    from = to = QDate();

    // set the charts range
    setDateRange();
}

void
DiarySidebar::refresh()
{
    if (!isHidden()) {
        multiCalendar->refresh();
        setDateRange();
    }
    repaint();
}

void
DiarySidebar::setRide(RideItem *ride)
{
    _ride = ride;

    multiCalendar->setRide(ride);
    setDateRange();
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
        if (!underMouse()) {
            painter.fillRect(all, bgColor);
        } else {
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

        if (!GCColor::isFlat() && (xoff || yoff)) {

            // draw text in white behind...
            QRectF off(xoff,yoff,width(),height());
            painter.setPen(QColor(255,255,255,200));
            painter.drawText(off, alignment(), text());
        }

        if (filtered && !selected && !underMouse()) painter.setPen(GColor(CCALCURRENT));
        else {

            if (isChrome && GCColor::isFlat()) {

                if (GCColor::luminance(GColor(CCHROME)) < 127)
                    painter.setPen(QColor(Qt::white));
                else
                    painter.setPen(QColor(30,30,30,200));

            } else painter.setPen(palette().color(QPalette::WindowText));
        }

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

//********************************************************************************
// MINI CALENDAR (GcMiniCalendar)
//********************************************************************************
GcMiniCalendar::GcMiniCalendar(Context *context, bool master) : context(context), master(master)
{
    setContentsMargins(1,1,1,1);
    setAutoFillBackground(true);
    setObjectName("miniCalendar");

    month = year = 0;
    _ride = NULL;

    layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    // get the model
    fieldDefinitions = GlobalContext::context()->rideMetadata->getFields();
    calendarModel = new GcCalendarModel(this, &fieldDefinitions, context);
    calendarModel->setSourceModel(context->athlete->rideCache->model());

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
        left->setFont(font);
        left->setAlignment(Qt::AlignLeft);
        line->addWidget(left);
        connect (left, SIGNAL(clicked()), this, SLOT(previous()));
    }

    monthName = new GcLabel("January 2012", this);
    monthName->setAutoFillBackground(false);
    monthName->setFont(font);
    monthName->setAlignment(Qt::AlignCenter);
    line->addWidget(monthName);

    if (master) {
        right = new GcLabel(">", this);
        right->setAutoFillBackground(false);
        right->setFont(font);
        right->setAlignment(Qt::AlignRight);
        line->addWidget(right);
        connect (right, SIGNAL(clicked()), this, SLOT(next()));
    }

    monthWidget = new QWidget(this);
    monthWidget->setContentsMargins(0,0,0,0);
    monthWidget->setFixedWidth(180);
    monthWidget->setFixedHeight(180);
    monthWidget->setAutoFillBackground(true);
    QGridLayout *dayLayout = new QGridLayout(monthWidget);
    dayLayout->setSpacing(1 *dpiXFactor);
    dayLayout->setContentsMargins(0,0,0,0);
    dayLayout->addLayout(line, 0,0,1,7);
    layout->addWidget(monthWidget, Qt::AlignCenter);

    font.setWeight(QFont::Normal);

    // Mon
#ifdef WIN32
    font.setPointSize(8);
#else
    font.setPointSize(9);
#endif
    GcLabel *day = new GcLabel(tr("Mon"), this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 0);
    dayNames[0] = day;

    // Tue
    day = new GcLabel(tr("Tue"), this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 1);
    dayNames[1] = day;

    // Wed
    day = new GcLabel(tr("Wed"), this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 2);
    dayNames[2] = day;

    // Thu
    day = new GcLabel(tr("Thu"), this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 3);
    dayNames[3] = day;

    // Fri
    day = new GcLabel(tr("Fri"), this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 4);
    dayNames[4] = day;

    // Sat
    day = new GcLabel(tr("Sat"), this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 5);
    dayNames[5] = day;

    // Sun
    day = new GcLabel(tr("Sun"), this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 1, 6);
    dayNames[6] = day;

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

    // set up for current selections - and watch for future changes
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    configChanged(CONFIG_APPEARANCE);
}

void
GcMiniCalendar::configChanged(qint32)
{
    QColor bgColor = GColor(CPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);
    //XXX setStyleSheet(QString("color: %1; background: %2;").arg(fgColor.name()).arg(bgColor.name())); // clear any shit left behind from parents (Larkin ?)
    tint.setColor(QPalette::Window, bgColor);
    tint.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    white.setColor(QPalette::WindowText, fgColor);
    white.setColor(QPalette::Window, bgColor);
    grey.setColor(QPalette::WindowText, fgColor);
    grey.setColor(QPalette::Window, bgColor);

    monthWidget->setPalette(white);
    setPalette(white);

    // only the top one has cursors
    if (master) {
        left->setPalette(white);
        right->setPalette(white);
    }

    monthName->setPalette(tint);
    for (int i=0; i<7; i++) dayNames[i]->setPalette(tint);

    foreach(GcLabel*label, dayLabels) {
        label->setPalette(white);
    }
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
    if (e->type() == QEvent::Paint) {
        // fill the background
        QPainter painter(this);
        QRect all(0,0,width(),height());
        //painter.fillRect(all, QColor("#B3B4BA"));
        painter.fillRect(all, GColor(CPLOTBACKGROUND));

        if (_ride) {
            QDate when = _ride->dateTime.date();
            if (when >= QDate(year,month,01) && when < QDate(year,month,01).addMonths(1))
            painter.fillRect(all, GColor(CPLOTMARKER));
        }
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

                //QPoint pos = dynamic_cast<QHelpEvent*>(e)->pos();
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
        context->athlete->selectRideFile(QFileInfo(files[0]).fileName());

}

void
GcMiniCalendar::previous()
{
    QList<QDateTime> allDates = context->athlete->rideCache->getAllDates();
    std::sort(allDates.begin(), allDates.end());

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
                    if (files.count()) context->athlete->selectRideFile(QFileInfo(files[0]).fileName());
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
    QList<QDateTime> allDates = context->athlete->rideCache->getAllDates();
    std::sort(allDates.begin(), allDates.end());

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
                    if (files.count()) context->athlete->selectRideFile(QFileInfo(files[0]).fileName());
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
    monthName->setText(QDate(_year,_month,01).toString(tr("MMMM yyyy")));

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
                    d->setPalette(grey);
                    if (colors.at(0) == QColor(1,1,1)) d->setBgColor(GColor(CPLOTMARKER)); // default
                    else d->setBgColor(colors.at(0)); // use first always
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

void
DiarySidebar::setDateRange()
{
    QDate when;
    if (_ride && _ride->ride()) when = _ride->dateTime.date();
    else when = QDate::currentDate();

    // what date range should we use?
    QDate newFrom = when.addDays((when.dayOfWeek()-1)*-1);
    QDate newTo = newFrom.addDays(6);

    // if changed lets tell everyone
    if (newFrom != from || newTo != to) {

        // date range changed lets refresh
        from = newFrom;
        to = newTo;
        QString name = QString(tr("Week Commencing %1")).arg(from.toString(tr("dddd MMMM d")));

        emit dateRangeChanged(DateRange(from, to, name));
    }
}

//********************************************************************************
// MULTI CALENDAR (GcMultiCalendar)
//********************************************************************************
GcMultiCalendar::GcMultiCalendar(Context *context) : QScrollArea(context->mainWindow), context(context), active(false)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setContentsMargins(0,0,0,0);
    setObjectName("multiCalendar");
    setAutoFillBackground(true);

    _ride = NULL;

    setFrameStyle(QFrame::NoFrame);

    layout = new GcWindowLayout(this, 0, 4, 0);
    layout->setSpacing(0);
    layout->setContentsMargins(10,10,0,0);

    GcMiniCalendar *mini = new GcMiniCalendar(context, true);
    calendars.append(mini);
    mini->setDate(1, 2000); // default to 01/2000 - which then moves to month of latest Ride
    layout->addWidget(mini);

    // only 1 months are visible right now
    showing = 1;

    connect(mini, SIGNAL(dateChanged(int,int)), this, SLOT(dateChanged(int,int)));
    connect (context, SIGNAL(filterChanged()), this, SLOT(filterChanged()));
    connect (GlobalContext::context(), SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    configChanged(CONFIG_APPEARANCE);
}

void
GcMultiCalendar::configChanged(qint32)
{
    QColor bgColor = GColor(CPLOTBACKGROUND);
    QColor fgColor = GCColor::invertColor(bgColor);

    QPalette pal;
    pal.setColor(QPalette::Window, bgColor);
    pal.setColor(QPalette::WindowText, fgColor);

    viewport()->setPalette(pal);
    setPalette(pal);

    viewport()->repaint();
    repaint();
}

void
GcMultiCalendar::filterChanged()
{
    if (context->isfiltered) setFilter(context->filters);
    else clearFilter();
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
            GcMiniCalendar *mini = new GcMiniCalendar(context, false);
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
