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
#include "HelpWhatsThis.h"

GcScopeBar::GcScopeBar(Context *context) : QWidget(context->mainWindow), context(context)
{

    setFixedHeight(23 *dpiYFactor);
    setContentsMargins(10,0,10,0);
    layout = new QHBoxLayout(this);
    layout->setSpacing(2 *dpiYFactor);
    layout->setContentsMargins(0,0,0,0);

    searchLabel = new GcLabel(tr("Search/Filter:"));
    searchLabel->setYOff(1);
    searchLabel->setFixedHeight(20 *dpiYFactor);
    searchLabel->setHighlighted(true);
    QFont font;
    layout->addWidget(searchLabel);
    searchLabel->hide();

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(filterChanged()), this, SLOT(setHighlighted()));
    connect(context, SIGNAL(compareIntervalsStateChanged(bool)), this, SLOT(setCompare()));
    connect(context, SIGNAL(compareDateRangesStateChanged(bool)), this, SLOT(setCompare()));

    //We always use QT widgets now
    home = new GcScopeButton(this);
#ifdef GC_HAVE_ICAL
    diary = new GcScopeButton(this);
#endif
    anal = new GcScopeButton(this);
    train = new GcScopeButton(this);

    // now set the text for each one
    home->setText(tr("Trends"));
    layout->addWidget(home);
    connect(home, SIGNAL(clicked(bool)), this, SLOT(clickedHome()));

    HelpWhatsThis *helpHome = new HelpWhatsThis(home);
    home->setWhatsThis(helpHome->getWhatsThisText(HelpWhatsThis::ScopeBar_Trends));

#ifdef GC_HAVE_ICAL
    diary->setText(tr("Diary"));
    layout->addWidget(diary);
    connect(diary, SIGNAL(clicked(bool)), this, SLOT(clickedDiary()));

    HelpWhatsThis *helpDiary = new HelpWhatsThis(diary);
    diary->setWhatsThis(helpDiary->getWhatsThisText(HelpWhatsThis::ScopeBar_Diary));

#endif

    anal->setText(tr("Activities"));
    anal->setChecked(true);
    layout->addWidget(anal);
    connect(anal, SIGNAL(clicked(bool)), this, SLOT(clickedAnal()));

    HelpWhatsThis *helpAnal = new HelpWhatsThis(anal);
    anal->setWhatsThis(helpAnal->getWhatsThisText(HelpWhatsThis::ScopeBar_Rides));

    train->setText(tr("Train"));
    layout->addWidget(train);
    connect(train, SIGNAL(clicked(bool)), this, SLOT(clickedTrain()));

    HelpWhatsThis *helpTrain = new HelpWhatsThis(train);
    train->setWhatsThis(helpTrain->getWhatsThisText(HelpWhatsThis::ScopeBar_Train));

    configChanged(255);
}

void
GcScopeBar::configChanged(qint32 reason)
{
    if ((reason & CONFIG_APPEARANCE) == 0) return;

    QFont font; // default
    QFontMetrics fm(font);

    // we need to be the right height
    setFixedHeight(fm.height() + (7 * dpiYFactor));

    // set font sizes
#ifndef Q_OS_MAC
    font.setWeight(QFont::Bold);
#endif
    searchLabel->setFont(font);
    searchLabel->setFixedHeight(fm.height() + (7*dpiYFactor));

    // Windows / Linux uses GcScopeButton - pushbutton
    home->setFixedHeight(fm.height() + (7*dpiYFactor));
    home->setFont(font);
#ifdef GC_HAVE_ICAL
    diary->setFixedHeight(fm.height() + (7*dpiYFactor));
    diary->setFont(font);
#endif
    anal->setFixedHeight(fm.height() + (7*dpiYFactor));
    anal->setFont(font);
    train->setFixedHeight(fm.height() + (7*dpiYFactor));
    train->setFont(font);

    int width = fm.width(tr("Trends"));
    home->setWidth(width+(30*dpiXFactor));

    width = fm.width(tr("Activities"));
    anal->setWidth(width+(30*dpiXFactor));

    width = fm.width(tr("Train"));
    train->setWidth(width+(30*dpiXFactor));

#ifdef GC_HAVE_ICAL
    width = fm.width(tr("Diary"));
    diary->setWidth(width+(30*dpiXFactor));
#endif
}

void
GcScopeBar::setHighlighted()
{
    if (context->isfiltered) {
        searchLabel->setHighlighted(true);
        searchLabel->show();
        home->setHighlighted(true);
        anal->setHighlighted(true);
#ifdef GC_HAVE_ICAL
        diary->setHighlighted(true);
#endif
    } else {
        searchLabel->setHighlighted(false);
        searchLabel->hide();
        home->setHighlighted(false);
        anal->setHighlighted(false);
#ifdef GC_HAVE_ICAL
        diary->setHighlighted(false);
#endif
    }
}

void
GcScopeBar::setCompare()
{
    home->setRed(context->isCompareDateRanges);
    anal->setRed(context->isCompareIntervals);
    repaint();
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
    QLinearGradient linearGradient = GCColor::instance()->linearGradient(23*dpiYFactor, isActiveWindow());
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
GcScopeBar::clickedInterval()
{
    home->setChecked(false);
#ifdef GC_HAVE_ICAL
    diary->setChecked(false);
#endif
    anal->setChecked(false);
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
#else
    if (anal->isChecked()) return 1;
    if (train->isChecked()) return 2;
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

GcScopeButton::GcScopeButton(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(20);
    setFixedWidth(60);
    red = highlighted = checked = false;
    QFont font;
    font.setPointSize(10);
    //font.setWeight(QFont::Black);
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
        painter.drawRoundedRect(body, 7,6);
        if (highlighted) {
            QColor over = GColor(CCALCURRENT);
            over.setAlpha(180);
            painter.setBrush(over);
            painter.drawRoundedRect(body, 7,6);
        }
        if (red) {
            QColor over = QColor(Qt::red);
            over.setAlpha(180);
            painter.setBrush(over);
            painter.drawRoundedRect(body, 7,6);
        }
    } else if (checked && !underMouse()) {
        painter.setBrush(QBrush(QColor(120,120,120)));     
        painter.drawRoundedRect(body, 7,6);
        if (highlighted) {
            QColor over = GColor(CCALCURRENT);
            over.setAlpha(180);
            painter.setBrush(over);
            painter.drawRoundedRect(body, 7,6);
        }
        if (red) {
            QColor over = QColor(Qt::red);
            over.setAlpha(180);
            painter.setBrush(over);
            painter.drawRoundedRect(body, 7,6);
        }
    } else if (!checked && underMouse()) {
        painter.setBrush(QBrush(QColor(180,180,180)));     
        painter.drawRoundedRect(body, 7,6);
    } else if (!checked && !underMouse()) {
    }

    // now paint the text
    // don't do all that offset nonsense for flat style
    // set fg checked and unchecked colors
    QColor checkedCol(240,240,240), uncheckedCol(30,30,30,200);
    if (!GCColor::instance()->isFlat()) {

        // metal style
        painter.setPen((underMouse() || checked) ? QColor(50,50,50) : Qt::white);
        painter.drawText(off, text, Qt::AlignVCenter | Qt::AlignCenter);

    } else {

        // adjust colors if flat and dark
        if (GCColor::instance()->luminance(CCHROME) < 127) {
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
