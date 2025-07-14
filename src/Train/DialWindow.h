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

#ifndef _GC_DialWindow_h
#define _GC_DialWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QObject> // for Q_PROPERTY
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>

#include "ScalingLabel.h"

#include "Context.h"
#include "Zones.h" // for data series types
#include "RideFile.h" // for data series types
#include "ErgFile.h" // for workout modes
#include "RealtimeData.h" // for realtimedata structure

#include "Settings.h" // for realtimedata structure
#include "Units.h" // for realtimedata structure
#include "Colors.h" // for realtimedata structure

class DialWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // the plot properties are used by the layout manager
    // to save and restore the plot parameters, this is so
    // we can have multiple windows open at once with different
    // settings managed by the user.
    // in this example we might show a stacked plot and a ride
    // plot at the same time.

    // show the instant value?
    Q_PROPERTY(bool showInstant READ isInstant WRITE setInstant USER true)

    // rolling average window
    Q_PROPERTY(int avgsecs READ avgSecs WRITE setAvgSecs USER true)

    // average - none, 30s rolling, total, currentlap
    Q_PROPERTY(int avgType READ avgType WRITE setAvgType USER true)

    // which data series to show?
    Q_PROPERTY(int dataSeries READ dataSeries WRITE setDataSeries USER true)

    // how to show - numeric, dial
    Q_PROPERTY(int style READ style WRITE setStyle USER true)

    public:

        DialWindow(Context *context);

        // get properties - the setters are below
        bool isInstant() const { return _instant; }
        int avgType() const { return _avgType; }
        int dataSeries() const { return seriesSelector->currentIndex(); }
        int style() const { return _style; }
        int avgSecs() const { return average; }

   public slots:

        // trap signals
        void seriesChanged();
        void telemetryUpdate(const RealtimeData &rtData); // got new data
        void lap(int lapnumber);
        void start();
        void stop();
        void pause();
        void onNewLap();

    protected:

        void setInstant(bool x) { _instant=x; }
        void setAvgSecs(int x) { average=x; averageSlider->setValue(x); setAverageFromSlider(); }
        void setAvgType(int x) { _avgType=x; }
        void setDataSeries(int x) { seriesSelector->setCurrentIndex(x); }
        void setStyle(int x) { _style=x; }

    private:

        Context *context;

        // properties
        bool _instant;
        int _avgType;
        int _style;

        // values to display
        double instantValue;
        double avg30, avgLap, avgTotal;
        double lapNumber;

        // for calculating averages
        int average;
        int count;
        double sum;
        bool isNewLap;

        // for keeping track of rolling averages (max 30s at 5hz)
        // used by IsoPower and XPower
        QVector<double> rolling;
        double rollingSum;
        int index; // index into rolling (circular buffer)


        // VI/RI makes us track AP too
        int apcount;
        int apsum;

        // used by XPower algorithm
        double rsum, ewma;

        //heat load estimate (don't reset in resetValues, but preserve between sessions, and reset when local date changes)
        //note: each athelete has it's own set of DialWindows if more than one athlete is loaded at the same time
        double heatLoad;
        qint64 heatLoadMSec;
        QDateTime heatLoadLocalDate;
        bool isRunning = false;

        void resetValues() {

            rolling.fill(0.00);
            rsum = ewma = 0.0f;
            rollingSum = index = 0;
            apcount = count = sum = instantValue = avg30 =
            apsum = avgLap = avgTotal = lapNumber = 0;
            telemetryUpdate(RealtimeData());
        }

        // controls
        QComboBox   *seriesSelector;

        QLabel      *averageLabel;
        QSlider     *averageSlider;
        QLineEdit   *averageEdit;

        // display
        ScalingLabel *valueLabel;

        QColor foreground, background;

    private slots :
        void setAverageFromSlider();
        void setAverageFromText(const QString text);
};

#endif // _GC_DialWindow_h
