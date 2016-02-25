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

#ifndef _GC_LapsEditor_h
#define _GC_LapsEditor_h 1
#include "GoldenCheetah.h"
#include "RideFile.h"

#include <QtGui>
#include <QLineEdit>
#include <QDialog>
#include <QLabel>
#include <QMessageBox>
#include <QTableWidget>

class LapsEditor : public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        LapsEditor(bool isSwim);
        ~LapsEditor();
        const QVector<RideFilePoint*> &dataPoints() const { return dataPoints_; }

    private slots:
        void okClicked();
        void cancelClicked();

    private:

        bool isSwim; // Swimming indicator, used for distance units selection
        QVector<RideFilePoint*> dataPoints_; // samples generated from laps

        QPushButton *okButton, *cancelButton;

        QTableWidget *tableWidget;
};

#endif // _GC_LapsEditor_h
