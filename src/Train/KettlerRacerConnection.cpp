/*
 * Copyright (c) 2015 Erik Botö (erik.boto@gmail.com)
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

#include "KettlerRacerConnection.h"

#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QTextStream>

KettlerRacerConnection::KettlerRacerConnection() :
    m_mode(1),
    line(0),
    m_splits(0),
    m_pollInterval(1000),
    pauseRead(false),
    m_load(0),
    m_loadToWrite(0),
    m_shouldWriteLoad(false),
    m_parseALLok(0),
    m_parseESok(0),
    m_parseSTok(0),
    m_parseGearsok(0),
    m_gradient(0.1),
    m_internalSpeed(10),
    m_pignone(36),
    conta(0),
    idx(0),
    m_power(0),
    m_countKR(0)
{
    //connect(this, SIGNAL(finishe()), this, SLOT(deleteLater()));
}

KettlerRacerConnection::~KettlerRacerConnection(){
}

void KettlerRacerConnection::setSerialPort(const QString serialPortName)
{
    if (!this->isRunning())
    {
        m_serialPortName = serialPortName;
    } else {
        qWarning() << "KettlerRacerConnection: Cannot set serialPortName while running";
    }
}

//start here (run is running)
//called from trainsidebar at exercise selection
//set mode and start polling bike data
void KettlerRacerConnection::setPollInterval(int interval)
{
    conta=0;
    //ERG
    if (m_mode == 1){
        cmd = "ES1\r\n";//erg mode
    }
    if (interval != m_pollInterval)
    {
        m_pollInterval = interval;
        emit timerIntervalChanged(interval);
    }
    //SLOPE  set 25 Watt - sequence pw25 kr kr kr pw kr kr kr pw
    if(m_mode==2){
        m_gradient=0.02;//set pw and read initial speed cadence pw
        m_countKR = 0;
        m_internalSpeed=10; m_shiftFront=0; m_shiftRear=1; m_shiftGear=1;
        m_load=25; m_loadToWrite=25; m_pignone=36;
        m_parseALLok=false; m_parseESok=false; m_parseSTok=false; m_parseGearsok=false;
        m_gearsKeyPressed=0;
        m_rapp=0.94;
    }
    pauseRead=false;
}

int KettlerRacerConnection::pollInterval()
{
    return m_pollInterval;
}


void KettlerRacerConnection::setLoad(unsigned int load)
{
    m_loadToWrite = load;
    m_shouldWriteLoad = true;
}

void KettlerRacerConnection::setGradient(double slope)
{
    m_gradient_old=m_gradient;
    m_gradient=slope/100;
    //do not recalc speed here
    //recalc speed on KR polling eticre TODO smooth slope in two or three pass
    //if (mode == 2) updateInternalSpeed(); // slope: refresh computed speed
}

/**
 * QThread::run()
 *
 * Open the serial port and set it up, then starts polling.
 *
 */
void KettlerRacerConnection::run()
{
    m_serial = new QSerialPort();
    m_timer = new QTimer();
    //m_btSocket = new QBluetoothSocket; // avoid error on testing
    m_serial->setPortName(m_serialPortName);
    //m_device = m_serial; // usa as serial or BT
    QTimer *startupTimer = new QTimer();
    if (!m_serial->open(QSerialPort::ReadWrite))
    {
        qDebug() << "Error opening serial";
        this->exit(-1);
    }
   if (!configurePort(m_serial)){
        qDebug() << "Error serial comunication";
        this->exit(-1);
    }

    connect(this, &KettlerRacerConnection::timerIntervalChanged,
            m_timer, qOverload<int>(&QTimer::setInterval));


    connect(m_timer, &QTimer::timeout, this,
            &KettlerRacerConnection::requestAll, Qt::DirectConnection);

    // Set up initial model detection
    connect(startupTimer, SIGNAL(timeout()), this,
            SLOT(initializePcConnection()), Qt::DirectConnection);

    m_timer->setInterval(m_pollInterval);
    m_timer->start();

    startupTimer->setSingleShot(true);
    startupTimer->setInterval(0);
    startupTimer->start();

    // slope mode: no external speed available -> compute initial speed
    // internally (load + gradient + gear) before the first poll step
    //if (mode == 2) updateInternalSpeed();

    exec();
    m_timer->stop();
    this->deleteSerialPort();
    m_timer = nullptr;
}

//pauseRead = true; reject requestAll from m_timer
//pauseRead false only after response - KR or new PW,self ST(after new PW),ES1 (after self ST)
void KettlerRacerConnection::writeCommand(const QByteArray& c)
{
    //if (m_useBluetooth)
       // m_btSocket->write(cmd);
     //   int onlyfortest=1;
   // else
    //qDebug() << "cmd" << c;
    m_serial->write(c);
    m_serial->waitForBytesWritten(300);
}

void KettlerRacerConnection::requestAll()
{
    if (pauseRead){
        conta++;
        if(conta>5) conta=0;
        else
        return;
    }
    if (m_mode == 2) {
        if (m_countKR == 0) {
            cmd = "PW " + QByteArray::number(m_loadToWrite) + "\r\n";
        } else {
            cmd = "KR\r\n";
        }
        m_countKR = (m_countKR + 1) % 4;
    }
    pauseRead = true;
    m_parseALLok=false; m_parseESok=false; m_parseSTok=false; m_parseGearsok=false;
    writeCommand(cmd);
}

//stay active during pauseRead and read ST after PW command
void KettlerRacerConnection::onReadyRead()
{
    if(!m_serial->bytesAvailable()) return;
    m_rxBuffer.append(m_serial->readAll());
    //qDebug() << m_rxBuffer << " buffer " << m_rxBuffer.size();
    //if(!pauseRead) m_rxBuffer.clear();
    //if (m_rxBuffer.contains("\r\n"))
    while (m_rxBuffer.contains("\r\n"))
    {
        idx = m_rxBuffer.indexOf("\r\n");
        line = m_rxBuffer.left(idx).trimmed();
        m_rxBuffer.remove(0, idx + 2);
        this->parseALL();
    }
    if (m_rxBuffer.size() > 50) {
        m_rxBuffer.clear();
        pauseRead = false;
    }
}

void KettlerRacerConnection::parseALL()
{
        m_splits = line.split('\t');
        //qDebug() << mode << " - read " << m_rxBuffer;
        //qDebug() << "conta=" << conta;
        switch ( m_splits.size() )
        {
        case 13:
            m_parseALLok=true;
            parseES();
            break;
        case 8:
            m_parseALLok=true;
            parseST();
            break;
        case 1:
            m_parseALLok=true;
            m_gears = line.split(' ');
            parseGear();
            break;
        default:
            //len 4 byte 0 response or else
            qDebug() << "Unknown line:" << line;
            pauseRead = false;
            break;
        }

}

void KettlerRacerConnection::parseES()
{
    bool ok=false;
    quint32 newHeartrate = m_splits.at( 4 ).toUInt(&ok);
    if (ok){
        emit pulse(newHeartrate);
        //emit pulse(m_internalSpeed);
    }
    quint32 newCadence = m_splits.at( 5 ).toUInt(&ok);
    if (ok){
        m_cadence=newCadence;
        emit cadence(newCadence);
    }
    double newSpeed = m_splits.at( 6 ).toDouble(&ok);
    if (ok && m_mode != 2){
        emit speed(newSpeed/10.0); // slope mode: bike speed ignored, use computed speed
    }
    quint32 newPower = m_splits.at( 11 ).toUInt(&ok);
    if (ok){
        m_power=newPower;
        emit power(newPower);
    }
    m_parseESok=true;
    if ((m_loadToWrite != m_load))
    {
        QByteArray pw = "PW ";
        pw += QByteArray::number(m_loadToWrite);
        pw += "\r\n";
        writeCommand(pw);
        m_load = m_loadToWrite;
        return;
        //polling cmd rest stopped
        //pauseRead false after PW bike send ST
        //ST set pauseRead false
    }
    pauseRead = false;
    //qDebug() << "parseES";
}

/* only used in slope mode bike send ST after received new PW */
void KettlerRacerConnection::parseST()
{
    bool ok=false;
    quint32 newHeartrate = m_splits.at( 0 ).toUInt(&ok);
    if (ok){
        emit pulse(newHeartrate);
        //emit pulse(m_internalSpeed);
    }
    quint32 newCadence = m_splits.at( 1 ).toUInt(&ok);
    if (ok){
        m_cadence=newCadence;
        emit cadence(newCadence);
    }

    double newSpeed = m_splits.at( 2 ).toDouble(&ok);
    if (ok && m_mode != 2){
        emit speed(newSpeed/10.0); // slope mode: bike speed ignored, use computed speed
    }

    quint32 newPower = m_splits.at( 7 ).toUInt(&ok);
    if (ok){
        m_power=newPower;
        emit power(newPower);
    }
    m_parseSTok=true;
    if ((m_loadToWrite != m_load))
    {
        QByteArray pw = "PW ";
        pw += QByteArray::number(m_loadToWrite);
        pw += "\r\n";
        writeCommand(pw);
        m_load = m_loadToWrite;
        return;
    }
    /* send ES1 after self ST received after PW */
    /*if(m_mode==2){
        writeCommand("ES1\r\n");
        return;
    }*/
    //qDebug() << "parseST";
    pauseRead = false;
}

// KR polling receive 0 if no gear change - lenght==1 discard
void KettlerRacerConnection::parseGear()
{
    updateInternalSpeed();
    if ( (m_gears.length() >= 4 && m_gears.at(1) == "P") ||  m_gearsKeyPressed) {
        int prGear;
        if(m_gearsKeyPressed){
            prGear=m_gearsKeyPressed;
            m_gearsKeyPressed=0;
        }else{
            prGear = m_gears.at(0).toUInt();
        }
        if (prGear == 1024)      changeGearByAction(fdDown);
        else if (prGear == 2048) changeGearByAction(fdUp);
        else if (prGear == 4096) changeGearByAction(rdUp);
        else if (prGear == 8192) changeGearByAction(rdDown);
    }
    m_parseGearsok = true;
    pauseRead = false;
}
//from kettlerracer class dragfilter --
void KettlerRacerConnection::handleKeyPress(int key)
{
    // Questo codice viene eseguito nel thread secondario in sicurezza grazie a QueuedConnection
    switch (key) {
    case Qt::Key_S:
        m_gearsKeyPressed = 1024;
        break;
    case Qt::Key_W:
        m_gearsKeyPressed = 2048;
        break;
    case Qt::Key_A:
        m_gearsKeyPressed = 4096;
        break;
    case Qt::Key_D:
        m_gearsKeyPressed = 8192;
        break;
    default:
        return;
    }
}
void KettlerRacerConnection::changeGearByAction(GearAction action)
{
    // Questa logica ora accetta comandi sia dalla bici che dalla tastiera
    switch (action) {
    case fdDown: // w o 1024
        m_shiftFront = 0;
        break;
    case fdUp:  // s o 2048
        m_shiftFront = 12;
        break;
    case rdUp: // a o 4096 (marcia più leggera / pignone più grande)
        if (m_shiftRear < 12) m_shiftRear++;
        break;
    case rdDown: // d o 8192 (marcia più dura / pignone più piccolo)
        if (m_shiftRear > 1) m_shiftRear--;
        break;
    }

    m_shiftGear = m_shiftFront + m_shiftRear;

    if (m_shiftGear < 1 || m_shiftGear > 24) {
        pauseRead = false;
        return;
    }

    if (m_shiftGear >= 1 && m_shiftGear <= 12) {
        m_pignone = m_pignoni[m_shiftGear - 1];
        m_rapp = 34.0 / m_pignone;
        emit gearChanged(34, m_pignone, m_rapp);
    }
    else if (m_shiftGear > 12 && m_shiftGear <= 24) {
        m_pignone = m_pignoni[m_shiftGear - 13];
        m_rapp = 50.0 / m_pignone;
        emit gearChanged(50, m_pignone, m_rapp);
    }

    updateInternalSpeed();
}

//slope mode
//called on start, or after load, (gradient not used polling from trainsidebar is too frequent)
//or gear change
//no external speed available: compute the equivalent road speed internally
//from target load (watts), gradient and gear ratio (inverse of the
//"power needed to hold this speed on this slope" model)
void KettlerRacerConnection::updateInternalSpeed()
{
    //const double m       = 85.0;
    const double g       = 9.81;
    const double Crr     = 0.005;
    const double Cda     = 0.32;
    const double rho     = 1.225;
    const double chain   = 0.95;
    const double dt      = 0.5;
    const double r_wheel = 0.34;

    // (m/s)
    double v = m_internalSpeed / 3.6;
    if (v < 0.0) v = 0.0;

    // power calc
    int targetLoad = 25;
    if (m_cadence > 5.0) {
        double omega_pedale = m_cadence * 2.0 * M_PI / 60.0;


        double v_pedalata = omega_pedale * m_rapp * r_wheel;

        // resist force
        double sinTheta = m_gradient;
        double F_res_teorica = m_athleteWeight * g * (sinTheta + Crr) + 0.5 * Cda * rho * v_pedalata * v_pedalata;

        // Potenza richiesta ai pedali per spingere quel rapporto a quella cadenza
        double P_res_teorica = F_res_teorica * v_pedalata;
        targetLoad = static_cast<int>(P_res_teorica / chain);
    }

    // Limit target
    targetLoad = qBound(25, targetLoad, 800);

    // delta 5 Watt from previous
    if (qAbs(targetLoad - m_loadToWrite) >= 5) {
        m_loadToWrite = targetLoad;
        m_load = m_loadToWrite;
    }

    // power
    int P_ped_int = qMax(m_power, 0);
    double P_ped = static_cast<double>(P_ped_int);
    double P_ruota = P_ped * chain;

    double F_traz = 0.0;
    if (P_ruota > 0.0) {
        double v_calc = qMax(v, 1.0);
        F_traz = P_ruota / v_calc;
    }

    // Resistenza stradale effettiva basata sulla velocità virtuale attuale
    double sinTheta = m_gradient;
    double F_res = m_athleteWeight * g * (sinTheta + Crr) + 0.5 * Cda * rho * v * v;

    double dv = (F_traz - F_res) / m_athleteWeight * dt;
    dv = qBound(-1.5, dv, 1.5); // fluid variation

    v += dv;
    v = qBound(0.0, v, 25.0);

    // notify new speed
    double newSpeed = v * 3.6;
    if (std::abs(newSpeed - m_internalSpeed) > 0.01) {
        m_internalSpeed = newSpeed;
        emit speed(m_internalSpeed);
    }
}

void KettlerRacerConnection::initializePcConnection()
{
    // Set kettler racer into PC-mode, reply should be ACK or RUN
    QByteArray data;
    m_serial->write("CM\r\n");
    if (!m_serial->waitForBytesWritten(500))
    {
        // failure to write to device, bail out
        this->exit(-1);
    }
    for(int i=0; i<30; i++)
    {
        if (m_serial->waitForReadyRead(500))
        {
            data.append(m_serial->readAll());
            if(data.contains("ACK") || data.contains("RUN"))
            {
                data.clear();
                m_serial->waitForReadyRead(100);
                m_serial->readAll();
                connect(m_serial, &QSerialPort::readyRead,
                        this, &KettlerRacerConnection::onReadyRead,
                        Qt::DirectConnection);
                break;
            }
        }
    }
}


/*
 * Configures a serialport for communicating with a Kettler bike.
 */
bool KettlerRacerConnection::configurePort(QSerialPort *serialPort)
{
    if (!serialPort)
    {
        qFatal("Trying to configure null port, start debugging.");
    }
    serialPort->setBaudRate(QSerialPort::Baud57600);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
    serialPort->setParity(QSerialPort::NoParity);
    QByteArray data = serialPort->readAll();
    data.clear();
    // find kettler racer
    bool found=false;
    for(int i=0; i<4; i++)
    {
        // Read id from bike
        data.clear();
        serialPort->write("KI\r\n");
        if (serialPort->waitForBytesWritten(500))
        {}
        for(int i=0; i<15; i++)
        {
            if (serialPort->waitForReadyRead(500))
            {
                data.append(serialPort->readAll());
                if(data.contains("\r\n"))
                {
                    found=true;
                    break;
                }
            }
        }
        if(found)
        {
            break;
        } else {
            switch (i) {
            case 0:
                serialPort->setBaudRate(QSerialPort::Baud9600);
                break;
            case 1:
                serialPort->setBaudRate(QSerialPort::Baud19200);
                break;
            case 2:
                serialPort->setBaudRate(QSerialPort::Baud38400);
                break;
            case 3:
                serialPort->setBaudRate(QSerialPort::Baud115200);
                break;
            default:
                break;
            }
        }
    }
    data.clear();
    return found;
}

void KettlerRacerConnection::deleteSerialPort()
{
    if (m_timer) {
        m_timer->disconnect();
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }

    if (m_serial) {
        if (m_serial->isOpen()) {
            m_serial->close();
        }
        m_serial->deleteLater();
        m_serial = nullptr;
    }
}

void KettlerRacerConnection::resetRead()
{
    pauseRead=false;
    m_shiftFront=0;
    m_shiftRear=1;
    m_shiftGear=1;
}

