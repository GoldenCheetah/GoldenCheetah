/*
 * Copyright (c) 2012 Steven Gribble (gribble [at] gmail [dot] com)
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

#include "GoldenCheetah.h"

#include <QDateTime>
#include <QtGui>
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>

class Context;

// This class implements a dialog box containing a tool for
// helping the user estimate rho, the air density, given
// temperature, pressure, and dew point as inputs.
class ToolsRhoEstimator : public QDialog {
        Q_OBJECT
        G_OBJECT

    public:
        ToolsRhoEstimator(Context *context, QWidget *parent = 0);

    private:
        Context *context;

        bool useMetricUnits;
        QRadioButton *metBut;
        QRadioButton *impBut;
        QPushButton *btnOK;
        QLineEdit *txtRhoImp;
        QLineEdit *txtRhoMet;
        QLabel *tempLabel;
        QLabel *pressLabel;
        QLabel *dewpLabel;
        QLabel *rhumLabel;
        QDoubleSpinBox *tempSpinBox;
        QDoubleSpinBox *pressSpinBox;
        QDoubleSpinBox *dewpSpinBox;
        QDoubleSpinBox *rhumSpinBox;
        QGroupBox *rhumGroupBox;
        double fahrenheit_to_celsius(double f);
        double celsius_to_fahrenheit(double c);
        double hectopascals_to_inchesmercury(double hpa);
        double inchesmercury_to_hectopascals(double inhg);
        double rho_met_to_imp(double rho);
        double dewp_from_rhum(double rhum, double temp);
        double rhum_from_dewp(double dewp, double temp);
        double calculate_rho(double temp, double press, double dewp);

    private slots:
        void on_radio_toggled(bool checked);
        void on_btnOK_clicked();
        void on_valueChanged(double newval=0.0);
};
