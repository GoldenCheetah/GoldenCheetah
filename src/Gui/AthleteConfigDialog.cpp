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
#include <QWidget>

#include "Context.h"
#include "Athlete.h"
#include "AthleteConfigDialog.h"
#include "RideCache.h"
#include "Pages.h"
#include "Settings.h"
#include "HelpWhatsThis.h"

#include "AddDeviceWizard.h"
#include "MainWindow.h"

AthleteConfigDialog::AthleteConfigDialog(QDir _home, Context *context) :
    QDialog(NULL), home(_home), context(context)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumSize(800 *dpiXFactor,650 *dpiYFactor);   //changed for hidpi sizing

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    athlete = new AthleteConfig(_home, context);
    HelpWhatsThis *athleteHelp = new HelpWhatsThis(athlete);
    athlete->setWhatsThis(athleteHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_About));
    mainLayout->addWidget(athlete);

    closeButton = new QPushButton(tr("Close"));
    saveButton = new QPushButton(tr("Save"));

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->setSpacing(5 *dpiXFactor);
    buttonsLayout->addWidget(closeButton);
    buttonsLayout->addWidget(saveButton);

    mainLayout->addLayout(buttonsLayout);

    // We go fixed width to ensure a consistent layout for
    // tabs, sub-tabs and internal widgets and lists
    setWindowTitle(tr("Athlete Settings"));

    connect(closeButton, SIGNAL(clicked()), this, SLOT(closeClicked()));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(saveClicked()));
}

AthleteConfigDialog::~AthleteConfigDialog()
{
    // hack for raise event
}

void AthleteConfigDialog::closeClicked()
{
    // don't save!
    close();
}

// if save is clicked, we want to:
//   new mode:   create a new zone starting at the selected date (which may be null, implying BEGIN
//   ! new mode: change the CP associated with the present mode
void AthleteConfigDialog::saveClicked()
{
    // what changed ?
    qint32 changed = 0;

    changed |= athlete->saveClicked();

    if (changed) {

        // hide away whilst changes are applied
        hide();

        // if a refresh is happenning stop it, whilst we
        // update all the configuration settings!
        context->athlete->rideCache->cancel();

        // tell this context athlete settings changed
        context->notifyConfigChanged(changed);

        // hide the dialog
        close();
    }
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

    // one for each Measures Group
    measuresPages = QVector<MeasuresPage*>(context->athlete->measures->getGroupNames().count());
    for (int i = 0; i < measuresPages.count(); i++) {
        measuresPages[i] = new MeasuresPage(this, context, context->athlete->measures->getGroup(i));
        HelpWhatsThis *measuresHelp = new HelpWhatsThis(measuresPages[i]);
        measuresPages[i]->setWhatsThis(measuresHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_Measures));
    }

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
    backupPage->setWhatsThis(backupPageHelp->getWhatsThisText(HelpWhatsThis::Preferences_Athlete_Backup));

    setContentsMargins(0,0,0,0);
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    QWidget *mc = new QWidget(this);
    QVBoxLayout *ml = new QVBoxLayout(mc);
    QTabWidget *measuresTab = new QTabWidget(this);
    ml->addWidget(measuresTab);
    measuresTab->setContentsMargins(10*dpiXFactor,10*dpiXFactor,10*dpiXFactor,10*dpiXFactor);
    for (int i=0; i<context->athlete->measures->getGroupNames().count(); i++)
        measuresTab->addTab(measuresPages[i], context->athlete->measures->getGroupNames()[i]);

    QWidget *zc = new QWidget(this);
    QVBoxLayout *zl = new QVBoxLayout(zc);
    QTabWidget *zonesTab = new QTabWidget(this);
    zl->addWidget(zonesTab);
    zonesTab->setContentsMargins(10*dpiXFactor,10*dpiXFactor,10*dpiXFactor,10*dpiXFactor);
    zonesTab->addTab(zonePage, tr("Power"));
    zonesTab->addTab(hrZonePage, tr("Heartrate"));
    zonesTab->addTab(paceZonePage, tr("Pace"));

    // if the plot background and window background
    // are the same color, lets use the accent color since
    // it fits in with the rest of the color theme
    // otherwise just use same as the system color for text
    QPalette std;
    QColor tabselect = std.color(QPalette::Text);
    if (GColor(CPLOTBACKGROUND) == std.color(QPalette::Base)) tabselect = GColor(CPLOTMARKER);

#ifndef Q_OS_MAC
    QString styling = QString("QTabWidget { background: %1; }"
                          "QTabWidget::pane { border: 0px; }"
                          "QTabBar::tab { background: %1; "
                          "               color: %6; "
                          "               min-width: %5px; "
                          "               padding: %4px; "
                          "               border-top: 0px;"
                          "               border-left: 0px;"
                          "               border-right: 0px;"
                          "               border-bottom: %3px solid %1; } "
                          "QTabBar::tab:selected { border-bottom-right-radius: 0px; border-bottom-left-radius: 0px; border-bottom-color: %2; }"
                         ).arg(std.color(QPalette::Base).name())                      // 1 tab background color
                          .arg(tabselect.name())                                      // 2 selected bar color
                          .arg(4*dpiYFactor)                                          // 3 selected bar width
                          .arg(2*dpiXFactor)                                          // 4 padding
                          .arg(75*dpiXFactor)                                         // 5 tab minimum width
                          .arg(std.color(QPalette::Text).name()); // 6 tab text color
    zonesTab->setStyleSheet(styling);
    measuresTab->setStyleSheet(styling);
#endif

    QTabWidget *tabs = new QTabWidget(this);
    tabs->addTab(athletePage, tr("About"));
    tabs->addTab(credentialsPage, tr("Accounts"));
    tabs->addTab(zc, tr("Zones"));
    tabs->addTab(mc, tr("Measures"));
    tabs->addTab(modelPage, tr("Model"));
    tabs->addTab(autoImportPage, tr("Auto Import"));
    tabs->addTab(backupPage, tr("Backup"));

    mainLayout->addWidget(tabs);
}

qint32 AthleteConfig::saveClicked()
{
    qint32 state = 0;

    state |= athletePage->saveClicked();
    state |= modelPage->saveClicked();
    foreach (MeasuresPage *measuresPage, measuresPages)
        state |= measuresPage->saveClicked();
    state |= zonePage->saveClicked();
    state |= hrZonePage->saveClicked();
    state |= paceZonePage->saveClicked();
    state |= credentialsPage->saveClicked();
    state |= autoImportPage->saveClicked();
    state |= backupPage->saveClicked();

    return state;
}
