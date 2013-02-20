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

GcSplitterHandle::GcSplitterHandle(QString title, GcSplitterItem *widget, Qt::Orientation orientation, GcSplitter *parent) : QSplitterHandle(orientation, parent), widget(widget), title(title)
{
    setContentsMargins(0,0,0,0);
    setFixedHeight(24);

    gcSplitter = parent;

    titleLayout = new QHBoxLayout(this);
    titleLayout->setContentsMargins(0,0,0,0);
    titleLayout->setSpacing(0);

    titleLabel = new QLabel(title, this);
#ifdef Q_OS_MAC
    titleLabel->setFixedHeight(16);
    titleLabel->setFont(QFont("Helvetica", 11, QFont::Normal));
#else
    titleLabel->setFont(QFont("Helvetica", 10, QFont::Normal));
#endif

    showHide = new QPushButton(this);
    showHide->setStyleSheet("QPushButton {color : blue;background: transparent}");
    showHide->setFixedWidth(20);
    state = false;
    showHideClicked();
    titleLayout->addWidget(showHide);
    connect(showHide, SIGNAL(clicked(bool)), this, SLOT(showHideClicked()));

    titleLayout->addSpacing(5);

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    titleToolbar = new QToolBar(this);
    titleToolbar->setFixedHeight(16);
    titleToolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    titleLayout->addWidget(titleToolbar);
}

QSize
GcSplitterHandle::sizeHint() const
{
    return QSize(200, 20);
}

GcSplitter*
GcSplitterHandle::splitter() const
{
    return gcSplitter;
}

void
GcSplitterHandle::addAction(QAction *action)
{
    titleToolbar->addAction(action);
}

void
GcSplitterHandle::addActions(QList<QAction*> actions)
{
    titleToolbar->addActions(actions);
}

void
GcSplitterHandle::paintEvent(QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
    QWidget::paintEvent(event);
}

void
GcSplitterHandle::paintBackground(QPaintEvent *)
{
    static QPixmap active = QPixmap(":images/mac/scope-active.png");
    static QPixmap inactive = QPixmap(":images/scope-inactive.png");

    // setup a painter and the area to paint
    QPainter painter(this);

    // background light gray for now?
    QRect all(0,0,width(),height());
    painter.drawTiledPixmap(all, state ? active : inactive);
}

void
GcSplitterHandle::setExpanded(bool expanded)
{
    static QPixmap *hide = new QPixmap(":images/mac/hide.png");
    static QPixmap *show = new QPixmap(":images/mac/show.png");

    state = expanded;
    if (expanded == false) {
        showHide->setIcon(QIcon(*show));
        titleLabel->setStyleSheet("QLabel { color: gray; }");
        fullHeight = widget->height();
        widget->setFixedHeight(0);
    } else {
        showHide->setIcon(QIcon(*hide));
        titleLabel->setStyleSheet("QLabel { color: black; }");
        widget->setBaseSize(widget->width(), fullHeight);
        widget->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);
        widget->setMinimumSize(0,0);
    }
    showHide->setChecked(false);
    repaint();
}

void
GcSplitterHandle::showHideClicked()
{
    state = !state;
    setExpanded(state);
}

GcSplitter::GcSplitter(Qt::Orientation orientation, QWidget *parent) : QSplitter(orientation, parent)
{
    _insertedWidget = NULL;
    QLabel *fake = new QLabel("fake");
    fake->setFixedHeight(0);
    setHandleWidth(0);
    addWidget(fake);
}

void
GcSplitter::addWidget(QWidget *widget)
{
    _insertedWidget = widget;
    QSplitter::addWidget(widget);
}

void
GcSplitter::insertWidget(int index, QWidget *widget)
{
    _insertedWidget = widget;
    QSplitter::insertWidget(index, widget);
}

QSplitterHandle*
GcSplitter::createHandle()
{
    if(_insertedWidget != 0) {
        GcSplitterItem* _item = dynamic_cast<GcSplitterItem*>(_insertedWidget);
        if(_item != 0) {
            _item->splitterHandle = new GcSplitterHandle(_item->title, _item, orientation(), this);
            _item->splitterHandle->addActions(actions());
            return _item->splitterHandle;
        }
    }
    return QSplitter::createHandle();
}

GcSplitterItem::GcSplitterItem(QString title, QWidget *parent) : QWidget(parent), title(title)
{
    setContentsMargins(0,0,0,0);
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    content = NULL;
    //titleBar = new GcSideBarTitle(title, this);
    //layout->addWidget(titleBar);
}

void
GcSplitterItem::addWidget(QWidget *p)
{
    content = p;
    p->setContentsMargins(0,0,0,0);
    layout->addWidget(p);
}

GcSplitterItem::~GcSplitterItem()
{
}

