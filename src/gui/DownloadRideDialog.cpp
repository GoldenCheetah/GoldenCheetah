/* 
 * $Id: DownloadRideDialog.cpp,v 1.4 2006/08/11 20:02:13 srhea Exp $
 *
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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
#include "MainWindow.h"
#include <assert.h>
#include <QtGui>
#include <errno.h>
#include <fcntl.h>
#include <math.h>

#define MAX_DEVICES 10

DownloadRideDialog::DownloadRideDialog(MainWindow *mainWindow,
                                       const QDir &home) : 
    mainWindow(mainWindow), home(home), fd(-1), out(NULL), device(NULL), 
    notifier(NULL), timer(NULL), blockCount(0)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Download Ride Data");

    listWidget = new QListWidget(this);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    QLabel *availLabel = new QLabel(tr("Available devices:"), this);
    QLabel *instructLabel = new QLabel(tr("Instructions:"), this);
    label = new QLabel(this);
    label->setIndent(10);

    downloadButton = new QPushButton(tr("&Download"), this);
    rescanButton = new QPushButton(tr("&Rescan"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);

    connect(downloadButton, SIGNAL(clicked()), this, SLOT(downloadClicked()));
    connect(rescanButton, SIGNAL(clicked()), this, SLOT(scanDevices()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(listWidget, 
            SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)),
            this, SLOT(setReadyInstruct()));
    connect(listWidget, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(downloadClicked())); 

    QHBoxLayout *buttonLayout = new QHBoxLayout; 
    buttonLayout->addWidget(downloadButton); 
    buttonLayout->addWidget(rescanButton); 
    buttonLayout->addWidget(cancelButton); 

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(availLabel);
    mainLayout->addWidget(listWidget);
    mainLayout->addWidget(instructLabel);
    mainLayout->addWidget(label);
    mainLayout->addLayout(buttonLayout);

    scanDevices();
}

DownloadRideDialog::~DownloadRideDialog()
{
    if (device)
        free(device);
    if (fd >= 0)
        ::close(fd);
    if (out)
        fclose(out);
}

void 
DownloadRideDialog::setReadyInstruct()
{
    label->setText(tr("Make sure the PowerTap unit is turned on,\n"
                      "and that the screen display says, \"Host\",\n"
                      "then click Download to begin downloading."));
}

void
DownloadRideDialog::scanDevices()
{
    listWidget->clear();
    char *devices[MAX_DEVICES];
    int devcnt = pt_find_device(devices, MAX_DEVICES);
    for (int i = 0; i < devcnt; ++i) {
        new QListWidgetItem(devices[i], listWidget);
        free(devices[i]);
    }
    if (listWidget->count() == 1) {
        listWidget->setCurrentRow(0);
        setReadyInstruct();
        downloadButton->setEnabled(true);
        downloadButton->setFocus();
    }
    else {
        downloadButton->setEnabled(false);
        if (listWidget->count() > 1) {
            label->setText(tr("Select the device from the above list from\n"
                              "which you would like to download a ride."));
        }
        else {
            label->setText(tr("No devices found.  Make sure the PowerTap\n"
                              "unit is plugged into the computer's USB port,\n"
                              "then click \"Rescan\" to check again."));
        }
    }
}

static void
time_cb(struct tm *time, void *self) 
{
    ((DownloadRideDialog*) self)->time_cb(time);
}

static void
record_cb(unsigned char *buf, void *self) 
{
    ((DownloadRideDialog*) self)->record_cb(buf);
}

void
DownloadRideDialog::time_cb(struct tm *time)
{
    timer->stop();
    if (!out) {
        if (!time) {
            QMessageBox::critical(this, tr("Read error"), 
                                  tr("Can't find ride time"));
            reject();
            return;
        } 
        sprintf(outname, "%04d_%02d_%02d_%02d_%02d_%02d.raw", 
                time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, 
                time->tm_hour, time->tm_min, time->tm_sec);
        assert(strlen(outname) == sizeof(outname) - 1);
        label->setText(label->text() + tr("done.\nWriting to ") 
                       + outname + ".");
        std::string path = home.absolutePath().toStdString() + "/" + outname;
        if ((out = fopen(path.c_str(), "r")) != NULL) {
            if (QMessageBox::warning(this,
                                     tr("Ride Already Downloaded"),
                                     tr("This ride appears to have already ")
                                     + tr("been downloaded.  Do you want to ")
                                     + tr("download it again and overwrite ")
                                     + tr("the previous download?"),
                                     tr("&Overwrite"), tr("&Cancel"), 
                                     QString(), 1, 1) == 1) {
                reject();
                return;
            }
        }
        if ((out = fopen(path.c_str(), "w")) == NULL) {
            QMessageBox::critical(this, tr("Write error"), 
                                  tr("Can't open ") + path.c_str() 
                                  + tr(" for writing: ") + strerror(errno));
            reject();
            return;
        }
        label->setText(label->text() + tr("\nRide data read: "));
        endingOffset = label->text().size();
    }
    timer->start(5000);
}

void
DownloadRideDialog::record_cb(unsigned char *buf) 
{
    timer->stop();
    if (!out) {
        QMessageBox::critical(this, tr("Read error"), 
                              tr("Can't find ride time."));
        reject();
    }
    for (int i = 0; i < 6; ++i)
        fprintf(out, "%02x%s", buf[i], (i == 5) ? "\n" : " ");
    if ((++blockCount % 256) == 0) {
        QString existing = label->text();
        existing.chop(existing.size() - endingOffset);
        int minutes = (int) round(blockCount * 0.021);
        existing.append(QString("%1:%2").arg(minutes / 60)
                        .arg(minutes % 60, 2, 10, QLatin1Char('0')));
        label->setText(existing);
        repaint();
    }
    timer->start(5000);
}

void
DownloadRideDialog::readVersion()
{
    if (notifier)
        notifier->setEnabled(false);
    int r = pt_read_version(&vstate, fd, &hwecho);
    if (r == PT_DONE) {
        if (notifier) {
            delete notifier;
            notifier = NULL;
        }
        if (timer) {
            delete timer;
            timer = NULL;
        }
        label->setText(label->text() + tr("done."));
        ::close(fd);
        fd = open(device, O_RDWR | O_NOCTTY);
        if (fd < 0) {
            QMessageBox::critical(this, tr("Read error"), 
                                  tr("Could not open device, ") + device +
                                  + ": " + strerror(errno));
            reject();
        }
        pt_make_async(fd);
        label->setText(label->text() + tr("\nReading ride time..."));
        memset(&dstate, 0, sizeof(dstate));
        readData();
    }
    else {
        assert(r == PT_NEED_READ);
        if (notifier)
            notifier->setEnabled(true);
        else {
            notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
            connect(notifier, SIGNAL(activated(int)), this, SLOT(readVersion()));
        }
        if (!timer) {
            timer = new QTimer(this);
            connect(timer, SIGNAL(timeout()), this, SLOT(versionTimeout()));
            timer->start(5000);
        }
    }
}

void
DownloadRideDialog::versionTimeout()
{
    timer->stop();
    label->setText(label->text() + tr("timeout."));
    QMessageBox::critical(this, tr("Read Error"), 
                          tr("Timed out reading device ") + device
                          + tr(".  Check that the device is plugged in and ")
                          + tr("try again."));
    reject();
}

void
DownloadRideDialog::readData()
{
    if (notifier)
        notifier->setEnabled(false);
    int r = pt_read_data(&dstate, fd, hwecho, ::time_cb, ::record_cb, this);
    if (r == PT_DONE) {
        if (notifier) {
            delete notifier;
            notifier = NULL;
        }
        if (timer) {
            delete timer;
            timer = NULL;
        }
        // label->setText(label->text() + tr("done."));
        QMessageBox::information(this, tr("Success"), tr("Download complete."));
        fclose(out);
        out = NULL;
        mainWindow->addRide(outname);
        accept();
    }
    else {
        assert(r == PT_NEED_READ);
        if (notifier)
            notifier->setEnabled(true);
        else {
            notifier = new QSocketNotifier(fd, QSocketNotifier::Read, this);
            connect(notifier, SIGNAL(activated(int)), this, SLOT(readData()));
        }
        if (!timer) {
            timer = new QTimer(this);
            connect(timer, SIGNAL(timeout()), this, SLOT(versionTimeout()));
            timer->start(5000);
        }
    }
}

void 
DownloadRideDialog::downloadClicked()
{
    downloadButton->setEnabled(false);
    rescanButton->setEnabled(false);
    if (device)
        free(device);
    device = strdup(listWidget->currentItem()->text().toAscii().data());
    hwecho = 0;
    fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        QMessageBox::critical(this, tr("Read error"), 
                              tr("Could not open device, ") + device +
                              + ": " + strerror(errno));
        reject();
    }
    pt_make_async(fd);
    label->setText(tr("Reading version information..."));
    memset(&vstate, 0, sizeof(vstate));
    readVersion();
}

void 
DownloadRideDialog::cancelClicked()
{
    reject();
}

