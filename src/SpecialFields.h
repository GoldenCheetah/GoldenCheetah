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

#include <QApplication>
#include <QStringList>
#include <QStringListModel>
#include "RideMetric.h"

class SpecialFields
{
    Q_DECLARE_TR_FUNCTIONS(SpecialFields)

    public:
        SpecialFields();

        enum FieldType { User, Special, Metric };
        FieldType fieldType(QString &) const;         // what type is it?
        bool isUser(QString&) const;                  // is this a real user defined field?
        bool isSpecial(QString&) const;               // is this a special field name?
        bool isMetric(QString&) const;                // is this a metric override?

        QString makeTechName(QString &) const;        // return a SQL friendly name
        QString metricSymbol(QString &) const;        // return symbol for user friendly name
        const RideMetric *rideMetric(QString&) const; // return metric ptr for user friendly name
#ifdef ENABLE_METRICS_TRANSLATION
        QString displayName(QString &) const;         // return display (localized) name for name
        QString internalName(QString) const;        // return internal (english) Name for display
#else
        const QStringList &names() const { return names_; }
#endif

        // the config pane uses the model
        QStringListModel *model() { return model_; }

    private:
#ifdef ENABLE_METRICS_TRANSLATION
        QMap<QString, QString> namesmap; // Map Internal (english) name to external (Localized) name
#else
        QStringList names_;
#endif
        QMap<QString, const RideMetric *> metricmap;
        QStringListModel *model_;
};

#ifdef ENABLE_METRICS_TRANSLATION
class SpecialTabs
{
    Q_DECLARE_TR_FUNCTIONS(SpecialTabs)

    public:
        SpecialTabs();
        QString displayName(QString &) const;       // return display (localized)
        QString internalName(QString) const;        // return internal (english)

    private:
        QMap<QString, QString> namesmap; // Map Internal (english) name to external (Localized) name
};
#endif

#endif
