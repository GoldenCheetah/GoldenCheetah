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
#include <QMutex>
#include <QString>

#include "Context.h"
#include <cmath>

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

        enum fittype { Envelope=0,                 // envelope fit
                       LeastSquares=1,             // uses Levenberg-Marquardt Damped Least Squares
                       LinearRegression=2         // using slope and intercept of linear regression
                     } fit;

        PDModel();

        // set which variant of the model to use (if the model
        // supports such a thing it needs to reimplement)
        virtual void setVariant(int) {}

        // set fit method
        void setFit(fittype x) { fit=x; }

        // set data using doubles or float always
        void setData(QVector<double> meanMaxPower);
        void setData(QVector<float> meanMaxPower);
        void setPtData(QVector<double> power, QVector<double> secs);

        void setMinutes(bool); // use minutes in y()

        // set the intervals to search for bests
        void setIntervals(double sanI1, double sanI2, double anI1, double anI2,
                          double aeI1, double aeI2, double laeI1, double laeI2);

        // provide data to a QwtPlotCurve
        double x(unsigned int index) const; 
        virtual double y(double /* t */) const = 0;

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

        // User Descriptions
        static QString WPrimeDescription() { return tr("W': Formerly known as Anaerobic Work Capacity (AWC). An estimate for the fixed amount of work, expressed in kJ, that you can do above Critical Power."); }
        static QString CPDescription() { return tr("Critical Power. An estimate of the power that, theoretically, can be maintained for a long time without fatigue."); }
        static QString FTPDescription() { return tr("Functional Threshold Power. The highest power that a rider can maintain in a quasi-steady state without fatiguing for approximately one hour."); }
        static QString PMaxDescription() { return tr("P-max: An estimate of the maximal power over one full rotation of the cranks."); }

        virtual void saveParameters(QList<double>&here) = 0;
        virtual void loadParameters(QList<double>&here) = 0;

        virtual QString name()   { return tr("Base Model"); }  // model name e.g. CP 2 parameter model
        virtual QString code()   { return "Base"; }        // short name used in metric names e.g. 2P model

        bool inverseTime;

        // using data from setData() and intervals from setIntervals()
        // this is the old function from CPPlot to extract the best points
        // in the data series to calculate cp, tau and t0.
        virtual void deriveCPParameters(int model);

        // when using lest squares fitting
        virtual int nparms() { return -1; }
        virtual double f(double, const double *) { return -1; }
        virtual bool setParms(double *) { return false; }

        // we identify peak efforts when modelling
        // lets make these available, currently only
        // available with the extended CP model
        QList<QPointF> cherries();

        // calculate the fit summary
        void calcSummary();
        QString fitsummary;

    protected:

    signals:

        // new data arrived
        void dataChanged();

        // we changed intervals
        void intervalsChanged();

    protected:

        int model;

        // intervals to search for best points
        double sanI1, sanI2, anI1, anI2, aeI1, aeI2, laeI1, laeI2;

        // standard CP derived values - set when deriveCPParameters is called
        double cp, tau, t0; // CP model parameters

        // mean max power data set by setData
        QVector<double> data;

        // specific points to fit (using OLS usually)
        QVector<double> tdata;

        // map of points used; secs, watts
        QMap<int,double> map;

        bool minutes;
};

// control calling lmfit
extern QMutex calllmfit;
extern PDModel *calllmfitmodel;
extern double calllmfitf(double t, const double *p);

// estimates are recorded
class PDEstimate
{
    public:
        PDEstimate() : WPrime(0), CP(0), FTP(0), PMax(0), EI(0), wpk(false), sport("Bike") {}

        QDate from, to;
        QString model;
        double WPrime,
            CP,
            FTP,
            PMax,
            EI;

        bool wpk;
        QString sport;

        QList<double> parameters; // parameters are stored/retrieved from here
                                  // so we can run the model using pre-computed
                                  // parameters
};

// 2 parameter model
class CP2Model : public PDModel
{
    Q_OBJECT

    public:
        CP2Model();

        // synthetic data for a curve
        virtual double y(double t) const;

        // when using lest squares fitting
        int nparms() { return 2; } // CP + W'
        double f(double t, const double *parms) {
            return parms[0] + (parms[1]/t);
        }
        bool setParms(double *parms) {

            // set the model parameters with the values from the fit
            cp = parms[0];
            tau = parms[1] / (cp * 60);
            return true;
        }

        bool hasWPrime() { return true; }  // can estimate W'
        bool hasCP()     { return true; }  // can estimate W'
        bool hasFTP()    { return false; }  // can estimate W'
        bool hasPMax()   { return false; }  // can estimate W'

        // 2 parameter model can calculate these
        double WPrime();
        double CP();

        QString name()   { return tr("Classic 2 Parameter"); }  // model name e.g. CP 2 parameter model
        QString code()   { return "cp2"; }        // short name used in metric names e.g. 2P model

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
        CP3Model();

        bool modelDecay;    // should we apply CP + W' decay constants?

        // synthetic data for a curve
        virtual double y(double t) const;

        // working with least squares
        int nparms() { return 3; }
        double f(double t, const double *parms) {
            double cp = parms[0];
            double w = parms[1];
            double k = parms[2];

            return cp + (w/(t+k));
        }
        bool setParms(double *parms) {
            this->cp = parms[0];
            this->tau = parms[1] / (cp * 60.00);
            double pmax = parms[0] + (parms[1]/(1+parms[2]));
            this->t0 = this->tau / (pmax / this->cp - 1) - 1 / 60.0;
            return true;
        }

        bool hasWPrime() { return true; }  // can estimate W'
        bool hasCP()     { return true; }  // can CP
        bool hasFTP()    { return false; }  // can estimate FTP
        bool hasPMax()   { return true; }  // can estimate p-Max

        // 2 parameter model can calculate these
        double WPrime();
        double CP();
        double PMax();

        QString name()   { return tr("Morton 3 Parameter"); }  // model name e.g. CP 2 parameter model
        QString code()   { return "cp3"; }        // short name used in metric names e.g. 2P model

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
        WSModel();

        // synthetic data for a curve
        virtual double y(double t) const;

        bool hasWPrime() { return true; }  // can estimate W'
        bool hasCP()     { return true; }  // can CP
        bool hasFTP()    { return true; }  // can estimate FTP
        bool hasPMax()   { return true; }  // can estimate p-Max

        // 2 parameter model can calculate these
        double WPrime();
        double CP();
        double FTP();
        double PMax();

        QString name()   { return tr("Ward-Smith"); }  // model name e.g. CP 2 parameter model
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
        MultiModel();
        void setVariant(int); // which variant to use

        // synthetic data for a curve
        virtual double y(double t) const;

        bool hasWPrime() { return true; }  // can estimate W'
        bool hasCP()     { return true; }  // can CP
        bool hasFTP()    { return true; }  // can estimate FTP
        bool hasPMax()   { return true; }  // can estimate p-Max

        // 2 parameter model can calculate these
        double WPrime();
        double CP();
        double FTP();
        double PMax();

        QString name()   { return tr("Veloclinic Multicomponent"); }  // model name e.g. CP 2 parameter model
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
        ExtendedModel();

        // synthetic data for a curve
        virtual double y(double t) const;

        int nparms() { return 8; }
        double f(double x, const double *parms) {

            double paa = parms[0];
            double paadec  = parms[1];
            double cp = parms[2];
            double tau = parms[3];
            double taudel  = parms[4];
            double cpdel = parms[5];
            double cpdec = parms[6];
            double cpdecdel = parms[7];

            x = x/60.00;

#if 0       // constrain the fit
            if (paa < paa_min) paa = paa_min;
            if (tau < etau_min) etau = etau_min;
            if (paadec < paa_dec_min) paadec = paa_dec_min;
            if (paadec > paa_dec_max) paadec = paa_dec_max;
            if (cpdec < ecp_dec_min) cpdec = ecp_dec_min;
#endif

            double rt= (paa*(1.20-0.20*exp(-1*x))*exp(paadec*(x))
                    + ( cp * (1-exp(taudel*x)) *
                    (1-exp(cpdel*x)) *
                    (1+cpdec*exp(cpdecdel/x)) *
                    (tau/(x)))
                    + ( cp * (1-exp(taudel*x)) *
                    (1-exp(cpdel*x)) *
                    (1+cpdec*exp(cpdecdel/x)) *
                    (1)));
            if (std::isinf(rt) || std::isnan(rt)) rt=0;
            return rt;

        }

        bool setParms(double *parms) {

            paa = parms[0];
            paa_dec = parms[1];
            ecp = parms[2];
            etau = parms[3];
            tau_del = parms[4];
            ecp_del = parms[5];
            ecp_dec = parms[6];
            ecp_dec_del = parms[7];
#if 0
            // constrain the fit
            if (paa < paa_min) paa = paa_min;
            if (tau < etau_min) etau = etau_min;
            if (paa_dec < paa_dec_min) paa_dec = paa_dec_min;
            if (paa_dec > paa_dec_max) paa_dec = paa_dec_max;
            if (ecp_dec < ecp_dec_min) ecp_dec = ecp_dec_min;
#endif

            return true;
        }

        // parameter constraints (still not physiologically plausible)

        // lower bound
        const double paa_min = 400;
        const double etau_min = 0.5;
        const double paa_dec_max = -0.25;
        const double paa_dec_min = -3;
        const double ecp_dec_min = -5;

        // convergence delta
        const double etau_delta_max = 1e-4;
        const double paa_delta_max = 1e-2;
        const double paa_dec_delta_max = 1e-4;
        const double ecp_del_delta_max = 1e-4;
        const double ecp_dec_delta_max  = 1e-8;

        bool hasWPrime() { return true; }  // can estimate W'
        bool hasCP()     { return true; }  // can CP
        bool hasFTP()    { return true; }  // can estimate FTP
        bool hasPMax()   { return true; }  // can estimate p-Max

        // 4 parameter model can calculate these
        double WPrime();
        double CP();
        double FTP();
        double PMax();

        QString name()   { return tr("Extended CP"); }  // model name e.g. CP 2 parameter model
        QString code()   { return "ext"; }        // short name used in metric names e.g. 2P model

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
