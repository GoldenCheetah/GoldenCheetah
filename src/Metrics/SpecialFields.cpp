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

#include "SpecialFields.h"
#include "RideMetric.h"
#include "Utils.h"

#include <QTextEdit>

SpecialFields::SpecialFields()
{
    namesmap.insert("Start Date", tr("Start Date"));                 // linked to RideFile::starttime
    namesmap.insert("Start Time", tr("Start Time"));                 // linked to RideFile::starttime
    namesmap.insert("Identifier", tr("Identifier"));                 // linked to RideFile::id
    namesmap.insert("Recording Interval", tr("Recording Interval")); // linked to RideFile::recIntSecs
    namesmap.insert("Change History", tr("Change History"));         // set by RideFileCommand
    namesmap.insert("Calendar Text", "Calendar Text");               // set by openRideFile and rideMetadata DO NOT TRANSLATE
    namesmap.insert("Source Filename", tr("Source Filename"));       // set by openRideFile
    namesmap.insert("Route", tr("Route"));                           // GPS map Route tag
    namesmap.insert("Sport", tr("Sport"));                           // Sport Code
    namesmap.insert("Workout Code", tr("Workout Code"));             // Workout Code
    namesmap.insert("Workout Title", tr("Workout Title"));           // Workout Title
    namesmap.insert("Weight", tr("Weight"));                         // Weight tag
    namesmap.insert("RPE", tr("RPE"));                               // RPE tag
    namesmap.insert("Objective", tr("Objective"));                   // Objective tag
    namesmap.insert("Keywords", tr("Keywords"));                     // Keywords tag
    namesmap.insert("Equipment", tr("Equipment"));                   // Equipment tag
    namesmap.insert("Device", tr("Device"));                         // Device tag
    namesmap.insert("Device Info", tr("Device Info"));               // Device Info tag
    namesmap.insert("Pool Length", tr("Pool Length"));               // Pool Length tag

    // now add all the metric fields (for metric overrides)
    const RideMetricFactory &factory = RideMetricFactory::instance();

    for (int i=0; i<factory.metricCount(); i++) {
        const RideMetric *add = factory.rideMetric(factory.metricName(i));

        QString name(Utils::unprotect(add->name()));
        QString internal(Utils::unprotect(add->internalName()));
        // add->internalName() used for compatibility win metadata.xml, could be replaced by factory.metricName(i) or add->symbol()
        namesmap.insert(internal, name);
        metricmap.insert(internal, add);
    }

    model_ = new QStringListModel;
    model_->setStringList(namesmap.keys());
}

bool
SpecialFields::isSpecial(QString &name) const
{
    return namesmap.contains(name);
}

bool
SpecialFields::isUser(QString &name) const
{
    return !namesmap.contains(name);
}

bool
SpecialFields::isMetric(QString &name) const
{
    if (metricSymbol(name) != "") return true;
    else return false;
}

QString
SpecialFields::makeTechName(QString name) const
{
    // strip spaces and only keep alpha values - everything else
    // becomes an underscore
    QString s = name;
    return s.replace(QRegExp("[^0-9A-Za-z]"), "_");
}

QString
SpecialFields::metricSymbol(QString name) const
{
    // return technical name for metric long name
    const RideMetric *metric = metricmap.value(name, NULL);
    if (metric) return metric->symbol();
    else return ("");
}

const RideMetric *
SpecialFields::rideMetric(QString&name) const
{
    return metricmap.value(name, NULL);
}

QString
SpecialFields::displayName(QString &name) const
{
    // return localized name for display
    if (namesmap.contains(name)) return namesmap.value(name);
    else return(name);
}

QString
SpecialFields::internalName(QString displayName) const
{
    // return internal name for storage
    QMapIterator<QString, QString> i(namesmap);
    while (i.hasNext()) {
        i.next();
        if (i.value() == displayName) return i.key();
    }
    return(displayName);
}

SpecialTabs::SpecialTabs()
{
    namesmap.insert("Workout", tr("Workout"));
    namesmap.insert("Notes", tr("Notes"));
    namesmap.insert("Metric", tr("Metric"));
    namesmap.insert("Extra", tr("Extra"));
    namesmap.insert("Device", tr("Device"));
    namesmap.insert("Athlete", tr("Athlete"));
}

QString
SpecialTabs::displayName(QString &internalName) const
{
    // return localized name for display
    if (namesmap.contains(internalName)) return namesmap.value(internalName);
    else return(internalName);
}

QString
SpecialTabs::internalName(QString displayName) const
{
    // return internal name for storage
    QMapIterator<QString, QString> i(namesmap);
    while (i.hasNext()) {
        i.next();
        if (i.value() == displayName) return i.key();
    }
    return(displayName);
}

