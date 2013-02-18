/*
 * Copyright (c) 2013 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#include "GcSideBarItem.h"
#include "GcCalendar.h"

GcSideBarTitle::GcSideBarTitle(QString title, GcSideBarItem *parent) : QWidget(parent), parent(parent)
{
    setContentsMargins(0,0,0,0);
    setFixedHeight(24);

    titleLayout = new QHBoxLayout(this);
    titleLayout->setContentsMargins(2,2,2,2);

    titleLabel = new QLabel(title, this);
    titleLabel->setFont(QFont("Helvetica", 10, QFont::Normal));
    parent->state = false;

    showHide = new QPushButton(this);
    showHide->setStyleSheet("QPushButton {color : blue;background: transparent}");
    showHide->setFixedWidth(20);
    showHideClicked();
    titleLayout->addWidget(showHide);
    connect(showHide, SIGNAL(clicked(bool)), this, SLOT(showHideClicked()));

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    titleToolbar = new QToolBar(this);
    titleToolbar->setFixedHeight(20);
    titleToolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);
    titleToolbar->setFocusPolicy(Qt::NoFocus);

    titleLayout->addWidget(titleToolbar);
}

void
GcSideBarTitle::addAction(QAction *action)
{
    titleToolbar->addAction(action);
}

GcSideBarTitle::~GcSideBarTitle()
{
}

void
GcSideBarTitle::paintEvent (QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
    QWidget::paintEvent(event);
}

void
GcSideBarTitle::paintBackground(QPaintEvent *)
{
    static QPixmap active = QPixmap(":images/mac/scope-active.png");
    static QPixmap inactive = QPixmap(":images/scope-inactive.png");

    // setup a painter and the area to paint
    QPainter painter(this);

    // background light gray for now?
    QRect all(0,0,width(),height());
    painter.drawTiledPixmap(all, parent->state ? active : inactive);
}

void
GcSideBarTitle::setExpanded(bool expanded)
{
    static QPixmap *hide = new QPixmap(":images/mac/hide.png");
    static QPixmap *show = new QPixmap(":images/mac/show.png");

    parent->state = expanded;
    if (expanded == false) {
        showHide->setIcon(QIcon(*show));
        titleLabel->setStyleSheet("QLabel { color: gray; }");
        if (parent->content != NULL) {
            parent->content->hide();
            fullHeight = parent->height();
            parent->setFixedSize(parent->width(), 24);

        }
    } else {
        showHide->setIcon(QIcon(*hide));
        titleLabel->setStyleSheet("QLabel { color: black; }");
        if (parent->content != NULL) {
            parent->content->show();
            parent->setBaseSize(parent->width(), fullHeight);
            parent->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
            parent->setMinimumSize(0,0);
        }
    }
    showHide->setChecked(false);
    repaint();
}

void
GcSideBarTitle::showHideClicked()
{
    parent->state = !parent->state;
    setExpanded(parent->state);
}

GcSideBarItem::GcSideBarItem(QString title, QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);

    content = NULL;
    titleBar = new GcSideBarTitle(title, this);
    layout->addWidget(titleBar);
}

void
GcSideBarItem::addWidget(QWidget *p)
{
    content = p;
    layout->addWidget(p);
}

void
GcSideBarItem::addAction(QAction *action)
{
    titleBar->addAction(action);
}

GcSideBarItem::~GcSideBarItem()
{
}

