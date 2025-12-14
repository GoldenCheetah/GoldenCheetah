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
#include "MainWindow.h"
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

// singleton
static GlobalContext *globalContext = NULL;
static QList<Context*> _contexts;

GlobalContext::GlobalContext()
{
    rideMetadata = NULL;
    colorEngine = NULL;
    readConfig(0); // don't reread user metrics just yet
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

GlobalContext *GlobalContext::context()
{
    if (globalContext == NULL) globalContext = new GlobalContext();
    return globalContext;
}

bool Context::isValid(Context *p) { return p != NULL &&_contexts.contains(p); }

Context::Context(MainWindow *mainWindow): DataContext(mainWindow), mainWindow(mainWindow)
{
    isfiltered = ishomefiltered = false;
    showSidebar = showLowbar = showToolbar = true;

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
Context::notifyConfigChanged(qint32 state)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    emit configChanged(state);
    QApplication::restoreOverrideCursor();
}

#include "RealtimeData.h"
#include "ErgFile.h"
#include <QStatusBar>

// API to access MainWindow functionality without exposing the pointer
QWidget *Context::mainWidget() const { return mainWindow; }
void Context::switchToTrainView() { if(mainWindow) mainWindow->selectTrain(); }
void Context::switchToTrendsView() { if(mainWindow) mainWindow->selectTrends(); }
void Context::switchToAnalysisView() { if(mainWindow) mainWindow->selectAnalysis(); }
void Context::switchToDiaryView() { if(mainWindow) mainWindow->selectPlan(); }
bool Context::isCurrent() const { return mainWindow ? mainWindow->athleteTab() == tab : false; }
void Context::requestImportFile() { if (mainWindow) mainWindow->importFile(); }
void Context::requestDownloadRide() { if (mainWindow) mainWindow->downloadRide(); }
void Context::fillinWorkoutFilterBox(const QString &text) {
    if (mainWindow) mainWindow->fillinWorkoutFilterBox(text);
}

// Proxies for slots
void Context::importFile() { if(mainWindow) mainWindow->importFile(); }
void Context::importCloud() { if(mainWindow) mainWindow->importCloud(); }
void Context::downloadRide() { if(mainWindow) mainWindow->downloadRide(); }
void Context::addDevice() { if(mainWindow) mainWindow->addDevice(); }
void Context::manageLibrary() { if(mainWindow) mainWindow->manageLibrary(); }
void Context::downloadTrainerDay() { if(mainWindow) mainWindow->downloadTrainerDay(); }
void Context::importChart() { if (mainWindow) mainWindow->importChart(); }
void Context::addChart(QAction *action) { if (mainWindow) mainWindow->addChart(action); }
void Context::setChartMenu(QMenu *menu) { if (mainWindow) mainWindow->setChartMenu(menu); }
void Context::openAthleteTab(QString path) { if(mainWindow) mainWindow->openAthleteTab(path); }
void Context::closeAthleteTab(QString path)
{
    mainWindow->closeAthleteTab(path);
}

void Context::setSidebarVisible(bool show)
{
    mainWindow->showSidebar(show);
}

void Context::setLowbarVisible(bool show)
{
    mainWindow->showLowbar(show);
}

void Context::importWorkout() { if(mainWindow) mainWindow->importWorkout(); }
void Context::showWorkoutWizard() { if(mainWindow) mainWindow->showWorkoutWizard(); }
void Context::downloadStravaRoutes() { if(mainWindow) mainWindow->downloadStravaRoutes(); }
void Context::switchPerspective(int index) { if(mainWindow) mainWindow->switchPerspective(index); }

void Context::forwardDragEnter(QDragEnterEvent *event) { if (mainWindow) mainWindow->dragEnterEvent(event); }
void Context::forwardDragLeave(QDragLeaveEvent *event) { if (mainWindow) mainWindow->dragLeaveEvent(event); }
void Context::forwardDragMove(QDragMoveEvent *event) { if (mainWindow) mainWindow->dragMoveEvent(event); }
void Context::forwardDrop(QDropEvent *event) { if (mainWindow) mainWindow->dropEvent(event); }

bool Context::isStarting() const { return mainWindow ? mainWindow->isStarting() : false; }
void Context::saveSilent(RideItem *item) { if (mainWindow) mainWindow->saveSilent(this, item); }
bool Context::saveRideSingleDialog(RideItem *item) { return mainWindow ? mainWindow->saveRideSingleDialog(this, item) : false; }

void Context::saveRide() { if(mainWindow) mainWindow->saveRide(); }
void Context::revertRide() { if(mainWindow) mainWindow->revertRide(); }
void Context::deleteRide() { if(mainWindow) mainWindow->deleteRide(); }
void Context::splitRide() { if(mainWindow) mainWindow->splitRide(); }

#include "NewSideBar.h"
void Context::resetPerspective(int index) { if(mainWindow) mainWindow->resetPerspective(index); }
void Context::setToolButtons() { if(mainWindow) mainWindow->setToolButtons(); }
NewSideBar *Context::sidebar() { return mainWindow ? mainWindow->sidebar : nullptr; }
QAction *Context::showHideSidebarAction() { return mainWindow ? mainWindow->showhideSidebar : nullptr; }
bool Context::isMainWindowInitialized() { return mainWindow ? mainWindow->init : false; }

void Context::notifySetNotification(QString msg, int timeout)
{
    if (mainWindow && mainWindow->statusBar()) {
        mainWindow->statusBar()->showMessage(msg, timeout*1000);
    }
}

void Context::notifyTelemetryUpdate(const RealtimeData &rtData)
{
    DataContext::notifyTelemetryUpdate(rtData);
}

