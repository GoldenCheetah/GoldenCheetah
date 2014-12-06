/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#include "Athlete.h"
#include "Zones.h"
#include "Colors.h"
#include "CPPlot.h"

#include <unistd.h>
#include <QDebug>
#include <qwt_series_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_layout.h>
#include <qwt_plot_marker.h>
#include <qwt_symbol.h>
#include <qwt_scale_engine.h>
#include <qwt_scale_widget.h>
#include <qwt_color_map.h>
#include <algorithm> // for std::lower_bound

#include "CriticalPowerWindow.h"
#include "RideItem.h"
#include "LogTimeScaleDraw.h"
#include "RideFile.h"
#include "Season.h"
#include "Settings.h"
#include "LTMCanvasPicker.h"
#include "TimeUtils.h"
#include "Units.h"


CPPlot::CPPlot(QWidget *parent, Context *context, bool rangemode) : QwtPlot(parent), parent(parent),

    // model
    model(0), modelVariant(0),

    // state
    context(context), rideCache(NULL), bestsCache(NULL), rideSeries(RideFile::watts), 
    isFiltered(false), shadeMode(2),
    shadeIntervals(true), rangemode(rangemode), 
    showBest(true), showPercent(false), showHeat(false), showHeatByDate(false), showDelta(false), showDeltaPercent(false),
    plotType(0),

    // curves and plot objects
    rideCurve(NULL), modelCurve(NULL), heatCurve(NULL), heatAgeCurve(NULL), pdModel(NULL)

{
    setAutoFillBackground(true);
    setAxisTitle(xBottom, tr("Interval Length"));

    // Log scale on x-axis
    ltsd = new LogTimeScaleDraw;
    ltsd->setTickLength(QwtScaleDiv::MajorTick, 3);
    setAxisScaleDraw(xBottom, ltsd);
    setAxisScaleEngine(xBottom, new QwtLogScaleEngine);

    // left yAxis scale prettify
    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(yLeft, sd);
    setAxisTitle(yLeft, tr("Average Power (watts)"));
    setAxisMaxMinor(yLeft, 0);
    plotLayout()->setAlignCanvasToScales(true);

    // right yAxis scale prettify
    sd = new QwtScaleDraw;
    sd->setTickLength(QwtScaleDiv::MajorTick, 3);
    sd->enableComponent(QwtScaleDraw::Ticks, false);
    sd->enableComponent(QwtScaleDraw::Backbone, false);
    setAxisScaleDraw(yRight, sd);
    setAxisTitle(yRight, tr("Percent of Best"));
    setAxisMaxMinor(yRight, 0);

    // zoom
    zoomer = new penTooltip(static_cast<QwtPlotCanvas*>(this->canvas()));
    zoomer->setMousePattern(QwtEventPattern::MouseSelect1,
                            Qt::LeftButton, Qt::ShiftModifier);

    // hover
    canvasPicker = new LTMCanvasPicker(this);
    static_cast<QwtPlotCanvas*>(canvas())->setFrameStyle(QFrame::NoFrame);
    connect(canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)), this, SLOT(pointHover(QwtPlotCurve*, int)));

    // now color everything we created
    configChanged();
}

// set colours mostly
void
CPPlot::configChanged()
{
    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    setPalette(palette);

    axisWidget(QwtPlot::xBottom)->setPalette(palette);
    axisWidget(QwtPlot::yLeft)->setPalette(palette);
    axisWidget(QwtPlot::yRight)->setPalette(palette);

    setCanvasBackground(GColor(CPLOTBACKGROUND));
}

// get the fonts and colors right for the axis scales
void
CPPlot::setAxisTitle(int axis, QString label)
{
    // setup the default fonts
    QFont stGiles; // hoho - Chart Font St. Giles ... ok you have to be British to get this joke
    stGiles.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    stGiles.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

    QwtText title(label);
    title.setColor(GColor(CPLOTMARKER));
    title.setFont(stGiles);
    QwtPlot::setAxisFont(axis, stGiles);
    QwtPlot::setAxisTitle(axis, title);
}

// change the date range for the 'bests' curve
void
CPPlot::setDateRange(const QDate &start, const QDate &end)
{
    // wipe out current - calculate will reinstate
    startDate = (start == QDate()) ? QDate(1900, 1, 1) : start;
    endDate = (end == QDate()) ? QDate(3000, 12, 31) : end;

    // we need to replot the bests and model curves
    clearCurves(); // clears all bar the ride curve
}

// what series are we plotting ?
void
CPPlot::setSeries(CriticalPowerWindow::CriticalSeriesType criticalSeries)
{
    rideSeries = CriticalPowerWindow::getRideSeries(criticalSeries);
    this->criticalSeries = criticalSeries;

    // we need to set the y axis label to reflect delta
    // comparisons too, or even percent, if that is chosen.
    // first, are we in compare mode ?
    QString prefix = "";
    QString series = "";
    QString units = "";
    QString postfix = "";
    if ((rangemode && context->isCompareDateRanges) || (!rangemode && context->isCompareIntervals)) {
        if (showDelta) {
            prefix = "Delta ";
            if (showDeltaPercent) {
                postfix = "percent";
            }
        }
    }

    enum { linear, inverse, log } scale = log;

    switch (criticalSeries) {

    case CriticalPowerWindow::veloclinicplot:
        series = tr("Veloclinic Plot");
        units = tr("J");
        scale = linear;
        break;

    case CriticalPowerWindow::work:
        series = tr("Total work");
        units = tr("kJ");
        scale = linear;
        break;

    case CriticalPowerWindow::cad:
        series = tr("Cadence");
        units = tr("rpm");
        break;

    case CriticalPowerWindow::hr:
        series = tr("Heartrate");
        units = tr("bpm");
        break;

    case CriticalPowerWindow::wattsd:
        series = tr("Watts delta");
        units = tr("watts/s");
        break;

    case CriticalPowerWindow::cadd:
        series = tr("Cadence delta");
        units = tr("rpm/s");
        break;

    case CriticalPowerWindow::nmd:
        series = tr("Torque delta");
        units = tr("nm/s");
        break;

    case CriticalPowerWindow::hrd:
        series = tr("Heartrate delta");
        units = tr("bpm/s");
        break;

    case CriticalPowerWindow::kphd:
        series = tr("Acceleration");
        units = tr("m/s/s");
        break;

    case CriticalPowerWindow::kph:
        series = tr("Speed");
        units = tr("kph");
        break;

    case CriticalPowerWindow::nm:
        series = tr("Pedal Force");
        units = tr("nm");
        break;

    case CriticalPowerWindow::NP:
        series = tr("Normalised Power");
        units = tr("watts");
        break;

    case CriticalPowerWindow::aPower:
        series = tr("Altitude Power");
        units = tr("watts");
        break;

    case CriticalPowerWindow::xPower:
        series = tr("xPower");
        units = tr("watts");
        break;

    case CriticalPowerWindow::wattsKg:
        if (context->athlete->useMetricUnits) {
            series = tr("Watts per kilogram");
            units = tr("w/kg");
        } else {
            series = tr("Watts per lb");
            units = tr("w/lb");
        }
        break;

    case CriticalPowerWindow::vam:
        series = tr("VAM");
        units = tr("m/hour");
        break;

    default:
    case CriticalPowerWindow::watts:
        series = tr("Power");
        units = tr("watts");
        break;

    }

    // set scale to match what's needed
    if (scale != log) setAxisScaleEngine(xBottom, new QwtLinearScaleEngine);
    else setAxisScaleEngine(xBottom, new QwtLogScaleEngine);

    if (criticalSeries == CriticalPowerWindow::veloclinicplot) {
        sd = new QwtScaleDraw;
        sd->setTickLength(QwtScaleDiv::MajorTick, 3);
        setAxisScaleDraw(xBottom, sd);
        setAxisTitle(xBottom, tr("Power (W)"));
    } else {
        ltsd = new LogTimeScaleDraw;
        ltsd->setTickLength(QwtScaleDiv::MajorTick, 3);
        setAxisScaleDraw(xBottom, ltsd);
        setAxisTitle(xBottom, tr("Interval Length"));
    }

    // set axis title
    setAxisTitle(yLeft, QString ("%1 %2 (%3) %4").arg(prefix).arg(series).arg(units).arg(postfix));

    // zap the old curves
    clearCurves();
}

void
CPPlot::initModel()
{
    switch (model) {

    case 0 : // no model - do nothing
        pdModel = NULL;
        break;
    case 1 : // 2 param
        pdModel = new CP2Model(context);
        break;
    case 2 : // 3 param
        pdModel = new CP3Model(context);
        break;
    case 3 : // extended model
        pdModel = new ExtendedModel(context);
        break;
    case 4 : // multimodel
        pdModel = new MultiModel(context);
        pdModel->setVariant(modelVariant);
        break;
    }

    if (pdModel) {

        // set the model and load data
        pdModel->setIntervals(sanI1, sanI2, anI1, anI2, aeI1, aeI2, laeI1, laeI2);
        pdModel->setMinutes(true); // we're minutes here ...
        pdModel->setData(bestsCache->meanMaxArray(rideSeries));
   }
}

// Plot the dashed line model curve according to the parameters
// and will also plot the heat on the curve or below since it is
// related to the model
void
CPPlot::plotModel()
{
    // first lets clear any curves we shouldn't be displaying
    // no model curve if not power !
    if (model == 0 || (rideSeries != RideFile::watts && rideSeries != RideFile::wattsKg)) {
        if (modelCurve) {
            modelCurve->detach();
            delete modelCurve;
            modelCurve = NULL;
        }
        return;
    }

    // no heat ?
    if ((rideSeries != RideFile::watts || showHeat == false) && heatCurve) {
        heatCurve->detach();
        delete heatCurve;
        heatCurve = NULL;
    }

    // no heat age ?
    if ((rideSeries != RideFile::watts || showHeatByDate == false) && heatAgeCurve) {
        heatAgeCurve->detach();
        delete heatAgeCurve;
        heatAgeCurve = NULL;
    }

    // we don't want a model
    if (rideSeries != RideFile::wattsKg && rideSeries != RideFile::watts) return;

    // we don't have any bests yet?
    if (bestsCache == NULL) return;

    // if you want something you need to wipe the old one first
    if (!modelCurve) {

        // new model please
        initModel();

        if (pdModel) {
            // create curve
            modelCurve = new QwtPlotCurve("Model");
            if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true)
                modelCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

            // set the point data
            if (criticalSeries == CriticalPowerWindow::veloclinicplot) {
                // Plot the model for the veloclinic plot
                pdModel->setMinutes(false);
                QVector<double> power(pdModel->size());
                QVector<double> wprime(pdModel->size());
                for (size_t t = 0; t < pdModel->size(); t++) {
                    power[t] = pdModel->y(t);
                    wprime[t] = (pdModel->y(t)-pdModel->CP()) * (pdModel->x(t)); // Joules
                }
                modelCurve->setSamples(power.data(), wprime.data(), pdModel->size()-1);
            }
            else
                modelCurve->setData(pdModel);


            // curve cosmetics
            QPen pen(GColor(CCP));
            double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
            pen.setWidth(width);
            if (showBest) pen.setStyle(Qt::DashLine);
            modelCurve->setPen(pen);
            modelCurve->attach(this);

            // update the model paramters display
            CriticalPowerWindow *cpw = static_cast<CriticalPowerWindow*>(parent);

            // update the helper widget -- either as watts or w/kg

            if (rideSeries == RideFile::watts) {

                //WPrime
                if (pdModel->hasWPrime()) cpw->wprimeValue->setText(QString(tr("%1 kJ")).arg(pdModel->WPrime() / 1000.0, 0, 'f', 1));
                else cpw->wprimeValue->setText(tr("n/a"));

                //CP
                cpw->cpValue->setText(QString(tr("%1 w")).arg(pdModel->CP(), 0, 'f', 0));

                //FTP and FTP ranking
                if (pdModel->hasFTP()) {
                    cpw->ftpValue->setText(QString(tr("%1 w")).arg(pdModel->FTP(), 0, 'f', 0));

                    // Reference 6.25W/kg -> untrained 2.5W/kg
                    int _ftpLevel = 15 * (pdModel->FTP() / appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble() - 2.5) / (6.25-2.5) ;
                    if (_ftpLevel > 0 && _ftpLevel < 16) // check bounds
                        cpw->ftpRank->setText(QString("%1").arg(_ftpLevel));
                    else
                        cpw->ftpRank->setText(tr("n/a"));

                } else {

                    cpw->ftpValue->setText(tr("n/a"));
                    cpw->ftpRank->setText(tr("n/a"));
                }

                // P-MAX and P-MAX ranking
                if (pdModel->hasPMax()) {
                    cpw->pmaxValue->setText(QString(tr("%1 w")).arg(pdModel->PMax(), 0, 'f', 0));

                    // Reference 22.5W/kg -> untrained 8W/kg
                    int _pMaxLevel = 15 * (pdModel->PMax() / appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble() - 8) / (23-8) ;
                    if (_pMaxLevel > 0 && _pMaxLevel < 16) // check bounds
                        cpw->pmaxRank->setText(QString("%1").arg(_pMaxLevel));
                    else
                        cpw->pmaxRank->setText(tr("n/a"));

                } else  {
                    cpw->pmaxValue->setText(tr("n/a"));
                    cpw->pmaxRank->setText(tr("n/a"));
                }

                // Endurance Index
                if (pdModel->hasWPrime() && pdModel->WPrime() && pdModel->hasCP() && pdModel->CP()) {
                    cpw->eiValue->setText(QString("%1").arg(pdModel->WPrime() / pdModel->CP(), 0, 'f', 0));
                }

            } else {

                //WPrime
                if (pdModel->hasWPrime()) cpw->wprimeValue->setText(QString(tr("%1 J/kg")).arg(pdModel->WPrime(), 0, 'f', 0));
                else cpw->wprimeValue->setText(tr("n/a"));

                //CP
                cpw->cpValue->setText(QString(tr("%1 w/kg")).arg(pdModel->CP(), 0, 'f', 2));

                //FTP and FTP ranking
                if (pdModel->hasFTP()) {
                    cpw->ftpValue->setText(QString(tr("%1 w/kg")).arg(pdModel->FTP(), 0, 'f', 2));

                    // Reference 6.25W/kg -> untrained 2.5W/kg
                    int _ftpLevel = 15 * (pdModel->FTP() - 2.5) / (6.25-2.5) ;
                    if (_ftpLevel > 0 && _ftpLevel < 16) // check bounds
                        cpw->ftpRank->setText(QString("%1").arg(_ftpLevel));
                    else
                        cpw->ftpRank->setText(tr("n/a"));

                } else {
                    cpw->ftpValue->setText(tr("n/a"));
                    cpw->ftpRank->setText(tr("n/a"));
                }

                // P-MAX and P-MAX ranking
                if (pdModel->hasPMax()) {
                    cpw->pmaxValue->setText(QString(tr("%1 w/kg")).arg(pdModel->PMax(), 0, 'f', 2));

                    // Reference 22.5W/kg -> untrained 8W/kg
                    int _pMaxLevel = 15 * (pdModel->PMax() - 8) / (23-8) ;
                    if (_pMaxLevel > 0 && _pMaxLevel < 16) // check bounds
                        cpw->pmaxRank->setText(QString("%1").arg(_pMaxLevel));
                    else
                        cpw->pmaxRank->setText(tr("n/a"));
                } else  {
                    cpw->pmaxValue->setText(tr("n/a"));
                    cpw->pmaxRank->setText(tr("n/a"));
                }

                // Endurance Index
                if (pdModel->hasWPrime() && pdModel->WPrime() && pdModel->hasCP() && pdModel->CP()) {
                    cpw->eiValue->setText(QString("%1").arg(pdModel->WPrime() / pdModel->CP(), 0, 'f', 0));
                }

            }
        }
    }

    //
    // HEAT
    //
    // we want a heat curve but don't have one
    if (heatCurve == NULL && showHeat && rideSeries == RideFile::watts && bestsCache && bestsCache->heatMeanMaxArray().count()) {
        // heat curve
        heatCurve = new QwtPlotCurve("heat");

        if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true) heatCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

        heatCurve->setBrush(QBrush(GColor(CCP).darker(200)));
        heatCurve->setPen(QPen(Qt::NoPen));
        heatCurve->setZ(-1);

        // generate samples
        QVector<double> heat;
        QVector<double> time;

        for (int i=1; i<bestsCache->meanMaxArray(RideFile::watts).count() && i<bestsCache->heatMeanMaxArray().count(); i++) {

            QwtIntervalSample add(i/60.00f, bestsCache->meanMaxArray(RideFile::watts)[i] - bestsCache->heatMeanMaxArray()[i],
                                  bestsCache->meanMaxArray(RideFile::watts)[i]/* + bestsCache->heatMeanMaxArray()[i]*/);
            time << double(i)/60.00f;
            heat << bestsCache->heatMeanMaxArray()[i];
        }

        heatCurve->setSamples(time, heat);
        heatCurve->setYAxis(yRight);
        setAxisScale(yRight, 0, 100);  // fine if only heat is shown and percentage Scale will be fixed if shown
        if (showPercent) setAxisTitle(yRight, tr("Percent of Best / Heat Rides"));
        else setAxisTitle(yRight, tr("Heat Rides"));
        heatCurve->attach(this);
    }

    setAxisVisible(yRight, showHeat || (showPercent && rideCurve));

   // setAxisVisible(yRight, showHeat || showPercent);

    //
    // HEAT AGE
    //
    // we need a heat by date curve but don't have one
    if (heatAgeCurve == NULL && showHeatByDate && bestsCache) {
        // HeatCurveByDate
        heatAgeCurve = new CpPlotCurve("heat by date");

        if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true)
            heatAgeCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

        heatAgeCurve->setPenWidth(1);
        QwtLinearColorMap *colorMap = new QwtLinearColorMap(Qt::blue, Qt::red);
        heatAgeCurve->setColorMap(colorMap);

        // generate samples
        QVector<QwtPoint3D> heatByDateSamples;

        for (int i=1; i<bestsCache->meanMaxArray(rideSeries).count(); i++) {
            QDate date = bestsCache->meanMaxDates(rideSeries)[i];
            double heat = 1000*(bestsCache->start.daysTo(bestsCache->end)-date.daysTo(bestsCache->end))/(bestsCache->start.daysTo(bestsCache->end));

            QwtPoint3D add(i/60.00f, bestsCache->meanMaxArray(rideSeries)[i], heat);

            heatByDateSamples << add;

        }
        heatAgeCurve->setSamples(heatByDateSamples);
        heatAgeCurve->attach(this);

    }
    zoomer->setZoomBase(false);
}

// our model for combining a model for delta mode
class DeltaModel : public QwtSyntheticPointData
{
    public:

        double x(unsigned int index) const { return baseline->x(index); }
        double y(double t)  const { return us->y(t) - baseline->y(t); }

        // use the same interval and size as the baseline model
        DeltaModel(PDModel *us, PDModel *baseline) : QwtSyntheticPointData(baseline->size()), us(us), baseline(baseline) {
            setInterval(baseline->interval());
        }

    private:
        PDModel *us, *baseline;
};

// in compare mode we can plot models and compare them...
void 
CPPlot::plotModel(QVector<double> vector, QColor plotColor, PDModel *baseline)
{
    // first lets clear any curves we shouldn't be displaying
    // no model curve if not power !
    if (!context->isCompareDateRanges || model == 0 || (rideSeries != RideFile::watts && rideSeries != RideFile::wattsKg)) {
        return;
    }

    // we don't want a model
    if (rideSeries != RideFile::wattsKg && rideSeries != RideFile::watts) return;

    PDModel *pdmodel = NULL; // synthetic data provider for curve

    // new model please
    switch (model) {

    case 1 : // 2 param
        pdmodel = new CP2Model(context);
        break;
    case 2 : // 3 param
        pdmodel = new CP3Model(context);
        break;
    case 3 : // extended model
        pdmodel = new ExtendedModel(context);
        break;
    case 4 : // multimodel
        pdmodel = new MultiModel(context);
        pdmodel->setVariant(modelVariant);
        break;
    }

    // set the model and load data
    pdmodel->setIntervals(sanI1, sanI2, anI1, anI2, aeI1, aeI2, laeI1, laeI2);
    pdmodel->setMinutes(true); // we're minutes here ...
    pdmodel->setData(vector);

    // create curve
    QwtPlotCurve *curve = new QwtPlotCurve("Model");
    if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true)
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    if (baseline) {

        // doing a delta model
        curve->setData(new DeltaModel(pdmodel, baseline));

    } else {

        // set the point data
        curve->setData(pdmodel);
    }

    // curve cosmetics
    QPen pen(plotColor);
    double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
    pen.setWidth(width);
    if (showBest) pen.setStyle(Qt::DashLine);
    curve->setPen(pen);
    curve->attach(this);

    intervalCurves.append(curve);
    zoomer->setZoomBase(false);
}

// wipe away all the curves
void
CPPlot::clearCurves()
{
    // bests ridefilecache
    if (bestsCache) {
        delete bestsCache;
        bestsCache = NULL;
    }

    // model curve
    if (modelCurve) {
        delete modelCurve;
        modelCurve = NULL;
    }

    // ride curve
    if (rideCurve) {
        delete rideCurve;
        rideCurve = NULL;
    }

    // rainbow curve
    if (bestsCurves.count()) {
        foreach (QwtPlotCurve *curve, bestsCurves) delete curve;
        bestsCurves.clear();
    }

    // rainbow labels
    if (allZoneLabels.size()) {
        foreach (QwtPlotMarker *label, allZoneLabels)
        delete label;
        allZoneLabels.clear();
    }

    // heat curves
    if (heatCurve) {
        delete heatCurve;
        heatCurve = NULL;
    }
    if (heatAgeCurve) {
        delete heatAgeCurve;
        heatAgeCurve = NULL;
    }
}

// get bests or an empty set if it is null
QVector<double>
CPPlot::getBests()
{
    if (bestsCache) return bestsCache->meanMaxArray(rideSeries);
    else return QVector<double>();
}

// get bests dates or an empty set if it is null
QVector<QDate>
CPPlot::getBestDates()
{
    if (bestsCache) return bestsCache->meanMaxDates(rideSeries);
    else return QVector<QDate>();
}

// plot the bests curve and refresh the data if needed too
void
CPPlot::plotBests()
{
    // we already drew the bests, if you want them again
    // you need to wipe away whats there buddy
    if (bestsCurves.count()) return;

    // do we need to get the cache ?
    if (bestsCache == NULL) {
        bestsCache = new RideFileCache(context, startDate, endDate, isFiltered, files, rangemode);
    }

    // how much we got ?
    int maxNonZero = 0;
    if (bestsCache->meanMaxArray(rideSeries).size()) {
        for (int i = 0; i < bestsCache->meanMaxArray(rideSeries).size(); ++i) {
            if (bestsCache->meanMaxArray(rideSeries)[i] > 0) maxNonZero = i;
        }
    }

    // no data dude
    if (maxNonZero == 0) return;

    // lets call the curve drawer
    const double *values = bestsCache->meanMaxArray(rideSeries).constData() + 1;

    // we can only do shading of the bests curve
    // when we have power or speed and the user wants it to
    // be a rainbow curve. Otherwise its gonna be plain
    int shadingCP = 0; 
    double shadingRatio = 1.0;
    if ((rideSeries == RideFile::wattsKg || rideSeries == RideFile::watts) && shadeMode) shadingCP = dateCP;
    if (rideSeries == RideFile::wattsKg && shadeMode) shadingRatio = appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble();
    double shadingCV = 0.0;
    if (rideSeries == RideFile::kph && shadeMode) shadingCV = dateCV;

    //For veloclinic plot we need to start by using a 2 parameters model
    if (criticalSeries == CriticalPowerWindow::veloclinicplot) {
        int selectedModel = model;
        if (model == 0)
            model = 1;
        initModel();
        model = selectedModel;
    }


    // lets setup a time array to the size we want to plot the bests curve
    // and do work at the same time since its used in a few places below
    QVector<double> time(maxNonZero);
    QVector<double> work(maxNonZero);
    QVector<double> wprime(maxNonZero);
    for (int t = 0; t < maxNonZero; t++) {
        time[t] = (t+1.00f) / 60.00f;
        work[t] = values[t] * t / 1000; // kJ not Joules
        if (criticalSeries == CriticalPowerWindow::veloclinicplot) {
            wprime[t] = (values[t]<pdModel->CP()?0:(values[t]-pdModel->CP()) * time[t] * 60.0); // Joules
        }
    }

    if (showBest) {
        if (shadingCP == 0 && shadingCV == 0.0) {

            // PLAIN CURVE

            // if we're plotting work we need to adjust from
            // power to work from the bests cache, before we
            // set the curve samples.
            //

            // no zones wanted
            QwtPlotCurve *curve = new QwtPlotCurve(tr("Bests"));

            if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true)
                curve->setRenderHint(QwtPlotItem::RenderAntialiased);

            // lets make it the right colour for the date series
            QPen line;
            QColor fill;
            switch (rideSeries) {

            case RideFile::kphd:
                line.setColor(GColor(CACCELERATION).darker(200));
                fill = (GColor(CACCELERATION));
                break;

            case RideFile::kph:
                line.setColor(GColor(CSPEED).darker(200));
                fill = (GColor(CSPEED));
                break;

            case RideFile::cad:
            case RideFile::cadd:
                line.setColor(GColor(CCADENCE).darker(200));
                fill = (GColor(CCADENCE));
                break;

            case RideFile::nm:
            case RideFile::nmd:
                line.setColor(GColor(CTORQUE).darker(200));
                fill = (GColor(CTORQUE));
                break;

            case RideFile::hr:
            case RideFile::hrd:
                line.setColor(GColor(CHEARTRATE).darker(200));
                fill = (GColor(CHEARTRATE));
                break;

            case RideFile::vam:
                line.setColor(GColor(CALTITUDE).darker(200));
                fill = (GColor(CALTITUDE));
                break;

            default:
            case RideFile::watts:
                line.setColor(GColor(CCP));
                fill = (GColor(CCP));
                break;
            case RideFile::wattsd:
            case RideFile::NP:
            case RideFile::xPower:
                line.setColor(GColor(CPOWER).darker(200));
                fill = (GColor(CPOWER));
                break;
            }

            // when plotting power bests AND a model we draw bests as dots
            // but only if in 'plain' mode .. not doing a rainbow curve.
            if ((rideSeries == RideFile::wattsKg || rideSeries == RideFile::watts) && model) {

                QwtSymbol *sym = new QwtSymbol;
                sym->setStyle(QwtSymbol::Ellipse);
                sym->setSize(4);
                sym->setBrush(QBrush(fill));
                sym->setPen(QPen(fill));
                curve->setSymbol(sym);
                curve->setStyle(QwtPlotCurve::Dots);
            }

            fill.setAlpha(64);
            line.setWidth(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());

            curve->setPen(line);
            if (rideSeries == RideFile::watts || rideSeries == RideFile::wattsKg)
                curve->setBrush(Qt::NoBrush);
            else
                curve->setBrush(QBrush(fill));

            if (criticalSeries == CriticalPowerWindow::work)
                curve->setSamples(time, work);
            else if (criticalSeries == CriticalPowerWindow::veloclinicplot)
                curve->setSamples(bestsCache->meanMaxArray(rideSeries).data()+1, wprime.data(), maxNonZero-1);
            else
                curve->setSamples(time.data(), bestsCache->meanMaxArray(rideSeries).data()+1, maxNonZero-1);

            curve->attach(this);
            bestsCurves.append(curve);

        } else if (shadingCP > 0) {

            //
            // RAINBOW CURVE We are plotting power AND the user wants a rainbow
            //

            // set zones from shading CP
            QList <int> power_zone;
            int n_zones = context->athlete->zones()->lowsFromCP(&power_zone, (int) int(shadingCP));

            // now run through each zone and create a curve
            int high = maxNonZero - 1;
            int zone = 0;
            while (zone < n_zones && high > 0) {

                // create the curve
                QwtPlotCurve *curve = new QwtPlotCurve("");
                bestsCurves.append(curve);
                curve->attach(this);

                // get range for the curve
                int low = high - 1;
                int nextZone = zone + 1;
                if (nextZone >= power_zone.size())
                    low = 0;
                else {
                    while ((low > 0) && (values[low] < power_zone[nextZone]/shadingRatio))
                        --low;
                }

                // set samples
                if (criticalSeries == CriticalPowerWindow::work) { // this is Energy mode
                    curve->setSamples(time.data() + low, work.data() + low, high - low + 1);
                } if (criticalSeries == CriticalPowerWindow::veloclinicplot) {
                    curve->setSamples(values + low, wprime.data() + low, high - low + 1);
                } else {
                    curve->setSamples(time.data() + low, values + low, high - low + 1);
                }

                // set the pen color and line width etc
                QColor color = zoneColor(zone, n_zones);
                if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true)
                    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
                QPen pen(color.darker(200));
                pen.setColor(GColor(CCP)); //XXX color ?
                double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
                pen.setWidth(width);
                curve->setPen(pen);

                // use a linear gradient
                if (shadeMode && shadingCP) { // 0 value means no shading please - and only if proper value for shadingCP
                    color.setAlpha(128);
                    QColor color1 = color.darker();
                    QLinearGradient linearGradient(0, 0, 0, height());
                    linearGradient.setColorAt(0.0, color);
                    linearGradient.setColorAt(1.0, color1);
                    linearGradient.setSpread(QGradient::PadSpread);
                    curve->setBrush(linearGradient);   // fill below the line
                }

                // now the labels
                if (shadeMode && (criticalSeries != CriticalPowerWindow::work || work[high] > 100.0)) {

                    QwtText text(context->athlete->zones()->getDefaultZoneName(zone));
                    text.setFont(QFont("Helvetica", 20, QFont::Bold));
                    color.setAlpha(255);
                    text.setColor(color);
                    QwtPlotMarker *label_mark = new QwtPlotMarker();

                    // place the text in the geometric mean in time, at a decent power
                    double x, y;
                    if (criticalSeries == CriticalPowerWindow::work) {
                        x = (time[low] + time[high]) / 2;
                        y = (work[low] + work[high]) / 5;
                    } else if (criticalSeries == CriticalPowerWindow::veloclinicplot) {
                        x = (values[low] + values[high]) / 2;
                        if (wprime[high]<1000 && wprime[low]<1000)
                            y = -1000;
                        else
                            y = pdModel->WPrime()/4;
                    } else {
                        x = sqrt(time[low] * time[high]);
                        y = (values[low] + values[high]) / 5;
                    }

                    label_mark->setValue(x, y);
                    label_mark->setLabel(text);
                    label_mark->attach(this);
                    allZoneLabels.append(label_mark);
                }
    
                high = low;
                ++zone;
            }
        } else if (shadingCV > 0.0) {

            //
            // RAINBOW CURVE We are plotting speed AND the user wants a rainbow
            //

            // set zones from shading CV
            QList <double> pace_zone;
            int n_zones = context->athlete->paceZones()->lowsFromCV(&pace_zone, shadingCV);

            // now run through each zone and create a curve
            int high = maxNonZero - 1;
            int zone = 0;
            while (zone < n_zones && high > 0) {

                // create the curve
                QwtPlotCurve *curve = new QwtPlotCurve("");
                bestsCurves.append(curve);
                curve->attach(this);

                // get range for the curve
                int low = high - 1;
                int nextZone = zone + 1;
                if (nextZone >= pace_zone.size())
                    low = 0;
                else {
                    while ((low > 0) && (values[low] < pace_zone[nextZone]))
                        --low;
                }

                // set samples
                curve->setSamples(time.data() + low, values + low, high - low + 1);

                // set the pen color and line width etc
                QColor color = paceZoneColor(zone, n_zones);
                if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true)
                    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
                QPen pen(color.darker(200));
                pen.setColor(GColor(CCP)); //XXX color ?
                double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
                pen.setWidth(width);
                curve->setPen(pen);

                // use a linear gradient
                if (shadeMode && shadingCV) { // 0 value means no shading please - and only if proper value for shadingCV
                    color.setAlpha(128);
                    QColor color1 = color.darker();
                    QLinearGradient linearGradient(0, 0, 0, height());
                    linearGradient.setColorAt(0.0, color);
                    linearGradient.setColorAt(1.0, color1);
                    linearGradient.setSpread(QGradient::PadSpread);
                    curve->setBrush(linearGradient);   // fill below the line
                }

                // now the labels
                if (shadeMode) {

                    QwtText text(context->athlete->paceZones()->getDefaultZoneName(zone));
                    text.setFont(QFont("Helvetica", 20, QFont::Bold));
                    color.setAlpha(255);
                    text.setColor(color);
                    QwtPlotMarker *label_mark = new QwtPlotMarker();

                    // place the text in the geometric mean in time, at a decent power
                    double x, y;
                    x = sqrt(time[low] * time[high]);
                    y = (values[low] + values[high]) / 5;

                    label_mark->setValue(x, y);
                    label_mark->setLabel(text);
                    label_mark->attach(this);
                    allZoneLabels.append(label_mark);
                }

                high = low;
                ++zone;
            }
        }
}


    // X-AXIS

    // now sort the axis for the bests curve
    double xmin = 1.0f/60.0f - 0.001f;
    double xmax = time[maxNonZero - 1];

    // truncate at an hour for energy mode
    if (criticalSeries == CriticalPowerWindow::work) xmax = 60.0;

    // not interested in short durations for vam
    if (criticalSeries == CriticalPowerWindow::vam) xmin = 4.993;

    // now set the scale
    QwtScaleDiv div((double)xmin, (double)xmax);
    if (criticalSeries == CriticalPowerWindow::work)
        div.setTicks(QwtScaleDiv::MajorTick, LogTimeScaleDraw::ticksEnergy);
    else
        div.setTicks(QwtScaleDiv::MajorTick, LogTimeScaleDraw::ticks);
    setAxisScaleDiv(QwtPlot::xBottom, div);

    if (criticalSeries == CriticalPowerWindow::veloclinicplot) {
        double xmax;
        xmax = 100 * ceil(values[0] / 100);
        if (xmax == 100) xmax = 5 * ceil(values[0] / 5);

        // set ymax to nearest 100 if power
        int max = xmax * 1.1f;
        max = ((max/100) + 1) * 100;

        setAxisScale(QwtPlot::xBottom, 0, max);
    }

    // Y-AXIS

    double ymax;
    if (criticalSeries == CriticalPowerWindow::work) {
        int i = std::lower_bound(time.begin(), time.end(), 60.0) - time.begin();
        ymax = 10 * ceil(work[i] / 10);
    } else if (criticalSeries == CriticalPowerWindow::vam) {
        double yVam = bestsCache->meanMaxArray(RideFile::vam).value(296);
        ymax = 100 * ceil(yVam / 100); // index of xMin for Time
    } else {
        ymax = 100 * ceil(values[0] / 100);
        if (ymax == 100) ymax = 5 * ceil(values[0] / 5);
    }

    // adjust if for power
    if (rideSeries == RideFile::watts && criticalSeries != CriticalPowerWindow::veloclinicplot) {

        // set ymax to nearest 100 if power
        int max = ymax * 1.1f;
        max = ((max/100) + 1) * 100;

        setAxisScale(yLeft, 0, max);
    }
    else if (criticalSeries == CriticalPowerWindow::veloclinicplot) {
        setAxisScale(yLeft, 0, 1.5*pdModel->WPrime());
    }
    else if (criticalSeries == CriticalPowerWindow::vam) {
        // VAM is very big anyway - so just 5% headroom
        setAxisScale(yLeft, 0, 1.05*ymax);
    } else {

        // or just add 10% headroom
        setAxisScale(yLeft, 0, 1.1*values[0]);
    }
    zoomer->setZoomBase(false);
}

// plot the currently selected ride
void
CPPlot::plotRide(RideItem *rideItem)
{
    // currently selected ride wanted ?
    if (!rideItem || rangemode || plotType == 2) return;

    // if its already plotted we don't do it again
    // it is wiped when setRide is called to force
    // a replot
    if (rideCurve) return;

    // there is not data to plot!
    if (rideCache->meanMaxArray(rideSeries).size() == 0) return;

    // check what we do have to plot
    int maxNonZero = 0;
    QVector<double> timeArray(rideCache->meanMaxArray(rideSeries).size());
    for (int i = 0; i < rideCache->meanMaxArray(rideSeries).size(); ++i) {
        timeArray[i] = i / 60.0;
        if (rideCache->meanMaxArray(rideSeries)[i] > 0) maxNonZero = i;
    }

    // do we have nonzero data to plot ?
    if (maxNonZero == 1) return;

    // Right, lets actually plot the ride
    rideCurve = new QwtPlotCurve(rideItem->dateTime.toString(tr("ddd MMM d, yyyy hh:mm")));
    rideCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    rideCurve->setBrush(QBrush(Qt::NoBrush)); // never filled

    // what color and fill do we have for the ride ?
    // there is a specific colour setting for the "ride curve" on
    // the CP plot, regardless of the series. Its only the bests
    // curve that gets any special colour treatment.
    QPen ridePen;
    ridePen.setColor(GColor(CRIDECP));
    double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
    ridePen.setWidth(width);
    rideCurve->setPen(ridePen);

    // set the curve samples
    if (criticalSeries == CriticalPowerWindow::work) {

        // WORK

        QVector<double> energyArray(rideCache->meanMaxArray(RideFile::watts).size());
        for (int i = 0; i <= maxNonZero; ++i) {
            energyArray[i] = timeArray[i] * rideCache->meanMaxArray(RideFile::watts)[i] * 60.0 / 1000.0;
        }
        rideCurve->setSamples(timeArray.data() + 1, energyArray.constData() + 1,
                              maxNonZero > 0 ? maxNonZero-1 : 0);

    } else if (criticalSeries == CriticalPowerWindow::veloclinicplot) {

        // Veloclinic plot

        QVector<double> array(rideCache->meanMaxArray(RideFile::watts).size());
        for (int i = 0; i <= maxNonZero; ++i) {
            array[i] =  (rideCache->meanMaxArray(rideSeries)[i]<pdModel->CP()?0:(rideCache->meanMaxArray(rideSeries)[i]-pdModel->CP()) * timeArray[i] * 60.0);
        }
        rideCurve->setSamples(rideCache->meanMaxArray(rideSeries).constData()  + 1, array.constData() + 1,
                              maxNonZero > 0 ? maxNonZero-1 : 0);
    } else  {

        // ALL OTHER RIDE SERIES

        // AS A PERCENTAGE
        // we want as a percent of best and we do have
        // the bests available
        if (showPercent && bestsCache) {

            QVector<double> samples(timeArray.size());

            // percentify from the cache
            for(int i=0; i <samples.size() && i < rideCache->meanMaxArray(rideSeries).size() &&
                    i <bestsCache->meanMaxArray(rideSeries).size(); i++) {

                samples[i] = rideCache->meanMaxArray(rideSeries)[i] /
                             bestsCache->meanMaxArray(rideSeries)[i] * 100.00f;
            }
            rideCurve->setSamples(timeArray.data() + 1, samples.data() + 1,
                                  maxNonZero > 0 ? maxNonZero-1 : 0);

            // did we get over 100% .. because if so
            // we need to set the maxY on the RHS to reflect that
            int max = rideCurve->maxYValue();
            if (max < 100) max = 100;
            else max = max * 1.05f;
            setAxisScale(yRight, 0, max); // always 100

            // set the right titles in case both Heat and Percent of best is show
            if (showHeat) setAxisTitle(yRight, tr("Percent of Best / Heat Rides"));
            else setAxisTitle(yRight, tr("Percent of Best"));

        } else {

            // JUST A NORMAL CURVE
            rideCurve->setYAxis(yLeft);
            rideCurve->setSamples(timeArray.data() + 1, rideCache->meanMaxArray(rideSeries).constData() + 1,
                                  maxNonZero > 0 ? maxNonZero-1 : 0);

            // Set the YAxis Title if Heat is active
            if (showHeat) setAxisTitle(yRight, tr("Heat Rides"));
        }
    }

    // which axis should it be on?
    // and also make sure its visible
    rideCurve->setYAxis(showPercent ? yRight : yLeft);
    setAxisVisible(yRight, showPercent || showHeat);
    rideCurve->attach(this);

    zoomer->setZoomBase(false);
}

// notified that the user selected a ride
void
CPPlot::setRide(RideItem *rideItem)
{
    // null ride ?
    if (!rideItem) return;

    // Season Compare Mode -- so nothing for us to do
    if (rangemode && context->isCompareDateRanges) return calculateForDateRanges(context->compareDateRanges);

    // Interval Compare Mode -- so go do that instead
    if (!rangemode && context->isCompareIntervals) return calculateForIntervals(context->compareIntervals);

    // clear the old ride curve and cache
    // regardless, as its no longer relevant
    // we may not create new ones but at least
    // we will always remove out of date info
    if (rideCurve) {
        rideCurve->detach();
        delete rideCurve;
        rideCurve = NULL;
    }
    if (rideCache) {
        delete rideCache;
        rideCache = NULL;
    }
    // clear any centile and interval curves
    // since they will be for an old ride
    foreach(QwtPlotCurve *c, centileCurves) {
        c->detach();
        delete c;
    }
    centileCurves.clear();
    foreach(QwtPlotCurve *c, intervalCurves) {
        c->detach();
        delete c;
    }
    intervalCurves.clear();

    // MAKE SURE BESTS IS UP TO DATE FIRST AS WE REFERENCE IT
    // first make sure the bests cache is up to date as we may need it
    // if plotting in percentage mode, so get data and plot it now
    plotBests();

    // do we actually have something to plot?
    if (rideItem && rideItem->ride() && rideItem->ride()->dataPoints().count()) {

        // NOW PLOT OUR CURVE
        // We are a percent or plain curve
        switch (plotType) {

        case 0 :
            {
                // MEANMAX
                // Plot as normal or percent
                rideCache = new RideFileCache(context, context->athlete->home->cache().canonicalPath() + "/" + rideItem->fileName);
                plotRide(rideItem);
                refreshReferenceLines(rideItem);
            }
            break;

        case 1 :
            {
                // CENTILE
                // calculates all the data from the raw ride data, so doesn't need
                // a cache and doesn't make sense to plot reference lines
                plotCentile(rideItem);
            }
            break;

        case 2 :
            {
                // NONE
                // make sure there is no ride curve plotted then
                // NOTE: It was already wiped away at the beginning
                //       of this method so there really is nothing
                //       left to do !
            }
            break;

        }
    }

    // NOW PLOT THE MODEL CURVE
    // it will need to decide if it is relevant etc
    // but we need to make sure we have it
    plotModel();

    // now replot please
    replot();
}

// the picker hovered over a point on a curve
void
CPPlot::pointHover(QwtPlotCurve *curve, int index)
{
    if (criticalSeries == CriticalPowerWindow::watts && showBest && curve == modelCurve && modelCurve != NULL) 
        return; // ignore model curve hover

    if (index >= 0) {

        double xvalue = curve->sample(index).x();
        double yvalue = curve->sample(index).y();
        QString text, dateStr, paceStr;
        QString currentRidePercentStr;
        QString units1;
        QString units2;

        // add when to tooltip if its all curve
        if (bestsCurves.contains(curve)) {
            int index = xvalue * 60;
            if (index >= 0 && bestsCache && getBests().count() > index) {
                QDate date = getBestDates()[index];
                dateStr = date.toString(tr("\nddd, dd MMM yyyy"));
            }
        }

        if (criticalSeries == CriticalPowerWindow::veloclinicplot)
            units1 = RideFile::unitName(rideSeries, context);
        else
            units1 = ""; // time --> no units

        // show percent ?
        if ((((rangemode && context->isCompareDateRanges)
            || (!rangemode && context->isCompareIntervals)) && showDelta && showDeltaPercent)
            || (curve == rideCurve && showPercent)) units2 = QString("%");
        else if (criticalSeries == CriticalPowerWindow::veloclinicplot)
            units2 = "J"; // Joule
        else if (criticalSeries == CriticalPowerWindow::kph)
            units2 = tr("kph"); // yAxis doesn't obey units settings yet, remove when fixed
        else
            units2 = RideFile::unitName(rideSeries, context);

		// for the current ride curve, add a percent of rider's actual best.
		if (!showPercent && curve == rideCurve && index >= 0 && getBests().count() > index) {
			double bestY = getBests()[index];
			if (0 != bestY) {
				// use 0 decimals for the percent.
				currentRidePercentStr = QString("\n%1 %2")
					.arg((yvalue *100)/ bestY, 0, 'f', 0)
					.arg(tr("Percent of Best"));
			}
		}

        // no units for Heat Curve
        if (curve == heatCurve) units2 = QString(tr("Rides"));

        // for speed series add pace with units according to settings
        if (criticalSeries == CriticalPowerWindow::kph) {
            bool metricPace = appsettings->value(this, GC_PACE, true).toBool();
            QString paceunit = metricPace ? tr("min/km") : tr("min/mile");
            // yAxis doesn't obey units yet
            // when fixed, change true to: context->athlete->useMetricUnits
            paceStr = QString("\n%1 %2").arg(true ? kphToPace(yvalue, metricPace) : mphToPace(yvalue, metricPace)).arg(paceunit);
        }

        // output the tooltip
        text = QString("%1%2\n%3 %4%7%5%6")
               .arg(criticalSeries == CriticalPowerWindow::veloclinicplot?QString("%1").arg(xvalue, 0, 'f', RideFile::decimalsFor(rideSeries)):interval_to_str(60.0*xvalue))
               .arg(units1)
               .arg(yvalue, 0, 'f', RideFile::decimalsFor(rideSeries))
               .arg(units2)
               .arg(dateStr)
               .arg(currentRidePercentStr)
               .arg(paceStr);

        // set that text up
        zoomer->setText(text);
        return;
    }
    // no point
    zoomer->setText("");
}

void
CPPlot::exportBests(QString filename)
{
    QFile f(filename);

    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return; // couldn't open file

    // do we want to export the model estimate too ?
    bool expmodel = (model && pdModel && (rideSeries == RideFile::wattsKg || rideSeries == RideFile::watts));

    // open stream and write header
    QTextStream stream(&f);
    stream << "seconds, value," << (expmodel ? " model, date" : " date") << endl;

    // output a row for each second
    foreach(QwtPlotCurve *bestsCurve, bestsCurves) {

        // just output for the bests curve
        for (size_t i=0; i<bestsCurve->data()->size(); i++) {
            double xvalue = bestsCurve->sample(i).x();
            double yvalue = bestsCurve->sample(i).y();
            double modelvalue = expmodel ? pdModel->y(xvalue) : 0;

            int index = xvalue * 60.00f;
            QDate date;
            if (index >= 0 && bestsCache && getBests().count() > index) {
                date = getBestDates()[index];
            }

            // values
            if (expmodel) stream << int(xvalue * 60.00f) << "," << yvalue << "," << modelvalue << "," << date.toString() << endl;
            else stream << int(xvalue * 60.00f) << "," << yvalue << "," << date.toString() << endl;
        }
    }

    // and we're done
    f.close();
}

// no filter
void
CPPlot::clearFilter()
{
    isFiltered = false;
    files.clear();
    delete bestsCache;
    bestsCache = NULL;
    clearCurves();
}

// set a filter
void
CPPlot::setFilter(QStringList list)
{
    isFiltered = true;
    files = list;
    delete bestsCache;
    bestsCache = NULL;
    clearCurves();
}

void
CPPlot::setShowHeat(bool x)
{
    showHeat = x;
    clearCurves();
}

void
CPPlot::setShowBest(bool x)
{
    showBest = x;
    clearCurves();
}

void
CPPlot::setShowPercent(bool x)
{
    showPercent = x;
}

void
CPPlot::setShowDelta(bool delta, bool percent)
{
    showDelta = delta;
    showDeltaPercent = percent;
    setSeries(this->criticalSeries); // y-axis
}

void
CPPlot::setShowHeatByDate(bool x)
{
    showHeatByDate = x;
    clearCurves();
}


void
CPPlot::setShadeMode(int x)
{
    shadeMode = x;
    clearCurves();
}

void
CPPlot::setShadeIntervals(int x)
{
    shadeIntervals = x;
    clearCurves();
}

// model parameters!
void
CPPlot::setModel(int sanI1, int sanI2, int anI1, int anI2, int aeI1, int aeI2, int laeI1, int laeI2, int model, int variant)
{
    this->anI1 = double(anI1);
    this->anI2 = double(anI2);
    this->aeI1 = double(aeI1);
    this->aeI2 = double(aeI2);

    this->sanI1 = double(sanI1);
    this->sanI2 = double(sanI2);
    this->laeI1 = double(laeI1);
    this->laeI2 = double(laeI2);

    this->model = model;
    this->modelVariant = variant;

    clearCurves();
}

void
CPPlot::refreshReferenceLines(RideItem *rideItem)
{
    // we only do refs for a specific ride
    if (rangemode) return;

    // wipe existing
    foreach(QwtPlotMarker *referenceLine, referenceLines) {
        referenceLine->detach();
        delete referenceLine;
    }
    referenceLines.clear();

    if (!rideItem && !rideItem->ride()) return;

    // horizontal lines at reference points
    if (rideSeries == RideFile::aPower || rideSeries == RideFile::xPower || rideSeries == RideFile::NP || rideSeries == RideFile::watts  || rideSeries == RideFile::wattsKg) {

        if (rideItem->ride()) {
            foreach(const RideFilePoint *referencePoint, rideItem->ride()->referencePoints()) {

                if (referencePoint->watts != 0) {
                    QwtPlotMarker *referenceLine = new QwtPlotMarker;
                    QPen p;
                    p.setColor(GColor(CPLOTMARKER));
                    double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
                    p.setWidth(width);
                    p.setStyle(Qt::DashLine);
                    referenceLine->setLinePen(p);
                    referenceLine->setLineStyle(QwtPlotMarker::HLine);
                    referenceLine->setYValue(referencePoint->watts);
                    referenceLine->attach(this);
                    referenceLines.append(referenceLine);
                }
            }
        }
    }
}

// plot mean max, centile or none!
void
CPPlot::setPlotType(int index)
{
    plotType = index;
    clearCurves();
}

// calculate and plot a centile plot
void
CPPlot::plotCentile(RideItem *rideItem)
{
    qDebug() << "calculateCentile";
    QTime elapsed;
    elapsed.start();

    qDebug() << "prepare datas ";
    cpintdata data;
    data.rec_int_ms = (int) round(rideItem->ride()->recIntSecs() * 1000.0);
    double lastsecs = 0;
    bool first = true;
    double offset = 0;

    foreach (const RideFilePoint *p, rideItem->ride()->dataPoints()) {

        // get offset to apply on all samples if first sample
        if (first == true) {
            offset = p->secs;
            first = false;
        }

        // drag back to start at 0s
        double psecs = p->secs - offset;

        // fill in any gaps in recording - use same dodgy rounding as before
        int count = (psecs - lastsecs - rideItem->ride()->recIntSecs()) / rideItem->ride()->recIntSecs();

        // gap more than an hour, damn that ride file is a mess
        if (count > 3600) count = 1;

        for(int i=0; i<count; i++) {
            data.points.append(cpintpoint(round(lastsecs+((i+1)*rideItem->ride()->recIntSecs() *1000.0)/1000), 0));
        }

        lastsecs = psecs;

        double secs = round(psecs * 1000.0) / 1000;
        if (secs > 0)  {
            if (round(p->value(RideFile::watts))>1400)
                qDebug() << "append point " <<  round(p->value(RideFile::watts)) ;
            data.points.append(cpintpoint(secs, (int) round(p->value(RideFile::watts))));
        }
    }

    int total_secs = (int) ceil(rideItem->ride()->dataPoints().back()->secs);

    QVector < QVector<double> > ride_centiles(10);
    // Initialisation
    for (int i = 0; i < ride_centiles.size(); ++i) {
        ride_centiles[i] = QVector <double>(total_secs);
    }

    qDebug() << "end prepare datas " << elapsed.elapsed();
    qDebug() << "calcul for first 6min ";

    // loop through the decritized data from top
    // FIRST 6 MINUTES DO BESTS FOR EVERY SECOND
    // WE DO NOT DO THIS FOR NP or xPower SINCE
    // IT IS WELL KNOWN THAT THEY ARE NOT VALID
    // FOR SUCH SHORT DURATIONS AND IT IS VERY
    // CPU INTENSIVE, SO WE DON'T BOTHER

    double samplerate = rideItem->ride()->recIntSecs();

    for (int slice = 1; slice < 360;) {
        int windowsize = slice / samplerate;
        QVector<double> sums(data.points.size()-windowsize+1);

        int index=0;
        double sum=0;

        for (int i=0; i<data.points.size(); i++) {
            sum += data.points[i].value;

            if (i>windowsize-1)
                sum -= data.points[i-windowsize].value;

            if (i>=windowsize-1) {
                sums[index++] = sum/windowsize;
            }

        }
        //qSort(sums.begin(), sums.end());
        qSort(sums);

        qDebug() << "sums (" << slice << ") : " << sums.size() << " max " << sums[sums.size()-1];

        ride_centiles[9][slice] = sums[sums.size()-1];

        for (int i = ride_centiles.size()-1; i > 0; --i) {
            sum = 0;
            int count = 0;

            for (int n = (0.1*i)*sums.size(); n < sums.size()-1 && n < (0.1*(i+1))*sums.size(); ++n) {
                sum += sums[n];
                count++;
            }
            if (sum > 0) {
                if (sum > 0) {
                    double avg = sum / count;
                    ride_centiles[i-1][slice]=avg;
                }
            } else {
                ride_centiles[i-1][slice]=ride_centiles[i][slice];
            }
        }

        slice ++;
    }

    qDebug() << "end calcul for first 6min " << elapsed.elapsed();
    qDebug() << "downsampling to 5s after 6min ";

    QVector<double> downsampled(0);

    // moving to 5s samples would INCREASE the work...
    if (rideItem->ride()->recIntSecs() >= 5) {
        samplerate = rideItem->ride()->recIntSecs();
        for (int i=0; i<data.points.size(); i++)
            downsampled.append(data.points[i].value);
    } else {
        // moving to 5s samples is DECREASING the work...
        samplerate = 5;
        // we are downsampling to 5s
        long five=5; // start at 1st 5s sample
        double fivesum=0;

        int fivecount=0;

        for (int i=0; i<data.points.size(); i++) {
            if (data.points[i].secs <= five) {
                fivesum += data.points[i].value;
                fivecount++;
            } else {
                downsampled.append(fivesum / fivecount);
                fivecount = 1;
                fivesum = data.points[i].value;

                five += 5;
            }
        }
    }

    qDebug() << "end downsampling to 5s after 6min " << elapsed.elapsed();
    qDebug() << "calcul for rest of ride ";

    for (int slice = 360; slice < ride_centiles[9].size();) {
        int windowsize = slice / samplerate;
        QVector<double> sums(downsampled.size()-windowsize+2);


        int index=0;
        double sum=0;

        for (int i=0; i<downsampled.size(); i++) {
            sum += downsampled[i];

            if (i>windowsize-1)
                sum -= downsampled[i-windowsize];
            if (i>=windowsize-1)
                sums[index++] = sum / windowsize;

        }
        //qSort(sums.begin(), sums.end());
        qSort(sums);

        qDebug() << "sums (" << slice << ") : " << sums.size() << " max " << sums[sums.size()-1];



        ride_centiles[9][slice] = sums[sums.size()-1];

        for (int i = ride_centiles.size()-1; i > 0; --i) {
            sum = 0;
            int count = 0;

            for (int n = (0.1*i)*sums.size(); n < sums.size() && n < (0.1*(i+1))*sums.size(); ++n) {
                if (sums[n]>0)  {
                    sum += sums[n];
                    count++;
                }
            }
            if (sum > 0) {
                double avg = sum / count;
                ride_centiles[i-1][slice]=avg;
            } else {
                ride_centiles[i-1][slice]=ride_centiles[i][slice];
            }
        }



        // increment interval duration we are going to search
        // for next, gaps increase as duration increases to
        // reduce overall work, since we require far less
        // precision as the ride duration increases
        if (slice < 3600) slice +=20; // 20s up to one hour
        else if (slice < 7200) slice +=60; // 1m up to two hours
        else if (slice < 10800) slice += 300; // 5mins up to three hours
        else slice += 600; // 10mins after that
    }

    qDebug() << "end calcul for rest of ride " << elapsed.elapsed();
    qDebug() << "fill gaps ";

    /*for (int i = 0; i<ride_centiles.size(); i++) {
        double last=0.0;
        for (int j=ride_centiles[i].size()-1; j; j--) {
            if (ride_centiles[i][j] == 0) ride_centiles[i][j]=last;
            else last = ride_centiles[i][j];
        }
    }*/

    for (int i = ride_centiles.size()-1; i>=0; i--) {
        double last=0.0;
        for (int j=0; j<ride_centiles[i].size(); j++) {
            if (ride_centiles[i][j] == 0) ride_centiles[i][j]=last;
            else last = ride_centiles[i][j];
        }
    }

    qDebug() << "end fill gaps " << elapsed.elapsed();
    qDebug() << "plotting ";


    for (int i = 0; i<ride_centiles.size(); i++) {
        int maxNonZero = 0;
        QVector<double> timeArray(ride_centiles[i].size());
        for (int j = 0; j < ride_centiles[i].size(); ++j) {
            timeArray[j] = j / 60.0;
            if (ride_centiles[i][j] > 0) maxNonZero = j;
        }

        if (maxNonZero > 1) {

            QwtPlotCurve *rideCurve = new QwtPlotCurve(tr("%10 %").arg(i+1));
            rideCurve->setRenderHint(QwtPlotItem::RenderAntialiased);

            // red hue to cp curve color
            QColor std = GColor(CRIDECP);
            QPen pen(QColor(250-(i*20),std.green(),std.blue()));
            pen.setStyle(Qt::DashLine); // Qt::SolidLine
            double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
            pen.setWidth(width);
            rideCurve->setPen(pen);
            rideCurve->attach(this);


            rideCurve->setSamples(timeArray.data() + 1, ride_centiles[i].constData() + 1, maxNonZero - 1);
            centileCurves.append(rideCurve);
        }
    }


    qDebug() << "end plotting " << elapsed.elapsed();
    zoomer->setZoomBase(false);
}

void
CPPlot::calculateForDateRanges(QList<CompareDateRange> compareDateRanges)
{
    if (!rangemode) return;

    // zap old curves
    clearCurves();
    foreach(QwtPlotCurve *c, intervalCurves) {
        c->detach();
        delete c;
    }
    intervalCurves.clear();
    // don't need to zap old models as they
    // are deleted with the curve !

    // If no range
    if (compareDateRanges.count() == 0) {
        replot(); // show cleared plot
        return;
    }

    double ymax = 0;
    double ymin = 0;

    // when delta mode is invoked we compare to a baseline curve
    QVector<double> baseline;
    PDModel *baselineModel = NULL;

    if (showDelta && compareDateRanges.count()) {

        // set the baseline data
        CompareDateRange range = compareDateRanges.at(0);
        baseline = range.rideFileCache()->meanMaxArray(rideSeries);

        if (model && (rideSeries == RideFile::watts || rideSeries == RideFile::wattsKg)) {

            // get a model
            switch (model) {
            case 1 : // 2 param
                baselineModel = new CP2Model(context);
                break;
            case 2 : // 3 param
                baselineModel = new CP3Model(context);
                break;
            case 3 : // extended model
                baselineModel = new ExtendedModel(context);
                break;
            case 4 : // multimodel
                baselineModel = new MultiModel(context);
                baselineModel->setVariant(modelVariant);
                break;
            }

            // feed the data into the model
            if (baselineModel) {

                // set the model and load data
                baselineModel->setIntervals(sanI1, sanI2, anI1, anI2, aeI1, aeI2, laeI1, laeI2);
                baselineModel->setMinutes(true); // we're minutes here ...
                baselineModel->setData(baseline);
            }
        }
    }

    double xmin = 1.0f/60.0f - 0.001f;
    double xmax = 0;

    // prepare aggregates
    for (int j = 0; j < compareDateRanges.size(); ++j) {

        CompareDateRange range = compareDateRanges.at(j);

        if (range.isChecked())  {

            RideFileCache *cache = range.rideFileCache();

            // create a delta array
            if (showDelta && cache) {

                int n=0;
                QVector<double> deltaArray = cache->meanMaxArray(rideSeries);

                // make a delta to baseline
                for (n=1; n < deltaArray.size() && n < baseline.size(); n++) {
                    // stop when we get to zero!
                    if (deltaArray[n] > 0 && baseline[n] > 0) {

                        if (showDeltaPercent) deltaArray[n] = 100.00f * (double(deltaArray[n]) - double(baseline[n])) / double(baseline[n]); // delta percentage
                        else deltaArray[n] = deltaArray[n] - baseline[n];

                    } else
                        break;
                }
                deltaArray.resize(n-1);

                // now plot using the delta series and NOT the cache
                if (showBest) plotCache(deltaArray, range.color);

                // and plot a model too -- its neat to compare them...
                if (rideSeries == RideFile::watts || rideSeries == RideFile::wattsKg) {
                    plotModel(cache->meanMaxArray(rideSeries), range.color, baselineModel);
                }

                foreach(double v, deltaArray) {
                    if (v > ymax) ymax = v;
                    if (v < ymin) ymin = v;
                }

                // keep track of longest point
                int imax = cache->meanMaxArray(rideSeries).count()/60.00f;
                if (cache && imax > xmax) xmax = imax;

            }

            if (!showDelta && cache->meanMaxArray(rideSeries).size()) {

                // plot the bests if we want them
                if (showBest) plotCache(cache->meanMaxArray(rideSeries), range.color);

                // and plot a model too -- its neat to compare them...
                if (rideSeries == RideFile::watts || rideSeries == RideFile::wattsKg)
                    plotModel(cache->meanMaxArray(rideSeries), range.color, NULL);

                int xCount = 0;
                double vamYMax = 0;
                foreach(double v, cache->meanMaxArray(rideSeries)) {
                    // get the biggest y-Axes value
                    if (v > ymax) ymax = v;

                    // VAM x-Axis starts at 4.993 seconds = x-time array point 296 -
                    // so skip the values upto that point in x-Axis time
                    if (rideSeries == RideFile::vam) {
                       // VAM x-Axis starts at 4.993 seconds = x-time array point 296 -
                       // so skip the values upto that point in x-Axis time
                       if (xCount >= 296) {
                           if (v > vamYMax) vamYMax = v;
                       }
                     }
                     xCount++;
                }
                // overwrite ymax for VAM - if a proper value was found
                if (rideSeries == RideFile::vam && vamYMax > 0) ymax = vamYMax;
            }
        }
    }

    // X-AXIS

    // if max xvalue not set then default to 6 hours
    if (xmax == 0) xmax = 6 * 60;

    // truncate at an hour for energy mode
    if (criticalSeries == CriticalPowerWindow::work) xmax = 60.0;

    // not interested in short durations for vam
    if (criticalSeries == CriticalPowerWindow::vam) xmin = 4.993;

    // now set the scale
    QwtScaleDiv div((double)xmin, (double)xmax);
    if (criticalSeries == CriticalPowerWindow::work)
        div.setTicks(QwtScaleDiv::MajorTick, LogTimeScaleDraw::ticksEnergy);
    else
        div.setTicks(QwtScaleDiv::MajorTick, LogTimeScaleDraw::ticks);
    setAxisScaleDiv(QwtPlot::xBottom, div);

    // Y-AXIS

    if (!showDelta && rideSeries == RideFile::watts) {

        // set ymax to nearest 100 if power
        int max = ymax * 1.1f;
        max = ((max/100) + 1) * 100;

        setAxisScale(yLeft, 0, max);

    } else {

        // or just add 10% headroom
        setAxisScale(yLeft, ymin *1.1, 1.1*ymax);
    }

    // phew
    replot();
}

void
CPPlot::calculateForIntervals(QList<CompareInterval> compareIntervals)
{
    if (rangemode) return;

    // Zap what we got
    clearCurves();
    foreach(QwtPlotCurve *c, intervalCurves) {
        c->detach();
        delete c;
    }
    intervalCurves.clear();

    // If no intervals
    if (compareIntervals.count() == 0) {
        replot(); // show cleared plot
        return;
    }

    // set baseline if we're plotting deltas
    QVector<double> baseline;
    if (showDelta && compareIntervals.count()) {

        // set the baseline data
        CompareInterval range = compareIntervals.at(0);
        baseline = range.rideFileCache()->meanMaxArray(rideSeries);
    }

    double ymax = 0;
    double ymin = 0;
    double xmax = 0;
    double xmin = 1.0f/60.0f - 0.001f;

    // prepare aggregates
    for (int i = 0; i < compareIntervals.size(); ++i) {
        CompareInterval interval = compareIntervals.at(i);

        if (interval.isChecked())  {

            // no data ?
            if (interval.rideFileCache()->meanMaxArray(rideSeries).count() == 0) return;

            // create a delta array
            if (showDelta) {

                int n=0;
                QVector<double> deltaArray = interval.rideFileCache()->meanMaxArray(rideSeries);

                // make a delta to baseline
                for (n=1; n < deltaArray.size() && n < baseline.size(); n++) {
                    // stop when we get to zero!
                    if (deltaArray[n] > 0 && baseline[n] > 0)
                        if (showDeltaPercent) deltaArray[n] = 100.00f * (double(deltaArray[n]) - double(baseline[n])) / double(baseline[n]); // delta percentage
                        else deltaArray[n] = deltaArray[n] - baseline[n];
                    else
                        break;
                }
                deltaArray.resize(n-1);

                // now plot using the delta series and NOT the cache
                plotCache(deltaArray, interval.color);

                // set x-axis max
                if ((n/60.00f) > xmax) xmax = n/60.00f;

                foreach(double v, deltaArray) {
                    if (v > ymax) ymax = v;
                    if (v < ymin) ymin = v;
                }

            } else {

                // create curve data arrays
                plotCache(interval.rideFileCache()->meanMaxArray(rideSeries), interval.color);

                // whats ymax ?
                int xCount = 0;
                double vamYMax = 0;
                foreach(double v, interval.rideFileCache()->meanMaxArray(rideSeries)) {
                    // get the biggest y-Axes value
                    if (v > ymax) ymax = v;

                    // VAM x-Axis starts at 4.993 seconds = x-time array point 296 -
                    // so skip the values upto that point in x-Axis time
                    if (rideSeries == RideFile::vam) {
                       // VAM x-Axis starts at 4.993 seconds = x-time array point 296 -
                       // so skip the values upto that point in x-Axis time
                       if (xCount >= 296) {
                           if (v > vamYMax) vamYMax = v;
                       }
                     }
                     xCount++;
                }
                // overwrite ymax for VAM - if a proper value was found
                if (rideSeries == RideFile::vam && vamYMax > 0) ymax = vamYMax;

                double mins = interval.rideFileCache()->meanMaxArray(rideSeries).count() / 60.00f;
                if (mins > xmax) xmax = mins;
            }
        }
    }

    // X-AXIS

    // if max xvalue not set then default to 6 hours
    if (xmax == 0) xmax = 6 * 60;

    // truncate at an hour for energy mode
    if (criticalSeries == CriticalPowerWindow::work) xmax = 60.0;

    // not interested in short durations for vam
    if (criticalSeries == CriticalPowerWindow::vam) xmin = 4.993;

    // now set the scale
    QwtScaleDiv div((double)xmin, (double)xmax);
    if (criticalSeries == CriticalPowerWindow::work)
        div.setTicks(QwtScaleDiv::MajorTick, LogTimeScaleDraw::ticksEnergy);
    else
        div.setTicks(QwtScaleDiv::MajorTick, LogTimeScaleDraw::ticks);
    setAxisScaleDiv(QwtPlot::xBottom, div);

    // Y-AXIS

    if (!showDelta && rideSeries == RideFile::watts) {

        // set ymax to nearest 100 if power
        int max = ymax * 1.1f;
        max = ((max/100) + 1) * 100;

        setAxisScale(yLeft, ymin, max);

    } else {

        // or just add 10% headroom
        setAxisScale(yLeft, ymin *1.1, 1.1*ymax);
    }
    replot();
}

void
CPPlot::plotCache(QVector<double> vector, QColor intervalColor)
{
    // we don't shade if we're plotting in compare mode
    bool wantShadeIntervals = false;
    if ((rangemode && !context->isCompareDateRanges) || (!rangemode && !context->isCompareIntervals))
        wantShadeIntervals = shadeIntervals;

    int maxNonZero=0;
    QVector<double>x;
    QVector<double>y;
    for (int i=1; i<vector.count(); i++) {
        x << double(i)/60.00f;
        y << vector[i];
        if (vector[i] < 0 || vector[i] > 0) maxNonZero = i;
    }
    if (maxNonZero == 0) maxNonZero = y.size();

    // create a curve!
    QwtPlotCurve *curve = new QwtPlotCurve();
    if (appsettings->value(this, GC_ANTIALIAS, true).toBool() == true)
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    // set its color - based upon index in intervals!
    QPen pen(intervalColor);
    double width = appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble();
    pen.setWidth(width);
    //pen.setStyle(Qt::DotLine);
    intervalColor.setAlpha(64);
    QBrush brush = QBrush(intervalColor);
    if (wantShadeIntervals) curve->setBrush(brush);
    else curve->setBrush(Qt::NoBrush);
    curve->setPen(pen);
    curve->setSamples(x.data(), y.data(), maxNonZero-1);

    // attach and register
    curve->attach(this);

    intervalCurves.append(curve);
    zoomer->setZoomBase(false);
}
