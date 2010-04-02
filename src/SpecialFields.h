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

#ifndef _GC_SpecialFields_h
#define _GC_SpecialFields_h

#include <QStringList>
#include <QStringListModel>
#include "RideMetric.h"

class SpecialFields
{
    public:
        SpecialFields();

        enum FieldType { User, Special, Metric };
        FieldType fieldType(QString &) const;         // what type is it?
        bool isUser(QString&) const;                  // is this a real user defined field?
        bool isSpecial(QString&) const;               // is this a special field name?
        bool isMetric(QString&) const;                // is this a metric override?

        QString makeTechName(QString &) const;        // return a SQL friendly name
        QString metricSymbol(QString &) const;        // return symbol for user friendly name
        const RideMetric *rideMetric(QString&) const; // retuen metric ptr for user friendly name

        // the config pane uses the model
        const QStringList &names() const { return names_; }
        QStringListModel *model() { return model_; }

    private:
        QStringList names_;
        QMap<QString, const RideMetric *> metricmap;
        QStringListModel *model_;
};

#endif
