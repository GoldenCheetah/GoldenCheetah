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

    QLabel *warningLabel = new QLabel(tr("Requires Restart To Take Effect"));

    QHBoxLayout *unitLayout = new QHBoxLayout;
    unitLayout->addWidget(unitLabel);
    unitLayout->addWidget(unitCombo);

    QHBoxLayout *warningLayout = new QHBoxLayout;
    warningLayout->addWidget(warningLabel);
 
    QVBoxLayout *configLayout = new QVBoxLayout;
    configLayout->addLayout(unitLayout);
    configLayout->addLayout(warningLayout);
    configGroup->setLayout(configLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(configGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}


CyclistPage::CyclistPage(QWidget *parent, Zones *_zones)
        : QWidget(parent)
{
    QGroupBox *cyclistGroup = new QGroupBox(tr("Cyclist Options"));

    zones = _zones;

    setChoseNewZone(false);

    QLabel *lblThreshold = new QLabel(tr("Critical Power:"));
    txtThreshold = new QLineEdit();
    txtThreshold->setInputMask("999");


    calendar = new QCalendarWidget(this);
    setCurrentRange(zones->ranges.size() - 1);
    QDate date = zones->getStartDate(getCurrentRange());
    calendar->setSelectedDate(date);
    calendar->setMinimumDate(date);
    calendar->setEnabled(false);

    QString strCP;

    setCurrentRange((zones->ranges.size() - 1));
    int ftp = zones->getCP(zones->ranges.size() - 1);
    txtThreshold->setText(strCP.setNum(ftp));

    lblCurRange = new QLabel(this);
    lblCurRange->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    lblCurRange->setText(QString("Current Zone Range: %1").arg(getCurrentRange() + 1));

    btnBack = new QPushButton(this);
    btnBack->setText(tr("Back"));
    btnForward = new QPushButton(this);
    btnForward->setText(tr("Forward"));
    btnNew = new QPushButton(this);
    btnNew->setText(tr("New Zone Range"));

    btnForward->setEnabled(false);

    //Layout
    QHBoxLayout *powerLayout = new QHBoxLayout();
    powerLayout->addWidget(lblThreshold);
    powerLayout->addWidget(txtThreshold);

    QHBoxLayout *rangeLayout = new QHBoxLayout();
    rangeLayout->addWidget(lblCurRange);

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


