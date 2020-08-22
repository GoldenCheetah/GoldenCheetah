/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#include "Context.h"
#include "Settings.h"
#include "Athlete.h"
#include "RideMetadata.h"

#include "RideMetric.h"
#include "NavigationModel.h"
#include "UserMetricSettings.h"
#include "UserMetricParser.h"
#include "DataFilter.h"

#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QMutex>

// singleton
static GlobalContext *globalContext = NULL;
static QList<Context*> _contexts;
GlobalContext::GlobalContext()
{
    rideMetadata = NULL;
    colorEngine = NULL;
    readConfig();
}

void
GlobalContext::readConfig()
{
    if (rideMetadata) {
        delete rideMetadata;
        delete colorEngine;
    }

    // metric / non-metric
    QVariant unit = appsettings->value(NULL, GC_UNIT, GC_UNIT_METRIC);
    if (unit == 0) {
        // Default to system locale
        unit = QLocale::system().measurementSystem() == QLocale::MetricSystem ? GC_UNIT_METRIC : GC_UNIT_IMPERIAL;
        appsettings->setValue(GC_UNIT, unit);
    }
    useMetricUnits = (unit.toString() == GC_UNIT_METRIC);

    // redo
    rideMetadata = new RideMetadata(NULL);
    colorEngine = new ColorEngine(this);
    specialFields = SpecialFields();

    // watch config changes
    connect(this, SIGNAL(configChanged(qint32)), rideMetadata, SLOT(configChanged(qint32)));
}

GlobalContext *GlobalContext::context()
{
    if (globalContext == NULL) globalContext = new GlobalContext();
    return globalContext;
}

bool Context::isValid(Context *p) { return p != NULL &&_contexts.contains(p); }
Context::Context(MainWindow *mainWindow): mainWindow(mainWindow)
{
    ride = NULL;
    workout = NULL;
    videosync = NULL;
    isfiltered = ishomefiltered = false;
    isCompareIntervals = isCompareDateRanges = false;
    isRunning = isPaused = false;

#ifdef GC_HAS_CLOUD_DB
    cdbChartListDialog = NULL;
    cdbUserMetricListDialog = NULL;
#endif

    _contexts.append(this);
}

Context::~Context()
{
    int i=_contexts.indexOf(this);
    if (i >= 0) _contexts.removeAt(i);
}

void 
Context::notifyCompareIntervals(bool state) 
{ 
    isCompareIntervals = state; 
    emit compareIntervalsStateChanged(state); 
}

void 
Context::notifyCompareIntervalsChanged() 
{
    if (isCompareIntervals) {
        emit compareIntervalsChanged(); 
    }
}

void 
Context::notifyCompareDateRanges(bool state)
{
    isCompareDateRanges = state;
    emit compareDateRangesStateChanged(state); 
}

void 
Context::notifyCompareDateRangesChanged()
{ 
    if (isCompareDateRanges) {
        emit compareDateRangesChanged(); 
    }
}

void
Context::notifyConfigChanged(qint32 state)
{
    //if (state & CONFIG_ZONES) qDebug()<<"Zones config changed!";
    //if (state & CONFIG_ATHLETE) qDebug()<<"Athlete config changed!";
    //if (state & CONFIG_GENERAL) qDebug()<<"General config changed!";
    //if (state & CONFIG_PMC) qDebug()<<"PMC constants changed!";
    //if (state & CONFIG_UNITS) qDebug()<<"Unit of Measure config changed!";
    //if (state & CONFIG_APPEARANCE) qDebug()<<"Appearance config changed!";
    //if (state & CONFIG_NOTECOLOR) qDebug()<<"Note color config changed!";
    //if (state & CONFIG_FIELDS) qDebug()<<"Metadata config changed!";
    if (state & CONFIG_USERMETRICS) {
        userMetricsConfigChanged();

        // reset special fields (e.g. metric overrides)
        GlobalContext::context()->specialFields = SpecialFields();

    }
    GlobalContext::context()->notifyConfigChanged(state);
}

void 
Context::userMetricsConfigChanged()
{

    // read em in...
    QString metrics = QString("%1/../usermetrics.xml").arg(athlete->home->root().absolutePath());
    if (QFile(metrics).exists()) {

        QFile metricfile(metrics);
        QXmlInputSource source(&metricfile);
        QXmlSimpleReader xmlReader;
        UserMetricParser handler;

        xmlReader.setContentHandler(&handler);
        xmlReader.setErrorHandler(&handler);

        // parse and get return values
        xmlReader.parse(source);
        _userMetrics = handler.getSettings();
        UserMetric::addCompatibility(_userMetrics);
    }


    // change the schema version
    quint16 changed = RideMetric::userMetricFingerprint(_userMetrics);

    if (UserMetricSchemaVersion != changed) {

        // we'll fix it
        UserMetricSchemaVersion = changed;

        // update metric factory deleting originals
        RideMetricFactory::instance().removeUserMetrics();
    
        // now add initial metrics -- what about multiple contexts (?) XXX
        foreach(UserMetricSettings m, _userMetrics) {
            RideMetricFactory::instance().addMetric(UserMetric(this, m));
        }

        // tell eveyone else to compute metrics...
        foreach(Context *x, _contexts)
            if (x != this)
                x->notifyConfigChanged(CONFIG_USERMETRICS);
    }
}
