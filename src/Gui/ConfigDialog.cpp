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

    // Enable What's this button and disable minimize/maximize buttons
    setWindowFlags(Qt::Window | Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint);

    // center
    QWidget *spacer = new QWidget(this);
    spacer->setAutoFillBackground(false);
    spacer->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    head->addWidget(spacer);

    // icons
    static QIcon generalIcon(QPixmap(":images/toolbar/GeneralPreferences.png"));
    static QIcon appearanceIcon(QPixmap(":/images/toolbar/color.png"));
    static QIcon dataIcon(QPixmap(":/images/toolbar/data.png"));
    static QIcon metricsIcon(QPixmap(":/images/toolbar/abacus.png"));
    static QIcon intervalIcon(QPixmap(":/images/stopwatch.png"));
    static QIcon measuresIcon(QPixmap(":/images/toolbar/main/measures.png"));
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

    added =head->addAction(appearanceIcon, tr("Appearance"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 1);

    added =head->addAction(dataIcon, tr("Data Fields"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 2);

    added =head->addAction(metricsIcon, tr("Metrics"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 3);

    added =head->addAction(intervalIcon, tr("Intervals"));
    connect(added, SIGNAL(triggered()), iconMapper, SLOT(map()));
    iconMapper->setMapping(added, 4);

    added =head->addAction(measuresIcon, tr("Measures"));
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

    measures = new MeasuresConfig(_home, context);
    HelpWhatsThis *measuresHelp = new HelpWhatsThis(measures);
    measures->setWhatsThis(measuresHelp->getWhatsThisText(HelpWhatsThis::Preferences_Measures));
    pagesWidget->addWidget(measures);

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

ConfigDialog::~ConfigDialog()
{
    // hack for raise event
    configdialog_ptr = NULL;
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
    changed |= appearance->saveClicked();
    changed |= metric->saveClicked();
    changed |= data->saveClicked();
    changed |= train->saveClicked();
    changed |= interval->saveClicked();
    changed |= measures->saveClicked();

    hide();

    // did the home directory change?
    QString shome = appsettings->value(this, GC_HOMEDIR).toString();
    if (shome != general->generalPage->athleteWAS) {

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

            // revert to previous home
            appsettings->setValue(GC_HOMEDIR, general->generalPage->athleteWAS);
        }

    } 

    // global context changed, will be cascaded to each athlete context
    GlobalContext::context()->notifyConfigChanged(changed);

    // done.
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

// METRIC CONFIG
MetricConfig::MetricConfig(QDir home, Context *context) :
    home(home), context(context)
{
    // the widgets
    intervalsPage = new FavouriteMetricsPage(this);
    customPage = new CustomMetricsPage(this, context);

    setContentsMargins(0,0,0,0);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    QTabWidget *tabs = new QTabWidget(this);
    tabs->addTab(customPage, tr("Custom"));
    tabs->addTab(intervalsPage, tr("Favourites"));
    mainLayout->addWidget(tabs);
}

qint32 MetricConfig::saveClicked()
{
    qint32 state = 0;

    state |= intervalsPage->saveClicked();
    state |= customPage->saveClicked();

    return state;
}

// INTERVALS CONFIG
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

// MEASURES CONFIG
MeasuresConfig::MeasuresConfig(QDir home, Context *context) :
    home(home), context(context)
{
    // the widgets
    measuresPage = new MeasuresConfigPage(this, context);

    setContentsMargins(0,0,0,0);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    mainLayout->addWidget(measuresPage);
}

qint32 MeasuresConfig::saveClicked()
{
    qint32 state = 0;

    state |= measuresPage->saveClicked();

    return state;
}

// TRAIN CONFIG
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
