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
#include "GoldenCheetah.h"

#include "RealtimeController.h"
#include "Fortius.h"

// Abstract base class for Realtime device controllers

#ifndef _GC_FortiusController_h
#define _GC_FortiusController_h 1

template <size_t N>
class NSampleSmoothing
{
private:
    int    nSamples = 0;
    double samples[N];
    int    index = 0;
    double total = 0;
    bool   full = false;

public:
    NSampleSmoothing() { reset(); }

    void reset()
    {
        for (int i = 0; i < N; ++i)
            samples[i] = 0.;
        nSamples = 0;
        index = 0;
        total = 0;
        full = false;
    }

    void update(double newVal)
    {
        ++nSamples;

        total += newVal - samples[index];
        samples[index] = newVal;
        if (++index == N) { index = 0; full = true; }
    }

    bool is_full() const
    {
        return full;
    }

    double mean() const
    {
        // average if we have enough values, otherwise return latest
        return total / N;//(nSamples > N) ? (total / N) : samples[(index + (N-1))%N];
    }

    double stddev() const
    {
        const double avg = mean();
        const double sum_squares = std::accumulate(std::begin(samples), std::end(samples), 0.0, [avg](double acc, double sample) {return acc + (sample - avg) * (sample - avg); });
        return sqrt(sum_squares / static_cast<double>(N));
    }
};
class FortiusController : public RealtimeController
{
    Q_OBJECT

public:
    FortiusController (TrainSidebar *, DeviceConfiguration *);

    Fortius *myFortius;               // the device itself

    int start();
    int restart();                              // restart after paused
    int pause();                                // pauses data collection, inbound telemetry is discarded
    int stop();                                 // stops data collection thread

    bool find();
    bool discover(QString);              // tell if a device is present at port passed


    // telemetry push pull
    bool doesPush(), doesPull(), doesLoad();
    void getRealtimeData(RealtimeData &rtData);
    void pushRealtimeData(RealtimeData &rtData);
    void setLoad(double);
    void setGradient(double);
    void setMode(int);
    void setWeight(double);
    void setWindSpeed(double);
    void setRollingResistance(double);
    void setWindResistance(double);

    // calibration
    static const int calibrationDurationLimit_s = 60;

    uint8_t  getCalibrationType() override;
    double   getCalibrationTargetSpeed() override;
    uint8_t  getCalibrationState() override;
    void     setCalibrationState(uint8_t state) override;
    uint16_t getCalibrationZeroOffset() override;
    void     resetCalibrationState() override;

private:
    // calibration data for use by controller
    uint8_t  m_calibrationState;
    time_t   m_calibrationStarted;
    NSampleSmoothing<100> m_calibrationValues;

};

#endif // _GC_FortiusController_h

