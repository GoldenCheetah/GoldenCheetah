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

#ifndef _SplitActivityWizard_h
#define _SplitActivityWizard_h

#include "GoldenCheetah.h"
#include "MainWindow.h"
#include "RideItem.h"
#include "RideFile.h"
#include "JsonRideFile.h"
#include "Units.h"
#include <QWizard>
#include <QDoubleSpinBox>

class SplitActivityWizard : public QWizard
{
    Q_OBJECT

public:
    SplitActivityWizard(MainWindow *main);

    MainWindow *main;
    bool keepOriginal;
    RideItem *rideItem;

    int minimumGap,
        minimumSegmentSize;

    QTreeWidget *intervals;
    QTreeWidget *files; // what we will do!
    int usedMinimumGap,
        usedMinimumSegmentSize;

    QList<RideFile*> activities;

    bool done; // have we finished?

public slots:
    // ride file utilities
    QString hasBackup(QString filename);
    QStringList conflicts(QDateTime datetime);
    void setIntervalsList();
    void setFilesList();

signals:

private slots:

};

class SplitWelcome : public QWizardPage
{
    Q_OBJECT

    public:
        SplitWelcome(SplitActivityWizard *);

    private:
        SplitActivityWizard *wizard;
};

class SplitKeep : public QWizardPage
{
    Q_OBJECT

    public:
        SplitKeep(SplitActivityWizard *);
        QLabel *warning;

    public slots:
        void keepOriginalChanged();
        void setWarning();

    private:
        SplitActivityWizard *wizard;

        QCheckBox *keepOriginal;
};

class SplitParameters : public QWizardPage
{
    Q_OBJECT

    public:
        SplitParameters(SplitActivityWizard *);

    public slots:
        void valueChanged();

    private:
        SplitActivityWizard *wizard;

        QDoubleSpinBox *minimumGap,
                    *minimumSegmentSize;
};

class SplitSelect : public QWizardPage
{
    Q_OBJECT

    public:
        SplitSelect(SplitActivityWizard *);
        void initializePage(); // reset the interval list if needed
    private:
        SplitActivityWizard *wizard;
};

class SplitConfirm : public QWizardPage
{
    Q_OBJECT

    public:
        SplitConfirm(SplitActivityWizard *);
        bool isCommitPage() const { return true; }
        bool validatePage();
        void initializePage(); // create ridefile * for each selected
        bool isComplete() const;

        // constructs the new ridefile from current ride
        RideFile* createRideFile(int start, int stop);
    private:
        SplitActivityWizard *wizard;
};

#endif // _SplitActivityWizard_h
