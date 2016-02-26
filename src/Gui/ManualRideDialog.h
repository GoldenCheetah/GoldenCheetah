/*
 * Copyright (c) 2009 Eric Murray (ericm@lne.com)
 *               2012 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_ManualRideDialog_h
#define _GC_ManualRideDialog_h 1
#include "GoldenCheetah.h"
#include "LapsEditor.h"

#include <QtGui>
#include <QLineEdit>
#include <QTextEdit>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>

class Context;

class ManualRideDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        ManualRideDialog(Context *context);
        ~ManualRideDialog();

    private slots:
        void okClicked();
        void cancelClicked();

        void estimate();          // estimate TSS et al when method/duration/distance changes
        void deriveFactors();        // calculate factors to use for estimate

        void sportChanged(const QString& text); // enable/disable lapsButton
        void lapsClicked(); // starts laps editor

    private:

        Context *context;
        QVector<unsigned char> records;
        QString filename, filepath;
        int daysago; // remember last deriveFactors value

        // factors for estimator
        double timeBS, distanceBS,  // Bikescore (use same for TSS)
               timeDP, distanceDP,  // Daniel Points
               timeTSS, distanceTSS,  // Coggan TSS
               timeKJ, distanceKJ;  // Work

        LapsEditor *lapsEditor; // Laps Editor Dialog

        QPushButton *okButton, *cancelButton, *lapsButton;

        QDateEdit *dateEdit;  // start date
        QTimeEdit *timeEdit;  // start time

        QDoubleSpinBox *days; // how many days to estimate?

        QLineEdit *wcode;     // workout code
        QLineEdit *sport;     // sport
        QTextEdit *notes;     // capture some notes at least

        QTimeEdit *duration;  // ride duration as a time edit
        QDoubleSpinBox *distance, // ride distance
                       *avgBPM,   // heartrate
                       *avgKPH,   // speed
                       *avgRPM,   // cadence
                       *avgWatts; // power

        QRadioButton *byDistance, *byDuration, *byManual;

        QDoubleSpinBox *BS,       // skiba
                       *DP,       // rhea
                       *TSS,      // coggan
                       *KJ;       // work

};

#endif // _GC_ManualRideDialog_h

