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


#ifndef _GC_RealtimeData_h
#define _GC_RealtimeData_h 1

class RealtimeData
{


public:

    RealtimeData();
    void reset(); // set all values to zero
    void setWatts(double watts);
    void setHr(double hr);
    void setTime(long time);
    void setSpeed(double speed);
    void setWheelRpm(double wheelRpm);
    void setCadence(double aCadence);
    void setLoad(double load);
    double getWatts();
    double getHr();
    long getTime();
    double getSpeed();
    double getWheelRpm();
    double getCadence();
    double getLoad();

private:
    unsigned long time;
    double hr, watts, speed, wheelRpm, load;
    double cadence;      // in rpm
};


#endif
