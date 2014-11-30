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
#include <QCheckBox>
#include <QFormLayout>

#include "qxtstringspinbox.h"
#include "Context.h"
#include "RideItem.h"
#include "IntervalItem.h"
#include "math.h"
#include "Units.h" // for MILES_PER_KM


class ScatterPlot; // we don't include the header because it uses namespaces
class ScatterDataColor;

class ScatterSettings
{
    public:
        ScatterSettings() : ride(NULL), x(0), y(0), crop(false), ignore(false), frame(false),
                             gridlines(false), secStart(0), secEnd(0) {}

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


class ScatterWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int xseries READ xseries WRITE setXSeries USER true)
    Q_PROPERTY(int yseries READ yseries WRITE setYSeries USER true)
    Q_PROPERTY(bool ignore READ isIgnore WRITE set_Ignore USER true)
    Q_PROPERTY(bool grid READ isGrid WRITE set_Grid USER true)
    Q_PROPERTY(bool frame READ isFrame WRITE set_Frame USER true)

    public:

        ScatterWindow(Context *);

        // compare is supported
        bool isCompare() const { return context->isCompareIntervals; }

        // reveal
        bool hasReveal() { return true; }

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

    public slots:
        void rideSelected();
        void intervalSelected();
        void setData();

        void xSelectorChanged(int);
        void ySelectorChanged(int);
        void rxSelectorChanged(int);
        void rySelectorChanged(int);
        void configChanged();

        // these set the plot when the properties change
        void setGrid();
        void setFrame();
        void setIgnore();
        void setrFrame();
        void setrIgnore();

        // compare mode started or items to compare changed
        void compareChanged();

    protected:

        // passed from Context *
        Context *context;
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

        RideItem *current;

    private:
        // reveal controls
        QxtStringSpinBox    *rxSelector,
                            *rySelector;
        QCheckBox           *rFrameInterval,
                            *rIgnore;

        void addStandardChannels(QComboBox *);
        void addrStandardChannels(QxtStringSpinBox *box);
};

#endif // _GC_ScatterWindow_h
