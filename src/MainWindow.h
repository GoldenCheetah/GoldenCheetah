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

#include <QDir>
#include <QSqlDatabase>
#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include "RideItem.h"
#include "IntervalItem.h"
#include "QuarqdClient.h"
#include <boost/shared_ptr.hpp>

class AerolabWindow;
class GoogleMapControl;
class AllPlotWindow;
class CriticalPowerWindow;
class HistogramWindow;
class PfPvWindow;
class QwtPlotPanner;
class QwtPlotPicker;
class QwtPlotZoomer;
class LTMWindow;
class MetricAggregator;
class ModelWindow;
class RealtimeWindow;
class RideFile;
class RideMetadata;
class WeeklySummaryWindow;
class Zones;
class RideCalendar;
class PerformanceManagerWindow;
class RideSummaryWindow;
class ViewSelection;
class TrainWindow;

class MainWindow : public QMainWindow 
{
    Q_OBJECT

    public:
        MainWindow(const QDir &home);
        void addRide(QString name, bool bSelect=true);
        void removeCurrentRide();
        const RideFile *currentRide();
        const RideItem *currentRideItem() { return ride; }
        const QTreeWidgetItem *allRideItems() { return allRides; }
        const QTreeWidgetItem *allIntervalItems() { return allIntervals; }
        QTreeWidgetItem *mutableIntervalItems() { return allIntervals; }
	void getBSFactors(double &timeBS, double &distanceBS,
                          double &timeDP, double &distanceDP);
        QDir home;
        void setCriticalPower(int cp);

        const Zones *zones() const { return zones_; }
        void updateRideFileIntervals();
        void saveSilent(RideItem *);
        bool saveRideSingleDialog(RideItem *);
        RideItem *rideItem() const { return ride; }
        const QWidget *activeTab() const { return tabWidget->currentWidget(); }
        QTextEdit *rideNotesWidget() { return rideNotes; }
        RideMetadata *rideMetadata() { return _rideMetadata; }

        void notifyConfigChanged(); // used by ConfigDialog to notify MainWindow
                                    // when config has changed - and to get a
                                    // signal emitted to notify its children
        void notifyRideSelected();  // used by RideItem to notify when
                                    // rideItem date/time changes
        void selectView(int);

        // db connections to cyclistdir/metricDB - one per active MainWindow
        QSqlDatabase db;
        int session;
        bool isclean;

    protected:

        Zones *zones_;

        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void closeEvent(QCloseEvent*);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);
    
    signals:

        void rideSelected();
        void intervalSelected();
        void intervalsChanged();
        void zonesChanged();
        void configChanged();
        void viewChanged(int);
        void rideAdded(RideItem *);
        void rideDeleted(RideItem *);

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
        void exportCSV();
        void exportGC();
        void importFile();
        void findBestIntervals();
        void addIntervalForPowerPeaksForSecs(RideFile *ride, int windowSizeSecs, QString name);
        void findPowerPeaks();
        void splitRide();
        void deleteRide();
        void tabChanged(int index);
        void aboutDialog();
        void notesChanged();
        void saveRide();                        // save current ride menu item
        bool saveRideExitDialog();              // save dirty rides on exit dialog
        void saveNotes();
        void showOptions();
        void showTools();
	void importRideToDB();
        void scanForMissing();
	void saveAndOpenNotes();
	void dateChanged(const QDate &);
        void showTreeContextMenuPopup(const QPoint &);
        void showContextMenuPopup(const QPoint &);
        void deleteInterval();
        void renameInterval();
        void zoomInterval();
        void frontInterval();
        void backInterval();
        void intervalEdited(QTreeWidgetItem *, int);

    protected: 

        static QString notesFileName(QString rideFileName);

    private:
	bool parseRideFileName(const QString &name, QString *notesFileName, QDateTime *dt);

        struct TabInfo {
            QWidget *contents;
            QString name;
            QAction *action;
            TabInfo(QWidget *contents, QString name) :
                contents(contents), name(name), action(NULL) {}
        };
        QList<TabInfo> tabs;

	boost::shared_ptr<QSettings> settings;
        IntervalItem *activeInterval; // currently active for context menu popup
        RideItem *activeRide; // currently active for context menu popup

        ViewSelection *viewSelection;
        QStackedWidget *views;

        // Analysis
        RideCalendar *calendar;
        QSplitter *splitter;
        QTreeWidget *treeWidget;
        QSplitter *intervalsplitter;
        QTreeWidget *intervalWidget;
        QTabWidget *tabWidget;
        RideSummaryWindow *rideSummaryWindow;
        AllPlotWindow *allPlotWindow;
        HistogramWindow *histogramWindow;
        WeeklySummaryWindow *weeklySummaryWindow;
        MetricAggregator *metricDB;
        LTMWindow *ltmWindow;
        CriticalPowerWindow *criticalPowerWindow;
        ModelWindow *modelWindow;
        AerolabWindow *aerolabWindow;
        GoogleMapControl *googleMap;
        QTreeWidgetItem *allRides;
        QTreeWidgetItem *allIntervals;
        QSplitter *leftLayout;
        RideMetadata *_rideMetadata;
        QSplitter *summarySplitter;

        // Train
        TrainWindow   *trainWindow;

        QwtPlotCurve *weeklyBSCurve;
        QwtPlotCurve *weeklyRICurve;
	PerformanceManagerWindow *performanceManagerWindow;


        // pedal force/pedal velocity scatter plot widgets
        PfPvWindow  *pfPvWindow;

        QTextEdit *rideNotes;
        QString currentNotesFile;
        bool currentNotesChanged;

	RideItem *ride;  // the currently selected ride

	bool useMetricUnits;  // whether metric units are used (or imperial)

    QuarqdClient *client;
};

#endif // _GC_MainWindow_h

