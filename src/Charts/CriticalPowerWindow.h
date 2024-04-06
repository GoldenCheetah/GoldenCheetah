/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
 * Copyright (c) 2009 Dan Connelly (@djconnel)
 * Copyright (c) 2014 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#ifndef _GC_CriticalPowerWindow_h
#define _GC_CriticalPowerWindow_h 1
#include "GoldenCheetah.h"
#include "MainWindow.h" // for isfiltered and filters
#include "Season.h"
#include "RideFile.h"
#include "SearchFilterBox.h"
#include "Perspective.h"

#include "qxtstringspinbox.h"
#include <QtGui>
#include <QFormLayout>
#include <QCheckBox>
#include <qwt_plot_grid.h>

class CPPlot;
class QwtPlotCurve;
class Context;
class RideItem;
class IntervalItem;
class QwtPlotPicker;
class MUPlot;

class CriticalPowerWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // properties can be saved/restored/set by the layout manager

    Q_PROPERTY(QString filter READ filter WRITE setFilter USER true)
    Q_PROPERTY(int mode READ mode WRITE setMode USER true)
    Q_PROPERTY(bool showBest READ showBest WRITE setShowBest USER true)
    Q_PROPERTY(bool showTest READ showTest WRITE setShowTest USER true)
    Q_PROPERTY(bool filterBest READ filterBest WRITE setFilterBest USER true)
    Q_PROPERTY(bool showPercent READ showPercent WRITE setShowPercent USER true)
    Q_PROPERTY(bool showDelta READ showDelta WRITE setShowDelta USER true)
    Q_PROPERTY(bool showDeltaPercent READ showDeltaPercent WRITE setShowDeltaPercent USER true)
    Q_PROPERTY(bool showPowerIndex READ showPowerIndex WRITE setShowPowerIndex USER true)
    Q_PROPERTY(bool showGrid READ showGrid WRITE setShowGrid USER true)

    // for retro compatibility
    Q_PROPERTY(QString season READ season WRITE setSeason USER true)

    Q_PROPERTY(int cpmodel READ cpModel WRITE setCPModel USER true)
    Q_PROPERTY(int fit READ fit WRITE setFit USER true)
    Q_PROPERTY(int fitdata READ fitdata WRITE setFitdata USER true)
    Q_PROPERTY(int variant READ variant WRITE setVariant USER true)
    Q_PROPERTY(bool modelDecay READ modelDecay WRITE setModelDecay USER true)
    Q_PROPERTY(int ani1 READ anI1 WRITE setAnI1 USER true)
    Q_PROPERTY(int ani2 READ anI2 WRITE setAnI2 USER true)
    Q_PROPERTY(int aei1 READ aeI1 WRITE setAeI1 USER true)
    Q_PROPERTY(int aei2 READ aeI2 WRITE setAeI2 USER true)
    Q_PROPERTY(int sani1 READ sanI1 WRITE setSanI1 USER true)
    Q_PROPERTY(int sani2 READ sanI2 WRITE setSanI2 USER true)
    Q_PROPERTY(int laei1 READ laeI1 WRITE setLaeI1 USER true)
    Q_PROPERTY(int laei2 READ laeI2 WRITE setLaeI2 USER true)
    Q_PROPERTY(bool showPP READ showPP WRITE setShowPP USER true)

    Q_PROPERTY(QDate fromDate READ fromDate WRITE setFromDate USER true)
    Q_PROPERTY(QDate toDate READ toDate WRITE setToDate USER true)
    Q_PROPERTY(QDate startDate READ startDate WRITE setStartDate USER true)
    Q_PROPERTY(int lastN READ lastN WRITE setLastN USER true)
    Q_PROPERTY(int lastNX READ lastNX WRITE setLastNX USER true)
    Q_PROPERTY(int prevN READ prevN WRITE setPrevN USER true)
    Q_PROPERTY(int shading READ shading WRITE setShading USER true)
    Q_PROPERTY(bool showEffort READ showEffort WRITE setShowEffort USER true)
    Q_PROPERTY(bool showHeat READ showHeat WRITE setShowHeat USER true)
    Q_PROPERTY(bool showHeatByDate READ showHeatByDate WRITE setShowHeatByDate USER true)
    Q_PROPERTY(bool showCSLinear READ showCSLinear WRITE setShowCSLinear USER true)
    Q_PROPERTY(int shadeIntervals READ shadeIntervals WRITE setShadeIntervals USER true)
    Q_PROPERTY(int ridePlotMode READ ridePlotMode WRITE setRidePlotMode USER true)
    Q_PROPERTY(int useSelected READ useSelected WRITE setUseSelected USER true) // !! must be last property !!

    public:

        CriticalPowerWindow(Context *context, bool range);

        // compare is supported
        bool isCompare() const {
            return (rangemode && context->isCompareDateRanges) || (!rangemode && context->isCompareIntervals);
        }

        // reveal
        bool hasReveal() { return true; }

        void deleteCpiFile(QString filename);

        // set/get properties
        int mode() const { return seriesCombo->currentIndex(); }
        void setMode(int x) { seriesCombo->setCurrentIndex(x); rSeriesSelector->setValue(x); }

        int cpModel() const { return modelCombo->currentIndex(); }
        void setCPModel(int x) { modelCombo->setCurrentIndex(x); }

        bool modelDecay() const { return modelDecayCheck->isChecked(); }
        void setModelDecay(bool x) { modelDecayCheck->setChecked(x); }

        int fit() const { return fitCombo->currentIndex(); }
        void setFit(int x) { fitCombo->setCurrentIndex(x); }

        int fitdata() const { return fitdataCombo->currentIndex(); }
        void setFitdata(int x) { fitdataCombo->setCurrentIndex(x); }

        int variant() const;
        void setVariant(int x);

        // filter
        bool isFiltered() const { return (searchBox->isFiltered() || (myPerspective && myPerspective->isFiltered()) || context->ishomefiltered || context->isfiltered); }
        QString filter() const { return searchBox->filter(); }
        void setFilter(QString x) { searchBox->setFilter(x); }

        int ridePlotMode() const { return ridePlotStyleCombo->currentIndex(); }
        void setRidePlotMode(int x) { ridePlotStyleCombo->setCurrentIndex(x); }

        // for old settings config (<3.0) compatibility
        QString season() const { return cComboSeason->itemText(cComboSeason->currentIndex()); }
        void setSeason(QString x) { 
            int index = cComboSeason->findText(x);
            if (index != -1) {
                cComboSeason->setCurrentIndex(index);
                seasonSelected(index);
            }
        }

        int anI1() const { return anI1SpinBox->value(); }
        void setAnI1(int x) { return anI1SpinBox->setValue(x); }

        int anI2() const { return anI2SpinBox->value(); }
        void setAnI2(int x) { return anI2SpinBox->setValue(x); }

        int aeI1() const { return aeI1SpinBox->value(); }
        void setAeI1(int x) { return aeI1SpinBox->setValue(x); }

        int aeI2() const { return aeI2SpinBox->value(); }
        void setAeI2(int x) { return aeI2SpinBox->setValue(x); }

        int sanI1() const { return sanI1SpinBox->value(); }
        void setSanI1(int x) { return sanI1SpinBox->setValue(x); }

        int sanI2() const { return sanI2SpinBox->value(); }
        void setSanI2(int x) { return sanI2SpinBox->setValue(x); }

        int laeI1() const { return laeI1SpinBox->value(); }
        void setLaeI1(int x) { return laeI1SpinBox->setValue(x); }

        int laeI2() const { return laeI2SpinBox->value(); }
        void setLaeI2(int x) { return laeI2SpinBox->setValue(x); }

        enum criticalseriestype { watts, wattsd, wattsKg, xPower, IsoPower, hr, hrd, kph, kphd, cad, cadd, nm, nmd, vam, aPower, aPowerKg, work, veloclinicplot };

        typedef enum criticalseriestype CriticalSeriesType;

        QString seriesName(CriticalSeriesType series);

        CriticalSeriesType series() {
            return static_cast<CriticalSeriesType>
                (seriesCombo->itemData(seriesCombo->currentIndex()).toInt());
        }

        static RideFile::SeriesType getRideSeries(CriticalSeriesType series);

        int useSelected() { return dateSetting->mode(); }
        void setUseSelected(int x) { dateSetting->setMode(x); }

        QDate fromDate() { return dateSetting->fromDate(); }
        void setFromDate(QDate date)  { return dateSetting->setFromDate(date); }
        QDate toDate() { return dateSetting->toDate(); }
        void setToDate(QDate date)  { return dateSetting->setToDate(date); }
        QDate startDate() { return dateSetting->startDate(); }
        void setStartDate(QDate date)  { return dateSetting->setStartDate(date); }

        int lastN() { return dateSetting->lastN(); }
        void setLastN(int x) { dateSetting->setLastN(x); }
        int lastNX() { return dateSetting->lastNX(); }
        void setLastNX(int x) { dateSetting->setLastNX(x); }

        int prevN() { return dateSetting->prevN(); }
        void setPrevN(int x) { dateSetting->setPrevN(x); }

        int shading() { return shadeCheck->isChecked(); }
        void setShading(int x) { return shadeCheck->setChecked(x); }

        int shadeIntervals() { return shadeIntervalsCheck->isChecked(); }
        void setShadeIntervals(int x) { return shadeIntervalsCheck->setChecked(x); }

        bool showEffort() { return showEffortCheck->isChecked(); }
        void setShowEffort(bool x) { return showEffortCheck->setChecked(x); }

        bool showHeat() { return showHeatCheck->isChecked(); }
        void setShowHeat(bool x) { return showHeatCheck->setChecked(x); }

        bool showHeatByDate() { return showHeatByDateCheck->isChecked(); }
        void setShowHeatByDate(bool x) { return showHeatByDateCheck->setChecked(x); }

        bool showCSLinear() { return showCSLinearCheck->isChecked(); }
        void setShowCSLinear(bool x) { return showCSLinearCheck->setChecked(x); }

        bool showGrid() { return showGridCheck->isChecked(); }
        void setShowGrid(bool x) { return showGridCheck->setChecked(x); }

        bool filterBest() { return filterBestCheck->isChecked(); }
        void setFilterBest(bool x) { return filterBestCheck->setChecked(x); }

        bool showBest() { return showBestCheck->isChecked(); }
        void setShowBest(bool x) { return showBestCheck->setChecked(x); }

        bool showTest() { return showTestCheck->isChecked(); }
        void setShowTest(bool x) { return showTestCheck->setChecked(x); }

        bool showPowerIndex() { return showPowerIndexCheck->isChecked(); }
        void setShowPowerIndex(bool x) { return showPowerIndexCheck->setChecked(x); }

        bool showPercent() { return showPercentCheck->isChecked(); }
        void setShowPercent(bool x) { return showPercentCheck->setChecked(x); }

        bool showDelta() { return showDeltaCheck->isChecked(); }
        void setShowDelta(bool x) { return showDeltaCheck->setChecked(x); }
        bool showDeltaPercent() { return showDeltaPercentCheck->isChecked(); }
        void setShowDeltaPercent(bool x) { return showDeltaPercentCheck->setChecked(x); }

        bool showPP() { return showPPCheck->isChecked(); }
        void setShowPP(bool x) { return showPPCheck->setChecked(x); }

    protected slots:
        void forceReplot();
        void newRideAdded(RideItem*);
        void rideSelected();
        void configChanged(qint32);
        void intervalSelected();
        void intervalsChanged();
        void intervalHover(IntervalItem*);
        void seasonSelected(int season);
        void shadingSelected(int shading);
        void showEffortChanged(int check);
        void showPPChanged(int check);
        void showHeatChanged(int check);
        void showCSLinearChanged(int state);
        void showHeatByDateChanged(int check);
        void showPercentChanged(int check);
        void showDeltaChanged();
        void showPowerIndexChanged(int check);
        void showBestChanged(int check);
        void showTestChanged(int check);
        void filterBestChanged(int check);
        void showGridChanged(int check);
        void shadeIntervalsChanged(int state);
        void setPlotType(int index);
        void setSeries(int index);
        void resetSeasons();
        void filterChanged();
        void dateRangeChanged(DateRange);
        void perspectiveFilterChanged();

        void useCustomRange(DateRange);
        void useStandardRange();
        void useThruToday();

        void refreshRideSaved(RideItem*);
        void modelParametersChanged(); // we changed the intervals
        void modelChanged(); // we changed the model type
        void fitChanged();

        // reveal controls changed
        void rSeriesSelectorChanged(int);
        void rPercentChanged(int check);
        void rHeatChanged(int check);
        void rDeltaChanged();

        // edit CP value used for veloclinic plot
        void setSliderFromEdit();
        void setEditFromSlider();

        // get fit summary
        void setSummary(QString x) { summary->setText(x); }

        // menu option
        void exportData();

    private:
        void updateCpint(double minutes);
        void hideIntervalCurve(int index);
        void showIntervalCurve(IntervalItem *current, int index);

        QString _dateRange;

        double curve_to_point(double x, const QwtPlotCurve *curve, CriticalSeriesType serie);

    protected:

        friend class ::CPPlot;
        friend class ::MUPlot;

        bool event(QEvent *event);

        CPPlot *cpPlot;
        Context *context;
        QLabel *cpintTimeValue;
        QLabel *cpintTodayValue;
        QLabel *cpintAllValue;
        QLabel *cpintCPValue;
        QLabel *modelDecayLabel;
        QComboBox *seriesCombo;
        QComboBox *modelCombo;
        QComboBox *fitCombo;
        QComboBox *fitdataCombo;
        QLabel *vlabel;
        QRadioButton *velo1, *velo2, *velo3; // for selecting veloclinic formulation
        QComboBox *cComboSeason;
        QComboBox *ridePlotStyleCombo;
        QCheckBox *shadeCheck;
        QCheckBox *shadeIntervalsCheck;
        QCheckBox *showEffortCheck;
        QCheckBox *showPPCheck;
        QCheckBox *showHeatCheck;
        QCheckBox *showHeatByDateCheck;
        QCheckBox *showPercentCheck;
        QCheckBox *showDeltaCheck, *showDeltaPercentCheck;
        QCheckBox *showPowerIndexCheck;
        QCheckBox *showBestCheck;
        QCheckBox *showTestCheck;
        QCheckBox *filterBestCheck;
        QCheckBox *showGridCheck;
        QxtStringSpinBox *rSeriesSelector;
        QCheckBox *rPercent, *rHeat, *rDelta, *rDeltaPercent;
        QCheckBox *showCSLinearCheck;
        QCheckBox *modelDecayCheck;
        QLabel *showCSLinearLabel;
        QwtPlotPicker *picker;
        QwtPlotGrid *grid;

        // veloclinic controls
        QSlider *CPSlider;
        QLineEdit *CPEdit;
        QLabel *CPLabel;

        // model helper widget
        QWidget *helper;
        QLabel *titleBlank, *titleValue, *titleRank;
        QLabel *wprimeTitle, *wprimeValue, *wprimeRank;
        QLabel *cpTitle, *cpValue, *cpRank;
        QLabel *pmaxTitle, *pmaxValue, *pmaxRank;
        QLabel *eiTitle, *eiValue;
        QLabel *summary;

        void addSeries();
        void updateOptions(CriticalSeriesType series);
        Seasons *seasons;
        QList<Season> seasonsList;
        RideItem *currentRide;
        QList<CriticalSeriesType> seriesList;
        SearchFilterBox *searchBox;
        QList<QwtPlotCurve*> intervalCurves;

        QLabel *intervalLabel, *secondsLabel;
        QLabel *sanLabel;
        QLabel *anLabel;
        QLabel *aeLabel;
        QLabel *laeLabel;

        QDoubleSpinBox *sanI1SpinBox, *sanI2SpinBox;
        QDoubleSpinBox *anI1SpinBox, *anI2SpinBox;
        QDoubleSpinBox *aeI1SpinBox, *aeI2SpinBox;
        QDoubleSpinBox *laeI1SpinBox, *laeI2SpinBox;

        bool rangemode;
        bool isfiltered;
        QDate cfrom, cto;
        bool stale;
        bool useCustom;
        bool useToToday;
        DateRange custom;

        DateSettingsEdit *dateSetting;
        bool active; // when resetting parameters
        QwtPlotCurve *hoverCurve;

        bool firstShow;
};

#endif // _GC_CriticalPowerWindow_h
