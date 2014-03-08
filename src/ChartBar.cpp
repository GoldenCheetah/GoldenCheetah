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

#include "ChartBar.h"
#include "GcScopeBar.h"
#include "DiaryWindow.h"
#include "DiarySidebar.h"
#include "Context.h"
#include "QtMacButton.h"

ChartBar::ChartBar(Context *context) : QWidget(context->mainWindow), context(context)
{

    setFixedHeight(23);
    setContentsMargins(10,0,10,0);
    QHBoxLayout *vlayout = new QHBoxLayout(this);
    vlayout->setSpacing(0);
    vlayout->setContentsMargins(0,0,0,0);

    layout = new QHBoxLayout; 
    layout->setSpacing(2);
    layout->setContentsMargins(0,0,0,0);

    vlayout->addLayout(layout);
    vlayout->addStretch();


#ifdef Q_OS_MAC
    buttonFont.setFamily("Lucida Grande");
#else
    buttonFont.setFamily("Helvetica");
#endif
#ifdef WIN32
    buttonFont.setPointSize(8);
#else
    buttonFont.setPointSize(10);
#endif
    buttonFont.setWeight(QFont::Black);

    // linear gradients
#ifdef Q_OS_MAC
    int shade = 178;
    int inshade = 225;
#else
    int shade = 200;
    int inshade = 250;
#endif
    active = QLinearGradient(0, 0, 0, true ? 23 :18);
    active.setColorAt(0.0, QColor(shade,shade,shade, 100));
    active.setColorAt(0.5, QColor(shade,shade,shade, 180));
    active.setColorAt(1.0, QColor(shade,shade,shade, 255));
    active.setSpread(QGradient::PadSpread);
    inactive = QLinearGradient(0, 0, 0, true ? 23 :18);
    inactive.setColorAt(0.0, QColor(inshade,inshade,inshade, 100));
    inactive.setColorAt(0.5, QColor(inshade,inshade,inshade, 180));
    inactive.setColorAt(1.0, QColor(inshade,inshade,inshade, 255));
    inactive.setSpread(QGradient::PadSpread);

    signalMapper = new QSignalMapper(this); // maps each option
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(clicked(int)));
}

void
ChartBar::addWidget(QString title)
{
#ifdef Q_OS_MAC
    QtMacButton *newbutton = new QtMacButton(this, QtMacButton::Recessed);
#else
    GcScopeButton *newbutton = new GcScopeButton(this);
#endif
    newbutton->setText(title);
    newbutton->setFont(buttonFont);

    // make the right size
    QFontMetrics fontMetric(buttonFont);
    int width = fontMetric.width(title);
    newbutton->setWidth(width+20);

    // add to layout
    layout->addWidget(newbutton);
    buttons << newbutton;

    // map signals
    connect(newbutton, SIGNAL(clicked(bool)), signalMapper, SLOT(map()));
    signalMapper->setMapping(newbutton, buttons.count()-1);
}

void
ChartBar::clear()
{
    foreach(GcScopeButton *button, buttons) {
        layout->removeWidget(button);
        delete button;
    }
    buttons.clear();
}

void
ChartBar::removeWidget(int index)
{
    layout->removeWidget(buttons[index]);
    delete buttons[index];
    buttons.remove(index);

    // reset mappings
    for (int i=0; i<buttons.count(); i++)
        signalMapper->setMapping(buttons[i], i);

}

void
ChartBar::clicked(int index)
{
    setUpdatesEnabled(false);
    // set selected
    for(int i=0; i<buttons.count(); i++) {
        buttons[i]->setChecked(i == index);
    }
    setUpdatesEnabled(true);
    emit currentIndexChanged(index);
}


#if 0
ChartBar::setHighlighted()
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
#endif

ChartBar::~ChartBar()
{
}

void
ChartBar::paintEvent (QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
    QWidget::paintEvent(event);
}

// paint is the same as sidebar
void
ChartBar::paintBackground(QPaintEvent *)
{
    // setup a painter and the area to paint
    QPainter painter(this);

    painter.save();
    QRect all(0,0,width(),height());

    // fill with a linear gradient
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, QColor(Qt::white));
    painter.fillRect(all, isActiveWindow() ? active : inactive);

    QPen black(QColor(100,100,100,200));
    painter.setPen(black);
    painter.drawLine(0,height()-1, width()-1, height()-1);

    QPen gray(QColor(230,230,230));
    painter.setPen(gray);
    painter.drawLine(0,0, width()-1, 0);

    painter.restore();
}

#if 0
int
ChartBar::selected()
{
    if (home->isChecked()) return 0;
#ifdef GC_HAVE_ICAL
    if (diary->isChecked()) return 1;
    if (anal->isChecked()) return 2;
    if (train->isChecked()) return 3;
#else
    if (anal->isChecked()) return 1;
    if (train->isChecked()) return 2;
#endif

    // never gets here - shutup compiler
    return 0;
}

void
ChartBar::setSelected(int index)
{
    // we're already there
    if (index == selected()) return;

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
#endif
