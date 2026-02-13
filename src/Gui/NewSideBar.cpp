/*
 * Copyright (c) 2020 Mark Liversedge <liversedge@gmail.com>
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

#include "Context.h"
#include "NewSideBar.h"
#include "Colors.h"

static constexpr int gl_itemwidth = 70;
static constexpr int gl_itemheight = 50;
static constexpr int gl_margin = 0;

NewSideBar::NewSideBar(QWidget *parent) : QWidget(parent)

{
    setFixedWidth(gl_itemwidth *dpiXFactor);
    setAutoFillBackground(true);
    lastid=GcSideBarBtnId::NO_BUTTON_SET;

    QVBoxLayout *mainlayout = new QVBoxLayout(this);
    mainlayout->setSpacing(0);
    mainlayout->setContentsMargins(0,0,0,0);
    top=new QWidget(this);
    bottom=new QWidget(this);
    middle=new QWidget(this);
    middle->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    mainlayout->addWidget(top);
    mainlayout->addWidget(middle);
    mainlayout->addWidget(bottom);

    layout = new QVBoxLayout(middle);
    layout->setSpacing(0);
    layout->setContentsMargins(0,gl_margin *dpiXFactor,0,gl_margin*dpiYFactor);

    connect(GlobalContext::context(), &GlobalContext::configChanged, this, &NewSideBar::configChanged);
    configChanged(0);

    show();
}

void
NewSideBar::clicked(GcSideBarBtnId id)
{
    // un selectable item was clicked (e.g. symc or settings)
    emit itemClicked(id);
}

void
NewSideBar::selected(GcSideBarBtnId id)
{
    // we selected one, so need to set it selected
    // and deselect everyone else
    QMapIterator<GcSideBarBtnId,NewSideBarItem*>i(items);
    while(i.hasNext()) {
        i.next();
        if (i.key() == id) i.value()->setSelected(true);
        else i.value()->setSelected(false);
        i.value()->update();
    }
    emit itemSelected(id);
}

GcSideBarBtnId
NewSideBar::addItem(QImage icon, const QString& name, GcSideBarBtnId id, const QString& whatsThisText)
{
    if (id != GcSideBarBtnId::NO_BUTTON_SET) {
        NewSideBarItem *add = new NewSideBarItem(this, id, icon, name);
        if (!whatsThisText.isEmpty()) add->setWhatsThis(whatsThisText);
        layout->addWidget(add);
        items.insert(id, add);
    }

    return id;
}

// leave a gap- we have main icons, gap, apps, gap, sync, prefs
void
NewSideBar::addStretch()
{
    layout->addStretch();
}


// is it shown, is it usable?
void
NewSideBar::setItemVisible(GcSideBarBtnId id, bool x)
{
    NewSideBarItem *item = items.value(id, NULL);
    if (item) item->setVisible(x);
}

void
NewSideBar::setItemEnabled(GcSideBarBtnId id, bool x)
{
    NewSideBarItem *item = items.value(id, NULL);
    if (item) item->setEnabled(x);
}

// can we select it, select it by program?
void
NewSideBar::setItemSelectable(GcSideBarBtnId id, bool x)
{
    NewSideBarItem *item = items.value(id, NULL);
    if (item) item->setSelectable(x);
}

void
NewSideBar::setItemSelected(GcSideBarBtnId id, bool x)
{
    NewSideBarItem *item = items.value(id, NULL);
    if (item) item->setSelected(x);

    if (x) {
        // unset all the others
        foreach(NewSideBarItem*i, items) {
            if (item != i) {
                i->setSelected(false);
            }
        }
    }
}

void
NewSideBar::configChanged(qint32)
{
    QFont font;
    QFontMetrics fm(font);
    top->setFixedHeight(fm.height() + 16); // no scaling...
    bottom->setFixedHeight(22 * dpiXFactor);
    QColor col=GColor(CCHROME);
    QString style=QString("QWidget { background: rgb(%1,%2,%3); }").arg(col.red()).arg(col.green()).arg(col.blue());
    top->setStyleSheet(style);
    bottom->setStyleSheet(style);
    middle->setStyleSheet(style);
}

NewSideBarItem::NewSideBarItem(NewSideBar *sidebar, GcSideBarBtnId id, QImage icon, QString name) : QWidget(sidebar), sidebar(sidebar), id(id), icon(icon), name(name)
{
    clicked = selected = false;
    selectable = enabled = true;

    // some basics
    setFixedSize(gl_itemwidth *dpiXFactor,gl_itemheight *dpiYFactor);
    setAutoFillBackground(true);

    // trap events
    installEventFilter(this);
    connect(GlobalContext::context(), &GlobalContext::configChanged, this, &NewSideBarItem::configChanged);

    configChanged(0);

    show();
}

void
NewSideBarItem::setSelectable(bool x)
{
    selectable = x;
    update();
}

void
NewSideBarItem::setEnabled(bool x)
{
    enabled = x;
    update();
}

void
NewSideBarItem::setSelected(bool x)
{
    selected = x;
    update();
}

// trap all events for me
bool
NewSideBarItem::eventFilter(QObject *, QEvent *e)
{
    switch(e->type()) {
    case QEvent::Enter:
    case QEvent::Leave:
    case QEvent::MouseMove:
        update();
        break;

    case QEvent::MouseButtonPress:
        clicked=true;
        break;
    case QEvent::MouseButtonRelease:
        if (underMouse() && clicked) {
            // user clicked on us!
            if (enabled && selectable && !selected) sidebar->selected(id);
            else if (enabled && !selectable) sidebar->clicked(id);
        }
        clicked=false;
        break;
    default:
        break;
    }

    return false;
}

static QImage imageRGB(QImage &source, QColor target)
{
    QImage returning = source;

    if (target == QColor(Qt::white)) target = QColor(255,255,254, 255); // white is the alpha

    for(int x=0; x< returning.width(); x++)
        for(int y=0; y< returning.height(); y++) {
            QColor v = returning.pixelColor(x,y);
            if (v.red() == 0 && v.blue() == 0 && v.green() == 0)
                returning.setPixelColor(x,y,target);
        }

    return returning;
}

void
NewSideBarItem::configChanged(qint32)
{
    // set background colors
    bg_normal = GColor(CCHROME);
    QString style = QString("QWidget { background: rgb(%1,%2,%3); }").arg(bg_normal.red()).arg(bg_normal.green()).arg(bg_normal.blue());
    setStyleSheet(style);

    // set foreground colors
    fg_normal = GCColor::invertColor(GColor(CCHROME));

    // if foreground is white then we're "dark" if its
    // black the we're "light" so this controls palette
    bool dark = (fg_normal == QColor(Qt::white));

    if (dark) fg_disabled = QColor(80,80,80);
    else fg_disabled = QColor(180,180,180);

    // on select
    bg_select = GCColor::selectedColor(bg_normal);
    fg_select = GCColor::invertColor(bg_select);

    // on hover
    bg_hover =GColor(CHOVER);

    iconNormal = QPixmap::fromImage(imageRGB(icon, fg_normal), Qt::ColorOnly|Qt::PreferDither|Qt::DiffuseAlphaDither);
    iconNormal = iconNormal.scaled(24*dpiXFactor, 24*dpiXFactor, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    iconSelect = QPixmap::fromImage(imageRGB(icon, fg_select), Qt::ColorOnly|Qt::PreferDither|Qt::DiffuseAlphaDither);
    iconSelect = iconSelect.scaled(24*dpiXFactor, 24*dpiXFactor, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    iconDisabled = QPixmap::fromImage(imageRGB(icon, fg_disabled), Qt::ColorOnly|Qt::PreferDither|Qt::DiffuseAlphaDither);
    iconDisabled = iconDisabled.scaled(24*dpiXFactor, 24*dpiXFactor, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

void
NewSideBarItem::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    // background is normal, unless selected or hovering
    QBrush brush(bg_normal);
    if (selected) brush = QBrush(bg_select);
    else if (underMouse() && enabled) brush = QBrush(bg_hover);
    painter.fillRect(QRectF(0,0,gl_itemwidth*dpiXFactor, gl_itemheight*dpiXFactor), brush);

    // icon is normal or disabled
    if (selected) painter.drawPixmap(27*dpiXFactor,6*dpiXFactor,24*dpiXFactor,24*dpiXFactor,iconSelect);
    else if (enabled) painter.drawPixmap(27*dpiXFactor,6*dpiXFactor,24*dpiXFactor,24*dpiXFactor,iconNormal);
    else painter.drawPixmap(27*dpiXFactor,6*dpiXFactor,24*dpiXFactor,24*dpiXFactor,iconDisabled);

    // block
    if (selected) painter.fillRect(QRectF(0,0,3*dpiXFactor,gl_itemheight*dpiXFactor), QBrush(GColor(CPLOTMARKER)));

    // draw name
    QPen pen(fg_normal);
    if (!enabled) pen = QPen(fg_disabled);
    if (selected) pen = fg_select;
    QFont f;
    f.setPixelSize(12*dpiYFactor);
    painter.setFont(f);
    painter.setPen(pen);
    painter.drawText(QRect(5*dpiXFactor,24*dpiYFactor,(gl_itemwidth-5)*dpiXFactor,20*dpiYFactor), Qt::AlignCenter| Qt::AlignBottom, name);
}
