/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_UserMetricSettings_h
#define _GC_UserMetricSettings_h 1
#include "GoldenCheetah.h"

// Edit dialog
#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>

// defined in LTMTool
class Context;
class DataFilterEdit;

// what version of user metric structure are we using?
// Version      Date           Who               What
// 1            10 Dec 2015    Mark Liversedge   Initial version created
#define USER_METRICS_VERSION_NUMBER 1

// This structure is used to pass config back and forth
// and is "compiled" into a UserMetric at runtime
class UserMetricSettings {

    public:

        bool operator!= (const UserMetricSettings &right) const {

            // mostly used to see if it changed when editing
            if (this->symbol != right.symbol ||
                this->name != right.name ||
                this->description != right.description ||
                this->unitsMetric != right.unitsMetric ||
                this->unitsImperial != right.unitsImperial ||
                this->conversion != right.conversion ||
                this->conversionSum != right.conversionSum ||
                this->program != right.program ||
                this->precision != right.precision ||
                this->istime != right.istime ||
                this->aggzero != right.aggzero ||
                this->fingerprint != right.fingerprint)
                return true;
            else
                return false;
        }

        QString symbol,
                name,
                description,
                unitsMetric,
                unitsImperial;

        int type;
        int precision;

        bool aggzero, istime;

        double  conversion,
                conversionSum;

        QString program,
                fingerprint; // condensed form of program
};

class EditUserMetricDialog : public QDialog {

    Q_OBJECT

    public:

        EditUserMetricDialog(QWidget *parent, Context *context, UserMetricSettings &here);

    public slots:

        // refresh the outputs (time to compute, value for
        // the current ride, time to compute all rides)
        void refreshStats();
        void okClicked();

    private:

        void setSettings(UserMetricSettings &);

        Context *context;
        UserMetricSettings &settings;

        QLineEdit *symbol,
                  *name,
                  *unitsMetric,
                  *unitsImperial;

        QComboBox *type;
        QTextEdit *description;

        QCheckBox *istime, *aggzero;

        QDoubleSpinBox *conversion,
                       *conversionSum,
                       *precision;

        DataFilterEdit *formulaEdit; // edit your formula

        QLabel *mValue, *iValue, *elapsed;

        QPushButton *test, *okButton, *cancelButton;

};
#endif
