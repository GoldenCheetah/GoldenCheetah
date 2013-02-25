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

GcSplitter::GcSplitter(Qt::Orientation orientation, QWidget *parent) : QWidget(parent)
{
    setContentsMargins(0,0,0,0);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignTop);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);

    control = new GcSplitterControl(this);
    layout->addWidget(control);

    splitter = new GcSubSplitter(orientation, control, this);
    splitter->setHandleWidth(1);
    splitter->setFrameStyle(QFrame::NoFrame);
    splitter->setContentsMargins(0,0,0,0);
    layout->addWidget(splitter);

    connect(splitter,SIGNAL(splitterMoved(int,int)), this, SLOT(subSplitterMoved(int,int)));
}

void
GcSplitter::setOpaqueResize(bool opaque)
{
    return splitter->setOpaqueResize(opaque);
}

void
GcSplitter::setSizes(const QList<int> &list)
{
    return splitter->setSizes(list);
}

QByteArray
GcSplitter::saveState() const
{
    return splitter->saveState();
}

bool
GcSplitter::restoreState(const QByteArray &state)
{
    return splitter->restoreState(state);
}

void
GcSplitter::subSplitterMoved(int pos, int index)
{
   emit splitterMoved(pos, index);
}

void
GcSplitter::addWidget(QWidget *widget)
{
   splitter->addWidget(widget);
}

void
GcSplitter::insertWidget(int index, QWidget *widget)
{
    splitter->insertWidget(index, widget);
}

GcSubSplitter::GcSubSplitter(Qt::Orientation orientation, GcSplitterControl *control, QWidget *parent) : QSplitter(orientation, parent), control(control)
{
    _insertedWidget = NULL;
    QLabel *fake = new QLabel("fake");
    fake->setFixedHeight(0);
    setHandleWidth(0);
    QSplitter::addWidget(fake);
}

void
GcSubSplitter::addWidget(QWidget *widget)
{
    _insertedWidget = widget;
    QSplitter::addWidget(widget);
}

void
GcSubSplitter::insertWidget(int index, QWidget *widget)
{
    _insertedWidget = widget;
    QSplitter::insertWidget(index, widget);
}

QSplitterHandle*
GcSubSplitter::createHandle()
{
    if(_insertedWidget != 0) {
        GcSplitterItem* _item = dynamic_cast<GcSplitterItem*>(_insertedWidget);
        if(_item != 0) {
            _item->splitterHandle = new GcSplitterHandle(_item->title, _item, orientation(), this);
            _item->splitterHandle->addActions(_item->actions());
            QAction *action = new QAction(_item->icon, _item->title, this);
            control->addAction(action);
            connect(action, SIGNAL(triggered(void)), _item, SLOT(selectHandle(void)));
            return _item->splitterHandle;
        }
    }

    return QSplitter::createHandle();
}

GcSplitterHandle::GcSplitterHandle(QString title, GcSplitterItem *widget, Qt::Orientation orientation, GcSubSplitter *parent) : QSplitterHandle(orientation, parent), widget(widget), title(title)
{
    setContentsMargins(0,0,0,0);
    setFixedHeight(24);

    gcSplitter = parent;

    titleLayout = new QHBoxLayout(this);
    titleLayout->setContentsMargins(0,0,0,0);
    titleLayout->setSpacing(2);

    titleLabel = new QLabel(title, this);
#ifdef Q_OS_MAC
    titleLabel->setFixedHeight(16);
    titleLabel->setFont(QFont("Helvetica", 11, QFont::Normal));
#else
    titleLabel->setFont(QFont("Helvetica", 10, QFont::Normal));
#endif

    showHide = new QPushButton(this);
    showHide->setStyleSheet("QPushButton {color : blue;background: transparent}");
    showHide->setFixedWidth(16);
    state = true;
    //showHideClicked();
    //titleLayout->addWidget(showHide);
    //connect(showHide, SIGNAL(clicked(bool)), this, SLOT(showHideClicked()));

    titleLayout->addSpacing(10);

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    titleToolbar = new QToolBar(this);
    titleToolbar->setFixedHeight(10);
    titleToolbar->setIconSize(QSize(8,8));
    titleToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    titleLayout->addWidget(titleToolbar);
}

QSize
GcSplitterHandle::sizeHint() const
{
    return QSize(200, 20);
}

GcSubSplitter*
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
    painter.drawTiledPixmap(all, isActiveWindow() ? active : inactive);
}

void
GcSplitterHandle::setExpanded(bool expanded)
{
    //XXX static QPixmap *hide = new QPixmap(":images/mac/hide.png");
    //XXX static QPixmap *show = new QPixmap(":images/mac/show.png");

    state = expanded;
    if (expanded == false) {
        showHide->setIcon(widget->icon);//QIcon(*show));
        titleLabel->setStyleSheet("QLabel { color: gray; }");
        fullHeight = widget->height();
        widget->setFixedHeight(0);
    } else {
        showHide->setIcon(widget->icon);//QIcon(*hide));
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

GcSplitterControl::GcSplitterControl(QWidget *parent) : QToolBar(parent)
{
    setContentsMargins(0,0,0,0);
    setFixedHeight(25);
    setIconSize(QSize(18,18));
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setAutoFillBackground(false);

}

void
GcSplitterControl::paintEvent(QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
    //QToolBar::paintEvent(event);
}

void
GcSplitterControl::paintBackground(QPaintEvent *)
{
    static QPixmap active = QPixmap(":images/mac/scope-active.png");
    static QPixmap inactive = QPixmap(":images/scope-inactive.png");

    // setup a painter and the area to paint
    QPainter painter(this);

    // background light gray for now?
    QRect all(0,0,width(),height());
    painter.drawTiledPixmap(all, isActiveWindow() ? active : inactive);
}

void
GcSplitterControl::selectAction()
{
    this->setVisible(!this->isVisible());
    /*this->setBaseSize(width(), parentWidget()->height());
    this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);*/
}

GcSplitterItem::GcSplitterItem(QString title, QIcon icon, QWidget *parent) : QWidget(parent), title(title), icon(icon)
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

void
GcSplitterItem::selectHandle()
{
    this->setVisible(!this->isVisible());
    /*this->setBaseSize(width(), parentWidget()->height());
    this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);*/
}

GcSplitterItem::~GcSplitterItem()
{
}

