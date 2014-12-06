/*
 * Copyright (c) 2013 Damien Grauser (Damien.Grauser@gmail.com)
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


#include "ExtendedCriticalPower.h"
#include "Colors.h"
#include "Settings.h"
#include "RideFileCache.h"
#include <qwt_symbol.h>

#include <QtGui>
#include <QMessageBox>

ExtendedCriticalPower::ExtendedCriticalPower(Context *context) : context(context)
{
}

ExtendedCriticalPower::~ExtendedCriticalPower()
{
}

Model_eCP
ExtendedCriticalPower::deriveExtendedCP_2_3_Parameters(RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2)
{
    Model_eCP model;
    model.version = "eCP v2.3";

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
    for (i1 = 0; i1 < 60 * t1; i1++)
        if (i1 + 1 >= bests->meanMaxArray(series).size())
            return model;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i2 = i1; i2 + 1 <= 60 * t2; i2++)
        if (i2 + 1 >= bests->meanMaxArray(series).size())
            return model;

    // the third point is the beginning of the minimum duration for aerobic efforts
    for (i3 = i2; i3 < 60 * t3; i3++)
        if (i3 + 1 >= bests->meanMaxArray(series).size())
            return model;
    for (i4 = i3; i4 + 1 <= 60 * t4; i4++)
        if (i4 + 1 >= bests->meanMaxArray(series).size())
            break;

    // the fifth point is the beginning of the minimum duration for aerobic efforts
    for (i5 = i4; i5 < 60 * t5; i5++)
        if (i5 + 1 >= bests->meanMaxArray(series).size())
            return model;
    for (i6 = i5; i6 + 1 <= 60 * t6; i6++)
        if (i6 + 1 >= bests->meanMaxArray(series).size())
            break;

    // the first point must be at least the minimum for the anaerobic interval, or quit
    for (i7 = i6; i7 < 60 * t7; i7++)
        if (i7 + 1 >= bests->meanMaxArray(series).size())
            return model;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i8 = i7; i8 + 1 <= 60 * t8; i8++)
        if (i8 + 1 >= bests->meanMaxArray(series).size())
            break;

    bool withoutEcpDec = false;

    // initial estimate of tau
    if (model.paa == 0)
        model.paa = 300;

    if (model.paa_dec == 0)
        model.paa_dec = -2;

    if (model.etau == 0)
        model.etau = 1;

    if (model.ecp == 0)
        model.ecp = 300;

    if (model.ecp_del == 0)
        model.ecp_del = -1;

    if (model.ecp_dec == 0)
        model.ecp_dec = -0.2;

    if (model.ecp_dec_del == 0)
        model.ecp_dec_del = -180;


    // lower bound on tau
    const double paa_min = 100;
    //const double paa_max = 2000;
    const double etau_min = 0.5;
    const double paa_dec_max = -0.25;
    const double paa_dec_min = -3;
    const double ecp_dec_min = -1.5;

    // convergence delta for tau
    //const double ecp_delta_max = 1e-4;
    const double etau_delta_max = 1e-4;
    const double paa_delta_max = 1e-2;
    const double paa_dec_delta_max = 1e-4;
    const double ecp_del_delta_max = 1e-4;
    const double ecp_dec_delta_max  = 1e-8;

    // previous loop value of tau and t0
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
        if (iteration ++ > max_loops) {
            qDebug()<<"Maximum number of loops exceeded in ecp2 model";
            break;
        }

        // record the previous version of tau, for convergence
        etau_prev = model.etau;
        paa_prev = model.paa;
        paa_dec_prev = model.paa_dec;
        ecp_del_prev = model.ecp_del;
        ecp_dec_prev = model.ecp_dec;


        if (withoutEcpDec)
            model.ecp_dec = 0;

        // P = paa*exp(paa_dec*(x/60)) + ecp * (1-exp(ecp_del*x/60)) * (1+ecp_dec*exp(-180/x/60) (1 + etau/(x/60))
        // bests->meanMaxArray(series)[i] = paa*exp(paa_dec*(i/60.0)) + ecp * (1-exp(ecp*i/60.0)) * (exp(40+ecp_dec*i/60.0)) * ( 1 + etau/(i/60.0));

        // estimate ecp
        int i;
        model.ecp = 0;
        for (i = i5; i <= i6; i++) {
            double ecpn = (bests->meanMaxArray(series)[i] - model.paa*exp(model.paa_dec*(i/60.0))) / (1-exp(model.ecp_del*i/60.0)) / (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) / ( 1 + model.etau/(i/60.0));

            if (model.ecp < ecpn)
                model.ecp = ecpn;
        }
        //qDebug() << "estimate ecp" << model.ecp;

        // if ecp = 0; no valid data; give up
        if (model.ecp == 0.0)
            return model;

        // estimate etau
        model.etau = etau_min;
        for (i = i3; i <= i4; i++) {
            double etaun = ((bests->meanMaxArray(series)[i] - model.paa*exp(model.paa_dec*(i/60.0))) /model. ecp / (1-exp(model.ecp_del*i/60.0)) / (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) - 1) * (i/60.0);

            if (model.etau < etaun)
                model.etau = etaun;
        }
        //qDebug() << "estimate etau" << model.etau;

        model.paa_dec = paa_dec_min;
        for (i = i1; i <= i2; i++) {
            double paa_decn = log((bests->meanMaxArray(series)[i] - model.ecp * (1-exp(model.ecp_del*i/60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) * ( 1 + model.etau/(i/60.0)) ) / model.paa) / (i/60.0);
            if (model.paa_dec < paa_decn && paa_decn < paa_dec_max) {
                model.paa_dec = paa_decn;
            }
        }
        //qDebug() << "estimate paa_dec" << model.paa_dec;

        model.paa = paa_min;
        double _avg_paa = 0.0;
        int count=1;
        for (i = 1; i <= 8; i++) {
            double paan = (bests->meanMaxArray(series)[i] - model.ecp * (1-exp(model.ecp_del*i/60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) * ( 1 + model.etau/(i/60.0))) / exp(model.paa_dec*(i/60.0));
            _avg_paa = (double)((count-1)*_avg_paa+paan)/count;

            if (model.paa < paan)
                model.paa = paan;

            count++;
        }
        //qDebug() << "estimate paa" << model.paa;

        // Max power can be wrong data
        // Use avg if the diff is too big
        if (_avg_paa<0.95*model.paa) {
            model.paa = _avg_paa;
            //qDebug() << "use avg paa" << model.paa;
        }

        if (!withoutEcpDec) {
            model.ecp_dec = ecp_dec_min;
            for (i = i7; i <= i8; i=i+120) {
                double ecp_decn = ((bests->meanMaxArray(series)[i] - model.paa*exp(model.paa_dec*(i/60.0))) / model.ecp / (1-exp(model.ecp_del*i/60.0)) / ( 1 + model.etau/(i/60.0)) - 1) / exp(model.ecp_dec_del/(i / 60.0));

                if (ecp_decn > 0)
                    ecp_decn = 0;

                if (model.ecp_dec < ecp_decn)
                    model.ecp_dec = ecp_decn;
            }
            //qDebug() << "estimate ecp_dec" << model.ecp_dec;
        }
    } while ((fabs(model.etau - etau_prev) > etau_delta_max) ||
             (fabs(model.paa - paa_prev) > paa_delta_max)  ||
             (fabs(model.paa_dec - paa_dec_prev) > paa_dec_delta_max)  ||
             (fabs(model.ecp_del - ecp_del_prev) > ecp_del_delta_max)  ||
             (fabs(model.ecp_dec - ecp_dec_prev) > ecp_dec_delta_max)
            );

    //qDebug() << iteration << "iterations";

    model.pMax = model.paa*exp(model.paa_dec*(1/60.0)) + model.ecp * (1-exp(model.ecp_del*(1/60.0))) * (1+model.ecp_dec*exp(model.ecp_dec_del/(1/60.0))) * ( 1 + model.etau/(1/60.0));
    model.mmp60 = model.paa*exp(model.paa_dec*(60.0)) + model.ecp * (1-exp(model.ecp_del*60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/(60.0))) * ( 1 + model.etau/(60.0));

    qDebug() <<"eCP(2.3) " << "paa" << model.paa << "paa_dec" << model.paa_dec << "ecp" << model.ecp << "etau" << model.etau  << "ecp_del" << model.ecp_del << "ecp_dec" << model.ecp_dec  << "ecp_dec_del" << model.ecp_dec_del;
    qDebug() <<"eCP(2.3) " << "pmax" << model.pMax << "mmp60" << model.mmp60;

    return model;
}

QwtPlotCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_2_3(Model_eCP model)
{
    const int extendedCurve2_points = 1000;

    QVector<double> extended_cp_curve2_power(extendedCurve2_points);
    QVector<double> extended_cp_curve2_time(extendedCurve2_points);
    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.paa * exp(model.paa_dec*(t)) + model.ecp * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 + model.etau/(t));
    }

    QwtPlotCurve *extendedCPCurve2 = new QwtPlotCurve("eCP");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen e2pen(GColor(CHEARTRATE));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);
    extendedCPCurve2->setSamples(extended_cp_curve2_time.data(), extended_cp_curve2_power.data(), extendedCurve2_points);

    return extendedCPCurve2;
}

Model_eCP
ExtendedCriticalPower::deriveExtendedCP_4_3_Parameters(bool usebest, RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2)
{
    Model_eCP model;
    model.version = "eCP v4.3";

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
    for (i1 = 0; i1 < 60 * t1; i1++)
        if (i1 + 1 >= bests->meanMaxArray(series).size())
            return model;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i2 = i1; i2 + 1 <= 60 * t2; i2++)
        if (i2 + 1 >= bests->meanMaxArray(series).size())
            return model;

    // the third point is the beginning of the minimum duration for aerobic efforts
    for (i3 = i2; i3 < 60 * t3; i3++)
        if (i3 + 1 >= bests->meanMaxArray(series).size())
            return model;
    for (i4 = i3; i4 + 1 <= 60 * t4; i4++)
        if (i4 + 1 >= bests->meanMaxArray(series).size())
            break;

    // the fifth point is the beginning of the minimum duration for aerobic efforts
    for (i5 = i4; i5 < 60 * t5; i5++)
        if (i5 + 1 >= bests->meanMaxArray(series).size())
            return model;
    for (i6 = i5; i6 + 1 <= 60 * t6; i6++)
        if (i6 + 1 >= bests->meanMaxArray(series).size())
            break;

    // the first point must be at least the minimum for the anaerobic interval, or quit
    for (i7 = i6; i7 < 60 * t7; i7++)
        if (i7 + 1 >= bests->meanMaxArray(series).size())
            return model;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i8 = i7; i8 + 1 <= 60 * t8; i8++)
        if (i8 + 1 >= bests->meanMaxArray(series).size())
            break;

    bool withoutEd = false;

    // initial estimate of tau
    if (model.paa == 0)
        model.paa = 300;

    if (model.etau == 0)
        model.etau = 1;

    if (model.ecp == 0)
        model.ecp = 300;

    if (model.paa_dec == 0)
        model.paa_dec = -2;

    if (model.ecp_del == 0)
        model.ecp_del = -1;

    if (model.ecp_dec == 0)
        model.ecp_dec = -1;

    if (model.ecp_dec_del == 0)
        model.ecp_dec_del = -180;


    // lower bound on tau
    const double paa_min = 100;
    //const double paa_max = 2000;
    const double etau_min = 0.5;
    const double paa_dec_max = -0.25;
    const double paa_dec_min = -3;
    const double ecp_dec_min = -5; // Long

    // convergence delta for tau
    //const double ecp_delta_max = 1e-4;
    const double etau_delta_max = 1e-4;
    const double paa_delta_max = 1e-2;
    const double paa_dec_delta_max = 1e-4;
    const double ecp_del_delta_max = 1e-4;
    const double ecp_dec_delta_max  = 1e-8;

    // previous loop value of tau and t0
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
        if (iteration ++ > max_loops) {
            qDebug()<<"Maximum number of loops exceeded in ecp5 model";
            break;
        }

        // record the previous version of tau, for convergence
        etau_prev = model.etau;
        paa_prev = model.paa;
        paa_dec_prev = model.paa_dec;
        ecp_del_prev = model.ecp_del;
        ecp_dec_prev = model.ecp_dec;

        if (withoutEd)
            model.ecp_dec = 0;

        // P = paa* (2.0-exp(-1*(x/60.0)))*exp(paa_dec*(x/60)) + ecp * (1-exp(ecp_del*x/60)) * (1+ecp_dec*exp(-180/x/60) (1 + etau/(x/60))
        // bests->meanMaxArray(series)[i] = paa*(2.0-exp(-1*(i/60.0)))*exp(paa_dec*(i/60.0))  + ecp * (1-exp(ecp*i/60.0)) * (1+2d*exp(180/i/60.0)) * ( 1 + etau/(i/60.0));

        // estimate ecp
        int i;

        model.ecp = 0;
        double _avg_ecp = 0.0;
        int count=1;
        for (i = i5; i <= i6; i++) {
            double ecpn = (bests->meanMaxArray(series)[i] - model.paa * (2.0-exp(-1*(i/60.0))) * exp(model.paa_dec*(i/60.0))) / (1-exp(model.ecp_del*i/60.0)) / (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) / ( 1 + model.etau/(i/60.0));
            _avg_ecp = (double)((count-1)*_avg_ecp+ecpn)/count;

            if (model.ecp < ecpn)
                model.ecp = ecpn;

            count++;
        }
        //qDebug() << "estimate ecp" << model.ecp;
        if (!usebest) {
            model.ecp = _avg_ecp;
        }

        // if ecp = 0; no valid data; give up
        if (model.ecp == 0.0)
            return model;

        // estimate etau
        model.etau = etau_min;
        double _avg_etau = 0.0;
        count=1;
        for (i = i3; i <= i4; i++) {
            double etaun = ((bests->meanMaxArray(series)[i] - model.paa * (2.0-exp(-1*(i/60.0))) * exp(model.paa_dec*(i/60.0))) /model. ecp / (1-exp(model.ecp_del*i/60.0)) / (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) - 1) * (i/60.0);
            _avg_etau = (double)((count-1)*_avg_etau+etaun)/count;

            if (model.etau < etaun)
                model.etau = etaun;
            count++;
        }
        //qDebug() << "estimate etau" << model.etau;
        if (!usebest) {
            model.etau = _avg_etau;
        }



        model.paa_dec = paa_dec_min;
        double _avg_paa_dec = 0.0;
        count=1;
        for (i = i1; i <= i2; i++) {
            double paa_decn = log((bests->meanMaxArray(series)[i] - model.ecp * (1-exp(model.ecp_del*i/60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) * ( 1 + model.etau/(i/60.0)) ) / model.paa / (2.0-exp(-1*(i/60.0))) ) / (i/60.0);
            _avg_paa_dec = (double)((count-1)*_avg_paa_dec+paa_decn)/count;

            if (model.paa_dec < paa_decn && paa_decn < paa_dec_max) {
                model.paa_dec = paa_decn;
            }
            count++;
        }
        //qDebug() << "estimate paa_dec" << model.paa_dec;
        if (!usebest) {
            model.paa_dec = _avg_paa_dec;
        }

        model.paa = paa_min;
        double _avg_paa = 0.0;
        count=1;
        for (i = 1; i <= 8; i++) {
            double paan = (bests->meanMaxArray(series)[i] - model.ecp * (1-exp(model.ecp_del*i/60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) * ( 1 + model.etau/(i/60.0))) / exp(model.paa_dec*(i/60.0)) / (2.0-exp(-1*(i/60.0)));
            _avg_paa = (double)((count-1)*_avg_paa+paan)/count;

            //qDebug() << "   estimate paan" << paan;
            if (model.paa < paan)
                model.paa = paan;
            count++;
        }
        //qDebug() << "estimate paa" << model.paa;
        if (!usebest  || _avg_paa<0.9*model.paa) {
            model.paa = _avg_paa;
        }

        if (!withoutEd) {
            model.ecp_dec = ecp_dec_min;
            double _avg_ecp_dec = 0.0;
            count=1;
            for (i = i7; i <= i8; i=i+120) {
                double ecp_decn = ((bests->meanMaxArray(series)[i] - model.paa * (2.0-exp(-1*(i/60.0)))*exp(model.paa_dec*(i/60.0))) / model.ecp / (1-exp(model.ecp_del*i/60.0)) / ( 1 + model.etau/(i/60.0)) -1 ) / exp(model.ecp_dec_del/(i / 60.0));
                _avg_ecp_dec = (double)((count-1)*_avg_ecp_dec+ecp_decn)/count;

                if (ecp_decn > 0)
                    ecp_decn = 0;

                if (model.ecp_dec < ecp_decn)
                    model.ecp_dec = ecp_decn;
                count++;
            }
            //qDebug() << "estimate ecp_dec" << model.ecp_dec;
            if (!usebest) {
                model.ecp_dec = _avg_ecp_dec;
            }

        }
    } while ((fabs(model.etau - etau_prev) > etau_delta_max) ||
             (fabs(model.paa - paa_prev) > paa_delta_max)  ||
             (fabs(model.paa_dec - paa_dec_prev) > paa_dec_delta_max)  ||
             (fabs(model.ecp_del - ecp_del_prev) > ecp_del_delta_max)  ||
             (fabs(model.ecp_dec - ecp_dec_prev) > ecp_dec_delta_max)
            );

    //qDebug() << iteration << "iterations";

    model.pMax = model.paa*(2.0-exp(-1*(1/60.0)))*exp(model.paa_dec*(1/60.0)) + model.ecp * (1-exp(model.ecp_del*(1/60.0))) * (1+model.ecp_dec*exp(model.ecp_dec_del/(1/60.0))) * ( 1 + model.etau/(1/60.0));
    model.mmp60 = model.paa*(2.0-exp(-1*60.0))*exp(model.paa_dec*(60.0)) + model.ecp * (1-exp(model.ecp_del*60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/60.0)) * ( 1 + model.etau/(60.0));

    qDebug() <<"eCP(4.3) " << "paa" << model.paa  << "ecp" << model.ecp << "etau" << model.etau << "paa_dec" << model.paa_dec << "ecp_del" << model.ecp_del << "ecp_dec" << model.ecp_dec << "ecp_dec_del" << model.ecp_dec_del;
    qDebug() <<"eCP(4.3) " << "pmax" << model.pMax << "mmp60" << model.mmp60;

    return model;
}

Model_eCP
ExtendedCriticalPower::deriveExtendedCP_4_3_ParametersForBest(double best5s, double best1min, double best5min, double best1hour)
{
    Model_eCP model;

    // initial estimate of tau
    model.paa = 300;
    model.etau = 1;
    model.ecp = 300;
    model.paa_dec = -2;
    model.ecp_del = -1;
    model.ecp_dec = 0;

    // convergence delta for tau
    //const double ecp_delta_max = 1e-4;
    const double etau_delta_max = 1e-4;
    const double paa_delta_max = 1e-2;
    const double paa_dec_delta_max = 1e-4;
    const double ecp_del_delta_max = 1e-4;
    const double ecp_dec_delta_max  = 1e-8;

    // previous loop value of tau and t0
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
        if (iteration ++ > max_loops) {
            qDebug()<<"Maximum number of loops exceeded in ecp2 model";
            break;
        }

        if (model.paa_dec>0)
            model.paa_dec = -0.5;

        // record the previous version of tau, for convergence
        etau_prev = model.etau;
        paa_prev = model.paa;
        paa_dec_prev = model.paa_dec;
        ecp_del_prev = model.ecp_del;
        ecp_dec_prev = model.ecp_dec;

        //qDebug() << "best1hour" << best1hour;
        model.ecp = (best1hour - model.paa * (2.0-exp(-1*(3600/60.0))) * (exp(model.paa_dec*(3600/60.0))) ) / (1-exp(model.ecp_del*3600/60.0)) / (1+model.ecp_dec*exp(model.ecp_dec_del/(3600/60.0))) / ( 1 + model.etau/(3600/60.0));
        //qDebug() << "model.ecp" << model.ecp;

        //qDebug() << "best5min" << best5min;
        model.etau = ((best5min - model.paa * (2.0-exp(-1*(300/60.0))) * (exp(model.paa_dec*(300/60.0))) ) / model.ecp / (1-exp(model.ecp_del*300/60.0)) / (1+model.ecp_dec*exp(model.ecp_dec_del/(300/60.0))) - 1) * (300/60.0);
        //qDebug() << "model.etau" << model.etau;

        //qDebug() << "best1min" << best1min;
        model.paa_dec = log((best1min - model.ecp * (1-exp(model.ecp_del*60/60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/(60/60.0))) * ( 1 + model.etau/(60/60.0)) ) / model.paa / (2.0-exp(-1*(60/60.0)))) / (60/60.0);
        //qDebug() << "model.paa_dec" << model.paa_dec;

        //qDebug() << "best5s" << best5s;
        model.paa = (best5s - model.ecp * (1-exp(model.ecp_del*5/60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/(5/60.0))) * ( 1 + model.etau/(5/60.0))) / exp(model.paa_dec*(5/60.0)) / (2.0-exp(-1*(5/60.0)));
        //qDebug() << "model.paa" << model.paa;

    } while ((fabs(model.etau - etau_prev) > etau_delta_max) ||
             (fabs(model.paa - paa_prev) > paa_delta_max)  ||
             (fabs(model.paa_dec - paa_dec_prev) > paa_dec_delta_max)  ||
             (fabs(model.ecp_del - ecp_del_prev) > ecp_del_delta_max)  ||
             (fabs(model.ecp_dec - ecp_dec_prev) > ecp_dec_delta_max)
            );

    //qDebug() << iteration << "iterations";
    qDebug() <<"BEST eCP(4.3) " << "paa" << model.paa  << "ecp" << model.ecp << "etau" << model.etau << "paa_dec" << model.paa_dec << "ecp_del" << model.ecp_del  << "ecp_dec" << model.ecp_dec ;

    return model;
}

QwtPlotCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_4_3(Model_eCP model)
{
    //qDebug() <<"getPlotCurveForExtendedCP_4_3()";
    //qDebug() <<"Model eCP(4.3) " << "paa" << model.paa  << "ecp" << model.ecp << "etau" << model.etau << "paa_dec" << model.paa_dec << "ecp_del" << model.ecp_del  << "ecp_dec" << model.ecp_dec ;

    const int extendedCurve2_points = 1000;

    QVector<double> extended_cp_curve2_power(extendedCurve2_points);
    QVector<double> extended_cp_curve2_time(extendedCurve2_points);
    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.paa*(2.0-exp(-1*t))*exp(model.paa_dec*(t)) + model.ecp * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 + model.etau/(t));
    }

    QwtPlotCurve *extendedCPCurve2 = new QwtPlotCurve("eCP2");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen e2pen(Qt::yellow);
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);
    extendedCPCurve2->setSamples(extended_cp_curve2_time.data(), extended_cp_curve2_power.data(), extendedCurve2_points);

    return extendedCPCurve2;
}

QwtPlotCurve*
ExtendedCriticalPower::getPlotLevelForExtendedCP_4_3(Model_eCP model)
{
    const int extendedCurve2_points = 20;

    QVector<double> extended_cp_curve2_power(4*extendedCurve2_points);
    QVector<double> extended_cp_curve2_time(4*extendedCurve2_points);

    double tmin = 1.0/60;
    double tmax = 8.0/60;
    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.paa*(2.0-exp(-1*t))*exp(model.paa_dec*(t)) + model.ecp * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 + model.etau/(t));
    }

    tmin = 20.0/60;
    tmax = 40.0/60;
    for (int i = 1*extendedCurve2_points; i < 2*extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.paa*(2.0-exp(-1*t))*exp(model.paa_dec*(t)) + model.ecp * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 + model.etau/(t));
    }

    tmin = 2.0;
    tmax = 5.0;
    for (int i = 2*extendedCurve2_points; i < 3*extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.paa*(2.0-exp(-1*t))*exp(model.paa_dec*(t)) + model.ecp * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 + model.etau/(t));
    }

    tmin = 10.0;
    tmax = 45.0;
    for (int i = 3*extendedCurve2_points; i < 4*extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.paa*(2.0-exp(-1*t))*exp(model.paa_dec*(t)) + model.ecp * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 + model.etau/(t));
    }

    QwtPlotCurve *extendedCPCurve2 = new QwtPlotCurve("eCP2");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen e2pen(GColor(CCP));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::NoPen);
    extendedCPCurve2->setPen(e2pen);

    QwtSymbol *sym = new QwtSymbol;
    sym->setStyle(QwtSymbol::HLine);
    sym->setSize(2);
    sym->setPen(QPen(Qt::black));
    sym->setBrush(QBrush(Qt::NoBrush));
    extendedCPCurve2->setSymbol(sym);

    extendedCPCurve2->setSamples(extended_cp_curve2_time.data(), extended_cp_curve2_power.data(), 4*extendedCurve2_points);

    return extendedCPCurve2;
}

QwtPlotMarker*
ExtendedCriticalPower::getPlotMarkerForExtendedCP(Model_eCP model)
{
    QwtPlotMarker* extendedCurveTitle2 = new QwtPlotMarker();
    QString extendedCurve2_title;

    extendedCurve2_title.sprintf("CP=%.0f W, MMP60=%.0d W, Pmax=%.0d W, W'=%.0f kJ (%s)", model.ecp, model.mmp60, model.pMax, model.etau*model.ecp* 60.0 / 1000.0, model.version.toLatin1().constData());
    QwtText text(extendedCurve2_title, QwtText::PlainText);
    text.setColor(GColor(CPLOTMARKER));
    extendedCurveTitle2->setLabel(text);

    return extendedCurveTitle2;
}

QwtPlotCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_4_3_WSecond(Model_eCP model)
{
    const int extendedCurve2_points = 1000;

    QVector<double> extended_cp_curve2_power(extendedCurve2_points);
    QVector<double> extended_cp_curve2_time(extendedCurve2_points);
    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.paa*(2.0-exp(-1*t))*exp(model.paa_dec*(t));
    }

    QwtPlotCurve *extendedCPCurve2 = new QwtPlotCurve("eCP_4_3_WSecond");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen e2pen(GColor(CCADENCE));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);
    extendedCPCurve2->setSamples(extended_cp_curve2_time.data(), extended_cp_curve2_power.data(), extendedCurve2_points);

    return extendedCPCurve2;
}

QwtPlotCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_4_3_WPrime(Model_eCP model)
{
    const int extendedCurve2_points = 1000;

    QVector<double> extended_cp_curve2_power(extendedCurve2_points);
    QVector<double> extended_cp_curve2_time(extendedCurve2_points);
    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.ecp * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( model.etau/(t) );
    }

    QwtPlotCurve *extendedCPCurve2 = new QwtPlotCurve("eCP_4_3_WPrime");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen e2pen(GColor(CPOWER));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);
    extendedCPCurve2->setSamples(extended_cp_curve2_time.data(), extended_cp_curve2_power.data(), extendedCurve2_points);

    return extendedCPCurve2;
}

QwtPlotCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_4_3_CP(Model_eCP model)
{
    const int extendedCurve2_points = 1000;

    QVector<double> extended_cp_curve2_power(extendedCurve2_points);
    QVector<double> extended_cp_curve2_time(extendedCurve2_points);
    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.ecp * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 );
    }

    QwtPlotCurve *extendedCPCurve2 = new QwtPlotCurve("eCP_4_3_CP");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen e2pen(GColor(CHEARTRATE));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);
    extendedCPCurve2->setSamples(extended_cp_curve2_time.data(), extended_cp_curve2_power.data(), extendedCurve2_points);

    return extendedCPCurve2;
}



QwtPlotCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_4_3_WPrime_CP(Model_eCP model)
{
    const int extendedCurve2_points = 1000;

    QVector<double> extended_cp_curve2_power(extendedCurve2_points);
    QVector<double> extended_cp_curve2_time(extendedCurve2_points);
    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.ecp * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 + model.etau/(t));
    }

    QwtPlotCurve *extendedCPCurve2 = new QwtPlotCurve("eCP_4_3_WPrime_CP");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen e2pen(GColor(CCP));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);
    extendedCPCurve2->setSamples(extended_cp_curve2_time.data(), extended_cp_curve2_power.data(), extendedCurve2_points);

    return extendedCPCurve2;
}

Model_eCP
ExtendedCriticalPower::deriveExtendedCP_5_3_Parameters(bool usebest, RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2)
{
    Model_eCP model;
    model.version = "eCP v5.3";

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
    for (i1 = 0; i1 < 60 * t1; i1++)
        if (i1 + 1 >= bests->meanMaxArray(series).size())
            return model;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i2 = i1; i2 + 1 <= 60 * t2; i2++)
        if (i2 + 1 >= bests->meanMaxArray(series).size())
            return model;

    // the third point is the beginning of the minimum duration for aerobic efforts
    for (i3 = i2; i3 < 60 * t3; i3++)
        if (i3 + 1 >= bests->meanMaxArray(series).size())
            return model;
    for (i4 = i3; i4 + 1 <= 60 * t4; i4++)
        if (i4 + 1 >= bests->meanMaxArray(series).size())
            break;

    // the fifth point is the beginning of the minimum duration for aerobic efforts
    for (i5 = i4; i5 < 60 * t5; i5++)
        if (i5 + 1 >= bests->meanMaxArray(series).size())
            return model;
    for (i6 = i5; i6 + 1 <= 60 * t6; i6++)
        if (i6 + 1 >= bests->meanMaxArray(series).size())
            break;

    // the first point must be at least the minimum for the anaerobic interval, or quit
    for (i7 = i6; i7 < 60 * t7; i7++)
        if (i7 + 1 >= bests->meanMaxArray(series).size())
            return model;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i8 = i7; i8 + 1 <= 60 * t8; i8++)
        if (i8 + 1 >= bests->meanMaxArray(series).size())
            break;

    bool withoutEd = false;

    // initial estimate of tau
    if (model.paa == 0)
        model.paa = 300;

    if (model.etau == 0)
        model.etau = 1;

    if (model.ecp == 0)
        model.ecp = 300;

    if (model.paa_dec == 0)
        model.paa_dec = -2;

    if (model.ecp_del == 0)
        model.ecp_del = -0.9;

    if (model.tau_del == 0)
        model.tau_del = -4.8;

    if (model.ecp_dec == 0)
        model.ecp_dec = -1;

    if (model.ecp_dec_del == 0)
        model.ecp_dec_del = -180;

    // lower bound on tau
    const double paa_min = 100;
    //const double paa_max = 2000;
    const double etau_min = 0.5;
    const double paa_dec_max = -0.25;
    const double paa_dec_min = -3;
    const double ecp_dec_min = -5; // Long

    // convergence delta for tau
    //const double ecp_delta_max = 1e-4;
    const double etau_delta_max = 1e-4;
    const double paa_delta_max = 1e-2;
    const double paa_dec_delta_max = 1e-4;
    const double ecp_del_delta_max = 1e-4;
    const double ecp_dec_delta_max  = 1e-8;

    // previous loop value of tau and t0
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
        if (iteration ++ > max_loops) {
            qDebug()<<"Maximum number of loops exceeded in ecp2 model";
            break;
        }

        // record the previous version of tau, for convergence
        etau_prev = model.etau;
        paa_prev = model.paa;
        paa_dec_prev = model.paa_dec;
        ecp_del_prev = model.ecp_del;
        ecp_dec_prev = model.ecp_dec;

        if (withoutEd)
            model.ecp_dec = 0;

        // P = paa* (1.20-0.20*exp(-1*(x/60.0)))*exp(paa_dec*(x/60)) + ecp * (1-exp(tau_del*x/60)) * (1-exp(ecp_del*x/60)) * (1+ecp_dec*exp(-180/x/60) (1 + etau/(x/60))
        // bests->meanMaxArray(series)[i] = paa*(1.20-0.20*exp(-1*(i/60.0)))*exp(paa_dec*(i/60.0))  + ecp * (1-exp(tau_del*x/60)) * (1-exp(ecp*i/60.0)) * (1+2d*exp(180/i/60.0)) * ( 1 + etau/(i/60.0));

        // estimate ecp
        int i;

        model.ecp = 0;
        double _avg_ecp = 0.0;
        int count=1;
        for (i = i5; i <= i6; i++) {
            double ecpn = (bests->meanMaxArray(series)[i] - model.paa * (1.20-0.20*exp(-1*(i/60.0))) * exp(model.paa_dec*(i/60.0))) / (1-exp(model.tau_del*i/60.0)) / (1-exp(model.ecp_del*i/60.0)) / (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) / ( 1 + model.etau/(i/60.0));
            _avg_ecp = (double)((count-1)*_avg_ecp+ecpn)/count;

            if (model.ecp < ecpn)
                model.ecp = ecpn;

            count++;
        }
        //qDebug() << "estimate ecp" << model.ecp;
        if (!usebest) {
            model.ecp = _avg_ecp;
        }

        // if ecp = 0; no valid data; give up
        if (model.ecp == 0.0)
            return model;

        // estimate etau
        model.etau = etau_min;
        double _avg_etau = 0.0;
        count=1;
        for (i = i3; i <= i4; i++) {
            double etaun = ((bests->meanMaxArray(series)[i] - model.paa * (1.20-0.20*exp(-1*(i/60.0))) * exp(model.paa_dec*(i/60.0))) /model. ecp / (1-exp(model.tau_del*i/60.0)) / (1-exp(model.ecp_del*i/60.0)) / (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) - 1) * (i/60.0);
            _avg_etau = (double)((count-1)*_avg_etau+etaun)/count;

            if (model.etau < etaun)
                model.etau = etaun;
            count++;
        }
        //qDebug() << "estimate etau" << model.etau;
        if (!usebest) {
            model.etau = _avg_etau;
        }



        model.paa_dec = paa_dec_min;
        double _avg_paa_dec = 0.0;
        count=1;
        for (i = i1; i <= i2; i++) {
            double paa_decn = log((bests->meanMaxArray(series)[i] - model.ecp * (1-exp(model.tau_del*i/60.0)) * (1-exp(model.ecp_del*i/60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) * ( 1 + model.etau/(i/60.0)) ) / model.paa / (1.20-0.20*exp(-1*(i/60.0))) ) / (i/60.0);
            _avg_paa_dec = (double)((count-1)*_avg_paa_dec+paa_decn)/count;

            if (model.paa_dec < paa_decn && paa_decn < paa_dec_max) {
                model.paa_dec = paa_decn;
            }
            count++;
        }
        //qDebug() << "estimate paa_dec" << model.paa_dec;
        if (!usebest) {
            model.paa_dec = _avg_paa_dec;
        }

        model.paa = paa_min;
        double _avg_paa = 0.0;
        count=1;
        for (i = 2; i <= 8; i++) {
            double paan = (bests->meanMaxArray(series)[i] - model.ecp * (1-exp(model.tau_del*i/60.0)) * (1-exp(model.ecp_del*i/60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) * ( 1 + model.etau/(i/60.0))) / exp(model.paa_dec*(i/60.0)) / (1.20-0.20*exp(-1*(i/60.0)));
            _avg_paa = (double)((count-1)*_avg_paa+paan)/count;

            //qDebug() << "   estimate paan" << paan;
            if (model.paa < paan)
                model.paa = paan;
            count++;
        }
        //qDebug() << "estimate paa" << model.paa;
        if (!usebest  || _avg_paa<0.95*model.paa) {
            model.paa = _avg_paa;
        }

        if (!withoutEd) {
            model.ecp_dec = ecp_dec_min;
            double _avg_ecp_dec = 0.0;
            count=1;
            for (i = i7; i <= i8; i=i+120) {
                double ecp_decn = ((bests->meanMaxArray(series)[i] - model.paa * (1.20-0.20*exp(-1*(i/60.0))) * exp(model.paa_dec*(i/60.0))) / model.ecp / (1-exp(model.tau_del*i/60.0)) / (1-exp(model.ecp_del*i/60.0)) / ( 1 + model.etau/(i/60.0)) -1 ) / exp(model.ecp_dec_del/(i / 60.0));
                _avg_ecp_dec = (double)((count-1)*_avg_ecp_dec+ecp_decn)/count;

                if (ecp_decn > 0)
                    ecp_decn = 0;

                if (model.ecp_dec < ecp_decn)
                    model.ecp_dec = ecp_decn;
                count++;
            }
            //qDebug() << "estimate ecp_dec" << model.ecp_dec;
            if (!usebest) {
                model.ecp_dec = _avg_ecp_dec;
            }

        }
    } while ((fabs(model.etau - etau_prev) > etau_delta_max) ||
             (fabs(model.paa - paa_prev) > paa_delta_max)  ||
             (fabs(model.paa_dec - paa_dec_prev) > paa_dec_delta_max)  ||
             (fabs(model.ecp_del - ecp_del_prev) > ecp_del_delta_max)  ||
             (fabs(model.ecp_dec - ecp_dec_prev) > ecp_dec_delta_max)
            );

    //qDebug() << iteration << "iterations";

    model.pMax = model.paa*(1.20-0.20*exp(-1*(1/60.0)))*exp(model.paa_dec*(1/60.0)) + model.ecp * (1-exp(model.tau_del*(1/60.0))) * (1-exp(model.ecp_del*(1/60.0))) * (1+model.ecp_dec*exp(model.ecp_dec_del/(1/60.0))) * ( 1 + model.etau/(1/60.0));
    model.mmp60 = model.paa*(1.20-0.20*exp(-1*60.0))*exp(model.paa_dec*(60.0)) + model.ecp * (1-exp(model.tau_del*(60.0))) * (1-exp(model.ecp_del*60.0)) * (1+model.ecp_dec*exp(model.ecp_dec_del/60.0)) * ( 1 + model.etau/(60.0));

    qDebug() <<"eCP(5.3) " << "paa" << model.paa  << "ecp" << model.ecp << "etau" << model.etau << "paa_dec" << model.paa_dec << "ecp_del" << model.ecp_del << "ecp_dec" << model.ecp_dec << "ecp_dec_del" << model.ecp_dec_del;
    qDebug() <<"eCP(5.3) " << "pmax" << model.pMax << "mmp60" << model.mmp60;

    return model;
}

Model_eCP
ExtendedCriticalPower::deriveExtendedCP_5_3_ParametersForBest(double best5s, double best1min, double best5min, double best1hour)
{
    Model_eCP model;
    model.version = "eCP v5.3";

    // initial estimate of tau
    model.paa = 300;
    model.paa_dec = -2;
    model.etau = 1;
    model.tau_del = -4.8;
    model.ecp = 300;
    model.ecp_del = -0.9;
    model.ecp_dec = 0;
    model.ecp_dec_del = -180;

    // convergence delta for tau
    const double etau_delta_max = 1e-4;
    const double paa_delta_max = 1e-2;
    const double paa_dec_delta_max = 1e-4;

    // previous loop value of tau and t0
    double etau_prev;
    double paa_prev;
    double paa_dec_prev;

    // maximum number of loops
    const int max_loops = 100;

    // loop to convergence
    int iteration = 0;
    do {
        if (iteration ++ > max_loops) {
            qDebug()<<"Maximum number of loops exceeded in ecp2 model";
            break;
        }

        // record the previous version of tau, for convergence
        etau_prev = model.etau;
        paa_prev = model.paa;
        paa_dec_prev = model.paa_dec;

        // P = paa* (1.20-0.20*exp(-1*(x/60.0)))*exp(paa_dec*(x/60)) + ecp * (1-exp(tau_del*x/60)) * (1-exp(ecp_del*x/60)) * (1+ecp_dec*exp(-180/x/60) (1 + etau/(x/60))
        // bests->meanMaxArray(series)[i] = paa*(1.20-0.20*exp(-1*(i/60.0)))*exp(paa_dec*(i/60.0))  + ecp * (1-exp(tau_del*x/60)) * (1-exp(ecp*i/60.0)) * (1+2d*exp(180/i/60.0)) * ( 1 + etau/(i/60.0));

        // estimate ecp
        model.ecp = (best1hour - model.paa * (1.20-0.20*exp(-1*(3600/60.0))) * exp(model.paa_dec*(3600/60.0))) / (1-exp(model.tau_del*(3600/60.0))) / (1-exp(model.ecp_del*(3600/60.0))) / (1+model.ecp_dec*exp(model.ecp_dec_del/(3600/60.0))) / ( 1 + model.etau/(3600/60.0));
        //qDebug() << "model.ecp" << model.ecp;

        // estimate etau
        model.etau = ((best5min - model.paa * (1.20-0.20*exp(-1*(300/60.0))) * exp(model.paa_dec*(300/60.0))) /model.ecp / (1-exp(model.tau_del*(300/60.0))) / (1-exp(model.ecp_del*(300/60.0))) / (1+model.ecp_dec*exp(model.ecp_dec_del/(300/60.0))) - 1) * (300/60.0);
        //qDebug() << "model.etau" << model.etau;

        // estimate paa_dec
        model.paa_dec = log((best1min - model.ecp * (1-exp(model.tau_del*(60/60.0))) * (1-exp(model.ecp_del*(60/60.0))) * (1+model.ecp_dec*exp(model.ecp_dec_del/(60/60.0))) * ( 1 + model.etau/(60/60.0)) ) / model.paa / (1.20-0.20*exp(-1*(60/60.0))) ) / (60/60.0);
        //qDebug() << "paa_dec.etau" << model.paa_dec;

        // estimate paa
        model.paa = (best5s - model.ecp * (1-exp(model.tau_del*(5/60.0))) * (1-exp(model.ecp_del*(5/60.0))) * (1+model.ecp_dec*exp(model.ecp_dec_del/(5/60.0))) * ( 1 + model.etau/(5/60.0))) / exp(model.paa_dec*(5/60.0)) / (1.20-0.20*exp(-1*(5/60.0)));
        //qDebug() << "model.paa" << model.paa;

    } while ((fabs(model.etau - etau_prev) > etau_delta_max) ||
             (fabs(model.paa - paa_prev) > paa_delta_max)  ||
             (fabs(model.paa_dec - paa_dec_prev) > paa_dec_delta_max)
            );

    qDebug() <<"BEST eCP(5.3) " << "paa" << model.paa  << "ecp" << model.ecp << "etau" << model.etau << "paa_dec" << model.paa_dec << "ecp_del" << model.ecp_del << "ecp_dec" << model.ecp_dec << "ecp_dec_del" << model.ecp_dec_del;

    return model;
}

QwtPlotCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_5_3(Model_eCP model)
{
    const int extendedCurve2_points = 1000;

    QVector<double> extended_cp_curve2_power(extendedCurve2_points);
    QVector<double> extended_cp_curve2_time(extendedCurve2_points);
    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.paa*(1.20-0.20*exp(-1*t))*exp(model.paa_dec*(t)) + model.ecp * (1-exp(model.tau_del*t)) * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 + model.etau/(t));
    }

    QwtPlotCurve *extendedCPCurve2 = new QwtPlotCurve("eCP_5_3");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen e2pen(GColor(CCP)); // Qt::cyan
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);
    extendedCPCurve2->setSamples(extended_cp_curve2_time.data(), extended_cp_curve2_power.data(), extendedCurve2_points);

    return extendedCPCurve2;
}

QwtPlotCurve*
ExtendedCriticalPower::getPlotLevelForExtendedCP_5_3(Model_eCP model)
{
    const int extendedCurve2_points = 1000;

    QVector<double> extended_cp_curve2_power(extendedCurve2_points);
    QVector<double> extended_cp_curve2_time(extendedCurve2_points);
    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.paa*(1.20-0.20*exp(-1*t))*exp(model.paa_dec*(t)) + model.ecp * (1-exp(model.tau_del*t)) * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 + model.etau/(t));
    }

    QwtPlotCurve *extendedCPCurve2 = new QwtPlotCurve("level_eCP_5_3");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen e2pen(GColor(Qt::lightGray)); // Qt::cyan
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::SolidLine);
    extendedCPCurve2->setPen(e2pen);
    extendedCPCurve2->setSamples(extended_cp_curve2_time.data(), extended_cp_curve2_power.data(), extendedCurve2_points);

    return extendedCPCurve2;
}

QwtPlotIntervalCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_5_3_WSecond(Model_eCP model, bool)
{
    const int extendedCurve2_points = 1000;

    QVector<QwtIntervalSample> extended_cp_curve_power(extendedCurve2_points);

    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);

        double power_wsecond = model.paa*(1.20-0.20*exp(-1*t))*exp(model.paa_dec*(t));

        extended_cp_curve_power[i] = QwtIntervalSample(t, 0, power_wsecond);
    }

    QwtPlotIntervalCurve *extendedCPCurve = new QwtPlotIntervalCurve("eCP_5_3_WSecond");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

    QPen e2pen(GColor(CCADENCE));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve->setPen(e2pen);

    QColor color1 = GColor(CCADENCE);
    color1.setAlpha(64);
    QColor color2 = color1.darker();
    QLinearGradient linearGradient(0, 0, 0, model.paa);
    linearGradient.setColorAt(0.0, color1);
    linearGradient.setColorAt(1.0, color2);
    linearGradient.setSpread(QGradient::PadSpread);
    extendedCPCurve->setBrush(linearGradient);   // fill below the line

    extendedCPCurve->setSamples(new QwtIntervalSeriesData(extended_cp_curve_power));

    return extendedCPCurve;
}

QwtPlotIntervalCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_5_3_WPrime(Model_eCP model, bool stacked)
{
    const int extendedCurve2_points = 1000;

    QVector<QwtIntervalSample> extended_cp_curve_power(extendedCurve2_points);

    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);

        double power_wsecond = model.paa*(1.20-0.20*exp(-1*t))*exp(model.paa_dec*(t));
        double power_wprime = model.ecp * (1-exp(model.tau_del*t)) * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( model.etau/(t) );

        extended_cp_curve_power[i] = QwtIntervalSample(t, (stacked?power_wsecond:0), (stacked?power_wprime + power_wsecond:power_wprime));
    }

    QwtPlotIntervalCurve *extendedCPCurve = new QwtPlotIntervalCurve("eCP_5_3_WPrime");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

    QPen e2pen(GColor(CPOWER));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve->setPen(e2pen);

    QColor color1 = GColor(CPOWER);
    color1.setAlpha(64);
    QColor color2 = color1.darker();
    QLinearGradient linearGradient(0, 0, 0, model.ecp);
    linearGradient.setColorAt(0.0, color1);
    linearGradient.setColorAt(1.0, color2);
    linearGradient.setSpread(QGradient::PadSpread);
    extendedCPCurve->setBrush(linearGradient);   // fill below the line

    extendedCPCurve->setSamples(new QwtIntervalSeriesData(extended_cp_curve_power));


    return extendedCPCurve;
}

QwtPlotIntervalCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_5_3_CP(Model_eCP model, bool stacked)
{
    const int extendedCurve2_points = 1000;

    QVector<QwtIntervalSample> extended_cp_curve_power(extendedCurve2_points);

    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);

        double power_wsecond = model.paa*(1.20-0.20*exp(-1*t))*exp(model.paa_dec*(t));
        double power_wprime = model.ecp * (1-exp(model.tau_del*t)) * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( model.etau/(t) );
        double power_cp = model.ecp * (1-exp(model.tau_del*t)) * (1-exp(model.ecp_del*t)) * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 );

        extended_cp_curve_power[i] = QwtIntervalSample(t, (stacked?power_wprime + power_wsecond:0), (stacked?power_wprime + power_wsecond + power_cp:power_cp));
    }

    QwtPlotIntervalCurve *extendedCPCurve = new QwtPlotIntervalCurve("eCP_5_3_CP");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

    QPen e2pen(GColor(CHEARTRATE));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve->setPen(e2pen);

    QColor color1 = GColor(CHEARTRATE);
    color1.setAlpha(64);
    QColor color2 = color1.darker();
    QLinearGradient linearGradient(0, 0, 0, 1.1*model.ecp);
    linearGradient.setColorAt(0.0, color1);
    linearGradient.setColorAt(1.0, color2);
    linearGradient.setSpread(QGradient::PadSpread);
    extendedCPCurve->setBrush(linearGradient);   // fill below the line

    extendedCPCurve->setSamples(new QwtIntervalSeriesData(extended_cp_curve_power));

    return extendedCPCurve;
}



/**************************************************
 * Version 6
 *
 **************************************************/
Model_eCP
ExtendedCriticalPower::deriveExtendedCP_6_3_Parameters(bool usebest, RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2)
{
    Model_eCP model;
    model.version = "eCP v6.3";

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
    for (i1 = 0; i1 < 60 * t1; i1++)
        if (i1 + 1 >= bests->meanMaxArray(series).size())
            return model;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i2 = i1; i2 + 1 <= 60 * t2; i2++)
        if (i2 + 1 >= bests->meanMaxArray(series).size())
            return model;

    // the third point is the beginning of the minimum duration for aerobic efforts
    for (i3 = i2; i3 < 60 * t3; i3++)
        if (i3 + 1 >= bests->meanMaxArray(series).size())
            return model;
    for (i4 = i3; i4 + 1 <= 60 * t4; i4++)
        if (i4 + 1 >= bests->meanMaxArray(series).size())
            break;

    // the fifth point is the beginning of the minimum duration for aerobic efforts
    for (i5 = i4; i5 < 60 * t5; i5++)
        if (i5 + 1 >= bests->meanMaxArray(series).size())
            return model;
    for (i6 = i5; i6 + 1 <= 60 * t6; i6++)
        if (i6 + 1 >= bests->meanMaxArray(series).size())
            break;

    // the first point must be at least the minimum for the anaerobic interval, or quit
    for (i7 = i6; i7 < 60 * t7; i7++)
        if (i7 + 1 >= bests->meanMaxArray(series).size())
            return model;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i8 = i7; i8 + 1 <= 60 * t8; i8++)
        if (i8 + 1 >= bests->meanMaxArray(series).size())
            break;

    bool withoutEd = false;

    // initial estimate of tau
    if (model.paa == 0)
        model.paa = 300;

    if (model.etau == 0)
        model.etau = 1;

    if (model.ecp == 0)
        model.ecp = 300;

    if (model.paa_dec == 0)
        model.paa_dec = -2;

    if (model.ecp_del == 0)
        model.ecp_del = -0.6;

    if (model.tau_del == 0)
        model.tau_del = -1.2;

    if (model.ecp_dec == 0)
        model.ecp_dec = -1;

    if (model.ecp_dec_del == 0)
        model.ecp_dec_del = -180;

    // lower bound on tau
    const double paa_min = 100;
    //const double paa_max = 2000;
    const double etau_min = 0.5;
    const double paa_dec_max = -0.25;
    const double paa_dec_min = -5;
    const double ecp_dec_min = -5; // Long

    // convergence delta for tau
    //const double ecp_delta_max = 1e-4;
    const double etau_delta_max = 1e-4;
    const double paa_delta_max = 1e-2;
    const double paa_dec_delta_max = 1e-4;
    const double ecp_del_delta_max = 1e-4;
    const double ecp_dec_delta_max  = 1e-8;

    // previous loop value of tau and t0
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
        if (iteration ++ > max_loops) {
            qDebug()<<"Maximum number of loops exceeded in ecp2 model";
            break;
        }

        // record the previous version of tau, for convergence
        etau_prev = model.etau;
        paa_prev = model.paa;
        paa_dec_prev = model.paa_dec;
        ecp_del_prev = model.ecp_del;
        ecp_dec_prev = model.ecp_dec;

        if (withoutEd)
            model.ecp_dec = 0;

        // P = paa* (1.10-(1.10-1)*exp(-8*(x/60.0)))*exp(paa_dec*(x/60)) + ecp * (1+ecp_dec*exp(-180/x/60) (1 * (1-exp(ecp_del*x/60)) + (1-exp(tau_del*x/60))^2 * etau/(x/60))
        // bests->meanMaxArray(series)[i] = paa * (1.10-(1.10-1)*exp(-8*(i/60.0))) * exp(paa_dec*(i/60.0))  + ecp * (1+ecp_dec*exp(-180/i/60.0)) * ( 1 *  (1-exp(ecp*i/60.0))  + (1-exp(tau_del*x/60))^2 * etau/(i/60.0));

        // estimate ecp
        int i;

        model.ecp = 0;
        double _avg_ecp = 0.0;
        int count=1;
        for (i = i5; i <= i6; i++) {
            double ecpn = (bests->meanMaxArray(series)[i] - model.paa * (1.10-(1.10-1)*exp(-8*(i/60.0))) * exp(model.paa_dec*(i/60.0))) / (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) / ( 1 * (1-exp(model.ecp_del*i/60.0)) + pow((1-exp(model.tau_del*i/60.0)),2) * model.etau/(i/60.0));
            _avg_ecp = (double)((count-1)*_avg_ecp+ecpn)/count;

            if (model.ecp < ecpn)
                model.ecp = ecpn;

            count++;
        }
        //qDebug() << "estimate ecp" << model.ecp;
        if (!usebest) {
            model.ecp = _avg_ecp;
        }

        // if ecp = 0; no valid data; give up
        if (model.ecp == 0.0)
            return model;

        // estimate etau
        model.etau = etau_min;
        double _avg_etau = 0.0;
        count=1;
        for (i = i3; i <= i4; i++) {
            double etaun = ((bests->meanMaxArray(series)[i] - model.paa * (1.10-(1.10-1)*exp(-8*(i/60.0))) * exp(model.paa_dec*(i/60.0))) / model. ecp / (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) - 1 * (1-exp(model.ecp_del*i/60.0))) * (i/60.0) / pow((1-exp(model.tau_del*i/60.0)),2);
            _avg_etau = (double)((count-1)*_avg_etau+etaun)/count;

            if (model.etau < etaun)
                model.etau = etaun;
            count++;
        }
        //qDebug() << "estimate etau" << model.etau;
        if (!usebest) {
            model.etau = _avg_etau;
        }



        model.paa_dec = paa_dec_min;
        double _avg_paa_dec = 0.0;
        count=1;
        for (i = i1; i <= i2; i++) {
            double paa_decn = log((bests->meanMaxArray(series)[i] - model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) * ( 1 * (1-exp(model.ecp_del*i/60.0)) + pow((1-exp(model.tau_del*i/60.0)),2) * model.etau/(i/60.0)) ) / model.paa / (1.10-(1.10-1)*exp(-8*(i/60.0))) ) / (i/60.0);
            _avg_paa_dec = (double)((count-1)*_avg_paa_dec+paa_decn)/count;

            if (model.paa_dec < paa_decn && paa_decn < paa_dec_max) {
                model.paa_dec = paa_decn;
            }
            count++;
        }
        //qDebug() << "estimate paa_dec" << model.paa_dec;
        if (!usebest) {
            model.paa_dec = _avg_paa_dec;
        }

        model.paa = paa_min;
        double _avg_paa = 0.0;
        count=1;
        for (i = 2; i <= 8; i++) {
            double paan = (bests->meanMaxArray(series)[i] - model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) * ( 1 * (1-exp(model.ecp_del*i/60.0)) + pow((1-exp(model.tau_del*i/60.0)),2) * model.etau/(i/60.0))) / exp(model.paa_dec*(i/60.0)) / (1.10-(1.10-1)*exp(-8*(i/60.0)));
            _avg_paa = (double)((count-1)*_avg_paa+paan)/count;

            //qDebug() << "   estimate paan" << paan;
            if (model.paa < paan)
                model.paa = paan;
            count++;
        }
        //qDebug() << "estimate paa" << model.paa;
        if (!usebest  || _avg_paa<0.95*model.paa) {
            model.paa = _avg_paa;
        }

        if (!withoutEd) {
            model.ecp_dec = ecp_dec_min;
            double _avg_ecp_dec = 0.0;
            count=1;
            for (i = i7; i <= i8; i=i+120) {
                double ecp_decn = ((bests->meanMaxArray(series)[i] - model.paa * (1.10-(1.10-1)*exp(-8*(i/60.0))) * exp(model.paa_dec*(i/60.0))) / model.ecp / ( 1 * (1-exp(model.ecp_del*i/60.0)) + pow((1-exp(model.tau_del*i/60.0)),2) * model.etau/(i/60.0)) -1 ) / exp(model.ecp_dec_del/(i / 60.0));
                _avg_ecp_dec = (double)((count-1)*_avg_ecp_dec+ecp_decn)/count;

                if (ecp_decn > 0)
                    ecp_decn = 0;

                if (model.ecp_dec < ecp_decn)
                    model.ecp_dec = ecp_decn;
                count++;
            }
            //qDebug() << "estimate ecp_dec" << model.ecp_dec;
            if (!usebest) {
                model.ecp_dec = _avg_ecp_dec;
            }

        }
    } while ((fabs(model.etau - etau_prev) > etau_delta_max) ||
             (fabs(model.paa - paa_prev) > paa_delta_max)  ||
             (fabs(model.paa_dec - paa_dec_prev) > paa_dec_delta_max)  ||
             (fabs(model.ecp_del - ecp_del_prev) > ecp_del_delta_max)  ||
             (fabs(model.ecp_dec - ecp_dec_prev) > ecp_dec_delta_max)
            );

    //qDebug() << iteration << "iterations";

    model.pMax = model.paa*(1.10-(1.10-1)*exp(-8*(1/60.0)))*exp(model.paa_dec*(1/60.0)) + model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/(1/60.0))) * ( 1 * (1-exp(model.ecp_del*(1/60.0))) + pow((1-exp(model.tau_del*(1/60.0))),2) * model.etau/(1/60.0));
    model.mmp60 = model.paa*(1.10-(1.10-1)*exp(-8*60.0))*exp(model.paa_dec*(60.0)) + model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/60.0)) * ( 1 * (1-exp(model.ecp_del*60.0)) + pow((1-exp(model.tau_del*(60.0))),2) * model.etau/(60.0));

    qDebug() <<"eCP(6.3) " << "paa" << model.paa  << "ecp" << model.ecp << "etau" << model.etau << "paa_dec" << model.paa_dec << "ecp_del" << model.ecp_del << "ecp_dec" << model.ecp_dec << "ecp_dec_del" << model.ecp_dec_del;
    qDebug() <<"eCP(6.3) " << "pmax" << model.pMax << "mmp60" << model.mmp60;

    return model;
}

QwtPlotCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_6_3(Model_eCP model)
{
    const int extendedCurve2_points = 1000;

    QVector<double> extended_cp_curve2_power(extendedCurve2_points);
    QVector<double> extended_cp_curve2_time(extendedCurve2_points);
    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        extended_cp_curve2_time[i] = t;
        extended_cp_curve2_power[i] = model.paa*(1.10-(1.10-1)*exp(-8*t)) * exp(model.paa_dec*(t)) + model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 * (1-exp(model.ecp_del*t)) + pow( (1-exp(model.tau_del*t)),2) * model.etau/(t));
    }

    QwtPlotCurve *extendedCPCurve2 = new QwtPlotCurve("eCP_6_3");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen e2pen(Qt::cyan);
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);
    extendedCPCurve2->setSamples(extended_cp_curve2_time.data(), extended_cp_curve2_power.data(), extendedCurve2_points);

    return extendedCPCurve2;
}

QwtPlotIntervalCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_6_3_WSecond(Model_eCP model, bool)
{
    const int extendedCurve2_points = 1000;

    QVector<QwtIntervalSample> extended_cp_curve(extendedCurve2_points);
    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);

        double power_wsecond = model.paa*(1.10-(1.10-1)*exp(-8*t))*exp(model.paa_dec*(t));
        extended_cp_curve[i] = QwtIntervalSample(t, 0, power_wsecond);
    }

    QwtPlotIntervalCurve *extendedCPCurve2 = new QwtPlotIntervalCurve("eCP_6_3_WSecond");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);

    QPen e2pen(GColor(CCADENCE));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);

    QColor color1 = GColor(CCADENCE);
    color1.setAlpha(64);
    QColor color2 = color1.darker();
    QLinearGradient linearGradient(0, 0, 0, 100);
    linearGradient.setColorAt(0.0, color1);
    linearGradient.setColorAt(1.0, color2);
    linearGradient.setSpread(QGradient::PadSpread);
    extendedCPCurve2->setBrush(linearGradient);   // fill below the line

    extendedCPCurve2->setSamples(new QwtIntervalSeriesData(extended_cp_curve));

    return extendedCPCurve2;
}

QwtPlotIntervalCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_6_3_WPrime(Model_eCP model, bool stacked)
{
    const int extendedCurve2_points = 1000;

    QVector<QwtIntervalSample> extended_cp_curve(extendedCurve2_points);

    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);

        double power_wsecond = model.paa*(1.10-(1.10-1)*exp(-8*t))*exp(model.paa_dec*(t));
        double power_wprime = model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( pow((1-exp(model.tau_del*t)),2) * model.etau/(t) );
        extended_cp_curve[i] = QwtIntervalSample(t, (stacked?power_wsecond:0), (stacked?power_wsecond:0) + power_wprime);
    }

    QwtPlotIntervalCurve *extendedCPCurve2 = new QwtPlotIntervalCurve("eCP_6_3_WPrime");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);

    QPen e2pen(GColor(CPOWER));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);

    QColor color1 = GColor(CPOWER);
    color1.setAlpha(64);
    QColor color2 = color1.darker();
    QLinearGradient linearGradient(0, 0, 0, model.ecp);
    linearGradient.setColorAt(0.0, color1);
    linearGradient.setColorAt(1.0, color2);
    linearGradient.setSpread(QGradient::PadSpread);
    extendedCPCurve2->setBrush(linearGradient);   // fill below the line

    extendedCPCurve2->setSamples(new QwtIntervalSeriesData(extended_cp_curve));

    return extendedCPCurve2;
}

QwtPlotIntervalCurve*
ExtendedCriticalPower::getPlotCurveForExtendedCP_6_3_CP(Model_eCP model, bool stacked)
{
    const int extendedCurve2_points = 1000;

    QVector<QwtIntervalSample> extended_cp_curve(extendedCurve2_points);

    double tmin = 1.0/60;
    double tmax = 600;

    for (int i = 0; i < extendedCurve2_points; i ++) {
        double x = (double) i / (extendedCurve2_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);

        double power_second = model.paa*(1.10-(1.10-1)*exp(-8*t))*exp(model.paa_dec*(t));
        double power_wprime = model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( pow((1-exp(model.tau_del*t)),2) * model.etau/(t) );
        double power_wcp =  model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/t)) * ( 1 * (1-exp(model.ecp_del*t)) );
        extended_cp_curve[i] = QwtIntervalSample(t, (stacked?power_second+power_wprime:0), (stacked?power_second + power_wprime:0) + power_wcp);
    }

    QwtPlotIntervalCurve *extendedCPCurve2 = new QwtPlotIntervalCurve("eCP_6_3_CP");
    if (appsettings->value(NULL, GC_ANTIALIAS, true).toBool() == true)
        extendedCPCurve2->setRenderHint(QwtPlotItem::RenderAntialiased);

    QPen e2pen(GColor(CHEARTRATE));
    e2pen.setWidth(1);
    e2pen.setStyle(Qt::DashLine);
    extendedCPCurve2->setPen(e2pen);

    QColor color1 = GColor(CHEARTRATE);
    color1.setAlpha(64);
    QColor color2 = color1.darker();
    QLinearGradient linearGradient(0, 0, 0, 1.1*model.ecp);
    linearGradient.setColorAt(0.0, color1);
    linearGradient.setColorAt(1.0, color2);
    linearGradient.setSpread(QGradient::PadSpread);
    extendedCPCurve2->setBrush(linearGradient);   // fill below the line

    extendedCPCurve2->setSamples(new QwtIntervalSeriesData(extended_cp_curve));

    return extendedCPCurve2;
}

/***************************************************************
  Dan's version of 2 component veloclinic model

  P = P1 ( τ1 / t ) ( 1 - exp[-t / τ1] ) + P2 / ( 1 + t / α2 τ2 )α2

 ***************************************************************/
/*QVector<double> &
ExtendedCriticalPower::decimateData(QVector<double> data)
{
   QVector<double> result;

   for (int i=0;i<data.length();i++) {
       if (data.at(i))
   }
}*/

Model_eCP
ExtendedCriticalPower::deriveDanVeloclinicCP_Parameters(bool usebest, RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2)
{
    Model_eCP model;
    model.version = "2 components (Dan v1)";

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
    for (i1 = 0; i1 < 60 * t1; i1++)
        if (i1 + 1 >= bests->meanMaxArray(series).size())
            return model;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i2 = i1; i2 + 1 <= 60 * t2; i2++)
        if (i2 + 1 >= bests->meanMaxArray(series).size())
            return model;

    // the third point is the beginning of the minimum duration for aerobic efforts
    for (i3 = i2; i3 < 60 * t3; i3++)
        if (i3 + 1 >= bests->meanMaxArray(series).size())
            return model;
    for (i4 = i3; i4 + 1 <= 60 * t4; i4++)
        if (i4 + 1 >= bests->meanMaxArray(series).size())
            break;

    // the fifth point is the beginning of the minimum duration for aerobic efforts
    for (i5 = i4; i5 < 60 * t5; i5++)
        if (i5 + 1 >= bests->meanMaxArray(series).size())
            return model;
    for (i6 = i5; i6 + 1 <= 60 * t6; i6++)
        if (i6 + 1 >= bests->meanMaxArray(series).size())
            break;

    // the first point must be at least the minimum for the anaerobic interval, or quit
    for (i7 = i6; i7 < 60 * t7; i7++)
        if (i7 + 1 >= bests->meanMaxArray(series).size())
            return model;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i8 = i7; i8 + 1 <= 60 * t8; i8++)
        if (i8 + 1 >= bests->meanMaxArray(series).size())
            break;

    bool withoutEd = false;

    // initial estimate of tau
    if (model.paa == 0)
        model.paa = 300;

    if (model.etau == 0)
        model.etau = 1;

    if (model.ecp == 0)
        model.ecp = 300;

    if (model.paa_dec == 0)
        model.paa_dec = -2;

    if (model.ecp_del == 0)
        model.ecp_del = -0.6;

    if (model.tau_del == 0)
        model.tau_del = -1.2;

    if (model.ecp_dec == 0)
        model.ecp_dec = -1;

    if (model.ecp_dec_del == 0)
        model.ecp_dec_del = -180;

    // lower bound on tau
    const double paa_min = 100;
    //const double paa_max = 2000;
    const double etau_min = 0.5;
    const double paa_dec_max = -0.25;
    const double paa_dec_min = -5;
    const double ecp_dec_min = -5; // Long

    // convergence delta for tau
    //const double ecp_delta_max = 1e-4;
    const double etau_delta_max = 1e-4;
    const double paa_delta_max = 1e-2;
    const double paa_dec_delta_max = 1e-4;
    const double ecp_del_delta_max = 1e-4;
    const double ecp_dec_delta_max  = 1e-8;

    // previous loop value of tau and t0
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
        if (iteration ++ > max_loops) {
            qDebug()<<"Maximum number of loops exceeded in ecp2 model";
            break;
        }

        // record the previous version of tau, for convergence
        etau_prev = model.etau;
        paa_prev = model.paa;
        paa_dec_prev = model.paa_dec;
        ecp_del_prev = model.ecp_del;
        ecp_dec_prev = model.ecp_dec;

        if (withoutEd)
            model.ecp_dec = 0;

        // P = P1 ( τ1 / t ) ( 1 - exp[-t / τ1] ) + P2 / ( 1 + t / α2 τ2 )α2
        // P = P1 ( τ1 / t ) ( 1 - exp[-t / τ1] ) + P2 / ( 1 + t / α2 1200 )α2
        // bests->meanMaxArray(series)[i] = p1 * (t1 / (i/60.0)) * ( 1 - exp(-(i/60.0) / t1) ) + p2 / ( 1 + (i/60.0) / (a2 * t2))^a2;

        // estimate ecp
        int i;

        model.ecp = 0;
        double _avg_ecp = 0.0;
        int count=1;
        for (i = i5; i <= i6; i++) {
            double ecpn = (bests->meanMaxArray(series)[i] - model.paa * (1.10-(1.10-1)*exp(-8*(i/60.0))) * exp(model.paa_dec*(i/60.0))) / (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) / ( 1 * (1-exp(model.ecp_del*i/60.0)) + pow((1-exp(model.tau_del*i/60.0)),2) * model.etau/(i/60.0));
            _avg_ecp = (double)((count-1)*_avg_ecp+ecpn)/count;

            if (model.ecp < ecpn)
                model.ecp = ecpn;

            count++;
        }
        //qDebug() << "estimate ecp" << model.ecp;
        if (!usebest) {
            model.ecp = _avg_ecp;
        }

        // if ecp = 0; no valid data; give up
        if (model.ecp == 0.0)
            return model;

        // estimate etau
        model.etau = etau_min;
        double _avg_etau = 0.0;
        count=1;
        for (i = i3; i <= i4; i++) {
            double etaun = ((bests->meanMaxArray(series)[i] - model.paa * (1.10-(1.10-1)*exp(-8*(i/60.0))) * exp(model.paa_dec*(i/60.0))) / model. ecp / (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) - 1 * (1-exp(model.ecp_del*i/60.0))) * (i/60.0) / pow((1-exp(model.tau_del*i/60.0)),2);
            _avg_etau = (double)((count-1)*_avg_etau+etaun)/count;

            if (model.etau < etaun)
                model.etau = etaun;
            count++;
        }
        //qDebug() << "estimate etau" << model.etau;
        if (!usebest) {
            model.etau = _avg_etau;
        }



        model.paa_dec = paa_dec_min;
        double _avg_paa_dec = 0.0;
        count=1;
        for (i = i1; i <= i2; i++) {
            double paa_decn = log((bests->meanMaxArray(series)[i] - model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) * ( 1 * (1-exp(model.ecp_del*i/60.0)) + pow((1-exp(model.tau_del*i/60.0)),2) * model.etau/(i/60.0)) ) / model.paa / (1.10-(1.10-1)*exp(-8*(i/60.0))) ) / (i/60.0);
            _avg_paa_dec = (double)((count-1)*_avg_paa_dec+paa_decn)/count;

            if (model.paa_dec < paa_decn && paa_decn < paa_dec_max) {
                model.paa_dec = paa_decn;
            }
            count++;
        }
        //qDebug() << "estimate paa_dec" << model.paa_dec;
        if (!usebest) {
            model.paa_dec = _avg_paa_dec;
        }

        model.paa = paa_min;
        double _avg_paa = 0.0;
        count=1;
        for (i = 2; i <= 8; i++) {
            double paan = (bests->meanMaxArray(series)[i] - model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/(i/60.0))) * ( 1 * (1-exp(model.ecp_del*i/60.0)) + pow((1-exp(model.tau_del*i/60.0)),2) * model.etau/(i/60.0))) / exp(model.paa_dec*(i/60.0)) / (1.10-(1.10-1)*exp(-8*(i/60.0)));
            _avg_paa = (double)((count-1)*_avg_paa+paan)/count;

            //qDebug() << "   estimate paan" << paan;
            if (model.paa < paan)
                model.paa = paan;
            count++;
        }
        //qDebug() << "estimate paa" << model.paa;
        if (!usebest  || _avg_paa<0.95*model.paa) {
            model.paa = _avg_paa;
        }

        if (!withoutEd) {
            model.ecp_dec = ecp_dec_min;
            double _avg_ecp_dec = 0.0;
            count=1;
            for (i = i7; i <= i8; i=i+120) {
                double ecp_decn = ((bests->meanMaxArray(series)[i] - model.paa * (1.10-(1.10-1)*exp(-8*(i/60.0))) * exp(model.paa_dec*(i/60.0))) / model.ecp / ( 1 * (1-exp(model.ecp_del*i/60.0)) + pow((1-exp(model.tau_del*i/60.0)),2) * model.etau/(i/60.0)) -1 ) / exp(model.ecp_dec_del/(i / 60.0));
                _avg_ecp_dec = (double)((count-1)*_avg_ecp_dec+ecp_decn)/count;

                if (ecp_decn > 0)
                    ecp_decn = 0;

                if (model.ecp_dec < ecp_decn)
                    model.ecp_dec = ecp_decn;
                count++;
            }
            //qDebug() << "estimate ecp_dec" << model.ecp_dec;
            if (!usebest) {
                model.ecp_dec = _avg_ecp_dec;
            }

        }
    } while ((fabs(model.etau - etau_prev) > etau_delta_max) ||
             (fabs(model.paa - paa_prev) > paa_delta_max)  ||
             (fabs(model.paa_dec - paa_dec_prev) > paa_dec_delta_max)  ||
             (fabs(model.ecp_del - ecp_del_prev) > ecp_del_delta_max)  ||
             (fabs(model.ecp_dec - ecp_dec_prev) > ecp_dec_delta_max)
            );

    //qDebug() << iteration << "iterations";

    model.pMax = model.paa*(1.10-(1.10-1)*exp(-8*(1/60.0)))*exp(model.paa_dec*(1/60.0)) + model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/(1/60.0))) * ( 1 * (1-exp(model.ecp_del*(1/60.0))) + pow((1-exp(model.tau_del*(1/60.0))),2) * model.etau/(1/60.0));
    model.mmp60 = model.paa*(1.10-(1.10-1)*exp(-8*60.0))*exp(model.paa_dec*(60.0)) + model.ecp * (1+model.ecp_dec*exp(model.ecp_dec_del/60.0)) * ( 1 * (1-exp(model.ecp_del*60.0)) + pow((1-exp(model.tau_del*(60.0))),2) * model.etau/(60.0));

    qDebug() <<"eCP(6.3) " << "paa" << model.paa  << "ecp" << model.ecp << "etau" << model.etau << "paa_dec" << model.paa_dec << "ecp_del" << model.ecp_del << "ecp_dec" << model.ecp_dec << "ecp_dec_del" << model.ecp_dec_del;
    qDebug() <<"eCP(6.3) " << "pmax" << model.pMax << "mmp60" << model.mmp60;

    return model;
}
