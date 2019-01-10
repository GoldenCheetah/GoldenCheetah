/*
 * Copyright (c) 2018 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_Estimator_h
#define GC_Estimator_h

#include "Athlete.h"
#include "Context.h"
#include "RideCache.h"
#include "PDModel.h"

#include <QThread>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QScrollArea>
#include <QPushButton>

class Performance {

    public:
        Performance(QDate wc, double power, double duration, double powerIndex) :
            weekcommencing(wc), power(power), duration(duration), powerIndex(powerIndex), submaximal(false) {}

        QDate when, weekcommencing;
        double power, duration, powerIndex;
        bool submaximal; // set by the filter, user can choose to include.

        double x; // different units, but basically when as a julian day
};

class Banister;
class Estimator : public QThread {

    Q_OBJECT

    public:

        Estimator(Context *);
        ~Estimator();

        // perform calculation in thread
        void run();

        // halt the thread
        void stop();

        // cancel any pending/running and kick off with 15 sec delay
        void refresh();

        // filter marks performances as submax
        QList<Performance> filter(QList<Performance>);

    public slots:

        // setup and run estimators
        void calculate();

        // get a performance for a given day
        Performance getPerformanceForDate(QDate date);

    protected:

        friend class ::Athlete;
        friend class ::Banister;

        Context *context;
        QMutex lock;
        QList<PDEstimate> estimates;
        QList<Performance> performances;
        QVector<RideItem*> rides; // worklist
        QTimer singleshot;

        bool abort;
};

#endif
