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
    action(actionIdle)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Download Ride Data");

    deviceCombo = new QComboBox(this);
    QList<QString> deviceTypes = Devices::typeNames();
    assert(deviceTypes.size() > 0);
    BOOST_FOREACH(QString device, deviceTypes) {
        deviceCombo->addItem(device);
    }
    connect(deviceCombo, SIGNAL(currentIndexChanged(QString)), this, SLOT(deviceChanged(QString)));

    portCombo = new QComboBox(this);

    statusLabel = new QTextEdit(this);
    statusLabel->setReadOnly(true);
    statusLabel->setAcceptRichText(false);

    // would prefer a progress bar, but some devices (eg. PTap) don't give
    // a hint about the total work, so this isn't possible.
    progressLabel = new QLabel(this);
    progressLabel->setIndent(10);
    progressLabel->setTextFormat(Qt::PlainText);

    downloadButton = new QPushButton(tr("&Download"), this);
    eraseRideButton = new QPushButton(tr("&Erase Ride(s)"), this);
    rescanButton = new QPushButton(tr("&Rescan"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    closeButton = new QPushButton(tr("&Close"), this);

    downloadButton->setDefault( true );

    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadClicked()));
    connect(eraseRideButton, SIGNAL(clicked()), this, SLOT(eraseClicked()));
    connect(rescanButton, SIGNAL(clicked()), this, SLOT(scanCommPorts()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(closeClicked()));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(downloadButton);
    buttonLayout->addWidget(eraseRideButton);
    buttonLayout->addWidget(rescanButton);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(closeButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel(tr("Select device type:"), this));
    mainLayout->addWidget(deviceCombo);
    mainLayout->addWidget(new QLabel(tr("Select port:"), this));
    mainLayout->addWidget(portCombo);
    mainLayout->addWidget(new QLabel(tr("Info:"), this));
    mainLayout->addWidget(statusLabel, 1);
    mainLayout->addWidget(progressLabel);
    mainLayout->addLayout(buttonLayout);

    scanCommPorts();
}

void
DownloadRideDialog::setReadyInstruct()
{
    progressLabel->setText("");

    if (portCombo->count() == 0) {
        statusLabel->setPlainText(tr("No devices found.  Make sure the device\n"
                          "unit is plugged into the computer,\n"
                          "then click \"Rescan\" to check again."));
        updateAction( actionMissing );
    }
    else {
        DevicesPtr devtype = Devices::getType(deviceCombo->currentText());
        QString inst = devtype->downloadInstructions();
        if (inst.size() == 0)
            statusLabel->setPlainText("Click Download to begin downloading.");
        else
            statusLabel->setPlainText(inst + ", \nthen click Download.");

        updateAction( actionIdle );
    }
}

void
DownloadRideDialog::scanCommPorts()
{

    QString err;
    devList = CommPort::listCommPorts(err);
    if (err != "") {
        QString msg = "Warning(s):\n\n" + err + "\n\nYou may need to (re)install "
            "the FTDI or PL2303 drivers before downloading.";
        QMessageBox::warning(0, "Error Loading Device Drivers", msg,
                             QMessageBox::Ok, QMessageBox::NoButton);
    }

    updatePort();
}

void
DownloadRideDialog::updatePort()
{

    portCombo->clear();
    QList<QString> typeNames( Devices::typeNames() );
    DevicesPtr selectedType = Devices::getType(deviceCombo->currentText());

    for (int i = 0; i < devList.size(); ++i) {
        // suppress unsupported ports
        if( ! selectedType->supportsPort( devList.at(i) ) )
            continue;

        // suppress ports exclusively supported by other devices
        bool otherExclusive = false;
        for( int t = 0; t < typeNames.size(); ++t ){
            DevicesPtr otherType = Devices::getType(typeNames.at(t));

            // hmm, comparing pointers makes me feel a bit dizzy...
            if( selectedType == otherType )
                continue;

            if( otherType->exclusivePort( devList.at(i) ) ){
                otherExclusive = true;
                break;
            }
        }

        if( otherExclusive )
            continue;

        // add port
        portCombo->addItem( devList[i]->id());

        // make an exclusive port the default selection:
        if( selectedType->exclusivePort( devList.at(i) ) )
            portCombo->setCurrentIndex(portCombo->findText(devList[i]->id()));
    }

    setReadyInstruct();
}

bool
DownloadRideDialog::isCancelled()
{
    return cancelled;
}

void
DownloadRideDialog::updateAction( downloadAction newAction )
{

    switch( newAction ){
      case actionMissing:
        downloadButton->setEnabled(false);
        eraseRideButton->setEnabled(false);
        rescanButton->setEnabled(true);
        cancelButton->setEnabled(false);
        closeButton->setEnabled(true);
        portCombo->setEnabled(false);
        deviceCombo->setEnabled(true);
        break;

      case actionIdle: {
        DevicesPtr devtype = Devices::getType(deviceCombo->currentText());

        downloadButton->setEnabled(true);
        eraseRideButton->setEnabled(devtype->canCleanup());
        rescanButton->setEnabled(true);
        cancelButton->setEnabled(false);
        closeButton->setEnabled(true);
        portCombo->setEnabled(true);
        deviceCombo->setEnabled(true);
        break;
        }

      case actionDownload:
      case actionCleaning:
        downloadButton->setEnabled(false);
        eraseRideButton->setEnabled(false);
        rescanButton->setEnabled(false);
        cancelButton->setEnabled(true);
        closeButton->setEnabled(false);
        portCombo->setEnabled(false);
        deviceCombo->setEnabled(false);
        break;

    }

    action = newAction;
    cancelled = false;
    QCoreApplication::processEvents();
}

void
DownloadRideDialog::updateStatus(const QString &statusText)
{
    statusLabel->append(statusText);
    QCoreApplication::processEvents();
}

void
DownloadRideDialog::updateProgress( const QString &progressText )
{
    progressLabel->setText(progressText);
    QCoreApplication::processEvents();
}


void
DownloadRideDialog::deviceChanged( QString deviceType )
{
    (void)deviceType;

    updateAction(action); // adjust erase button visibility
    updatePort();
}

void
DownloadRideDialog::downloadClicked()
{
    updateAction( actionDownload );

    updateProgress( "" );
    statusLabel->setPlainText( "" );

    CommPortPtr dev;
    for (int i = 0; i < devList.size(); ++i) {
        if (devList[i]->id() == portCombo->currentText()) {
            dev = devList[i];
            break;
        }
    }
    assert(dev);
    QString err;
    QList<DeviceDownloadFile> files;

    DevicesPtr devtype = Devices::getType(deviceCombo->currentText());
    DevicePtr device = devtype->newDevice( dev );

    if( ! device->preview(
        boost::bind(&DownloadRideDialog::updateStatus, this, _1),
        err ) ){

        QMessageBox::information(this, tr("Preview failed"), err);
        updateAction( actionIdle );
        return;
    }

    QList<DeviceRideItemPtr> &rides( device->rides() );
    if( ! rides.empty() ){
        // XXX: let user select, which rides he wants to download
        for( int i = 0; i < rides.size(); ++i ){
            rides.at(i)->wanted = true;
        }
    }

    if (!device->download( home, files,
            boost::bind(&DownloadRideDialog::isCancelled, this),
            boost::bind(&DownloadRideDialog::updateStatus, this, _1),
            boost::bind(&DownloadRideDialog::updateProgress, this, _1),
            err))
    {
        if (cancelled) {
            QMessageBox::information(this, tr("Download canceled"),
                                     tr("Cancel clicked by user."));
            cancelled = false;
        }
        else {
            QMessageBox::information(this, tr("Download failed"), err);
        }
        updateStatus(tr("Download failed"));
        updateAction( actionIdle );
        return;
    }

    updateProgress( "" );

    int failures = 0;
    for( int i = 0; i < files.size(); ++i ){
        if( ! files.at(i).startTime.isValid() ){
            updateStatus(tr("file %1 has no valid timestamp, falling back to 'now'")
                .arg(files.at(i).name));
            files[i].startTime = QDateTime::currentDateTime();
        }

        QString filename( files.at(i).startTime
            .toString("yyyy_MM_dd_hh_mm_ss")
            + "." + files.at(i).extension );
        QString filepath( home.absoluteFilePath(filename) );

        if (QFile::exists(filepath)) {
            if (QMessageBox::warning( this,
                    tr("Ride Already Downloaded"),
                    tr("The ride starting at %1 appears to have already "
                        "been downloaded.  Do you want to overwrite the "
                        "previous download?")
                        .arg(files.at(i).startTime.toString()),
                    tr("&Overwrite"), tr("&Skip"),
                    QString(), 1, 1) == 1) {
                QFile::remove(files.at(i).name);
                updateStatus(tr("skipped file %1")
                    .arg( files.at(i).name ));
                continue;
            }
        }

#ifdef __WIN32__
        // Windows ::rename won't overwrite an existing file.
        if (QFile::exists(filepath)) {
            QFile old(filepath);
            if (!old.remove()) {
                QMessageBox::critical(this, tr("Error"),
                    tr("Failed to remove existing file %1: %2")
                        .arg(filepath)
                        .arg(old.error()) );
                QFile::remove(files.at(i).name);
                updateStatus(tr("failed to rename %1 to %2")
                    .arg( files.at(i).name )
                    .arg( filename ));
                ++failures;
                continue;
            }
        }
#endif

        // Use ::rename() instead of QFile::rename() to get forced overwrite.
        if (rename(QFile::encodeName(files.at(i).name),
            QFile::encodeName(filepath)) < 0) {

            QMessageBox::critical(this, tr("Error"),
                tr("Failed to rename %1 to %2: %3")
                    .arg(files.at(i).name)
                    .arg(filepath)
                    .arg(strerror(errno)) );
                updateStatus(tr("failed to rename %1 to %2")
                    .arg( files.at(i).name )
                    .arg( filename ));
            QFile::remove(files.at(i).name);
            ++failures;
            continue;
        }

        QFile::remove(files.at(i).name);
        mainWindow->addRide(filename);
    }

    if( ! failures )
        updateStatus( tr("download completed successfully") );

    updateAction( actionIdle );
}

void
DownloadRideDialog::eraseClicked()
{
    updateAction( actionCleaning );

    statusLabel->setPlainText( "" );
    updateProgress( "" );

    CommPortPtr dev;
    for (int i = 0; i < devList.size(); ++i) {
        if (devList[i]->id() == portCombo->currentText()) {
            dev = devList[i];
            break;
        }
    }
    assert(dev);
    DevicesPtr devtype = Devices::getType(deviceCombo->currentText());
    DevicePtr device = devtype->newDevice( dev );

    QString err;
    if( device->cleanup( err) )
        updateStatus( tr("cleaned data") );
    else
        updateStatus( err );

    updateAction( actionIdle );
}

void
DownloadRideDialog::cancelClicked()
{
    switch( action ){
      case actionIdle:
      case actionMissing:
        // do nothing
        break;

      default:
        cancelled = true;
        break;
     }
}

void
DownloadRideDialog::closeClicked()
{
    accept();
}


