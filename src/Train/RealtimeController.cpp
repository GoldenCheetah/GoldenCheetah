/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "RealtimeController.h"
#include "TrainSidebar.h"
#include "RealtimeData.h"
#include "Units.h"

void VirtualPowerTrainer::to_string(std::string& s) const {
    // poly|wheelrpm|name
    s.clear();
    m_pf->append(s);
    s.append(m_fUseWheelRpm ? "|1|" : "|0|");
    s.append(m_pName);
    s.append("|");
    s.append(QString::number(m_inertialMomentKGM2, 'f', 3).toStdString());
}

// Abstract base class for Realtime device controllers

RealtimeController::RealtimeController(TrainSidebar *parent, DeviceConfiguration *dc) :
    parent(parent), dc(dc), polyFit(NULL), fUseWheelRpm(false), 
    inertialMomentKGM2(0.), fAdvancedSpeedPowerMapping(true),
    prevTime(), prevRpm(0.), prevWatts(0.)
{
    if (dc != NULL)
    {
        // Save a copy of the dc
        devConf = *dc;
        this->dc = &devConf;
    } else {
        this->dc = NULL;
    }

    // setup algorithm
    processSetup();
}

int RealtimeController::start() { return 0; }
int RealtimeController::restart() { return 0; }
int RealtimeController::pause() { return 0; }
int RealtimeController::stop() { return 0; }
bool RealtimeController::find() { return false; }
bool RealtimeController::discover(QString) { return false; }
bool RealtimeController::doesPull() { return false; }
bool RealtimeController::doesPush() { return false; }
bool RealtimeController::doesLoad() { return false; }
void RealtimeController::getRealtimeData(RealtimeData &) { }
void RealtimeController::pushRealtimeData(RealtimeData &) { } // update realtime data with current values

// Estimate Power From Speed
//
// Simply mapping current speed sample to power has significant error for intervals
// where speed has changed.
//
// - Determine average power during time span since previous device speed update.
//     Uses average power over interval because point power is significantly
//     wrong for intervals where speed has changed. An average still isn't perfect
//     because acceleration probably isn't constant, probably the best general
//     approach would weight lower velocities more heavily since for a speed/power
//     curve the acceleration is greater at higher velocities.
//
// - Determine change in power due to change of device flywheel inertia.
//     Inertial energy is what goes into spinning the trainer's flywheel (and rear
//     wheel for wheel-on trainer). The InertialMomentKGM2 value is the 'I' of the
//     bicycle and trainer. The inertial adjustment computes energy that has entered
//     or left the trainer system in the form of rotational inertia, as must occur
//     when speed changes.
double RealtimeController::estimatePowerFromSpeed(double v, double wheelRpm, const std::chrono::high_resolution_clock::time_point & wheelRpmSampleTime) {

    // Early Out: Simply compute power with raw fit function.
    if (!fAdvancedSpeedPowerMapping) {
        return polyFit->Fit(v);
    }

    // milliseconds since previous device rpm data
    double ms = std::chrono::duration_cast<std::chrono::milliseconds>(wheelRpmSampleTime - prevTime).count();

    // If sample data time has not changed then skip calculation and use old value.
    if (ms <= 0.) {
        return prevWatts;
    }

    // If speed has not changed significantly then skip calculation and use old value.
    // Compute previous velocity using difference between old and new wheel rpm.    

    double previousV = wheelRpm ? (prevRpm * (v / wheelRpm)) : 0.;
    double deltaV = v - previousV;

    static const double s_KphEpsilon = 0.01;
    if (fabs(deltaV) < s_KphEpsilon) {
        return prevWatts;
    }

    // Otherwise compute power from speed, previous speed and time delta.
    double watts = 0;

    // -------------------------------------------------------------------------------------
    // Average Power Since Previous Data

    // Average speed is area under power curve in speed range, divided by change
    // in speed. Integrate across velocity interval then divide by delta v.
    //
    // Note that this will still overweight higher velocity since gradient is
    // steeper at higher velocities.
    watts = polyFit->Integrate(previousV, v) / deltaV;

    // -------------------------------------------------------------------------------------
    // Watt adjustment due to inertia change.
    double dInertialWatts = 0.;

    // If trainer's inertialMoment is defined
    if (inertialMomentKGM2) {
        // If time is less than 2 seconds then compute power interaction with trainer's moment of inertia
        // Time greater than 2 seconds... something is wrong with sample rate, skip the
        // adjustment to avoid power spikes.
        static const double s_OutOfRangeTimeSampleMS = 2000;
        if (ms <= s_OutOfRangeTimeSampleMS) {
            // Compute Deltas
            double dRadiansPS = (2 * M_PI * 60.) * (wheelRpm - prevRpm);          // rotations/minute to radians/second
            double dRadiansPSSquared = dRadiansPS * fabs(dRadiansPS);             // sign preserving ^2
            double dInertialJoules = inertialMomentKGM2 * dRadiansPSSquared / 2.; // K = m I w^2 / 2
            dInertialWatts = dInertialJoules / (ms * 1000.);                      // watts = K / s
        }
    }

    // Debug Summary
    static const bool fPrintDebugSummary = false;
    if (fPrintDebugSummary) {
        double wattFit = polyFit->Fit(v);
        qDebug() << "v: " << v << " Watts:" << (watts + dInertialWatts) << " accelAdjustment:" << (watts - wattFit) 
            << " inertialAdjustment: " << dInertialWatts << " totalAdjust:" << ((watts+dInertialWatts) - wattFit);
    }

    // Keep Power Non-Negative
    //
    // Note: Negative watts while coasting indicates an error in the device inertia, or in the device
    // speed to power fit function, but could also occur if rider uses their brakes.
    watts = std::max(0., watts + dInertialWatts);

    // Store watts, rpm and sample time for use as tehe next 'previous sample.'
    prevWatts = watts;
    prevRpm = wheelRpm;
    prevTime = wheelRpmSampleTime;

    return watts;
}

void RealtimeController::processRealtimeData(RealtimeData &rtData)
{
    // Compute speed from power if a post-process is defined. At this point the postprocess id
    // has been instantiated into polyfit.
    if (polyFit) {
        double wheelRpm = rtData.getWheelRpm();
        double v = (fUseWheelRpm) ? wheelRpm : rtData.getSpeed();
        double watts = estimatePowerFromSpeed(v, wheelRpm, rtData.getWheelRpmSampleTime());

        rtData.setWatts(watts);
    }
}

// Wrap static array in function to ensure it is init on use, which is after PolyFitGenerator
// who is static init at load time.
const VirtualPowerTrainer * PredefinedVirtualPowerTrainerArray(size_t &size) {

    static const VirtualPowerTrainer s_PredefinedVirtualPowerTrainerArray[] =
    {
        {
            "None",
            PolyFitGenerator::GetPolyFit({ 0, 0, 0, 0 }, MILES_PER_KM),
            false
        },
        {
            "Power - Kurt Kinetic - Cyclone",
            // using the algorithm from http://www.kurtkinetic.com/powercurve.php
            PolyFitGenerator::GetPolyFit({ 0, 6.481090, 0, 0.020106 }, MILES_PER_KM),
            false
        },
        {
            "Power - Kurt Kinetic - Road Machine",
            // using the algorithm from http://www.kurtkinetic.com/powercurve.php
            PolyFitGenerator::GetPolyFit({ 0, 5.244820, 0, 0.019168 }, MILES_PER_KM),
            false,
        },
        {
            "Power - Cyclops Fluid 2",
            // using the algorithm from:
            // http://thebikegeek.blogspot.com/2009/12/while-we-wait-for-better-and-better.html
            PolyFitGenerator::GetPolyFit({ 0, 8.9788, 0.0137, 0.0115 }, MILES_PER_KM),
            false,
        },
        {
            "Power - BT-ATS - BT Advanced Training System",
            // using the algorithm from Steven Sansonetti of BT:

            //  This is a 3rd order polynomial, where P = av3 + bv2 + cv + d
            //  where:
            //        v is expressed in revs/second
            PolyFitGenerator::GetPolyFit({ 0.0,5.92125507E-01,-4.61311774E-02,2.90390167E-01 }, 1. / 60.),
            true // use wheel rpm
        },
        {
            "Power - Lemond Revolution",
            // Tom Anhalt spent a lot of time working this all out
            // for the data / analysis see: http://wattagetraining.com/forum/viewtopic.php?f=2&t=335
            PolyFitGenerator::GetPolyFit({ 0, 4.25, 0, 0.21 }, 15. / 18.),
            false
        },
        {
            "Power - 1UP USA",
            // Power curve provided by extraction from SportsTracks plugin
            PolyFitGenerator::GetPolyFit({ 25, 2.65, 0.42, 0.058 }, MILES_PER_KM),
            false
        },
        // MINOURA - Has many gears
        {
            "Power - MINOURA V100 on H",
            // 7 = V100 on H: y = -0.0036x^3 + 0.2815x^2 + 3.4978x - 9.7857 
            PolyFitGenerator::GetPolyFit({ -9.7857, 3.4978, 0.2815, -0.0036 }),
            false
        },
        {
            "Power - MINOURA V100 on 5",
            // 8 = V100 on 5: y = -0.0023x^3 + 0.2067x^2 + 3.8906x - 11.214
            PolyFitGenerator::GetPolyFit({ -11.214, 3.8906, 0.2067, -0.0023 }),
            false
        },
        {
            "Power - MINOURA V100 on 4",
            // 9 = V100 on 4: y = -0.00173x^3 + 0.1825x^2 + 3.4036x - 10 
            PolyFitGenerator::GetPolyFit({ -10, 3.4036, 0.1824, -0.00173 }),
            false
        },
        {
            "Power - MINOURA V100 on 3",
            // 10 = V100 on 3: y = -0.0011x^3 + 0.1433x^2 + 2.8808x - 8.1429 
            PolyFitGenerator::GetPolyFit({ -8.1429, 2.8808, 0.1433, -0.0011 }),
            false,
        },
        {
            "Power - MINOURA V100 on 2",
            // 11 = V100 on 2: y = -0.0007x^3 + 0.1348x^2 + 1.581x - 3.3571 
            PolyFitGenerator::GetPolyFit({ -3.3571, 1.581, 0.1348, -0.0007 }),
            false
        },
        {
            "Power - MINOURA V100 on 1",
            // 12 = V100 on 1: y = 0.0004x^3 + 0.057x^2 + 1.7797x - 5.0714 
            PolyFitGenerator::GetPolyFit({ -5.0714, 1.7797, 0.057, 0.0004 }),
            false
        },
        {
            "Power - MINOURA V100 on L",
            // 13 = V100 on L: y = 0.0557x^2 + 1.231x - 3.7143 
            PolyFitGenerator::GetPolyFit({ -3.7143, 1.231, 0.557 }),
            false
        },
        {
            "Power - MINOURA V270/v130 on H",
            PolyFitGenerator::GetPolyFit({ -0.628959276017,  0.538329905388 , 0.782320032908 ,-0.0237725215961 ,0.000338955162485 ,-1.84615384615e-06 }),
            false
        },
        {
            "Power - MINOURA V270/v130 on 5",
            PolyFitGenerator::GetPolyFit({ -0.472850678733, 1.16395173454 , 0.587825311943 ,-0.0153105032223 ,0.000193281228575 ,-9.35143288085e-07 }),
            false
        },
        {
            "Power - MINOURA V270/v130 on 4",
            PolyFitGenerator::GetPolyFit({ -0.364253393665, 1.10649184149, 0.52033045386 ,-0.0117029686, 0.000114219114219 , -3.34841628959e-07 }),
            false
        },
        {
            "Power - MINOURA V270/v130 on 3",
            PolyFitGenerator::GetPolyFit({ -0.386877828054 ,1.00085150144 , 0.439146098999, -0.00805837789661 , 3.96681749623e-05, 2.32277526395e-07 }),
            false
        },
        {
            "Power - MINOURA V270/v130 on 2",
            PolyFitGenerator::GetPolyFit({ -0.00678733031682, 0.278777594954, 0.434414507062 , -0.0103757370081, 0.000123625394214 , -5.58069381599e-07 }),
            false
        },
        {
            "Power - MINOURA V270/v130 on 1",
            PolyFitGenerator::GetPolyFit({ -0.056561085973, 0.704304812834, 0.237884615385 , -0.00251391745509 , -1.7235705471e-05, 3.74057315234e-07 }),
            false
        },
        {
            "Power - MINOURA V270/v130 on L",
            PolyFitGenerator::GetPolyFit({ -0.0859728506788, 0.642516796929, 0.122555189908 , -6.92787604552e-05, -3.38406691348e-05, 3.46907993967e-07 }),
            false
        },
        {
            "Power - SARIS POWERBEAM PRO",
            // 21 = 0.0008x^3 + 0.145x^2 + 2.5299x + 14.641 where x = speed in kph
            PolyFitGenerator::GetPolyFit({ 14.641, 2.5299, 0.145, 0.0008 }),
            false
        },
        {
            "Power - TACX SATORI SETTING 2",
            PolyFitGenerator::GetPolyFit({ -19.5, 3.9 }),
            false
        },
        {
            "Power - TACX SATORI SETTING 4",
            PolyFitGenerator::GetPolyFit({ -52.3, 6.66 }),
            false,
        },
        {
            "Power - TACX SATORI SETTING 6",
            PolyFitGenerator::GetPolyFit({ -43.65, 9.43 }),
            false
        },
        {   "Power - TACX SATORI SETTING 8",
            PolyFitGenerator::GetPolyFit({ -51.15, 13.73 }),
            false
        },
        {
            "Power - TACX SATORI SETTING 10",
            PolyFitGenerator::GetPolyFit({ -76.0, 17.7 }),
            false
        },
        {
            "Power - TACX FLOW SETTING 0",
            PolyFitGenerator::GetPolyFit({ -47.27, 7.75 }),
            false
        },
        {
            "Power - TACX FLOW SETTING 2",
            PolyFitGenerator::GetPolyFit({ -66.69, 9.51 }),
            false
        },
        { "Power - TACX FLOW SETTING 4",
            PolyFitGenerator::GetPolyFit({ -71.59, 11.03 }),
            false
        },
        {
            "Power - TACX FLOW SETTING 6",
            PolyFitGenerator::GetPolyFit({ -95.05, 12.81 }),
            false
        },
        {
            "Power - TACX FLOW SETTING 8",
            PolyFitGenerator::GetPolyFit({ -102.43, 14.37 }),
            false
        },
        {
            "Power - TACX BLUE TWIST SETTING 1",
            PolyFitGenerator::GetPolyFit({ -24, 3.2 }),
            false
        },
        {
            "Power - TACX BLUE TWIST SETTING 3",
            PolyFitGenerator::GetPolyFit({ -46.5, 6.525 }),
            false
        },
        {
            "Power - TACX BLUE TWIST SETTING 5",
            PolyFitGenerator::GetPolyFit({ -66.5, 9.775 }),
            false
        },
        {
            "Power - TACX BLUE TWIST SETTING 7",
            PolyFitGenerator::GetPolyFit({ -89.5, 13.075 }),
            false
        },
        {
            "Power - TACX BLUE MOTION SETTING 2",
            PolyFitGenerator::GetPolyFit({ -36.5, 5.225 }),
            false
        },
        {
            "Power - TACX BLUE MOTION SETTING 4",
            PolyFitGenerator::GetPolyFit({ -53.0, 8.25 }),
            false
        },
        {
            "Power - TACX BLUE MOTION SETTING 6",
            PolyFitGenerator::GetPolyFit({ -74.0, 11.45 }),
            false
        },
        {
            "Power - TACX BLUE MOTION SETTING 8",
            PolyFitGenerator::GetPolyFit({ -89, 14.45 }),
            false
        },
        {
            "Power - TACX BLUE MOTION SETTING 10",
            PolyFitGenerator::GetPolyFit({ -110.5, 17.575 }),
            false
        },
        {
            "Power - ELITE SUPERCRONO POWER MAG LEVEL 1",
            // Power curve provided by extraction from SportsTracks plugin
            PolyFitGenerator::GetPolyFit({ -1.16783216783223, 3.62446277061515, 0.17689196198325 , -0.000803192769148186 }, MILES_PER_KM),
            false
        },
        {
            "Power - ELITE SUPERCRONO POWER MAG LEVEL 2",
            // Power curve provided by extraction from SportsTracks plugin
            PolyFitGenerator::GetPolyFit({ -0.363636363636395, 3.54843470904764, 0.442531768374482 ,-0.00590735326986424 }, MILES_PER_KM),
            false
        },
        {
            "Power - ELITE SUPERCRONO POWER MAG LEVEL 3",
            // Power curve provided by extraction from SportsTracks plugin
            PolyFitGenerator::GetPolyFit({ -1.48951048951047, 5.08762781732785, 0.614352424962992 , -0.00917194323478923 }, MILES_PER_KM),
            false
        },
        {
            "Power - ELITE SUPERCRONO POWER MAG LEVEL 4",
            // Power curve provided by extraction from SportsTracks plugin
            PolyFitGenerator::GetPolyFit({ -1.7342657342657, 5.16903286351279, 0.880112976720764 ,-0.0150015681721553 }, MILES_PER_KM),
            false
        },
        {
            "Power - ELITE SUPERCRONO POWER MAG LEVEL 5",
            // Power curve provided by extraction from SportsTracks plugin
            PolyFitGenerator::GetPolyFit({ -3.18881118881126,6.23730215622854,1.0207209560583 ,-0.0172621671756449 }, MILES_PER_KM),
            false
        },
        {
            "Power - ELITE SUPERCRONO POWER MAG LEVEL 6",
            // Power curve provided by extraction from SportsTracks plugin
            PolyFitGenerator::GetPolyFit({ -4.18881118881114, 7.47138264900755, 1.15505017633569, -0.0195227661791347 }, MILES_PER_KM),
            false
        },
        {
            "Power - ELITE SUPERCRONO POWER MAG LEVEL 7",
            // Power curve provided by extraction from SportsTracks plugin
            PolyFitGenerator::GetPolyFit({ -5.11888111888112, 8.74972948026508, 1.2917943039439 , -0.0222497351776137 }, MILES_PER_KM),
            false
        },
        {
            "Power - ELITE SUPERCRONO POWER MAG LEVEL 8",
            // Power curve provided by extraction from SportsTracks plugin
            PolyFitGenerator::GetPolyFit({ -6.48951048951042, 10.2050166192824, 1.42902141301828 ,-0.0255078477814972 }, MILES_PER_KM),
            false
        },
        {
            "Power - ELITE TURBO MUIN 2013",
            // Power curve fit from data collected by Ray Maker at dcrainmaker.com
            PolyFitGenerator::GetPolyFit({ 0, 2.30615942, -0.28395558, 0.02099661 }),
            false
        },
        {
            "Power - ELITE QUBO POWER FLUID",
            // Power curve fit from powercurvesensor
            //     f(x) = 4.31746 * x -2.59259e-002 * x^2 +  9.41799e-003 * x^3
            PolyFitGenerator::GetPolyFit({ 0, 4.31746, -2.59259e-002 , 9.41799e-003 }),
            false
        },
        {
            "Power - CYCLOPS MAGNETO PRO (ROAD)",
            //     Watts = 6.0f + (-0.93 * speed) + (0.275 * speed^2) + (-0.00175 * speed^3)
            PolyFitGenerator::GetPolyFit({ 0, -0.93, 0.275, -0.00175 }),
            false
        },
        {
            "Power - ELITE ARION MAG LEVEL 0",
            PolyFitGenerator::GetFractionalPolyFit({ 3.335794377, 1.217110021, 0. }),
            false
        },
        {
            "Power - ELITE ARION MAG LEVEL 1",
            PolyFitGenerator::GetFractionalPolyFit({ 1.206592577, 4.362485081, 0. }),
            false
        },
        {
            "Power - ELITE ARION MAG LEVEL 2",
            PolyFitGenerator::GetFractionalPolyFit({ 1.206984321, 6.374459698, 0. }),
            false
        },
        {
            "Power - Blackburn Tech Fluid",
            PolyFitGenerator::GetPolyFit({ 6.758241758241894, -1.9995004995004955, 0.24165834165834146 }),
            false
        },
        {
            "Power - TACX SIRIUS LEVEL 1",
            PolyFitGenerator::GetPolyFit({ -20.64808196, 3.23874687 }),
            false
        },
        {
            "Power - TACX SIRIUS LEVEL 2",
            PolyFitGenerator::GetPolyFit({ -27.25589246, 4.30606133 }),
            false
        },
        {
            "Power - TACX SIRIUS LEVEL 3",
            PolyFitGenerator::GetPolyFit({ -36.57159131, 5.44879778 }),
            false
        },
        {
            "Power - TACX SIRIUS LEVEL 4",
            PolyFitGenerator::GetPolyFit({ -40.48616615, 6.48525956 }),
            false
        },
        {
            "Power - TACX SIRIUS LEVEL 5",
            PolyFitGenerator::GetPolyFit({ -48.35481582, 7.60643338 }),
            false
        },
        {
            "Power - TACX SIRIUS LEVEL 6",
            PolyFitGenerator::GetPolyFit({ -58.57819586, 8.73140257 }),
            false
        },
        {
            "Power - TACX SIRIUS LEVEL 7",
            PolyFitGenerator::GetPolyFit({ -59.61416463, 9.73079724 }),
            false
        },
        {
            "Power - TACX SIRIUS LEVEL 8",
            PolyFitGenerator::GetPolyFit({ -73.08258491, 10.94228338 }),
            false
        },
        {
            "Power - TACX SIRIUS LEVEL 9",
            PolyFitGenerator::GetPolyFit({ -77.88781457, 11.99373782 }),
            false
        },
        {
            "Power - TACX SIRIUS LEVEL 10",
            PolyFitGenerator::GetPolyFit({ -85.38477516, 13.09580164 }),
            false
        },
        {
            "Power - ELITE CRONO FLUID ELASTOGEL",
            PolyFitGenerator::GetPolyFit({ 0, 2.4508084253648112, -0.0005622440886940634 ,0.0031006831781331848 }),
            false
        },
        {
            "Power - ELITE TURBO MUIN 2015",
            PolyFitGenerator::GetPolyFit({ 0, 3.01523235942763813175,  -0.12045521113635432750 , 0.01739165560254292102 }),
            false
        },
        {
            "Power - CycleOps JetFluid Pro",
            PolyFitGenerator::GetPolyFit({ 0, 0.94874757375670469046 , 0.11615123681031937322 , 0.00400691905019748630 }),
            false
        },
        {
            "Power - Elite Crono Mag elastogel",
            PolyFitGenerator::GetPolyFit({ 0, 7.34759700400455518172, -0.00278841177590215417, 0.00052233430180969281 }),
            false
        },
        {
            "Power - Tacx Magnetic T1820 (4/7)",
            PolyFitGenerator::GetPolyFit({ 0, 6.77999074563685972005, -0.00148787143661883094, 0.00085058630585658189 }),
            false
        },
        {
            "Power - Tacx Magnetic T1820 (7/7)",
            PolyFitGenerator::GetPolyFit({ 0, 9.80556623734881995572, -0.00668894865103764724,  0.00125560535410925628 }),
            false
        }
    };

    size = (int)(sizeof(s_PredefinedVirtualPowerTrainerArray) / sizeof(s_PredefinedVirtualPowerTrainerArray[0]));

    return s_PredefinedVirtualPowerTrainerArray;
}

// Virtual Power Trainer Manager

int VirtualPowerTrainerManager::GetPredefinedVirtualPowerTrainerCount() const {
    return RealtimeController::GetPredefinedVirtualPowerTrainerCount();
}

bool VirtualPowerTrainerManager::IsPredefinedVirtualPowerTrainerIndex(int idx) {
    return idx < RealtimeController::GetPredefinedVirtualPowerTrainerCount();
}

int VirtualPowerTrainerManager::GetCustomVirtualPowerTrainerCount() const {
    return (int)customVirtualPowerTrainers.size();
}

const VirtualPowerTrainer* VirtualPowerTrainerManager::GetCustomVirtualPowerTrainer(int id) const {
    if (id < 0 || id > GetCustomVirtualPowerTrainerCount())
        return NULL;

    return customVirtualPowerTrainers[id];
}

int VirtualPowerTrainerManager::GetVirtualPowerTrainerCount() const {
    return
        GetPredefinedVirtualPowerTrainerCount() +
        GetCustomVirtualPowerTrainerCount();
}

const VirtualPowerTrainer* VirtualPowerTrainerManager::GetVirtualPowerTrainer(int id) const {
    if (id <= 0 || id > GetVirtualPowerTrainerCount())
        return NULL;

    int predefinedCount = (int)RealtimeController::GetPredefinedVirtualPowerTrainerCount();
    if (id < predefinedCount)
        return RealtimeController::GetPredefinedVirtualPowerTrainer(id);

    return GetCustomVirtualPowerTrainer(id - predefinedCount);
}

int VirtualPowerTrainerManager::PushCustomVirtualPowerTrainer(const VirtualPowerTrainer* vpt) {
    customVirtualPowerTrainers.push_back(vpt);
    return (int)customVirtualPowerTrainers.size();
}

void VirtualPowerTrainerManager::GetVirtualPowerTrainerAsString(int idx, QString& s) {
    s.clear();

    const VirtualPowerTrainer* p = GetVirtualPowerTrainer(idx);
    if (p) {
        // Stringify virtual trainer and store...
        std::string string;
        p->to_string(string);
        s = string.c_str();
    } else {
        s = "";
    }
}

// Synthesize a new custom power curve from string description.
// Returns index of new virtualpowertrainer
int VirtualPowerTrainerManager::PushCustomVirtualPowerTrainer(const QString& string) {

    // parse string back into virtualtrainer.
    //
    // type,coefcount,numeratorcount|coefs,...|scale|usewheelrpm|name
    // DEF,COEFS,SCALE,RPM,NAME

    // DEF:   tla,coefcount,numeratorcount
    // COEFS:   |coefs,...
    // SCALE:   |scale}
    // RPM:     |wheelrpm
    // NAME:    |name
    // INERTIA: |rotationInertiaGrams

    do {
        QStringList pieces = string.split("|");

        size_t pieceCount = pieces.size();
        if (pieceCount != 5 && pieceCount != 6) break; // no inertia supported for back compat

        // section 0 DEF
        QStringList defPieces = pieces.at(0).split(QRegularExpression(QRegExp::escape(",")));
        size_t defCount = defPieces.size();
        if (defCount != 3) break; // bad prefix section count...

        // 0,0 DEF:TLA
        bool isFractional = false;
        if (!defPieces.at(0).compare("FPR")) isFractional = true;
        else if (!defPieces.at(0).compare("RPR")) isFractional = false;
        else break; // bad tla...

        // 0,1 DEF:COEFCOUNT
        int coefCount = defPieces.at(1).toInt();
        if (coefCount <= 0 || coefCount > 14) break; // bad coef count

        // 0,2 DEF:NUMERATORCOUNT
        int numeratorCount = defPieces.at(2).toInt();
        if (numeratorCount <= 0 || numeratorCount > 14) break; // bad numerator coef count

        // section 1 COEFS
        QStringList coefs = pieces.at(1).split(QRegularExpression(QRegExp::escape(",")));
        size_t coefPieceCount = coefs.size();
        if (coefPieceCount != coefCount) break; // wrong number of coefs in string

        // Distribute coefs to numerator and denominator coefficient arrays.
        // Denominator always has an implict constant 1. If denominator array
        // is empty then fit is simple polynomial, otherwise it is rational.
        std::vector<double> num, den;

        int i = 0;
        for (; i < numeratorCount; i++) num.push_back(coefs.at(i).toDouble());
        for (; i < coefCount; i++) den.push_back(coefs.at(i).toDouble());

        // section 2 SCALE
        double scale = pieces.at(2).toDouble();

        // section 3 RPM
        bool fUseWheelRpm = pieces.at(3).compare("0") != 0;

        // section 4 Name
        QString namePiece = pieces.at(4);

        // section 5 OPTIONAL: Rotational Inertia (grams)
        double inertialMomentKGM2 = 0;
        if (pieceCount == 6)
            inertialMomentKGM2 = pieces.at(5).toDouble();

        // Finished with parse errors. All memory allocated is held by new
        // VirtualPowerTrainer and freed by manager when manager is destroyed.

        std::string name = namePiece.toStdString();
        char* pNameCopy = new char[name.size() + 1];
        strcpy(pNameCopy, name.c_str());

        VirtualPowerTrainer* p = new VirtualPowerTrainer();

        p->m_pName = pNameCopy;

        if (isFractional) 
            p->m_pf = PolyFitGenerator::GetFractionalPolyFit(num, scale);
        else if (numeratorCount == coefCount)
            p->m_pf = PolyFitGenerator::GetPolyFit(num, scale);
        else
            p->m_pf = PolyFitGenerator::GetRationalPolyFit(num, den, scale);

        p->m_fUseWheelRpm = fUseWheelRpm;

        p->m_inertialMomentKGM2 = inertialMomentKGM2;

        PushCustomVirtualPowerTrainer(p);

        return GetVirtualPowerTrainerCount() - 1;

    } while (false);

    return 0;
}

VirtualPowerTrainerManager::~VirtualPowerTrainerManager() {
    for (auto& i : customVirtualPowerTrainers) {
        delete i->m_pName; // custom trainer name string is allocated on create but owned and freed by manager.
        delete i;
    }
    customVirtualPowerTrainers.clear();
}

int RealtimeController::GetPredefinedVirtualPowerTrainerCount() {
    size_t size;
    PredefinedVirtualPowerTrainerArray(size);
    return (int)size;
}

const VirtualPowerTrainer* RealtimeController::GetPredefinedVirtualPowerTrainer(int id) {
    size_t size;
    const VirtualPowerTrainer * const p = PredefinedVirtualPowerTrainerArray(size);
    if (id < 0 || id >= size)
        return NULL;

    return &(p[id]);
}

void
RealtimeController::processSetup()
{
    if (!dc) return; // no config

    // Custom postprocess are stored with postprocess 0 AND a definition string.
    int postProcess = dc->postProcess;
    if (!postProcess && dc->virtualPowerDefinitionString.size() > 1) {
        // Instantiate custom postprocess from device configuration.
        postProcess = virtualPowerTrainerManager.PushCustomVirtualPowerTrainer(dc->virtualPowerDefinitionString);
    }

    // Setup polyfit and rpm settings from postprocess id.
    const VirtualPowerTrainer* pTrainer = virtualPowerTrainerManager.GetVirtualPowerTrainer(postProcess);
    if (pTrainer) {
        polyFit = pTrainer->m_pf;
        fUseWheelRpm = pTrainer->m_fUseWheelRpm;
        inertialMomentKGM2 = pTrainer->m_inertialMomentKGM2;
    } else {
        polyFit = NULL;
        fUseWheelRpm = false;
        inertialMomentKGM2 = 0.;
    }

    prevWatts = 0;
    prevRpm = 0;
    prevTime = std::chrono::high_resolution_clock::time_point();
}

void
RealtimeController::setCalibrationTimestamp()
{
    lastCalTimestamp = QTime::currentTime();
}

QTime
RealtimeController::getCalibrationTimestamp()
{
    return lastCalTimestamp;
}
