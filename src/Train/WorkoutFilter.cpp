/*
 * Copyright (c) 2023 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "WorkoutFilter.h"

#include <QString>
#include <QDebug>

#include "ModelFilter.h"


extern int WorkoutFilterlex();
extern char *WorkoutFiltertext;

extern void WorkoutFilter_setString(QString);
extern void WorkoutFilter_clearString();

extern int WorkoutFilterparse();

QList<ModelFilter*> workoutModelFilters;
QString workoutModelErrorMsg;


extern QList<ModelFilter*>
parseWorkoutFilter
(QString text, bool &ok, QString &msg)
{
    workoutModelFilters.clear();
    workoutModelErrorMsg.clear();
    WorkoutFilter_setString(text);
    msg = workoutModelErrorMsg;
    ok = (WorkoutFilterparse() == 0);
    if (ok) {
        return workoutModelFilters;
    } else {
        return QList<ModelFilter*>();
    }
}


extern QStringList
workoutFilterCommands
(int numZones)
{
    QStringList filterCommands;
    filterCommands << "duration"
                   << "stress"
                   << "intensity"
                   << "variability"
                   << "ri"
                   << "bikescore"
                   << "svi"
                   << "rating"
                   << "minpower"
                   << "maxpower"
                   << "avgpower"
                   << "isopower"
                   << "xpower"
                   << "power"
                   << "dominantzone"
                   << "lastrun"
                   << "created"
                   << "distance"
                   << "elevation"
                   << "grade";
    for (int i = 0; i < numZones; ++i) {
        filterCommands << QString("z%1").arg(i + 1);
    }
    filterCommands.sort(Qt::CaseInsensitive);
    return filterCommands;
}


extern QString
buildWorkoutFilter
(RideItem * const rideItem)
{
    QString filter;
    QString filename = rideItem->getText("WorkoutFilename", "").trimmed();
    if (! filename.isEmpty()) {
        filter = QString("file \"%1\"").arg(filename);
    } else {
        QHash<QString, int> ergMetrics;
        ergMetrics["duration"] = rideItem->getForSymbol("workout_time");
        ergMetrics["avgpower"] = rideItem->getForSymbol("average_pwer");
        ergMetrics["stress"] = rideItem->getForSymbol("coggan_tss");
        ergMetrics["isopower"] = rideItem->getForSymbol("coggan_np");
        ergMetrics["bikescore"] = rideItem->getForSymbol("skiba_bike_score");
        ergMetrics["xpower"] = rideItem->getForSymbol("skiba_xpower");
        QHash<QString, int> slpMetrics;
        slpMetrics["distance"] = rideItem->getForSymbol("total_distance");
        slpMetrics["elevation"] = rideItem->getForSymbol("elevation_gain");
        for (const QString &key : ergMetrics.keys()) {
            if (ergMetrics[key] == 0) {
                ergMetrics.remove(key);
            }
        }
        for (const QString &key : slpMetrics.keys()) {
            if (slpMetrics[key] == 0) {
                slpMetrics.remove(key);
            }
        }

        if (! filename.isEmpty()) {
        } else if (ergMetrics.count() > 0) {
            bool first = true;
            for (const QString &key : ergMetrics.keys()) {
                if (! first) {
                    filter += ", ";
                }
                if (key == "duration") {
                    filter += QString("%1 %2:%3").arg(key).arg(ergMetrics[key] / 3600).arg(ergMetrics[key] % 3600 / 60, 2, 10, QChar('0'));
                } else {
                    filter += QString("%1 %2").arg(key).arg(ergMetrics[key]);
                }
                first = false;
            }
        } else if (slpMetrics.count() > 0) {
            bool first = true;
            for (const QString &key : slpMetrics.keys()) {
                if (! first) {
                    filter += ", ";
                }
                filter += QString("%1 %2").arg(key).arg(slpMetrics[key]);
                first = false;
            }
        }
    }
    return filter;
}
