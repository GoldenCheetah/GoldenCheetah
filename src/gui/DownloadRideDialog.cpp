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
#include <sys/types.h>
#include <sys/stat.h>
#include <boost/bind.hpp>

#define MAX_DEVICES 10

DownloadRideDialog::DownloadRideDialog(MainWindow *mainWindow,
                                       const QDir &home) : 
    mainWindow(mainWindow), home(home), /*fd(-1), out(NULL), device(NULL), 
    notifier(NULL), timer(NULL), blockCount(0),*/ cancelled(false),
    downloadInProgress(false), recInt(0), endingOffset(0)
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
            SIGNAL(itemSelectionChanged()), this, SLOT(setReadyInstruct()));
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
    /*if (device)
        free(device);
    if (fd >= 0)
        ::close(fd);
    if (out)
        fclose(out);*/
}

void 
DownloadRideDialog::setReadyInstruct()
{
    int selected = listWidget->selectedItems().size();
    assert(selected <= 1);
    if (selected == 0) {
        if (listWidget->count() > 1) {
            label->setText(tr("Select the device from the above list from\n"
                              "which you would like to download a ride."));
        }
        else {
            label->setText(tr("No devices found.  Make sure the PowerTap\n"
                              "unit is plugged into the computer's USB port,\n"
                              "then click \"Rescan\" to check again."));
        }
        downloadButton->setEnabled(false);
    }
    else {
        label->setText(tr("Make sure the PowerTap unit is turned on,\n"
                          "and that the screen display says, \"Host\",\n"
                          "then click Download to begin downloading."));
        downloadButton->setEnabled(true);
    }
}

void
DownloadRideDialog::scanDevices()
{
    listWidget->clear();
    QString err;
    devList = Device::listDevices(err);
    for (int i = 0; i < devList.size(); ++i)
        new QListWidgetItem(devList[i]->name(), listWidget);
    if (listWidget->count() > 0) {
        listWidget->setCurrentRow(0);
        downloadButton->setFocus();
    }
    setReadyInstruct();
}

/*
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
        fd = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
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
*/

bool
DownloadRideDialog::statusCallback(PowerTap::State state)
{
    switch (state) {
        case PowerTap::STATE_READING_VERSION:
            label->setText("Reading version...");
            break;
        case PowerTap::STATE_READING_HEADER:
            label->setText(label->text() + "done.\nReading header...");
            break;
        case PowerTap::STATE_READING_DATA:
            label->setText(label->text() + "done.\nReading ride data...\n");
            endingOffset = label->text().length();
            break;
        case PowerTap::STATE_DATA_AVAILABLE:
            unsigned char *buf = records.data();
            if (recInt == 0.0) {
                for (int i = 0; i < records.size(); i += 6) {
                    if (pt_is_config(buf + i)) {
                        unsigned unused1, unused2, unused3;
                        pt_unpack_config(buf + i, &unused1, &unused2,
                                         &recInt, &unused3);
                    }
                }
            }
            if (recInt != 0.0) {
                int min = (int) round(records.size() / 6 * recInt * 0.021);
                printf("records.size()=%d, min=%d\n", records.size(), min);
                QString existing = label->text();
                existing.chop(existing.size() - endingOffset);
                existing.append(QString("Ride data read: %1:%2").arg(min / 60)
                                .arg(min % 60, 2, 10, QLatin1Char('0')));
                label->setText(existing);
            }
            if (filename == "") {
                struct tm time;
                for (int i = 0; i < records.size(); i += 6) {
                    if (pt_is_time(buf + i)) {
                        pt_unpack_time(buf + i, &time);
                        char tmp[32];
                        sprintf(tmp, "%04d_%02d_%02d_%02d_%02d_%02d.raw", 
                                time.tm_year + 1900, time.tm_mon + 1, 
                                time.tm_mday, time.tm_hour, time.tm_min,
                                time.tm_sec);
                        filename = tmp;
                        filepath = home.absolutePath() + "/" + filename;
                        FILE *out = fopen(filepath.toAscii().constData(), "r"); 
                        if (out) {
                            fclose(out);
                            if (QMessageBox::warning(
                                    this,
                                    tr("Ride Already Downloaded"),
                                    tr("This ride appears to have already ")
                                    + tr("been downloaded.  Do you want to ")
                                    + tr("download it again and overwrite ")
                                    + tr("the previous download?"),
                                    tr("&Overwrite"), tr("&Cancel"), 
                                    QString(), 1, 1) == 1) {
                                reject();
                                return false;
                            }
                        }
                        break;
                    }
                }
            }
            break;
        default:
            assert(false);
    }
    QCoreApplication::processEvents();
    return !cancelled;
}

void 
DownloadRideDialog::downloadClicked()
{
    downloadButton->setEnabled(false);
    rescanButton->setEnabled(false);
    downloadInProgress = true;
    DevicePtr dev;
    for (int i = 0; i < devList.size(); ++i) {
        if (devList[i]->name() == listWidget->currentItem()->text()) {
            dev = devList[i];
            break;
        }
    }
    assert(dev);
    QString err;
    QByteArray version;
    if (!PowerTap::download(
            dev, version, records,
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
    char *tmpname = new char[home.absolutePath().length() + 32];
    sprintf(tmpname, "%s/.ptdl.XXXXXX",
            home.absolutePath().toAscii().constData());
    int fd = mkstemp(tmpname);
    if (fd == -1)
        assert(false);
    FILE *file = fdopen(fd, "w");
    if (file == NULL)
        assert(false);
    unsigned char *data = records.data();
    struct tm time;
    bool time_set = false;

    for (int i = 0; i < records.size(); i += 6) {
        if (data[i] == 0)
            continue;
        fprintf(file, "%02x %02x %02x %02x %02x %02x\n", 
                data[i], data[i+1], data[i+2],
                data[i+3], data[i+4], data[i+5]); 
        if (!time_set && pt_is_time(data + i)) {
            pt_unpack_time(data + i, &time);
            time_set = true;
        }
    }
    fclose(file);
    assert(time_set);

    if (chmod(tmpname, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) == -1) {
        perror("chmod");
        assert(false);
    }

    if (rename(tmpname, filepath.toAscii().constData()) == -1) {
        perror("rename");
        assert(false);
    }
    delete [] tmpname;

    QMessageBox::information(this, tr("Success"), tr("Download complete."));
    mainWindow->addRide(filename);
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

