/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#include "MoxyDevice.h"
#include "DownloadRideDialog.h"
#include "PowerTapUtil.h"
#include "Device.h"
#include "RideFile.h"

#include <cmath>
#include <errno.h>
#include <string.h>

static bool moxyRegistered =
    Devices::addType("Moxy Muscle Oxygen Monitor", DevicesPtr(new MoxyDevices()) );

QString
MoxyDevices::downloadInstructions() const
{
    return (tr("Make sure the Moxy is connected via USB"));
}

DevicePtr
MoxyDevices::newDevice( CommPortPtr dev )
{
    return DevicePtr( static_cast<Device *>(new MoxyDevice( dev )));
}

static QDateTime dateTimeForRow(QString line)
{
    // first row has data and time ?
    QStringList tokens = line.split(",");

    if (tokens.count() != 6 && tokens.count() !=7) return QDateTime();

    // is  the date in UTC Format ?
    // XXX we have no way of converting from UTC
    // XXX at present since we don't know what TZ
    // XXX the ride was recorded in ...
    bool UTCdate = tokens.count() == 7;
    Q_UNUSED(UTCdate);

    // parse date and time
    QStringList mmdd = tokens[0].split("-");
    int month = mmdd[0].toInt();
    int day = mmdd[1].toInt();
    int year = QDate::currentDate().year();

    QStringList hhmmss = tokens[1].split(":");
    int hh = hhmmss[0].toInt();
    int mm = hhmmss[1].toInt();
    int ss = hhmmss[2].toInt();

    return QDateTime(QDate(year,month,day), QTime(hh,mm,ss));
}

// we need to sort the rows by the timestamp since it is downloaded
// cycled round and we need to unpick that !
class MoxyData {

    public:
    QDateTime timestamp;
    QString line;

    bool operator<(const MoxyData &right) const { return timestamp < right.timestamp; }
};

bool
MoxyDevice::download( const QDir &tmpdir,
                         QList<DeviceDownloadFile> &files,
                         QString &err)
{ Q_UNUSED(tmpdir);
  Q_UNUSED(files);

    int bytes=0;
    char vbuf[256];

    QString verString;
    QString deviceInfo;

    emit updateProgress(QString(tr("Connecting ... \n")));

    if (!dev->open(err)) {
        err = tr("ERROR: open failed: ") + err;
        return false;
    }
    dev->setBaudRate(115200, err);

    // get into engineering mode
    if (writeCommand(dev, "\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    }

    if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) <= 0) {
        dev->close();
        return false;
    }

    if (writeCommand(dev, "\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    }

    if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) <= 0) {
        dev->close();
        return false;
    }

    // now get a prompt
    if (writeCommand(dev, "\r", err) == false) {
        dev->close();
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    }

    // get a prompt back ?
    if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) <= 0) {
        dev->close();
        return false;
    }

    // now get version
    if (writeCommand(dev, "ver\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    } else if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) > 0) {
        vbuf[bytes-1] = '\0';
        verString = vbuf;
        deviceInfo += vbuf;
    } else {
        dev->close();
        return false;
    }

    // now get time on unit
    if (writeCommand(dev, "time\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    } else if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) > 0) {
        vbuf[bytes-1] = '\0';
        deviceInfo += vbuf;
    } else {
        dev->close();
        return false;
    }

    // now get battery status
    if (writeCommand(dev, "batt\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    } else if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) > 0) {
        vbuf[bytes-1] = '\0';
        deviceInfo += vbuf;
    } else {
        dev->close();
        return false;
    }

    // now get update mode
    if (writeCommand(dev, "um\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    } else if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) > 0) {
        vbuf[bytes-1] = '\0';
        deviceInfo += vbuf;
    } else {
        dev->close();
        return false;
    }

    // now lets get the data
    if (writeCommand(dev, "gd\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    }

    emit updateProgress(QString(tr("Downloading ... \n")));

    QList<MoxyData> data;
    long count = 0;

    do {
        if ((bytes=readData(dev, vbuf, 256, err)) > 0) {

            count += bytes;
            updateProgress(QString(tr("Downloading ... [%1 bytes]")).arg(count));
            vbuf[bytes] = '\0';

            // ignore 'delimiters' (they're just noise)
            if (QString(vbuf) == " ,,,,," || QString(vbuf) == ",,,,,") continue;

            // get data
            QDateTime line = dateTimeForRow(vbuf);

            MoxyData row;
            row.line = vbuf;
            row.timestamp = line;
            data << row;

        } else break;

        // don't hang gui !
        QApplication::processEvents();

    } while(vbuf[0] != '>');

    emit updateProgress(QString(tr("Parsing ... ")));
    emit updateStatus(QString(tr("\nParsing ... ")));

    // we need to sort the data by timestamp since the moxy uses a circular
    // buffer which mean new lines arrive before old ones.
    qSort(data);

    // for deciding when to split rides
    QDateTime last = QDateTime();
    int rows=0;
    int nfiles=0;

    QTextStream os;
    QTemporaryFile *tmpFile = NULL; // the file being output

    // the download dialog structure
    DeviceDownloadFile file;
    file.extension = "csv";

    // run through the data creating rides
    foreach(MoxyData line, data) {

        // ignore badly formatted lines
        if (line.timestamp == QDateTime() || !line.timestamp.isValid()) continue;

        if (line.timestamp.toMSecsSinceEpoch() > (last.toMSecsSinceEpoch() + 1800000)) {

            // close last file
            if (tmpFile) {

                // does it have more than 30 rows?
                if (rows > 30) {

                    // we have one file anyway
                    emit updateStatus(QString(tr("File #%1: %2")).arg(++nfiles).arg(file.startTime.toString("d-MMM-yy hh:mm")));
                    tmpFile->close();
                    files << file;

                    // free memory, but leave on disk
                    tmpFile->setAutoRemove(false);
                    delete tmpFile;
                    tmpFile = NULL;

                } else {

                    // not enough data in this one
                    tmpFile->close();
                    tmpFile->setAutoRemove(true);
                    delete tmpFile;
                    tmpFile = NULL;
                }
                rows = 0;
            }

            QString tmpl = tmpdir.absoluteFilePath(".mxdl.XXXXXX");
            tmpFile = new QTemporaryFile(tmpl);
            if (!tmpFile->open()) {
                err = tr("Failed to create temporary file ") + tmpl + ": " + tmpFile->error();
                return false;
            }

            // QTemporaryFile initially has permissions set to 0600.
            // Make it readable by everyone.
            tmpFile->setPermissions(tmpFile->permissions() | QFile::ReadOwner | QFile::ReadUser 
                                                      | QFile::ReadGroup | QFile::ReadOther); 

            file.name = tmpFile->fileName();
            file.startTime = line.timestamp;

            os.setDevice(tmpFile);
            os<<verString;
            os<<"mm-dd,hh:mm:ss,SmO2 Live,SmO2 Averaged,THb,Lap\n";
        }

        // ok, so moxy csv has version string, header then line data
        os<<line.line;

        rows++;
        last = line.timestamp;
    }

    // close last file
    if (tmpFile) {

        // does it have more than 30 rows?
        if (rows > 30) {

            // we have one file anyway
            emit updateStatus(QString(tr("File #%1: %2")).arg(++nfiles).arg(file.startTime.toString("d-MMM-yy hh:mm")));
            tmpFile->close();
            files << file;

            // free memory, but leave on disk
            tmpFile->setAutoRemove(false);
            delete tmpFile;
            tmpFile = NULL;

        } else {

            // not enough data in this one
            tmpFile->close();
            tmpFile->setAutoRemove(true);
            delete tmpFile;
            tmpFile = NULL;
        }
    }
    
    // exit
    if (writeCommand(dev, "exit\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    }

    // close device
    dev->close();

    if (files.count() > 0)  {
        emit updateProgress("Importing ...");
        emit updateStatus(QString(tr("\nImporting %1 Ride(s)... \n")).arg(files.count()));

    } else emit updateStatus(QString(tr("\nNo rides found to import. \n")));

    // success !
    return true;
}


bool
MoxyDevice::cleanup( QString &err )
{
    int bytes=0;
    char vbuf[256];
  
    emit updateStatus(tr("Erase all records on Moxy"));

    if (!dev->open(err)) {
        err = tr("ERROR: open failed: ") + err;
        return false;
    }
    dev->setBaudRate(115200, err);

    // get into engineering mode
    if (writeCommand(dev, "\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    }

    if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) <= 0) {
        dev->close();
        return false;
    }

    if (writeCommand(dev, "\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    }

    if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) <= 0) {
        dev->close();
        return false;
    }

    // now get a prompt
    if (writeCommand(dev, "\r", err) == false) {
        dev->close();
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        return false;
    }

    // get a prompt back ?
    if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) <= 0) {
        dev->close();
        return false;
    }

    // now clear the entries
    if (writeCommand(dev, "cd\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    } else if ((bytes=readUntilPrompt(dev, vbuf, 256, err)) > 0) {
        vbuf[bytes-1] = '\0';
        emit updateStatus(vbuf);
    } else {
        dev->close();
        return false;
    }

    // give it some time to get the work done !
    sleep(3);

    // exit engineering mode
    if (writeCommand(dev, "exit\r", err) == false) {
        emit updateStatus(QString(tr("Write error: %1\n")).arg(err));
        dev->close();
        return false;
    }

    // close device
    dev->close();

    // success !
    return true;
}

static bool
hasNewline(const char *buf, int len)
{
    static char newline[] = { 0x0d, 0x0a };
    if (len < 2)
        return false;
    for (int i = 0; i < len; ++i) {
        bool success = true;
        for (int j = 0; j < 2; ++j) {
            if (buf[i+j] != newline[j]) {
                success = false;
                break;
            }
        }
        if (success)
            return true;
    }
    return false;
}

static QString
cEscape(const char *buf, int len)
{
    char *result = new char[4 * len + 1];
    char *tmp = result;
    for (int i = 0; i < len; ++i) {
        if (buf[i] == '"')
            tmp += sprintf(tmp, "\\\"");
        else if (isprint(buf[i]))
            *(tmp++) = buf[i];
        else
            tmp += sprintf(tmp, "\\x%02x", 0xff & (unsigned) buf[i]);
    }
    return result;
}

// read the entire response after a command issued it ends with
// a command prompt
int
MoxyDevice::readUntilPrompt(CommPortPtr dev, char *buf, int len, QString &err)
{

    int sofar = 0;
    while (sofar < len) {
        // Read one byte at a time to avoid waiting for timeout.
        int n = dev->read(buf + sofar, 1, err);
        if (n <= 0) {
            err = (n < 0) ? (tr("read error: ") + err) : tr("read timeout");
            err += QString(tr(", read %1 bytes so far: \"%2\""))
                .arg(sofar).arg(sofar > 0 ? cEscape(buf, sofar) : "");
            return -1;
        }

        sofar += n;

        // we got our prompt
        if (*(buf+sofar-1) == '>') break;
    }

    return sofar;
}

// just read whatever we get before the new line
int
MoxyDevice::readUntilNewline(CommPortPtr dev, char *buf, int len, QString &err)
{
    int sofar = 0;
    while (!hasNewline(buf, sofar) && sofar < len) {
        // Read one byte at a time to avoid waiting for timeout.
        int n = dev->read(buf + sofar, 1, err);
        if (n <= 0) {
            err = (n < 0) ? (tr("read error: ") + err) : tr("read timeout");
            err += QString(tr(", read %1 bytes so far: \"%2\""))
                .arg(sofar).arg(cEscape(buf, sofar));
            return -1;
        }
        sofar += n;
    }
    return sofar;
}

// read a line of data, if it starts with a prompt ">" then its done
int
MoxyDevice::readData(CommPortPtr dev, char *buf, int len, QString &err)
{
    int sofar = 0;
    while (sofar < len) {
        // Read one byte at a time to avoid waiting for timeout.
        int n = dev->read(buf + sofar, 1, err);
        if (n <= 0) {
            err = (n < 0) ? (tr("read error: ") + err) : tr("read timeout");
            err += QString(tr(", read %1 bytes so far: \"%2\""))
                .arg(sofar).arg(cEscape(buf, sofar));
            return -1;
        }

        sofar += n;

        if (hasNewline(buf, sofar) || *buf == '>') {
            break;
        }
    }
    return sofar;
}

bool
MoxyDevice::writeCommand(CommPortPtr dev, const char *command, QString &err)
{
    // on qt4 we need to waste some cycles
    msleep(100);

    int len = strlen(command);
    int n = dev->write(const_cast<char*>(command), len, err);

    if (n != len) {
        if (n < 0)
            err = QString(tr("failed to write '%1' to device: %2")).arg(const_cast<char*>(command)).arg(err);
        else
            err = QString(tr("timeout writing '%1' to device")).arg(const_cast<char*>(command));
        return false;
    }
    return true;
}
