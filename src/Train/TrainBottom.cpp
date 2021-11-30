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

#include "TrainBottom.h"
#include "TrainSidebar.h"
#include "HelpWhatsThis.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QPlainTextEdit>
#include <QTimer>

TrainBottom::TrainBottom(TrainSidebar *trainSidebar, QWidget *parent) :
    QWidget(parent),
    m_trainSidebar(trainSidebar)
{
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::WorkoutControl));

    // Control buttons
    QHBoxLayout *toolbuttons = new QHBoxLayout;
    toolbuttons->setSpacing(0);
    toolbuttons->setContentsMargins(0,0,0,0);

    QIcon connectButtonIcon(":images/oxygen/power-off.png");
    m_connectButton = new QPushButton(connectButtonIcon, "", this);
    m_connectButton->setFocusPolicy(Qt::NoFocus);
    m_connectButton->setIconSize(QSize(64,64));
    m_connectButton->setAutoFillBackground(false);
    m_connectButton->setAutoDefault(false);
    m_connectButton->setFlat(true);
    m_connectButton->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    m_connectButton->setAutoRepeat(true);
    m_connectButton->setAutoRepeatDelay(200);
    m_connectButton->setShortcut(Qt::Key_MediaPrevious);
    toolbuttons->addWidget(m_connectButton);

    QIcon rewIcon(":images/oxygen/rewind.png");
    m_rewindButton = new QPushButton(rewIcon, "", this);
    m_rewindButton->setFocusPolicy(Qt::NoFocus);
    m_rewindButton->setIconSize(QSize(64,64));
    m_rewindButton->setAutoFillBackground(false);
    m_rewindButton->setAutoDefault(false);
    m_rewindButton->setFlat(true);
    m_rewindButton->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    m_rewindButton->setAutoRepeat(true);
    m_rewindButton->setAutoRepeatDelay(200);
    m_rewindButton->setShortcut(Qt::Key_MediaPrevious);
    toolbuttons->addWidget(m_rewindButton);

    QIcon stopIcon(":images/oxygen/stop.png");
    m_stopButton = new QPushButton(stopIcon, "", this);
    m_stopButton->setFocusPolicy(Qt::NoFocus);
    m_stopButton->setIconSize(QSize(64,64));
    m_stopButton->setAutoFillBackground(false);
    m_stopButton->setAutoDefault(false);
    m_stopButton->setFlat(true);
    m_stopButton->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    m_stopButton->setShortcut(Qt::Key_MediaStop);
    toolbuttons->addWidget(m_stopButton);

    QIcon playIcon(":images/oxygen/play.png");
    m_playButton = new QPushButton(playIcon, "", this);
    m_playButton->setFocusPolicy(Qt::NoFocus);
    m_playButton->setIconSize(QSize(64,64));
    m_playButton->setAutoFillBackground(false);
    m_playButton->setAutoDefault(false);
    m_playButton->setFlat(true);
    m_playButton->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    m_playButton->setShortcut(Qt::Key_MediaTogglePlayPause);
    toolbuttons->addWidget(m_playButton);

    QIcon fwdIcon(":images/oxygen/ffwd.png");
    m_forwardButton = new QPushButton(fwdIcon, "", this);
    m_forwardButton->setFocusPolicy(Qt::NoFocus);
    m_forwardButton->setIconSize(QSize(64,64));
    m_forwardButton->setAutoFillBackground(false);
    m_forwardButton->setAutoDefault(false);
    m_forwardButton->setFlat(true);
    m_forwardButton->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    m_forwardButton->setAutoRepeat(true);
    m_forwardButton->setAutoRepeatDelay(200);
    m_forwardButton->setShortcut(Qt::Key_MediaNext);
    toolbuttons->addWidget(m_forwardButton);

    QIcon backLapIcon(":images/oxygen/back.png");
    backLap = new QPushButton(backLapIcon, "", this);
    backLap->setFocusPolicy(Qt::NoFocus);
    backLap->setIconSize(QSize(64,64));
    backLap->setAutoFillBackground(false);
    backLap->setAutoDefault(false);
    backLap->setFlat(true);
    backLap->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    backLap->setAutoRepeat(true);
    backLap->setAutoRepeatDelay(200);
    toolbuttons->addWidget(backLap);

    QIcon lapIcon(":images/oxygen/lap.png");
    m_lapButton = new QPushButton(lapIcon, "", this);
    m_lapButton->setFocusPolicy(Qt::NoFocus);
    m_lapButton->setIconSize(QSize(64,64));
    m_lapButton->setAutoFillBackground(false);
    m_lapButton->setAutoDefault(false);
    m_lapButton->setFlat(true);
    m_lapButton->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    m_lapButton->setShortcut(Qt::Key_0);
    toolbuttons->addWidget(m_lapButton);

    QIcon fwdLapIcon(":images/oxygen/fwd.png");
    fwdLap = new QPushButton(fwdLapIcon, "", this);
    fwdLap->setFocusPolicy(Qt::NoFocus);
    fwdLap->setIconSize(QSize(64,64));
    fwdLap->setAutoFillBackground(false);
    fwdLap->setAutoDefault(false);
    fwdLap->setFlat(true);
    fwdLap->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    fwdLap->setAutoRepeat(true);
    fwdLap->setAutoRepeatDelay(200);
    fwdLap->setShortcut(Qt::Key_MediaLast);
    toolbuttons->addWidget(fwdLap);

    QIcon calIcon(":images/oxygen/cal.png");
    cal = new QPushButton(calIcon, "", this);
    cal->setFocusPolicy(Qt::NoFocus);
    cal->setIconSize(QSize(64,64));
    cal->setAutoFillBackground(false);
    cal->setAutoDefault(false);
    cal->setFlat(true);
    cal->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    cal->setShortcut(Qt::Key_C);
    toolbuttons->addWidget(cal);

    QIcon upIcon(":images/oxygen/up.png");
    loadUp = new QPushButton(upIcon, "", this);
    loadUp->setFocusPolicy(Qt::NoFocus);
    loadUp->setIconSize(QSize(64,64));
    loadUp->setAutoFillBackground(false);
    loadUp->setAutoDefault(false);
    loadUp->setFlat(true);
    loadUp->setAutoRepeat(true);
    loadUp->setAutoRepeatInterval(50);
    loadUp->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    loadUp->setShortcut(Qt::Key_Plus);
    toolbuttons->addWidget(loadUp);

    QIcon downIcon(":images/oxygen/down.png");
    loadDown = new QPushButton(downIcon, "", this);
    loadDown->setFocusPolicy(Qt::NoFocus);
    loadDown->setIconSize(QSize(64,64));
    loadDown->setAutoFillBackground(false);
    loadDown->setAutoDefault(false);
    loadDown->setFlat(true);
    loadDown->setAutoRepeat(true);
    loadDown->setAutoRepeatInterval(50);
    loadDown->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    loadDown->setShortcut(Qt::Key_Minus);
    toolbuttons->addWidget(loadDown);

    intensitySlider = new QSlider(Qt::Vertical, this);
    intensitySlider->setAutoFillBackground(false);
    intensitySlider->setFocusPolicy(Qt::NoFocus);
    intensitySlider->setMinimum(75);
    intensitySlider->setMaximum(125);
    intensitySlider->setValue(100);
    toolbuttons->addWidget(intensitySlider);

    // notification area
    QHBoxLayout *notifications = new QHBoxLayout();
    notifications->setSpacing(0);
    notifications->setContentsMargins(0,0,0,0);

    notificationText = new QPlainTextEdit();
    notifications->addWidget(notificationText);

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    font.setBold(1);
    font.setPointSize(14);

    notificationText->setFont(font);
    notificationText->setStyleSheet("QPlainTextEdit {background-color: black; color: red}");
    notificationText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    notificationText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    notificationText->setTextInteractionFlags(Qt::NoTextInteraction);
    notificationText->setLineWrapMode(QPlainTextEdit::NoWrap);

    //QCheckBox *hideOnIdle = new QCheckBox(tr("Auto Hide"), this);
    //intensityControlLayout->addWidget(hideOnIdle);

    QHBoxLayout *allControlsLayout = new QHBoxLayout();
    allControlsLayout->addLayout(toolbuttons);
    allControlsLayout->addLayout(notifications);

    connect(m_playButton, SIGNAL(clicked()), m_trainSidebar, SLOT(Start()));
    connect(m_rewindButton, SIGNAL(clicked()), m_trainSidebar, SLOT(Rewind()));
    connect(m_forwardButton, SIGNAL(clicked()), m_trainSidebar, SLOT(FFwd()));
    connect(backLap, SIGNAL(clicked()), m_trainSidebar, SLOT(RewindLap()));
    connect(m_lapButton, SIGNAL(clicked()), m_trainSidebar, SLOT(newLap()));
    connect(fwdLap, SIGNAL(clicked()), m_trainSidebar, SLOT(FFwdLap()));
    connect(m_stopButton, SIGNAL(clicked()), m_trainSidebar, SLOT(Stop()));
    connect(m_trainSidebar->context, SIGNAL(start()), this, SLOT(updatePlayButtonIcon()));
    connect(m_trainSidebar->context, SIGNAL(pause()), this, SLOT(updatePlayButtonIcon()));
    connect(m_trainSidebar->context, SIGNAL(unpause()), this, SLOT(updatePlayButtonIcon()));
    connect(m_trainSidebar->context, SIGNAL(stop()), this, SLOT(updatePlayButtonIcon()));
    //connect(hideOnIdle, SIGNAL(stateChanged(int)), this, SLOT(autoHideCheckboxChanged(int)));
    connect(m_trainSidebar, SIGNAL(statusChanged(int)), this, SLOT(statusChanged(int)));
    connect(m_connectButton, SIGNAL(released()), m_trainSidebar, SLOT(toggleConnect()));
    connect(cal, SIGNAL(clicked()), m_trainSidebar, SLOT(Calibrate()));

    connect(loadUp, SIGNAL(clicked()), m_trainSidebar, SLOT(Higher()));
    connect(loadDown, SIGNAL(clicked()), m_trainSidebar, SLOT(Lower()));
    connect(intensitySlider, SIGNAL(valueChanged(int)), m_trainSidebar, SLOT(adjustIntensity(int)));
    connect(m_trainSidebar, SIGNAL(intensityChanged(int)), intensitySlider, SLOT(setValue(int)));

    connect(m_trainSidebar, SIGNAL(setNotification(QString, int)), this, SLOT(setNotification(QString, int)));
    connect(m_trainSidebar, SIGNAL(clearNotification(void)), this, SLOT(clearNotification(void)));

    this->setContentsMargins(0,0,0,0);
    this->setFocusPolicy(Qt::NoFocus);
    this->setAutoFillBackground(false);
    this->setStyleSheet("background-color: rgba( 255, 255, 255, 0% ); border: 0px;");
    this->setLayout(allControlsLayout);
    this->installEventFilter(trainSidebar);

    // Ensure the bottom bar is sized to show 3 lines of notification text
    // but don't allow it to crop the buttons if smaller font is used..
    int buttonHeight = m_connectButton->sizeHint().height();
    int notificationHeight = 3 * notificationText->fontMetrics().lineSpacing();
    int contentHeight = qMax(buttonHeight, notificationHeight);

    int layoutMargins = allControlsLayout->contentsMargins().top() +
                        allControlsLayout->contentsMargins().bottom();

    // Adding an extra quarter line looks more symmetrical
    int extra = notificationHeight / 12;

    this->setFixedHeight(contentHeight + layoutMargins + extra);

    //qDebug() << "Button height" << buttonHeight;
    //qDebug() << "Notification line spacing" << notificationText->fontMetrics().lineSpacing();
    //qDebug() << "bottombar margins:" << this->contentsMargins();
    //qDebug() << "- allControlsLayout margins:" << allControlsLayout->contentsMargins();
    //qDebug() << " - toolbuttons margins:" << toolbuttons->contentsMargins();
    //qDebug() << " - notifications margins:" << notifications->contentsMargins();
    //qDebug() << "  - notificationText margins:" << notificationText->contentsMargins();
    //notificationText->setPlainText("LINE1\nLINE2\nLINE3");

    // Create a timer for notifications, but don't start yet
    notificationTimer = new QTimer(this);
    connect(notificationTimer, SIGNAL(timeout()), SLOT(clearNotification()));
}

void TrainBottom::updatePlayButtonIcon()
{
    static QIcon playIcon(":images/oxygen/play.png");
    static QIcon pauseIcon(":images/oxygen/pause.png");

    if (m_trainSidebar->currentStatus() & RT_PAUSED)
    {
        m_playButton->setIcon(playIcon);
    }
    else if (m_trainSidebar->currentStatus() & RT_RUNNING)
    {
        m_playButton->setIcon(pauseIcon);
    }
    else // Not running or paused means stopped
    {
        m_playButton->setIcon(playIcon);
    }
}

void TrainBottom::autoHideCheckboxChanged(int state)
{
    emit autoHideChanged(state == Qt::Checked);
}


void TrainBottom::statusChanged(int status)
{
    static QIcon connectedIcon(":images/oxygen/power-on.png");
    static QIcon disconnectedIcon(":images/oxygen/power-off.png");

    // not yet connected
    if ((status&RT_CONNECTED) == 0) {
        m_connectButton->setIcon(disconnectedIcon);
        m_connectButton->setEnabled(true);
        m_playButton->setEnabled(false);
        m_stopButton->setEnabled(false);
        m_forwardButton->setEnabled(false);
        m_rewindButton->setEnabled(false);
        backLap->setEnabled(false);
        m_lapButton->setEnabled(false);
        fwdLap->setEnabled(false);
        cal->setEnabled(false);
        loadUp->setEnabled(false);
        loadDown->setEnabled(false);
        intensitySlider->setEnabled(false);
        return;
    }

    // connected, but not running
    if ((status&RT_CONNECTED) && ((status&RT_RUNNING) == 0)) {
        m_connectButton->setIcon(connectedIcon);
        m_connectButton->setEnabled(true);
        m_playButton->setEnabled(true);
        m_stopButton->setEnabled(false);
        m_forwardButton->setEnabled(false);
        m_rewindButton->setEnabled(false);
        backLap->setEnabled(false);
        m_lapButton->setEnabled(false);
        fwdLap->setEnabled(false);
        cal->setEnabled(false);
        loadUp->setEnabled(false);
        loadDown->setEnabled(false);
        intensitySlider->setEnabled(false);
        return;
    }

    // paused - important to check for paused before running
    if (status&RT_PAUSED) {
        m_connectButton->setIcon(connectedIcon);
        m_connectButton->setEnabled(false);
        m_playButton->setEnabled(true);
        m_stopButton->setEnabled(true);
        m_forwardButton->setEnabled(false);
        m_rewindButton->setEnabled(false);
        backLap->setEnabled(false);
        m_lapButton->setEnabled(false);
        fwdLap->setEnabled(false);
        cal->setEnabled(false);
        loadUp->setEnabled(false);
        loadDown->setEnabled(false);
        intensitySlider->setEnabled(false);
        return;
    }

    // running & calibrating
    if ((status&RT_CALIBRATING) && (status&RT_RUNNING)) {
        m_connectButton->setIcon(connectedIcon);
        m_connectButton->setEnabled(false);
        m_playButton->setEnabled(false);
        m_stopButton->setEnabled(true);
        m_forwardButton->setEnabled(false);
        m_rewindButton->setEnabled(false);
        backLap->setEnabled(false);
        m_lapButton->setEnabled(false);
        fwdLap->setEnabled(false);
        cal->setEnabled(true);
        loadUp->setEnabled(false);
        loadDown->setEnabled(false);
        intensitySlider->setEnabled(false);
        return;
    }

    // running
    if (status&RT_RUNNING) {
        m_connectButton->setIcon(connectedIcon);
        m_connectButton->setEnabled(false);
        m_playButton->setEnabled(true);
        m_stopButton->setEnabled(true);
        m_forwardButton->setEnabled(true);
        m_rewindButton->setEnabled(true);
        backLap->setEnabled(true);
        m_lapButton->setEnabled(true);
        fwdLap->setEnabled(true);
        cal->setEnabled(true);
        loadUp->setEnabled(true);
        loadDown->setEnabled(true);
        intensitySlider->setEnabled(true);
        return;
    }

}

void TrainBottom::setNotification(QString msg, int timeout)
{
    if (timeout > 0) {
        notificationTimer->setInterval(timeout * 1000);
        notificationTimer->setSingleShot(true);
        notificationTimer->start();
    } else {
        // stop any running timer
        notificationTimer->stop();
    }

    notificationText->clear();
    notificationText->setPlainText(msg);
}

void TrainBottom::clearNotification(void)
{
    notificationText->clear();
}
