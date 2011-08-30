/* 
 * $Id: DownloadRideDialog.cpp,v 1.4 2006/08/11 20:02:13 srhea Exp $
 *
 * Copyright (c) 2006-2008 Sean C. Rhea (srhea@srhea.net)
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

#include "DownloadRideDialog.h"
#include "Device.h"
#include "MainWindow.h"
#include <assert.h>
#include <errno.h>
#include <QtGui>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

DownloadRideDialog::DownloadRideDialog(MainWindow *mainWindow,
                                       const QDir &home) : 
    mainWindow(mainWindow), home(home), cancelled(false),
    downloadInProgress(false)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Download Ride Data"));

    portCombo = new QComboBox(this);

    QLabel *instructLabel = new QLabel(tr("Instructions:"), this);
    label = new QLabel(this);
    label->setIndent(10);

    deviceCombo = new QComboBox(this);
    QList<QString> deviceTypes = Device::deviceTypes();
    assert(deviceTypes.size() > 0);
    BOOST_FOREACH(QString device, deviceTypes) {
        deviceCombo->addItem(device);
    }

    downloadButton = new QPushButton(tr("&Download"), this);
    eraseRideButton = new QPushButton(tr("&Erase Ride(s)"), this);
    rescanButton = new QPushButton(tr("&Rescan"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);

    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadClicked()));
    connect(eraseRideButton, SIGNAL(clicked()), this, SLOT(eraseClicked()));
    connect(rescanButton, SIGNAL(clicked()), this, SLOT(scanCommPorts()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(deviceCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(setReadyInstruct()));

    QHBoxLayout *buttonLayout = new QHBoxLayout; 
    buttonLayout->addWidget(downloadButton); 
    buttonLayout->addWidget(eraseRideButton);
    buttonLayout->addWidget(rescanButton); 
    buttonLayout->addWidget(cancelButton); 

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel(tr("Select port:"), this));
    mainLayout->addWidget(portCombo);
    mainLayout->addWidget(new QLabel(tr("Select device type:"), this));
    mainLayout->addWidget(deviceCombo);
    mainLayout->addWidget(instructLabel);
    mainLayout->addWidget(label);
    mainLayout->addLayout(buttonLayout);

    scanCommPorts();
}

void 
DownloadRideDialog::setReadyInstruct()
{
    if (portCombo->count() == 0) {
        label->setText(tr("No devices found.  Make sure the device\n"
                          "unit is plugged into the computer,\n"
                          "then click \"Rescan\" to check again."));
        downloadButton->setEnabled(false);
        eraseRideButton->setEnabled(false);
    }
    else {
        Device &device = Device::device(deviceCombo->currentText());
        QString inst = device.downloadInstructions();
        if (inst.size() == 0)
            label->setText(tr("Click Download to begin downloading."));
        else
            label->setText(inst + tr(", \nthen click Download."));
        downloadButton->setEnabled(true);
        if (deviceCombo->currentText() == "SRM") // only SRM supports erase ride for now
            eraseRideButton->setEnabled(true);
    }
}

void
DownloadRideDialog::scanCommPorts()
{
    portCombo->clear();
    QString err;
    devList = CommPort::listCommPorts(err);
    if (err != "") {
        QString msg = tr("Warning(s):\n\n") + err + tr("\n\nYou may need to (re)install "
            "the FTDI or PL2303 drivers before downloading.");
        QMessageBox::warning(0, tr("Error Loading Device Drivers"), msg,
                             QMessageBox::Ok, QMessageBox::NoButton);
    }
    for (int i = 0; i < devList.size(); ++i) {
        portCombo->addItem(devList[i]->name());
        // Hack: SRM PCV download cables use the PL2203 chipset.  If the
        // first device name contains "PL2303", then, we're probably dealing
        // with an SRM, so go ahead and select the SRM device.  Generalize?
        if ((i == 0) && devList[i]->name().contains("PL2303")) {
            int j = deviceCombo->findText("SRM");
            if (j >= 0)
                deviceCombo->setCurrentIndex(j);
        }
    }
    if (portCombo->count() > 0)
        downloadButton->setFocus();
    setReadyInstruct();
}

bool
DownloadRideDialog::statusCallback(const QString &statusText)
{
    label->setText(statusText);
    QCoreApplication::processEvents();
    return !cancelled;
}

void 
DownloadRideDialog::downloadClicked()
{
    downloadButton->setEnabled(false);
    eraseRideButton->setEnabled(false);
    rescanButton->setEnabled(false);
    downloadInProgress = true;
    CommPortPtr dev;
    for (int i = 0; i < devList.size(); ++i) {
        if (devList[i]->name() == portCombo->currentText()) {
            dev = devList[i];
            break;
        }
    }
    assert(dev);
    QString err;
    QString tmpname, filename;
    Device &device = Device::device(deviceCombo->currentText());
    if (!device.download(
            dev, home, tmpname, filename,
            boost::bind(&DownloadRideDialog::statusCallback, this, _1), err))
    {
        if (cancelled) {
            QMessageBox::information(this, tr("Download canceled"),
                                     tr("Cancel clicked by user."));
            cancelled = false;
        }
        else {
            QMessageBox::information(this, tr("Download failed"), err);
        }
        downloadInProgress = false;
        reject();
        return;
    }

    QString filepath = home.absolutePath() + "/" + filename;
    if (QFile::exists(filepath)) {
        if (QMessageBox::warning(
                this,
                tr("Ride Already Downloaded"),
                tr("This ride appears to have already ")
                + tr("been downloaded.  Do you want to ")
                + tr("overwrite the previous download?"),
                tr("&Overwrite"), tr("&Cancel"), 
                QString(), 1, 1) == 1) {
            reject();
            return;
        }
    }

#ifdef __WIN32__
    // Windows ::rename won't overwrite an existing file.
    if (QFile::exists(filepath)) {
        QFile old(filepath);
        if (!old.remove()) {
            QMessageBox::critical(this, tr("Error"), 
                                  tr("Failed to remove existing file ") 
                                  + filepath + ": " + old.error()); 
            QFile::remove(tmpname);
            reject();
        }
    }
#endif

    // Use ::rename() instead of QFile::rename() to get forced overwrite.
    if (rename(QFile::encodeName(tmpname), QFile::encodeName(filepath)) < 0) {
        QMessageBox::critical(this, tr("Error"), 
                              tr("Failed to rename ") + tmpname + tr(" to ")
                              + filepath + ": " + strerror(errno));
        QFile::remove(tmpname);
        reject();
        return;
    }

    QMessageBox::information(this, tr("Success"), tr("Download complete."));
    mainWindow->addRide(filename);

    device.cleanup(dev);

    downloadInProgress = false;
    accept();
}

void
DownloadRideDialog::eraseClicked()
{
    downloadButton->setEnabled(false);
    eraseRideButton->setEnabled(false);
    rescanButton->setEnabled(false);
    downloadInProgress = true;
    CommPortPtr dev;
    for (int i = 0; i < devList.size(); ++i) {
        if (devList[i]->name() == portCombo->currentText()) {
            dev = devList[i];
            break;
        }
    }
    assert(dev);
    Device &device = Device::device(deviceCombo->currentText());
    device.cleanup(dev);
    downloadInProgress = false;
    accept();
}

void 
DownloadRideDialog::cancelClicked()
{
    if (!downloadInProgress)
        reject();
    else
        cancelled = true;
}

