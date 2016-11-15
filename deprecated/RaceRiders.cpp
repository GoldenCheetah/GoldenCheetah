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

#include "RaceRiders.h"
#include "RaceRider.h"
#include "GoldenClient.h"

RaceRiders::RaceRiders(QWidget *parent, TrainTool *trainTool) : QFrame(parent), trainTool(trainTool)
{
    setLineWidth(1);
    setFrameStyle(QFrame::Box | QFrame::Raised);
    setContentsMargins(0,0,0,0);

    racerLayout = new QHBoxLayout;
    racerLayout->setContentsMargins(0,0,0,0);
    racerLayout->setSpacing(0);
    racerLayout->addStretch();
    setLayout(racerLayout);

    connect(trainTool, SIGNAL(raceStandings(RaceStatus)), this, SLOT(telemetryReceived(RaceStatus)));
}

void
RaceRiders::telemetryReceived(RaceStatus current)
{
    if (current.membership_changed == true) {

        // iterate over the membership creating a new
        // racer for anyone we don't already have
        QMapIterator<QString,RiderData> i(current.riders_status);
        i.toFront();
        while (i.hasNext()) {
            i.next();
            if (!racers.contains(i.key())) {
                QColor color;
                color.setHsv (racers.count() * (255/8), 255,255);
                RaceRider *newone = new RaceRider(this, color, i.value().rider_name, i.key());
                connect(trainTool, SIGNAL(raceStandings(RaceStatus)), newone,
                                   SLOT(telemetryReceived(RaceStatus)));
                racerLayout->insertWidget(0, newone);
                racers.insert(i.key(), newone);
            }
        }

        // now remove any that are in our map but not in the current
        // membership
        QMapIterator <QString, RaceRider *> j(racers);
        j.toFront();
        while (j.hasNext()) {
            j.next();
            if (!current.riders_status.contains(j.key())) {
                racerLayout->removeWidget(j.value());
                delete j.value();
                racers.remove(j.key());
            }
        }
     }
}
