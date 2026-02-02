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
#include "RideMetadata.h"
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include "Colors.h"
#include "CloudService.h"
#include "OAuthDialog.h"

#include <QMessageBox>
#include <QPixmap>
#include <QRegExp>

// WIZARD FLOW
//
// 01. Select Service Class (e.g. Activities, Measures)
// 10. Select Cloud Service Type (via CloudServiceFactory)
// 15. Agree to terms of service (optional)
// 20. Authenticate Account (URL+Key, OAUTH or User/Pass)
// 25. Select Athlete [optional]
// 30. Settings (Folder,sync on startup, sync on import)
// 90. Finalise (Confirm complete and add)
//

// Main wizard - if passed a service name we are in edit mode, not add mode.
AddCloudWizard::AddCloudWizard(Context *context, QString sname, bool sync) : QWizard(context->mainWindow), context(context), service(sname), fsync(sync)
{
#ifdef Q_OS_MAC
    setWizardStyle(QWizard::ModernStyle);
#endif

    // delete when done
    setWindowModality(Qt::NonModal); // avoid blocking WFAPI calls for kickr
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumWidth(600 *dpiXFactor);
    setMinimumHeight(500 *dpiYFactor);

    // if we're passed the service, we're editing, otherwise
    // we're adding a new one.
    if (service == "") {

        setWindowTitle(tr("Add Cloud Wizard"));
        setPage(01, new AddClass(this));
        setPage(10, new AddService(this));
        setPage(15, new AddConsent(this));
        cloudService = NULL; // not cloned yet

    } else {

        setWindowTitle(tr("Edit Account Details"));
        cloudService = CloudServiceFactory::instance().newService(service, context);
    }


    setPage(20, new AddAuth(this)); // done
    setPage(25, new AddAthlete(this)); // done
    setPage(30, new AddSettings(this)); // done
    setPage(90, new AddFinish(this));     // done

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
    connect(mapper, &QSignalMapper::mappedInt, this, &AddClass::clicked);

    // Activities
    QFont font;
    QCommandLinkButton *p = new QCommandLinkButton(tr("Activities"), tr("Sync activities with services like Today's Plan, Strava, Dropbox and Google Drive"), this);
    p->setStyleSheet(QString("font-size: %1px;").arg(font.pointSizeF() * dpiXFactor));
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, CloudService::Activities);
    layout->addWidget(p);

    // Measures
    p = new QCommandLinkButton(tr("Measures"), tr("Download measures such as weight, body fat, HRV and sleep."));
    p->setStyleSheet(QString("font-size: %1px;").arg(font.pointSizeF() * dpiXFactor));
    connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
    mapper->setMapping(p, CloudService::Measures);
    layout->addWidget(p);

    // Calendar
    p = new QCommandLinkButton(tr("Calendar"), tr("Sync planned workouts to WebDAV and CalDAV calendars like Google Calendar."));
    p->setStyleSheet(QString("font-size: %1px;").arg(font.pointSizeF() * dpiXFactor));
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
    connect(mapper, &QSignalMapper::mappedString, this, &AddService::clicked);

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

    QFont font; // default font size

    // iterate over names, as they are sorted alphabetically
    foreach(QString name, factory.serviceNames()) {

        // get the service
        const CloudService *s = factory.service(name);

        // only ones with the capability we need.
        if (s->type() != wizard->type) continue;

        QCommandLinkButton *p = new QCommandLinkButton(s->uiName(), s->description(), this);
        p->setStyleSheet(QString("font-size: %1px;").arg(font.pointSizeF() * dpiXFactor));
        p->setFixedHeight(50 *dpiYFactor);
        connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
        mapper->setMapping(p, s->id());
        buttonlayout->addWidget(p);
    }
    buttonlayout->addStretch();
}

int AddService::nextId() const
{
    if (wizard->cloudService) {
        if (wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Consent, "") != "") return 15;
        else return 20;
    }

    // loop round
    return 10;
}
void
AddService::clicked(QString p)
{
    // reset -- particularly since we might get here from
    //          other pages hitting 'Back'
    wizard->service = p;

    // instatiate the cloudservice, complete with current configuration etc
    if (wizard->cloudService) delete wizard->cloudService;
    wizard->cloudService = CloudServiceFactory::instance().newService(p, wizard->context);

    wizard->next();
}

// Consent to terms of service if needed
AddConsent::AddConsent(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent), consented(false)
{
    setTitle(tr("Terms of Service"));
    setSubTitle(tr("Your consent is needed"));

    layout = new QVBoxLayout;
    setLayout(layout);

    document = new QTextEdit(this);
    document->setReadOnly(true);
    layout->addWidget(document);

    QHBoxLayout *buttons = new QHBoxLayout;
    approve = new QPushButton(tr("Accept"), this);
    buttons->addStretch();
    buttons->addWidget(approve);
    buttons->addStretch();
    layout->addLayout(buttons);

    connect(approve, SIGNAL(clicked(bool)), this, SLOT(setConsent()));
}

void AddConsent::setConsent()
{
    consented = true;
    emit completeChanged();

    // move on if accepted
    wizard->next();
}

void AddConsent::initializePage()
{
    QStringList parts = wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Consent, "::").split("::");
    if (parts.count() < 2) document->setHtml("");
    else document->setHtml(parts.at(1));
}

//Select Cloud type
AddAuth::AddAuth(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Service Credentials "));
    setSubTitle(tr("Credentials and authorisation"));

    QFormLayout *layout = new QFormLayout;
    //layout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // input boxes
    combo = new SettingCombo(this);

    url = new QLineEdit(this);
    url->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    key = new QLineEdit(this);
    key->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    user = new QLineEdit(this);
    user->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    pass = new QLineEdit(this);
    pass->setEchoMode(QLineEdit::Password);
    pass->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    token = new QLabel(this);
    token->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    message = new QLabel(this);
    message->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // is there an icon for the authorise button?
    auth = new QPushButton(tr("Authorise"), this);
    auth->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // labels
    comboLabel = new QLabel("");
    urlLabel = new QLabel(tr("URL"));
    keyLabel = new QLabel(tr("Key (optional)"));
    userLabel = new QLabel(tr("Username"));
    passLabel = new QLabel(tr("Password"));
    authLabel = new QLabel(tr("Authorise"));
    tokenLabel = new QLabel(tr("Token"));
    messageLabel = new QLabel(tr("Message"));

    layout->addRow(comboLabel, combo);
    layout->addRow(urlLabel, url);
    layout->addRow(keyLabel, key);
    layout->addRow(userLabel, user);
    layout->addRow(passLabel, pass);
    layout->addRow(authLabel, auth);
    layout->addRow(messageLabel, message);
    layout->addRow(tokenLabel, token);

    connect(auth, SIGNAL(clicked(bool)), this, SLOT(doAuth()));

    setLayout(layout);
    setFinalPage(false);
}

void
AddAuth::doAuth()
{
    QString cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::OAuthToken, "");

    // no config for token !?
    if (cname == "") return;

    // update the service values with what the user has edited
    // so they are up-to-date before we perform an OAUTH process
    updateServiceSettings();

    if (wizard->cloudService->capabilities() & CloudService::OAuth) {
        OAuthDialog *oauthDialog = new OAuthDialog(wizard->context, OAuthDialog::NONE, wizard->cloudService);
        if (oauthDialog->sslLibMissing()) {
            delete oauthDialog;
        } else {
            oauthDialog->setWindowModality(Qt::ApplicationModal);
            oauthDialog->exec();
            token->setText(wizard->cloudService->getSetting(cname, "").toString());

            QString msg = wizard->cloudService->message;
            if (msg != "") {
                message->setText(msg);
                messageLabel->show();
                message->show();
                wizard->cloudService->message = "";
            }

            // Due to the OAuth dialog being modal, the order of the background windows can get out of order
            // This ensures the wizard is back on top
            wizard->raise();
        }
    }
}

void
AddAuth::initializePage()
{
    setSubTitle(tr("Credentials and authorisation"));

    hasAthlete = (wizard->cloudService->settings.value(CloudService::AthleteID, "") != "");

    // hide all the widgets
    combo->hide();
    url->hide();
    key->hide();
    user->hide();
    pass->hide();
    auth->hide();
    message->hide();
    token->hide();
    comboLabel->hide();
    urlLabel->hide();
    keyLabel->hide();
    userLabel->hide();
    passLabel->hide();
    authLabel->hide();
    messageLabel->hide();
    tokenLabel->hide();

    // clone to do next few steps!
    setSubTitle(QString(tr("%1 Credentials and authorisation")).arg(wizard->cloudService->uiName()));

    // icon on the authorize button
    if (wizard->cloudService && wizard->cloudService->authiconpath() != "") {

        // scaling icon hack (193x48 is strava icon size)
        QPixmap pix(wizard->cloudService->authiconpath());
        QIcon authicon(pix.scaled(193*dpiXFactor, 48*dpiXFactor));
        auth->setIconSize(QSize(193*dpiXFactor, 48*dpiYFactor));

        // set the pushbutton
        auth->setText("");
        auth->setIcon(authicon);
    } else {

        // standard pushbutton (reset after used by strava)
        auth->setText(tr("Authorise"));
        auth->setIcon(QIcon());
    }

    // show  all the widgets relevant for this service and update the value from the
    // settings we have collected (which will have been defaulted).
    QString cname;
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Combo1, "")) != "") {
        combo->show(); comboLabel->show();
        combo->setup(cname);
        combo->setText(wizard->cloudService->getSetting(cname.split("::").at(0), "").toString());
        comboLabel->setText(combo->name);
    }
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
    // just extract edited values
    updateServiceSettings();

    // always move on -- for now.
    return true;
}

void
AddAuth::updateServiceSettings()
{
    QString cname;
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Combo1, "")) != "") {
        wizard->cloudService->setSetting(cname.split("::").at(0), combo->text());
    }
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
}

//Select Athlete, if needed
AddAthlete::AddAthlete(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Coached Athletes"));
    setSubTitle(tr("Select Athlete for this account"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    buttons=new QWidget(this);
    buttons->setContentsMargins(0,0,0,0);
    buttonlayout= new  QVBoxLayout(buttons);
    buttonlayout->setSpacing(0);
    scrollarea=new QScrollArea(this);
    scrollarea->setWidgetResizable(true);
    scrollarea->setWidget(buttons);

    mapper = new QSignalMapper(this);
    connect(mapper, &QSignalMapper::mappedInt, this, &AddAthlete::clicked);

    layout->addWidget(scrollarea);

    setFinalPage(false);
}

void
AddAthlete::initializePage()
{
    athletes = wizard->cloudService->listAthletes();

    // clear whatever we have, if anything
    QLayoutItem *item = NULL;
    while((item = buttonlayout->takeAt(0)) != NULL) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    int i=0;
    foreach(CloudServiceAthlete a, athletes) {

        // only ones with the capability we need.
        QCommandLinkButton *p = new QCommandLinkButton(a.name, a.desc, this);
        p->setFixedHeight(50 *dpiYFactor);
        p->setStyleSheet(QString("font-size: %1px;").arg(12 * dpiXFactor));
        connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
        mapper->setMapping(p, i++);
        buttonlayout->addWidget(p);
    }
    buttonlayout->addStretch();
}

void
AddAthlete::clicked(int i)
{
    // select it
    wizard->cloudService->selectAthlete(athletes[i]);
    wizard->next();
}

// Scan for Cloud port / usb etc
AddSettings::AddSettings(AddCloudWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setSubTitle(tr("Cloud Service Settings"));

    QFormLayout *layout = new QFormLayout(this);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    setLayout(layout);

    metaLabel = new QLabel("none", this);
    metaCombo = new QComboBox(this);
    metaCombo->addItem("None", QVariant("")); // default "None" .. before adding the rest
    // add an entry for every single metadata field, which is a text
    foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {

        // only add text fields
        if (field.isTextField()) metaCombo->addItem(field.name, QVariant(field.name));
    }

    folderLabel = new QLabel(tr("Folder"));
    folder = new QLineEdit(this);
    folder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    folder->setReadOnly(true); // only allow folder selection via di
    browse = new QPushButton(tr("Browse"));
    syncStartup = new QCheckBox(tr("Sync on startup"));
    syncImport = new QCheckBox(tr("Sync on import"));

    QHBoxLayout *flayout = new QHBoxLayout;
    flayout->addWidget(folderLabel);
    flayout->addWidget(folder);
    flayout->addWidget(browse);
    layout->addRow(flayout);
    layout->addRow(metaLabel, metaCombo);

    layout->addRow(syncStartup); // only makes sense if the service has a query api
    layout->addRow(syncImport); // only makes sense if the service has an upload api

    connect(browse, SIGNAL(clicked()), this, SLOT(browseFolder()));
}

void
AddSettings::initializePage()
{
    setTitle(QString(tr("Service Settings")));

    // hide everything first
    metaLabel->hide();
    metaCombo->hide();
    folderLabel->hide();
    folder->hide();
    browse->hide();
    syncStartup->hide();
    syncImport->hide();

    QString cname;
    // if we need a meta field
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Metadata1, "")) != "") {
        metaCombo->setCurrentIndex(0); // default to none
        metaLabel->setText(cname.split("::").at(1)); // set name
        metaLabel->show(); metaCombo->show();
        QString current = wizard->cloudService->getSetting(cname.split("::").at(0), "").toString();
        if (current != "") {
            int index=metaCombo->findText(current);
            if (index >=0) metaCombo->setCurrentIndex(index);
        }
    }
    // if we need a folder then set that to show
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
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Metadata1, "")) != "") {
        QString meta;
        if (metaCombo->currentIndex() > 0) meta=metaCombo->itemData(metaCombo->currentIndex(), Qt::UserRole).toString();
        wizard->cloudService->setSetting(cname.split("::").at(0), meta);
    }
    if ((cname=wizard->cloudService->settings.value(CloudService::CloudServiceSetting::Folder, "")) != "") {
        wizard->cloudService->setSetting(cname, folder->text());
    }

    // generic settings, but applied on a per service basis
    wizard->cloudService->setSetting(wizard->cloudService->syncOnImportSettingName(), syncImport->isChecked() ? "true" : "false");
    wizard->cloudService->setSetting(wizard->cloudService->syncOnStartupSettingName(), syncStartup->isChecked() ? "true" : "false");
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

        // let the cloud service set any local ids etc when the
        // home directory is selected (used by google drive/kent uni)
        wizard->cloudService->folderSelected(path);
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

        QString label, value, sname=want.value();
        switch(want.key()) {
            case CloudService::URL: label=tr("URL"); break;
            case CloudService::Key: label=tr("Key"); break;
            case CloudService::Username: label=tr("Username"); break;
            case CloudService::Password: label=tr("Password"); break;
            case CloudService::OAuthToken: label=tr("Token"); break;
            case CloudService::Folder: label=tr("Folder"); break;
            case CloudService::AthleteID: label=tr("Athlete ID"); break;
            case CloudService::Combo1: label=want.value().split("::").at(1); sname=want.value().split("::").at(0); break;
            case CloudService::Metadata1: label=want.value().split("::").at(1); sname=want.value().split("::").at(0); break;
            case CloudService::Local1:
            case CloudService::Local2:
            case CloudService::Local3:
            case CloudService::Local4:
            case CloudService::Local5:
            case CloudService::Local6: label=want.value().split(QRegularExpression("[<>/]")).last(); break;
            case CloudService::Consent:
            case CloudService::DefaultURL: break;
        }
        // no clue
        if (label == "") continue;

        // get value
        value = wizard->cloudService->getSetting(sname, "").toString();
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
    // save settings away
    CloudServiceFactory::instance().saveSettings(wizard->cloudService, wizard->context);

    // this service is now active, only way to set to non active would be to delete it
    // in the athlete preferences
    appsettings->setCValue(wizard->context->athlete->cyclist, wizard->cloudService->activeSettingName(), "true");

    // start a sync straight away
    if (wizard->fsync) {
        CloudService *db = CloudServiceFactory::instance().newService(wizard->cloudService->id(), wizard->context);
        CloudServiceSyncDialog *syncnow = new CloudServiceSyncDialog(wizard->context, db);
        syncnow->open();
    }

    // delete the instance
    delete wizard->cloudService;

    return true;
}
