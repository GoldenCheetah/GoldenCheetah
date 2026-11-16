/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_KettlerRacer_h
#define _GC_KettlerRacer_h 1


#include <QString>
#include <QDebug>
#include <QThread>
//#include <QMutex>
#include <QFile>
#include "KettlerRacerConnection.h"
#include <QWidget>

// --- Classe di supporto corretta per il trascinamento anche su Wayland/Linux ---
class DragFilter : public QObject {
    Q_OBJECT
public:
    DragFilter(QObject *parent = nullptr) : QObject(parent) {}
signals:
    // Definiamo il tipo di azione inviata (puoi usare un intero o l'enum se pubblico)
    void keyActionTriggered(int key);
protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        QWidget *clickedWidget = qobject_cast<QWidget*>(obj);
        QString pollo = obj->objectName();
        if(pollo != "")qDebug() << "pollo " << pollo;
        if (!clickedWidget) return false;
        // m_gearOverlay_RootWindowRisaliamo alla finestra radice ("m_gearOverlay_Root")
        QWidget *w = clickedWidget->window();
        if (!w) return false;

        //key press
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            int pressedKey = keyEvent->key();

            if (pressedKey == Qt::Key_W || pressedKey == Qt::Key_S ||
                pressedKey == Qt::Key_A || pressedKey == Qt::Key_D)
            {
                // Emettiamo il segnale indicando quale tasto è stato premuto
                emit keyActionTriggered(pressedKey);
                return true; // Blocca l'evento
            }
        }
        // mouse click
        if(event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                // Chiediamo a Qt di agganciare il canale grafico nativo del sistema operativo
                if (w->windowHandle()) {
                    w->windowHandle()->startSystemMove();
                    event->accept();
                    return true;
                }
            }
        }
        /*if (event->type() == QEvent::MouseButtonPress)
        {
            // 1) Gestisci solo eventi mouse
            if (event->type() != QEvent::MouseButtonPress &&
                event->type() != QEvent::MouseMove &&
                event->type() != QEvent::MouseButtonRelease)
            {
                return QObject::eventFilter(obj, event);
            }

            // 2) Interessa solo l'overlay e i suoi figli
            QWidget *w = qobject_cast<QWidget*>(obj);
            if (!w || (w. != m_gearOverlay && !m_gearOverlay->isAncestorOf(w)))
                return QObject::eventFilter(obj, event);

            // 3) Da qui in poi gestisci solo l'overlay
            if (event->type() == QEvent::MouseButtonPress) {
                auto *me = static_cast<QMouseEvent*>(event);
                if (me->button() == Qt::LeftButton && w->windowHandle()) {
                    w->windowHandle()->startSystemMove();
                    return true;   // ok mangiare il press: è sull'overlay
                }
            }

            return QObject::eventFilter(obj, event);
        }*/


        return QObject::eventFilter(obj, event);
    }
};
class KettlerRacer : public QObject
{
    Q_OBJECT
public:
    KettlerRacer(QObject *parent=0, QString deviceFilename=0);       // pass device
    ~KettlerRacer();

    int start();
    int stop();
    int restart();
    void setMode(int m);

    quint32 heartRate() {return m_heartRate;}
    quint32 power() {return m_power;}
    quint32 cadence() {return m_cadence;}
    double speed() {return m_speed;}
    bool discover(QString portName);
    void setLoad(double load);
    void setGradient(double slope);
    void setAthleteWeight(double m){m_athleteWeight=m;};
    bool isConnected();

private:
    KettlerRacerConnection *m_kettlerRacerConnection;
    quint32 m_heartRate;
    quint32 m_power;
    quint32 m_cadence;
    double m_speed; // in km/h
    bool m_isKettlerRacerConnectionAlive;
    QWidget *m_gearOverlay = nullptr;
    void createLabelGears();
    double m_athleteWeight;

private slots:
    void newHeartRate(quint32);
    void newPower(quint32);
    void newCadence(quint32 cadence);
    void newSpeed(double speed);
    void onKettlerRacerConnectionFinished();
public slots:
    void gearOverlay_getFocus();

//signals:
    //void signalGradient(double gr);
};

#endif // _GC_KettlerRacer_h

