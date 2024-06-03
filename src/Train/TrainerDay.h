/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _Gc_TrainerDay_h
#define _Gc_TrainerDay_h
#include "GoldenCheetah.h"
#include "ErgFile.h"
#include <QNetworkReply>
#include <QUrl>
#include <QEventLoop>
#include <QObject>

struct TrainerDayItem
{
    int             id;
    QString         workoutType,
                    author,
                    name,
                    description;
    int             duration;
    QDateTime       added;
    QJsonDocument   document;
};

// get workout list
class TrainerDay : public QObject
{
    Q_OBJECT
    G_OBJECT

public:

    TrainerDay(QObject *parent = 0);

    QList<TrainerDayItem> &items() { return _items; } // get the list of files available
    QString getWorkout(int id);            // get the file contents for id

private slots:

    void getListFinished(QNetworkReply *reply);

private:
    void getList(); // refresh the db
    QList<TrainerDayItem> _items;

    QString fileContents; // not thread safe!
};
#endif
