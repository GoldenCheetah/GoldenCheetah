/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include <QObject>
#include <QString>

#include "Context.h"

// series data from a function
#include <qwt_series_data.h>
#include <qwt_point_data.h>

#ifndef _GC_PDModel_h
#define _GC_PDModel_h 1

// A Power-Duration Model base class
//
// 1. setData is used to provide 'bests' power data in 1s intervals
//
// 2. PDModels can be used as a data provider to a QwtPlotCurve 
//    since (QwtPlotCurve::setData(*QwtSyntheticPointData)
//    the data is returned via double y(double x) const which
//    the sub-classes must implement
//
// 3. By default the model is set-up to provide 5 hours of points in 1s intervals
//    so x will be 0,1,2,3 .. 18,000. To change this the setInterval() and setSize()
//    members of QwtSyntheticPointData can be called by derived classes.
//
// 4. The model will also be used to derive W', CP, FTP etc and any subclass
//    should declare what capabilities it has via bool hasWPrime et al
//    since this controls which model parameters it can extract
//
// 5. The setData functions setup the data for all models and store it
//    in a local QVector. It will issue a signal dataChanged() when this
//    happens, and the subclasses can then choose to call a base method
//    deriveCPparameters() to extract the basic parameters and cherry-pick
//    the best points to use.
//
// 6. The setIntervals method can be used to setup the intervals the
//    base derviceCPparameters will use

#define PDMODEL_MAXT 18000 // maximum value for t we will provide p(t) for
#define PDMODEL_INTERVAL 1 // intervals used in seconds; 0t, 1t, 2t .. 18000t

class PDModel : public QObject, public QwtSyntheticPointData
{
    Q_OBJECT

    public:

        PDModel(Context *context);

        // set which variant of the model to use (if the model
        // supports such a thing it needs to reimplement)
        virtual void setVariant(int) {}

        // set data using doubles or float always
        void setData(QVector<double> meanMaxPower);
        void setData(QVector<float> meanMaxPower);

        void setMinutes(bool); // use minutes in y()

        // set the intervals to search for bests
        void setIntervals(double sanI1, double sanI2, double anI1, double anI2,
                          double aeI1, double aeI2, double laeI1, double laeI2);

        // provide data to a QwtPlotCurve
        double x(unsigned int index) const; 
        virtual double y(double /* t */) const { return 0; }

        // return an estimate of vo2max using the predicted 5m power
        // if using a model using w/kg then it will return ml/min/kg
        // if using a model using watts then it will return ml/min
        virtual double vo2max() { return 10.8 * y(300) + 7; }

        // what capabilities do you have ?
        // sticking with 4 key measures for now
        virtual bool hasWPrime() { return false; }  // can estimate W'
        virtual bool hasCP()     { return false; }  // can estimate W'
        virtual bool hasFTP()    { return false; }  // can estimate W'
        virtual bool hasPMax()   { return false; }  // can estimate W'

        virtual double WPrime()     { return 0; }      // return estimated W'
        virtual double CP()         { return 0; }      // return CP
        virtual double FTP()        { return 0; }      // return FTP
        virtual double PMax()       { return 0; }      // return PMax

        virtual void saveParameters(QList<double>&here) = 0;
        virtual void loadParameters(QList<double>&here) = 0;

        virtual QString name()   { return "Base Model"; }  // model name e.g. CP 2 parameter model
        virtual QString code()   { return "Base"; }        // short name used in metric names e.g. 2P model

        bool inverseTime;

        // using data from setData() and intervals from setIntervals()
        // this is the old function from CPPlot to extract the best points
        // in the data series to calculate cp, tau and t0.
        void deriveCPParameters(int model); 

        // we identify peak efforts when modelling
        // lets make these available, currently only
        // available with the extended CP model
        QList<QPointF> cherries();

    protected:

    signals:

        // new data arrived
        void dataChanged();

        // we changed intervals
        void intervalsChanged();

    protected:

        Context *context;

        // intervals to search for best points
        double sanI1, sanI2, anI1, anI2, aeI1, aeI2, laeI1, laeI2;

        // standard CP derived values - set when deriveCPParameters is called
        double cp, tau, t0; // CP model parameters

        // mean max power data set by setData
        QVector<double> data;

        // map of points used; secs, watts
        QMap<int,double> map;

        bool minutes;
};

// estimates are recorded 
class PDEstimate
{
    public:
        PDEstimate() : WPrime(0), CP(0), FTP(0), PMax(0), EI(0), wpk(false) {}

        QDate from, to;
        QString model;
        double WPrime,
            CP,
            FTP,
            PMax,
            EI;

        bool wpk;

        QList<double> parameters; // parameters are stored/retrieved from here
                                  // so we can run the model using pre-computed
                                  // parameters
};

// 2 parameter model
class CP2Model : public PDModel
{
    Q_OBJECT

    public:
        CP2Model(Context *context);

        // synthetic data for a curve
        double y(double t) const;

        bool hasWPrime() { return true; }  // can estimate W'
        bool hasCP()     { return true; }  // can estimate W'
        bool hasFTP()    { return false; }  // can estimate W'
        bool hasPMax()   { return false; }  // can estimate W'

        // 2 parameter model can calculate these
        double WPrime();
        double CP();

        QString name()   { return "Classic 2 Parameter"; }  // model name e.g. CP 2 parameter model
        QString code()   { return "2 Parm"; }        // short name used in metric names e.g. 2P model

        void saveParameters(QList<double>&here);
        void loadParameters(QList<double>&here);

    public slots:

        void onDataChanged();      // catch data changes
        void onIntervalsChanged(); //catch interval changes
};

// 3 parameter model
class CP3Model : public PDModel
{
    Q_OBJECT

    public:
        CP3Model(Context *context);

        // synthetic data for a curve
        double y(double t) const;

        bool hasWPrime() { return true; }  // can estimate W'
        bool hasCP()     { return true; }  // can CP
        bool hasFTP()    { return false; }  // can estimate FTP
        bool hasPMax()   { return true; }  // can estimate p-Max

        // 2 parameter model can calculate these
        double WPrime();
        double CP();
        double PMax();

        QString name()   { return "Morton 3 Parameter"; }  // model name e.g. CP 2 parameter model
        QString code()   { return "3 Parm"; }        // short name used in metric names e.g. 2P model

        void saveParameters(QList<double>&here);
        void loadParameters(QList<double>&here);

    public slots:

        void onDataChanged();      // catch data changes
        void onIntervalsChanged(); //catch interval changes
};

class WSModel : public PDModel // ward-smith
{
    Q_OBJECT

    public:
        WSModel(Context *context);

        // synthetic data for a curve
        double y(double t) const;

        bool hasWPrime() { return true; }  // can estimate W'
        bool hasCP()     { return true; }  // can CP
        bool hasFTP()    { return true; }  // can estimate FTP
        bool hasPMax()   { return true; }  // can estimate p-Max

        // 2 parameter model can calculate these
        double WPrime();
        double CP();
        double FTP();
        double PMax();

        QString name()   { return "Ward-Smith"; }  // model name e.g. CP 2 parameter model
        QString code()   { return "WS"; }        // short name used in metric names e.g. 2P model

        void saveParameters(QList<double>&here);
        void loadParameters(QList<double>&here);

    public slots:

        void onDataChanged();      // catch data changes
        void onIntervalsChanged(); //catch interval changes
};

// multicomponent model
class MultiModel : public PDModel
{
    Q_OBJECT

    public:
        MultiModel(Context *context);
        void setVariant(int); // which variant to use

        // synthetic data for a curve
        double y(double t) const;

        bool hasWPrime() { return true; }  // can estimate W'
        bool hasCP()     { return true; }  // can CP
        bool hasFTP()    { return true; }  // can estimate FTP
        bool hasPMax()   { return true; }  // can estimate p-Max

        // 2 parameter model can calculate these
        double WPrime();
        double CP();
        double FTP();
        double PMax();

        QString name()   { return "Veloclinic Multicomponent"; }  // model name e.g. CP 2 parameter model
        QString code()   { return "Velo"; }        // short name used in metric names e.g. 2P model

        // veloclinic has multiple additional parameters
        int variant;
        double w1, p1, p2, tau1, tau2, alpha, beta;

        void saveParameters(QList<double>&here);
        void loadParameters(QList<double>&here);

    public slots:

        void onDataChanged();      // catch data changes
        void onIntervalsChanged(); //catch interval changes
};


// extended model
class ExtendedModel : public PDModel
{
    Q_OBJECT

    public:
        ExtendedModel(Context *context);

        // synthetic data for a curve
        double y(double t) const;

        bool hasWPrime() { return true; }  // can estimate W'
        bool hasCP()     { return true; }  // can CP
        bool hasFTP()    { return true; }  // can estimate FTP
        bool hasPMax()   { return true; }  // can estimate p-Max

        // 4 parameter model can calculate these
        double WPrime();
        double CP();
        double FTP();
        double PMax();

        QString name()   { return "Extended CP"; }  // model name e.g. CP 2 parameter model
        QString code()   { return "Ext"; }        // short name used in metric names e.g. 2P model

        // Extended has multiple additional parameters
        double paa, paa_dec, ecp, etau, ecp_del, tau_del, ecp_dec, ecp_dec_del;

        void saveParameters(QList<double>&here);
        void loadParameters(QList<double>&here);

    public slots:

        void onDataChanged();      // catch data changes
        void onIntervalsChanged(); //catch interval changes

    private:
        void deriveExtCPParameters();
};

#endif 
