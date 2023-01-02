/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com),
 *               2018 Florian Nairz (nairz.florian@gmail.com)
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

#include "Daum.h"

Daum::Daum(QObject *parent, QString device, QString profile) : QThread(parent),
    timer_(nullptr),
    serialDeviceName_(device),
    serial_dev_(nullptr),
    deviceAddress_(-1),
    maxDeviceLoad_(800),
    serialWriteDelay_(0),
    playSound_(profile.contains("sound", Qt::CaseInsensitive)),
    paused_(false),
    devicePower_(0),
    deviceHeartRate_(0),
    deviceCadence_(0),
    deviceSpeed_(0),
    load_(kDefaultLoad),
    loadToWrite_(kDefaultLoad),
    forceUpdate_(profile.contains("force", Qt::CaseInsensitive)),
    profile_(profile) {
}

int Daum::start() {
    QThread::start();
    return isRunning() ? 0 : 1;
}
int Daum::restart() {
    QMutexLocker locker(&pvars);
    paused_ = true;
    return 0;
}
int Daum::pause() {
    QMutexLocker locker(&pvars);
    paused_ = true;
    return 0;
}
int Daum::stop() {
    exit(-1);
    return 0;
}

bool Daum::discover(QString dev) {
    if (!openPort(dev)) {
        return false;
    }

    QSerialPort &s(*serial_dev_);
    QByteArray data;
    data.append(0x11);

    s.write(data);
    if (!s.waitForBytesWritten(1000)) {
        return false;
    }
    if (!s.waitForReadyRead(1000)) {
        return false;
    }
    data = s.read(2);
    if ((int)data[0] != 0x11) {
        return false;
    }
    data = s.readAll();
    closePort();

    return true;
}

void Daum::setLoad(double load) {
    const unsigned int minDeviceLoad = 25;
    unsigned int local_load = (unsigned int)load;
    QMutexLocker locker(&pvars);
    if (local_load > maxDeviceLoad_) { local_load = maxDeviceLoad_; }
    if (local_load < minDeviceLoad) { local_load = minDeviceLoad; }
    qDebug() << "setLoad(): " << local_load;
    loadToWrite_ = local_load;
}

double Daum::getPower() const {
    QMutexLocker locker(&pvars);
    return devicePower_;
}
double Daum::getSpeed() const {
    QMutexLocker locker(&pvars);
    return deviceSpeed_;
}
double Daum::getCadence() const {
    QMutexLocker locker(&pvars);
    return deviceCadence_;
}
double Daum::getHeartRate() const {
    QMutexLocker locker(&pvars);
    return deviceHeartRate_;
}

bool Daum::openPort(QString dev) {
    QMutexLocker locker(&pvars);
    if (serial_dev_ == nullptr) {
        serial_dev_ = new QSerialPort();
    }
    if (serial_dev_->isOpen()) {
        serial_dev_->close();
    }

    serial_dev_->setPortName(dev);
    if (profile_ == "OLD_DAUM"){
        serial_dev_->setBaudRate(QSerialPort::Baud4800);  // Old cockpits use 4800 Baud
    }else{
        serial_dev_->setBaudRate(QSerialPort::Baud9600);
    }
    serial_dev_->setStopBits(QSerialPort::OneStop);
    serial_dev_->setDataBits(QSerialPort::Data8);
    serial_dev_->setFlowControl(QSerialPort::NoFlowControl);
    serial_dev_->setParity(QSerialPort::NoParity);

    if (!serial_dev_->open(QSerialPort::ReadWrite)) {
       return false;
    }

    return true;
}

bool Daum::closePort() {
    QMutexLocker locker(&pvars);
    delete serial_dev_;
    serial_dev_ = nullptr;
    return true;
}

void Daum::run() {
    //closePort();
    if (!openPort(serialDeviceName_)) {
        exit(-1);
    }

    {
        QMutexLocker locker(&pvars);
        if (timer_ == nullptr) {
            timer_ = new QTimer();

            connect(this, SIGNAL(finished()), timer_, SLOT(stop()), Qt::DirectConnection);
            connect(timer_, SIGNAL(timeout()), this, SLOT(requestRealtimeData()), Qt::DirectConnection);
        }

        // discard prev. read data
        serial_dev_->readAll();
    }

    initializeConnection();

    // setup polling
    {
        QMutexLocker locker(&pvars);
        timer_->setInterval(kQueryIntervalMS);
        timer_->start();
    }

    StartProgram(0);

    // enter event loop and wait for a call to quit() or exit()
    exec();

    {
        QMutexLocker locker(&pvars);
        timer_->stop();
    }
}

void Daum::initializeConnection() {

    serialWriteDelay_ = 0;

    char addr = (char)GetAddress();
    {
        QMutexLocker locker(&pvars);
        deviceAddress_ = addr;
    }
    if (addr < 0) {
        qWarning() << "unable to detect device address";
        exit(-1);
    }

    QThread::msleep(100);

    // reset device
    if (!ResetDevice()) {
        qWarning() << "reset device failed";
    }

    QThread::msleep(100);

    // unused so far
    qDebug() << "CheckCockpit() returned " << CheckCockpit();

    QThread::msleep(100);

    int dat;
    if (profile_ == "OLD_DAUM"){
        qDebug() << "Daum cockpit: using old serial interface type";
        dat = COCKPIT_8008;   // Old cockpits do not have a version information. Use 8008 timing
    }else{
        // check version info for know devices
        dat = GetDeviceVersion();
    };

    if (configureForCockpitType(dat)) {
        qDebug() << "Daum cockpit type: " << Qt::hex << dat;
        qDebug() << "  Using communication delay:" << serialWriteDelay_ << "msec";
        qDebug() << "  Playing sound:" << playSound_;
    } else {
        qWarning() << "unable to identify daum cockpit type" << Qt::hex << dat;
        exit(-1);
    }
    
    if (profile_ != "OLD_DAUM"){ // Function not available
        if (!SetDate()) {
            qWarning() << "set date failed";
        }
        if (!SetTime()) {
            qWarning() << "set time failed";
        }
    }

    if (!SetProgram(0)) {
        qWarning() << "setting program failed";
    }

    if (!StartProgram(0)) {
        qWarning() << "starting program failed";
    }

    PlaySound();
}

bool Daum::configureForCockpitType(int cockpitType) {
    serialWriteDelay_ = 0;

    if (configureFromProfile()) {
        // the profile string contains a valid configuration
        return true;
    }

    switch (cockpitType) {
    case COCKPIT_CARDIO:
    case COCKPIT_FITNESS:
    case COCKPIT_VITA_DE_LUXE:
    case COCKPIT_UNKNOWN:
    case COCKPIT_THERAPIE:
        return true;
    case COCKPIT_8008:
    case COCKPIT_8080:
    case COCKPIT_8008_TRS:
    case COCKPIT_8008_TRS_PRO:
        serialWriteDelay_ = 50;
        playSound_ = true;
        return true;
    }
    return false;
}

bool Daum::configureFromProfile() {
    QRegularExpression re("(\\d+)");
    QRegularExpressionMatch match = re.match(profile_);
    bool ok = false;

    if (!match.hasMatch() || match.lastCapturedIndex() == 0) {
        // no delay found in the profile

        // if the string sound was found, use delay 0 and enable sound
        return playSound_;
    }

    uint delay = match.captured(1).toUInt(&ok);
    if (ok) {
        // use the matched number found as the command delay
        serialWriteDelay_ = delay;
    }

    return true;
}

void Daum::requestRealtimeData() {
    char addr = -1;
    QByteArray data;
    // Discard any existing data
    {
        QMutexLocker locker(&pvars);
        serial_dev_->readAll();
        addr = deviceAddress_;
    }

    qDebug() << "querying device info";

    data.clear();
    data.append((char)0x40);
    data.append(addr);

    int answer_len = 19;
    if (profile_ == "OLD_DAUM"){
        answer_len = 17;   // Old cockpits only send 17 bytes
    };
    
    data = WriteDataAndGetAnswer(data, answer_len);
    if (data.length() < answer_len) {
        return;
    }

    // local cache of telemetry data
    int pwr = (unsigned char) data[5];
    int rpm = (unsigned char) data[6];
    int speed = data[7];
    int pulse = (unsigned char) data[14];

    // sanity check
    if (pwr >= 5 && pwr <= 160) {
        int pedalling = data[4];   // either 0/1 or w/ offset of 128
        if (profile_ == "OLD_DAUM"){
            pedalling = (data[4] & 0x01);  // Only use last bit for old cockpit
        };
        pwr = pwr * 5 * (pedalling != 0 ? 1 : 0);
    } else {
        pwr = 0;
    }
    if (rpm < 0 || rpm > 199) {
        rpm = 0;
    }
    if (speed < 0 || speed > 99) {
        speed = 0;
    }
    if (pulse < 0 || pulse > 199) {
        pulse = 0;
    }

    // assign
    {
        QMutexLocker locker(&pvars);
        devicePower_ = pwr;
        deviceCadence_ = rpm;
        deviceSpeed_ = speed;
        deviceHeartRate_ = pulse;
    }

    // write load to device
    bool p = isPaused();
    unsigned int load = kDefaultLoad;
    {
        QMutexLocker locker(&pvars);
        load = load_;
        pwr = loadToWrite_;
    }

    if (!p && (forceUpdate_ || load != pwr)) {
        data.clear();
        data.append((char)0x51);
        data.append(deviceAddress_);
        data.append(MapLoadToByte(pwr));
        qInfo() << "Writing power to device: " << pwr << "W";
        QByteArray res = WriteDataAndGetAnswer(data, 3);
        qDebug() << "set power to " << (int)data[2]*5 << "W";
        if (res != data && res[2] != data[2] && pwr > 400) {
            // reduce power limit because some devices are limited too 400W instead of 800W
            QMutexLocker locker(&pvars);
            maxDeviceLoad_ = 400;
        }
        // update class cache
        {
            QMutexLocker locker(&pvars);
            load_ = pwr;
        }
    }
}

bool Daum::ResetDevice() {
    QByteArray dat;
    dat.append((char)0x12).append(deviceAddress_);
    int answer_len = 2;
    if (profile_ == "OLD_DAUM"){
        answer_len = 3;   // Old cockpits send 3 bytes answer
    };
    return WriteDataAndGetAnswer(dat, answer_len).length() == answer_len; // device tells pedalling state too
}
int Daum::GetAddress() {
    QByteArray dat;
    dat.append((char)0x11);
    if (profile_ == "OLD_DAUM"){
        dat.append((char)0x00); // Old cockpits need additional 0 byte
    }
    dat = WriteDataAndGetAnswer(dat, 2);
    if (dat.length() == 2 && (int)dat[0] == 0x11) {
        return (int)dat[1];
    }
    return -1;
}
int Daum::CheckCockpit() {
    QByteArray dat;
    dat.append((char)0x10);
    dat.append(deviceAddress_);
    dat = WriteDataAndGetAnswer(dat, 3);
    if (dat.length() == 3 && (int)dat[0] == 0x10 && (char)dat[1] == deviceAddress_) {
        return (int)dat[2];
    }
    return -1;
}
int Daum::GetDeviceVersion() {
    QByteArray dat;
    dat.append((char)0x73);
    dat.append(deviceAddress_);
    dat = WriteDataAndGetAnswer(dat, 11);
    if (dat.length() == 11 && (int)dat[0] == 0x73 && (char)dat[1] == deviceAddress_) {
        return (int)dat[10];
    }
    return -1;
}
bool Daum::SetProgram(unsigned int prog) {
    QByteArray dat;
    if (prog > 79) { prog = 79; }   // clamp to max
    dat.append((char)0x23).append(deviceAddress_).append((char)prog);
    int answer_len = 4;
    if (profile_ == "OLD_DAUM"){
        answer_len = 3;   // Old cockpits send 3 bytes answer
    };
    return WriteDataAndGetAnswer(dat, answer_len).length() == answer_len; // device tells pedalling state too
}
bool Daum::StartProgram(unsigned int prog) {
    Q_UNUSED(prog);
    QByteArray dat;
    dat.append((char)0x21).append(deviceAddress_);
    return WriteDataAndGetAnswer(dat, dat.length() + 1).length() == 3; // device tells pedalling state too
}
bool Daum::StopProgram(unsigned int prog) {
    Q_UNUSED(prog);
    QByteArray dat;
    dat.append((char)0x22).append(deviceAddress_);
    return WriteDataAndGetAnswer(dat, dat.length() + 1).length() == 3; // device tells pedalling state too
}
bool Daum::SetDate() {
    QDate d = QDate::currentDate();
    QByteArray dat;
    char year = d.year() - 2000;
    if (year < 0 || year > 99) { year = 0; }
    dat.append((char)0x64).append(deviceAddress_)
            .append((char)d.day())
            .append((char)d.month())
            .append(year);
    return WriteDataAndGetAnswer(dat, 2).length() == 2;
}
bool Daum::SetTime() {
    QTime tim = QTime::currentTime();
    QByteArray dat;
    dat.append((char)0x62).append(deviceAddress_)
            .append((char)tim.second())
            .append((char)tim.minute())
            .append((char)tim.hour());
    return WriteDataAndGetAnswer(dat, 2).length() == 2;
}
void Daum::PlaySound() {
    if (playSound_) {
        // might be buggy in device
        QByteArray dat;
        dat.append((char)0xd3).append((char)0x27).append((char)0x10);
        WriteDataAndGetAnswer(dat, 2);
    }
}

char Daum::MapLoadToByte(unsigned int load) const {
    char load_map = load/5;
    return load_map;
}

bool Daum::isPaused() const {
    QMutexLocker locker(&pvars);
    return paused_;
}

QByteArray Daum::WriteDataAndGetAnswer(QByteArray const& dat, int response_bytes) {
    QMutexLocker locker(&pvars);

    QSerialPort &s(*serial_dev_);
    QByteArray ret;
    if (!s.isOpen()) {
        return ret;
    }

    QThread::msleep(serialWriteDelay_);
    s.write(dat);
    if(!s.waitForBytesWritten(1000)) {
        qWarning() << "failed to write data to daum cockpit";
        exit(-1);
    }

    if (response_bytes > 0) {
        int retries = 20;
        do {
            if (!s.waitForReadyRead(1000)) {
                return ret;
            }
            ret.append(s.read(response_bytes - ret.length()));
        } while(--retries > 0 && ret.length() < response_bytes);
        if (retries <= 0) {
            qWarning() << "failed to read desired (" << response_bytes << ") data from device. Read: " << ret.length();
            ret.clear();
        }
    }

    s.readAll();    // discard additional data
    return ret;
}
