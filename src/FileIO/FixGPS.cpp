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

struct AltitudeSmoothingStats
{
    double minSlope;
    double maxSlope;
    double avgSlope;
    double outlierCount;

    AltitudeSmoothingStats() : minSlope(0.), maxSlope(0.), outlierCount(0) {}
};

bool GatherForAltitudeSmoothing(const RideFile *ride, std::vector <Vector2<double>> &controls);
bool smoothAltitude(const std::vector<Vector2<double>> &inControls, unsigned degree0, double outlierCriteria, unsigned degree1, std::vector<Vector2<double>> & out, AltitudeSmoothingStats &stats);

struct RouteSmoothingStats
{
    double   pass1Variance;
    double   pass2Variance;
    unsigned outlierCount;

    RouteSmoothingStats() : pass1Variance(0.), pass2Variance(0.), outlierCount(0) {}
};

bool GatherForRouteSmoothing(const RideFile * ride, std::vector<Vector4<double>> &controls, const std::vector<Vector2<double>> smoothedAltitudes);
bool smoothRoute(const std::vector<Vector4<double>> &inControls, unsigned degree0, double outlierCriteria, unsigned degree1, std::vector<Vector4<double>> & out, RouteSmoothingStats &stats);

void ComputeRideFileStats(const RideFile * ride, AltitudeSmoothingStats & stats, double &sampleDistanceVariance)
{
    sampleDistanceVariance = 0;

    double slopeDistance = 0;

    const RideFilePoint * prevPi = NULL;
    double prevKM = 0;
    for (int i = 0; i < ride->dataPoints().count(); i++) {
        const RideFilePoint * pi = (ride->dataPoints()[i]);

        // Compute slope since ridefile slope is truncated.
        double slope = 0;
        if (i > 0) {
            double rise = pi->alt - prevPi->alt;
            double run = 10. * (pi->km - prevPi->km);
            slope = run ? (rise / run) : 0;
            slopeDistance += slope * run;

            stats.minSlope = std::min(slope, stats.minSlope);
            stats.maxSlope = std::max(slope, stats.maxSlope);
        }

        double kmDelta = pi->km - prevKM;
        prevKM = pi->km;

        sampleDistanceVariance += (1000 * kmDelta * 1000 * kmDelta);

        prevPi = pi;
    }

    double routeLength = 10. * ride->dataPoints()[ride->dataPoints().count() - 1]->km;
    stats.avgSlope = routeLength ? slopeDistance / routeLength : stats.avgSlope;

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

        // Altitude
        QCheckBox   *doSmoothAltitude;

        // Altitude smoothing degrees
        QLabel      *degree0Label;
        QSpinBox    *degree0SpinBox;
        QLabel      *degree1Label;
        QSpinBox    *degree1SpinBox;

        // Altitude Outlier Criteria
        QLabel      *outlierLabel;
        QDoubleSpinBox *outlierSpinBox;

        // Altitude Stats
        QLabel      *minSlopeLabel;
        QLabel      *maxSlopeLabel;
        QLabel      *avgSlopeLabel;
        QLabel      *outlierCountLabel;

        // Route
        QCheckBox   *doSmoothRoute;

        // Route smoothing degrees
        QLabel      *degree0LabelRoute;
        QLabel      *degree1LabelRoute;
        QSpinBox    *degree0SpinBoxRoute;
        QSpinBox    *degree1SpinBoxRoute;

        // Route Outlier Criteria
        QLabel      *outlierLabelRoute;
        QDoubleSpinBox *outlierSpinBoxRoute;

        // Route Stats
        QLabel      *variance0LabelRoute;
        QLabel      *variance1LabelRoute;
        QLabel      *outlierCountLabelRoute;

        QPushButton *testButton;
        QLabel      *varianceLabel;
        const RideFile *ride;

        // Defaults:
        static const int s_Default_AltitudeDegree0        = 500;
        static const int s_Default_AltitudeDegree1        = 200;
        static const int s_Default_AltitudeOutlierPercent = 10;
        static const int s_Default_RouteDegree0           = 20;
        static const int s_Default_RouteDegree1           = 10;
        static const int s_Default_RouteOutlierPercent    = 10;

    public slots:
        void testClicked()
        {
            // This function should be unreachable unless ride exists.
            if (ride)
            {
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                testButton->setEnabled(false);
                
                // PASS 0: ALTITUDE SMOOTHING TEST

                AltitudeSmoothingStats altitudeSmoothingStats;
                unsigned outlierCount = 0;
                double sampleDistanceVariance = 0;

                std::vector<Vector2<double>> inControls2, outControls2;
                bool fAltitudeSmoothingSuccess = false;

                bool fDoSmoothAltitude = doSmoothAltitude->checkState();
                if (fDoSmoothAltitude)
                {
                    unsigned degree0 = degree0SpinBox->value();
                    unsigned degree1 = degree1SpinBox->value();

                    double outlierCriteria = outlierSpinBox->value() / 100.;

                    // Test and report effectiveness of smoothing but do not apply changes to ride file.
                    bool fGathered = GatherForAltitudeSmoothing(ride, inControls2);
                    if (fGathered) {
                        fAltitudeSmoothingSuccess = smoothAltitude(inControls2, degree0, outlierCriteria, degree1, outControls2, altitudeSmoothingStats);
                    }
                } else {
                    // Otherwise no smoothing, simply compute the ride stats.
                    outlierCount = 0;
                    ComputeRideFileStats(ride, altitudeSmoothingStats, sampleDistanceVariance);
                }

                // PASS 1: ROUTE SMOOTHING

                bool fDoSmoothRoute = doSmoothRoute->checkState();
                RouteSmoothingStats routeSmoothingStats;
                if (fDoSmoothRoute)
                {
                    unsigned degree0Route = degree0SpinBoxRoute->value();
                    unsigned degree1Route = degree0SpinBoxRoute->value();

                    double outlierCriteriaRoute = outlierSpinBoxRoute->value() / 100.;

                    // Test and report effectiveness of smoothing but do not apply changes to ride file.
                    std::vector<Vector4<double>> inControls4, outControls4;

                    bool fGathered = GatherForRouteSmoothing(ride, inControls4, outControls2);
                    if (fGathered) {
                        smoothRoute(inControls4, degree0Route, outlierCriteriaRoute, degree1Route, outControls4, routeSmoothingStats);

                        // Regenerate min and max slope from route smoothing output
                        altitudeSmoothingStats.minSlope = 0;
                        altitudeSmoothingStats.maxSlope = 0;
                        double slopeDistance = 0;
                        geolocation prevGeo;
                        for (int i = 0; i < outControls4.size(); i++) {
                            xyz loc(outControls4[i][1], outControls4[i][2], outControls4[i][3]);
                            geolocation geo = loc.togeolocation();

                            if (i > 0) {
                                double rise = geo.Alt() - prevGeo.Alt();
                                double run = 10 * (outControls4[i][0] - outControls4[i - 1][0]);
                                double slope = run ? (rise / run) : 0;

                                altitudeSmoothingStats.minSlope = std::min(slope, altitudeSmoothingStats.minSlope);
                                altitudeSmoothingStats.maxSlope = std::max(slope, altitudeSmoothingStats.maxSlope);
                                slopeDistance += (slope * run);
                            }
                            prevGeo = geo;
                        }
                        double routeDistance = 10. * outControls4[outControls4.size() - 1][0];
                        altitudeSmoothingStats.avgSlope = routeDistance ? slopeDistance / routeDistance : altitudeSmoothingStats.avgSlope;
                    }
                }

                QString minLabel(tr("Min Slope"));
                QString maxLabel(tr("Max Slope"));
                QString avgLabel(tr("Avg Slope"));
                QString outlierLabel(tr("OutlierCount"));

                minLabel.append(QString(": %1").arg(altitudeSmoothingStats.minSlope));
                maxLabel.append(QString(": %1").arg(altitudeSmoothingStats.maxSlope));
                avgLabel.append(QString(": %1").arg(altitudeSmoothingStats.avgSlope));
                outlierLabel.append(QString(": %1").arg(altitudeSmoothingStats.outlierCount));

                minSlopeLabel->setText(minLabel);
                minSlopeLabel->setToolTip(tr("Min slope after smoothing applied."));

                maxSlopeLabel->setText(maxLabel);
                maxSlopeLabel->setToolTip(tr("Max slope after smoothing applied."));

                avgSlopeLabel->setText(avgLabel);
                avgSlopeLabel->setToolTip(tr("Avg slope after smoothing applied."));

                outlierCountLabel->setText(outlierLabel);
                outlierCountLabel->setToolTip(tr("Count of outliers found during altitude smoothing."));

                QString pass1VarianceLabelString(tr("Route Smoothing Pass1 Variance"));
                QString pass2VarianceLabelString(tr("Route Smoothing Pass2 Variance"));
                QString outlierRouteLabelString (tr("Route Smoothing Outlier Count"));

                pass1VarianceLabelString.append(QString(": %1").arg(routeSmoothingStats.pass1Variance));
                pass2VarianceLabelString.append(QString(": %1").arg(routeSmoothingStats.pass2Variance));
                outlierRouteLabelString.append (QString(": %1").arg(routeSmoothingStats.outlierCount));

                variance0LabelRoute->setText(pass1VarianceLabelString);
                variance1LabelRoute->setText(pass2VarianceLabelString);
                outlierCountLabelRoute->setText(outlierRouteLabelString);

                testButton->setEnabled(true);
                QApplication::restoreOverrideCursor();
            }
        }

    public:
        FixGPSConfig(QWidget *parent, const RideFile * rideFile) : DataProcessorConfig(parent) {

            mainLayout = NULL;
            simpleLayout = NULL;
            layout = NULL;

            // Altitude
            doSmoothAltitude = NULL;

            // Altitude smoothing degrees
            degree0Label = NULL;
            degree0SpinBox = NULL;
            degree1Label = NULL;
            degree1SpinBox = NULL;

            // Altitude Outlier Criteria
            outlierLabel = NULL;
            outlierSpinBox = NULL;

            // Altitude Stats
            minSlopeLabel = NULL;
            maxSlopeLabel = NULL;
            avgSlopeLabel = NULL;
            outlierCountLabel = NULL;

            // Route
            doSmoothRoute = NULL;

            // Route smoothing degrees
            degree0LabelRoute = NULL;
            degree1LabelRoute = NULL;
            degree0SpinBoxRoute = NULL;
            degree1SpinBoxRoute = NULL;

            // Route Outlier Criteria
            outlierLabelRoute = NULL;
            outlierSpinBoxRoute = NULL;

            // Route Stats
            variance0LabelRoute = NULL;
            variance1LabelRoute = NULL;
            outlierCountLabelRoute = NULL;

            testButton = NULL;
            varianceLabel = NULL;

            ride = rideFile;

            HelpWhatsThis *help = new HelpWhatsThis(parent);
            parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FixGPSErrors));

            // In import config there's no room for verbosity.
            bool fUseShortDescription = (rideFile == NULL);


            // ALTITUDE SMOOTHING CONTROLS

            // Apply Altitude Smoothing Checkbox

            doSmoothAltitude = new QCheckBox(fUseShortDescription ? tr("Altitude") : tr("Apply BSpline Altitude Smoothing"));
            doSmoothAltitude->setToolTip(tr("Apply B-Spline based altitude smoothing after running the GPS outlier pass."));
            doSmoothAltitude->setCheckState(appsettings->value(NULL, GC_FIXGPS_ALTITUDE_FIX_DOAPPLY, Qt::Unchecked).toBool() ? Qt::Checked : Qt::Unchecked);

            // Altitude Degree 0 SpinBox - First Pass

            degree0Label= new QLabel(fUseShortDescription ? tr("P1 Deg") : tr("Pass 1 Altitude Smoothing Degree:"));
            degree0SpinBox = new QSpinBox();
            degree0SpinBox->setRange(0, 1000);
            degree0SpinBox->setSingleStep(10);
            degree0SpinBox->setValue(appsettings->value(this, GC_FIXGPS_ALTITUDE_FIX_DEGREE, s_Default_AltitudeDegree0).toInt());
            degree0SpinBox->setToolTip(tr("ALTITUDE PASS 1 Smoothing Degree:\n"
                                          "The 'degree' of a b-spline is the number of samples that are\n"
                                          "used to compute each point. Smoothing becomes more global\n"
                                          "as 'degree' increases. Degree is in terms of samples so exact\n"
                                          "window size in meters depends on the sample separation in\n"
                                          "the ridefile and can vary by sample. This is the initial spline\n"
                                          "degree prior to outlier removal.\n"));

            // Altitude Degree 1 SpinBox - Second Pass

            degree1Label = new QLabel(fUseShortDescription ? tr("P2 Deg") : tr("Pass 2 Altitude Smoothing Degree:"));
            degree1SpinBox = new QSpinBox();
            degree1SpinBox->setRange(0, 1000);
            degree1SpinBox->setSingleStep(10);
            degree1SpinBox->setValue(appsettings->value(this, GC_FIXGPS_ALTITUDE_FIX_DEGREE1, s_Default_AltitudeDegree1).toInt());
            degree1SpinBox->setToolTip(tr("ALTITUDE PASS 2 Smoothing Degree:\n"
                                          "The 'degree' of a b-spline is the number of samples that are\n"
                                          "used to compute each point. Smoothing becomes more global\n"
                                          "as 'degree' increases. Degree is in terms of samples so exact\n"
                                          "window size in meters depends on the sample separation in\n"
                                          "the ridefile and can vary by sample. This is the degree to be\n"
                                          "used for the second pass spline made after outliers are removed.\n"));

            // Altitude Outlier Criteria

            outlierLabel = new QLabel(fUseShortDescription ? tr("Crit") : tr("Altitude Outlier criteria %:"));
            outlierSpinBox = new QDoubleSpinBox();
            outlierSpinBox->setRange(0.001, 10000);
            outlierSpinBox->setSingleStep(10);
            outlierSpinBox->setDecimals(3);
            outlierSpinBox->setValue(appsettings->value(this, GC_FIXGPS_ALTITUDE_OUTLIER_PERCENT, s_Default_AltitudeOutlierPercent).toInt());
            outlierSpinBox->setToolTip(tr("ALTITUDE OUTLIER CRITERIA (%)\n"
                                          "Outlier percent is used to decide outlier points that will be discarded after\n"
                                          "initial bspline is constructed. This value is the percent of variance between\n"
                                          "original and bspline where a value will be considered an outlier.\n"));

            // ROUTE SMOOTHING CONTROLS

            // Apply Route Smoothing Checkbox

            doSmoothRoute = new QCheckBox(fUseShortDescription ? tr("Route") : tr("Apply BSpline Route Smoothing"));
            doSmoothRoute->setToolTip(tr("Apply B-Spline based location smoothing after running the altitude smoothing pass."));
            doSmoothRoute->setCheckState(appsettings->value(NULL, GC_FIXGPS_ROUTE_FIX_DOAPPLY, Qt::Unchecked).toBool() ? Qt::Checked : Qt::Unchecked);

            // Route Degree 0 SpinBox - PASS 1

            degree0LabelRoute= new QLabel(fUseShortDescription ? tr("P1 Deg") : tr("Pass 1 Route Smoothing Degree:"));
            degree0SpinBoxRoute = new QSpinBox();
            degree0SpinBoxRoute->setRange(0, 1000);
            degree0SpinBoxRoute->setSingleStep(10);
            degree0SpinBoxRoute->setValue(appsettings->value(this, GC_FIXGPS_ROUTE_FIX_DEGREE, s_Default_RouteDegree0).toInt());
            degree0SpinBoxRoute->setToolTip(tr("ROUTE PASS 1 Smoothing Degree:\n"
                                               "The 'degree' of a b-spline is the number of samples that are\n"
                                               "used to compute each point. Smoothing becomes more global\n"
                                               "as 'degree' increases. Degree is in terms of samples so exact\n"
                                               "window size in meters depends on the sample separation in\n"
                                               "the ridefile and can vary by sample. This is the initial spline\n"
                                               "degree prior to outlier removal.\n"));

            degree1LabelRoute = new QLabel(fUseShortDescription ? tr("P2 Deg") : tr("Pass 2 Route Smoothing Degree:"));
            degree1SpinBoxRoute = new QSpinBox();
            degree1SpinBoxRoute->setRange(0, 1000);
            degree1SpinBoxRoute->setSingleStep(10);
            degree1SpinBoxRoute->setValue(appsettings->value(this, GC_FIXGPS_ROUTE_FIX_DEGREE1, s_Default_RouteDegree1).toInt());
            degree1SpinBoxRoute->setToolTip(tr("ROUTE PASS 2 Smoothing Degree:\n"
                                               "The 'degree' of a b-spline is the number of samples that are\n"
                                               "used to compute each point. Smoothing becomes more global\n"
                                               "as 'degree' increases. Degree is in terms of samples so exact\n"
                                               "window size in meters depends on the sample separation in\n"
                                               "the ridefile and can vary by sample. This is the degree to be\n"
                                               "used for the second pass spline made after outliers are removed.\n"));

            outlierLabelRoute = new QLabel(fUseShortDescription ? tr("Crit") : tr("Route Outlier criteria %:"));
            outlierSpinBoxRoute = new QDoubleSpinBox();
            outlierSpinBoxRoute->setRange(0.001, 10000);
            outlierSpinBoxRoute->setSingleStep(10);
            outlierSpinBoxRoute->setDecimals(3);
            outlierSpinBoxRoute->setValue(appsettings->value(this, GC_FIXGPS_ROUTE_OUTLIER_PERCENT, s_Default_RouteOutlierPercent).toInt());
            outlierSpinBoxRoute->setToolTip(tr("ROUTE OUTLIER CRITERIA (%)"
                                               "Outlier percent is used to decide outlier points that will be discarded after\n"
                                               "initial bspline is constructed. This value is the percent of variance between\n"
                                               "original and bspline where a value will be considered an outlier.\n"));

            // If no ridefile is provided then present simple dialog
            if (rideFile == NULL)
            {
                simpleLayout = new QHBoxLayout(this);

                simpleLayout->addWidget(doSmoothAltitude);
                simpleLayout->addWidget(degree0Label);
                simpleLayout->addWidget(degree0SpinBox);
                simpleLayout->addWidget(degree1Label);
                simpleLayout->addWidget(degree1SpinBox);
                simpleLayout->addWidget(outlierLabel);
                simpleLayout->addWidget(outlierSpinBox);

                simpleLayout->addWidget(doSmoothRoute);
                simpleLayout->addWidget(degree0LabelRoute);
                simpleLayout->addWidget(degree0SpinBoxRoute);
                simpleLayout->addWidget(degree1LabelRoute);
                simpleLayout->addWidget(degree1SpinBoxRoute);
                simpleLayout->addWidget(outlierLabelRoute);
                simpleLayout->addWidget(outlierSpinBoxRoute);

                simpleLayout->setContentsMargins(0, 0, 0, 0);
                setContentsMargins(0, 0, 0, 0);

                simpleLayout->addStretch();
            }
            else
            {
                // Determine min and max slope of original ride file.
                AltitudeSmoothingStats altitudeSmoothingStats;
                double routePass1Variance = 0;
                double routePass2Variance = 0;
                double routeOutlierCount = 0;
                double sampleDistanceVariance = 0;
                bool fHasAlt = ride && ride->areDataPresent()->alt;
                if (fHasAlt) {
                    ComputeRideFileStats(ride, altitudeSmoothingStats, sampleDistanceVariance);
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
                QString avgSlopeLabelString(tr("Avg Slope"));
                QString outlierLabelString(tr("Outlier Count"));
                QString varianceLabelString(tr("Distance Variance"));

                minSlopeLabelString.append(QString(": %1").arg(altitudeSmoothingStats.minSlope));
                maxSlopeLabelString.append(QString(": %1").arg(altitudeSmoothingStats.maxSlope));
                avgSlopeLabelString.append(QString(": %1").arg(altitudeSmoothingStats.avgSlope));
                outlierLabelString. append(QString(": %1").arg(altitudeSmoothingStats.outlierCount));
                varianceLabelString.append(QString(": %1").arg(sampleDistanceVariance));

                minSlopeLabel = new QLabel(minSlopeLabelString);
                minSlopeLabel->setToolTip(tr("Min slope computed from ride file altitude and distance information."));
                maxSlopeLabel = new QLabel(maxSlopeLabelString);
                maxSlopeLabel->setToolTip(tr("Max slope computed from ride file altitude and distance information."));
                avgSlopeLabel = new QLabel(avgSlopeLabelString);
                avgSlopeLabel->setToolTip(tr("Avg slope computed from ride file altitude and distance information."));
                outlierCountLabel = new QLabel(outlierLabelString);
                outlierCountLabel->setToolTip(tr("Count of outliers found."));
                varianceLabel = new QLabel(varianceLabelString);
                varianceLabel->setToolTip(tr("Variance of distance between ride file samples."));

                // Route Stats

                QString pass1VarianceLabelString(tr("Route Smoothing Pass1 Variance"));
                QString pass2VarianceLabelString(tr("Route Smoothing Pass2 Variance"));
                QString outlierRouteLabelString (tr("Route SmoothingOutlier Count"));

                pass1VarianceLabelString.append(QString(": %1").arg(routePass1Variance));
                pass2VarianceLabelString.append(QString(": %1").arg(routePass2Variance));
                outlierRouteLabelString. append(QString(": %1").arg(routeOutlierCount));

                variance0LabelRoute = new QLabel(pass1VarianceLabelString);
                variance0LabelRoute->setToolTip(tr("Variance between original samples and pass 1 output."));
                variance1LabelRoute = new QLabel(pass2VarianceLabelString);
                variance1LabelRoute->setToolTip(tr("Variance between pass 1 output and pass 2 output."));
                outlierCountLabelRoute = new QLabel(outlierRouteLabelString);
                outlierCountLabelRoute->setToolTip(tr("Count of outliers discarded prior to pass 2."));

                // Test Smoothing Button

                testButton = new QPushButton(tr("Test Current Smoothing Setup"), this);
                testButton->setToolTip(tr("Click this button to simulate behavior of current smoothing settings and update the statistics. NOTE: This simulation does not perform the Pass 0 outlier removal."));

                // Create Rows of Widgets

                std::vector<QHBoxLayout*> rows;

                QHBoxLayout* row;
                
                // Altitude Smoothing
                
                row = new QHBoxLayout();
                row->addWidget(doSmoothAltitude);
                rows.push_back(row);

                row = new QHBoxLayout();
                row->addWidget(degree0Label);
                row->addWidget(degree0SpinBox);
                rows.push_back(row);

                row = new QHBoxLayout();
                row->addWidget(outlierLabel);
                row->addWidget(outlierSpinBox);
                row->addWidget(degree1Label);
                row->addWidget(degree1SpinBox);
                rows.push_back(row);

                // Route Smoothing

                row = new QHBoxLayout();
                row->addWidget(doSmoothRoute);
                rows.push_back(row);

                row = new QHBoxLayout();
                row->addWidget(degree0LabelRoute);
                row->addWidget(degree0SpinBoxRoute);
                rows.push_back(row);

                row = new QHBoxLayout();
                row->addWidget(outlierLabelRoute);
                row->addWidget(outlierSpinBoxRoute);
                row->addWidget(degree1LabelRoute);
                row->addWidget(degree1SpinBoxRoute);
                rows.push_back(row);

                // Test Button

                row = new QHBoxLayout();
                row->addWidget(testButton);
                rows.push_back(row);

                // Altitude Stats

                row = new QHBoxLayout();
                row->addWidget(minSlopeLabel);
                row->addWidget(maxSlopeLabel);
                row->addWidget(avgSlopeLabel);
                row->addWidget(outlierCountLabel);
                row->addWidget(varianceLabel);
                rows.push_back(row);

                // Route Stats

                row = new QHBoxLayout();
                row->addWidget(variance0LabelRoute);
                row->addWidget(variance1LabelRoute);
                row->addWidget(outlierCountLabelRoute);
                rows.push_back(row);

                // Insert rows into layout
                for (int i = 0; i < rows.size(); i++)
                    layout->insertRow(i, rows[i]);

                // Hook up testButton to testClicked method.

                connect(testButton, &QPushButton::clicked, this, &FixGPSConfig::testClicked);
            }
        }
        
        QString explain() {
            return(QString(tr("Multiple Pass GPS Repair:\n"
                              "0 - Always: Remove GPS errors and interpolate positional\n"
                              "    data where the GPS device did not record any data,\n"
                              "    or the data that was recorded is invalid.\n"
                              "1 - Optional: Altitude B-Spline smoothing will be applied\n"
                              "    if checkbox is set. This is potentially two pass smoothing.\n"
                              "    Spline is built with Pass 1 Degree, any original samples\n"
                              "    that fail outlier criteria are discarded and then a final\n"
                              "    smoothing is run with Pass 2 degree.\n"
                              "2 - Optional: Route B-Spline smoothing will be applied\n"
                              "    if checkbox is set. Again this is two pass smoothing where\n"
                              "    outliers are determined by euclidean distance variance from\n"
                              "    smoothing spline. Again a final pass is run without outliers\n"
                              "    using Pass 2 degree.\n\n"
                              "Generally altitude data is noisiest and requires highest degree for\n"
                              "reasonable smoothness. Route gps data gnerally requires a much\n"
                              "lighter touch.\n")));
        }

        void readConfig() {
            doSmoothAltitude->setCheckState(appsettings->value(NULL, GC_FIXGPS_ALTITUDE_FIX_DOAPPLY, Qt::Unchecked).toBool() ? Qt::Checked : Qt::Unchecked);
            degree0SpinBox->setValue(appsettings->value(this, GC_FIXGPS_ALTITUDE_FIX_DEGREE,        s_Default_AltitudeDegree0).toInt());
            degree1SpinBox->setValue(appsettings->value(this, GC_FIXGPS_ALTITUDE_FIX_DEGREE1,       s_Default_AltitudeDegree1).toInt());
            outlierSpinBox->setValue(appsettings->value(this, GC_FIXGPS_ALTITUDE_OUTLIER_PERCENT,   s_Default_AltitudeOutlierPercent).toInt());

            doSmoothRoute->setCheckState(appsettings->value(NULL,  GC_FIXGPS_ROUTE_FIX_DOAPPLY, Qt::Unchecked).toBool() ? Qt::Checked : Qt::Unchecked);
            degree0SpinBoxRoute->setValue(appsettings->value(this, GC_FIXGPS_ROUTE_FIX_DEGREE,      s_Default_RouteDegree0).toInt());
            degree1SpinBoxRoute->setValue(appsettings->value(this, GC_FIXGPS_ROUTE_FIX_DEGREE1,     s_Default_RouteDegree1).toInt());
            outlierSpinBoxRoute->setValue(appsettings->value(this, GC_FIXGPS_ROUTE_OUTLIER_PERCENT, s_Default_RouteOutlierPercent).toInt());
        }

        void saveConfig() {
            appsettings->setValue(GC_FIXGPS_ALTITUDE_FIX_DOAPPLY,     doSmoothAltitude->checkState());
            appsettings->setValue(GC_FIXGPS_ALTITUDE_FIX_DEGREE,      degree0SpinBox->value());
            appsettings->setValue(GC_FIXGPS_ALTITUDE_FIX_DEGREE1,     degree1SpinBox->value());
            appsettings->setValue(GC_FIXGPS_ALTITUDE_OUTLIER_PERCENT, outlierSpinBox->value());

            appsettings->setValue(GC_FIXGPS_ROUTE_FIX_DOAPPLY,        doSmoothRoute->checkState());
            appsettings->setValue(GC_FIXGPS_ROUTE_FIX_DEGREE,         degree0SpinBoxRoute->value());
            appsettings->setValue(GC_FIXGPS_ROUTE_FIX_DEGREE1,        degree1SpinBoxRoute->value());
            appsettings->setValue(GC_FIXGPS_ROUTE_OUTLIER_PERCENT,    outlierSpinBoxRoute->value());
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

class SaveState
{
    double m_t;
    double m_step;
    double m_achieved;
    bool   m_fGoingUp;

public:

    SaveState() : m_t(0.), m_step(0.), m_achieved(1000000.), m_fGoingUp(true) {};

    void Save(double t, double step, double achieved, bool fGoingUp) {
        m_t = t;
        m_step = step;
        m_achieved = achieved;
        m_fGoingUp = fGoingUp;
    }

    void Restore(double& t, double& step, double &achieved, bool& fGoingUp) {
        t = m_t;
        step = m_step;
        fGoingUp = m_fGoingUp;
        achieved = m_achieved;
    }
};

class GradientPrecisionHistory
{
    static const size_t s_HistSize = 4;
    double    precisionHistory[s_HistSize];
    SaveState saveHistory[s_HistSize];

public:

    GradientPrecisionHistory() {
        for (int i = 0; i < s_HistSize; i++) precisionHistory[i] = 1000000.;
    }

    void GetBestState(SaveState& state) {
        int lowestIdx = 0;
        double lowestValue = precisionHistory[lowestIdx];
		for (int i = 1; i < s_HistSize; i++) {
            double val = precisionHistory[i];
            if (val < lowestValue)
            {
                lowestIdx = i;
                lowestValue = precisionHistory[i];
            }
        }

        state = saveHistory[lowestIdx];
    }

    bool Push(double newDelta, SaveState state) {
        int highestIdx = 0;
        double highestValue = precisionHistory[highestIdx];

        for (int i = 1; i < s_HistSize; i++) {
            double val = precisionHistory[i];
            if (val > highestValue) {
                highestValue = val;
                highestIdx = i;
            }
        }

        // Only dealing in magnitudes here.
        newDelta = fabs(newDelta);

		// Making progress if new value is less than the biggest value in history.
		if (newDelta < highestValue) {
			precisionHistory[highestIdx] = newDelta;
			saveHistory[highestIdx] = state;
			return true;;
		}

        return false;
    }
};

// Gradient descent requires 2x the compute per evaluation but usually seens 7x fewer evalutaions,
// so > 3x speedup.
// Returns delta from epsilon, 0. if perfect resolution.
template <typename T_Curve, typename T_Pos, bool T_GradientDescent>
double EvalAtTargetDistance(T_Curve &curve, double targetDistance, double epsilon, unsigned sampleCount, double &prevT, T_Pos &pos, unsigned &evalCount)
{
    static const int s_iGradientDegree = T_GradientDescent ? 1 : 0;

    double t = prevT;
    double step = std::max(0.1, 1 / (double)sampleCount); // default step size if gradient descent isn't used.
    bool fGoingUp = true;
    bool fGradientPossible = true;
    double achievedPrecision = 1000000.;
    GradientPrecisionHistory gph;
    SaveState gss;

    gss.Save(t, step, achievedPrecision,fGoingUp);

    while (true) {
        evalCount++;

        // Fail if no convegence after 100 tries.
        if (evalCount > 100)
            return achievedPrecision;

        T_Pos evalResults[1 + s_iGradientDegree];

        curve.Evaluate(t, s_iGradientDegree, evalResults);

        pos = evalResults[0];
        achievedPrecision = pos[0] - targetDistance;
        if (fabs(achievedPrecision) < epsilon) {
            prevT = t;
            break;
        }

        bool fSuccess = false;
        if (T_GradientDescent && fGradientPossible) {

            if (!gph.Push(achievedPrecision, gss)) {
                fGradientPossible = false;
            } else {
                // Try to use gradient to predict next sample point.
                double evalRise = evalResults[1][0];

                // Abandon gradient descent if slope is too close to zero.
                if (evalRise == 0.) {
                    fGradientPossible = false;
                }
                else {
                    int exp;
                    std::frexp(evalRise, &exp);
                    if (exp < -10) {
                        fGradientPossible = false;
                    }
                }

                if (fGradientPossible) {
                    double evalStep = -(achievedPrecision / evalRise);

                    // Gradient descent can fail if unlucky.
                    // If suggested next sample point is out of range then fall back
                    // to binary search.
                    if (t + evalStep >= 0. && t + evalStep <= 1.) {

                        gss.Save(t, step, achievedPrecision, fGoingUp);

                        step = evalStep;
                        fGoingUp = (step > 0);
                        fSuccess = true;
                    }
                }
            }

            // Restore previos step if gradient descent ran off the rails.
            if (!fGradientPossible) {
                gph.GetBestState(gss);
                gss.Restore(t, step, achievedPrecision, fGoingUp);
            }
        }

        if (!fSuccess) {
            bool directionChange = (achievedPrecision > 0) == fGoingUp;
            if (directionChange) {
                step /= -2.;
                fGoingUp = !fGoingUp;
            }
        }

        t += step;
    }

    return achievedPrecision;
}

// Create new distance/altitude curve
//
// Rule for T_Pos: its first element [0] must be monotonically increasing distance.
template <typename T_Curve, typename T_Pos>
bool InterpolateBSplineCurve(const std::vector<T_Pos> &inControls, std::vector<T_Pos> &outControls, T_Curve &curve, unsigned &evalCountSum)
{
    // Tell eval it can stop if it finds this precision.
    static const double s_desiredPrecision = 0.0001; // 10cm

    // Fail eval if required precision isn't achieved.
    static const double s_requiredPrecision = 0.001; // 1m.

    outControls.resize(0);

    double t = 0;
    for (unsigned u = 0; u < inControls.size(); u++) {
        double targetDistance = inControls[u][0];

        T_Pos pos;
        unsigned evalCount = 0;
        double obtainedPrecision = EvalAtTargetDistance<T_Curve, T_Pos, true>(curve, targetDistance, s_desiredPrecision, (unsigned)inControls.size(), t, pos, evalCount);
        evalCountSum += evalCount;

        // Fail if required precision not met.
        if (fabs(obtainedPrecision) > s_requiredPrecision) {
            outControls.resize(0);
            return false;
        }

        outControls.push_back(pos);
    }

    return true;
}

bool GatherForAltitudeSmoothing(const RideFile *ride, std::vector < Vector2<double>> &controls)
{
    controls.resize(0);

    bool fHasAlt = ride && ride->areDataPresent()->alt;
    if (!fHasAlt)
        return false;

    // Gather distance/altitude pairs
    for (int i = 0; i < ride->dataPoints().count(); i++) {
        const RideFilePoint * pi = (ride->dataPoints()[i]);

        controls.push_back({ pi->km, pi->alt });
    }

    return true;
}

// Smooth ridefile altitude. Return true if outControls populated, otherwise false.
bool smoothAltitude(const std::vector<Vector2<double>> &inControls, unsigned degree0, double outlierCriteria, unsigned degree1, std::vector<Vector2<double>> & outControls, AltitudeSmoothingStats &stats)
{
    stats.minSlope = 0;
    stats.maxSlope = 0;
    stats.outlierCount = 0;

    outControls.resize(0);

    // Spline undefined if degree is less than 3.
    if (degree0 < 3) return false;

    // First thing, check for monotonaity. This smoothing
    // cannot work if distance can decrease. User should
    // recompute distance before try again.
    double dist = 0.;
    for (int i = 0; i < inControls.size(); i++) {
        double newDist = inControls[i][0];
        if (newDist < dist) {
            return false;
        }
        dist = newDist;
    }

    // Degree can't exceed control size - 1.
    degree0 = std::min(degree0, (unsigned)inControls.size() - 1);

    BasisFunctionInput<double> inBasis((int)inControls.size(), degree0);
    typedef BSplineCurve<2, double> DistanceAltitudeBSplineCurve;
    DistanceAltitudeBSplineCurve curve(inBasis, inControls.data());

    // Gather new altitudes for all incoming distances.
    unsigned evalCount = 0;
    bool fSuccess = InterpolateBSplineCurve<DistanceAltitudeBSplineCurve, Vector2<double>>(inControls, outControls, curve, evalCount);
    if (!fSuccess) {
        // Translation failed to converge.
        return false;
    }

    // No second pass if its smoothing degree too small.
    if (degree1 >= 3) {
        // Push non-outliers to new input vector
        std::vector <Vector2<double>> inControls2;
        for (int i = 0; i < outControls.size(); i++) {
            double d = inControls[i][1] - outControls[i][1];
            if (outlierCriteria > (d*d)) {
                inControls2.push_back(inControls[i]);
            }
        }

        // If there are outliers or second pass degree is different than first:
        // Create new bspline from non-outliers and redo interpolation.
        stats.outlierCount = (unsigned)(inControls.size() - inControls2.size());
        if (stats.outlierCount || (degree0 != degree1)) {
            const std::vector <Vector2<double>> &pass2InControls = (inControls2.size()) ? inControls2 : inControls;

            // Create b-spline curve of desired degree using non-outliers.
            degree1 = std::min(degree1, (unsigned)pass2InControls.size() - 1);
            BasisFunctionInput<double> inBasis2((int)pass2InControls.size(), degree1);
            BSplineCurve<2, double> curve2(inBasis2, pass2InControls.data());

            // Gather new altitudes for all incoming distances using original inControls sample distances.
            bool fSuccess2 = InterpolateBSplineCurve<DistanceAltitudeBSplineCurve, Vector2<double>>(inControls, outControls, curve2, evalCount);
            if (!fSuccess2) {
                // Translation failed to converge.
                return false;
            }
        }
    }

    // Determine min, max, avg slope of smoothed distance/altitude pairs.
    double slopeDistance = 0;
    for (int i = 1; i < outControls.size(); i++) {
        double run = 10*(outControls[i][0] - outControls[i-1][0]);
        double rise = outControls[i][1] - outControls[i-1][1];
        double slope = run ? (rise / run) : 0;

        stats.minSlope = std::min(stats.minSlope, slope);
        stats.maxSlope = std::max(stats.maxSlope, slope);

        slopeDistance += slope * run;
    }

    double routeLength = 10. * outControls[outControls.size() - 1][0];
    stats.avgSlope = routeLength ? slopeDistance / routeLength : stats.avgSlope;

    return true;
}

double ComputeLocationResultVariance(std::vector<Vector4<double>> inControls, std::vector<Vector4<double>> outControls)
{
    double var = 0;
    for (int i = 0; i < outControls.size(); i++) {
        xyz in (inControls [i][1], inControls [i][2], inControls [i][3]);
        xyz out(outControls[i][1], outControls[i][2], outControls[i][3]);

        double d = out.subtract(in).magnitude();
        var += (d*d);
    }
    var /= outControls.size();

    return var;
}

bool GatherForRouteSmoothing(const RideFile * ride, std::vector<Vector4<double>> &controls, const std::vector<Vector2<double>> smoothedAltitudes)
{
    bool fHasLoc = ride && ride->areDataPresent()->lat && ride->areDataPresent()->lon && ride->areDataPresent()->alt;
    if (!fHasLoc)
        return false;

    bool fUseSmoothedAltitudes = (ride->dataPoints().count() == smoothedAltitudes.size());

    // Convert geo to xyz and gather {distance,x,y,z} quads
    for (int i = 0; i < ride->dataPoints().count(); i++) {
        const RideFilePoint * pi = (ride->dataPoints()[i]);

        double lat = pi->lat;
        double lon = pi->lon;
        double alt = (fUseSmoothedAltitudes) ? smoothedAltitudes[i][1] : pi->alt;

        geolocation geoloc(lat, lon, alt);
        xyz c = geoloc.toxyz();

        controls.push_back({ pi->km, c.x(), c.y(), c.z() });
    }
    return true;
}

// Smooth ridefile location data. Return true if outControls populated, otherwise false.
bool smoothRoute(const std::vector<Vector4<double>> &inControls, unsigned degree0, double outlierCriteria, unsigned degree1, std::vector<Vector4<double>> & outControls, RouteSmoothingStats &routeSmoothingStats)
{
    routeSmoothingStats.pass1Variance = 0;
    routeSmoothingStats.pass2Variance = 0;
    routeSmoothingStats.outlierCount = 0;

    outControls.resize(0);

    if (degree0 < 3) return false;

    // Create b-spline curve of desired degree.
    degree0 = std::min(degree0, (unsigned)(inControls.size() - 1));
    BasisFunctionInput<double> inBasis((int)inControls.size(), degree0);
    typedef BSplineCurve<4, double> DistanceXYZBSplineCurve;
    DistanceXYZBSplineCurve curve(inBasis, inControls.data());

    // Gather new altitudes for all incoming distances.
    unsigned evalCount = 0;
    bool fSuccess = InterpolateBSplineCurve<DistanceXYZBSplineCurve, Vector4<double>>(inControls, outControls, curve, evalCount);
    if (!fSuccess) {
        // Translation failed to converge.
        return false;
    }

    routeSmoothingStats.pass1Variance = ComputeLocationResultVariance(inControls, outControls);

    // No second pass if its smoothing degree too small.
    if (degree1 >= 3) {
        // Push non-outliers to new input vector
        std::vector <Vector4<double>> inControls2;
        for (int i = 0; i < outControls.size(); i++) {
            xyz in(inControls[i][1], inControls[i][2], inControls[i][3]);
            xyz out(outControls[i][1], outControls[i][2], outControls[i][3]);

            double d = out.subtract(in).magnitude();
            if (outlierCriteria > (d*d)) {
                inControls2.push_back(inControls[i]);
            }
        }

        // Redo interpolation if there are outliers or pass2 degree is different than pass1 degree.
        routeSmoothingStats.outlierCount = (unsigned)(inControls.size() - inControls2.size());
        if (routeSmoothingStats.outlierCount || (degree0 != degree1)) {
            const std::vector <Vector4<double>> &pass2InControls = (inControls2.size()) ? inControls2 : inControls;

            // Create b-spline curve of desired degree using non-outliers.
            degree1 = std::min(degree1, (unsigned)pass2InControls.size() - 1);
            BasisFunctionInput<double> inBasis2((int)pass2InControls.size(), degree1);
            DistanceXYZBSplineCurve curve2(inBasis2, pass2InControls.data());

            // Gather new altitudes for all incoming distances using original inControls sample distances.
            bool fSuccess2 = InterpolateBSplineCurve<DistanceXYZBSplineCurve, Vector4<double>>(inControls, outControls, curve2, evalCount);
            if (!fSuccess2) {
                // Translation failed to converge.
                return false;
            }
            routeSmoothingStats.pass2Variance = ComputeLocationResultVariance(inControls, outControls);
        }
    }

    return true;
}

bool FixGPS::postProcess(RideFile *ride, DataProcessorConfig *config, QString op)
{
    Q_UNUSED(config)
    Q_UNUSED(op)

    if (!ride) return false;

    bool fHasAlt = ride->areDataPresent()->alt;
    bool fHasLoc = ride->areDataPresent()->lat && ride->areDataPresent()->lon;

    // We can operate on alt and/or loc, but cannot operate if there are neither.
    if (!fHasAlt && !fHasLoc)
        return false;

    int errors=0;

    ride->command->startLUW("Fix GPS Errors");

    // Default behavior: Interpolate missing points.
    if (fHasLoc) {

        GeoPointInterpolator gpi;
        int ii = 0; // interpolation input index

        for (int i = 0; i < ride->dataPoints().count(); i++) {
            const RideFilePoint* pi = (ride->dataPoints()[i]);
            geolocation curLoc(pi->lat, pi->lon, fHasAlt ? pi->alt : 0.0);

            // Activate interpolation if this sample isn't reasonable.
            if (!IsReasonableGeoLocation(&curLoc)) {
                double km = pi->km;

                // Feed interpolator until it has samples that span current distance.
                while (gpi.WantsInput(km)) {
                    if (ii < ride->dataPoints().count()) {
                        const RideFilePoint* pii = (ride->dataPoints()[ii]);
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

                geolocation interpLoc = gpi.Location(km);

                ride->command->setPointValue(i, RideFile::lat, interpLoc.Lat());
                ride->command->setPointValue(i, RideFile::lon, interpLoc.Long());

                if (fHasAlt) {
                    ride->command->setPointValue(i, RideFile::alt, interpLoc.Alt());
                }

                errors++;
            }
        }
    }

    ride->command->endLUW();

    bool smoothingSuccess = false;

    bool fDoSmoothAltitude, fDoSmoothRoute;
    unsigned degree0, degree1, degree0Route, degree1Route;
    double outlierCriteria, outlierCriteriaRoute;
    if (config) {
        fDoSmoothAltitude    = ((FixGPSConfig*)(config))->doSmoothAltitude->checkState();
        degree0              = ((FixGPSConfig*)(config))->degree0SpinBox->value();
        degree1              = ((FixGPSConfig*)(config))->degree1SpinBox->value();
        outlierCriteria      = (((FixGPSConfig*)(config))->outlierSpinBox->value()) / 100.;

        fDoSmoothRoute       = ((FixGPSConfig*) (config))->doSmoothRoute->checkState();
        degree0Route         = ((FixGPSConfig*) (config))->degree0SpinBoxRoute->value();
        degree1Route         = ((FixGPSConfig*) (config))->degree1SpinBoxRoute->value();
        outlierCriteriaRoute = (((FixGPSConfig*)(config))->outlierSpinBoxRoute->value()) / 100.;
    } else {
        fDoSmoothAltitude     = appsettings->value(NULL, GC_FIXGPS_ALTITUDE_FIX_DOAPPLY, Qt::Unchecked).toBool();
        degree0               = appsettings->value(NULL, GC_FIXGPS_ALTITUDE_FIX_DEGREE, 200).toUInt();
        degree1               = appsettings->value(NULL, GC_FIXGPS_ALTITUDE_FIX_DEGREE1, 200).toUInt();
        outlierCriteria       = appsettings->value(NULL, GC_FIXGPS_ALTITUDE_OUTLIER_PERCENT, 100).toInt();

        fDoSmoothRoute        = appsettings->value(NULL, GC_FIXGPS_ROUTE_FIX_DOAPPLY, Qt::Unchecked).toBool();
        degree0Route          = appsettings->value(NULL, GC_FIXGPS_ROUTE_FIX_DEGREE, 200).toUInt();
        degree1Route          = appsettings->value(NULL, GC_FIXGPS_ROUTE_FIX_DEGREE1, 200).toUInt();
        outlierCriteriaRoute  = appsettings->value(NULL, GC_FIXGPS_ROUTE_OUTLIER_PERCENT, 100).toInt();
    }

    // If no alt then dont try to smooth it.
    if (!fHasAlt) {
        fDoSmoothAltitude = false;
    }

    // If no loc then dont try to smooth it.
    if (!fHasLoc) {
        fDoSmoothRoute = false;
    }

    std::vector<Vector2<double>> inControls2, outControls2;
    if (fDoSmoothAltitude) {
        ride->command->startLUW("Smooth Altitude");

        bool fHaveControls = GatherForAltitudeSmoothing(ride, inControls2);
        if (fHaveControls) {
            AltitudeSmoothingStats altitudeSmoothingStats;
            smoothingSuccess = smoothAltitude(inControls2, degree0, outlierCriteria, degree1, outControls2, altitudeSmoothingStats);
            if (smoothingSuccess) {
                // Apply smoothed altitudes onto ride file
                for (int i = 0; i < ride->dataPoints().count(); i++) {
                    ride->command->setPointValue(i, RideFile::alt, outControls2[i][1]);
                }
            }
        }

        ride->command->endLUW();
    }

    if (fDoSmoothRoute) {
        ride->command->startLUW("Smooth Route");

        std::vector<Vector4<double>> inControls4, outControls4;
        bool fGathered = GatherForRouteSmoothing(ride, inControls4, outControls2);
        if (fGathered) {
            RouteSmoothingStats routeSmoothingStats;
            smoothingSuccess = smoothRoute(inControls4, degree0Route, outlierCriteriaRoute, degree1Route, outControls4, routeSmoothingStats);
            if (smoothingSuccess) {
                // Apply smoothed location points onto ride file
                for (int i = 0; i < ride->dataPoints().count(); i++) {
                    xyz loc(outControls4[i][1], outControls4[i][2], outControls4[i][3]);
                    geolocation geo = loc.togeolocation();

                    ride->command->setPointValue(i, RideFile::lat, geo.Lat());
                    ride->command->setPointValue(i, RideFile::lon, geo.Long());
                    ride->command->setPointValue(i, RideFile::alt, geo.Alt());
                }
                smoothingSuccess = true;
            }
        }

        ride->command->endLUW();
    }

    if (smoothingSuccess) {
        ride->recalculateDerivedSeries(true);
    }

    if (errors) {
        ride->setTag("GPS errors", QString("%1").arg(errors));
    }

    return errors || smoothingSuccess;
}
