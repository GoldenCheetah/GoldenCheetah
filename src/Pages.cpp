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
    langLabel = new QLabel(tr("Language:"));
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
    else langCombo->setCurrentIndex(0);

    configLayout->addWidget(langLabel, 0,0, Qt::AlignRight);
    configLayout->addWidget(langCombo, 0,1, Qt::AlignLeft);

    //
    // Crank length - only used by PfPv chart (should move there!)
    //
    QLabel *crankLengthLabel = new QLabel(tr("Crank Length:"));
    QVariant crankLength = appsettings->value(this, GC_CRANKLENGTH);
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

    configLayout->addWidget(crankLengthLabel, 1,0, Qt::AlignRight);
    configLayout->addWidget(crankLengthCombo, 1,1, Qt::AlignLeft);

    //
    // Wheel size
    //
    QLabel *wheelSizeLabel = new QLabel(tr("Wheelsize:"), this);
    int wheelSize = appsettings->value(this, GC_WHEELSIZE, 2100).toInt();

    wheelSizeCombo = new QComboBox();
    wheelSizeCombo->addItem(tr("Road/Cross (700C/622)")); // 2100mm
    wheelSizeCombo->addItem(tr("Tri/TT (650C)")); // 1960mm
    wheelSizeCombo->addItem(tr("Mountain (26\")")); // 1985mm
    wheelSizeCombo->addItem(tr("BMX (20\")")); // 1750mm

    switch (wheelSize) {
    default:
    case 2100 : wheelSizeCombo->setCurrentIndex(0); break;
    case 1960 : wheelSizeCombo->setCurrentIndex(1); break;
    case 1985 : wheelSizeCombo->setCurrentIndex(2); break;
    case 1750 : wheelSizeCombo->setCurrentIndex(3); break;
    }

    configLayout->addWidget(wheelSizeLabel, 2,0, Qt::AlignRight);
    configLayout->addWidget(wheelSizeCombo, 2,1, Qt::AlignLeft);

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
    QLabel *garminHWLabel = new QLabel(tr("Smart Recoding Threshold (secs):"));
    garminHWMarkedit = new QLineEdit(garminHWMark.toString(),this);
    garminHWMarkedit->setInputMask("009");

    configLayout->addWidget(garminSmartRecord, 3,1, Qt::AlignLeft);
    configLayout->addWidget(garminHWLabel, 4,0, Qt::AlignRight);
    configLayout->addWidget(garminHWMarkedit, 4,1, Qt::AlignLeft);

    // Elevation hysterisis  GC_ELEVATION_HYSTERISIS
    QVariant elevationHysteresis = appsettings->value(this, GC_ELEVATION_HYSTERESIS);
    if (elevationHysteresis.isNull() || elevationHysteresis.toFloat() == 0.0)
       elevationHysteresis.setValue(3.0);  // default is 1 meter

    QLabel *hystlabel = new QLabel(tr("Elevation hysteresis (meters):"));
    hystedit = new QLineEdit(elevationHysteresis.toString(),this);
    hystedit->setInputMask("9.00");
    
    configLayout->addWidget(hystlabel, 7,0, Qt::AlignRight);
    configLayout->addWidget(hystedit, 7,1, Qt::AlignLeft);

    // wbal formula preference
    QLabel *wbalFormLabel = new QLabel(tr("W' bal formula:"));
    wbalForm = new QComboBox(this);
    wbalForm->addItem(tr("Differential"));
    wbalForm->addItem(tr("Integral"));
    if (appsettings->value(this, GC_WBALFORM, "diff").toString() == "diff") wbalForm->setCurrentIndex(0);
    else wbalForm->setCurrentIndex(1);

    configLayout->addWidget(wbalFormLabel, 8,0, Qt::AlignRight);
    configLayout->addWidget(wbalForm, 8,1, Qt::AlignLeft);

    //
    // Performance manager
    //

    perfManSTSLabel = new QLabel(tr("STS average (days):"));
    perfManLTSLabel = new QLabel(tr("LTS average (days):"));
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

    configLayout->addWidget(perfManSTSLabel, 9,0, Qt::AlignRight);
    configLayout->addWidget(perfManSTSavg, 9,1, Qt::AlignLeft);
    configLayout->addWidget(perfManLTSLabel, 10,0, Qt::AlignRight);
    configLayout->addWidget(perfManLTSavg, 10,1, Qt::AlignLeft);
    configLayout->addWidget(showSBToday, 11,1, Qt::AlignLeft);

    //
    // Athlete directory (home of athletes)
    //
    QVariant athleteDir = appsettings->value(this, GC_HOMEDIR);
    athleteLabel = new QLabel(tr("Athlete Library:"));
    athleteDirectory = new QLineEdit;
    athleteDirectory->setText(athleteDir.toString() == "0" ? "" : athleteDir.toString());
    athleteWAS = athleteDirectory->text(); // remember what we started with ...
    athleteBrowseButton = new QPushButton(tr("Browse"));
    athleteBrowseButton->setFixedWidth(120);

    configLayout->addWidget(athleteLabel, 12,0, Qt::AlignRight);
    configLayout->addWidget(athleteDirectory, 12,1);
    configLayout->addWidget(athleteBrowseButton, 12,2);

    connect(athleteBrowseButton, SIGNAL(clicked()), this, SLOT(browseAthleteDir()));

    //
    // Workout directory (train view)
    //
    QVariant workoutDir = appsettings->value(this, GC_WORKOUTDIR, "");
    // fix old bug..
    if (workoutDir == "0") workoutDir = "";
    workoutLabel = new QLabel(tr("Workout Library:"));
    workoutDirectory = new QLineEdit;
    workoutDirectory->setText(workoutDir.toString());
    workoutBrowseButton = new QPushButton(tr("Browse"));
    workoutBrowseButton->setFixedWidth(120);

    configLayout->addWidget(workoutLabel, 14,0, Qt::AlignRight);
    configLayout->addWidget(workoutDirectory, 14,1);
    configLayout->addWidget(workoutBrowseButton, 14,2);

    connect(workoutBrowseButton, SIGNAL(clicked()), this, SLOT(browseWorkoutDir()));

}

void
GeneralPage::saveClicked()
{
    // Language
    static const QString langs[] = {
        "en", "fr", "ja", "pt-br", "it", "de", "ru", "cs", "es", "pt"
    };
    appsettings->setValue(GC_LANG, langs[langCombo->currentIndex()]);

    // Garmon and cranks
    appsettings->setValue(GC_GARMIN_HWMARK, garminHWMarkedit->text().toInt());
    appsettings->setValue(GC_GARMIN_SMARTRECORD, garminSmartRecord->checkState());
    appsettings->setValue(GC_CRANKLENGTH, crankLengthCombo->currentText());

    // save wheel size
    static const int wheelSizes[] = {
        2100, 1960, 1985, 1750
    };

    appsettings->setValue(GC_WHEELSIZE, wheelSizes[wheelSizeCombo->currentIndex()]);

    // Bike score estimation
    appsettings->setValue(GC_WORKOUTDIR, workoutDirectory->text());
    appsettings->setValue(GC_HOMEDIR, athleteDirectory->text());
    appsettings->setValue(GC_ELEVATION_HYSTERESIS, hystedit->text());

    // wbal formula
    appsettings->setValue(GC_WBALFORM, wbalForm->currentIndex() ? "int" : "diff");

    // Performance Manager
    appsettings->setCValue(context->athlete->cyclist, GC_STS_DAYS, perfManSTSavg->text());
    appsettings->setCValue(context->athlete->cyclist, GC_LTS_DAYS, perfManLTSavg->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SB_TODAY, (int) showSBToday->isChecked());

    // set default stress names if not set:
    appsettings->setValue(GC_STS_NAME, appsettings->value(this, GC_STS_NAME,tr("Short Term Stress")));
    appsettings->setValue(GC_STS_ACRONYM, appsettings->value(this, GC_STS_ACRONYM,tr("STS")));
    appsettings->setValue(GC_LTS_NAME, appsettings->value(this, GC_LTS_NAME,tr("Long Term Stress")));
    appsettings->setValue(GC_LTS_ACRONYM, appsettings->value(this, GC_LTS_ACRONYM,tr("LTS")));
    appsettings->setValue(GC_SB_NAME, appsettings->value(this, GC_SB_NAME,tr("Stress Balance")));
    appsettings->setValue(GC_SB_ACRONYM, appsettings->value(this, GC_SB_ACRONYM,tr("SB")));
}

void
GeneralPage::browseWorkoutDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Workout Library"),
                            "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir != "") workoutDirectory->setText(dir);  //only overwrite current dir, if a new was selected
}

void
GeneralPage::browseAthleteDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Athlete Library"),
                            "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
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

    //////////////////////////////////////////////////
    // TrainingPeaks

    QLabel *tp = new QLabel(tr("TrainingPeaks"));
    tp->setFont(current);

    QLabel *urlLabel = new QLabel(tr("Website"));
    QLabel *userLabel = new QLabel(tr("Username"));
    QLabel *passLabel = new QLabel(tr("Password"));
    QLabel *typeLabel = new QLabel(tr("Account Type"));

    tpURL = new QLineEdit(this);
    tpURL->setText(appsettings->cvalue(context->athlete->cyclist, GC_TPURL, "http://www.trainingpeaks.com").toString());

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

    grid->addWidget(urlLabel, ++row, 0);
    grid->addWidget(tpURL, row, 1, 0);

    grid->addWidget(userLabel, ++row, 0);
    grid->addWidget(tpUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(passLabel, ++row, 0);
    grid->addWidget(tpPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(typeLabel, ++row, 0);
    grid->addWidget(tpType, row, 1, Qt::AlignLeft | Qt::AlignVCenter);


    //////////////////////////////////////////////////
    // Golden Cheetah Racing

    QLabel *gc = new QLabel(tr("Golden Cheetah Racing"));
    gc->setFont(current);

    QLabel *gcurlLabel = new QLabel(tr("Website"));
    QLabel *gcuserLabel = new QLabel(tr("Username"));
    QLabel *gcpassLabel = new QLabel(tr("Password"));

    gcURL = new QLineEdit(this);
    gcURL->setText(appsettings->cvalue(context->athlete->cyclist, GC_GCURL, "http://race.goldencheetah.org").toString());

    gcUser = new QLineEdit(this);
    gcUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_GCUSER, "").toString());

    gcPass = new QLineEdit(this);
    gcPass->setEchoMode(QLineEdit::Password);
    gcPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_GCPASS, "").toString());

    grid->addWidget(gc, ++row, 0);

    grid->addWidget(gcurlLabel, ++row, 0);
    grid->addWidget(gcURL, row, 1, 0);

    grid->addWidget(gcuserLabel, ++row, 0);
    grid->addWidget(gcUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(gcpassLabel, ++row, 0);
    grid->addWidget(gcPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    //////////////////////////////////////////////////
    // Twitter

#ifdef GC_HAVE_LIBOAUTH
    QLabel *twp = new QLabel(tr("Twitter"));
    twp->setFont(current);

    QLabel *twurlLabel = new QLabel(tr("Website"));
    QLabel *twauthLabel = new QLabel(tr("Authorise"));


    twitterURL = new QLineEdit(this);
    twitterURL->setText(appsettings->cvalue(context->athlete->cyclist, GC_TWURL, "http://www.twitter.com").toString());
    twitterAuthorise = new QPushButton(tr("Authorise"), this);
    QPixmap passwords = QPixmap(":/images/toolbar/passwords.png");

    twitterAuthorised = new QPushButton(this);
    twitterAuthorised->setContentsMargins(0,0,0,0);
    twitterAuthorised->setIcon(passwords.scaled(16,16));
    twitterAuthorised->setIconSize(QSize(16,16));
    twitterAuthorised->setFixedHeight(16);
    twitterAuthorised->setFixedWidth(16);

    grid->addWidget(twp, ++row, 0);

    grid->addWidget(twurlLabel, ++row, 0);
    grid->addWidget(twitterURL, row, 1, 0);

    grid->addWidget(twauthLabel, ++row, 0);
    grid->addWidget(twitterAuthorise, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
    if (appsettings->cvalue(context->athlete->cyclist, GC_TWITTER_TOKEN, "")!="")
        grid->addWidget(twitterAuthorised, row, 1, Qt::AlignLeft | Qt::AlignVCenter);
    else
        twitterAuthorised->hide(); // if no token no show

    connect(twitterAuthorise, SIGNAL(clicked()), this, SLOT(authoriseTwitter()));
#endif

    //grid->addWidget(twpinLabel, ++row, 0);

    //////////////////////////////////////////////////
    // Strava

#ifdef GC_HAVE_LIBOAUTH
    QLabel *str = new QLabel(tr("Strava"));
    str->setFont(current);

    //QLabel *struserLabel = new QLabel(tr("Username"));
    //QLabel *strpassLabel = new QLabel(tr("Password"));
    QLabel *strauthLabel = new QLabel(tr("Authorise"));

    stravaAuthorise = new QPushButton(tr("Authorise"), this);

#ifndef GC_STRAVA_CLIENT_SECRET
    stravaAuthorise->setEnabled(false);
#endif

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
#endif

    //////////////////////////////////////////////////
    // Cycling Analytics

#ifdef GC_HAVE_LIBOAUTH
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
#endif

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
    // Zeo Sleep Data

    QLabel *zeo = new QLabel(tr("Zeo Sleep Data"));
    zeo->setFont(current);

    QLabel *zeourlLabel = new QLabel(tr("Website"));
    QLabel *zeouserLabel = new QLabel(tr("User"));
    QLabel *zeopassLabel = new QLabel(tr("Password"));

    zeoURL = new QLineEdit(this);
    zeoURL->setText(appsettings->cvalue(context->athlete->cyclist, GC_ZEOURL, "http://app-pro.myzeo.com:8080/").toString());

    zeoUser = new QLineEdit(this);
    zeoUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_ZEOUSER, "").toString());

    zeoPass = new QLineEdit(this);
    zeoPass->setEchoMode(QLineEdit::Password);
    zeoPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_ZEOPASS, "").toString());

    grid->addWidget(zeo, ++row, 0);

    grid->addWidget(zeourlLabel, ++row, 0);
    grid->addWidget(zeoURL, row, 1, 0);

    grid->addWidget(zeouserLabel, ++row, 0);
    grid->addWidget(zeoUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(zeopassLabel, ++row, 0);
    grid->addWidget(zeoPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

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
    dv->setFont(current);
    QLabel *dvurlLabel = new QLabel(tr("CalDAV URL"));
    QLabel *dvuserLabel = new QLabel(tr("CalDAV User Id"));
    QLabel *dvpassLabel = new QLabel(tr("CalDAV Password"));

    dvURL = new QLineEdit(this);
    QString url = appsettings->cvalue(context->athlete->cyclist, GC_DVURL, "").toString();
    url.replace("%40", "@"); // remove escape of @ character
    dvURL->setText(url);

    dvUser = new QLineEdit(this);
    dvUser->setText(appsettings->cvalue(context->athlete->cyclist, GC_DVUSER, "").toString());

    dvPass = new QLineEdit(this);
    dvPass->setEchoMode(QLineEdit::Password);
    dvPass->setText(appsettings->cvalue(context->athlete->cyclist, GC_DVPASS, "").toString());

    grid->addWidget(dv, ++row, 0);

    grid->addWidget(dvurlLabel, ++row, 0);
    grid->addWidget(dvURL, row, 1, 0);

    grid->addWidget(dvuserLabel, ++row, 0);
    grid->addWidget(dvUser, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(dvpassLabel, ++row, 0);
    grid->addWidget(dvPass, row, 1, Qt::AlignLeft | Qt::AlignVCenter);

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
    // End of grid

    grid->setColumnStretch(0,0);
    grid->setColumnStretch(1,3);

    main->setLayout(grid);

    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
    setWidget(main);

}


#ifdef GC_HAVE_LIBOAUTH
void CredentialsPage::authoriseTwitter()
{
    OAuthDialog *oauthDialog = new OAuthDialog(context, OAuthDialog::TWITTER);
    oauthDialog->setWindowModality(Qt::ApplicationModal);
    oauthDialog->exec();
    /*
#ifdef GC_HAVE_LIBOAUTH
    int rc;
    char **rv = NULL;
    QString token;
    QString url = QString();
    t_key = NULL;
    t_secret = NULL;

    const char *request_token_uri = "https://api.twitter.com/oauth/request_token";

    char *req_url = NULL;
    char *postarg = NULL;
    char *reply   = NULL;
    req_url = oauth_sign_url2(request_token_uri, NULL, OA_HMAC, NULL, GC_TWITTER_CONSUMER_KEY, GC_TWITTER_CONSUMER_SECRET, NULL, NULL);
    reply = oauth_http_get(req_url,postarg);

    rc = oauth_split_url_parameters(reply, &rv);
    qsort(rv, rc, sizeof(char *), oauth_cmpstringp);
    token = QString(rv[1]);
    t_key  =strdup(&(rv[1][12]));
    t_secret =strdup(&(rv[2][19]));
    url = QString("https://api.twitter.com/oauth/authorize?");
    url.append(token);
    QDesktopServices::openUrl(QUrl(url));
    if(rv) free(rv);
#endif
    */
}
#endif

#ifdef GC_HAVE_LIBOAUTH
void CredentialsPage::authoriseStrava()
{
    OAuthDialog *oauthDialog = new OAuthDialog(context, OAuthDialog::STRAVA);
    oauthDialog->setWindowModality(Qt::ApplicationModal);
    oauthDialog->exec();
}
#endif

#ifdef GC_HAVE_LIBOAUTH
void CredentialsPage::authoriseCyclingAnalytics()
{
    OAuthDialog *oauthDialog = new OAuthDialog(context, OAuthDialog::CYCLING_ANALYTICS);
    oauthDialog->setWindowModality(Qt::ApplicationModal);
    oauthDialog->exec();
}
#endif

void
CredentialsPage::saveClicked()
{
    appsettings->setCValue(context->athlete->cyclist, GC_GCURL, gcURL->text());
    appsettings->setCValue(context->athlete->cyclist, GC_GCUSER, gcUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_GCPASS, gcPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_TPURL, tpURL->text());
    appsettings->setCValue(context->athlete->cyclist, GC_TPUSER, tpUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_TPPASS, tpPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_RWGPSUSER, rideWithGPSUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_RWGPSPASS, rideWithGPSPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_TTBUSER, ttbUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_TTBPASS, ttbPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_VELOHEROUSER, veloHeroUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_VELOHEROPASS, veloHeroPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SELUSER, selUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_SELPASS, selPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_TPTYPE, tpType->currentIndex());
    appsettings->setCValue(context->athlete->cyclist, GC_WIURL, wiURL->text());
    appsettings->setCValue(context->athlete->cyclist, GC_WIUSER, wiUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_WIKEY, wiPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_ZEOURL, zeoURL->text());
    appsettings->setCValue(context->athlete->cyclist, GC_ZEOUSER, zeoUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_ZEOPASS, zeoPass->text());
    appsettings->setCValue(context->athlete->cyclist, GC_WEBCAL_URL, webcalURL->text());

    // escape the at character
    QString url = dvURL->text();
    url.replace("@", "%40");
    appsettings->setCValue(context->athlete->cyclist, GC_DVURL, url);
    appsettings->setCValue(context->athlete->cyclist, GC_DVUSER, dvUser->text());
    appsettings->setCValue(context->athlete->cyclist, GC_DVPASS, dvPass->text());
}

//
// About me
//
RiderPage::RiderPage(QWidget *parent, Context *context) : QWidget(parent), context(context)
{
    QVBoxLayout *all = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout;

    QLabel *nicklabel = new QLabel(tr("Nickname"));
    QLabel *doblabel = new QLabel(tr("Date of Birth"));
    QLabel *sexlabel = new QLabel(tr("Sex"));
    QLabel *unitlabel = new QLabel(tr("Unit"));
    QLabel *biolabel = new QLabel(tr("Bio"));

    QString weighttext = QString(tr("Weight (%1)")).arg(context->athlete->useMetricUnits ? tr("kg") : tr("lb"));
    weightlabel = new QLabel(weighttext);

    QString heighttext = QString(tr("Height (%1)")).arg(context->athlete->useMetricUnits ? tr("cm") : tr("in"));
    heightlabel = new QLabel(heighttext);

    wbaltaulabel = new QLabel(tr("W'bal tau (s)"));

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

    unitCombo = new QComboBox();
    unitCombo->addItem(tr("Metric"));
    unitCombo->addItem(tr("Imperial"));

    if (context->athlete->useMetricUnits)
        unitCombo->setCurrentIndex(0);
    else
        unitCombo->setCurrentIndex(1);

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

    bio = new QTextEdit(this);
    bio->setText(appsettings->cvalue(context->athlete->cyclist, GC_BIO).toString());

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

    Qt::Alignment alignment = Qt::AlignLeft|Qt::AlignVCenter;

    grid->addWidget(nicklabel, 0, 0, alignment);
    grid->addWidget(doblabel, 1, 0, alignment);
    grid->addWidget(sexlabel, 2, 0, alignment);
    grid->addWidget(unitlabel, 3, 0, alignment);
    grid->addWidget(weightlabel, 4, 0, alignment);
    grid->addWidget(heightlabel, 4, 2, alignment);
    grid->addWidget(wbaltaulabel, 5, 0, alignment);
    grid->addWidget(biolabel, 6, 0, alignment);

    grid->addWidget(nickname, 0, 1, alignment);
    grid->addWidget(dob, 1, 1, alignment);
    grid->addWidget(sex, 2, 1, alignment);
    grid->addWidget(unitCombo, 3, 1, alignment);
    grid->addWidget(weight, 4, 1, alignment);
    grid->addWidget(height, 4, 3, alignment);
    grid->addWidget(wbaltau, 5, 1, alignment);
    grid->addWidget(bio, 7, 0, 1, 4);

    grid->addWidget(avatarButton, 0, 2, 4, 2, Qt::AlignRight|Qt::AlignVCenter);

    all->addLayout(grid);
    all->addStretch();

    connect (avatarButton, SIGNAL(clicked()), this, SLOT(chooseAvatar()));
    connect (unitCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(unitChanged(int)));
}

void
RiderPage::chooseAvatar()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Choose Picture"),
                            "", tr("Images (*.png *.jpg *.bmp)"));
    if (filename != "") {

        avatar = QPixmap(filename);
        avatarButton->setIcon(avatar.scaled(140,140));
        avatarButton->setIconSize(QSize(140,140));
    }
}

void
RiderPage::unitChanged(int currentIndex)
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
RiderPage::saveClicked()
{
    appsettings->setCValue(context->athlete->cyclist, GC_NICKNAME, nickname->text());
    appsettings->setCValue(context->athlete->cyclist, GC_DOB, dob->date());
    appsettings->setCValue(context->athlete->cyclist, GC_WEIGHT, weight->value() * (unitCombo->currentIndex() ? KG_PER_LB : 1.0));
    appsettings->setCValue(context->athlete->cyclist, GC_HEIGHT, height->value() * (unitCombo->currentIndex() ? CM_PER_INCH/100.0 : 1.0/100.0));
    appsettings->setCValue(context->athlete->cyclist, GC_WBALTAU, wbaltau->value());

    if (unitCombo->currentIndex()==0)
        appsettings->setCValue(context->athlete->cyclist, GC_UNIT, GC_UNIT_METRIC);
    else if (unitCombo->currentIndex()==1)
        appsettings->setCValue(context->athlete->cyclist, GC_UNIT, GC_UNIT_IMPERIAL);

    appsettings->setCValue(context->athlete->cyclist, GC_SEX, sex->currentIndex());
    appsettings->setCValue(context->athlete->cyclist, GC_BIO, bio->toPlainText());
    avatar.save(context->athlete->home->config().canonicalPath() + "/" + "avatar.png", "PNG");

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

    multiCheck = new QCheckBox(tr("Allow multiple devices in Train View"), this);
    multiCheck->setChecked(appsettings->value(this, TRAIN_MULTI, false).toBool());

    mainLayout->addWidget(deviceList);
    QHBoxLayout *bottom = new QHBoxLayout;
    bottom->setSpacing(2);
    bottom->addWidget(multiCheck);
    bottom->addStretch();
    bottom->addWidget(addButton);
    bottom->addWidget(delButton);
    mainLayout->addLayout(bottom);

    connect(addButton, SIGNAL(clicked()), this, SLOT(devaddClicked()));
    connect(delButton, SIGNAL(clicked()), this, SLOT(devdelClicked()));
}

void
DevicePage::saveClicked()
{
    // Save the device configuration...
    DeviceConfigurations all;
    all.writeConfig(deviceListModel->Configuration);
    appsettings->setValue(TRAIN_MULTI, multiCheck->isChecked());
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
    QString chrome = appsettings->value(this, GC_CHROME, "Mac").toString();
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
    QLabel *rideScrollLabel = new QLabel(tr("Ride Scrollbar"));
    rideScroll = new QCheckBox;
    rideScroll->setChecked(appsettings->value(this, GC_RIDESCROLL, true).toBool());
    QLabel *rideHeadLabel = new QLabel(tr("Ride Headings"));
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

void
ColorsPage::saveClicked()
{
    // chrome style only has 2 types for now
    switch(chromeCombo->currentIndex()) {
    default:
    case 0:
        appsettings->setValue(GC_CHROME, "Mac");
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
}

IntervalMetricsPage::IntervalMetricsPage(QWidget *parent) :
    QWidget(parent), changed(false)
{
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
        QTextEdit name(m->name()); // process html encoding of(TM)
        QListWidgetItem *item = new QListWidgetItem(name.toPlainText());
        item->setData(Qt::UserRole, symbol);
        availList->addItem(item);
    }
    foreach (QString symbol, selectedMetrics) {
        if (!factory.haveMetric(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QTextEdit name(m->name());  // process html encoding of(TM)
        QListWidgetItem *item = new QListWidgetItem(name.toPlainText());
        item->setData(Qt::UserRole, symbol);
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

void
IntervalMetricsPage::saveClicked()
{
    if (!changed)
        return;
    QStringList metrics;
    for (int i = 0; i < selectedList->count(); ++i)
        metrics << selectedList->item(i)->data(Qt::UserRole).toString();
    appsettings->setValue(GC_SETTINGS_INTERVAL_METRICS, metrics.join(","));
}

BestsMetricsPage::BestsMetricsPage(QWidget *parent) :
    QWidget(parent), changed(false)
{
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
        QTextEdit name(m->name()); //  process html encoding of(TM)
        QListWidgetItem *item = new QListWidgetItem(name.toPlainText());
        item->setData(Qt::UserRole, symbol);
        availList->addItem(item);
    }
    foreach (QString symbol, selectedMetrics) {
        if (!factory.haveMetric(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QTextEdit name(m->name()); //  process html encoding of(TM)
        QListWidgetItem *item = new QListWidgetItem(name.toPlainText());
        item->setData(Qt::UserRole, symbol);
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

void
BestsMetricsPage::saveClicked()
{
    if (!changed)
        return;
    QStringList metrics;
    for (int i = 0; i < selectedList->count(); ++i)
        metrics << selectedList->item(i)->data(Qt::UserRole).toString();
    appsettings->setValue(GC_SETTINGS_BESTS_METRICS, metrics.join(","));
}

SummaryMetricsPage::SummaryMetricsPage(QWidget *parent) :
    QWidget(parent), changed(false)
{
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
        QTextEdit name(m->name()); //  process html encoding of(TM)
        QListWidgetItem *item = new QListWidgetItem(name.toPlainText());
        item->setData(Qt::UserRole, symbol);
        availList->addItem(item);
    }
    foreach (QString symbol, selectedMetrics) {
        if (!factory.haveMetric(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QTextEdit name(m->name()); //  process html encoding of(TM)
        QListWidgetItem *item = new QListWidgetItem(name.toPlainText());
        item->setData(Qt::UserRole, symbol);
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

void
SummaryMetricsPage::saveClicked()
{
    if (!changed)
        return;
    QStringList metrics;
    for (int i = 0; i < selectedList->count(); ++i)
        metrics << selectedList->item(i)->data(Qt::UserRole).toString();
    appsettings->setValue(GC_SETTINGS_SUMMARY_METRICS, metrics.join(","));
}

MetadataPage::MetadataPage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // get current config using default file
    keywordDefinitions = context->athlete->rideMetadata()->getKeywords();
    fieldDefinitions = context->athlete->rideMetadata()->getFields();
    colorfield = context->athlete->rideMetadata()->getColorField();

    // setup maintenance pages using current config
    fieldsPage = new FieldsPage(this, fieldDefinitions);
    keywordsPage = new KeywordsPage(this, keywordDefinitions);
    processorPage = new ProcessorPage(context);

    tabs = new QTabWidget(this);
    tabs->addTab(fieldsPage, tr("Fields"));
    tabs->addTab(keywordsPage, tr("Notes Keywords"));
    tabs->addTab(processorPage, tr("Processing"));

    // refresh the keywords combo when change tabs .. will do more often than
    // needed but better that than less often than needed
    connect (tabs, SIGNAL(currentChanged(int)), keywordsPage, SLOT(pageSelected()));

    layout->addWidget(tabs);
}

void
MetadataPage::saveClicked()
{
    // get current state
    fieldsPage->getDefinitions(fieldDefinitions);
    keywordsPage->getDefinitions(keywordDefinitions);

    // save settings
    appsettings->setValue(GC_RIDEBG, keywordsPage->rideBG->isChecked());

    // write to metadata.xml
    RideMetadata::serialize(context->athlete->home->config().canonicalPath() + "/metadata.xml", keywordDefinitions, fieldDefinitions, colorfield);

    // save processors config
    processorPage->saveClicked();
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

    // get the current fields defiitions 
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
    processorTree->setColumnCount(3);
    processorTree->setSelectionMode(QAbstractItemView::NoSelection);
    processorTree->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    processorTree->setUniformRowHeights(true);
    processorTree->setIndentation(0);
    //processorTree->header()->resizeSection(0,150);

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

        // Auto or Manual run?
        QComboBox *comboButton = new QComboBox(this);
        comboButton->addItem(tr("Manual"));
        comboButton->addItem(tr("Auto"));
        processorTree->setItemWidget(add, 1, comboButton);

        QString configsetting = QString("dp/%1/apply").arg(i.key());
        if (appsettings->value(this, configsetting, "Manual").toString() == "Manual")
            comboButton->setCurrentIndex(0);
        else
            comboButton->setCurrentIndex(1);

        // Get and Set the Config Widget
        DataProcessorConfig *config = i.value()->processorConfig(this);
        config->readConfig();

        processorTree->setItemWidget(add, 2, config);
    }

    mainLayout->addWidget(processorTree, 0,0);
}

void
ProcessorPage::saveClicked()
{
    // call each processor config widget's saveConfig() to
    // write away separately
    for (int i=0; i<processorTree->invisibleRootItem()->childCount(); i++) {
        // auto or manual?
        QString configsetting = QString("dp/%1/apply").arg(processorTree->invisibleRootItem()->child(i)->text(0));
        QString apply = ((QComboBox*)(processorTree->itemWidget(processorTree->invisibleRootItem()->child(i), 1)))->currentIndex() ?
                        "Auto" : "Manual";
        appsettings->setValue(configsetting, apply);
        ((DataProcessorConfig*)(processorTree->itemWidget(processorTree->invisibleRootItem()->child(i), 2)))->saveConfig();
    }
}

//
// Power Zone Config page
//
ZonePage::ZonePage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // get current config by reading it in (leave mainwindow zones alone)
    QFile zonesFile(context->athlete->home->config().canonicalPath() + "/power.zones");
    if (zonesFile.exists()) {
        zones.read(zonesFile);
        zonesFile.close();
    }

    // setup maintenance pages using current config
    schemePage = new SchemePage(this);
    cpPage = new CPPage(this);

    tabs = new QTabWidget(this);
    tabs->addTab(cpPage, tr("Critical Power"));
    tabs->addTab(schemePage, tr("Default"));

    layout->addWidget(tabs);
}

void
ZonePage::saveClicked()
{
    zones.setScheme(schemePage->getScheme());
    zones.write(context->athlete->home->config());
}

SchemePage::SchemePage(ZonePage* zonePage) : zonePage(zonePage)
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
    for (int i=0; i< zonePage->zones.getScheme().nzones_default; i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, zonePage->zones.getScheme().zone_default_name[i]);
        // field name
        add->setText(1, zonePage->zones.getScheme().zone_default_desc[i]);

        // low
        QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
        loedit->setMinimum(0);
        loedit->setMaximum(1000);
        loedit->setSingleStep(1.0);
        loedit->setDecimals(0);
        loedit->setValue(zonePage->zones.getScheme().zone_default[i]);
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


CPPage::CPPage(ZonePage* zonePage) : zonePage(zonePage)
{
    active = false;

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setSpacing(10);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
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
    QLabel *cpLabel = new QLabel(tr("Critical Power"));
    QLabel *wLabel = new QLabel(tr("W'"));
    dateEdit = new QDateEdit;
    dateEdit->setDate(QDate::currentDate());

    cpEdit = new QDoubleSpinBox;
    cpEdit->setMinimum(0);
    cpEdit->setMaximum(1000);
    cpEdit->setSingleStep(1.0);
    cpEdit->setDecimals(0);

    wEdit = new QDoubleSpinBox;
    wEdit->setMinimum(0);
    wEdit->setMaximum(100000);
    wEdit->setSingleStep(100);
    wEdit->setDecimals(0);

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addWidget(cpLabel);
    actionButtons->addWidget(cpEdit);
    actionButtons->addWidget(wLabel);
    actionButtons->addWidget(wEdit);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(defaultButton);

    addLayout->addWidget(dateLabel);
    addLayout->addWidget(dateEdit);
    addLayout->addStretch();

    ranges = new QTreeWidget;
    ranges->headerItem()->setText(0, tr("From Date"));
    ranges->headerItem()->setText(1, tr("Critical Power"));
    ranges->headerItem()->setText(2, tr("W'"));
    ranges->setColumnCount(3);
    ranges->setSelectionMode(QAbstractItemView::SingleSelection);
    //ranges->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    ranges->setUniformRowHeights(true);
    ranges->setIndentation(0);
    //ranges->header()->resizeSection(0,180);

    // setup list of ranges
    for (int i=0; i< zonePage->zones.getRangeSize(); i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(zonePage->zones.getZoneRange(i).zonesSetFromCP ?
                                        QFont::Normal : QFont::Black);

        // date
        add->setText(0, zonePage->zones.getStartDate(i).toString(tr("MMM d, yyyy")));
        add->setFont(0, font);

        // CP
        add->setText(1, QString("%1").arg(zonePage->zones.getCP(i)));
        add->setFont(1, font);

        // W'
        add->setText(2, QString("%1").arg(zonePage->zones.getWprime(i)));
        add->setFont(2, font);

    }

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

    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(defaultButton, SIGNAL(clicked()), this, SLOT(defaultClicked()));
    connect(addZoneButton, SIGNAL(clicked()), this, SLOT(addZoneClicked()));
    connect(deleteZoneButton, SIGNAL(clicked()), this, SLOT(deleteZoneClicked()));
    connect(ranges, SIGNAL(itemSelectionChanged()), this, SLOT(rangeSelectionChanged()));
    connect(zones, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(zonesChanged()));
}

void
CPPage::addClicked()
{
    // get current scheme
    zonePage->zones.setScheme(zonePage->schemePage->getScheme());

    int cp = cpEdit->value();
    if( cp <= 0 ){
        QMessageBox err;
        err.setText(tr("CP must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    //int index = ranges->invisibleRootItem()->childCount();
    int wp = wEdit->value() ? wEdit->value() : 20000;
    if (wp < 1000) wp *= 1000; // entered in kJ we want joules
    int index = zonePage->zones.addZoneRange(dateEdit->date(), cpEdit->value(), wp);

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    ranges->invisibleRootItem()->insertChild(index, add);

    // date
    add->setText(0, dateEdit->date().toString(tr("MMM d, yyyy")));

    // CP
    add->setText(1, QString("%1").arg(cpEdit->value()));

    // W'
    add->setText(2, QString("%1").arg(wp));

}

void
CPPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        zonePage->zones.deleteRange(index);
    }
}

void
CPPage::defaultClicked()
{
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        ZoneRange current = zonePage->zones.getZoneRange(index);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);


        // set the range to use defaults on the scheme page
        zonePage->zones.setScheme(zonePage->schemePage->getScheme());
        zonePage->zones.setZonesFromCP(index);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
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
        ZoneRange current = zonePage->zones.getZoneRange(index);

        if (current.zonesSetFromCP) {

            // reapply the scheme in case it has been changed
            zonePage->zones.setScheme(zonePage->schemePage->getScheme());
            zonePage->zones.setZonesFromCP(index);
            current = zonePage->zones.getZoneRange(index);

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
            ZoneRange current = zonePage->zones.getZoneRange(index);

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
            zonePage->zones.setZoneRange(index, current);
        }
    }
}

//
// Zone Config page
//
HrZonePage::HrZonePage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // get current config by reading it in (leave mainwindow zones alone)
    QFile zonesFile(context->athlete->home->config().canonicalPath() + "/hr.zones");
    if (zonesFile.exists()) {
        zones.read(zonesFile);
        zonesFile.close();
    }

    // setup maintenance pages using current config
    schemePage = new HrSchemePage(this);
    ltPage = new LTPage(this);

    tabs = new QTabWidget(this);
    tabs->addTab(ltPage, tr("Lactate Threshold"));
    tabs->addTab(schemePage, tr("Default"));

    layout->addWidget(tabs);
}

void
HrZonePage::saveClicked()
{
    zones.setScheme(schemePage->getScheme());
    zones.write(context->athlete->home->config());
}

HrSchemePage::HrSchemePage(HrZonePage* zonePage) : zonePage(zonePage)
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
    for (int i=0; i< zonePage->zones.getScheme().nzones_default; i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, zonePage->zones.getScheme().zone_default_name[i]);
        // field name
        add->setText(1, zonePage->zones.getScheme().zone_default_desc[i]);

        // low
        QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
        loedit->setMinimum(0);
        loedit->setMaximum(1000);
        loedit->setSingleStep(1.0);
        loedit->setDecimals(0);
        loedit->setValue(zonePage->zones.getScheme().zone_default[i]);
        scheme->setItemWidget(add, 2, loedit);

        // trimp
        QDoubleSpinBox *trimpedit = new QDoubleSpinBox(this);
        trimpedit->setMinimum(0);
        trimpedit->setMaximum(10);
        trimpedit->setSingleStep(0.1);
        trimpedit->setDecimals(2);
        trimpedit->setValue(zonePage->zones.getScheme().zone_default_trimp[i]);
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


LTPage::LTPage(HrZonePage* zonePage) : zonePage(zonePage)
{
    active = false;

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
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(defaultButton);

    QHBoxLayout *zoneButtons = new QHBoxLayout;
    zoneButtons->addStretch();
    zoneButtons->addWidget(addZoneButton);
    zoneButtons->addWidget(deleteZoneButton);

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
    for (int i=0; i< zonePage->zones.getRangeSize(); i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(zonePage->zones.getHrZoneRange(i).hrZonesSetFromLT ?
                       QFont::Normal : QFont::Black);

        // date
        add->setText(0, zonePage->zones.getStartDate(i).toString(tr("MMM d, yyyy")));
        add->setFont(0, font);

        // LT
        add->setText(1, QString("%1").arg(zonePage->zones.getLT(i)));
        add->setFont(1, font);

        // Rest HR
        add->setText(2, QString("%1").arg(zonePage->zones.getRestHr(i)));
        add->setFont(2, font);

        // Max HR
        add->setText(3, QString("%1").arg(zonePage->zones.getMaxHr(i)));
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

    // button connect
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
    zonePage->zones.setScheme(zonePage->schemePage->getScheme());

    //int index = ranges->invisibleRootItem()->childCount();
    int index = zonePage->zones.addHrZoneRange(dateEdit->date(), ltEdit->value(), restHrEdit->value(), maxHrEdit->value());

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
LTPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        zonePage->zones.deleteRange(index);
    }
}

void
LTPage::defaultClicked()
{
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        HrZoneRange current = zonePage->zones.getHrZoneRange(index);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);
        ranges->currentItem()->setFont(3, font);


        // set the range to use defaults on the scheme page
        zonePage->zones.setScheme(zonePage->schemePage->getScheme());
        zonePage->zones.setHrZonesFromLT(index);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
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
        HrZoneRange current = zonePage->zones.getHrZoneRange(index);

        if (current.hrZonesSetFromLT) {

            // reapply the scheme in case it has been changed
            zonePage->zones.setScheme(zonePage->schemePage->getScheme());
            zonePage->zones.setHrZonesFromLT(index);
            current = zonePage->zones.getHrZoneRange(index);

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
            HrZoneRange current = zonePage->zones.getHrZoneRange(index);

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
            zonePage->zones.setHrZoneRange(index, current);
        }
    }
}

//
// Pace Zone Config page
//
PaceZonePage::PaceZonePage(Context *context) : context(context)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // get current config by reading it in (leave mainwindow zones alone)
    QFile zonesFile(context->athlete->home->config().canonicalPath() + "/pace.zones");
    if (zonesFile.exists()) {
        zones.read(zonesFile);
        zonesFile.close();
    }

    // setup maintenance pages using current config
    schemePage = new PaceSchemePage(this);
    cvPage = new CVPage(this);

    tabs = new QTabWidget(this);
    tabs->addTab(cvPage, tr("Critical Velocity"));
    tabs->addTab(schemePage, tr("Default"));

    layout->addWidget(tabs);
}

void
PaceZonePage::saveClicked()
{
    appsettings->setValue(GC_PACE, cvPage->metric->isChecked());
    zones.setScheme(schemePage->getScheme());
    zones.write(context->athlete->home->config());
}

PaceSchemePage::PaceSchemePage(PaceZonePage* zonePage) : zonePage(zonePage)
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
    for (int i=0; i< zonePage->zones.getScheme().nzones_default; i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(scheme->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, zonePage->zones.getScheme().zone_default_name[i]);
        // field name
        add->setText(1, zonePage->zones.getScheme().zone_default_desc[i]);

        // low
        QDoubleSpinBox *loedit = new QDoubleSpinBox(this);
        loedit->setMinimum(0);
        loedit->setMaximum(1000);
        loedit->setSingleStep(1.0);
        loedit->setDecimals(0);
        loedit->setValue(zonePage->zones.getScheme().zone_default[i]);
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

static inline
double kphFromTime(QTimeEdit *cvedit, bool isminmile)
{
    // get the value from a time edit and convert
    // it to kph so we can store it in the zones file

    double secs = cvedit->time().secsTo(QTime(0,0,0)) * -1;
    return (isminmile ? KM_PER_MILE : 1.00f ) * (3600.00f / secs);
}

CVPage::CVPage(PaceZonePage* zonePage) : zonePage(zonePage)
{
    active = false;

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setSpacing(10);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#else
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

    cvEdit = new QTimeEdit(QTime::fromString("05:00", "mm:ss"));
    cvEdit->setMinimumTime(QTime::fromString("01:00", "mm:ss"));
    cvEdit->setMaximumTime(QTime::fromString("20:00", "mm:ss"));
    cvEdit->setDisplayFormat("mm:ss");

    per = new QLabel(this);
    metric = new QCheckBox("Metric Pace");
    metric->setChecked(appsettings->value(this, GC_PACE, true).toBool());
    if (metric->isChecked()) per->setText("per km");
    else per->setText("per mile");

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addWidget(cpLabel);
    actionButtons->addWidget(cvEdit);
    actionButtons->addWidget(per);
    actionButtons->addStretch();
    actionButtons->addWidget(metric);
    actionButtons->addStretch();
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
    for (int i=0; i< zonePage->zones.getRangeSize(); i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(ranges->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Embolden ranges with manually configured zones
        QFont font;
        font.setWeight(zonePage->zones.getZoneRange(i).zonesSetFromCV ?
                                        QFont::Normal : QFont::Black);

        // date
        add->setText(0, zonePage->zones.getStartDate(i).toString(tr("MMM d, yyyy")));
        add->setFont(0, font);

        // CV
        double kph = zonePage->zones.getCV(i);
        double minsPerKM = 60.00f / kph;
        double minsPerMile = minsPerKM * KM_PER_MILE;

        add->setText(1, QString("%1 min/km %2 min/mile").arg(minsPerKM, 0, 'f', 2).arg(minsPerMile, 0, 'f', 2));
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

    // button connect
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
    // need to switch between metric and imperial!
    if (metric->isChecked()) per->setText("per km");
    else per->setText("per mile");

}

void
CVPage::addClicked()
{
    // get current scheme
    zonePage->zones.setScheme(zonePage->schemePage->getScheme());

    int cp = kphFromTime(cvEdit, !metric->isChecked());
    if( cp <= 0 ){
        QMessageBox err;
        err.setText(tr("CV must be > 0"));
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    int index = zonePage->zones.addZoneRange(dateEdit->date(), kphFromTime(cvEdit, !metric->isChecked()));

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    ranges->invisibleRootItem()->insertChild(index, add);

    // date
    add->setText(0, dateEdit->date().toString(tr("MMM d, yyyy")));

    // CP
    double kph = kphFromTime(cvEdit, !metric->isChecked());
    double minsPerKM = 60.00f / kph;
    double minsPerMile = minsPerKM * KM_PER_MILE;

    add->setText(1, QString("%1 min/km %2 min/mile").arg(minsPerKM, 0, 'f', 2).arg(minsPerMile, 0, 'f', 2));

}

void
CVPage::deleteClicked()
{
    if (ranges->currentItem()) {
        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        delete ranges->invisibleRootItem()->takeChild(index);
        zonePage->zones.deleteRange(index);
    }
}

void
CVPage::defaultClicked()
{
    if (ranges->currentItem()) {

        int index = ranges->invisibleRootItem()->indexOfChild(ranges->currentItem());
        PaceZoneRange current = zonePage->zones.getZoneRange(index);

        // unbold
        QFont font;
        font.setWeight(QFont::Normal);
        ranges->currentItem()->setFont(0, font);
        ranges->currentItem()->setFont(1, font);
        ranges->currentItem()->setFont(2, font);


        // set the range to use defaults on the scheme page
        zonePage->zones.setScheme(zonePage->schemePage->getScheme());
        zonePage->zones.setZonesFromCV(index);

        // hide the default button since we are now using defaults
        defaultButton->hide();

        // update the zones display
        rangeSelectionChanged();
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
        PaceZoneRange current = zonePage->zones.getZoneRange(index);

        if (current.zonesSetFromCV) {

            // reapply the scheme in case it has been changed
            zonePage->zones.setScheme(zonePage->schemePage->getScheme());
            zonePage->zones.setZonesFromCV(index);
            current = zonePage->zones.getZoneRange(index);

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
            PaceZoneRange current = zonePage->zones.getZoneRange(index);

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
                zoneinfos << PaceZoneInfo(item->text(0),
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
            zonePage->zones.setZoneRange(index, current);
        }
    }
}

//
// Ride metadata page
//
MeasuresPage::MeasuresPage(Context *context) : context(context)
{
    QGridLayout *mainLayout = new QGridLayout(this);

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
    fields->headerItem()->setText(1, tr("Measure"));
    fields->headerItem()->setText(2, tr("Type"));
    fields->setColumnCount(3);
    fields->setSelectionMode(QAbstractItemView::SingleSelection);
    fields->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    //fields->setUniformRowHeights(true);
    fields->setIndentation(0);
    //fields->header()->resizeSection(0,130);
    //fields->header()->resizeSection(1,140);

    // temporary storage for measures config
    QList<FieldDefinition> fieldDefinitions;
    QList<KeywordDefinition> keywordDefinitions; //NOTE: not used in measures.xml

    // check we have one and use built in if not there
    QString filename = context->athlete->home->config().absolutePath()+"/measures.xml";
    QString colorfield;

    if (!QFile(filename).exists()) filename = ":/xml/measures.xml";
    RideMetadata::readXML(filename, keywordDefinitions, fieldDefinitions, colorfield);

    // iterate over the fields and setup the editable items
    foreach(FieldDefinition field, fieldDefinitions) {
        QTreeWidgetItem *add;
        QComboBox *comboButton = new QComboBox(this);

        FieldsPage::addFieldTypes(comboButton);
        comboButton->setCurrentIndex(field.type);

        add = new QTreeWidgetItem(fields->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // tab name
        add->setText(0, field.tab);
        // field name
        add->setText(1, field.name);

        // type button
        add->setTextAlignment(2, Qt::AlignHCenter);
        fields->setItemWidget(add, 2, comboButton);
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
MeasuresPage::upClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == 0) return; // its at the top already

        // movin on up!
        QWidget *button = fields->itemWidget(fields->currentItem(),2);
        QComboBox *comboButton = new QComboBox(this);
        FieldsPage::addFieldTypes(comboButton);
        comboButton->setCurrentIndex(((QComboBox*)button)->currentIndex());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index-1, moved);
        fields->setItemWidget(moved, 2, comboButton);
        fields->setCurrentItem(moved);
    }
}

void
MeasuresPage::downClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
        if (index == (fields->invisibleRootItem()->childCount()-1)) return; // its at the bottom already

        QWidget *button = fields->itemWidget(fields->currentItem(),2);
        QComboBox *comboButton = new QComboBox(this);
        FieldsPage::addFieldTypes(comboButton);
        comboButton->setCurrentIndex(((QComboBox*)button)->currentIndex());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index+1, moved);
        fields->setItemWidget(moved, 2, comboButton);
        fields->setCurrentItem(moved);
    }
}

void
MeasuresPage::renameClicked()
{
    // which one is selected?
    if (fields->currentItem()) fields->editItem(fields->currentItem(), 0);
}

void
MeasuresPage::addClicked()
{
    int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());
    if (index < 0) index = 0;
    QTreeWidgetItem *add;
    QComboBox *comboButton = new QComboBox(this);
    FieldsPage::addFieldTypes(comboButton);

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
}

void
MeasuresPage::deleteClicked()
{
    if (fields->currentItem()) {
        int index = fields->invisibleRootItem()->indexOfChild(fields->currentItem());

        // zap!
        delete fields->invisibleRootItem()->takeChild(index);
    }
}

void
MeasuresPage::getDefinitions(QList<FieldDefinition> &fieldList)
{
    QStringList checkdups;

    // clear current just in case
    fieldList.clear();

    for (int idx =0; idx < fields->invisibleRootItem()->childCount(); idx++) {

        FieldDefinition add;
        QTreeWidgetItem *item = fields->invisibleRootItem()->child(idx);

        // silently ignore duplicates
        if (checkdups.contains(item->text(1))) continue;
        else checkdups << item->text(1);

        add.tab = item->text(0);
        add.name = item->text(1);
        add.type = ((QComboBox*)fields->itemWidget(item, 2))->currentIndex();

        fieldList.append(add);
    }
}

void
MeasuresPage::saveClicked()
{
    QList<FieldDefinition> current;
    getDefinitions(current);

    // write to measures.xml (uses same format as metadata.xml)
    RideMetadata::serialize(context->athlete->home->config().canonicalPath() + "/measures.xml", QList<KeywordDefinition>(), current, "");
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
        add->setText(4, season._id.toString());

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

void
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
        array[i]._id = QUuid(item->text(4));
    }

    // write to disk
    QString file = QString(context->athlete->home->config().canonicalPath() + "/seasons.xml");
    SeasonParser::serialize(file, array);

    // re-read
    context->athlete->seasons->readSeasons();
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

void
AutoImportPage::saveClicked() {

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

}

void
AutoImportPage::addRuleTypes(QComboBox *p) {


    p->addItem(tr("No autoimport"));
    p->addItem(tr("Autoimport with dialog"));

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

