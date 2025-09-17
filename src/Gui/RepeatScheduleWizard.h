/*
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

#ifndef _GC_RepeatScheduleWizard_h
#define _GC_RepeatScheduleWizard_h 1

#include "GoldenCheetah.h"

#include <QtGui>
#include <QWizard>
#include <QWizardPage>
#include <QLabel>
#include <QTreeWidget>


class RepeatScheduleWizard : public QWizard
{
    Q_OBJECT

    public:
        enum {
            Finalize = -1,
            PageSetup,
            PageActivities,
            PageSummary
        };

        RepeatScheduleWizard(Context *context, const QDate &when, QWidget *parent = nullptr);

    protected:
        virtual void done(int result) override;

    private:
        Context *context;
        QDate when;
};


class RepeatSchedulePageSetup : public QWizardPage
{
    Q_OBJECT

    public:
        RepeatSchedulePageSetup(Context *context, const QDate &when, QWidget *parent = nullptr);

        int nextId() const override;

    private:
        Context *context;
};


class RepeatSchedulePageActivities : public QWizardPage
{
    Q_OBJECT

    public:
        RepeatSchedulePageActivities(Context *context, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool isComplete() const override;

        QList<RideItem*> getSelectedRideItems() const;

    private:
        Context *context;
        QTreeWidget *activityTree;
        int numSelected = 0;
};


class RepeatSchedulePageSummary : public QWizardPage
{
    Q_OBJECT

    public:
        RepeatSchedulePageSummary(Context *context, const QDate &when, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool isComplete() const override;

        QList<RideItem*> getDeletionList() const;
        QList<std::pair<RideItem*, QDate>> getScheduleList() const;

    private:
        Context *context;
        QDate when;

        bool failed = false;
        QList<RideItem*> deletionList;  // was: preexistingPlanned
        QList<std::pair<RideItem*, QDate>> scheduleList;  // was: targetMap

        QLabel *failedLabel;
        QLabel *scheduleLabel;
        QTreeWidget *scheduleTree;
        QLabel *deletionLabel;
        QTreeWidget *deletionTree;
};

#endif // _GC_ManualActivityWizard_h
