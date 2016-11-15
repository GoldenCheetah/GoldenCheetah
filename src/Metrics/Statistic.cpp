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

#include <cmath>
#include <float.h>
#include "Statistic.h"

#include <QDebug>

Statistic::Statistic() :
          minX(0.0), maxX(0.0), minY(0.0), maxY(0.0),
          points(0.0), sumX(0.0), sumY(0.0), sumXsquared(0.0),
          sumYsquared(0.0), sumXY(0.0), a(0.0), b(0.0), c(1.0)
{
    array_temp.resize(1);
}

Statistic::Statistic(double *xdata, double *ydata, int count) :
          minX(0.0), maxX(0.0), minY(0.0), maxY(0.0),
          points(0.0), sumX(0.0), sumY(0.0), sumXsquared(0.0),
          sumYsquared(0.0), sumXY(0.0), a(0.0), b(0.0), c(1.0)
{
    if (count <= 2) return;

    for (int i = 0; i < count; i++) {

        // ignore zero points
        //if (ydata[i] == 0.00) continue;

        points++;
        sumX += xdata[i];
        sumY += ydata[i];
        sumXsquared += xdata[i] * xdata[i];
        sumYsquared += ydata[i] * ydata[i];
        sumXY += xdata[i] * ydata[i];

        if (xdata[i]>maxX)
            maxX = xdata[i];
        else if (xdata[i]<minX)
            minX = xdata[i];

        if (ydata[i]>maxY)
            maxY = ydata[i];
        else if (ydata[i]<minY)
            minY = ydata[i];
    }

    if (fabs( double(points) * sumXsquared - sumX * sumX) > DBL_EPSILON) {
        b = ( double(points) * sumXY - sumY * sumX) /
            ( double(points) * sumXsquared - sumX * sumX);
        a = (sumY - b * sumX) / double(points);
        c = ( double(points) * sumXY - sumY * sumX) /
            sqrt(( double(points) * sumXsquared - sumX * sumX) * ( double(points) * sumYsquared - sumY * sumY));


        double sx = b * ( sumXY - sumX * sumY / double(points) );
        double sy2 = sumYsquared - sumY * sumY / double(points);
        double sy = sy2 - sx;

        double coefD = sx / sy2;
        double coefC = sqrt(coefD);
        double stdError = sqrt(sy / double(points - 2));

        qDebug() << "Statistic" << coefD << coefC << stdError << c;
    }
}

QString
Statistic::label()
{
    return QString("%1x%2%3 (%4)").arg(QString("").setNum(slope(), 'f', 3)).arg(intercept()>=0?"+":"").arg(QString("").setNum(intercept(), 'f', 1)).arg(QString("").setNum(r(), 'f', 3));
}

/**************************************/
/* CPP file of reglin libary          */
/* Created by GONNELLA Stéphane       */
/**************************************/


/* Global declaration of variables */


/*********************/
/* Test methods */
/*********************/

/* Test for zero in values */

int
Statistic::test_zero(QVector<double> &array,int n)
{
        for(int i=0;i<n;i++)
        {
                if(array[i]==0)
                { return i;}
        }

        return -1;
}

/* Test for negative values */

int
Statistic::test_negative(QVector<double> &array,int n)
{
        for(int i=0;i<n;i++)
        {
                if(array[i]<0)
                { return i;}
        }

        return -1;
}

/*********************************/
/* Methods for absolute values*/
/*********************************/

/* Method that returns the absolute value*/

double
Statistic::val_abs(double x)
{
    if(x<0) {return -x;}
    else {return x;}
}
/********************************/
/* Methods to search max values */
/********************************/

/* Method that returns the max vlaue*/

int
Statistic::rmax(QVector<double> &r)
{
        double temp=0;
        int ajust=0;

        for(int i=0;i<5;i++)
        {
                if(r[i]>temp)
                {
                     temp=r[i];
                     ajust = i+1;
                }
        }

        return ajust;
}

/**********************/
/* Summation methods */
/**********************/

/* Summation method for array of integers */

int
Statistic::sum(QVector<int> &array,int n)
{
    int sum=0;

    for (int i=0;i<n;i++)
    {
     sum=((array[i])+(sum));
    }

    return(sum);
}

/* Summation method for array of doubles */

double
Statistic::sum(QVector<double> &array,int n)
{
    double sum=0;

    for (int i=0;i<n;i++)
    {
     sum=((array[i])+(sum));
    }

    return(sum);
}

/**********************************/
/* Methods for calculation of averages */
/**********************************/

/* Method that calculates average for array of integers */

double
Statistic::average(QVector<int> &array,int n)
{
    double average = double(sum(array,n))/n;

    return (average);
}

/* Method that calculates average for array of doubles */

double
Statistic::average(QVector<double> &array,int n)
{
    double average = sum(array,n)/n;

    return (average);
}

/* Method that calculates average for array of doubles(2) */

double
Statistic::average2(double sum,int n)
{
    double average = sum/n;

    return (average);
}

/***********************/
/* Multiplication methods */
/***********************/

/* Method for element-wise multiplication of two arrays */

void
Statistic::arrayproduct(QVector<double> &array1,QVector<double> &array2,int n)
{
    array_temp.resize(n);   //Réservation de l'espace mémoire

    for (int i=0;i<n;i++)
    {
        array_temp[i]=(array1[i]*array2[i]);
    }
}

/***************************************/
/* Method to change variables */
/***************************************/

/* Method to calculate the natural logarithm of an array of doubles element-wise */

void
Statistic::lnarray(QVector<double> &array,QVector<double> &arrayTemp,int n)
{
    array_temp.resize(n);   //Réservation de l'espace mémoire

    for (int i=0;i<n;i++)
    {
        arrayTemp[i]=(log(array[i]));
    }
}

/* Method to calculate the base 10 logarithm of an array of doubles element-wise */

void
Statistic::logarray(QVector<double> &array,QVector<double> &arrayTemp,int n)
{
    array_temp.resize(n);   //Réservation de l'espace mémoire

    for (int i=0;i<n;i++)
    {
        arrayTemp[i]=(log10(array[i]));
    }
}

/* Method to calculate the inverse (1/x) of an array of doubles element-wise */

void
Statistic::invarray(QVector<double> &array,QVector<double> &arrayTemp,int n)
{
    array_temp.resize(n);   //Réservation de l'espace mémoire

    for (int i=0;i<n;i++)
    {
        arrayTemp[i]=(1/array[i]);
    }
}

/********************/
/* Other methods */
/********************/

/* Method to calculate the deviation from the average */

void
Statistic::deviation_from_average(QVector<double> &array,double Average,int n)
{
    array_temp.resize(n);   //Réservation de l'espace mémoire

    for (int i=0;i<n;i++)
    {
        array_temp[i]=(array[i] - Average);
    }
}

/****************************/
/* Statistical methods */
/****************************/

/* Method to calculate covariance */

double
Statistic::covariance(QVector<double> &Xi, QVector<double> &Yi,int n)
{
    double cov;

    arrayproduct(Xi,Yi,n);
    cov = average(array_temp,n) - ( average(Xi,n) * average(Yi,n) );

    return (cov);
}

/* Method to calculate variance */

double
Statistic::variance(QVector<double> &val,int n)
{
    double sce;

    arrayproduct(val,val,n);
    sce = average(array_temp,n) - ( average(val,n) * average(val,n));

    return (sce);
}

/* Method to calculate the standard deviation */

double
Statistic::standarddeviation(QVector<double> &val,int n)
{
    double sd= sqrt(variance(val,n));

    return (sd);
}
/******************************************************/
/* Methods to calculate a linear regression model using the method of least squares */
/******************************************************/

/* Method to calculate the slope of the linear regression model */

double
Statistic::slope(QVector<double> &Xi,QVector<double> &Yi,int n)
{
    double a = covariance(Xi,Yi,n)/variance(Xi,n);

    return (a);
}

/* Method to calculate the intercept of the linear regression model */

double
Statistic::intercept(QVector<double> &Xi,QVector<double> &Yi,int n)
{
    double b = average(Yi,n) - ( slope(Xi,Yi,n) * average(Xi,n) );

    return (b);
}

/* Method to calculate the correlation coefficient */

double
Statistic::corr(QVector<double> &Xi, QVector<double> &Yi,int n)
{
    double r = covariance(Xi,Yi,n)/(standarddeviation(Xi,n)*standarddeviation(Yi,n));
        //double r=slope(Xi,Yi,n)*slope(Xi,Yi,n)*(variance(Xi,n)/variance(Yi,n));
    return (r);
}

/* Method to calculate the best fit */

int
Statistic::fit(QVector<double> &Xi,QVector<double> &Yi,int n)
{
        QVector<double> r(5),lnXi(100),lnYi(100),logXi(100),logYi(100),invXi(100);

        //Linear correlation

        r[0]=val_abs(corr(Xi,Yi,n));

        //Exponential correlation

        lnarray(Yi,lnYi,n);
        r[1]=val_abs(corr(Xi,lnYi,n));

        //Power correlation

        logarray(Xi,logXi,n);
        logarray(Yi,logYi,n);
        r[2]=val_abs(corr(logXi,logYi,n));

        //Inverse correlation

        invarray(Xi,invXi,n);
        r[3]=val_abs(corr(invXi,Yi,n));

        //Logarithmic correlation

        lnarray(Xi,lnXi,n);
        r[4]=val_abs(corr(lnXi,Yi,n));

        //Best fit test

        return rmax(r);
}

/*****************************/
/* End of CPP library reglin */
/*****************************/
