/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#include "CriticalPowerWindow.h"
#include "Settings.h"
#include "SearchFilterBox.h"
#include "MetricAggregator.h"
#include "CpintPlot.h"
#include "Context.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "TimeUtils.h"
#include "IntervalItem.h"
#include <qwt_picker.h>
#include <qwt_picker_machine.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>
#include <qwt_compat.h>
#include <QFile>
#include "Season.h"
#include "SeasonParser.h"
#include "Colors.h"
#include "Zones.h"
#include <QXmlInputSource>
#include <QXmlSimpleReader>

CriticalPowerWindow::CriticalPowerWindow(const QDir &home, Context *context, bool rangemode) :
    GcChartWindow(context), _dateRange("{00000000-0000-0000-0000-000000000001}"), home(home), context(context), currentRide(NULL), rangemode(rangemode), isfiltered(false), stale(true), useCustom(false), useToToday(false)
{
    setInstanceName("Critical Power Window");

    //
    // reveal controls widget
    //

    // layout reveal controls
    QHBoxLayout *revealLayout = new QHBoxLayout;
    revealLayout->setContentsMargins(0,0,0,0);
    revealLayout->addStretch();

    setRevealLayout(revealLayout);

    //
    // main plot area
    //
    QVBoxLayout *vlayout = new QVBoxLayout;
    cpintPlot = new CpintPlot(context, home.path(), context->athlete->zones(), rangemode);
    vlayout->addWidget(cpintPlot);

    QGridLayout *mainLayout = new QGridLayout();
    mainLayout->addLayout(vlayout, 0, 0);
    setChartLayout(mainLayout);


    //
    // picker - on chart controls/display
    //

    // picker widget
    QWidget *pickerControls = new QWidget(this);
    mainLayout->addWidget(pickerControls, 0, 0, Qt::AlignTop | Qt::AlignRight);
    pickerControls->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    // picker layout
    QVBoxLayout *pickerLayout = new QVBoxLayout(pickerControls);
    QFormLayout *pcl = new QFormLayout;
    pickerLayout->addLayout(pcl);
    pickerLayout->addStretch(); // get labels at top right

    // picker details
    QLabel *cpintTimeLabel = new QLabel(tr("Duration:"), this);
    cpintTimeValue = new QLabel("0 s");
    QLabel *cpintTodayLabel = new QLabel(tr("Today:"), this);
    cpintTodayValue = new QLabel(tr("no data"));
    QLabel *cpintAllLabel = new QLabel(tr("Best:"), this);
    cpintAllValue = new QLabel(tr("no data"));
    QLabel *cpintCPLabel = new QLabel(tr("CP Curve:"), this);
    cpintCPValue = new QLabel(tr("no data"));

    // chart overlayed values in smaller font
    QFont font = cpintTimeValue->font();
    font.setPointSize(font.pointSize()-2);
    cpintTodayValue->setFont(font);
    cpintAllValue->setFont(font);
    cpintCPValue->setFont(font);
    cpintTimeValue->setFont(font);
    cpintTimeLabel->setFont(font);
    cpintTodayLabel->setFont(font);
    cpintAllLabel->setFont(font);
    cpintCPLabel->setFont(font);

    pcl->addRow(cpintTimeLabel, cpintTimeValue);
    if (rangemode) {
        cpintTodayLabel->hide();
        cpintTodayValue->hide();
    } else {
        pcl->addRow(cpintTodayLabel, cpintTodayValue);
    }
    pcl->addRow(cpintAllLabel, cpintAllValue);
    pcl->addRow(cpintCPLabel, cpintCPValue);


    //
    // Chart settings
    //

    // controls widget and layout
    QWidget *c = new QWidget;
    QFormLayout *cl = new QFormLayout(c);
    cl->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    setControls(c);

#ifdef GC_HAVE_LUCENE
    // filter / searchbox
    searchBox = new SearchFilterBox(this, context);
    connect(searchBox, SIGNAL(searchClear()), cpintPlot, SLOT(clearFilter()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), cpintPlot, SLOT(setFilter(QStringList)));
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(filterChanged()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(filterChanged()));
    cl->addRow(new QLabel(tr("Filter")), searchBox);
    cl->addWidget(new QLabel("")); //spacing
#endif

    // series
    seriesCombo = new QComboBox(this);
    addSeries();

    // data -- season / daterange edit
    cComboSeason = new QComboBox(this);
    seasons = context->athlete->seasons;
    resetSeasons();
    QLabel *label = new QLabel(tr("Date range"));
    QLabel *label2 = new QLabel(tr("Date range"));
    if (rangemode) {
        cComboSeason->hide();
        label2->hide();
    }
    cl->addRow(label2, cComboSeason);
    dateSetting = new DateSettingsEdit(this);
    cl->addRow(label, dateSetting);
    if (rangemode == false) {
        dateSetting->hide();
        label->hide();
    }

    cl->addWidget(new QLabel("")); //spacing
    cl->addRow(new QLabel(tr("Data series")), seriesCombo);

    // shading
    shadeCombo = new QComboBox(this);
    shadeCombo->addItem(tr("None"));
    shadeCombo->addItem(tr("Using CP"));
    shadeCombo->addItem(tr("Using derived CP"));
    QLabel *shading = new QLabel(tr("Power Shading"));
    shadeCombo->setCurrentIndex(2);
    cl->addRow(shading, shadeCombo);

    // model config
    // 2 or 3 point model ?
    modelCombo= new QComboBox(this);
    modelCombo->addItem("2 parameter");
    modelCombo->addItem("3 parameter");
    modelCombo->setCurrentIndex(0);

    cl->addWidget(new QLabel("")); //spacing
    cl->addWidget(new QLabel("")); //spacing
    cl->addRow(new QLabel(tr("CP Model")), modelCombo);

    anI1SpinBox = new QDoubleSpinBox(this);
    anI1SpinBox->setDecimals(0);
    anI1SpinBox->setMinimum(0);
    anI1SpinBox->setMaximum(3600);
    anI1SpinBox->setSingleStep(1.0);
    anI1SpinBox->setAlignment(Qt::AlignRight);
    anI1SpinBox->setValue(180); // 3 minutes

    anI2SpinBox = new QDoubleSpinBox(this);
    anI2SpinBox->setDecimals(0);
    anI2SpinBox->setMinimum(0);
    anI2SpinBox->setMaximum(3600);
    anI2SpinBox->setSingleStep(1.0);
    anI2SpinBox->setAlignment(Qt::AlignRight);
    anI2SpinBox->setValue(360); // 6 minutes

    QHBoxLayout *anLayout = new QHBoxLayout(this);
    anLayout->addWidget(anI1SpinBox);
    anLayout->addWidget(anI2SpinBox);
    cl->addRow(new QLabel(tr("Interval 1 (seconds)")), anLayout);

    aeI1SpinBox = new QDoubleSpinBox(this);
    aeI1SpinBox->setDecimals(0);
    aeI1SpinBox->setMinimum(0.0);
    aeI1SpinBox->setMaximum(3600);
    aeI1SpinBox->setSingleStep(1.0);
    aeI1SpinBox->setAlignment(Qt::AlignRight);
    aeI1SpinBox->setValue(1800); // 30 minutes

    aeI2SpinBox = new QDoubleSpinBox(this);
    aeI2SpinBox->setDecimals(0);
    aeI2SpinBox->setMinimum(0.0);
    aeI2SpinBox->setMaximum(3600);
    aeI2SpinBox->setSingleStep(1.0);
    aeI2SpinBox->setAlignment(Qt::AlignRight);
    aeI2SpinBox->setValue(3600); // 30 minutes

    QHBoxLayout *aeLayout = new QHBoxLayout(this);
    aeLayout->addWidget(aeI1SpinBox);
    aeLayout->addWidget(aeI2SpinBox);
    cl->addRow(new QLabel(tr("Interval 2 (seconds)")), aeLayout);

    // point 2 + 3 -or- point 1 + 2 in a 2 point model

    picker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOff, cpintPlot->canvas());
    picker->setStateMachine(new QwtPickerDragPointMachine);
    picker->setRubberBandPen(GColor(CPLOTTRACKER));

    connect(picker, SIGNAL(moved(const QPoint &)), SLOT(pickerMoved(const QPoint &)));

    if (rangemode) {
        connect(this, SIGNAL(dateRangeChanged(DateRange)), SLOT(dateRangeChanged(DateRange)));
    } else {
        // when working on a ride we can selecct intervals!
        connect(cComboSeason, SIGNAL(currentIndexChanged(int)), this, SLOT(seasonSelected(int)));
        connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
        connect(context, SIGNAL(intervalsChanged()), this, SLOT(intervalsChanged()));
    }

    connect(seriesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setSeries(int)));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(configChanged()), cpintPlot, SLOT(configChanged()));

    // model updated?
    connect(modelCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(modelParametersChanged()));
    connect(anI1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(anI2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(aeI1SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));
    connect(aeI2SpinBox, SIGNAL(valueChanged(double)), this, SLOT(modelParametersChanged()));

    // redraw on config change -- this seems the simplest approach
    connect(context, SIGNAL(filterChanged()), this, SLOT(forceReplot()));
    connect(context, SIGNAL(configChanged()), this, SLOT(rideSelected()));
    connect(context->athlete->metricDB, SIGNAL(dataChanged()), this, SLOT(refreshRideSaved()));
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(newRideAdded(RideItem*)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(newRideAdded(RideItem*)));
    connect(seasons, SIGNAL(seasonsChanged()), this, SLOT(resetSeasons()));
    connect(shadeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(shadingSelected(int)));
    connect(dateSetting, SIGNAL(useCustomRange(DateRange)), this, SLOT(useCustomRange(DateRange)));
    connect(dateSetting, SIGNAL(useThruToday()), this, SLOT(useThruToday()));
    connect(dateSetting, SIGNAL(useStandardRange()), this, SLOT(useStandardRange()));

    modelParametersChanged();
}

void
CriticalPowerWindow::modelParametersChanged()
{
    // tell the plot
    cpintPlot->setModel(anI1SpinBox->value(),
                        anI2SpinBox->value(),
                        aeI1SpinBox->value(),
                        aeI2SpinBox->value(),
                        modelCombo->currentIndex() > 0 ? true : false); // true=use 3point model

    // and apply
    if (amVisible() && myRideItem != NULL) {
        cpintPlot->calculate(myRideItem);
    }
}

void
CriticalPowerWindow::refreshRideSaved()
{
    const RideItem *current = context->rideItem();
    if (!current) return;

    // if the saved ride is in the aggregated time period
    QDate date = current->dateTime.date();
    if (date >= cpintPlot->startDate &&
        date <= cpintPlot->endDate) {

        // force a redraw next time visible
        cpintPlot->changeSeason(cpintPlot->startDate, cpintPlot->endDate);
    }
}

void
CriticalPowerWindow::forceReplot()
{
    stale = true; // we must become stale
    if (rangemode) {

        // force replot...
        dateRangeChanged(myDateRange); 

    } else {
        Season season = seasons->seasons.at(cComboSeason->currentIndex());

        // Refresh aggregated curve (ride added/filter changed)
        cpintPlot->changeSeason(season.getStart(), season.getEnd());

        // if visible make the changes visible
        // rideSelected is easiest way
        if (amVisible()) rideSelected();
    }
    repaint();
}

void
CriticalPowerWindow::newRideAdded(RideItem *here)
{
    // mine just got Zapped, a new rideitem would not be my current item
    if (here == currentRide) currentRide = NULL;

    // any plots we already have are now stale
    if (!rangemode) {

        Season season = seasons->seasons.at(cComboSeason->currentIndex());
        stale = true;

        if ((here->dateTime.date() >= season.getStart() || season.getStart() == QDate())
                && (here->dateTime.date() <= season.getEnd() || season.getEnd() == QDate())) {
            // replot
            forceReplot();
        }

    } else {

        forceReplot();

    }
}

void
CriticalPowerWindow::intervalSelected()
{
    if (rangemode) return; // do nothing for ranges!

    // nothing to plot
    if (!amVisible() || myRideItem == NULL) return;
    if (context->athlete->allIntervalItems() == NULL) return; // not inited yet!

    // if the array hasn't been initialised properly then clean it up
    // this is because intervalsChanged gets called when selecting rides
    if (intervalCurves.count() != context->athlete->allIntervalItems()->childCount()) {
        // wipe away what we got, even if not visible
        // clear any interval curves -- even if we are not visible
        foreach(QwtPlotCurve *p, intervalCurves) {
            if (p) {
                p->detach();
                delete p;
            }
        }

        // clear, resize to interval count and set to null
        intervalCurves.clear();
        for (int i=0; i< context->athlete->allIntervalItems()->childCount(); i++) intervalCurves << NULL;
    }

    // which itervals are selected?
    IntervalItem *current=NULL;
    for (int i=0; i<context->athlete->allIntervalItems()->childCount(); i++) {
        current = dynamic_cast<IntervalItem *>(context->athlete->allIntervalItems()->child(i));
        if (current != NULL) {
            if (current->isSelected() == true) {
                showIntervalCurve(current, i); // set it all up
            } else {
                hideIntervalCurve(i); // in case its shown at present
            }
        }
    }
    cpintPlot->replot();
}

void
CriticalPowerWindow::hideIntervalCurve(int index)
{
    if (rangemode) return; // do nothing for ranges!

    if (intervalCurves[index] != NULL) {
        intervalCurves[index]->detach(); // in case
    }
}

void
CriticalPowerWindow::showIntervalCurve(IntervalItem *current, int index)
{
    if (rangemode) return; // do nothing for ranges!

    // we already made it?
    if (intervalCurves[index] != NULL) {
        intervalCurves[index]->detach(); // in case
        intervalCurves[index]->attach(cpintPlot);

        return;
    }

    // ack, we need to create the curve for this interval

    // make a ridefile
    RideFile f;
    f.context = context; // hack, until we refactor athlete and mainwindow
    f.setRecIntSecs(myRideItem->ride()->recIntSecs());
   
    foreach(RideFilePoint *p, myRideItem->ride()->dataPoints()) { 
       if (p->secs+f.recIntSecs() > current->start && p->secs< current->stop) {
           f.appendPoint(p->secs, p->cad, p->hr, p->km, p->kph, p->nm,
                       p->watts, p->alt, p->lon, p->lat, p->headwind,
                       p->slope, p->temp, p->lrbalance, 0);
       }

        // for xpower et al
       f.recalculateDerivedSeries();
    }

    // compute the mean max, this is BLAZINGLY fast, thanks to Mark Rages'
    // mean-max computer. Does a 11hr ride in 150ms
    QVector<float>vector;
    MeanMaxComputer thread1(&f, vector, series()); thread1.run();
    thread1.wait();

    // no data!
    if (vector.count() == 0) return;

    // create curve data arrays
    int i=0;
    QVector<double>x;
    QVector<double>y;
    x.resize(vector.size());
    y.resize(vector.size());
    foreach(float yv, vector) { x << i / 60.00; y << yv; i++; }

    // create a curve!
    QwtPlotCurve *curve = new QwtPlotCurve();
    if (appsettings->value(this, GC_ANTIALIAS, false).toBool() == true)
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);

    // set its color - based upon index in intervals!
    QColor intervalColor;
    int count=context->athlete->allIntervalItems()->childCount();
    intervalColor.setHsv(index * (255/count), 255,255);
    QPen pen(intervalColor);
    pen.setWidth(2.0);
    pen.setStyle(Qt::DotLine);
    intervalColor.setAlpha(64);
    QBrush brush = QBrush(intervalColor);
    curve->setBrush(brush);
    curve->setPen(pen);
    curve->setData(x.data(), y.data(), x.count()-1);

    // attach and register
    curve->attach(cpintPlot);
    intervalCurves[index] = curve;
}

void
CriticalPowerWindow::intervalsChanged()
{
    if (rangemode) return; // do nothing for ranges!

    // wipe away what we got, even if not visible
    // clear any interval curves -- even if we are not visible
    foreach(QwtPlotCurve *p, intervalCurves) {
        if (p) {
            p->detach();
            delete p;
        }
    }

    // clear, resize to interval count and set to null
    intervalCurves.clear();
    for (int i=0; i<= context->athlete->allIntervalItems()->childCount(); i++) intervalCurves << NULL;

    if (!amVisible()) return;

    // replot if needed
    intervalSelected(); // will refresh
}

void
CriticalPowerWindow::rideSelected()
{
    if (!rangemode) { // we only highlight intervals in normal mode

        // clear any interval curves -- even if we are not visible
        foreach(QwtPlotCurve *p, intervalCurves) {
            if (p) {
                p->detach();
                delete p;
            }
        }

        // clear, resize to interval count and set to null
        intervalCurves.clear();
        for (int i=0; i<= context->athlete->allIntervalItems()->childCount(); i++) intervalCurves << NULL;
    }

    if (!amVisible()) return;
    currentRide = myRideItem;

    if (currentRide) {
        if (context->athlete->zones()) {
            int zoneRange = context->athlete->zones()->whichRange(currentRide->dateTime.date());
            int CP = zoneRange >= 0 ? context->athlete->zones()->getCP(zoneRange) : 0;
            cpintPlot->setDateCP(CP);
        } else {
            cpintPlot->setDateCP(0);
        }
        cpintPlot->calculate(currentRide);

        // apply latest colors
        picker->setRubberBandPen(GColor(CPLOTTRACKER));
        setIsBlank(false);
    } else if (!rangemode) {
        setIsBlank(true);
    }

    // refresh intervals
    intervalSelected();
}

void
CriticalPowerWindow::setSeries(int index)
{
    if (index >= 0) {

        if (rangemode) {

            cpintPlot->setSeries(static_cast<RideFile::SeriesType>(seriesCombo->itemData(index).toInt()));
            cpintPlot->calculate(currentRide);

        } else {

            // clear any interval curves -- even if we are not visible
            foreach(QwtPlotCurve *p, intervalCurves) {
                if (p) {
                    p->detach();
                    delete p;
                }
            }

            // clear, resize to interval count and set to null
            intervalCurves.clear();
            for (int i=0; i<= context->athlete->allIntervalItems()->childCount(); i++) intervalCurves << NULL;

            cpintPlot->setSeries(static_cast<RideFile::SeriesType>(seriesCombo->itemData(index).toInt()));
            cpintPlot->calculate(currentRide);

            // refresh intervals
            intervalSelected();
        }
    }
}

static double
curve_to_point(double x, const QwtPlotCurve *curve, RideFile::SeriesType serie)
{
    double result = 0;
    if (curve) {
        const QwtSeriesData<QPointF> *data = curve->data();

        if (data->size() > 0) {
            if (x < data->sample(0).x() || x > data->sample(data->size() - 1).x())
                return 0;
            unsigned min = 0, mid = 0, max = data->size();
            while (min < max - 1) {
                mid = (max - min) / 2 + min;
                if (x < data->sample(mid).x()) {
                    double a = pow(10,RideFileCache::decimalsFor(serie));

                    result = ((int)((0.5/a + data->sample(mid).y()) * a))/a;
                    //result = (unsigned) round(data->sample(mid).y());
                    max = mid;
                }
                else {
                    min = mid;
                }
            }
        }
    }
    return result;
}

void
CriticalPowerWindow::updateCpint(double minutes)
{
    QString units;

    switch (series()) {

        case RideFile::none:
            units = "kJ";
            break;

        case RideFile::cad:
            units = "rpm";
            break;

        case RideFile::kph:
            units = "kph";
            break;

        case RideFile::hr:
            units = "bpm";
            break;

        case RideFile::nm:
            units = "nm";
            break;

        case RideFile::vam:
            units = "metres/hour";
            break;

        case RideFile::wattsKg:
            units = "Watts/kg";
            break;

        default:
        case RideFile::aPower:
        case RideFile::watts:
            units = "Watts";
            break;

    }

    // current ride
    {
      double value = curve_to_point(minutes, cpintPlot->getThisCurve(), series());
      QString label;
      if (value > 0)
          label = QString("%1 %2").arg(value).arg(units);
      else
          label = tr("no data");
      cpintTodayValue->setText(label);
    }

    // cp line
    if (cpintPlot->getCPCurve()) {
      double value = curve_to_point(minutes, cpintPlot->getCPCurve(), series());
      QString label;
      if (value > 0)
        label = QString("%1 %2").arg(value).arg(units);
      else
        label = tr("no data");
      cpintCPValue->setText(label);
    }

    // global ride
    {
      QString label;
      int index = (int) ceil(minutes * 60);
      if (index >= 0 && cpintPlot->getBests().count() > index) {
          QDate date = cpintPlot->getBestDates()[index];
          double value = cpintPlot->getBests()[index];

          double a = pow(10,RideFileCache::decimalsFor(series()));
          value = ((int)((0.5/a + value) * a))/a;

              label = QString("%1 %2 (%3)").arg(value).arg(units)
                      .arg(date.isValid() ? date.toString(tr("MM/dd/yyyy")) : tr("no date"));
      }
      else {
        label = tr("no data");
      }
      cpintAllValue->setText(label);
    }
}

void
CriticalPowerWindow::cpintTimeValueEntered()
{
  double minutes = str_to_interval(cpintTimeValue->text()) / 60.0;
  updateCpint(minutes);
}

void
CriticalPowerWindow::pickerMoved(const QPoint &pos)
{
    double minutes = cpintPlot->invTransform(QwtPlot::xBottom, pos.x());
    cpintTimeValue->setText(interval_to_str(60.0*minutes));
    updateCpint(minutes);
}
void CriticalPowerWindow::addSeries()
{
    // setup series list
    seriesList << RideFile::watts
               << RideFile::wattsKg
               << RideFile::xPower
               << RideFile::NP
               << RideFile::hr
               << RideFile::kph
               << RideFile::cad
               << RideFile::nm
               << RideFile::vam
               << RideFile::aPower
               << RideFile::none; // this shows energy (hack)

    foreach (RideFile::SeriesType x, seriesList) {
        if (x==RideFile::none) {
            seriesCombo->addItem(tr("Energy"), static_cast<int>(x));
        }
        else {
            seriesCombo->addItem(RideFile::seriesName(x), static_cast<int>(x));
        }
    }
}

/*----------------------------------------------------------------------
 * Seasons stuff
 *--------------------------------------------------------------------*/


void
CriticalPowerWindow::resetSeasons()
{
    if (rangemode) return;

    QString prev = cComboSeason->itemText(cComboSeason->currentIndex());

    // remove seasons
    cComboSeason->clear();

    //Store current selection
    QString previousDateRange = _dateRange;
    // insert seasons
    for (int i=0; i <seasons->seasons.count(); i++) {
        Season season = seasons->seasons.at(i);
        cComboSeason->addItem(season.getName());
    }
    // restore previous selection
    int index = cComboSeason->findText(prev);
    if (index != -1)  {
        cComboSeason->setCurrentIndex(index);
    }
}

void
CriticalPowerWindow::useCustomRange(DateRange range)
{
    // plot using the supplied range
    useCustom = true;
    useToToday = false;
    custom = range;
    dateRangeChanged(custom);
}

void
CriticalPowerWindow::useStandardRange()
{
    useToToday = useCustom = false;
    dateRangeChanged(myDateRange);
}

void
CriticalPowerWindow::useThruToday()
{
    // plot using the supplied range
    useCustom = false;
    useToToday = true;
    custom = myDateRange;
    if (custom.to > QDate::currentDate()) custom.to = QDate::currentDate();
    dateRangeChanged(custom);
}

void
CriticalPowerWindow::dateRangeChanged(DateRange dateRange)
{
    if (!amVisible()) return;

    // it will either be sidebar or custom...
    if (useCustom) dateRange = custom;
    else if (useToToday) {

        dateRange = myDateRange;
        QDate today = QDate::currentDate();
        if (dateRange.to > today) dateRange.to = today;

    } else dateRange = myDateRange;
    
    if (dateRange.from == cfrom && dateRange.to == cto && !stale) return;

    cfrom = dateRange.from;
    cto = dateRange.to;

    // lets work out the average CP configure value
    if (context->athlete->zones()) {
        int fromZoneRange = context->athlete->zones()->whichRange(cfrom);
        int toZoneRange = context->athlete->zones()->whichRange(cto);

        int CPfrom = fromZoneRange >= 0 ? context->athlete->zones()->getCP(fromZoneRange) : 0;
        int CPto = toZoneRange >= 0 ? context->athlete->zones()->getCP(toZoneRange) : CPfrom;
        if (CPfrom == 0) CPfrom = CPto;
        int dateCP = (CPfrom + CPto) / 2;

        cpintPlot->setDateCP(dateCP);
    }

    cpintPlot->changeSeason(dateRange.from, dateRange.to);
    cpintPlot->calculate(currentRide);

    stale = false;
}

void CriticalPowerWindow::seasonSelected(int iSeason)
{
    if (iSeason >= seasons->seasons.count() || iSeason < 0) return;
    Season season = seasons->seasons.at(iSeason);
    _dateRange = season.id();
    cpintPlot->changeSeason(season.getStart(), season.getEnd());
    cpintPlot->calculate(currentRide);
}

void CriticalPowerWindow::filterChanged()
{
    cpintPlot->calculate(currentRide);
}

void
CriticalPowerWindow::shadingSelected(int shading)
{
    cpintPlot->setShadeMode(shading);
    if (rangemode) dateRangeChanged(DateRange());
    else cpintPlot->calculate(currentRide);
}
