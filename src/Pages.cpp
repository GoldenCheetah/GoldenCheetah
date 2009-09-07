#include <QtGui>
#include <QIntValidator>
#include <assert.h>
#include "Pages.h"
#include "Settings.h"


ConfigurationPage::~ConfigurationPage()
{
    delete configGroup;
    delete unitLabel;
    delete unitCombo;
    delete allRidesAscending;
    delete warningLabel;
    delete unitLayout;
    delete warningLayout;
    delete configLayout;
    delete mainLayout;
}

ConfigurationPage::ConfigurationPage()
{
    configGroup = new QGroupBox(tr("Golden Cheetah Configuration"));

    unitLabel = new QLabel(tr("Unit of Measurement:"));

    unitCombo = new QComboBox();
    unitCombo->addItem(tr("Metric"));
    unitCombo->addItem(tr("English"));

    boost::shared_ptr<QSettings> settings = GetApplicationSettings();

    QVariant unit = settings->value(GC_UNIT);

    if(unit.toString() == "Metric")
	unitCombo->setCurrentIndex(0);
    else
	unitCombo->setCurrentIndex(1);

    QLabel *crankLengthLabel = new QLabel(tr("Crank Length:"));

    QVariant crankLength = settings->value(GC_CRANKLENGTH);

    crankLengthCombo = new QComboBox();
    crankLengthCombo->addItem("160");
    crankLengthCombo->addItem("162.5");
    crankLengthCombo->addItem("165");
    crankLengthCombo->addItem("167.5");
    crankLengthCombo->addItem("170");
    crankLengthCombo->addItem("172.5");
    crankLengthCombo->addItem("175");
    crankLengthCombo->addItem("177.5");
    crankLengthCombo->addItem("180");
    crankLengthCombo->addItem("182.5");
    crankLengthCombo->addItem("185");
    if(crankLength.toString() == "160")
	crankLengthCombo->setCurrentIndex(0);
    if(crankLength.toString() == "162.5")
	crankLengthCombo->setCurrentIndex(1);
    if(crankLength.toString() == "165")
	crankLengthCombo->setCurrentIndex(2);
    if(crankLength.toString() == "167.5")
	crankLengthCombo->setCurrentIndex(3);
    if(crankLength.toString() == "170")
	crankLengthCombo->setCurrentIndex(4);
    if(crankLength.toString() == "172.5")
	crankLengthCombo->setCurrentIndex(5);
    if(crankLength.toString() == "175")
	crankLengthCombo->setCurrentIndex(6);
    if(crankLength.toString() == "177.5")
	crankLengthCombo->setCurrentIndex(7);
    if(crankLength.toString() == "180")
	crankLengthCombo->setCurrentIndex(8);
    if(crankLength.toString() == "182.5")
	crankLengthCombo->setCurrentIndex(9);
    if(crankLength.toString() == "185")
	crankLengthCombo->setCurrentIndex(10);

    allRidesAscending = new QCheckBox("Sort ride list ascending.", this);
    QVariant isAscending = settings->value(GC_ALLRIDES_ASCENDING,Qt::Checked); // default is ascending sort
    if(isAscending.toInt() > 0 ){
	allRidesAscending->setCheckState(Qt::Checked);
    } else {
	allRidesAscending->setCheckState(Qt::Unchecked);
    }

    warningLabel = new QLabel(tr("Requires Restart To Take Effect"));

    unitLayout = new QHBoxLayout;
    unitLayout->addWidget(unitLabel);
    unitLayout->addWidget(unitCombo);

    warningLayout = new QHBoxLayout;
    warningLayout->addWidget(warningLabel);

    QHBoxLayout *crankLengthLayout = new QHBoxLayout;
    crankLengthLayout->addWidget(crankLengthLabel);
    crankLengthLayout->addWidget(crankLengthCombo);


    // BikeScore Estimate
    QVariant BSdays = settings->value(GC_BIKESCOREDAYS);
    QVariant BSmode = settings->value(GC_BIKESCOREMODE);

    QGridLayout *bsDaysLayout = new QGridLayout;
    bsModeLayout = new QHBoxLayout;
    QLabel *BSDaysLabel1 = new QLabel(tr("BikeScore Estimate: use rides within last "));
    QLabel *BSDaysLabel2 = new QLabel(tr(" days"));
    BSdaysEdit = new QLineEdit(BSdays.toString(),this);
    BSdaysEdit->setInputMask("009");

    QLabel *BSModeLabel = new QLabel(tr("BikeScore estimate mode: "));
    bsModeCombo = new QComboBox();
    bsModeCombo->addItem("time");
    bsModeCombo->addItem("distance");
    if (BSmode.toString() == "time")
	bsModeCombo->setCurrentIndex(0);
    else
	bsModeCombo->setCurrentIndex(1);

    bsDaysLayout->addWidget(BSDaysLabel1,0,0);
    bsDaysLayout->addWidget(BSdaysEdit,0,1);
    bsDaysLayout->addWidget(BSDaysLabel2,0,2);

    bsModeLayout->addWidget(BSModeLabel);
    bsModeLayout->addWidget(bsModeCombo);




    configLayout = new QVBoxLayout;
    configLayout->addLayout(unitLayout);
    configLayout->addWidget(allRidesAscending);
    configLayout->addLayout(crankLengthLayout);
    configLayout->addLayout(bsDaysLayout);
    configLayout->addLayout(bsModeLayout);
    configLayout->addLayout(warningLayout);
    configGroup->setLayout(configLayout);


    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}


CyclistPage::~CyclistPage()
{
    delete cyclistGroup;
    delete lblThreshold;
    delete txtThreshold;
    delete txtThresholdValidator;
    delete btnBack;
    delete btnForward;
    delete btnDelete;
    delete checkboxNew;
    delete txtStartDate;
    delete txtEndDate;
    delete lblStartDate;
    delete lblEndDate;
    delete calendar;
    delete lblCurRange;
    delete powerLayout;
    delete rangeLayout;
    delete dateRangeLayout;
    delete zoneLayout;
    delete calendarLayout;
    delete cyclistLayout;
    delete mainLayout;
}

CyclistPage::CyclistPage(Zones **_zones):
    zones(_zones)
{
    cyclistGroup = new QGroupBox(tr("Cyclist Options"));
    lblThreshold = new QLabel(tr("Critical Power:"));
    txtThreshold = new QLineEdit();

    // the validator will prevent numbers above the upper limit
    // from being entered, but will not prevent non-negative numbers
    // below the lower limit (since it is still plausible a valid
    // entry will result)
    txtThresholdValidator = new QIntValidator(20,999,this);
    txtThreshold->setValidator(txtThresholdValidator);

    btnBack = new QPushButton(this);
    btnBack->setText(tr("Back"));
    btnForward = new QPushButton(this);
    btnForward->setText(tr("Forward"));
    btnDelete = new QPushButton(this);
    btnDelete->setText(tr("Delete Range"));
    checkboxNew = new QCheckBox(this);
    checkboxNew->setText(tr("New Range from Date"));
    btnForward->setEnabled(false);
    txtStartDate = new QLabel("BEGIN");
    txtEndDate = new QLabel("END");
    lblStartDate = new QLabel("Start: ");
    lblStartDate->setAlignment(Qt::AlignRight);
    lblEndDate = new QLabel("End: ");
    lblEndDate->setAlignment(Qt::AlignRight);


    calendar = new QCalendarWidget(this);

    lblCurRange = new QLabel(this);
    lblCurRange->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    lblCurRange->setText(QString("Current Zone Range: %1").arg(currentRange + 1));

    QDate today = QDate::currentDate();
    calendar->setSelectedDate(today);

    if ((! *zones) || ((*zones)->getRangeSize() == 0))
    	setCurrentRange();
    else
    {
    	setCurrentRange((*zones)->whichRange(today));
    	btnDelete->setEnabled(true);
	checkboxNew->setCheckState(Qt::Unchecked);
    }
    
    int cp = (*zones ? (*zones)->getCP(currentRange) : 0);
    if (cp > 0)
	setCP(cp);

    //Layout
    powerLayout = new QHBoxLayout();
    powerLayout->addWidget(lblThreshold);
    powerLayout->addWidget(txtThreshold);

    rangeLayout = new QHBoxLayout();
    rangeLayout->addWidget(lblCurRange);
    
    dateRangeLayout = new QHBoxLayout();
    dateRangeLayout->addWidget(lblStartDate);
    dateRangeLayout->addWidget(txtStartDate);
    dateRangeLayout->addWidget(lblEndDate);
    dateRangeLayout->addWidget(txtEndDate);

    zoneLayout = new QHBoxLayout();
    zoneLayout->addWidget(btnBack);
    zoneLayout->addWidget(btnForward);
    zoneLayout->addWidget(btnDelete);
    zoneLayout->addWidget(checkboxNew);

    calendarLayout = new QHBoxLayout();
    calendarLayout->addWidget(calendar);

    cyclistLayout = new QVBoxLayout;
    cyclistLayout->addLayout(powerLayout);
    cyclistLayout->addLayout(rangeLayout);
    cyclistLayout->addLayout(zoneLayout);
    cyclistLayout->addLayout(dateRangeLayout);
    cyclistLayout->addLayout(calendarLayout);

    cyclistGroup->setLayout(cyclistLayout);

    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(cyclistGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

QString CyclistPage::getText()
{
    return txtThreshold->text();
}

int CyclistPage::getCP()
{
    int cp = txtThreshold->text().toInt();
    return (
	    (
	     (cp >= txtThresholdValidator->bottom()) &&
	     (cp <= txtThresholdValidator->top())
	     ) ?
	    cp :
	    0
	    );
}

void CyclistPage::setCP(int cp)
{
    txtThreshold->setText(QString("%1").arg(cp));
}

void CyclistPage::setSelectedDate(QDate date)
{
    calendar->setSelectedDate(date);
}

void CyclistPage::setCurrentRange(int range)
{
    int num_ranges =
	*zones ?
	(*zones)->getRangeSize() :
	0;

    if ((num_ranges == 0) || (range < 0)) {
	btnBack->setEnabled(false);
	btnDelete->setEnabled(false);
	btnForward->setEnabled(false);
	calendar->setEnabled(false);
	checkboxNew->setCheckState(Qt::Checked);
	checkboxNew->setEnabled(false);
	currentRange = -1;
	lblCurRange->setText("no Current Zone Range");
	txtEndDate->setText("undefined");
	txtStartDate->setText("undefined");
	return;
    }

    assert ((range >= 0) && (range < num_ranges));
    currentRange = range;

    // update the labels
    lblCurRange->setText(QString("Current Zone Range: %1").arg(currentRange + 1));

    // update the visibility of the range select buttons
    btnForward->setEnabled(currentRange < num_ranges - 1);
    btnBack->setEnabled(currentRange > 0);

    // if we have ranges to set to, then the calendar must be on
    calendar->setEnabled(true);

    // update the CP display
    setCP((*zones)->getCP(currentRange));

    // update date limits
    txtStartDate->setText((*zones)->getStartDateString(currentRange));
    txtEndDate->setText((*zones)->getEndDateString(currentRange));
}


int CyclistPage::getCurrentRange()
{
    return currentRange;
}


bool CyclistPage::isNewMode()
{
    return (checkboxNew->checkState() == Qt::Checked);
}
