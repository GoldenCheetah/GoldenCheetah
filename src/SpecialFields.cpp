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
            ;

    // now add all the metric fields (for metric overrides)
    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i=0; i<factory.metricCount(); i++) {
        const RideMetric *add = factory.rideMetric(factory.metricName(i));
        QTextEdit processHTML(add->name());
        names_ << processHTML.toPlainText();
        metricmap.insert(processHTML.toPlainText(), add);
    }

    model_ = new QStringListModel;
    model_->setStringList(names_);
}

bool
SpecialFields::isSpecial(QString &name) const
{
    return names_.contains(name);
}

bool
SpecialFields::isUser(QString &name) const
{
    return !names_.contains(name);
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
