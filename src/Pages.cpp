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

/*
CyclistPage::CyclistPage(QWidget *parent)
: QWidget(parent)
{
    QGroupBox *cyclistGroup = new QGroupBox(tr("Cyclist Options"));

    QLabel *lblThreshold = new QLabel(tr("Threshold Power:"));
    txtThreshold = new QLineEdit();

    //QLabel *lblWeight = new QLabel(tr("Weight:"));
    //QLineEdit *txtWeight = new QLineEdit(this);


    //Layout
    QHBoxLayout *powerLayout = new QHBoxLayout;
    powerLayout->addWidget(lblThreshold);
    powerLayout->addWidget(txtThreshold);

    
    //QHBoxLayout *thresholdLayout = new QHBoxLayout;
    //thresholdLayout->addWidget(lblWeight);
    //thresholdLayout->addWidget(txtWeight);

    QVBoxLayout *cyclistLayout = new QVBoxLayout;
    cyclistLayout->addLayout(powerLayout);
    //cyclistLayout->addLayout(thresholdLayout);
    cyclistGroup->setLayout(cyclistLayout);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(cyclistGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}

QString CyclistPage::getText()
{
     qDebug() << txtThreshold->text();
     return txtThreshold->text();
}
*/

/*
PowerPage::PowerPage(QWidget *parent)
: QWidget(parent)
{
    QGroupBox *powerGroup = new QGroupBox(tr("Power Levels"));

    QLabel *lblThreshold = new QLabel(tr("Threshold Power:"));
    QLineEdit *txtThreshold = new QLineEdit(this);

    QLabel *lblWeight = new QLabel(tr("Weight:"));
    QLineEdit *txtWeight = new QLineEdit(this);


    //Layout
    QHBoxLayout *powerLayout = new QHBoxLayout;
    powerLayout->addWidget(lblThreshold);
    powerLayout->addWidget(txtThreshold);

    
    QHBoxLayout *thresholdLayout = new QHBoxLayout;
    thresholdLayout->addWidget(lblWeight);
    thresholdLayout->addWidget(txtWeight);

    QVBoxLayout *cyclistLayout = new QVBoxLayout;
    cyclistLayout->addLayout(powerLayout);
    cyclistLayout->addLayout(thresholdLayout);
    powerGroup->setLayout(cyclistLayout);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(powerGroup);
    mainLayout->addStretch(1);
    setLayout(mainLayout);
}
*/

//Active recovery: <55%
//Endurance: 56-75%
//Tempo: 76-90%
//Threshold: 91-105%
//VO2 max: 106-120%
//Anaerobic Capacity: 121-150%
//Neuromuscular power: N/A but could be anything >151%
