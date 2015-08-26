/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_UserData_h
#define _GC_UserData_h 1

#include "GoldenCheetah.h"

#include <QColor>
#include <QString>
#include <QXmlDefaultHandler>

// working with ride data
#include "RideItem.h"
#include "RideFile.h"

// working with metrics and formulas
#include "Specification.h"
#include "DataFilter.h"
#include "RideMetric.h"

class UserDataParser;

// provide API for working with user defined data series
class UserData : public QObject {

    Q_OBJECT

    Q_PROPERTY(QString settings READ settings WRITE setSettings USER true)
    Q_PROPERTY(RideItem* rideItem READ getRideItem WRITE setRideItem USER true)

    public:

        UserData();
        UserData(QString settings);
        UserData(QString name, QString units, QString formula, QColor color);
        ~UserData();

    public slots:

        // allow user to maintain, returns true if changed
        bool edit();

        // get and set properties
        QString settings() const;
        void setSettings(QString);
        RideItem* getRideItem() const;
        void setRideItem(RideItem*);

    friend class ::UserDataParser;
    protected:

        // user maintained attributes
        QString name, 
                units,
                formula;
        QColor color;

        // Ride item we are working on
        RideItem *rideItem;
};

class UserDataParser : public QXmlDefaultHandler
{

public:
    UserDataParser(UserData *here) : here(here) {}

    // where to put the results
    UserData *here;

    // unmarshall
    bool startDocument();
    bool endDocument();
    bool endElement( const QString&, const QString&, const QString &qName );
    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );
    bool characters( const QString& str );

};
#endif
