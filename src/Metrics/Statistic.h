/*
 * Copyright (c) 2014 Damien Grauser
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

#ifndef _GC_Statistic_h
#define _GC_Statistic_h 1
#include "GoldenCheetah.h"

class Statistic
{
    public:
        Statistic();
        // Constructor using arrays of x values and y values
        Statistic(double *, double *, int);

        double getYforX(double x) const { return (a + b * x); }
        double intercept() { return a; }
        double slope() { return b; }
        double r() { return c; }
        QString label();

        double minX, maxX, minY, maxY; // for the data set we have

        double slope(QVector<double> &Xi,QVector<double> &Yi,int n);
        double intercept(QVector<double> &Xi,QVector<double> &Yi,int n);
        double corr(QVector<double> &Xi, QVector<double> &Yi,int n);
        double average(QVector<double> &Xi,int n);

        // Maths functions used by the plots
        QVector<double> array_temp; //Déclaration d'un tableau temporaire

        int test_zero(QVector<double> &array,int n);
        int test_negative(QVector<double> &array,int n);
        void logarray(QVector<double> &array,QVector<double> &arrayTemp,int n);
        void lnarray(QVector<double> &array,QVector<double> &arrayTemp,int n);
        void invarray(QVector<double> &array,QVector<double> &arrayTemp,int n);
        int fit(QVector<double> &Xi,QVector<double> &Yi,int n);
        double average(QVector<int> &array,int n);
        double average2(double sum,int n);
        double val_abs(double x);
        int rmax(QVector<double> &r);
        int sum(QVector<int> &array,int n);
        double sum(QVector<double> &array,int n);
        void arrayproduct(QVector<double> &array1,QVector<double> &array2,int n);
        void deviation_from_average(QVector<double> &array,double Average,int n);
        double covariance(QVector<double> &Xi, QVector<double> &Yi,int n);
        double variance(QVector<double> &val,int n);
        double standarddeviation(QVector<double> &val,int n);

    protected:
        long points;
        double sumX, sumY;
        double sumXsquared,
               sumYsquared;
        double sumXY;
        double a, b, c;   // a = intercept, b = slope, c = r2

};

#endif
