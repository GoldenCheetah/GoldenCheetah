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
#include "GoldenCheetah.h"

#include <QtGui>
#include <QObject> // for Q_PROPERTY
#include <QFormLayout>
#include <QStyle>
#include <QStyleFactory>

class AllPlot;
class Context;
class QwtPlotPanner;
class QwtPlotZoomer;
class QwtPlotPicker;
class QwtPlotMarker;
class QwtArrowButton;
class RideItem;
class IntervalItem;
class QxtSpanSlider;
class QxtGroupBox;
class WPrime;

#include "LTMWindow.h" // for tooltip/canvaspicker

class AllPlotWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // the plot properties are used by the layout manager
    // to save and restore the plot parameters, this is so
    // we can have multiple windows open at once with different
    // settings managed by the user.
    // in this example we might show a stacked plot and a ride
    // plot at the same time.
    Q_PROPERTY(bool stacked READ isStacked WRITE setStacked USER true)
    Q_PROPERTY(bool byseries READ isBySeries WRITE setBySeries USER true)
    Q_PROPERTY(int stackWidth READ _stackWidth WRITE setStackWidth USER true)
    Q_PROPERTY(int showGrid READ isShowGrid WRITE setShowGrid USER true)
    Q_PROPERTY(int showFull READ isShowFull WRITE setShowFull USER true)
    Q_PROPERTY(int showHr READ isShowHr WRITE setShowHr USER true)
    Q_PROPERTY(int showNP READ isShowNP WRITE setShowNP USER true)
    Q_PROPERTY(int showXP READ isShowXP WRITE setShowXP USER true)
    Q_PROPERTY(int showAP READ isShowAP WRITE setShowAP USER true)
    Q_PROPERTY(int showSpeed READ isShowSpeed WRITE setShowSpeed USER true)
    Q_PROPERTY(int showCad READ isShowCad WRITE setShowCad USER true)
    Q_PROPERTY(int showAlt READ isShowAlt WRITE setShowAlt USER true)
    Q_PROPERTY(int showTorque READ isShowTorque WRITE setShowTorque USER true)
    Q_PROPERTY(int showPower READ isShowPower WRITE setShowPower USER true)
    Q_PROPERTY(int showBalance READ isShowBalance WRITE setShowBalance USER true)
    Q_PROPERTY(int showTemp READ isShowTemp WRITE setShowTemp USER true)
    Q_PROPERTY(int showW READ isShowW WRITE setShowW USER true)
    Q_PROPERTY(int byDistance READ isByDistance WRITE setByDistance USER true)
    Q_PROPERTY(int smoothing READ smoothing WRITE setSmoothing USER true)
    Q_PROPERTY(int paintBrush READ isPaintBrush WRITE setPaintBrush USER true)

    public:

        AllPlotWindow(Context *context);
        void setData(RideItem *ride);

        bool hasReveal() { return true; }

        // highlight a selection on the plots
        void setStartSelection(AllPlot* plot, double xValue);
        void setEndSelection(AllPlot* plot, double xValue, bool newInterval, QString name);
        void clearSelection();
        void hideSelection();

        // get properties - the setters are below
        bool isStacked() const { return showStack->isChecked(); }
        bool isBySeries() const { return showBySeries->isChecked(); }
        int _stackWidth() const { return stackWidth; }
        int isShowGrid() const { return showGrid->checkState(); }
        int isShowFull() const { return showFull->checkState(); }
        int isShowHr() const { return showHr->checkState(); }
        int isShowNP() const { return showNP->checkState(); }
        int isShowXP() const { return showXP->checkState(); }
        int isShowAP() const { return showAP->checkState(); }
        int isShowSpeed() const { return showSpeed->checkState(); }
        int isShowCad() const { return showCad->checkState(); }
        int isShowAlt() const { return showAlt->checkState(); }
        int isShowTorque() const { return showTorque->checkState(); }
        int isShowPower() const { return showPower->currentIndex(); }
        int isShowBalance() const { return showBalance->checkState(); }
        int isShowTemp() const { return showTemp->checkState(); }
        int isShowW() const { return showW->checkState(); }
        int isByDistance() const { return comboDistance->currentIndex(); }
        int isPaintBrush() const { return paintBrush->isChecked(); }
        int smoothing() const { return smoothSlider->value(); }

   public slots:

        // trap GC signals
        void rideSelected();
        void rideDeleted(RideItem *ride);
        void intervalSelected();
        void zonesChanged();
        void intervalsChanged();
        void configChanged();

        // set properties
        void setSmoothingFromSlider();
        void setSmoothingFromLineEdit();
        void setrSmoothingFromSlider();
        void setrSmoothingFromLineEdit();
        void setStackWidth(int x);
        void setShowPower(int state);
        void setShowHr(int state);
        void setShowNP(int state);
        void setShowXP(int state);
        void setShowAP(int state);
        void setShowSpeed(int state);
        void setShowCad(int state);
        void setShowAlt(int state);
        void setShowTemp(int state);
        void setShowWind(int state);
        void setShowTorque(int state);
        void setShowBalance(int state);
        void setShowW(int state);
        void setShowGrid(int state);
        void setPaintBrush(int state);
        void setShowFull(int state);
        void setSmoothing(int value);
        void setByDistance(int value);
        void setStacked(int value);
        void setBySeries(int value);

        // trap widget signals
        void zoomChanged();
        void zoomOut();
        void zoomInterval(IntervalItem *);
        void stackZoomSliderChanged();
        void resizeSeriesPlots();
        void moveLeft();
        void moveRight();
        void showStackChanged(int state);
        void showBySeriesChanged(int state);

    protected:

        // whilst we refactor, lets make friend
        friend class IntervalPlotData;
        friend class Context;
        friend class AllPlot;

        void setAllPlotWidgets(RideItem *rideItem);

        // cached state
        RideItem *current;
        int selection;
        Context *context;
        WPrime *wpData;

        // All the plot widgets
        QVBoxLayout *allPlotLayout;
        AllPlot *allPlot;
        AllPlot *fullPlot;
        QList <AllPlot *> allPlots;
        QList <AllPlot *> seriesPlots;
        QwtPlotPanner *allPanner;
        QwtPlotZoomer *allZoomer;

        // Stacked view
        QScrollArea *stackFrame;
        QVBoxLayout *stackPlotLayout;
        QWidget *stackWidget;

        // series stack view
        QScrollArea *seriesstackFrame;
        QVBoxLayout *seriesstackPlotLayout;
        QWidget *seriesstackWidget;

        // stack zoomer for setting stack width
        // has 7 settings from 0 - 6
        QSlider *stackZoomSlider;
        const int stackZoomWidth[7] = { 5, 10, 15, 20, 30, 45, 60 };

        // Normal view
        QScrollArea *allPlotFrame;
        QPushButton *scrollLeft, *scrollRight;

        // Common controls
        QGridLayout *controlsLayout;
        QCheckBox *showStack;
        QCheckBox *showBySeries;
        QCheckBox *showGrid;
        QCheckBox *showFull;
        QCheckBox *paintBrush;
        QCheckBox *showHr;
        QCheckBox *showNP;
        QCheckBox *showXP;
        QCheckBox *showAP;
        QCheckBox *showSpeed;
        QCheckBox *showCad;
        QCheckBox *showAlt;
        QCheckBox *showTemp;
        QCheckBox *showWind;
        QCheckBox *showTorque;
        QCheckBox *showBalance;
        QCheckBox *showW;
        QComboBox *showPower;
        QComboBox *comboDistance;
        QSlider *smoothSlider;
        QLineEdit *smoothLineEdit;
        QxtSpanSlider *spanSlider;

    private:
        // reveal controls
        QLabel *rSmooth;
        QSlider *rSmoothSlider;
        QLineEdit *rSmoothEdit;
        QCheckBox *rStack, *rBySeries, *rFull;
        QStackedWidget *allPlotStack;

        // reset/redraw all the plots
        void setupStackPlots();
        void forceSetupSeriesStackPlots();
        void setupSeriesStackPlots();
        void redrawAllPlot();
        void redrawFullPlot();
        void redrawStackPlot();

        void showInfo(QString);
        void resetStackedDatas();
        void resetSeriesStackedDatas();
        int stackWidth;

        bool active;
        bool stale;
        bool setupStack; // we optimise this out, its costly
        bool setupSeriesStack; // we optimise this out, its costly

    private slots:

        void addPickers(AllPlot *allPlot2);
        void plotPickerMoved(const QPoint &);
        void plotPickerSelected(const QPoint &);
};

#endif // _GC_AllPlotWindow_h
