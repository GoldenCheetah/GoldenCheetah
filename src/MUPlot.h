/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2014 Andy Froncioni
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

#ifndef _GC_MUPlot_h
#define _GC_MUPlot_h 1
#include "GoldenCheetah.h"
#include "MUPool.h"
#include <math.h> // for erf()

#include "CriticalPowerWindow.h"

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_zoomer.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_spectrocurve.h>
#include <qwt_point_3d.h>
#include <qwt_compat.h>

// series data from a function
#include <qwt_series_data.h>
#include <qwt_point_data.h>

#include <QtGui>
#include <QMessageBox>

class QwtPlotCurve;
class QwtPlotGrid;
class QwtPlotMarker;
class Context;
class MUNormal;
class MUWidget;

extern double PI;

// these two affect performance of the MU -> MMP code
// the lower the number of bins and highter the increment
// the faster the MMP is computed, but obviously this will
// affect the smoothness of the curve

#define MUN 250 // bin count
#define MUSAMPLES 50 // watts increment for MMP extraction

class MUPlot : public QwtPlot, public QwtSyntheticPointData
{
    Q_OBJECT
    G_OBJECT

    public:

        MUPlot(MUWidget *muw, CriticalPowerWindow *parent, Context *);

    public slots:

        // colors/appearance changed
        void configChanged();

        // setup the model curve(s) with the model type passed
        // 0 = no model, 1 = normal distribution
        void setModel(int);
        virtual double y(double x) const;
        double totalY();

        // watch for user interaction... need to fix OverlayWidget
        bool eventFilter(QObject *, QEvent *);
        void mouseMoved();

        // set the MUSet when distribution changes
        void setMUSet();

    private:

        Context *context;
	MUWidget *muw;
        CriticalPowerWindow *parent;

        // utility
        void setAxisTitle(int axis, QString label);

	QVector<MUPool> *muSet; // set of Motor Units set with distribution

        // model being used
        int model;
        QwtPlotCurve *modelCurve; // all additive

        // may want to abstract this out...
        QwtPlotMarker *slowHandle; 
        QwtPlotMarker *slowLine;
        QwtPlotCurve *slowCurve;
        MUNormal *slowNormal;
        bool slowDrag;

        QwtPlotMarker *fastHandle; 
        QwtPlotMarker *fastLine;
        QwtPlotCurve *fastCurve;
        MUNormal *fastNormal;
        bool fastDrag;

        QwtPlotCurve *mmpCurve; // placed onto CPPlot!
};

//just a boring old normal
class MUNormal : public QwtSyntheticPointData
{
    public:
        MUNormal(double mean, double variance) : QwtSyntheticPointData(MUN), mean(mean), variance(variance) {

            // initialise A+B phi
            calcPhi();
        }

        // when mean or variance change we need to recompute A + B phis
        void calcPhi() {
            // when we set mean / variance recalculate phis
            Phi_A = 0.5*( 1 + erf( (0.0f-mean)/sqrt(2.0f*variance) ));
            Phi_B =  0.5*( 1 + erf( (1.0f-mean)/sqrt(2.0f*variance) )) - Phi_A;
        }

        // change the mean
        void setMean(double x) {
            mean = x;
            calcPhi();
        }

        // change the variance
        void setVariance(double x) {
            variance = x;
            calcPhi();
        }
        

        // return the y value for x between 0 and 1
        virtual double y(double x) const {

            double xx = (x - mean);
            double phi = 1/( sqrt(variance)*sqrt( 2.0 * PI )) * exp( -1.0f*xx*xx/(2.0*variance) ) / Phi_B;

            return phi;
        }

    private:

        double mean, variance;
        double Phi_A, Phi_B;
};

// qwt breaks QWidget::mouseMoveEvent, so we have to use their dogshit
// quite whats wrong with the QT event handling is beyond me.
class muMouseTracker: public QwtPlotPicker
{
public:

    muMouseTracker(MUPlot *me) : QwtPlotPicker(me->canvas()), me(me) {
        setTrackerMode(AlwaysOn);
    }

protected:

    virtual QwtText trackerText(const QPoint &) const {
        me->mouseMoved();
        return QwtText("");
    }

private:
    MUPlot *me;
};
#endif // _GC_MUPlot_h
