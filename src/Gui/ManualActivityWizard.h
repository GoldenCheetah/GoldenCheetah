/*
 * Copyright (c) 2009 Eric Murray (ericm@lne.com)
 *               2012 Mark Liversedge (liversedge@gmail.com)
 * Copyright (c) 2025 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _GC_ManualActivityWizard_h
#define _GC_ManualActivityWizard_h 1
#include "GoldenCheetah.h"
#include "LapsEditor.h"

#include <QtGui>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QWizard>
#include <QWizardPage>

#include "StyledItemDelegates.h"
#include "MultiFilterProxyModel.h"
#include "WorkoutFilterBox.h"
#include "ErgFile.h"
#include "ErgFilePlot.h"
#include "InfoWidget.h"

class Context;


class ManualActivityWizard : public QWizard
{
    Q_OBJECT

    public:
        enum {
            Finalize = -1,
            PageBasics,
            PageWorkout,
            PageMetrics,
            PageSummary,
            PageBusyRejection
        };

        ManualActivityWizard(Context *context, bool plan = false, QWidget *parent = nullptr);

    protected:
        virtual void done(int result) override;

    private:
        Context *context;
        bool plan = false;

        void field2MetricDouble(RideFile &rideFile, const QString &fieldName, const QString &metricName) const;
        void field2MetricInt(RideFile &rideFile, const QString &fieldName, const QString &metricName) const;
        void field2TagString(RideFile &rideFile, const QString &fieldName, const QString &tagName) const;
        void field2TagInt(RideFile &rideFile, const QString &fieldName, const QString &tagName) const;
};


class ManualActivityPageBasics : public QWizardPage
{
    Q_OBJECT

    public:
        ManualActivityPageBasics(Context *context, bool plan = false, QWidget *parent = nullptr);

        virtual void initializePage() override;
        virtual int nextId() const override;
        virtual bool isComplete() const override;

    private slots:
        void checkDateTime();
        void sportsChanged();

    private:
        Context *context;
        bool plan = false;
        QLabel *duplicateActivityLabel;
};


class ManualActivityPageWorkout : public QWizardPage
{
    Q_OBJECT

    public:
        ManualActivityPageWorkout(Context *context, QWidget *parent = nullptr);
        virtual ~ManualActivityPageWorkout();

        virtual bool eventFilter(QObject *watched, QEvent *event) override;
        virtual void cleanupPage() override;
        virtual void initializePage() override;
        virtual int nextId() const override;

    private:
        Context *context = nullptr;
        ErgFile *ergFile = nullptr;
        QAbstractTableModel *workoutModel = nullptr;
        MultiFilterProxyModel *sortModel = nullptr;
        WorkoutFilterBox *workoutFilterBox;
        QTreeView *workoutTree;
        QStackedWidget *contentStack;
        ErgFilePlot *ergFilePlot;
        InfoWidget *infoWidget;
        ErgFile *backupWorkout = nullptr;

        void resetFields();

    private slots:
        void selectionChanged();
};


class ManualActivityPageMetrics : public QWizardPage
{
    Q_OBJECT

    public:
        ManualActivityPageMetrics(Context *context, bool plan = false, QWidget *parent = nullptr);

        virtual void cleanupPage() override;
        virtual void initializePage() override;
        virtual int nextId() const override;

    private slots:
        void updateVisibility();
        void updateEstimates();

    private:
        Context *context;
        bool plan = false;

        std::pair<double, double> getDurationDistance() const;

    private:
        QLabel *distanceLabel;
        QDoubleSpinBox *distanceEdit;
        QLabel *swimDistanceLabel;
        QSpinBox *swimDistanceEdit;
        QLabel *durationLabel;
        QTimeEdit *durationEdit;
        QCheckBox *paceIntervals;
        LapsEditorWidget *lapsEditor;
        QLabel *stressHL;
        QLabel *averageCadenceLabel;
        QSpinBox *averageCadenceEdit;
        QLabel *averagePowerLabel;
        QSpinBox *averagePowerEdit;
        QLabel *estimateByLabel;
        QComboBox *estimateByEdit;
        QLabel *estimationDaysLabel;
        QSpinBox *estimationDaysEdit;
        QLabel *workLabel;
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


class ManualActivityPageSummary : public QWizardPage
{
    Q_OBJECT

    public:
        ManualActivityPageSummary(bool plan = false, QWidget *parent = nullptr);

        virtual void cleanupPage() override;
        virtual void initializePage() override;
        virtual int nextId() const override;

    private:
        bool plan = false;
        QFormLayout *form = nullptr;

        bool addRowString(const QString &label, const QString &fieldName);
        bool addRowInt(const QString &label, const QString &fieldName, const QString &unit = QString(), double metricFactor = 1.0);
        bool addRowDouble(const QString &label, const QString &fieldName, const QString &unit = QString(), double metricFactor = 1.0);
        bool addRow(const QString &label, const QString &value);
};

#endif // _GC_ManualActivityWizard_h
