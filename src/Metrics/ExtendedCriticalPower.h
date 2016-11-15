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

#ifndef _gc_extendedcriticalpower_include
#define _gc_extendedcriticalpower_include

#include "RideFile.h"
#include "Context.h"
#include "Athlete.h"
#include "Zones.h"
#include "RideMetric.h"
#include "CpPlotCurve.h"
#include <cmath>

#include <qwt_plot_curve.h>
#include <qwt_plot_intervalcurve.h>
#include <qwt_plot_marker.h>

// Model
class TestModel // : public PDModel
{
    public:

        TestModel()  :
            version(""),
            paa(0), paa_dec(0), ecp(0), etau(0), ecp_del(0), tau_del(0), ecp_dec(0), ecp_dec_del(0),
            paa_pow(1), pMax(0), mmp60(0) {}

        QString version;

        // Parameters ecp
        double paa, paa_dec, ecp, etau, ecp_del, tau_del, ecp_dec, ecp_dec_del, paa_pow;

        // Parameters dan
        double p1, t1, p2, t2, a2;

        int pMax, mmp60;
};

/*class Model_eCP
{
    public:

        Model_eCP()  :
            ecp(0), etau(0), eb(0), ec(0), ed(0) {}

        Model_eCP(double ecp, double etau, double eb, double ec, double ed) :
            ecp(ecp), etau(etau), eb(eb), ec(ec), ed(ed) {}

        double ecp;
        double etau;
        double eb;
        double ec;
        double ed;
};

class Model_wsCP
{
    public:

        Model_wsCP()  :
            wa(0), wb(0), wcp(0), wpmax(0), wd(0) {}

        Model_wsCP(double wa, double wb, double wcp, double wpmax, double wd) :
            wa(wa), wb(0), wcp(wcp), wpmax(wpmax), wd(wd) {}

        double wcp, wa, wb, wpmax, wd; // Ward-Smith CP model parameters
};
*/





class ExtendedCriticalPower
{

    public:
        // construct and calculate series/metrics
        ExtendedCriticalPower(Context *context);
        ~ExtendedCriticalPower();

        //TestModel deriveExtendedCPParameters(RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);

        /*Model_wsCP deriveWardSmithCPParameters(RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);
        QwtPlotCurve* getPlotCurveForWScp(Model_wsCP athleteModeleWSCP);*/


        TestModel deriveExtendedCP_2_1_Parameters(RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);
        QwtPlotCurve* getPlotCurveForExtendedCP_2_1(TestModel athleteModeleCP2);
        QwtPlotMarker* getPlotMarkerForExtendedCP_2_1(TestModel athleteModeleCP2);

        TestModel deriveExtendedCP_2_3_Parameters(RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);
        //TestModel deriveExtendedCP_2_3_ParametersForBest(double best5s, double best1min, double best5min, double best60min);
        QwtPlotCurve* getPlotCurveForExtendedCP_2_3(TestModel model);

        TestModel deriveExtendedCP_3_1_Parameters(RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);
        QwtPlotCurve* getPlotCurveForExtendedCP_3_1(TestModel athleteModeleCP2);

        TestModel deriveExtendedCP_4_1_Parameters(RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);
        QwtPlotCurve* getPlotCurveForExtendedCP_4_1(TestModel athleteModeleCP2);

        TestModel deriveExtendedCP_4_2_Parameters(RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);
        QwtPlotCurve* getPlotCurveForExtendedCP_4_2(TestModel athleteModeleCP2);

        TestModel deriveExtendedCP_4_3_Parameters(bool usebest, RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);
        TestModel deriveExtendedCP_4_3_ParametersForBest(double best5s, double best1min, double best5min, double best60min);
        QwtPlotCurve* getPlotCurveForExtendedCP_4_3(TestModel athleteModeleCP2);
        QwtPlotCurve* getPlotLevelForExtendedCP_4_3(TestModel athleteModeleCP2);

        QwtPlotCurve* getPlotCurveForExtendedCP_4_3_WSecond(TestModel athleteModeleCP2);
        QwtPlotCurve* getPlotCurveForExtendedCP_4_3_WPrime(TestModel athleteModeleCP2);
        QwtPlotCurve* getPlotCurveForExtendedCP_4_3_CP(TestModel athleteModeleCP2);
        QwtPlotCurve* getPlotCurveForExtendedCP_4_3_WPrime_CP(TestModel athleteModeleCP2);

        // Extended CP Model version 5
        TestModel deriveExtendedCP_5_3_Parameters(bool usebest, RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);
        QwtPlotCurve* getPlotCurveForExtendedCP_5_3(TestModel athleteModeleCP2);

        TestModel deriveExtendedCP_5_3_ParametersForBest(double best5s, double best1min, double best5min, double best60min);
        QwtPlotCurve* getPlotLevelForExtendedCP_5_3(TestModel athleteModeleCP2);

        // Contributions
        QwtPlotIntervalCurve* getPlotCurveForExtendedCP_5_3_WSecond(TestModel athleteModeleCP2, bool stacked, bool inversed, int balance);
        QwtPlotIntervalCurve* getPlotCurveForExtendedCP_5_3_WPrime(TestModel athleteModeleCP2, bool stacked, bool inversed, int balance);
        QwtPlotIntervalCurve* getPlotCurveForExtendedCP_5_3_CP(TestModel athleteModeleCP2, bool stacked, bool inversed);

        // Extended CP Model version 6
        TestModel deriveExtendedCP_6_3_Parameters(bool usebest, RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);
        QwtPlotCurve* getPlotCurveForExtendedCP_6_3(TestModel athleteModeleCP2);

        // Contributions
        QwtPlotIntervalCurve* getPlotCurveForExtendedCP_6_3_WSecond(TestModel athleteModeleCP2, bool stacked);
        QwtPlotIntervalCurve* getPlotCurveForExtendedCP_6_3_WPrime(TestModel athleteModeleCP2, bool stacked);
        QwtPlotIntervalCurve* getPlotCurveForExtendedCP_6_3_CP(TestModel athleteModeleCP2, bool stacked);

        // Extended CP Model version 7
        TestModel deriveExtendedCP_7_3_Parameters(bool usebest, RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);
        QwtPlotCurve* getPlotCurveForExtendedCP_7_3(TestModel athleteModeleCP2);
        QwtPlotCurve* getPlotCurveForExtendedCP_7_3_balance(TestModel athleteModeleCP2, int depletion);


        // Marker for Model
        QwtPlotMarker* getPlotMarkerForExtendedCP(TestModel athleteModeleCP2);

        // Dan-Veloclinic Model
        TestModel deriveDanVeloclinicCP_Parameters(bool usebest, RideFileCache *bests, RideFile::SeriesType series, double sanI1, double sanI2, double anI1, double anI2, double aeI1, double aeI2, double laeI1, double laeI2);

        // Test curves
        QwtPlotCurve* getPlotCurveFor10secRollingAverage(RideFileCache *bests, RideFile::SeriesType series);
        CpPlotCurve* getPlotCurveForQualityPoint(RideFileCache *bests, RideFile::SeriesType series);
        QwtPlotCurve* getPlotCurveForDerived(RideFileCache *bests, RideFile::SeriesType series);

        // Tools
        void verifyMin(double paa, double paa_min, double paa_dec, double paa_dec_min, double ecp_dec, double ecp_dec_min, double etau, double etau_min);

    private:
        Context *context;
};

#endif
