#include "BT40DeviceConnectionDialog.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "BT40Controller.h"
#include "Colors.h"

BT40DeviceConnectionDialog::BT40DeviceConnectionDialog(BT40Controller *controller)
    : controller(controller)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setModal(true);

    setMinimumWidth(500 * dpiXFactor);
    setMinimumHeight(400 * dpiYFactor);

    mainLayout = new QVBoxLayout;
    searching = tr("Searching...");
    connected = tr("Connected");
    setWindowTitle(tr("Connecting to Bluetooth devices"));

    createSubTitle();
    createTree();
    createButtons();
    setLayout(mainLayout);

    addDevices();

    connect(controller, SIGNAL(deviceConnecting(QString, QString)), this, SLOT(deviceConnecting(QString, QString)));
    connect(controller, SIGNAL(deviceConnected(QString, QString)), this, SLOT(deviceConnected(QString, QString)));
    connect(controller, SIGNAL(deviceDisconnected(QString, QString)), this, SLOT(deviceDisconnected(QString, QString)));
    connect(controller, SIGNAL(deviceConnectionError(QString, QString)), this, SLOT(deviceConnectionError(QString, QString)));
    connect(controller, SIGNAL(scanFinished(bool)), this, SLOT(scanFinished(bool)));
}

int
BT40DeviceConnectionDialog::exec()
{
    controller->start();
    controller->resetCalibrationState();
    return QDialog::exec();
}

void
BT40DeviceConnectionDialog::okClicked()
{
    // Nothing to do. Just accept configuration
    accept();
}

void
BT40DeviceConnectionDialog::cancelClicked()
{
    // Cleanup and notify, that user does not want to start the workout
    controller->stop();
    reject();
}

void
BT40DeviceConnectionDialog::retryClicked()
{
    /*
     * ToDo: Only retry connection to "not connected", "disconnected" or "not found"
    */

    // avoid multiple concurrent retries
    retryButton->setEnabled(false);

    // Wait for scan to finish
    okButton->setEnabled(false);

    // reset bt40 controller
    controller->stop();

    for (int i=0; i< tree->invisibleRootItem()->childCount(); i++)
    {
        QTreeWidgetItem *item = tree->invisibleRootItem()->child(i);
        QLabel* label = dynamic_cast<QLabel *>(tree->itemWidget(item,1));
        updateLabel(label, searching, Qt::black);
    }

    // Give bluetooth manager some time to disconnect all devices.
    // Five seconds was enough on a i5-450M from 2010. Even if it's
    // still not enough time, this will only result in a incorrect
    // connection status label.
    QTime sleepTime = QTime::currentTime().addSecs(5);

    // Allow UI to render and process evens during wait
    while (QTime::currentTime() < sleepTime)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        // Avoid 100% cpu
        QThread::usleep(100);
    }

    controller->start();
    controller->resetCalibrationState();
}

void
BT40DeviceConnectionDialog::deviceConnecting(QString address, QString uuid)
{
    setDeviceStatus(
        address,
        uuid,
        tr("Connecting..."),
        Qt::black);
}

void
BT40DeviceConnectionDialog::deviceConnected(QString address, QString uuid)
{
    setDeviceStatus(
        address,
        uuid,
        connected,
        Qt::darkGreen);

    if (allConnected())
    {
        // Don't wait for scan to finish. All required devices
        // already discovered and connected.
        okButton->setEnabled(true);
    }
}

void
BT40DeviceConnectionDialog::deviceDisconnected(QString address, QString uuid)
{
    setDeviceStatus(
        address,
        uuid,
        tr("Disconnected"),
        Qt::darkRed);
}

void
BT40DeviceConnectionDialog::deviceConnectionError(QString address, QString uuid)
{
    // Treat connection erros as "not found", since
    // QBluetoothDeviceDiscoveryAgent also reports
    // "trusted" or cached devices. This means, the Agent
    // will also try to connect to devices, not found by
    // the current scan. If this fails, we will receive a
    // connection error. Just print "not found" to avoid
    // confusion.
    setDeviceStatus(
        address,
        uuid,
        tr("Device not found"),
        Qt::darkYellow);
}

void
BT40DeviceConnectionDialog::scanFinished(bool /*foundAnyDevices*/)
{
    // Set all "searching" devices to "not found"
    for (int i=0; i< tree->invisibleRootItem()->childCount(); i++)
    {
        QTreeWidgetItem *item = tree->invisibleRootItem()->child(i);
        QLabel* label = dynamic_cast<QLabel *>(tree->itemWidget(item,1));
        if (label->text() == searching)
        {
            updateLabel(label, tr("Device not found"), Qt::darkYellow);
        }
    }

    // Scan finished, allow user to start workout or search again
    retryButton->setEnabled(true);
    okButton->setEnabled(true);
}

void
BT40DeviceConnectionDialog::createSubTitle()
{
    QLabel* subTitle = new QLabel(this);
    subTitle->setText("Searching for devices.\n\nPress OK to start your workout with the current configuration.");
    subTitle->setMargin(4 * dpiXFactor);
    mainLayout->addWidget(subTitle);
}

void
BT40DeviceConnectionDialog::createTree()
{
    // Device status list
    tree = new QTreeWidget(this);
    tree->headerItem()->setText(0, tr("Device"));
    tree->headerItem()->setText(1, tr("Status"));
    tree->setColumnCount(2);
    tree->setSelectionMode(QAbstractItemView::NoSelection);
    tree->setUniformRowHeights(true);
    tree->setIndentation(0);

    tree->header()->resizeSection(0, 600 * dpiXFactor); // Device name
    tree->header()->resizeSection(0, 200 * dpiXFactor); // status
    mainLayout->addWidget(tree);
}

void
BT40DeviceConnectionDialog::createButtons()
{
    // Buttons
    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    mainLayout->addLayout(buttons);

    cancelButton = new QPushButton(tr("Cancel"), this);
    buttons->addWidget(cancelButton);

    retryButton = new QPushButton(tr("Retry"), this);
     // Disable until scan finished
    retryButton->setEnabled(false);
    buttons->addWidget(retryButton);

    okButton = new QPushButton(tr("OK"), this);
    // Disable until scan finished
    okButton->setEnabled(false);
    buttons->addWidget(okButton);

    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(retryButton, SIGNAL(clicked()), this, SLOT(retryClicked()));
    connect(okButton, SIGNAL(clicked()), this, SLOT(okClicked()));
}

void
BT40DeviceConnectionDialog::addDevices()
{
    // Add available devices with "searching..." status
    foreach(const DeviceInfo deviceInfo, controller->getAllowedDevices())
    {
        QTreeWidgetItem *add = new QTreeWidgetItem(tree->invisibleRootItem());
        QLabel *name = new QLabel(this);
        name->setText(deviceInfo.getName());

        QLabel *status = new QLabel(this);
        updateLabel(status, searching, Qt::black);

        tree->setItemWidget(add, 0, name);
        tree->setItemWidget(add, 1, status);

        add->setData(0, AddressRole, deviceInfo.getAddress()); // other OS
        add->setData(0, UuidRole, deviceInfo.getUuid()); // macOS
    }
}

void
BT40DeviceConnectionDialog::setDeviceStatus(const QString& address, const QString& uuid, const QString& status, QColor color)
{
    for (int i=0; i< tree->invisibleRootItem()->childCount(); i++)
    {
        QTreeWidgetItem *item = tree->invisibleRootItem()->child(i);

        if (item->data(0, AddressRole).toString() == address
                && item->data(0, UuidRole).toString() == uuid)
        {
            updateLabel(dynamic_cast<QLabel *>(tree->itemWidget(item,1)), status, color);
        }
    }
}

void BT40DeviceConnectionDialog::updateLabel(QLabel* label, const QString& status, QColor color)
{
    if (label)
    {
        QPalette palette = label->palette();
        palette.setColor(label->foregroundRole(), color);
        label->setPalette(palette);

        label->setText(status);
    }
}

bool
BT40DeviceConnectionDialog::allConnected()
{
    for (int i=0; i< tree->invisibleRootItem()->childCount(); i++)
    {
        QTreeWidgetItem *item = tree->invisibleRootItem()->child(i);
        QLabel *label = dynamic_cast<QLabel *>(tree->itemWidget(item,1));
        if (label->text() != connected)
        {
            return false;
        }
    }

    return true;
}
