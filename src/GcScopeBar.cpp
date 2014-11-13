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
#include "DiaryWindow.h"
#include "DiarySidebar.h"
#include "Context.h"
#include "QtMacButton.h"

GcScopeBar::GcScopeBar(Context *context) : QWidget(context->mainWindow), context(context)
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

    font.setPointSize(10);
    font.setWeight(QFont::Black);
    searchLabel->setFont(font);
    layout->addWidget(searchLabel);
    searchLabel->hide();

#ifdef GC_HAVE_LUCENE
    connect(context, SIGNAL(filterChanged()), this, SLOT(setHighlighted()));
#endif
    connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(setCompare()));
    connect(context, SIGNAL(compareDateRangesStateChanged(bool)), this, SLOT(setCompare()));

    // Mac uses QtMacButton - recessed etc
#ifdef Q_OS_MAC
    home = new QtMacButton(this, QtMacButton::Recessed);
#ifdef GC_HAVE_ICAL
    diary = new QtMacButton(this, QtMacButton::Recessed);
#endif
    anal = new QtMacButton(this, QtMacButton::Recessed);
#ifdef GC_HAVE_INTERVALS
    interval = new QtMacButton(this, QtMacButton::Recessed);
#endif
    train = new QtMacButton(this, QtMacButton::Recessed);
#else
    // Windows / Linux uses GcScopeButton - pushbutton
    home = new GcScopeButton(this);
#ifdef GC_HAVE_ICAL
    diary = new GcScopeButton(this);
#endif
    anal = new GcScopeButton(this);
#ifdef GC_HAVE_INTERVALS
    interval = new GcScopeButton(this);
#endif
    train = new GcScopeButton(this);
#endif

    // now set the text for each one
    home->setText(tr("Trends"));
    layout->addWidget(home);
    connect(home, SIGNAL(clicked(bool)), this, SLOT(clickedHome()));

#ifdef GC_HAVE_ICAL
    diary->setText(tr("Diary"));
    layout->addWidget(diary);
    connect(diary, SIGNAL(clicked(bool)), this, SLOT(clickedDiary()));
#endif

    anal->setText(tr("Rides"));
    anal->setWidth(70);
    anal->setChecked(true);
    layout->addWidget(anal);
    connect(anal, SIGNAL(clicked(bool)), this, SLOT(clickedAnal()));

#ifdef GC_HAVE_INTERVALS
    interval->setText(tr("Intervals"));
    interval->setWidth(70);
    layout->addWidget(interval);
    connect(interval, SIGNAL(clicked(bool)), this, SLOT(clickedInterval()));
#endif

    train->setText(tr("Train"));
    layout->addWidget(train);
    connect(train, SIGNAL(clicked(bool)), this, SLOT(clickedTrain()));

    //layout->addWidget(traintool); //XXX!!! eek

    // we now need to adjust the buttons according to their text size
    // this is particularly bad for German's who, as a nation, must
    // suffer from RSI from typing and writing more than any other nation ;)
    QFontMetrics fontMetric(font);
    int width = fontMetric.width(tr("Trends"));
    home->setWidth(width+20);

    width = fontMetric.width(tr("Rides"));
    anal->setWidth(width+20);

    width = fontMetric.width(tr("Train"));
    train->setWidth(width+20);

#ifdef GC_HAVE_ICAL
    width = fontMetric.width(tr("Diary"));
    diary->setWidth(width+20);
#endif
}

void
GcScopeBar::setHighlighted()
{
#ifdef GC_HAVE_LUCENE
    if (context->isfiltered) {
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
GcScopeBar::setCompare()
{
#ifndef Q_OS_MAC
    home->setRed(context->isCompareDateRanges);
    anal->setRed(context->isCompareIntervals);
    repaint();
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
    //paintBackground(event);
    QWidget::paintEvent(event);
}

void
GcScopeBar::paintBackground(QPaintEvent *)
{
    QPainter painter(this);

    painter.save();
    QRect all(0,0,width(),height());

    // fill with a linear gradient
    QLinearGradient linearGradient = GCColor::linearGradient(23, isActiveWindow());
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
#ifdef GC_HAVE_INTERVALS
    interval->setChecked(false);
#endif
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
#ifdef GC_HAVE_INTERVALS
    interval->setChecked(false);
#endif
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
#ifdef GC_HAVE_INTERVALS
    interval->setChecked(false);
#endif
    train->setChecked(false);
    emit selectAnal();
}

void
GcScopeBar::clickedInterval()
{
    home->setChecked(false);
#ifdef GC_HAVE_ICAL
    diary->setChecked(false);
#endif
    anal->setChecked(false);
#ifdef GC_HAVE_INTERVALS
    interval->setChecked(true);
#endif
    train->setChecked(false);
    emit selectInterval();
}

void
GcScopeBar::clickedTrain()
{
    home->setChecked(false);
#ifdef GC_HAVE_ICAL
    diary->setChecked(false);
#endif
    anal->setChecked(false);
#ifdef GC_HAVE_INTERVALS
    interval->setChecked(false);
#endif
    train->setChecked(true);
    emit selectTrain();
}

int
GcScopeBar::selected()
{
    if (home->isChecked()) return 0;
#ifdef GC_HAVE_ICAL
    if (diary->isChecked()) return 1;
    if (anal->isChecked()) return 2;
    if (train->isChecked()) return 3;
#ifdef GC_HAVE_INTERVALS
    if (interval->isChecked()) return 4;
#endif
#else
    if (anal->isChecked()) return 1;
    if (train->isChecked()) return 2;
#ifdef GC_HAVE_INTERVALS
    if (interval->isChecked()) return 3;
#endif
#endif

    // never gets here - shutup compiler
    return 0;
}

void
GcScopeBar::setSelected(int index)
{
    // we're already there
    if (index == selected()) return;

    // mainwindow wants to tell us to switch to a selection
    home->setChecked(false);
#ifdef GC_HAVE_ICAL
    diary->setChecked(false);
#endif
    anal->setChecked(false);
#ifdef GC_HAVE_INTERVALS
    interval->setChecked(false);
#endif
    train->setChecked(false);

#ifdef GC_HAVE_ICAL
    switch (index) {
        case 0 : home->setChecked(true); break;
        case 1 : diary->setChecked(true); break;
        case 2 : anal->setChecked(true); break;
        case 3 : train->setChecked(true); break;
#ifdef GC_HAVE_INTERVALS
        case 4 : interval->setChecked(true); break;
#endif
    }
#else
    switch (index) {
        case 0 : home->setChecked(true); break;
        case 1 : anal->setChecked(true); break;
        case 2 : train->setChecked(true); break;
#ifdef GC_HAVE_INTERVALS
        case 3 : interval->setChecked(true); break;
#endif
    }
#endif
}

#ifndef Q_OS_MAC
GcScopeButton::GcScopeButton(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(20);
    setFixedWidth(60);
    red = highlighted = checked = false;
    QFont font;
    font.setPointSize(10);
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
        if (red) {
            QColor over = QColor(Qt::red);
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
        if (red) {
            QColor over = QColor(Qt::red);
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
    // don't do all that offset nonsense for flat style
    // set fg checked and unchecked colors
    QColor checkedCol(240,240,240), uncheckedCol(30,30,30,200);
    if (!GCColor::isFlat()) {

        // metal style
        painter.setPen((underMouse() || checked) ? QColor(50,50,50) : Qt::white);
        painter.drawText(off, text, Qt::AlignVCenter | Qt::AlignCenter);

    } else {

        // adjust colors if flat and dark
        if (GCColor::luminance(GColor(CCHROME)) < 127) {
            // dark background so checked is white and unchecked is light gray
            checkedCol = QColor(Qt::white);
            uncheckedCol = QColor(Qt::lightGray);
        }
    }

    // draw the text
    painter.setPen((underMouse() || checked) ? checkedCol : uncheckedCol);
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
