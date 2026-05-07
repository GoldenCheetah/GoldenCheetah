/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_RealtimePlotWindow_h
#define _GC_RealtimePlotWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QObject> // for Q_PROPERTY
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>


#include "Context.h"
#include "RideFile.h" // for data series types
#include "RealtimePlot.h"
#include "RealtimeData.h" // for realtimedata structure

#include "Settings.h"
#include "Units.h"
#include "Colors.h"

class RealtimePlotWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int showHr READ isShowHr WRITE setShowHr USER true)
    Q_PROPERTY(int showSpeed READ isShowSpeed WRITE setShowSpeed USER true)
    Q_PROPERTY(int showCad READ isShowCad WRITE setShowCad USER true)
    Q_PROPERTY(int showAlt READ isShowAlt WRITE setShowAlt USER true)
    Q_PROPERTY(int showPower READ isShowPower WRITE setShowPower USER true)
    Q_PROPERTY(int showHHb READ isShowHHb WRITE setShowHHb USER true)
    Q_PROPERTY(int showO2Hb READ isShowO2Hb WRITE setShowO2Hb USER true)
    Q_PROPERTY(int showtHb READ isShowtHb WRITE setShowtHb USER true)
    Q_PROPERTY(int showSmO2 READ isShowSmO2 WRITE setShowSmO2 USER true)
    Q_PROPERTY(int showPow30s READ isShowPow30s WRITE setShowPow30s USER true)
    Q_PROPERTY(int smoothing READ smoothing WRITE setSmoothing USER true)

    public:

        RealtimePlotWindow(Context *context);

        // get properties - the setters are below
        int isShowHr() const { return showHr->checkState(); }
        int isShowSpeed() const { return showSpeed->checkState(); }
        int isShowCad() const { return showCad->checkState(); }
        int isShowAlt() const { return showAlt->checkState(); }
        int isShowPower() const { return showPower->checkState(); }
        int isShowHHb() const { return showHHb->checkState(); }
        int isShowO2Hb() const { return showO2Hb->checkState(); }
        int isShowtHb() const { return showtHb->checkState(); }
        int isShowSmO2() const { return showSmO2->checkState(); }
        int isShowPow30s() const { return showPow30s->checkState(); }
        int smoothing() const { return smoothSlider->value(); }

   public slots:

        // trap signals
        void telemetryUpdate(const RealtimeData &rtData); // got new data
        void configChanged(qint32);
        void start();
        void stop();
        void pause();

        // set properties
        void setSmoothingFromSlider();
        void setSmoothingFromLineEdit();
        void setShowPower(int state);
        void setShowHHb(int state);
        void setShowO2Hb(int state);
        void setShowSmO2(int state);
        void setShowtHb(int state);
        void setShowPow30s(int state);
        void setShowHr(int state);
        void setShowSpeed(int state);
        void setShowCad(int state);
        void setShowAlt(int state);
        void setSmoothing(int value);

    private:

        Context *context;
        RealtimePlot *rtPlot;
        bool active;

        // Common controls
        QGridLayout *controlsLayout;
        QCheckBox *showHr;
        QCheckBox *showSpeed;
        QCheckBox *showCad;
        QCheckBox *showAlt;
        QCheckBox *showPower;
        QCheckBox *showHHb;
        QCheckBox *showO2Hb;
        QCheckBox *showSmO2;
        QCheckBox *showtHb;
        QCheckBox *showPow30s;
        QSlider *smoothSlider;
        QLineEdit *smoothLineEdit;

        // for smoothing charts
        double powHist[150];
        double powtot;
        int powindex;
        double altHist[150];
        double alttot;
        int altindex;
        double spdHist[150];
        double spdtot;
        int spdindex;
        double cadHist[150];
        double cadtot;
        int cadindex;
        double hrHist[150];
        double hrtot;
        int hrindex;
        double hhbHist[150];
        double hhbtot;
        int hhbindex;
        double o2hbHist[150];
        double o2hbtot;
        int o2hbindex;
        double smo2Hist[150];
        double smo2tot;
        int smo2index;
        double thbHist[150];
        double thbtot;
        int thbindex;
};

#endif // _GC_RealtimePlotWindow_h
