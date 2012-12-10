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
#include "Pages.h"
#include "MainWindow.h"

class QListWidget;
class QListWidgetItem;
class QStackedWidget;
class MainWindow;
class Zones;

// GENERAL PAGE
class GeneralConfig : public QWidget
{
    Q_OBJECT

    public:
        GeneralConfig(QDir home, Zones *zones, MainWindow *mainWindow);

    public slots:
        void saveClicked();

    private:
        QDir home;
        Zones *zones;
        MainWindow *mainWindow;

        GeneralPage *generalPage;
};

// ATHLETE PAGE
class AthleteConfig : public QWidget
{
    Q_OBJECT

    public:
        AthleteConfig(QDir home, Zones *zones, MainWindow *mainWindow);

    public slots:
        void saveClicked();
    
    private:
        QDir home;
        Zones *zones;
        MainWindow *mainWindow;

        // about me, power ones and hr zones
        RiderPage *athletePage;
        ZonePage *zonePage;
        HrZonePage *hrZonePage;
};

// APPEARANCE PAGE
class AppearanceConfig : public QWidget
{
    Q_OBJECT

    public:
        AppearanceConfig(QDir home, Zones *zones, MainWindow *mainWindow);

    public slots:
        void saveClicked();
    
    private:
        QDir home;
        Zones *zones;
        MainWindow *mainWindow;

        ColorsPage *appearancePage;
};

// PASSWORDS PAGE
class PasswordConfig : public QWidget
{
    Q_OBJECT

    public:
        PasswordConfig(QDir home, Zones *zones, MainWindow *mainWindow);

    public slots:
        void saveClicked();
    
    private:
        QDir home;
        Zones *zones;
        MainWindow *mainWindow;

        CredentialsPage *passwordPage;
};

// METADATA PAGE
class DataConfig : public QWidget
{
    Q_OBJECT

    public:
        DataConfig(QDir home, Zones *zones, MainWindow *mainWindow);

    public slots:
        void saveClicked();
    
    private:
        QDir home;
        Zones *zones;
        MainWindow *mainWindow;

        MetadataPage *dataPage;
};

// METRIC PAGE
class MetricConfig : public QWidget
{
    Q_OBJECT

    public:
        MetricConfig(QDir home, Zones *zones, MainWindow *mainWindow);

    public slots:
        void saveClicked();
    
    private:
        QDir home;
        Zones *zones;
        MainWindow *mainWindow;

        IntervalMetricsPage *intervalsPage;
        SummaryMetricsPage *summaryPage;
};

// DEVICE PAGE
class DeviceConfig : public QWidget
{
    Q_OBJECT

    public:
        DeviceConfig(QDir home, Zones *zones, MainWindow *mainWindow);

    public slots:
        void saveClicked();
    
    private:
        QDir home;
        Zones *zones;
        MainWindow *mainWindow;

        DevicePage *devicePage;
};

class ConfigDialog : public QMainWindow
{
    Q_OBJECT
    G_OBJECT

    public:
        ConfigDialog(QDir home, Zones *zones, MainWindow *mainWindow);

    public slots:
        void changePage(int);
        void saveClicked();

    private:
        QSettings *settings;
        QDir home;
        Zones *zones;
        MainWindow *mainWindow;

        QStackedWidget *pagesWidget;
        QPushButton *saveButton;
	    QPushButton *closeButton;

        // the config pages
        GeneralConfig *general;
        AthleteConfig *athlete;
        AppearanceConfig *appearance;
        PasswordConfig *password;
        DataConfig *data;
        MetricConfig *metric;
        DeviceConfig *device;
};
#endif
