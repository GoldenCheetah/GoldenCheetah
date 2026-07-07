/*
 * Copyright (c) 2007 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#include "EstimateCPDialog.h"
#include "HelpWhatsThis.h"
#include "Settings.h"
#include "Colors.h"
#include "Context.h"

QHBoxLayout*
EstimateCPDialog::setupMinsSecs(QDoubleSpinBoxPtr &minsSpinBox,
                           QDoubleSpinBoxPtr &secsSpinBox,
                           QDoubleSpinBoxPtr &wattsSpinBox,
                           double maxMin, double defaultMin)
{
    QHBoxLayout *result = new QHBoxLayout;
    minsSpinBox = new QDoubleSpinBox(this);
    minsSpinBox->setDecimals(0);
    minsSpinBox->setRange(0.0, maxMin);
    minsSpinBox->setSuffix(tr(" mins"));
    minsSpinBox->setSingleStep(1.0);
    minsSpinBox->setWrapping(false);
    minsSpinBox->setAlignment(Qt::AlignRight);
    minsSpinBox->setValue(defaultMin);
    result->addWidget(minsSpinBox);
    secsSpinBox = new QDoubleSpinBox(this);
    secsSpinBox->setDecimals(0);
    secsSpinBox->setRange(0.0, 59.0);
    secsSpinBox->setSuffix(tr(" secs"));
    secsSpinBox->setSingleStep(1.0);
    secsSpinBox->setWrapping(true);
    secsSpinBox->setAlignment(Qt::AlignRight);
    result->addWidget(secsSpinBox);
    wattsSpinBox = new QDoubleSpinBox(this);
    wattsSpinBox->setDecimals(0);
    wattsSpinBox->setRange(0.0, 3000.0);
    wattsSpinBox->setSuffix(tr(" watts"));
    wattsSpinBox->setSingleStep(1.0);
    wattsSpinBox->setWrapping(false);
    wattsSpinBox->setAlignment(Qt::AlignRight);
    result->addWidget(wattsSpinBox);
    return result;
}

EstimateCPDialog::EstimateCPDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Critical Power Estimator"));

    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Tools_CP_EST));

    setAttribute(Qt::WA_DeleteOnClose);

    setMinimumSize(300 *dpiXFactor, 300 *dpiYFactor);

    QVBoxLayout *mainVBox = new QVBoxLayout(this);

    QHBoxLayout *hlayout = new QHBoxLayout;
    QLabel *sportLabel = new QLabel(tr("Sport"));
    sportCombo = new QComboBox();
    sportCombo->addItem(tr("Bike"));
    sportCombo->addItem(tr("Run"));
    sportCombo->addItem(tr("Swim"));
    sportCombo->setCurrentIndex(0);
    hlayout->addStretch();
    hlayout->addWidget(sportLabel);
    hlayout->addWidget(sportCombo);
    hlayout->addStretch();
    mainVBox->addLayout(hlayout);

    mainVBox->addWidget(new QLabel(tr("Your best short effort (3-5 min):")));
    mainVBox->addLayout(setupMinsSecs(shortMinsSpinBox, shortSecsSpinBox,
                                      shortWattsSpinBox, 5.0, 3.0));

    mainVBox->addWidget(new QLabel(tr("Your best long effort (15-60 min):")));
    mainVBox->addLayout(setupMinsSecs(longMinsSpinBox, longSecsSpinBox,
                                      longWattsSpinBox, 60.0, 20.0));
    mainVBox->addStretch();

    QHBoxLayout *cpHBox = new QHBoxLayout;
    cpHBox->addStretch();
    labelCP = new QLabel(tr("Your critical power:"));
    cpHBox->addWidget(labelCP);
    txtCP = new QLineEdit(this);
    txtCP->setAlignment(Qt::AlignRight);
    txtCP->setReadOnly(true);
    cpHBox->addWidget(txtCP, Qt::AlignLeft);
    cpHBox->addStretch();
    mainVBox->addLayout(cpHBox);

    QHBoxLayout *wpHBox = new QHBoxLayout;
    wpHBox->addStretch();
    labelWP = new QLabel(tr("Your W':"));
    wpHBox->addWidget(labelWP);
    txtWP = new QLineEdit(this);
    txtWP->setAlignment(Qt::AlignRight);
    txtWP->setReadOnly(true);
    wpHBox->addWidget(txtWP, Qt::AlignLeft);
    wpHBox->addStretch();
    mainVBox->addLayout(wpHBox);

    mainVBox->addStretch();

    QHBoxLayout *buttonHBox = new QHBoxLayout;
    btnCalculate = new QPushButton(this);
    btnCalculate->setText(tr("Estimate"));
    buttonHBox->addStretch();
    buttonHBox->addWidget(btnCalculate);
    btnOK = new QPushButton(this);
    btnOK->setText(tr("Done"));
    buttonHBox->addWidget(btnOK);
    mainVBox->addLayout(buttonHBox);

    connect(sportCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSport()));
    connect(btnOK, SIGNAL(clicked()), this, SLOT(on_btnOK_clicked()));
    connect(btnCalculate, SIGNAL(clicked()), this, SLOT(on_btnCalculate_clicked()));
}

void
EstimateCPDialog::changeSport()
{
    QString rnSuffix = appsettings->value(this, GC_PACE, GlobalContext::context()->useMetricUnits).toBool() ? tr(" km") : tr(" mi");
    QString swSuffix = appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool() ? tr(" m") : tr(" yd");
    switch (sportCombo->currentIndex()) {
        case 0: // Bike
            shortWattsSpinBox->setDecimals(0);
            shortWattsSpinBox->setSuffix(tr(" watts"));
            longWattsSpinBox->setDecimals(0);
            longWattsSpinBox->setSuffix(tr(" watts"));
            labelCP->setText(tr("Your critical power:"));
            labelWP->setText(tr("Your W':"));
            break;
        case 1: // Run
            shortWattsSpinBox->setDecimals(3);
            shortWattsSpinBox->setSuffix(rnSuffix);
            longWattsSpinBox->setDecimals(3);
            longWattsSpinBox->setSuffix(rnSuffix);
            labelCP->setText(tr("Your critical pace:"));
            labelWP->setText(tr("Your D':"));
            break;
        case 2: // Swim
            shortWattsSpinBox->setDecimals(0);
            shortWattsSpinBox->setSuffix(swSuffix);
            longWattsSpinBox->setDecimals(0);
            longWattsSpinBox->setSuffix(swSuffix);
            labelCP->setText(tr("Your critical pace:"));
            labelWP->setText(tr("Your D':"));
            break;
    }
    shortWattsSpinBox->clear();
    longWattsSpinBox->clear();
    txtCP->clear();
    txtWP->clear();
}

void EstimateCPDialog::on_btnOK_clicked()
{
    accept();
}

void EstimateCPDialog::on_btnCalculate_clicked()
{
    bool metricRnPace = appsettings->value(this, GC_PACE, GlobalContext::context()->useMetricUnits).toBool();
    bool metricSwPace = appsettings->value(this, GC_SWIMPACE, GlobalContext::context()->useMetricUnits).toBool();
    double shortSecs =
        shortMinsSpinBox->value() * 60.0 + shortSecsSpinBox->value();
    double shortWatts = shortWattsSpinBox->value();
    double longSecs =
        longMinsSpinBox->value() * 60.0 + longSecsSpinBox->value();
    double longWatts = longWattsSpinBox->value();
    double CP;
    double Wprime;
    switch (sportCombo->currentIndex()) {
        case 0: // Bike
            CP = (longSecs * longWatts - shortSecs * shortWatts)
                 / (longSecs - shortSecs);
            txtCP->setText(tr("%1 watts").arg(static_cast<int>(round(CP))));

            Wprime = ((shortSecs * (shortWatts-CP)) +
                     (longSecs * (longWatts-CP))) /2;
            txtWP->setText(tr("%1 kJ").arg(static_cast<int>(round(Wprime/1000))));
            break;
        case 1: // Run
            CP = (longSecs - shortSecs) / (longWatts - shortWatts);
            txtCP->setText(QString("%1 %2").arg(QTime(0,0,0).addSecs(CP).toString("mm:ss")).arg(metricRnPace ? tr("min/km") : tr("min/mi")));

            Wprime = ((shortWatts-shortSecs/CP) +
                      (longWatts-longSecs/CP)) /2;
            txtWP->setText(QString("%1 %2").arg(round(Wprime*1000)/1000).arg(metricRnPace ? tr("km") : tr("mi")));
            break;
        case 2: // Swim
            CP = 100 * (longSecs - shortSecs) / (longWatts - shortWatts);
            txtCP->setText(QString("%1 %2").arg(QTime(0,0,0).addSecs(CP).toString("mm:ss")).arg(metricSwPace ? tr("min/100m") : tr("min/100yd")));

            Wprime = ((shortWatts-100*shortSecs/CP) +
                      (longWatts-100*longSecs/CP)) /2;
            txtWP->setText(QString("%1 %2").arg(round(Wprime)).arg(metricSwPace ? tr("m") : tr("yd")));
            break;
    }
}

