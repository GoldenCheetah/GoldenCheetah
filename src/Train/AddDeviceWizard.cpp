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

#include "AddDeviceWizard.h"
#include "MainWindow.h"
#include "Athlete.h"
#include "Context.h"
#include "Colors.h"
#include "ConfigDialog.h"
#include "HelpWhatsThis.h"

#include "RealtimeController.h" // for power trainer definitions
#include "MultiRegressionizer.h"

 // WIZARD FLOW
//
// 10. Select Device Type
// 20. Scan for Device / select Serial
// 30. Firmware for Fortius
// 35. Firmware for Imagic
// 50. Pair for ANT
// 55. Pair for BTLE
// 60. Virtual Power
// 70. Finalise
//

// Main wizard
AddDeviceWizard::AddDeviceWizard(Context *context) : QWizard(context->mainWindow), context(context), controller(NULL)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Training_AddDeviceWizard));

#ifdef Q_OS_MAC
    setWizardStyle(QWizard::ModernStyle);
#endif

    virtualPowerIndex = 0;           // index of selected virtual power function
    wheelSize = 0;
    inertialMomentKGM2 = 0;
    strideLength = 0;

    // delete when done
    setWindowModality(Qt::NonModal); // avoid blocking WFAPI calls for kickr
    setAttribute(Qt::WA_DeleteOnClose);
    setMinimumWidth(600 *dpiXFactor);
    setMinimumHeight(500 *dpiYFactor);

    // title
    setWindowTitle(tr("Add Device Wizard"));
    scanner = new DeviceScanner(this);

    setPage(10, new AddType(this));   // done
    setPage(20, new AddSearch(this)); // done
    setPage(30, new AddFirmware(this)); // done
    setPage(35, new AddImagic(this)); // done
    setPage(50, new AddPair(this));     // done
    setPage(55, new AddPairBTLE(this));     // done
    setPage(60, new AddVirtualPower(this));
    setPage(70, new AddFinal(this));    // todo -- including virtual power

    done = false;

    type = -1;
    current = 0;
}

/*----------------------------------------------------------------------
 * Wizard Pages
 *--------------------------------------------------------------------*/

//Select device type
AddType::AddType(AddDeviceWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Select Device"));
    setSubTitle(tr("What kind of device to add"));

    QWidget *buttons = new QWidget(this);

    QVBoxLayout *mainlayout = new QVBoxLayout(this);
    setLayout(mainlayout);
    scrollarea=new QScrollArea(this);
    scrollarea->setWidgetResizable(true);
    scrollarea->setWidget(buttons);
    mainlayout->addWidget(scrollarea);

    QVBoxLayout *layout = new QVBoxLayout;
    buttons->setLayout(layout);

    mapper = new QSignalMapper(this);
    connect(mapper, &QSignalMapper::mappedString, this, &AddType::clicked);

    foreach(DeviceType t, wizard->deviceTypes.Supported) {
        if (t.type) {
            QCommandLinkButton *p = new QCommandLinkButton(t.name, t.description, this);
            p->setStyleSheet(QString("font-size: %1px;").arg(12 * dpiXFactor));
            connect(p, SIGNAL(clicked()), mapper, SLOT(map()));
            mapper->setMapping(p, QString("%1").arg(t.type));
            layout->addWidget(p);
        }
    }
    label = new QLabel("", this);
    layout->addWidget(label);

    next = 20;
    setFinalPage(false);
}

void
AddType::initializePage()
{
    // reset any device search info
    wizard->portSpec = "";
    wizard->found = false;
    wizard->current = 0;
    if (wizard->controller) {
        delete wizard->controller;
        wizard->controller = NULL;
    }
}
   

void
AddType::clicked(QString p)
{
    // reset -- particularly since we might get here from
    //          other pages hitting 'Back'
    initializePage();
    wizard->type = p.toInt();

    // what are we scanning for?
    int i=0;
    foreach(DeviceType t, wizard->deviceTypes.Supported) {
        if (t.type == wizard->type) wizard->current = i;
        i++;
    }

    // we don't do a quick scan for a kickr since it takes 15 seconds
    // to timeout and we don't want to get stuck on the front page for
    // that long -- it will seem like it has not worked / crashed
    if (wizard->deviceTypes.Supported[wizard->current].connector != DEV_BTLE)
        wizard->found = wizard->scanner->quickScan(false); // do a quick scan
    else
        wizard->found = false;

    // Still no dice. Go to the not found dialog
    if (wizard->found == false) next =20;
    else {
        switch(wizard->deviceTypes.Supported[wizard->current].type) {
        case DEV_BT40 : next = 55; break;
        case DEV_ANTLOCAL : next = 50; break; // pair 
        default:
        case DEV_CT : next = 60; break; // confirm and add 
        case DEV_MONARK : next = 60; break; // confirm and add
        case DEV_FORTIUS : next = 30; break; // confirm and add
        case DEV_IMAGIC : next = 35; break; // confirm and add
        }
    }
    wizard->next();
}

DeviceScanner::DeviceScanner(AddDeviceWizard *wizard) : wizard(wizard) {}

void
DeviceScanner::run()
{
    active = true;
    bool result = false;

    for (int i=0; active && !result && i<10; i++) { // search for longer

        // better to wait a while, esp. if its just a USB device
#ifdef WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
        result = quickScan(false);
    }
    if (active) emit finished(result); // only signal if we weren't aborted!
}

void
DeviceScanner::stop()
{
    active = false;
}


bool
DeviceScanner::quickScan(bool deep) // scan quickly or if true scan forever, as deep as possible
                                       // for now deep just means try 3 time before giving up, but we
                                       // may want to change that to include scanning more devices?
{

    // get controller
    if (wizard->controller) {
        delete wizard->controller;
        wizard->controller=NULL;
    }

    switch (wizard->deviceTypes.Supported[wizard->current].type) {

    // we will need a factory for this soon..
    case DEV_CT : wizard->controller = new ComputrainerController(NULL, NULL); break;
    case DEV_MONARK : wizard->controller = new MonarkController(NULL, NULL); break;
    case DEV_KETTLER : wizard->controller = new KettlerController(NULL, NULL); break;
    case DEV_KETTLER_RACER : wizard->controller = new KettlerRacerController (NULL, NULL); break;
    case DEV_ERGOFIT : wizard->controller = new ErgofitController(NULL, NULL); break;
    case DEV_DAUM : wizard->controller = new DaumController(NULL, NULL); break;
#ifdef GC_HAVE_LIBUSB
    case DEV_FORTIUS : wizard->controller = new FortiusController(NULL, NULL); break;
    case DEV_IMAGIC : wizard->controller = new ImagicController(NULL, NULL); break;
#endif
    case DEV_NULL : wizard->controller = new NullController(NULL, NULL); break;
    case DEV_ANTLOCAL : wizard->controller = new ANTlocalController(NULL, NULL); break;
#ifdef QT_BLUETOOTH_LIB
    case DEV_BT40 : wizard->controller = new BT40Controller(NULL, NULL); break;
#endif

    default: wizard->controller = NULL; break;
    }

    //----------------------------------------------------------------------
    // Search for USB devices
    //----------------------------------------------------------------------

    bool isfound = false;
    int count=0;
    do {

        // can we find it automatically?
        if (wizard->controller) {
            isfound = wizard->controller->find();
        }

        if (isfound == false && (wizard->deviceTypes.Supported[wizard->current].connector == DEV_LIBUSB ||
                            wizard->deviceTypes.Supported[wizard->current].connector == DEV_USB)) {

            // Go to next page where we do not found, rescan and manual override
            if (!deep) return false;
        }
    
        //----------------------------------------------------------------------
        // Search serial ports
        //----------------------------------------------------------------------

        if (isfound == false && wizard->deviceTypes.Supported[wizard->current].connector == DEV_SERIAL) {

            // automatically discover a serial port ...
            QString error;
            foreach (CommPortPtr port, Serial::myListCommPorts(error)) {

                // check if controller still exists. gets deleted when scan cancelled
                if (wizard->controller && wizard->controller->discover(port->name()) == true) {
                    isfound = true;
                    wizard->portSpec = port->name();
                    break;
                }
            }

            // if we still didn't find it then we need to fall back to the user
            // specifying the device on the next page
        }

    } while (!isfound && deep && count++ < 2);

    return isfound;

}

// Scan for device port / usb etc
AddSearch::AddSearch(AddDeviceWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setSubTitle(tr("Scan for connected devices"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    isSearching = false;
    active = false;

    label = new QLabel(tr("Please make sure your device is connected, switched on and working. "
                          "We will scan for the device type you have selected at known ports.\n\n"));
    label->setWordWrap(true);
    layout->addWidget(label);

    bar = new QProgressBar(this);
    bar->setMaximum(100);
    bar->setMinimum(0);
    bar->setValue(0);
    //bar->setText("Searching...");
    layout->addWidget(bar);

    QHBoxLayout *hlayout2 = new QHBoxLayout;
    stop = new QPushButton(tr("Search"), this);
    hlayout2->addStretch();
    hlayout2->addWidget(stop);
    layout->addLayout(hlayout2);

    label1 = new QLabel(tr("If your device is not found you can select the device port "
                               "manually by using the selection box below.")); 
    label1->setWordWrap(true);
    layout->addWidget(label1);

    label2 = new QLabel(tr("\nDevice found.\nClick Next to Continue\n"));
    label2->hide();
    label2->setWordWrap(true);
    layout->addWidget(label2);

    QHBoxLayout *hlayout = new QHBoxLayout;
    manual = new QComboBox(this);
    hlayout->addStretch();
    hlayout->addWidget(manual);
    layout->addLayout(hlayout);

    layout->addStretch();

    connect(stop, SIGNAL(clicked()), this, SLOT(doScan()));
    connect(manual, SIGNAL(currentIndexChanged(int)), this, SLOT(chooseCOMPort()));
    connect(wizard->scanner, SIGNAL(finished(bool)), this, SLOT(scanFinished(bool)));
}

void
AddSearch::chooseCOMPort()
{
    if (active) return;

    if (manual->currentIndex() <= 0) { // we unselected or something.
        wizard->found = false;
        wizard->portSpec = "";
        return;
    }

    // stop any scan that may be in process?
    if (isSearching == true) {
       doScan(); // remember doScan toggles with the stop/search again button
    }

    // let the user select the port
    wizard->portSpec = manual->itemText(manual->currentIndex());

    // carry on then 
    wizard->found = true; // ugh
}

void
AddSearch::initializePage()
{
    setTitle(QString(tr("%1 Search")).arg(wizard->deviceTypes.Supported[wizard->current].name));

    // we only ask for the device file if it is a serial device
    if (wizard->deviceTypes.Supported[wizard->current].connector == DEV_SERIAL) {

        // wipe away whatever items it has now
        for (int i=manual->count(); i > 0; i--) manual->removeItem(0);

        // add in the items we have..
        manual->addItem(tr("Select COM port"));
        QString error;
        foreach (CommPortPtr port, Serial::myListCommPorts(error)) manual->addItem(port->name());
        manual->show();
        label1->show();


    } else {
        label1->hide();
        manual->hide();
    }

    bar->show();
    stop->show();
    label->show();
    label2->hide();
    active = false;
    doScan();
}

void
AddSearch::scanFinished(bool result)
{
    isSearching = false;
    wizard->found = result;
    bar->setMaximum(100);
    bar->setMinimum(0);
    bar->setValue(0);
    stop->setText(tr("Search Again"));

    if (result == true) { // woohoo we found one

        if (wizard->deviceTypes.Supported[wizard->current].type == DEV_BT40) {
            // ok we've started finding devices, lets go straight into the
            // pair screen since we now want to see the data etc.
            wizard->next();
        } else {

            bar->hide();
            stop->hide();
            manual->hide();
            label->hide();
            label1->hide();
            if (wizard->portSpec != "")
                label2->setText(QString(tr("\nDevice found (%1).\nPress Next to Continue\n")).arg(wizard->portSpec));
            else
                label2->setText(tr("\nDevice found.\nPress Next to Continue\n"));
            label2->show();
        }
    } 
    QApplication::processEvents();
    emit completeChanged();
}

void
AddSearch::doScan()
{
    if (isSearching == false) { // start a scan

        // make bar bouncy...
        bar->setMaximum(0);
        bar->setMinimum(0);
        bar->setValue(0);
        stop->setText(tr("Stop Searching"));
        isSearching = true;
        manual->setCurrentIndex(0); //deselect any chosen port
        wizard->found = false;
        wizard->portSpec = "";

        wizard->scanner->start();

    } else { // stop a scan

        isSearching = false;
        // make bar stationary...
        bar->setMaximum(100);
        bar->setMinimum(0);
        bar->setValue(0);
        stop->setText(tr("Search again"));

        wizard->scanner->stop();
    }
}

int
AddSearch::nextId() const
{
    // Still no dice. Go to the not found dialog
    if (wizard->found == false)  return 60;
    else {
        switch(wizard->deviceTypes.Supported[wizard->current].type) {
        case DEV_ANTLOCAL : return 50; break; // pair 
        case DEV_BT40 : return 55; break; // pair BT devices
        default:
        case DEV_CT : return 60; break; // confirm and add 
        case DEV_MONARK : return 60; break; // confirm and add
        case DEV_KETTLER : return 60; break; // confirm and add
        case DEV_KETTLER_RACER : return 60; break; // confirm and add
        case DEV_ERGOFIT : return 60; break; // confirm and add
        case DEV_DAUM : return 60; break; // confirm and add
        case DEV_FORTIUS : return 30; break; // confirm and add
        case DEV_IMAGIC : return 35; break; // confirm and add
        }
    }
}

bool
AddSearch::validatePage()
{
    return wizard->found;
}

void
AddSearch::cleanupPage()
{
    wizard->scanner->stop();
    if (isSearching) {
    // give it time to stop...
#ifdef WIN32
        Sleep(2000);
#else
        sleep(2);
#endif
    }
    isSearching=false;
    if (wizard->controller) {
        delete wizard->controller;
        wizard->controller = NULL;
    }
}

// Fortius Firmware
AddFirmware::AddFirmware(AddDeviceWizard *parent) : QWizardPage(parent), parent(parent)
{
    setTitle(tr("Select Firmware"));
    setSubTitle(tr("Select Firmware for Tacx Fortius"));

    // create widgets
    browse = new QPushButton(tr("Browse"), this);
    copy = new QCheckBox(tr("Copy to Library"));
    copy->setChecked(true);

    help = new QLabel(this);
    help->setWordWrap(true);
    help->setText(tr("Tacx Fortius trainers require a firmware file "
                  "which is provided by Tacx BV. This file is a "
                  "copyrighted file and cannot be distributed with "
                  "GoldenCheetah.\n\n"
                  "On windows it is typically installed in C:\\Windows\\system32 "
                  "and is called 'FortiusSWPID1942Renum.hex'.\n\n"
#if defined Q_OS_LINUX || defined Q_OS_MAC
                  "On Linux and Apple computers you will need to "
                  "extract it from the VR Software CD."
                  "The file we need is within the 'data2.cab' file, "
                  "which is an InstallShield file that can be read "
                  "with the 'unshield' tool\n\n"
#endif
                  "Please take care to ensure that the file is the latest version "
                  "of the Firmware file.\n\n"
                  "If you choose to copy to library the file will be copied into the "
                  "GoldenCheetah library, otherwise we will reference it. "));

    file = new QLabel(tr("File:"), this);

    name= new QLineEdit(this);
    name->setEnabled(false);

    QString fortiusFirmware = appsettings->value(this, FORTIUS_FIRMWARE, "").toString();
    name->setText(fortiusFirmware);

    // Layout widgets
    QHBoxLayout *buttons = new QHBoxLayout;
    QHBoxLayout *filedetails = new QHBoxLayout;
    filedetails->addWidget(file);
    filedetails->addWidget(name);
    filedetails->addWidget(browse);
    filedetails->addStretch();

    buttons->addWidget(copy);
    buttons->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(filedetails);
    mainLayout->addWidget(help);
    mainLayout->addStretch();
    mainLayout->addLayout(buttons);

    // connect widgets
    connect(browse, SIGNAL(clicked()), this, SLOT(browseClicked()));
}

bool
AddFirmware::validatePage()
{
    // changed to true to support 1932 version which doesn't require a firmware file
    QString filePath = name->text();
    if (filePath == "" || !QFile(filePath).exists()) return true;

    // either copy it, or reference it!
    if (copy->isChecked()) {

        QString fileName = QFileInfo(filePath).fileName();
        QString targetFilePath = QFileInfo(parent->context->athlete->home->root().canonicalPath() + "/../").canonicalPath() + "/" + fileName;

        // check not the same thing!
        if(QFileInfo(filePath).canonicalPath() != QFileInfo(targetFilePath).canonicalPath()) {
            // if the current file exists, wipe it
            if (QFile(targetFilePath).exists()) QFile(targetFilePath).remove();
            QFile(filePath).copy(targetFilePath);
        }
        name->setText(targetFilePath);
    }
    appsettings->setValue(FORTIUS_FIRMWARE, name->text());
    return true;
}

void
AddFirmware::browseClicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Intel Firmware File (*.hex)"));
    if (file != "") name->setText(file);
}

// Imagic Firmware
AddImagic::AddImagic(AddDeviceWizard *parent) : QWizardPage(parent), parent(parent)
{
    setTitle(tr("Select Device driver"));
    setSubTitle(tr("Select Driver for Tacx Imagic"));

    // create widgets
    browse = new QPushButton(tr("Browse"), this);
    copy = new QCheckBox(tr("Copy to Library"));
    copy->setChecked(true);

    help = new QLabel(this);
    help->setWordWrap(true);

    help->setText(tr("Tacx Imagic trainers require firmware to be loaded. "
                  "This firmware is embedded within the device driver I-magic.sys "
                  "which is provided by Tacx BV."
                  "This is a copyrighted file and cannot be distributed with "
                  "GoldenCheetah.\n"
                  "On windows systems with Tacx Fortius/Imagic installed "
                  "you will typically find this in C:\\Windows\\system32.\n\n"
                  "Alternatively, you can extract it from a Tacx software CD\n"
                  "On older Fortius cds it lives in the directory FortiusInstall\\Support\\driver_imagic.\n"
                  "On later TTS cds it is in support\\drivers_usb_interface\\32\n\n"
                  "Place a copy of this file somewhere on this PC and reference it above.\n\n"
                  "If you choose to copy to library the file will be copied into the "
                  "GoldenCheetah library, otherwise we will reference it in place. "));

    file = new QLabel(tr("File:"), this);

    name= new QLineEdit(this);
    name->setEnabled(false);

    QString imagicFirmware = appsettings->value(this, IMAGIC_FIRMWARE, "").toString();
    name->setText(imagicFirmware);

    // Layout widgets
    QHBoxLayout *buttons = new QHBoxLayout;
    QHBoxLayout *filedetails = new QHBoxLayout;
    filedetails->addWidget(file);
    filedetails->addWidget(name);
    filedetails->addWidget(browse);
    filedetails->addStretch();

    buttons->addWidget(copy);
    buttons->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(filedetails);
    mainLayout->addWidget(help);
    mainLayout->addStretch();
    mainLayout->addLayout(buttons);

    // connect widgets
    connect(browse, SIGNAL(clicked()), this, SLOT(browseClicked()));
}

bool
AddImagic::validatePage()
{
    QString filePath = name->text();
    if (filePath == "" || !QFile(filePath).exists()) return false;

    // either copy it, or reference it!
    if (copy->isChecked()) {

        QString fileName = QFileInfo(filePath).fileName();
        QString targetFilePath = QFileInfo(parent->context->athlete->home->root().canonicalPath() + "/../").canonicalPath() + "/" + fileName;

        // check not the same thing!
        if(QFileInfo(filePath).canonicalPath() != QFileInfo(targetFilePath).canonicalPath()) {
            // if the current file exists, wipe it
            if (QFile(targetFilePath).exists()) QFile(targetFilePath).remove();
            QFile(filePath).copy(targetFilePath);
        }
        name->setText(targetFilePath);
    }
    appsettings->setValue(IMAGIC_FIRMWARE, name->text());
    return true;
}

void
AddImagic::browseClicked()
{
    QString file = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("Tacx Device driver (*.sys)"));
    if (file != "") name->setText(file);
}

// Pair devices
AddPair::AddPair(AddDeviceWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Pair Devices"));
    setSubTitle(tr("Search for and pair ANT+ devices (Pair FE-C sensor only for FE-C devices)"));

    signalMapper = NULL;

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    channelWidget = new QTreeWidget(this);
    layout->addWidget(channelWidget);

    cyclist = parent->context->athlete->cyclist;

}

static void
addSensorTypes(ANT *ant, QComboBox *p)
{
    for (int i=0; ant->ant_sensor_types[i].suffix !=  '\0'; i++) {
        if (ant->ant_sensor_types[i].user) {
            if (*ant->ant_sensor_types[i].iconname != '\0') {

                // we want dark not light
                QImage img(ant->ant_sensor_types[i].iconname);
                img.invertPixels();
                QIcon icon(QPixmap::fromImage(img));

                p->addItem(icon, ant->ant_sensor_types[i].descriptive_name, ant->ant_sensor_types[i].type);
            } else {
                p->addItem(ant->ant_sensor_types[i].descriptive_name, ant->ant_sensor_types[i].type);
            }
        }
    }
}

void
AddPair::cleanupPage()
{
    updateValues.stop();
    if (wizard->controller) {
        wizard->controller->stop();
#ifdef WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
        delete wizard->controller;
        wizard->controller = NULL;
    }
}

static void enableDisable(QTreeWidget *tree)
{
    // enable disable widgets based upon sensor selection
    for (int i=0; i< tree->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *item = tree->invisibleRootItem()->child(i);

        // is it selected or not?
        bool enable = (dynamic_cast<QComboBox*>(tree->itemWidget(item,0))->currentIndex() != 0);

        // enable all thos widgetry
        tree->itemWidget(item,2)->setEnabled(enable); // value
        tree->itemWidget(item,3)->setEnabled(enable); // status
    }
}

void
AddPair::initializePage()
{
    // setup the controller and start it off so we can
    // manipulate it
    if (wizard->controller) delete wizard->controller;
    if (signalMapper) delete signalMapper;
    wizard->controller = new ANTlocalController(NULL,NULL);
    dynamic_cast<ANTlocalController*>(wizard->controller)->setDevice(wizard->portSpec);
    dynamic_cast<ANTlocalController*>(wizard->controller)->myANTlocal->setConfigurationMode(true);
    wizard->controller->start();
    wizard->profile=""; // clear any thing thats there now
    signalMapper = new QSignalMapper(this);

    // Channel 0, look for any (0 devicenumber) speed and distance device

    // wait for it to start
#ifdef WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
    int channels = dynamic_cast<ANTlocalController*>(wizard->controller)->channels();

    // Tree Widget of the channel controls
    channelWidget->clear();
    channelWidget->headerItem()->setText(0, tr("Sensor"));
    channelWidget->headerItem()->setText(1, tr("ANT+ Id"));
    channelWidget->headerItem()->setText(2, tr("Value"));
    channelWidget->headerItem()->setText(3, tr("Status"));
    channelWidget->setColumnCount(4);
    channelWidget->setSelectionMode(QAbstractItemView::NoSelection);
    //channelWidget->setEditTriggers(QAbstractItemView::SelectedClicked); // allow edit
    channelWidget->setUniformRowHeights(true);
    channelWidget->setIndentation(0);

    channelWidget->header()->resizeSection(0,175*dpiXFactor); // type
    channelWidget->header()->resizeSection(1,75*dpiXFactor); // id
    channelWidget->header()->resizeSection(2,120*dpiXFactor); // value
    channelWidget->header()->resizeSection(3,110*dpiXFactor); // status

    // defaults
    static const int index4[4] = { 1,2,3,5 };
    static const int index8[8] = { 1,2,3,4,5,6,9,10 };
    const int *index = channels == 4 ? index4 : index8;

    // how many devices we got then?
    for (int i=0; i< channels; i++) {

        QTreeWidgetItem *add = new QTreeWidgetItem(channelWidget->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);

        // sensor type
        QComboBox *sensorSelector = new QComboBox(this);
        addSensorTypes(dynamic_cast<ANTlocalController*>(wizard->controller)->myANTlocal, sensorSelector);
        sensorSelector->setCurrentIndex(index[i]);
        channelWidget->setItemWidget(add, 0, sensorSelector);

        // sensor id
        QLineEdit *sensorId = new QLineEdit(this);
        sensorId->setEnabled(false);
        sensorId->setText(tr("none"));
        channelWidget->setItemWidget(add, 1, sensorId);

        // value
        QLabel *value = new QLabel(this);
        QFont bigger;
        bigger.setPointSize(20);
        value->setFont(bigger);
        value->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
        value->setText("0");
        channelWidget->setItemWidget(add, 2, value);

        // status
        QLabel *status = new QLabel(this);
        status->setText(tr("Un-Paired"));
        channelWidget->setItemWidget(add, 3, status);

        //channelWidget->verticalHeader()->resizeSection(i,40)
        connect(sensorSelector, SIGNAL(currentIndexChanged(int)), signalMapper, SLOT(map()));
        signalMapper->setMapping(sensorSelector, i);
    }
    channelWidget->setCurrentItem(channelWidget->invisibleRootItem()->child(0));
    enableDisable(channelWidget);

    updateValues.start(200); // 5hz
    connect(signalMapper, &QSignalMapper::mappedInt, this, &AddPair::sensorChanged);
    connect(&updateValues, SIGNAL(timeout()), this, SLOT(getChannelValues()));
    connect(wizard->controller, SIGNAL(foundDevice(int,int,int)), this, SLOT(channelInfo(int,int,int)));
    connect(wizard->controller, SIGNAL(searchTimeout(int)), this, SLOT(searchTimeout(int)));
    //connect(wizard->controller, SIGNAL(lostDevice(int)), this, SLOT(searchTimeout(int)));

    // now we're ready to get notifications - set channels
    for (int i=0; i<channels; i++) sensorChanged(i);

}

void
AddPair::sensorChanged(int channel)
{
    QTreeWidgetItem *item = channelWidget->invisibleRootItem()->child(channel);
    enableDisable(channelWidget);

    // first off lets unassign this channel
    dynamic_cast<ANTlocalController*>(wizard->controller)->myANTlocal->setChannel(channel, -1, 0);
    dynamic_cast<QLineEdit*>(channelWidget->itemWidget(item,1))->setText(tr("none"));
    dynamic_cast<QLabel*>(channelWidget->itemWidget(item,2))->setText(0);

    // what is it then? unused or restart scan?
    QComboBox *p = dynamic_cast<QComboBox *>(channelWidget->itemWidget(item,0));
    int channel_type = p->itemData(p->currentIndex()).toInt();
    if (channel_type == ANTChannel::CHANNEL_TYPE_UNUSED) {
        dynamic_cast<QLabel*>(channelWidget->itemWidget(item,3))->setText(tr("Unused"));
    }else if (channel_type == ANTChannel::CHANNEL_TYPE_CONTROL) {
        // for a remote control we are the master, so there is nothing for us to pair
        // just generate a random device number between 1 & 65535
        dynamic_cast<QLabel*>(channelWidget->itemWidget(item,3))->setText(tr("Master"));
        uint16_t deviceNumber = (QRandomGenerator::global()->generate() % 65535) + 1;
        dynamic_cast<QLineEdit *>(channelWidget->itemWidget(item,1))->setText(QString("%1").arg(deviceNumber));
        dynamic_cast<ANTlocalController*>(wizard->controller)->myANTlocal->setChannel(channel, deviceNumber, channel_type);
    } else {
        dynamic_cast<QLabel*>(channelWidget->itemWidget(item,3))->setText(tr("Searching..."));
        dynamic_cast<ANTlocalController*>(wizard->controller)->myANTlocal->setChannel(channel, 0, channel_type);
    }
}

void
AddPair::channelInfo(int channel, int device_number, int device_id)
{
    Q_UNUSED(device_id);
    QTreeWidgetItem *item = channelWidget->invisibleRootItem()->child(channel);
    dynamic_cast<QLineEdit *>(channelWidget->itemWidget(item,1))->setText(QString("%1").arg(device_number));
    dynamic_cast<QLabel *>(channelWidget->itemWidget(item,3))->setText(QString(tr("Paired")));
}

void
AddPair::searchTimeout(int channel)
{
    // Kick if off again, just mimic user reselecting the same sensor type
    sensorChanged(channel);
}


void 
AddPair::getChannelValues()
{
    if (wizard->controller == NULL) return;

    // enable disable widgets based upon sensor selection
    for (int i=0; i< channelWidget->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *item = channelWidget->invisibleRootItem()->child(i);

        // is it selected or not?
        bool enable = (dynamic_cast<QComboBox*>(channelWidget->itemWidget(item,0))->currentIndex() != 0);

        if (enable) {
            QComboBox *p =dynamic_cast<QComboBox*>(channelWidget->itemWidget(item,0));

            // speed+cadence is two values!
            if (p->itemData(p->currentIndex()) == ANTChannel::CHANNEL_TYPE_SandC) {

                dynamic_cast<QLabel *>(channelWidget->itemWidget(item,2))->setText(QString("%1 %2")
                .arg((int)dynamic_cast<ANTlocalController*>(wizard->controller)->myANTlocal->channelValue2(i) //speed
                          * (appsettings->cvalue(cyclist, GC_WHEELSIZE, 2100).toInt()/1000) * 60 / 1000)
                .arg((int)dynamic_cast<ANTlocalController*>(wizard->controller)->myANTlocal->channelValue(i))); // cad

            } else if (p->itemData(p->currentIndex()) == ANTChannel::CHANNEL_TYPE_MOXY) {

                dynamic_cast<QLabel *>(channelWidget->itemWidget(item,2))->setText(QString("%1 %2")
                .arg(dynamic_cast<ANTlocalController*>(wizard->controller)->myANTlocal->channelValue(i), 0, 'f', 1) // tHb
                .arg(dynamic_cast<ANTlocalController*>(wizard->controller)->myANTlocal->channelValue2(i), 0, 'f', 1)); // SmO2

            } else {
            dynamic_cast<QLabel *>(channelWidget->itemWidget(item,2))->setText(QString("%1")
                .arg((int)dynamic_cast<ANTlocalController*>(wizard->controller)->myANTlocal->channelValue(i)));
            }
        }
    }
    
}

bool
AddPair::validatePage()
{
    // when next is clicked we need to get the paired values
    // and create a profile, a blank profile will be created if
    // no devices have been paired. This means devices will be
    // automatically paired at runtime
    wizard->profile="";
    for (int i=0; i< channelWidget->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *item = channelWidget->invisibleRootItem()->child(i);

        // what is it then? unused or restart scan?
        QComboBox *p = dynamic_cast<QComboBox *>(channelWidget->itemWidget(item,0));
        int channel_type = p->itemData(p->currentIndex()).toInt();

        if (channel_type == ANTChannel::CHANNEL_TYPE_UNUSED) continue; // not paired

        int device_number = dynamic_cast<QLineEdit*>(channelWidget->itemWidget(item,1))->text().toInt();

        if (device_number)
            wizard->profile += QString(wizard->profile != "" ? ", %1%2" : "%1%2")
                               .arg(device_number)
                               .arg(ANT::deviceIdCode(channel_type));
    }
    return true;
}

// Pair devices
AddPairBTLE::AddPairBTLE(AddDeviceWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Bluetooth 4.0 Sensors"));
    setSubTitle(tr("Search for and pair of BTLE sensors."));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    // Indicate search in progress
    bar = new QProgressBar(this);
    bar->setMaximum(0);
    bar->setMinimum(0);
    bar->setValue(0);
    layout->addWidget(bar);

    channelWidget = new QTreeWidget(this);
    layout->addWidget(channelWidget);
}

void
AddPairBTLE::cleanupPage()
{
    // Reset progress bar on "back"
    bar->setMaximum(0);
    bar->setMinimum(0);
    bar->setValue(0);
}

void
AddPairBTLE::initializePage()
{
    // currently BT40Controller cannot report supported and detected sensors
    // nor can enable them selectively, it just uses HR,Power and CSC sensors

    // Tree Widget of the channel controls
    channelWidget->clear();
    channelWidget->headerItem()->setText(0, tr("Device"));
    channelWidget->setColumnCount(1);
    // Use MultiSelection for device selection
    channelWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    channelWidget->setUniformRowHeights(true);
    channelWidget->setIndentation(0);

    channelWidget->header()->resizeSection(0,440*dpiXFactor); // Device name


    // Start search
    if (wizard->controller) delete wizard->controller;
    wizard->controller = new BT40Controller(NULL,NULL);
    connect(wizard->controller, SIGNAL(scanFinished(bool)), this, SLOT(scanFinished(bool)));
    wizard->controller->start();
}

void
AddPairBTLE::scanFinished(bool foundDevices)
{
    // Show search done
    bar->setMaximum(1);
    bar->setValue(1);

    if (foundDevices)
    {
        foreach(QBluetoothDeviceInfo deviceInfo, dynamic_cast<BT40Controller*>(wizard->controller)->getDeviceInfo())
        {
            QTreeWidgetItem *add = new QTreeWidgetItem(channelWidget->invisibleRootItem());

            // Remove chars used as separator in storage
            QString deviceName = deviceInfo.name();
            deviceName.replace(';', ' ');
            deviceName.replace(',', ' ');
            deviceName = deviceName.trimmed();

            // Save device info in item
            add->setData(0, NameRole, deviceName);
            add->setData(0, AddressRole, deviceInfo.address().toString()); // other OS
            add->setData(0, UuidRole, deviceInfo.deviceUuid().toString()); // macOS

            // Setup display text
            QLabel *status = new QLabel(this);
            status->setText(deviceName);
            channelWidget->setItemWidget(add, 0, status);
        }
    }
    else
    {
        QTreeWidgetItem *add = new QTreeWidgetItem(channelWidget->invisibleRootItem());
        QLabel *status = new QLabel(this);
        status->setText(tr("No sensors found..."));
        channelWidget->setItemWidget(add, 0, status);
    }

    // Disconnect and cleanup
    wizard->controller->stop();
}

bool
AddPairBTLE::validatePage()
{
    wizard->profile="";

    // Add selected sensors to profile
    for (int i=0; i< channelWidget->invisibleRootItem()->childCount(); i++) {
        QTreeWidgetItem *item = channelWidget->invisibleRootItem()->child(i);

        if (item->isSelected())
        {
            // pack into device info for validation
            DeviceInfo deviceInfo(
                        item->data(0, NameRole).toString(),
                        item->data(0, AddressRole).toString(),
                        item->data(0, UuidRole).toString());

            if (deviceInfo.isValid())
            {
                // Split sensors with ','
                // Split name, address and uuid with ';'
                wizard->profile += QString(wizard->profile != "" ? ", %1;%2;%3" : "%1;%2;%3")
                                   .arg(deviceInfo.getName())
                                   .arg(deviceInfo.getAddress())
                                   .arg(deviceInfo.getUuid());
            }
        }
    }

    return true;
}

void
AddVirtualPower::mySpinBoxChanged(int i)
{
    Q_UNUSED(i)
    drawConfig();
}

void
AddVirtualPower::myDoubleSpinBoxChanged(double d)
{
    Q_UNUSED(d)
    drawConfig();
}

void
AddVirtualPower::myCellChanged(int nRow, int nCol) 
{
    Q_UNUSED(nCol)
    bool state = this->blockSignals(true);
    if (state) return;

    // If clicked row is last row
    if ((nRow + 1) == virtualPowerTableWidget->rowCount()) {
        // If last row isn't zeros...
        if (virtualPowerTableWidget->item(nRow, 0)->text().toDouble() != 0. ||
            virtualPowerTableWidget->item(nRow, 1)->text().toDouble() != 0.) {
            // Make a new row and put zeros there.
            virtualPowerTableWidget->setRowCount(nRow + 2);
            virtualPowerTableWidget->setItem(nRow + 1, 0, new QTableWidgetItem("0."));
            virtualPowerTableWidget->setItem(nRow + 1, 1, new QTableWidgetItem("0."));
        }
    }

    // A table cell is changed... If selected virtual trainer isn't custom then swap index
    // to None.
    if (controller->virtualPowerTrainerManager.IsPredefinedVirtualPowerTrainerIndex(virtualPower->currentIndex())) {
        virtualPower->setCurrentIndex(0);
        wizard->virtualPowerIndex = 0;
    } else {
        // Otherwise copy current custom name to edit save box.
        virtualPowerNameEdit->insert(virtualPower->currentText());
        wizard->virtualPowerIndex = virtualPower->currentIndex();
    }

    drawConfig();

    this->blockSignals(state);
}

// Sort the virtualPowerTableWidget
void AddVirtualPower::mySortTable(int i) {
    Q_UNUSED(i)

    bool state = this->blockSignals(true);
    if (state) return;

    struct pair { double speed, watts; };
    std::vector<pair> elements;

    int rowCount = virtualPowerTableWidget->rowCount();
    for (int i = 0; i < rowCount; i++) {
        double speed = virtualPowerTableWidget->item(i, 0)->text().toDouble();
        double watts = virtualPowerTableWidget->item(i, 1)->text().toDouble();

        // Remove cleared rows: ignore and remove entries with zero speed, except the final row.
        if (((i + 1) == rowCount) || (speed != 0.) || (watts != 0.)) {
            elements.push_back({ speed, watts });
        }
    }

    std::sort(elements.begin(), elements.end(),
        [](const pair& a, const pair& b) {
            if (a.speed == 0. && a.watts == 0.) return false; // ensure 0.,0. entry stays at end.
            if (a.speed == b.speed) return a.watts < b.watts; // if speed is equal sort by watts.
            return a.speed < b.speed; 
        }
    );

    // Resize table to element count
    virtualPowerTableWidget->setRowCount((int)(elements.size()));

    // Rewrite all rows to match sorted elements
    for (int i = 0; i < elements.size(); i++) {
        virtualPowerTableWidget->item(i, 0)->setData(Qt::EditRole, QString::number(elements[i].speed));
        virtualPowerTableWidget->item(i, 1)->setData(Qt::EditRole, QString::number(elements[i].watts));
    }

    drawConfig();

    this->blockSignals(state);
}

// Clear table widget and set it to values from selected interpolator.
void
AddVirtualPower::mySetTableFromComboBox(int i) {

    bool state = this->blockSignals(true);
    if (state) return;

    const VirtualPowerTrainer* pFit = controller->virtualPowerTrainerManager.GetVirtualPowerTrainer(i);

    int rows = 0;
    if (pFit) {

        rows = 11;
        virtualPowerTableWidget->setRowCount(rows + 1);

        // First element is power at 1kph
        virtualPowerTableWidget->setItem(0, 0, new QTableWidgetItem(QString::number(1.)));
        virtualPowerTableWidget->setItem(0, 1, new QTableWidgetItem(QString::number(pFit->m_pf->Fit(1.))));

        // Convention, every additional row is +10kph.
        double vFactor = 10.;

        // Some trainers are too good for velocity and use wheel rpm.
        // Convert velocity to wheel rpm
        double vToRpm = 1.;
        if (pFit->m_fUseWheelRpm) {
            // Read wheel circumference. Is in mm.
            double circumferenceMM = wheelSizeEdit->text().toDouble();

            // v to rpm
            vToRpm = ((1. * 1000 * 1000) / 60) / circumferenceMM;
        }

        // Body of elements is power every 10kph
        for (int row = 1; row < rows; row++) {
            double v = row * vFactor;
            virtualPowerTableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(v)));
            virtualPowerTableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(pFit->m_pf->Fit(vToRpm * v))));
        }
    } else {
        rows = 0;
        virtualPowerTableWidget->setRowCount(rows + 1);
    }

    // End of table are the zeros...
    virtualPowerTableWidget->setItem(rows, 0, new QTableWidgetItem(QString::number(0.)));
    virtualPowerTableWidget->setItem(rows, 1, new QTableWidgetItem(QString::number(0.)));

    // Clear epsilon so fitters fit.
    fitEpsilonSpinBox->setValue(0.);

    // Set new index in wizard.
    wizard->virtualPowerIndex = virtualPower->currentIndex();

    drawConfig();

    this->blockSignals(state);
}

void
AddVirtualPower::virtualPowerNameChanged() {
    virtualPowerCreateButton->setEnabled(!virtualPowerNameEdit->text().isEmpty());
}

void
AddVirtualPower::myCreateCustomPowerCurve() {

    bool state = this->blockSignals(true);
    if (state) return;

    T_MultiRegressionizer<XYVector<double>> fit(fitEpsilonSpinBox->value(), fitOrderSpinBox->value());

    for (int i = 0; i < virtualPowerTableWidget->rowCount(); i++) {
        double x = virtualPowerTableWidget->item(i, 0)->text().toDouble();
        double y = virtualPowerTableWidget->item(i, 1)->text().toDouble();

        if (x > 0) {
            fit.Push({ x, y });
        }
    }

    PolyFit<double>* pf = fit.AsPolyFit();

    VirtualPowerTrainer* p = new VirtualPowerTrainer;

    // At this point ensure there are no '|' in name, since if encoded it would confuse decoding.
    QStringList namePieces = virtualPowerNameEdit->text().split("|");

    // No point being fancy, simply take the name before the '|'.
    char* pNameCopy = new char[strlen(namePieces.at(0).toStdString().c_str()) + 1];
    strcpy(pNameCopy, namePieces.at(0).toStdString().c_str());

    p->m_pName = pNameCopy; // freed by manager when manager is destroyed.
    p->m_pf = pf;
    p->m_fUseWheelRpm = false;
    p->m_inertialMomentKGM2 = inertialMomentKGM2Edit->value();

    controller->virtualPowerTrainerManager.PushCustomVirtualPowerTrainer(p);

    int idx = (int)controller->virtualPowerTrainerManager.GetVirtualPowerTrainerCount() - 1;
    const VirtualPowerTrainer* p2 = controller->virtualPowerTrainerManager.GetVirtualPowerTrainer(idx);

    virtualPower->addItem(tr(p2->m_pName));

    virtualPower->setCurrentIndex(virtualPower->count() - 1);

    wizard->virtualPowerIndex = virtualPower->currentIndex();

    this->blockSignals(state);
}

void
AddVirtualPower::drawConfig() {
    virtualPowerTableWidget->resizeColumnsToContents();
    virtualPowerTableWidget->resizeRowsToContents();
    virtualPowerTableWidget->verticalHeader()->hide();

    QScatterSeries* series0 = new QScatterSeries();
    series0->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    series0->setMarkerSize(15.0);
    series0->setColor(QColor("blue"));

    double minX = 0., minY = 0., maxX = 1, maxY = 1.;

    T_MultiRegressionizer<XYVector<double>> fit(fitEpsilonSpinBox->value(), fitOrderSpinBox->value());

    for (int i = 0; i < virtualPowerTableWidget->rowCount(); i++) {
        double x = virtualPowerTableWidget->item(i, 0)->text().toDouble();
        double y = virtualPowerTableWidget->item(i, 1)->text().toDouble();

        if (x > 0) {
            series0->append(x, y);
            fit.Push({ x, y });
            minX = std::min<double>(minX, x);
            minY = std::min<double>(minY, y);
            maxX = std::max<double>(maxX, x);
            maxY = std::max<double>(maxY, y);
        }
    }

    // Add display point for fit's zero intercept.
    series0->append(0., fit.Fit(0.));
    
    QScatterSeries* series1 = new QScatterSeries();
    series1->setMarkerShape(QScatterSeries::MarkerShapeRectangle);
    series1->setMarkerSize(15.0);
    series1->setColor(QColor("red"));

    double delta = maxX / 200;
    for (double v = 0; v < maxX; v += delta) {
        double fitV = fit.Fit(v);
        minY = std::min<double>(minY, fitV);
        maxY = std::max<double>(maxY, fitV);
        series1->append(v, fit.Fit(v));
    }
    minY = std::max<double>(minY, -10.);

    virtualPowerScatterChart->removeAllSeries();
    virtualPowerScatterChart->addSeries(series1);
    virtualPowerScatterChart->addSeries(series0); // draw second so points overlay interpolation
    virtualPowerScatterChart->createDefaultAxes();
    virtualPowerScatterChart->axes(Qt::Horizontal, series0).first()->setRange(0., maxX * 1.10);
    virtualPowerScatterChart->axes(Qt::Vertical,   series0).first()->setRange(minY, maxY * 1.10);
    
    fitStdDevLabel->setText(QString(tr("Fit StdDev: %1")).arg(fit.StdDev()));
    fitOrderLabel->setText(QString(tr("Max Order: %1")).arg(fit.Order()));
}

AddVirtualPower::AddVirtualPower(AddDeviceWizard* parent) : QWizardPage(parent), wizard(parent), controller(parent->controller)
{
    QVBoxLayout* layout = new QVBoxLayout;
    setLayout(layout);

    // Title
    setTitle(tr("Setup Virtual Power"));

    QLabel* label = new QLabel(tr("Use this page to setup virtual power for trainers that can only report speed or rpm. It is probably a bad idea to derive power from speed when also receiving power data from the trainer.\n\n"));
    label->setWordWrap(true);

    layout->addWidget(label);

    // Virtual Power Curve Choices
    QHBoxLayout* hlayout = new QHBoxLayout;
    layout->addLayout(hlayout);

    QFormLayout* form2layout = new QFormLayout;
    form2layout->addRow(new QLabel(tr("Virtual Power Curve"), this), (virtualPower = new QComboBox(this)));

    // Wheel size
    int wheelSize = appsettings->cvalue(parent->context->athlete->cyclist, GC_WHEELSIZE, 2100).toInt();

    rimSizeCombo = new QComboBox();
    rimSizeCombo->addItems(WheelSize::RIM_SIZES);

    tireSizeCombo = new QComboBox();
    tireSizeCombo->addItems(WheelSize::TIRE_SIZES);

    wheelSizeEdit = new QLineEdit(QString("%1").arg(wheelSize), this);
    wheelSizeEdit->setInputMask("0000");

    QLabel* wheelSizeUnitLabel = new QLabel(tr("mm"), this);

    QHBoxLayout* wheelSizeLayout = new QHBoxLayout();
    wheelSizeLayout->addWidget(rimSizeCombo);
    wheelSizeLayout->addWidget(tireSizeCombo);
    wheelSizeLayout->addWidget(wheelSizeEdit);
    wheelSizeLayout->addWidget(wheelSizeUnitLabel);

    inertialMomentKGM2Edit = new QDoubleSpinBox(this);
    inertialMomentKGM2Edit->setRange(0, 100);
    inertialMomentKGM2Edit->setDecimals(3);
    inertialMomentKGM2Edit->setSingleStep(.1);
    inertialMomentKGM2Edit->setValue(0.);
    inertialMomentKGM2Edit->setToolTip(tr("Trainer's Moment of Inertia (KG M^2)"));

    QHBoxLayout* inertialMomentKGM2Layout = new QHBoxLayout;
    inertialMomentKGM2Layout->addWidget(inertialMomentKGM2Edit);
    inertialMomentKGM2Layout->addStretch();

    stridelengthEdit = new QLineEdit(this);
    stridelengthEdit->setText("115");
    QHBoxLayout* stridelengthLayout = new QHBoxLayout;
    stridelengthLayout->addWidget(stridelengthEdit);
    stridelengthLayout->addStretch();

    connect(rimSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(calcWheelSize()));
    connect(tireSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(calcWheelSize()));
    connect(wheelSizeEdit, SIGNAL(textEdited(QString)), this, SLOT(resetWheelSize()));

    form2layout->addRow(new QLabel(tr("Wheel Size"), this), wheelSizeLayout);
    form2layout->addRow(new QLabel(tr("Stride Length (cm)"), this), stridelengthLayout);
    form2layout->addRow(new QLabel(tr("Rotational Inertia (KG M^2)"), this), inertialMomentKGM2Layout);

    hlayout->addLayout(form2layout);

    // ---------------------------------------------
    // Virtual Power
    //
    QGroupBox* groupBox = new QGroupBox(tr("Custom Virtual Power Curve"));
    QVBoxLayout* virtualPowerGroupLayout = new QVBoxLayout;

    QHBoxLayout* virtualPowerLayout = new QHBoxLayout;

    // Virtual Power Input Table
    virtualPowerTableWidget = new QTableWidget(1, 2, this);
    virtualPowerTableWidget->setHorizontalHeaderLabels({ "Kph", "Watts" });
    virtualPowerTableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    virtualPowerTableWidget->resize(0, 0);
    virtualPowerTableWidget->setItem(0, 0, new QTableWidgetItem("0."));
    virtualPowerTableWidget->setItem(0, 1, new QTableWidgetItem("0."));
    virtualPowerTableWidget->setItem(1, 0, new QTableWidgetItem("0."));
    virtualPowerTableWidget->setItem(1, 1, new QTableWidgetItem("0."));
    virtualPowerLayout->addWidget(virtualPowerTableWidget);
    virtualPowerTableWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

    connect(virtualPowerTableWidget, SIGNAL(cellChanged(int, int)),
        this, SLOT(myCellChanged(int, int)));

    // Virtual Power Graph
    virtualPowerScatterChart = new QChart();
    virtualPowerScatterChart->setAnimationOptions(QChart::AllAnimations);
    virtualPowerScatterChart->createDefaultAxes();
    virtualPowerScatterChart->layout()->setContentsMargins(0, 0, 0, 0);
    virtualPowerScatterChart->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    virtualPowerScatterChart->legend()->hide();

    virtualPowerScatterChartView = new QChartView(virtualPowerScatterChart);

    virtualPowerLayout->addWidget(virtualPowerScatterChartView);

    virtualPowerGroupLayout->addLayout(virtualPowerLayout);

    QString fitOrderSpinBoxText = tr("Max Polynomial Order:");
    fitOrderSpinBoxLabel = new QLabel(fitOrderSpinBoxText);

    fitOrderSpinBox = new QSpinBox();
    fitOrderSpinBox->setRange(1, 6);
    fitOrderSpinBox->setSingleStep(1);
    fitOrderSpinBox->setValue(3);
    fitOrderSpinBox->setToolTip(tr("Max Order for polynomial fit"));
    fitOrderSpinBox->setFixedWidth(50);

    QString fitEpsilonSpinBoxText = tr("Polynomial Epsilon:");
    fitEpsilonSpinBoxLabel = new QLabel(fitEpsilonSpinBoxText);

    fitEpsilonSpinBox = new QDoubleSpinBox();
    fitEpsilonSpinBox->setRange(0., 100.);
    fitEpsilonSpinBox->setSingleStep(0.5);
    fitEpsilonSpinBox->setValue(3);
    fitEpsilonSpinBox->setToolTip(tr("Polynomial fit criteria, in watts. Larger value permits looser fit."));
    fitEpsilonSpinBox->setFixedWidth(150);

    fitStdDevLabel = new QLabel();
    fitStdDevLabel->setText(QString(tr("StdDev of fit to data: %1 ")).arg(0.));

    fitOrderLabel = new QLabel();
    fitOrderLabel->setText(QString(tr("Order of fit: %1")).arg(1.));

    QHBoxLayout* virtualPowerSpinBoxLayout = new QHBoxLayout;

    virtualPowerSpinBoxLayout->addWidget(fitOrderSpinBoxLabel);
    virtualPowerSpinBoxLayout->addWidget(fitOrderSpinBox);
    virtualPowerSpinBoxLayout->addWidget(fitEpsilonSpinBoxLabel);
    virtualPowerSpinBoxLayout->addWidget(fitEpsilonSpinBox);
    virtualPowerSpinBoxLayout->addWidget(fitOrderLabel);
    virtualPowerSpinBoxLayout->addWidget(fitStdDevLabel);

    virtualPowerGroupLayout->addLayout(virtualPowerSpinBoxLayout);

    QHBoxLayout* virtualPowerNameLayout = new QHBoxLayout;
    virtualPowerNameLabel = new QLabel(tr("Name:"));
    virtualPowerNameEdit = new QLineEdit(this);
    virtualPowerCreateButton = new QPushButton(tr("Create and Select"), this);
    virtualPowerCreateButton->setEnabled(false);
    virtualPowerCreateButton->setToolTip(tr("Give the current fit a name and use for this device."));

    virtualPowerNameLayout->addWidget(virtualPowerNameLabel);
    virtualPowerNameLayout->addWidget(virtualPowerNameEdit);
    virtualPowerNameLayout->addWidget(virtualPowerCreateButton);

    connect(virtualPowerNameEdit, SIGNAL(textChanged(const QString&)),
        this, SLOT(virtualPowerNameChanged()));
    connect(virtualPowerCreateButton, SIGNAL(clicked()),
        this, SLOT(myCreateCustomPowerCurve()));

    virtualPowerGroupLayout->addLayout(virtualPowerNameLayout);

    groupBox->setLayout(virtualPowerGroupLayout);
    layout->addWidget(groupBox);

    connect(fitOrderSpinBox,   SIGNAL(valueChanged(int)), this, SLOT(mySpinBoxChanged(int)));
    connect(fitEpsilonSpinBox, SIGNAL(valueChanged(double)), this, SLOT(myDoubleSpinBoxChanged(double)));

    QHeaderView* header = virtualPowerTableWidget->horizontalHeader();
    connect (header,           SIGNAL(sectionClicked(int)), this, SLOT(mySortTable(int)));

    connect (virtualPower,     SIGNAL(currentIndexChanged(int)), this, SLOT(mySetTableFromComboBox(int)));

    drawConfig();
}

void
AddVirtualPower::initializePage()
{
    bool state = this->blockSignals(true);

    wizard->virtualPowerIndex = 0;

    virtualPower->addItem(tr("None"));
    for (int i = 1; i < wizard->controller->virtualPowerTrainerManager.GetVirtualPowerTrainerCount(); i++) {
        virtualPower->addItem(tr(wizard->controller->virtualPowerTrainerManager.GetVirtualPowerTrainer(i)->m_pName));
    }
    virtualPower->setCurrentIndex(0);

    controller = wizard->controller;

    resetWheelSize();

    this->blockSignals(state);
}

bool
AddVirtualPower::validatePage()
{
    wizard->virtualPowerName = virtualPower->currentText();
    return true;
}

void
AddVirtualPower::calcWheelSize()
{
    int diameter = WheelSize::calcPerimeter(rimSizeCombo->currentIndex(), tireSizeCombo->currentIndex());
    if (diameter > 0)
        wheelSizeEdit->setText(QString("%1").arg(diameter));
    bool fValidDouble = false;
    double wheelSize = wheelSizeEdit->text().toDouble(&fValidDouble);
    wizard->wheelSize = fValidDouble ? wheelSize : 0.;
}

void
AddVirtualPower::resetWheelSize()
{
    rimSizeCombo->setCurrentIndex(0);
    tireSizeCombo->setCurrentIndex(0);
    wizard->virtualPowerIndex = 0;
    calcWheelSize();
}

// Final confirmation
AddFinal::AddFinal(AddDeviceWizard *parent) : QWizardPage(parent), wizard(parent)
{
    setTitle(tr("Done"));
    setSubTitle(tr("Confirm configuration and add device"));

    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);

    QLabel *label = new QLabel(tr("We will now add a new device with the configuration shown "
                               "below. Please take a moment to review and then click Finish "
                               "to add the device and complete this wizard, or press the Back "
                               "button to make amendments.\n\n"));
    label->setWordWrap(true);
    layout->addWidget(label);

    QHBoxLayout *hlayout = new QHBoxLayout;
    layout->addLayout(hlayout);

    QFormLayout *formlayout = new QFormLayout;
    formlayout->addRow(new QLabel(tr("Name*"), this), (name=new QLineEdit(this)));
    formlayout->addRow(new QLabel(tr("Port"), this), (port=new QLineEdit(this)));
    formlayout->addRow(new QLabel(tr("Profile"), this), (profile=new QLineEdit(this)));
    formlayout->addRow(new QLabel(tr("Virtual Power"), this), (virtualPowerName=new QLineEdit(this)));
    port->setEnabled(false); // no edit
    virtualPowerName->setEnabled(false); // no edit
    hlayout->addLayout(formlayout);

    layout->addStretch();
}

void
AddFinal::initializePage()
{
    port->setText(wizard->portSpec);
    profile->setText(wizard->profile);
    virtualPowerName->setText(wizard->virtualPowerName);
}

bool
AddFinal::validatePage()
{
    if (name->text() != "") {

        DeviceConfigurations all;
        DeviceConfiguration add;

        // lets update 'here' with what we did then...
        add.type = wizard->type;
        add.name = name->text();
        add.portSpec = port->text();
        add.deviceProfile = profile->text();
        add.postProcess = wizard->virtualPowerIndex;
        add.wheelSize = wizard->wheelSize;
        add.inertialMomentKGM2 = wizard->inertialMomentKGM2;
        add.stridelength = wizard->strideLength;
        add.controller = wizard->controller;

        QList<DeviceConfiguration> list = all.getList();
        list.insert(0, add);

        // call device add wizard.
        all.writeConfig(list);

        // tell everyone
        wizard->context->notifyConfigChanged(CONFIG_DEVICES);
 
        // shut down the controller, if it is there, since it will
        // still be connected to the device (in case we hit the back button)
        if (wizard->controller) {
            wizard->controller->stop();
#ifdef WIN32
            Sleep(1000);
#else
            sleep(1);
#endif
            delete wizard->controller;
            wizard->controller = NULL;
        }
        return true;
    }
    return false;
}
