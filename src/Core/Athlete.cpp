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

#include "Athlete.h"

#include "MainWindow.h"
#include "Context.h"
#include "Season.h"
#include "Colors.h"
#include "RideMetadata.h"
#include "RideCache.h"
#include "Estimator.h"
#include "RideFileCache.h"
#include "RideMetric.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"
#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"
#include "WithingsDownload.h"
#include "CalendarDownload.h"
#include "PMCData.h"
#include "Banister.h"
#include "ErgDB.h"
#ifdef GC_HAVE_ICAL
#include "ICalendar.h"
#include "CalDAV.h"
#endif
#include "NamedSearch.h"
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "LTMSettings.h"
#include "RideImportWizard.h"
#include "RideAutoImportConfig.h"
#include "AthleteBackup.h"
#include "CloudService.h"

#include "Route.h"

#include "GcUpgrade.h" // upgrade wizard
#include "GcCrashDialog.h" // recovering from a crash?

Athlete::Athlete(Context *context, const QDir &homeDir)
{
    // athlete name / structured directory
    this->home = new AthleteDirectoryStructure(homeDir);
    this->context = context;
    context->athlete = this;
    cyclist = this->home->root().dirName();

    // get id and set id all at one
    id = QUuid(appsettings->cvalue(cyclist, GC_ATHLETE_ID, QUuid::createUuid().toString()).toString());
    appsettings->setCValue(cyclist, GC_ATHLETE_ID, id.toString());

    // Recovering from a crash?
    if(!appsettings->cvalue(cyclist, GC_SAFEEXIT, true).toBool()) {

        // From V4 we have started to disable this dialog since
        // the information captured is not longer that valuable
        // and the instances of crashes are much, much lower
        // XXXGcCrashDialog *crashed = new GcCrashDialog(homeDir);
        // XXXcrashed->exec();
    }
    appsettings->setCValue(cyclist, GC_SAFEEXIT, false); // will be set to true on exit

    // make sure that the latest folder structure exists in Athlete Directory -
    // e.g. Cache could be deleted by mistake or empty folders are not copied
    // later GC expects the folders are available

    if (!this->home->subDirsExist()) this->home->createAllSubdirs();

    // Before we initialise we need to run the upgrade wizard for this athlete
    GcUpgrade v3;
    int returnCode = v3.upgrade(home->root());
    if (returnCode != 0) return;

    // metric / non-metric
    QVariant unit = appsettings->value(NULL, GC_UNIT, GC_UNIT_METRIC);
    if (unit == 0) {
        // Default to system locale
        unit = QLocale::system().measurementSystem() == QLocale::MetricSystem ? GC_UNIT_METRIC : GC_UNIT_IMPERIAL;
        appsettings->setValue(GC_UNIT, unit);
    }
    useMetricUnits = (unit.toString() == GC_UNIT_METRIC);

    // Power Zones for Bike & Run
    for (int i=0; i < 2; i++) {
        zones_[i] = new Zones(i>0);
        QFile zonesFile(home->config().canonicalPath() + "/" + zones_[i]->fileName());
        if (zonesFile.exists()) {
            if (!zones_[i]->read(zonesFile)) {
                QMessageBox::critical(context->mainWindow, tr("Zones File %1 Error").arg(zones_[i]->fileName()), zones_[i]->errorString());
            } else if (! zones_[i]->warningString().isEmpty()) {
                QMessageBox::warning(context->mainWindow, tr("Reading Zones File %1").arg(zones_[i]->fileName()), zones_[i]->warningString());
            }
        }
        if (i == 1 && zones_[i]->getRangeSize() == 0) { // No running Power zones
            // Start with Cycling Power zones for backward compatibilty
            QFile zonesFile(home->config().canonicalPath() + "/" + zones_[0]->fileName());
            // Load without error/warning report to avoid repetition
            if (zonesFile.exists()) zones_[i]->read(zonesFile);
        }
    }

    // Heartrate Zones for Bike & Run
    for (int i=0; i < 2; i++) {
        hrzones_[i] = new HrZones(i>0);
        QFile hrzonesFile(home->config().canonicalPath() + "/" + hrzones_[i]->fileName());
        if (hrzonesFile.exists()) {
            if (!hrzones_[i]->read(hrzonesFile)) {
                QMessageBox::critical(context->mainWindow, tr("HR Zones File %1 Error").arg(hrzones_[i]->fileName()), hrzones_[i]->errorString());
            } else if (! hrzones_[i]->warningString().isEmpty()) {
                QMessageBox::warning(context->mainWindow, tr("Reading HR Zones File %1").arg(hrzones_[i]->fileName()), hrzones_[i]->warningString());
            }
        }
        if (i == 1 && hrzones_[i]->getRangeSize() == 0) { // No running HR zones
            // Start with Cycling HR zones for backward compatibilty
            QFile hrzonesFile(home->config().canonicalPath() + "/" + hrzones_[0]->fileName());
            // Load without error/warning report to avoid repetition
            if (hrzonesFile.exists()) hrzones_[i]->read(hrzonesFile);
        }
    }

    // Pace Zones for Run & Swim
    for (int i=0; i < 2; i++) {
        pacezones_[i] = new PaceZones(i>0);
        QFile pacezonesFile(home->config().canonicalPath() + "/" + pacezones_[i]->fileName());
        if (pacezonesFile.exists()) {
            if (!pacezones_[i]->read(pacezonesFile)) {
                QMessageBox::critical(context->mainWindow, tr("Pace Zones File %1 Error").arg(pacezones_[i]->fileName()), pacezones_[i]->errorString());
            }
        }
    }

    // read athlete's autoimport configuration and initialize the autoimport process
    autoImportConfig = new RideAutoImportConfig(home->config());
    autoImport = NULL;

    // Search / filter
    namedSearches = new NamedSearches(this); // must be before navigator

    // Metadata
    rideCache = NULL; // let metadata know we don't have a ridecache yet
    rideMetadata_ = new RideMetadata(context,true);
    rideMetadata_->hide();
    colorEngine = new ColorEngine(context);

    // Date Ranges
    seasons = new Seasons(home->config());

    // seconds step of the upgrade - now everything of configuration needed should be in place in Context
    v3.upgradeLate(context);

    // Routes
    routes = new Routes(context, home->config());

    // Daily Measures
    measures = new Measures(home->config(), true);

    // auto downloader
    cloudAutoDownload = new CloudServiceAutoDownload(context);
    connect(context, SIGNAL(refreshEnd()), cloudAutoDownload, SLOT(autoDownload()));

    // now most dependencies are in get cache
    rideCache = new RideCache(context);

    // read athlete's charts.xml and translate etc, it needs to be
    // after RideCache creation to allow for Custom Metrics initialization
    loadCharts();

    // Downloaders
    calendarDownload = new CalendarDownload(context);

    // Calendar
#ifdef GC_HAVE_ICAL
    rideCalendar = new ICalendar(context); // my local/remote calendar entries
    davCalendar = new CalDAV(context); // remote caldav
    davCalendar->download(true); // refresh the diary window but do not show any error messages
#endif

    // trap signals
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context,SIGNAL(rideAdded(RideItem*)),this,SLOT(checkCPX(RideItem*)));
    connect(context,SIGNAL(rideDeleted(RideItem*)),this,SLOT(checkCPX(RideItem*)));
}

void
Athlete::close()
{
    // set to latest so we don't repeat
    appsettings->setCValue(context->athlete->home->root().dirName(), GC_VERSION_USED, VERSION_LATEST);
    appsettings->setCValue(context->athlete->home->root().dirName(), GC_SAFEEXIT, true);

    // run autobackup on close (if configured)
    AthleteBackup *backup = new AthleteBackup(context->athlete->home->root());
    backup->backupOnClose();
    delete backup;

}
void
Athlete::loadCharts()
{
    presets.clear();
    LTMSettings reader;
    reader.readChartXML(context->athlete->home->config(), context->athlete->useMetricUnits, presets);
    translateDefaultCharts(presets);
}

Athlete::~Athlete()
{
    // close the ride cache down first
    delete rideCache;

    // save those preset charts
    LTMSettings reader;
    reader.writeChartXML(home->config(), presets); // don't write it until we fix the code
                                               // all the changes to LTM settings and chart config
                                               // have not been reflected in the charts.xml file

    delete calendarDownload;

#ifdef GC_HAVE_ICAL
    delete rideCalendar;
    delete davCalendar;
#endif

    delete namedSearches;
    delete routes;
    delete seasons;
    delete measures;

    delete rideMetadata_;
    delete colorEngine;
    for (int i=0; i<2; i++) delete zones_[i];
    for (int i=0; i<2; i++) delete hrzones_[i];
    for (int i=0; i<2; i++) delete pacezones_[i];
    delete autoImportConfig;
    delete autoImport;

}

void Athlete::selectRideFile(QString fileName)
{
    // it already is ...
    if (context->ride && context->ride->fileName == fileName) return;

    // lets find it
    foreach (RideItem *rideItem, rideCache->rides()) {

        context->ride = (RideItem*) rideItem;
        if (context->ride->fileName == fileName) 
            break;
    }
    context->notifyRideSelected(context->ride);
}

void
Athlete::addRide(QString name, bool dosignal, bool select, bool useTempActivities, bool planned)
{
    rideCache->addRide(name, dosignal, select, useTempActivities, planned);
}

void
Athlete::removeCurrentRide()
{
    rideCache->removeCurrentRide();
}


void
Athlete::checkCPX(RideItem*ride)
{
    QList<RideFileCache*> newList;

    foreach(RideFileCache *p, cpxCache) {
        if (ride->dateTime.date() < p->start || ride->dateTime.date() > p->end)
            newList.append(p);
    }
    cpxCache = newList;
}

void
Athlete::translateDefaultCharts(QList<LTMSettings>&charts)
{
    // Map default (english) chart name to external (Localized) name
    // New default charts need to be added to this list to be translated
    QMap<QString, QString> chartNameMap;
	chartNameMap.insert("Athlete Weight", tr("Athlete Weight"));
	chartNameMap.insert("Time In Power Zone", tr("Time In Power Zone"));
	chartNameMap.insert("Sustained Time In Zone", tr("Sustained Time In Zone"));
	chartNameMap.insert("Time in W' Zone", tr("Time in W' Zone"));
	chartNameMap.insert("Time In HR Zone", tr("Time In HR Zone"));
	chartNameMap.insert("Power Distribution", tr("Power Distribution"));
    chartNameMap.insert("Vo2max Estimation", tr("Vo2max Estimation"));
	chartNameMap.insert("KPI Tracker", tr("KPI Tracker"));
	chartNameMap.insert("Critical Power Trend", tr("Critical Power Trend"));
	chartNameMap.insert("Aerobic Power", tr("Aerobic Power"));
	chartNameMap.insert("Power Variance", tr("Power Variance"));
	chartNameMap.insert("Power Profile", tr("Power Profile"));
	chartNameMap.insert("Anaerobic Power", tr("Anaerobic Power"));
	chartNameMap.insert("Power & Speed Trend", tr("Power & Speed Trend"));
	chartNameMap.insert("Tempo & Threshold Time", tr("Tempo & Threshold Time"));
	chartNameMap.insert("Training Mix", tr("Training Mix"));
	chartNameMap.insert("Time & Distance", tr("Time & Distance"));
	chartNameMap.insert("BikeScore and Intensity", tr("BikeScore and Intensity"));
	chartNameMap.insert("BikeStress and BikeIntensity", tr("BikeStress and BikeIntensity"));
	chartNameMap.insert("Stress and Distance", tr("Stress and Distance"));
	chartNameMap.insert("Calories vs Duration", tr("Calories vs Duration"));
    chartNameMap.insert("Stress (TISS)", tr("Stress (TISS)"));
    chartNameMap.insert("Aerobic Response", tr("Aerobic Response"));
    chartNameMap.insert("Anaerobic Response", tr("Anaerobic Response"));
    chartNameMap.insert("PMC (Coggan)", tr("PMC (Coggan)"));
    chartNameMap.insert("PMC (Skiba)", tr("PMC (Skiba)"));
    chartNameMap.insert("PMC (TRIMP)", tr("PMC (TRIMP)"));
    chartNameMap.insert("PMC (Distance)", tr("PMC (Distance)"));
    chartNameMap.insert("PMC (Duration)", tr("PMC (Duration)"));
    chartNameMap.insert("CP History", tr("CP History"));
    chartNameMap.insert("CP Analysis", tr("CP Analysis"));
    chartNameMap.insert("PMC (TriScore)", tr("PMC (TriScore)"));
    chartNameMap.insert("Time in Pace Zones", tr("Time in Pace Zones"));
    chartNameMap.insert("Run Pace", tr("Run Pace"));
    chartNameMap.insert("Swim Pace", tr("Swim Pace"));

    for(int i=0; i<charts.count(); i++) {
        // Replace chart name for localized version, default to english name
        charts[i].name = chartNameMap.value(charts[i].name, charts[i].name);
    }
}

void
Athlete::configChanged(qint32 state)
{
    // change units
    if (state & CONFIG_UNITS) {
        QVariant unit = appsettings->value(NULL, GC_UNIT, GC_UNIT_METRIC);
        useMetricUnits = (unit.toString() == GC_UNIT_METRIC);
    }

    // invalidate PMC data
    if (state & (CONFIG_PMC | CONFIG_SEASONS)) {
        QMapIterator<QString, PMCData *> pmcs(pmcData);
        pmcs.toFront();
        while(pmcs.hasNext()) {
            pmcs.next();
            pmcs.value()->invalidate();
        }
    }
}

void
Athlete::importFilesWhenOpeningAthlete() {

    autoImport = NULL;
    // just do it if something is configured
    if (autoImportConfig->hasRules()) {

        autoImport = new RideImportWizard(autoImportConfig, context);

        // only process the popup if we have any files available at all
        if ( autoImport->getNumberOfFiles() > 0) {
           autoImport->process();
        }
    }
}


AthleteDirectoryStructure::AthleteDirectoryStructure(const QDir home){

    myhome = home;

    athlete_activities = "activities";
    athlete_tmp_activities = "tempActivities";
    athlete_imports = "imports";
    athlete_records = "records";
    athlete_downloads = "downloads";
    athlete_fileBackup = "bak";
    athlete_config = "config";
    athlete_cache = "cache";
    athlete_calendar = "calendar";
    athlete_workouts = "workouts";
    athlete_temp = "temp";
    athlete_logs = "logs";
    athlete_quarantine = "quarantine";
    athlete_planned = "planned";
    athlete_snippets = "snippets";
    athlete_media = "media";

}

AthleteDirectoryStructure::~AthleteDirectoryStructure() {

    myhome = NULL;

}

void
AthleteDirectoryStructure::createAllSubdirs() {

    myhome.mkdir(athlete_activities);
    myhome.mkdir(athlete_tmp_activities);
    myhome.mkdir(athlete_imports);
    myhome.mkdir(athlete_records);
    myhome.mkdir(athlete_downloads);
    myhome.mkdir(athlete_fileBackup);
    myhome.mkdir(athlete_config);
    myhome.mkdir(athlete_cache);
    myhome.mkdir(athlete_calendar);
    myhome.mkdir(athlete_workouts);
    myhome.mkdir(athlete_logs);
    myhome.mkdir(athlete_temp);
    myhome.mkdir(athlete_quarantine);
    myhome.mkdir(athlete_planned);
    myhome.mkdir(athlete_snippets);
    myhome.mkdir(athlete_media);

}

bool
AthleteDirectoryStructure::subDirsExist() {

    return (activities().exists() &&
            tmpActivities().exists() &&
            imports().exists() &&
            records().exists() &&
            downloads().exists() &&
            fileBackup().exists() &&
            config().exists() &&
            cache().exists() &&
            calendar().exists() &&
            workouts().exists() &&
            logs().exists() &&
            temp().exists() &&
            quarantine().exists()&&
            planned().exists() &&
            snippets().exists() &&
            media().exists()
            );
}

bool
AthleteDirectoryStructure::upgradedDirectoriesHaveData() {

   if ( activities().exists() && config().exists()) {
       // just check for config files (activities are empty in case of a new athlete)
       QStringList configFiles = config().entryList(QDir::Files);
       if (!configFiles.isEmpty()) { return true; }
   }
   return false;

}

QList<PDEstimate>
Athlete::getPDEstimates()
{
    // returns whatever estimator has, if not running
    QList<PDEstimate> returning;

    // get estimates
    rideCache->estimator->lock.lock();
    returning = rideCache->estimator->estimates;
    rideCache->estimator->lock.unlock();

    return returning;
}


// working with weight data
double
Athlete::getWeight(QDate date, RideFile *ride)
{
    double weight;

    // daily weight first
    weight = measures->getGroup(Measures::Body)->getFieldValue(date);

    // ride (if available)
    if (!weight && ride)
        weight = ride->getTag("Weight", "0.0").toDouble();

    // global options
    if (!weight)
        weight = appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT, "75.0").toString().toDouble(); // default to 75kg

    // No weight default is weird, we'll set to 80kg
    if (weight <= 0.00) weight = 80.00;

    return weight;
}

double
Athlete::getHeight(RideFile *ride)
{
    double height = 0;

    // ride if present?
    if (ride) height = ride->getTag("Height", "0.0").toDouble();

    // global options ?
    if (!height) height = appsettings->cvalue(context->athlete->cyclist, GC_HEIGHT, 0.0f).toString().toDouble();

    // from weight via Stillman Average?
    if (!height && ride) height = (getWeight(ride->startTime().date(), ride)+100.0)/98.43;

    // it must not be zero!!!
    if (!height) height = 1.7526f; // 5'9" is average male height

    return height;
}

// working with Banister data series
Banister *
Athlete::getBanisterFor(QString metricName, int t1 , int t2)
{
    Banister *returning = NULL;

    // if we don't already have one, create it
    returning = banisterData.value(metricName, NULL); // we do
    if (!returning) {

        // specification is blank and passes for all
        returning = new Banister(context, metricName, t1, t2); // we don't seed t1/t2 yet. (maybe never will)

        // add to our collection
        banisterData.insert(metricName, returning);
    }

    return returning;

}

// working with PMC data series
PMCData *
Athlete::getPMCFor(QString metricName, int stsdays, int ltsdays)
{
    PMCData *returning = NULL;

    // if we don't already have one, create it
    returning = pmcData.value(metricName, NULL);
    if (!returning) {

        // specification is blank and passes for all
        returning = new PMCData(context, Specification(), metricName, stsdays, ltsdays);

        // add to our collection
        pmcData.insert(metricName, returning);
    }

    return returning;
}

PMCData *
Athlete::getPMCFor(Leaf *expr, DataFilterRuntime *df, int stsdays, int ltsdays)
{
    PMCData *returning = NULL;

    // if we don't already have one, create it
    returning = pmcData.value(expr->signature(), NULL);
    if (!returning) {

        // specification is blank and passes for all
        returning = new PMCData(context, Specification(), expr, df, stsdays, ltsdays);

        // add to our collection
        pmcData.insert(expr->signature(), returning);
    }

    return returning;
}

PDEstimate
Athlete::getPDEstimateFor(QDate date, QString model, bool wpk)
{
    // whats the estimate for this date
    foreach(PDEstimate est, getPDEstimates()) {
        if (est.model == model && est.wpk == wpk && est.from <= date && est.to >= date)
            return est;
    }
    return PDEstimate();
}

