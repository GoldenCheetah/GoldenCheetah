/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#include "AddCloudWizard.h"
#include "MainWindow.h"
#include "Athlete.h"
#include "Context.h"
#include "Colors.h"
#include "CloudService.h"

// WIZARD FLOW
//
// 01. Select Service Class (e.g. Activities, Measures, Calendar)
// 10. Select Cloud Service Type (via CloudServiceFactory)
// 20. Authenticate Account (URL+Key, OAUTH or User/Pass)
// 30. Settings (Folder,sync on startup, sync on import)
// 90. Finalise (Confirm complete and add)
//

// Main wizard
AddCloudWizard::AddCloudWizard(Context *context) : QWizard(context->mainWindow), context(context)
{
#ifdef Q_OS_MAC
    setWizardStyle(QWizard::ModernStyle);
#endif

    // delete when done
    setWindowModality(Qt::NonModal); // avoid blocking WFAPI calls for kickr
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumWidth(600 *dpiXFactor);
    setMinimumHeight(500 *dpiYFactor);

    // title
    setWindowTitle(tr("Add Cloud Wizard"));

    setPage(01, new AddClass(this));   // done
    setPage(10, new AddService(this));   // done
    setPage(20, new AddAuth(this)); // done
    setPage(30, new AddSettings(this)); // done
    setPage(90, new AddFinish(this));     // done

    cloudService = NULL; // not cloned yet
    done = false;
}

/*----------------------------------------------------------------------
 * Wizard Pages
 *--------------------------------------------------------------------*/

//Select Cloud type
AddClass::AddClass(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Service Type"));
    setSubTitle(tr("What type of Service are you adding an account for ?"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mapped(int)), this, SLOT(clicked(int)));

    // Activities
    QCommandLinkButton *p = new QCommandLinkButton(tr("Activities"), tr("Sync activities with services like Today's Plan, Strava, Dropbox and Google Drive"), this);
    p->setStyleSheet(QString("font-size: %1px;").arg(12 * dpiXFactor));
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, CloudService::Activities);
    layout->addWidget(p);

    // Measures
    p = new QCommandLinkButton(tr("Measures"), tr("Sync measures data such as weight, body fat, HRV and sleep."));
    p->setStyleSheet(QString("font-size: %1px;").arg(12 * dpiXFactor));
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, CloudService::Measures);
    layout->addWidget(p);

    // Activities
    p = new QCommandLinkButton(tr("Calendar"), tr("Sync planned workouts to WebDAV and CalDAV calendars."));
    p->setStyleSheet(QString("font-size: %1px;").arg(12 * dpiXFactor));
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, CloudService::Calendar);
    layout->addWidget(p);

    setFinalPage(false);
}

void
AddClass::clicked(int t)
{
    // reset -- particularly since we might get here from
    //          other pages hitting 'Back'
    wizard->type = t;
    initializePage();
    wizard->next();
}

//Select Cloud type
AddService::AddService(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Account Type"));
    setSubTitle(tr("Select the cloud service type"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    buttons=new QWidget(this);
    buttons->setContentsMargins(0,0,0,0);
    buttonlayout= new  QVBoxLayout(buttons);
    buttonlayout->setSpacing(0);
    scrollarea=new QScrollArea(this);
    scrollarea->setWidgetResizable(true);
    scrollarea->setWidget(buttons);

    mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mapped(QString)), this, SLOT(clicked(QString)));

    layout->addWidget(scrollarea);

    setFinalPage(false);
}

void
AddService::initializePage()
{
    // clear whatever we have, if anything
    QLayoutItem *item = NULL;
    while((item = buttonlayout->takeAt(0)) != NULL) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    CloudServiceFactory &factory = CloudServiceFactory::instance();
    foreach(CloudService *s, factory.services()) {

        // only ones with the capability we need.
        if (s->type() != wizard->type) continue;

        QCommandLinkButton *p = new QCommandLinkButton(s->name(), s->description(), this);
        p->setStyleSheet(QString("font-size: %1px;").arg(12 * dpiXFactor));
        connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
        mapper->setMapping(p, s->name());
        buttonlayout->addWidget(p);
    }
}

void
AddService::clicked(QString p)
{
    // reset -- particularly since we might get here from
    //          other pages hitting 'Back'
    wizard->service = p;
    wizard->next();
}

//Select Cloud type
AddAuth::AddAuth(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Service Credentials "));
    setSubTitle(tr("Credentials and authorisation"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    setFinalPage(false);
}

void
AddAuth::initializePage()
{
    setSubTitle(tr("Credentials and authorisation"));
    const CloudService *s = CloudServiceFactory::instance().service(wizard->service);

    // hide all the widgets
    // TODO

    // clone to do next few steps!
    if (s) {
        if (wizard->cloudService) delete wizard->cloudService;
        wizard->cloudService = const_cast<CloudService*>(s)->clone(wizard->context);
        setSubTitle(QString(tr("%1 Credentials and authorisation")).arg(wizard->cloudService->name()));

        // show  all the widgets relevant for this service
        // TODO
    }
}

bool
AddAuth::validatePage()
{
    // check the authorisation has been completed
    // TODO
    return true;
}


// Scan for Cloud port / usb etc
AddSettings::AddSettings(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setSubTitle(tr("Cloud Service Settings"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
}

void
AddSettings::initializePage()
{
    setTitle(QString(tr("Service Settings")));
}

bool
AddSettings::validatePage()
{
    return true;
}

// Final confirmation
AddFinish::AddFinish(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Done"));
    setSubTitle(tr("Add Cloud Account"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
#if 0
    QLabel *label = new QLabel(tr("We will now add a new Cloud with the configuration shown "
                               "below. Please take a moment to review and then click Finish "
                               "to add the Cloud and complete this wizard, or press the Back "
                               "button to make amendments.\n\n"));
    label->setWordWrap(true);
    layout->addWidget(label);

    QHBoxLayout *hlayout = new QHBoxLayout;
    layout->addLayout(hlayout);

    QFormLayout *formlayout = new QFormLayout;
    formlayout->addRow(new QLabel(tr("Name*"), this), (name=new QLineEdit(this)));
    formlayout->addRow(new QLabel(tr("Port"), this), (port=new QLineEdit(this)));
    formlayout->addRow(new QLabel(tr("Profile"), this), (profile=new QLineEdit(this)));
    formlayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    //profile->setFixedWidth(200);
    port->setFixedWidth(150);
    port->setEnabled(false); // no edit
    //name->setFixedWidth(230);
    hlayout->addLayout(formlayout);

    QFormLayout *form2layout = new QFormLayout;
    form2layout->addRow(new QLabel(tr("Virtual"), this), (virtualPower=new QComboBox(this)));

    // NOTE: These must correspond to the code in RealtimeController.cpp that
    //       post-processes inbound telemetry.
    virtualPower->addItem(tr("None"));
    virtualPower->addItem(tr("Power - Kurt Kinetic Cyclone"));                              // 1
    virtualPower->addItem(tr("Power - Kurt Kinetic Road Machine"));                         // 2
    virtualPower->addItem(tr("Power - Cyclops Fluid 2"));                                   // 3
    virtualPower->addItem(tr("Power - BT Advanced Training System"));                       // 4
    virtualPower->addItem(tr("Power - LeMond Revolution"));                                 // 5
    virtualPower->addItem(tr("Power - 1UP USA Trainer"));                                   // 6
    virtualPower->addItem(tr("Power - Minoura V100 Trainer (H)"));                          // 7
    virtualPower->addItem(tr("Power - Minoura V100 Trainer (5)"));                          // 8
    virtualPower->addItem(tr("Power - Minoura V100 Trainer (4)"));                          // 9
    virtualPower->addItem(tr("Power - Minoura V100 Trainer (3)"));                          // 10
    virtualPower->addItem(tr("Power - Minoura V100 Trainer (2)"));                          // 11
    virtualPower->addItem(tr("Power - Minoura V100 Trainer (1)"));                          // 12
    virtualPower->addItem(tr("Power - Minoura V100 Trainer (L)"));                          // 13
    virtualPower->addItem(tr("Power - Minoura V270/V150/V130/LR340/LR540 Trainer (H)"));    // 14
    virtualPower->addItem(tr("Power - Minoura V270/V150/V130/LR340/LR540 Trainer (5)"));    // 15
    virtualPower->addItem(tr("Power - Minoura V270/V150/V130/LR340/LR540 Trainer (4)"));    // 16
    virtualPower->addItem(tr("Power - Minoura V270/V150/V130/LR340/LR540 Trainer (3)"));    // 17
    virtualPower->addItem(tr("Power - Minoura V270/V150/V130/LR340/LR540 Trainer (2)"));    // 18
    virtualPower->addItem(tr("Power - Minoura V270/V150/V130/LR340/LR540 Trainer (1)"));    // 19
    virtualPower->addItem(tr("Power - Minoura V270/V150/V130/LR340/LR540 Trainer (L)"));    // 20
    virtualPower->addItem(tr("Power - Saris Powerbeam Pro"));                               // 21
    virtualPower->addItem(tr("Power - Tacx Satori (2)"));                                   // 22
    virtualPower->addItem(tr("Power - Tacx Satori (4)"));                                   // 23
    virtualPower->addItem(tr("Power - Tacx Satori (6)"));                                   // 24
    virtualPower->addItem(tr("Power - Tacx Satori (8)"));                                   // 25
    virtualPower->addItem(tr("Power - Tacx Satori (10)"));                                  // 26
    virtualPower->addItem(tr("Power - Tacx Flow (0)"));                                     // 27
    virtualPower->addItem(tr("Power - Tacx Flow (2)"));                                     // 28
    virtualPower->addItem(tr("Power - Tacx Flow (4)"));                                     // 29
    virtualPower->addItem(tr("Power - Tacx Flow (6)"));                                     // 30
    virtualPower->addItem(tr("Power - Tacx Flow (8)"));                                     // 31
    virtualPower->addItem(tr("Power - Tacx Blue Twist (1)"));                               // 32
    virtualPower->addItem(tr("Power - Tacx Blue Twist (3)"));                               // 33
    virtualPower->addItem(tr("Power - Tacx Blue Twist (5)"));                               // 34
    virtualPower->addItem(tr("Power - Tacx Blue Twist (7)"));                               // 35
    virtualPower->addItem(tr("Power - Tacx Blue Motion (2)"));                              // 36
    virtualPower->addItem(tr("Power - Tacx Blue Motion (4)"));                              // 37
    virtualPower->addItem(tr("Power - Tacx Blue Motion (6)"));                              // 38
    virtualPower->addItem(tr("Power - Tacx Blue Motion (8)"));                              // 39
    virtualPower->addItem(tr("Power - Tacx Blue Motion (10)"));                             // 40
    virtualPower->addItem(tr("Power - Elite Supercrono Powermag (1)"));                     // 41
    virtualPower->addItem(tr("Power - Elite Supercrono Powermag (2)"));                     // 42
    virtualPower->addItem(tr("Power - Elite Supercrono Powermag (3)"));                     // 43
    virtualPower->addItem(tr("Power - Elite Supercrono Powermag (4)"));                     // 44
    virtualPower->addItem(tr("Power - Elite Supercrono Powermag (5)"));                     // 45
    virtualPower->addItem(tr("Power - Elite Supercrono Powermag (6)"));                     // 46
    virtualPower->addItem(tr("Power - Elite Supercrono Powermag (7)"));                     // 47
    virtualPower->addItem(tr("Power - Elite Supercrono Powermag (8)"));                     // 48
    virtualPower->addItem(tr("Power - Elite Turbo Muin (2013)"));                           // 49
    virtualPower->addItem(tr("Power - Elite Qubo Power Fluid"));                            // 50
    virtualPower->addItem(tr("Power - Cyclops Magneto Pro (Road)"));                        // 51
    virtualPower->addItem(tr("Power - Elite Arion Mag (0)"));                               // 52
    virtualPower->addItem(tr("Power - Elite Arion Mag (1)"));                               // 53
    virtualPower->addItem(tr("Power - Elite Arion Mag (2)"));                               // 54
    virtualPower->addItem(tr("Power - Blackburn Tech Fluid"));                              // 55
    virtualPower->addItem(tr("Power - Tacx Sirius (1)"));                                   // 56
    virtualPower->addItem(tr("Power - Tacx Sirius (2)"));                                   // 57
    virtualPower->addItem(tr("Power - Tacx Sirius (3)"));                                   // 58
    virtualPower->addItem(tr("Power - Tacx Sirius (4)"));                                   // 59
    virtualPower->addItem(tr("Power - Tacx Sirius (5)"));                                   // 60
    virtualPower->addItem(tr("Power - Tacx Sirius (6)"));                                   // 61
    virtualPower->addItem(tr("Power - Tacx Sirius (7)"));                                   // 62
    virtualPower->addItem(tr("Power - Tacx Sirius (8)"));                                   // 63
    virtualPower->addItem(tr("Power - Tacx Sirius (9)"));                                   // 64
    virtualPower->addItem(tr("Power - Tacx Sirius (10)"));                                  // 65

    //
    // Wheel size
    //
    int wheelSize = appsettings->cvalue(parent->context->athlete->cyclist, GC_WHEELSIZE, 2100).toInt();

    rimSizeCombo = new QComboBox();
    rimSizeCombo->addItems(WheelSize::RIM_SIZES);

    tireSizeCombo = new QComboBox();
    tireSizeCombo->addItems(WheelSize::TIRE_SIZES);


    wheelSizeEdit = new QLineEdit(QString("%1").arg(wheelSize),this);
    wheelSizeEdit->setInputMask("0000");
    wheelSizeEdit->setFixedWidth(40);

    QLabel *wheelSizeUnitLabel = new QLabel(tr("mm"), this);

    QHBoxLayout *wheelSizeLayout = new QHBoxLayout();
    wheelSizeLayout->addWidget(rimSizeCombo);
    wheelSizeLayout->addWidget(tireSizeCombo);
    wheelSizeLayout->addWidget(wheelSizeEdit);
    wheelSizeLayout->addWidget(wheelSizeUnitLabel);

    stridelengthEdit = new QLineEdit(this);
    stridelengthEdit->setText("115");
    QHBoxLayout *stridelengthLayout = new QHBoxLayout;
    stridelengthLayout->addWidget(stridelengthEdit);
    stridelengthLayout->addStretch();

    connect(rimSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(calcWheelSize()));
    connect(tireSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(calcWheelSize()));
    connect(wheelSizeEdit, SIGNAL(textEdited(QString)), this, SLOT(resetWheelSize()));

    form2layout->addRow(new QLabel(tr("Wheel Size"), this), wheelSizeLayout);
    form2layout->addRow(new QLabel(tr("Stride Length (cm)"), this), stridelengthLayout);

    hlayout->addLayout(form2layout);
    layout->addStretch();

    selectDefault = new QGroupBox(tr("Selected by default"), this);
    selectDefault->setCheckable(true);
    selectDefault->setChecked(false);
    layout->addWidget(selectDefault);

    QGridLayout *grid = new QGridLayout;
    selectDefault->setLayout(grid);
    grid->addWidget((defWatts=new QCheckBox(tr("Power"))), 0,0, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget((defBPM=new QCheckBox(tr("Heartrate"))), 1,0, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget((defKPH=new QCheckBox(tr("Speed"))), 0,1, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget((defRPM=new QCheckBox(tr("Cadence"))), 1,1, Qt::AlignVCenter|Qt::AlignLeft);
    layout->addStretch();
#endif
}

void
AddFinish::initializePage()
{
}
