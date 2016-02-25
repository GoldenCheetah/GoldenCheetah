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
 */
#include "GoldenCheetah.h"

#include <QDateTime>
#include <QtGui>
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>

class VDOTCalculator : public QDialog
{
        Q_OBJECT
        G_OBJECT


    public:
        VDOTCalculator(QWidget *parent = 0);
        static double vdot(double mins, double vel);
        static double vVdot(double VDOT);
        static double eqvTime(double VDOT, double dist);

    private:
        QPushButton *btnCalculate;
        QPushButton *btnOK;
        QLabel *labelVDOT;
        QLineEdit *txtVDOT;
        QLabel *labelTPACE;
        QTableWidget *tableWidgetTPACE;
        QDoubleSpinBox *distSpinBox;
        QDoubleSpinBox *hoursSpinBox;
        QDoubleSpinBox *minsSpinBox;
        QDoubleSpinBox *secsSpinBox;
        QDoubleSpinBox *targetSpinBox;
        QLabel *labelEQV;
        QLineEdit *txtEQV;

    private slots:
        void on_btnOK_clicked();
        void on_btnCalculate_clicked();
};
