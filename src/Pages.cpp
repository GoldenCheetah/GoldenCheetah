#include <QtGui>
#include "Pages.h"
#include "Settings.h"

ConfigurationPage::ConfigurationPage(QWidget *parent)
: QWidget(parent)
{
    QGroupBox *configGroup = new QGroupBox(tr("Golden Cheetah Configuration"));

    QLabel *unitLabel = new QLabel(tr("Unit of Measurement:"));

    unitCombo = new QComboBox();
    unitCombo->addItem(tr("Metric"));
    unitCombo->addItem(tr("English"));

    QSettings settings(GC_SETTINGS_CO, GC_SETTINGS_APP);
    QVariant unit = settings.value(GC_UNIT);

    if(unit.toString() == "Metric")
       unitCombo->setCurrentIndex(0);
    else
       unitCombo->setCurrentIndex(1);

    QLabel *crankLengthLabel = new QLabel(tr("Crank Length:"));
    
    QVariant crankLength = settings.value(GC_CRANKLENGTH);

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
    QVariant isAscending = settings.value(GC_ALLRIDES_ASCENDING,Qt::Checked); // default is ascending sort
    if(isAscending.toInt() > 0 ){
        allRidesAscending->setCheckState(Qt::Checked);
    } else {
        allRidesAscending->setCheckState(Qt::Unchecked);
    }
    
    QLabel *warningLabel = new QLabel(tr("Requires Restart To Take Effect"));

    QHBoxLayout *unitLayout = new QHBoxLayout;
    unitLayout->addWidget(unitLabel);
    unitLayout->addWidget(unitCombo);

    QHBoxLayout *warningLayout = new QHBoxLayout;
    warningLayout->addWidget(warningLabel);
    
    QHBoxLayout *crankLengthLayout = new QHBoxLayout;
    crankLengthLayout->addWidget(crankLengthLabel);
    crankLengthLayout->addWidget(crankLengthCombo);
 
    QVBoxLayout *configLayout = new QVBoxLayout;
    configLayout->addLayout(unitLayout);
    configLayout->addWidget(allRidesAscending);
    configLayout->addLayout(crankLengthLayout);
    configLayout->addLayout(warningLayout);
    configGroup->setLayout(configLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}


CyclistPage::CyclistPage(QWidget *parent, Zones *_zones, bool emptyZone)
        : QWidget(parent)
{
    QGroupBox *cyclistGroup = new QGroupBox(tr("Cyclist Options"));

    zones = _zones;

    setChoseNewZone(false);

    QLabel *lblThreshold = new QLabel(tr("Critical Power:"));
    txtThreshold = new QLineEdit();
    txtThreshold->setInputMask("999");
    btnBack = new QPushButton(this);
    btnBack->setText(tr("Back"));
    btnForward = new QPushButton(this);
    btnForward->setText(tr("Forward"));
    btnNew = new QPushButton(this);
    btnNew->setText(tr("New Zone Range"));
    btnForward->setEnabled(false);
    txtStartDate = new QDateEdit();
    txtEndDate = new QDateEdit();
    lblStartDate = new QLabel("Start");
    lblEndDate = new QLabel("End");

    calendar = new QCalendarWidget(this);

    txtStartDate->setEnabled(false);
    txtEndDate->setEnabled(false);
    
    if(emptyZone)
    {
    	setCurrentRange(0);
    	QDate date;
    	btnNew->setEnabled(false);
    	calendar->setSelectedDate(date.currentDate());
    	calendar->setEnabled(true);
    	btnBack->setEnabled(false);
    	txtEndDate->setDate(date.currentDate());
    }
    else if(zones->getRangeSize() == 1)
    {
    	setCurrentRange(0);
    	QDate date;
    	btnNew->setEnabled(true);
    	btnBack->setEnabled(false);
    	calendar->setSelectedDate(date.currentDate());
    	calendar->setEnabled(false);
    	//txtStartDate->setDate(date.currentDate()));
    	//txtEndDate->setDate(date.currentDate()));

    }
    else
    {
    	setCurrentRange(zones->getRangeSize() - 1);
    	QDate date = zones->getStartDate(getCurrentRange());
    	calendar->setSelectedDate(date);
    	calendar->setMinimumDate(date);
    	calendar->setEnabled(false);
    	txtStartDate->setDate(zones->getStartDate(getCurrentRange()));
    	txtEndDate->setDate(zones->getEndDate(getCurrentRange()));
    }

    txtStartDate->setEnabled(false);
    txtEndDate->setEnabled(false);
    
    if(emptyZone)
    {
    	setCurrentRange(0);
    	QDate date;
    	btnNew->setEnabled(false);
    	calendar->setSelectedDate(date.currentDate());
    	calendar->setEnabled(true);
    	btnBack->setEnabled(false);
    	txtEndDate->setDate(date.currentDate());
    }
    else if(zones->getRangeSize() == 1)
    {
    	setCurrentRange(0);
    	QDate date = zones->getEndDate(getCurrentRange());
    	calendar->setMinimumDate(date);
    	btnNew->setEnabled(true);
    	btnBack->setEnabled(false);
    	calendar->setSelectedDate(date);
    	calendar->setEnabled(false);
    	txtStartDate->setDate(zones->getStartDate(getCurrentRange()));
    	txtEndDate->setDate(zones->getEndDate(getCurrentRange()));

    }
    else
    {
    	setCurrentRange(zones->getRangeSize() - 1);
    	QDate date = zones->getStartDate(getCurrentRange());
    	calendar->setSelectedDate(date);
    	calendar->setMinimumDate(date);
    	calendar->setEnabled(false);
    	txtStartDate->setDate(zones->getStartDate(getCurrentRange()));
    	txtEndDate->setDate(zones->getEndDate(getCurrentRange()));
    }

    QString strCP;
    int ftp = zones->getCP(getCurrentRange());
    txtThreshold->setText(strCP.setNum(ftp));

    lblCurRange = new QLabel(this);
    lblCurRange->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    lblCurRange->setText(QString("Current Zone Range: %1").arg(getCurrentRange() + 1));

    

    //Layout
    QHBoxLayout *powerLayout = new QHBoxLayout();
    powerLayout->addWidget(lblThreshold);
    powerLayout->addWidget(txtThreshold);

    QHBoxLayout *rangeLayout = new QHBoxLayout();
    rangeLayout->addWidget(lblCurRange);
    
    QHBoxLayout *dateRangeLayout = new QHBoxLayout();
    dateRangeLayout->addWidget(lblStartDate);
    dateRangeLayout->addWidget(txtStartDate);
    dateRangeLayout->addWidget(lblEndDate);
    dateRangeLayout->addWidget(txtEndDate);

    QHBoxLayout *zoneLayout = new QHBoxLayout();
    zoneLayout->addWidget(btnBack);
    zoneLayout->addWidget(btnForward);
    zoneLayout->addWidget(btnNew);

    QHBoxLayout *calendarLayout = new QHBoxLayout();
    calendarLayout->addWidget(calendar);

    QVBoxLayout *cyclistLayout = new QVBoxLayout;
    cyclistLayout->addLayout(powerLayout);
    cyclistLayout->addLayout(rangeLayout);
    cyclistLayout->addLayout(zoneLayout);
    cyclistLayout->addLayout(dateRangeLayout);
    cyclistLayout->addLayout(calendarLayout);

    cyclistGroup->setLayout(cyclistLayout);


    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(cyclistGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

QString CyclistPage::getText()
{
    return txtThreshold->text();
}

void CyclistPage::setCurrentRange(int _range)
{
    currentRange = _range;
}

int CyclistPage::getCurrentRange()
{
    return currentRange;
}

bool CyclistPage::isNewZone()
{
    return newZone;
}

void CyclistPage::setChoseNewZone(bool _newZone)
{
    newZone = _newZone;
}


