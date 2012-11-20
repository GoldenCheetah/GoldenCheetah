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
#include "QtMacButton.h"

GcScopeBar::GcScopeBar(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(25);
    setContentsMargins(10,0,10,0);
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(2);
    layout->setContentsMargins(0,0,0,0);
    installEventFilter(this);

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
    layout->addStretch();
    connect(train, SIGNAL(clicked(bool)), this, SLOT(clickedTrain()));
}

GcScopeBar::~GcScopeBar()
{
}

void
GcScopeBar::paintEvent (QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
}

void
GcScopeBar::paintBackground(QPaintEvent *)
{
    static QPixmap active = QPixmap(":images/mac/scope-active.png");
    static QPixmap inactive = QPixmap(":images/scope-inactive.png");

    // setup a painter and the area to paint
    QPainter painter(this);

    // background light gray for now?
    QRect all(0,0,width(),height());
    painter.drawTiledPixmap(all, isActiveWindow() ? active : inactive);
}

bool
GcScopeBar::eventFilter(QObject *obj, QEvent *e)
{
    return false;
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
