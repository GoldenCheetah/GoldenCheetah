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

#ifndef _GC_ModelWindow_h
#define _GC_ModelWindow_h 1

#include <QtGui>
#include <QTimer>
#include "MainWindow.h"

class ModelPlot; // we don't include the header because it uses namespaces
class ModelDataColor;

class ModelSettings
{
    public:
        RideItem *ride;   // ride to use
        int x,y,z,color;  // which channels to use
        int xbin, ybin;   // bin size
        bool crop,        // crop to non-zero z values
             ignore,      // ignore zeroes on x or y
             adjustPlot,  // reset plot settings to dataset
             gridlines,   // show gridlines
             frame,       // max z when showing intervals?
             legend;      // display a legend?
        int  zpane;       // where to show zpane 0-100% of the z-axis
        QList<IntervalItem *> intervals; // intervals to apply

        ModelDataColor *colorProvider; // color model to update
};


class ModelWindow : public QWidget
{
    Q_OBJECT

    public:

        ModelWindow(MainWindow *, const QDir &);

    public slots:
        void rideSelected();
        void intervalSelected();
        void applyPreset(int);
        void setData(bool);
        void setGrid();
        void setLegend();
        void setFrame();
        void setZPane(int);
        void resetViewPoint();
        void setBinWidthFromSlider();
        void setBinWidthFromLineEdit();
        void styleSelected(int);
        void setDirty();
        void setClean();

    protected:

        // passed from MainWindow
        QDir home;
        MainWindow *main;
        bool useMetricUnits;
        bool active;

        bool dirty;             // settings changed but not reploted
        ModelSettings settings; // last used settings

        // Ride to plot - captured from rideSelected signal
        RideItem *ride;

        // layout
        ModelPlot *modelPlot;

        // labels
        QLabel *presetLabel,
               *xLabel,
               *yLabel,
               *zLabel,
               *colorLabel,
               *binLabel;

        // top of screen selectors
        QComboBox   *presetValues;

        // bottom selectors
        QComboBox   *xSelector,
                    *ySelector,
                    *zSelector,
                    *colorSelector,
                    *styleSelector;
        QCheckBox   *ignore,
                    *grid,
                    *frame,
                    *legend;
        QPushButton *resetView;

        QLineEdit *binWidthLineEdit;
        QSlider *binWidthSlider;

        // z pane slider
        QSlider *zpane;

    private:

        void addStandardChannels(QComboBox *);
        void fillPresets(QComboBox *);
};

#endif // _GC_ModelWindow_h
