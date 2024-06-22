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

    setAttribute(Qt::WA_StyledBackground, true);  // otherwise the background-color of the widget is not shown

    // Control buttons
    QHBoxLayout *toolbuttons = new QHBoxLayout;
    toolbuttons->setSpacing(0);
    toolbuttons->setContentsMargins(0,0,0,0);

    m_connectButton = new QPushButton(this);
    applyIcon(m_connectButton, "offline");
    m_connectButton->setFocusPolicy(Qt::NoFocus);
    m_connectButton->setAutoDefault(false);
    m_connectButton->setFlat(true);
    m_connectButton->setShortcut(Qt::Key_MediaPrevious);
    toolbuttons->addWidget(m_connectButton);

    toolbuttons->addSpacing(5);
    toolbuttons->addWidget(newSep());
    toolbuttons->addSpacing(5);

    m_rewindButton = new QPushButton(this);
    applyIcon(m_rewindButton, "rewind");
    m_rewindButton->setFocusPolicy(Qt::NoFocus);
    m_rewindButton->setAutoDefault(false);
    m_rewindButton->setFlat(true);
    m_rewindButton->setAutoRepeat(true);
    m_rewindButton->setAutoRepeatDelay(400);
    m_rewindButton->setShortcut(Qt::Key_MediaPrevious);
    toolbuttons->addWidget(m_rewindButton);

    m_stopButton = new QPushButton(this);
    applyIcon(m_stopButton, "stop");
    m_stopButton->setFocusPolicy(Qt::NoFocus);
    m_stopButton->setAutoDefault(false);
    m_stopButton->setFlat(true);
    m_stopButton->setShortcut(Qt::Key_MediaStop);
    toolbuttons->addWidget(m_stopButton);

    m_playButton = new QPushButton(this);
    applyIcon(m_playButton, "play");
    m_playButton->setFocusPolicy(Qt::NoFocus);
    m_playButton->setAutoDefault(false);
    m_playButton->setFlat(true);
    m_playButton->setShortcut(Qt::Key_MediaTogglePlayPause);
    toolbuttons->addWidget(m_playButton);

    m_forwardButton = new QPushButton(this);
    applyIcon(m_forwardButton, "ffwd");
    m_forwardButton->setFocusPolicy(Qt::NoFocus);
    m_forwardButton->setAutoDefault(false);
    m_forwardButton->setFlat(true);
    m_forwardButton->setAutoRepeat(true);
    m_forwardButton->setAutoRepeatDelay(400);
    m_forwardButton->setShortcut(Qt::Key_MediaNext);
    toolbuttons->addWidget(m_forwardButton);

    toolbuttons->addSpacing(5);
    toolbuttons->addWidget(newSep());
    toolbuttons->addSpacing(5);

    backLap = new QPushButton(this);
    applyIcon(backLap, "back");
    backLap->setFocusPolicy(Qt::NoFocus);
    backLap->setAutoDefault(false);
    backLap->setFlat(true);
    backLap->setAutoRepeat(true);
    backLap->setAutoRepeatDelay(400);
    toolbuttons->addWidget(backLap);

    m_lapButton = new QPushButton(this);
    applyIcon(m_lapButton, "lap");
    m_lapButton->setFocusPolicy(Qt::NoFocus);
    m_lapButton->setAutoDefault(false);
    m_lapButton->setFlat(true);
    m_lapButton->setShortcut(Qt::Key_0);
    toolbuttons->addWidget(m_lapButton);

    fwdLap = new QPushButton(this);
    applyIcon(fwdLap, "fwd");
    fwdLap->setFocusPolicy(Qt::NoFocus);
    fwdLap->setAutoDefault(false);
    fwdLap->setFlat(true);
    fwdLap->setAutoRepeat(true);
    fwdLap->setAutoRepeatDelay(400);
    fwdLap->setShortcut(Qt::Key_MediaLast);
    toolbuttons->addWidget(fwdLap);

    toolbuttons->addSpacing(5);
    toolbuttons->addWidget(newSep());
    toolbuttons->addSpacing(5);

    cal = new QPushButton(this);
    applyIcon(cal, "cal");
    cal->setFocusPolicy(Qt::NoFocus);
    cal->setAutoDefault(false);
    cal->setFlat(true);
    cal->setShortcut(Qt::Key_C);
    toolbuttons->addWidget(cal);

    toolbuttons->addSpacing(5);
    toolbuttons->addWidget(newSep());
    toolbuttons->addSpacing(5);

    loadDown = new QPushButton(this);
    applyIcon(loadDown, "down");
    loadDown->setFocusPolicy(Qt::NoFocus);
    loadDown->setAutoDefault(false);
    loadDown->setFlat(true);
    loadDown->setAutoRepeat(true);
    loadDown->setAutoRepeatInterval(50);
    loadDown->setShortcut(Qt::Key_Minus);
    toolbuttons->addWidget(loadDown);

    loadUp = new QPushButton(this);
    applyIcon(loadUp, "up");
    loadUp->setFocusPolicy(Qt::NoFocus);
    loadUp->setAutoDefault(false);
    loadUp->setFlat(true);
    loadUp->setAutoRepeat(true);
    loadUp->setAutoRepeatInterval(50);
    loadUp->setShortcut(Qt::Key_Plus);
    toolbuttons->addWidget(loadUp);

    intensitySlider = new QSlider(Qt::Vertical, this);
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
    connect(m_trainSidebar->context, SIGNAL(intensityChanged(int)), intensitySlider, SLOT(setValue(int)));

    connect(m_trainSidebar, SIGNAL(setNotification(QString, int)), this, SLOT(setNotification(QString, int)));
    connect(m_trainSidebar, SIGNAL(clearNotification(void)), this, SLOT(clearNotification(void)));

    connect(m_trainSidebar->context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    this->setContentsMargins(0,0,0,0);
    this->setFocusPolicy(Qt::NoFocus);
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

    updateStyles();
}

void TrainBottom::updatePlayButtonIcon()
{
    if (m_trainSidebar->currentStatus() & RT_PAUSED)
    {
        applyIcon(m_playButton, "play");
    }
    else if (m_trainSidebar->currentStatus() & RT_RUNNING)
    {
        applyIcon(m_playButton, "pause");
    }
    else // Not running or paused means stopped
    {
        applyIcon(m_playButton, "play");
    }
}

void TrainBottom::autoHideCheckboxChanged(int state)
{
    emit autoHideChanged(state == Qt::Checked);
}


void TrainBottom::statusChanged(int status)
{
    // not yet connected
    if ((status&RT_CONNECTED) == 0) {
        applyIcon(m_connectButton, "offline");
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
        applyIcon(m_connectButton, "online");
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
        applyIcon(m_connectButton, "online");
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
        applyIcon(m_connectButton, "online");
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
        applyIcon(m_connectButton, "online");
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


void TrainBottom::updateStyles()
{
    QColor bg = GColor(CCHROME);
    QColor frame = GCColor::invertColor(bg);
    QColor hover = GColor(CHOVER);
    QString bss = QString(
        "QWidget { background-color: %2; }"
        "QPushButton { border: 0px; } "
        "QPushButton:hover { background-color: %1; } "
        "QFrame { color: %3; } "
        ).arg(hover.name(QColor::HexArgb))
         .arg(bg.name(QColor::HexArgb))
         .arg(frame.name(QColor::HexArgb));
    bool dark = isDark();

    reapplyIcon(m_connectButton, dark);
    reapplyIcon(m_playButton, dark);
    reapplyIcon(m_rewindButton, dark);
    reapplyIcon(m_stopButton, dark);
    reapplyIcon(m_forwardButton, dark);
    reapplyIcon(m_lapButton, dark);
    reapplyIcon(m_connectButton, dark);
    reapplyIcon(loadDown, dark);
    reapplyIcon(loadUp, dark);
    reapplyIcon(cal, dark);
    reapplyIcon(fwdLap, dark);
    reapplyIcon(backLap, dark);

    setStyleSheet(bss);
}


QFrame* TrainBottom::newSep()
{
    QFrame *sep = new QFrame(this);
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Plain);
    sep->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    return sep;
}


void TrainBottom::configChanged(qint32)
{
    updateStyles();
}


void
TrainBottom::applyIcon
(QPushButton *button, QString iconName, bool dark)
{
    iconNames[button] = iconName;
    button->setIcon(QIcon(QString(":images/breeze/%1/%2.svg").arg(dark ? "dark" : "light").arg(iconName)));
    button->setIconSize(QSize(64, 64));
    button->setFixedSize(64, 64);
}


void
TrainBottom::applyIcon
(QPushButton *button, QString iconName)
{
    applyIcon(button, iconName, isDark());
}


void
TrainBottom::reapplyIcon
(QPushButton *button, bool dark)
{
    applyIcon(button, iconNames.value(button, "unknown"), dark);
}


void
TrainBottom::reapplyIcon
(QPushButton *button)
{
    applyIcon(button, iconNames.value(button, "unknown"));
}


bool
TrainBottom::isDark
() const
{
    return GColor(CCHROME).lightness() < 127;
}
