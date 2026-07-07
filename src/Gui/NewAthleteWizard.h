/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2024 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _NewAthleteWizard_h
#define _NewAthleteWizard_h

#include "GoldenCheetah.h"
#include "Units.h"
#include "Settings.h"

#include <QtGui>
#include <QLineEdit>
#include <QTextEdit>
#include <QWizard>


class NewAthleteWizard : public QWizard
{
    Q_OBJECT

    public:
        enum {
            Finalize = -1,
            PageUser,
            PagePerformance
        };

        NewAthleteWizard(const QDir &home, QWidget *parent = nullptr);

        QString getName() const;

    protected:
        virtual void done(int result) override;

    private:
        QDir home;
};

class NewAthletePageUser : public QWizardPage
{
    Q_OBJECT

    public:
        NewAthletePageUser(const QDir &home, QWidget *parent = nullptr);

        virtual int nextId() const override;
        virtual bool isComplete() const override;

    private slots:
        void chooseAvatar();
        void nameChanged();
        void unitChanged(int idx);

    private:
        QPushButton *avatarButton;
        QLineEdit *name;
        QDateEdit *dob;
        QDoubleSpinBox *weight;
        QDoubleSpinBox *height;
        QComboBox *sex;
        QComboBox *unitCombo;
        QTextEdit *bio;

        QDir home;
        bool allowNext;

        bool isNameValid() const;
};

class NewAthletePagePerformance : public QWizardPage
{
    Q_OBJECT

    public:
        NewAthletePagePerformance(QWidget *parent = nullptr);

        virtual void initializePage() override;
        virtual int nextId() const override;

    private:
        bool useMetricUnits = true;
        QSpinBox *cp, *ftp, *w, *pmax, *lthr, *resthr, *maxhr; // mandatory non-zero, default from age
        QTimeEdit *cvRn, *cvSw;
        QSpinBox *wbaltau;
        QComboBox *templateCombo;
};

#endif
