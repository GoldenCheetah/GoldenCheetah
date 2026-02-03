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
#include "SpecialFields.h"
#include "DataFilter.h"

#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <QMutex>
#include <QWebEngineProfile>


static QList<Context*> _contexts;

GlobalContext::GlobalContext()
{
    rideMetadata = NULL;
    colorEngine = NULL;
    readConfig(0); // don't reread user metrics just yet
}

GlobalContext*
GlobalContext::context()
{
    // Meyer's singleton pattern
    static GlobalContext globalContext; // Guaranteed thread-safe initialization
    return &globalContext;
}

void
GlobalContext::notifyConfigChanged(qint32 state)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // read it in - global only
    readConfig(state);

    // tell global widgets, like the sidebar
    emit configChanged(state);

    // tell every valid context, for athlete specific widgets/charts
    foreach(Context *p, _contexts)
        if (Context::isValid(p))
            p->notifyConfigChanged(state);

    QApplication::restoreOverrideCursor();
}

void
GlobalContext::readConfig(qint32 state)
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
    SpecialFields::getInstance().reloadFields();

    if (state & CONFIG_USERMETRICS)  userMetricsConfigChanged();

    // watch config changes- we do it for ridemetadata since it cannot connect
    // whilst we are being instantiated- and it gets deleted on each
    // change to configuration- so needs to be redone every time we reset
    connect(this, SIGNAL(configChanged(qint32)), rideMetadata, SLOT(configChanged(qint32)));
}

void
GlobalContext::userMetricsConfigChanged()
{
    // read em in...
    QString metrics(gcroot + "/usermetrics.xml");
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


    // change the schema version, this may trigger metrics recomputation
    UserMetricSchemaVersion = RideMetric::userMetricFingerprint(_userMetrics);

    // update metric factory deleting originals
    RideMetricFactory::instance().removeUserMetrics();

    // now add user metrics
    foreach(UserMetricSettings m, _userMetrics) {
        RideMetricFactory::instance().addMetric(UserMetric(_contexts.at(0), m));
    }

    // refresh SpecialFields to include updated user metrics
    SpecialFields::getInstance().reloadFields();
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

    connect(this, SIGNAL(loadProgress(QString, double)), mainWindow, SLOT(loadProgress(QString, double)));

#ifdef GC_HAS_CLOUD_DB
    cdbChartListDialog = NULL;
    cdbUserMetricListDialog = NULL;
#endif

    // WebEngineProfile - cookies and storage
    webEngineProfile = new QWebEngineProfile("Default", this);
    webEngineProfile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

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
    QApplication::setOverrideCursor(Qt::WaitCursor);
    emit configChanged(state);
    QApplication::restoreOverrideCursor();
}

