#include <QtGui>
#include <QSettings>
#include <assert.h>

#include "MainWindow.h"
#include "ConfigDialog.h"
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

    closeButton = new QPushButton(tr("Close"));
    saveButton = new QPushButton(tr("Save"));

    createIcons();
    contentsWidget->setCurrentItem(contentsWidget->item(0));

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

    fortiusFirmware = appsettings->value(this, FORTIUS_FIRMWARE, "").toString();
    connect(closeButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(devicePage->typeSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(changedType(int)));
    connect(devicePage->addButton, SIGNAL(clicked()), this, SLOT(devaddClicked()));
    connect(devicePage->firmwareButton, SIGNAL(clicked()), this, SLOT(firmwareClicked()));
    connect(devicePage->delButton, SIGNAL(clicked()), this, SLOT(devdelClicked()));
    connect(devicePage->pairButton, SIGNAL(clicked()), this, SLOT(devpairClicked()));
    connect(contentsWidget, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
    connect(saveButton, SIGNAL(clicked()), this, SLOT(save_Clicked()));
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

    connect(contentsWidget,
            SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)),
            this, SLOT(changePage(QListWidgetItem *, QListWidgetItem*)));
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
    if (configPage->langCombo->currentIndex()==0)
        appsettings->setValue(GC_LANG, "en");
    else if (configPage->langCombo->currentIndex()==1)
        appsettings->setValue(GC_LANG, "fr");
    else if (configPage->langCombo->currentIndex()==2)
        appsettings->setValue(GC_LANG, "ja");
    else if (configPage->langCombo->currentIndex()==3)
        appsettings->setValue(GC_LANG, "pt-br");
    else if (configPage->langCombo->currentIndex()==4)
        appsettings->setValue(GC_LANG, "it");
    else if (configPage->langCombo->currentIndex()==5)
        appsettings->setValue(GC_LANG, "de");
    else if (configPage->langCombo->currentIndex()==6)
        appsettings->setValue(GC_LANG, "ru");
    else if (configPage->langCombo->currentIndex()==7)
        appsettings->setValue(GC_LANG, "cs");

    if (configPage->unitCombo->currentIndex()==0)
        appsettings->setValue(GC_UNIT, "Metric");
    else if (configPage->unitCombo->currentIndex()==1)
        appsettings->setValue(GC_UNIT, "Imperial");

    appsettings->setValue(GC_GARMIN_HWMARK, configPage->garminHWMarkedit->text().toInt());
    appsettings->setValue(GC_GARMIN_SMARTRECORD, configPage->garminSmartRecord->checkState());
    appsettings->setValue(GC_CRANKLENGTH, configPage->crankLengthCombo->currentText());
    appsettings->setValue(GC_BIKESCOREDAYS, configPage->BSdaysEdit->text());
    appsettings->setValue(GC_BIKESCOREMODE, configPage->bsModeCombo->currentText());
    appsettings->setValue(GC_WORKOUTDIR, configPage->workoutDirectory->text());
    appsettings->setCValue(mainWindow->cyclist, GC_INITIAL_STS, cyclistPage->perfManStart->text());
    appsettings->setCValue(mainWindow->cyclist, GC_INITIAL_LTS, cyclistPage->perfManStart->text());
    appsettings->setCValue(mainWindow->cyclist, GC_STS_DAYS, cyclistPage->perfManSTSavg->text());
    appsettings->setCValue(mainWindow->cyclist, GC_LTS_DAYS, cyclistPage->perfManLTSavg->text());
    appsettings->setCValue(mainWindow->cyclist, GC_SB_TODAY, (int) cyclistPage->showSBToday->isChecked());

    // set default stress names if not set:
    appsettings->setValue(GC_STS_NAME, appsettings->value(this, GC_STS_NAME,tr("Short Term Stress")));
    appsettings->setValue(GC_STS_ACRONYM, appsettings->value(this, GC_STS_ACRONYM,tr("STS")));
    appsettings->setValue(GC_LTS_NAME, appsettings->value(this, GC_LTS_NAME,tr("Long Term Stress")));
    appsettings->setValue(GC_LTS_ACRONYM, appsettings->value(this, GC_LTS_ACRONYM,tr("LTS")));
    appsettings->setValue(GC_SB_NAME, appsettings->value(this, GC_SB_NAME,tr("Stress Balance")));
    appsettings->setValue(GC_SB_ACRONYM, appsettings->value(this, GC_SB_ACRONYM,tr("SB")));

    // Save Cyclist page stuff
    cyclistPage->saveClicked();

    // save interval metrics and ride data pages
    configPage->saveClicked();

    // Save the device configuration...
    DeviceConfigurations all;
    all.writeConfig(devicePage->deviceListModel->Configuration);
    appsettings->setValue(FORTIUS_FIRMWARE, fortiusFirmware);
    appsettings->setValue(TRAIN_MULTI, devicePage->multiCheck->isChecked());

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
        case DEV_QUARQ :
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

        case DEV_LIBUSB :

            // Is the fortius firmware configured?
            if (add.type == DEV_FORTIUS && fortiusFirmware == "") {
                QMessageBox::critical(0, "Fortius Firmware",
                QString("For Fortius devices you must import the firmware ") +
                        "using the Firmware button.");
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

void
ConfigDialog::firmwareClicked()
{
    // we need to set the firmware for this device
    QString current = fortiusFirmware;

    FortiusDialog *p = new FortiusDialog(mainWindow, fortiusFirmware);
    p->exec();
}

//
// Choose the firmware to use with a Fortius trainer
//
FortiusDialog::FortiusDialog(MainWindow *mainWindow, QString &path) : path(path), mainWindow(mainWindow)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowModality(Qt::WindowModal);
    setWindowTitle("Select Firmware File");

    // create widgets
    browse = new QPushButton("Browse", this);
    cancel = new QPushButton("Cancel", this);
    ok = new QPushButton("OK", this);
    copy = new QCheckBox("Copy to Library");
    copy->setChecked(true);

    help = new QLabel(this);
    help->setWordWrap(true);
    help->setText("Tacx Fortius trainers require a firmware file "
                  "which is provided by Tacx BV. This file is a "
                  "copyrighted file and cannot be distributed with "
                  "GoldenCheetah.\n\n"
                  "On windows it is typically installed in C:\\Windows\\system32 "
                  "and is called 'FortiusSWPID1942Renum.hex'.\n\n"
                  "On Linux and Apple computers you will need to "
                  "extract it from the Virtual Reality Software CD that "
                  "is distributed with the device. The file that "
                  "we need is contained within the 'data2.cab' file.\n\n"
                  "The 'data2.cab' file is an InstallShield file. To read "
                  "this file and extract the FortiusSWPID1942Renum.hex file we need "
                  "you will need to use the 'unshield' tool.\n\n"
                  "Please take care to ensure that the file is the latest version "
                  "of the Firmware file.\n\n"
                  "If you choose to copy to library the file will be copied into the "
                  "GoldenCheetah library, otherwise we will reference it. We recommend "
                  "that you copy the file, but can reference it where it may be updated "
                  "by the standard Tacx software.\n\n");

    file = new QLabel("File:", this);

    name= new QLineEdit(this);
    name->setEnabled(false);
    name->setText(path);

    // Layout widgets
    QHBoxLayout *buttons = new QHBoxLayout;
    QHBoxLayout *filedetails = new QHBoxLayout;
    filedetails->addWidget(file);
    filedetails->addWidget(name);
    filedetails->addWidget(browse);
    filedetails->addStretch();

    buttons->addWidget(copy);
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(ok);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(filedetails);
    mainLayout->addWidget(help);
    mainLayout->addLayout(buttons);

    // connect widgets
    connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(browse, SIGNAL(clicked()), this, SLOT(browseClicked()));
}

void
FortiusDialog::okClicked()
{
    QString filePath = name->text();
    if (!QFile(filePath).exists()) {

        QMessageBox::critical(0, "No File Selected",
        QString("You must select a firmware file, typically called ") +
                 "FortiusSWPID1942Renum.hex");
        return;
    }

    // either copy it, or reference it!
    if (copy->isChecked()) {

        QString fileName = QFileInfo(filePath).fileName();
        QString targetFileName = QFileInfo(mainWindow->home.absolutePath() + "/../").absolutePath() + "/" + fileName;

        // if the current file exists, wipe it
        if (QFile(targetFileName).exists()) QFile(targetFileName).remove();
        QFile(filePath).copy(targetFileName);

        name->setText(targetFileName);
    }
    path = name->text();
    accept();
}

void
FortiusDialog::cancelClicked()
{
    reject();
}

void
FortiusDialog::browseClicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Intel Firmware File (*.hex)"));
    if (file != "") name->setText(file);
}
