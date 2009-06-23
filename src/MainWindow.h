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

class AllPlot;
class CpintPlot;
class PfPvPlot;
class PowerHist;
class QwtPlotPanner;
class QwtPlotPicker;
class QwtPlotZoomer;
class RideFile;
class Zones;

class MainWindow : public QMainWindow 
{
    Q_OBJECT

    public:
        MainWindow(const QDir &home);
        void addRide(QString name, bool bSelect=true);
        void removeCurrentRide();
        const RideFile *currentRide();
        QDir home;

    protected:
        virtual void resizeEvent(QResizeEvent*);
        virtual void moveEvent(QMoveEvent*);
        virtual void closeEvent(QCloseEvent*);
    
    private slots:
        void rideSelected();
        void splitterMoved();
        void newCyclist();
        void openCyclist();
        void downloadRide();
        void exportCSV();
        void exportXML();
        void importCSV();
        void importSRM();
        void importTCX();
        void importPolar();
        void findBestIntervals();
        void splitRide();
        void deleteRide();
	void setAllPlotWidgets(RideItem *rideItem);
	void setHistWidgets(RideItem *rideItem);
        void setSmoothingFromSlider();
        void setSmoothingFromLineEdit();
	void cpintSetCPButtonClicked();
        void setBinWidthFromSlider();
        void setBinWidthFromLineEdit();
	void setlnYHistFromCheckBox();
        void setWithZerosFromCheckBox();
        void setHistSelection(int id);
        void setQaCPFromLineEdit();
        void setQaCADFromLineEdit();
        void setQaCLFromLineEdit();
        void setShadeZonesPfPvFromCheckBox();
        void tabChanged(int index);
        void pickerMoved(const QPoint &);
        void aboutDialog();
        void notesChanged();
        void saveNotes();
        void showOptions();
        void showTools();
	void importRideToDB();
        void scanForMissing();
	void generateWeeklySummary();

    protected: 

        static QString notesFileName(QString rideFileName);

    private:
	bool parseRideFileName(const QString &name, QString *notesFileName, QDateTime *dt);
	void setHistBinWidthText();
	void setHistTextValidator();

        QSettings settings;

        QSplitter *splitter;
        QTreeWidget *treeWidget;
        QTabWidget *tabWidget;
        QTextEdit *rideSummary;
        QTextEdit *weeklySummary;
        AllPlot *allPlot;
        QwtPlotZoomer *allZoomer;
        QwtPlotPanner *allPanner;
	QCheckBox *showHr;
	QCheckBox *showSpeed;
	QCheckBox *showCad;
	QComboBox *showPower;
        CpintPlot *cpintPlot;
        QLineEdit *cpintTimeValue;
        QLineEdit *cpintTodayValue;
        QLineEdit *cpintAllValue;
	QPushButton *cpintSetCPButton;
        QwtPlotPicker *picker;
        QSlider *smoothSlider;
        QLineEdit *smoothLineEdit;
        QSlider *binWidthSlider;
        QLineEdit *binWidthLineEdit;
        QCheckBox *lnYHistCheckBox;
        QCheckBox *withZerosCheckBox;
        QComboBox *histParameterCombo;
        QCheckBox *shadeZonesPfPvCheckBox;
        QTreeWidgetItem *allRides;
        PowerHist *powerHist;
        QwtPlot *weeklyPlot;
        QwtPlotCurve *weeklyDistCurve;
        QwtPlotCurve *weeklyDurationCurve;
        QwtPlotCurve *weeklyBaselineCurve;
        QwtPlotCurve *weeklyBSBaselineCurve;
        QwtPlot *weeklyBSPlot;

        QwtPlotCurve *weeklyBSCurve;
        QwtPlotCurve *weeklyRICurve;

        Zones *zones;

        // pedal force/pedal velocity scatter plot widgets
        PfPvPlot  *pfPvPlot;
        QLineEdit *qaCPValue;
        QLineEdit *qaCadValue;
        QLineEdit *qaClValue;

        QTextEdit *rideNotes;
        QString currentNotesFile;
        bool currentNotesChanged;

	RideItem *ride;  // the currently selected ride

	int histWattsShadedID;
	int histWattsUnshadedID;
	int histNmID;
	int histHrID;
	int histKphID;
	int histCadID;

	bool useMetricUnits;  // whether metric units are used (or imperial)
};

#endif // _GC_MainWindow_h

