/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_MainWindow_h
#define _GC_MainWindow_h 1
#include "GoldenCheetah.h"

#include <QDir>
#include <QSqlDatabase>
#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include "RideItem.h"
#include "IntervalItem.h"
#include "QuarqdClient.h"
#include "RealtimeData.h"
#include <boost/shared_ptr.hpp>

class AerolabWindow;
class GoogleMapControl;
class AllPlotWindow;
class CriticalPowerWindow;
class HistogramWindow;
class PfPvWindow;
class HrPwWindow;
class QwtPlotPanner;
class QwtPlotPicker;
class QwtPlotZoomer;
class LTMWindow;
class MetricAggregator;
class ModelWindow;
class ScatterWindow;
class RealtimeWindow;
class RideFile;
class RideMetadata;
class WeeklySummaryWindow;
class Zones;
class HrZones;
class RideCalendar;
class PerformanceManagerWindow;
class SummaryWindow;
class ViewSelection;
class TrainWindow;
class RideEditor;
class RideNavigator;
class WithingsDownload;
class CalendarDownload;
class DiaryWindow;
class TreeMapWindow;
class GcWindowTool;
class HomeWindow;
class ICalendar;
class CalDAV;
class Seasons;
class IntervalSummaryWindow;
class ErgFile;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    G_OBJECT


    public:
        MainWindow(const QDir &home);
        void addRide(QString name, bool bSelect=true);
        void removeCurrentRide();
        const RideFile *currentRide();
        const RideItem *currentRideItem() { return ride; }
        const QTreeWidgetItem *allRideItems() { return allRides; }
        const QTreeWidgetItem *allIntervalItems() { return allIntervals; }
        QTreeWidget *intervalTreeWidget() { return intervalWidget; }
        QTreeWidget *rideTreeWidget() { return treeWidget; }
        QTreeWidgetItem *mutableIntervalItems() { return allIntervals; }
	    void getBSFactors(double &timeBS, double &distanceBS,
                          double &timeDP, double &distanceDP);
        QDir home;
        void setCriticalPower(int cp);

        const Zones *zones() const { return zones_; }
        const HrZones *hrZones() const { return hrzones_; }

        void updateRideFileIntervals();
        void saveSilent(RideItem *);
        bool saveRideSingleDialog(RideItem *);
        RideItem *rideItem() const { return ride; }
        RideMetadata *rideMetadata() { return _rideMetadata; }

        // let the other widgets know when ride status changes
        void notifyConfigChanged(); // used by ConfigDialog to notify MainWindow
                                    // when config has changed - and to get a
                                    // signal emitted to notify its children
        void notifyRideSelected();  // used by RideItem to notify when
                                    // rideItem date/time changes
        void notifyRideClean() { rideClean(); }
        void notifyRideDirty() { rideDirty(); }
        void selectView(int);
        void selectRideFile(QString);

        // realtime signals
        void notifyTelemetryUpdate(RealtimeData rtData) { telemetryUpdate(rtData); }
        void notifyErgFileSelected(ErgFile *x) { ergFileSelected(x); }
        void notifySetNow(long now) { setNow(now); }

        // db connections to cyclistdir/metricDB - one per active MainWindow
        QSqlDatabase db;
        MetricAggregator *metricDB;
        Seasons *seasons;

        int session;
        bool isclean;
        QString cyclist; // the cyclist name
	    bool useMetricUnits;  // whether metric units are used (or imperial)

        CalendarDownload *calendarDownload;
#ifdef GC_HAVE_ICAL
        ICalendar *rideCalendar;
        CalDAV *davCalendar;
#endif

    protected:

        Zones *zones_;
        HrZones *hrzones_;

        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void closeEvent(QCloseEvent*);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);

    signals:

        //void rideSelected();
        void intervalSelected();
        void intervalsChanged();
        void zonesChanged();
        void seasonsChanged();
        void configChanged();
        void rideAdded(RideItem *);
        void rideDeleted(RideItem *);
        void rideDirty();
        void rideClean();
        void telemetryUpdate(RealtimeData rtData);
        void ergFileSelected(ErgFile *);
        void setNow(long);

    private slots:
        void tabViewTriggered(bool);
        void rideTreeWidgetSelectionChanged();
        void intervalTreeWidgetSelectionChanged();
        void leftLayoutMoved();
        void splitterMoved();
        void summarySplitterMoved();
        void newCyclist();
        void openCyclist();
        void downloadRide();
        void manualRide();
        void exportPWX();
        void exportCSV();
        void exportGC();
        void exportJson();
        void exportMetrics();
#ifdef GC_HAVE_KML
        void exportKML();
#endif
        void manualProcess(QString);
#ifdef GC_HAVE_SOAP
        void uploadTP();
        void downloadTP();
#endif
#ifdef GC_HAVE_ICAL
        void uploadCalendar(); // upload ride to calendar
#endif
        void importFile();
        void findBestIntervals();
        void addIntervalForPowerPeaksForSecs(RideFile *ride, int windowSizeSecs, QString name);
        void findPowerPeaks();
        void splitRide();
        void deleteRide();
        void tabChanged(int index);
        void aboutDialog();
        void saveRide();                        // save current ride menu item
        void revertRide();
        bool saveRideExitDialog();              // save dirty rides on exit dialog
        void showOptions();
        void showTools();
        void showWorkoutWizard();
	void importRideToDB();
        void scanForMissing();
	void dateChanged(const QDate &);
        void showTreeContextMenuPopup(const QPoint &);
        void showContextMenuPopup(const QPoint &);
        void deleteInterval();
        void renameInterval();
        void zoomInterval();
        void frontInterval();
        void backInterval();
        void intervalEdited(QTreeWidgetItem *, int);
#ifdef GC_HAVE_LIBOAUTH
        void tweetRide();
#endif

        // working with measures, not rides
        void recordMeasure();
        void downloadMeasures();
        void exportMeasures();
        void importMeasures();

        // get calendars
        void refreshCalendar();
        void importCalendar();
        void exportCalendar();

        void helpView();

        void selectHome();
        void selectDiary();
        void selectAnalysis();
        void selectTrain();
        void selectAthlete();

        void showDock();
    protected:

        static QString notesFileName(QString rideFileName);

    private:
        QToolBox *toolBox;
        QToolBar *toolbar;
	QDockWidget *dock;
        QAction *homeAct, *diaryAct, *analysisAct, *measuresAct, *trainAct, *athleteAct, *helpAct, *configAct;

	bool parseRideFileName(const QString &name, QString *notesFileName, QDateTime *dt);
        struct TabInfo {
            GcWindow *contents;
            QString name;
            QAction *action;
            TabInfo(GcWindow *contents, QString name) :
                contents(contents), name(name), action(NULL) {}
        };
        QList<TabInfo> tabs;

        boost::shared_ptr<QSettings> settings;
        IntervalItem *activeInterval; // currently active for context menu popup
        RideItem *activeRide; // currently active for context menu popup

        QStackedWidget *views;

        // each view has its own controls XXX more to come
        QStackedWidget *masterControls,
                       *analysisControls,
                       *trainControls,
                       *diaryControls,
                       *homeControls;

        QAction *sideView;
        QAction *toolView;

        // Analysis
        RideCalendar *calendar;
        QSplitter *splitter;
        QSplitter *metaSplitter;
        QTreeWidget *treeWidget;
        QSplitter *intervalSplitter;
        QTreeWidget *intervalWidget;
        IntervalSummaryWindow *intervalSummaryWindow;
        QTabWidget *tabWidget;
        SummaryWindow *summaryWindow;
        DiaryWindow *diaryWindow;
        HomeWindow *homeWindow;
        HomeWindow *trainWindow;
        AllPlotWindow *allPlotWindow;
        HistogramWindow *histogramWindow;
        WeeklySummaryWindow *weeklySummaryWindow;
        LTMWindow *ltmWindow;
        CriticalPowerWindow *criticalPowerWindow;
        ModelWindow *modelWindow;
        ScatterWindow *scatterWindow;
        AerolabWindow *aerolabWindow;
        GoogleMapControl *googleMap;
        TreeMapWindow *treemapWindow;
        RideEditor *rideEdit;
        RideNavigator *rideNavigator;
        QTreeWidgetItem *allRides;
        QTreeWidgetItem *allIntervals;
        QSplitter *leftLayout;
        RideMetadata *_rideMetadata;
        QSplitter *summarySplitter;

        //QwtPlotCurve *weeklyBSCurve;
        //QwtPlotCurve *weeklyRICurve;
	PerformanceManagerWindow *performanceManagerWindow;


        // pedal force/pedal velocity scatter plot widgets
        PfPvWindow  *pfPvWindow;

        HrPwWindow  *hrPwWindow;

	RideItem *ride;  // the currently selected ride


    QuarqdClient *client;

    QSignalMapper *toolMapper;
    WithingsDownload *withingsDownload;

    GcWindowTool *chartTool;
};

#endif // _GC_MainWindow_h
