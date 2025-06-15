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
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QWizard>
#include <QWizardPage>

#include "StyledItemDelegates.h"

class Context;


extern QPixmap svgAsColoredPixmap(const QString &file, const QSize &size, const QColor &color);


class ManualActivityWizard : public QWizard
{
    Q_OBJECT

    public:
        enum {
            Finalize = -1,
            PageBasics,
            PageSpecifics
        };

        ManualActivityWizard(Context *context, QWidget *parent = nullptr);

    protected:
        virtual void done(int result) override;

    private:
        Context *context;
};


class ManualActivityPageBasics : public QWizardPage
{
    Q_OBJECT

    public:
        ManualActivityPageBasics(Context *context, QWidget *parent = nullptr);

        virtual void initializePage() override;
        virtual int nextId() const override;

    private slots:
        void updateVisibility();
        void sportsChanged();

    private:
        Context *context;

        QLabel *distanceLabel;
        QDoubleSpinBox *distanceEdit;
        QLabel *swimDistanceLabel;
        QSpinBox *swimDistanceEdit;
        QLabel *durationLabel;
        QTimeEdit *durationEdit;
        QCheckBox *paceIntervals;
        LapsEditorWidget *lapsEditor;
        QLabel *averageCadenceLabel;
        QSpinBox *averageCadenceEdit;
        QLabel *averagePowerLabel;
        QSpinBox *averagePowerEdit;
};


class ManualActivityPageSpecifics : public QWizardPage
{
    Q_OBJECT

    public:
        ManualActivityPageSpecifics(Context *context, QWidget *parent = nullptr);

        virtual void cleanupPage() override;
        virtual void initializePage() override;
        virtual int nextId() const override;

    private slots:
        void updateVisibility();
        void updateEstimates();

    private:
        Context *context;

        std::pair<double, double> getDurationDistance() const;

    private:
        QComboBox *estimateBy;
        QSpinBox *estimationDayEdit;
        QSpinBox *workEdit;
        QLabel *bikeStressLabel;
        QSpinBox *bikeStressEdit;
        QLabel *bikeScoreLabel;
        QSpinBox *bikeScoreEdit;
        QLabel *swimScoreLabel;
        QSpinBox *swimScoreEdit;
        QLabel *triScoreLabel;
        QSpinBox *triScoreEdit;
};

#endif // _GC_ManualRideDialog_h

