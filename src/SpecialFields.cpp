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

#include <QTextEdit>

SpecialFields::SpecialFields()
{
#ifdef ENABLE_METRICS_TRANSLATION
    namesmap.insert("Start Date", tr("Start Date"));                 // linked to RideFile::starttime
    namesmap.insert("Start Time", tr("Start Time"));                 // linked to RideFile::starttime
    namesmap.insert("Workout Code", tr("Workout Code"));             // in WKO and possibly others
    namesmap.insert("Sport", tr("Sport"));                           // in WKO and possible others
    namesmap.insert("Objective", tr("Objective"));                   // in WKO as "goal" nad possibly others
    namesmap.insert("Notes", tr("Notes"));                           // linked to MainWindow::rideNotes
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
    namesmap.insert("Change History", tr("Change History"));         // set by RideFileCommand
#else
    names_  << "Start Date"    // linked to RideFile::starttime
            << "Start Time"    // linked to RideFile::starttime
            << "Workout Code" // in WKO and possibly others
            << "Sport"        // in WKO and possible others
            << "Objective"    // in WKO as "goal" nad possibly others
            << "Notes"        // linked to MainWindow::rideNotes
            << "Keywords"     // extracted from Notes / used for highlighting calendar
            << "Recording Interval" // linked to RideFile::recIntSecs
            << "Weight"       // in WKO and possibly others
            << "Device"       // RideFile::devicetype
            << "Device Info"  // in WKO and TCX and possibly others
            << "Dropouts"     // calculated from source data by FixGaps
            << "Dropout Time" // calculated from source data vy FixGaps
            << "Spikes"       // calculated from source data by FixSpikes
            << "Spike Time"   // calculated from source data by FixSpikes
            << "Torque Adjust" // the last torque adjust applied
            << "Filename"      // set by the rideFile reader
            << "Change History" // set by RideFileCommand
            ;
#endif

    // now add all the metric fields (for metric overrides)
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++) {
        const RideMetric *add = factory.rideMetric(factory.metricName(i));
#ifdef ENABLE_METRICS_TRANSLATION
        QTextEdit processHTML(add->name());
        QTextEdit processHTMLinternal(add->internalName());
        // add->internalName() used for compatibility win metadata.xml, could be replaced by factory.metricName(i) or add->symbol()
        namesmap.insert(processHTMLinternal.toPlainText(), processHTML.toPlainText());
        metricmap.insert(processHTMLinternal.toPlainText(), add);
#else
        QTextEdit processHTML(add->name());
        names_ << processHTML.toPlainText();
        metricmap.insert(processHTML.toPlainText(), add);
#endif
    }

    model_ = new QStringListModel;
#ifdef ENABLE_METRICS_TRANSLATION
    model_->setStringList(namesmap.keys());
#else
    model_->setStringList(names_);
#endif
}

bool
SpecialFields::isSpecial(QString &name) const
{
#ifdef ENABLE_METRICS_TRANSLATION
    return namesmap.contains(name);
#else
    return names_.contains(name);
#endif
}

bool
SpecialFields::isUser(QString &name) const
{
#ifdef ENABLE_METRICS_TRANSLATION
    return !namesmap.contains(name);
#else
    return !names_.contains(name);
#endif
}

bool
SpecialFields::isMetric(QString &name) const
{
    if (metricSymbol(name) != "") return true;
    else return false;
}

QString
SpecialFields::makeTechName(QString &name) const
{
    // strip spaces and only keep alpha values - everything else
    // becomes an underscore
    QString s = name;
    return s.replace(QRegExp("[^0-9A-Za-z]"), "_");
}

QString
SpecialFields::metricSymbol(QString &name) const
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

#ifdef ENABLE_METRICS_TRANSLATION

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

#endif // ENABLE_METRICS_TRANSLATION
