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
#include <QIntValidator>
#include <QHttp>
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

//
// Main Config Page - tabs for each sub-page
//
GeneralPage::GeneralPage(MainWindow *main) : main(main)
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
    if(crankLength.toString() == "160") crankLengthCombo->setCurrentIndex(0);
    if(crankLength.toString() == "162.5") crankLengthCombo->setCurrentIndex(1);
    if(crankLength.toString() == "165") crankLengthCombo->setCurrentIndex(2);
    if(crankLength.toString() == "167.5") crankLengthCombo->setCurrentIndex(3);
    if(crankLength.toString() == "170") crankLengthCombo->setCurrentIndex(4);
    if(crankLength.toString() == "172.5") crankLengthCombo->setCurrentIndex(5);
    if(crankLength.toString() == "175") crankLengthCombo->setCurrentIndex(6);
    if(crankLength.toString() == "177.5") crankLengthCombo->setCurrentIndex(7);
    if(crankLength.toString() == "180") crankLengthCombo->setCurrentIndex(8);
    if(crankLength.toString() == "182.5") crankLengthCombo->setCurrentIndex(9);
    if(crankLength.toString() == "185") crankLengthCombo->setCurrentIndex(10);

    configLayout->addWidget(crankLengthLabel, 1,0, Qt::AlignRight);
    configLayout->addWidget(crankLengthCombo, 1,1, Qt::AlignLeft);

    //
    // Wheel size
    //
    QLabel *wheelSizeLabel = new QLabel(tr("Wheelsize:"), this);
    int wheelSize = appsettings->value(this, GC_WHEELSIZE, 2100).toInt();

    wheelSizeCombo = new QComboBox();
    wheelSizeCombo->addItem("Road/Cross (700C/622)"); // 2100mm
    wheelSizeCombo->addItem("Tri/TT (650C)"); // 1960mm
    wheelSizeCombo->addItem("Mountain (26\")"); // 1985mm
    wheelSizeCombo->addItem("BMX (20\")"); // 1750mm

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
    QLabel *garminHWLabel = new QLabel(tr("Threshold (secs):"));
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
    
    //
    // Performance manager
    //

    perfManSTSLabel = new QLabel(tr("STS average (days):"));
    perfManLTSLabel = new QLabel(tr("LTS average (days):"));
    perfManSTSavgValidator = new QIntValidator(1,21,this);
    perfManLTSavgValidator = new QIntValidator(7,56,this);

    // get config or set to defaults
    QVariant perfManSTSVal = appsettings->cvalue(main->cyclist, GC_STS_DAYS);
    if (perfManSTSVal.isNull() || perfManSTSVal.toInt() == 0) perfManSTSVal = 7;
    QVariant perfManLTSVal = appsettings->cvalue(main->cyclist, GC_LTS_DAYS);
    if (perfManLTSVal.isNull() || perfManLTSVal.toInt() == 0) perfManLTSVal = 42;

    perfManSTSavg = new QLineEdit(perfManSTSVal.toString(),this);
    perfManSTSavg->setValidator(perfManSTSavgValidator);
    perfManLTSavg = new QLineEdit(perfManLTSVal.toString(),this);
    perfManLTSavg->setValidator(perfManLTSavgValidator);

    showSBToday = new QCheckBox(tr("PMC Stress Balance Today"), this);
    showSBToday->setChecked(appsettings->cvalue(main->cyclist, GC_SB_TODAY).toInt());

    configLayout->addWidget(perfManSTSLabel, 9,0, Qt::AlignRight);
    configLayout->addWidget(perfManSTSavg, 9,1, Qt::AlignLeft);
    configLayout->addWidget(perfManLTSLabel, 10,0, Qt::AlignRight);
    configLayout->addWidget(perfManLTSavg, 10,1, Qt::AlignLeft);
    configLayout->addWidget(showSBToday, 11,1, Qt::AlignLeft);

    //
    // Workout directory (train view)
    //
    QVariant workoutDir = appsettings->value(this, GC_WORKOUTDIR);
    workoutLabel = new QLabel(tr("Workout Library:"));
    workoutDirectory = new QLineEdit;
    workoutDirectory->setText(workoutDir.toString());
    workoutBrowseButton = new QPushButton(tr("Browse"));
    workoutBrowseButton->setFixedWidth(70);

    configLayout->addWidget(workoutLabel, 12,0, Qt::AlignRight);
    configLayout->addWidget(workoutDirectory, 12,1);
    configLayout->addWidget(workoutBrowseButton, 13,1);

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
    appsettings->setValue(GC_ELEVATION_HYSTERESIS, hystedit->text());

    // Performance Manager
    appsettings->setCValue(main->cyclist, GC_STS_DAYS, perfManSTSavg->text());
    appsettings->setCValue(main->cyclist, GC_LTS_DAYS, perfManLTSavg->text());
    appsettings->setCValue(main->cyclist, GC_SB_TODAY, (int) showSBToday->isChecked());

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
    workoutDirectory->setText(dir);
}

//
// Passwords page
//
CredentialsPage::CredentialsPage(QWidget *parent, MainWindow *mainWindow) : QScrollArea(parent), mainWindow(mainWindow)
{
    QWidget *main = new QWidget(this);
    main->setContentsMargins(0,0,0,0);

    QGridLayout *grid = new QGridLayout;

    QLabel *gc = new QLabel(tr("Golden Cheetah Racing"));
    QFont current;
    current.setWeight(QFont::Black);
    gc->setFont(current);

    QLabel *gcurlLabel = new QLabel(tr("Website"));
    QLabel *gcuserLabel = new QLabel(tr("Username"));
    QLabel *gcpassLabel = new QLabel(tr("Password"));

    QLabel *tp = new QLabel(tr("TrainingPeaks"));
    tp->setFont(current);

    QLabel *urlLabel = new QLabel(tr("Website"));
    QLabel *userLabel = new QLabel(tr("Username"));
    QLabel *passLabel = new QLabel(tr("Password"));
    QLabel *typeLabel = new QLabel(tr("Account Type"));

    QLabel *twp = new QLabel(tr("Twitter"));
    twp->setFont(current);

    QLabel *twurlLabel = new QLabel(tr("Website"));
    QLabel *twauthLabel = new QLabel(tr("Authorise"));
    QLabel *twpinLabel = new QLabel(tr("PIN"));

    QLabel *str = new QLabel(tr("Strava"));
    str->setFont(current);

    QLabel *struserLabel = new QLabel(tr("Username"));
    QLabel *strpassLabel = new QLabel(tr("Password"));

    QLabel *rwgps = new QLabel(tr("RideWithGPS"));
    rwgps->setFont(current);

    QLabel *rwgpsuserLabel = new QLabel(tr("Username"));
    QLabel *rwgpspassLabel = new QLabel(tr("Password"));

    QLabel *ttb = new QLabel(tr("Trainingstagebuch"));
    ttb->setFont(current);

    QLabel *ttbuserLabel = new QLabel(tr("Username"));
    QLabel *ttbpassLabel = new QLabel(tr("Password"));

    QLabel *wip = new QLabel(tr("Withings Wifi Scales"));
    wip->setFont(current);

    QLabel *wiurlLabel = new QLabel(tr("Website"));
    QLabel *wiuserLabel = new QLabel(tr("User Id"));
    QLabel *wipassLabel = new QLabel(tr("Public Key"));

    QLabel *zeo = new QLabel(tr("Zeo Sleep Data"));
    zeo->setFont(current);

    QLabel *zeourlLabel = new QLabel(tr("Website"));
    QLabel *zeouserLabel = new QLabel(tr("User"));
    QLabel *zeopassLabel = new QLabel(tr("Password"));


    QLabel *webcal = new QLabel(tr("Web Calendar"));
    webcal->setFont(current);
    QLabel *wcurlLabel = new QLabel(tr("Webcal URL"));

    QLabel *dv = new QLabel(tr("CalDAV Calendar"));
    dv->setFont(current);
    QLabel *dvurlLabel = new QLabel(tr("CalDAV URL"));
    QLabel *dvuserLabel = new QLabel(tr("CalDAV User Id"));
    QLabel *dvpassLabel = new QLabel(tr("CalDAV Password"));


    gcURL = new QLineEdit(this);
    gcURL->setText(appsettings->cvalue(mainWindow->cyclist, GC_GCURL, "http://race.goldencheetah.org").toString());

    gcUser = new QLineEdit(this);
    gcUser->setText(appsettings->cvalue(mainWindow->cyclist, GC_GCUSER, "").toString());

    gcPass = new QLineEdit(this);
    gcPass->setEchoMode(QLineEdit::Password);
    gcPass->setText(appsettings->cvalue(mainWindow->cyclist, GC_GCPASS, "").toString());

    tpURL = new QLineEdit(this);
    tpURL->setText(appsettings->cvalue(mainWindow->cyclist, GC_TPURL, "http://www.trainingpeaks.com").toString());

    tpUser = new QLineEdit(this);
    tpUser->setText(appsettings->cvalue(mainWindow->cyclist, GC_TPUSER, "").toString());

    tpPass = new QLineEdit(this);
    tpPass->setEchoMode(QLineEdit::Password);
    tpPass->setText(appsettings->cvalue(mainWindow->cyclist, GC_TPPASS, "").toString());

    tpType = new QComboBox(this);
    tpType->addItem("Shared Free");
    tpType->addItem("Coached Free");
    tpType->addItem("Self Coached Premium");
    tpType->addItem("Shared Self Coached Premium");
    tpType->addItem("Coached Premium");
    tpType->addItem("Shared Coached Premium");

    tpType->setCurrentIndex(appsettings->cvalue(mainWindow->cyclist, GC_TPTYPE, "0").toInt());

    twitterURL = new QLineEdit(this);
    twitterURL->setText(appsettings->cvalue(mainWindow->cyclist, GC_TWURL, "http://www.twitter.com").toString());
    twitterAuthorise = new QPushButton("Authorise", this);
    twitterPIN = new QLineEdit(this);
    twitterPIN->setText("");

    stravaUser = new QLineEdit(this);
    stravaUser->setText(appsettings->cvalue(mainWindow->cyclist, GC_STRUSER, "").toString());

    stravaPass = new QLineEdit(this);
    stravaPass->setEchoMode(QLineEdit::Password);
    stravaPass->setText(appsettings->cvalue(mainWindow->cyclist, GC_STRPASS, "").toString());

    rideWithGPSUser = new QLineEdit(this);
    rideWithGPSUser->setText(appsettings->cvalue(mainWindow->cyclist, GC_RWGPSUSER, "").toString());

    rideWithGPSPass = new QLineEdit(this);
    rideWithGPSPass->setEchoMode(QLineEdit::Password);
    rideWithGPSPass->setText(appsettings->cvalue(mainWindow->cyclist, GC_RWGPSPASS, "").toString());

    ttbUser = new QLineEdit(this);
    ttbUser->setText(appsettings->cvalue(mainWindow->cyclist, GC_TTBUSER, "").toString());

    ttbPass = new QLineEdit(this);
    ttbPass->setEchoMode(QLineEdit::Password);
    ttbPass->setText(appsettings->cvalue(mainWindow->cyclist, GC_TTBPASS, "").toString());

    wiURL = new QLineEdit(this);
    wiURL->setText(appsettings->cvalue(mainWindow->cyclist, GC_WIURL, "http://wbsapi.withings.net/").toString());

    wiUser = new QLineEdit(this);
    wiUser->setText(appsettings->cvalue(mainWindow->cyclist, GC_WIUSER, "").toString());

    wiPass = new QLineEdit(this);
    wiPass->setText(appsettings->cvalue(mainWindow->cyclist, GC_WIKEY, "").toString());

    zeoURL = new QLineEdit(this);
    zeoURL->setText(appsettings->cvalue(mainWindow->cyclist, GC_ZEOURL, "http://app-pro.myzeo.com:8080/").toString());

    zeoUser = new QLineEdit(this);
    zeoUser->setText(appsettings->cvalue(mainWindow->cyclist, GC_ZEOUSER, "").toString());

    zeoPass = new QLineEdit(this);
    zeoPass->setEchoMode(QLineEdit::Password);
    zeoPass->setText(appsettings->cvalue(mainWindow->cyclist, GC_ZEOPASS, "").toString());

    webcalURL = new QLineEdit(this);
    webcalURL->setText(appsettings->cvalue(mainWindow->cyclist, GC_WEBCAL_URL, "").toString());

    dvURL = new QLineEdit(this);
    QString url = appsettings->cvalue(mainWindow->cyclist, GC_DVURL, "").toString();
    url.replace("%40", "@"); // remove escape of @ character
    dvURL->setText(url);

    dvUser = new QLineEdit(this);
    dvUser->setText(appsettings->cvalue(mainWindow->cyclist, GC_DVUSER, "").toString());

    dvPass = new QLineEdit(this);
    dvPass->setEchoMode(QLineEdit::Password);
    dvPass->setText(appsettings->cvalue(mainWindow->cyclist, GC_DVPASS, "").toString());

    grid->addWidget(tp, 0,0);
    grid->addWidget(urlLabel, 1,0);
    grid->addWidget(userLabel, 2,0);
    grid->addWidget(passLabel, 3,0);
    grid->addWidget(typeLabel, 4,0);
    grid->addWidget(gc, 5,0);
    grid->addWidget(gcurlLabel, 6,0);
    grid->addWidget(gcuserLabel, 7,0);
    grid->addWidget(gcpassLabel, 8,0);
    grid->addWidget(twp, 9,0);
    grid->addWidget(twurlLabel, 10,0);
    grid->addWidget(twauthLabel, 11,0);
    grid->addWidget(twpinLabel, 12,0);
    grid->addWidget(str, 13,0);
    grid->addWidget(struserLabel, 14,0);
    grid->addWidget(strpassLabel, 15,0);
    grid->addWidget(rwgps, 17,0);
    grid->addWidget(rwgpsuserLabel, 18,0);
    grid->addWidget(rwgpspassLabel, 19,0);
    grid->addWidget(wip, 20,0);
    grid->addWidget(wiurlLabel, 21,0);
    grid->addWidget(wiuserLabel, 22,0);
    grid->addWidget(wipassLabel, 23,0);
    grid->addWidget(zeo, 24,0);
    grid->addWidget(zeourlLabel, 25,0);
    grid->addWidget(zeouserLabel, 26,0);
    grid->addWidget(zeopassLabel, 27,0);
    grid->addWidget(webcal, 28, 0);
    grid->addWidget(wcurlLabel, 29, 0);
    grid->addWidget(dv, 30,0);
    grid->addWidget(dvurlLabel, 31,0);
    grid->addWidget(dvuserLabel, 32,0);
    grid->addWidget(dvpassLabel, 33,0);
    grid->addWidget(ttb, 34,0);
    grid->addWidget(ttbuserLabel, 35,0);
    grid->addWidget(ttbpassLabel, 36,0);

    grid->addWidget(tpURL, 1, 1, 0);
    grid->addWidget(tpUser, 2, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(tpPass, 3, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(tpType, 4, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(gcURL, 6, 1, 0);
    grid->addWidget(gcUser, 7, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(gcPass, 8, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(twitterURL, 10, 1, 0);
    grid->addWidget(twitterAuthorise, 11, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(twitterPIN, 12, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(stravaUser, 14, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(stravaPass, 15, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(rideWithGPSUser, 18, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(rideWithGPSPass, 19, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(wiURL, 21, 1, 0);
    grid->addWidget(wiUser, 22, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(wiPass, 23, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(zeoURL, 25, 1, 0);
    grid->addWidget(zeoUser, 26, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(zeoPass, 27, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(webcalURL, 29, 1, 0);

    grid->addWidget(dvURL, 31, 1, 0);
    grid->addWidget(dvUser, 32, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(dvPass, 33, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(ttbUser, 35, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(ttbPass, 36, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->setColumnStretch(0,0);
    grid->setColumnStretch(1,3);

    main->setLayout(grid);

    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
    setWidget(main);

    connect(twitterAuthorise, SIGNAL(clicked()), this, SLOT(authoriseTwitter()));
}


void
CredentialsPage::saveClicked()
{
    appsettings->setCValue(mainWindow->cyclist, GC_GCURL, gcURL->text());
    appsettings->setCValue(mainWindow->cyclist, GC_GCUSER, gcUser->text());
    appsettings->setCValue(mainWindow->cyclist, GC_GCPASS, gcPass->text());
    appsettings->setCValue(mainWindow->cyclist, GC_TPURL, tpURL->text());
    appsettings->setCValue(mainWindow->cyclist, GC_TPUSER, tpUser->text());
    appsettings->setCValue(mainWindow->cyclist, GC_TPPASS, tpPass->text());
    appsettings->setCValue(mainWindow->cyclist, GC_STRUSER, stravaUser->text());
    appsettings->setCValue(mainWindow->cyclist, GC_STRPASS, stravaPass->text());
    appsettings->setCValue(mainWindow->cyclist, GC_RWGPSUSER, rideWithGPSUser->text());
    appsettings->setCValue(mainWindow->cyclist, GC_RWGPSPASS, rideWithGPSPass->text());
    appsettings->setCValue(mainWindow->cyclist, GC_TTBUSER, ttbUser->text());
    appsettings->setCValue(mainWindow->cyclist, GC_TTBPASS, ttbPass->text());
    appsettings->setCValue(mainWindow->cyclist, GC_TPTYPE, tpType->currentIndex());
    appsettings->setCValue(mainWindow->cyclist, GC_TWURL, twitterURL->text());
    appsettings->setCValue(mainWindow->cyclist, GC_WIURL, wiURL->text());
    appsettings->setCValue(mainWindow->cyclist, GC_WIUSER, wiUser->text());
    appsettings->setCValue(mainWindow->cyclist, GC_WIKEY, wiPass->text());
    appsettings->setCValue(mainWindow->cyclist, GC_ZEOURL, zeoURL->text());
    appsettings->setCValue(mainWindow->cyclist, GC_ZEOUSER, zeoUser->text());
    appsettings->setCValue(mainWindow->cyclist, GC_ZEOPASS, zeoPass->text());
    appsettings->setCValue(mainWindow->cyclist, GC_WEBCAL_URL, webcalURL->text());

    // escape the at character
    QString url = dvURL->text();
    url.replace("@", "%40");
    appsettings->setCValue(mainWindow->cyclist, GC_DVURL, url);
    appsettings->setCValue(mainWindow->cyclist, GC_DVUSER, dvUser->text());
    appsettings->setCValue(mainWindow->cyclist, GC_DVPASS, dvPass->text());
    saveTwitter(); // get secret key if PIN set
}

//
// About me
//
RiderPage::RiderPage(QWidget *parent, MainWindow *mainWindow) : QWidget(parent), mainWindow(mainWindow)
{
    QVBoxLayout *all = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout;

    QLabel *nicklabel = new QLabel(tr("Nickname"));
    QLabel *doblabel = new QLabel(tr("Date of Birth"));
    QLabel *sexlabel = new QLabel(tr("Sex"));
    QLabel *unitlabel = new QLabel(tr("Unit"));
    QLabel *biolabel = new QLabel(tr("Bio"));

    QString weighttext = QString(tr("Weight (%1)")).arg(mainWindow->useMetricUnits ? tr("kg") : tr("lb"));
    weightlabel = new QLabel(weighttext);

    nickname = new QLineEdit(this);
    nickname->setText(appsettings->cvalue(mainWindow->cyclist, GC_NICKNAME).toString());

    dob = new QDateEdit(this);
    dob->setDate(appsettings->cvalue(mainWindow->cyclist, GC_DOB).toDate());

    sex = new QComboBox(this);
    sex->addItem(tr("Male"));
    sex->addItem(tr("Female"));


    // we set to 0 or 1 for male or female since this
    // is language independent (and for once the code is easier!)
    sex->setCurrentIndex(appsettings->cvalue(mainWindow->cyclist, GC_SEX).toInt());

    unitCombo = new QComboBox();
    unitCombo->addItem(tr("Metric"));
    unitCombo->addItem(tr("Imperial"));

    if (mainWindow->useMetricUnits)
        unitCombo->setCurrentIndex(0);
    else
        unitCombo->setCurrentIndex(1);

    weight = new QDoubleSpinBox(this);
    weight->setMaximum(999.9);
    weight->setMinimum(0.0);
    weight->setDecimals(1);
    weight->setValue(appsettings->cvalue(mainWindow->cyclist, GC_WEIGHT).toDouble() * (mainWindow->useMetricUnits ? 1.0 : LB_PER_KG));

    bio = new QTextEdit(this);
    bio->setText(appsettings->cvalue(mainWindow->cyclist, GC_BIO).toString());

    if (QFileInfo(mainWindow->home.absolutePath() + "/" + "avatar.png").exists())
        avatar = QPixmap(mainWindow->home.absolutePath() + "/" + "avatar.png");
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
    grid->addWidget(biolabel, 5, 0, alignment);

    grid->addWidget(nickname, 0, 1, alignment);
    grid->addWidget(dob, 1, 1, alignment);
    grid->addWidget(sex, 2, 1, alignment);
    grid->addWidget(unitCombo, 3, 1, alignment);
    grid->addWidget(weight, 4, 1, alignment);
    grid->addWidget(bio, 6, 0, 1, 4);

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
                            "", tr("Images (*.png *.jpg *.bmp"));
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
    }
    else {
        QString weighttext = QString(tr("Weight (%1)")).arg(tr("lb"));
        weightlabel->setText(weighttext);
        weight->setValue(weight->value() * LB_PER_KG);
    }
}

void
RiderPage::saveClicked()
{
    appsettings->setCValue(mainWindow->cyclist, GC_NICKNAME, nickname->text());
    appsettings->setCValue(mainWindow->cyclist, GC_DOB, dob->date());
    appsettings->setCValue(mainWindow->cyclist, GC_WEIGHT, weight->value() * (unitCombo->currentIndex() ? KG_PER_LB : 1.0));

    if (unitCombo->currentIndex()==0)
        appsettings->setCValue(mainWindow->cyclist, GC_UNIT, GC_UNIT_METRIC);
    else if (unitCombo->currentIndex()==1)
        appsettings->setCValue(mainWindow->cyclist, GC_UNIT, GC_UNIT_IMPERIAL);

    appsettings->setCValue(mainWindow->cyclist, GC_SEX, sex->currentIndex());
    appsettings->setCValue(mainWindow->cyclist, GC_BIO, bio->toPlainText());
    avatar.save(mainWindow->home.absolutePath() + "/" + "avatar.png", "PNG");
}

//
// Realtime devices page
//
DevicePage::DevicePage(QWidget *parent, MainWindow *mainWindow) : QWidget(parent), mainWindow(mainWindow)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    DeviceTypes all;
    devices = all.getList();

    addButton = new QPushButton(tr("+"),this);
    delButton = new QPushButton(tr("-"),this);

    deviceList = new QTableView(this);
#ifdef Q_OS_MAC
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

    multiCheck = new QCheckBox("Allow multiple devices in Train View", this);
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
    AddDeviceWizard *p = new AddDeviceWizard(mainWindow);
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

    colors = new QTreeWidget;
    colors->headerItem()->setText(0, tr("Color"));
    colors->headerItem()->setText(1, tr("Select"));
    colors->setColumnCount(2);
    colors->setColumnWidth(0,310);
    colors->setSelectionMode(QAbstractItemView::NoSelection);
    //colors->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    colors->setUniformRowHeights(true);
    colors->setIndentation(0);
    //colors->header()->resizeSection(0,300);

    shadeZones = new QCheckBox;
    shadeZones->setChecked(appsettings->value(this, GC_SHADEZONES, true).toBool());
    antiAliased = new QCheckBox;
    antiAliased->setChecked(appsettings->value(this, GC_ANTIALIAS, false).toBool());
    lineWidth = new QDoubleSpinBox;
    lineWidth->setMaximum(5);
    lineWidth->setMinimum(0.5);
    lineWidth->setSingleStep(0.5);
    reset = new QPushButton("Reset");
    lineWidth->setValue(appsettings->value(this, GC_LINEWIDTH, 2.0).toDouble());

    QLabel *lineWidthLabel = new QLabel(tr("Line Width"));
    QLabel *antialiasLabel = new QLabel(tr("Antialias" ));
    QLabel *shadeZonesLabel = new QLabel(tr("Shade Zones" ));

    QLabel *defaultLabel = new QLabel(tr("Default"));
    QLabel *titlesLabel = new QLabel(tr("Title" ));
    QLabel *markerLabel = new QLabel(tr("Chart Markers" ));
    QLabel *chartLabel = new QLabel(tr("Chart Labels" ));
    QLabel *calendarLabel = new QLabel(tr("Calendar Text" ));
    QLabel *popupLabel = new QLabel(tr("Popup Text" ));

    def = new QFontComboBox(this);
    titles = new QFontComboBox(this);
    chartmarkers = new QFontComboBox(this);
    chartlabels = new QFontComboBox(this);
    calendar = new QFontComboBox(this);
    popup = new QFontComboBox(this);

    defaultSize = new QComboBox(this); setSizes(defaultSize);
    titlesSize = new QComboBox(this); setSizes(titlesSize);
    chartmarkersSize = new QComboBox(this); setSizes(chartmarkersSize);
    chartlabelsSize = new QComboBox(this); setSizes(chartlabelsSize);
    calendarSize = new QComboBox(this); setSizes(calendarSize);
    popupSize = new QComboBox(this); setSizes(popupSize);

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
    popup->setCurrentIndex(0);
    popup->setCurrentIndex(1);
    popup->setCurrentFont(QFont());

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

    font.fromString(appsettings->value(this, GC_FONT_POPUP, QFont().toString()).toString());
    popup->setCurrentFont(font);

#ifdef Q_OS_MAC
    defaultSize->setCurrentIndex((appsettings->value(this, GC_FONT_DEFAULT_SIZE, 12).toInt() -7) / 2);
    titlesSize->setCurrentIndex((appsettings->value(this, GC_FONT_TITLES_SIZE, 12).toInt() -7) / 2);
    chartmarkersSize->setCurrentIndex((appsettings->value(this, GC_FONT_CHARTMARKERS_SIZE, 12).toInt() -7) / 2);
    chartlabelsSize->setCurrentIndex((appsettings->value(this, GC_FONT_CHARTLABELS_SIZE, 12).toInt() -7) / 2);
    calendarSize->setCurrentIndex((appsettings->value(this, GC_FONT_CALENDAR_SIZE, 12).toInt() -7) / 2);
    popupSize->setCurrentIndex((appsettings->value(this, GC_FONT_POPUP_SIZE, 10).toInt() -7) / 2);
#else
    defaultSize->setCurrentIndex((appsettings->value(this, GC_FONT_DEFAULT_SIZE, 12).toInt() -6) / 2);
    titlesSize->setCurrentIndex((appsettings->value(this, GC_FONT_TITLES_SIZE, 12).toInt() -6) / 2);
    chartmarkersSize->setCurrentIndex((appsettings->value(this, GC_FONT_CHARTMARKERS_SIZE, 12).toInt() -6) / 2);
    chartlabelsSize->setCurrentIndex((appsettings->value(this, GC_FONT_CHARTLABELS_SIZE, 12).toInt() -6) / 2);
    calendarSize->setCurrentIndex((appsettings->value(this, GC_FONT_CALENDAR_SIZE, 12).toInt() -6) / 2);
    popupSize->setCurrentIndex((appsettings->value(this, GC_FONT_POPUP_SIZE, 10).toInt() -6) / 2);
#endif

    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(5);

    QHBoxLayout *misc = new QHBoxLayout;
    misc->addWidget(lineWidthLabel);
    misc->addWidget(lineWidth);
    misc->addStretch();
    misc->addWidget(antialiasLabel);
    misc->addWidget(antiAliased);
    misc->addWidget(shadeZonesLabel);
    misc->addWidget(shadeZones);
    misc->addStretch();
    misc->addWidget(reset);

    grid->addWidget(defaultLabel, 0,0);
    grid->addWidget(titlesLabel, 1,0);
    grid->addWidget(markerLabel, 2,0);
    grid->addWidget(chartLabel, 3,0);
    grid->addWidget(calendarLabel, 4,0);
    grid->addWidget(popupLabel, 5,0);

    grid->addWidget(def, 0,1, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(titles, 1,1, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(chartmarkers, 2,1, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(chartlabels, 3,1, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(calendar, 4,1, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(popup, 5,1, Qt::AlignVCenter|Qt::AlignLeft);

    grid->addWidget(defaultSize, 0,2, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(titlesSize, 1,2, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(chartmarkersSize, 2,2, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(chartlabelsSize, 3,2, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(calendarSize, 4,2, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(popupSize, 5,2, Qt::AlignVCenter|Qt::AlignLeft);

    grid->setColumnStretch(0,1);
    grid->setColumnStretch(1,5);
    grid->setColumnStretch(2,1);
    grid->setColumnStretch(3,1);
    grid->setColumnStretch(4,3);

    mainLayout->addLayout(grid);
    mainLayout->addLayout(misc);
    mainLayout->addWidget(colors);

    colorSet = GCColor::colorSet();
    for (int i=0; colorSet[i].name != ""; i++) {

        QTreeWidgetItem *add;
        ColorButton *colorButton = new ColorButton(this, colorSet[i].name, colorSet[i].color);
        add = new QTreeWidgetItem(colors->invisibleRootItem());
        add->setText(0, colorSet[i].name);
        colors->setItemWidget(add, 1, colorButton);

    }
    connect(reset, SIGNAL(clicked()), this, SLOT(resetClicked()));
}

void
ColorsPage::resetClicked()
{
    colorSet = GCColor::defaultColorSet();
    colors->clear();
    for (int i=0; colorSet[i].name != ""; i++) {

        QTreeWidgetItem *add;
        ColorButton *colorButton = new ColorButton(this, colorSet[i].name, colorSet[i].color);
        add = new QTreeWidgetItem(colors->invisibleRootItem());
        add->setText(0, colorSet[i].name);
        colors->setItemWidget(add, 1, colorButton);

    }
}

void
ColorsPage::saveClicked()
{
    appsettings->setValue(GC_LINEWIDTH, lineWidth->value());
    appsettings->setValue(GC_ANTIALIAS, antiAliased->isChecked());
    appsettings->setValue(GC_SHADEZONES, shadeZones->isChecked());

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
    appsettings->setValue(GC_FONT_POPUP, popup->currentFont().toString());
#ifdef Q_OS_MAC
    appsettings->setValue(GC_FONT_DEFAULT_SIZE, 7+(defaultSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_TITLES_SIZE, 7+(titlesSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CHARTMARKERS_SIZE, 7+(chartmarkersSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CHARTLABELS_SIZE, 7+(chartlabelsSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CALENDAR_SIZE, 7+(calendarSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_POPUP_SIZE, 7+(calendarSize->currentIndex()*2));
#else
    appsettings->setValue(GC_FONT_DEFAULT_SIZE, 6+(defaultSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_TITLES_SIZE, 6+(titlesSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CHARTMARKERS_SIZE, 6+(chartmarkersSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CHARTLABELS_SIZE, 6+(chartlabelsSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_CALENDAR_SIZE, 6+(calendarSize->currentIndex()*2));
    appsettings->setValue(GC_FONT_POPUP_SIZE, 6+(calendarSize->currentIndex()*2));
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
        QString name = m->name();
        name.replace(tr("&#8482;"), tr(" (TM)"));
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, symbol);
        availList->addItem(item);
    }
    foreach (QString symbol, selectedMetrics) {
        if (!factory.haveMetric(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QString name = m->name();
        name.replace(tr("&#8482;"), tr(" (TM)"));
        QListWidgetItem *item = new QListWidgetItem(name);
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
        QString name = m->name();
        name.replace(tr("&#8482;"), tr(" (TM)"));
        QListWidgetItem *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, symbol);
        availList->addItem(item);
    }
    foreach (QString symbol, selectedMetrics) {
        if (!factory.haveMetric(symbol))
            continue;
        QSharedPointer<RideMetric> m(factory.newMetric(symbol));
        QString name = m->name();
        name.replace(tr("&#8482;"), tr(" (TM)"));
        QListWidgetItem *item = new QListWidgetItem(name);
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

MetadataPage::MetadataPage(MainWindow *main) : main(main)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // get current config using default file
    keywordDefinitions = main->rideMetadata()->getKeywords();
    fieldDefinitions = main->rideMetadata()->getFields();
    colorfield = main->rideMetadata()->getColorField();

    // setup maintenance pages using current config
    fieldsPage = new FieldsPage(this, fieldDefinitions);
    keywordsPage = new KeywordsPage(this, keywordDefinitions);
    processorPage = new ProcessorPage(main);

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

    // write to metadata.xml
    RideMetadata::serialize(main->home.absolutePath() + "/metadata.xml", keywordDefinitions, fieldDefinitions, colorfield);

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
    keywords->setUniformRowHeights(true);
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
    fields->setUniformRowHeights(true);
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
    QString text = "New";
    for (int i=0; fields->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
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
ProcessorPage::ProcessorPage(MainWindow *main) : main(main)
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
// Zone Config page
//
ZonePage::ZonePage(MainWindow *main) : main(main)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // get current config by reading it in (leave mainwindow zones alone)
    QFile zonesFile(main->home.absolutePath() + "/power.zones");
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
    zones.write(main->home);
}

SchemePage::SchemePage(ZonePage* zonePage) : zonePage(zonePage)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
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
        err.setText("Maximum of 10 zones reached.");
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
    QString text = "New";
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
    }
    add->setText(0, text);

    // long
    text = "New";
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
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
#endif
    defaultButton = new QPushButton(tr("Def"));
    defaultButton->hide();

    addZoneButton = new QPushButton(tr("+"));
    deleteZoneButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addZoneButton->setFixedSize(20,20);
    deleteZoneButton->setFixedSize(20,20);
#endif

    QHBoxLayout *actionButtons = new QHBoxLayout;
    actionButtons->setSpacing(2);
    actionButtons->addStretch();
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(defaultButton);

    QHBoxLayout *zoneButtons = new QHBoxLayout;
    zoneButtons->addStretch();
    zoneButtons->setSpacing(0);
    zoneButtons->addWidget(addZoneButton);
    zoneButtons->addWidget(deleteZoneButton);

    QHBoxLayout *addLayout = new QHBoxLayout;
    QLabel *dateLabel = new QLabel(tr("From Date"));
    QLabel *cpLabel = new QLabel(tr("Critical Power"));
    dateEdit = new QDateEdit;
    dateEdit->setDate(QDate::currentDate());

    cpEdit = new QDoubleSpinBox;
    cpEdit->setMinimum(0);
    cpEdit->setMaximum(1000);
    cpEdit->setSingleStep(1.0);
    cpEdit->setDecimals(0);

    addLayout->addWidget(dateLabel);
    addLayout->addWidget(dateEdit);
    addLayout->addWidget(cpLabel);
    addLayout->addWidget(cpEdit);
    addLayout->addStretch();

    ranges = new QTreeWidget;
    ranges->headerItem()->setText(0, tr("From Date"));
    ranges->headerItem()->setText(1, tr("Critical Power"));
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
        font.setWeight(zonePage->zones.getZoneRange(i).zonesSetFromCP ?
                                        QFont::Normal : QFont::Black);

        // date
        add->setText(0, zonePage->zones.getStartDate(i).toString("MMM d, yyyy"));
        add->setFont(0, font);

        // CP
        add->setText(1, QString("%1").arg(zonePage->zones.getCP(i)));
        add->setFont(1, font);
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
        err.setText("CP must be > 0");
        err.setIcon(QMessageBox::Warning);
        err.exec();
        return;
    }

    //int index = ranges->invisibleRootItem()->childCount();
    int index = zonePage->zones.addZoneRange(dateEdit->date(), cpEdit->value());

    // new item
    QTreeWidgetItem *add = new QTreeWidgetItem;
    add->setFlags(add->flags() & ~Qt::ItemIsEditable);
    ranges->invisibleRootItem()->insertChild(index, add);

    // date
    add->setText(0, dateEdit->date().toString("MMM d, yyyy"));

    // CP
    add->setText(1, QString("%1").arg(cpEdit->value()));
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
        err.setText("Maximum of 10 zones reached.");
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
    QString text = "New";
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
    }
    add->setText(0, text);

    // long
    text = "New";
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
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
HrZonePage::HrZonePage(MainWindow *main) : main(main)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // get current config by reading it in (leave mainwindow zones alone)
    QFile zonesFile(main->home.absolutePath() + "/hr.zones");
    if (zonesFile.exists()) {
        zones.read(zonesFile);
        zonesFile.close();
    }

    // setup maintenance pages using current config
    schemePage = new HrSchemePage(this);
    ltPage = new LTPage(this);

    tabs = new QTabWidget(this);
    tabs->addTab(ltPage, tr("Lactic Threshold"));
    tabs->addTab(schemePage, tr("Default"));

    layout->addWidget(tabs);
}

void
HrZonePage::saveClicked()
{
    zones.setScheme(schemePage->getScheme());
    zones.write(main->home);
}

HrSchemePage::HrSchemePage(HrZonePage* zonePage) : zonePage(zonePage)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
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
        err.setText("Maximum of 10 zones reached.");
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
    QString text = "New";
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
    }
    add->setText(0, text);

    // long
    text = "New";
    for (int i=0; scheme->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
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

    addButton = new QPushButton(tr("+"));
    deleteButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addButton->setFixedSize(20,20);
    deleteButton->setFixedSize(20,20);
#endif
    defaultButton = new QPushButton(tr("Def"));
    defaultButton->hide();

    addZoneButton = new QPushButton(tr("+"));
    deleteZoneButton = new QPushButton(tr("-"));
#ifndef Q_OS_MAC
    addZoneButton->setFixedSize(20,20);
    deleteZoneButton->setFixedSize(20,20);
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
    QLabel *ltLabel = new QLabel(tr("Lactic Threshold"));
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
    ranges->headerItem()->setText(1, tr("Lactic Threshold"));
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
        add->setText(0, zonePage->zones.getStartDate(i).toString("MMM d, yyyy"));
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
    add->setText(0, dateEdit->date().toString("MMM d, yyyy"));

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
        err.setText("Maximum of 10 zones reached.");
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
    QString text = "New";
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
    }
    add->setText(0, text);

    // long
    text = "New";
    for (int i=0; zones->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
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
// Ride metadata page
//
MeasuresPage::MeasuresPage(MainWindow *main) : main(main)
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
    fields->setUniformRowHeights(true);
    fields->setIndentation(0);
    //fields->header()->resizeSection(0,130);
    //fields->header()->resizeSection(1,140);

    // temporary storage for measures config
    QList<FieldDefinition> fieldDefinitions;
    QList<KeywordDefinition> keywordDefinitions; //NOTE: not used in measures.xml

    // check we have one and use built in if not there
    QString filename = main->home.absolutePath()+"/measures.xml";
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
    QString text = "New";
    for (int i=0; fields->findItems(text, Qt::MatchExactly, 1).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
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
    RideMetadata::serialize(main->home.absolutePath() + "/measures.xml", QList<KeywordDefinition>(), current, "");
}

void CredentialsPage::authoriseTwitter()
{
#ifdef GC_HAVE_LIBOAUTH
    int rc;
    char **rv = NULL;
    QString token;
    QString url = QString();
    t_key = NULL;
    t_secret = NULL;

    const char *request_token_uri = "http://api.twitter.com/oauth/request_token";

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
    url = QString("http://api.twitter.com/oauth/authorize?");
    url.append(token);
    QDesktopServices::openUrl(QUrl(url));
    if(rv) free(rv);
#endif
}

void CredentialsPage::saveTwitter()
{
#ifdef GC_HAVE_LIBOAUTH
    char *reply;
    char *req_url;
    char **rv = NULL;
    char *postarg = NULL;
    QString url = QString("http://api.twitter.com/oauth/access_token?a=b&oauth_verifier=");

    QString strPin = twitterPIN->text();
    if(strPin.size() == 0)
        return;

    url.append(strPin);

    req_url = oauth_sign_url2(url.toLatin1(), NULL, OA_HMAC, NULL, GC_TWITTER_CONSUMER_KEY, GC_TWITTER_CONSUMER_SECRET, t_key, t_secret);
    reply = oauth_http_get(req_url,postarg);

    int rc = oauth_split_url_parameters(reply, &rv);

    if(rc ==4)
    {
        qsort(rv, rc, sizeof(char *), oauth_cmpstringp);

        const char *oauth_token = strdup(&(rv[0][12]));
        const char *oauth_secret = strdup(&(rv[1][19]));

        //Save Twitter oauth_token and oauth_secret;
        appsettings->setValue(GC_TWITTER_TOKEN, oauth_token);
        appsettings->setValue(GC_TWITTER_SECRET, oauth_secret);
    }
#endif
}

//
// Season Editor
//
SeasonsPage::SeasonsPage(QWidget *parent, MainWindow *mainWindow) : QWidget(parent), mainWindow(mainWindow)
{
    QGridLayout *mainLayout = new QGridLayout(this);
    QFormLayout *editLayout = new QFormLayout;
    editLayout->setFieldGrowthPolicy(QFormLayout::FieldsStayAtSizeHint);

    // get the list
    array = mainWindow->seasons->seasons;

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
    editLayout->addRow(new QLabel("Name"), nameEdit);
    editLayout->addRow(new QLabel("Type"), typeEdit);
    editLayout->addRow(new QLabel("From"), fromEdit);
    editLayout->addRow(new QLabel("To"), toEdit);

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
        add->setText(2, season.start.toString("ddd MMM d, yyyy"));
        // to
        add->setText(3, season.end.toString("ddd MMM d, yyyy"));
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
    add->setText(2, fromEdit->date().toString("ddd MMM d, yyyy"));
    // to
    add->setText(3, toEdit->date().toString("ddd MMM d, yyyy"));
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
    QString file = QString(mainWindow->home.absolutePath() + "/seasons.xml");
    SeasonParser::serialize(file, array);

    // re-read
    mainWindow->seasons->readSeasons();
}
