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

// base class for all models
PDModel::PDModel(Context *context) :
    QwtSyntheticPointData(PDMODEL_MAXT),
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
    data.resize(meanMaxPower.size());
    data = meanMaxPower;
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
    data.resize(meanMaxPower.size());
    for (int i=0; i< data.size(); i++) data[i] = meanMaxPower[i];
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

// using the data and intervals from above, derive the
// cp, tau and t0 values needed for the model
// this is the function originally found in CPPlot
void
PDModel::deriveCPParameters(bool three)
{
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

        // bounds check, don't go on for ever
        if (iteration++ > max_loops) break;

        // record the previous version of tau, for convergence
        tau_prev = tau;
        t0_prev  = t0;

        // estimate cp, given tau
        int i;
        cp = 0;
        for (i = i3; i < i4; i++) {
            double cpn = data[i] / (1 + tau / (t0 + i / 60.0));
            if (cp < cpn)
                cp = cpn;
        }

        // if cp = 0; no valid data; give up
        if (cp == 0.0)
            return;

        // estimate tau, given cp
        tau = tau_min;
        for (i = i1; i <= i2; i++) {
            double taun = (data[i] / cp - 1) * (i / 60.0 + t0) - t0;
            if (tau < taun)
                tau = taun;
        }

        // estimate t0 - but only for veloclinic/3parm cp
        if (three) t0 = tau / (data[1] / cp - 1) - 1 / 60.0;

    } while ((fabs(tau - tau_prev) > tau_delta_max) || (fabs(t0 - t0_prev) > t0_delta_max));
}

//
// Classic 2 Parameter Model
//

CP2Model::CP2Model(Context *context) :
    PDModel(context)
{
    // set default intervals to search CP 2-20
    anI1=100;
    anI2=120;
    aeI1=1000;
    aeI2=1200;

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
    deriveCPParameters(); 
    setInterval(QwtInterval(tau, PDMODEL_MAXT));

}

void CP2Model::onIntervalsChanged() 
{ 
    deriveCPParameters(); 
    setInterval(QwtInterval(tau, PDMODEL_MAXT));
}

//
// Morton 3 Parameter Model
//
CP3Model::CP3Model(Context *context) :
    PDModel(context)
{
    // set default intervals to search CP 30-60
    anI1=1800;
    anI2=2400;
    aeI1=2400;
    aeI2=3600;

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

    // classic model - W' / t + CP
    return cp * (double(1.00f)+tau /(((double(t)/double(60))+t0)));
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
    deriveCPParameters(true); 
    setInterval(QwtInterval(tau, PDMODEL_MAXT));

}

void CP3Model::onIntervalsChanged() 
{ 
    deriveCPParameters(true); 
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
    PDModel(context)
{
    // set default intervals to search CP 30-60
    // uses the same as the 3 parameter model
    anI1=1800;
    anI2=2400;
    aeI1=2400;
    aeI2=3600;

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
    deriveCPParameters(true); 

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
    deriveCPParameters(true); 

    // and veloclinic paramters too;
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
    PDModel(context)
{
    // set default intervals to search Extended CP
    sanI1=20;
    sanI2=90;
    anI1=120;
    anI2=300;
    aeI1=600;
    aeI2=3000;
    laeI1=3600;
    laeI2=30000;

    connect (this, SIGNAL(dataChanged()), this, SLOT(onDataChanged()));
    connect (this, SIGNAL(intervalsChanged()), this, SLOT(onIntervalsChanged()));
}

// P(t) - return y for t in Extended model
double
ExtendedModel::y(double t) const
{
    // don't start at zero !
    t += (!minutes?1.00f:1/60.00f);

    if (!minutes) t /= 60.00f;
    return paa*(1.20-0.20*exp(-1*double(t)))*exp(paa_dec*(double(t))) + ecp * (1-exp(tau_del*double(t))) * (1-exp(ecp_del*double(t))) * (1+ecp_dec*exp(ecp_dec_del/double(t))) * ( 1 + etau/(double(t)));
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



    // initial estimate
    paa = 300;
    etau = 1;
    ecp = 300;
    paa_dec = -2;
    ecp_del = -0.9;
    tau_del = -4.8;
    ecp_dec = -1;
    ecp_dec_del = -180;

    // lower bound
    const double paa_min = 100;
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

        // record the previous version of tau, for convergence
        etau_prev = etau;
        paa_prev = paa;
        paa_dec_prev = paa_dec;
        ecp_del_prev = ecp_del;
        ecp_dec_prev = ecp_dec;

        // estimate cp, given tau
        int i;
        ecp = 0;
        for (i = i5; i <= i6; i++) {
            double ecpn = (data[i] - paa * (1.20-0.20*exp(-1*(i/60.0))) * exp(paa_dec*(i/60.0))) / (1-exp(tau_del*i/60.0)) / (1-exp(ecp_del*i/60.0)) / (1+ecp_dec*exp(ecp_dec_del/(i/60.0))) / ( 1 + etau/(i/60.0));

            if (ecp < ecpn)
                ecp = ecpn;
        }


        // if cp = 0; no valid data; give up
        if (ecp == 0.0)
            return;

        // estimate etau, given ecp
        etau = etau_min;
        for (i = i3; i <= i4; i++) {
            double etaun = ((data[i] - paa * (1.20-0.20*exp(-1*(i/60.0))) * exp(paa_dec*(i/60.0))) / ecp / (1-exp(tau_del*i/60.0)) / (1-exp(ecp_del*i/60.0)) / (1+ecp_dec*exp(ecp_dec_del/(i/60.0))) - 1) * (i/60.0);

            if (etau < etaun)
                etau = etaun;
        }

        // estimate paa_dec
        paa_dec = paa_dec_min;
        for (i = i1; i <= i2; i++) {
            double paa_decn = log((data[i] - ecp * (1-exp(tau_del*i/60.0)) * (1-exp(ecp_del*i/60.0)) * (1+ecp_dec*exp(ecp_dec_del/(i/60.0))) * ( 1 + etau/(i/60.0)) ) / paa / (1.20-0.20*exp(-1*(i/60.0))) ) / (i/60.0);

            if (paa_dec < paa_decn && paa_decn < paa_dec_max) {
                paa_dec = paa_decn;
            }
        }

        paa = paa_min;
        double _avg_paa = 0.0;
        int count=1;
        for (i = 2; i <= 8; i++) {
            double paan = (data[i] - ecp * (1-exp(tau_del*i/60.0)) * (1-exp(ecp_del*i/60.0)) * (1+ecp_dec*exp(ecp_dec_del/(i/60.0))) * ( 1 + etau/(i/60.0))) / exp(paa_dec*(i/60.0)) / (1.20-0.20*exp(-1*(i/60.0)));
            _avg_paa = (double)((count-1)*_avg_paa+paan)/count;

            if (paa < paan)
                paa = paan;
            count++;
        }
        if (_avg_paa<0.95*paa) {
            paa = _avg_paa;
        }


        ecp_dec = ecp_dec_min;
        for (i = i7; i <= i8; i=i+120) {
            double ecp_decn = ((data[i] - paa * (1.20-0.20*exp(-1*(i/60.0))) * exp(paa_dec*(i/60.0))) / ecp / (1-exp(tau_del*i/60.0)) / (1-exp(ecp_del*i/60.0)) / ( 1 + etau/(i/60.0)) -1 ) / exp(ecp_dec_del/(i / 60.0));

            if (ecp_decn > 0) ecp_decn = 0;

            if (ecp_dec < ecp_decn)
                ecp_dec = ecp_decn;
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

#if 0
    int pMax = paa*(1.20-0.20*exp(-1*(1/60.0)))*exp(paa_dec*(1/60.0)) + ecp * 
               (1-exp(tau_del*(1/60.0))) * (1-exp(ecp_del*(1/60.0))) * 
               (1+ecp_dec*exp(ecp_dec_del/(1/60.0))) * 
               (1+etau/(1/60.0));

    int mmp60 = paa*(1.20-0.20*exp(-1*60.0))*exp(paa_dec*(60.0)) + ecp * 
               (1-exp(tau_del*(60.0))) * (1-exp(ecp_del*60.0)) * 
               (1+ecp_dec*exp(ecp_dec_del/60.0)) * 
               (1+etau/(60.0));

    qDebug() <<"eCP(5.3) " << "paa" << paa  << "ecp" << ecp << "etau" << etau 
             << "paa_dec" << paa_dec << "ecp_del" << ecp_del << "ecp_dec" 
             << ecp_dec << "ecp_dec_del" << ecp_dec_del;

    qDebug() <<"eCP(5.3) " << "pmax" << pMax << "mmp60" << mmp60;
#endif
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
