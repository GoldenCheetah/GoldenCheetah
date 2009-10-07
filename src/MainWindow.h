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
#include <QtGui>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include "RideItem.h"
#include <boost/shared_ptr.hpp>

class AllPlotWindow;
class CriticalPowerWindow;
class HistogramWindow;
class PfPvWindow;
class QwtPlotPanner;
class QwtPlotPicker;
class QwtPlotZoomer;
class RideFile;
class WeeklySummaryWindow;
class Zones;
class RideCalendar;

class MainWindow : public QMainWindow 
{
    Q_OBJECT

    public:
        MainWindow(const QDir &home);
        void addRide(QString name, bool bSelect=true);
        void removeCurrentRide();
        const RideFile *currentRide();
	void getBSFactors(float &timeBS, float &distanceBS);
        QDir home;
        void setCriticalPower(int cp);

    protected:
        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void closeEvent(QCloseEvent*);
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dropEvent(QDropEvent *);
    
    private slots:
        void rideSelected();
        void leftLayoutMoved();
        void splitterMoved();
        void newCyclist();
        void openCyclist();
        void downloadRide();
        void manualRide();
        void exportCSV();
        void exportXML();
        void importFile();
        void findBestIntervals();
        void splitRide();
        void deleteRide();
        void tabChanged(int index);
        void aboutDialog();
        void notesChanged();
        void saveNotes();
        void showOptions();
        void showTools();
	void importRideToDB();
        void scanForMissing();
	void saveAndOpenNotes();
	void dateChanged(const QDate &);

    protected: 

        static QString notesFileName(QString rideFileName);

    private:
	bool parseRideFileName(const QString &name, QString *notesFileName, QDateTime *dt);

	boost::shared_ptr<QSettings> settings;

        RideCalendar *calendar;
        QSplitter *splitter;
        QTreeWidget *treeWidget;
        QTabWidget *tabWidget;
        QTextEdit *rideSummary;
        AllPlotWindow *allPlotWindow;
        HistogramWindow *histogramWindow;
        WeeklySummaryWindow *weeklySummaryWindow;
        CriticalPowerWindow *criticalPowerWindow;
        QTreeWidgetItem *allRides;
        QSplitter *leftLayout;

        Zones *zones;

        // pedal force/pedal velocity scatter plot widgets
        PfPvWindow  *pfPvWindow;

        QTextEdit *rideNotes;
        QString currentNotesFile;
        bool currentNotesChanged;

	RideItem *ride;  // the currently selected ride

	bool useMetricUnits;  // whether metric units are used (or imperial)

	float timebsfactor;
	float distancebsfactor;
};

#endif // _GC_MainWindow_h

