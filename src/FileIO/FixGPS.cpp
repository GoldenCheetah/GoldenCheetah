/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "DataProcessor.h"
#include "Settings.h"
#include "Units.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>
#include <QFormLayout>

#include "LocationInterpolation.h"
#include "GeometricTools_BSplineCurve.h"

using namespace gte;

bool smoothAltitude(const RideFile *ride, int degree, double &minSlope, double &maxSlope, std::vector<Vector2<double>> & outControls);

void ComputeRideFileStats(const RideFile * ride, double &minSlope, double &maxSlope, double &sampleDistanceVariance)
{
    minSlope = 0;
    maxSlope = 0;
    sampleDistanceVariance = 0;

    const RideFilePoint * prevPi = NULL;
    double prevKM = 0;
    for (int i = 0; i < ride->dataPoints().count(); i++) {
        const RideFilePoint * pi = (ride->dataPoints()[i]);

        // Compute slope since ridefile slope is truncated.
        double slope = 0;
        if (i > 0) {
            double rise = pi->alt - prevPi->alt;
            double run = 1000. * (pi->km - prevPi->km);
            slope = run ? (rise / run) : 0;
            slope *= 100.;
        }

        if (slope < minSlope)
            minSlope = slope;
        if (slope > maxSlope)
            maxSlope = slope;

        double kmDelta = pi->km - prevKM;
        prevKM = pi->km;

        sampleDistanceVariance += (1000 * kmDelta * 1000 * kmDelta);

        prevPi = pi;
    }

    sampleDistanceVariance /= ride->dataPoints().count();
}

// Config widget used by the Preferences/Options config panes
class FixGPS;
class FixGPSConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixGPSConfig)
    friend class ::FixGPS;
    protected:
        QVBoxLayout *mainLayout;
        QHBoxLayout *simpleLayout;
        QFormLayout *layout;
        QLabel *degreeLabel;
        QSpinBox *degreeSpinBox;
        QCheckBox *doSmoothAltitude;
        QPushButton *testButton;
        QLabel *minSlopeLabel;
        QLabel *maxSlopeLabel;
        QLabel *varianceLabel;
        const RideFile *ride;

    public slots:
        void testClicked()
        {
            // This function should be unreachable unless ride exists.
            if (ride)
            {
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                testButton->setEnabled(false);

                int degree = degreeSpinBox->value();

                // Test and report effectiveness of smoothing but do not apply changes to ride file.
                double minSlope, maxSlope;
                std::vector<Vector2<double>> outControls;
                smoothAltitude(ride, degree, minSlope, maxSlope, outControls);

                QString minLabel(tr("Min Slope"));
                QString maxLabel(tr("Max Slope"));

                minLabel.append(QString(": %1").arg(minSlope));
                maxLabel.append(QString(": %1").arg(maxSlope));

                minSlopeLabel->setText(minLabel);
                minSlopeLabel->setToolTip(tr("Min slope computed from current smoothing parameters."));

                maxSlopeLabel->setText(maxLabel);
                maxSlopeLabel->setToolTip(tr("Max slope computed from current smoothing parameters."));

                testButton->setEnabled(true);
                QApplication::restoreOverrideCursor();
            }
        }

    public:
        FixGPSConfig(QWidget *parent, const RideFile * rideFile) : DataProcessorConfig(parent) {

            mainLayout = NULL;
            simpleLayout = NULL;
            layout = NULL;
            degreeLabel = NULL;
            degreeSpinBox = NULL;
            doSmoothAltitude = NULL;
            testButton = NULL;
            minSlopeLabel = NULL;
            maxSlopeLabel = NULL;
            varianceLabel = NULL;
            ride = rideFile;

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixGPSErrors));

            // Degree SpinBox

            degreeLabel = new QLabel(tr("Altitude Smoothing Sample Degree:"));
            degreeSpinBox = new QSpinBox();
            degreeSpinBox->setRange(0, 500);
            degreeSpinBox->setSingleStep(10);
            degreeSpinBox->setValue(appsettings->value(this, GC_FIXGPS_ALTITUDE_FIX_DEGREE, 200).toInt());
            degreeSpinBox->setToolTip(tr("The 'degree' of a b-spline is the number of samples that are\n"
                                         "used to compute each point. Smoothing becomes more global\n"
                                         "as 'degree' increases. Degree is in terms of samples so exact\n"
                                         "window size in meters depends on the sample separation in\n"
                                         "the ridefile and can vary by sample."));

            // Apply Smoothing Checkbox

            doSmoothAltitude = new QCheckBox(tr("Apply BSpline Altitude Smoothing"));
            doSmoothAltitude->setToolTip(tr("Check this box to apply b-spline based altitude smoothing after running the GPS outlier pass."));
            doSmoothAltitude->setCheckState(appsettings->value(NULL, GC_FIXGPS_ALTITUDE_FIX_DOAPPLY, Qt::Unchecked).toBool() ? Qt::Checked : Qt::Unchecked);

            // If no ridefile is provided then present simple dialog
            if (rideFile == NULL)
            {
                simpleLayout = new QHBoxLayout(this);

                simpleLayout->addWidget(doSmoothAltitude);
                simpleLayout->addWidget(degreeLabel);
                simpleLayout->addWidget(degreeSpinBox);

                simpleLayout->setContentsMargins(0, 0, 0, 0);
                setContentsMargins(0, 0, 0, 0);

                simpleLayout->addStretch();
            }
            else
            {
                // Determine min and max slope of original ride file.
                double minSlope = 0;
                double maxSlope = 0;
                double sampleDistanceVariance = 0;
                bool fHasAlt = ride && ride->areDataPresent()->alt;
                if (fHasAlt) {
                    ComputeRideFileStats(ride, minSlope, maxSlope, sampleDistanceVariance);
                }

                mainLayout = new QVBoxLayout(this);
                layout = new QFormLayout();
                mainLayout->addLayout(layout);

                mainLayout->setContentsMargins(0, 0, 0, 0);
                setContentsMargins(0, 0, 0, 0);

                // Create widgets

                // Min/Max Slope and distance variance Labels

                QString minSlopeLabelString(tr("Min Slope"));
                QString maxSlopeLabelString(tr("Max Slope"));
                QString varianceLabelString(tr("Distance Variance"));

                minSlopeLabelString.append(QString(": %1").arg(minSlope));
                maxSlopeLabelString.append(QString(": %1").arg(maxSlope));
                varianceLabelString.append(QString(": %1").arg(sampleDistanceVariance));

                minSlopeLabel = new QLabel(minSlopeLabelString);
                minSlopeLabel->setToolTip(tr("Min slope computed from ride file altitude and distance information."));
                maxSlopeLabel = new QLabel(maxSlopeLabelString);
                maxSlopeLabel->setToolTip(tr("Max slope computed from ride file altitude and distance information."));
                varianceLabel = new QLabel(varianceLabelString);
                varianceLabel->setToolTip(tr("Variance of distance between ride file samples."));

                // Test Smoothing Button

                testButton = new QPushButton(tr("Test Altitude Smoothing"), this);
                testButton->setToolTip(tr("Click this button to simulate behavior of current smoothing settings and report what min and max slope will be."));

                // Create Rows of Widgets

                QHBoxLayout* row0 = new QHBoxLayout();
                row0->addWidget(degreeLabel);
                row0->addWidget(degreeSpinBox);

                QHBoxLayout* row1 = new QHBoxLayout();
                row1->addWidget(doSmoothAltitude);

                QHBoxLayout* row2 = new QHBoxLayout();
                row2->addWidget(testButton);

                QHBoxLayout* row3 = new QHBoxLayout();
                row3->addWidget(minSlopeLabel);
                row3->addWidget(maxSlopeLabel);
                row3->addWidget(varianceLabel);

                // Insert rows into layout

                layout->insertRow(0, row0);
                layout->insertRow(1, row1);
                layout->insertRow(2, row2);
                layout->insertRow(3, row3);

                // Hook up testButton to testClicked method.

                connect(testButton, &QPushButton::clicked, this, &FixGPSConfig::testClicked);
            }
        }
        
        QString explain() {
            return(QString(tr("Remove GPS errors and interpolate positional "
                           "data where the GPS device did not record any data, "
                           "or the data that was recorded is invalid.\n"
                           "B-Spline smoothing will be applied to altitude if "
                           "checkbox is set. A larger degree will smooth more globally "
                           "but will dampen local variance. The effectiveness of "
                           "spline degree's smoothing on min and max slope can be "
                           "checked with the test button.\n")));
        }

        void readConfig() {
            degreeSpinBox->setValue(appsettings->value(this, GC_FIXGPS_ALTITUDE_FIX_DEGREE, 200).toInt());
            doSmoothAltitude->setCheckState(appsettings->value(NULL, GC_FIXGPS_ALTITUDE_FIX_DOAPPLY, Qt::Unchecked).toBool() ? Qt::Checked : Qt::Unchecked);
        }

        void saveConfig() {
            appsettings->setValue(GC_FIXGPS_ALTITUDE_FIX_DEGREE, degreeSpinBox->value());
            appsettings->setValue(GC_FIXGPS_ALTITUDE_FIX_DOAPPLY, doSmoothAltitude->checkState());
        }
};

// RideFile Dataprocessor -- used to handle gaps in recording
//                           by inserting interpolated/zero samples
//                           to ensure dataPoints are contiguous in time
//
class FixGPS : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FixGPS)

    public:
        FixGPS() {}
        ~FixGPS() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig*settings=0, QString op="");

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
            Q_UNUSED(ride);
            return new FixGPSConfig(parent, ride);
        }

        // Localized Name
        QString name() {
            return (tr("Fix GPS errors"));
        }
};

static bool fixGPSAdded = DataProcessorFactory::instance().registerProcessor(QString("Fix GPS errors"), new FixGPS());

bool IsReasonableGeoLocation(geolocation *ploc) {
    return  (ploc->Lat()  && ploc->Lat()  >= double(-90)  && ploc->Lat()  <= double(90) &&
             ploc->Long() && ploc->Long() >= double(-180) && ploc->Long() <= double(180) &&
             ploc->Alt() >= -1000 && ploc->Alt() < 10000);
}

// Gradient descent requires 2x the compute per evaluation but usually seens 7x fewer evalutaions,
// so > 3x speedup.
template <bool T_GradientDescent>
bool EvalAtTargetDistance(BSplineCurve<2, double> &curve, double targetDistance, double epsilon, unsigned sampleCount, double &prevT, Vector2<double> &pos, unsigned &evalCount)
{
    static const int s_iGradientDegree = T_GradientDescent ? 1 : 0;

    double t = prevT;
    double step = 1 / (double)sampleCount; // default step size if gradient descent isn't used.
    bool fGoingUp = true;

    while (true) {
        evalCount++;

        // Fail if no convegence after 100 tries.
        if (evalCount > 100)
            return false;

        Vector2<double> evalResults[1 + s_iGradientDegree];

        curve.Evaluate(t, s_iGradientDegree, evalResults);

        pos = evalResults[0];
        double delta = pos[0] - targetDistance;
        if (fabs(delta) < epsilon) {
            prevT = t;
            break;
        }

        bool fSuccess = false;
        if (T_GradientDescent) {
            // Use gradient to predict next sample point.
            double evalRise = evalResults[1][0];
            double evalStep = -(delta / evalRise);

            // Gradient descent can fail if unlucky.
            // If suggested next sample point is out of range then fall back
            // to binary search.
            if (t + evalStep >= 0. && t + evalStep <= 1.) {
                step = evalStep;
                fSuccess = true;
            }
        }

        if (!fSuccess) {
            bool directionChange = (delta > 0) == fGoingUp;
            if (directionChange)
            {
                step /= -2.;
                fGoingUp = !fGoingUp;
            }
        }

        t += step;
    }

    return true;
}

// Create new distance/altitude curve
bool InterpolateBSplineCurve(std::vector<Vector2<double>> &inControls, std::vector<Vector2<double>> &outControls, BSplineCurve<2, double> &curve)
{
    static const double precision = 0.0001; // 10cm

    outControls.resize(0);

    double t = 0;
    for (unsigned u = 0; u < inControls.size(); u++) {
        double targetDistance = inControls[u][0];

        unsigned evalCount = 0;
        Vector2<double> pos;

        bool fConvergence = EvalAtTargetDistance<true>(curve, targetDistance, precision, (int)inControls.size(), t, pos, evalCount);
        if (!fConvergence) {
            outControls.resize(0);
            return false;
        }

        outControls.push_back({ pos[0] , pos[1] });
    }

    return true;
}

// Smooth ridefile altitude. Return true if outControls populated, otherwise false.
bool smoothAltitude(const RideFile *ride, int degree, double &minSlope, double &maxSlope, std::vector<Vector2<double>> & outControls)
{
    outControls.resize(0);

    bool fHasAlt = ride && ride->areDataPresent()->alt;
    if (!fHasAlt)
        return false;

    // Spline undefined if degree is less than 3. Instead compute ridefile's min and max slope
    // for characterization display.
    if (degree < 3) {
        double sampleDistanceVariance = 0;
        ComputeRideFileStats(ride, minSlope, maxSlope, sampleDistanceVariance);
        return false;
    }

    // Gather distance/altitude pairs
    std::vector<Vector2<double>> inControls;
    for (int i = 0; i < ride->dataPoints().count(); i++) {
        const RideFilePoint * pi = (ride->dataPoints()[i]);

        inControls.push_back({ pi->km, pi->alt });
    }

    // Create b-spline curve of desired degree.
    BasisFunctionInput<double> inBasis((int)inControls.size(), degree);
    BSplineCurve<2, double> curve(inBasis, inControls.data());

    // Gather new altitudes for all incoming distances.
    bool fSuccess = InterpolateBSplineCurve(inControls, outControls, curve);
    if (!fSuccess) {
        // Translation failed to converge.
        return false;
    }

    // Determine min and max slope of smoothed distance/altitude pairs.
    minSlope = 0; maxSlope = 0;
    for (int i = 1; i < outControls.size(); i++) {
        double run = 1000*(outControls[i - 1][0] - outControls[i][0]);
        double rise = outControls[i - 1][1] - outControls[i][1];
        double slope = run ? (rise / run) : 0;
        slope *= 100;

        if (slope < minSlope) minSlope = slope;
        if (slope > maxSlope) maxSlope = slope;
    }

    return true;
}

bool FixGPS::postProcess(RideFile *ride, DataProcessorConfig *config, QString op)
{
    Q_UNUSED(config)
    Q_UNUSED(op)

    // ignore null or files without GPS data
    if (!ride || ride->areDataPresent()->lat == false || ride->areDataPresent()->lon == false)
        return false;

    // Interpolate altitude if its available.
    bool fHasAlt = ride->areDataPresent()->alt;

    int errors=0;

    ride->command->startLUW("Fix GPS Errors");

    GeoPointInterpolator gpi;
    int ii = 0; // interpolation input index

    for (int i=0; i<ride->dataPoints().count(); i++) {
        const RideFilePoint * pi = (ride->dataPoints()[i]);
        geolocation curLoc(pi->lat, pi->lon, fHasAlt ? pi->alt : 0.0);

        // Activate interpolation if this sample isn't reasonable.
        if (!IsReasonableGeoLocation(&curLoc)) {
            double km = pi->km;

            // Feed interpolator until it has samples that span current distance.
            while (gpi.WantsInput(km)) {
                if (ii < ride->dataPoints().count()) {
                    const RideFilePoint * pii = (ride->dataPoints()[ii]);
                    geolocation geo(pii->lat, pii->lon, fHasAlt ? pii->alt : 0.0);

                    // Only feed reasonable locations to interpolator
                    if (IsReasonableGeoLocation(&geo)) {
                        gpi.Push(pii->km, geo);
                    }
                    ii++;
                } else {
                    gpi.NotifyInputComplete();
                    break;
                }
            }

            geolocation interpLoc = gpi.Interpolate(km);

            ride->command->setPointValue(i, RideFile::lat, interpLoc.Lat());
            ride->command->setPointValue(i, RideFile::lon, interpLoc.Long());

            if (fHasAlt) {
                ride->command->setPointValue(i, RideFile::alt, interpLoc.Alt());
            }

            errors++;
        }
    }

    ride->command->endLUW();

    bool smoothingSuccess = false;

    bool fDoSmoothAltitude;
    int degree;
    if (config) {
        fDoSmoothAltitude = ((FixGPSConfig*)(config))->doSmoothAltitude->checkState();
        degree = ((FixGPSConfig*)(config))->degreeSpinBox->value();
    } else {
        fDoSmoothAltitude = appsettings->value(NULL, GC_FIXGPS_ALTITUDE_FIX_DOAPPLY, Qt::Unchecked).toBool();
        degree = appsettings->value(NULL, GC_FIXGPS_ALTITUDE_FIX_DEGREE, 200).toInt();
    }

    if (fDoSmoothAltitude) {
        double minSlope = 0;
        double maxSlope = 0;

        ride->command->startLUW("Smooth Altitude");

        std::vector<Vector2<double>> outControls;
        smoothingSuccess = smoothAltitude(ride, degree, minSlope, maxSlope, outControls);
        if (smoothingSuccess) {
            // Apply smoothed altitudes to ride file if requested.
            for (int i = 0; i < ride->dataPoints().count(); i++) {
                ride->command->setPointValue(i, RideFile::alt, outControls[i][1]);
            }
 
            ride->recalculateDerivedSeries(true);
        }

        ride->command->endLUW();
    }

    if (errors) {
        ride->setTag("GPS errors", QString("%1").arg(errors));
    }

    return errors || smoothingSuccess;
}
