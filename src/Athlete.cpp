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
#include "RideMetadata.h"
#include "RideFileCache.h"
#include "RideMetric.h"
#include "Settings.h"
#include "TimeUtils.h"
#include "Units.h"
#include "Zones.h"
#include "MetricAggregator.h"
#include "WithingsDownload.h"
#include "ZeoDownload.h"
#include "CalendarDownload.h"
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
#include "Route.h"
#include "RouteWindow.h"
#include "RideImportWizard.h"


#include "GcUpgrade.h" // upgrade wizard
#include "GcCrashDialog.h" // recovering from a crash?

Athlete::Athlete(Context *context, const QDir &home)
{
    // athlete name
    this->home = home;
    this->context = context;
    context->athlete = this;
    cyclist = home.dirName();
    isclean = false;

    // Recovering from a crash?
    if(!appsettings->cvalue(cyclist, GC_SAFEEXIT, true).toBool()) {
        GcCrashDialog *crashed = new GcCrashDialog(home);
        crashed->exec();
    }
    appsettings->setCValue(cyclist, GC_SAFEEXIT, false); // will be set to true on exit

    // Before we initialise we need to run the upgrade wizard for this athlete
    GcUpgrade v3;
    v3.upgrade(context->athlete->home);

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
    QFile zonesFile(home.absolutePath() + "/power.zones");
    if (zonesFile.exists()) {
        if (!zones_->read(zonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("Zones File Error"),
				  zones_->errorString());
        } else if (! zones_->warningString().isEmpty())
            QMessageBox::warning(context->mainWindow, tr("Reading Zones File"), zones_->warningString());
    }

    // Heartrate Zones
    hrzones_ = new HrZones;
    QFile hrzonesFile(home.absolutePath() + "/hr.zones");
    if (hrzonesFile.exists()) {
        if (!hrzones_->read(hrzonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("HR Zones File Error"),
				  hrzones_->errorString());
        } else if (! hrzones_->warningString().isEmpty())
            QMessageBox::warning(context->mainWindow, tr("Reading HR Zones File"), hrzones_->warningString());
    }

    // read athlete's charts.xml and translate etc
    LTMSettings reader;
    reader.readChartXML(context->athlete->home, context->athlete->useMetricUnits, presets);
    translateDefaultCharts(presets);

    // Metadata
    metricDB = NULL; // warn metadata we haven't got there yet !
    rideMetadata_ = new RideMetadata(context,true);
    rideMetadata_->hide();

    // Date Ranges
    seasons = new Seasons(home);

    // Search / filter
#ifdef GC_HAVE_LUCENE
    namedSearches = new NamedSearches(this); // must be before navigator
    lucene = new Lucene(context, context); // before metricDB attempts to refresh
#endif

    // metrics DB
    metricDB = new MetricAggregator(context); // just to catch config updates!
    metricDB->refreshMetrics();

    // the model atop the metric DB
    sqlModel = new QSqlTableModel(this, metricDB->db()->connection());
    sqlModel->setTable("metrics");
    sqlModel->setEditStrategy(QSqlTableModel::OnManualSubmit);

    // Downloaders
    withingsDownload = new WithingsDownload(context);
    zeoDownload      = new ZeoDownload(context);
    calendarDownload = new CalendarDownload(context);

    // Calendar
#ifdef GC_HAVE_ICAL
    rideCalendar = new ICalendar(context); // my local/remote calendar entries
    davCalendar = new CalDAV(context); // remote caldav
    davCalendar->download(); // refresh the diary window
#endif

    // Routes
    routes = new Routes(context, home);

    // RIDE TREE -- transitionary
    treeWidget = new QTreeWidget;
    treeWidget->setColumnCount(3);
    treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    treeWidget->header()->resizeSection(0,60);
    treeWidget->header()->resizeSection(1,100);
    treeWidget->header()->resizeSection(2,70);
    treeWidget->header()->hide();
    treeWidget->setAlternatingRowColors (false);
    treeWidget->setIndentation(5);
    treeWidget->hide();

    allRides = new QTreeWidgetItem(context->athlete->treeWidget, FOLDER_TYPE);
    allRides->setText(0, tr("All Activities"));
    treeWidget->expandItem(context->athlete->allRides);
    treeWidget->setFirstItemColumnSpanned (context->athlete->allRides, true);

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

    // populate ride list
    QTreeWidgetItem *last = NULL;
    QStringListIterator i(RideFileFactory::instance().listRideFiles(home));
    while (i.hasNext()) {
        QString name = i.next();
        QDateTime dt;
        if (RideFile::parseRideFileName(name, &dt)) {
            last = new RideItem(RIDE_TYPE, home.path(), name, dt, zones(), hrZones(), context);
            allRides->addChild(last);
        }
    }

    // trap signals
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(treeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(rideTreeWidgetSelectionChanged()));
    connect(context,SIGNAL(rideAdded(RideItem*)),this,SLOT(checkCPX(RideItem*)));
    connect(context,SIGNAL(rideDeleted(RideItem*)),this,SLOT(checkCPX(RideItem*)));
    connect(intervalWidget,SIGNAL(itemSelectionChanged()), this, SLOT(intervalTreeWidgetSelectionChanged()));
    connect(intervalWidget,SIGNAL(itemChanged(QTreeWidgetItem *,int)), this, SLOT(updateRideFileIntervals()));
}

void
Athlete::close()
{
    // set to latest so we don't repeat
    appsettings->setCValue(context->athlete->home.dirName(), GC_VERSION_USED, VERSION_LATEST);
    appsettings->setCValue(context->athlete->home.dirName(), GC_SAFEEXIT, true);
}

Athlete::~Athlete()
{
    // save those preset charts
    LTMSettings reader;
    reader.writeChartXML(home, presets); // don't write it until we fix the code
                                               // all the changes to LTM settings and chart config
                                               // have not been reflected in the charts.xml file

    delete withingsDownload;
    delete zeoDownload;
    delete calendarDownload;

#ifdef GC_HAVE_ICAL
    delete rideCalendar;
    delete davCalendar;
#endif
    delete treeWidget;

    // close the db connection (but clear models first!)
    delete sqlModel;
    delete metricDB;

#ifdef GC_HAVE_LUCENE
    delete namedSearches;
    delete lucene;
#endif
    delete seasons;

    delete rideMetadata_;
    delete zones_;
    delete hrzones_;
}

void Athlete::selectRideFile(QString fileName)
{
    for (int i = 0; i < allRides->childCount(); i++)
    {
        context->ride = (RideItem*) allRides->child(i);
        if (context->ride->fileName == fileName) {
            treeWidget->scrollToItem(allRides->child(i),
                QAbstractItemView::EnsureVisible);
            treeWidget->setCurrentItem(allRides->child(i));
            i = allRides->childCount();
        }
    }
}

void
Athlete::rideTreeWidgetSelectionChanged()
{
    if (treeWidget->selectedItems().isEmpty())
        context->ride = NULL;
    else {
        QTreeWidgetItem *which = treeWidget->selectedItems().first();
        if (which->type() != RIDE_TYPE) return; // ignore!
        else
            context->ride = (RideItem*) which;
    }

    // emit signal!
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
    RideItem *which = (RideItem *)treeWidget->selectedItems().first();
    RideFile *current = which->ride();
    current->clearIntervals();
    for (int i=0; i < allIntervals->childCount(); i++) {
        // add the intervals as updated
        IntervalItem *it = (IntervalItem *)allIntervals->child(i);
        current->addInterval(it->start, it->stop, it->text(0));
    }

    // emit signal for interval data changed
    context->notifyIntervalsChanged();

    // set dirty
    which->setDirty(true);
}

const RideFile *
Athlete::currentRide()
{
    if ((treeWidget->selectedItems().size() != 1)
        || (treeWidget->selectedItems().first()->type() != RIDE_TYPE)) {
        return NULL;
    }
    return ((RideItem*) treeWidget->selectedItems().first())->ride();
}

void
Athlete::addRide(QString name, bool dosignal)
{
    QDateTime dt;
    if (!RideFile::parseRideFileName(name, &dt)) return;

    RideItem *last = new RideItem(RIDE_TYPE, home.path(), name, dt, zones(), hrZones(), context);

    int index = 0;
    while (index < allRides->childCount()) {
        QTreeWidgetItem *item = allRides->child(index);
        if (item->type() != RIDE_TYPE) continue;
        RideItem *other = static_cast<RideItem*>(item);

        if (other->dateTime > dt) break;
        if (other->fileName == name) {
            delete allRides->takeChild(index);
            break;
        }
        ++index;
    }
    if (dosignal) context->notifyRideAdded(last); // here so emitted BEFORE rideSelected is emitted!
    allRides->insertChild(index, last);

    // if it is the very first ride, we need to select it
    // after we added it
    if (!index) treeWidget->setCurrentItem(last);
}

void
Athlete::removeCurrentRide()
{
    int x = 0;

    QTreeWidgetItem *_item = treeWidget->currentItem();
    if (_item->type() != RIDE_TYPE) return;
    RideItem *item = static_cast<RideItem*>(_item);

    QTreeWidgetItem *itemToSelect = NULL;
    for (x=0; x<allRides->childCount(); ++x)
        if (item==allRides->child(x)) break;

    if (x>0) itemToSelect = allRides->child(x-1);

    if ((x+1)<allRides->childCount())
        itemToSelect = allRides->child(x+1);

    QString strOldFileName = item->fileName;
    allRides->removeChild(item);


    QFile file(home.absolutePath() + "/" + strOldFileName);
    // purposefully don't remove the old ext so the user wouldn't have to figure out what the old file type was
    QString strNewName = strOldFileName + ".bak";

    // in case there was an existing bak file, delete it
    // ignore errors since it probably isn't there.
    QFile::remove(home.absolutePath() + "/" + strNewName);

    if (!file.rename(home.absolutePath() + "/" + strNewName)) {
        QMessageBox::critical(NULL, "Rename Error", tr("Can't rename %1 to %2")
            .arg(strOldFileName).arg(strNewName));
    }

    // remove any other derived/additional files; notes, cpi etc
    QStringList extras;
    extras << "notes" << "cpi" << "cpx";
    foreach (QString extension, extras) {

        QString deleteMe = QFileInfo(strOldFileName).baseName() + "." + extension;
        QFile::remove(home.absolutePath() + "/" + deleteMe);
    }

    // we don't want the whole delete, select next flicker
    context->mainWindow->setUpdatesEnabled(false);

    // notify AFTER deleted from DISK..
    context->notifyRideDeleted(item);

    // ..but before MEMORY cleared
    item->freeMemory();
    delete item;

    // any left?
    if (allRides->childCount() == 0) {
        context->ride = NULL;
        context->notifyRideSelected(NULL); // notifies children
    }

    treeWidget->setCurrentItem(itemToSelect);

    // now we can update
    context->mainWindow->setUpdatesEnabled(true);
    QApplication::processEvents();

    context->notifyRideSelected((RideItem*)itemToSelect);

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
    QFile zonesFile(home.absolutePath() + "/power.zones");
    if (zonesFile.exists()) {
        if (!zones_->read(zonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("Zones File Error"),
                                 zones_->errorString());
        }
       else if (! zones_->warningString().isEmpty())
            QMessageBox::warning(context->mainWindow, tr("Reading Zones File"), zones_->warningString());
    }

    // reread HR zones
    QFile hrzonesFile(home.absolutePath() + "/hr.zones");
    if (hrzonesFile.exists()) {
        if (!hrzones_->read(hrzonesFile)) {
            QMessageBox::critical(context->mainWindow, tr("HR Zones File Error"),
                                 hrzones_->errorString());
        }
       else if (! hrzones_->warningString().isEmpty())
            QMessageBox::warning(context->mainWindow, tr("Reading HR Zones File"), hrzones_->warningString());
    }

    QVariant unit = appsettings->cvalue(cyclist, GC_UNIT);
    useMetricUnits = (unit.toString() == GC_UNIT_METRIC);

    // forget all the cached weight values in case weight changed
    for (int i=0; i<allRides->childCount(); i++) {
        RideItem *rideItem = static_cast<RideItem*>(allRides->child(i));
        if (rideItem->ride(false)) {
            rideItem->ride(false)->setWeight(0);
            rideItem->ride(false)->getWeight();
        }
    }
}

void
Athlete::importFilesWithoutDialog() {

    int importSettings = appsettings->cvalue(context->athlete->cyclist, GC_IMPORTSETTINGS).toInt(); // default/unset = 0
    if (importSettings == 0) return; // no autoimport requested

    QVariant importDirQV = appsettings->cvalue(context->athlete->cyclist, GC_IMPORTDIR, "");
    QString importDirectory = importDirQV.toString();
    if (importDirectory == "") return; // no implicit assumptions on the directory - only explicite is allowed

    // now get the files formats
    const RideFileFactory &rff = RideFileFactory::instance();
    QStringList suffixList = rff.suffixes();
    suffixList.replaceInStrings(QRegExp("^"), "*.");
    QStringList allFormats;
    QFileInfoList fileInfos;
    foreach(QString suffix, rff.suffixes())
        allFormats << QString("*.%1").arg(suffix);

    // and now the files for the GC formats / no SubDirs considered
    QDir *importDir = new QDir (importDirectory);
    if (!importDir->exists()) return;    // directory might not be available (USB,..)
    if (!importDir->isReadable()) return; // check if directory is readable,

    // now get the files with their full names
    fileInfos = importDir->entryInfoList(allFormats, QDir::Files, QDir::NoSort);
    if (!fileInfos.isEmpty()) {
        QStringList fileNames;
        foreach(QFileInfo f, fileInfos) {
            fileNames.append(f.absoluteFilePath());
        }
        RideImportWizard *import = new RideImportWizard(fileNames, context->athlete->home, context);
        if (importSettings == 1) import->setDialogMode(RideImportWizard::allButDupFileErrors);
        if (importSettings == 2) import->setDialogMode(RideImportWizard::allErrors);
        import->process();
    }
}
