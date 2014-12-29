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
#include "GoldenCheetah.h"

#include <QtGui>
#include <QTimer>
#include <QDialog>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>
#include "Context.h"

class ModelPlot; // we don't include the header because it uses namespaces
class ModelDataColor;
class IntervalItem;
class RideItem;

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


class ModelWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int style READ style WRITE setStyle USER true)
    Q_PROPERTY(bool grid READ isGrid WRITE setGrid USER true)
    Q_PROPERTY(bool legend READ isLegend WRITE setLegend USER true)
    Q_PROPERTY(bool frame READ isFrame WRITE setFrame USER true)
    Q_PROPERTY(bool ignore READ isIgnore WRITE setIgnore USER true)
    Q_PROPERTY(int preset READ preset WRITE setPreset USER true)
    Q_PROPERTY(int xseries READ xseries WRITE setXSeries USER true)
    Q_PROPERTY(int yseries READ yseries WRITE setYSeries USER true)
    Q_PROPERTY(int zseries READ zseries WRITE setZSeries USER true)
    Q_PROPERTY(int cseries READ cseries WRITE setCSeries USER true)
    Q_PROPERTY(QString bin READ bin WRITE setBin USER true)

    public:

        ModelWindow(Context *);

        // reveal
        bool hasReveal() { return false; }

        // set/get properties
        int preset() const { return presetValues->currentIndex(); }
        void setPreset(int x) { presetValues->setCurrentIndex(x); }
        int xseries() const { return xSelector->currentIndex(); }
        void setXSeries(int x) { xSelector->setCurrentIndex(x); }
        int yseries() const { return ySelector->currentIndex(); }
        void setYSeries(int x) { ySelector->setCurrentIndex(x); }
        int zseries() const { return zSelector->currentIndex(); }
        void setZSeries(int x) { zSelector->setCurrentIndex(x); }
        int cseries() const { return colorSelector->currentIndex(); }
        void setCSeries(int x) { colorSelector->setCurrentIndex(x); }
        int style() const { return styleSelector->currentIndex(); }
        void setStyle(int x) { styleSelector->setCurrentIndex(x); }
        bool isIgnore() const { return ignore->isChecked(); }
        void setIgnore(bool x) { ignore->setChecked(x); }
        bool isGrid() const { return grid->isChecked(); }
        void setGrid(bool x) { grid->setChecked(x); }
        bool isFrame() const { return frame->isChecked(); }
        void setFrame(bool x) { frame->setChecked(x); }
        bool isLegend() const { return legend->isChecked(); }
        void setLegend(bool x) { legend->setChecked(x); }
        QString bin() const { return binWidthLineEdit->text(); }
        void setBin(QString x) { binWidthLineEdit->setText(x); }

    public slots:

        void configChanged(qint32);
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

        // passed from Context *
        Context *context;

        bool active;
        bool dirty;             // settings changed but not reploted
        ModelSettings settings; // last used settings

        // Ride to plot - captured from rideSelected signal
        RideItem *ride;

        // layout
        ModelPlot *modelPlot;

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

        QLineEdit *binWidthLineEdit;
        QSlider *binWidthSlider;

        // z pane slider
        QSlider *zpane;

        RideItem *current;

    private:

        void addStandardChannels(QComboBox *);
        void fillPresets(QComboBox *);
};

#endif // _GC_ModelWindow_h
