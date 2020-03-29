/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H
#include "GoldenCheetah.h"

#include <QDialog>
#include <QSettings>
#include <QMainWindow>
#include <QStackedWidget>

#include "Pages.h"
#include "Context.h"


class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class Context;

// GENERAL PAGE
class GeneralConfig : public QWidget
{
    Q_OBJECT

    public:
        GeneralConfig(QDir home, Context *context);

        GeneralPage *generalPage;

    public slots:
        qint32 saveClicked();

    private:
        QDir home;
        Context *context;

};

// ATHLETE PAGE
class AthleteConfig : public QWidget
{
    Q_OBJECT

    public:
        AthleteConfig(QDir home, Context *context);
        AboutRiderPage *athletePage;
        AboutModelPage *modelPage;
        CredentialsPage *credentialsPage;
        RiderPhysPage *athletePhysPage;
        HrvPage *hrvPage;

    public slots:
        qint32 saveClicked();
    
    private:
        QDir home;
        Context *context;

        // about me, power ones and hr zones
        ZonePage *zonePage;
        HrZonePage *hrZonePage;
        PaceZonePage *paceZonePage;
        AutoImportPage *autoImportPage;
        BackupPage *backupPage;
};

// APPEARANCE PAGE
class AppearanceConfig : public QWidget
{
    Q_OBJECT

    public:
        AppearanceConfig(QDir home, Context *context);

    public slots:
        qint32 saveClicked();
    
    private:
        QDir home;
        Context *context;

        ColorsPage *appearancePage;
};

// METADATA PAGE
class DataConfig : public QWidget
{
    Q_OBJECT

    public:
        DataConfig(QDir home, Context *context);

    public slots:
        qint32 saveClicked();
    
    private:
        QDir home;
        Context *context;

        MetadataPage *dataPage;
};

// METRIC PAGE
class MetricConfig : public QWidget
{
    Q_OBJECT

    public:
        MetricConfig(QDir home, Context *context);

    public slots:
        qint32 saveClicked();
    
    private:
        QDir home;
        Context *context;

        BestsMetricsPage *bestsPage;
        IntervalMetricsPage *intervalsPage;
        SummaryMetricsPage *summaryPage;
        CustomMetricsPage *customPage;
};

// TRAIN PAGE
class TrainConfig : public QWidget
{
    Q_OBJECT

    public:
        TrainConfig(QDir home, Context *context);

    public slots:
        qint32 saveClicked();
    
    private:
        QDir home;
        Context *context;

        DevicePage *devicePage;
        TrainOptionsPage *optionsPage;
        RemotePage *remotePage;
        SimBicyclePage *simBicyclePage;
};

// INTERVAL PAGE
class IntervalConfig : public QWidget
{
    Q_OBJECT

    public:
        IntervalConfig(QDir home, Context *context);

    public slots:
        qint32 saveClicked();
    
    private:
        QDir home;
        Context *context;

        IntervalsPage *intervalsPage;
};

class ConfigDialog : public QMainWindow
{
    Q_OBJECT
    G_OBJECT

    public:
        ConfigDialog(QDir home, Context *context);

    public slots:
        void changePage(int);
        void saveClicked(); // collects state
        void closeClicked();

    private:
        QSettings *settings;
        QDir home;
        Context *context;

        QStackedWidget *pagesWidget;
        QPushButton *saveButton;
	    QPushButton *closeButton;

        // the config pages
        GeneralConfig *general;
        AthleteConfig *athlete;
        AppearanceConfig *appearance;
        DataConfig *data;
        MetricConfig *metric;
        IntervalConfig *interval;
        TrainConfig *train;
};
#endif
