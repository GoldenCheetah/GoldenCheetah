#include <QtGui>
#include <QSettings>
#include <assert.h>

#include "MainWindow.h"
#include "ConfigDialog.h"
#include "RealtimeWindow.h"
#include "Pages.h"
#include "Settings.h"
#include "Zones.h"


/* cyclist dialog protocol redesign:
 * no zones:
 *    calendar disabled
 *    automatically go into "new" mode
 * zone(s) defined:
 *    click on calendar: sets current zone to that associated with date
 * save clicked:
 *    if new mode, create a new zone starting at selected date, or for all dates
 *    if this is only zone.
 * delete clicked:
 *    deletes currently selected zone
 */

ConfigDialog::ConfigDialog(QDir _home, Zones *_zones, MainWindow *mainWindow) :
    mainWindow(mainWindow), zones(_zones)
{
    setAttribute(Qt::WA_DeleteOnClose);

    home = _home;

    cyclistPage = new CyclistPage(mainWindow);

    contentsWidget = new QListWidget;
    contentsWidget->setViewMode(QListView::IconMode);
    contentsWidget->setIconSize(QSize(96, 84));
    contentsWidget->setMovement(QListView::Static);
    contentsWidget->setMinimumWidth(112);
    contentsWidget->setMaximumWidth(112);
    //contentsWidget->setMinimumHeight(200);
    contentsWidget->setSpacing(12);
    contentsWidget->setUniformItemSizes(true);

    configPage = new ConfigurationPage(mainWindow);
    devicePage = new DevicePage(this);
    pagesWidget = new QStackedWidget;
    pagesWidget->addWidget(configPage);
    pagesWidget->addWidget(cyclistPage);
    pagesWidget->addWidget(devicePage);
    #ifdef GC_HAVE_LIBOAUTH
    twitterPage = new TwitterPage(this);
    pagesWidget->addWidget(twitterPage);
    #endif

    closeButton = new QPushButton(tr("Close"));
    saveButton = new QPushButton(tr("Save"));

    createIcons();
    contentsWidget->setCurrentItem(contentsWidget->item(0));

    // connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));
    // connect(saveButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(accept()));

    // connect the pieces...
    connect(devicePage->typeSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(changedType(int)));
    connect(devicePage->addButton, SIGNAL(clicked()), this, SLOT(devaddClicked()));
    connect(devicePage->delButton, SIGNAL(clicked()), this, SLOT(devdelClicked()));
    connect(devicePage->pairButton, SIGNAL(clicked()), this, SLOT(devpairClicked()));

    horizontalLayout = new QHBoxLayout;
    horizontalLayout->addWidget(contentsWidget);
    horizontalLayout->addWidget(pagesWidget, 1);

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);
    buttonsLayout->addWidget(saveButton);

    mainLayout = new QVBoxLayout;
    mainLayout->addLayout(horizontalLayout);
    //mainLayout->addStretch(1);
    //mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    // We go fixed width to ensure a consistent layout for
    // tabs, sub-tabs and internal widgets and lists
#ifdef Q_OS_MACX
    setWindowTitle(tr("Preferences"));
#else
    setWindowTitle(tr("Options"));
#endif
    setFixedSize(QSize(800, 600));
}

void ConfigDialog::createIcons()
{
    QListWidgetItem *configButton = new QListWidgetItem(contentsWidget);
    configButton->setIcon(QIcon(":/images/config.png"));
    configButton->setText(tr("Settings"));
    configButton->setTextAlignment(Qt::AlignHCenter);
    configButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);


    QListWidgetItem *cyclistButton = new QListWidgetItem(contentsWidget);
    cyclistButton->setIcon(QIcon(":images/cyclist.png"));
    cyclistButton->setText(tr("Athlete"));
    cyclistButton->setTextAlignment(Qt::AlignHCenter);
    cyclistButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    QListWidgetItem *realtimeButton = new QListWidgetItem(contentsWidget);
    realtimeButton->setIcon(QIcon(":images/arduino.png"));
    realtimeButton->setText(tr("Devices"));
    realtimeButton->setTextAlignment(Qt::AlignHCenter);
    realtimeButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

#ifdef GC_HAVE_LIBOAUTH
    QListWidgetItem *twitterButton = new QListWidgetItem(contentsWidget);
    twitterButton->setIcon(QIcon(":images/twitter.png"));
    twitterButton->setText(tr("Twitter"));
    twitterButton->setTextAlignment(Qt::AlignHCenter);
    twitterButton->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
#endif

    connect(contentsWidget,
            SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));

    connect(saveButton, SIGNAL(clicked()), this, SLOT(save_Clicked()));
}


void ConfigDialog::changePage(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (!current)
        current = previous;

    pagesWidget->setCurrentIndex(contentsWidget->row(current));
}

// if save is clicked, we want to:
//   new mode:   create a new zone starting at the selected date (which may be null, implying BEGIN
//   ! new mode: change the CP associated with the present mode
void ConfigDialog::save_Clicked()
{
    boost::shared_ptr<QSettings> settings = GetApplicationSettings();

    if (configPage->langCombo->currentIndex()==0)
        settings->setValue(GC_LANG, "en");
    else if (configPage->langCombo->currentIndex()==1)
        settings->setValue(GC_LANG, "fr");
    else if (configPage->langCombo->currentIndex()==2)
        settings->setValue(GC_LANG, "ja");
    else if (configPage->langCombo->currentIndex()==3)
        settings->setValue(GC_LANG, "pt-br");
    else if (configPage->langCombo->currentIndex()==4)
        settings->setValue(GC_LANG, "it");
    else if (configPage->langCombo->currentIndex()==5)
        settings->setValue(GC_LANG, "de");
    else if (configPage->langCombo->currentIndex()==6)
        settings->setValue(GC_LANG, "ru");
    else if (configPage->langCombo->currentIndex()==7)
        settings->setValue(GC_LANG, "cs");
    else if (configPage->langCombo->currentIndex()==8)
        settings->setValue(GC_LANG, "es");

    if (configPage->unitCombo->currentIndex()==0)
        settings->setValue(GC_UNIT, "Metric");
    else if (configPage->unitCombo->currentIndex()==1)
        settings->setValue(GC_UNIT, "Imperial");

    settings->setValue(GC_ALLRIDES_ASCENDING, configPage->allRidesAscending->checkState());
    settings->setValue(GC_GARMIN_SMARTRECORD, configPage->garminSmartRecord->checkState());
    settings->setValue(GC_GARMIN_HWMARK, configPage->garminHWMarkedit->text());
    settings->setValue(GC_MAP_INTERVAL, configPage->mapIntervaledit->text());
    settings->setValue(GC_CRANKLENGTH, configPage->crankLengthCombo->currentText());
    settings->setValue(GC_BIKESCOREDAYS, configPage->BSdaysEdit->text());
    settings->setValue(GC_BIKESCOREMODE, configPage->bsModeCombo->currentText());
    settings->setValue(GC_WORKOUTDIR, configPage->workoutDirectory->text());
    settings->setValue(GC_INITIAL_STS, cyclistPage->perfManStart->text());
    settings->setValue(GC_INITIAL_LTS, cyclistPage->perfManStart->text());
    settings->setValue(GC_STS_DAYS, cyclistPage->perfManSTSavg->text());
    settings->setValue(GC_LTS_DAYS, cyclistPage->perfManLTSavg->text());
    settings->setValue(GC_SB_TODAY, (int) cyclistPage->showSBToday->isChecked());
    settings->setValue(GC_PM_DAYS, cyclistPage->perfManDays->text());

    // set default stress names if not set:
    settings->setValue(GC_STS_NAME, settings->value(GC_STS_NAME,tr("Short Term Stress")));
    settings->setValue(GC_STS_ACRONYM, settings->value(GC_STS_ACRONYM,tr("STS")));
    settings->setValue(GC_LTS_NAME, settings->value(GC_LTS_NAME,tr("Long Term Stress")));
    settings->setValue(GC_LTS_ACRONYM, settings->value(GC_LTS_ACRONYM,tr("LTS")));
    settings->setValue(GC_SB_NAME, settings->value(GC_SB_NAME,tr("Stress Balance")));
    settings->setValue(GC_SB_ACRONYM, settings->value(GC_SB_ACRONYM,tr("SB")));

    // Save Cyclist page stuff
    cyclistPage->saveClicked();

    // save interval metrics and ride data pages
    configPage->saveClicked();

#ifdef GC_HAVE_LIBOAUTH
    //Call Twitter Save Dialog to get Access Token
    twitterPage->saveClicked();
#endif
    // Save the device configuration...
    DeviceConfigurations all;
    all.writeConfig(devicePage->deviceListModel->Configuration);

    // Tell MainWindow we changed config, so it can emit the signal
    // configChanged() to all its children
    mainWindow->notifyConfigChanged();

    // close
    accept();
}

//
// DEVICE CONFIG STUFF
//

void
ConfigDialog::changedType(int)
{
// THIS CODE IS DISABLED FOR THIS RELEASE XXX
//    // disable/enable default checkboxes
//    if (devicePage->devices.at(index).download == false) {
//        devicePage->isDefaultDownload->setEnabled(false);
//        devicePage->isDefaultDownload->setCheckState(Qt::Unchecked);
//    } else {
//        devicePage->isDefaultDownload->setEnabled(true);
//    }
//    if (devicePage->devices.at(index).realtime == false) {
//        devicePage->isDefaultRealtime->setEnabled(false);
//        devicePage->isDefaultRealtime->setCheckState(Qt::Unchecked);
//    } else {
//        devicePage->isDefaultRealtime->setEnabled(true);
//    }
    devicePage->setConfigPane();
}

void
ConfigDialog::devaddClicked()
{
    DeviceConfiguration add;
    DeviceTypes Supported;

    // get values from the gui elements
    add.name = devicePage->deviceName->displayText();
    add.type = devicePage->typeSelector->itemData(devicePage->typeSelector->currentIndex()).toInt();
    add.portSpec = devicePage->deviceSpecifier->displayText();
    add.deviceProfile = devicePage->deviceProfile->displayText();
    add.postProcess = devicePage->virtualPower->currentIndex();

    // NOT IMPLEMENTED IN THIS RELEASE XXX
    //add.isDefaultDownload = devicePage->isDefaultDownload->isChecked() ? true : false;
    //add.isDefaultRealtime = devicePage->isDefaultDownload->isChecked() ? true : false;

    // Validate the name
    QRegExp nameSpec(".+");
    if (nameSpec.exactMatch(add.name) == false) {
        QMessageBox::critical(0, "Invalid Device Name",
                QString("Device Name should be non-blank"));
        return ;
    }

    // Validate the portSpec
    QRegExp antSpec("[^:]*:[^:]*");         // ip:port same as TCP ... for now...
    QRegExp tcpSpec("[^:]*:[^:]*");         // ip:port
#ifdef WIN32
    QRegExp serialSpec("COM[0-9]*");      // COMx for WIN32, /dev/* for others
#else
    QRegExp serialSpec("/dev/.*");      // COMx for WIN32, /dev/* for others
#endif

    // check the portSpec is valid, based upon the connection type
    switch (Supported.getType(add.type).connector) {
        case DEV_ANT :
            if (antSpec.exactMatch(add.portSpec) == false) {
                QMessageBox::critical(0, "Invalid Port Specification",
                QString("For ANT devices the specifier must be ") +
                        "hostname:portnumber");
                return ;
            }
            break;
        case DEV_SERIAL :
            if (serialSpec.exactMatch(add.portSpec) == false) {
                QMessageBox::critical(0, "Invalid Port Specification",
                QString("For Serial devices the specifier must be ") +
#ifdef WIN32
                        "COMn"
#else
                        "/dev/xxxxx"
#endif
                );
                return ;
            }
            break;
        case DEV_TCP :
            if (tcpSpec.exactMatch(add.portSpec) == false) {
                QMessageBox::critical(0, "Invalid Port Specification",
                QString("For TCP streaming devices the specifier must be ") +
                        "hostname:portnumber");
                return ;
            }
            break;
    }

    devicePage->deviceListModel->add(add);
}

void
ConfigDialog::devdelClicked()
{
    devicePage->deviceListModel->del();
}

void
ConfigDialog::devpairClicked()
{
    DeviceConfiguration add;

    // get values from the gui elements
    add.name = devicePage->deviceName->displayText();
    add.type = devicePage->typeSelector->itemData(devicePage->typeSelector->currentIndex()).toInt();
    add.portSpec = devicePage->deviceSpecifier->displayText();
    add.deviceProfile = devicePage->deviceProfile->displayText();

    QProgressDialog *progress = new QProgressDialog("Looking for Devices...", "Abort Scan", 0, 200, this);
    progress->setWindowModality(Qt::WindowModal);

    devicePage->pairClicked(&add, progress);
}
