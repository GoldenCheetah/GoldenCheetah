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

GcCalendar::GcCalendar(MainWindow *mainWindow) : mainWindow(mainWindow)
{
    setAutoFillBackground(false);
    setContentsMargins(0,0,0,0);

    month = year = 0;

    layout = new QVBoxLayout(this);
    layout->setSpacing(0);

    black.setColor(QPalette::WindowText, QColor(0,0,0,240));
    white.setColor(QPalette::WindowText, QColor(255,255,255,240));
    grey.setColor(QPalette::WindowText, QColor(127,127,127,240));

    // get the model
    fieldDefinitions = mainWindow->rideMetadata()->getFields();
    calendarModel = new GcCalendarModel(this, &fieldDefinitions, mainWindow);
    calendarModel->setSourceModel(mainWindow->listView->sqlModel);

    QFont font;
    font.setPointSize(48);
    dayNumber = new GcLabel("4", this);
    dayNumber->setYOff(-1);
    dayNumber->setAutoFillBackground(false);
    dayNumber->setPalette(white);
    dayNumber->setFont(font);
    dayNumber->setAlignment(Qt::AlignLeft);
    layout->addWidget(dayNumber);

    font.setPointSize(24);
    dayName = new GcLabel("Sunday", this);
    dayName->setYOff(-1);
    dayName->setAutoFillBackground(false);
    dayName->setPalette(white);
    dayName->setFont(font);
    dayName->setAlignment(Qt::AlignLeft);
    layout->addWidget(dayName);

    font.setPointSize(12);
    font.setWeight(QFont::Bold);
    dayDate = new GcLabel("4th January 2012", this);
    dayDate->setAutoFillBackground(false);
    dayDate->setPalette(grey);
    dayDate->setFont(font);
    dayDate->setAlignment(Qt::AlignLeft);
    layout->addWidget(dayDate);

    GcLabel *spacer1 = new GcLabel("", this);
    spacer1->setFixedHeight(20);
    layout->addWidget(spacer1);

    QHBoxLayout *line = new QHBoxLayout;
    line->setSpacing(5);
    layout->addLayout(line);

    GcLabel *spacer2 = new GcLabel("", this);
    spacer2->setFixedHeight(10);
    layout->addWidget(spacer2);

    font.setPointSize(12);
    left = new GcLabel("<", this);
    left->setYOff(1);
    left->setAutoFillBackground(false);
    left->setPalette(white);
    left->setFont(font);
    left->setAlignment(Qt::AlignLeft);
    line->addWidget(left);
    connect (left, SIGNAL(clicked()), this, SLOT(previous()));

    font.setPointSize(12);
    monthName = new GcLabel("January 2012", this);
    monthName->setYOff(1);
    monthName->setAutoFillBackground(false);
    monthName->setPalette(white);
    monthName->setFont(font);
    monthName->setAlignment(Qt::AlignCenter);
    line->addWidget(monthName);

    font.setPointSize(12);
    right = new GcLabel(">", this);
    right->setYOff(1);
    right->setAutoFillBackground(false);
    right->setPalette(white);
    right->setFont(font);
    right->setAlignment(Qt::AlignRight);
    line->addWidget(right);
    connect (right, SIGNAL(clicked()), this, SLOT(next()));

    dayLayout = new QGridLayout;
    dayLayout->setSpacing(1);
    layout->addLayout(dayLayout);
    layout->addStretch();

    font.setWeight(QFont::Normal);

    // Mon
    font.setPointSize(9);
    GcLabel *day = new GcLabel("Mon", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 0, 0);

    // Tue
    day = new GcLabel("Tue", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 0, 1);

    // Wed
    day = new GcLabel("Wed", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 0, 2);

    // Thu
    day = new GcLabel("Thu", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 0, 3);

    // Fri
    day = new GcLabel("Fri", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 0, 4);

    // Sat
    day = new GcLabel("Sat", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 0, 5);

    // Sun
    day = new GcLabel("Sun", this);
    day->setFont(font);
    day->setAutoFillBackground(false);
    day->setPalette(white);
    day->setFont(font);
    day->setAlignment(Qt::AlignCenter);
    dayLayout->addWidget(day, 0, 6);

    signalMapper = new QSignalMapper(this);
    font.setPointSize(11);
    for (int row=1; row<7; row++) {

        for (int col=0; col < 7; col++) {

            GcLabel *d = new GcLabel(QString("%1").arg((row-1)*7+col), this);
            d->setFont(font);
            d->setAutoFillBackground(false);
            d->setPalette(white);
            d->setAlignment(Qt::AlignCenter);
            dayLayout->addWidget(d,row,col);

            // we like squares
            d->setFixedHeight(28);
            d->setFixedWidth(28);

            dayLabels << d;

            if (row== 3 && col == 4) d->setSelected(true);

            connect (d, SIGNAL(clicked()), signalMapper, SLOT(map()));
            signalMapper->setMapping(d, (row-1)*7+col);
        }
    }
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(dayClicked(int)));

    // refresh on these events...
    connect(mainWindow, SIGNAL(rideAdded(RideItem*)), this, SLOT(refresh()));
    connect(mainWindow, SIGNAL(rideDeleted(RideItem*)), this, SLOT(refresh()));
    connect(mainWindow, SIGNAL(configChanged()), this, SLOT(refresh()));
}

void
GcCalendar::refresh()
{
    calendarModel->setMonth(month, year);
    repaint();
}

void
GcCalendar::dayClicked(int i)
{
    if (dayLabels.at(i)->text() == "") return;

    // select a ride if there is one for this one?
    int row = i / 7;
    int col = i % 7;


    QModelIndex p = calendarModel->index(row,col);
    QStringList files = calendarModel->data(p, GcCalendarModel::FilenamesRole).toStringList();

    if (files.count()) // XXX if more than one file cycle through them?
        mainWindow->selectRideFile(QFileInfo(files[0]).fileName());
}

void
GcCalendar::previous()
{
    QList<QDateTime> allDates = mainWindow->metricDB->db()->getAllDates();
    qSort(allDates);

    // begin of month
    QDateTime bom(QDate(year,month,01), QTime(0,0,0));
    for(int i=allDates.count()-1; i>0; i--) {
        if (allDates.at(i) < bom) {

            QDate date = allDates.at(i).date();
            calendarModel->setMonth(date.month(), date.year());

            // find the day in the calendar...
            for (int day=42; day>0;day--) {

                QModelIndex p = calendarModel->index(day/7,day%7);
                QDate heredate = calendarModel->date(p);
                if (date == heredate) {
                    // select this ride...
                    QStringList files = calendarModel->data(p, GcCalendarModel::FilenamesRole).toStringList();
                    if (files.count()) mainWindow->selectRideFile(QFileInfo(files[0]).fileName());
                }
            }
            break;
        }
    }
}

void
GcCalendar::next()
{
    QList<QDateTime> allDates = mainWindow->metricDB->db()->getAllDates();
    qSort(allDates);

    // end of month
    QDateTime eom(QDate(year,month,01).addMonths(1), QTime(00,00,00));
    for(int i=0; i<allDates.count(); i++) {
        if (allDates.at(i) >= eom) {

            QDate date = allDates.at(i).date();
            calendarModel->setMonth(date.month(), date.year());

            // find the day in the calendar...
            for (int day=0; day<42;day++) {

                QModelIndex p = calendarModel->index(day/7,day%7);
                QDate heredate = calendarModel->date(p);
                if (date == heredate) {
                    // select this ride...
                    QStringList files = calendarModel->data(p, GcCalendarModel::FilenamesRole).toStringList();
                    if (files.count()) mainWindow->selectRideFile(QFileInfo(files[0]).fileName());
                }
            }
            break;
        }
    }
}

void
GcCalendar::setRide(RideItem *ride)
{
    QDate when;
    if (ride && ride->ride()) when = ride->dateTime.date();
    else when = QDate::currentDate();

    // refresh the model
    if (when.month() != month || when.year() != year) {
        calendarModel->setMonth(when.month(), when.year());
        year = when.year();
        month = when.month();

        monthName->setText(when.toString("MMMM yyyy"));
    }

    dayNumber->setText(when.toString("d"));
    dayName->setText(when.toString("dddd"));
    dayDate->setText(when.toString("d MMMM yyyy"));

    // now reapply for each row/col of calendar...
    for (int row=0; row<6; row++) {
        for (int col=0; col < 7; col++) {

            GcLabel *d = dayLabels.at(row*7+col);
            QModelIndex p = calendarModel->index(row,col);
            QDate date = calendarModel->date(p);

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
                if (date == when) {
                    d->setSelected(true);
                    d->setPalette(white);
                } else d->setSelected(false);
            }
        }
    }

    repaint();
}

void
GcLabel::paintEvent(QPaintEvent *e)
{
    int x,y,w,l;

    if (bg) {
        // setup a painter and the area to paint
        QPainter painter(this);
        QRect all(0,0,width(),height());
        painter.fillRect(all, bgColor);
    }

    if (selected) {
        QPainter painter(this);
        QRect all(0,0,width(),height());
        painter.fillRect(all, QColor(Qt::black));
    }

    if (xoff || yoff) {
        // save settings
        QPalette p = palette();
        getContentsMargins(&x,&y,&w,&l);

        // adjust for emboss
        setContentsMargins(x+xoff,y+yoff,w,l);
        QPalette r;
        r.setColor(QPalette::WindowText, QColor(0,0,0,160));
        setPalette(r);
        QLabel::paintEvent(e);

        // Now normal
        setContentsMargins(x,y,w,l);
        setPalette(p);
    }

    QLabel::paintEvent(e);
}
