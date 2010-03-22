/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_AllPlotWindow_h
#define _GC_AllPlotWindow_h 1

#include <QtGui>

class AllPlot;
class MainWindow;
class QwtPlotPanner;
class QwtPlotZoomer;
class QwtPlotPicker;
class QwtPlotMarker;
class QwtArrowButton;
class RideItem;
class IntervalItem;

class AllPlotWindow : public QWidget
{
    Q_OBJECT

    public:

        AllPlotWindow(MainWindow *mainWindow);
        void setData(RideItem *ride);
        void setStartSelection(AllPlot* plot, int xPosition);
        void setEndSelection(AllPlot* plot, int xPosition, bool newInterval, QString name);
        void clearSelection();
        void hideSelection();
        void zoomInterval(IntervalItem *); // zoom into a specified interval

   public slots:

        void setSmoothingFromSlider();
        void setSmoothingFromLineEdit();
        void rideSelected();
        void intervalSelected();
        void zonesChanged();
        void intervalsChanged();
        void configChanged();

        void setStackZoomUp();
        void setStackZoomDown();

        void setShowStack(int state);
        void setShowPower(int state);
        void setShowHr(int state);
        void setShowSpeed(int state);
        void setShowCad(int state);
        void setShowAlt(int state);
        void setShowGrid(int state);
        void setSmoothing(int value);
        void setByDistance(int value);

    protected:

        // whilst we refactor, lets make friend
        friend class IntervalPlotData;
        friend class MainWindow;

	void setAllPlotWidgets(RideItem *rideItem);

        MainWindow *mainWindow;
        AllPlot *allPlot;
        QList <AllPlot *> allPlots;
        QList <QwtPlotPicker *> allPickers;

        QScrollArea *stackFrame;
        QwtPlotPanner *allPanner;
        QwtPlotZoomer *allZoomer;
        QwtPlotPicker *allPicker;
        int selection;
        QCheckBox *showStack;

        QwtArrowButton *stackZoomDown;
        QwtArrowButton *stackZoomUp;

	QCheckBox *showHr;
	QCheckBox *showSpeed;
	QCheckBox *showCad;
	QCheckBox *showAlt;
	QComboBox *showPower;
        QSlider *smoothSlider;
        QLineEdit *smoothLineEdit;

    private:
        void showInfo(QString);
        void resetStackedDatas();
        int stackWidth;

    private slots:
        void addPickers(AllPlot *allPlot2);

        void plotPickerMoved(const QPoint &);
        void plotPickerSelected(const QPoint &);
};

#endif // _GC_AllPlotWindow_h

