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
class QxtSpanSlider;
class QxtGroupBox;

#include "LTMWindow.h" // for tooltip/canvaspicker

class AllPlotWindow : public QWidget
{
    Q_OBJECT

    public:

        AllPlotWindow(MainWindow *mainWindow);
        void setData(RideItem *ride);

        // highlight a selection on the plots
        void setStartSelection(AllPlot* plot, double xValue);
        void setEndSelection(AllPlot* plot, double xValue, bool newInterval, QString name);
        void clearSelection();
        void hideSelection();

        // zoom to interval range (via span-slider)
        void zoomInterval(IntervalItem *);

   public slots:

        // trap GC signals
        void rideSelected();
        void intervalSelected();
        void zonesChanged();
        void intervalsChanged();
        void configChanged();

        // trap child widget signals
        void setSmoothingFromSlider();
        void setSmoothingFromLineEdit();
        void setStackZoomUp();
        void setStackZoomDown();
        void zoomChanged();
        void moveLeft();
        void moveRight();
        void showStackChanged(int state);
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

        // cached state
        RideItem *current;
        int selection;
        MainWindow *mainWindow;

        // All the plot widgets
        AllPlot *allPlot;
        AllPlot *fullPlot;
        QList <AllPlot *> allPlots;
        QwtPlotPanner *allPanner;
        QwtPlotZoomer *allZoomer;

        // Stacked view
        QScrollArea *stackFrame;
        QVBoxLayout *stackPlotLayout;
        QWidget *stackWidget;
        QwtArrowButton *stackZoomDown;
        QwtArrowButton *stackZoomUp;

        // Normal view
        QScrollArea *allPlotFrame;
        QPushButton *scrollLeft, *scrollRight;

        // Common controls
        QGridLayout *controlsLayout;
        QCheckBox *showStack;
        QCheckBox *showHr;
        QCheckBox *showSpeed;
        QCheckBox *showCad;
        QCheckBox *showAlt;
        QComboBox *showPower;
        QSlider *smoothSlider;
        QLineEdit *smoothLineEdit;
        QxtSpanSlider *spanSlider;

    private:
        // reset/redraw all the plots
        void setupStackPlots();
        void redrawAllPlot();
        void redrawFullPlot();
        void redrawStackPlot();

        void showInfo(QString);
        void resetStackedDatas();
        int stackWidth;

        bool active;
        bool stale;

    private slots:

        void addPickers(AllPlot *allPlot2);
        bool stackZoomUpShouldEnable(int sw);
        bool stackZoomDownShouldEnable(int sw);

        void plotPickerMoved(const QPoint &);
        void plotPickerSelected(const QPoint &);
};

#endif // _GC_AllPlotWindow_h

