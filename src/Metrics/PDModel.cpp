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

#include "PDModel.h"
#include "LTMTrend.h"
#include "lmcurve.h"

//extern ztable PD_ZTABLE;
// base class for all models
PDModel::PDModel(Context *context) :
    QwtSyntheticPointData(PDMODEL_MAXT),
    fit(Envelope),
    inverseTime(false),
    context(context),
    sanI1(0), sanI2(0), anI1(0), anI2(0), aeI1(0), aeI2(0), laeI1(0), laeI2(0),
    cp(0), tau(0), t0(0), minutes(false)
{
    setInterval(QwtInterval(1, PDMODEL_MAXT));
    setSize(PDMODEL_MAXT);
}

// set data using doubles always
void 
PDModel::setData(QVector<double> meanMaxPower)
{
    cp = tau = t0 = 0; // reset on new data
    int size = meanMaxPower.size();

    // truncate the data for 2p and 3p models
    if (model < 3)  size = size < 1200 ? size : 1200;

    data = meanMaxPower;
    data.resize(size);

    // generate t data, in seconds
    tdata.resize(size);
    for(int i=0; i<size; i++) tdata[i] = i+1;
    emit dataChanged();
}

void
PDModel::setPtData(QVector<double>p, QVector<double>t)
{
    cp = tau = t0 = 0;
    data = p;
    tdata = t;
    for(int i=0; i<t.count(); i++) {
        tdata[i] = tdata[i] * 60;
    }
    emit dataChanged();
}
void
PDModel::setMinutes(bool x)
{
    minutes = x;
    if (minutes) {
        setInterval(QwtInterval(1.00f / 60.00f, double(PDMODEL_MAXT)/ 60.00f));
        setSize(PDMODEL_MAXT);
    }
}

double PDModel::x(unsigned int index) const
{
    double returning = 1;

    // don't start at zero !
    index += 1;

    if (minutes) returning = (double(index)/60.00f);
    else returning = index;

    // reverse !
    if (inverseTime) return 1.00f / returning;
    else return returning;
}


// set data using floats, we convert
void
PDModel::setData(QVector<float> meanMaxPower)
{
    cp = tau = t0 = 0; // reset on new data
    int size = meanMaxPower.size();

    // truncate the data for 2p and 3p models
    if (model < 3)  size = size < 1200 ? size : 1200;

    data.resize(size);
    tdata.resize(size);
    for (int i=0; i< size; i++) {
        data[i] = meanMaxPower[i];
        tdata[i] = i+1;
    }
    emit dataChanged();
}

// set the intervals to search for bests
void
PDModel::setIntervals(double sanI1, double sanI2, double anI1, double anI2,
                  double aeI1, double aeI2, double laeI1, double laeI2)
{
    this->sanI1 = sanI1;
    this->sanI2 = sanI2;
    this->anI1 = anI1;
    this->anI2 = anI2;
    this->aeI1 = aeI1;
    this->aeI2 = aeI2;
    this->laeI1 = laeI1;
    this->laeI2 = laeI2;

    emit intervalsChanged();
}

// used to wrap a function call when deriving parameters
QMutex calllmfit;
PDModel *calllmfitmodel = NULL;
double calllmfitf(double t, const double *p) {
    return static_cast<PDModel*>(calllmfitmodel)->f(t, p);
}

// using the data and intervals from above, derive the
// cp, tau and t0 values needed for the model
// this is the function originally found in CPPlot
void
PDModel::deriveCPParameters(int model)
{
    // bit of a hack, but the model deriving code is shared
    // basically because it does pretty much the same thing
    // for all the models and I didn't want to abstract it
    // any further, so we pass the subclass as a model number
    // to control which intervals and formula to use
    // where model is
    // 0 = CP 2 parameter
    // 1 = CP 3 parameter
    // 2 = Extended Model  (Damien)
    // 3 = Veloclinic (Mike P)
    // 4 = Ward Smith
    fitsummary="";

    if (fit == LinearRegression) {

        // first lets just do a linear regression on the data as supplied
        QVector<double> joules;
        QVector<double> t;
        QVector<double> p;
        for(int i=0; i<data.count(); i++) {
            if (tdata[i] > 120 && tdata[i] <= 1200) {
                joules << (data[i]*tdata[i]);
                t << tdata[i];
                p << data[i];
            }
        }
        LTMTrend lr(t.constData(),joules.constData(), t.count());
        //fprintf(stderr, "Linear regression: CP=%f, W'=%f\n", lr.slope(), lr.intercept()); fflush(stderr);

        double par[3];
        par[0]=lr.slope();
        par[1]=lr.intercept();
        par[2]=0;
        setParms(par);

        // get vector of residuals
        QVector<double> residuals(t.size());
        double errtot=0;
        double sse=0;
        double meany=0;
        double MEAN=0;
        for(int i=0; i<t.size(); i++){
            double py = y(t[i]/60.0f);  // predicted y
            MEAN += py;
            meany += p[i];              // calculatng mean divided at end
            double err = p[i] - py;     // error for this point
            errtot += err;              // for mean errors divided at end
            sse += err*err;             // sum of squared errors for r2 calc
            residuals[i]=err;           // vector of residuals
        }

        // lets use to calc R2
        meany /= double(t.size());
        MEAN /= double(t.size());
        double sv=0;
        for(int i=0; i<t.size(); i++) {
            sv += (p[i]-meany) * (p[i]-meany);
        }
        double R2=1-(sse/sv);

        // carry on with RMSE
        double mean = errtot/double(t.size()); // mean of residuals MEAN is mean of variable
        errtot = 0;

        // mean of residuals^2
        for(int i=0; i<t.size(); i++) errtot += pow(residuals[i]-mean, 2);
        mean = errtot / double(t.size());

        // RMSE
        double RMSE=sqrt(mean);
        double CV=(RMSE/MEAN) * 100;
        fitsummary = QString("RMSE %1w CV %4% R<sup>2</sup>=%3 [LR] %2 points").arg(RMSE, 0, 'f', 0)
                                                                                       .arg(t.size())
                                                                                       .arg(R2, 0, 'f', 3)
                                                                                       .arg(CV, 0, 'f', 1);

    } else if (fit == LeastSquares && this->nparms() > 0) {
    // only try lmfit if the model supports that

        // used for fit
        QVector<double> p, t;

        // starting values for parameters
        double par[3];

        if (model == 0) {

            // CLASSIC CP
            // we need to prepare the data for the fit
            // mostly this means classic CP only fits to
            // points between 2mins and 20mins to avoid
            // data that is short and rate limited or long
            // and likely submaximal or fatigued (or both!)
            //fprintf(stderr, "points=%d\n", data.count()); fflush(stderr);
            for(int i=0; i<data.count(); i++) {
                if (tdata[i] >= 120 && tdata[i] <= 1200) {
                    p << data[i];
                    t << tdata[i];
                }
            }
            //fprintf(stderr, " used points=%d\n", p.count()); fflush(stderr);

            // if not enough data use everything in desperation...
            if (p.count() < 3) {
                p.resize(0);
                t.resize(0);
                for(int i=0; i<data.count(); i++) {
                    p << data[i];
                    t << tdata[i];
                }
            }

            // set starting values cp and w'
            par[0] = 200;
            par[1] = 15000;

        } else if (model==1) {

            // MORTON 3 PARAMETER
            // Just skip post 20 mins to avoid data that
            // is most likely submaximal or fatigued
            for(int i=0; i<data.count(); i++) {
                if (tdata[i] <= 1200) {
                    p << data[i];
                    t << tdata[i];
                }
            }

            // if not enough data use everything in desperation...
            if (p.count() < 3) {
                p.resize(0);
                t.resize(0);
                for(int i=0; i<data.count(); i++) {
                    p << data[i];
                    t << tdata[i];
                }
            }

            // set starting values cp, w' and k
            par[0] = 250;
            par[1] = 18000;
            par[2] = 32;


        } else {
            p = data;
            t = tdata;
        }

        lm_control_struct control = lm_control_double;
        lm_status_struct status;

        // use forwarder via global variable, so mutex around this !
        calllmfit.lock();
        calllmfitmodel = this;

        //fprintf(stderr, "Fitting ...\n" ); fflush(stderr);
        lmcurve(this->nparms(), par, p.count(), t.constData(), p.constData(), calllmfitf, &control, &status);

        // release for others
        calllmfit.unlock();

        //fprintf(stderr, "Results:\n" );
        //fprintf(stderr, "status after %d function evaluations:\n  %s\n",
        //        status.nfev, lm_infmsg[status.outcome] ); fflush(stderr);
        //fprintf(stderr,"obtained parameters:\n"); fflush(stderr);
        //for (int i = 0; i < this->nparms(); ++i)
        //    fprintf(stderr,"  par[%i] = %12g\n", i, par[i]);
        //fprintf(stderr,"obtained norm:\n  %12g\n", status.fnorm ); fflush(stderr);

        // save away the values we fit to.
        this->setParms(par);

        // calculate RMSE

        // get vector of residuals
        QVector<double> residuals(p.size());
        double MEAN=0;
        double errtot=0;
        for(int i=0; i<p.size(); i++){
            double err = y(t[i]/60.0f) - p[i];  // error
            errtot += err * err; // accumulate squar of errors
            MEAN += y(t[i]/60.0f); // for average error divided at end
        }
        MEAN /= double(p.size()); // average error
        double mean = errtot/double(p.size()); // average square of error

        // RMSE and CV
        double RMSE=sqrt(mean);
        double CV=(RMSE/MEAN) * 100;
        fitsummary = QString("RMSE %1w CV %3% [LM] %2 points").arg(RMSE, 0, 'f', 0)
                                                                      .arg(p.size())
                                                                      .arg(CV, 0, 'f', 1);

    } else {

        // bounds on anaerobic interval in minutes
        const double t1 = anI1;
        const double t2 = anI2;

        // bounds on aerobic interval in minutes
        const double t3 = aeI1;
        const double t4 = aeI2;

        // bounds of these time values in the data
        int i1, i2, i3, i4;

        // find the indexes associated with the bounds
        // the first point must be at least the minimum for the anaerobic interval, or quit
        for (i1 = 0; i1 < t1; i1++)
            if (i1 + 1 > data.size())
                return;
        // the second point is the maximum point suitable for anaerobicly dominated efforts.
        for (i2 = i1; i2 + 1 <= t2; i2++)
            if (i2 + 1 > data.size())
                return;
        // the third point is the beginning of the minimum duration for aerobic efforts
        for (i3 = i2; i3 < t3; i3++)
            if (i3 + 1 > data.size())
                return;
        for (i4 = i3; i4 + 1 <= t4; i4++)
            if (i4 + 1 > data.size())
                break;

        // initial estimate of tau
        if (tau == 0) tau = 1;

        // initial estimate of cp (if not already available)
        if (cp == 0) cp = 300;

        // initial estimate of t0: start small to maximize sensitivity to data
        t0 = 0;

        // lower bound on tau
        const double tau_min = 0.5;

        // convergence delta for tau
        const double tau_delta_max = 1e-4;
        const double t0_delta_max  = 1e-4;

        // previous loop value of tau and t0
        double tau_prev;
        double t0_prev;

        // maximum number of loops
        const int max_loops = 100;

        // loop to convergence
        int iteration = 0;
        do {

            // clear cherries
            map.clear();

            // bounds check, don't go on for ever
            if (iteration++ > max_loops) break;

            // record the previous version of tau, for convergence
            tau_prev = tau;
            t0_prev  = t0;

            // estimate cp, given tau
            int i;
            cp = 0;
            bool changed=false;
            int index=0;
            double value=0;
            for (i = i3; i < i4; i++) {
                double cpn = data[i] / (1 + tau / (t0 + i / 60.0));
                if (cp < cpn) {
                    cp = cpn;
                    index=i; value=data[i];
                    changed=true;
                }
            }
            if(changed) map.insert(index,value);


            // if cp = 0; no valid data; give up
            if (cp == 0.0)
                return;

            // estimate tau, given cp
            tau = tau_min;
            changed=false;
            for (i = i1; i <= i2; i++) {
                double taun = (data[i] / cp - 1) * (i / 60.0 + t0) - t0;
                if (tau < taun) {
                    changed=true;
                    index=i;
                    value=data[i];
                    tau = taun;
                }
            }
            if (changed) map.insert(index,value);

            // estimate t0 - but only for veloclinic/3parm cp
            // where model is non-zero (CP2 is nonzero)
            if (model) t0 = tau / (data[1] / cp - 1) - 1 / 60.0;

            //qDebug()<<"CONVERGING:"<<iteration<<"t0="<<t0<<"tau="<<tau<<"cp="<<cp;

        } while ((fabs(tau - tau_prev) > tau_delta_max) || (fabs(t0 - t0_prev) > t0_delta_max));

        //fprintf(stderr, "tau=%f, t0= %f\n", tau, t0); fflush(stderr);

        // RMSE etc calc for summary
        calcSummary();

    }

}

void
PDModel::calcSummary()
{
    // get vector of residuals
    QVector<double> residuals(data.size());
    double errtot=0;
    double MEAN=0;
    for(int i=0; i<data.size(); i++){
        double err = data[i] - y((i+1)/60.0f);
        MEAN +=  y((i+1)/60.0f);
        errtot += err; // for mean
        residuals[i]=err;
    }
    double mean = errtot/double(data.size());
    MEAN/=double(data.size());
    errtot = 0;

    // mean of residuals^2
    for(int i=0; i<data.size(); i++) errtot += pow(residuals[i]-mean, 2);
    mean = errtot / double(data.size());

    // RMSE
    double RMSE=sqrt(mean);
    double CV=(RMSE/MEAN) *100;
    fitsummary = QString("RMSE %1w CV %3% [envelope] %2 points").arg(RMSE, 0, 'f', 0).arg(data.size()).arg(CV,0,'f',1);
}

//
// Classic 2 Parameter Model
//

CP2Model::CP2Model(Context *context) :
    PDModel(context)
{
    // set default intervals to search CP 2-3 mins and 10-20 mins
    anI1=120;
    anI2=200;
    aeI1=720;
    aeI2=1200;
    model = 0;

    connect (this, SIGNAL(dataChanged()), this, SLOT(onDataChanged()));
    connect (this, SIGNAL(intervalsChanged()), this, SLOT(onIntervalsChanged()));
}

// P(t) - return y for t in 2 parameter model
double 
CP2Model::y(double t) const
{
    // don't start at zero !
    t += (!minutes?1.00f:1/60.00f);

    // adjust to seconds
    if (minutes) t *= 60.00f;

    // classic model - W' / t + CP
    return (cp * tau * 60) / t + cp;
}

// 2 parameter model can calculate these
double 
CP2Model::WPrime()
{
    // kjoules
    return (cp * tau * 60);
}

double
CP2Model::CP()
{
    return cp;
}

// could have just connected signal to slot
// but might want to be more sophisticated in future
void CP2Model::onDataChanged() 
{ 
    // calc tau etc and make sure the interval is
    // set correctly - i.e. 'domain of validity'
    deriveCPParameters(0); 
    setInterval(QwtInterval(tau, PDMODEL_MAXT));

}

void CP2Model::onIntervalsChanged() 
{ 
    deriveCPParameters(0); 
    setInterval(QwtInterval(tau, PDMODEL_MAXT));
}

//
// Morton 3 Parameter Model
//
CP3Model::CP3Model(Context *context) :
    PDModel(context)
{
    // set default intervals to search CP 2-3 mins and 12-20mins
    anI1=120;
    anI2=200;
    aeI1=720;
    aeI2=1200;

    model = 1;
    modelDecay = false;

    connect (this, SIGNAL(dataChanged()), this, SLOT(onDataChanged()));
    connect (this, SIGNAL(intervalsChanged()), this, SLOT(onIntervalsChanged()));
}

// P(t) - return y for t in 2 parameter model
double 
CP3Model::y(double t) const
{
    // don't start at zero !
    t += (!minutes?1.00f:1/60.00f);

    // adjust to seconds
    if (minutes) t *= 60.00f;

    // decay value (75% at 10 hrs)
    double cpdecay=1.0;
    double wdecay=1.0;

    // just use a constant for now - it modifies CP/W' so should adjust to
    // athlete without needing to be fitted (?)
    if (modelDecay) {
        cpdecay = 2.0-(1.0/exp(-0.000009*t));
        wdecay = 2.0-(1.0/exp(-0.000025*t));
    }
    // typical values: CP=285.355547 tau=0.500000 t0=0.381158

    // classic model - W' / t + CP
    return (cp*cpdecay) * (double(1.00f)+(tau*wdecay) /(((double(t)/double(60))+t0)));
}

// 2 parameter model can calculate these
double 
CP3Model::WPrime()
{
    // kjoules
    return (cp * tau * 60);
}

double
CP3Model::CP()
{
    return cp;
}

double
CP3Model::PMax()
{
    // casting to double across to ensure we don't lose precision
    // but basically its the max value of the curve at time t of 1s
    // which is cp * 1 + tau / ((t/60) + t0)
    return cp * (double(1.00f)+tau /(((double(1)/double(60))+t0)));
}


// could have just connected signal to slot
// but might want to be more sophisticated in future
void CP3Model::onDataChanged() 
{ 
    // calc tau etc and make sure the interval is
    // set correctly - i.e. 'domain of validity'
    deriveCPParameters(1); 
    setInterval(QwtInterval(tau, PDMODEL_MAXT));

}

void CP3Model::onIntervalsChanged() 
{ 
    deriveCPParameters(1); 
    setInterval(QwtInterval(tau, PDMODEL_MAXT));
}

//
// Ward Smith Model
//
WSModel::WSModel(Context *context) : PDModel(context)
{
    // set default intervals to search CP 30-60
    anI1=1800;
    anI2=2400;
    aeI1=2400;
    aeI2=3600;

    model = 3;

    connect (this, SIGNAL(dataChanged()), this, SLOT(onDataChanged()));
    connect (this, SIGNAL(intervalsChanged()), this, SLOT(onIntervalsChanged()));
}

// P(t) - return y for t in 2 parameter model
double 
WSModel::y(double t) const
{
    // don't start at zero !
    t += (!minutes?1.00f:1/60.00f);

    // adjust to seconds
    if (minutes) t *= 60.00f;

    //WPrime and PMax
    double WPrime = cp * tau * 60;
    double PMax = cp * (double(1.00f)+tau /(((double(1)/double(60))+t0)));
    double wPMax = PMax-cp;

    // WS Model P(t) = W'/t * (1- exp(-t/( W'/(wPmax)))) + CP
    return ((WPrime / (double(t)))  * (1- exp(-t/(WPrime/wPMax)))) + cp;
}

// 2 parameter model can calculate these
double 
WSModel::WPrime()
{
    // kjoules
    return (cp * tau * 60);
}

double
WSModel::CP()
{
    return cp;
}

double
WSModel::FTP()
{
    return y(45 * 60);
}

double
WSModel::PMax()
{
    // casting to double across to ensure we don't lose precision
    // but basically its the max value of the curve at time t of 1s
    // which is cp * 1 + tau / ((t/60) + t0)
    double WPrime = cp * tau * 60;
    double PMax = cp * (double(1.00f)+tau /(((double(1)/double(60))+t0)));
    double wPMax = PMax-cp;

    // WS Model P(t) = W'/t * (1- exp(-t/( W'/(wPmax)))) + CP
    return ((WPrime / (double(1)))  * (1- exp(-1/(WPrime/wPMax)))) + cp;
}


// could have just connected signal to slot
// but might want to be more sophisticated in future
void WSModel::onDataChanged() 
{ 
    // calc tau etc and make sure the interval is
    // set correctly - i.e. 'domain of validity'
    deriveCPParameters(4); 
    setInterval(QwtInterval(tau, PDMODEL_MAXT));

}

void WSModel::onIntervalsChanged() 
{ 
    deriveCPParameters(4); 
    setInterval(QwtInterval(tau, PDMODEL_MAXT));
}

//
// Veloclinic Multicomponent Model
//
// p(t) = pc1 + pc2
//        Power at time t is the sum of;
//        pc1 - the power from component 1 (fast twitch pools)
//        pc2 - the power from component 2 (slow twitch motor units)
//
// The inputs are derived from the CP2-20 model and 3 constants:
//
//      Pmax - as derived from the CP2-20 model (via t0)
//      w1   - W' as derived from the CP2-20 model
//      p1   - pmax - cp as derived from the CP2-20 model
//      p2   - cp as derived from the CP2-20 model
//      tau1 - W'1 / p1
//      tau2 - 15,000
//      w2   -  A slow twitch W' derived from p2 * tau2
//      alpha- 0.1 thru -0.1, we default to zero
//      beta - 1.0
//
// Fast twitch component is:
//      pc1(t) = W'1 / t * (1-exp(-t/tau1)) * ((1-exp(-t/10)) ^ (1/alpha))
//
// Slow twitch component has three formulations:
//      sprint capped linear)          pc2(t) = p2 * tau2 * (1-exp(-t/tau2))
//      sprint capped regeneration)    pc2(t) = p2 / (1 + t/tau2)
//      sprint capped exponential)     pc2(t) = p2 / (1 + t/5400) ^ (1/beta)
//
// Currently deciding which of the three formulations to use
// as the base for GoldenCheetah (we have enough models already !)
MultiModel::MultiModel(Context *context) :
    PDModel(context),
    variant(0), w1(0), p1(0), p2(0), tau1(0), tau2(0), alpha(0), beta(0)
{
    // set default intervals to search CP 30-60
    // uses the same as the 3 parameter model
    anI1=1800;
    anI2=2400;
    aeI1=2400;
    aeI2=3600;

    model = 4;
    variant = 0; // use exp top/bottom by default.

    connect (this, SIGNAL(dataChanged()), this, SLOT(onDataChanged()));
    connect (this, SIGNAL(intervalsChanged()), this, SLOT(onIntervalsChanged()));
}

void
MultiModel::setVariant(int variant)
{
    this->variant = variant;
    emit intervalsChanged(); // refresh then
}


// P(t) - return y for t in 2 parameter model
double 
MultiModel::y(double t) const
{
    // don't start at zero !
    t += (!minutes?1.00f:1/60.00f);

    // adjust to seconds
    if (minutes) t *= 60.00f;

    // two component model
    double pc1 = w1 / t * (1.00f - exp(-t/tau1)) * pow(1-exp(-t/10.00f), alpha);

    // which variant for pc2 ?
    double pc2 = 0.0f;
    switch (variant) {

            default:
            case 0 : // exponential top and bottom
                pc2 = p2 * tau2 / t * (1-exp(-t/tau2));
                break;

            case 1 : // linear feedback
                pc2 = p2 / (1+t/tau2);
                break;

            case 2 : // regeneration
                pc2 = pow(p2 / (1+t/5400),1/beta);
                //pc2 = p2 / pow((1+t/5400),(1/beta));
                break;

        }

    return (pc1 + pc2);
}

double
MultiModel::FTP()
{
    if (data.size()) return y(minutes ? 60 : 3600);
    return 0;
}

// 2 parameter model can calculate these
double 
MultiModel::WPrime()
{
    // kjoules -- add in difference between CP60 from
    //            velo model and cp as derived
    return w1;
}

double
MultiModel::CP()
{
    if (data.size()) return y(minutes ? 60 : 3600);
    return 0;
}

double
MultiModel::PMax()
{
    // casting to double across to ensure we don't lose precision
    // but basically its the max value of the curve at time t of 1s
    // which is cp * 1 + tau / ((t/60) + t0)
    return cp * (double(1.00f)+tau /(((double(1)/double(60))+t0)));
}


// could have just connected signal to slot
// but might want to be more sophisticated in future
void MultiModel::onDataChanged() 
{ 
    // calc tau etc and make sure the interval is
    // set correctly - i.e. 'domain of validity'
    deriveCPParameters(3); 

    // and veloclinic parameters too;
    w1 = cp*tau*60; // initial estimate from classic cp model
    p1 = PMax() - cp;
    p2 = cp;
    tau1 = w1 / p1;
    tau2 = 15000;
    alpha = 0.0f;
    beta = 1.0;

    // now account for model -- this is rather problematic
    // since the formula uses cp/w' as derived via CP220 but
    // the resulting W' is higher.
    //w1 = (cp + (cp-CP())) * tau * 60;

    setInterval(QwtInterval(tau, PDMODEL_MAXT));
}

void MultiModel::onIntervalsChanged() 
{ 
    deriveCPParameters(3); 

    // and veloclinic parameters too;
    w1 = cp*tau*60; // initial estimate from classic model
    p1 = PMax() - cp;
    p2 = cp;
    tau1 = w1 / p1;
    tau2 = 15000;
    alpha = 0.0f;
    beta = 1.0;

    // now account for model -- this is rather problematic
    // since the formula uses cp/w' as derived via CP220 but
    // the resulting W' is higher.
    w1 = (cp + (cp-CP())) * tau * 60;


    setInterval(QwtInterval(tau, PDMODEL_MAXT));
}

//
// Extended CP Model
//
ExtendedModel::ExtendedModel(Context *context) :
    PDModel(context),
    paa(0), paa_dec(0), ecp(0), etau(0), ecp_del(0), tau_del(0), ecp_dec(0),
    ecp_dec_del(0)
{
    // set default intervals to search Extended CP
    sanI1=20;
    sanI2=90;
    anI1=120;
    anI2=300;
    aeI1=420;
    aeI2=1800;
    laeI1=3600;
    laeI2=30000;

    model = 3;

    connect (this, SIGNAL(dataChanged()), this, SLOT(onDataChanged()));
    connect (this, SIGNAL(intervalsChanged()), this, SLOT(onIntervalsChanged()));
}

// P(t) - return y for t in Extended model
double
ExtendedModel::y(double t) const
{
    // don't start at zero !
    if (t == 0) qDebug() << "ExtendedModel t=0 !!";

    if (!minutes) t /= 60.00f;

    // ATP/PCr in muscles
    double immediate = paa*(1.20-0.20*exp(-1*t))*exp(paa_dec*(t));

    // anaerobic glycolysis
    double nonoxidative  = ecp * (1-exp(tau_del*t)) * (1-exp(ecp_del*t)) * (1+ecp_dec*exp(ecp_dec_del/t)) * (etau/(t));

    // aerobic glycolysis and lipolysis
    double oxidative  = ecp * (1-exp(tau_del*t)) * (1-exp(ecp_del*t)) * (1+ecp_dec*exp(ecp_dec_del/t)) * (1);

    // sum of all for time t
    return immediate + nonoxidative + oxidative;
}

// 2 parameter model can calculate these
double
ExtendedModel::WPrime()
{
    // kjoules
    return (ecp * etau * 60);
}

double
ExtendedModel::CP()
{
    return ecp;
}

double
ExtendedModel::PMax()
{
    // casting to double across to ensure we don't lose precision
    // but basically its the max value of the curve at time t of 1s
    // which is cp * 1 + tau / ((t/60) + t0)
    return paa*(1.20-0.20*exp(-1*(1/60.0)))*exp(paa_dec*(1/60.0)) + ecp * (1-exp(tau_del*(1/60.0))) * (1-exp(ecp_del*(1/60.0))) * (1+ecp_dec*exp(ecp_dec_del/(1/60.0))) * ( 1 + etau/(1/60.0));
}

double
ExtendedModel::FTP()
{
    // casting to double across to ensure we don't lose precision
    // but basically its the max value of the curve at time t of 1s
    // which is cp * 1 + tau / ((t/60) + t0)
    return paa*(1.20-0.20*exp(-1*60.0))*exp(paa_dec*(60.0)) + ecp * (1-exp(tau_del*(60.0))) * (1-exp(ecp_del*60.0)) * (1+ecp_dec*exp(ecp_dec_del/60.0)) * ( 1 + etau/(60.0));
}


// could have just connected signal to slot
// but might want to be more sophisticated in future
void
ExtendedModel::onDataChanged()
{
    // calc tau etc and make sure the interval is
    // set correctly - i.e. 'domain of validity'
    deriveExtCPParameters();
    setInterval(QwtInterval(etau, PDMODEL_MAXT));

}

void
ExtendedModel::onIntervalsChanged()
{
    deriveExtCPParameters();
    setInterval(QwtInterval(etau, PDMODEL_MAXT));
}

void
ExtendedModel::deriveExtCPParameters()
{
#if 0
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // LEAST SQUARES FIT WITH ECP MODEL REQUIRES
    // CONSTRAINTS TO BE VALIDATED AND REFINED
    // THE MODEL IS GROSSLY OVERFITTING WITHOUT
    // PLAUSIBLE CONSTRAINTS FOR DECAY/DELAY
    // PARAMETERS
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // now, if we have a ls fit, at least we have some
    // half decent starting parameters to work from
    if (fit == LeastSquares) {

        // used for fit
        QVector<double> p, t;

        p = data;
        t = tdata;

        fprintf(stderr, "derive %d params using lmfit!\n", this->nparms()); fflush(stderr);

        // set starting values
        double par[8];
        par[0]= paa;
        par[1]= paa_dec;
        par[2]= ecp;
        par[3]= etau;
        par[4]= tau_del;
        par[5]= ecp_del;
        par[6]= ecp_dec;
        par[7]= ecp_dec_del;

        lm_control_struct control = lm_control_double;
        lm_status_struct status;

        // use forwarder via global variable, so mutex around this !
        calllmfit.lock();
        calllmfitmodel = this;

        fprintf(stderr, "Fitting ...\n" ); fflush(stderr);
        lmcurve(this->nparms(), par, p.count(), t.constData(), p.constData(), calllmfitf, &control, &status);

        // release for others
        calllmfit.unlock();

        fprintf(stderr, "Results:\n" );
        fprintf(stderr, "status after %d function evaluations:\n  %s\n",
                status.nfev, lm_infmsg[status.outcome] ); fflush(stderr);

        fprintf(stderr,"obtained parameters:\n"); fflush(stderr);
        for (int i = 0; i < this->nparms(); ++i)
            fprintf(stderr,"  par[%i] = %12g\n", i, par[i]);
        fprintf(stderr,"obtained norm:\n  %12g\n", status.fnorm ); fflush(stderr);

        // save away the values we fit to.
        this->setParms(par);

    } else {
 #endif

        // bounds on anaerobic interval in minutes
        const double t1 = sanI1;
        const double t2 = sanI2;

        // bounds on anaerobic interval in minutes
        const double t3 = anI1;
        const double t4 = anI2;

        // bounds on aerobic interval in minutes
        const double t5 = aeI1;
        const double t6 = aeI2;

        // bounds on long aerobic interval in minutes
        const double t7 = laeI1;
        const double t8 = laeI2;

        // bounds of these time values in the data
        int i1, i2, i3, i4, i5, i6, i7, i8;

        // find the indexes associated with the bounds
        // the first point must be at least the minimum for the anaerobic interval, or quit
        for (i1 = 0; i1 < t1; i1++)
            if (i1 + 1 > data.size())
                return;
        // the second point is the maximum point suitable for anaerobicly dominated efforts.
        for (i2 = i1; i2 + 1 <= t2; i2++)
            if (i2 + 1 >= data.size())
                return;

        // the third point is the beginning of the minimum duration for aerobic efforts
        for (i3 = i2; i3 < t3; i3++)
            if (i3 + 1 >= data.size())
                return;
        for (i4 = i3; i4 + 1 <= t4; i4++)
            if (i4 + 1 >= data.size())
                break;

        // the fifth point is the beginning of the minimum duration for aerobic efforts
        for (i5 = i4; i5 < t5; i5++)
            if (i5 + 1 >= data.size())
                return;
        for (i6 = i5; i6 + 1 <= t6; i6++)
            if (i6 + 1 >= data.size())
                break;

        // the first point must be at least the minimum for the anaerobic interval, or quit
        for (i7 = i6; i7 < t7; i7++)
            if (i7 + 1 >= data.size())
                return;
        // the second point is the maximum point suitable for anaerobicly dominated efforts.
        for (i8 = i7; i8 + 1 <= t8; i8++)
            if (i8 + 1 >= data.size())
                break;


        // initial estimates
        paa = 1000;
        etau = 1.2;
        ecp = 300;
        paa_dec = -2;
        ecp_del = -0.9;
        tau_del = -4.8;
        ecp_dec = -0.6;
        ecp_dec_del = -180;

        // previous loop values
        double etau_prev;
        double paa_prev;
        double paa_dec_prev;
        double ecp_del_prev;
        double ecp_dec_prev;

        // maximum number of loops
        const int max_loops = 100;

        // loop to convergence
        int iteration = 0;
        do {

            // bounds check, don't go on for ever
            if (iteration++ > max_loops) break;

            // clear last point used
            map.clear();

            // record the previous version of tau, for convergence
            etau_prev = etau;
            paa_prev = paa;
            paa_dec_prev = paa_dec;
            ecp_del_prev = ecp_del;
            ecp_dec_prev = ecp_dec;

            //
            // Solve for CP [asymptote]
            //
            int i;
            ecp = 0;
            bool changed=false;
            double val = 0;
            int index=0;
            for (i = i5; i <= i6; i++) {
                double ecpn = (data[i] - paa * (1.20-0.20*exp(-1*(i/60.0))) * exp(paa_dec*(i/60.0))) / (1-exp(tau_del*i/60.0)) / (1-exp(ecp_del*i/60.0)) / (1+ecp_dec*exp(ecp_dec_del/(i/60.0))) / ( 1 + etau/(i/60.0));

                if (ecp < ecpn) {
                    ecp = ecpn;
                    val = data[i];
                    index=i;
                    changed=true;
                }
            }
            if (changed) {
                map.insert(index,val);
                //qDebug()<<iteration<<"eCP Resolving: cp="<<ecp<<"tau="<<etau<<"p[i]"<<val;
            }


            // if cp = 0; no valid data; give up
            if (ecp == 0.0) {
                return;
            }

            //
            // SOLVE FOR TAU [curvature constant]
            //
            etau = etau_min;
            changed=false;
            val = 0;
            for (i = i3; i <= i4; i++) {
                double etaun = ((data[i] - paa * (1.20-0.20*exp(-1*(i/60.0))) * exp(paa_dec*(i/60.0))) / ecp / (1-exp(tau_del*i/60.0)) / (1-exp(ecp_del*i/60.0)) / (1+ecp_dec*exp(ecp_dec_del/(i/60.0))) - 1) * (i/60.0);

                if (etau < etaun) {
                    etau = etaun;
                    val=data[i];
                    index=i;
                    changed=true;
                }
            }
            if (changed) {
                map.insert(index,val);
                //qDebug()<<iteration<<"eCP Resolving: tau="<<etau<<"CP="<<ecp<<"p[i]"<<val;
            }


            //
            // SOLVE FOR PAA_DEC [decay rate for ATP-PCr energy system]
            //
            paa_dec = paa_dec_min;
            changed=false;
            val=0;
            for (i = i1; i <= i2; i++) {
                double paa_decn = log((data[i] - ecp * (1-exp(tau_del*i/60.0)) * (1-exp(ecp_del*i/60.0)) * (1+ecp_dec*exp(ecp_dec_del/(i/60.0))) * ( 1 + etau/(i/60.0)) ) / paa / (1.20-0.20*exp(-1*(i/60.0))) ) / (i/60.0);

                if (paa_dec < paa_decn && paa_decn < paa_dec_max) {
                    paa_dec = paa_decn;
                    changed=true;
                    val=data[i];
                    index=i;
                }
            }
            if(changed) {
                //qDebug()<<iteration<<"eCP Resolving: paa_dec="<<paa_dec<<"CP="<<ecp<<"p[i]"<<val;
                map.insert(index,val);
            }

            //
            // SOLVE FOR PAA [max power]
            //
            paa = paa_min;
            double _avg_paa = 0.0;
            int count=1;
            changed=false;
            val=0;
            for (i = 1; i <= 8; i++) {
                double paan = (data[i] - ecp * (1-exp(tau_del*i/60.0)) * (1-exp(ecp_del*i/60.0)) * (1+ecp_dec*exp(ecp_dec_del/(i/60.0))) * ( 1 + etau/(i/60.0))) / exp(paa_dec*(i/60.0)) / (1.20-0.20*exp(-1*(i/60.0)));
                _avg_paa = (double)((count-1)*_avg_paa+paan)/count;

                if (paa < paan) {
                    paa = paan;
                    changed=true;
                    val=data[i];
                    index=i;
                }
                count++;
            }
            if (changed) {
                map.insert(index,val);
                //qDebug()<<iteration<<"eCP Resolving: paa="<<paa<<"CP="<<ecp<<"p[i]"<<val;
            }

            if (_avg_paa<0.95*paa) {
                paa = _avg_paa;
            }


            //
            // SOLVE FOR ECP_DEC [Fatigue rate; CHO, Motivation, Hydration etc]
            //
            ecp_dec = ecp_dec_min;
            changed=false;
            val=0;
            for (i = i7; i <= i8; i=i+120) {
                double ecp_decn = ((data[i] - paa * (1.20-0.20*exp(-1*(i/60.0))) * exp(paa_dec*(i/60.0))) / ecp / (1-exp(tau_del*i/60.0)) / (1-exp(ecp_del*i/60.0)) / ( 1 + etau/(i/60.0)) -1 ) / exp(ecp_dec_del/(i / 60.0));

                if (ecp_decn > 0) ecp_decn = 0;

                if (ecp_dec < ecp_decn) {
                    ecp_dec = ecp_decn;
                    changed=true;
                    val=data[i];
                    index=i;
                }
            }
            if (changed) {
                //qDebug()<<iteration<<"eCP Resolving: ecp_dec="<<ecp_dec<<"CP="<<ecp<<"p[i]"<<val;
                map.insert(index,val);
            }


        } while ((fabs(etau - etau_prev) > etau_delta_max) ||
                 (fabs(paa - paa_prev) > paa_delta_max)  ||
                 (fabs(paa_dec - paa_dec_prev) > paa_dec_delta_max)  ||
                 (fabs(ecp_del - ecp_del_prev) > ecp_del_delta_max)  ||
                 (fabs(ecp_dec - ecp_dec_prev) > ecp_dec_delta_max)
                );

        // What did we get ...
        // To help debug this below we output the derived values
        // commented out for release, its quite a mouthful !

        int pMax = paa*(1.20-0.20*exp(-1*(1/60.0)))*exp(paa_dec*(1/60.0)) + ecp *
                   (1-exp(tau_del*(1/60.0))) * (1-exp(ecp_del*(1/60.0))) *
                   (1+ecp_dec*exp(ecp_dec_del/(1/60.0))) *
                   (1+etau/(1/60.0));

        int mmp60 = paa*(1.20-0.20*exp(-1*60.0))*exp(paa_dec*(60.0)) + ecp *
                   (1-exp(tau_del*(60.0))) * (1-exp(ecp_del*60.0)) *
                   (1+ecp_dec*exp(ecp_dec_del/60.0)) *
                   (1+etau/(60.0));

#if 0
        qDebug() <<"eCP(5.3) " << "paa" << paa  << "ecp" << ecp << "etau" << etau
                 << "paa_dec" << paa_dec << "ecp_del" << ecp_del << "ecp_dec"
                 << ecp_dec << "ecp_dec_del" << ecp_dec_del;

        qDebug() <<"eCP(5.3) " << "pmax" << pMax << "mmp60" << mmp60;
    }
#endif
        // get vector of residuals
        QVector<double> residuals(data.size());
        double errtot=0;
        double MEAN=0;
        for(int i=0; i<data.size(); i++){
            double err = data[i] - y((i+1)/60.0f);
            MEAN +=  y((i+1)/60.0f);
            errtot += err; // for mean
            residuals[i]=err;
        }
        double mean = errtot/double(data.size());
        MEAN/=double(data.size());
        errtot = 0;

        // mean of residuals^2
        for(int i=0; i<data.size(); i++) errtot += pow(residuals[i]-mean, 2);
        mean = errtot / double(data.size());

        // RMSE
        double RMSE=sqrt(mean);
        double CV=(RMSE/MEAN)*100;
        fitsummary = QString("RMSE %1w CV %3% [envelope] %2 points").arg(RMSE, 0, 'f', 0).arg(data.size()).arg(CV,0,'f',1);
}

QList<QPointF> 
PDModel::cherries()
{
    QList<QPointF> returning;

    QMapIterator<int,double> it(map);
    it.toFront();
    while(it.hasNext()) {
        it.next();
        returning << QPointF(it.key(), it.value());
    }
    return returning;
}

void MultiModel::loadParameters(QList<double>&here)
{
    cp = here[0];
    tau = here[1];
    t0 = here[2];
    w1 = here[3];
    p1 = here[4];
    p2 = here[5];
    tau1 = here[6];
    tau2 = here[7];
    alpha = here[8];
    beta = here[9];
}

void MultiModel::saveParameters(QList<double>&here)
{
    here.clear();
    here << cp;
    here << tau;
    here << t0;
    here << w1;
    here << p1;
    here << p2;
    here << tau1;
    here << tau2;
    here << alpha;
    here << beta;
}

void ExtendedModel::loadParameters(QList<double>&here)
{
    cp = here[0];
    tau = here[1];
    t0 = here[2];
    paa = here[3];
    etau = here[4];
    ecp = here[5];
    paa_dec = here[6];
    ecp_del = here[7];
    tau_del = here[8];
    ecp_dec = here[9];
    ecp_dec_del = here[10];
}

void ExtendedModel::saveParameters(QList<double>&here)
{
    here.clear();
    here << cp;
    here << tau;
    here << t0;
    here << paa;
    here << etau;
    here << ecp;
    here << paa_dec;
    here << ecp_del;
    here << tau_del;
    here << ecp_dec;
    here << ecp_dec_del;
}

// 2 and 3 parameter models only load and save cp, tau and t0
void CP2Model::loadParameters(QList<double>&here)
{
    cp = here[0];
    tau = here[1];
    t0 = here[2];
}

void CP2Model::saveParameters(QList<double>&here) 
{
    here.clear();
    here << cp;
    here << tau;
    here << t0;
}

void CP3Model::loadParameters(QList<double>&here)
{
    cp = here[0];
    tau = here[1];
    t0 = here[2];
}
void CP3Model::saveParameters(QList<double>&here)
{
    here.clear();
    here << cp;
    here << tau;
    here << t0;
}
void WSModel::loadParameters(QList<double>&here)
{
    cp = here[0];
    tau = here[1];
    t0 = here[2];
}
void WSModel::saveParameters(QList<double>&here)
{
    here.clear();
    here << cp;
    here << tau;
    here << t0;
}
