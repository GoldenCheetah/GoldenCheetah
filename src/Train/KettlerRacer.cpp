/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net),
 *                    Mark Liversedge (liversedge@gmail.com),
 *               2015 Erik Botö (erik.boto@gmail.com)
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


#include "KettlerRacer.h"
#include <QMouseEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QWindow>



KettlerRacer::KettlerRacer(QObject *parent,  QString devname) : QObject(parent),
    m_heartRate(0),
    m_power(0),
    m_cadence(0),
    m_speed(0),
    m_isKettlerRacerConnectionAlive(true), // Inizializziamo sempre a nullptr nel costruttore
    m_gearOverlay(nullptr)
{
    m_kettlerRacerConnection = new KettlerRacerConnection;
    m_kettlerRacerConnection->setSerialPort(devname);
    connect(m_kettlerRacerConnection, SIGNAL(power(quint32)), this, SLOT(newPower(quint32)), Qt::QueuedConnection);
    connect(m_kettlerRacerConnection, SIGNAL(cadence(quint32)), this, SLOT(newCadence(quint32)), Qt::QueuedConnection);
    connect(m_kettlerRacerConnection, SIGNAL(pulse(quint32)), this, SLOT(newHeartRate(quint32)), Qt::QueuedConnection);
    connect(m_kettlerRacerConnection, SIGNAL(speed(double)), this, SLOT(newSpeed(double)), Qt::QueuedConnection);
    connect(m_kettlerRacerConnection, SIGNAL(finished()), this, SLOT(onKettlerRacerConnectionFinished()), Qt::QueuedConnection);
}

KettlerRacer::~KettlerRacer()
{
    if (m_gearOverlay) {
        m_gearOverlay->close();
        delete m_gearOverlay;
        m_gearOverlay = nullptr;
    }

    if (m_kettlerRacerConnection)
    {
        if (m_kettlerRacerConnection->isRunning()) {
            m_kettlerRacerConnection->quit();

            if (!m_kettlerRacerConnection->wait(2000)) {
                qWarning() << "Il thread non risponde, forzo la chiusura...";
                m_kettlerRacerConnection->terminate();
                m_kettlerRacerConnection->wait();
            }
        }
        delete m_kettlerRacerConnection;
        m_kettlerRacerConnection = nullptr;
    }
}

int KettlerRacer::start()
{
    m_kettlerRacerConnection->setWeight(m_athleteWeight);
    m_kettlerRacerConnection->start();
    createLabelGears();
    m_kettlerRacerConnection->resetRead();
    m_kettlerRacerConnection->setPollInterval(1000,1);
    return 0;
}

int KettlerRacer::restart()
{
    m_kettlerRacerConnection->resetRead();
    return 0;
}

int KettlerRacer::stop()
{
    m_kettlerRacerConnection->resetRead();
    if(m_gearOverlay){
        m_gearOverlay->close();
        delete m_gearOverlay;
        m_gearOverlay = nullptr;
    }
    return 0;
}

void KettlerRacer::newCadence(quint32 cadence)
{
    m_cadence = cadence;
}

void KettlerRacer::newHeartRate(quint32 heartRate)
{
    m_heartRate = heartRate;
}

void KettlerRacer::newPower(quint32 power)
{
    m_power = power;
}

void KettlerRacer::newSpeed(double speed)
{
    m_speed = speed;
}

void KettlerRacer::createLabelGears()
{
    if (m_gearOverlay) return;

    m_gearOverlay = new QWidget();
    m_gearOverlay->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    m_gearOverlay->resize(180, 75);
    m_gearOverlay->move(40, 60);

    QLabel *textLabel = new QLabel(m_gearOverlay);
    //textLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    textLabel->setAlignment(Qt::AlignCenter);
    textLabel->setText("34 x 36\nRapp: 0.94");

    QVBoxLayout *mainLayout = new QVBoxLayout(m_gearOverlay);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->addWidget(textLabel);
    m_gearOverlay->setLayout(mainLayout);

    // dark style
    m_gearOverlay->setStyleSheet(
        "QWidget#m_gearOverlay_Root { background-color: #1a1a1a; border: 2px solid #555555; border-radius: 8px; }"
        "QLabel { background-color: transparent; border: none; color: #FFFFFF; font-family: Arial; font-size: 20px; font-weight: bold; }"
        );
    m_gearOverlay->setObjectName("Gears");

    m_gearOverlay->show();

    DragFilter *filter = new DragFilter(m_gearOverlay);

    // Install filter
    m_gearOverlay->installEventFilter(filter);
    //textLabel->installEventFilter(filter);
    // filter on qapp
    //if (qApp) qApp->installEventFilter(filter);


    connect(filter, &DragFilter::keyActionTriggered,
            m_kettlerRacerConnection, &KettlerRacerConnection::handleKeyPress,
            Qt::QueuedConnection);

    connect(m_kettlerRacerConnection, &KettlerRacerConnection::gearChanged, m_gearOverlay,
            [this, textLabel](int front, int rear, double rapp) {
                if (!m_gearOverlay || !textLabel) return;

                textLabel->setText(
                        QString::number(front)+" x "
                        +QString::number(rear)
                        +"\nRapp.= "+QString::number(rapp));

                m_gearOverlay->raise();
                m_gearOverlay->activateWindow();
            },
            Qt::QueuedConnection
            );
}

void KettlerRacer::gearOverlay_getFocus()
{
    QTimer::singleShot(1000, m_gearOverlay, [this]() {
        if (m_gearOverlay) {
            m_gearOverlay->activateWindow();
            m_gearOverlay->raise();
            m_gearOverlay->setFocus();
        }
    });

}

/**
 * This functions takes a serial port and tries if it can find a Kettler bike connected
 * to it.
 */
bool KettlerRacer::discover(QString portName)
{
    bool found = false;
    QSerialPort sp;
    QLabel *label = new QLabel();
    label->setWordWrap(true);
    label->show();
    label->setText(label->text() +"---"+ portName);
    label->setFixedSize(300,100);
    sp.setPortName(portName);

    if (sp.open(QSerialPort::ReadWrite))
    {
        m_kettlerRacerConnection->configurePort(&sp);
        // Discard any existing data
        QByteArray reply = sp.readAll();

        for(int i=0; i<4; i++)
        {

            // Read id from bike
            reply.clear();
            sp.write("KI\r\n");
            if (sp.waitForBytesWritten(500))
            {}
            for(int i=0; i<15; i++)
            {
                if (sp.waitForReadyRead(500))
                {
                    reply.append(sp.readAll());
                    if(reply.contains("\r\n"))
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
                    sp.setBaudRate(QSerialPort::Baud9600);
                    break;
                case 1:
                    sp.setBaudRate(QSerialPort::Baud19200);
                    break;
                case 2:
                    sp.setBaudRate(QSerialPort::Baud38400);
                    break;
                case 3:
                    sp.setBaudRate(QSerialPort::Baud115200);
                    break;
                default:
                    break;
                }
            }
        }
        //found=false;
        reply.append('\0');
        QString replyString(reply);
        qDebug() << "find" << reply;
        label->setText(label->text() +"---"+ replyString);
        if (replyString.contains("RACER"))
        {
            found = true;
        }
    }

    sp.close();

    return found;
}

//from TrainSidebar::workoutTreeWidgetSelectionChanged()
//set m_modee and timeer
void KettlerRacer::setMode(int m)
{
    if (m==2) {
        m_kettlerRacerConnection->setPollInterval(100,m);//slope
        m_gearOverlay->show();
        return;
    }
    m_kettlerRacerConnection->setPollInterval(1000,m);//erg
    m_gearOverlay->hide();
    return;
}

void KettlerRacer::setLoad(double load)
{
    m_kettlerRacerConnection->setLoad((unsigned int)load);
}

//Used in KettlerRacerConnection::updateInternalSpeed()
void KettlerRacer::setGradient(double slope)
{
    m_kettlerRacerConnection->setGradient(slope);
}

bool KettlerRacer::isConnected()
{
    return m_isKettlerRacerConnectionAlive;
}

void KettlerRacer::onKettlerRacerConnectionFinished()
{
    m_isKettlerRacerConnectionAlive = false;
}

