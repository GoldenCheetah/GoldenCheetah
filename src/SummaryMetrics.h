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

#ifndef SUMMARYMETRICS_H_
#define SUMMARYMETRICS_H_

#include <QString>
#include <QDateTime>

class SummaryMetrics
{
	public:
		SummaryMetrics();
	    QString getFileName();
		double getDistance();
		double getSpeed();
		double getWatts();
		double getBikeScore();
		double getXPower();
		double getCadence();
		double getHeartRate();
		double getRideTime();
	    double getWorkoutTime();
	    double getTotalWork();
	    double getRelativeIntensity();
        QDateTime getRideDate();
	
	    void setDistance(double _distance);
	    void setSpeed(double _speed);
		void setWatts(double _watts);
		void setBikeScore(double _bikescore);
	    void setXPower(double _xPower);
	    void setCadence(double _cadence);
		void setHeartRate(double _heartRate);
    	void setWorkoutTime(double _workoutTime);
	    void setRideTime(double _rideTime);
	    void setFileName(QString _filename);
	    void setTotalWork(double _totalWork);
		void setRelativeIntensity(double _relativeIntensity);
        void setRideDate(QDateTime _rideDate);


	private:
		double distance;
		double speed;
		double watts;
		double bikeScore;
		double xPower;
		double cadence;
		double heartRate;
		double rideTime;
	    QString fileName;
	    double totalWork;
	    double workoutTime;
	    double relativeIntensity;
        QDateTime rideDate;

};


#endif /* SUMMARYMETRICS_H_ */
