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


#include "MainWindow.h"
#include "RideFile.h" // for data series types
#include "RealtimePlot.h"
#include "RealtimeData.h" // for realtimedata structure

#include "Settings.h"
#include "Units.h"
#include "Colors.h"

class RealtimePlotWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int showHr READ isShowHr WRITE setShowHr USER true)
    Q_PROPERTY(int showSpeed READ isShowSpeed WRITE setShowSpeed USER true)
    Q_PROPERTY(int showCad READ isShowCad WRITE setShowCad USER true)
    Q_PROPERTY(int showAlt READ isShowAlt WRITE setShowAlt USER true)
    Q_PROPERTY(int showPower READ isShowPower WRITE setShowPower USER true)
    Q_PROPERTY(int showPow30s READ isShowPow30s WRITE setShowPow30s USER true)
    Q_PROPERTY(int smoothing READ smoothing WRITE setSmoothing USER true)

    public:

        RealtimePlotWindow(MainWindow *mainWindow);

        // get properties - the setters are below
        int isShowHr() const { return showHr->checkState(); }
        int isShowSpeed() const { return showSpeed->checkState(); }
        int isShowCad() const { return showCad->checkState(); }
        int isShowAlt() const { return showAlt->checkState(); }
        int isShowPower() const { return showPower->checkState(); }
        int isShowPow30s() const { return showPow30s->checkState(); }
        int smoothing() const { return smoothSlider->value(); }

   public slots:

        // trap signals
        void telemetryUpdate(RealtimeData rtData); // got new data
        void start();
        void stop();
        void pause();

        // set properties
        void setSmoothingFromSlider();
        void setSmoothingFromLineEdit();
        void setShowPower(int state);
        void setShowPow30s(int state);
        void setShowHr(int state);
        void setShowSpeed(int state);
        void setShowCad(int state);
        void setShowAlt(int state);
        void setSmoothing(int value);

    private:

        MainWindow *mainWindow;
        RealtimePlot *rtPlot;
        bool active;

        // Common controls
        QGridLayout *controlsLayout;
        QCheckBox *showHr;
        QCheckBox *showSpeed;
        QCheckBox *showCad;
        QCheckBox *showAlt;
        QCheckBox *showPower;
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
};

#endif // _GC_RealtimePlotWindow_h
