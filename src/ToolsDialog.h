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
#include "GoldenCheetah.h"

#include <QDateTime>
#include <QtGui>

class ToolsDialog : public QDialog
{
        Q_OBJECT
        G_OBJECT


    public:
        ToolsDialog(QWidget *parent = 0);

    private:
        QPushButton *btnCalculate;
        QPushButton *btnOK;
        QLineEdit *txtCP;
        QLineEdit *txtWP;
        QDoubleSpinBox *shortMinsSpinBox;
        QDoubleSpinBox *shortSecsSpinBox;
        QDoubleSpinBox *shortWattsSpinBox;
        QDoubleSpinBox *longMinsSpinBox;
        QDoubleSpinBox *longSecsSpinBox;
        QDoubleSpinBox *longWattsSpinBox;

    private slots:
        void on_btnOK_clicked();
        void on_btnCalculate_clicked();
};

