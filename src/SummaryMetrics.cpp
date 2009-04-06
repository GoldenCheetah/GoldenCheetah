/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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


#include "SummaryMetrics.h"

SummaryMetrics::SummaryMetrics()	{	}



double SummaryMetrics::getDistance()	{ return distance;	}
double SummaryMetrics::getSpeed()	{ return speed;	}
double SummaryMetrics::getWatts()	{ return watts;	}
double SummaryMetrics::getBikeScore()	{ return bikeScore;	}
double SummaryMetrics::getXPower()	{ return xPower;	}
double SummaryMetrics::getCadence()	{ return cadence;	}
double SummaryMetrics::getHeartRate()	{ return heartRate;	}
double SummaryMetrics::getRideTime()	{ return rideTime;	}
QString SummaryMetrics::getFileName() { return fileName; }
double SummaryMetrics::getTotalWork() { return totalWork; }
double SummaryMetrics::getWorkoutTime() { return workoutTime; }
double SummaryMetrics::getRelativeIntensity() { return relativeIntensity; }
QDateTime SummaryMetrics::getRideDate() { return rideDate; }

void SummaryMetrics::setSpeed(double _speed) { speed = _speed; }
void SummaryMetrics::setWatts(double _watts) { watts = _watts; }
void SummaryMetrics::setBikeScore(double _bikescore) { bikeScore = _bikescore; }
void SummaryMetrics::setXPower(double _xPower) { xPower = _xPower; }
void SummaryMetrics::setCadence(double _cadence) { cadence = _cadence; }
void SummaryMetrics::setDistance(double _distance) { distance = _distance; }
void SummaryMetrics::setRideTime(double _rideTime) { rideTime = _rideTime; }
void SummaryMetrics::setTotalWork(double _totalWork) { totalWork = _totalWork; }
void SummaryMetrics::setFileName(QString _fileName) { fileName = _fileName; }
void SummaryMetrics::setWorkoutTime(double _workoutTime) { workoutTime = _workoutTime; }
void SummaryMetrics::setRelativeIntensity(double _relativeIntensity) { relativeIntensity = _relativeIntensity; }
void SummaryMetrics::setHeartRate(double _heartRate) { heartRate = _heartRate; }
void SummaryMetrics::setRideDate(QDateTime _rideDate) { rideDate = _rideDate; }