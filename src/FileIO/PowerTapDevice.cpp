/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

#include "PowerTapDevice.h"
#include "PowerTapUtil.h"
#include <cmath>
#include "assert.h"

#define PT_DEBUG false

static bool powerTapRegistered =
    Devices::addType("PowerTap", DevicesPtr(new PowerTapDevices()) );

DevicePtr
PowerTapDevices::newDevice( CommPortPtr dev )
{
    return DevicePtr( new PowerTapDevice( dev ));
}

QString
PowerTapDevices::downloadInstructions() const
{
    return (tr("Make sure the PowerTap unit is turned\n"
            "on and that its display says, \"Host\""));
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

bool
PowerTapDevice::doWrite(CommPortPtr dev, char c, bool hwecho, QString &err)
{
    if (PT_DEBUG) printf("writing '%c' to device\n", c);
    int n = dev->write(&c, 1, err);
    if (n != 1) {
        if (n < 0)
            err = QString(tr("failed to write %1 to device: %2")).arg(c).arg(err);
        else
            err = QString(tr("timeout writing %1 to device")).arg(c);
        return false;
    }
    if (hwecho) {
        char c;
        int n = dev->read(&c, 1, err);
        if (n != 1) {
            if (n < 0)
                err = QString(tr("failed to read back hardware echo: %2")).arg(err);
            else
                err = tr("timeout reading back hardware echo");
            return false;
        }
    }
    return true;
}

int
PowerTapDevice::readUntilNewline(CommPortPtr dev, char *buf, int len, QString &err)
{
    Q_UNUSED(len);
    int sofar = 0;
    while (!hasNewline(buf, sofar)) {
        assert(sofar < len);
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

bool
PowerTapDevice::download( const QDir &tmpdir,
                         QList<DeviceDownloadFile> &files,
                         QString &err)
{
    if (!dev->open(err)) {
        err = tr("ERROR: open failed: ") + err;
        return false;
    }
    // make several attempts at reading the version
    int attempts = 3;
    int veridx = -1;
    int version_len;
    char vbuf[256];
    QByteArray version;

    do {
	if (!doWrite(dev, 0x56, false, err)) // 'V'
	    return false;

    emit updateStatus( tr("Reading version...") );
    if(m_Cancelled) {
        err = tr("download cancelled");
	    return false;
	}

	version_len = readUntilNewline(dev, vbuf, sizeof(vbuf), err);
	if (version_len < 0) {
	    err = tr("Error reading version: ") + err;
	    return false;
	}
	if (PT_DEBUG) {
	    printf("read version \"%s\"\n",
		   cEscape(vbuf, version_len).toLatin1().constData());
	}
	version = QByteArray(vbuf, version_len);

	// We expect the version string to be something like
	// "VER 02.21 PRO...", so if we see two V's, it's probably
	// because there's a hardware echo going on.
	veridx = version.indexOf("VER");

    } while ((--attempts > 0) && (veridx < 0));

    if (veridx < 0) {
	err = QString(tr("Unrecognized version \"%1\""))
	    .arg(cEscape(vbuf, version_len));
	return false;
    }
    bool hwecho = version.indexOf('V') < veridx;
    if (PT_DEBUG) printf("hwecho=%s\n", hwecho ? "true" : "false");

    emit updateStatus( tr("Reading header...") );
    if(m_Cancelled) {
        err = tr("download cancelled");
        return false;
    }

    if (!doWrite(dev, 0x44, hwecho, err)) // 'D'
        return false;
    unsigned char header[6];
    int header_len = dev->read(header, sizeof(header), err);
    if (header_len != 6) {
        if (header_len < 0)
            err = tr("ERROR: reading header: ") + err;
        else
            err = tr("ERROR: timeout reading header");
        return false;
    }
    if (PT_DEBUG) {
        printf("read header \"%s\"\n",
               cEscape((char*) header,
                       sizeof(header)).toLatin1().constData());
    }
    QVector<unsigned char> records;
    for (size_t i = 0; i < sizeof(header); ++i)
        records.append(header[i]);

    emit updateStatus( tr("Reading data...") );
    if(m_Cancelled) {
        err = tr("download cancelled");
        return false;
    }
    double recIntSecs = 0.0;

    fflush(stdout);
    while (true) {
        if (PT_DEBUG) printf("reading block\n");
        unsigned char buf[256 * 6 + 1];
        int n = dev->read(buf, 2, err);
        if (n < 2) {
            if (n < 0)
                err = tr("ERROR: reading first two: ") + err;
            else
                err = tr("ERROR: timeout reading first two");
            return false;
        }
        if (PT_DEBUG) {
            printf("read 2 bytes: \"%s\"\n",
                   cEscape((char*) buf, 2).toLatin1().constData());
        }
        if (hasNewline((char*) buf, 2))
            break;
        unsigned count = 2;
        while (count < sizeof(buf)) {
            n = dev->read(buf + count, sizeof(buf) - count, err);
            if (n < 0) {
                err = tr("ERROR: reading block: ") + err;
                return false;
            }
            if (n == 0) {
                err = tr("ERROR: timeout reading block");
                return false;
            }
            if (PT_DEBUG) {
                printf("read %d bytes: \"%s\"\n", n,
                       cEscape((char*) buf + count, n).toLatin1().constData());
            }
            count += n;
        }
        unsigned csum = 0;
        for (int i = 0; i < ((int) sizeof(buf)) - 1; ++i)
            csum += buf[i];
        if ((csum % 256) != buf[sizeof(buf) - 1]) {
            err = tr("ERROR: bad checksum");
            return false;
        }
        if (PT_DEBUG) printf("good checksum\n");
        for (size_t i = 0; i < sizeof(buf) - 1; ++i)
            records.append(buf[i]);
        if (recIntSecs == 0.0) {
            unsigned char *data = records.data();
            bool bIsVer81 = PowerTapUtil::is_Ver81(data);
            for (int i = 0; i < records.size(); i += 6) {
                if (PowerTapUtil::is_config(data + i, bIsVer81)) {
                    unsigned unused1, unused2, unused3;
                    PowerTapUtil::unpack_config(
                        data + i, &unused1, &unused2,
                        &recIntSecs, &unused3, bIsVer81);
                }
            }
        }
        if (recIntSecs != 0.0) {
            int min = (int) round(records.size() / 6 * recIntSecs);
            emit updateProgress( QString(tr("progress: %1:%2"))
                .arg(min / 60)
                .arg(min % 60, 2, 10, QLatin1Char('0')));
        }
        if(m_Cancelled){
            err = tr("download cancelled");
            return false;
        }
        if (!doWrite(dev, 0x71, hwecho, err)) // 'q'
            return false;
    }

    QString tmpl = tmpdir.absoluteFilePath(".ptdl.XXXXXX");
    QTemporaryFile tmp(tmpl);
    tmp.setAutoRemove(false);
    if (!tmp.open()) {
        err = tr("Failed to create temporary file ")
            + tmpl + ": " + tmp.error();
        return false;
    }
    // QTemporaryFile initially has permissions set to 0600.
    // Make it readable by everyone.
    tmp.setPermissions(tmp.permissions()
                       | QFile::ReadOwner | QFile::ReadUser
                       | QFile::ReadGroup | QFile::ReadOther);

    DeviceDownloadFile file;
    file.extension = "raw";
    file.name = tmp.fileName();

    QTextStream os(&tmp);
    os << Qt::hex;
    os.setPadChar('0');

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
            struct tm time;
            time_t timet = PowerTapUtil::unpack_time(data + i, &time, bIsVer81);
            file.startTime.setSecsSinceEpoch( timet );
            time_set = true;
        }
    }
    if (!time_set) {
        err = tr("Failed to find start time.");
        tmp.setAutoRemove(true);
        return false;
    }

    // hack for CERVO bug dates >= 2016 set the year to 2000 (!)
    if (file.startTime.date().year() == 2000) {

        // it's in the past... so set year to this
        QDate date(QDate::currentDate().year(), file.startTime.date().month(), file.startTime.date().day());

        // is that a future date?
        if (date > QDate::currentDate()) date = date.addYears(-1);

        // now update for file, if it's valid otherwise keep as it is
        if(date.isValid()) file.startTime.setDate(date);
    }

    files << file;
    return true;
}
