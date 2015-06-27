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

#include <QtGui>
#include <QSettings>

#include "Context.h"
#include "Context.h"
#include "Athlete.h"
#include "ConfigDialog.h"
#include "RideCache.h"
#include "Pages.h"
#include "Settings.h"
#include "Zones.h"
#include "HelpWhatsThis.h"

#include "AddDeviceWizard.h"
#include "MainWindow.h"

extern bool restarting; //its actually in main.cpp

ConfigDialog::ConfigDialog(QDir _home, Zones *_zones, Context *context) :
    home(_home), zones(_zones), context(context)
{
    setAttribute(Qt::WA_DeleteOnClose);

#ifdef Q_OS_MAC
    QToolBar *head = addToolBar(tr("Preferences"));
    setMinimumSize(600,540);
    setUnifiedTitleAndToolBarOnMac(true);
    head->setFloatable(false);
    head->setMovable(false);
#else
    QToolBar *head = addToolBar(tr("Options"));
    head->setMovable(false); // oops!

    QFont defaultFont;
    setMinimumSize(60 * defaultFont.pointSize(),580);   //Change for 53 to 60 - To be decided if also Size for Q_OS_MAC need change
#endif

    // center
    QWidget *spacer = new QWidget(this);
    spacer->setAutoFillBackground(false);
    spacer->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    head->addWidget(spacer);

    // icons
    static QIcon generalIcon(QPixmap(":images/toolbar/GeneralPreferences.png"));
    static QIcon athleteIcon(QPixmap(":/images/toolbar/user.png"));
    static QIcon passwordIcon(QPixmap(":/images/toolbar/passwords.png"));
    static QIcon appearanceIcon(QPixmap(":/images/toolbar/color.png"));
    static QIcon dataIcon(QPixmap(":/images/toolbar/data.png"));
    static QIcon metricsIcon(QPixmap(":/images/toolbar/abacus.png"));
    static QIcon intervalIcon(QPixmap(":/images/stopwatch.png"));
    static QIcon devicesIcon(QPixmap(":/images/devices/kickr.png"));

    // Setup the signal mapping so the right config
    // widget is displayed when the icon is clicked
    QSignalMapper *iconMapper = new QSignalMapper(this); // maps each option
    connect(iconMapper, SIGNAL(mapped(int)), this, SLOT(changePage(int)));
    head->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    QAction *added;

    // General settings
    added = head->addAction(generalIcon, tr("General"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 0);

    added =head->addAction(athleteIcon, tr("Athlete"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 1);

    added =head->addAction(passwordIcon, tr("Passwords"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 2);

    added =head->addAction(appearanceIcon, tr("Appearance"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 3);

    added =head->addAction(dataIcon, tr("Data Fields"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 4);

    added =head->addAction(metricsIcon, tr("Metrics"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 5);

    added =head->addAction(intervalIcon, tr("Intervals"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 6);


    added =head->addAction(devicesIcon, tr("Train Devices"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 7);

    // more space
    spacer = new QWidget(this);
    spacer->setAutoFillBackground(false);
    spacer->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    head->addWidget(spacer);


    pagesWidget = new QStackedWidget(this);

    // create those config pages
    general = new GeneralConfig(_home, _zones, context);
    HelpWhatsThis *generalHelp = new HelpWhatsThis(general);
    general->setWhatsThis(generalHelp->getWhatsThisText(HelpWhatsThis::Preferences_General));
    pagesWidget->addWidget(general);

    athlete = new AthleteConfig(_home, _zones, context);
    HelpWhatsThis *athleteHelp = new HelpWhatsThis(athlete);
    athlete->setWhatsThis(athleteHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_About));
    pagesWidget->addWidget(athlete);

    password = new PasswordConfig(_home, _zones, context);
    HelpWhatsThis *passwordHelp = new HelpWhatsThis(password);
    password->setWhatsThis(passwordHelp->getWhatsThisText(HelpWhatsThis::Preferences_Passwords));
    pagesWidget->addWidget(password);

    appearance = new AppearanceConfig(_home, _zones, context);
    HelpWhatsThis *appearanceHelp = new HelpWhatsThis(appearance);
    appearance->setWhatsThis(appearanceHelp->getWhatsThisText(HelpWhatsThis::Preferences_Appearance));
    pagesWidget->addWidget(appearance);

    data = new DataConfig(_home, _zones, context);
    HelpWhatsThis *dataHelp = new HelpWhatsThis(data);
    data->setWhatsThis(dataHelp->getWhatsThisText(HelpWhatsThis::Preferences_DataFields));
    pagesWidget->addWidget(data);

    metric = new MetricConfig(_home, _zones, context);
    HelpWhatsThis *metricHelp = new HelpWhatsThis(metric);
    metric->setWhatsThis(metricHelp->getWhatsThisText(HelpWhatsThis::Preferences_Metrics));
    pagesWidget->addWidget(metric);

    interval = new IntervalConfig(_home, _zones, context);
    //HelpWhatsThis *intervalHelp = new HelpWhatsThis(interval);
    //interval->setWhatsThis(intervalHelp->getWhatsThisText(HelpWhatsThis::Preferences_Intervals));
    pagesWidget->addWidget(interval);

    device = new DeviceConfig(_home, _zones, context);
    HelpWhatsThis *deviceHelp = new HelpWhatsThis(device);
    device->setWhatsThis(deviceHelp->getWhatsThisText(HelpWhatsThis::Preferences_TrainDevices));
    pagesWidget->addWidget(device);

    closeButton = new QPushButton(tr("Close"));
    saveButton = new QPushButton(tr("Save"));

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->setSpacing(5);
    buttonsLayout->addWidget(closeButton);
    buttonsLayout->addWidget(saveButton);

    QWidget *contents = new QWidget(this);
    setCentralWidget(contents);
    contents->setContentsMargins(0,0,0,0);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout);
    mainLayout->addLayout(buttonsLayout);
    mainLayout->setSpacing(0);
    contents->setLayout(mainLayout);

    // We go fixed width to ensure a consistent layout for
    // tabs, sub-tabs and internal widgets and lists
#ifdef Q_OS_MACX
    setWindowTitle(tr("Preferences"));
#else
    setWindowTitle(tr("Options"));
#endif

    connect(closeButton, SIGNAL(clicked()), this, SLOT(closeClicked()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveClicked()));
}

void ConfigDialog::changePage(int index)
{
    pagesWidget->setCurrentIndex(index);
}

void ConfigDialog::closeClicked()
{
    // don't save!
    close();
}
// if save is clicked, we want to:
//   new mode:   create a new zone starting at the selected date (which may be null, implying BEGIN
//   ! new mode: change the CP associated with the present mode
void ConfigDialog::saveClicked()
{
    // if a refresh is happenning stop it, whilst we 
    // update all the configuration settings!
    context->athlete->rideCache->cancel();

    // what changed ?
    qint32 changed = 0;

    changed |= general->saveClicked();
    changed |= athlete->saveClicked();
    changed |= appearance->saveClicked();
    changed |= password->saveClicked();
    changed |= metric->saveClicked();
    changed |= data->saveClicked();
    changed |= device->saveClicked();
    changed |= interval->saveClicked();

    hide();

    // did the home directory change?
    QString shome = appsettings->value(this, GC_HOMEDIR).toString();
    if (shome != general->generalPage->athleteWAS || QFileInfo(shome).absoluteFilePath() != QFileInfo(home.absolutePath()).absolutePath()) {

        // are you sure you want to change the location of the athlete library?
        // if so we will restart, if not I'll revert to current directory
        QMessageBox msgBox;
        msgBox.setText(tr("You changed the location of the athlete library"));
        msgBox.setInformativeText(tr("This is where all new athletes and their files "
                                  "will now be stored.\n\nCurrent athlete data will no longer be "
                                  "available and GoldenCheetah will need to restart for the change to take effect."
                                  "\n\nDo you want to apply and restart GoldenCheetah?"));

        // we want our own buttons...
        msgBox.addButton(tr("No, Keep current"), QMessageBox::RejectRole);
        msgBox.addButton(tr("Yes, Apply and Restart"), QMessageBox::AcceptRole);
        msgBox.setDefaultButton(QMessageBox::Abort);

        if (msgBox.exec() == 1) { // accept!

            // lets restart
            restarting = true;

            // close all the mainwindows
            foreach(MainWindow *m, mainwindows) m->byebye();

            // NOTE: we don't notifyConfigChanged() now because the context
            //       has been zapped along with the windows we need to get out
            //       as quickly as possible.
            close();
            return; 

        } else {

            // revert to current home and let everyone know
            appsettings->setValue(GC_HOMEDIR, QFileInfo(home.absolutePath()).absolutePath());
        }

    } 

    // we're done.
    context->notifyConfigChanged(changed);
    close();
}

// GENERAL CONFIG
GeneralConfig::GeneralConfig(QDir home, Zones *zones, Context *context) :
    home(home), zones(zones), context(context)
{
    generalPage = new GeneralPage(context);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(generalPage);

    layout->setSpacing(0);
    setContentsMargins(0,0,0,0);
}

qint32 GeneralConfig::saveClicked()
{
    return generalPage->saveClicked();
}

// ATHLETE CONFIG
AthleteConfig::AthleteConfig(QDir home, Zones *zones, Context *context) :
    home(home), zones(zones), context(context)
{
    // the widgets
    athletePage = new RiderPage(this, context);
    HelpWhatsThis *athleteHelp = new HelpWhatsThis(athletePage);
    athletePage->setWhatsThis(athleteHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_About));

    zonePage = new ZonePage(context);
    HelpWhatsThis *zoneHelp = new HelpWhatsThis(zonePage);
    zonePage->setWhatsThis(zoneHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_TrainingZones_Power));

    hrZonePage = new HrZonePage(context);
    HelpWhatsThis *hrZoneHelp = new HelpWhatsThis(hrZonePage);
    hrZonePage->setWhatsThis(hrZoneHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_TrainingZones_HR));

    paceZonePage = new PaceZonePage(context);
    HelpWhatsThis *paceZoneHelp = new HelpWhatsThis(paceZonePage);
    paceZonePage->setWhatsThis(paceZoneHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_TrainingZones_Pace));

    autoImportPage = new AutoImportPage(context);
    HelpWhatsThis *autoImportHelp = new HelpWhatsThis(autoImportPage);
    autoImportPage->setWhatsThis(autoImportHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_Autoimport));

    setContentsMargins(0,0,0,0);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    QTabWidget *tabs = new QTabWidget(this);
    tabs->addTab(athletePage, tr("About"));
    tabs->addTab(zonePage, tr("Power Zones"));
    tabs->addTab(hrZonePage, tr("Heartrate Zones"));
    tabs->addTab(paceZonePage, tr("Pace Zones"));
    tabs->addTab(autoImportPage, tr("Auto Import"));

    mainLayout->addWidget(tabs);
}

qint32 AthleteConfig::saveClicked()
{
    qint32 state = 0;

    state |= athletePage->saveClicked();
    state |= zonePage->saveClicked();
    state |= hrZonePage->saveClicked();
    state |= paceZonePage->saveClicked();
    state |= autoImportPage->saveClicked();

    return state;
}

// APPEARANCE CONFIG
AppearanceConfig::AppearanceConfig(QDir home, Zones *zones, Context *context) :
    home(home), zones(zones), context(context)
{
    appearancePage = new ColorsPage(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(appearancePage);

    layout->setSpacing(0);
    setContentsMargins(0,0,0,0);
}

qint32 AppearanceConfig::saveClicked()
{
    return appearancePage->saveClicked();
}

// PASSWORD CONFIG
PasswordConfig::PasswordConfig(QDir home, Zones *zones, Context *context) :
    home(home), zones(zones), context(context)
{
    passwordPage = new CredentialsPage(this, context);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(passwordPage);

    layout->setSpacing(0);
    setContentsMargins(0,0,0,0);
}

qint32 PasswordConfig::saveClicked()
{
    return passwordPage->saveClicked();
}

// METADATA CONFIG
DataConfig::DataConfig(QDir home, Zones *zones, Context *context) :
    home(home), zones(zones), context(context)
{
    dataPage = new MetadataPage(context);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(dataPage);

    layout->setSpacing(0);
    setContentsMargins(0,0,0,0);
}

qint32 DataConfig::saveClicked()
{
    return dataPage->saveClicked();
}

// GENERAL CONFIG
MetricConfig::MetricConfig(QDir home, Zones *zones, Context *context) :
    home(home), zones(zones), context(context)
{
    // the widgets
    bestsPage = new BestsMetricsPage(this);
    intervalsPage = new IntervalMetricsPage(this);
    summaryPage = new SummaryMetricsPage(this);

    setContentsMargins(0,0,0,0);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    QTabWidget *tabs = new QTabWidget(this);
    tabs->addTab(bestsPage, tr("Bests"));
    tabs->addTab(summaryPage, tr("Summary"));
    tabs->addTab(intervalsPage, tr("Intervals"));

    mainLayout->addWidget(tabs);
}

qint32 MetricConfig::saveClicked()
{
    qint32 state = 0;

    state |= bestsPage->saveClicked();
    state |= summaryPage->saveClicked();
    state |= intervalsPage->saveClicked();

    return state;
}

IntervalConfig::IntervalConfig(QDir home, Zones *zones, Context *context) :
    home(home), zones(zones), context(context)
{
    // the widgets
    intervalsPage = new IntervalsPage(context);

    setContentsMargins(0,0,0,0);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    mainLayout->addWidget(intervalsPage);
    mainLayout->addStretch();
}

qint32 IntervalConfig::saveClicked()
{
    qint32 state = 0;

    state |= intervalsPage->saveClicked();

    return state;
}

// GENERAL CONFIG
DeviceConfig::DeviceConfig(QDir home, Zones *zones, Context *context) :
    home(home), zones(zones), context(context)
{
    devicePage = new DevicePage(this, context);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(devicePage);

    layout->setSpacing(0);
    setContentsMargins(0,0,0,0);
}

qint32 DeviceConfig::saveClicked()
{
    return devicePage->saveClicked();
}
