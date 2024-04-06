/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _NewCyclistDialog_h
#define _NewCyclistDialog_h

#include "GoldenCheetah.h"
#include "Context.h"
#include "Units.h"
#include "Settings.h"

#include <QtGui>
#include <QLineEdit>
#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

class NewCyclistDialog : public QDialog
{
    Q_OBJECT

    public:
        NewCyclistDialog(QDir);

        QDir home;
        QLineEdit *name;

    public slots:
        void chooseAvatar();
        void unitChanged(int);
        void saveClicked();
        void cancelClicked();

    private:
        Context *context;
        bool useMetricUnits;
        QDateEdit *dob;
        QComboBox *sex;
        QLabel *weightlabel;
        QLabel *heightlabel;
        QLabel *cvRnlabel, *cvSwlabel;
        QComboBox *unitCombo;
        QSpinBox *cp, *ftp, *w, *pmax, *lthr, *resthr, *maxhr; // mandatory non-zero, default from age
        QTimeEdit *cvRn, *cvSw;
        QDoubleSpinBox *weight;
        QDoubleSpinBox *height;
        QSpinBox *wbaltau;
        QTextEdit  *bio;
        QPushButton *avatarButton;
        QPixmap     avatar;
        QLabel *templatelabel;
        QComboBox *templateCombo;

        QPushButton *cancel, *save;
};

#endif
