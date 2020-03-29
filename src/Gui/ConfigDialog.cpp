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
#include "Athlete.h"
#include "ConfigDialog.h"
#include "RideCache.h"
#include "Pages.h"
#include "Settings.h"
#include "HelpWhatsThis.h"

#include "AddDeviceWizard.h"
#include "MainWindow.h"

extern bool restarting; //its actually in main.cpp
ConfigDialog *configdialog_ptr = NULL;

ConfigDialog::ConfigDialog(QDir _home, Context *context) :
    home(_home), context(context)
{
    setAttribute(Qt::WA_DeleteOnClose);
    configdialog_ptr = this;

#ifdef Q_OS_MAC
    QToolBar *head = addToolBar(tr("Preferences"));
    setMinimumSize(650,540);
    setUnifiedTitleAndToolBarOnMac(true);
    head->setFloatable(false);
    head->setMovable(false);
#else
    QToolBar *head = addToolBar(tr("Options"));
    head->setMovable(false); // oops!

    setMinimumSize(800 *dpiXFactor,650 *dpiYFactor);   //changed for hidpi sizing
#endif

    // center
    QWidget *spacer = new QWidget(this);
    spacer->setAutoFillBackground(false);
    spacer->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    head->addWidget(spacer);

    // icons
    static QIcon generalIcon(QPixmap(":images/toolbar/GeneralPreferences.png"));
    static QIcon athleteIcon(QPixmap(":/images/toolbar/user.png"));
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
    head->setIconSize(QSize(32*dpiXFactor,32*dpiXFactor)); // use XFactor for both to ensure aspect ratio maintained

    QAction *added;

    // General settings
    added = head->addAction(generalIcon, tr("General"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 0);

    added =head->addAction(athleteIcon, tr("Athlete"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 1);

    added =head->addAction(appearanceIcon, tr("Appearance"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 2);

    added =head->addAction(dataIcon, tr("Data Fields"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 3);

    added =head->addAction(metricsIcon, tr("Metrics"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 4);

    added =head->addAction(intervalIcon, tr("Intervals"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 5);


    added =head->addAction(devicesIcon, tr("Training"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 6);

    // more space
    spacer = new QWidget(this);
    spacer->setAutoFillBackground(false);
    spacer->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    head->addWidget(spacer);


    pagesWidget = new QStackedWidget(this);

    // create those config pages
    general = new GeneralConfig(_home, context);
    HelpWhatsThis *generalHelp = new HelpWhatsThis(general);
    general->setWhatsThis(generalHelp->getWhatsThisText(HelpWhatsThis::Preferences_General));
    pagesWidget->addWidget(general);

    athlete = new AthleteConfig(_home, context);
    HelpWhatsThis *athleteHelp = new HelpWhatsThis(athlete);
    athlete->setWhatsThis(athleteHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_About));
    pagesWidget->addWidget(athlete);

    // units change on general affects units used on entry in athlete pages
    connect (general->generalPage->unitCombo, SIGNAL(currentIndexChanged(int)), athlete->athletePage, SLOT(unitChanged(int)));
    connect (general->generalPage->unitCombo, SIGNAL(currentIndexChanged(int)), athlete->athletePhysPage, SLOT(unitChanged(int)));

    appearance = new AppearanceConfig(_home, context);
    HelpWhatsThis *appearanceHelp = new HelpWhatsThis(appearance);
    appearance->setWhatsThis(appearanceHelp->getWhatsThisText(HelpWhatsThis::Preferences_Appearance));
    pagesWidget->addWidget(appearance);

    data = new DataConfig(_home, context);
    HelpWhatsThis *dataHelp = new HelpWhatsThis(data);
    data->setWhatsThis(dataHelp->getWhatsThisText(HelpWhatsThis::Preferences_DataFields));
    pagesWidget->addWidget(data);

    metric = new MetricConfig(_home, context);
    HelpWhatsThis *metricHelp = new HelpWhatsThis(metric);
    metric->setWhatsThis(metricHelp->getWhatsThisText(HelpWhatsThis::Preferences_Metrics));
    pagesWidget->addWidget(metric);

    interval = new IntervalConfig(_home, context);
    HelpWhatsThis *intervalHelp = new HelpWhatsThis(interval);
    interval->setWhatsThis(intervalHelp->getWhatsThisText(HelpWhatsThis::Preferences_Intervals));
    pagesWidget->addWidget(interval);

    train = new TrainConfig(_home, context);
    HelpWhatsThis *trainHelp = new HelpWhatsThis(train);
    train->setWhatsThis(trainHelp->getWhatsThisText(HelpWhatsThis::Preferences_Training));
    pagesWidget->addWidget(train);

    closeButton = new QPushButton(tr("Close"));
    saveButton = new QPushButton(tr("Save"));

    QHBoxLayout *horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(pagesWidget, 1);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->setSpacing(5 *dpiXFactor);
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
    // hack for raise event
    configdialog_ptr = NULL;

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
    changed |= metric->saveClicked();
    changed |= data->saveClicked();
    changed |= train->saveClicked();
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

            // save all setttings to disk
            appsettings->syncQSettings();

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
GeneralConfig::GeneralConfig(QDir home, Context *context) :
    home(home), context(context)
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
AthleteConfig::AthleteConfig(QDir home, Context *context) :
    home(home), context(context)
{
    //static QIcon passwordIcon(QPixmap(":/images/toolbar/cloud.png")); //Not used for now

    // the widgets
    athletePage = new AboutRiderPage(this, context);
    HelpWhatsThis *athleteHelp = new HelpWhatsThis(athletePage);
    athletePage->setWhatsThis(athleteHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_About));

    modelPage = new AboutModelPage(context);
    HelpWhatsThis *athleteModelHelp = new HelpWhatsThis(modelPage);
    modelPage->setWhatsThis(athleteModelHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_About_Model));

    athletePhysPage = new RiderPhysPage(this, context);
    HelpWhatsThis *athletePhysHelp = new HelpWhatsThis(athletePhysPage);
    athletePhysPage->setWhatsThis(athletePhysHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_About_Phys));

    hrvPage = new HrvPage(this, context);
    HelpWhatsThis *hrvHelp = new HelpWhatsThis(hrvPage);
    hrvPage->setWhatsThis(hrvHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_Hrv));

    zonePage = new ZonePage(context);
    HelpWhatsThis *zoneHelp = new HelpWhatsThis(zonePage);
    zonePage->setWhatsThis(zoneHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_TrainingZones_Power));

    hrZonePage = new HrZonePage(context);
    HelpWhatsThis *hrZoneHelp = new HelpWhatsThis(hrZonePage);
    hrZonePage->setWhatsThis(hrZoneHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_TrainingZones_HR));

    paceZonePage = new PaceZonePage(context);
    HelpWhatsThis *paceZoneHelp = new HelpWhatsThis(paceZonePage);
    paceZonePage->setWhatsThis(paceZoneHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_TrainingZones_Pace));

    credentialsPage = new CredentialsPage(context);
    HelpWhatsThis *credentialsHelp = new HelpWhatsThis(credentialsPage);
    credentialsPage->setWhatsThis(credentialsHelp->getWhatsThisText(HelpWhatsThis::Preferences_Passwords));

    autoImportPage = new AutoImportPage(context);
    HelpWhatsThis *autoImportHelp = new HelpWhatsThis(autoImportPage);
    autoImportPage->setWhatsThis(autoImportHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_Autoimport));

    backupPage = new BackupPage(context);
    HelpWhatsThis *backupPageHelp = new HelpWhatsThis(backupPage);
    autoImportPage->setWhatsThis(backupPageHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_Backup));

    setContentsMargins(0,0,0,0);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    QTabWidget *zonesTab = new QTabWidget(this);
    zonesTab->addTab(zonePage, tr("Power Zones"));
    zonesTab->addTab(hrZonePage, tr("Heartrate Zones"));
    zonesTab->addTab(paceZonePage, tr("Pace Zones"));

    QTabWidget *tabs = new QTabWidget(this);
    tabs->addTab(athletePage, tr("About"));
    tabs->addTab(modelPage, tr("Model"));
    tabs->addTab(athletePhysPage, tr("Measurements"));
    tabs->addTab(hrvPage, tr("HRV"));
    tabs->addTab(zonesTab, tr("Zones"));
    tabs->addTab(credentialsPage, tr("Accounts"));
    tabs->addTab(autoImportPage, tr("Auto Import"));
    tabs->addTab(backupPage, tr("Backup"));

    mainLayout->addWidget(tabs);
}

qint32 AthleteConfig::saveClicked()
{
    qint32 state = 0;

    state |= athletePage->saveClicked();
    state |= modelPage->saveClicked();
    state |= athletePhysPage->saveClicked();
    state |= hrvPage->saveClicked();
    state |= zonePage->saveClicked();
    state |= hrZonePage->saveClicked();
    state |= paceZonePage->saveClicked();
    state |= credentialsPage->saveClicked();
    state |= autoImportPage->saveClicked();
    state |= backupPage->saveClicked();

    return state;
}

// APPEARANCE CONFIG
AppearanceConfig::AppearanceConfig(QDir home, Context *context) :
    home(home), context(context)
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

// METADATA CONFIG
DataConfig::DataConfig(QDir home, Context *context) :
    home(home), context(context)
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
MetricConfig::MetricConfig(QDir home, Context *context) :
    home(home), context(context)
{
    // the widgets
    bestsPage = new BestsMetricsPage(this);
    intervalsPage = new IntervalMetricsPage(this);
    summaryPage = new SummaryMetricsPage(this);
    customPage = new CustomMetricsPage(this, context);

    setContentsMargins(0,0,0,0);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    QTabWidget *tabs = new QTabWidget(this);
    tabs->addTab(customPage, tr("Custom"));
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
    state |= customPage->saveClicked();

    return state;
}

IntervalConfig::IntervalConfig(QDir home, Context *context) :
    home(home), context(context)
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
TrainConfig::TrainConfig(QDir home, Context *context) :
    home(home), context(context)
{

    // the widgets
    devicePage = new DevicePage(this, context);
    optionsPage = new TrainOptionsPage(this, context);
    remotePage = new RemotePage(this, context);
    simBicyclePage = new SimBicyclePage(this, context);

    setContentsMargins(0,0,0,0);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    QTabWidget *tabs = new QTabWidget(this);
    tabs->addTab(devicePage, tr("Train Devices"));
    tabs->addTab(optionsPage, tr("Preferences"));
    tabs->addTab(remotePage, tr("Remote Controls"));
    tabs->addTab(simBicyclePage, tr("Virtual Bicycle Specifications"));

    mainLayout->addWidget(tabs);
}

qint32 TrainConfig::saveClicked()
{
    qint32 state = 0;

    state |= devicePage->saveClicked();
    state |= optionsPage->saveClicked();
    state |= remotePage->saveClicked();
    state |= simBicyclePage->saveClicked();

    return state;
}
