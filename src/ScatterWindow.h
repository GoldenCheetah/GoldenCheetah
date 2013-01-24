/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_ScatterWindow_h
#define _GC_ScatterWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTimer>
#include "qxtstringspinbox.h"
#include "MainWindow.h"


class ScatterPlot; // we don't include the header because it uses namespaces
class ScatterDataColor;

class ScatterSettings
{
    public:
        RideItem *ride;   // ride to use
        int x,y;  // which channels to use
        bool crop,        // crop to non-zero z values
             ignore,      // ignore zeroes on x or y
             frame,       // frame intervals
             gridlines;   // show gridlines
        int secStart,
            secEnd;       // what time slice to show?
        QList<IntervalItem *> intervals; // intervals to apply
};


class ScatterWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int xseries READ xseries WRITE setXSeries USER true)
    Q_PROPERTY(int yseries READ yseries WRITE setYSeries USER true)
    Q_PROPERTY(bool ignore READ isIgnore WRITE set_Ignore USER true)
    Q_PROPERTY(bool grid READ isGrid WRITE set_Grid USER true)
    Q_PROPERTY(bool frame READ isFrame WRITE set_Frame USER true)
    Q_PROPERTY(int time READ time WRITE set_Time USER true)

    public:

        ScatterWindow(MainWindow *, const QDir &);

        // reveal
        bool hasReveal() { return true; }
        void reveal() { revealControls->show(); }
        void unreveal() { revealControls->hide(); }

        // set/get properties
        int xseries() const { return xSelector->currentIndex(); }
        void setXSeries(int x) { xSelector->setCurrentIndex(x); rxSelector->setValue(x); }
        int yseries() const { return ySelector->currentIndex(); }
        void setYSeries(int x) { ySelector->setCurrentIndex(x); rySelector->setValue(x); }
        bool isGrid() const { return grid->isChecked(); }
        void set_Grid(bool x) { grid->setChecked(x); }
        bool isIgnore() const { return ignore->isChecked(); }
        void set_Ignore(bool x) { ignore->setChecked(x); }
        bool isFrame() const { return frame->isChecked(); }
        void set_Frame(bool x) { frame->setChecked(x); }
        int time() const { return timeSlider->value(); }
        void set_Time(int x) { timeSlider->setValue(x); }

    public slots:
        void rideSelected();
        void intervalSelected();
        void setData();

        void rxSelectorChanged(int);
        void rySelectorChanged(int);

        // these set the plot when the properties change
        void setGrid();
        void setFrame();
        void setIgnore();
        void setrFrame();
        void setrIgnore();
        void setTime(int);

    protected:

        // passed from MainWindow
        QDir home;
        MainWindow *main;
        bool useMetricUnits;
        bool active;

        ScatterSettings settings; // last used settings

        // Ride to plot - captured from rideSelected signal
        RideItem *ride;

        // layout
        ScatterPlot *scatterPlot;

        // labels
        QLabel *xLabel,
               *yLabel,
               *timeLabel;

        // bottom selectors
        QComboBox   *xSelector,
                    *ySelector;
        QCheckBox   *ignore,
                    *grid,
                    *frame;

        QSlider *timeSlider;

        RideItem *current;

    private:
        // reveal controls
        QWidget *revealControls;
        QxtStringSpinBox    *rxSelector,
                            *rySelector;
        QCheckBox           *rFrameInterval,
                            *rIgnore;

        void addStandardChannels(QComboBox *);
        void addrStandardChannels(QxtStringSpinBox *box);

        bool _lockReveal;
};

#endif // _GC_ScatterWindow_h
