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
#include "Settings.h"
#include "Colors.h"
#include "CloudService.h"

#include <QMessageBox>

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
    const CloudService *s = CloudServiceFactory::instance().service(wizard->service);

    // instatiate the cloudservice, complete with current configuration etc
    if (wizard->cloudService) delete wizard->cloudService;
    wizard->cloudService = CloudServiceFactory::instance().newService(p, wizard->context);

    wizard->next();
}

//Select Cloud type
AddAuth::AddAuth(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Service Credentials "));
    setSubTitle(tr("Credentials and authorisation"));

    QFormLayout *layout = new QFormLayout;
    //layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // labels
    urlLabel = new QLabel(tr("URL"));
    keyLabel = new QLabel(tr("Key (optional)"));
    userLabel = new QLabel(tr("Username"));
    passLabel = new QLabel(tr("Password"));
    authLabel = new QLabel(tr("Authorise"));
    tokenLabel = new QLabel(tr("Token"));

    // input boxes
    url = new QLineEdit(this);
    url->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    key = new QLineEdit(this);
    key->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    user = new QLineEdit(this);
    user->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    pass = new QLineEdit(this);
    pass->setEchoMode(QLineEdit::Password);
    pass->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auth = new QPushButton(tr("Authorise"), this);
    auth->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    token = new QLabel(this);
    token->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    layout->addRow(urlLabel, url);
    layout->addRow(keyLabel, key);
    layout->addRow(userLabel, user);
    layout->addRow(passLabel, pass);
    layout->addRow(authLabel, auth);
    layout->addRow(tokenLabel, token);

    setLayout(layout);
    setFinalPage(false);
}

void
AddAuth::initializePage()
{
    setSubTitle(tr("Credentials and authorisation"));

    // hide all the widgets
    url->hide();
    key->hide();
    user->hide();
    pass->hide();
    auth->hide();
    token->hide();
    urlLabel->hide();
    keyLabel->hide();
    userLabel->hide();
    passLabel->hide();
    authLabel->hide();
    tokenLabel->hide();

    // clone to do next few steps!
    setSubTitle(QString(tr("%1 Credentials and authorisation")).arg(wizard->cloudService->name()));

    // show  all the widgets relevant for this service and update the value from the
    // settings we have collected (which will have been defaulted).
    QString cname;
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::URL, "")) != "") {
        url->show(); urlLabel->show();
        url->setText(wizard->cloudService->getSetting(cname, "").toString());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Key, "")) != "") {
        key->show(); keyLabel->show();
        key->setText(wizard->cloudService->getSetting(cname, "").toString());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Username, "")) != "") {
        user->show(); userLabel->show();
        user->setText(wizard->cloudService->getSetting(cname, "").toString());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Password, "")) != "") {
        pass->show(); passLabel->show();
        pass->setText(wizard->cloudService->getSetting(cname, "").toString());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::OAuthToken, "")) != "") {
        auth->show(); authLabel->show();
        token->show(); tokenLabel->show();
        token->setText(wizard->cloudService->getSetting(cname, "").toString());
    }

}

bool
AddAuth::validatePage()
{
    // check the authorisation has been completed
    QString cname;
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::URL, "")) != "") {
        wizard->cloudService->setSetting(cname, url->text());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Key, "")) != "") {
        wizard->cloudService->setSetting(cname, key->text());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Username, "")) != "") {
        wizard->cloudService->setSetting(cname, user->text());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Password, "")) != "") {
        wizard->cloudService->setSetting(cname, pass->text());
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::OAuthToken, "")) != "") {
        wizard->cloudService->setSetting(cname, token->text());
    }
    return true;
}


// Scan for Cloud port / usb etc
AddSettings::AddSettings(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setSubTitle(tr("Cloud Service Settings"));

    QFormLayout *layout = new QFormLayout(this);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(layout);

    folderLabel = new QLabel(tr("Folder"));
    folder = new QLineEdit(this);
    folder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    browse = new QPushButton(tr("Browse"));
    syncStartup = new QCheckBox(tr("Sync on startup"));
    syncImport = new QCheckBox(tr("Sync on import"));

    QHBoxLayout *flayout = new QHBoxLayout;
    flayout->addWidget(folderLabel);
    flayout->addWidget(folder);
    flayout->addWidget(browse);
    layout->addRow(flayout);

    layout->addRow(syncStartup); // only makes sense if the service has a query api
    layout->addRow(syncImport); // only makes sense if the service has an upload api

    connect(browse, SIGNAL(clicked()), this, SLOT(browseFolder()));
}

void
AddSettings::initializePage()
{
    setTitle(QString(tr("Service Settings")));

    // hide everything first
    folderLabel->hide();
    folder->hide();
    browse->hide();
    syncStartup->hide();
    syncImport->hide();

    // if we need a folder then set that to show
    QString cname;
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Folder, "")) != "") {
        browse->show(); folder->show(); folderLabel->show();
        folder->setText(wizard->cloudService->getSetting(cname, "").toString());
    }
    if (wizard->cloudService->capabilities() & CloudService::Query) {
        QString value = wizard->cloudService->getSetting(wizard->cloudService->syncOnStartupSettingName(), "false").toString();
        syncStartup->setChecked(value == "true");
        syncStartup->show();
    }
    if (wizard->cloudService->capabilities() & CloudService::Upload) {
        QString value = wizard->cloudService->getSetting(wizard->cloudService->syncOnImportSettingName(), "false").toString();
        syncImport->setChecked(value == "true");
        syncImport->show();
    }
}

bool
AddSettings::validatePage()
{
    // check the authorisation has been completed
    QString cname;
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Folder, "")) != "") {
        wizard->cloudService->setSetting(cname, folder->text());
    }

    // generic settings, but applied on a per service basis
    wizard->cloudService->setSetting(wizard->cloudService->syncOnImportSettingName(), syncImport->isChecked() ? "true" : "false");
    wizard->cloudService->setSetting(wizard->cloudService->syncOnStartupSettingName(), syncImport->isChecked() ? "true" : "false");
    return true;
}

void
AddSettings::browseFolder()
{
    // get current edit..
    QString path = folder->text();
    QStringList errors;

    // open the connection using the current token
    if (!wizard->cloudService->open(errors)) {
        QMessageBox err;
        err.setText(tr("Connection Failed"));
        err.setDetailedText(errors.join("\n\n"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
    }

    // find the folder using the current settings
    CloudServiceDialog dialog(this, wizard->cloudService, tr("Choose Athlete Directory"), path, true);
    int ret = dialog.exec();

    // did we actually select something?
    if (ret == QDialog::Accepted) {
        path = dialog.pathnameSelected();
        folder->setText(path);
        //XXX ack QString id = google_drive.GetFileId(path);
        //XXX ack appsettings->setCValue(context->athlete->cyclist, GC_GOOGLE_DRIVE_FOLDER_ID, id);
    }
}

// Final confirmation
AddFinish::AddFinish(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Done"));
    setSubTitle(tr("Add Cloud Account"));

    layout = new QFormLayout;
    setLayout(layout);
}

void
AddFinish::initializePage()
{
    // clear previous
    while(layout->count() > 0) {
       QLayoutItem *item = layout->takeAt(0);
       if (item->widget()) delete item->widget();
       delete item;
    }

    // add from wizard settings -- this is what we
    // will now create.
    QHashIterator<CloudService::CloudServiceSetting,QString> want(wizard->cloudService->settings);
    want.toFront();
    while(want.hasNext()) {
        want.next();

        QString label, value;
        switch(want.key()) {
            case CloudService::URL: label=tr("URL"); break;
            case CloudService::Key: label=tr("Key"); break;
            case CloudService::Username: label=tr("Username"); break;
            case CloudService::Password: label=tr("Password"); break;
            case CloudService::OAuthToken: label=tr("Token"); break;
            case CloudService::Folder: label=tr("Folder"); break;
            case CloudService::Local: label=want.value(); break;
            case CloudService::DefaultURL: break;
        }
        // no clue
        if (label == "") continue;

        // get value
        value = wizard->cloudService->getSetting(want.value(), "").toString();
        if (value == "") continue;

        // ok, we have a setting
        if (label==tr("Password")) layout->addRow(new QLabel(label), new QLabel (QString("*").repeated(value.length())));
        else layout->addRow(new QLabel(label), new QLabel (value));
    }
    QString syncstartup = wizard->cloudService->getSetting(wizard->cloudService->syncOnStartupSettingName(), "").toString();
    if (syncstartup != "") layout->addRow(new QLabel(tr("Sync on start")), new QLabel (syncstartup));
    QString syncimport = wizard->cloudService->getSetting(wizard->cloudService->syncOnImportSettingName(), "").toString();
    if (syncimport != "") layout->addRow(new QLabel(tr("Sync on import")), new QLabel (syncimport));
}

bool
AddFinish::validatePage()
{
    // ok, last thing to do is write out the new settings
    QHashIterator<CloudService::CloudServiceSetting,QString> want(wizard->cloudService->settings);
    want.toFront();
    while(want.hasNext()) {
        want.next();

        // get value
        QString value = wizard->cloudService->getSetting(want.value(), "").toString();
        if (value == "") continue;

        // ok, we have a setting
        appsettings->setCValue(wizard->context->athlete->cyclist, want.value(), value);
    }

    // generic settings
    QString syncstartup = wizard->cloudService->getSetting(wizard->cloudService->syncOnStartupSettingName(), "").toString();
    if (syncstartup != "")  appsettings->setCValue(wizard->context->athlete->cyclist,
                                                   wizard->cloudService->syncOnStartupSettingName(),
                                                   syncstartup);

    QString syncimport = wizard->cloudService->getSetting(wizard->cloudService->syncOnImportSettingName(), "").toString();
    if (syncimport != "")  appsettings->setCValue(wizard->context->athlete->cyclist,
                                                   wizard->cloudService->syncOnImportSettingName(),
                                                   syncimport);

    // this service is now active, only way to set to non active would be to delete it
    // in the athlete preferences
    appsettings->setCValue(wizard->context->athlete->cyclist,
                                                   wizard->cloudService->activeSettingName(),
                                                   "true");
    return true;
}
