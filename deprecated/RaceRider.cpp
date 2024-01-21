/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "RaceRider.h"
#include "GoldenClient.h"

RaceRider::RaceRider(QWidget *parent, QColor color, QString name, QString id) :
           QFrame(parent), me(name), id(id)
{
    QPalette pal = palette();
    //QColor RGB = color.convertTo(QColor::Rgb);
    pal.setColor(QPalette::Normal, QPalette::Window, color);
    pal.setColor(QPalette::Normal, QPalette::Base, color);
    pal.setColor(QPalette::Normal, QPalette::Button, color);
    pal.setColor(QPalette::Normal, QPalette::Text, Qt::black);
    pal.setColor(QPalette::Normal, QPalette::ButtonText, Qt::black);
    setPalette(pal);

    setFrameStyle(QFrame::Box | QFrame::Raised);
    setLineWidth(1);
    setContentsMargins(4,4,4,4);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0,0,0,0);

    // the labels and displays!
    nameLabel = new QLabel(name);
    nameLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    nameLabel->setFixedHeight(20);

    powerLabel = new QLabel(tr("Watts"));
    powerLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    powerLabel->setFixedHeight(16);

    powerLCD   = new QLCDNumber;
    powerLCD->setSegmentStyle(QLCDNumber::Filled);
    powerLCD->setAutoFillBackground(true);
    powerLCD->setPalette(pal);

    heartrateLabel = new QLabel(tr("HeartRate"));
    heartrateLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    heartrateLabel->setFixedHeight(16);
    heartrateLCD   = new QLCDNumber;
    heartrateLCD->setSegmentStyle(QLCDNumber::Filled);
    heartrateLCD->setAutoFillBackground(true);
    heartrateLCD->setPalette(pal);

    speedLabel = new QLabel(tr("Speed"));
    speedLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    speedLabel->setFixedHeight(16);
    speedLCD   = new QLCDNumber;
    speedLCD->setSegmentStyle(QLCDNumber::Filled);
    speedLCD->setAutoFillBackground(true);
    speedLCD->setPalette(pal);

    cadenceLabel = new QLabel(tr("Cadence"));
    cadenceLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    cadenceLabel->setFixedHeight(16);
    cadenceLCD   = new QLCDNumber;
    cadenceLCD->setAutoFillBackground(true);
    cadenceLCD->setSegmentStyle(QLCDNumber::Filled);
    cadenceLCD->setPalette(pal);

    distanceLabel = new QLabel(tr("Distance"));
    distanceLabel->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    distanceLabel->setFixedHeight(16);
    distanceLCD   = new QLCDNumber;
    distanceLCD->setSegmentStyle(QLCDNumber::Filled);
    distanceLCD->setAutoFillBackground(true);
    distanceLCD->setPalette(pal);

    // add to the layout
    mainLayout->addWidget(nameLabel);

    mainLayout->addWidget(powerLabel);
    mainLayout->addWidget(powerLCD);

    mainLayout->addWidget(heartrateLabel);
    mainLayout->addWidget(heartrateLCD);

    mainLayout->addWidget(speedLabel);
    mainLayout->addWidget(speedLCD);

    mainLayout->addWidget(cadenceLabel);
    mainLayout->addWidget(cadenceLCD);

    mainLayout->addWidget(distanceLabel);
    mainLayout->addWidget(distanceLCD);

    // Labels
    mainLayout->setStretch(1, 1);
    mainLayout->setStretch(3, 1);
    mainLayout->setStretch(5, 1);
    mainLayout->setStretch(7, 1);
    mainLayout->setStretch(9, 1);

    mainLayout->setStretch(2, 2);
    mainLayout->setStretch(4, 2);
    mainLayout->setStretch(6, 2);
    mainLayout->setStretch(8, 2);
    mainLayout->setStretch(10, 2);

    setLayout(mainLayout);
    setFixedWidth(120);

}

void
RaceRider::telemetryReceived(RaceStatus current)
{
    if (!current.riders_status.contains(id)) return;

    RiderData mydata = current.riders_status.value(id);
    powerLCD->display(mydata.power_watts);
    speedLCD->display(mydata.speed_kph);
    cadenceLCD->display(mydata.cadence_rpm);
    distanceLCD->display(mydata.distance_km);
    heartrateLCD->display(mydata.heartrate_bpm);
}
