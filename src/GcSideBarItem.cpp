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
    QImage gray8 = pngImage.convertToFormat(QImage::Format_Indexed8);
    QImage white8 = pngImage.convertToFormat(QImage::Format_Indexed8);
    gray8.setColor(0, QColor(80,80,80, 170).rgb());
    white8.setColor(0, QColor(255,255,255, 255).rgb());

    // now convert to a format we can paint with!
    QImage white = white8.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QImage gray = gray8.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    QPainter painter;
    painter.begin(&white);
    painter.setBackgroundMode(Qt::TransparentMode);
    painter.drawImage(0,-1, gray);
    painter.end();

    QIcon icon(QPixmap::fromImage(white));
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
    layout->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);
    layout->setContentsMargins(0,0,0,0);
    layout->addWidget(splitter);
    layout->addWidget(control);

    connect(splitter,SIGNAL(splitterMoved(int,int)), this, SLOT(subSplitterMoved(int,int)));
}

void
GcSplitter::prepare(QString cyclist, QString name)
{
    this->name = name;
    this->cyclist = cyclist;

    QWidget *spacer = new QWidget(this);
    spacer->setAutoFillBackground(false);
    spacer->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    control->addWidget(spacer);

    // get saved state
    QString statesetting = QString("splitter/%1/sizes").arg(name);
    QVariant sizes = appsettings->cvalue(cyclist, statesetting); 
    if (sizes != QVariant()) {
        splitter->restoreState(sizes.toByteArray());
        splitter->setOpaqueResize(true); // redraw when released, snappier UI
    }

    // should we hide / show each widget?
    for(int i=0; i<splitter->count(); i++) {
        QString hidesetting = QString("splitter/%1/hide/%2").arg(name).arg(i);
        QVariant hidden = appsettings->cvalue(cyclist, hidesetting);
        if (i && hidden != QVariant()) {
            if (hidden.toBool() == true) {
                splitter->widget(i)->hide();
            }
        }
    }
}

void
GcSplitter::saveSettings()
{
    // get saved state
    QString statesetting = QString("splitter/%1/sizes").arg(name);
    appsettings->setCValue(cyclist, statesetting, splitter->saveState()); 

    // should we hide / show each widget?
    for(int i=0; i<splitter->count(); i++) {
        QString hidesetting = QString("splitter/%1/hide/%2").arg(name).arg(i);
        appsettings->setCValue(cyclist, hidesetting, QVariant(splitter->widget(i)->isHidden()));
    }
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
   saveSettings();
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

GcSubSplitter::GcSubSplitter(Qt::Orientation orientation, GcSplitterControl *control, GcSplitter *parent) : QSplitter(orientation, parent), control(control), gcSplitter (parent)
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
            connect(_item->controlAction, SIGNAL(triggered(void)), gcSplitter, SLOT(saveSettings()));
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
    titleLabel->setXOff(0);

#ifdef Q_OS_MAC
    int shade = 178;
    int inshade = 225;
#else
    int shade = 200;
    int inshade = 250;
#endif
    active = QLinearGradient(0, 0, 0, 23);
    active.setColorAt(0.0, QColor(shade,shade,shade, 100));
    active.setColorAt(0.5, QColor(shade,shade,shade, 180));
    active.setColorAt(1.0, QColor(shade,shade,shade, 255));
    active.setSpread(QGradient::PadSpread);
    inactive = QLinearGradient(0, 0, 0, 23);
    inactive.setColorAt(0.0, QColor(inshade,inshade,inshade, 100));
    inactive.setColorAt(0.5, QColor(inshade,inshade,inshade, 180));
    inactive.setColorAt(1.0, QColor(inshade,inshade,inshade, 255));
    inactive.setSpread(QGradient::PadSpread);

    QFont font;
#ifdef Q_OS_MAC
    titleLabel->setFixedHeight(16);
    titleLabel->setYOff(1);
    font.setFamily("Lucida Grande");
    font.setPointSize(11);
#else
    titleLabel->setYOff(1);
    font.setFamily("Helvetica");
#ifdef WIN32
    font.setPointSize(8);
#else
    font.setPointSize(10);
#endif
#endif
    font.setWeight(QFont::Black);
    titleLabel->setFont(font);

    titleLayout->addSpacing(10);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

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
GcSplitterHandle::addAction(QAction *)
{
    //not used anyway titleToolbar->addAction(action);
}

void
GcSplitterHandle::addActions(QList<QAction*> actions)
{
    foreach(QAction *action, actions) {
        QPushButton *p = new QPushButton(action->icon(), "", this);
        p->setAutoFillBackground(false);
        p->setFlat(true);
        p->setFixedSize(20,20);
        p->setIconSize(QSize(10,10));
        p->setFocusPolicy(Qt::NoFocus);
        titleLayout->addWidget(p);
        connect(p, SIGNAL(clicked()), action, SLOT(trigger()));
    }
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
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, isActiveWindow() ? active : inactive);

    QPen black(QColor(100,100,100,200));
    painter.setPen(black);
    painter.drawLine(0,height()-1, width()-1, height()-1);

    QPen gray(QColor(230,230,230));
    painter.setPen(gray);
    painter.drawLine(0,0, width()-1, 0);

    painter.restore();
}

GcSplitterControl::GcSplitterControl(QWidget *parent) : QToolBar(parent)
{
    setContentsMargins(0,0,0,0);
    setFixedHeight(20);
    setIconSize(QSize(14,14));
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setAutoFillBackground(false);

#ifdef Q_OS_MAC
    int shade = 178;
    int inshade = 225;
#else
    int shade = 200;
    int inshade = 250;
#endif
    active = QLinearGradient(0, 0, 0, 20);
    active.setColorAt(0.0, QColor(shade,shade,shade, 100));
    active.setColorAt(0.5, QColor(shade,shade,shade, 180));
    active.setColorAt(1.0, QColor(shade,shade,shade, 255));
    active.setSpread(QGradient::PadSpread);
    inactive = QLinearGradient(0, 0, 0, 20);
    inactive.setColorAt(0.0, QColor(inshade,inshade,inshade, 100));
    inactive.setColorAt(0.5, QColor(inshade,inshade,inshade, 180));
    inactive.setColorAt(1.0, QColor(inshade,inshade,inshade, 255));
    inactive.setSpread(QGradient::PadSpread);

    QWidget *spacer = new QWidget(this);
    spacer->setAutoFillBackground(false);
    spacer->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    addWidget(spacer);
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
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, isActiveWindow() ? active : inactive);
    QPen gray(QColor(230,230,230));
    painter.setPen(gray);
    painter.drawLine(0,0, width()-1, 0);

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

