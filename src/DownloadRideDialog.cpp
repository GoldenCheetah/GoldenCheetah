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
#include "MainWindow.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <QtGui>
#include <math.h>
#include <boost/bind.hpp>

DownloadRideDialog::DownloadRideDialog(MainWindow *mainWindow,
                                       const QDir &home) : 
    mainWindow(mainWindow), home(home), cancelled(false),
    downloadInProgress(false), recIntSecs(0.0), endingOffset(0)
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
    connect(rescanButton, SIGNAL(clicked()), this, SLOT(scanCommPorts()));
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

    scanCommPorts();
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
DownloadRideDialog::scanCommPorts()
{
    listWidget->clear();
    QString err;
    devList = CommPort::listCommPorts(err);
    if (err != "") {
        QString msg = "Warning:\n\n" + err + "You may need to (re)install "
            "the FTDI drivers before downloading.";
        QMessageBox::warning(0, "Error Loading Device Drivers", msg, 
                             QMessageBox::Ok, QMessageBox::NoButton);
    }
    for (int i = 0; i < devList.size(); ++i)
        new QListWidgetItem(devList[i]->name(), listWidget);
    if (listWidget->count() > 0) {
        listWidget->setCurrentRow(0);
        downloadButton->setFocus();
    }
    setReadyInstruct();
}

bool
DownloadRideDialog::statusCallback(PowerTapDevice::State state)
{
    if (state == PowerTapDevice::STATE_READING_VERSION)
        label->setText("Reading version...");
    else if (state == PowerTapDevice::STATE_READING_HEADER)
        label->setText(label->text() + "done.\nReading header...");
    else if (state == PowerTapDevice::STATE_READING_DATA) {
        label->setText(label->text() + "done.\nReading ride data...\n");
        endingOffset = label->text().length();
    }
    else {
        assert(state == PowerTapDevice::STATE_DATA_AVAILABLE);
        unsigned char *buf = records.data();
        bool bIsVer81 = PowerTapUtil::is_Ver81(buf);
        if (recIntSecs == 0.0) {
            for (int i = 0; i < records.size(); i += 6) {
                if (PowerTapUtil::is_config(buf + i, bIsVer81)) {
                    unsigned unused1, unused2, unused3;
                    PowerTapUtil::unpack_config(buf + i, &unused1, &unused2,
                                                &recIntSecs, &unused3,
                                                bIsVer81);
                }
            }
        }
        if (recIntSecs != 0.0) {
            int min = (int) round(records.size() / 6 * recIntSecs);
            QString existing = label->text();
            existing.chop(existing.size() - endingOffset);
            existing.append(QString("Ride data read: %1:%2").arg(min / 60)
                            .arg(min % 60, 2, 10, QLatin1Char('0')));
            label->setText(existing);
        }
        if (filename == "") {
            struct tm time;
            for (int i = 0; i < records.size(); i += 6) {
                if (PowerTapUtil::is_time(buf + i, bIsVer81)) {
                    PowerTapUtil::unpack_time(buf + i, &time, bIsVer81);
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
    CommPortPtr dev;
    for (int i = 0; i < devList.size(); ++i) {
        if (devList[i]->name() == listWidget->currentItem()->text()) {
            dev = devList[i];
            break;
        }
    }
    assert(dev);
    QString err;
    QByteArray version;
    if (!PowerTapDevice::download(
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

    QString tmpname;
    {
        // QTemporaryFile doesn't actually close the file on .close(); it
        // closes the file when in its destructor.  On Windows, we can't
        // rename an open file.  So let tmp go out of scope before calling
        // rename.

        QString tmpl = home.absoluteFilePath(".ptdl.XXXXXX");
        QTemporaryFile tmp(tmpl);
        tmp.setAutoRemove(false);
        if (!tmp.open()) {
            QMessageBox::critical(this, tr("Error"), 
                                  tr("Failed to create temporary file ")
                                  + tmpl + ": " + tmp.error());
            reject();
            return;
        }
        QTextStream os(&tmp);
        os << hex;
        os.setPadChar('0');

        struct tm time;
        bool time_set = false;
        unsigned char *data = records.data();
        bool bIsVer81 = PowerTapUtil::is_Ver81(data);

        for (int i = 0; i < records.size(); i += 6) {
            if (data[i] == 0 && !bIsVer81)
                continue;
            for (int j = 0; j < 6; ++j) {
                os.setFieldWidth(2);
                os << data[i+j];
                os.setFieldWidth(1);
                os << ((j == 5) ? "\n" : " ");
            }
            if (!time_set && PowerTapUtil::is_time(data + i, bIsVer81)) {
                PowerTapUtil::unpack_time(data + i, &time, bIsVer81);
                time_set = true;
            }
        }
        if (!time_set) {
            QMessageBox::critical(this, tr("Error"), 
                                  tr("Failed to find ride time."));
            tmp.setAutoRemove(true);
            reject();
            return;
        }
        tmpname = tmp.fileName(); // after close(), tmp.fileName() is ""
        tmp.close();

        // QTemporaryFile initially has permissions set to 0600.
        // Make it readable by everyone.
        tmp.setPermissions(tmp.permissions() 
                           | QFile::ReadOwner | QFile::ReadUser
                           | QFile::ReadGroup | QFile::ReadOther);
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

