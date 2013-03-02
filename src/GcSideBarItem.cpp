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

QIcon iconFromPNG(QString filename)
{
    QImage pngImage;
    pngImage.load(filename);

    // use muted dark gray color
    QImage result = pngImage.convertToFormat(QImage::Format_Indexed8);
    result.setColor(0, QColor(80,80,80, 255).rgb());

    QIcon icon(QPixmap::fromImage(result));
    return icon;
}

GcSplitter::GcSplitter(Qt::Orientation orientation, QWidget *parent) : QWidget(parent)
{

    setContentsMargins(0,0,0,0);

    control = new GcSplitterControl(this);

    splitter = new GcSubSplitter(orientation, control, this);
    splitter->setHandleWidth(23);
    splitter->setFrameStyle(QFrame::NoFrame);
    splitter->setContentsMargins(0,0,0,0);
    splitter->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setAlignment(Qt::AlignBottom);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(splitter);
    layout->addWidget(control);

    connect(splitter,SIGNAL(splitterMoved(int,int)), this, SLOT(subSplitterMoved(int,int)));
}

void
GcSplitter::setOpaqueResize(bool opaque)
{
    return splitter->setOpaqueResize(opaque);
}

QList<int>
GcSplitter::sizes() const
{
    return splitter->sizes();
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

    // we add a fake widget to ensure the first real widget
    // that is added has a handle (even though it cannot be moved)
    QLabel *fake = new QLabel("fake");
    fake->setFixedHeight(0);
    setHandleWidth(0);
    QSplitter::addWidget(fake);
    //setStretchFactor(0,0);
}

void
GcSubSplitter::addWidget(QWidget *widget)
{
    _insertedWidget = widget;
    QSplitter::addWidget(widget);
    //setStretchFactor(indexOf(widget), 1);
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
            _item->controlAction = new QAction(_item->icon, _item->title, this);
            _item->controlAction->setStatusTip(_item->title);
            control->addAction(_item->controlAction);

            connect(_item->controlAction, SIGNAL(triggered(void)), _item, SLOT(selectHandle(void)));
            return _item->splitterHandle;
        }
    }

    return QSplitter::createHandle();
}

GcSplitterHandle::GcSplitterHandle(QString title, GcSplitterItem *widget, Qt::Orientation orientation, GcSubSplitter *parent) : QSplitterHandle(orientation, parent), widget(widget), title(title)
{
    setContentsMargins(0,0,0,0);
    setFixedHeight(23);

    gcSplitter = parent;

    titleLayout = new QHBoxLayout(this);
    titleLayout->setContentsMargins(0,0,0,0);
    titleLayout->setSpacing(2);

    titleLabel = new GcLabel(title, this);
    titleLabel->setXOff(1);

    QFont font;
#ifdef Q_OS_MAC
    titleLabel->setFixedHeight(16);
    titleLabel->setYOff(2);
    font.setFamily("Lucida Grande");
    font.setPointSize(11);
#else
    titleLabel->setYOff(2);
    font.setFamily("Helvetica");
    font.setPointSize(10);
#endif
    font.setWeight(QFont::Black);
    titleLabel->setFont(font);

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
#ifndef Q_OS_MAC
    titleToolbar->setFixedHeight(23);
#else
    titleToolbar->setFixedHeight(10);
#endif
    titleToolbar->setIconSize(QSize(8,8));
    titleToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    titleLayout->addWidget(titleToolbar);

    setCursor(Qt::ArrowCursor);
}

QSize
GcSplitterHandle::sizeHint() const
{
    return QSize(200, 23);
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
    // setup a painter and the area to paint
    QPainter painter(this);

    painter.save();
    QRect all(0,0,width(),height());

    // fill with a linear gradient
#ifdef Q_OS_MAC
    int shade = isActiveWindow() ? 178 : 225;
#else
    int shade = isActiveWindow() ? 200 : 250;
#endif
    QLinearGradient linearGradient(0, 0, 0, height());
    linearGradient.setColorAt(0.0, QColor(shade,shade,shade, 100));
    linearGradient.setColorAt(0.5, QColor(shade,shade,shade, 180));
    linearGradient.setColorAt(1.0, QColor(shade,shade,shade, 255));
    linearGradient.setSpread(QGradient::PadSpread);
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, linearGradient);

    QPen black(QColor(100,100,100,200));
    painter.setPen(black);
    painter.drawLine(0,height()-1, width()-1, height()-1);
    painter.restore();
}

void
GcSplitterHandle::setExpanded(bool expanded)
{

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
    setFixedHeight(20);
    setIconSize(QSize(14,14));
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
    QRect all(0,0,width(),height());

    // setup a painter and the area to paint
    QPainter painter(this);

    // fill with a linear gradient
#ifdef Q_OS_MAC
    int shade = isActiveWindow() ? 178 : 225;
#else
    int shade = isActiveWindow() ? 200 : 250;
#endif
    QLinearGradient linearGradient(0, 0, 0, height());
    linearGradient.setColorAt(0.0, QColor(shade,shade,shade, 100));
    linearGradient.setColorAt(0.5, QColor(shade,shade,shade, 180));
    linearGradient.setColorAt(1.0, QColor(shade,shade,shade, 255));
    linearGradient.setSpread(QGradient::PadSpread);
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, linearGradient);
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
    //p->setContentsMargins(0,0,0,0);
    layout->addWidget(p);
}

void
GcSplitterItem::selectHandle()
{
    //splitterHandle->gcSplitter->setStretchFactor(splitterHandle->index, this->isVisible() ? 0 : 1);
    this->setVisible(!this->isVisible());
    controlAction->setChecked(this->isVisible());
    /*this->setBaseSize(width(), parentWidget()->height());
    this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);*/
}

GcSplitterItem::~GcSplitterItem()
{
}

