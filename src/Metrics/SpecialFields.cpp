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
    namesmap.insert("Workout Code", tr("Workout Code"));             // in WKO and possibly others
    namesmap.insert("Sport", tr("Sport"));                           // in WKO and possible others
    namesmap.insert("Objective", tr("Objective"));                   // in WKO as "goal" nad possibly others
    namesmap.insert("Summary", tr("Summary"));                       // embeds the RideSummary widget
    namesmap.insert("Notes", tr("Notes"));                           // linked to Context::rideNotes
    namesmap.insert("Keywords", tr("Keywords"));                     // extracted from Notes / used for highlighting calendar
    namesmap.insert("Recording Interval", tr("Recording Interval")); // linked to RideFile::recIntSecs
    namesmap.insert("Weight", tr("Weight"));                         // in WKO and possibly others
    namesmap.insert("Device", tr("Device"));                         // RideFile::devicetype
    namesmap.insert("Device Info", tr("Device Info"));               // in WKO and TCX and possibly others
    namesmap.insert("Dropouts", tr("Dropouts"));                     // calculated from source data by FixGaps
    namesmap.insert("Dropout Time", tr("Dropout Time"));             // calculated from source data vy FixGaps
    namesmap.insert("Spikes", tr("Spikes"));                         // calculated from source data by FixSpikes
    namesmap.insert("Spike Time", tr("Spike Time"));                 // calculated from source data by FixSpikes
    namesmap.insert("Torque Adjust", tr("Torque Adjust"));           // the last torque adjust applied
    namesmap.insert("Filename", tr("Filename"));                     // set by the rideFile reader
    namesmap.insert("Year", tr("Year"));                             // set by the rideFile reader
    namesmap.insert("Change History", tr("Change History"));         // set by RideFileCommand
    namesmap.insert("Calendar Text", "Calendar Text");               // set by openRideFile and rideMetadata DO NOT TRANSLATE
    namesmap.insert("Data", tr("Data"));                             // set by openRideFile for areDataPresent
    namesmap.insert("Lean Mass", tr("Lean Mass"));                   // measure
    namesmap.insert("Fat Mass", tr("Fat Mass"));                     // measure
    namesmap.insert("Fat Ratio", tr("Fat Ratio"));                   // measure
    namesmap.insert("Height", tr("Height"));                         // measure
    namesmap.insert("BMI", tr("BMI"));                               // measure
    namesmap.insert("File Format", tr("File Format"));               // set by openRideFile
    namesmap.insert("Athlete", tr("Athlete"));                       // set by openRideFile
    namesmap.insert("Year", tr("Year"));                             // set by openRideFile
    namesmap.insert("Month", tr("Month"));                            // set by openRideFile
    namesmap.insert("Weekday", tr("Weekday"));                       // set by openRideFile
    namesmap.insert("Source Filename", tr("Source Filename"));       // set by openRideFile
    namesmap.insert("Route", tr("Route"));                           // GPS map Route tag
    namesmap.insert("RPE", tr("RPE"));                               // used by Session RPE metric


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

