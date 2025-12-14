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

#ifndef _GC_SpinScanPlotWindow_h
#define _GC_SpinScanPlotWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QObject> // for Q_PROPERTY
#include <QStackedWidget>
#include <QLabel>


#include "Context.h"
#include "RideFile.h" // for data series types
#include "SpinScanPolarPlot.h"
#include "SpinScanPlot.h"
#include "RealtimeData.h" // for realtimedata structure

#include "Settings.h"
#include "Colors.h"

class SpinScanPlotWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int style READ getStyle WRITE setStyle USER true)

    public:

        SpinScanPlotWindow(Context *context);

   public slots:

        // trap signals
        void telemetryUpdate(const RealtimeData &rtData); // got new data
        void start();
        void stop();
        void pause();

        int getStyle() const { return mode->currentIndex(); }
        void setStyle(int x);
        void styleChanged();

    private:

        // we keep 8 sets of spinscan data (last 2 seconds)
        // and maintain a rolling average for display
        // we use 2 seconds since that is the default refresh
        // rate from the computrainer. We don't use a
        // multidimensional array because we need to use
        // memcpy and don't want to guess the model used by
        // the compiler (although in reality it is always
        // gcc and we could, but this isn't a huge overhead)
        uint8_t set1[24];
        uint8_t set2[24];
        uint8_t set3[24];
        uint8_t set4[24];
        uint8_t set5[24];
        uint8_t set6[24];
        uint8_t set7[24];
        uint8_t set8[24];
        uint8_t set9[24];
        uint8_t set10[24];
        uint8_t set11[24];
        uint8_t set12[24];
        uint8_t set13[24];
        uint8_t set14[24];
        uint8_t set15[24];
        uint8_t set16[24];
        uint8_t *history[16];
        uint16_t rtot[24];
        int current;

        Context *context;
        bool active;

        QStackedWidget *stack;
        SpinScanPlot *rtPlot;
        SpinScanPolarPlot *plPlot;

        uint8_t spinData[24]; // what we give to the plot

        QComboBox *mode;
};

#endif // _GC_SpinScanPlotWindow_h
