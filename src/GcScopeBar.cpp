/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "GcScopeBar.h"
#include "GcCalendar.h"
#include "MainWindow.h"
#include "QtMacButton.h"

GcScopeBar::GcScopeBar(MainWindow *main, QWidget *traintool) : QWidget(main), mainWindow(main)
{
    setFixedHeight(23);
    setContentsMargins(10,0,10,0);
    layout = new QHBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(0,0,0,0);

    searchLabel = new GcLabel(tr("Search/Filter:"));
    searchLabel->setYOff(1);
    searchLabel->setFixedHeight(20);
    searchLabel->setHighlighted(true);
    QFont font;
    font.setFamily("Helvetica");
#ifdef WIN32
    font.setPointSize(8);
#else
    font.setPointSize(10);
#endif
    font.setWeight(QFont::Black);
    searchLabel->setFont(font);
    layout->addWidget(searchLabel);
    searchLabel->hide();

#ifdef GC_HAVE_LUCENE
    connect(mainWindow, SIGNAL(filterChanged(QStringList&)), this, SLOT(setHighlighted()));
#endif

    // Mac uses QtMacButton - recessed etc
#ifdef Q_OS_MAC
    home = new QtMacButton(this, QtMacButton::Recessed);
#ifdef GC_HAVE_ICAL
    diary = new QtMacButton(this, QtMacButton::Recessed);
#endif
    anal = new QtMacButton(this, QtMacButton::Recessed);
    train = new QtMacButton(this, QtMacButton::Recessed);
#else
    // Windows / Linux uses GcScopeButton - pushbutton
    home = new GcScopeButton(this);
#ifdef GC_HAVE_ICAL
    diary = new GcScopeButton(this);
#endif
    anal = new GcScopeButton(this);
    train = new GcScopeButton(this);
#endif

    // now set the text for each one
    home->setText(tr("Home"));
    layout->addWidget(home);
    connect(home, SIGNAL(clicked(bool)), this, SLOT(clickedHome()));

#ifdef GC_HAVE_ICAL
    diary->setText(tr("Diary"));
    layout->addWidget(diary);
    connect(diary, SIGNAL(clicked(bool)), this, SLOT(clickedDiary()));
#endif

    anal->setText(tr("Analysis"));
    anal->setWidth(70);
    anal->setChecked(true);
    layout->addWidget(anal);
    connect(anal, SIGNAL(clicked(bool)), this, SLOT(clickedAnal()));

    train->setText(tr("Train"));
    layout->addWidget(train);
    connect(train, SIGNAL(clicked(bool)), this, SLOT(clickedTrain()));

    layout->addStretch();
    layout->addWidget(traintool);
    layout->addStretch();
}

void
GcScopeBar::setHighlighted()
{
#ifdef GC_HAVE_LUCENE
    if (mainWindow->isfiltered) {
        searchLabel->setHighlighted(true);
        searchLabel->show();
#ifndef Q_OS_MAC
        home->setHighlighted(true);
        anal->setHighlighted(true);
#ifdef GC_HAVE_ICAL
        diary->setHighlighted(true);
#endif
#endif
    } else {
        searchLabel->setHighlighted(false);
        searchLabel->hide();
#ifndef Q_OS_MAC
        home->setHighlighted(false);
        anal->setHighlighted(false);
#ifdef GC_HAVE_ICAL
        diary->setHighlighted(false);
#endif
#endif
    }
#endif
}

void
GcScopeBar::addWidget(QWidget *p)
{
    layout->addWidget(p);
}

GcScopeBar::~GcScopeBar()
{
}

void
GcScopeBar::paintEvent (QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
    QWidget::paintEvent(event);
}

void
GcScopeBar::paintBackground(QPaintEvent *)
{
    QPainter painter(this);

    painter.save();
    QRect all(0,0,width(),height());

    // fill with a linear gradient
#ifdef Q_OS_MAC
    int shade = isActiveWindow() ? 178 : 225;
#else
    int shade = isActiveWindow() ? 200 : 250;
#endif
    QLinearGradient linearGradient(0, 0, 0, 23);
    linearGradient.setColorAt(0.0, QColor(shade,shade,shade, 100));
    linearGradient.setColorAt(0.5, QColor(shade,shade,shade, 180));
    linearGradient.setColorAt(1.0, QColor(shade,shade,shade, 255));
    linearGradient.setSpread(QGradient::PadSpread);
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, linearGradient);

    QPen black(QColor(100,100,100));
    painter.setPen(black);
    painter.drawLine(0,height()-1, width()-1, height()-1);

    QPen gray(QColor(230,230,230));
    painter.setPen(gray);
    painter.drawLine(0,0, width()-1, 0);

    painter.restore();
}

void
GcScopeBar::clickedHome()
{
    home->setChecked(true);
#ifdef GC_HAVE_ICAL
    diary->setChecked(false);
#endif
    anal->setChecked(false);
    train->setChecked(false);
    emit selectHome();
}

void
GcScopeBar::clickedDiary()
{
    home->setChecked(false);
#ifdef GC_HAVE_ICAL
    diary->setChecked(true);
#endif
    anal->setChecked(false);
    train->setChecked(false);
    emit selectDiary();
}

void
GcScopeBar::clickedAnal()
{
    home->setChecked(false);
#ifdef GC_HAVE_ICAL
    diary->setChecked(false);
#endif
    anal->setChecked(true);
    train->setChecked(false);
    emit selectAnal();
}

void
GcScopeBar::clickedTrain()
{
    home->setChecked(false);
#ifdef GC_HAVE_ICAL
    diary->setChecked(false);
#endif
    anal->setChecked(false);
    train->setChecked(true);
    emit selectTrain();
}

void
GcScopeBar::selected(int index)
{
    // mainwindow wants to tell us to switch to a selection
    home->setChecked(false);
#ifdef GC_HAVE_ICAL
    diary->setChecked(false);
#endif
    anal->setChecked(false);
    train->setChecked(false);

#ifdef GC_HAVE_ICAL
    switch (index) {
        case 0 : home->setChecked(true); break;
        case 1 : diary->setChecked(true); break;
        case 2 : anal->setChecked(true); break;
        case 3 : train->setChecked(true); break;
    }
#else
    switch (index) {
        case 0 : home->setChecked(true); break;
        case 1 : anal->setChecked(true); break;
        case 2 : train->setChecked(true); break;
    }
#endif
}

#ifndef Q_OS_MAC
GcScopeButton::GcScopeButton(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(20);
    setFixedWidth(60);
    highlighted = checked = false;
    QFont font;
    font.setFamily("Helvetica");
#ifdef WIN32
    font.setPointSize(8);
#else
    font.setPointSize(10);
#endif
    font.setWeight(QFont::Black);
    setFont(font);
}

void
GcScopeButton::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing, true);

    // widget rectangle
    QRectF body(0,0,width(), height());
    QRectF off(0,1,width(), height());
    QRectF inside(0,2,width(), height()-4);

    QPainterPath clip;
    clip.addRect(inside);
    painter.setClipPath(clip);
    painter.setPen(Qt::NoPen);

    if (checked && underMouse()) {
        painter.setBrush(QBrush(QColor(150,150,150)));     
        painter.drawRoundedRect(body, 19, 11);
        if (highlighted) {
            QColor over = GColor(CCALCURRENT);
            over.setAlpha(180);
            painter.setBrush(over);
            painter.drawRoundedRect(body, 19, 11);
        }
    } else if (checked && !underMouse()) {
        painter.setBrush(QBrush(QColor(120,120,120)));     
        painter.drawRoundedRect(body, 19, 11);
        if (highlighted) {
            QColor over = GColor(CCALCURRENT);
            over.setAlpha(180);
            painter.setBrush(over);
            painter.drawRoundedRect(body, 19, 11);
        }
    } else if (!checked && underMouse()) {
        painter.setBrush(QBrush(QColor(180,180,180)));     
        painter.drawRoundedRect(body, 19, 11);
    } else if (!checked && !underMouse()) {
    }

    // now paint the text
    painter.setPen((underMouse() || checked) ? QColor(50,50,50) : Qt::white);
    painter.drawText(off, text, Qt::AlignVCenter | Qt::AlignCenter);
    painter.setPen((underMouse() || checked) ? QColor(240,240,240) : QColor(30,30,30,200));
    painter.drawText(body, text, Qt::AlignVCenter | Qt::AlignCenter);
    painter.restore();
}

bool
GcScopeButton::event(QEvent *e)
{
    // entry / exit event repaint for hover color
    if (e->type() == QEvent::Leave || e->type() == QEvent::Enter) repaint();
    if (e->type() == QEvent::MouseButtonPress && underMouse()) emit clicked(checked);
    return QWidget::event(e);
}
#endif
