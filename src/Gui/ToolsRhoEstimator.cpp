/*
 * Copyright (c) Steven Gribble (gribble [at] gmail [dot] com)
 * http://www.gribble.org/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#include "ToolsRhoEstimator.h"
#include "Settings.h"
#include "Context.h"
#include "Athlete.h"
#include "Units.h"
#include "Colors.h"
#include "HelpWhatsThis.h"

#include <QtGui>
#include <sstream>
#include <cmath>

typedef QDoubleSpinBox* QDoubleSpinBoxPtr;

ToolsRhoEstimator::ToolsRhoEstimator(Context *context, QWidget *parent) : QDialog(parent), context(context) {

  // Set the main window title.
  setWindowTitle(tr("Air Density (Rho) Estimator"));
  HelpWhatsThis *help = new HelpWhatsThis(this);
  this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Tools_AirDens_EST));
  setAttribute(Qt::WA_DeleteOnClose);

  // Create the main layout box.
  QVBoxLayout *mainVBox = new QVBoxLayout(this);

  // Set up the instructions field.
  mainVBox->addWidget(new QLabel(
       tr("Enter measured values for the following:")));
  mainVBox->addSpacing(5);

  // "metric vs. imperial" radio buttons.  (Makes it easier than
  // forcing them to change their preference in the preferences menu.)
  QHBoxLayout *rads = new QHBoxLayout;
  metBut = new QRadioButton(tr("Metric"));
  metBut->setChecked(GlobalContext::context()->useMetricUnits);
  rads->addWidget(metBut);
  impBut = new QRadioButton(tr("Imperial"));
  impBut->setChecked(!GlobalContext::context()->useMetricUnits);
  // note that we only need to connect one of the radio button
  // signals, since changing one also changes the other.
  connect(impBut, SIGNAL(toggled(bool)),
          this, SLOT(on_radio_toggled(bool)));
  rads->addWidget(impBut);
  mainVBox->addLayout(rads);

  // The temperature box.
  QHBoxLayout *thl = new QHBoxLayout;
  tempSpinBox = new QDoubleSpinBox(this);
  tempSpinBox->setDecimals(2);
  tempSpinBox->setRange(-200, 200);
  if (GlobalContext::context()->useMetricUnits) {
    tempLabel = new QLabel(tr("Temperature (C):"));
    thl->addWidget(tempLabel);
    tempSpinBox->setValue(15);
  } else {
    tempLabel = new QLabel(tr("Temperature (F):"));
    thl->addWidget(tempLabel);
    tempSpinBox->setValue(59);
  }
  // Use Average Temp from current activity when available
  if (context && context->rideItem() &&
      context->rideItem()->getForSymbol("average_temp", GlobalContext::context()->useMetricUnits) != RideFile::NA) {
    tempSpinBox->setValue(context->rideItem()->getForSymbol("average_temp", GlobalContext::context()->useMetricUnits));
  }
  tempSpinBox->setWrapping(false);
  tempSpinBox->setAlignment(Qt::AlignRight);
  connect(tempSpinBox, SIGNAL(valueChanged(double)),
          this, SLOT(on_valueChanged(double)));
  thl->addWidget(tempSpinBox);
  mainVBox->addLayout(thl);

  // The air pressure box.
  QHBoxLayout *phl = new QHBoxLayout;
  pressSpinBox = new QDoubleSpinBox(this);
  pressSpinBox->setDecimals(2);
  pressSpinBox->setRange(0, 2000);
  if (GlobalContext::context()->useMetricUnits) {
    pressLabel = new QLabel(tr("Air Pressure (hPa):"));
    phl->addWidget(pressLabel);
    pressSpinBox->setValue(1018);
  } else {
    pressLabel = new QLabel(tr("Air Pressure (inHg):"));
    phl->addWidget(pressLabel);
    pressSpinBox->setValue(30.06);
  }
  pressSpinBox->setWrapping(false);
  pressSpinBox->setAlignment(Qt::AlignRight);
  connect(pressSpinBox, SIGNAL(valueChanged(double)),
          this, SLOT(on_valueChanged(double)));
  phl->addWidget(pressSpinBox);
  mainVBox->addLayout(phl);

  // The dewpoint box.
  QHBoxLayout *dhl = new QHBoxLayout;
  dewpSpinBox = new QDoubleSpinBox(this);
  dewpSpinBox->setDecimals(2);
  dewpSpinBox->setRange(-200, 200);
  if (GlobalContext::context()->useMetricUnits) {
    dewpLabel = new QLabel(tr("Dewpoint (C):"));
    dhl->addWidget(dewpLabel);
    dewpSpinBox->setValue(7.5);
  } else {
    dewpLabel = new QLabel(tr("Dewpoint (F):"));
    dhl->addWidget(dewpLabel);
    dewpSpinBox->setValue(45.5);
  }
  dewpSpinBox->setWrapping(false);
  dewpSpinBox->setAlignment(Qt::AlignRight);
  connect(dewpSpinBox, SIGNAL(valueChanged(double)),
          this, SLOT(on_valueChanged(double)));
  dhl->addWidget(dewpSpinBox);
  mainVBox->addLayout(dhl);

  // The relative humidity box.
  QHBoxLayout *hhl = new QHBoxLayout;
  rhumLabel = new QLabel(tr("Relative Humidity (%):"));
  hhl->addWidget(rhumLabel);
  rhumSpinBox = new QDoubleSpinBox(this);
  rhumSpinBox->setDecimals(2);
  rhumSpinBox->setRange(0, 100);
  rhumSpinBox->setWrapping(false);
  rhumSpinBox->setAlignment(Qt::AlignRight);
  connect(rhumSpinBox, SIGNAL(valueChanged(double)),
          this, SLOT(on_valueChanged(double)));
  hhl->addWidget(rhumSpinBox);
  rhumGroupBox = new QGroupBox(tr("Use Relative Humidity"));
  rhumGroupBox->setCheckable(true);
  rhumGroupBox->setChecked(false);
  rhumGroupBox->setLayout(hhl);
  connect(rhumGroupBox, SIGNAL(toggled(bool)),
          this, SLOT(on_valueChanged()));
  mainVBox->addWidget(rhumGroupBox);

  // Label for the computed air density.
  mainVBox->addSpacing(15);
  mainVBox->addWidget(new QLabel(
        tr("Estimated air density (rho):")));

  // Estimated Rho (metric units).
  QHBoxLayout *rhoMetHBox = new QHBoxLayout;
  txtRhoMet = new QLineEdit(this);
  txtRhoMet->setAlignment(Qt::AlignRight);
  txtRhoMet->setReadOnly(true);
  rhoMetHBox->addWidget(txtRhoMet);
  rhoMetHBox->addWidget(new QLabel(tr("(kg/m^3)")));
  mainVBox->addLayout(rhoMetHBox);

  // Estimated Rho (imperial units).
  QHBoxLayout *rhoImpHBox = new QHBoxLayout;
  txtRhoImp = new QLineEdit(this);
  txtRhoImp->setAlignment(Qt::AlignRight);
  txtRhoImp->setReadOnly(true);
  rhoImpHBox->addWidget(txtRhoImp);
  rhoImpHBox->addWidget(new QLabel(tr("(lb/ft^3)")));
  mainVBox->addLayout(rhoImpHBox);

  // "Done" button.
  mainVBox->addSpacing(15);
  QHBoxLayout *buttonHBox = new QHBoxLayout;
  btnOK = new QPushButton(this);
  btnOK->setText(tr("Done"));
  buttonHBox->addWidget(btnOK);
  mainVBox->addLayout(buttonHBox);
  connect(btnOK, SIGNAL(clicked()), this, SLOT(on_btnOK_clicked()));

  // Force the initial "rho" calculation when the dialog box comes up
  // initially.
  this->on_valueChanged(0.0);
}

void ToolsRhoEstimator::on_radio_toggled(bool /* checked */) {

  if (impBut->isChecked()) {
    // we just changed from metric --> imperial, so relabel and do the
    // field conversions for the user.
    tempSpinBox->setValue(celsius_to_fahrenheit(tempSpinBox->value()));
    tempLabel->setText(tr("Temperature (F):"));
    pressSpinBox->setValue(
        hectopascals_to_inchesmercury(pressSpinBox->value()));
    pressLabel->setText(tr("Air Pressure (inHg):"));
    dewpSpinBox->setValue(celsius_to_fahrenheit(dewpSpinBox->value()));
    dewpLabel->setText(tr("Dewpoint (F):"));
  } else {
    // we just changed from imperial --> metric, so relabel and do the
    // field conversions for the user.
    tempSpinBox->setValue(fahrenheit_to_celsius(tempSpinBox->value()));
    tempLabel->setText(tr("Temperature (C):"));
    pressSpinBox->setValue(
        inchesmercury_to_hectopascals(pressSpinBox->value()));
    pressLabel->setText(tr("Air Pressure (hPa):"));
    dewpSpinBox->setValue(fahrenheit_to_celsius(dewpSpinBox->value()));
    dewpLabel->setText(tr("Dewpoint (C):"));
  }

  // Relay the "something has changed" signal to
  // on_valueChanged(double).
  this->on_valueChanged(0.0);
}

void ToolsRhoEstimator::on_btnOK_clicked() {
  // all done!
  accept();
}

void ToolsRhoEstimator::on_valueChanged(double newval) {
  Q_UNUSED(newval)

  // scrape the field values, convert to metric if needed.
  double temp = tempSpinBox->value();
  double press = pressSpinBox->value();
  double dewp = dewpSpinBox->value();
  if (impBut->isChecked()) {
    // yup, convert to metric.
    temp = fahrenheit_to_celsius(temp);
    press = inchesmercury_to_hectopascals(press);
    dewp = fahrenheit_to_celsius(dewp);
  }

  if (rhumGroupBox->isChecked()) {
    // compute dew point from relative humidity
    dewpSpinBox->setEnabled(false);
    dewp = dewp_from_rhum(rhumSpinBox->value(), temp);
    dewpSpinBox->setValue(impBut->isChecked() ? celsius_to_fahrenheit(dewp) : dewp);
  } else {
    // compute relative humidity from dew point
    rhumSpinBox->setValue(rhum_from_dewp(dewp, temp));
    dewpSpinBox->setEnabled(true);
  }

  // calculate rho, in (kg/m^3)
  double rho_met = calculate_rho(temp, press, dewp);
  // calculate rho in imperial units.
  double rho_imp = rho_met_to_imp(rho_met);

  // display the calculated Rho's.
  std::stringstream ss_met, ss_imp;
  ss_met.precision(6);
  ss_met << rho_met;
  txtRhoMet->setText(tr(ss_met.str().c_str()));
  ss_imp.precision(6);
  ss_imp << rho_imp;
  txtRhoImp->setText(tr(ss_imp.str().c_str()));
}

double ToolsRhoEstimator::fahrenheit_to_celsius(double f) {
  return (f - 32.0) * (5.0/9.0);
}

double ToolsRhoEstimator::celsius_to_fahrenheit(double c) {
    return (c * 9.0 / 5.0) + 32.0;
}

double ToolsRhoEstimator::hectopascals_to_inchesmercury(double hpa) {
  return (hpa / 1000.0) * 29.53;
}

double ToolsRhoEstimator::inchesmercury_to_hectopascals(double inhg) {
  return (inhg * 1000.0) / 29.53;
}

double ToolsRhoEstimator::rho_met_to_imp(double rho) {
  return rho * ((0.30480061 * 0.30480061 * 0.30480061) / KG_PER_LB);
}

// source: http://bmcnoldy.rsmas.miami.edu/Humidity.html
double ToolsRhoEstimator::dewp_from_rhum(double rhum, double temp) {
    return 243.04*(log(rhum/100)+((17.625*temp)/(243.04+temp)))/(17.625-log(rhum/100)-((17.625*temp)/(243.04+temp)));
}

// source: http://bmcnoldy.rsmas.miami.edu/Humidity.html
double ToolsRhoEstimator::rhum_from_dewp(double dewp, double temp) {
    return 100*(exp((17.625*dewp)/(243.04+dewp))/exp((17.625*temp)/(243.04+temp)));
}

double ToolsRhoEstimator::calculate_rho(double temp,
                                        double press,
                                        double dewp) {
  // Step 1: calculate the saturation water pressure at the dew point,
  // i.e., the vapor pressure in the air.  Use Herman Wobus' equation.
  double c0 = 0.99999683;
  double c1 = -0.90826951E-02;
  double c2 = 0.78736169E-04;
  double c3 = -0.61117958E-06;
  double c4 = 0.43884187E-08;
  double c5 = -0.29883885E-10;
  double c6 = 0.21874425E-12;
  double c7 = -0.17892321E-14;
  double c8 = 0.11112018E-16;
  double c9 = -0.30994571E-19;
  double p = c0 +
    dewp*(c1 +
          dewp*(c2 +
                dewp*(c3 +
                      dewp*(c4 +
                            dewp*(c5 +
                                  dewp*(c6 +
                                        dewp*(c7 +
                                              dewp*(c8 +
                                                    dewp*(c9)))))))));
  double psat_mbar = 6.1078 / std::pow(p, 8);

  // Step 2: calculate the vapor pressure.
  double pv_pascals = psat_mbar * 100.0;

  // Step 3: calculate the pressure of dry air, given the vapor
  // pressure and actual pressure.
  double pd_pascals = (press*100) - pv_pascals;

  // Step 4: calculate the air density, using the equation for
  // the density of a mixture of dry air and water vapor.
  double density =
    (pd_pascals / (287.0531 * (temp + 273.15))) +
    (pv_pascals / (461.4964 * (temp + 273.15)));

  return density;
}
