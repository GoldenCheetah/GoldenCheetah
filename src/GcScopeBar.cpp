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
#include "QtMacButton.h"

GcScopeBar::GcScopeBar(QWidget *parent, QWidget *traintool) : QWidget(parent)
{
    setFixedHeight(23);
    setContentsMargins(10,0,10,0);
    layout = new QHBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(0,0,0,0);

#if 0 // moving to the main toolbar
    showHide = new QtMacButton(this, QtMacButton::Recessed);
    showHide->setWidth(60);
    showHide->setIconAndText();
    state = true;
    showHideClicked();
    layout->addWidget(showHide);
    connect(showHide, SIGNAL(clicked(bool)), this, SLOT(showHideClicked()));

    GcLabel *sep = new GcLabel("|");
    sep->setFixedWidth(4);
    sep->setYOff(1);
    layout->addWidget(sep);
#endif
   
    home = new QtMacButton(this, QtMacButton::Recessed);
    home->setText("Home");
    layout->addWidget(home);
    connect(home, SIGNAL(clicked(bool)), this, SLOT(clickedHome()));

#ifdef GC_HAVE_ICAL
    diary = new QtMacButton(this, QtMacButton::Recessed);
    diary->setText("Diary");
    layout->addWidget(diary);
    connect(diary, SIGNAL(clicked(bool)), this, SLOT(clickedDiary()));
#endif

    anal = new QtMacButton(this, QtMacButton::Recessed);
    anal->setText("Analysis");
    anal->setWidth(70);
    anal->setChecked(true);
    layout->addWidget(anal);
    connect(anal, SIGNAL(clicked(bool)), this, SLOT(clickedAnal()));

    train = new QtMacButton(this, QtMacButton::Recessed);
    train->setText("Train");
    layout->addWidget(train);
    connect(train, SIGNAL(clicked(bool)), this, SLOT(clickedTrain()));

    layout->addStretch();
    layout->addWidget(traintool);
    layout->addStretch();
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
    int shade = isActiveWindow() ? 178 : 225;
    QLinearGradient linearGradient(0, 0, 0, height());
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

    switch (index) {
        case 0 : home->setChecked(true); break;
        case 1 : diary->setChecked(true); break;
        case 2 : anal->setChecked(true); break;
        case 3 : train->setChecked(true); break;
    }
}

void
GcScopeBar::setShowSidebar(bool showSidebar)
{
    return; //XXX moving to main toolbar
    static QPixmap *hide = new QPixmap(":images/mac/hide.png");
    static QPixmap *show = new QPixmap(":images/mac/show.png");

    state = showSidebar;
    if (showSidebar == false) {
        showHide->setImage(show);
        showHide->setText("Show");
    } else {
        showHide->setImage(hide);
        showHide->setText("Hide");
    }
    showHide->setChecked(false);
}

void
GcScopeBar::showHideClicked()
{
    return; //XXX moving to main toolbar
    state = !state;
    emit showSideBar(state);
    setShowSidebar(state);
}

void
GcScopeBar::setEnabledHideButton(bool EnableHideButton) {
    return;; //XXX moving to toolbar
    showHide->setEnabled(EnableHideButton);
}

