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
#include "ErgDB.h"
#ifdef GC_HAVE_ICAL
#include "ICalendar.h"
#include "CalDAV.h"
#endif
#ifdef GC_HAVE_LUCENE
#include "Lucene.h"
#include "NamedSearch.h"
#endif
#include "IntervalItem.h"
#include "IntervalTreeView.h"
#include "LTMSettings.h"
#include "RideImportWizard.h"
#include "RideAutoImportConfig.h"
#include "Route.h"
#include "RouteWindow.h"

#include "GcUpgrade.h" // upgrade wizard
#include "GcCrashDialog.h" // recovering from a crash?

Athlete::Athlete(Context *context, const QDir &homeDir)
{
    // athlete name / structured directory
    this->home = new AthleteDirectoryStructure(homeDir);
    this->context = context;
    context->athlete = this;
    cyclist = this->home->root().dirName();
    isclean = false;

    // Recovering from a crash?
    if(!appsettings->cvalue(cyclist, GC_SAFEEXIT, true).toBool()) {
        GcCrashDialog *crashed = new GcCrashDialog(homeDir);
        crashed->exec();
    }
    appsettings->setCValue(cyclist, GC_SAFEEXIT, false); // will be set to true on exit

    // Before we initialise we need to run the upgrade wizard for this athlete
    GcUpgrade v3;
    int returnCode = v3.upgrade(home->root());
    if (returnCode != 0) return;

    // metric / non-metric
    QVariant unit = appsettings->cvalue(cyclist, GC_UNIT);
    if (unit == 0) {
        // Default to system locale
        unit = appsettings->value(this, GC_UNIT,
             QLocale::system().measurementSystem() == QLocale::MetricSystem ? GC_UNIT_METRIC : GC_UNIT_IMPERIAL);
        appsettings->setCValue(cyclist, GC_UNIT, unit);
    }
    useMetricUnits = (unit.toString() == GC_UNIT_METRIC);

    // Power Zones
    zones_ = new Zones;
    QFile zonesFile(home->config().canonicalPath() + "/power.zones");
    if (zonesFile.exists()) {
        if (!zones_->read(zonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("Zones File Error"),
				  zones_->errorString());
        } else if (! zones_->warningString().isEmpty())
            QMessageBox::warning(context->mainWindow, tr("Reading Zones File"), zones_->warningString());
    }

    // Heartrate Zones
    hrzones_ = new HrZones;
    QFile hrzonesFile(home->config().canonicalPath() + "/hr.zones");
    if (hrzonesFile.exists()) {
        if (!hrzones_->read(hrzonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("HR Zones File Error"),
				  hrzones_->errorString());
        } else if (! hrzones_->warningString().isEmpty())
            QMessageBox::warning(context->mainWindow, tr("Reading HR Zones File"), hrzones_->warningString());
    }

    // Pace Zones
    pacezones_ = new PaceZones;
    QFile pacezonesFile(home->config().canonicalPath() + "/pace.zones");
    if (pacezonesFile.exists()) {
        if (!pacezones_->read(pacezonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("Pace Zones File Error"), pacezones_->errorString());
        }
    }

    // read athlete's autoimport configuration
    autoImportConfig = new RideAutoImportConfig(home->config());

    // read athlete's charts.xml and translate etc
    LTMSettings reader;
    reader.readChartXML(context->athlete->home->config(), context->athlete->useMetricUnits, presets);
    translateDefaultCharts(presets);

    // Metadata
    rideCache = NULL; // let metadata know we don't have a ridecache yet
    rideMetadata_ = new RideMetadata(context,true);
    rideMetadata_->hide();
    colorEngine = new ColorEngine(context);

    // Date Ranges
    seasons = new Seasons(home->config());

    // seconds step of the upgrade - now everything of configuration needed should be in place in Context
    v3.upgradeLate(context);

#ifdef GC_HAVE_INTERVALS
    // Routes
    routes = new Routes(context, home->config());
#endif

    // Search / filter
#ifdef GC_HAVE_LUCENE
    namedSearches = new NamedSearches(this); // must be before navigator
    lucene = new Lucene(context, context);
#endif

    // get withings in if there is a cache
    QFile withingsJSON(QString("%1/withings.json").arg(context->athlete->home->cache().canonicalPath()));
    if (withingsJSON.exists() && withingsJSON.open(QFile::ReadOnly)) {

        QString text;
        QStringList errors;
        QTextStream stream(&withingsJSON);
        text = stream.readAll();
        withingsJSON.close();

        WithingsParser parser;
        parser.parse(text, errors);
        if (errors.count() == 0) setWithings(parser.readings());
    }

    // now most dependencies are in get cache
    rideCache = new RideCache(context);

#ifdef GC_HAVE_INTERVALS
    sqlRouteIntervalsModel = new QSqlTableModel(this, metricDB->db()->connection());
    sqlRouteIntervalsModel->setTable("interval_metrics");
    sqlRouteIntervalsModel->setFilter("type='Route'");
    sqlRouteIntervalsModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    sqlBestIntervalsModel = new QSqlTableModel(this, metricDB->db()->connection());
    sqlBestIntervalsModel->setTable("interval_metrics");
    sqlBestIntervalsModel->setFilter("type='Best'");
    sqlBestIntervalsModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
#endif

    // Downloaders
    withingsDownload = new WithingsDownload(context);
    calendarDownload = new CalendarDownload(context);

    // Calendar
#ifdef GC_HAVE_ICAL
    rideCalendar = new ICalendar(context); // my local/remote calendar entries
    davCalendar = new CalDAV(context); // remote caldav
    davCalendar->download(); // refresh the diary window
#endif

    //.INTERVALS TREE -- transitionary
    intervalWidget = new IntervalTreeView(context);
    intervalWidget->setColumnCount(1);
    intervalWidget->setIndentation(5);
    intervalWidget->setSortingEnabled(false);
    intervalWidget->header()->hide();
    intervalWidget->setAlternatingRowColors (false);
    intervalWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    intervalWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    intervalWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    intervalWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    intervalWidget->setFrameStyle(QFrame::NoFrame);
    allIntervals = context->athlete->intervalWidget->invisibleRootItem();
    allIntervals->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDropEnabled);
    allIntervals->setText(0, tr("Intervals"));

    // trap signals
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(context,SIGNAL(rideAdded(RideItem*)),this,SLOT(checkCPX(RideItem*)));
    connect(context,SIGNAL(rideDeleted(RideItem*)),this,SLOT(checkCPX(RideItem*)));
    connect(intervalWidget,SIGNAL(itemSelectionChanged()), this, SLOT(intervalTreeWidgetSelectionChanged()));
    connect(intervalWidget,SIGNAL(itemChanged(QTreeWidgetItem *,int)), this, SLOT(updateRideFileIntervals()));
}

void
Athlete::close()
{
    // set to latest so we don't repeat
    appsettings->setCValue(context->athlete->home->root().dirName(), GC_VERSION_USED, VERSION_LATEST);
    appsettings->setCValue(context->athlete->home->root().dirName(), GC_SAFEEXIT, true);
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

    delete withingsDownload;
    delete calendarDownload;

#ifdef GC_HAVE_ICAL
    delete rideCalendar;
    delete davCalendar;
#endif

#ifdef GC_HAVE_INTERVALS
    // close the db connection (but clear models first!)
    delete sqlRouteIntervalsModel;
    delete sqlBestIntervalsModel;
#endif

#ifdef GC_HAVE_LUCENE
    delete namedSearches;
    delete lucene;
#endif
    delete seasons;

    delete rideMetadata_;
    delete colorEngine;
    delete zones_;
    delete hrzones_;
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
Athlete::intervalTreeWidgetSelectionChanged()
{
    context->notifyIntervalHover(RideFileInterval()); // clear
    context->notifyIntervalSelected();
}

void
Athlete::updateRideFileIntervals()
{
    // iterate over context->athlete->allIntervals as they are now defined
    // and update the RideFile->intervals
    if (context->ride) {

        RideFile *current = context->ride->ride();
        current->clearIntervals();

        for (int i=0; i < allIntervals->childCount(); i++) {
            // add the intervals as updated
            IntervalItem *it = (IntervalItem *)allIntervals->child(i);
            current->addInterval(it->start, it->stop, it->text(0));
        }

        // emit signal for interval data changed
        context->notifyIntervalsChanged();

        // set dirty
        context->ride->setDirty(true);
    }
}

void
Athlete::addRide(QString name, bool dosignal)
{
    rideCache->addRide(name, dosignal);
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
	chartNameMap.insert("PMC", tr("PMC"));
	chartNameMap.insert("Track Weight", tr("Track Weight"));
	chartNameMap.insert("Time In Power Zone (Stacked)", tr("Time In Power Zone (Stacked)"));
	chartNameMap.insert("Time In Power Zone (Bar)", tr("Time In Power Zone (Bar)"));
	chartNameMap.insert("Time In HR Zone", tr("Time In HR Zone"));
	chartNameMap.insert("Power Distribution", tr("Power Distribution"));
	chartNameMap.insert("KPI Tracker", tr("KPI Tracker"));
	chartNameMap.insert("Critical Power Trend", tr("Critical Power Trend"));
	chartNameMap.insert("Aerobic Power", tr("Aerobic Power"));
	chartNameMap.insert("Aerobic WPK", tr("Aerobic WPK"));
	chartNameMap.insert("Power Variance", tr("Power Variance"));
	chartNameMap.insert("Power Profile", tr("Power Profile"));
	chartNameMap.insert("Anaerobic Power", tr("Anaerobic Power"));
	chartNameMap.insert("Anaerobic WPK", tr("Anaerobic WPK"));
	chartNameMap.insert("Power & Speed Trend", tr("Power & Speed Trend"));
	chartNameMap.insert("Cardiovascular Response", tr("Cardiovascular Response"));
	chartNameMap.insert("Tempo & Threshold Time", tr("Tempo & Threshold Time"));
	chartNameMap.insert("Training Mix", tr("Training Mix"));
	chartNameMap.insert("Time & Distance", tr("Time & Distance"));
	chartNameMap.insert("Skiba Power", tr("Skiba Power"));
	chartNameMap.insert("Daniels Power", tr("Daniels Power"));
	chartNameMap.insert("PM Ramp & Peak", tr("PM Ramp & Peak"));
	chartNameMap.insert("Skiba PM", tr("Skiba PM"));
	chartNameMap.insert("Daniels PM", tr("Daniels PM"));
	chartNameMap.insert("Device Reliability", tr("Device Reliability"));
	chartNameMap.insert("Withings Weight", tr("Withings Weight"));
	chartNameMap.insert("Stress and Distance", tr("Stress and Distance"));
	chartNameMap.insert("Calories vs Duration", tr("Calories vs Duration"));
    chartNameMap.insert("Stress (TISS)", tr("Stress (TISS)"));
    chartNameMap.insert("PMC (Coggan)", tr("PMC (Coggan)"));
    chartNameMap.insert("PMC (Skiba)", tr("PMC (Skiba)"));
    chartNameMap.insert("PMC (TRIMP)", tr("PMC (TRIMP)"));
    chartNameMap.insert("CP History", tr("CP History"));

    for(int i=0; i<charts.count(); i++) {
        // Replace chart name for localized version, default to english name
        charts[i].name = chartNameMap.value(charts[i].name, charts[i].name);
    }
}

void
Athlete::configChanged()
{
    // re-read Zones in case it changed
    QFile zonesFile(home->config().canonicalPath() + "/power.zones");
    if (zonesFile.exists()) {
        if (!zones_->read(zonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("Zones File Error"),
                                 zones_->errorString());
        }
       else if (! zones_->warningString().isEmpty())
            QMessageBox::warning(context->mainWindow, tr("Reading Zones File"), zones_->warningString());
    }

    // reread HR zones
    QFile hrzonesFile(home->config().canonicalPath() + "/hr.zones");
    if (hrzonesFile.exists()) {
        if (!hrzones_->read(hrzonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("HR Zones File Error"),
                                 hrzones_->errorString());
        }
       else if (! hrzones_->warningString().isEmpty())
            QMessageBox::warning(context->mainWindow, tr("Reading HR Zones File"), hrzones_->warningString());
    }

    // reread Pace zones
    QFile pacezonesFile(home->config().canonicalPath() + "/pace.zones");
    if (pacezonesFile.exists()) {
        if (!pacezones_->read(pacezonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("Pace Zones File Error"),
                                 pacezones_->errorString());
        }
    }

    QVariant unit = appsettings->cvalue(cyclist, GC_UNIT);
    useMetricUnits = (unit.toString() == GC_UNIT_METRIC);
}

void
Athlete::importFilesWhenOpeningAthlete() {

    // just do it if something is configured
    if (autoImportConfig->hasRules()) {

        RideImportWizard *import = new RideImportWizard(autoImportConfig, context);

        // only process the popup if we have any files available at all
        if ( import->getNumberOfFiles() > 0) {
           import->process();
        } else {
           delete import;
        }
    }
}


AthleteDirectoryStructure::AthleteDirectoryStructure(const QDir home){

    myhome = home;

    athlete_activities = "activities";
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


}

AthleteDirectoryStructure::~AthleteDirectoryStructure() {

    myhome = NULL;

}

void
AthleteDirectoryStructure::createAllSubdirs() {

    myhome.mkdir(athlete_activities);
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


}

bool
AthleteDirectoryStructure::subDirsExist() {

    return (activities().exists() &&
            imports().exists() &&
            records().exists() &&
            downloads().exists() &&
            fileBackup().exists() &&
            config().exists() &&
            cache().exists() &&
            calendar().exists() &&
            workouts().exists() &&
            logs().exists() &&
            temp().exists()
            );
}

// working with withings data
void 
Athlete::setWithings(QList<WithingsReading>&x)
{
    withings_ = x;
    qSort(withings_); // date order
}

double 
Athlete::getWithingsWeight(QDate date)
{
    if (withings_.count() == 0) return 0;

    double lastWeight=0.0f;
    foreach(WithingsReading x, withings_) {
        if (x.when.date() <= date) lastWeight = x.weightkg;
        if (x.when.date() > date) break;
    }
    return lastWeight;
}

double
Athlete::getWeight(QDate date, RideFile *ride)
{
    double weight;

    // withings first
    weight = getWithingsWeight(date);

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
