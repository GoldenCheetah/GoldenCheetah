
#include "PerformanceManagerWindow.h"
#include "MainWindow.h"
#include "PerfPlot.h"
#include "Colors.h"
#include "StressCalculator.h"
#include "RideItem.h"



PerformanceManagerWindow::PerformanceManagerWindow(MainWindow *mainWindow) :
    QWidget(mainWindow), mainWindow(mainWindow), active(false)
{
    days = count = 0;
    sc = NULL;

    settings = GetApplicationSettings();

    QVBoxLayout *vlayout = new QVBoxLayout;

    QHBoxLayout *PMPickerLayout = new QHBoxLayout;

    QLabel *PMSTSLabel = new QLabel(settings->value(GC_STS_ACRONYM,"STS").toString() + ":", this);
    PMSTSValue = new QLineEdit("0");
    PMSTSValue->setReadOnly(true);
    PMSTSValue->setValidator(new QDoubleValidator(0,500,1,PMSTSValue));
    PMPickerLayout->addWidget(PMSTSLabel);
    PMPickerLayout->addWidget(PMSTSValue);

    QLabel *PMLTSLabel = new QLabel(settings->value(GC_LTS_ACRONYM,"LTS").toString() + ":", this);

    PMLTSValue = new QLineEdit("0");
    PMLTSValue->setReadOnly(true);
    PMLTSValue->setValidator(new QDoubleValidator(0,500,1,PMLTSValue));
    PMPickerLayout->addWidget(PMLTSLabel);
    PMPickerLayout->addWidget(PMLTSValue);

    QLabel *PMSBLabel = new QLabel(settings->value(GC_SB_ACRONYM,"SB").toString() + ":", this);

    PMSBValue = new QLineEdit("0");
    PMSBValue->setReadOnly(true);
    PMSBValue->setValidator(new QDoubleValidator(-500,500,1,PMSBValue));
    PMPickerLayout->addWidget(PMSBLabel);
    PMPickerLayout->addWidget(PMSBValue);

    QLabel *PMDayLabel = new QLabel(tr("Day:"), this);
    PMDayValue = new QLineEdit(tr("no data"));
    PMPickerLayout->addWidget(PMDayLabel);
    PMPickerLayout->addWidget(PMDayValue);

    metricCombo = new QComboBox(this);
    metricCombo->addItem(tr("Use BikeScore"), "skiba_bike_score");
    metricCombo->addItem(tr("Use DanielsPoints"), "daniels_points");
    metricCombo->addItem(tr("Use TRIMP"), "trimp_points");
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    QString metricName =
        settings->value(GC_PERF_MAN_METRIC, "skiba_bike_score").toString();
    for (int i = 0; i < metricCombo->count(); ++i) {
        if (metricCombo->itemData(i).toString() == metricName)
            metricCombo->setCurrentIndex(i);
    }
    metric = metricCombo->itemData(metricCombo->currentIndex()).toString();
    PMPickerLayout->addWidget(metricCombo);

    // date range
    QHBoxLayout *dateRangeLayout = new QHBoxLayout;
    PMdateRangefrom = new QLineEdit("0");
    dateRangeLayout->addWidget(PMdateRangefrom);

    PMleftSlider = new QSlider(Qt::Horizontal);
    PMleftSlider->setTickPosition(QSlider::TicksBelow);
    PMleftSlider->setTickInterval(1);
    PMleftSlider->setMinimum(0);
    dateRangeLayout->addWidget(PMleftSlider);

    QLabel *dateRangeLabel = new QLabel(tr("to"),this);
    dateRangeLayout->addWidget(dateRangeLabel);

    PMrightSlider = new QSlider(Qt::Horizontal);
    PMrightSlider->setTickPosition(QSlider::TicksBelow);
    PMrightSlider->setTickInterval(1);
    PMrightSlider->setMinimum(0);
    dateRangeLayout->addWidget(PMrightSlider);

    PMdateRangeto = new QLineEdit("0");
    dateRangeLayout->addWidget(PMdateRangeto);


    perfplot = new PerfPlot();
    vlayout->addWidget(perfplot);
    vlayout->addLayout(dateRangeLayout);
    vlayout->addLayout(PMPickerLayout);
    setLayout(vlayout);

    PMpicker = new QwtPlotPicker(QwtPlot::xBottom, QwtPlot::yLeft,
                               QwtPicker::PointSelection,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOff, perfplot->canvas());
    PMpicker->setRubberBandPen(GColor(CPLOTSELECT));

    connect(PMpicker, SIGNAL(moved(const QPoint &)),
           SLOT(PMpickerMoved(const QPoint &)));
    connect(metricCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(metricChanged()));
    connect(mainWindow, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(mainWindow, SIGNAL(configChanged()), perfplot, SLOT(configUpdate()));
    connect(mainWindow, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
}

PerformanceManagerWindow::~PerformanceManagerWindow()
{
    if (sc)
	delete sc;
}

void PerformanceManagerWindow::configChanged()
{
    if (active) {
        days = 0; // force replot
        replot();
    }
    PMpicker->setRubberBandPen(GColor(CPLOTSELECT));
}

void PerformanceManagerWindow::metricChanged()
{
    QString newMetric = metricCombo->itemData(metricCombo->currentIndex()).toString();
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();
    settings->setValue(GC_PERF_MAN_METRIC, newMetric);
    replot();
}

void PerformanceManagerWindow::rideSelected()
{
    bool wasActive = active;
    active = (mainWindow->activeTab() == this);
    if (!wasActive && active)
        replot();
}

void PerformanceManagerWindow::replot()
{
    const QDir &home = mainWindow->home;
    const QTreeWidgetItem *allRides = mainWindow->allRideItems();

    int newdays, endIndex;
    RideItem *firstRideItem;
    RideItem *lastRideItem;
    QDateTime now;


    // calculate the number of days to look at... for now
    // use first ride in allRides to today.  When Season stuff is hooked
    // up, maybe use that, or will allRides reflect only current season?
    
    QVariant isAscending = settings->value(GC_ALLRIDES_ASCENDING,Qt::Checked);
    if(isAscending.toInt() > 0 ){
        firstRideItem =  (RideItem*) allRides->child(0);
        lastRideItem =  (RideItem*) allRides->child(allRides->childCount()-1);
    } else {
        firstRideItem =  (RideItem*) allRides->child(allRides->childCount()-1);
        lastRideItem =  (RideItem*) allRides->child(0);
    }
    
    if (firstRideItem) {

        QDateTime endTime = std::max(lastRideItem->dateTime, now.currentDateTime());
        newdays = firstRideItem->dateTime.daysTo(endTime);
        QString newMetric = metricCombo->itemData(metricCombo->currentIndex()).toString();

        if (newdays != days || allRides->childCount() != count || newMetric != metric) {

	    // days in allRides changed, so recalculate stress
	    //
	    bool firstrun = false;

	    /*
	    fprintf(stderr,
		    "PerformanceManagerWindow::replot: %d days from %s to %s\n",
		    newdays,firstRideItem->dateTime.toString().toAscii().data(),
		    now.currentDateTime().toString().toAscii().data());
		*/

	    if (days == 0)
		firstrun = true;

	    if (sc) delete sc;

	    sc = new StressCalculator(
		    firstRideItem->dateTime,
		    endTime,
		    (settings->value(GC_INITIAL_STS)).toInt(),
		    (settings->value(GC_INITIAL_LTS)).toInt(),
		    (settings->value(GC_STS_DAYS,7)).toInt(),
		    (settings->value(GC_LTS_DAYS,42)).toInt());

            sc->calculateStress(mainWindow,home.absolutePath(),newMetric);

	    perfplot->setStressCalculator(sc);

	    endIndex  = sc->n();


	    PMleftSlider->setMaximum(endIndex);

	    PMrightSlider->setMaximum(endIndex);

	    PMdateRangefrom->setValidator(new QIntValidator(PMleftSlider->minimum(),
			PMleftSlider->maximum(),PMleftSlider));
	    PMdateRangeto->setValidator(new QIntValidator(PMrightSlider->minimum(),
			PMrightSlider->maximum(),PMrightSlider));


	    setPMSliderDates();


	    if (firstrun) {
		// connect slider change handlers only once
		connect(PMleftSlider, SIGNAL(valueChanged(int)),
			this, SLOT(setPMSizeFromSlider()));
		connect(PMrightSlider, SIGNAL(valueChanged(int)),
			this, SLOT(setPMSizeFromSlider()));

		// set slider values  only on the first time
		// set left slider to last 6 months
		if (newdays > (settings->value(GC_PM_DAYS)).toInt())
		    PMleftSlider->setValue(newdays - (settings->value(GC_PM_DAYS).toInt()));
		else
		    PMleftSlider->setValue(0);

		PMrightSlider->setValue(endIndex);
	    }

            perfplot->resize(PMleftSlider->value(),PMrightSlider->value());
	    days = newdays;
            metric = newMetric;
	    count = allRides->childCount();
	}
	perfplot->plot();
    }
}

// performance manager picker callback
void
PerformanceManagerWindow::PMpickerMoved(const QPoint &pos)
{
    double day = perfplot->invTransform(QwtPlot::xBottom, pos.x());
    QDateTime date;
    double sts, lts, sb;

    if (day >= perfplot->min() && day < perfplot->max()) {
	// set the date string
	PMDayValue->setText(perfplot->getStartDate().addDays(day).toString(tr("MMM d yyyy")));


	sts = perfplot->getSTS(day);
	QString STSlabel = QString("%1").arg(sts,0,'f',1,0);
	PMSTSValue->setText(STSlabel);

	lts = perfplot->getLTS(day);
	QString LTSlabel = QString("%1").arg(lts,0,'f',1,0);
	PMLTSValue->setText(LTSlabel);

	sb = perfplot->getSB(day);
	QString SBlabel = QString("%1").arg(sb,0,'f',1,0);
	PMSBValue->setText(SBlabel);
    }

}


void
PerformanceManagerWindow::setPMSizeFromSlider()
{
    perfplot->resize(PMleftSlider->value(),PMrightSlider->value());
    setPMSliderDates();
}

void
PerformanceManagerWindow::setPMSliderDates()
{
    PMdateRangefrom->setText(
	perfplot->getStartDate().addDays(PMleftSlider->value()).toString(tr("MMM d yyyy")));
    PMdateRangeto->setText(
	perfplot->getEndDate().addDays(PMrightSlider->value() - perfplot->n()).toString(tr("MMM d yyyy")));
}

