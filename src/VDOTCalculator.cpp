/*
 * Copyright (c) 2015 Alejandro Martinez (amtriathlon@gmail.com)
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
 *
 * The Daniels-Gilbert fomula used for VDOT and vVDOT was taken from:
 * http://www.simpsonassociatesinc.com/runningmath1.htm
 * T-PACE as 90%vVDOT from Daniels' Running Formula book
 */

#include "VDOTCalculator.h"
#include "HelpWhatsThis.h"
#include "Settings.h"
#include "Units.h"

VDOTCalculator::VDOTCalculator(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("VDOT and T-Pace Calculator"));

    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Tools_VDOT_CALC));

    setAttribute(Qt::WA_DeleteOnClose);

    setFixedSize(300, 250);

    QVBoxLayout *mainVBox = new QVBoxLayout(this);

    mainVBox->addWidget(new QLabel(tr("Your race (1500 to Marathon):")));

    QHBoxLayout *distHBox = new QHBoxLayout;
    distSpinBox = new QDoubleSpinBox(this);
    distSpinBox->setDecimals(3);
    if (appsettings->value(this, GC_PACE, true).toBool()) {
        distSpinBox->setRange(1.5, 42.195);
        distSpinBox->setSuffix(tr(" km"));
        distSpinBox->setValue(10.0);
    } else {
        distSpinBox->setRange(1.5/KM_PER_MILE, 42.195/KM_PER_MILE);
        distSpinBox->setSuffix(tr(" mi"));
        distSpinBox->setValue(10.0/KM_PER_MILE);
    }
    distSpinBox->setSingleStep(1.0);
    distSpinBox->setWrapping(false);
    distSpinBox->setAlignment(Qt::AlignRight);
    distHBox->addWidget(distSpinBox);
    distHBox->addStretch();
    mainVBox->addLayout(distHBox);

    QHBoxLayout *timeHBox = new QHBoxLayout;
    hoursSpinBox = new QDoubleSpinBox(this);
    hoursSpinBox->setDecimals(0);
    hoursSpinBox->setRange(0, 6);
    hoursSpinBox->setSuffix(tr(" hours"));
    hoursSpinBox->setSingleStep(1.0);
    hoursSpinBox->setWrapping(false);
    hoursSpinBox->setAlignment(Qt::AlignRight);
    hoursSpinBox->setValue(0.0);
    timeHBox->addWidget(hoursSpinBox);
    minsSpinBox = new QDoubleSpinBox(this);
    minsSpinBox->setDecimals(0);
    minsSpinBox->setRange(0.0, 59.0);
    minsSpinBox->setSuffix(tr(" mins"));
    minsSpinBox->setSingleStep(1.0);
    minsSpinBox->setWrapping(false);
    minsSpinBox->setAlignment(Qt::AlignRight);
    minsSpinBox->setValue(40.0);
    timeHBox->addWidget(minsSpinBox);
    secsSpinBox = new QDoubleSpinBox(this);
    secsSpinBox->setDecimals(0);
    secsSpinBox->setRange(0.0, 59.0);
    secsSpinBox->setSuffix(tr(" secs"));
    secsSpinBox->setSingleStep(1.0);
    secsSpinBox->setWrapping(true);
    secsSpinBox->setAlignment(Qt::AlignRight);
    timeHBox->addWidget(secsSpinBox);
    mainVBox->addLayout(timeHBox);

    mainVBox->addStretch();

    QHBoxLayout *vdotHBox = new QHBoxLayout;
    vdotHBox->addStretch();
    labelVDOT = new QLabel(tr("Your Daniels VDOT:"));
    vdotHBox->addWidget(labelVDOT);
    txtVDOT = new QLineEdit(this);
    txtVDOT->setAlignment(Qt::AlignRight);
    txtVDOT->setReadOnly(true);
    vdotHBox->addWidget(txtVDOT, Qt::AlignLeft);
    vdotHBox->addStretch();
    mainVBox->addLayout(vdotHBox);

    QHBoxLayout *tpaceHBox = new QHBoxLayout;
    tpaceHBox->addStretch();
    labelTPACE = new QLabel(tr("Your Threshold Pace:"));
    tpaceHBox->addWidget(labelTPACE);
    txtTPACE = new QLineEdit(this);
    txtTPACE->setAlignment(Qt::AlignRight);
    txtTPACE->setReadOnly(true);
    tpaceHBox->addWidget(txtTPACE, Qt::AlignLeft);
    tpaceHBox->addStretch();
    mainVBox->addLayout(tpaceHBox);

    mainVBox->addStretch();

    QHBoxLayout *buttonHBox = new QHBoxLayout;
    btnCalculate = new QPushButton(this);
    btnCalculate->setText(tr("Calculate"));
    buttonHBox->addStretch();
    buttonHBox->addWidget(btnCalculate);
    btnOK = new QPushButton(this);
    btnOK->setText(tr("Done"));
    buttonHBox->addWidget(btnOK);
    mainVBox->addLayout(buttonHBox);

    connect(btnOK, SIGNAL(clicked()), this, SLOT(on_btnOK_clicked()));
    connect(btnCalculate, SIGNAL(clicked()), this, SLOT(on_btnCalculate_clicked()));
}

void VDOTCalculator::on_btnOK_clicked()
{
    accept();
}

void VDOTCalculator::on_btnCalculate_clicked()
{
    bool metricRnPace = appsettings->value(this, GC_PACE, true).toBool();
    double mins = hoursSpinBox->value() * 60.0 + minsSpinBox->value() + secsSpinBox->value() / 60.0;
    double dist = distSpinBox->value();
    // velocity m/min
    double vel = (metricRnPace ? 1.0 : KM_PER_MILE)*1000*dist/mins;
    // estimated VO2 costo of running at vel speed
    double VO2 = -4.3 + 0.182258*vel + 0.000104*pow(vel, 2);
    // fractional utilization of VO2max for mins duration
    double FVO2 = 0.8 + 0.1894393*exp(-0.012778*mins) + 0.2989558*exp(-0.1932605*mins);
    // VDOT: estimated VO2max based on Daniels/Gilbert Formula
    double VDOT = VO2 / FVO2;
    txtVDOT->setText(QString("%1 ml/min/kg").arg(round(VDOT*10)/10));
    // velocity at VO2max according to Daniels/Gilbert Formula
    double vVDOT = 29.54 + 5.000663*VDOT - 0.007546*pow(VDOT, 2);
    // Threshold Pace estimated at 90%vVDOT, from Daniels's Running Formula
    double TPACE = 1000.0/vVDOT/0.9;
    txtTPACE->setText(QString("%1 %2")
        .arg(QTime(0,0,0).addSecs(TPACE*60*(metricRnPace ? 1.0 : KM_PER_MILE)).toString("mm:ss"))
        .arg(metricRnPace ? tr("min/km") : tr("min/mi")));
}
