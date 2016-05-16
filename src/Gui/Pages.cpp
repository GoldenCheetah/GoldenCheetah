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

#include "Athlete.h"
#include <QtGui>
#include <QIntValidator>

#include <assert.h>

#include "Pages.h"
#include "Units.h"
#include "Settings.h"
#include "UserMetricParser.h"
#include "Units.h"
#include "Colors.h"
#include "AddDeviceWizard.h"
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include "ColorButton.h"
#include "SpecialFields.h"
#include "DataProcessor.h"
#include "OAuthDialog.h"
#include "RideAutoImportConfig.h"
#include "HelpWhatsThis.h"
#if QT_VERSION >= 0x050000
#include "Dropbox.h"
#include "GoogleDrive.h"
#endif
#include "LocalFileStore.h"
#include "Secrets.h"
#include "Utils.h"

//
// Main Config Page - tabs for each sub-page
//
GeneralPage::GeneralPage(Context *context) : context(context)
{
    // layout without too wide margins
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGridLayout *configLayout = new QGridLayout;
    mainLayout->addLayout(configLayout);
    mainLayout->addStretch();
    setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    configLayout->setSpacing(10);

    //
    // Language Selection
    //
    langLabel = new QLabel(tr("Language"));
    langCombo = new QComboBox();
    langCombo->addItem(tr("English"));
    langCombo->addItem(tr("French"));
    langCombo->addItem(tr("Japanese"));
    langCombo->addItem(tr("Portugese (Brazil)"));
    langCombo->addItem(tr("Italian"));
    langCombo->addItem(tr("German"));
    langCombo->addItem(tr("Russian"));
    langCombo->addItem(tr("Czech"));
    langCombo->addItem(tr("Spanish"));
    langCombo->addItem(tr("Portugese"));
    langCombo->addItem(tr("Chinese (Simplified)"));    
    langCombo->addItem(tr("Chinese (Traditional)"));

    // Default to system locale
    QVariant lang = appsettings->value(this, GC_LANG, QLocale::system().name());

    if(lang.toString().startsWith("en")) langCombo->setCurrentIndex(0);
    else if(lang.toString().startsWith("fr")) langCombo->setCurrentIndex(1);
    else if(lang.toString().startsWith("ja")) langCombo->setCurrentIndex(2);
    else if(lang.toString().startsWith("pt-br")) langCombo->setCurrentIndex(3);
    else if(lang.toString().startsWith("it")) langCombo->setCurrentIndex(4);
    else if(lang.toString().startsWith("de")) langCombo->setCurrentIndex(5);
    else if(lang.toString().startsWith("ru")) langCombo->setCurrentIndex(6);
    else if(lang.toString().startsWith("cs")) langCombo->setCurrentIndex(7);
    else if(lang.toString().startsWith("es")) langCombo->setCurrentIndex(8);
    else if(lang.toString().startsWith("pt")) langCombo->setCurrentIndex(9);
    else if(lang.toString().startsWith("zh-cn")) langCombo->setCurrentIndex(10);    
    else if (lang.toString().startsWith("zh-tw")) langCombo->setCurrentIndex(11);
    else langCombo->setCurrentIndex(0);

    configLayout->addWidget(langLabel, 0,0, Qt::AlignRight);
    configLayout->addWidget(langCombo, 0,1, Qt::AlignLeft);

    QLabel *unitlabel = new QLabel(tr("Unit"));
    unitCombo = new QComboBox();
    unitCombo->addItem(tr("Metric"));
    unitCombo->addItem(tr("Imperial"));

    if (context->athlete->useMetricUnits)
        unitCombo->setCurrentIndex(0);
    else
        unitCombo->setCurrentIndex(1);

    configLayout->addWidget(unitlabel, 1,0, Qt::AlignRight);
    configLayout->addWidget(unitCombo, 1,1, Qt::AlignLeft);

    //
    // Garmin crap
    //
    // garmin Smart Recording options
    QVariant garminHWMark = appsettings->value(this, GC_GARMIN_HWMARK);
    garminSmartRecord = new QCheckBox(tr("Use Garmin Smart Recording"), this);
    QVariant isGarminSmartRecording = appsettings->value(this, GC_GARMIN_SMARTRECORD, Qt::Checked);
    garminSmartRecord->setCheckState(isGarminSmartRecording.toInt() > 0 ? Qt::Checked : Qt::Unchecked);

    // by default, set the threshold to 25 seconds
    if (garminHWMark.isNull() || garminHWMark.toInt() == 0) garminHWMark.setValue(25);
    QLabel *garminHWLabel = new QLabel(tr("Smart Recording Threshold (secs)"));
    garminHWMarkedit = new QLineEdit(garminHWMark.toString(),this);
    garminHWMarkedit->setInputMask("009");

    configLayout->addWidget(garminSmartRecord, 2,1, Qt::AlignLeft);
    configLayout->addWidget(garminHWLabel, 3,0, Qt::AlignRight);
    configLayout->addWidget(garminHWMarkedit, 3,1, Qt::AlignLeft);

    // Elevation hysterisis  GC_ELEVATION_HYSTERISIS
    QVariant elevationHysteresis = appsettings->value(this, GC_ELEVATION_HYSTERESIS);
    if (elevationHysteresis.isNull() || elevationHysteresis.toFloat() == 0.0)
       elevationHysteresis.setValue(3.0);  // default is 1 meter

    QLabel *hystlabel = new QLabel(tr("Elevation hysteresis (meters)"));
    hystedit = new QLineEdit(elevationHysteresis.toString(),this);
    hystedit->setInputMask("9.00");

    configLayout->addWidget(hystlabel, 4,0, Qt::AlignRight);
    configLayout->addWidget(hystedit, 4,1, Qt::AlignLeft);


    // wbal formula preference
    QLabel *wbalFormLabel = new QLabel(tr("W' bal formula"));
    wbalForm = new QComboBox(this);
    wbalForm->addItem(tr("Differential"));
    wbalForm->addItem(tr("Integral"));
    if (appsettings->value(this, GC_WBALFORM, "diff").toString() == "diff") wbalForm->setCurrentIndex(0);
    else wbalForm->setCurrentIndex(1);

    configLayout->addWidget(wbalFormLabel, 5,0, Qt::AlignRight);
    configLayout->addWidget(wbalForm, 5,1, Qt::AlignLeft);


    //
    // Warn to save on exit
    warnOnExit = new QCheckBox(tr("Warn for unsaved activities on exit"), this);
    warnOnExit->setChecked(appsettings->value(NULL, GC_WARNEXIT, true).toBool());
    configLayout->addWidget(warnOnExit, 6,1, Qt::AlignLeft);

    //
    // Run API web services when running
    //
    int offset=0;
#ifdef GC_WANT_HTTP
    offset += 1;
    startHttp = new QCheckBox(tr("Enable API Web Services"), this);
    startHttp->setChecked(appsettings->value(NULL, GC_START_HTTP, true).toBool());
    configLayout->addWidget(startHttp, 7,1, Qt::AlignLeft);
#endif
#ifdef GC_WANT_R
    embedR = new QCheckBox(tr("Enable R"), this);
    embedR->setChecked(appsettings->value(NULL, GC_EMBED_R, true).toBool());
    configLayout->addWidget(embedR, 7+offset,1, Qt::AlignLeft);
    offset += 1;
    connect(embedR, SIGNAL(stateChanged(int)), this, SLOT(embedRchanged(int)));
#endif

    //
    // Athlete directory (home of athletes)
    //
    QVariant athleteDir = appsettings->value(this, GC_HOMEDIR);
    athleteLabel = new QLabel(tr("Athlete Library"));
    athleteDirectory = new QLineEdit;
    athleteDirectory->setText(athleteDir.toString() == "0" ? "" : athleteDir.toString());
    athleteWAS = athleteDirectory->text(); // remember what we started with ...
    athleteBrowseButton = new QPushButton(tr("Browse"));
    athleteBrowseButton->setFixedWidth(120);

    configLayout->addWidget(athleteLabel, 7 + offset,0, Qt::AlignRight);
    configLayout->addWidget(athleteDirectory, 7 + offset,1);
    configLayout->addWidget(athleteBrowseButton, 7 + offset,2);

    connect(athleteBrowseButton, SIGNAL(clicked()), this, SLOT(browseAthleteDir()));

    //
    // Workout directory (train view)
    //
    QVariant workoutDir = appsettings->value(this, GC_WORKOUTDIR, "");
    // fix old bug..
    if (workoutDir == "0") workoutDir = "";
    workoutLabel = new QLabel(tr("Workout and VideoSync Library"));
    workoutDirectory = new QLineEdit;
    workoutDirectory->setText(workoutDir.toString());
    workoutBrowseButton = new QPushButton(tr("Browse"));
    workoutBrowseButton->setFixedWidth(120);

    configLayout->addWidget(workoutLabel, 8 + offset,0, Qt::AlignRight);
    configLayout->addWidget(workoutDirectory, 8 + offset,1);
    configLayout->addWidget(workoutBrowseButton, 8 + offset,2);

    connect(workoutBrowseButton, SIGNAL(clicked()), this, SLOT(browseWorkoutDir()));
    offset++;

#ifdef GC_WANT_R
    //
    // R Home directory
    //
    QVariant rDir = appsettings->value(this, GC_R_HOME, "");
    // fix old bug..
    if (rDir == "0") rDir = "";
    rLabel = new QLabel(tr("R Installation Directory"));
    rDirectory = new QLineEdit;
    rDirectory->setText(rDir.toString());
    rBrowseButton = new QPushButton(tr("Browse"));
    rBrowseButton->setFixedWidth(120);

    configLayout->addWidget(rLabel, 8 + offset,0, Qt::AlignRight);
    configLayout->addWidget(rDirectory, 8 + offset,1);
    configLayout->addWidget(rBrowseButton, 8 + offset,2);
    offset++;

    connect(rBrowseButton, SIGNAL(clicked()), this, SLOT(browseRDir()));
#endif

    // save away initial values
    b4.unit = unitCombo->currentIndex();
    b4.hyst = elevationHysteresis.toFloat();
    b4.wbal = wbalForm->currentIndex();
    b4.warn = warnOnExit->isChecked();
#ifdef GC_WANT_HTTP
    b4.starthttp = startHttp->isChecked();
#endif
}

#ifdef GC_WANT_R
void
GeneralPage::embedRchanged(int state)
{
    rBrowseButton->setVisible(state);
    rDirectory->setVisible(state);
    rLabel->setVisible(state);
}
#endif

qint32
GeneralPage::saveClicked()
{
    // Language
    static const QString langs[] = {
        "en", "fr", "ja", "pt-br", "it", "de", "ru", "cs", "es", "pt", "zh-cn", "zh-tw"
    };
    appsettings->setValue(GC_LANG, langs[langCombo->currentIndex()]);

    // Garmin and cranks
    appsettings->setValue(GC_GARMIN_HWMARK, garminHWMarkedit->text().toInt());
    appsettings->setValue(GC_GARMIN_SMARTRECORD, garminSmartRecord->checkState());

    // save on exit
    appsettings->setValue(GC_WARNEXIT, warnOnExit->isChecked());

    // Directories
    appsettings->setValue(GC_WORKOUTDIR, workoutDirectory->text());
    appsettings->setValue(GC_HOMEDIR, athleteDirectory->text());
#ifdef GC_WANT_R
    appsettings->setValue(GC_R_HOME, rDirectory->text());
#endif

    // Elevation
    appsettings->setValue(GC_ELEVATION_HYSTERESIS, hystedit->text());

    // wbal formula
    appsettings->setValue(GC_WBALFORM, wbalForm->currentIndex() ? "int" : "diff");

    if (unitCombo->currentIndex()==0)
        appsettings->setValue(GC_UNIT, GC_UNIT_METRIC);
    else if (unitCombo->currentIndex()==1)
        appsettings->setValue(GC_UNIT, GC_UNIT_IMPERIAL);


#ifdef GC_WANT_HTTP
    // start http
    appsettings->setValue(GC_START_HTTP, startHttp->isChecked());
#endif
#ifdef GC_WANT_R
    appsettings->setValue(GC_EMBED_R, embedR->isChecked());
#endif

    qint32 state=0;

    // general stuff changed ?
#ifdef GC_WANT_HTTP
    if (b4.hyst != hystedit->text().toFloat() ||
        b4.starthttp != startHttp->isChecked())
#else
    if (b4.hyst != hystedit->text().toFloat())
#endif
        state += CONFIG_GENERAL;

    if (b4.wbal != wbalForm->currentIndex())
        state += CONFIG_WBAL;

    // units prefs changed?
    if (b4.unit != unitCombo->currentIndex())
        state += CONFIG_UNITS;

    return state;
}

#ifdef GC_WANT_R
void
GeneralPage::browseRDir()
{
    QString currentDir = rDirectory->text();
    if (!QDir(currentDir).exists()) currentDir = "";
    QString dir = QFileDialog::getExistingDirectory(this, tr("R Installation (R_HOME)"),
                            currentDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != "") rDirectory->setText(dir);  //only overwrite current dir, if a new was selected
}
#endif

void
GeneralPage::browseWorkoutDir()
{
    QString currentDir = workoutDirectory->text();
    if (!QDir(currentDir).exists()) currentDir = "";
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Workout Library"),
                            currentDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != "") workoutDirectory->setText(dir);  //only overwrite current dir, if a new was selected
}


void
GeneralPage::browseAthleteDir()
{
    QString currentDir = athleteDirectory->text();
    if (!QDir(currentDir).exists()) currentDir = "";
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Athlete Library"),
                            currentDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != "") athleteDirectory->setText(dir);  //only overwrite current dir, if a new was selected
}

//
// Passwords page
//
CredentialsPage::CredentialsPage(QWidget *parent, Context *context) : QScrollArea(parent), context(context)
{
    QWidget *main = new QWidget(this);
    main->setContentsMargins(0,0,0,0);

    QGridLayout *grid = new QGridLayout;
    unsigned int row = 0;

    QFont current;
    current.setWeight(QFont::Black);

    QPixmap passwords = QPixmap(":/images/toolbar/passwords.png");

    //////////////////////////////////////////////////
    // TrainingPeaks

    QLabel *tp = new QLabel(tr("TrainingPeaks"));
    tp->setFont(current);

    QLabel *userLabel = new QLabel(tr("Username"));
    QLabel *passLabel = new QLabel(tr("Password"));
    QLabel *typeLabel = new QLabel(tr("Account Type"));

    tpUser = new QLineEdit(this);
    tpUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_TPUSER, "").toString());

    tpPass = new QLineEdit(this);
    tpPass->setEchoMode(QLineEdit::Password);
    tpPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_TPPASS, "").toString());

    tpType = new QComboBox(this);
    tpType->addItem("Shared Free");
    tpType->addItem("Coached Free");
    tpType->addItem("Self Coached Premium");
    tpType->addItem("Shared Self Coached Premium");
    tpType->addItem("Coached Premium");
    tpType->addItem("Shared Coached Premium");

    tpType->setCurrentIndex(appsettings->cvalue(context->athlete->cyclist, GC_TPTYPE, "0").toInt());

    grid->addWidget(tp, row, 0);

    grid->addWidget(userLabel, ++row, 0);
    grid->addWidget(tpUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(passLabel, ++row, 0);
    grid->addWidget(tpPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(typeLabel, ++row, 0);
    grid->addWidget(tpType, row, 1, Qt::AlignLeft | Qt::AlignVCenter);


    //////////////////////////////////////////////////
    // Twitter

#ifdef GC_HAVE_KQOAUTH
    QLabel *twp = new QLabel(tr("Twitter"));
    twp->setFont(current);

    QLabel *twauthLabel = new QLabel(tr("Authorise"));

    twitterAuthorise = new QPushButton(tr("Authorise"), this);
    twitterAuthorised = new QPushButton(this);
    twitterAuthorised->setContentsMargins(0,0,0,0);
    twitterAuthorised->setIcon(passwords.scaled(16,16));
    twitterAuthorised->setIconSize(QSize(16,16));
    twitterAuthorised->setFixedHeight(16);
    twitterAuthorised->setFixedWidth(16);

    grid->addWidget(twp, ++row, 0);

    grid->addWidget(twauthLabel, ++row, 0);
    grid->addWidget(twitterAuthorise, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
    if (appsettings->cvalue(context->athlete->cyclist, GC_TWITTER_TOKEN, "")!="")
        grid->addWidget(twitterAuthorised, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
    else
        twitterAuthorised->hide(); // if no token no show

    connect(twitterAuthorise, SIGNAL(clicked()), this, SLOT(authoriseTwitter()));
#endif

#if QT_VERSION >= 0x050000 // only in QT5 or higher
    //
    // Authorising Dropbox via an OAuthDialog...
    QLabel *dwp = new QLabel(tr("Dropbox"));
    dwp->setFont(current);

    QLabel *dropauthLabel = new QLabel(tr("Authorise"));

    dropboxAuthorise = new QPushButton(tr("Authorise"), this);

    dropboxAuthorised = new QPushButton(this);
    dropboxAuthorised->setContentsMargins(0,0,0,0);
    dropboxAuthorised->setIcon(passwords.scaled(16,16));
    dropboxAuthorised->setIconSize(QSize(16,16));
    dropboxAuthorised->setFixedHeight(16);
    dropboxAuthorised->setFixedWidth(16);

    grid->addWidget(dwp, ++row, 0);

    grid->addWidget(dropauthLabel, ++row, 0);
    grid->addWidget(dropboxAuthorise, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
    if (appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_TOKEN, "")!="")
        grid->addWidget(dropboxAuthorised, row, 2, Qt::AlignLeft | Qt::AlignVCenter);
    else
        dropboxAuthorised->hide(); // if no token no show

    connect(dropboxAuthorise, SIGNAL(clicked()), this, SLOT(authoriseDropbox()));

    //
    // Selecting the athlete folder in dropbox
    QLabel *dropfolderLabel = new QLabel(tr("Athlete Folder"));
    dropboxFolder = new QLineEdit(this);
    dropboxFolder->setText(appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_FOLDER, "").toString());
    dropboxBrowse = new QPushButton(tr("Browse"));
    connect(dropboxBrowse, SIGNAL(clicked()), this, SLOT(chooseDropboxFolder()));
    QHBoxLayout *dfchoose = new QHBoxLayout;
    dfchoose->addWidget(dropboxFolder);
    dfchoose->addWidget(dropboxBrowse);
    grid->addWidget(dropfolderLabel, ++row, 0);
    grid->addLayout(dfchoose, row, 1);
    //grid->addWidget(twpinLabel, ++row, 0);

    QLabel *googleDriveLabel = new QLabel(tr("Google Drive"));
    googleDriveLabel->setFont(current);
    googleDriveFolder = new QLineEdit(this);
    googleDriveFolder->setReadOnly(true);
    googleDriveFolder->setText(
        appsettings->cvalue(context->athlete->cyclist,
                            GC_GOOGLE_DRIVE_FOLDER, "").toString());

    googleDriveAuthorise = new QPushButton(tr("Authorise"), this);
    googleDriveAuthorised = new QPushButton(this);
    googleDriveAuthorised->setContentsMargins(0, 0, 0, 0);
    googleDriveAuthorised->setIcon(passwords.scaled(16, 16));
    googleDriveAuthorised->setIconSize(QSize(16, 16));
    googleDriveAuthorised->setFixedHeight(16);
    googleDriveAuthorised->setFixedWidth(16);

    grid->addWidget(googleDriveLabel, ++row, 0);

    QHBoxLayout *gdauthlayout = new QHBoxLayout;
    //grid->addWidget(googleDriveAuthorise, row, 1,
    //Qt::AlignLeft | Qt::AlignVCenter);
    gdauthlayout->addWidget(googleDriveAuthorise);
    if (appsettings->cvalue(context->athlete->cyclist,
                            GC_GOOGLE_DRIVE_REFRESH_TOKEN, "") != "") {
        //grid->addWidget(googleDriveAuthorised, row, 2,
        //Qt::AlignLeft | Qt::AlignVCenter);
        gdauthlayout->addWidget(googleDriveAuthorised);
    } else {
        googleDriveAuthorised->hide(); // if no token no show
    }
    QComboBox *google_drive_scope = new QComboBox;
    int item = 0;
    const QString scope = GoogleDrive::GetScope(context);
    google_drive_scope->setEditable(false);
    google_drive_scope->insertItem(item++, "drive.appdata");
    google_drive_scope->insertItem(item++, "drive.file");
    google_drive_scope->insertItem(item++, "drive");
    if (scope == "drive.appdata") {
        google_drive_scope->setCurrentIndex(0);
    } else if (scope == "drive.file") {
        google_drive_scope->setCurrentIndex(1);
    } else if (scope == "drive") {
        google_drive_scope->setCurrentIndex(2);
    } else {
        google_drive_scope->setCurrentIndex(0);
        // re-write default.
        appsettings->setCValue(
            context->athlete->cyclist,
            GC_GOOGLE_DRIVE_AUTH_SCOPE, "drive.appdata");
    }
    connect(google_drive_scope, SIGNAL(currentIndexChanged(const QString&)),
            this, SLOT(chooseGoogleDriveAuthScope(const QString&)));
    gdauthlayout->addWidget(google_drive_scope);
    connect(googleDriveAuthorise, SIGNAL(clicked()), this,
            SLOT(authoriseGoogleDrive()));

    QLabel *googleDriveAuthLabel = new QLabel(tr("Authorise"));
    grid->addWidget(googleDriveAuthLabel, ++row, 0);
    grid->addLayout(gdauthlayout, row, 1);  // No stretch.
    // Selecting the athlete folder in GoogleDrive
    googleDriveBrowse = new QPushButton(tr("Browse"));
    connect(googleDriveBrowse, SIGNAL(clicked()), this,
            SLOT(chooseGoogleDriveFolder()));
    QHBoxLayout *gdfchoose = new QHBoxLayout;
    gdfchoose->addWidget(googleDriveFolder);
    gdfchoose->addWidget(googleDriveBrowse);

    QLabel *googleDriveFolderLabel = new QLabel(tr("Athlete Folder"));
    grid->addWidget(googleDriveFolderLabel, ++row, 0);
    grid->addLayout(gdfchoose, row, 1);
#endif

    //////////////////////////////////////////////////
    // Local FileStore


    QLabel *nfs = new QLabel(tr("Shared Local Folder"));
    nfs->setFont(current);
    grid->addWidget(nfs, ++row, 0);

    // Selecting the storage folder folder of the Local File Store
    QLabel *networkFileStoreFolderLabel = new QLabel(tr("Shared Local Athlete Folder"));
    networkFileStoreFolder = new QLineEdit(this);
    networkFileStoreFolder->setText(appsettings->cvalue(context->athlete->cyclist, GC_NETWORKFILESTORE_FOLDER, "").toString());
    networkFileStoreFolderBrowse = new QPushButton(tr("Browse"));
    connect(networkFileStoreFolderBrowse, SIGNAL(clicked()), this, SLOT(chooseLocalFileStoreFolder()));
    QHBoxLayout *nwfsfchoose = new QHBoxLayout;
    nwfsfchoose->addWidget(networkFileStoreFolder);
    nwfsfchoose->addWidget(networkFileStoreFolderBrowse);
    grid->addWidget(networkFileStoreFolderLabel, ++row, 0);
    grid->addLayout(nwfsfchoose, row, 1);


    //////////////////////////////////////////////////
    // Strava

    QLabel *str = new QLabel(tr("Strava"));
    str->setFont(current);

    QLabel *strauthLabel = new QLabel(tr("Authorise"));

    stravaAuthorise = new QPushButton(tr("Authorise"), this);

    stravaAuthorised = new QPushButton(this);
    stravaAuthorised->setContentsMargins(0,0,0,0);
    stravaAuthorised->setIcon(passwords.scaled(16,16));
    stravaAuthorised->setIconSize(QSize(16,16));
    stravaAuthorised->setFixedHeight(16);
    stravaAuthorised->setFixedWidth(16);

    grid->addWidget(str, ++row, 0);

    grid->addWidget(strauthLabel, ++row, 0);
    grid->addWidget(stravaAuthorise, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
    if (appsettings->cvalue(context->athlete->cyclist, GC_STRAVA_TOKEN, "")!="")
        grid->addWidget(stravaAuthorised, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
    else
        stravaAuthorised->hide(); // if no token no show

    connect(stravaAuthorise, SIGNAL(clicked()), this, SLOT(authoriseStrava()));

    //////////////////////////////////////////////////
    // Cycling Analytics


    QLabel *can = new QLabel(tr("Cycling Analytics"));
    can->setFont(current);
    QLabel *canauthLabel = new QLabel(tr("Authorise"));

    cyclingAnalyticsAuthorise = new QPushButton(tr("Authorise"), this);
#ifndef GC_CYCLINGANALYTICS_CLIENT_SECRET
    cyclingAnalyticsAuthorise->setEnabled(false);
#endif

    cyclingAnalyticsAuthorised = new QPushButton(this);
    cyclingAnalyticsAuthorised->setContentsMargins(0,0,0,0);
    cyclingAnalyticsAuthorised->setIcon(passwords.scaled(16,16));
    cyclingAnalyticsAuthorised->setIconSize(QSize(16,16));
    cyclingAnalyticsAuthorised->setFixedHeight(16);
    cyclingAnalyticsAuthorised->setFixedWidth(16);

    grid->addWidget(can, ++row, 0);

    grid->addWidget(canauthLabel, ++row, 0);
    grid->addWidget(cyclingAnalyticsAuthorise, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
    if (appsettings->cvalue(context->athlete->cyclist, GC_CYCLINGANALYTICS_TOKEN, "")!="")
        grid->addWidget(cyclingAnalyticsAuthorised, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
    else
        cyclingAnalyticsAuthorised->hide();

    connect(cyclingAnalyticsAuthorise, SIGNAL(clicked()), this, SLOT(authoriseCyclingAnalytics()));


    //////////////////////////////////////////////////
    // RideWithGPS

    QLabel *rwgps = new QLabel(tr("RideWithGPS"));
    rwgps->setFont(current);

    QLabel *rwgpsuserLabel = new QLabel(tr("Username"));
    QLabel *rwgpspassLabel = new QLabel(tr("Password"));

    rideWithGPSUser = new QLineEdit(this);
    rideWithGPSUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_RWGPSUSER, "").toString());

    rideWithGPSPass = new QLineEdit(this);
    rideWithGPSPass->setEchoMode(QLineEdit::Password);
    rideWithGPSPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_RWGPSPASS, "").toString());

    grid->addWidget(rwgps, ++row, 0);

    grid->addWidget(rwgpsuserLabel, ++row, 0);
    grid->addWidget(rideWithGPSUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(rwgpspassLabel, ++row, 0);
    grid->addWidget(rideWithGPSPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    //////////////////////////////////////////////////
    // Withings Wifi Scales

    QLabel *wip = new QLabel(tr("Withings Wifi Scales"));
    wip->setFont(current);

    QLabel *wiurlLabel = new QLabel(tr("Website"));
    QLabel *wiuserLabel = new QLabel(tr("User Id"));
    QLabel *wipassLabel = new QLabel(tr("Public Key"));

    wiURL = new QLineEdit(this);
    wiURL->setText(appsettings->cvalue(context->athlete->cyclist, GC_WIURL, "http://wbsapi.withings.net/").toString());

    wiUser = new QLineEdit(this);
    wiUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_WIUSER, "").toString());

    wiPass = new QLineEdit(this);
    wiPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_WIKEY, "").toString());

    grid->addWidget(wip, ++row, 0);

    grid->addWidget(wiurlLabel, ++row, 0);
    grid->addWidget(wiURL, row, 1, 0);

    grid->addWidget(wiuserLabel, ++row, 0);
    grid->addWidget(wiUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(wipassLabel, ++row, 0);
    grid->addWidget(wiPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    //////////////////////////////////////////////////
    // Web Calendar

    QLabel *webcal = new QLabel(tr("Web Calendar"));
    webcal->setFont(current);
    QLabel *wcurlLabel = new QLabel(tr("Webcal URL"));

    webcalURL = new QLineEdit(this);
    webcalURL->setText(appsettings->cvalue(context->athlete->cyclist, GC_WEBCAL_URL, "").toString());

    grid->addWidget(webcal, ++row, 0);

    grid->addWidget(wcurlLabel, ++row, 0);
    grid->addWidget(webcalURL, row, 1, 0);

    //////////////////////////////////////////////////
    // CalDAV Calendar

    QLabel *dv = new QLabel(tr("CalDAV Calendar"));
    dvCALDAVType = new QComboBox(this);
    dvCALDAVType->addItem(tr("Generic CalDAV"));
    dvCALDAVType->addItem(tr("Google Calendar"));

    dvCALDAVType->setCurrentIndex(appsettings->cvalue(context->athlete->cyclist, GC_DVCALDAVTYPE, "0").toInt());

    // setup and fill all fields
    dv->setFont(current);
    QLabel *dvurlLabel = new QLabel(tr("CalDAV URL"));
    QLabel *dvuserLabel = new QLabel(tr("CalDAV User Id"));
    QLabel *dvpassLabel = new QLabel(tr("CalDAV Password"));
    QLabel *dvTypeLabel = new QLabel(tr("Calendar Type"));
    QLabel *dvGoogleCalendarLabel = new QLabel(tr("Google CalID"));

    dvURL = new QLineEdit(this);
    QString url = appsettings->cvalue(context->athlete->cyclist, GC_DVURL, "").toString();
    url.replace("%40", "@"); // remove escape of @ character
    dvURL->setText(url);

    dvUser = new QLineEdit(this);
    dvUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_DVUSER, "").toString());

    dvPass = new QLineEdit(this);
    dvPass->setEchoMode(QLineEdit::Password);
    dvPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_DVPASS, "").toString());

    dvGoogleCalid = new QLineEdit(this);
    dvGoogleCalid->setText(
        appsettings->cvalue(context->athlete->cyclist, GC_DVGOOGLE_CALID, "")
        .toString());

    QLabel *googleCalendarAuthLabel = new QLabel(tr("Google Calendar"));

    googleCalendarAuthorise = new QPushButton(tr("Authorise"), this);
    googleCalendarAuthorised = new QPushButton(this);
    googleCalendarAuthorised->setContentsMargins(0,0,0,0);
    googleCalendarAuthorised->setIcon(passwords.scaled(16,16));
    googleCalendarAuthorised->setIconSize(QSize(16,16));
    googleCalendarAuthorised->setFixedHeight(16);
    googleCalendarAuthorised->setFixedWidth(16);

#if QT_VERSION >= 0x050000 // only in QT5 or higher
#endif

    grid->addWidget(dv, ++row, 0);

    grid->addWidget(dvTypeLabel, ++row, 0);
    grid->addWidget(dvCALDAVType, row, 1, 0);

    grid->addWidget(dvurlLabel, ++row, 0);
    grid->addWidget(dvURL, row, 1, 0);

    grid->addWidget(dvuserLabel, ++row, 0);
    grid->addWidget(dvUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(dvpassLabel, ++row, 0);
    grid->addWidget(dvPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(dvGoogleCalendarLabel, ++row, 0);
    grid->addWidget(dvGoogleCalid, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(googleCalendarAuthLabel, ++row, 0);
    grid->addWidget(googleCalendarAuthorise, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
    if (appsettings->cvalue(context->athlete->cyclist,
                            GC_GOOGLE_CALENDAR_REFRESH_TOKEN, "") != "") {
        grid->addWidget(googleCalendarAuthorised, row, 1,
                        Qt::AlignLeft | Qt::AlignVCenter);
    } else {
        googleCalendarAuthorised->hide(); // if no token no show
    }
    connect(googleCalendarAuthorise, SIGNAL(clicked()), this,
                SLOT(authoriseGoogleCalendar()));

    connect (dvCALDAVType, SIGNAL(currentIndexChanged(int)), this, SLOT(dvCALDAVTypeChanged(int)));

    // activate/deactivate the input fields according to the type selected
    dvCALDAVTypeChanged(dvCALDAVType->currentIndex());

    //////////////////////////////////////////////////
    // Trainingstagebuch

    QLabel *ttb = new QLabel(tr("Trainingstagebuch"));
    ttb->setFont(current);

    QLabel *ttbuserLabel = new QLabel(tr("Username"));
    QLabel *ttbpassLabel = new QLabel(tr("Password"));

    ttbUser = new QLineEdit(this);
    ttbUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_TTBUSER, "").toString());

    ttbPass = new QLineEdit(this);
    ttbPass->setEchoMode(QLineEdit::Password);
    ttbPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_TTBPASS, "").toString());

    grid->addWidget(ttb, ++row, 0);

    grid->addWidget(ttbuserLabel, ++row, 0);
    grid->addWidget(ttbUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(ttbpassLabel, ++row, 0);
    grid->addWidget(ttbPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    //////////////////////////////////////////////////
    // Selfloops

    QLabel *sel = new QLabel(tr("Selfloops"));
    sel->setFont(current);

    QLabel *seluserLabel = new QLabel(tr("Username"));
    QLabel *selpassLabel = new QLabel(tr("Password"));

    selUser = new QLineEdit(this);
    selUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_SELUSER, "").toString());

    selPass = new QLineEdit(this);
    selPass->setEchoMode(QLineEdit::Password);
    selPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_SELPASS, "").toString());

    grid->addWidget(sel, ++row, 0);

    grid->addWidget(seluserLabel, ++row, 0);
    grid->addWidget(selUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(selpassLabel, ++row, 0);
    grid->addWidget(selPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    //////////////////////////////////////////////////
    // Velo Hero

    QLabel *velohero = new QLabel(tr("Velo Hero"));
    velohero->setFont(current);

    QLabel *veloherouserLabel = new QLabel(tr("Username"));
    QLabel *veloheropassLabel = new QLabel(tr("Password"));

    veloHeroUser = new QLineEdit(this);
    veloHeroUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_VELOHEROUSER, "").toString());

    veloHeroPass = new QLineEdit(this);
    veloHeroPass->setEchoMode(QLineEdit::Password);
    veloHeroPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_VELOHEROPASS, "").toString());

    grid->addWidget(velohero, ++row, 0);

    grid->addWidget(veloherouserLabel, ++row, 0);
    grid->addWidget(veloHeroUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(veloheropassLabel, ++row, 0);
    grid->addWidget(veloHeroPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);


    //////////////////////////////////////////////////
    // SportPlusHealth

    QLabel *sph = new QLabel(tr("SportPlusHealth"));
    sph->setFont(current);

    QLabel *sphUserLabel = new QLabel(tr("Username"));
    QLabel *sphPassLabel = new QLabel(tr("Password"));

    sphUser = new QLineEdit(this);
    sphUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_SPORTPLUSHEALTHUSER, "").toString());

    sphPass = new QLineEdit(this);
    sphPass->setEchoMode(QLineEdit::Password);
    sphPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_SPORTPLUSHEALTHPASS, "").toString());

    grid->addWidget(sph, ++row, 0);

    grid->addWidget(sphUserLabel, ++row, 0);
    grid->addWidget(sphUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(sphPassLabel, ++row, 0);
    grid->addWidget(sphPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    //////////////////////////////////////////////////
    // End of grid

    grid->setColumnStretch(0,0);
    grid->setColumnStretch(1,3);

    main->setLayout(grid);

    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
    setWidget(main);

}

#if QT_VERSION >= 0x050000 // only in QT5 or higher
void CredentialsPage::chooseDropboxFolder()
{
    Dropbox dropbox(context);

    // open the connection
    QStringList errors;
    if (dropbox.open(errors) == false) {
        QMessageBox err;
        err.setText(tr("Dropbox Connection Failed"));
        err.setDetailedText(errors.join("\n\n"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    // did the user type something ?
    QString path = dropboxFolder->text();
    if (path == "") {
        path = appsettings->cvalue(context->athlete->cyclist, GC_DROPBOX_FOLDER,
                                   "/").toString();
    }
    FileStoreDialog dialog(this, &dropbox, tr("Choose Athlete Directory"), path, true);
    int ret = dialog.exec();

    // did we actually select something?
    if (ret == QDialog::Accepted) dropboxFolder->setText(dialog.pathnameSelected());
}

void CredentialsPage::chooseGoogleDriveFolder()
{
    GoogleDrive google_drive(context);

    // open the connection
    QStringList errors;
    if (google_drive.open(errors) == false) {
        QMessageBox err;
        err.setText(tr("Google Drive Connection Failed"));
        err.setDetailedText(errors.join("\n\n"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }
    // did the user type something ?
    QString path = googleDriveFolder->text();
    if (path == "") {
        path = appsettings->cvalue(
            context->athlete->cyclist, GC_GOOGLE_DRIVE_FOLDER, "/").toString();
        // At this point there just might be a google drive instance around?
    }
    FileStoreDialog dialog(this, &google_drive, tr("Choose Athlete Directory"),
                           path, true);
    int ret = dialog.exec();

    // did we actually select something?
    if (ret == QDialog::Accepted) {
        path = dialog.pathnameSelected();
        googleDriveFolder->setText(path);
        QString id = google_drive.GetFileId(path);
        appsettings->setCValue(context->athlete->cyclist,
                               GC_GOOGLE_DRIVE_FOLDER_ID, id);
    }
}

void CredentialsPage::chooseGoogleDriveAuthScope(const QString& scope) {
    appsettings->setCValue(context->athlete->cyclist,
                           GC_GOOGLE_DRIVE_AUTH_SCOPE, scope);
    // Clear out now invalid access tokens.
    appsettings->setCValue(context->athlete->cyclist,
                           GC_GOOGLE_DRIVE_ACCESS_TOKEN, "");
    appsettings->setCValue(context->athlete->cyclist,
                           GC_GOOGLE_DRIVE_REFRESH_TOKEN, "");
}
#endif


void CredentialsPage::chooseLocalFileStoreFolder()
{
    // did the user type something ? if not, get it from the Settings
    QString path = networkFileStoreFolder->text();
    if (path == "") path = appsettings->cvalue(context->athlete->cyclist, GC_NETWORKFILESTORE_FOLDER, "").toString();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose Shared Local Folder Athlete Directory"),
                            path, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != "") networkFileStoreFolder->setText(dir);  //only overwrite current dir, if a new was selected

}


#ifdef GC_HAVE_KQOAUTH
void CredentialsPage::authoriseTwitter()
{
    OAuthDialog *oauthDialog = new OAuthDialog(context, OAuthDialog::TWITTER);
    if (oauthDialog->sslLibMissing()) {
        delete oauthDialog;
    } else {
        oauthDialog->setWindowModality(Qt::ApplicationModal);
        oauthDialog->exec();
    }
}
#endif

#if QT_VERSION >= 0x050000 // only in QT5 or higher
void CredentialsPage::authoriseDropbox()
{
    OAuthDialog *oauthDialog = new OAuthDialog(context, OAuthDialog::DROPBOX);
    if (oauthDialog->sslLibMissing()) {
        delete oauthDialog;
    } else {
        oauthDialog->setWindowModality(Qt::ApplicationModal);
        oauthDialog->exec();
    }
}
#endif


void CredentialsPage::authoriseStrava()
{
    OAuthDialog *oauthDialog = new OAuthDialog(context, OAuthDialog::STRAVA);
    if (oauthDialog->sslLibMissing()) {
        delete oauthDialog;
    } else {
        oauthDialog->setWindowModality(Qt::ApplicationModal);
        oauthDialog->exec();
    }
}

void CredentialsPage::authoriseCyclingAnalytics()
{
    OAuthDialog *oauthDialog = new OAuthDialog(context, OAuthDialog::CYCLING_ANALYTICS);
    if (oauthDialog->sslLibMissing()) {
        delete oauthDialog;
    } else {
        oauthDialog->setWindowModality(Qt::ApplicationModal);
        oauthDialog->exec();
    }
}

void CredentialsPage::authoriseGoogleCalendar() {
    authoriseGoogle(CALENDAR);
}

#if QT_VERSION >= 0x050000
void CredentialsPage::authoriseGoogleDrive() {
    authoriseGoogle(DRIVE);
}
#endif

void CredentialsPage::authoriseGoogle(GoogleType type) {
    OAuthDialog *oauthDialog;
    if (type == CALENDAR) {
        oauthDialog = new OAuthDialog(context, OAuthDialog::GOOGLE_CALENDAR);
    } else if (type == DRIVE) {
        oauthDialog = new OAuthDialog(context, OAuthDialog::GOOGLE_DRIVE);
    } else {
            return;
    }
    if (oauthDialog->sslLibMissing()) {
        delete oauthDialog;
    } else {
        oauthDialog->setWindowModality(Qt::ApplicationModal);
        oauthDialog->exec();
    }
}

void CredentialsPage::dvCALDAVTypeChanged(int type)
{
    if (type == 0) {
        // normal CALDAV selected
        googleCalendarAuthorise->setEnabled(false);
        dvGoogleCalid->setEnabled(false);
        dvURL->setEnabled(true);
        dvUser->setEnabled(true);
        dvPass->setEnabled(true);
    } else {
        // Google Selected
        googleCalendarAuthorise->setEnabled(true);
        dvGoogleCalid->setEnabled(true);
        dvURL->setEnabled(false);
        dvUser->setEnabled(false);
        dvPass->setEnabled(false);
    }
    // set always inactive if authorization process not feasible
#ifndef GC_GOOGLE_CALENDAR_CLIENT_SECRET
        googleCalendarAuthorise->setEnabled(false);
#endif

}

qint32
CredentialsPage::saveClicked()
{
    appsettings->setCValue(context->athlete->cyclist, GC_TPUSER, tpUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_TPPASS, tpPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_RWGPSUSER, rideWithGPSUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_RWGPSPASS, rideWithGPSPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_TTBUSER, ttbUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_TTBPASS, ttbPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_VELOHEROUSER, veloHeroUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_VELOHEROPASS, veloHeroPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SPORTPLUSHEALTHUSER, sphUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SPORTPLUSHEALTHPASS, sphPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SELUSER, selUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SELPASS, selPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_TPTYPE, tpType->currentIndex());
    appsettings->setCValue(context->athlete->cyclist, GC_WIURL, wiURL->text());
    appsettings->setCValue(context->athlete->cyclist, GC_WIUSER, wiUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_WIKEY, wiPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_WEBCAL_URL, webcalURL->text());
    appsettings->setCValue(context->athlete->cyclist, GC_WEBCAL_URL, webcalURL->text());
#if QT_VERSION >= 0x050000 // only in QT5 or higher
    appsettings->setCValue(context->athlete->cyclist, GC_DROPBOX_FOLDER, dropboxFolder->text());
    appsettings->setCValue(context->athlete->cyclist, GC_GOOGLE_DRIVE_FOLDER,
                           googleDriveFolder->text());
#endif
    appsettings->setCValue(context->athlete->cyclist, GC_NETWORKFILESTORE_FOLDER, networkFileStoreFolder->text());

    // escape the at character
    QString url = dvURL->text();
    url.replace("@", "%40");
    appsettings->setCValue(context->athlete->cyclist, GC_DVURL, url);
    appsettings->setCValue(context->athlete->cyclist, GC_DVUSER, dvUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_DVPASS, dvPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_DVGOOGLE_CALID, dvGoogleCalid->text());
    appsettings->setCValue(context->athlete->cyclist, GC_DVCALDAVTYPE, dvCALDAVType->currentIndex());

    return 0;
}

//
// About me
//
AboutRiderPage::AboutRiderPage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    QVBoxLayout *all = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout;
#ifdef Q_OS_MAX
    setContentsMargins(10,10,10,10);
    grid->setSpacing(5);
    all->setSpacing(5);
#endif

    QLabel *nicklabel = new QLabel(tr("Nickname"));
    QLabel *doblabel = new QLabel(tr("Date of Birth"));
    QLabel *sexlabel = new QLabel(tr("Sex"));

    nickname = new QLineEdit(this);
    nickname->setText(appsettings->cvalue(context->athlete->cyclist, GC_NICKNAME, "").toString());
    if (nickname->text() == "0") nickname->setText("");

    dob = new QDateEdit(this);
    dob->setDate(appsettings->cvalue(context->athlete->cyclist, GC_DOB).toDate());

    sex = new QComboBox(this);
    sex->addItem(tr("Male"));
    sex->addItem(tr("Female"));


    // we set to 0 or 1 for male or female since this
    // is language independent (and for once the code is easier!)
    sex->setCurrentIndex(appsettings->cvalue(context->athlete->cyclist, GC_SEX).toInt());

    if (QFileInfo(context->athlete->home->config().canonicalPath() + "/" + "avatar.png").exists())
        avatar = QPixmap(context->athlete->home->config().canonicalPath() + "/" + "avatar.png");
    else
        avatar = QPixmap(":/images/noavatar.png");

    avatarButton = new QPushButton(this);
    avatarButton->setContentsMargins(0,0,0,0);
    avatarButton->setFlat(true);
    avatarButton->setIcon(avatar.scaled(140,140));
    avatarButton->setIconSize(QSize(140,140));
    avatarButton->setFixedHeight(140);
    avatarButton->setFixedWidth(140);

    //
    // Crank length - only used by PfPv chart (should move there!)
    //
    QLabel *crankLengthLabel = new QLabel(tr("Crank Length"));
    QVariant crankLength = appsettings->cvalue(context->athlete->cyclist, GC_CRANKLENGTH);
    crankLengthCombo = new QComboBox();
    crankLengthCombo->addItem("150");
    crankLengthCombo->addItem("155");
    crankLengthCombo->addItem("160");
    crankLengthCombo->addItem("162.5");
    crankLengthCombo->addItem("165");
    crankLengthCombo->addItem("167.5");
    crankLengthCombo->addItem("170");
    crankLengthCombo->addItem("172.5");
    crankLengthCombo->addItem("175");
    crankLengthCombo->addItem("177.5");
    crankLengthCombo->addItem("180");
    crankLengthCombo->addItem("182.5");
    crankLengthCombo->addItem("185");
    if(crankLength.toString() == "150") crankLengthCombo->setCurrentIndex(0);
    if(crankLength.toString() == "155") crankLengthCombo->setCurrentIndex(1);
    if(crankLength.toString() == "160") crankLengthCombo->setCurrentIndex(2);
    if(crankLength.toString() == "162.5") crankLengthCombo->setCurrentIndex(3);
    if(crankLength.toString() == "165") crankLengthCombo->setCurrentIndex(4);
    if(crankLength.toString() == "167.5") crankLengthCombo->setCurrentIndex(5);
    if(crankLength.toString() == "170") crankLengthCombo->setCurrentIndex(6);
    if(crankLength.toString() == "172.5") crankLengthCombo->setCurrentIndex(7);
    if(crankLength.toString() == "175") crankLengthCombo->setCurrentIndex(8);
    if(crankLength.toString() == "177.5") crankLengthCombo->setCurrentIndex(9);
    if(crankLength.toString() == "180") crankLengthCombo->setCurrentIndex(10);
    if(crankLength.toString() == "182.5") crankLengthCombo->setCurrentIndex(11);
    if(crankLength.toString() == "185") crankLengthCombo->setCurrentIndex(12);

    //
    // Wheel size
    //
    QLabel *wheelSizeLabel = new QLabel(tr("Wheelsize"), this);
    int wheelSize = appsettings->cvalue(context->athlete->cyclist, GC_WHEELSIZE, 2100).toInt();

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

    connect(rimSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(calcWheelSize()));
    connect(tireSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(calcWheelSize()));
    connect(wheelSizeEdit, SIGNAL(textEdited(QString)), this, SLOT(resetWheelSize()));


    //
    // Auto Backup
    //
    // Selecting the storage folder folder of the Local File Store
    QLabel *autoBackupFolderLabel = new QLabel(tr("Auto Backup Folder"));
    autoBackupFolder = new QLineEdit(this);
    autoBackupFolder->setText(appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_FOLDER, "").toString());
    autoBackupFolderBrowse = new QPushButton(tr("Browse"));
    connect(autoBackupFolderBrowse, SIGNAL(clicked()), this, SLOT(chooseAutoBackupFolder()));
    autoBackupPeriod = new QSpinBox(this);
    autoBackupPeriod->setMinimum(0);
    autoBackupPeriod->setMaximum(9999);
    autoBackupPeriod->setSingleStep(1);
    QLabel *autoBackupPeriodLabel = new QLabel(tr("Auto Backup execution every"));
    autoBackupPeriod->setValue(appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_PERIOD, 0).toInt());
    QLabel *autoBackupUnitLabel = new QLabel(tr("times the athlete is closed - 0 means never"));
    QHBoxLayout *backupInput = new QHBoxLayout();
    backupInput->addWidget(autoBackupPeriod);
    //backupInput->addStretch();
    backupInput->addWidget(autoBackupUnitLabel);

    Qt::Alignment alignment = Qt::AlignLeft|Qt::AlignVCenter;

    grid->addWidget(nicklabel, 0, 0, alignment);
    grid->addWidget(doblabel, 1, 0, alignment);
    grid->addWidget(sexlabel, 2, 0, alignment);

    grid->addWidget(nickname, 0, 1, alignment);
    grid->addWidget(dob, 1, 1, alignment);
    grid->addWidget(sex, 2, 1, alignment);

    grid->addWidget(crankLengthLabel, 4, 0, alignment);
    grid->addWidget(crankLengthCombo, 4, 1, alignment);
    grid->addWidget(wheelSizeLabel, 5, 0, alignment);
    grid->addLayout(wheelSizeLayout, 5, 1, 1, 2, alignment);
    grid->addWidget(autoBackupFolderLabel, 7,0, alignment);
    grid->addWidget(autoBackupFolder, 7, 1, alignment);
    grid->addWidget(autoBackupFolderBrowse, 7, 2, alignment);
    grid->addWidget(autoBackupPeriodLabel, 8, 0,alignment);
    grid->addLayout(backupInput, 8, 1, alignment);

    grid->addWidget(avatarButton, 0, 1, 4, 2, Qt::AlignRight|Qt::AlignVCenter);
    all->addLayout(grid);
    all->addStretch();

    // save initial values for things we care about
    // note we don't worry about age or sex at this point
    // since they are not used, nor the W'bal tau used in
    // the realtime code. This is limited to stuff we
    // care about tracking as it is used by metrics

    // height and weight as stored (always metric)
    b4.wheel = wheelSize;
    b4.crank = crankLengthCombo->currentIndex();

    connect (avatarButton, SIGNAL(clicked()), this, SLOT(chooseAvatar()));
}

void
AboutRiderPage::chooseAvatar()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Choose Picture"),
                            "", tr("Images (*.png *.jpg *.bmp)"));
    if (filename != "") {

        avatar = QPixmap(filename);
        avatarButton->setIcon(avatar.scaled(140,140));
        avatarButton->setIconSize(QSize(140,140));
    }
}

void AboutRiderPage::chooseAutoBackupFolder()
{
    // did the user type something ? if not, get it from the Settings
    QString path = autoBackupFolder->text();
    if (path == "") path = appsettings->cvalue(context->athlete->cyclist, GC_AUTOBACKUP_FOLDER, "").toString();
    QString dir = QFileDialog::getExistingDirectory(this, tr("Choose Backup Directory"),
                            path, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != "") autoBackupFolder->setText(dir);  //only overwrite current dir, if a new was selected

}

void
RiderPhysPage::unitChanged(int currentIndex)
{
    if (currentIndex == 0) {
        QString weighttext = QString(tr("Weight (%1)")).arg(tr("kg"));
        weightlabel->setText(weighttext);
        weight->setValue(weight->value() / LB_PER_KG);

        QString heighttext = QString(tr("Height (%1)")).arg(tr("cm"));
        heightlabel->setText(heighttext);
        height->setValue(height->value() * CM_PER_INCH);
    }
    else {
        QString weighttext = QString(tr("Weight (%1)")).arg(tr("lb"));
        weightlabel->setText(weighttext);
        weight->setValue(weight->value() * LB_PER_KG);

        QString heighttext = QString(tr("Height (%1)")).arg(tr("in"));
        heightlabel->setText(heighttext);
        height->setValue(height->value() / CM_PER_INCH);
    }
}

void
AboutRiderPage::calcWheelSize()
{
   int diameter = WheelSize::calcPerimeter(rimSizeCombo->currentIndex(), tireSizeCombo->currentIndex());
   if (diameter>0)
        wheelSizeEdit->setText(QString("%1").arg(diameter));
}

void
AboutRiderPage::resetWheelSize()
{
   rimSizeCombo->setCurrentIndex(0);
   tireSizeCombo->setCurrentIndex(0);
}

qint32
AboutRiderPage::saveClicked()
{
    appsettings->setCValue(context->athlete->cyclist, GC_NICKNAME, nickname->text());
    appsettings->setCValue(context->athlete->cyclist, GC_DOB, dob->date());


    appsettings->setCValue(context->athlete->cyclist, GC_SEX, sex->currentIndex());
    avatar.save(context->athlete->home->config().canonicalPath() + "/" + "avatar.png", "PNG");

    appsettings->setCValue(context->athlete->cyclist, GC_CRANKLENGTH, crankLengthCombo->currentText());
    appsettings->setCValue(context->athlete->cyclist, GC_WHEELSIZE, wheelSizeEdit->text().toInt());

    // Auto Backup
    appsettings->setCValue(context->athlete->cyclist, GC_AUTOBACKUP_FOLDER, autoBackupFolder->text());
    appsettings->setCValue(context->athlete->cyclist, GC_AUTOBACKUP_PERIOD, autoBackupPeriod->value());


    qint32 state=0;

    // general stuff changed ?
    if (b4.wheel != wheelSizeEdit->text().toInt() ||
        b4.crank != crankLengthCombo->currentIndex() )
        state += CONFIG_GENERAL;

    return state;
}

//
// About me - physiology settings
//
RiderPhysPage::RiderPhysPage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    QVBoxLayout *all = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout;
#ifdef Q_OS_MAX
    setContentsMargins(10,10,10,10);
    grid->setSpacing(5);
    all->setSpacing(5);
#endif

    QString weighttext = QString(tr("Weight (%1)")).arg(context->athlete->useMetricUnits ? tr("kg") : tr("lb"));
    weightlabel = new QLabel(weighttext);

    QString heighttext = QString(tr("Height (%1)")).arg(context->athlete->useMetricUnits ? tr("cm") : tr("in"));
    heightlabel = new QLabel(heighttext);

    wbaltaulabel = new QLabel(tr("W'bal tau (s)"));

    weight = new QDoubleSpinBox(this);
    weight->setMaximum(999.9);
    weight->setMinimum(0.0);
    weight->setDecimals(1);
    weight->setValue(appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble() * (context->athlete->useMetricUnits ? 1.0 : LB_PER_KG));

    height = new QDoubleSpinBox(this);
    height->setMaximum(999.9);
    height->setMinimum(0.0);
    height->setDecimals(1);
    height->setValue(appsettings->cvalue(context->athlete->cyclist, GC_HEIGHT).toDouble() * (context->athlete->useMetricUnits ? 100.0 : 100.0/CM_PER_INCH));

    wbaltau = new QSpinBox(this);
    wbaltau->setMinimum(30);
    wbaltau->setMaximum(1200);
    wbaltau->setSingleStep(10);
    wbaltau->setValue(appsettings->cvalue(context->athlete->cyclist, GC_WBALTAU, 300).toInt());

    //
    // Performance manager
    //

    perfManSTSLabel = new QLabel(tr("STS average (days)"));
    perfManLTSLabel = new QLabel(tr("LTS average (days)"));
    perfManSTSavgValidator = new QIntValidator(1,21,this);
    perfManLTSavgValidator = new QIntValidator(7,56,this);

    // get config or set to defaults
    QVariant perfManSTSVal = appsettings->cvalue(context->athlete->cyclist, GC_STS_DAYS);
    if (perfManSTSVal.isNull() || perfManSTSVal.toInt() == 0) perfManSTSVal = 7;
    QVariant perfManLTSVal = appsettings->cvalue(context->athlete->cyclist, GC_LTS_DAYS);
    if (perfManLTSVal.isNull() || perfManLTSVal.toInt() == 0) perfManLTSVal = 42;

    perfManSTSavg = new QLineEdit(perfManSTSVal.toString(),this);
    perfManSTSavg->setValidator(perfManSTSavgValidator);
    perfManLTSavg = new QLineEdit(perfManLTSVal.toString(),this);
    perfManLTSavg->setValidator(perfManLTSavgValidator);

    showSBToday = new QCheckBox(tr("PMC Stress Balance Today"), this);
    showSBToday->setChecked(appsettings->cvalue(context->athlete->cyclist, GC_SB_TODAY).toInt());

    Qt::Alignment alignment = Qt::AlignLeft|Qt::AlignVCenter;

    grid->addWidget(weightlabel, 1, 0, alignment);
    grid->addWidget(heightlabel, 2, 0, alignment);
    grid->addWidget(wbaltaulabel, 3, 0, alignment);

    grid->addWidget(weight, 1, 1, alignment);
    grid->addWidget(height, 2, 1, alignment);
    grid->addWidget(wbaltau, 3, 1, alignment);


    grid->addWidget(perfManSTSLabel, 4, 0, alignment);
    grid->addWidget(perfManSTSavg, 4, 1, alignment);
    grid->addWidget(perfManLTSLabel, 5, 0, alignment);
    grid->addWidget(perfManLTSavg, 5, 1, alignment);
    grid->addWidget(showSBToday, 6, 1, alignment);

    all->addLayout(grid);
    all->addStretch();

    // save initial values for things we care about
    // note we don't worry about age or sex at this point
    // since they are not used, nor the W'bal tau used in
    // the realtime code. This is limited to stuff we
    // care about tracking as it is used by metrics

    // height and weight as stored (always metric)
    b4.height = appsettings->cvalue(context->athlete->cyclist, GC_HEIGHT).toDouble();
    b4.weight = appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble();
    b4.lts = perfManLTSVal.toInt();
    b4.sts = perfManSTSVal.toInt();
}

qint32
RiderPhysPage::saveClicked()
{
    // it will be updated by the general page, but context->athlete->useMetricUnits
    // will not have been applied yet, so we need to honour the PENDING setting rather
    // than the value in context->athlete->useMetricUnits
    QVariant unit = appsettings->value(NULL, GC_UNIT, GC_UNIT_METRIC);
    bool metricUnits = (unit.toString() == GC_UNIT_METRIC);

    appsettings->setCValue(context->athlete->cyclist, GC_WEIGHT, weight->value() * (metricUnits ? 1.0 : KG_PER_LB));
    appsettings->setCValue(context->athlete->cyclist, GC_HEIGHT, height->value() * (metricUnits ? 1.0/100.0 : CM_PER_INCH/100.0));
    appsettings->setCValue(context->athlete->cyclist, GC_WBALTAU, wbaltau->value());


    // Performance Manager
    appsettings->setCValue(context->athlete->cyclist, GC_STS_DAYS, perfManSTSavg->text());
    appsettings->setCValue(context->athlete->cyclist, GC_LTS_DAYS, perfManLTSavg->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SB_TODAY, (int) showSBToday->isChecked());

    qint32 state=0;

    // PMC constants changed ?
    if(b4.lts != perfManLTSavg->text().toInt() ||
        b4.sts != perfManSTSavg->text().toInt())
        state += CONFIG_PMC;

    // default height or weight changed ?
    if (b4.height != appsettings->cvalue(context->athlete->cyclist, GC_HEIGHT).toDouble() ||
        b4.weight != appsettings->cvalue(context->athlete->cyclist, GC_WEIGHT).toDouble()) {
        state += CONFIG_ATHLETE;
    }

    return state;
}

//
// Realtime devices page
//
DevicePage::DevicePage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    DeviceTypes all;
    devices = all.getList();

    addButton = new QPushButton(tr("+"),this);
    delButton = new QPushButton(tr("-"),this);

    deviceList = new QTableView(this);
#ifdef Q_OS_MAC
    addButton->setText(tr("Add"));
    delButton->setText(tr("Delete"));
    deviceList->setAttribute(Qt::WA_MacShowFocusRect, 0);
#else
    addButton->setFixedSize(20,20);
    delButton->setFixedSize(20,20);
#endif
    deviceListModel = new deviceModel(this);

    // replace standard model with ours
    QItemSelectionModel *stdmodel = deviceList->selectionModel();
    deviceList->setModel(deviceListModel);
    delete stdmodel;

    deviceList->setSortingEnabled(false);
    deviceList->setSelectionBehavior(QAbstractItemView::SelectRows);
    deviceList->horizontalHeader()->setStretchLastSection(true);
    deviceList->verticalHeader()->hide();
    deviceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deviceList->setSelectionMode(QAbstractItemView::SingleSelection);

    autoConnect = new QCheckBox(tr("Auto-connect devices in Train View"), this);
    autoConnect->setChecked(appsettings->value(this, TRAIN_AUTOCONNECT, false).toBool());

    multiCheck = new QCheckBox(tr("Allow multiple devices in Train View"), this);
    multiCheck->setChecked(appsettings->value(this, TRAIN_MULTI, false).toBool());

    mainLayout->addWidget(deviceList);
    QHBoxLayout *bottom = new QHBoxLayout;
    bottom->setSpacing(2);
    bottom->addWidget(multiCheck);
    bottom->addWidget(autoConnect);
    bottom->addStretch();
    bottom->addWidget(addButton);
    bottom->addWidget(delButton);
    mainLayout->addLayout(bottom);

    connect(addButton, SIGNAL(clicked()), this, SLOT(devaddClicked()));
    connect(delButton, SIGNAL(clicked()), this, SLOT(devdelClicked()));
    connect(context, SIGNAL(configChanged(qint32)), deviceListModel, SLOT(doReset()));
}

qint32
DevicePage::saveClicked()
{
    // Save the device configuration...
    DeviceConfigurations all;
    all.writeConfig(deviceListModel->Configuration);
    appsettings->setValue(TRAIN_MULTI, multiCheck->isChecked());
    appsettings->setValue(TRAIN_AUTOCONNECT, autoConnect->isChecked());

    return 0;
}

void
DevicePage::devaddClicked()
{
    DeviceConfiguration add;
    AddDeviceWizard *p = new AddDeviceWizard(context);
    p->show();
}

void
DevicePage::devdelClicked()
{
    deviceListModel->del();
}

// add a new configuration
void
deviceModel::add(DeviceConfiguration &newone)
{
    insertRows(0,1, QModelIndex());

    // insert name
    QModelIndex index = deviceModel::index(0,0, QModelIndex());
    setData(index, newone.name, Qt::EditRole);

    // insert type
    index = deviceModel::index(0,1, QModelIndex());
    setData(index, newone.type, Qt::EditRole);

    // insert portSpec
    index = deviceModel::index(0,2, QModelIndex());
    setData(index, newone.portSpec, Qt::EditRole);

    // insert Profile
    index = deviceModel::index(0,3, QModelIndex());
    setData(index, newone.deviceProfile, Qt::EditRole);

    // insert postProcess
    index = deviceModel::index(0,4, QModelIndex());
    setData(index, newone.postProcess, Qt::EditRole);
}

// delete an existing configuration
void
deviceModel::del()
{
    // which row is selected in the table?
    DevicePage *temp = static_cast<DevicePage*>(parent);
    QItemSelectionModel *selectionModel = temp->deviceList->selectionModel();

    QModelIndexList indexes = selectionModel->selectedRows();
    QModelIndex index;

    foreach (index, indexes) {
        //int row = this->mapToSource(index).row();
        removeRows(index.row(), 1, QModelIndex());
    }
}

deviceModel::deviceModel(QObject *parent) : QAbstractTableModel(parent)
{
    this->parent = parent;

    // get current configuration
    DeviceConfigurations all;
    Configuration = all.getList();
}

void
deviceModel::doReset()
{
    beginResetModel();
    DeviceConfigurations all;
    Configuration = all.getList();
    endResetModel();
}

int
deviceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return Configuration.size();
}

int
deviceModel::columnCount(const QModelIndex &) const
{
    return 5;
}


// setup the headings!
QVariant deviceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
     if (role != Qt::DisplayRole) return QVariant(); // no display, no game!

     if (orientation == Qt::Horizontal) {
         switch (section) {
             case 0:
                 return tr("Device Name");
             case 1:
                 return tr("Device Type");
             case 2:
                 return tr("Port Spec");
             case 3:
                 return tr("Profile");
             case 4:
                 return tr("Virtual");
             default:
                 return QVariant();
         }
     }
     return QVariant();
 }

// return data item for row/col specified in index
QVariant deviceModel::data(const QModelIndex &index, int role) const
{
     if (!index.isValid()) return QVariant();
     if (index.row() >= Configuration.size() || index.row() < 0) return QVariant();

     if (role == Qt::DisplayRole) {
         DeviceConfiguration Entry = Configuration.at(index.row());

         switch(index.column()) {
            case 0 : return Entry.name;
                break;
            case 1 :
                {
                DeviceTypes all;
                DeviceType lookupType = all.getType (Entry.type);
                return lookupType.name;
                }
                break;
            case 2 :
                return Entry.portSpec;
                break;
            case 3 :
                return Entry.deviceProfile;
            case 4 :
                return Entry.postProcess;
         }
     }

     // how did we get here!?
     return QVariant();
}

// update the model with new data
bool deviceModel::insertRows(int position, int rows, const QModelIndex &index)
{
     Q_UNUSED(index);
     beginInsertRows(QModelIndex(), position, position+rows-1);

     for (int row=0; row < rows; row++) {
         DeviceConfiguration emptyEntry;
         Configuration.insert(position, emptyEntry);
     }
     endInsertRows();
     return true;
}

// delete a row!
bool deviceModel::removeRows(int position, int rows, const QModelIndex &index)
{
     Q_UNUSED(index);
     beginRemoveRows(QModelIndex(), position, position+rows-1);

     for (int row=0; row < rows; ++row) {
         Configuration.removeAt(position);
     }

     endRemoveRows();
     return true;
}

bool deviceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
        if (index.isValid() && role == Qt::EditRole) {
            int row = index.row();

            DeviceConfiguration p = Configuration.value(row);

            switch (index.column()) {
                case 0 : //name
                    p.name = value.toString();
                    break;
                case 1 : //type
                    p.type = value.toInt();
                    break;
                case 2 : // spec
                    p.portSpec = value.toString();
                    break;
                case 3 : // Profile
                    p.deviceProfile = value.toString();
                    break;
                case 4 : // Profile
                    p.postProcess = value.toInt();
                    break;
            }
            Configuration.replace(row,p);
                emit(dataChanged(index, index));

            return true;
        }

        return false;
}

//
// Remote control page
//
RemotePage::RemotePage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    remote = new RemoteControl;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    fields = new QTreeWidget;
    fields->headerItem()->setText(0, tr("Action"));
    fields->headerItem()->setText(1, tr("ANT+ Command"));
    fields->setColumnWidth(0,100);
    fields->setColumnWidth(1,200);
    fields->setColumnCount(2);
    fields->setSelectionMode(QAbstractItemView::SingleSelection);
    fields->setIndentation(0);

    fields->setCurrentItem(fields->invisibleRootItem()->child(0));

    mainLayout->addWidget(fields, 0,0);

    // Load the native command list
    QList <RemoteCmd> nativeCmds = remote->getNativeCmds();

    // Load the ant command list
    QList<RemoteCmd> antCmds = remote->getAntCmds();

    // Load the remote control mappings
    QList<CmdMap> cmdMaps = remote->getMappings();

    // create a row for each native command
    int index = 0;
    foreach (RemoteCmd nativeCmd, nativeCmds) {

        QComboBox *comboBox = new QComboBox(this);
        comboBox->addItem("<unset>");

        // populate the combo box with all possible ANT commands
        foreach(RemoteCmd antCmd, antCmds) {
            comboBox->addItem(antCmd.getCmdStr());
        }

        // is this native command mapped to an ANT command?
        foreach(CmdMap cmdMapping, cmdMaps) {
            if (cmdMapping.getNativeCmdId() == nativeCmd.getCmdId()) {
                if (cmdMapping.getAntCmdId() != 0xFFFF) {
                    //qDebug() << "ANT remote mapping found:" << cmdMapping.getNativeCmdId() << cmdMapping.getAntCmdId();

                    int i=0;
                    foreach(RemoteCmd antCmd, antCmds) {
                        i++; // increment first to skip <unset>
                        if (cmdMapping.getAntCmdId() == antCmd.getCmdId()) {
                            // set the default entry for the combo box
                            comboBox->setCurrentIndex(i);
                        }
                    }
                }
            }
        }

        QTreeWidgetItem *add = new QTreeWidgetItem;
        fields->invisibleRootItem()->insertChild(index, add);
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        add->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        add->setText(0, nativeCmd.getDisplayStr());

        add->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
        fields->setItemWidget(add, 1, comboBox);
        index++;
    }
}

qint32
RemotePage::saveClicked()
{
    // Save the remote control code mappings...

    //qDebug() << "RemotePage::saveClicked()";

    // Load the ant command list
    QList<RemoteCmd> antCmds = remote->getAntCmds();

    // Load the remote control mappings
    QList<CmdMap> cmdMaps = remote->getMappings();

    for(int i=0; i < cmdMaps.size(); i++) {

        QWidget *button = fields->itemWidget(fields->invisibleRootItem()->child(i),1);
        int index = ((QComboBox*)button)->currentIndex();

        if (index) {
            cmdMaps[i].setAntCmdId(antCmds[index-1].getCmdId());

        } else {
            cmdMaps[i].setAntCmdId(0xFFFF); // no command
        }
    }

    remote->writeConfig(cmdMaps);
    return 0;
}

static void setSizes(QComboBox *p)
{
#ifdef Q_OS_MAC
    p->addItem("7");
    p->addItem("9");
    p->addItem("11");
    p->addItem("13");
    p->addItem("15");
    p->addItem("17");
    p->addItem("19");
    p->addItem("21");
#else
    p->addItem("6");
    p->addItem("8");
    p->addItem("10");
    p->addItem("12");
    p->addItem("14");
    p->addItem("16");
    p->addItem("18");
    p->addItem("20");
#endif
}

//
// Appearances page
//
ColorsPage::ColorsPage(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // chrome style -- metal or flat currently offered
    QLabel *chromeLabel = new QLabel(tr("Styling" ));
    chromeCombo = new QComboBox(this);
    chromeCombo->addItem(tr("Metallic (Mac)"));
    chromeCombo->addItem(tr("Flat Color (Windows)"));
#ifdef Q_OS_MAC // gets reset on yosemite anyway
    QString chrome = appsettings->value(this, GC_CHROME, "Mac").toString();
#else
    QString chrome = appsettings->value(this, GC_CHROME, "Flat").toString();
#endif
    if (chrome == "Mac") chromeCombo->setCurrentIndex(0);
    if (chrome == "Flat") chromeCombo->setCurrentIndex(1);

    themes = new QTreeWidget;
    themes->headerItem()->setText(0, tr("Swatch"));
    themes->headerItem()->setText(1, tr("Name"));
    themes->setColumnCount(2);
    themes->setColumnWidth(0,240);
    themes->setSelectionMode(QAbstractItemView::SingleSelection);
    //colors->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    themes->setUniformRowHeights(true); // causes height problems when adding - in case of non-text fields
    themes->setIndentation(0);
    //colors->header()->resizeSection(0,300);

    colors = new QTreeWidget;
    colors->headerItem()->setText(0, tr("Color"));
    colors->headerItem()->setText(1, tr("Select"));
    colors->setColumnCount(2);
    colors->setColumnWidth(0,350);
    colors->setSelectionMode(QAbstractItemView::NoSelection);
    //colors->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    colors->setUniformRowHeights(true); // causes height problems when adding - in case of non-text fields
    colors->setIndentation(0);
    //colors->header()->resizeSection(0,300);

    QLabel *antiAliasLabel = new QLabel(tr("Antialias"));
    antiAliased = new QCheckBox;
    antiAliased->setChecked(appsettings->value(this, GC_ANTIALIAS, true).toBool());
#ifndef Q_OS_MAC
    QLabel *rideScrollLabel = new QLabel(tr("Activity Scrollbar"));
    rideScroll = new QCheckBox;
    rideScroll->setChecked(appsettings->value(this, GC_RIDESCROLL, true).toBool());
    QLabel *rideHeadLabel = new QLabel(tr("Activity Headings"));
    rideHead = new QCheckBox;
    rideHead->setChecked(appsettings->value(this, GC_RIDEHEAD, true).toBool());
#endif
    lineWidth = new QDoubleSpinBox;
    lineWidth->setMaximum(5);
    lineWidth->setMinimum(0.5);
    lineWidth->setSingleStep(0.5);
    applyTheme = new QPushButton(tr("Apply Theme"));
    lineWidth->setValue(appsettings->value(this, GC_LINEWIDTH, 0.5).toDouble());

    QLabel *lineWidthLabel = new QLabel(tr("Line Width"));
    QLabel *defaultLabel = new QLabel(tr("Default"));
    QLabel *titlesLabel = new QLabel(tr("Title" ));
    QLabel *markerLabel = new QLabel(tr("Chart Markers" ));
    QLabel *chartLabel = new QLabel(tr("Chart Labels" ));
    QLabel *calendarLabel = new QLabel(tr("Calendar Text" ));

    def = new QFontComboBox(this);
    titles = new QFontComboBox(this);
    chartmarkers = new QFontComboBox(this);
    chartlabels = new QFontComboBox(this);
    calendar = new QFontComboBox(this);

    defaultSize = new QComboBox(this); setSizes(defaultSize);
    titlesSize = new QComboBox(this); setSizes(titlesSize);
    chartmarkersSize = new QComboBox(this); setSizes(chartmarkersSize);
    chartlabelsSize = new QComboBox(this); setSizes(chartlabelsSize);
    calendarSize = new QComboBox(this); setSizes(calendarSize);

    // get round QTBUG
    def->setCurrentIndex(0);
    def->setCurrentIndex(1);
    def->setCurrentFont(QFont());
    titles->setCurrentIndex(0);
    titles->setCurrentIndex(1);
    titles->setCurrentFont(QFont());
    chartmarkers->setCurrentIndex(0);
    chartmarkers->setCurrentIndex(1);
    chartmarkers->setCurrentFont(QFont());
    chartlabels->setCurrentIndex(0);
    chartlabels->setCurrentIndex(1);
    chartlabels->setCurrentFont(QFont());
    calendar->setCurrentIndex(0);
    calendar->setCurrentIndex(1);
    calendar->setCurrentFont(QFont());

    QFont font;

    font.fromString(appsettings->value(this, GC_FONT_DEFAULT, QFont().toString()).toString());
    def->setCurrentFont(font);

    font.fromString(appsettings->value(this, GC_FONT_TITLES, QFont().toString()).toString());
    titles->setCurrentFont(font);

    font.fromString(appsettings->value(this, GC_FONT_CHARTMARKERS, QFont().toString()).toString());
    chartmarkers->setCurrentFont(font);

    font.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    chartlabels->setCurrentFont(font);

    font.fromString(appsettings->value(this, GC_FONT_CALENDAR, QFont().toString()).toString());
    calendar->setCurrentFont(font);

#ifdef Q_OS_MAC
    defaultSize->setCurrentIndex((appsettings->value(this, GC_FONT_DEFAULT_SIZE, 10).toInt() -7) / 2);
    titlesSize->setCurrentIndex((appsettings->value(this, GC_FONT_TITLES_SIZE, 10).toInt() -7) / 2);
    chartmarkersSize->setCurrentIndex((appsettings->value(this, GC_FONT_CHARTMARKERS_SIZE, 8).toInt() -7) / 2);
    chartlabelsSize->setCurrentIndex((appsettings->value(this, GC_FONT_CHARTLABELS_SIZE, 8).toInt() -7) / 2);
    calendarSize->setCurrentIndex((appsettings->value(this, GC_FONT_CALENDAR_SIZE, 8).toInt() -7) / 2);
#else
    defaultSize->setCurrentIndex((appsettings->value(this, GC_FONT_DEFAULT_SIZE, 10).toInt() -6) / 2);
    titlesSize->setCurrentIndex((appsettings->value(this, GC_FONT_TITLES_SIZE, 10).toInt() -6) / 2);
    chartmarkersSize->setCurrentIndex((appsettings->value(this, GC_FONT_CHARTMARKERS_SIZE, 8).toInt() -6) / 2);
    chartlabelsSize->setCurrentIndex((appsettings->value(this, GC_FONT_CHARTLABELS_SIZE, 8).toInt() -6) / 2);
    calendarSize->setCurrentIndex((appsettings->value(this, GC_FONT_CALENDAR_SIZE, 8).toInt() -6) / 2);
#endif

    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(5);

    grid->addWidget(defaultLabel, 0,0);
    grid->addWidget(titlesLabel, 1,0);

    grid->addWidget(lineWidthLabel, 0,3);
    grid->addWidget(lineWidth, 0,4);
    grid->addWidget(antiAliasLabel, 1,3);
    grid->addWidget(antiAliased, 1,4);
#ifndef Q_OS_MAC
    grid->addWidget(rideScrollLabel, 2,3);
    grid->addWidget(rideScroll, 2,4);
    grid->addWidget(rideHeadLabel, 3,3);
    grid->addWidget(rideHead, 3,4);
#endif

    grid->addWidget(markerLabel, 2,0);
    grid->addWidget(chartLabel, 3,0);
    grid->addWidget(calendarLabel, 4,0);

    grid->addWidget(def, 0,1, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(titles, 1,1, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(chartmarkers, 2,1, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(chartlabels, 3,1, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(calendar, 4,1, Qt::AlignVCenter|Qt::AlignLeft);

    grid->addWidget(defaultSize, 0,2, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(titlesSize, 1,2, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(chartmarkersSize, 2,2, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(chartlabelsSize, 3,2, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(calendarSize, 4,2, Qt::AlignVCenter|Qt::AlignLeft);

    grid->addWidget(chromeLabel, 5, 0);
    grid->addWidget(chromeCombo, 5, 1, Qt::AlignVCenter|Qt::AlignLeft);

    grid->setColumnStretch(0,1);
    grid->setColumnStretch(1,4);
    grid->setColumnStretch(2,1);
    grid->setColumnStretch(3,1);
    grid->setColumnStretch(4,4);

    mainLayout->addLayout(grid);

    colorTab = new QTabWidget(this);
    colorTab->addTab(themes, tr("Theme"));
    colorTab->addTab(colors, tr("Colors"));
    colorTab->setCornerWidget(applyTheme);

    mainLayout->addWidget(colorTab);

    colorSet = GCColor::colorSet();
    for (int i=0; colorSet[i].name != ""; i++) {

        QTreeWidgetItem *add;
        ColorButton *colorButton = new ColorButton(this, colorSet[i].name, colorSet[i].color);
        add = new QTreeWidgetItem(colors->invisibleRootItem());
        add->setText(0, colorSet[i].name);
        colors->setItemWidget(add, 1, colorButton);

    }
    connect(applyTheme, SIGNAL(clicked()), this, SLOT(applyThemeClicked()));

    foreach(ColorTheme theme, GCColor::themes().themes) {

        QTreeWidgetItem *add;
        ColorLabel *swatch = new ColorLabel(theme);
        swatch->setFixedHeight(30);
        add = new QTreeWidgetItem(themes->invisibleRootItem());
        themes->setItemWidget(add, 0, swatch);
        add->setText(1, theme.name);

    }
    connect(colorTab, SIGNAL(currentChanged(int)), this, SLOT(tabChanged()));

    // save initial values
    b4.alias = antiAliased->isChecked();
#ifndef Q_OS_MAC
    b4.scroll = rideScroll->isChecked();
    b4.head = rideHead->isChecked();
#endif
    b4.line = lineWidth->value();
    b4.chrome = chromeCombo->currentIndex();
    b4.fingerprint = Colors::fingerprint(colorSet);
}

void
ColorsPage::tabChanged()
{
    // are we on the them page
    if (colorTab->currentIndex() == 0) applyTheme->show();
    else applyTheme->hide();
}

void
ColorsPage::applyThemeClicked()
{
    int index=0;

    // first check we have a selection!
    if (themes->currentItem() && (index=themes->invisibleRootItem()->indexOfChild(themes->currentItem())) >= 0) {

        // now get the theme selected
        ColorTheme theme = GCColor::themes().themes[index];

        // reset to base
        colorSet = GCColor::defaultColorSet();

        // reset the color selection tools
        colors->clear();
        for (int i=0; colorSet[i].name != ""; i++) {

            QColor color;

            // apply theme to color
            switch(i) {

            case CPLOTBACKGROUND:
            case CRIDEPLOTBACKGROUND:
            case CTRENDPLOTBACKGROUND:
            case CTRAINPLOTBACKGROUND:
                color = theme.colors[0]; // background color
                break;

            // fg color theme.colors[1] not used YET XXX

            case CPLOTSYMBOL:
            case CRIDEPLOTXAXIS:
            case CRIDEPLOTYAXIS:
            case CPLOTMARKER:
                color = theme.colors[2]; // accent color
                break;

            case CPLOTSELECT:
            case CPLOTTRACKER:
            case CINTERVALHIGHLIGHTER:
                color = theme.colors[3]; // select color
                break;


            case CPLOTGRID: // grid doesn't have a theme color
                            // we make it barely distinguishable from background
                {
                    QColor bg = theme.colors[0];
                    if(bg == QColor(Qt::black)) color = QColor(30,30,30);
                    else color = bg.darker(110);
                }
                break;

            case CCP:
            case CWBAL:
            case CRIDECP:
                color = theme.colors[4];
                break;

            case CHEARTRATE:
                color = theme.colors[5];
                break;

            case CSPEED:
                color = theme.colors[6];
                break;

            case CPOWER:
                color = theme.colors[7];
                break;

            case CCADENCE:
                color = theme.colors[8];
                break;

            case CTORQUE:
                color = theme.colors[9];
                break;

                default:
                    color = colorSet[i].color;
            }

            QTreeWidgetItem *add;
            ColorButton *colorButton = new ColorButton(this, colorSet[i].name, color);
            add = new QTreeWidgetItem(colors->invisibleRootItem());
            add->setText(0, colorSet[i].name);
            colors->setItemWidget(add, 1, colorButton);

        }
    }
}

qint32
ColorsPage::saveClicked()
{
    // chrome style only has 2 types for now
    switch(chromeCombo->currentIndex()) {
    default:
    case 0:
        appsettings->setValue(GC_CHROME, "Mac");
#ifdef Q_OS_MAC
        // if on yosemite set ALWAYS do flat chrome
        if (QSysInfo::MacintoshVersion == 12) {
            appsettings->setValue(GC_CHROME, "Flat");
        }
#endif
        break;
    case 1:
        appsettings->setValue(GC_CHROME, "Flat");
        break;
    }

    appsettings->setValue(GC_LINEWIDTH, lineWidth->value());
    appsettings->setValue(GC_ANTIALIAS, antiAliased->isChecked());
#ifndef Q_OS_MAC
    appsettings->setValue(GC_RIDESCROLL, rideScroll->isChecked());
    appsettings->setValue(GC_RIDEHEAD, rideHead->isChecked());
#endif

    // run down and get the current colors and save
    for (int i=0; colorSet[i].name != ""; i++) {
        QTreeWidgetItem *current = colors->invisibleRootItem()->child(i);
        QColor newColor = ((ColorButton*)colors->itemWidget(current, 1))->getColor();
        QString colorstring = QString("%1:%2:%3").arg(newColor.red())
                                                 .arg(newColor.green())
                                                 .arg(newColor.blue());
        appsettings->setValue(colorSet[i].setting, colorstring);
    }
    // Font
    appsettings->setValue(GC_FONT_DEFAULT, def->currentFont().toString());
    appsettings->setValue(GC_FONT_TITLES, titles->currentFont().toString());
    appsettings->setValue(GC_FONT_CHARTMARKERS, chartmarkers->currentFont().toString());
    appsettings->setValue(GC_FONT_CHARTLABELS, chartlabels->currentFont().toString());
    appsettings->setValue(GC_FONT_CALENDAR, calendar->currentFont().toString());
#ifdef Q_OS_MAC
    appsettings->setValue(GC_FONT_DEFAULT_SIZE, 7+(defaultSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_TITLES_SIZE, 7+(titlesSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CHARTMARKERS_SIZE, 7+(chartmarkersSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CHARTLABELS_SIZE, 7+(chartlabelsSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CALENDAR_SIZE, 7+(calendarSize->currentIndex()*2));
#else
    appsettings->setValue(GC_FONT_DEFAULT_SIZE, 6+(defaultSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_TITLES_SIZE, 6+(titlesSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CHARTMARKERS_SIZE, 6+(chartmarkersSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CHARTLABELS_SIZE, 6+(chartlabelsSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CALENDAR_SIZE, 6+(calendarSize->currentIndex()*2));
#endif

    QFont font;
    font.fromString(appsettings->value(this, GC_FONT_DEFAULT, QFont().toString()).toString());
    font.setPointSize(appsettings->value(this, GC_FONT_DEFAULT_SIZE, 13).toInt());
    QApplication::setFont(font);

    // reread into colorset so we can check for changes
    GCColor::readConfig();

    // did we change anything ?
    if(b4.alias != antiAliased->isChecked() ||
#ifndef Q_OS_MAC // not needed on mac
       b4.scroll != rideScroll->isChecked() ||
       b4.head != rideHead->isChecked() ||
#endif
       b4.line != lineWidth->value() ||
       b4.chrome != chromeCombo->currentIndex() ||
       b4.fingerprint != Colors::fingerprint(colorSet))
        return CONFIG_APPEARANCE;
    else
        return 0;
}

IntervalMetricsPage::IntervalMetricsPage(QWidget *parent) :
    QWidget(parent), changed(false)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Metrics_Intervals));

    availList = new QListWidget;
    availList->setSortingEnabled(true);
    availList->setSelectionMode(QAbstractItemView::SingleSelection);
    QVBoxLayout *availLayout = new QVBoxLayout;
    availLayout->addWidget(new QLabel(tr("Available Metrics")));
    availLayout->addWidget(availList);
    selectedList = new QListWidget;
    selectedList->setSelectionMode(QAbstractItemView::SingleSelection);
    QVBoxLayout *selectedLayout = new QVBoxLayout;
    selectedLayout->addWidget(new QLabel(tr("Selected Metrics")));
    selectedLayout->addWidget(selectedList);
#ifndef Q_OS_MAC
    upButton = new QToolButton(this);
    downButton = new QToolButton(this);
    leftButton = new QToolButton(this);
    rightButton = new QToolButton(this);
    upButton->setArrowType(Qt::UpArrow);
    downButton->setArrowType(Qt::DownArrow);
    leftButton->setArrowType(Qt::LeftArrow);
    rightButton->setArrowType(Qt::RightArrow);
    upButton->setFixedSize(20,20);
    downButton->setFixedSize(20,20);
    leftButton->setFixedSize(20,20);
    rightButton->setFixedSize(20,20);
#else
    upButton = new QPushButton(tr("Up"));
    downButton = new QPushButton(tr("Down"));
    leftButton = new QPushButton("<");
    rightButton = new QPushButton(">");
#endif
    QVBoxLayout *buttonGrid = new QVBoxLayout;
    QHBoxLayout *upLayout = new QHBoxLayout;
    QHBoxLayout *inexcLayout = new QHBoxLayout;
    QHBoxLayout *downLayout = new QHBoxLayout;

    upLayout->addStretch();
    upLayout->addWidget(upButton);
    upLayout->addStretch();

    inexcLayout->addStretch();
    inexcLayout->addWidget(leftButton);
    inexcLayout->addWidget(rightButton);
    inexcLayout->addStretch();

    downLayout->addStretch();
    downLayout->addWidget(downButton);
    downLayout->addStretch();

    buttonGrid->addStretch();
    buttonGrid->addLayout(upLayout);
    buttonGrid->addLayout(inexcLayout);
    buttonGrid->addLayout(downLayout);
    buttonGrid->addStretch();

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addLayout(availLayout);
    hlayout->addLayout(buttonGrid);
    hlayout->addLayout(selectedLayout);
    setLayout(hlayout);

    QString s;
    if (appsettings->contains(GC_SETTINGS_INTERVAL_METRICS))
        s = appsettings->value(this, GC_SETTINGS_INTERVAL_METRICS).toString();
    else
        s = GC_SETTINGS_INTERVAL_METRICS_DEFAULT;
    QStringList selectedMetrics = s.split(",");

    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i = 0; i < factory.metricCount(); ++i) {
        QString symbol = factory.metricName(i);
        if (selectedMetrics.contains(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QListWidgetItem *item = new QListWidgetItem(Utils::unprotect(m->name()));
        item->setData(Qt::UserRole, symbol);
        item->setToolTip(m->description());
        availList->addItem(item);
    }
    foreach (QString symbol, selectedMetrics) {
        if (!factory.haveMetric(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QListWidgetItem *item = new QListWidgetItem(Utils::unprotect(m->name()));
        item->setData(Qt::UserRole, symbol);
        item->setToolTip(m->description());
        selectedList->addItem(item);
    }

    upButton->setEnabled(false);
    downButton->setEnabled(false);
    leftButton->setEnabled(false);
    rightButton->setEnabled(false);

    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(leftButton, SIGNAL(clicked()), this, SLOT(leftClicked()));
    connect(rightButton, SIGNAL(clicked()), this, SLOT(rightClicked()));
    connect(availList, SIGNAL(itemSelectionChanged()),
            this, SLOT(availChanged()));
    connect(selectedList, SIGNAL(itemSelectionChanged()),
            this, SLOT(selectedChanged()));
}

void
IntervalMetricsPage::upClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    assert(row > 0);
    selectedList->takeItem(row);
    selectedList->insertItem(row - 1, item);
    selectedList->setCurrentItem(item);
    changed = true;
}

void
IntervalMetricsPage::downClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    assert(row < selectedList->count() - 1);
    selectedList->takeItem(row);
    selectedList->insertItem(row + 1, item);
    selectedList->setCurrentItem(item);
    changed = true;
}

void
IntervalMetricsPage::leftClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    selectedList->takeItem(selectedList->row(item));
    availList->addItem(item);
    changed = true;
    selectedChanged();
}

void
IntervalMetricsPage::rightClicked()
{
    assert(!availList->selectedItems().isEmpty());
    QListWidgetItem *item = availList->selectedItems().first();
    availList->takeItem(availList->row(item));
    selectedList->addItem(item);
    changed = true;
}

void
IntervalMetricsPage::availChanged()
{
    rightButton->setEnabled(!availList->selectedItems().isEmpty());
}

void
IntervalMetricsPage::selectedChanged()
{
    if (selectedList->selectedItems().isEmpty()) {
        upButton->setEnabled(false);
        downButton->setEnabled(false);
        leftButton->setEnabled(false);
        return;
    }
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    if (row == 0)
        upButton->setEnabled(false);
    else
        upButton->setEnabled(true);
    if (row == selectedList->count() - 1)
        downButton->setEnabled(false);
    else
        downButton->setEnabled(true);
    leftButton->setEnabled(true);
}

qint32
IntervalMetricsPage::saveClicked()
{
    if (!changed) return 0;

    QStringList metrics;
    for (int i = 0; i < selectedList->count(); ++i)
        metrics << selectedList->item(i)->data(Qt::UserRole).toString();
    appsettings->setValue(GC_SETTINGS_INTERVAL_METRICS, metrics.join(","));

    return 0;
}

BestsMetricsPage::BestsMetricsPage(QWidget *parent) :
    QWidget(parent), changed(false)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Metrics_Best));

    availList = new QListWidget;
    availList->setSortingEnabled(true);
    availList->setSelectionMode(QAbstractItemView::SingleSelection);
    QVBoxLayout *availLayout = new QVBoxLayout;
    availLayout->addWidget(new QLabel(tr("Available Metrics")));
    availLayout->addWidget(availList);
    selectedList = new QListWidget;
    selectedList->setSelectionMode(QAbstractItemView::SingleSelection);
    QVBoxLayout *selectedLayout = new QVBoxLayout;
    selectedLayout->addWidget(new QLabel(tr("Selected Metrics")));
    selectedLayout->addWidget(selectedList);
#ifndef Q_OS_MAC
    upButton = new QToolButton(this);
    downButton = new QToolButton(this);
    leftButton = new QToolButton(this);
    rightButton = new QToolButton(this);
    upButton->setArrowType(Qt::UpArrow);
    downButton->setArrowType(Qt::DownArrow);
    leftButton->setArrowType(Qt::LeftArrow);
    rightButton->setArrowType(Qt::RightArrow);
    upButton->setFixedSize(20,20);
    downButton->setFixedSize(20,20);
    leftButton->setFixedSize(20,20);
    rightButton->setFixedSize(20,20);
#else
    upButton = new QPushButton(tr("Up"));
    downButton = new QPushButton(tr("Down"));
    leftButton = new QPushButton("<");
    rightButton = new QPushButton(">");
#endif
    QVBoxLayout *buttonGrid = new QVBoxLayout;
    QHBoxLayout *upLayout = new QHBoxLayout;
    QHBoxLayout *inexcLayout = new QHBoxLayout;
    QHBoxLayout *downLayout = new QHBoxLayout;

    upLayout->addStretch();
    upLayout->addWidget(upButton);
    upLayout->addStretch();

    inexcLayout->addStretch();
    inexcLayout->addWidget(leftButton);
    inexcLayout->addWidget(rightButton);
    inexcLayout->addStretch();

    downLayout->addStretch();
    downLayout->addWidget(downButton);
    downLayout->addStretch();

    buttonGrid->addStretch();
    buttonGrid->addLayout(upLayout);
    buttonGrid->addLayout(inexcLayout);
    buttonGrid->addLayout(downLayout);
    buttonGrid->addStretch();

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addLayout(availLayout);
    hlayout->addLayout(buttonGrid);
    hlayout->addLayout(selectedLayout);
    setLayout(hlayout);

    QString s;
    if (appsettings->contains(GC_SETTINGS_BESTS_METRICS))
        s = appsettings->value(this, GC_SETTINGS_BESTS_METRICS).toString();
    else
        s = GC_SETTINGS_BESTS_METRICS_DEFAULT;
    QStringList selectedMetrics = s.split(",");

    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i = 0; i < factory.metricCount(); ++i) {
        QString symbol = factory.metricName(i);
        if (selectedMetrics.contains(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QListWidgetItem *item = new QListWidgetItem(Utils::unprotect(m->name()));
        item->setData(Qt::UserRole, symbol);
        item->setToolTip(m->description());
        availList->addItem(item);
    }
    foreach (QString symbol, selectedMetrics) {
        if (!factory.haveMetric(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QListWidgetItem *item = new QListWidgetItem(Utils::unprotect(m->name()));
        item->setData(Qt::UserRole, symbol);
        item->setToolTip(m->description());
        selectedList->addItem(item);
    }

    upButton->setEnabled(false);
    downButton->setEnabled(false);
    leftButton->setEnabled(false);
    rightButton->setEnabled(false);

    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(leftButton, SIGNAL(clicked()), this, SLOT(leftClicked()));
    connect(rightButton, SIGNAL(clicked()), this, SLOT(rightClicked()));
    connect(availList, SIGNAL(itemSelectionChanged()),
            this, SLOT(availChanged()));
    connect(selectedList, SIGNAL(itemSelectionChanged()),
            this, SLOT(selectedChanged()));
}

void
BestsMetricsPage::upClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    assert(row > 0);
    selectedList->takeItem(row);
    selectedList->insertItem(row - 1, item);
    selectedList->setCurrentItem(item);
    changed = true;
}

void
BestsMetricsPage::downClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    assert(row < selectedList->count() - 1);
    selectedList->takeItem(row);
    selectedList->insertItem(row + 1, item);
    selectedList->setCurrentItem(item);
    changed = true;
}

void
BestsMetricsPage::leftClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    selectedList->takeItem(selectedList->row(item));
    availList->addItem(item);
    changed = true;
    selectedChanged();
}

void
BestsMetricsPage::rightClicked()
{
    assert(!availList->selectedItems().isEmpty());
    QListWidgetItem *item = availList->selectedItems().first();
    availList->takeItem(availList->row(item));
    selectedList->addItem(item);
    changed = true;
}

void
BestsMetricsPage::availChanged()
{
    rightButton->setEnabled(!availList->selectedItems().isEmpty());
}

void
BestsMetricsPage::selectedChanged()
{
    if (selectedList->selectedItems().isEmpty()) {
        upButton->setEnabled(false);
        downButton->setEnabled(false);
        leftButton->setEnabled(false);
        return;
    }
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    if (row == 0)
        upButton->setEnabled(false);
    else
        upButton->setEnabled(true);
    if (row == selectedList->count() - 1)
        downButton->setEnabled(false);
    else
        downButton->setEnabled(true);
    leftButton->setEnabled(true);
}

qint32
BestsMetricsPage::saveClicked()
{
    if (!changed) return 0;

    QStringList metrics;
    for (int i = 0; i < selectedList->count(); ++i)
        metrics << selectedList->item(i)->data(Qt::UserRole).toString();
    appsettings->setValue(GC_SETTINGS_BESTS_METRICS, metrics.join(","));

    return 0;
}

CustomMetricsPage::CustomMetricsPage(QWidget *parent, Context *context) :
    QWidget(parent), context(context)
{
    // copy as current, so we can edit...
    metrics = _userMetrics;
    b4.crc = RideMetric::userMetricFingerprint(metrics);

    table = new QTreeWidget;
    table->headerItem()->setText(0, tr("Symbol"));
    table->headerItem()->setText(1, tr("Name"));
    table->setColumnCount(2);
    table->setColumnWidth(0,200);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setUniformRowHeights(true); // causes height problems when adding - in case of non-text fields
    table->setIndentation(0);

    refreshTable();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(table);
    connect(table, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(doubleClicked(QTreeWidgetItem*, int)));

    editButton = new QPushButton(tr("Edit"));
    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(editButton);
    buttons->addStretch();
    buttons->addWidget(addButton);
    buttons->addWidget(deleteButton);

    layout->addLayout(buttons);

    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(editButton, SIGNAL(clicked()), this, SLOT(editClicked()));
}

void
CustomMetricsPage::refreshTable()
{
    table->clear();
    foreach(UserMetricSettings m, metrics) {

        QTreeWidgetItem *add = new QTreeWidgetItem(table->invisibleRootItem());
        add->setText(0, m.symbol);
        add->setToolTip(0, m.description);
        add->setText(1, m.name);
        add->setToolTip(1, m.description);
    }
}

void
CustomMetricsPage::deleteClicked()
{
    // nothing selected
    if (table->selectedItems().count() <= 0) return;

    // which one?
    QTreeWidgetItem *item = table->selectedItems().first();
    int row = table->invisibleRootItem()->indexOfChild(item);

    // Are you sure ?
    QMessageBox msgBox;
    msgBox.setText(tr("Are you sure you want to delete this metric?"));
    msgBox.setInformativeText(metrics[row].name);
    QPushButton *deleteButton = msgBox.addButton(tr("Remove"),QMessageBox::YesRole);
    msgBox.setStandardButtons(QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.exec();

    // nope, don't want to
    if(msgBox.clickedButton() != deleteButton) return;

    // wipe it away
    metrics.removeAt(row);

    // refresh
    refreshTable();
}

void
CustomMetricsPage::addClicked()
{
    UserMetricSettings here;

    // provide an example program to edit....
    here.symbol = "My_Average_Power";
    here.name = "My Average Power";
    here.type = 1;
    here.precision = 0;
    here.description = "Average Power computed using Joules to account for variable recording.";
    here.unitsMetric = "watts";
    here.unitsImperial = "watts";
    here.conversion = 1.00;
    here.conversionSum = 0.00;
    here.program ="{\n\
    # only calculate for rides containing power\n\
    relevant { Data contains \"P\"; }\n\
\n\
    # initialise aggregating variables\n\
    init { joules <- 0; seconds <- 0; }\n\
\n\
    # joules = power x time, for each sample\n\
    sample { \n\
        joules <- joules + (POWER * RECINTSECS);\n\
        seconds <- seconds + RECINTSECS;\n\
    }\n\
\n\
    # calculate metric value at end\n\
    value { joules / seconds; }\n\
    count { seconds; }\n\
}";

    EditUserMetricDialog editor(this, context, here);
    if (editor.exec() == QDialog::Accepted) {

        // add to the list
        metrics.append(here);
        refreshTable();

    }
}

void
CustomMetricsPage::editClicked()
{
    // nothing selected
    if (table->selectedItems().count() <= 0) return;

    // which one?
    QTreeWidgetItem *item = table->selectedItems().first();

    doubleClicked(item, 0);
}

void
CustomMetricsPage::doubleClicked(QTreeWidgetItem *item, int)
{
    // nothing selected
    if (item == NULL) return;

    // find row
    int row = table->invisibleRootItem()->indexOfChild(item);

    // edit it
    UserMetricSettings here = metrics[row];

    EditUserMetricDialog editor(this, context, here);
    if (editor.exec() == QDialog::Accepted) {

        // add to the list
        metrics[row] = here;
        refreshTable();

    }
}

qint32
CustomMetricsPage::saveClicked()
{
    qint32 returning=0;

    // did we actually change them ?
    if (b4.crc != RideMetric::userMetricFingerprint(metrics))
        returning |= CONFIG_USERMETRICS;

    // save away OUR version to top-level NOT athlete dir
    // and don't overwrite in memory version the signal handles that
    QString filename = QString("%1/../usermetrics.xml").arg(context->athlete->home->root().absolutePath());
    UserMetricParser::serialize(filename, metrics);

    // the change will need to be signalled by context, not us
    return returning;
}

SummaryMetricsPage::SummaryMetricsPage(QWidget *parent) :
    QWidget(parent), changed(false)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Metrics_Summary));

    availList = new QListWidget;
    availList->setSortingEnabled(true);
    availList->setSelectionMode(QAbstractItemView::SingleSelection);
    QVBoxLayout *availLayout = new QVBoxLayout;
    availLayout->addWidget(new QLabel(tr("Available Metrics")));
    availLayout->addWidget(availList);
    selectedList = new QListWidget;
    selectedList->setSelectionMode(QAbstractItemView::SingleSelection);
    QVBoxLayout *selectedLayout = new QVBoxLayout;
    selectedLayout->addWidget(new QLabel(tr("Selected Metrics")));
    selectedLayout->addWidget(selectedList);
#ifndef Q_OS_MAC
    upButton = new QToolButton(this);
    downButton = new QToolButton(this);
    leftButton = new QToolButton(this);
    rightButton = new QToolButton(this);
    upButton->setArrowType(Qt::UpArrow);
    downButton->setArrowType(Qt::DownArrow);
    leftButton->setArrowType(Qt::LeftArrow);
    rightButton->setArrowType(Qt::RightArrow);
    upButton->setFixedSize(20,20);
    downButton->setFixedSize(20,20);
    leftButton->setFixedSize(20,20);
    rightButton->setFixedSize(20,20);
#else
    upButton = new QPushButton(tr("Up"));
    downButton = new QPushButton(tr("Down"));
    leftButton = new QPushButton("<");
    rightButton = new QPushButton(">");
#endif
    QVBoxLayout *buttonGrid = new QVBoxLayout;
    QHBoxLayout *upLayout = new QHBoxLayout;
    QHBoxLayout *inexcLayout = new QHBoxLayout;
    QHBoxLayout *downLayout = new QHBoxLayout;

    upLayout->addStretch();
    upLayout->addWidget(upButton);
    upLayout->addStretch();

    inexcLayout->addStretch();
    inexcLayout->addWidget(leftButton);
    inexcLayout->addWidget(rightButton);
    inexcLayout->addStretch();

    downLayout->addStretch();
    downLayout->addWidget(downButton);
    downLayout->addStretch();

    buttonGrid->addStretch();
    buttonGrid->addLayout(upLayout);
    buttonGrid->addLayout(inexcLayout);
    buttonGrid->addLayout(downLayout);
    buttonGrid->addStretch();

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addLayout(availLayout);
    hlayout->addLayout(buttonGrid);
    hlayout->addLayout(selectedLayout);
    setLayout(hlayout);

    QString s = appsettings->value(this, GC_SETTINGS_SUMMARY_METRICS, GC_SETTINGS_SUMMARY_METRICS_DEFAULT).toString();
    QStringList selectedMetrics = s.split(",");

    const RideMetricFactory &factory = RideMetricFactory::instance();
    for (int i = 0; i < factory.metricCount(); ++i) {
        QString symbol = factory.metricName(i);
        if (selectedMetrics.contains(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QListWidgetItem *item = new QListWidgetItem(Utils::unprotect(m->name()));
        item->setData(Qt::UserRole, symbol);
        item->setToolTip(m->description());
        availList->addItem(item);
    }
    foreach (QString symbol, selectedMetrics) {
        if (!factory.haveMetric(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QListWidgetItem *item = new QListWidgetItem(Utils::unprotect(m->name()));
        item->setData(Qt::UserRole, symbol);
        item->setToolTip(m->description());
        selectedList->addItem(item);
    }

    upButton->setEnabled(false);
    downButton->setEnabled(false);
    leftButton->setEnabled(false);
    rightButton->setEnabled(false);

    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(leftButton, SIGNAL(clicked()), this, SLOT(leftClicked()));
    connect(rightButton, SIGNAL(clicked()), this, SLOT(rightClicked()));
    connect(availList, SIGNAL(itemSelectionChanged()),
            this, SLOT(availChanged()));
    connect(selectedList, SIGNAL(itemSelectionChanged()),
            this, SLOT(selectedChanged()));
}

void
SummaryMetricsPage::upClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    assert(row > 0);
    selectedList->takeItem(row);
    selectedList->insertItem(row - 1, item);
    selectedList->setCurrentItem(item);
    changed = true;
}

void
SummaryMetricsPage::downClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    assert(row < selectedList->count() - 1);
    selectedList->takeItem(row);
    selectedList->insertItem(row + 1, item);
    selectedList->setCurrentItem(item);
    changed = true;
}

void
SummaryMetricsPage::leftClicked()
{
    assert(!selectedList->selectedItems().isEmpty());
    QListWidgetItem *item = selectedList->selectedItems().first();
    selectedList->takeItem(selectedList->row(item));
    availList->addItem(item);
    changed = true;
    selectedChanged();
}

void
SummaryMetricsPage::rightClicked()
{
    assert(!availList->selectedItems().isEmpty());
    QListWidgetItem *item = availList->selectedItems().first();
    availList->takeItem(availList->row(item));
    selectedList->addItem(item);
    changed = true;
}

void
SummaryMetricsPage::availChanged()
{
    rightButton->setEnabled(!availList->selectedItems().isEmpty());
}

void
SummaryMetricsPage::selectedChanged()
{
    if (selectedList->selectedItems().isEmpty()) {
        upButton->setEnabled(false);
        downButton->setEnabled(false);
        leftButton->setEnabled(false);
        return;
    }
    QListWidgetItem *item = selectedList->selectedItems().first();
    int row = selectedList->row(item);
    if (row == 0)
        upButton->setEnabled(false);
    else
        upButton->setEnabled(true);
    if (row == selectedList->count() - 1)
        downButton->setEnabled(false);
    else
        downButton->setEnabled(true);
    leftButton->setEnabled(true);
}

qint32
SummaryMetricsPage::saveClicked()
{
    if (!changed) return 0;

    QStringList metrics;
    for (int i = 0; i < selectedList->count(); ++i)
        metrics << selectedList->item(i)->data(Qt::UserRole).toString();
    appsettings->setValue(GC_SETTINGS_SUMMARY_METRICS, metrics.join(","));

    return 0;
}

MetadataPage::MetadataPage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // get current config using default file
    keywordDefinitions = context->athlete->rideMetadata()->getKeywords();
    fieldDefinitions = context->athlete->rideMetadata()->getFields();
    colorfield = context->athlete->rideMetadata()->getColorField();
    defaultDefinitions = context->athlete->rideMetadata()->getDefaults();

    // setup maintenance pages using current config
    fieldsPage = new FieldsPage(this, fieldDefinitions);
    keywordsPage = new KeywordsPage(this, keywordDefinitions);
    defaultsPage = new DefaultsPage(this, defaultDefinitions);
    processorPage = new ProcessorPage(context);


    tabs = new QTabWidget(this);
    tabs->addTab(fieldsPage, tr("Fields"));
    tabs->addTab(keywordsPage, tr("Notes Keywords"));
    tabs->addTab(defaultsPage, tr("Defaults"));
    tabs->addTab(processorPage, tr("Processing"));


    // refresh the keywords combo when change tabs .. will do more often than
    // needed but better that than less often than needed
    connect (tabs, SIGNAL(currentChanged(int)), keywordsPage, SLOT(pageSelected()));

    layout->addWidget(tabs);

    // save initial values
    b4.keywordFingerprint = KeywordDefinition::fingerprint(keywordDefinitions);
    b4.colorfield = colorfield;
    b4.fieldFingerprint = FieldDefinition::fingerprint(fieldDefinitions);
}

qint32
MetadataPage::saveClicked()
{
    // get current state
    fieldsPage->getDefinitions(fieldDefinitions);
    keywordsPage->getDefinitions(keywordDefinitions);
    defaultsPage->getDefinitions(defaultDefinitions);

    // save settings
    appsettings->setValue(GC_RIDEBG, keywordsPage->rideBG->isChecked());

    // write to metadata.xml
    RideMetadata::serialize(context->athlete->home->config().canonicalPath() + "/metadata.xml", keywordDefinitions, fieldDefinitions, colorfield, defaultDefinitions);

    // save processors config
    processorPage->saveClicked();

    qint32 state = 0;

    if (b4.keywordFingerprint != KeywordDefinition::fingerprint(keywordDefinitions) || b4.colorfield != colorfield)
        state += CONFIG_NOTECOLOR;

    if (b4.fieldFingerprint != FieldDefinition::fingerprint(fieldDefinitions))
        state += CONFIG_FIELDS;

    return state;
}

// little helper since we create/recreate combos
// for field types all over the place (init, move up, move down)
void
FieldsPage::addFieldTypes(QComboBox *p)
{
    p->addItem(tr("Text"));
    p->addItem(tr("Textbox"));
    p->addItem(tr("ShortText"));
    p->addItem(tr("Integer"));
    p->addItem(tr("Double"));
    p->addItem(tr("Date"));
    p->addItem(tr("Time"));
    p->addItem(tr("Checkbox"));
}

//
// Calendar coloring page
//
KeywordsPage::KeywordsPage(MetadataPage *parent, QList<KeywordDefinition>keywordDefinitions) : QWidget(parent), parent(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_DataFields_Notes_Keywords));

    QHBoxLayout *field = new QHBoxLayout();
    fieldLabel = new QLabel(tr("Field"),this);
    fieldChooser = new QComboBox(this);
    field->addWidget(fieldLabel);
    field->addWidget(fieldChooser);
    field->addStretch();
    mainLayout->addLayout(field);

    rideBG = new QCheckBox(tr("Use for Background"));
    rideBG->setChecked(appsettings->value(this, GC_RIDEBG, false).toBool());
    field->addWidget(rideBG);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    upButton = new QToolButton(this);
    downButton = new QToolButton(this);
    upButton->setArrowType(Qt::UpArrow);
    downButton->setArrowType(Qt::DownArrow);
    upButton->setFixedSize(20,20);
    downButton->setFixedSize(20,20);
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
    upButton = new QPushButton(tr("Up"));
    downButton = new QPushButton(tr("Down"));
#endif

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    keywords = new QTreeWidget;
    keywords->headerItem()->setText(0, tr("Keyword"));
    keywords->headerItem()->setText(1, tr("Color"));
    keywords->headerItem()->setText(2, tr("Related Notes Words"));
    keywords->setColumnCount(3);
    keywords->setSelectionMode(QAbstractItemView::SingleSelection);
    keywords->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    //keywords->setUniformRowHeights(true); // causes height problems when adding - in case of non-text fields
    keywords->setIndentation(0);
    //keywords->header()->resizeSection(0,100);
    //keywords->header()->resizeSection(1,45);

    foreach(KeywordDefinition keyword, keywordDefinitions) {
        QTreeWidgetItem *add;
        ColorButton *colorButton = new ColorButton(this, keyword.name, keyword.color);
        add = new QTreeWidgetItem(keywords->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // keyword
        add->setText(0, keyword.name);

        // color button
        add->setTextAlignment(1, Qt::AlignHCenter);
        keywords->setItemWidget(add, 1, colorButton);

        QString text;
        for (int i=0; i< keyword.tokens.count(); i++) {
            if (i != keyword.tokens.count()-1)
                text += keyword.tokens[i] + ",";
            else
                text += keyword.tokens[i];
        }

        // notes texts
        add->setText(2, text);
    }
    keywords->setCurrentItem(keywords->invisibleRootItem()->child(0));

    mainLayout->addWidget(keywords);
    mainLayout->addLayout(actionButtons);

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));

    connect(fieldChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(colorfieldChanged()));
}

void
KeywordsPage::pageSelected()
{
    SpecialFields sp;
    QString prev = "";

    // remember what was selected, if anything?
    if (fieldChooser->count()) {
        prev = sp.internalName(fieldChooser->itemText(fieldChooser->currentIndex()));
        parent->colorfield = prev;
    } else prev = parent->colorfield;
    // load in texts from metadata
    fieldChooser->clear();

    // get the current fields definitions
    QList<FieldDefinition> fromFieldsPage;
    parent->fieldsPage->getDefinitions(fromFieldsPage);
    foreach(FieldDefinition x, fromFieldsPage) {
        if (x.type < 3) fieldChooser->addItem(sp.displayName(x.name));
    }
    fieldChooser->setCurrentIndex(fieldChooser->findText(sp.displayName(prev)));
}

void
KeywordsPage::colorfieldChanged()
{
    SpecialFields sp;
    int index = fieldChooser->currentIndex();
    if (index >=0) parent->colorfield = sp.internalName(fieldChooser->itemText(fieldChooser->currentIndex()));
}


void
KeywordsPage::upClicked()
{
    if (keywords->currentItem()) {
        int index = keywords->invisibleRootItem()->indexOfChild(keywords->currentItem());
        if (index == 0) return; // its at the top already

        // movin on up!
        QWidget *button = keywords->itemWidget(keywords->currentItem(),1);
        ColorButton *colorButton = new ColorButton(this, ((ColorButton*)button)->getName(), ((ColorButton*)button)->getColor());
        QTreeWidgetItem* moved = keywords->invisibleRootItem()->takeChild(index);
        keywords->invisibleRootItem()->insertChild(index-1, moved);
        keywords->setItemWidget(moved, 1, colorButton);
        keywords->setCurrentItem(moved);
        //LTMSettings save = (*presets)[index];
        //presets->removeAt(index);
        //presets->insert(index-1, save);
    }
}

void
KeywordsPage::downClicked()
{
    if (keywords->currentItem()) {
        int index = keywords->invisibleRootItem()->indexOfChild(keywords->currentItem());
        if (index == (keywords->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        // movin on up!
        QWidget *button = keywords->itemWidget(keywords->currentItem(),1);
        ColorButton *colorButton = new ColorButton(this, ((ColorButton*)button)->getName(), ((ColorButton*)button)->getColor());
        QTreeWidgetItem* moved = keywords->invisibleRootItem()->takeChild(index);
        keywords->invisibleRootItem()->insertChild(index+1, moved);
        keywords->setItemWidget(moved, 1, colorButton);
        keywords->setCurrentItem(moved);
    }
}

void
KeywordsPage::renameClicked()
{
    // which one is selected?
    if (keywords->currentItem()) keywords->editItem(keywords->currentItem(), 0);
}

void
KeywordsPage::addClicked()
{
    int index = keywords->invisibleRootItem()->indexOfChild(keywords->currentItem());
    if (index < 0) index = 0;
    QTreeWidgetItem *add;
    ColorButton *colorButton = new ColorButton(this, tr("New"), QColor(Qt::blue));
    add = new QTreeWidgetItem;
    keywords->invisibleRootItem()->insertChild(index, add);
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    // keyword
    QString text = tr("New");
    for (int i=0; keywords->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // color button
    add->setTextAlignment(1, Qt::AlignHCenter);
    keywords->setItemWidget(add, 1, colorButton);

    // notes texts
    add->setText(2, "");
}

void
KeywordsPage::deleteClicked()
{
    if (keywords->currentItem()) {
        int index = keywords->invisibleRootItem()->indexOfChild(keywords->currentItem());

        // zap!
        delete keywords->invisibleRootItem()->takeChild(index);
    }
}

void
KeywordsPage::getDefinitions(QList<KeywordDefinition> &keywordList)
{
    // clear current just in case
    keywordList.clear();

    for (int idx =0; idx < keywords->invisibleRootItem()->childCount(); idx++) {
        KeywordDefinition add;
        QTreeWidgetItem *item = keywords->invisibleRootItem()->child(idx);

        add.name = item->text(0);
        add.color = ((ColorButton*)keywords->itemWidget(item, 1))->getColor();
        add.tokens = item->text(2).split(",", QString::SkipEmptyParts);

        keywordList.append(add);
    }
}

//
// Ride metadata page
//
FieldsPage::FieldsPage(QWidget *parent, QList<FieldDefinition>fieldDefinitions) : QWidget(parent)
{
    QGridLayout *mainLayout = new QGridLayout(this);
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_DataFields_Fields));

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    upButton = new QToolButton(this);
    downButton = new QToolButton(this);
    upButton->setArrowType(Qt::UpArrow);
    downButton->setArrowType(Qt::DownArrow);
    upButton->setFixedSize(20,20);
    downButton->setFixedSize(20,20);
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
    upButton = new QPushButton(tr("Up"));
    downButton = new QPushButton(tr("Down"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    fields = new QTreeWidget;
    fields->headerItem()->setText(0, tr("Screen Tab"));
    fields->headerItem()->setText(1, tr("Field"));
    fields->headerItem()->setText(2, tr("Type"));
    fields->headerItem()->setText(3, tr("Values"));
    fields->headerItem()->setText(4, tr("Diary"));
    fields->setColumnWidth(0,80);
    fields->setColumnWidth(1,100);
    fields->setColumnWidth(2,100);
    fields->setColumnWidth(3,80);
    fields->setColumnWidth(4,20);
    fields->setColumnCount(5);
    fields->setSelectionMode(QAbstractItemView::SingleSelection);
    fields->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    //fields->setUniformRowHeights(true); // causes height problems when adding - in case of non-text fields
    fields->setIndentation(0);

    SpecialFields specials;
    SpecialTabs specialTabs;
    foreach(FieldDefinition field, fieldDefinitions) {
        QTreeWidgetItem *add;
        QComboBox *comboButton = new QComboBox(this);
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(field.diary);

        addFieldTypes(comboButton);
        comboButton->setCurrentIndex(field.type);

        add = new QTreeWidgetItem(fields->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, specialTabs.displayName(field.tab));
        // field name
        add->setText(1, specials.displayName(field.name));
        // values
        add->setText(3, field.values.join(","));

        // type button
        add->setTextAlignment(2, Qt::AlignHCenter);
        fields->setItemWidget(add, 2, comboButton);
        fields->setItemWidget(add, 4, checkBox);
    }
    fields->setCurrentItem(fields->invisibleRootItem()->child(0));

    mainLayout->addWidget(fields, 0,0);
    mainLayout->addLayout(actionButtons, 1,0);

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
}

void
FieldsPage::upClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == 0) return; // its at the top already

        // movin on up!
        QWidget *button = fields->itemWidget(fields->currentItem(),2);
        QWidget *check = fields->itemWidget(fields->currentItem(),4);
        QComboBox *comboButton = new QComboBox(this);
        addFieldTypes(comboButton);
        comboButton->setCurrentIndex(((QComboBox*)button)->currentIndex());
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(((QCheckBox*)check)->isChecked());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index-1, moved);
        fields->setItemWidget(moved, 2, comboButton);
        fields->setItemWidget(moved, 4, checkBox);
        fields->setCurrentItem(moved);
    }
}

void
FieldsPage::downClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == (fields->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        QWidget *button = fields->itemWidget(fields->currentItem(),2);
        QWidget *check = fields->itemWidget(fields->currentItem(),4);
        QComboBox *comboButton = new QComboBox(this);
        addFieldTypes(comboButton);
        comboButton->setCurrentIndex(((QComboBox*)button)->currentIndex());
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(((QCheckBox*)check)->isChecked());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index+1, moved);
        fields->setItemWidget(moved, 2, comboButton);
        fields->setItemWidget(moved, 4, checkBox);
        fields->setCurrentItem(moved);
    }
}

void
FieldsPage::renameClicked()
{
    // which one is selected?
    if (fields->currentItem()) fields->editItem(fields->currentItem(), 0);
}

void
FieldsPage::addClicked()
{
    int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
    if (index < 0) index = 0;
    QTreeWidgetItem *add;
    QComboBox *comboButton = new QComboBox(this);
    addFieldTypes(comboButton);
    QCheckBox *checkBox = new QCheckBox("", this);

    add = new QTreeWidgetItem;
    fields->invisibleRootItem()->insertChild(index, add);
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    // field
    QString text = tr("New");
    for (int i=0; fields->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);

    // type button
    add->setTextAlignment(2, Qt::AlignHCenter);
    fields->setItemWidget(add, 2, comboButton);
    fields->setItemWidget(add, 4, checkBox);
}

void
FieldsPage::deleteClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());

        // zap!
        delete fields->invisibleRootItem()->takeChild(index);
    }
}

void
FieldsPage::getDefinitions(QList<FieldDefinition> &fieldList)
{
    SpecialFields sp;
    SpecialTabs st;
    QStringList checkdups;

    // clear current just in case
    fieldList.clear();

    for (int idx =0; idx < fields->invisibleRootItem()->childCount(); idx++) {

        FieldDefinition add;
        QTreeWidgetItem *item = fields->invisibleRootItem()->child(idx);

        // silently ignore duplicates
        if (checkdups.contains(item->text(1))) continue;
        else checkdups << item->text(1);

        add.tab = st.internalName(item->text(0));
        add.name = sp.internalName(item->text(1));
        add.values = item->text(3).split(QRegExp("(, *|,)"), QString::KeepEmptyParts);
        add.diary = ((QCheckBox*)fields->itemWidget(item, 4))->isChecked();

        if (sp.isMetric(add.name))
            add.type = 4;
        else
            add.type = ((QComboBox*)fields->itemWidget(item, 2))->currentIndex();

        fieldList.append(add);
    }
}

//
// Data processors config page
//
ProcessorPage::ProcessorPage(Context *context) : context(context)
{
    // get the available processors
    const DataProcessorFactory &factory = DataProcessorFactory::instance();
    processors = factory.getProcessors();

    QGridLayout *mainLayout = new QGridLayout(this);

    processorTree = new QTreeWidget;
    processorTree->headerItem()->setText(0, tr("Processor"));
    processorTree->headerItem()->setText(1, tr("Apply"));
    processorTree->headerItem()->setText(2, tr("Settings"));
    processorTree->headerItem()->setText(3, "Technical Name of Processor"); // add invisible Column with technical name
    processorTree->setColumnCount(4);
    processorTree->setSelectionMode(QAbstractItemView::NoSelection);
    processorTree->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    processorTree->setUniformRowHeights(true);
    processorTree->setIndentation(0);
    //processorTree->header()->resizeSection(0,150);
    HelpWhatsThis *help = new HelpWhatsThis(processorTree);
    processorTree->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_DataFields_Processing));

    // iterate over all the processors and add an entry to the
    QMapIterator<QString, DataProcessor*> i(processors);
    i.toFront();
    while (i.hasNext()) {
        i.next();

        QTreeWidgetItem *add;

        add = new QTreeWidgetItem(processorTree->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Processor Name: it shows the localized name
        add->setText(0, i.value()->name());
        // Processer Key - for Settings
        add->setText(3, i.key());

        // Auto or Manual run?
        QComboBox *comboButton = new QComboBox(this);
        comboButton->addItem(tr("Manual"));
        comboButton->addItem(tr("Import"));
        processorTree->setItemWidget(add, 1, comboButton);

        QString configsetting = QString("dp/%1/apply").arg(i.key());
        if (appsettings->value(NULL, GC_QSETTINGS_GLOBAL_GENERAL+configsetting, "Manual").toString() == "Manual")
            comboButton->setCurrentIndex(0);
        else
            comboButton->setCurrentIndex(1);

        // Get and Set the Config Widget
        DataProcessorConfig *config = i.value()->processorConfig(this);
        config->readConfig();

        processorTree->setItemWidget(add, 2, config);

    }
    processorTree->setColumnHidden(3, true);

    mainLayout->addWidget(processorTree, 0,0);
}

qint32
ProcessorPage::saveClicked()
{
    // call each processor config widget's saveConfig() to
    // write away separately
    for (int i=0; i<processorTree->invisibleRootItem()->childCount(); i++) {
        // auto (which means run on import) or manual?
        QString configsetting = QString("dp/%1/apply").arg(processorTree->invisibleRootItem()->child(i)->text(3));
        QString apply = ((QComboBox*)(processorTree->itemWidget(processorTree->invisibleRootItem()->child(i), 1)))->currentIndex() ?
                        "Auto" : "Manual";
        appsettings->setValue(GC_QSETTINGS_GLOBAL_GENERAL+configsetting, apply);
        ((DataProcessorConfig*)(processorTree->itemWidget(processorTree->invisibleRootItem()->child(i), 2)))->saveConfig();
    }

    return 0;
}

//
// Default values page
//
DefaultsPage::DefaultsPage(QWidget *parent, QList<DefaultDefinition>defaultDefinitions) : QWidget(parent)
{
    QGridLayout *mainLayout = new QGridLayout(this);
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_DataFields_Defaults));

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    upButton = new QToolButton(this);
    downButton = new QToolButton(this);
    upButton->setArrowType(Qt::UpArrow);
    downButton->setArrowType(Qt::DownArrow);
    upButton->setFixedSize(20,20);
    downButton->setFixedSize(20,20);
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
    upButton = new QPushButton(tr("Up"));
    downButton = new QPushButton(tr("Down"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    defaults = new QTreeWidget;
    defaults->headerItem()->setText(0, tr("Field"));
    defaults->headerItem()->setText(1, tr("Value"));
    defaults->headerItem()->setText(2, tr("Linked field"));
    defaults->headerItem()->setText(3, tr("Default Value"));
    defaults->setColumnWidth(0,80);
    defaults->setColumnWidth(1,100);
    defaults->setColumnWidth(2,80);
    defaults->setColumnWidth(3,100);
    defaults->setColumnCount(4);
    defaults->setSelectionMode(QAbstractItemView::SingleSelection);
    defaults->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    //defaults->setUniformRowHeights(true); // causes height problems when adding - in case of non-text fields
    defaults->setIndentation(0);

    SpecialFields specials;
    foreach(DefaultDefinition adefault, defaultDefinitions) {
        QTreeWidgetItem *add;

        add = new QTreeWidgetItem(defaults->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // field name
        add->setText(0, specials.displayName(adefault.field));
        // value
        add->setText(1, adefault.value);
        // Linked field
        add->setText(2, adefault.linkedField);
        // Default Value
        add->setText(3, adefault.linkedValue);
    }
    defaults->setCurrentItem(defaults->invisibleRootItem()->child(0));

    mainLayout->addWidget(defaults, 0,0);
    mainLayout->addLayout(actionButtons, 1,0);

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
}

void
DefaultsPage::upClicked()
{
    if (defaults->currentItem()) {
        int index = defaults->invisibleRootItem()->indexOfChild(defaults->currentItem());
        if (index == 0) return; // its at the top already

        QTreeWidgetItem* moved = defaults->invisibleRootItem()->takeChild(index);
        defaults->invisibleRootItem()->insertChild(index-1, moved);
        defaults->setCurrentItem(moved);
    }
}

void
DefaultsPage::downClicked()
{
    if (defaults->currentItem()) {
        int index = defaults->invisibleRootItem()->indexOfChild(defaults->currentItem());
        if (index == (defaults->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        QTreeWidgetItem* moved = defaults->invisibleRootItem()->takeChild(index);
        defaults->invisibleRootItem()->insertChild(index+1, moved);
        defaults->setCurrentItem(moved);
    }
}

void
DefaultsPage::addClicked()
{
    int index = defaults->invisibleRootItem()->indexOfChild(defaults->currentItem());
    if (index < 0) index = 0;
    QTreeWidgetItem *add;

    add = new QTreeWidgetItem;
    defaults->invisibleRootItem()->insertChild(index, add);
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    // field
    QString text = tr("New");
    for (int i=0; defaults->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);
}

void
DefaultsPage::deleteClicked()
{
    if (defaults->currentItem()) {
        int index = defaults->invisibleRootItem()->indexOfChild(defaults->currentItem());

        // zap!
        delete defaults->invisibleRootItem()->takeChild(index);
    }
}

void
DefaultsPage::getDefinitions(QList<DefaultDefinition> &defaultList)
{
    SpecialFields sp;

    // clear current just in case
    defaultList.clear();

    for (int idx =0; idx < defaults->invisibleRootItem()->childCount(); idx++) {

        DefaultDefinition add;
        QTreeWidgetItem *item = defaults->invisibleRootItem()->child(idx);

        add.field = sp.internalName(item->text(0));
        add.value = item->text(1);
        add.linkedField = sp.internalName(item->text(2));
        add.linkedValue = item->text(3);

        defaultList.append(add);
    }
}

//
// Power Zone Config page
//
ZonePage::ZonePage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hlayout = new QHBoxLayout;

    sportLabel = new QLabel(tr("Sport"));
    sportCombo = new QComboBox();
    sportCombo->addItem(tr("Bike"));
    sportCombo->addItem(tr("Run"));
    sportCombo->setCurrentIndex(0);
    hlayout->addStretch();
    hlayout->addWidget(sportLabel);
    hlayout->addWidget(sportCombo);
    hlayout->addStretch();
    layout->addLayout(hlayout);
    connect(sportCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSport(int)));
    tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    for (int i=0; i < nSports; i++) {
        zones[i] = new Zones(i > 0);

        // get current config by reading it in (leave mainwindow zones alone)
        QFile zonesFile(context->athlete->home->config().canonicalPath() + "/" + zones[i]->fileName());
        if (zonesFile.exists()) {
            zones[i]->read(zonesFile);
            zonesFile.close();
            b4Fingerprint[i] = zones[i]->getFingerprint(); // remember original state
        }

        // setup maintenance pages using current config
        schemePage[i] = new SchemePage(zones[i]);
        cpPage[i] = new CPPage(context, zones[i], schemePage[i]);
    }

    // finish setup for the default sport
    changeSport(sportCombo->currentIndex());
}

ZonePage::~ZonePage()
{
    for (int i=0; i<nSports; i++) delete zones[i];
}

void
ZonePage::changeSport(int i)
{
    // change tabs according to the selected sport
    tabs->clear();
    tabs->addTab(cpPage[i], tr("Critical Power"));
    tabs->addTab(schemePage[i], tr("Default"));
}


qint32
ZonePage::saveClicked()
{
    qint32 changed = 0;
    qint32 cppageChanged = 0;
    // write
    for (int i=0; i < nSports; i++) {
        zones[i]->setScheme(schemePage[i]->getScheme());
        zones[i]->write(context->athlete->home->config());

        // re-read Zones in case it changed
        QFile zonesFile(context->athlete->home->config().canonicalPath() + "/" + context->athlete->zones_[i]->fileName());
        context->athlete->zones_[i]->read(zonesFile);

        // use CP for FTP?
        appsettings->setCValue(context->athlete->cyclist, zones[i]->useCPforFTPSetting(), cpPage[i]->useCPForFTPCombo->currentIndex());


        // did cp for ftp change ?
        cppageChanged |= cpPage[i]->saveClicked();

        // did we change ?
        if (zones[i]->getFingerprint() != b4Fingerprint[i])
            changed = CONFIG_ZONES;
    }
    return changed | cppageChanged;
}

SchemePage::SchemePage(Zones* zones) : zones(zones)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    scheme = new QTreeWidget;
    scheme->headerItem()->setText(0, tr("Short"));
    scheme->headerItem()->setText(1, tr("Long"));
    scheme->headerItem()->setText(2, tr("Percent of CP"));
    scheme->setColumnCount(3);
    scheme->setSelectionMode(QAbstractItemView::SingleSelection);
    scheme->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    scheme->setUniformRowHeights(true);
    scheme->setIndentation(0);
    //scheme->header()->resizeSection(0,90);
    //scheme->header()->resizeSection(1,200);
    //scheme->header()->resizeSection(2,80);

    // setup list
    for (int i=0; i< zones->getScheme().nzones_default; i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, zones->getScheme().zone_default_name[i]);
        // field name
        add->setText(1, zones->getScheme().zone_default_desc[i]);

        // low
        QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
        loedit->setMinimum(0);
        loedit->setMaximum(1000);
        loedit->setSingleStep(1.0);
        loedit->setDecimals(0);
        loedit->setValue(zones->getScheme().zone_default[i]);
        scheme->setItemWidget(add, 2, loedit);
    }

    mainLayout->addWidget(scheme);
    mainLayout->addLayout(actionButtons);

    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
}

void
SchemePage::addClicked()
{
    // are we at maximum already?
    if (scheme->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int index = scheme->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
    loedit->setMinimum(0);
    loedit->setMaximum(1000);
    loedit->setSingleStep(1.0);
    loedit->setDecimals(0);
    loedit->setValue(100);

    scheme->invisibleRootItem()->insertChild(index, add);
    scheme->setItemWidget(add, 2, loedit);

    // Short
    QString text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
}

void
SchemePage::renameClicked()
{
    // which one is selected?
    if (scheme->currentItem()) scheme->editItem(scheme->currentItem(), 0);
}

void
SchemePage::deleteClicked()
{
    if (scheme->currentItem()) {
        int index = scheme->invisibleRootItem()->indexOfChild(scheme->currentItem());
        delete scheme->invisibleRootItem()->takeChild(index);
    }
}

// just for qSorting
struct schemeitem {
    QString name, desc;
    int lo;
    double trimp;
    bool operator<(schemeitem right) const { return lo < right.lo; }
};

ZoneScheme
SchemePage::getScheme()
{
    // read the scheme widget and return a scheme object
    QList<schemeitem> table;
    ZoneScheme results;

    // read back the details from the table
    for (int i=0; i<scheme->invisibleRootItem()->childCount(); i++) {

        schemeitem add;
        add.name = scheme->invisibleRootItem()->child(i)->text(0);
        add.desc = scheme->invisibleRootItem()->child(i)->text(1);
        add.lo = ((QDoubleSpinBox *)(scheme->itemWidget(scheme->invisibleRootItem()->child(i), 2)))->value();
        table.append(add);
    }

    // sort the list into ascending order
    qSort(table);

    // now update the results
    results.nzones_default = 0;
    foreach(schemeitem zone, table) {
        results.nzones_default++;
        results.zone_default.append(zone.lo);
        results.zone_default_is_pct.append(true);
        results.zone_default_name.append(zone.name);
        results.zone_default_desc.append(zone.desc);
    }

    return results;
}


CPPage::CPPage(Context *context, Zones *zones_, SchemePage *schemePage) :
               context(context), zones_(zones_), schemePage(schemePage)
{
    active = false;

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setSpacing(10);

    updateButton = new QPushButton(tr("Update"));
    updateButton->hide();
    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    addButton->setText(tr("Add"));
    updateButton->setText(tr("Update"));
    deleteButton->setText(tr("Delete"));
#endif
    defaultButton = new QPushButton(tr("Def"));
    defaultButton->hide();

    addZoneButton = new QPushButton(tr("+"));
    deleteZoneButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addZoneButton->setFixedSize(20,20);
    deleteZoneButton->setFixedSize(20,20);
#else
    addZoneButton->setText(tr("Add"));
    deleteZoneButton->setText(tr("Delete"));
#endif

    QHBoxLayout *zoneButtons = new QHBoxLayout;
    zoneButtons->addStretch();
    zoneButtons->setSpacing(0);
    zoneButtons->addWidget(addZoneButton);
    zoneButtons->addWidget(deleteZoneButton);
    zoneButtons->addWidget(defaultButton);

    QHBoxLayout *addLayout = new QHBoxLayout;
    QLabel *dateLabel = new QLabel(tr("From Date"));
    QLabel *cpLabel = new QLabel(tr("Critical Power"));
    QLabel *ftpLabel = new QLabel(tr("FTP"));
    QLabel *wLabel = new QLabel(tr("W'"));
    QLabel *pmaxLabel = new QLabel(tr("Pmax"));
    dateEdit = new QDateEdit;
    dateEdit->setDate(QDate::currentDate());

     // Use CP for FTP
    useCPForFTPCombo = new QComboBox(this);
    useCPForFTPCombo->addItem(tr("Use CP for all metrics"));
    useCPForFTPCombo->addItem(tr("Use FTP for Coggan metrics"));

    b4.cpforftp = appsettings->cvalue(context->athlete->cyclist, zones_->useCPforFTPSetting(), 0).toInt() ? 1 : 0;
    useCPForFTPCombo->setCurrentIndex(b4.cpforftp);

    cpEdit = new QDoubleSpinBox;
    cpEdit->setMinimum(0);
    cpEdit->setMaximum(1000);
    cpEdit->setSingleStep(1.0);
    cpEdit->setDecimals(0);

    ftpEdit = new QDoubleSpinBox;
    ftpEdit->setMinimum(0);
    ftpEdit->setMaximum(100000);
    ftpEdit->setSingleStep(100);
    ftpEdit->setDecimals(0);

    wEdit = new QDoubleSpinBox;
    wEdit->setMinimum(0);
    wEdit->setMaximum(100000);
    wEdit->setSingleStep(100);
    wEdit->setDecimals(0);

    pmaxEdit = new QDoubleSpinBox;
    pmaxEdit->setMinimum(0);
    pmaxEdit->setMaximum(3000);
    pmaxEdit->setSingleStep(1.0);
    pmaxEdit->setDecimals(0);

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addWidget(cpLabel);
    actionButtons->addWidget(cpEdit);


    actionButtons->addWidget(ftpLabel);
    actionButtons->addWidget(ftpEdit);

    actionButtons->addWidget(wLabel);
    actionButtons->addWidget(wEdit);
    actionButtons->addWidget(pmaxLabel);
    actionButtons->addWidget(pmaxEdit);
    actionButtons->addStretch();
    actionButtons->addWidget(updateButton);
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);
    //actionButtons->addWidget(defaultButton); // moved to zoneButtons

    addLayout->addWidget(dateLabel);
    addLayout->addWidget(dateEdit);
    addLayout->addStretch();
    addLayout->addWidget(useCPForFTPCombo);

    ranges = new QTreeWidget;
    initializeRanges();

    zones = new QTreeWidget;
    zones->headerItem()->setText(0, tr("Short"));
    zones->headerItem()->setText(1, tr("Long"));
    zones->headerItem()->setText(2, tr("From Watts"));
    zones->setColumnCount(3);
    zones->setSelectionMode(QAbstractItemView::SingleSelection);
    zones->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    zones->setUniformRowHeights(true);
    zones->setIndentation(0);
    //zones->header()->resizeSection(0,80);
    //zones->header()->resizeSection(1,150);

    mainLayout->addLayout(addLayout, 0,0);
    mainLayout->addLayout(actionButtons, 1,0);
    mainLayout->addWidget(ranges, 2,0);
    mainLayout->addLayout(zoneButtons, 3,0);
    mainLayout->addWidget(zones, 4,0);

    // edit connect
    connect(dateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(rangeEdited()));
    connect(cpEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(ftpEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(wEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(pmaxEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(updateButton, SIGNAL(clicked()), this, SLOT(editClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultClicked()));
    connect(addZoneButton, SIGNAL(clicked()), this, SLOT(addZoneClicked()));
    connect(deleteZoneButton, SIGNAL(clicked()), this, SLOT(deleteZoneClicked()));
    connect(useCPForFTPCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(initializeRanges()));
    connect(ranges, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));
    connect(zones, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(zonesChanged()));
}

qint32
CPPage::saveClicked()
{
    if (b4.cpforftp != useCPForFTPCombo->currentIndex())
        return CONFIG_ZONES;
    else
        return 0;
}

void
CPPage::initializeRanges() {
    bool useCPForFTP = (useCPForFTPCombo->currentIndex() == 0? true : false);

    int column = 0;

    bool resize = (ranges->columnCount() == 4);

    while( int nb = ranges->topLevelItemCount () )
    {
        delete ranges->takeTopLevelItem( nb - 1 );
    }
    ranges->headerItem()->setText(column++, tr("From Date"));

    ranges->headerItem()->setText(column++, tr("Critical Power"));
    if (!useCPForFTP) {
        ranges->headerItem()->setText(column++, tr("FTP"));
    }

    ranges->headerItem()->setText(column++, tr("W'"));
    ranges->headerItem()->setText(column++, tr("Pmax"));

    if (resize)
        ranges->setColumnWidth(3, ranges->columnWidth(3)/2);

    ranges->setColumnCount(column);

    ranges->setSelectionMode(QAbstractItemView::SingleSelection);
    //ranges->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    ranges->setUniformRowHeights(true);
    ranges->setIndentation(0);

    // setup list of ranges
    for (int i=0; i< zones_->getRangeSize(); i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(zones_->getZoneRange(i).zonesSetFromCP ?
                                        QFont::Normal : QFont::Black);

        int column = 0;
        // date
        add->setText(column, zones_->getStartDate(i).toString(tr("MMM d, yyyy")));
        add->setFont(column++, font);

        // CP
        add->setText(column, QString("%1").arg(zones_->getCP(i)));
        add->setFont(column++, font);

        if (!useCPForFTP) {
            // FTP
            add->setText(column, QString("%1").arg(zones_->getFTP(i)));
            add->setFont(column++, font);
        }

        // W'
        add->setText(column, QString("%1").arg(zones_->getWprime(i)));
        add->setFont(column++, font);

        // Pmax
        add->setText(column, QString("%1").arg(zones_->getPmax(i)));
        add->setFont(column++, font);
    }

}



void
CPPage::addClicked()
{

    // get current scheme
    zones_->setScheme(schemePage->getScheme());

    int cp = cpEdit->value();
    if( cp <= 0 ) {
        QMessageBox err;
        err.setText(tr("CP must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    //int index = ranges->invisibleRootItem()->childCount();
    int wp = wEdit->value() ? wEdit->value() : 20000;
    if (wp < 1000) wp *= 1000; // entered in kJ we want joules

    int pmax = pmaxEdit->value() ? pmaxEdit->value() : 1000;

    int index = zones_->addZoneRange(dateEdit->date(), cpEdit->value(), ftpEdit->value(), wp, pmax);

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    ranges->invisibleRootItem()->insertChild(index, add);

    int column = 0;

    // date
    add->setText(column++, dateEdit->date().toString(tr("MMM d, yyyy")));

    // CP
    add->setText(column++, QString("%1").arg(cpEdit->value()));

    // FTP
    if (useCPForFTPCombo->currentIndex() == 1) {
        add->setText(column++, QString("%1").arg(ftpEdit->value()));
    }

    // W'
    add->setText(column++, QString("%1").arg(wp));

    // Pmax
    add->setText(column++, QString("%1").arg(pmax));

}

void
CPPage::editClicked()
{
    // get current scheme
    zones_->setScheme(schemePage->getScheme());

    int cp = cpEdit->value();

    if( cp <= 0 ){
        QMessageBox err;
        err.setText(tr("CP must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int ftp = ftpEdit->value() ? ftpEdit->value() : cp;
    int wp = wEdit->value() ? wEdit->value() : 20000;
    if (wp < 1000) wp *= 1000; // entered in kJ we want joules

    int pmax = pmaxEdit->value() ? pmaxEdit->value() : 1000;

    QTreeWidgetItem *edit = ranges->selectedItems().at(0);
    int index = ranges->indexOfTopLevelItem(edit);


    int columns = 0;

    // date
    zones_->setStartDate(index, dateEdit->date());
    edit->setText(columns++, dateEdit->date().toString(tr("MMM d, yyyy")));

    // CP
    zones_->setCP(index, cp);
    edit->setText(columns++, QString("%1").arg(cp));

    // show FTP if we use FTP for Coggan Metrics
    if (useCPForFTPCombo->currentIndex() == 1) {
        zones_->setFTP(index, ftp);
        edit->setText(columns++, QString("%1").arg(ftp));
    }

    // W'
    zones_->setWprime(index, wp);
    edit->setText(columns++, QString("%1").arg(wp));

    // Pmax
    zones_->setPmax(index, pmax);
    edit->setText(columns++, QString("%1").arg(pmax));

}

void
CPPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        zones_->deleteRange(index);
    }
}

void
CPPage::defaultClicked()
{
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        ZoneRange current = zones_->getZoneRange(index);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);


        // set the range to use defaults on the scheme page
        zones_->setScheme(schemePage->getScheme());
        zones_->setZonesFromCP(index);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
    }
}

void
CPPage::rangeEdited()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());

        QDate date = dateEdit->date();
        QDate odate = zones_->getStartDate(index);

        int cp = cpEdit->value();
        int ocp = zones_->getCP(index);

        int ftp = ftpEdit->value();
        int oftp = zones_->getFTP(index);

        int wp = wEdit->value();
        int owp = zones_->getWprime(index);

        int pmax = pmaxEdit->value();
        int opmax = zones_->getPmax(index);

        if (date != odate || cp != ocp || ftp != oftp || wp != owp || pmax != opmax)
            updateButton->show();
        else
            updateButton->hide();
    }
}

void
CPPage::rangeSelectionChanged()
{
    active = true;

    // wipe away current contents of zones
    foreach (QTreeWidgetItem *item, zones->invisibleRootItem()->takeChildren()) {
        delete zones->itemWidget(item, 2);
        delete item;
    }

    // fill with current details
    if (ranges->currentItem()) {


        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        ZoneRange current = zones_->getZoneRange(index);

        dateEdit->setDate(zones_->getStartDate(index));
        cpEdit->setValue(zones_->getCP(index));
        ftpEdit->setValue(zones_->getFTP(index));
        wEdit->setValue(zones_->getWprime(index));
        pmaxEdit->setValue(zones_->getPmax(index));

        if (current.zonesSetFromCP) {

            // reapply the scheme in case it has been changed
            zones_->setScheme(schemePage->getScheme());
            zones_->setZonesFromCP(index);
            current = zones_->getZoneRange(index);

            defaultButton->hide();

        } else defaultButton->show();

        for (int i=0; i< current.zones.count(); i++) {

            QTreeWidgetItem *add = new QTreeWidgetItem(zones->invisibleRootItem());
            add->setFlags(add->flags() | Qt::ItemIsEditable);

            // tab name
            add->setText(0, current.zones[i].name);
            // field name
            add->setText(1, current.zones[i].desc);

            // low
            QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
            loedit->setMinimum(0);
            loedit->setMaximum(1000);
            loedit->setSingleStep(1.0);
            loedit->setDecimals(0);
            loedit->setValue(current.zones[i].lo);
            zones->setItemWidget(add, 2, loedit);
            connect(loedit, SIGNAL(valueChanged(double)), this, SLOT(zonesChanged()));
        }
    }

    active = false;
}

void
CPPage::addZoneClicked()
{
    // no range selected
    if (!ranges->currentItem()) return;

    // are we at maximum already?
    if (zones->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    active = true;
    int index = zones->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
    loedit->setMinimum(0);
    loedit->setMaximum(1000);
    loedit->setSingleStep(1.0);
    loedit->setDecimals(0);
    loedit->setValue(100);

    zones->invisibleRootItem()->insertChild(index, add);
    zones->setItemWidget(add, 2, loedit);
    connect(loedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

    // Short
    QString text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
    active = false;

    zonesChanged();
}

void
CPPage::deleteZoneClicked()
{
    // no range selected
    if (ranges->invisibleRootItem()->indexOfChild(ranges->currentItem()) == -1)
        return;

    active = true;
    if (zones->currentItem()) {
        int index = zones->invisibleRootItem()->indexOfChild(zones->currentItem());
        delete zones->invisibleRootItem()->takeChild(index);
    }
    active = false;

    zonesChanged();
}

void
CPPage::zonesChanged()
{
    // only take changes when they are not done programmatically
    // the active flag is set when the tree is being modified
    // programmatically, but not when users interact with the widgets
    if (active == false) {
        // get the current zone range
        if (ranges->currentItem()) {

            int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
            ZoneRange current = zones_->getZoneRange(index);

            // embolden that range on the list to show it has been edited
            QFont font;
            font.setWeight(QFont::Black);
            ranges->currentItem()->setFont(0, font);
            ranges->currentItem()->setFont(1, font);
            ranges->currentItem()->setFont(2, font);

            // show the default button to undo
            defaultButton->show();

            // we manually edited so save in full
            current.zonesSetFromCP = false;

            // create the new zoneinfos for this range
            QList<ZoneInfo> zoneinfos;
            for (int i=0; i< zones->invisibleRootItem()->childCount(); i++) {
                QTreeWidgetItem *item = zones->invisibleRootItem()->child(i);
                zoneinfos << ZoneInfo(item->text(0),
                                      item->text(1),
                                      ((QDoubleSpinBox*)zones->itemWidget(item, 2))->value(),
                                      0);
            }

            // now sort the list
            qSort(zoneinfos);

            // now fill the highs
            for(int i=0; i<zoneinfos.count(); i++) {
                if (i+1 <zoneinfos.count())
                    zoneinfos[i].hi = zoneinfos[i+1].lo;
                else
                    zoneinfos[i].hi = INT_MAX;
            }
            current.zones = zoneinfos;

            // now replace the current range struct
            zones_->setZoneRange(index, current);
        }
    }
}

//
// Zone Config page
//
HrZonePage::HrZonePage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hlayout = new QHBoxLayout;

    sportLabel = new QLabel(tr("Sport"));
    sportCombo = new QComboBox();
    sportCombo->addItem(tr("Bike"));
    sportCombo->addItem(tr("Run"));
    sportCombo->setCurrentIndex(0);
    hlayout->addStretch();
    hlayout->addWidget(sportLabel);
    hlayout->addWidget(sportCombo);
    hlayout->addStretch();
    layout->addLayout(hlayout);
    connect(sportCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSport(int)));
    tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    for (int i=0; i < nSports; i++) {
        hrZones[i] = new HrZones(i > 0);

        // get current config by reading it in (leave mainwindow zones alone)
        QFile zonesFile(context->athlete->home->config().canonicalPath() + "/" + hrZones[i]->fileName());
        if (zonesFile.exists()) {
            hrZones[i]->read(zonesFile);
            zonesFile.close();
            b4Fingerprint[i] = hrZones[i]->getFingerprint(); // remember original state
        }

        // setup maintenance pages using current config
        schemePage[i] = new HrSchemePage(hrZones[i]);
        ltPage[i] = new LTPage(context, hrZones[i], schemePage[i]);
    }

    // finish setup for the default sport
    changeSport(sportCombo->currentIndex());
}

HrZonePage::~HrZonePage()
{
    for (int i=0; i<nSports; i++) delete hrZones[i];
}

void
HrZonePage::changeSport(int i)
{
    // change tabs according to the selected sport
    tabs->clear();
    tabs->addTab(ltPage[i], tr("Lactate Threshold"));
    tabs->addTab(schemePage[i], tr("Default"));
}

qint32
HrZonePage::saveClicked()
{
    qint32 changed = 0;

    // write
    for (int i=0; i < nSports; i++) {
        hrZones[i]->setScheme(schemePage[i]->getScheme());
        hrZones[i]->write(context->athlete->home->config());

        // reread HR zones
        QFile hrzonesFile(context->athlete->home->config().canonicalPath() + "/" + context->athlete->hrzones_[i]->fileName());
        context->athlete->hrzones_[i]->read(hrzonesFile);

        // did we change ?
        if (hrZones[i]->getFingerprint() != b4Fingerprint[i])
            changed = CONFIG_ZONES;
    }

    return changed;
}

HrSchemePage::HrSchemePage(HrZones *hrZones) : hrZones(hrZones)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    scheme = new QTreeWidget;
    scheme->headerItem()->setText(0, tr("Short"));
    scheme->headerItem()->setText(1, tr("Long"));
    scheme->headerItem()->setText(2, tr("Percent of LT"));
    scheme->headerItem()->setText(3, tr("Trimp k"));
    scheme->setColumnCount(4);
    scheme->setSelectionMode(QAbstractItemView::SingleSelection);
    scheme->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    scheme->setUniformRowHeights(true);
    scheme->setIndentation(0);
    //scheme->header()->resizeSection(0,60);
    //scheme->header()->resizeSection(1,180);
    //scheme->header()->resizeSection(2,65);
    //scheme->header()->resizeSection(3,65);

    // setup list
    for (int i=0; i< hrZones->getScheme().nzones_default; i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, hrZones->getScheme().zone_default_name[i]);
        // field name
        add->setText(1, hrZones->getScheme().zone_default_desc[i]);

        // low
        QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
        loedit->setMinimum(0);
        loedit->setMaximum(1000);
        loedit->setSingleStep(1.0);
        loedit->setDecimals(0);
        loedit->setValue(hrZones->getScheme().zone_default[i]);
        scheme->setItemWidget(add, 2, loedit);

        // trimp
        QDoubleSpinBox *trimpedit = new QDoubleSpinBox(this);
        trimpedit->setMinimum(0);
        trimpedit->setMaximum(10);
        trimpedit->setSingleStep(0.1);
        trimpedit->setDecimals(2);
        trimpedit->setValue(hrZones->getScheme().zone_default_trimp[i]);
        scheme->setItemWidget(add, 3, trimpedit);
    }

    mainLayout->addWidget(scheme);
    mainLayout->addLayout(actionButtons);

    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
}

void
HrSchemePage::addClicked()
{
    // are we at maximum already?
    if (scheme->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int index = scheme->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
    loedit->setMinimum(0);
    loedit->setMaximum(1000);
    loedit->setSingleStep(1.0);
    loedit->setDecimals(0);
    loedit->setValue(100);

    scheme->invisibleRootItem()->insertChild(index, add);
    scheme->setItemWidget(add, 2, loedit);

    //trimp
    QDoubleSpinBox *trimpedit = new QDoubleSpinBox(this);
    trimpedit->setMinimum(0);
    trimpedit->setMaximum(10);
    trimpedit->setSingleStep(0.1);
    trimpedit->setDecimals(2);
    trimpedit->setValue(1);

    scheme->setItemWidget(add, 3, trimpedit);

    // Short
    QString text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
}

void
HrSchemePage::renameClicked()
{
    // which one is selected?
    if (scheme->currentItem()) scheme->editItem(scheme->currentItem(), 0);
}

void
HrSchemePage::deleteClicked()
{
    if (scheme->currentItem()) {
        int index = scheme->invisibleRootItem()->indexOfChild(scheme->currentItem());
        delete scheme->invisibleRootItem()->takeChild(index);
    }
}

HrZoneScheme
HrSchemePage::getScheme()
{
    // read the scheme widget and return a scheme object
    QList<schemeitem> table;
    HrZoneScheme results;

    // read back the details from the table
    for (int i=0; i<scheme->invisibleRootItem()->childCount(); i++) {

        schemeitem add;
        add.name = scheme->invisibleRootItem()->child(i)->text(0);
        add.desc = scheme->invisibleRootItem()->child(i)->text(1);
        add.lo = ((QDoubleSpinBox *)(scheme->itemWidget(scheme->invisibleRootItem()->child(i), 2)))->value();
        add.trimp = ((QDoubleSpinBox *)(scheme->itemWidget(scheme->invisibleRootItem()->child(i), 3)))->value();
        table.append(add);
    }

    // sort the list into ascending order
    qSort(table);

    // now update the results
    results.nzones_default = 0;
    foreach(schemeitem zone, table) {
        results.nzones_default++;
        results.zone_default.append(zone.lo);
        results.zone_default_is_pct.append(true);
        results.zone_default_name.append(zone.name);
        results.zone_default_desc.append(zone.desc);
        results.zone_default_trimp.append(zone.trimp);
    }

    return results;
}


LTPage::LTPage(Context *context, HrZones *hrZones, HrSchemePage *schemePage) :
               context(context), hrZones(hrZones), schemePage(schemePage)
{
    active = false;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5);

    updateButton = new QPushButton(tr("Update"));
    updateButton->hide();
    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    updateButton->setFixedSize(60,20);
    deleteButton->setFixedSize(20,20);
#else
    updateButton->setText(tr("Update"));
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    defaultButton = new QPushButton(tr("Def"));
    defaultButton->hide();

    addZoneButton = new QPushButton(tr("+"));
    deleteZoneButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addZoneButton->setFixedSize(20,20);
    deleteZoneButton->setFixedSize(20,20);
#else
    addZoneButton->setText(tr("Add"));
    deleteZoneButton->setText(tr("Delete"));
#endif

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addStretch();
    actionButtons->addWidget(updateButton);
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);
    //actionButtons->addWidget(defaultButton); moved to zoneButtons

    QHBoxLayout *zoneButtons = new QHBoxLayout;
    zoneButtons->addStretch();
    zoneButtons->addWidget(addZoneButton);
    zoneButtons->addWidget(deleteZoneButton);
    zoneButtons->addWidget(defaultButton);

    QHBoxLayout *addLayout = new QHBoxLayout;
    QLabel *dateLabel = new QLabel(tr("From Date"));
    QLabel *ltLabel = new QLabel(tr("Lactate Threshold"));
    dateEdit = new QDateEdit;
    dateEdit->setDate(QDate::currentDate());

    ltEdit = new QDoubleSpinBox;
    ltEdit->setMinimum(0);
    ltEdit->setMaximum(240);
    ltEdit->setSingleStep(1.0);
    ltEdit->setDecimals(0);

    addLayout->addWidget(dateLabel);
    addLayout->addWidget(dateEdit);
    addLayout->addWidget(ltLabel);
    addLayout->addWidget(ltEdit);
    addLayout->addStretch();

    QHBoxLayout *addLayout2 = new QHBoxLayout;
    QLabel *restHrLabel = new QLabel(tr("Rest HR"));
    QLabel *maxHrLabel = new QLabel(tr("Max HR"));

    restHrEdit = new QDoubleSpinBox;
    restHrEdit->setMinimum(0);
    restHrEdit->setMaximum(240);
    restHrEdit->setSingleStep(1.0);
    restHrEdit->setDecimals(0);

    maxHrEdit = new QDoubleSpinBox;
    maxHrEdit->setMinimum(0);
    maxHrEdit->setMaximum(240);
    maxHrEdit->setSingleStep(1.0);
    maxHrEdit->setDecimals(0);

    addLayout2->addWidget(restHrLabel);
    addLayout2->addWidget(restHrEdit);
    addLayout2->addWidget(maxHrLabel);
    addLayout2->addWidget(maxHrEdit);
    addLayout2->addStretch();

    ranges = new QTreeWidget;
    ranges->headerItem()->setText(0, tr("From Date"));
    ranges->headerItem()->setText(1, tr("Lactate Threshold"));
    ranges->headerItem()->setText(2, tr("Rest HR"));
    ranges->headerItem()->setText(3, tr("Max HR"));
    ranges->setColumnCount(4);
    ranges->setSelectionMode(QAbstractItemView::SingleSelection);
    //ranges->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    ranges->setUniformRowHeights(true);
    ranges->setIndentation(0);
    //ranges->header()->resizeSection(0,180);

    // setup list of ranges
    for (int i=0; i< hrZones->getRangeSize(); i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(hrZones->getHrZoneRange(i).hrZonesSetFromLT ?
                       QFont::Normal : QFont::Black);

        // date
        add->setText(0, hrZones->getStartDate(i).toString(tr("MMM d, yyyy")));
        add->setFont(0, font);

        // LT
        add->setText(1, QString("%1").arg(hrZones->getLT(i)));
        add->setFont(1, font);

        // Rest HR
        add->setText(2, QString("%1").arg(hrZones->getRestHr(i)));
        add->setFont(2, font);

        // Max HR
        add->setText(3, QString("%1").arg(hrZones->getMaxHr(i)));
        add->setFont(3, font);
    }

    zones = new QTreeWidget;
    zones->headerItem()->setText(0, tr("Short"));
    zones->headerItem()->setText(1, tr("Long"));
    zones->headerItem()->setText(2, tr("From BPM"));
    zones->headerItem()->setText(3, tr("Trimp k"));
    zones->setColumnCount(4);
    zones->setSelectionMode(QAbstractItemView::SingleSelection);
    zones->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    zones->setUniformRowHeights(true);
    zones->setIndentation(0);
    //zones->header()->resizeSection(0,50);
    //zones->header()->resizeSection(1,150);
    //zones->header()->resizeSection(2,65);
    //zones->header()->resizeSection(3,65);

    mainLayout->addLayout(addLayout);
    mainLayout->addLayout(addLayout2);
    mainLayout->addLayout(actionButtons);
    mainLayout->addWidget(ranges);
    mainLayout->addLayout(zoneButtons);
    mainLayout->addWidget(zones);

    // edit connect
    connect(dateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(rangeEdited()));
    connect(ltEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(restHrEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));
    connect(maxHrEdit, SIGNAL(valueChanged(double)), this, SLOT(rangeEdited()));

    // button connect
    connect(updateButton, SIGNAL(clicked()), this, SLOT(editClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultClicked()));
    connect(addZoneButton, SIGNAL(clicked()), this, SLOT(addZoneClicked()));
    connect(deleteZoneButton, SIGNAL(clicked()), this, SLOT(deleteZoneClicked()));
    connect(ranges, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));
    connect(zones, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(zonesChanged()));
}

void
LTPage::addClicked()
{
    // get current scheme
    hrZones->setScheme(schemePage->getScheme());

    //int index = ranges->invisibleRootItem()->childCount();
    int index = hrZones->addHrZoneRange(dateEdit->date(), ltEdit->value(), restHrEdit->value(), maxHrEdit->value());

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    ranges->invisibleRootItem()->insertChild(index, add);

    // date
    add->setText(0, dateEdit->date().toString(tr("MMM d, yyyy")));

    // LT
    add->setText(1, QString("%1").arg(ltEdit->value()));
    // Rest HR
    add->setText(2, QString("%1").arg(restHrEdit->value()));
    // Max HR
    add->setText(3, QString("%1").arg(maxHrEdit->value()));
}

void
LTPage::editClicked()
{
    // get current scheme
    hrZones->setScheme(schemePage->getScheme());

    QTreeWidgetItem *edit = ranges->selectedItems().at(0);
    int index = ranges->indexOfTopLevelItem(edit);

    // date
    edit->setText(0, dateEdit->date().toString(tr("MMM d, yyyy")));

    // LT
    hrZones->setLT(index, ltEdit->value());
    edit->setText(1, QString("%1").arg(ltEdit->value()));
    // Rest HR
    hrZones->setRestHr(index, restHrEdit->value());
    edit->setText(2, QString("%1").arg(restHrEdit->value()));
    // Max HR
    hrZones->setMaxHr(index, maxHrEdit->value());
    edit->setText(3, QString("%1").arg(maxHrEdit->value()));
}

void
LTPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        hrZones->deleteRange(index);
    }
}

void
LTPage::defaultClicked()
{
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        HrZoneRange current = hrZones->getHrZoneRange(index);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);
        ranges->currentItem()->setFont(3, font);


        // set the range to use defaults on the scheme page
        hrZones->setScheme(schemePage->getScheme());
        hrZones->setHrZonesFromLT(index);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
    }
}

void
LTPage::rangeEdited()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());

        QDate date = dateEdit->date();
        QDate odate = hrZones->getStartDate(index);

        int lt = ltEdit->value();
        int olt = hrZones->getLT(index);

        int maxhr = maxHrEdit->value();
        int omaxhr = hrZones->getMaxHr(index);

        int resthr = restHrEdit->value();
        int oresthr = hrZones->getRestHr(index);

        if (date != odate || lt != olt || maxhr != omaxhr || resthr != oresthr)
            updateButton->show();
        else
            updateButton->hide();
    }
}

void
LTPage::rangeSelectionChanged()
{
    active = true;

    // wipe away current contents of zones
    foreach (QTreeWidgetItem *item, zones->invisibleRootItem()->takeChildren()) {
        delete zones->itemWidget(item, 2);
        delete item;
    }

    // fill with current details
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        HrZoneRange current = hrZones->getHrZoneRange(index);

        dateEdit->setDate(hrZones->getStartDate(index));
        ltEdit->setValue(hrZones->getLT(index));
        maxHrEdit->setValue(hrZones->getMaxHr(index));
        restHrEdit->setValue(hrZones->getRestHr(index));

        if (current.hrZonesSetFromLT) {

            // reapply the scheme in case it has been changed
            hrZones->setScheme(schemePage->getScheme());
            hrZones->setHrZonesFromLT(index);
            current = hrZones->getHrZoneRange(index);

            defaultButton->hide();

        } else defaultButton->show();

        for (int i=0; i< current.zones.count(); i++) {

            QTreeWidgetItem *add = new QTreeWidgetItem(zones->invisibleRootItem());
            add->setFlags(add->flags() | Qt::ItemIsEditable);

            // tab name
            add->setText(0, current.zones[i].name);
            // field name
            add->setText(1, current.zones[i].desc);

            // low
            QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
            loedit->setMinimum(0);
            loedit->setMaximum(1000);
            loedit->setSingleStep(1.0);
            loedit->setDecimals(0);
            loedit->setValue(current.zones[i].lo);
            zones->setItemWidget(add, 2, loedit);
            connect(loedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

            //trimp
            QDoubleSpinBox *trimpedit = new QDoubleSpinBox(this);
            trimpedit->setMinimum(0);
            trimpedit->setMaximum(10);
            trimpedit->setSingleStep(0.1);
            trimpedit->setDecimals(2);
            trimpedit->setValue(current.zones[i].trimp);
            zones->setItemWidget(add, 3, trimpedit);
            connect(trimpedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

        }
    }

    active = false;
}

void
LTPage::addZoneClicked()
{
    // no range selected
    if (!ranges->currentItem()) return;

    // are we at maximum already?
    if (zones->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    active = true;
    int index = zones->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
    loedit->setMinimum(0);
    loedit->setMaximum(1000);
    loedit->setSingleStep(1.0);
    loedit->setDecimals(0);
    loedit->setValue(100);

    zones->invisibleRootItem()->insertChild(index, add);
    zones->setItemWidget(add, 2, loedit);
    connect(loedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

    //trimp
    QDoubleSpinBox *trimpedit = new QDoubleSpinBox(this);
    trimpedit->setMinimum(0);
    trimpedit->setMaximum(10);
    trimpedit->setSingleStep(0.1);
    trimpedit->setDecimals(2);
    trimpedit->setValue(1);
    zones->setItemWidget(add, 3, trimpedit);
    connect(trimpedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

    // Short
    QString text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
    active = false;

    zonesChanged();
}

void
LTPage::deleteZoneClicked()
{
    // no range selected
    if (ranges->invisibleRootItem()->indexOfChild(ranges->currentItem()) == -1)
        return;

    active = true;
    if (zones->currentItem()) {
        int index = zones->invisibleRootItem()->indexOfChild(zones->currentItem());
        delete zones->invisibleRootItem()->takeChild(index);
    }
    active = false;

    zonesChanged();
}

void
LTPage::zonesChanged()
{
    // only take changes when they are not done programmatically
    // the active flag is set when the tree is being modified
    // programmatically, but not when users interact with the widgets
    if (active == false) {
        // get the current zone range
        if (ranges->currentItem()) {

            int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
            HrZoneRange current = hrZones->getHrZoneRange(index);

            // embolden that range on the list to show it has been edited
            QFont font;
            font.setWeight(QFont::Black);
            ranges->currentItem()->setFont(0, font);
            ranges->currentItem()->setFont(1, font);
            ranges->currentItem()->setFont(2, font);
            ranges->currentItem()->setFont(3, font);

            // show the default button to undo
            defaultButton->show();

            // we manually edited so save in full
            current.hrZonesSetFromLT = false;

            // create the new zoneinfos for this range
            QList<HrZoneInfo> zoneinfos;
            for (int i=0; i< zones->invisibleRootItem()->childCount(); i++) {
                QTreeWidgetItem *item = zones->invisibleRootItem()->child(i);
                zoneinfos << HrZoneInfo(item->text(0),
                                        item->text(1),
                                        ((QDoubleSpinBox*)zones->itemWidget(item, 2))->value(),
                                        0, ((QDoubleSpinBox*)zones->itemWidget(item, 3))->value());
            }

            // now sort the list
            qSort(zoneinfos);

            // now fill the highs
            for(int i=0; i<zoneinfos.count(); i++) {
                if (i+1 <zoneinfos.count())
                    zoneinfos[i].hi = zoneinfos[i+1].lo;
                else
                    zoneinfos[i].hi = INT_MAX;
            }
            current.zones = zoneinfos;

            // now replace the current range struct
            hrZones->setHrZoneRange(index, current);
        }
    }
}

//
// Pace Zone Config page
//
PaceZonePage::PaceZonePage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *hlayout = new QHBoxLayout;

    sportLabel = new QLabel(tr("Sport"));
    sportCombo = new QComboBox();
    sportCombo->addItem(tr("Run"));
    sportCombo->addItem(tr("Swim"));
    sportCombo->setCurrentIndex(0);
    hlayout->addStretch();
    hlayout->addWidget(sportLabel);
    hlayout->addWidget(sportCombo);
    hlayout->addStretch();
    layout->addLayout(hlayout);
    connect(sportCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(changeSport(int)));
    tabs = new QTabWidget(this);
    layout->addWidget(tabs);

    for (int i=0; i < nSports; i++) {
        paceZones[i] = new PaceZones(i > 0);

        // get current config by reading it in (leave mainwindow zones alone)
        QFile zonesFile(context->athlete->home->config().canonicalPath() + "/" + paceZones[i]->fileName());
        if (zonesFile.exists()) {
            paceZones[i]->read(zonesFile);
            zonesFile.close();
            b4Fingerprint[i] = paceZones[i]->getFingerprint(); // remember original state
        }

        // setup maintenance pages using current config
        schemePages[i] = new PaceSchemePage(paceZones[i]);
        cvPages[i] = new CVPage(paceZones[i], schemePages[i]);
    }

    // finish setup for the default sport
    changeSport(sportCombo->currentIndex());
}

PaceZonePage::~PaceZonePage()
{
    for (int i=0; i<nSports; i++) delete paceZones[i];
}

void
PaceZonePage::changeSport(int i)
{
    // change tabs according to the selected sport
    tabs->clear();
    tabs->addTab(cvPages[i], tr("Critical Velocity"));
    tabs->addTab(schemePages[i], tr("Default"));
}

qint32
PaceZonePage::saveClicked()
{
    qint32 changed = 0;
    // write it
    for (int i=0; i < nSports; i++) {
        appsettings->setValue(paceZones[i]->paceSetting(), cvPages[i]->metric->isChecked());
        paceZones[i]->setScheme(schemePages[i]->getScheme());
        paceZones[i]->write(context->athlete->home->config());

        // reread Pace zones
        QFile pacezonesFile(context->athlete->home->config().canonicalPath() + "/" + context->athlete->pacezones_[i]->fileName());
        context->athlete->pacezones_[i]->read(pacezonesFile);

        // did we change ?
        if (paceZones[i]->getFingerprint() != b4Fingerprint[i])
            changed = CONFIG_ZONES;
    }
    return changed;
}

PaceSchemePage::PaceSchemePage(PaceZones* paceZones) : paceZones(paceZones)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    scheme = new QTreeWidget;
    scheme->headerItem()->setText(0, tr("Short"));
    scheme->headerItem()->setText(1, tr("Long"));
    scheme->headerItem()->setText(2, tr("Percent of CV"));
    scheme->setColumnCount(3);
    scheme->setSelectionMode(QAbstractItemView::SingleSelection);
    scheme->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    scheme->setUniformRowHeights(true);
    scheme->setIndentation(0);
    //scheme->header()->resizeSection(0,90);
    //scheme->header()->resizeSection(1,200);
    //scheme->header()->resizeSection(2,80);

    // setup list
    for (int i=0; i< paceZones->getScheme().nzones_default; i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, paceZones->getScheme().zone_default_name[i]);
        // field name
        add->setText(1, paceZones->getScheme().zone_default_desc[i]);

        // low
        QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
        loedit->setMinimum(0);
        loedit->setMaximum(1000);
        loedit->setSingleStep(1.0);
        loedit->setDecimals(0);
        loedit->setValue(paceZones->getScheme().zone_default[i]);
        scheme->setItemWidget(add, 2, loedit);
    }

    mainLayout->addWidget(scheme);
    mainLayout->addLayout(actionButtons);

    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
}

void
PaceSchemePage::addClicked()
{
    // are we at maximum already?
    if (scheme->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int index = scheme->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
    loedit->setMinimum(0);
    loedit->setMaximum(1000);
    loedit->setSingleStep(1.0);
    loedit->setDecimals(0);
    loedit->setValue(100);

    scheme->invisibleRootItem()->insertChild(index, add);
    scheme->setItemWidget(add, 2, loedit);

    // Short
    QString text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
}

void
PaceSchemePage::renameClicked()
{
    // which one is selected?
    if (scheme->currentItem()) scheme->editItem(scheme->currentItem(), 0);
}

void
PaceSchemePage::deleteClicked()
{
    if (scheme->currentItem()) {
        int index = scheme->invisibleRootItem()->indexOfChild(scheme->currentItem());
        delete scheme->invisibleRootItem()->takeChild(index);
    }
}

// just for qSorting
struct paceschemeitem {
    QString name, desc;
    int lo;
    double trimp;
    bool operator<(paceschemeitem right) const { return lo < right.lo; }
};

PaceZoneScheme
PaceSchemePage::getScheme()
{
    // read the scheme widget and return a scheme object
    QList<paceschemeitem> table;
    PaceZoneScheme results;

    // read back the details from the table
    for (int i=0; i<scheme->invisibleRootItem()->childCount(); i++) {

        paceschemeitem add;
        add.name = scheme->invisibleRootItem()->child(i)->text(0);
        add.desc = scheme->invisibleRootItem()->child(i)->text(1);
        add.lo = ((QDoubleSpinBox *)(scheme->itemWidget(scheme->invisibleRootItem()->child(i), 2)))->value();
        table.append(add);
    }

    // sort the list into ascending order
    qSort(table);

    // now update the results
    results.nzones_default = 0;
    foreach(paceschemeitem zone, table) {
        results.nzones_default++;
        results.zone_default.append(zone.lo);
        results.zone_default_is_pct.append(true);
        results.zone_default_name.append(zone.name);
        results.zone_default_desc.append(zone.desc);
    }

    return results;
}

CVPage::CVPage(PaceZones* paceZones, PaceSchemePage *schemePage) :
               paceZones(paceZones), schemePage(schemePage)
{
    active = false;

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setSpacing(10);

    updateButton = new QPushButton(tr("Update"));
    updateButton->hide();
    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    updateButton->setFixedSize(60,20);
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    updateButton->setText(tr("Update"));
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
#endif
    defaultButton = new QPushButton(tr("Def"));
    defaultButton->hide();

    addZoneButton = new QPushButton(tr("+"));
    deleteZoneButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addZoneButton->setFixedSize(20,20);
    deleteZoneButton->setFixedSize(20,20);
#else
    addZoneButton->setText(tr("Add"));
    deleteZoneButton->setText(tr("Delete"));
#endif

    QHBoxLayout *zoneButtons = new QHBoxLayout;
    zoneButtons->addStretch();
    zoneButtons->setSpacing(0);
    zoneButtons->addWidget(addZoneButton);
    zoneButtons->addWidget(deleteZoneButton);

    QHBoxLayout *addLayout = new QHBoxLayout;
    QLabel *dateLabel = new QLabel(tr("From Date"));
    QLabel *cpLabel = new QLabel(tr("Critical Velocity"));
    dateEdit = new QDateEdit;
    dateEdit->setDate(QDate::currentDate());

    // CV default is 4min/km for Running a round number inline with
    // CP default and 1:36min/100 for swimming (4:1 relation)
    cvEdit = new QTimeEdit(QTime::fromString(paceZones->isSwim() ? "01:36" : "04:00", "mm:ss"));
    cvEdit->setMinimumTime(QTime::fromString("01:00", "mm:ss"));
    cvEdit->setMaximumTime(QTime::fromString("20:00", "mm:ss"));
    cvEdit->setDisplayFormat("mm:ss");

    per = new QLabel(this);
    metric = new QCheckBox(tr("Metric Pace"));
    metric->setChecked(appsettings->value(this, paceZones->paceSetting(), true).toBool());
    per->setText(paceZones->paceUnits(metric->isChecked()));
    if (!metric->isChecked()) metricChanged(); // default is metric

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addWidget(cpLabel);
    actionButtons->addWidget(cvEdit);
    actionButtons->addWidget(per);
    actionButtons->addStretch();
    actionButtons->addWidget(metric);
    actionButtons->addStretch();
    actionButtons->addWidget(updateButton);
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(defaultButton);

    addLayout->addWidget(dateLabel);
    addLayout->addWidget(dateEdit);
    addLayout->addStretch();

    ranges = new QTreeWidget;
    ranges->headerItem()->setText(0, tr("From Date"));
    ranges->headerItem()->setText(1, tr("Critical Velocity"));
    ranges->setColumnCount(2);
    ranges->setSelectionMode(QAbstractItemView::SingleSelection);
    //ranges->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    ranges->setUniformRowHeights(true);
    ranges->setIndentation(0);
    //ranges->header()->resizeSection(0,180);

    // setup list of ranges
    for (int i=0; i< paceZones->getRangeSize(); i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(paceZones->getZoneRange(i).zonesSetFromCV ?
                                        QFont::Normal : QFont::Black);

        // date
        add->setText(0, paceZones->getStartDate(i).toString(tr("MMM d, yyyy")));
        add->setFont(0, font);

        // CV
        double kph = paceZones->getCV(i);

        add->setText(1, QString("%1 %2 %3 %4")
                    .arg(paceZones->kphToPaceString(kph, true))
                    .arg(paceZones->paceUnits(true))
                    .arg(paceZones->kphToPaceString(kph, false))
                    .arg(paceZones->paceUnits(false)));
        add->setFont(1, font);

    }

    zones = new QTreeWidget;
    zones->headerItem()->setText(0, tr("Short"));
    zones->headerItem()->setText(1, tr("Long"));
    zones->headerItem()->setText(2, tr("From"));
    zones->setColumnCount(3);
    zones->setSelectionMode(QAbstractItemView::SingleSelection);
    zones->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    zones->setUniformRowHeights(true);
    zones->setIndentation(0);
    //zones->header()->resizeSection(0,80);
    //zones->header()->resizeSection(1,150);

    mainLayout->addLayout(addLayout, 0,0);
    mainLayout->addLayout(actionButtons, 1,0);
    mainLayout->addWidget(ranges, 2,0);
    mainLayout->addLayout(zoneButtons, 3,0);
    mainLayout->addWidget(zones, 4,0);

    // edit connect
    connect(dateEdit, SIGNAL(dateChanged(QDate)), this, SLOT(rangeEdited()));
    connect(cvEdit, SIGNAL(timeChanged(QTime)), this, SLOT(rangeEdited()));

    // button connect
    connect(updateButton, SIGNAL(clicked()), this, SLOT(editClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultClicked()));
    connect(addZoneButton, SIGNAL(clicked()), this, SLOT(addZoneClicked()));
    connect(deleteZoneButton, SIGNAL(clicked()), this, SLOT(deleteZoneClicked()));
    connect(ranges, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));
    connect(zones, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(zonesChanged()));
    connect(metric, SIGNAL(stateChanged(int)), this, SLOT(metricChanged()));
}

void
CVPage::metricChanged()
{
    // Switch between metric and imperial!
    per->setText(paceZones->paceUnits(metric->isChecked()));
    double kphCV = paceZones->kphFromTime(cvEdit, !metric->isChecked());
    cvEdit->setTime(QTime::fromString(paceZones->kphToPaceString(kphCV, metric->isChecked()), "mm:ss"));
}

void
CVPage::addClicked()
{
    // get current scheme
    paceZones->setScheme(schemePage->getScheme());

    int cp = paceZones->kphFromTime(cvEdit, metric->isChecked());
    if( cp <= 0 ){
        QMessageBox err;
        err.setText(tr("CV must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int index = paceZones->addZoneRange(dateEdit->date(), paceZones->kphFromTime(cvEdit, metric->isChecked()));

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    ranges->invisibleRootItem()->insertChild(index, add);

    // date
    add->setText(0, dateEdit->date().toString(tr("MMM d, yyyy")));

    // CV
    double kph = paceZones->kphFromTime(cvEdit, metric->isChecked());

    add->setText(1, QString("%1 %2 %3 %4")
            .arg(paceZones->kphToPaceString(kph, true))
            .arg(paceZones->paceUnits(true))
            .arg(paceZones->kphToPaceString(kph, false))
            .arg(paceZones->paceUnits(false)));

}

void
CVPage::editClicked()
{
    // get current scheme
    paceZones->setScheme(schemePage->getScheme());

    QTreeWidgetItem *edit = ranges->selectedItems().at(0);
    int index = ranges->indexOfTopLevelItem(edit);

    // date
    edit->setText(0, dateEdit->date().toString(tr("MMM d, yyyy")));

    // CV
    double kph = paceZones->kphFromTime(cvEdit, metric->isChecked());
    paceZones->setCV(index, kph);
    edit->setText(1, QString("%1 %2 %3 %4")
            .arg(paceZones->kphToPaceString(kph, true))
            .arg(paceZones->paceUnits(true))
            .arg(paceZones->kphToPaceString(kph, false))
            .arg(paceZones->paceUnits(false)));
}

void
CVPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        paceZones->deleteRange(index);
    }
}

void
CVPage::defaultClicked()
{
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        PaceZoneRange current = paceZones->getZoneRange(index);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);


        // set the range to use defaults on the scheme page
        paceZones->setScheme(schemePage->getScheme());
        paceZones->setZonesFromCV(index);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
    }
}

void
CVPage::rangeEdited()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());

        QDate date = dateEdit->date();
        QDate odate = paceZones->getStartDate(index);
        QTime cv = cvEdit->time();
        QTime ocv = QTime::fromString(paceZones->kphToPaceString(paceZones->getCV(index), metric->isChecked()), "mm:ss");

        if (date != odate || cv != ocv)
            updateButton->show();
        else
            updateButton->hide();
    }
}

void
CVPage::rangeSelectionChanged()
{
    active = true;

    // wipe away current contents of zones
    foreach (QTreeWidgetItem *item, zones->invisibleRootItem()->takeChildren()) {
        delete zones->itemWidget(item, 2);
        delete item;
    }

    // fill with current details
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        PaceZoneRange current = paceZones->getZoneRange(index);
        dateEdit->setDate(paceZones->getStartDate(index));
        cvEdit->setTime(QTime::fromString(paceZones->kphToPaceString(paceZones->getCV(index), metric->isChecked()), "mm:ss"));

        if (current.zonesSetFromCV) {

            // reapply the scheme in case it has been changed
            paceZones->setScheme(schemePage->getScheme());
            paceZones->setZonesFromCV(index);
            current = paceZones->getZoneRange(index);

            defaultButton->hide();

        } else defaultButton->show();

        for (int i=0; i< current.zones.count(); i++) {

            QTreeWidgetItem *add = new QTreeWidgetItem(zones->invisibleRootItem());
            add->setFlags(add->flags() | Qt::ItemIsEditable);

            // tab name
            add->setText(0, current.zones[i].name);
            // field name
            add->setText(1, current.zones[i].desc);

            // low
            QTimeEdit *loedit = new QTimeEdit(QTime::fromString(paceZones->kphToPaceString(current.zones[i].lo, metric->isChecked()), "mm:ss"), this);
            loedit->setMinimumTime(QTime::fromString("00:00", "mm:ss"));
            loedit->setMaximumTime(QTime::fromString("20:00", "mm:ss"));
            loedit->setDisplayFormat("mm:ss");
            zones->setItemWidget(add, 2, loedit);
            connect(loedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));
        }
    }

    active = false;
}

void
CVPage::addZoneClicked()
{
    // no range selected
    if (!ranges->currentItem()) return;

    // are we at maximum already?
    if (zones->invisibleRootItem()->childCount() == 10) {
        QMessageBox err;
        err.setText(tr("Maximum of 10 zones reached."));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    active = true;
    int index = zones->invisibleRootItem()->childCount();

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    QTimeEdit *loedit = new QTimeEdit(QTime::fromString("00:00", "mm:ss"), this);
    loedit->setMinimumTime(QTime::fromString("00:00", "mm:ss"));
    loedit->setMaximumTime(QTime::fromString("20:00", "mm:ss"));
    loedit->setDisplayFormat("mm:ss");

    zones->invisibleRootItem()->insertChild(index, add);
    zones->setItemWidget(add, 2, loedit);
    connect(loedit, SIGNAL(editingFinished()), this, SLOT(zonesChanged()));

    // Short
    QString text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(0, text);

    // long
    text = tr("New");
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString(tr("New (%1)")).arg(i+1);
    }
    add->setText(1, text);
    active = false;

    zonesChanged();
}

void
CVPage::deleteZoneClicked()
{
    // no range selected
    if (ranges->invisibleRootItem()->indexOfChild(ranges->currentItem()) == -1)
        return;

    active = true;
    if (zones->currentItem()) {
        int index = zones->invisibleRootItem()->indexOfChild(zones->currentItem());
        delete zones->invisibleRootItem()->takeChild(index);
    }
    active = false;

    zonesChanged();
}

void
CVPage::zonesChanged()
{
    // only take changes when they are not done programmatically
    // the active flag is set when the tree is being modified
    // programmatically, but not when users interact with the widgets
    if (active == false) {
        // get the current zone range
        if (ranges->currentItem()) {

            int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
            PaceZoneRange current = paceZones->getZoneRange(index);

            // embolden that range on the list to show it has been edited
            QFont font;
            font.setWeight(QFont::Black);
            ranges->currentItem()->setFont(0, font);
            ranges->currentItem()->setFont(1, font);
            ranges->currentItem()->setFont(2, font);

            // show the default button to undo
            defaultButton->show();

            // we manually edited so save in full
            current.zonesSetFromCV = false;

            // create the new zoneinfos for this range
            QList<PaceZoneInfo> zoneinfos;
            for (int i=0; i< zones->invisibleRootItem()->childCount(); i++) {
                QTreeWidgetItem *item = zones->invisibleRootItem()->child(i);
                QTimeEdit *loTimeEdit = (QTimeEdit*)zones->itemWidget(item, 2);
                double kph = loTimeEdit->time() == QTime(0,0,0) ? 0.0 : paceZones->kphFromTime(loTimeEdit, metric->isChecked());
                zoneinfos << PaceZoneInfo(item->text(0),
                                      item->text(1),
                                      kph,
                                      0);
            }

            // now sort the list
            qSort(zoneinfos);

            // now fill the highs
            for(int i=0; i<zoneinfos.count(); i++) {
                if (i+1 <zoneinfos.count())
                    zoneinfos[i].hi = zoneinfos[i+1].lo;
                else
                    zoneinfos[i].hi = INT_MAX;
            }
            current.zones = zoneinfos;

            // now replace the current range struct
            paceZones->setZoneRange(index, current);
        }
    }
}

//
// Season Editor
//
SeasonsPage::SeasonsPage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    QGridLayout *mainLayout = new QGridLayout(this);
    QFormLayout *editLayout = new QFormLayout;
    editLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    // get the list
    array = context->athlete->seasons->seasons;

    // Edit widgets
    nameEdit = new QLineEdit(this);
    typeEdit = new QComboBox(this);
    foreach(QString t, Season::types) typeEdit->addItem(t);
    typeEdit->setCurrentIndex(0);
    fromEdit = new QDateEdit(this);
    fromEdit->setCalendarPopup(true);

    toEdit = new QDateEdit(this);
    toEdit->setCalendarPopup(true);

    // set form
    editLayout->addRow(new QLabel(tr("Name")), nameEdit);
    editLayout->addRow(new QLabel(tr("Type")), typeEdit);
    editLayout->addRow(new QLabel(tr("From")), fromEdit);
    editLayout->addRow(new QLabel(tr("To")), toEdit);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    upButton = new QToolButton(this);
    downButton = new QToolButton(this);
    upButton->setArrowType(Qt::UpArrow);
    downButton->setArrowType(Qt::DownArrow);
    upButton->setFixedSize(20,20);
    downButton->setFixedSize(20,20);
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
    upButton = new QPushButton(tr("Up"));
    downButton = new QPushButton(tr("Down"));
#endif

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();

    seasons = new QTreeWidget;
    seasons->headerItem()->setText(0, tr("Name"));
    seasons->headerItem()->setText(1, tr("Type"));
    seasons->headerItem()->setText(2, tr("From"));
    seasons->headerItem()->setText(3, tr("To"));
    seasons->setColumnCount(4);
    seasons->setSelectionMode(QAbstractItemView::SingleSelection);
    //seasons->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    seasons->setUniformRowHeights(true);
    seasons->setIndentation(0);

#ifdef Q_OS_MAC
    //seasons->header()->resizeSection(0,160); // tab
    //seasons->header()->resizeSection(1,80); // name
    //seasons->header()->resizeSection(2,130); // type
    //seasons->header()->resizeSection(3,130); // values
#else
    //seasons->header()->resizeSection(0,160); // tab
    //seasons->header()->resizeSection(1,80); // name
    //seasons->header()->resizeSection(2,130); // type
    //seasons->header()->resizeSection(3,130); // values
#endif

    foreach(Season season, array) {
        QTreeWidgetItem *add;

        if (season.type == Season::temporary) continue;

        add = new QTreeWidgetItem(seasons->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // tab name
        add->setText(0, season.name);
        // type
        add->setText(1, Season::types[static_cast<int>(season.type)]);
        // from
        add->setText(2, season.start.toString(tr("ddd MMM d, yyyy")));
        // to
        add->setText(3, season.end.toString(tr("ddd MMM d, yyyy")));
        // guid -- hidden
        add->setText(4, season.id().toString());

    }
    seasons->setCurrentItem(seasons->invisibleRootItem()->child(0));

    mainLayout->addLayout(editLayout, 0,0);
    mainLayout->addWidget(addButton, 0,1, Qt::AlignTop);
    mainLayout->addWidget(seasons, 1,0);
    mainLayout->addLayout(actionButtons, 1,1);

    // set all the edits to a default value
    clearEdit();

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
}

void
SeasonsPage::clearEdit()
{
    typeEdit->setCurrentIndex(0);
    nameEdit->setText("");
    fromEdit->setDate(QDate::currentDate());
    toEdit->setDate(QDate::currentDate().addMonths(3));
}

void
SeasonsPage::upClicked()
{
    if (seasons->currentItem()) {
        int index = seasons->invisibleRootItem()->indexOfChild(seasons->currentItem());
        if (index == 0) return; // its at the top already

        // movin on up!
        QTreeWidgetItem* moved = seasons->invisibleRootItem()->takeChild(index);
        seasons->invisibleRootItem()->insertChild(index-1, moved);
        seasons->setCurrentItem(moved);

        // and move the array too
        array.move(index, index-1);
    }
}

void
SeasonsPage::downClicked()
{
    if (seasons->currentItem()) {
        int index = seasons->invisibleRootItem()->indexOfChild(seasons->currentItem());
        if (index == (seasons->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        QTreeWidgetItem* moved = seasons->invisibleRootItem()->takeChild(index);
        seasons->invisibleRootItem()->insertChild(index+1, moved);
        seasons->setCurrentItem(moved);

        array.move(index, index+1);
    }
}

void
SeasonsPage::renameClicked()
{
    // which one is selected?
    if (seasons->currentItem()) seasons->editItem(seasons->currentItem(), 0);
}

void
SeasonsPage::addClicked()
{
    if (nameEdit->text() == "") return; // just ignore it

    // swap if not right way around
    if (fromEdit->date() > toEdit->date()) {
        QDate temp = fromEdit->date();
        fromEdit->setDate(toEdit->date());
        toEdit->setDate(temp);
    }

    QTreeWidgetItem *add = new QTreeWidgetItem(seasons->invisibleRootItem());
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    QString id;

    // tab name
    add->setText(0, nameEdit->text());
    // type
    add->setText(1, Season::types[typeEdit->currentIndex()]);
    // from
    add->setText(2, fromEdit->date().toString(tr("ddd MMM d, yyyy")));
    // to
    add->setText(3, toEdit->date().toString(tr("ddd MMM d, yyyy")));
    // guid -- hidden
    add->setText(4, (id=QUuid::createUuid().toString()));

    // now clear the edits
    clearEdit();

    Season addSeason;
    addSeason.setStart(fromEdit->date());
    addSeason.setEnd(toEdit->date());
    addSeason.setName(nameEdit->text());
    addSeason.setType(typeEdit->currentIndex());
    addSeason.setId(id);
    array.append(Season());
}

void
SeasonsPage::deleteClicked()
{
    if (seasons->currentItem()) {
        int index = seasons->invisibleRootItem()->indexOfChild(seasons->currentItem());

        // zap!
        delete seasons->invisibleRootItem()->takeChild(index);

        array.removeAt(index);
    }
}

qint32
SeasonsPage::saveClicked()
{
    // get any edits to the names and dates
    // since we don't trap these as they are made
    for(int i=0; i<seasons->invisibleRootItem()->childCount(); i++) {

        QTreeWidgetItem *item = seasons->invisibleRootItem()->child(i);

        array[i].setName(item->text(0));
        array[i].setType(Season::types.indexOf(item->text(1)));
        array[i].setStart(QDate::fromString(item->text(2), "ddd MMM d, yyyy"));
        array[i].setEnd(QDate::fromString(item->text(3), "ddd MMM d, yyyy"));
        array[i].setId(QUuid(item->text(4)));
    }

    // write to disk
    QString file = QString(context->athlete->home->config().canonicalPath() + "/seasons.xml");
    SeasonParser::serialize(file, array);

    // re-read
    context->athlete->seasons->readSeasons();

    return 0;
}


AutoImportPage::AutoImportPage(Context *context) : context(context)
{
    QGridLayout *mainLayout = new QGridLayout(this);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
    browseButton = new QPushButton(tr("Browse"));
#ifndef Q_OS_MAC
    upButton = new QToolButton(this);
    downButton = new QToolButton(this);
    upButton->setArrowType(Qt::UpArrow);
    downButton->setArrowType(Qt::DownArrow);
    upButton->setFixedSize(20,20);
    downButton->setFixedSize(20,20);
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
    addButton->setText(tr("Add"));
    deleteButton->setText(tr("Delete"));
    upButton = new QPushButton(tr("Up"));
    downButton = new QPushButton(tr("Down"));
#endif
    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();
    actionButtons->addWidget(browseButton);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);

    fields = new QTreeWidget;
    fields->headerItem()->setText(0, tr("Directory"));
    fields->headerItem()->setText(1, tr("Import Rule"));
    fields->setColumnWidth(0,400);
    fields->setColumnWidth(1,100);
    fields->setColumnCount(2);
    fields->setSelectionMode(QAbstractItemView::SingleSelection);
    //fields->setUniformRowHeights(true);
    fields->setIndentation(0);

    fields->setCurrentItem(fields->invisibleRootItem()->child(0));

    mainLayout->addWidget(fields, 0,0);
    mainLayout->addLayout(actionButtons, 1,0);

    context->athlete->autoImportConfig->readConfig();
    QList<RideAutoImportRule> rules = context->athlete->autoImportConfig->getConfig();
    int index = 0;
    foreach (RideAutoImportRule rule, rules) {
        QComboBox *comboButton = new QComboBox(this);
        addRuleTypes(comboButton);
        QTreeWidgetItem *add = new QTreeWidgetItem;
        fields->invisibleRootItem()->insertChild(index, add);
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        add->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
        add->setText(0, rule.getDirectory());

        add->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
        comboButton->setCurrentIndex(rule.getImportRule());
        fields->setItemWidget(add, 1, comboButton);
        index++;
    }

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(browseButton, SIGNAL(clicked()), this, SLOT(browseImportDir()));
}

void
AutoImportPage::upClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == 0) return; // its at the top already

        //movin on up!
        QWidget *button = fields->itemWidget(fields->currentItem(),1);
        QComboBox *comboButton = new QComboBox(this);
        addRuleTypes(comboButton);
        comboButton->setCurrentIndex(((QComboBox*)button)->currentIndex());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index-1, moved);
        fields->setItemWidget(moved, 1, comboButton);
        fields->setCurrentItem(moved);
    }
}

void
AutoImportPage::downClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == (fields->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        QWidget *button = fields->itemWidget(fields->currentItem(),1);
        QComboBox *comboButton = new QComboBox(this);
        addRuleTypes(comboButton);
        comboButton->setCurrentIndex(((QComboBox*)button)->currentIndex());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index+1, moved);
        fields->setItemWidget(moved, 1, comboButton);
        fields->setCurrentItem(moved);
    }
}


void
AutoImportPage::addClicked()
{

    int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
    if (index < 0) index = 0;

    QComboBox *comboButton = new QComboBox(this);
    addRuleTypes(comboButton);

    QTreeWidgetItem *add = new QTreeWidgetItem;
    fields->invisibleRootItem()->insertChild(index, add);
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    add->setTextAlignment(0, Qt::AlignLeft | Qt::AlignVCenter);
    add->setText(0, tr("Enter directory or press [Browse] to select"));
    add->setTextAlignment(1, Qt::AlignHCenter | Qt::AlignVCenter);
    fields->setItemWidget(add, 1, comboButton);

}

void
AutoImportPage::deleteClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());

        // zap!
        delete fields->invisibleRootItem()->takeChild(index);
    }
}

qint32
AutoImportPage::saveClicked()
{

    rules.clear();
    for(int i=0; i<fields->invisibleRootItem()->childCount(); i++) {

        RideAutoImportRule rule;
        rule.setDirectory(fields->invisibleRootItem()->child(i)->text(0));

        QWidget *button = fields->itemWidget(fields->invisibleRootItem()->child(i),1);
        rule.setImportRule(((QComboBox*)button)->currentIndex());
        rules.append(rule);

    }

    // write to disk
    QString file = QString(context->athlete->home->config().canonicalPath() + "/autoimport.xml");
    RideAutoImportConfigParser::serialize(file, rules);

    // re-read
    context->athlete->autoImportConfig->readConfig();
    return 0;
}

void
AutoImportPage::addRuleTypes(QComboBox *p) {

    RideAutoImportRule* config = new RideAutoImportRule();
    QList<QString> descriptions = config->getRuleDescriptions();

    foreach(QString description, descriptions) {
           p->addItem(description);
    }
}

void
AutoImportPage::browseImportDir()
{
    QStringList selectedDirs;
    if (fields->currentItem()) {
        QFileDialog fileDialog(this);
        fileDialog.setFileMode(QFileDialog::Directory);
        fileDialog.setOptions(QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if (fileDialog.exec()) {
            selectedDirs = fileDialog.selectedFiles();
        }
        if (selectedDirs.count() > 0) {
            QString dir = selectedDirs.at(0);
            if (dir != "") {
                fields->currentItem()->setText(0, dir);
            }
        }
    }
}

IntervalsPage::IntervalsPage(Context *context) : context(context)
{
    // get config
    b4.discovery = appsettings->cvalue(context->athlete->cyclist, GC_DISCOVERY, 57).toInt(); // 57 does not include search for PEAKs

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGridLayout *layout = new QGridLayout();
    mainLayout->addLayout(layout);
    mainLayout->addStretch();

    QLabel *heading = new QLabel(tr("Enable interval auto-discovery"));
    heading->setFixedHeight(QFontMetrics(heading->font()).height() + 4);

    int row = 0;
    layout->addWidget(heading, row, 0, Qt::AlignRight);

    user = 99;
    for(int i=0; i<= static_cast<int>(RideFileInterval::last()); i++) {

        // ignore until we get past user interval type
        if (i == static_cast<int>(RideFileInterval::USER)) user=i;

        // if past user then add
        if (i>user) {
            QCheckBox *add = new QCheckBox(RideFileInterval::typeDescriptionLong(static_cast<RideFileInterval::IntervalType>(i)));
            checkBoxes << add;
            add->setChecked(b4.discovery & RideFileInterval::intervalTypeBits(static_cast<RideFileInterval::IntervalType>(i)));
            layout->addWidget(add, row++, 1, Qt::AlignLeft);
        }
    }
}

qint32
IntervalsPage::saveClicked()
{
    int discovery = 0;

    // now update discovery !
    for(int i=0; i< checkBoxes.count(); i++)
        if (checkBoxes[i]->isChecked())
            discovery += RideFileInterval::intervalTypeBits(static_cast<RideFileInterval::IntervalType>(i+user+1));

    // new value returned
    appsettings->setCValue(context->athlete->cyclist, GC_DISCOVERY, discovery);

    // return change !
    if (b4.discovery != discovery) return CONFIG_DISCOVERY;
    else return 0;
}
