
#include "PerformanceManagerWindow.h"
#include "MainWindow.h"
#include "Context.h"
#include "Athlete.h"
#include "PerfPlot.h"
#include "Colors.h"
#include "StressCalculator.h"
#include "RideItem.h"
#include <qwt_picker_machine.h>


PerformanceManagerWindow::PerformanceManagerWindow(Context *context) :
    GcWindow(context), context(context), active(false), isfiltered(false)
{
    setControls(NULL);

    days = count = 0;
    sc = NULL;

    QVBoxLayout *vlayout = new QVBoxLayout;

    QHBoxLayout *PMPickerLayout = new QHBoxLayout;

    QLabel *PMSTSLabel = new QLabel(appsettings->value(this, GC_STS_ACRONYM,"STS").toString() + ":", this);
    PMSTSValue = new QLineEdit("0");
    PMSTSValue->setReadOnly(true);
    PMSTSValue->setValidator(new QDoubleValidator(0,500,1,PMSTSValue));
    PMPickerLayout->addWidget(PMSTSLabel);
    PMPickerLayout->addWidget(PMSTSValue);

    QLabel *PMLTSLabel = new QLabel(appsettings->value(this, GC_LTS_ACRONYM,"LTS").toString() + ":", this);

    PMLTSValue = new QLineEdit("0");
    PMLTSValue->setReadOnly(true);
    PMLTSValue->setValidator(new QDoubleValidator(0,500,1,PMLTSValue));
    PMPickerLayout->addWidget(PMLTSLabel);
    PMPickerLayout->addWidget(PMLTSValue);

    QLabel *PMSBLabel = new QLabel(appsettings->value(this, GC_SB_ACRONYM,"SB").toString() + ":", this);

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
    metricCombo->addItem(tr("Use TSS"), "coggan_tss");
    metricCombo->addItem(tr("Use Aerobic TISS"), "atiss_score");
    metricCombo->addItem(tr("Use BikeScore"), "skiba_bike_score");
    metricCombo->addItem(tr("Use DanielsPoints"), "daniels_points");
    metricCombo->addItem(tr("Use TRIMP"), "trimp_points");
    metricCombo->addItem(tr("Use TRIMP 100"), "trimp_100_points");
    metricCombo->addItem(tr("Use Trimp Zonal"), "trimp_zonal_points");
    metricCombo->addItem(tr("Use Work (Kj)"), "total_work");
    metricCombo->addItem(tr("Use W' Work (Kj)"), "skiba_wprime_exp");
    metricCombo->addItem(tr("Use Distance (km/mi)"), "total_distance");
    QString metricName =
        appsettings->value(this, GC_PERF_MAN_METRIC, "skiba_bike_score").toString();
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

    PMpicker = new QwtPlotPicker(QwtAxis::XBottom, QwtAxis::YLeft,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOff, perfplot->canvas());
    PMpicker->setStateMachine(new QwtPickerDragPointMachine);
    PMpicker->setRubberBandPen(GColor(CPLOTSELECT));

    connect(PMpicker, SIGNAL(moved(const QPoint &)),
           SLOT(PMpickerMoved(const QPoint &)));
    connect(metricCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(metricChanged()));
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));
    connect(context, SIGNAL(configChanged()), perfplot, SLOT(configUpdate()));
    connect(context, SIGNAL(rideAdded(RideItem*)), this, SLOT(replot()));
    connect(context, SIGNAL(rideDeleted(RideItem*)), this, SLOT(replot()));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
#ifdef GC_HAVE_LUCENE
    connect(context, SIGNAL(filterChanged()), this, SLOT(filterChanged()));
#endif

    // set colors
    configChanged();
}

PerformanceManagerWindow::~PerformanceManagerWindow()
{
    if (sc)
	delete sc;
}

#ifdef GC_HAVE_LUCENE
void 
PerformanceManagerWindow::filterChanged()
{
    filter = context->filters;
    isfiltered = context->isfiltered;
    days = 0; // force it
    replot();
    repaint();
}
#endif

void PerformanceManagerWindow::configChanged()
{
    if (active) {
        days = 0; // force replot
        replot();
    }
    PMpicker->setRubberBandPen(GColor(CPLOTSELECT));
    setProperty("color", GColor(CPLOTBACKGROUND));

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setBrush(QPalette::Background, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setBrush(QPalette::Base, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::Normal, QPalette::Window, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);
    setStyleSheet(QString("background-color: %1; color: %2; border: %1")
                    .arg(GColor(CPLOTBACKGROUND).name())
                    .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name()));
}

void PerformanceManagerWindow::metricChanged()
{
    QString newMetric = metricCombo->itemData(metricCombo->currentIndex()).toString();
    appsettings->setValue(GC_PERF_MAN_METRIC, newMetric);
    replot();
}

void PerformanceManagerWindow::rideSelected()
{
    bool wasActive = active;
    active = amVisible();
    if (!wasActive && active)
        replot();
}

void PerformanceManagerWindow::replot()
{
    const QTreeWidgetItem *allRides = context->athlete->allRideItems();

    int newdays, rightIndex, endIndex;
    RideItem *firstRideItem = NULL;
    RideItem *lastRideItem = NULL;
    QDateTime now;


    // calculate the number of days to look at... for now
    // use first ride in allRides to today.  When Season stuff is hooked
    // up, maybe use that, or will allRides reflect only current season?
    if (allRides->childCount()) {
        firstRideItem =  (RideItem*) allRides->child(0);
        lastRideItem =  (RideItem*) allRides->child(allRides->childCount()-1);
    }

    if (firstRideItem) {
        int lookahead = (appsettings->cvalue(context->athlete->cyclist, GC_STS_DAYS,7)).toInt();
        QDateTime endTime = std::max(lastRideItem->dateTime, now.currentDateTime());
        endTime = endTime.addDays( lookahead );
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
            context->athlete->cyclist,
		    firstRideItem->dateTime,
		    endTime,
		    (appsettings->cvalue(context->athlete->cyclist, GC_STS_DAYS,7)).toInt(),
		    (appsettings->cvalue(context->athlete->cyclist, GC_LTS_DAYS,42)).toInt());
#ifdef GC_HAVE_LUCENE
            sc->calculateStress(context,context->athlete->home.absolutePath(),newMetric,isfiltered,filter);
#else
            sc->calculateStress(context,context->athlete->home.absolutePath(),newMetric);
#endif

	    perfplot->setStressCalculator(sc);

	    endIndex  = sc->n();
        rightIndex = std::max(0,endIndex-lookahead);


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
		if (newdays > 182)
		    PMleftSlider->setValue(newdays - 182);
		else
		    PMleftSlider->setValue(0);

		PMrightSlider->setValue(rightIndex);
	    }

            perfplot->resize(PMleftSlider->value(),PMrightSlider->value());
	    days = newdays;
            metric = newMetric;
	    count = allRides->childCount();
	}
	perfplot->plot();
    }
    repaint();
}

// performance manager picker callback
void
PerformanceManagerWindow::PMpickerMoved(const QPoint &pos)
{
    double day = perfplot->invTransform(QwtAxis::XBottom, pos.x());
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

