/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
 *               2011 Mark Liversedge (liversedge@gmail.com)
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

#include "HistogramWindow.h"

// predefined deltas for each series
static const double wattsDelta = 1.0;
static const double wattsKgDelta = 0.01;
static const double nmDelta    = 0.1;
static const double hrDelta    = 1.0;
static const double kphDelta   = 0.1;
static const double cadDelta   = 1.0;
static const double gearDelta  = 0.01; //RideFileCache creates POW(10) * decimals sections

// digits for text entry validator
static const int wattsDigits = 0;
static const int wattsKgDigits = 2;
static const int nmDigits    = 1;
static const int hrDigits    = 0;
static const int kphDigits   = 1;
static const int cadDigits   = 0;
static const int gearDigits  = 2;


//
// Constructor
//
HistogramWindow::HistogramWindow(Context *context, bool rangemode) : GcChartWindow(context), context(context), stale(true), source(NULL), active(false), bactive(false), rangemode(rangemode), compareStale(false), useCustom(false), useToToday(false), precision(99)
{
    QWidget *c = new QWidget;
    c->setContentsMargins(0,0,0,0);
    QFormLayout *cl = new QFormLayout(c);
    cl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    cl->setSpacing(5);
    setControls(c);

    //
    // reveal controls widget
    //

    // reveal controls
    rWidth = new QLabel(tr("Bin Width"));
    rBinEdit = new QLineEdit();
    rBinEdit->setFixedWidth(40);
    rBinSlider = new QSlider(Qt::Horizontal);
    rBinSlider->setTickPosition(QSlider::TicksBelow);
    rBinSlider->setTickInterval(10);
    rBinSlider->setMinimum(1);
    rBinSlider->setMaximum(100);
    rShade = new QCheckBox(tr("Shade zones"));
    rZones = new QCheckBox(tr("Show in zones"));

    // layout reveal controls
    QHBoxLayout *r = new QHBoxLayout;
    r->setContentsMargins(0,0,0,0);
    r->addStretch();
    r->addWidget(rWidth);
    r->addWidget(rBinEdit);
    r->addWidget(rBinSlider);
    QVBoxLayout *v = new QVBoxLayout;
    v->addWidget(rShade);
    v->addWidget(rZones);
    r->addSpacing(20);
    r->addLayout(v);
    r->addStretch();
    setRevealLayout(r);

    // plot
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->setSpacing(10);
    powerHist = new PowerHist(context, rangemode);
    vlayout->addWidget(powerHist);

    setChartLayout(vlayout);

#ifdef GC_HAVE_LUCENE
    // search filter box
    isfiltered = false;
    searchBox = new SearchFilterBox(this, context);
    connect(searchBox, SIGNAL(searchClear()), this, SLOT(clearFilter()));
    connect(searchBox, SIGNAL(searchResults(QStringList)), this, SLOT(setFilter(QStringList)));
    if (!rangemode) searchBox->hide();
    else {
        cl->addRow(new QLabel(tr("Filter")), searchBox);
        cl->addWidget(new QLabel(""));
    }
#endif

    // date selection
    dateSetting = new DateSettingsEdit(this);

    if (rangemode) {

        cl->addRow(new QLabel(tr("Date Range")), dateSetting);
        cl->addWidget(new QLabel("")); // spacing

        // default to data series!
        data = new QRadioButton(tr("Ride Data Samples"));
        metric = new QRadioButton(tr("Ride Metrics"));
        data->setChecked(true);
        metric->setChecked(false);
        QHBoxLayout *radios = new QHBoxLayout;
        radios->addWidget(data);
        radios->addWidget(metric);
        cl->addRow(new QLabel(tr("Plot")), radios);

        connect(data, SIGNAL(toggled(bool)), this, SLOT(dataToggled(bool)));
        connect(metric, SIGNAL(toggled(bool)), this, SLOT(metricToggled(bool)));
    }

    // data series
    seriesCombo = new QComboBox();
    addSeries();
    if (rangemode) comboLabel = new QLabel("");
    else comboLabel = new QLabel(tr("Data Series"));
    cl->addRow(comboLabel, seriesCombo);

    if (rangemode) {

        // TOTAL METRIC
        totalMetricTree = new QTreeWidget;
#ifdef Q_OS_MAC
        totalMetricTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
        totalMetricTree->setColumnCount(2);
        totalMetricTree->setColumnHidden(1, true);
        totalMetricTree->setSelectionMode(QAbstractItemView::SingleSelection);
        totalMetricTree->header()->hide();
        //totalMetricTree->setFrameStyle(QFrame::NoFrame);
        //totalMetricTree->setAlternatingRowColors (true);
        totalMetricTree->setIndentation(5);
        totalMetricTree->setContextMenuPolicy(Qt::CustomContextMenu);

        // ALL METRIC
        distMetricTree = new QTreeWidget;
#ifdef Q_OS_MAC
        distMetricTree->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
        distMetricTree->setColumnCount(2);
        distMetricTree->setColumnHidden(1, true);
        distMetricTree->setSelectionMode(QAbstractItemView::SingleSelection);
        distMetricTree->header()->hide();
        //distMetricTree->setFrameStyle(QFrame::NoFrame);
        distMetricTree->setIndentation(5);
        distMetricTree->setContextMenuPolicy(Qt::CustomContextMenu);

        // add them all
        const RideMetricFactory &factory = RideMetricFactory::instance();
        for (int i = 0; i < factory.metricCount(); ++i) {

            const RideMetric *m = factory.rideMetric(factory.metricName(i));

            QTextEdit processHTML(m->name()); // process html encoding of(TM)
            QString processed = processHTML.toPlainText();

            QTreeWidgetItem *add;
            add = new QTreeWidgetItem(distMetricTree->invisibleRootItem());
            add->setText(0, processed);
            add->setText(1, m->symbol());

            // we only want totalising metrics
            if (m->type() != RideMetric::Total) continue;

            add = new QTreeWidgetItem(totalMetricTree->invisibleRootItem());
            add->setText(0, processed);
            add->setText(1, m->symbol());
        }

        QHBoxLayout *labels = new QHBoxLayout;

        metricLabel1 = new QLabel(tr("Total (x-axis)"));
        labels->addWidget(metricLabel1);
        metricLabel1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

        metricLabel2 = new QLabel(tr("Distribution (y-axis)"));
        metricLabel2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        labels->addWidget(metricLabel2);

        cl->addRow((blankLabel1=new QLabel("")), labels);
        QHBoxLayout *trees = new QHBoxLayout;
        trees->addWidget(totalMetricTree);
        trees->addWidget(distMetricTree);
        cl->addRow((blankLabel2 = new QLabel("")), trees);

        colorButton = new ColorButton(this, "Color", QColor(Qt::blue));
        colorLabel = new QLabel(tr("Color"));
        connect(colorButton, SIGNAL(colorChosen(QColor)), this, SLOT(updateChart()));
        cl->addRow(colorLabel, colorButton);

        // by default select number of rides by duration
        // which are the metrics workout_time and ride_count
        selectTotal("ride_count");
        selectMetric("workout_time");
    }

    showSumY = new QComboBox();
    showSumY->addItem(tr("Absolute Time"));
    showSumY->addItem(tr("Percentage Time"));

    showLabel = new QLabel(tr("Show"));
    cl->addRow(showLabel, showSumY);

    showLnY = new QCheckBox;
    showLnY->setText(tr("Log Y"));
    cl->addRow(blankLabel3 = new QLabel(""), showLnY);

    showZeroes = new QCheckBox;
    showZeroes->setText(tr("With zeros"));
    cl->addRow(blankLabel4 = new QLabel(""), showZeroes);

    shadeZones = new QCheckBox;
    shadeZones->setText(tr("Shade zones"));
    cl->addRow(blankLabel5 = new QLabel(""), shadeZones);

    showInZones = new QCheckBox;
    showInZones->setText(tr("Show in zones"));
    cl->addRow(blankLabel6 = new QLabel(""), showInZones);

    showInCPZones = new QCheckBox;
    showInCPZones->setText(tr("Use polarised zones"));
    cl->addRow(blankLabel7 = new QLabel(""), showInCPZones);

    // bin width
    QHBoxLayout *binWidthLayout = new QHBoxLayout;
    QLabel *binWidthLabel = new QLabel(tr("Bin width"), this);
    binWidthLineEdit = new QLineEdit(this);
    binWidthLineEdit->setFixedWidth(40);

    binWidthLayout->addWidget(binWidthLineEdit);
    binWidthSlider = new QSlider(Qt::Horizontal);
    binWidthSlider->setTickPosition(QSlider::TicksBelow);
    binWidthSlider->setTickInterval(1);
    binWidthSlider->setMinimum(1);
    binWidthSlider->setMaximum(100);
    binWidthLayout->addWidget(binWidthSlider);
    cl->addRow(binWidthLabel, binWidthLayout);

    // sort out default values
    setBinEditors();
    showLnY->setChecked(powerHist->islnY());
    showZeroes->setChecked(powerHist->withZeros());
    shadeZones->setChecked(powerHist->shade);
    binWidthSlider->setValue(powerHist->binWidth());
    rBinSlider->setValue(powerHist->binWidth());
    rShade->setChecked(powerHist->shade);

    // fixup series selected by default
    seriesChanged();

    // hide/show according to default mode
    switchMode(); // does nothing if not in rangemode

    // set the defaults etc
    updateChart();

    // the bin slider/input update each other
    // only the input box triggers an update to the chart
    connect(binWidthSlider, SIGNAL(valueChanged(int)), this, SLOT(setBinWidthFromSlider()));
    connect(binWidthLineEdit, SIGNAL(editingFinished()), this, SLOT(setBinWidthFromLineEdit()));
    connect(rBinSlider, SIGNAL(valueChanged(int)), this, SLOT(setrBinWidthFromSlider()));
    connect(rBinEdit, SIGNAL(editingFinished()), this, SLOT(setrBinWidthFromLineEdit()));
    connect(rZones, SIGNAL(stateChanged(int)), this, SLOT(setZoned(int)));
    connect(rShade, SIGNAL(stateChanged(int)), this, SLOT(setShade(int)));

    // when season changes we need to retrieve data from the cache then update the chart
    if (rangemode) {
        connect(this, SIGNAL(dateRangeChanged(DateRange)), this, SLOT(dateRangeChanged(DateRange)));
        connect(dateSetting, SIGNAL(useCustomRange(DateRange)), this, SLOT(useCustomRange(DateRange)));
        connect(dateSetting, SIGNAL(useThruToday()), this, SLOT(useThruToday()));
        connect(dateSetting, SIGNAL(useStandardRange()), this, SLOT(useStandardRange()));
        connect(distMetricTree, SIGNAL(itemSelectionChanged()), this, SLOT(treeSelectionChanged()));
        connect(totalMetricTree, SIGNAL(itemSelectionChanged()), this, SLOT(treeSelectionChanged()));

        lagger = new QTimer;
        lagger->setSingleShot(true);
        connect(lagger, SIGNAL(timeout()), this, SLOT(treeSelectionTimeout()));

        // comparing things
        connect(context, SIGNAL(compareDateRangesStateChanged(bool)), this, SLOT(compareChanged()));
        connect(context, SIGNAL(compareDateRangesChanged()), this, SLOT(compareChanged()));

    } else {
        dateSetting->hide();
        connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
        connect(context, SIGNAL(intervalSelected()), this, SLOT(intervalSelected()));
        connect(context, SIGNAL(intervalHover(RideFileInterval)), powerHist, SLOT(intervalHover(RideFileInterval)));

        // comparing things
        connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(compareChanged()));
        connect(context, SIGNAL(compareIntervalsChanged()), this, SLOT(compareChanged()));
    }

    // if any of the controls change we pass the chart everything
    connect(showLnY, SIGNAL(stateChanged(int)), this, SLOT(forceReplot()));
    connect(showZeroes, SIGNAL(stateChanged(int)), this, SLOT(forceReplot()));
    connect(seriesCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(seriesChanged()));
    connect(showInCPZones, SIGNAL(stateChanged(int)), this, SLOT(setCPZoned(int)));
    connect(showInCPZones, SIGNAL(stateChanged(int)), this, SLOT(forceReplot()));
    connect(showInZones, SIGNAL(stateChanged(int)), this, SLOT(setZoned(int)));
    connect(showInZones, SIGNAL(stateChanged(int)), this, SLOT(forceReplot()));
    connect(shadeZones, SIGNAL(stateChanged(int)), this, SLOT(setShade(int)));
    connect(shadeZones, SIGNAL(stateChanged(int)), this, SLOT(forceReplot()));
    connect(showSumY, SIGNAL(currentIndexChanged(int)), this, SLOT(forceReplot()));

    connect(context->athlete, SIGNAL(zonesChanged()), this, SLOT(zonesChanged()));
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));

    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(rideAddorRemove(RideItem*)));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(rideAddorRemove(RideItem*)));
    connect(context, SIGNAL(filterChanged()), this, SLOT(forceReplot()));
    connect(context, SIGNAL(homeFilterChanged()), this, SLOT(forceReplot()));

    // set colors etc
    configChanged();
}

void
HistogramWindow::configChanged()
{
    setProperty("color", GColor(CPLOTBACKGROUND)); // called on config change
    powerHist->configChanged();
}

bool
HistogramWindow::isCompare() const
{
    // compare intervals ?
    if (!rangemode && context->isCompareIntervals) return true;

    // compare date ranges ?
    if (rangemode && context->isCompareDateRanges) return true;

    return false;
}

void 
HistogramWindow::compareChanged()
{
    stale = true; // the 'standard' plots will need to be updated
    compareStale = true;

    if (!isVisible()) return;

    setUpdatesEnabled(false);

    // to stop getting into an infinite loop
    // when turning off coimpare mode and using
    // rideSelected to refresh
    compareStale = false;

    if (isCompare()) {

        // is blank?
        setIsBlank(false); // current ride irrelevant!

        // hide normal curves
        powerHist->hideStandard(true);

        // now set the controls
        RideFile::SeriesType series = static_cast<RideFile::SeriesType>
                                        (seriesCombo->itemData(seriesCombo->currentIndex()).toInt());
        powerHist->setSeries(series);

        // and now the controls
        powerHist->setShading(shadeZones->isChecked() ? true : false);
        powerHist->setZoned(showInZones->isChecked() ? true : false);
        powerHist->setCPZoned(showInCPZones->isChecked() ? true : false);
        powerHist->setlnY(showLnY->isChecked() ? true : false);
        powerHist->setWithZeros(showZeroes->isChecked() ? true : false);
        powerHist->setSumY(showSumY->currentIndex()== 0 ? true : false);

        // set data and create empty curves
        if (!rangemode || data->isChecked()) {

            // using the bests (ride file cache)
            powerHist->setDataFromCompare();

        } else {
            // using the metric arrays
            powerHist->setDelta(getDelta());
            powerHist->setDigits(getDigits());
            powerHist->setDataFromCompare(totalMetric(), distMetric());
        }
        powerHist->recalcCompare();

    } else {

            // show our normal curves and wipe rest
            powerHist->hideStandard(false);
            rideSelected(); // back to where we were
    }

    // replot!
    powerHist->replot();

    // repaint (in case optimised out)
    repaint();

    // and we're done
    setUpdatesEnabled(true);
}

//
// Colors (used by metric plotting)
//
QString HistogramWindow::plotColor() const
{
    if (!rangemode) return QColor(Qt::blue).name();
    else {
        return colorButton->getColor().name();
    }
}

void HistogramWindow::setPlotColor(QString color)
{
    if (rangemode) colorButton->setColor(QColor(color));
}

//
// Set Bin Width property by setting the slider
//
void HistogramWindow::setBin(double x)
{
    if (bactive) return;
    bactive = true;
    binWidthSlider->setValue(x/getDelta());
    rBinSlider->setValue(x/getDelta());
    binWidthLineEdit->setText(QString("%1").arg(x, 0, 'f', getDigits()));
    rBinEdit->setText(QString("%1").arg(x, 0, 'f', getDigits()));
    powerHist->setBinWidth(x);
    bactive = false;

    // redraw
    stale = true;
    updateChart();
}

//
// Get/Set the metric selections
//
QString HistogramWindow::distMetric() const
{
    if (!rangemode) return "workout_time";
    else {
        // get current selection
        QList<QTreeWidgetItem *> select = distMetricTree->selectedItems();
        if (select.count() == 0) return "workout_time";
        else return select[0]->text(1);
    }
}

void HistogramWindow::setDistMetric(QString value)
{
    if (!rangemode) return;
    selectMetric(value);
}

QString HistogramWindow::totalMetric() const
{
    if (!rangemode) return "ride_count";
    else {
        // get current selection
        QList<QTreeWidgetItem *> select = totalMetricTree->selectedItems();
        if (select.count() == 0) return "ride_count";
        else return select[0]->text(1);
    }
}

void HistogramWindow::setTotalMetric(QString value)
{
    if (!rangemode) return;
    selectTotal(value);
}

void
HistogramWindow::selectTotal(QString symbol)
{
    QList<QTreeWidgetItem *> found = totalMetricTree->findItems(symbol, Qt::MatchRecursive|Qt::MatchExactly, 1);
    if (found.count() == 0) return;

    // clear the current selection
    foreach(QTreeWidgetItem *selected, totalMetricTree->selectedItems()) selected->setSelected(false);

    // select the first one (there shouldn't be more than that!!!
    found[0]->setSelected(true);

    // make sure it is the current item and visible
    totalMetricTree->setCurrentItem(found[0]);
    totalMetricTree->scrollToItem(totalMetricTree->currentItem());
}

void
HistogramWindow::selectMetric(QString symbol)
{
    QList<QTreeWidgetItem *> found = distMetricTree->findItems(symbol, Qt::MatchRecursive|Qt::MatchExactly, 1);
    if (found.count() == 0) return;

    // clear the current selection
    foreach(QTreeWidgetItem *selected, distMetricTree->selectedItems()) selected->setSelected(false);

    // select the first one (there shouldn't be more than that!!!
    found[0]->setSelected(true);

    // make sure it is the current item and visible
    distMetricTree->setCurrentItem(found[0]);
    distMetricTree->scrollToItem(distMetricTree->currentItem());
}

//
// get set mode (data series or metric?) -- only in rangemode
//
bool HistogramWindow::dataMode() const
{
    if (!rangemode) return true;
    else return data->isChecked();
}

void HistogramWindow::setDataMode(bool value)
{
    if (!rangemode) return;
    else {
        active = true;
        data->setChecked(value);
        metric->setChecked(!value);
        active = false;
        switchMode();
    }
}

//
// When user changes from data series to metric
//
void
HistogramWindow::dataToggled(bool x)
{
    if (active) return;

    active = true;
    stale = true;
    metric->setChecked(!x);
    switchMode();
    active = false;
}

void
HistogramWindow::metricToggled(bool x)
{
    if (active) return;

    active = true;
    stale = true;
    data->setChecked(!x);
    switchMode();
    active = false;
}

void
HistogramWindow::switchMode()
{
    if (!rangemode) return; // ! only valid in rangemode

    if (data->isChecked()) {

        // hide all the metric controls
        blankLabel1->hide();
        blankLabel2->hide();
        distMetricTree->hide();
        totalMetricTree->hide();
        metricLabel1->hide();
        metricLabel2->hide();
        colorLabel->hide();
        colorButton->hide();
        
        // show all the data series controls
        comboLabel->show();
        seriesCombo->show();
        blankLabel3->show();
        blankLabel4->show();
        blankLabel5->show();
        blankLabel6->show();
        blankLabel7->show();
        showSumY->show();
        showLabel->show();
        showLnY->show();
        showZeroes->show();
        shadeZones->show();
        showInZones->show();
        showInCPZones->show();

        // select the series..
        seriesChanged();

    } else {

        // hide all the data series controls
        comboLabel->hide();
        seriesCombo->hide();
        blankLabel3->hide();
        blankLabel4->hide();
        blankLabel5->hide();
        blankLabel6->hide();
        blankLabel7->hide();
        showSumY->hide();
        showLabel->hide();
        showLnY->hide();
        showZeroes->hide();
        shadeZones->hide();
        showInZones->hide();
        showInCPZones->hide();

        // show all the metric controls
        blankLabel1->show();
        blankLabel2->show();
        metricLabel1->show();
        metricLabel2->show();
        distMetricTree->show();
        totalMetricTree->show();
        colorLabel->show();
        colorButton->show();

        // select the series.. (but without the half second delay)
        treeSelectionTimeout();
    }

    stale = true;
    updateChart(); // to whatever is currently selected.
}


//
// When user selects a new metric
//
void
HistogramWindow::treeSelectionChanged()
{
    stale = true;
    lagger->start(500);
}

void
HistogramWindow::treeSelectionTimeout()
{
    // new metric chosen, so set up all the bin width, line edit
    // delta, precision etc
    powerHist->setSeries(RideFile::none);
    powerHist->setDelta(getDelta());
    powerHist->setDigits(getDigits());

    // now update the slider stepper and linedit
    setBinEditors();

    // initial value -- but only if need to
    double minbinw = getDelta();
    double maxbinw = getDelta() * 100;
    double current = binWidthLineEdit->text().toDouble();
    if (current < minbinw || current > maxbinw) setBin(getDelta());

    // replot
    updateChart();
}

//
// When user selects a different data series
//
void
HistogramWindow::seriesChanged()
{
    // series changed so tell power hist
    powerHist->setSeries(static_cast<RideFile::SeriesType>(seriesCombo->itemData(seriesCombo->currentIndex()).toInt()));
    powerHist->setDelta(getDelta());
    powerHist->setDigits(getDigits());

    // now update the slider stepper and linedit
    setBinEditors();

    // set an initial bin width value
    setBin(getDelta());

    // replot
    stale = true;
    updateChart();
}

//
// We need to config / update the controls when data series/metrics change
//
void
HistogramWindow::setBinEditors()
{
    // the line edit validator
    QValidator *validator;
    if (getDigits() == 0) {

        validator = new QIntValidator(binWidthSlider->minimum() * getDelta(),
                                      binWidthSlider->maximum() * getDelta(),
                                      binWidthLineEdit);
    } else {

        validator = new QDoubleValidator(binWidthSlider->minimum() * getDelta(),
                                         binWidthSlider->maximum() * getDelta(),
                                         getDigits(),
                                         binWidthLineEdit);
    }
    binWidthLineEdit->setValidator(validator);
    rBinEdit->setValidator(validator);
}

//
// A new ride was selected
//
void
HistogramWindow::rideSelected()
{
    if (!amVisible()) return;

    RideItem *ride = myRideItem;
    // handle catch up to compare changed
    if (compareStale) compareChanged();

    if (!ride || isCompare() || (rangemode && !stale)) return;

    if (rangemode) {
        // get range that applies to this ride
        powerRange = context->athlete->zones()->whichRange(ride->dateTime.date());
        hrRange = context->athlete->hrZones()->whichRange(ride->dateTime.date());
    }

    // update
    updateChart();
}

//
// User selected a new interval
//
void
HistogramWindow::intervalSelected()
{
    if (!amVisible()) return;

    RideItem *ride = myRideItem;

    // null? or not plotting current ride, ignore signal
    if (!ride || isCompare() || rangemode) return;

    // update
    interval = true;
    updateChart();
}

void
HistogramWindow::rideAddorRemove(RideItem *)
{
    stale = true;
}

void
HistogramWindow::zonesChanged()
{
    if (!amVisible()) return;

    powerHist->refreshZoneLabels();
    powerHist->replot();
}

void
HistogramWindow::useCustomRange(DateRange range)
{
    // plot using the supplied range
    useCustom = true;
    useToToday = false;
    custom = range;
    dateRangeChanged(custom);
}

void
HistogramWindow::useStandardRange()
{
    useToToday= useCustom = false;
    dateRangeChanged(myDateRange);
}

void
HistogramWindow::useThruToday()
{
    // plot using the supplied range
    useCustom = false;
    useToToday = true;
    custom = myDateRange;
    if (custom.to > QDate::currentDate()) custom.to = QDate::currentDate();
    dateRangeChanged(custom);
}

void HistogramWindow::dateRangeChanged(DateRange dateRange)
{
    // compare mode?
    if (amVisible() && compareStale) compareChanged();

    // if we're using a custom one lets keep it
    if (rangemode && (useCustom || useToToday)) dateRange = custom;

    // has it changed?
    if (dateRange.from != cfrom || dateRange.to != cto) 
        stale = true;

    // don't do it if we're invisible, in compare or
    // nothing has changed since last time ..
    if (!amVisible() || isCompare() || !stale) return;

    updateChart();
}

void HistogramWindow::addSeries()
{
    // setup series list
    seriesList << RideFile::watts
               << RideFile::wattsKg
               << RideFile::hr
               << RideFile::kph
               << RideFile::cad
               << RideFile::nm
               << RideFile::aPower
               << RideFile::gear
               << RideFile::smo2;

    foreach (RideFile::SeriesType x, seriesList) 
        seriesCombo->addItem(RideFile::seriesName(x), static_cast<int>(x));
}

void
HistogramWindow::setBinWidthFromSlider()
{
    if (bactive) return;
    setBin(binWidthSlider->value() * getDelta());
}

void
HistogramWindow::setrBinWidthFromSlider()
{
    if (bactive) return;
    setBin(rBinSlider->value() * getDelta());
}

void
HistogramWindow::setBinWidthFromLineEdit()
{
    if (bactive) return;
    setBin(binWidthLineEdit->text().toDouble());
}

void
HistogramWindow::setrBinWidthFromLineEdit()
{
    if (bactive) return;
    setBin(rBinEdit->text().toDouble());
}

void
HistogramWindow::setCPZoned(int x)
{
    showInCPZones->setCheckState((Qt::CheckState)x);
    
}

void
HistogramWindow::setZoned(int x)
{
    rZones->setCheckState((Qt::CheckState)x);
    showInZones->setCheckState((Qt::CheckState)x);
    
}

void
HistogramWindow::setShade(int x)
{
    rShade->setCheckState((Qt::CheckState)x);
    shadeZones->setCheckState((Qt::CheckState)x);
}

void
HistogramWindow::forceReplot()
{
    stale = true;
    if (amVisible()) updateChart();
}

void
HistogramWindow::updateChart()
{
    // What is the selected series?
    RideFile::SeriesType series = static_cast<RideFile::SeriesType>(seriesCombo->itemData(seriesCombo->currentIndex()).toInt());

    // compare mode does its own thing so ignore
    // this request (but set stale) if we're comparing
    // we WILL get called when compare ends
    if (!amVisible()) {
        stale = true;
        return;
    }

    // just reflect chart setting changes
    if (isCompare()) {

        compareChanged();
        return;
    }

    // If no data present show the blank state page
    if (!rangemode) {
        RideFile::SeriesType baseSeries = (series == RideFile::wattsKg) ? RideFile::watts : series;
        if (rideItem() != NULL && rideItem()->ride()->isDataPresent(baseSeries))
            setIsBlank(false);
        else
            setIsBlank(true);
    } else {
        setIsBlank(false);
    }

    // Lets get the data then
    if (stale) {

        RideFileCache *old = source;

        if (rangemode) {

            // set the date range to the appropriate selection
            DateRange use;
            if (useCustom) {

                use = custom;

            } else if (useToToday) {

                use = myDateRange;
                QDate today = QDate::currentDate();
                if (use.to > today) use.to = today;

            } else {

                use = myDateRange;
            }

            if (data->isChecked()) {

                // plotting a data series, so refresh the ridefilecache

#ifdef GC_HAVE_LUCENE
                source = new RideFileCache(context, use.from, use.to, isfiltered, files, rangemode);
#else
                source = new RideFileCache(context, use.from, use.to);
#endif
                cfrom = use.from;
                cto = use.to;
                stale = false;

                if (old) delete old; // guarantee source pointer changes
                stale = false; // well we tried

                // and which series to plot
                powerHist->setSeries(series);

                // and now the controls
                powerHist->setShading(shadeZones->isChecked() ? true : false);
                powerHist->setZoned(showInZones->isChecked() ? true : false);
                powerHist->setCPZoned(showInCPZones->isChecked() ? true : false);
                powerHist->setlnY(showLnY->isChecked() ? true : false);
                powerHist->setWithZeros(showZeroes->isChecked() ? true : false);
                powerHist->setSumY(showSumY->currentIndex()== 0 ? true : false);

                // set the data on the plot
                powerHist->setData(source);

            } else {

                if (last.from != use.from || last.to != use.to) {

                    // remember the last lot we collected
                    last = use;

                    // plotting a metric, reread the metrics for the selected date range
                    results = context->athlete->metricDB->getAllMetricsFor(use);

                }

                if (results.count() == 0) setIsBlank(true);
                else setIsBlank(false);

                // setData using the summary metrics -- always reset since filters may
                // have changed, or perhaps the bin width...
                powerHist->setSeries(RideFile::none);
                powerHist->setDelta(getDelta());
                powerHist->setDigits(getDigits());
#ifdef GC_HAVE_LUCENE
                powerHist->setData(results, totalMetric(), distMetric(), isfiltered, files, &powerHist->standard);
#else
                powerHist->setData(results, totalMetric(), distMetric(), false, QStringList(), &powerHist->standard);
#endif
                powerHist->setColor(colorButton->getColor());

            }

        } else {

            // and which series to plot
            powerHist->setSeries(series);

            // and now the controls
            powerHist->setShading(shadeZones->isChecked() ? true : false);
            powerHist->setZoned(showInZones->isChecked() ? true : false);
            powerHist->setCPZoned(showInCPZones->isChecked() ? true : false);
            powerHist->setlnY(showLnY->isChecked() ? true : false);
            powerHist->setWithZeros(showZeroes->isChecked() ? true : false);
            powerHist->setSumY(showSumY->currentIndex()== 0 ? true : false);

            // do once the controls are set
            powerHist->setData(myRideItem, true); // intervals selected forces data to
                                                  // be recomputed since interval selection
                                                  // has changed.

        }

        powerHist->recalc(true); // interval changed? force recalc
        powerHist->replot();

        interval = false;// we force a recalc when called coz intervals
                        // have been selected. The recalc routine in
                        // powerhist optimises out, but doesn't keep track
                        // of interval selection -- simplifies the setters
                        // and getters, so worth this 'hack'.
    } // if stale
}

#ifdef GC_HAVE_LUCENE
void 
HistogramWindow::clearFilter()
{
    isfiltered = false;
    files.clear();
    stale = true;
    updateChart();
    repaint();
}

void
HistogramWindow::setFilter(QStringList list)
{
    isfiltered = true;
    files = list;
    stale = true;
    updateChart();
    repaint();
}
#endif

double
HistogramWindow::getDelta()
{
    if (rangemode && !data->isChecked()) {

        // based upon the metric chosen
        const RideMetricFactory &factory = RideMetricFactory::instance();
        const RideMetric *m = factory.rideMetric(distMetric());

        if (m) return 1.00F / pow(10, m->precision());
        else return 1;


    } else {

        // use the predefined delta
        switch (seriesCombo->currentIndex()) {
            case 0: return wattsDelta;
            case 1: return wattsKgDelta;
            case 2: return hrDelta;
            case 3: return kphDelta;
            case 4: return cadDelta;
            case 5: return nmDelta;
            case 6: return wattsDelta; //aPower
            case 7: return gearDelta;
            default: return 1;
        }
    }
}

int
HistogramWindow::getDigits()
{
    if (rangemode && !data->isChecked()) {

        // based upon the metric chosen
        const RideMetricFactory &factory = RideMetricFactory::instance();
        const RideMetric *m = factory.rideMetric(distMetric());

        if (m) return m->precision();
        else return 0;

    } else {

        // use predefined for data series
        switch (seriesCombo->currentIndex()) {
            case  0: return wattsDigits;
            case  1: return wattsKgDigits;
            case  2: return hrDigits;
            case  3: return kphDigits;
            case  4: return cadDigits;
            case  5: return nmDigits;
            case  6: return wattsDigits; // aPower
            case  7: return gearDigits;
            default: return 1;
        }
    }
}
