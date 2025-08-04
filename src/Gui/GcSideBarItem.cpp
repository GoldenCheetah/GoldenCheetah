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
#include "DiarySidebar.h"

// fixed width stuff, easier to set globally
// since progating across widgets we don't create
// or own is REALLY painful
static int bigHandle = 23;
static int smallHandle = 18;

// creates an icon with a modern style
QIcon iconFromPNG(QString filename, bool )
{
    QImage pngImage;
    pngImage.load(filename);

    // use muted dark gray color
    QImage gray8 = pngImage.convertToFormat(QImage::Format_Indexed8);
    gray8.setColor(0, QColor(127,127,127,127).rgb());

    return QIcon(QPixmap::fromImage(gray8));
}

QIcon iconFromPNG(QString filename, QSize size)
{
    QImage pngImage;
    pngImage.load(filename);

    // use muted dark gray color
    QImage gray8 = pngImage.convertToFormat(QImage::Format_Indexed8);
    gray8.setColor(0, QColor(127,127,127, 127).rgb());

    return QIcon(QPixmap::fromImage(gray8,Qt::ColorOnly|Qt::PreferDither|Qt::DiffuseAlphaDither).scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

}

//
// GcSplitter -- The sidebar is largely comprised of this which contains a splitter (GcSubSplitter)
//               and a control (GcSplitterControl) at the bottom with icons to show/hide items.
//               The GcSplitter contains 2 or more such items (GcSplitterItem) and each of those will
//               have a special handle (GcSplitterHandle).
//
GcSplitter::GcSplitter(Qt::Orientation orientation, QWidget *parent) : QWidget(parent)
{

    setContentsMargins(0,0,0,0);

    control = new GcSplitterControl(this);

    splitter = new GcSubSplitter(orientation, control, this);
    splitter->setHandleWidth(bigHandle);
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
    QString statesetting = GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/%1/sizes").arg(name);
    QVariant sizes = appsettings->cvalue(cyclist, statesetting); 
    if (sizes != QVariant()) {
        splitter->restoreState(sizes.toByteArray());
        splitter->setOpaqueResize(true); // redraw when released, snappier UI
    }

    // should we hide / show each widget?
    for(int i=0; i<splitter->count(); i++) {
        QString hidesetting = GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/%1/hide/%2").arg(name).arg(i);
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
    QString statesetting = GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/%1/sizes").arg(name);
    appsettings->setCValue(cyclist, statesetting, splitter->saveState()); 

    // should we hide / show each widget?
    for(int i=0; i<splitter->count(); i++) {
        QString hidesetting = GC_QSETTINGS_ATHLETE_LAYOUT+QString("splitter/%1/hide/%2").arg(name).arg(i);
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

//
// GcSubSplitter -- the actual QSplitter widget but needs to have a handle at the top even if there is
//                  only one item shown, hence making it a special widget
//

GcSubSplitter::GcSubSplitter(Qt::Orientation orientation, GcSplitterControl *control, GcSplitter *parent, bool plainstyle) : QSplitter(orientation, parent), control(control), gcSplitter (parent), plainstyle(plainstyle)
{
    _insertedWidget = NULL;

    // we add a fake widget to ensure the first real widget
    // that is added has a handle (even though it cannot be moved)
    if (!plainstyle) {
        QLabel *fake = new QLabel("fake");
        fake->setFixedHeight(0);
        setHandleWidth(0);
        QSplitter::addWidget(fake);
        //setStretchFactor(0,0);
    } else {
        QSplitter::addWidget((QWidget*)control);
    }
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
GcSplitterItem *
GcSubSplitter::removeItem(QString text)
{
    for(int i=1; i<= count(); i++) {
        const GcSplitterHandle *h = static_cast<GcSplitterHandle*>(handle(i));
        if (h && h->title() == text) {
            return static_cast<GcSplitterItem*>(widget(i));
        }
    }
    return NULL; // didn't find it!
}


QSplitterHandle*
GcSubSplitter::createHandle()
{
    if(_insertedWidget != 0) {
        GcSplitterItem* _item = dynamic_cast<GcSplitterItem*>(_insertedWidget);
        if(_item != 0) {
            _item->splitterHandle = new GcSplitterHandle(_item->title, orientation(), this, !plainstyle);

            if (!plainstyle) {
                _item->splitterHandle->addActions(_item->actions());
                _item->controlAction = new QAction(_item->icon, _item->title, this);
                _item->controlAction->setStatusTip(_item->title);
                control->addAction(_item->controlAction);

                connect(_item->controlAction, SIGNAL(triggered(void)), _item, SLOT(selectHandle(void)));
                connect(_item->controlAction, SIGNAL(triggered(void)), gcSplitter, SLOT(saveSettings()));
            }
            return _item->splitterHandle;
        }
    }

    return QSplitter::createHandle();
}

//
// GcSplitterHandle -- the handle shown at the top of a sidebar item with a title and a menu button
//                     the user drags this to resize the sidebar contents up and down.
//

GcSplitterHandle::GcSplitterHandle(QString title, Qt::Orientation orientation, QSplitter *parent, bool metal) : QSplitterHandle(orientation, parent), _title(title), metal(metal)
{
    
    init(title, orientation, NULL, NULL, NULL, parent, metal);
}

GcSplitterHandle::GcSplitterHandle(QString title, Qt::Orientation orientation, 
                QWidget *left, QWidget *mid, QWidget *right,
                QSplitter *parent, bool metal) : QSplitterHandle(orientation, parent), _title(title), metal(metal)
{
    init(title, orientation, left, mid, right, parent, metal);
}

void
GcSplitterHandle::init(QString title, Qt::Orientation orientation, 
                QWidget *left, QWidget *mid, QWidget *right, QSplitter *parent, bool metal) 
{
    Q_UNUSED(orientation);

    setContentsMargins(0,0,0,0);

    gcSplitter = (GcSubSplitter*)parent;

    titleLayout = new QHBoxLayout(this);
    titleLayout->setContentsMargins(0,0,0,0);
    titleLayout->setSpacing(2 *dpiXFactor);

    titleLabel = new GcLabel(title, this);
    titleLabel->setXOff(0);
    titleLabel->setChrome(true);

    // set handle size according to font metric
    QFont font;
    QFontMetrics fm(font);
    bigHandle = fm.height() + 16;
    smallHandle = fm.height() + 5;

    // use the sizes as set
    if (metal) setFixedHeight(bigHandle);
    else setFixedHeight(smallHandle);

#ifdef Q_OS_MAC
    //titleLabel->setFixedHeight(16);
    titleLabel->setYOff(1);
    //font.setPointSize(11);
#else
    titleLabel->setYOff(1);
    //font.setPointSize(10);
    font.setWeight(QFont::Bold);
#endif
    titleLabel->setFont(font);

    titleLayout->addSpacing(10);
    titleLayout->addWidget(titleLabel);

    // widgets
    if (left) titleLayout->addWidget(left);
    titleLayout->addStretch();

    if (mid) titleLayout->addWidget(mid);
    //titleLayout->addStretch();

    if (right) titleLayout->addWidget(right);
    setCursor(Qt::ArrowCursor);
}

QSize
GcSplitterHandle::sizeHint() const
{
    return QSize(200, metal ? bigHandle :smallHandle);
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
        QToolButton *p = new QToolButton(this);
        p->setStyleSheet("QToolButton { border: none; padding: 0px; }");
        p->setAutoFillBackground(false);
        p->setFixedSize(22*dpiXFactor,22*dpiYFactor);
        p->setIcon(action->icon());
        p->setIconSize(QSize(10*dpiXFactor,10*dpiYFactor));
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
    painter.fillRect(all, QColor(Qt::white));

    QLinearGradient active = GCColor::linearGradient(height(), true, !metal);
    QLinearGradient inactive = GCColor::linearGradient(height(), false, !metal);

    painter.fillRect(all, isActiveWindow() ? active : inactive);
    painter.restore();
}

//
// GcSplitterControl - the icon bar at the bottom of the left sidebar
//

GcSplitterControl::GcSplitterControl(QWidget *parent) : QToolBar(parent)
{
    setContentsMargins(0,0,0,0);
    setFixedHeight(22 *dpiYFactor);
    setIconSize(QSize(18 *dpiXFactor,18 *dpiYFactor));
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setAutoFillBackground(false);

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

    QLinearGradient active = GCColor::linearGradient(22 *dpiYFactor, true);
    QLinearGradient inactive = GCColor::linearGradient(22 *dpiYFactor, false);

    // fill with a linear gradient
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, isActiveWindow() ? active : inactive);
}

void
GcSplitterControl::selectAction()
{
    this->setVisible(!this->isVisible());

    /*this->setBaseSize(width(), parentWidget()->height());
    this->setMaximumSize(QWIDGETSIZE_MAX,QWIDGETSIZE_MAX);*/
}

//
// GcSplitterItem -- the selectable content in the sidebar; has a name, an icon and a widget to show
//

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

