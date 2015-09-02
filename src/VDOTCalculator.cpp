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

#include <QHeaderView>

VDOTCalculator::VDOTCalculator(QWidget *parent) : QDialog(parent)
{
    bool metricRnPace = appsettings->value(this, GC_PACE, true).toBool();
    setWindowTitle(tr("VDOT and T-Pace Calculator"));

    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Tools_VDOT_CALC));

    setAttribute(Qt::WA_DeleteOnClose);

    setFixedSize(300, 450);

    QVBoxLayout *mainVBox = new QVBoxLayout(this);

    mainVBox->addWidget(new QLabel(tr("Your race (1500 to Marathon):")));

    QHBoxLayout *distHBox = new QHBoxLayout;
    distSpinBox = new QDoubleSpinBox(this);
    distSpinBox->setDecimals(3);
    if (metricRnPace) {
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
    labelVDOT = new QLabel(tr("Your VDOT:"));
    vdotHBox->addWidget(labelVDOT);
    txtVDOT = new QLineEdit(this);
    txtVDOT->setAlignment(Qt::AlignRight);
    txtVDOT->setReadOnly(true);
    vdotHBox->addWidget(txtVDOT, Qt::AlignLeft);
    vdotHBox->addStretch();
    mainVBox->addLayout(vdotHBox);

    mainVBox->addStretch();

    // Training Pace Table
    QVBoxLayout *tableLayout = new QVBoxLayout;
    labelTPACE = new QLabel(tr("Your Daniels' Training Paces:"));
    tableLayout->addWidget(labelTPACE);
    tableWidgetTPACE = new QTableWidget(5, 3, this);
    for (int j = 0; j< 3; j++) tableWidgetTPACE->setColumnWidth(j, 60);
    QStringList vLabels;
    vLabels<<tr("E-Pace")<<tr("M-Pace")<< tr("T-Pace")<<tr("I-Pace")<<tr("R-Pace");
    tableWidgetTPACE->setVerticalHeaderLabels(vLabels);
    tableWidgetTPACE->verticalHeader()->setStretchLastSection(true);
    QStringList hLabels;
    hLabels<< tr("200")<<tr("400")<<(metricRnPace ? tr("1000") : tr("mile"));
    tableWidgetTPACE->setHorizontalHeaderLabels(hLabels);
    tableWidgetTPACE->horizontalHeader()->setStretchLastSection(true);
    for (int i = 0; i < tableWidgetTPACE->rowCount(); i++) {
        for (int j = 0; j < tableWidgetTPACE->rowCount(); j++) {
            QTableWidgetItem *item = new QTableWidgetItem();
            item->setFlags(item->flags() ^ Qt::ItemIsEditable);
            item->setTextAlignment(Qt::AlignCenter);
            tableWidgetTPACE->setItem(i, j, item);
        }
    }
    tableWidgetTPACE->selectRow(2); // Highlight T-Pace
    tableLayout->addWidget(tableWidgetTPACE);
    mainVBox->addLayout(tableLayout);

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
    double paceFactor = metricRnPace ? 1.0 : KM_PER_MILE;

    double mins = hoursSpinBox->value() * 60.0 + minsSpinBox->value() + secsSpinBox->value() / 60.0;
    double dist = distSpinBox->value();

    // velocity m/min
    double vel = paceFactor*1000*dist/mins;

    // estimated VO2 cost of running at vel speed
    double VO2 = -4.3 + 0.182258*vel + 0.000104*pow(vel, 2);

    // fractional utilization of VO2max for mins duration
    double FVO2 = 0.8 + 0.1894393*exp(-0.012778*mins) + 0.2989558*exp(-0.1932605*mins);

    // VDOT: estimated VO2max based on Daniels/Gilbert Formula
    double VDOT = VO2 / FVO2;
    txtVDOT->setText(QString("%1 ml/min/kg").arg(round(VDOT*10)/10));

    // velocity at VO2max according to Daniels/Gilbert Formula
    double vVDOT = 29.54 + 5.000663*VDOT - 0.007546*pow(VDOT, 2);

    // Training Paces relative to vVDOT from Daniels's Running Formula
    double relVDOT[] = { 0.72, 0.85, 0.9, 0.98, 1.05 };
    double relVDOT200[] = { 0.72, 0.85, 0.9, 0.98, 1.07 };
    for (int i = 0; i < tableWidgetTPACE->rowCount(); i++) {
        // 200 m
        double pace200 = 200.0*60.0/vVDOT/relVDOT200[i];
        tableWidgetTPACE->item(i, 0)->setData(Qt::EditRole, QString("%1")
            .arg(i > 2 ? QTime(0,0,0).addSecs(pace200).toString("mm:ss") : "-----"));
        // 400m
        double pace400 = 400.0*60.0/vVDOT/relVDOT[i];
        tableWidgetTPACE->item(i, 1)->setData(Qt::EditRole, QString("%1")
            .arg(i > 1 ? QTime(0,0,0).addSecs(pace400).toString("mm:ss") : "-----"));
        // km or mile
        double pace = 1000.0*60.0*paceFactor/vVDOT/relVDOT[i];
        tableWidgetTPACE->item(i, 2)->setData(Qt::EditRole, QString("%1")
            .arg(i < 4 ? QTime(0,0,0).addSecs(pace).toString("mm:ss") : "-----"));
    }
}
