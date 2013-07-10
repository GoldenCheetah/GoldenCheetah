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

// bubble config
static const int lineWidth = 3;
static const int spikeHeight = 10;
static const int spikeWidth = 20;
static const int radius = 7;
static const int alpha = 220;
static const int spikeMargin = 40;

#include "GcBubble.h"

#include "Athlete.h"
#include "MainWindow.h"
#include "MetricAggregator.h"
#include "SummaryMetrics.h"
#include "DBAccess.h"
#include "Settings.h"
#include "Units.h"

GcBubble::GcBubble(Context *context) : 
    QWidget(context->mainWindow, Qt::FramelessWindowHint),
    context(context), borderWidth(3), orientation(Qt::Horizontal)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setContentsMargins(0,0,0,0);
    setStyleSheet(QString::fromUtf8(
         "border-color: rgba(0,0,0,128); "
         "color: rgba(255,255,255,255);"
         "font: bold;"
         "background-color: rgba(255, 255, 255, 0);"
    ));


    QSize iconSize(23,23);

    display = new QWidget(this);
    display->setContentsMargins(3,3,3,3); // for the 3px border
    display->setAutoFillBackground(false);
    display->setGeometry(0,0, 200, 134);

    topleft = new QLabel("WCODE", this);
    topmiddle = new QLabel("RIDEDATE", this);
    topright = new QLabel("SPORT", this);

    // sport icons - in case we recognise the sport code
    bike = new QLabel(this);
    bike->setPixmap(QPixmap(":images/IconBike.png"));

    run = new QLabel(this);
    run->setPixmap(QPixmap(":images/IconRun.png"));

    swim = new QLabel(this);
    swim->setPixmap(QPixmap(":images/IconSwim.png"));

    topmiddle2 = new QLabel("TIME", this); // time

    m1 = new QLabel("DURATION", this);
    m1->setAlignment(Qt::AlignLeft);
    m2 = new QLabel("TSS", this);
    m2->setAlignment(Qt::AlignRight);
    m3 = new QLabel("DISTANCE", this);
    m3->setAlignment(Qt::AlignLeft);
    m4 = new QLabel("IF", this);
    m4->setAlignment(Qt::AlignRight);

    alt = new QLabel(this);
    alt->setPixmap(QPixmap(":images/IconAltitude.png"));

    cad = new QLabel(this);
    cad->setPixmap(QPixmap(":images/IconCadence.png"));

    gps = new QLabel(this);
    gps->setPixmap(QPixmap(":images/IconGPS.png"));

    hr = new QLabel(this);
    hr->setPixmap(QPixmap(":images/IconHR.png"));

    power = new QLabel(this);
    power->setPixmap(QPixmap(":images/IconPower.png"));

    speed = new QLabel(this);
    speed->setPixmap(QPixmap(":images/IconSpeed.png"));

    torque = new QLabel(this);
    torque->setPixmap(QPixmap(":images/IconTorque.png"));

    wind = new QLabel(this);
    wind->setPixmap(QPixmap(":images/IconWind.png"));

    temp = new QLabel(this);
    temp->setPixmap(QPixmap(":images/IconTemp.png"));

    // layout display
    QVBoxLayout *displayLayout = new QVBoxLayout(display);
    displayLayout->setContentsMargins(3,3,3,3);
    displayLayout->setSpacing(0);

    // workout code, date, sport, sport icon
    QHBoxLayout *top = new QHBoxLayout;
    top->addWidget(topleft);
    top->addStretch();
    top->addWidget(topmiddle);
    top->addStretch();
    top->addWidget(topright);
    top->addWidget(swim);
    top->addWidget(run);
    top->addWidget(bike);
    displayLayout->addLayout(top);

    // time
    QHBoxLayout *top2 = new QHBoxLayout;
    top2->addStretch();
    top2->addWidget(topmiddle2);
    top2->addStretch();
    displayLayout->addLayout(top2);

    displayLayout->addStretch();

    // metrics
    QHBoxLayout *mid = new QHBoxLayout;
    mid->addStretch();
    mid->addWidget(m1);
    mid->addStretch();
    mid->addWidget(m2);
    mid->addStretch();
    displayLayout->addLayout(mid);
    QHBoxLayout *mid2 = new QHBoxLayout;
    mid2->addStretch();
    mid2->addWidget(m3);
    mid2->addStretch();
    mid2->addWidget(m4);
    mid2->addStretch();
    displayLayout->addLayout(mid2);

    displayLayout->addStretch();

    // icons
    QHBoxLayout *icons = new QHBoxLayout;
    icons->setSpacing(0);
    icons->setContentsMargins(0,0,0,0);
    icons->addStretch();
    icons->addWidget(speed);
    icons->addWidget(power);
    icons->addWidget(hr);
    icons->addWidget(cad);
    icons->addWidget(torque);
    icons->addWidget(alt);
    icons->addWidget(gps);
    icons->addWidget(temp);
    icons->addWidget(wind);
    icons->addStretch();
    displayLayout->addLayout(icons);
}

void
GcBubble::paintEvent(QPaintEvent *)
{

    // Init paint settings
    QPainter painter(this);

    // we need smooth curves
    painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing, true);

    // outline border, adjust to offset, for now we outline
    painter.setBrush(Qt::NoBrush);
    QColor shadow = QColor(0,0,0,alpha);
    QPen shadowPen(shadow);
    shadowPen.setWidth(lineWidth);
    painter.setPen(shadowPen);

    painter.translate(0,1);
    painter.drawPath(path);

    // now draw for real
    painter.translate(0,-1); // adjust to offset, for now we outline

    // contents fill with a linear gradient
    QLinearGradient linearGradient(0, 0, 0, height());
    linearGradient.setColorAt(0.0, QColor(80,80,80, alpha));
    linearGradient.setColorAt(1.0, QColor(0, 0, 0, alpha));
    linearGradient.setSpread(QGradient::PadSpread);
    painter.setBrush(linearGradient);

    // border gray and opaque
    QColor borderColor = QColor(240,240,240,200);
    QPen border = QPen(borderColor);
    border.setWidth(lineWidth);
    painter.setPen(border);

    // draw the background
    painter.drawPath(path);
}

void
GcBubble::setPos(int x, int y, Qt::Orientation orientation) // always uses global co-ords
{
    QPoint here = context->mainWindow->mapFromGlobal(QPoint(x,y));
    this->orientation = orientation;

    // set to as big as possible
    setGeometry(0,0,display->width()+spikeHeight+(lineWidth*2),display->height()+spikeHeight+(lineWidth*2));

    // lets work out the position of the spike
    // is it left or right or top or bottom?
    if (orientation == Qt::Horizontal) {

        // would the window be off mainwindow?
        if ((x+width()) > context->mainWindow->geometry().x()+context->mainWindow->geometry().width()) {
            spikePosition = right;
        } else {
            spikePosition = left;
        }
        setGeometry(0,0,display->width()+spikeHeight+(lineWidth*2),display->height()+(lineWidth*2));

    } else {

        // would the window be off mainwindow?
        if ((y-height()) < (context->mainWindow->geometry().y()+40)) { // +40 for menu and title bar
            spikePosition = top;
        } else {
            spikePosition = bottom;
        }
        setGeometry(0,0,display->width()+(lineWidth*2),display->height()+spikeHeight+(lineWidth*2));
    }


    // lets put the spike in the middle but possibly
    // move it up/down or left right to make the bubble
    // stay visible in mainwindow
    if (orientation == Qt::Horizontal) {

        if (y-(height()/2) < (context->mainWindow->geometry().y()+40)) { // +40 for menu and title bar of mainwindow
            // put it at the top
            if (spikePosition == left) start = QPoint(lineWidth, 40);
            else start = QPoint(width()-lineWidth, 40);

        } else if (y+(height()/2) > (context->mainWindow->geometry().y()+context->mainWindow->geometry().height())) {
            // put it at the bottom
            if (spikePosition == left) start = QPoint(lineWidth, height()-40);
            else start = QPoint(width()-lineWidth, height()-40);

        } else {
            // put it in the middle
            if (spikePosition == left) start = QPoint(lineWidth, height()/2);
            else start = QPoint(width()-lineWidth, height()/2);

        }

    } else { // Qt::Vertical

        if (x-(width()/2) < context->mainWindow->geometry().x()) {
            // put it at the left
            if (spikePosition == top) start = QPoint(40, lineWidth);
            else start = QPoint(40, height()-lineWidth);

        } else if (x+(width()/2) > (context->mainWindow->geometry().x()+context->mainWindow->geometry().width())) {
            // put it at the right
            if (spikePosition == top) start = QPoint(width()-40, lineWidth);
            else start = QPoint(width()-40, height()-lineWidth);

        } else {
            // put it in the middle
            if (spikePosition == top) start = QPoint(width()/2, lineWidth);
            else start = QPoint(width()/2, height()-lineWidth);

        }

    }

    // bubble body
    QRect body;
    switch(spikePosition) {

    case left:
        body = QRect(spikeHeight+lineWidth, lineWidth,
                    display->width(),
                    display->height());
        break;

    case right:
        body = QRect(lineWidth, lineWidth,
                    display->width(),
                    display->height());
        break;

    case top:
        body = QRect(lineWidth, lineWidth+spikeHeight,
                    display->width(),
                    display->height());
        break;
    case bottom:
        body = QRect(lineWidth, lineWidth,
                    display->width(),
                    display->height());
        break;

    }

    display->move(body.x(), body.y());

    // set draw path
    path = QPainterPath(); // reset path
    path.moveTo(start);

    switch (spikePosition) {

    case left:
        path.lineTo(body.x(), start.y()-(spikeWidth/2)); // point to lhs
        path.lineTo(body.topLeft() + QPoint(0, radius));  // to top left corner, before curve
        path.quadTo(body.topLeft(), body.topLeft() + QPoint(radius, 0)); // curve to top

        path.lineTo(body.topRight()-QPoint(radius,0)); // to top right corner, before curve
        path.quadTo(body.topRight(), body.topRight() + QPoint(0, radius)); // curve to rhs

        path.lineTo(body.bottomRight()-QPoint(0, radius)); // to bottom right corner, befor curve
        path.quadTo(body.bottomRight(), body.bottomRight() - QPoint(radius, 0)); // curve to rhs

        path.lineTo(body.bottomLeft()+QPoint(radius, 0)); // to bottom left corner, before curve
        path.quadTo(body.bottomLeft(), body.bottomLeft() - QPoint(0, radius)); // curve to lhs

        path.lineTo(body.x(), start.y()+(spikeWidth/2)); // lhs before back to point
        break;

    case right:
        path.lineTo(body.right(), start.y()+(spikeWidth/2)); // point to rhs

        path.lineTo(body.bottomRight()-QPoint(0, radius)); // to bottom right corner, befor curve
        path.quadTo(body.bottomRight(), body.bottomRight() - QPoint(radius, 0)); // curve to rhs

        path.lineTo(body.bottomLeft()+QPoint(radius, 0)); // to bottom left corner, before curve
        path.quadTo(body.bottomLeft(), body.bottomLeft() - QPoint(0, radius)); // curve to lhs

        path.lineTo(body.topLeft() + QPoint(0, radius));  // to top left corner, before curve
        path.quadTo(body.topLeft(), body.topLeft() + QPoint(radius, 0)); // curve to top

        path.lineTo(body.topRight()-QPoint(radius,0)); // to top right corner, before curve
        path.quadTo(body.topRight(), body.topRight() + QPoint(0, radius)); // curve to rhs

        path.lineTo(body.right(), start.y()-(spikeWidth/2)); // rhs before back to point

        break;

    case top:
        path.lineTo(start.x()+(spikeWidth/2), body.top()); // point to top

        path.lineTo(body.topRight()-QPoint(radius,0)); // to top right corner, before curve
        path.quadTo(body.topRight(), body.topRight() + QPoint(0, radius)); // curve to rhs

        path.lineTo(body.bottomRight()-QPoint(0, radius)); // to bottom right corner, befor curve
        path.quadTo(body.bottomRight(), body.bottomRight() - QPoint(radius, 0)); // curve to rhs

        path.lineTo(body.bottomLeft()+QPoint(radius, 0)); // to bottom left corner, before curve
        path.quadTo(body.bottomLeft(), body.bottomLeft() - QPoint(0, radius)); // curve to lhs

        path.lineTo(body.topLeft() + QPoint(0, radius));  // to top left corner, before curve
        path.quadTo(body.topLeft(), body.topLeft() + QPoint(radius, 0)); // curve to top

        path.lineTo(start.x()-(spikeWidth/2), body.top()); // rhs before back to point
        break;

    case bottom:
        path.lineTo(start.x()-(spikeWidth/2), body.bottom()); // point to top

        path.lineTo(body.bottomLeft()+QPoint(radius, 0)); // to bottom left corner, before curve
        path.quadTo(body.bottomLeft(), body.bottomLeft() - QPoint(0, radius)); // curve to lhs

        path.lineTo(body.topLeft() + QPoint(0, radius));  // to top left corner, before curve
        path.quadTo(body.topLeft(), body.topLeft() + QPoint(radius, 0)); // curve to top

        path.lineTo(body.topRight()-QPoint(radius,0)); // to top right corner, before curve
        path.quadTo(body.topRight(), body.topRight() + QPoint(0, radius)); // curve to rhs

        path.lineTo(body.bottomRight()-QPoint(0, radius)); // to bottom right corner, befor curve
        path.quadTo(body.bottomRight(), body.bottomRight() - QPoint(radius, 0)); // curve to rhs

        path.lineTo(start.x()+(spikeWidth/2), body.bottom()); // rhs before back to point
        break;
    }
    path.lineTo(start);

    // offset the position to avoid
    // causing a mouse enter event onto the bubble(!)
    QPoint offset;
    switch (spikePosition) {
        case left: offset = QPoint(5,0); break;
        case right: offset = QPoint(-5,0); break;
        case top: offset = QPoint(0,5); break;
        case bottom: offset = QPoint(0,-5); break;
    }

    QWidget::move(here-start+offset);
}

void
GcBubble::setText(QString filename)
{
        SummaryMetrics metrics = context->athlete->metricDB->getAllMetricsFor(filename);
        useMetricUnits = context->athlete->useMetricUnits;
    

        //
        // Workout code
        //
        QString wcode = metrics.getText("Workout Code", "");
        if (wcode != "") {
            topleft->setText(wcode);
            topleft->show();
        } else {
            topleft->hide();
        }


        //
        // Sport
        //
        QString sport = metrics.getText("Sport", "");
        if (sport != "") {
            topright->setText(sport);
            topright->show();
        } else {
            topright->hide();
        }

        swim->hide();
        run->hide();
        bike->hide();

        // icons instead of text?
        if (sport == "Bike") { bike->show(); topright->hide(); }
        if (sport == "Swim") { swim->show(); topright->hide(); }
        if (sport == "Run") { run->show(); topright->hide(); }

        //
        // Ride Date
        // 
        QDateTime rideDate = metrics.getRideDate();
        topmiddle->setText(rideDate.toString("MMM d, yyyy")); // same format as ride list
        topmiddle2->setText(rideDate.toString("h:mm AP"));

        // Metrics 1,2,3,4
        m1->setText(QTime(0,0,0,0).addSecs(metrics.getForSymbol("workout_time")).toString("hh:mm:ss")); //duration
        
	if (useMetricUnits) { 
	  m3->setText(QString("%1 km").arg(metrics.getForSymbol("total_distance"), 0, 'f', 2));
	  } else {
	  m3->setText(QString("%1 mi").arg(metrics.getForSymbol("total_distance")*MILES_PER_KM, 0, 'f', 2));
	  } //distance

        m2->setText(QString("%1 TSS").arg(metrics.getForSymbol("coggan_tss"), 0, 'f', 0));
        m4->setText(QString("%1 IF").arg(metrics.getForSymbol("coggan_if"), 0, 'f', 3));

        // Icons
        QString data = metrics.getText("Data", "");

        if (data.contains("P")) power->show();
        else power->hide();
        if (data.contains("S")) speed->show();
        else speed->hide();
        if (data.contains("H")) hr->show();
        else hr->hide();
        if (data.contains("C")) cad->show();
        else cad->hide();
        if (data.contains("N")) torque->show();
        else torque->hide();
        if (data.contains("A")) alt->show();
        else alt->hide();
        if (data.contains("G")) gps->show();
        else gps->hide();
        if (data.contains("E")) temp->show();
        else temp->hide();
        if (data.contains("W")) wind->show();
        else wind->hide();
}
