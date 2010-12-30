#include <QtGui>
#include <QIntValidator>
#include <assert.h>
#include "Pages.h"
#include "Units.h"
#include "Settings.h"
#include "Units.h"
#include "Colors.h"
#include "DeviceTypes.h"
#include "DeviceConfiguration.h"
#include "ANTplusController.h"
#include "ColorButton.h"
#include "SpecialFields.h"
#include "DataProcessor.h"

//
// Main Config Page - tabs for each sub-page
//
ConfigurationPage::ConfigurationPage(MainWindow *main) : main(main)
{
    QTabWidget *tabs = new QTabWidget(this);
    QWidget *config = new QWidget(this);
    QVBoxLayout *configLayout = new QVBoxLayout(config);
    colorsPage = new ColorsPage(main);
    intervalMetrics = new IntervalMetricsPage;
    metadataPage = new MetadataPage(main);
    measuresPage = new MeasuresPage(main);
    proxyPage = new ProxyPage(this, main);

    tabs->addTab(config, tr("Settings"));
    tabs->addTab(colorsPage, tr("Appearance"));
    tabs->addTab(intervalMetrics, tr("Interval Metrics"));
    tabs->addTab(metadataPage, tr("Ride Data"));
    tabs->addTab(measuresPage, tr("Athlete Data"));
    tabs->addTab(proxyPage, tr("Proxy"));

    langLabel = new QLabel(tr("Language:"));

    langCombo = new QComboBox();
    langCombo->addItem(tr("English"));
    langCombo->addItem(tr("French"));
    langCombo->addItem(tr("Japanese"));
    langCombo->addItem(tr("Portugese (Brazil)"));
    langCombo->addItem(tr("Italian"));
    langCombo->addItem(tr("German"));

    QVariant lang = appsettings->value(this, GC_LANG);

    if(lang.toString() == "en")
        langCombo->setCurrentIndex(0);
    else if(lang.toString() == "fr")
        langCombo->setCurrentIndex(1);
    else if(lang.toString() == "ja")
        langCombo->setCurrentIndex(2);
    else if(lang.toString() == "pt-br")
        langCombo->setCurrentIndex(3);
    else if(lang.toString() == "it")
        langCombo->setCurrentIndex(4);
    else if(lang.toString() == "de")
        langCombo->setCurrentIndex(5);
    else // default : English
        langCombo->setCurrentIndex(0);

    unitLabel = new QLabel(tr("Unit of Measurement:"));

    unitCombo = new QComboBox();
    unitCombo->addItem(tr("Metric"));
    unitCombo->addItem(tr("Imperial"));

    QVariant unit = appsettings->value(this, GC_UNIT);

    if(unit.toString() == "Metric")
	unitCombo->setCurrentIndex(0);
    else
	unitCombo->setCurrentIndex(1);

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
    if(crankLength.toString() == "160")
	crankLengthCombo->setCurrentIndex(0);
    if(crankLength.toString() == "162.5")
	crankLengthCombo->setCurrentIndex(1);
    if(crankLength.toString() == "165")
	crankLengthCombo->setCurrentIndex(2);
    if(crankLength.toString() == "167.5")
	crankLengthCombo->setCurrentIndex(3);
    if(crankLength.toString() == "170")
	crankLengthCombo->setCurrentIndex(4);
    if(crankLength.toString() == "172.5")
	crankLengthCombo->setCurrentIndex(5);
    if(crankLength.toString() == "175")
	crankLengthCombo->setCurrentIndex(6);
    if(crankLength.toString() == "177.5")
	crankLengthCombo->setCurrentIndex(7);
    if(crankLength.toString() == "180")
	crankLengthCombo->setCurrentIndex(8);
    if(crankLength.toString() == "182.5")
	crankLengthCombo->setCurrentIndex(9);
    if(crankLength.toString() == "185")
	crankLengthCombo->setCurrentIndex(10);

    allRidesAscending = new QCheckBox(tr("Sort ride list ascending."), this);
    QVariant isAscending = appsettings->value(this, GC_ALLRIDES_ASCENDING,Qt::Checked); // default is ascending sort
    if(isAscending.toInt() > 0 ){
	allRidesAscending->setCheckState(Qt::Checked);
    } else {
	allRidesAscending->setCheckState(Qt::Unchecked);
    }

    // garmin Smart Recording options
    QVariant garminHWMark = appsettings->value(this, GC_GARMIN_HWMARK);
    if (garminHWMark.isNull() || garminHWMark.toInt() == 0)
      garminHWMark.setValue(25); // by default, set the threshold to 25 seconds
    QGridLayout *garminLayout = new QGridLayout;
    garminSmartRecord = new QCheckBox(tr("Use Garmin Smart Recording."), this);
    QVariant isGarminSmartRecording = appsettings->value(this, GC_GARMIN_SMARTRECORD, Qt::Checked);
    if (isGarminSmartRecording.toInt() > 0){
      garminSmartRecord->setCheckState(Qt::Checked);
    } else {
      garminSmartRecord->setCheckState(Qt::Unchecked);
    }
    QLabel *garminHWLabel1 = new QLabel(tr("Smart Recording Threshold "));
    QLabel *garminHWLabel2 = new QLabel(tr(" secs."));
    garminHWMarkedit = new QLineEdit(garminHWMark.toString(),this);
    garminHWMarkedit->setInputMask("009");
    garminLayout->addWidget(garminSmartRecord);
    garminLayout->addWidget(garminHWLabel1,1,0);
    garminLayout->addWidget(garminHWMarkedit,1,1);
    garminLayout->addWidget(garminHWLabel2,1,2);


    warningLabel = new QLabel(tr("Requires Restart To Take Effect"));

    langLayout = new QHBoxLayout;
    langLayout->addWidget(langLabel);
    langLayout->addWidget(langCombo);

    unitLayout = new QHBoxLayout;
    unitLayout->addWidget(unitLabel);
    unitLayout->addWidget(unitCombo);

    warningLayout = new QHBoxLayout;
    warningLayout->addWidget(warningLabel);

    QHBoxLayout *crankLengthLayout = new QHBoxLayout;
    crankLengthLayout->addWidget(crankLengthLabel);
    crankLengthLayout->addWidget(crankLengthCombo);


    // BikeScore Estimate
    QVariant BSdays = appsettings->value(this, GC_BIKESCOREDAYS);
    QVariant BSmode = appsettings->value(this, GC_BIKESCOREMODE);

    QGridLayout *bsDaysLayout = new QGridLayout;
    bsModeLayout = new QHBoxLayout;
    QLabel *BSDaysLabel1 = new QLabel(tr("BikeScore Estimate: use rides within last "));
    QLabel *BSDaysLabel2 = new QLabel(tr(" days"));
    BSdaysEdit = new QLineEdit(BSdays.toString(),this);
    BSdaysEdit->setInputMask("009");

    QLabel *BSModeLabel = new QLabel(tr("BikeScore estimate mode: "));
    bsModeCombo = new QComboBox();
    bsModeCombo->addItem(tr("time"));
    bsModeCombo->addItem(tr("distance"));
    if (BSmode.toString() == "time")
	bsModeCombo->setCurrentIndex(0);
    else
	bsModeCombo->setCurrentIndex(1);

    bsDaysLayout->addWidget(BSDaysLabel1,0,0);
    bsDaysLayout->addWidget(BSdaysEdit,0,1);
    bsDaysLayout->addWidget(BSDaysLabel2,0,2);

    bsModeLayout->addWidget(BSModeLabel);
    bsModeLayout->addWidget(bsModeCombo);

    // Workout Library
    QVariant workoutDir = appsettings->value(this, GC_WORKOUTDIR);
    workoutLabel = new QLabel(tr("Workout Library"));
    workoutDirectory = new QLineEdit;
    workoutDirectory->setText(workoutDir.toString());
    workoutBrowseButton = new QPushButton(tr("Browse"));
    workoutLayout = new QHBoxLayout;
    workoutLayout->addWidget(workoutLabel);
    workoutLayout->addWidget(workoutBrowseButton);
    workoutLayout->addWidget(workoutDirectory);
    connect(workoutBrowseButton, SIGNAL(clicked()),
            this, SLOT(browseWorkoutDir()));


    configLayout->addLayout(langLayout);
    configLayout->addLayout(unitLayout);
    configLayout->addWidget(allRidesAscending);
    configLayout->addLayout(garminLayout);
    //SmartRecord);
    configLayout->addLayout(crankLengthLayout);
    configLayout->addLayout(bsDaysLayout);
    configLayout->addLayout(bsModeLayout);
    configLayout->addLayout(workoutLayout);
    configLayout->addLayout(warningLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabs);
    setLayout(mainLayout);
}

void
ConfigurationPage::saveClicked()
{
    colorsPage->saveClicked();
    intervalMetrics->saveClicked();
    metadataPage->saveClicked();
    measuresPage->saveClicked();
    proxyPage->saveClicked();
}

void
ConfigurationPage::browseWorkoutDir()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Workout Library"),
                            "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    workoutDirectory->setText(dir);
}


//
// Cyclist settings - About me, Zones Config, PM and Password tabs
//
CyclistPage::CyclistPage(MainWindow *main) :
    mainWindow(main)
{
    QTabWidget *tabs = new QTabWidget(this);
    QWidget *cpTab = new QWidget(this);
    QWidget *hrTab = new QWidget(this);
    QWidget *rdTab = new QWidget(this);
    QWidget *psTab = new QWidget(this);
    QWidget *pmTab = new QWidget(this);
    tabs->addTab(rdTab, tr("About Me"));
    tabs->addTab(cpTab, tr("Power Zones"));
    tabs->addTab(hrTab, tr("HR Zones"));
    tabs->addTab(pmTab, tr("Performance Manager"));
    tabs->addTab(psTab, tr("Passwords"));
    QVBoxLayout *cpLayout = new QVBoxLayout(cpTab);
    QVBoxLayout *hrLayout = new QVBoxLayout(hrTab);
    QVBoxLayout *pmLayout = new QVBoxLayout(pmTab);
    QVBoxLayout *rdLayout = new QVBoxLayout(rdTab);
    QVBoxLayout *psLayout = new QVBoxLayout(psTab);

    riderPage = new RiderPage(this, mainWindow);
    rdLayout->addWidget(riderPage);

    zonePage = new ZonePage(mainWindow);
    cpLayout->addWidget(zonePage);

    hrZonePage = new HrZonePage(mainWindow);
    hrLayout->addWidget(hrZonePage);

    passPage = new CredentialsPage(this, mainWindow);
    psLayout->addWidget(passPage);

    perfManLabel = new QLabel(tr("Performance Manager"));
    showSBToday = new QCheckBox(tr("Show Stress Balance Today"), this);
    showSBToday->setChecked(appsettings->cvalue(mainWindow->cyclist, GC_SB_TODAY).toInt());

    perfManStartLabel = new QLabel(tr("Starting LTS"));
    perfManSTSLabel = new QLabel(tr("STS average (days)"));
    perfManLTSLabel = new QLabel(tr("LTS average (days)"));
    perfManStartValidator = new QIntValidator(0,200,this);
    perfManSTSavgValidator = new QIntValidator(1,21,this);
    perfManLTSavgValidator = new QIntValidator(7,56,this);
    QVariant perfManStartVal = appsettings->cvalue(mainWindow->cyclist, GC_INITIAL_STS);
    QVariant perfManSTSVal = appsettings->cvalue(mainWindow->cyclist, GC_STS_DAYS);

    if (perfManSTSVal.isNull() || perfManSTSVal.toInt() == 0)
	perfManSTSVal = 7;
    QVariant perfManLTSVal = appsettings->cvalue(mainWindow->cyclist, GC_LTS_DAYS);
    if (perfManLTSVal.isNull() || perfManLTSVal.toInt() == 0)
	perfManLTSVal = 42;
    perfManStart = new QLineEdit(perfManStartVal.toString(),this);
    perfManStart->setValidator(perfManStartValidator);
    perfManSTSavg = new QLineEdit(perfManSTSVal.toString(),this);
    perfManSTSavg->setValidator(perfManSTSavgValidator);
    perfManLTSavg = new QLineEdit(perfManLTSVal.toString(),this);
    perfManLTSavg->setValidator(perfManLTSavgValidator);

    // performance manager
    perfManLayout = new QVBoxLayout(); // outer
    perfManStartValLayout = new QHBoxLayout();
    perfManSTSavgLayout = new QHBoxLayout();
    perfManLTSavgLayout = new QHBoxLayout();
    perfManStartValLayout->addWidget(perfManStartLabel);
    perfManStartValLayout->addWidget(perfManStart);
    perfManSTSavgLayout->addWidget(perfManSTSLabel);
    perfManSTSavgLayout->addWidget(perfManSTSavg);
    perfManLTSavgLayout->addWidget(perfManLTSLabel);
    perfManLTSavgLayout->addWidget(perfManLTSavg);
    perfManLayout->addWidget(showSBToday);
    perfManLayout->addLayout(perfManStartValLayout);
    perfManLayout->addLayout(perfManSTSavgLayout);
    perfManLayout->addLayout(perfManLTSavgLayout);
    perfManLayout->addStretch();

    pmLayout->addLayout(perfManLayout);

    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabs);
    setLayout(mainLayout);
}

void
CyclistPage::saveClicked()
{
    // save zone config (other stuff is saved by configdialog)
    zonePage->saveClicked();
    hrZonePage->saveClicked();
    riderPage->saveClicked();
    passPage->saveClicked();
}

//
// Proxy settings page
//
ProxyPage::ProxyPage(QWidget *parent, MainWindow *mainWindow) : QWidget(parent), mainWindow(mainWindow)
{
    QVBoxLayout *main = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout;

    QLabel *typeLabel = new QLabel(tr("Proxy Type"));
    QLabel *hostLabel = new QLabel(tr("Hostname"));
    QLabel *portLabel = new QLabel(tr("Port"));
    QLabel *userLabel = new QLabel(tr("Username"));
    QLabel *passLabel = new QLabel(tr("Password"));

    pxType = new QComboBox(this);
    pxType->addItem(tr("Direct"));
    pxType->addItem(tr("Open"));
    pxType->addItem(tr("Authenticated"));
    pxType->setCurrentIndex(appsettings->value(this, GC_PROXYTYPE, 0).toInt());

    pxHost = new QLineEdit(this);
    pxHost->setText(appsettings->value(this, GC_PROXYHOST, "").toString());

    pxPort = new QSpinBox(this);
    pxPort->setMaximum(99999);
    pxPort->setMinimum(0);
    pxPort->setValue(appsettings->value(this, GC_PROXYPORT, 0).toInt());

    pxUser = new QLineEdit(this);
    pxUser->setText(appsettings->value(this, GC_PROXYUSER, "").toString());

    pxPass = new QLineEdit(this);
    pxPass->setEchoMode(QLineEdit::Password);
    pxPass->setText(appsettings->value(this, GC_PROXYPASS, "").toString());

    grid->addWidget(typeLabel, 0,0);
    grid->addWidget(hostLabel, 1,0);
    grid->addWidget(portLabel, 2,0);
    grid->addWidget(userLabel, 3,0);
    grid->addWidget(passLabel, 4,0);

    grid->addWidget(pxType, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(pxHost, 1, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(pxPort, 2, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(pxUser, 3, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(pxPass, 4, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->setColumnStretch(0,0);
    grid->setColumnStretch(1,3);

    main->addLayout(grid);
    main->addStretch();

    typeSelected(); // enable/disable as neccessary

    connect(pxType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeSelected()));
}

void ProxyPage::typeSelected()
{
    pxUser->setEnabled(false);
    pxPass->setEnabled(false);
    pxHost->setEnabled(false);
    pxPort->setEnabled(false);
    switch (pxType->currentIndex())
    {
    case 2 : // all fields enabled
        pxUser->setEnabled(true);
        pxPass->setEnabled(true);
    case 1 : // only host and port
        pxHost->setEnabled(true);
        pxPort->setEnabled(true);
    case 0 :
        break;
    }
}

void ProxyPage::saveClicked()
{
    appsettings->setValue(GC_PROXYTYPE, pxType->currentIndex());
    appsettings->setValue(GC_PROXYHOST, pxHost->text());
    appsettings->setValue(GC_PROXYPORT, pxPort->value());
    appsettings->setValue(GC_PROXYUSER, pxUser->text());
    appsettings->setValue(GC_PROXYPASS, pxPass->text());
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

    QLabel *tp = new QLabel(tr("Training Peaks"));
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

    QLabel *wip = new QLabel(tr("Withings Wifi Scales"));
    wip->setFont(current);

    QLabel *wiurlLabel = new QLabel(tr("Website"));
    QLabel *wiuserLabel = new QLabel(tr("User Id"));
    QLabel *wipassLabel = new QLabel(tr("Public Key"));

    QLabel *webcal = new QLabel(tr("Web Calendar"));
    webcal->setFont(current);

    QLabel *wcurlLabel = new QLabel(tr("Webcal URL"));

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

    wiURL = new QLineEdit(this);
    wiURL->setText(appsettings->cvalue(mainWindow->cyclist, GC_WIURL, "http://wbsapi.withings.net/").toString());

    wiUser = new QLineEdit(this);
    wiUser->setText(appsettings->cvalue(mainWindow->cyclist, GC_WIUSER, "").toString());

    wiPass = new QLineEdit(this);
    wiPass->setText(appsettings->cvalue(mainWindow->cyclist, GC_WIKEY, "").toString());

    webcalURL = new QLineEdit(this);
    webcalURL->setText(appsettings->cvalue(mainWindow->cyclist, GC_WEBCAL_URL, "").toString());

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
    grid->addWidget(wip, 13,0);
    grid->addWidget(wiurlLabel, 14,0);
    grid->addWidget(wiuserLabel, 15,0);
    grid->addWidget(wipassLabel, 16,0);
    grid->addWidget(webcal, 17, 0);
    grid->addWidget(wcurlLabel, 18, 0);

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

    grid->addWidget(wiURL, 14, 1, 0);
    grid->addWidget(wiUser, 15, 1, Qt::AlignLeft | Qt::AlignVCenter);
    grid->addWidget(wiPass, 16, 1, Qt::AlignLeft | Qt::AlignVCenter);

    grid->addWidget(webcalURL, 18, 1, 0);

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
    appsettings->setCValue(mainWindow->cyclist, GC_TPTYPE, tpType->currentIndex());
    appsettings->setCValue(mainWindow->cyclist, GC_TWURL, twitterURL->text());
    appsettings->setCValue(mainWindow->cyclist, GC_WIURL, wiURL->text());
    appsettings->setCValue(mainWindow->cyclist, GC_WIUSER, wiUser->text());
    appsettings->setCValue(mainWindow->cyclist, GC_WIKEY, wiPass->text());
    appsettings->setCValue(mainWindow->cyclist, GC_WEBCAL_URL, webcalURL->text());
    saveTwitter(); // get secret key if PIN set
}

//
// About me
//
RiderPage::RiderPage(QWidget *parent, MainWindow *mainWindow) : QWidget(parent), mainWindow(mainWindow)
{
    useMetricUnits = (appsettings->value(this, GC_UNIT).toString() == "Metric");

    QVBoxLayout *all = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout;

    QLabel *nicklabel = new QLabel(tr("Nickname"));
    QLabel *doblabel = new QLabel(tr("Date of Birth"));
    QLabel *sexlabel = new QLabel(tr("Sex"));
    QLabel *biolabel = new QLabel(tr("Bio"));

    QString weighttext = QString(tr("Weight (%1)")).arg(useMetricUnits ? tr("kg") : tr("lb"));
    QLabel *weightlabel = new QLabel(weighttext);

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

    weight = new QDoubleSpinBox(this);
    weight->setMaximum(999.9);
    weight->setMinimum(0.0);
    weight->setDecimals(1);
    weight->setValue(appsettings->cvalue(mainWindow->cyclist, GC_WEIGHT).toDouble() * (useMetricUnits ? 1.0 : LB_PER_KG));

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
    grid->addWidget(weightlabel, 3, 0, alignment);
    grid->addWidget(biolabel, 4, 0, alignment);

    grid->addWidget(nickname, 0, 1, alignment);
    grid->addWidget(dob, 1, 1, alignment);
    grid->addWidget(sex, 2, 1, alignment);
    grid->addWidget(weight, 3, 1, alignment);
    grid->addWidget(bio, 5, 0, 1, 4);

    grid->addWidget(avatarButton, 0, 2, 4, 2, Qt::AlignRight|Qt::AlignVCenter);
    all->addLayout(grid);
    all->addStretch();

    connect (avatarButton, SIGNAL(clicked()), this, SLOT(chooseAvatar()));
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
RiderPage::saveClicked()
{
    appsettings->setCValue(mainWindow->cyclist, GC_NICKNAME, nickname->text());
    appsettings->setCValue(mainWindow->cyclist, GC_DOB, dob->date());
    appsettings->setCValue(mainWindow->cyclist, GC_WEIGHT, weight->value() * (useMetricUnits ? 1.0 : KG_PER_LB));
    appsettings->setCValue(mainWindow->cyclist, GC_SEX, sex->currentIndex());
    appsettings->setCValue(mainWindow->cyclist, GC_BIO, bio->toPlainText());
    avatar.save(mainWindow->home.absolutePath() + "/" + "avatar.png", "PNG");
}

//
// Realtime devices page
//
DevicePage::DevicePage(QWidget *parent) : QWidget(parent)
{
    QTabWidget *tabs = new QTabWidget(this);
    QWidget *devs = new QWidget(this);
    tabs->addTab(devs, tr("Devices"));
    QVBoxLayout *devLayout = new QVBoxLayout(devs);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabs);

    DeviceTypes all;
    devices = all.getList();

    nameLabel = new QLabel(tr("Device Name"),this);
    deviceName = new QLineEdit(tr(""), this);

    typeLabel = new QLabel(tr("Device Type"),this);
    typeSelector = new QComboBox(this);
    typeSelector->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

    for (int i=0; i< devices.count(); i++) {
        DeviceType cur = devices.at(i);

        // WARNING: cur.type is what is stored in configuration
        //          do not change this!!
        typeSelector->addItem(cur.name, cur.type);
    }

    specLabel = new QLabel(tr("Device Port"),this);
    specHint = new QLabel();
    profHint = new QLabel();
    deviceSpecifier= new QLineEdit(tr(""), this);

    profLabel = new QLabel(tr("Device Profile"),this);
    deviceProfile = new QLineEdit(tr(""), this);

// THIS CODE IS DISABLED FOR THIS RELEASE XXX
//    isDefaultDownload = new QCheckBox(tr("Default download device"), this);
//    isDefaultRealtime = new QCheckBox(tr("Default realtime device"), this);

    addButton = new QPushButton(tr("Add"),this);
    delButton = new QPushButton(tr("Delete"),this);
    pairButton = new QPushButton(tr("Pair"),this);

    deviceList = new QTableView(this);

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
    deviceList->setColumnWidth(0,130);
    deviceList->setColumnWidth(1,130);
    deviceList->setColumnWidth(2,130);

    leftLayout = new QGridLayout();
    rightLayout = new QVBoxLayout();
    inLayout = new QGridLayout();

    leftLayout->addWidget(nameLabel, 0,0);
    leftLayout->addWidget(deviceName, 0,2);
    //leftLayout->setRowMinimumHeight(1,10);
    leftLayout->addWidget(typeLabel, 1,0);
    leftLayout->addWidget(typeSelector, 1,2);
    QHBoxLayout *squeeze = new QHBoxLayout;
    squeeze->addStretch();
    leftLayout->addLayout(squeeze, 1,3);
    //leftLayout->setRowMinimumHeight(3,10);
    leftLayout->addWidget(specHint, 2,2);
    leftLayout->addWidget(specLabel, 3,0);
    leftLayout->addWidget(deviceSpecifier, 3,2);
    //leftLayout->setRowMinimumHeight(6,10);
    leftLayout->addWidget(profHint, 4,2);
    leftLayout->addWidget(profLabel, 5,0);
    leftLayout->addWidget(deviceProfile, 5,2);
    leftLayout->setColumnMinimumWidth(1,10);

// THIS CODE IS DISABLED FOR THIS RELEASE XXX
//    leftLayout->addWidget(isDefaultDownload, 6,1);
//    leftLayout->addWidget(isDefaultRealtime, 8,1);

//    leftLayout->setRowStretch(0, 2);
//    leftLayout->setRowStretch(1, 1);
//    leftLayout->setRowStretch(2, 2);
//    leftLayout->setRowStretch(3, 1);
//    leftLayout->setRowStretch(4, 2);
//    leftLayout->setRowStretch(5, 1);
//    leftLayout->setRowStretch(6, 2);
//    leftLayout->setRowStretch(7, 1);
//    leftLayout->setRowStretch(8, 2);

    rightLayout->addWidget(addButton);
    rightLayout->addSpacing(10);
    rightLayout->addWidget(delButton);
    rightLayout->addStretch();
    rightLayout->addWidget(pairButton);

    inLayout->addItem(leftLayout, 0,0);
    inLayout->addItem(rightLayout, 0,1);

    devLayout->addLayout(inLayout);
    devLayout->addWidget(deviceList);
    devLayout->setStretch(0,0);
    devLayout->setStretch(1,99);

    // to make sure the default checkboxes have been set appropiately...
    // THIS CODE IS DISABLED IN THIS RELEASE XXX
    // isDefaultRealtime->setEnabled(false);

    setConfigPane();
}

void
DevicePage::setConfigPane()
{
    // depending upon the type of device selected
    // the spec hint tells the user the format they should use
    DeviceTypes Supported;

    // sorry... ;-) obfuscated c++ contest winner 2009
    switch (Supported.getType(typeSelector->itemData(typeSelector->currentIndex()).toInt()).connector) {

    case DEV_ANT:
        specHint->setText("hostname:port");
        profHint->setText("antid 1, antid 2 ...");
        profHint->show();
        pairButton->show();
        profLabel->show();
        deviceProfile->show();
        break;
    case DEV_SERIAL:
#ifdef WIN32
        specHint->setText("COMx");
#else
        specHint->setText("/dev/xxxx");
#endif
        pairButton->hide();
        profHint->hide();
        profLabel->hide();
        deviceProfile->hide();
        break;
    case DEV_TCP:
        specHint->setText("hostname:port");
        pairButton->hide();
        profHint->hide();
        profLabel->hide();
        deviceProfile->hide();
        break;
    }
    //specHint->setTextFormat(Qt::Italic); // mmm need to read the docos
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

void
DevicePage::pairClicked(DeviceConfiguration *dc, QProgressDialog *progress)
{
    ANTplusController ANTplus(0, dc);
    ANTplus.discover(dc, progress);
    deviceProfile->setText(dc->deviceProfile);
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
    return 4;
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
    colors->headerItem()->setText(0, "Color");
    colors->headerItem()->setText(1, "Select");
    colors->setColumnCount(2);
    colors->setSelectionMode(QAbstractItemView::NoSelection);
    //colors->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    colors->setUniformRowHeights(true);
    colors->setIndentation(0);
    colors->header()->resizeSection(0,300);

    shadeZones = new QCheckBox;
    shadeZones->setChecked(appsettings->value(this, GC_SHADEZONES, true).toBool());
    antiAliased = new QCheckBox;
    antiAliased->setChecked(appsettings->value(this, GC_ANTIALIAS, false).toBool());
    lineWidth = new QDoubleSpinBox;
    lineWidth->setMaximum(5);
    lineWidth->setMinimum(0.5);
    lineWidth->setSingleStep(0.5);
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

    grid->addWidget(lineWidthLabel, 0,3);
    grid->addWidget(antialiasLabel, 1,3);
    grid->addWidget(shadeZonesLabel, 2,3);
    grid->addWidget(lineWidth, 0,4, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(antiAliased, 1,4, Qt::AlignVCenter|Qt::AlignLeft);
    grid->addWidget(shadeZones, 2,4, Qt::AlignVCenter|Qt::AlignLeft);

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
    mainLayout->addWidget(colors);

    colorSet = GCColor::colorSet();
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
    upButton = new QPushButton("Move up");
    downButton = new QPushButton("Move down");
    leftButton = new QPushButton("Exclude");
    rightButton = new QPushButton("Include");
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

//
// Metadata page
//
MetadataPage::MetadataPage(MainWindow *main) : main(main)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // get current config using default file
    keywordDefinitions = main->rideMetadata()->getKeywords();
    fieldDefinitions = main->rideMetadata()->getFields();

    // setup maintenance pages using current config
    fieldsPage = new FieldsPage(this, fieldDefinitions);
    keywordsPage = new KeywordsPage(this, keywordDefinitions);
    processorPage = new ProcessorPage(main);

    tabs = new QTabWidget(this);
    tabs->addTab(fieldsPage, tr("Fields"));
    tabs->addTab(keywordsPage, tr("Notes Keywords"));
    tabs->addTab(processorPage, tr("Processing"));

    layout->addWidget(tabs);
}

void
MetadataPage::saveClicked()
{
    // get current state
    fieldsPage->getDefinitions(fieldDefinitions);
    keywordsPage->getDefinitions(keywordDefinitions);

    // write to metadata.xml
    RideMetadata::serialize(main->home.absolutePath() + "/metadata.xml", keywordDefinitions, fieldDefinitions);

    // save processors config
    processorPage->saveClicked();
}

// little helper since we create/recreate combos
// for field types all over the place (init, move up, move down)
static void addFieldTypes(QComboBox *p)
{
    p->addItem("Text");
    p->addItem("Textbox");
    p->addItem("ShortText");
    p->addItem("Integer");
    p->addItem("Double");
    p->addItem("Date");
    p->addItem("Time");
}

//
// Calendar coloring page
//
KeywordsPage::KeywordsPage(QWidget *parent, QList<KeywordDefinition>keywordDefinitions) : QWidget(parent)
{
    QGridLayout *mainLayout = new QGridLayout(this);

    upButton = new QPushButton(tr("Move up"));
    downButton = new QPushButton(tr("Move down"));
    addButton = new QPushButton(tr("Insert"));
    renameButton = new QPushButton(tr("Rename"));
    deleteButton = new QPushButton(tr("Delete"));

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(renameButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();

    keywords = new QTreeWidget;
    keywords->headerItem()->setText(0, "Keyword");
    keywords->headerItem()->setText(1, "Color");
    keywords->headerItem()->setText(2, "Related Notes Words");
    keywords->setColumnCount(3);
    keywords->setSelectionMode(QAbstractItemView::SingleSelection);
    keywords->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    keywords->setUniformRowHeights(true);
    keywords->setIndentation(0);
    keywords->header()->resizeSection(0,100);
    keywords->header()->resizeSection(1,45);

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

    mainLayout->addWidget(keywords, 0,0);
    mainLayout->addLayout(actionButtons, 0,1);

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(renameButton, SIGNAL(clicked()), this, SLOT(renameClicked()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
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
    ColorButton *colorButton = new ColorButton(this, "New", QColor(Qt::blue));
    add = new QTreeWidgetItem;
    keywords->invisibleRootItem()->insertChild(index, add);
    add->setFlags(add->flags() | Qt::ItemIsEditable);

    // keyword
    QString text = "New";
    for (int i=0; keywords->findItems(text, Qt::MatchExactly, 0).count() > 0; i++) {
        text = QString("New (%1)").arg(i+1);
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

    upButton = new QPushButton(tr("Move up"));
    downButton = new QPushButton(tr("Move down"));
    addButton = new QPushButton(tr("Insert"));
    renameButton = new QPushButton(tr("Rename"));
    deleteButton = new QPushButton(tr("Delete"));

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(renameButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();

    fields = new QTreeWidget;
    fields->headerItem()->setText(0, tr("Screen Tab"));
    fields->headerItem()->setText(1, tr("Field"));
    fields->headerItem()->setText(2, tr("Type"));
    fields->headerItem()->setText(3, tr("Diary"));
    fields->setColumnCount(4);
    fields->setSelectionMode(QAbstractItemView::SingleSelection);
    fields->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    fields->setUniformRowHeights(true);
    fields->setIndentation(0);

#ifdef Q_OS_MAC
    fields->header()->resizeSection(0,100);
    fields->header()->resizeSection(1,110);
    fields->header()->resizeSection(2,110);
    fields->header()->resizeSection(3,50);
#else
    fields->header()->resizeSection(0,120);
    fields->header()->resizeSection(1,130);
    fields->header()->resizeSection(2,150);
    fields->header()->resizeSection(3,50);
#endif

    SpecialFields specials;
    foreach(FieldDefinition field, fieldDefinitions) {
        QTreeWidgetItem *add;
        QComboBox *comboButton = new QComboBox(this);
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(field.diary);
        //QLineEdit *linedit = new QLineEdit(this); //XXX need a custom delegate for this
        //QCompleter *completer = new QCompleter(linedit);
        //completer->setModel(specials.model());
        //completer->setCaseSensitivity(Qt::CaseInsensitive);
        //linedit->setCompleter(completer);

        addFieldTypes(comboButton);
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
        fields->setItemWidget(add, 3, checkBox);
    }
    fields->setCurrentItem(fields->invisibleRootItem()->child(0));

    mainLayout->addWidget(fields, 0,0);
    mainLayout->addLayout(actionButtons, 0,1);

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(renameButton, SIGNAL(clicked()), this, SLOT(renameClicked()));
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
        QWidget *check = fields->itemWidget(fields->currentItem(),3);
        QComboBox *comboButton = new QComboBox(this);
        addFieldTypes(comboButton);
        comboButton->setCurrentIndex(((QComboBox*)button)->currentIndex());
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(((QCheckBox*)check)->isChecked());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index-1, moved);
        fields->setItemWidget(moved, 2, comboButton);
        fields->setItemWidget(moved, 3, checkBox);
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
        QWidget *check = fields->itemWidget(fields->currentItem(),3);
        QComboBox *comboButton = new QComboBox(this);
        addFieldTypes(comboButton);
        comboButton->setCurrentIndex(((QComboBox*)button)->currentIndex());
        QCheckBox *checkBox = new QCheckBox("", this);
        checkBox->setChecked(((QCheckBox*)check)->isChecked());
        QTreeWidgetItem* moved = fields->invisibleRootItem()->takeChild(index);
        fields->invisibleRootItem()->insertChild(index+1, moved);
        fields->setItemWidget(moved, 2, comboButton);
        fields->setItemWidget(moved, 3, checkBox);
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
    fields->setItemWidget(add, 3, checkBox);
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
        add.diary = ((QCheckBox*)fields->itemWidget(item, 3))->isChecked();

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
    processorTree->header()->resizeSection(0,150);

    // iterate over all the processors and add an entry to the
    QMapIterator<QString, DataProcessor*> i(processors);
    i.toFront();
    while (i.hasNext()) {
        i.next();

        QTreeWidgetItem *add;

        add = new QTreeWidgetItem(processorTree->invisibleRootItem());
        add->setFlags(add->flags() & ~Qt::ItemIsEditable);

        // Processor Name
        add->setText(0, i.key());

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
    tabs->addTab(cpPage, tr("Critical Power History"));
    tabs->addTab(schemePage, tr("Default Zones"));

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
    QGridLayout *mainLayout = new QGridLayout(this);

    addButton = new QPushButton(tr("Add"));
    renameButton = new QPushButton(tr("Rename"));
    deleteButton = new QPushButton(tr("Delete"));

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(renameButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addStretch();

    scheme = new QTreeWidget;
    scheme->headerItem()->setText(0, tr("Short"));
    scheme->headerItem()->setText(1, tr("Long"));
    scheme->headerItem()->setText(2, tr("Percent of CP"));
    scheme->setColumnCount(3);
    scheme->setSelectionMode(QAbstractItemView::SingleSelection);
    scheme->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    scheme->setUniformRowHeights(true);
    scheme->setIndentation(0);
    scheme->header()->resizeSection(0,90);
    scheme->header()->resizeSection(1,200);
    scheme->header()->resizeSection(2,80);

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

    mainLayout->addWidget(scheme, 0,0);
    mainLayout->addLayout(actionButtons, 0,1);

    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(renameButton, SIGNAL(clicked()), this, SLOT(renameClicked()));
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

    addButton = new QPushButton(tr("Add CP"));
    deleteButton = new QPushButton(tr("Delete CP"));
    defaultButton = new QPushButton(tr("Default"));
    defaultButton->hide();

    addZoneButton = new QPushButton(tr("Add Zone"));
    deleteZoneButton = new QPushButton(tr("Delete Zone"));

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(defaultButton);
    actionButtons->addStretch();

    QVBoxLayout *zoneButtons = new QVBoxLayout;
    zoneButtons->addWidget(addZoneButton);
    zoneButtons->addWidget(deleteZoneButton);
    zoneButtons->addStretch();

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
    ranges->header()->resizeSection(0,180);

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
    zones->header()->resizeSection(0,80);
    zones->header()->resizeSection(1,150);

    mainLayout->addLayout(addLayout, 0,0);
    mainLayout->addWidget(ranges, 2,0);
    mainLayout->addWidget(zones, 4,0);
    mainLayout->addLayout(actionButtons, 0,1,0,3);
    mainLayout->addLayout(zoneButtons, 4,1);

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
    tabs->addTab(ltPage, tr("Lactic Threshold History"));
    tabs->addTab(schemePage, tr("Default Zones"));

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
    QGridLayout *mainLayout = new QGridLayout(this);

    addButton = new QPushButton(tr("Add"));
    renameButton = new QPushButton(tr("Rename"));
    deleteButton = new QPushButton(tr("Delete"));

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(renameButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addStretch();

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
    scheme->header()->resizeSection(0,60);
    scheme->header()->resizeSection(1,180);
    scheme->header()->resizeSection(2,65);
    scheme->header()->resizeSection(3,65);

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

    mainLayout->addWidget(scheme, 0,0);
    mainLayout->addLayout(actionButtons, 0,1);

    // button connect
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(renameButton, SIGNAL(clicked()), this, SLOT(renameClicked()));
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

    QGridLayout *mainLayout = new QGridLayout(this);

    addButton = new QPushButton(tr("Add LT"));
    deleteButton = new QPushButton(tr("Delete LT"));
    defaultButton = new QPushButton(tr("Default"));
    defaultButton->hide();

    addZoneButton = new QPushButton(tr("Add Zone"));
    deleteZoneButton = new QPushButton(tr("Delete Zone"));

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(defaultButton);
    actionButtons->addStretch();

    QVBoxLayout *zoneButtons = new QVBoxLayout;
    zoneButtons->addWidget(addZoneButton);
    zoneButtons->addWidget(deleteZoneButton);
    zoneButtons->addStretch();

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
    ranges->header()->resizeSection(0,180);

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
    zones->header()->resizeSection(0,50);
    zones->header()->resizeSection(1,150);
    zones->header()->resizeSection(2,65);
    zones->header()->resizeSection(3,65);

    mainLayout->addLayout(addLayout, 0,0);
    mainLayout->addLayout(addLayout2, 1,0);
    mainLayout->addWidget(ranges, 2,0);
    mainLayout->addWidget(zones, 4,0);
    mainLayout->addLayout(actionButtons, 0,1,0,3);
    mainLayout->addLayout(zoneButtons, 4,1);

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

    upButton = new QPushButton(tr("Move up"));
    downButton = new QPushButton(tr("Move down"));
    addButton = new QPushButton(tr("Insert"));
    renameButton = new QPushButton(tr("Rename"));
    deleteButton = new QPushButton(tr("Delete"));

    QVBoxLayout *actionButtons = new QVBoxLayout;
    actionButtons->addWidget(addButton);
    actionButtons->addWidget(renameButton);
    actionButtons->addWidget(deleteButton);
    actionButtons->addWidget(upButton);
    actionButtons->addWidget(downButton);
    actionButtons->addStretch();

    fields = new QTreeWidget;
    fields->headerItem()->setText(0, tr("Screen Tab"));
    fields->headerItem()->setText(1, tr("Measure"));
    fields->headerItem()->setText(2, tr("Type"));
    fields->setColumnCount(3);
    fields->setSelectionMode(QAbstractItemView::SingleSelection);
    fields->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    fields->setUniformRowHeights(true);
    fields->setIndentation(0);
    fields->header()->resizeSection(0,130);
    fields->header()->resizeSection(1,140);

    // temporary storage for measures config
    QList<FieldDefinition> fieldDefinitions;
    QList<KeywordDefinition> keywordDefinitions; //NOTE: not used in measures.xml

    // check we have one and use built in if not there
    QString filename = main->home.absolutePath()+"/measures.xml";
    if (!QFile(filename).exists()) filename = ":/xml/measures.xml";
    RideMetadata::readXML(filename, keywordDefinitions, fieldDefinitions);

    // iterate over the fields and setup the editable items
    foreach(FieldDefinition field, fieldDefinitions) {
        QTreeWidgetItem *add;
        QComboBox *comboButton = new QComboBox(this);

        addFieldTypes(comboButton);
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
    mainLayout->addLayout(actionButtons, 0,1);

    // connect up slots
    connect(upButton, SIGNAL(clicked()), this, SLOT(upClicked()));
    connect(downButton, SIGNAL(clicked()), this, SLOT(downClicked()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(addClicked()));
    connect(renameButton, SIGNAL(clicked()), this, SLOT(renameClicked()));
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
        addFieldTypes(comboButton);
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
        addFieldTypes(comboButton);
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
    addFieldTypes(comboButton);

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
    RideMetadata::serialize(main->home.absolutePath() + "/measures.xml", QList<KeywordDefinition>(), current);
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
