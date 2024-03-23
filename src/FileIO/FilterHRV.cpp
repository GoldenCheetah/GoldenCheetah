/*
 *
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

#include "FilterHRV.h"
#include "Settings.h"
#include "DataProcessor.h"
#include "Context.h"
#include "HelpWhatsThis.h"
#include "MeasuresDownload.h"
#include "Measures.h"
#include "Athlete.h"

void FilterHrv(XDataSeries *rr, double rr_min, double rr_max, double filt, int hwin)
{
    int n = rr->datapoints.count();
    int win = 0;
    int idx_lead = hwin;
    int idx_lag = -hwin;

    double average, filtlim;
    double sum = 0.0;

    if (rr->valuename.length()==1)
        {
            rr->valuename << "R-R flag";
            rr->unitname << "bool";
        }

    // Flag R-R values which are outside min/max with -1. The values
    // flagged with -1 are *NOT* included when calculating the window
    // average. This deviates from the filtering methodology used
    // by https://physionet.org/tutorials/hrv-toolkit/ where all are
    // included.
    for (int idx=0; idx < n; idx++)
        {
            if (rr_min < rr->datapoints[idx]->number[0] &&
                rr_max > rr->datapoints[idx]->number[0])
                {
                    rr->datapoints[idx]->number[1] = 1;
                }
            else
                {
                    rr->datapoints[idx]->number[1] = -1;
                }
        }

    if (n>2*hwin)
        {
            // Initialize R-R sum for the value in the window around
            // current value.
            for (int idx=0; idx < hwin; idx++)
                {
                    if (rr->datapoints[idx]->number[1] == 1)
                        {
                            sum += rr->datapoints[idx]->number[0];
                            win++;
                        }
                }
            win--;

            for (int idx=0; idx < n; idx++)
                {

                    // Append new values to the window
                    if (idx_lead < n && rr->datapoints[idx_lead]->number[1] == 1)
                        {
                            sum += rr->datapoints[idx_lead]->number[0];
                            win++;
                        }
                    // Remove trailing values from the window
                    if (idx_lag >= 0 && rr->datapoints[idx_lag]->number[1] >= 0)
                        {
                            sum -= rr->datapoints[idx_lag]->number[0];
                            win--;
                        }

                    // Flag values which are outside +- (filt * 100) percent
                    // of the average value in a window around current value
                    // with 0.
                    if (rr->datapoints[idx]->number[1] == 1)
                        {
                            // Don't include current when calculating average.
                            sum -= rr->datapoints[idx]->number[0];

                            average = sum / win;
                            filtlim = filt * average;

                            if (rr->datapoints[idx]->number[0] <= average + filtlim &&
                                rr->datapoints[idx]->number[0] >= average - filtlim)
                                {
                                    rr->datapoints[idx]->number[1] = 1;
                                }
                            else
                                {
                                    rr->datapoints[idx]->number[1] = 0;
                                }

                            // Add current value to the window
                            sum += rr->datapoints[idx]->number[0];
                        }

                    idx_lead++;
                    idx_lag++;
                }
        }
}

class FilterHrvOutliers;
class FilterHrvOutliersConfig : public DataProcessorConfig
{

    Q_DECLARE_TR_FUNCTIONS(FilterHrvOutliersConfig)

    friend class ::FilterHrvOutliers;
protected:
    QHBoxLayout *layout;
    QLabel *hrvMaxLabel;
    QDoubleSpinBox *hrvMax;
    QLabel *hrvMinLabel;
    QDoubleSpinBox *hrvMin;

    QLabel *hrvFiltLabel;
    QDoubleSpinBox *hrvFilt;
    QLabel *hrvWindowLabel;
    QDoubleSpinBox *hrvWindow;

    QCheckBox *setRestHrv;

public:
    FilterHrvOutliersConfig(QWidget *parent) : DataProcessorConfig(parent) {
        HelpWhatsThis *help = new HelpWhatsThis(parent);
        parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_FilterHrv));

        layout = new QHBoxLayout(this);

        layout->setContentsMargins(0,0,0,0);
        setContentsMargins(0,0,0,0);

        hrvMaxLabel = new QLabel(tr("R-R maximum (msec)"));
        hrvMinLabel = new QLabel(tr("R-R minimum (msec)"));
        hrvFiltLabel = new QLabel(tr("Filter range"));
        hrvWindowLabel = new QLabel(tr("Filter window size (#)"));

        hrvMax = new QDoubleSpinBox();
        hrvMax->setMaximum(5000);
        hrvMax->setMinimum(0);
        hrvMax->setSingleStep(1);
        hrvMax->setDecimals(0);

        hrvMin = new QDoubleSpinBox();
        hrvMin->setMaximum(5000);
        hrvMin->setMinimum(0);
        hrvMin->setSingleStep(1);
        hrvMin->setDecimals(0);

        hrvFilt = new QDoubleSpinBox();
        hrvFilt->setMaximum(10.0);
        hrvFilt->setMinimum(-10.0);
        hrvFilt->setSingleStep(0.1);
        hrvFilt->setDecimals(2);

        hrvWindow = new QDoubleSpinBox();
        hrvWindow->setMaximum(50);
        hrvWindow->setMinimum(4);
        hrvWindow->setSingleStep(1);
        hrvWindow->setDecimals(0);

        setRestHrv = new QCheckBox(tr("Set Rest Hrv"));

        layout->addWidget(hrvMaxLabel);
        layout->addWidget(hrvMax);
        layout->addWidget(hrvMinLabel);
        layout->addWidget(hrvMin);
        layout->addWidget(hrvFiltLabel);
        layout->addWidget(hrvFilt);
        layout->addWidget(hrvWindowLabel);
        layout->addWidget(hrvWindow);
        layout->addWidget(setRestHrv);

        layout->addStretch();
    }
    QString explain() {
        return(QString(tr("Filter R-R outliers (see \"R-R flag\" in HRV Xdata). Non outliers are marked \"1\".\n"
                          "  - \"R-R min and maximum\" exclude samples outside (flag -1). Also excluded when filtering range.\n"
                          "  - \"Filter range\" of the average within a window (flag 0)\n"
                          "  - \"Filter window size\" distance on either side of the current interval\n"
                          "  - \"Set Rest HRV\" if checked on interactive use the computed HRV metrics are set as Rest HRV Measures\n"
                          ""
                          )));
    }
    void readConfig() {
        double MaxVal = appsettings->value(NULL, GC_RR_MAX, "2000.0").toDouble();
        double MinVal = appsettings->value(NULL, GC_RR_MIN, "270.0").toDouble();
        double FiltVal = appsettings->value(NULL, GC_RR_FILT, "0.20").toDouble();
        double Window = appsettings->value(NULL, GC_RR_WINDOW, "20").toDouble();

        bool SetRestHrv = appsettings->value(NULL, GC_RR_SET_REST_HRV, Qt::Checked).toBool();

        hrvMax->setValue(MaxVal);
        hrvMin->setValue(MinVal);
        hrvFilt->setValue(FiltVal);
        hrvWindow->setValue(Window);
        setRestHrv->setCheckState(SetRestHrv ? Qt::Checked : Qt::Unchecked);
    }
    void saveConfig() {
        appsettings->setValue(GC_RR_MAX, hrvMax->value());
        appsettings->setValue(GC_RR_MIN, hrvMin->value());
        appsettings->setValue(GC_RR_FILT, hrvFilt->value());
        appsettings->setValue(GC_RR_WINDOW, hrvWindow->value());
        appsettings->setValue(GC_RR_SET_REST_HRV, setRestHrv->checkState());
    }
};

class FilterHrvOutliers : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(FilterHrvOutliers)

public:
    FilterHrvOutliers() {}
    ~FilterHrvOutliers() {}
    // the processor
    bool postProcess(RideFile *, DataProcessorConfig* config, QString op);

    // the config widget
    DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) {
        Q_UNUSED(ride);
        return new FilterHrvOutliersConfig(parent);
    }

    // Localized Name
    QString name() {
        return (tr("Filter R-R Outliers"));
    }
};

static bool FilterHrvOutliersAdded = DataProcessorFactory::instance().registerProcessor(QString("Filter R-R Outliers"), new FilterHrvOutliers());

bool
FilterHrvOutliers::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(op)

    XDataSeries *series = ride->xdata("HRV");
    if (series && series->datapoints.count() > 0) {

        // Read settings
        double rrMax;
        double rrMin;
        double rrFilt;
        int rrWindow;
        bool setRestHrv;

        if (config == NULL) { // being called automatically
            rrMax = appsettings->value(NULL, GC_RR_MAX, "2000.0").toDouble();
            rrMin = appsettings->value(NULL, GC_RR_MIN, "270.0").toDouble();
            rrFilt = appsettings->value(NULL, GC_RR_FILT, "0.2").toDouble();
            rrWindow = appsettings->value(NULL, GC_RR_WINDOW, "20").toInt();
            setRestHrv = appsettings->value(NULL, GC_RR_SET_REST_HRV, Qt::Unchecked).toBool();
        }
        else { // being called manually
            rrMax = ((FilterHrvOutliersConfig*)(config))->hrvMax->value();
            rrMin = ((FilterHrvOutliersConfig*)(config))->hrvMin->value();
            rrFilt = ((FilterHrvOutliersConfig*)(config))->hrvFilt->value();
            rrWindow = (int) ((FilterHrvOutliersConfig*)(config))->hrvWindow->value();
            setRestHrv = (bool) ((FilterHrvOutliersConfig*)(config))->setRestHrv->checkState();
        }
        FilterHrv(series, rrMin, rrMax, rrFilt, rrWindow);

        // refresh if present in RideCache
        RideItem *rideItem = nullptr;
        foreach(RideItem *item, ride->context->athlete->rideCache->rides()) {
            if (item->dateTime == ride->startTime()) {
                rideItem = item;
                break;
            }
        }
        if (rideItem) rideItem->refresh();

        // Set HRV Measures according to user request if present in RideCache
        if (setRestHrv && rideItem) {
            QStringList fieldSymbols = ride->context->athlete->measures->getFieldSymbols(Measures::Hrv);

            double avnn = rideItem->getForSymbol("AVNN");

            Measure hrvMeasure;
            hrvMeasure.when = rideItem->dateTime;
            if (fieldSymbols.contains("HR"))
                hrvMeasure.values[fieldSymbols.indexOf("HR")] = !qFuzzyIsNull(avnn) ? 60000 / avnn : 0;
            if (fieldSymbols.contains("AVNN"))
                hrvMeasure.values[fieldSymbols.indexOf("AVNN")] = avnn;
            if (fieldSymbols.contains("SDNN"))
                hrvMeasure.values[fieldSymbols.indexOf("SDNN")] = rideItem->getForSymbol("SDNN");
            if (fieldSymbols.contains("RMSSD"))
                hrvMeasure.values[fieldSymbols.indexOf("RMSSD")] = rideItem->getForSymbol("rMSSD");
            if (fieldSymbols.contains("PNN50"))
                hrvMeasure.values[fieldSymbols.indexOf("PNN50")] = rideItem->getForSymbol("pNN50");
            if (fieldSymbols.contains("RECOVERY_POINTS"))
                hrvMeasure.values[fieldSymbols.indexOf("RECOVERY_POINTS")] = 1.5 * log(rideItem->getForSymbol("rMSSD")) + 2;

            QList<Measure> hrvMeasures;
            hrvMeasures.append(hrvMeasure);

            MeasuresDownload::updateMeasures(ride->context,
                                             ride->context->athlete->measures->getGroup(Measures::Hrv),
                                             hrvMeasures);
        }

        return true;
    }
    else {
        return false;
    }
}
