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

#ifndef _GC_TrainBottom_h
#define _GC_TrainBottom_h

#include <QWidget>
#include <QFrame>
#include <QMap>
#include <QString>

class TrainSidebar;
class QPushButton;
class QSlider;
class QPlainTextEdit;

class TrainBottom : public QWidget
{
    Q_OBJECT

public:
    TrainBottom(TrainSidebar * trainSidebar, QWidget *parent = 0);

public slots:
    void configChanged(qint32);

private:
    TrainSidebar *m_trainSidebar;
    QPushButton *m_playButton, *m_rewindButton, *m_stopButton, *m_forwardButton;
    QPushButton *m_lapButton, *m_connectButton, *loadDown, *loadUp, *cal, *fwdLap, *backLap;
    QSlider *intensitySlider;
    QPlainTextEdit *notificationText;
    QTimer *notificationTimer;
    QMap<void*, QString> iconNames;

    void updateStyles();
    QFrame *newSep();
    void applyIcon(QPushButton *button, QString iconName, bool dark);
    void applyIcon(QPushButton *button, QString iconName);
    void reapplyIcon(QPushButton *button, bool dark);
    void reapplyIcon(QPushButton *button);
    bool isDark() const;

private slots:
    void updatePlayButtonIcon();
    void autoHideCheckboxChanged(int state);
    void statusChanged(int status);
    void setNotification(QString msg, int timeout);
    void clearNotification(void);

signals:
    void autoHideChanged(bool enabled);
};

#endif // _GC_TrainBottom_h
