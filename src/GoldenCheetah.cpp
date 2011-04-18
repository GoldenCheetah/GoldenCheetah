/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "GoldenCheetah.h"
#include "Colors.h"
#include <QDebug>
#include <QPainter>
#include <QPixmap>
#include <QEvent>
#include <QMouseEvent>

QWidget *GcWindow::controls() const
{
    return _controls;
}

void GcWindow::setControls(QWidget *x)
{
    _controls = x;
    emit controlsChanged(_controls);
}

QString GcWindow::instanceName() const
{
    return _instanceName;
}

void GcWindow::_setInstanceName(QString x)
{
    _instanceName = x;
}

QString GcWindow::title() const
{
    return _title;
}

void GcWindow::setTitle(QString x)
{
    _title = x;
    emit titleChanged(_title);
}

RideItem* GcWindow::rideItem() const
{
    return _rideItem;
}

void GcWindow::setRideItem(RideItem* x)
{
    _rideItem = x;
    emit rideItemChanged(_rideItem);
}

double GcWindow::widthFactor() const
{
    return _widthFactor;
}

void GcWindow::setWidthFactor(double x)
{
    _widthFactor = x;
    emit widthFactorChanged(x);
}

double  GcWindow::heightFactor() const
{
    return _heightFactor;
}

void GcWindow::setHeightFactor(double x)
{
    _heightFactor = x;
    emit heightFactorChanged(x);
}

void GcWindow::setResizable(bool x)
{
    _resizable = x;
}

bool GcWindow::resizable() const
{
    return _resizable;
}

void GcWindow::setGripped(bool x)
{
    _gripped = x;
}

bool GcWindow::gripped() const
{
    return _gripped;
}

GcWindow::GcWindow()
{
}

GcWindow::GcWindow(QWidget *parent) : QFrame(parent), dragState(None) {
    qRegisterMetaType<QWidget*>("controls");
    qRegisterMetaType<RideItem*>("ride");
    qRegisterMetaType<GcWinID>("type");
    setParent(parent);
    setControls(NULL);
    setRideItem(NULL);
    setTitle("");
    setContentsMargins(0,0,0,0);
    setResizable(false);
    setMouseTracking(true);
}

GcWindow::~GcWindow()
{
    //qDebug()<<"deleting.."<<property("title").toString()<<property("instanceName").toString()<<_type;
}

bool
GcWindow::amVisible()
{
    // if we're hidden then say false!
    if (isHidden()) return false;
    return true; // XXX need to sort this!!
}



void
GcWindow::paintEvent(QPaintEvent *event)
{
    static QPixmap closeImage = QPixmap(":images/toolbar/popbutton.png");
    static QPixmap aluBar = QPixmap(":images/aluBar.png");
    static QPixmap aluBarDark = QPixmap(":images/aluBarDark.png");
    static QPixmap aluLight = QPixmap(":images/aluLight.jpg");
    static QPixmap carbon = QPixmap(":images/carbon.jpg");
    static QPalette defaultPalette;

    if (contentsMargins().top() > 0) {
        // draw a rectangle in the contents margins
        // at the top of the widget

        // setup a painter and the area to paint
        QPainter painter(this);

        // background light gray for now?
        QRect all(0,0,width(),height());
        //painter.drawTiledPixmap(all, aluLight);
        painter.fillRect(all, defaultPalette.color(QPalette::Window));

        // fill in the title bar
        QRect bar(0,0,width(),contentsMargins().top());
        QColor bg;
        if (property("active").toBool() == true) {
            bg = GColor(CTILEBARSELECT);
            painter.drawPixmap(bar, aluBarDark);
        } else {
            bg = GColor(CTILEBAR);
            painter.drawPixmap(bar, aluBar);
        }


        // heading
        QFont font;
        font.setPointSize((contentsMargins().top()/2)+2);
        font.setWeight(QFont::Bold);
        QString title = property("title").toString();
        painter.setFont(font);
        painter.drawText(bar, title, Qt::AlignVCenter | Qt::AlignCenter);

        // border
        painter.setBrush(Qt::NoBrush);
        if (underMouse()) {
            QPixmap sized = closeImage.scaled(QSize(contentsMargins().top(),
                                                    contentsMargins().top()));
            painter.setPen(Qt::black);
            painter.drawRect(QRect(0,0,width()-1,height()-1));
            painter.drawPixmap(width()-sized.width(), 0, sized.width(), sized.height(), sized);
        } else {
            painter.setPen(bg);
            painter.drawRect(QRect(0,0,width()-1,height()-1));
        }
    } else {
        // is this a layout manager?
        // background light gray for now?
        QPainter painter(this);
        QRect all(0,0,width(),height());
        if (property("isManager").toBool() == true) {
            painter.drawTiledPixmap(all, carbon);
        } else {
            //painter.drawTiledPixmap(all, aluLight);
        }
    }
}

/*----------------------------------------------------------------------
 * Drag and resize tiles
 *--------------------------------------------------------------------*/

bool
GcWindow::eventFilter(QObject *, QEvent *e)
{
    if (!resizable()) return false;

    // handle moving / resizing activity
    if (dragState != None) {
        switch (e->type()) {
        case QEvent::MouseMove:
            mouseMoveEvent((QMouseEvent*)e);
            return false;
            break;
        case QEvent::MouseButtonRelease:
            mouseReleaseEvent((QMouseEvent*)e);
            return false;
            break;
        default:
            break;
        }
    }
    return false;
}

void
GcWindow::mousePressEvent(QMouseEvent *e)
{
    if (!resizable() || e->button() == Qt::NoButton || isHidden()) {
        setDragState(None);
        return;
    }

    DragState h = spotHotSpot(e);

    // is it on the close icon?
    if (h == Close) {
        setDragState(None);
        //hide();
        //emit exit();
        return;
    } else if (h == Flip) {
        setDragState(None);
        //flip();
    }

    // get current window state
    oWidth = width();
    oHeight = height();
    oWidthFactor = widthFactor();
    oHeightFactor = heightFactor();
    oX = pos().x();
    oY = pos().y();
    mX = mapFromGlobal(QCursor::pos()).x();
    mY = mapFromGlobal(QCursor::pos()).y();

    setDragState(h); // set drag state then!

    repaint();
}

void
GcWindow::mouseReleaseEvent(QMouseEvent *)
{
    // tell the owner!
    if (dragState == Move) {
        setProperty("gripped", false);
        emit moved(this);
    } else if (dragState == Left ||
               dragState == Right ||
               dragState == Top ||
               dragState == Bottom ||
               dragState == TLCorner ||
               dragState == TRCorner ||
               dragState == BLCorner ||
               dragState == BRCorner) {
        emit resized(this);
    }
    setDragState(None);
    repaint();
}

// for the mouse position, are we in a hotspot?
// if so, what would the drag state become if we
// clicked?
GcWindow::DragState
GcWindow::spotHotSpot(QMouseEvent *e)
{
    // corner
    int corner = 9;
    int borderWidth = 3;

    // account for offset XXX map to GcWindow geom
    int _y = e->y();
    int _x = e->x();
    int _height = height();
    int _width = width();

    if (e->x() > (2 + width() - corner) && e->y() < corner) return (Close);
    else if (_x <= corner && _y <= corner) return (TLCorner);
    else if (_x >= (_width-corner) && _y <= corner) return (TRCorner);
    else if (_x <= corner && _y >= (_height-corner)) return (BLCorner);
    else if (_x >= (_width-corner) && _y >= (_height-corner)) return (BRCorner);
    else if (_x <= borderWidth) return (Left);
    else if (_x >= (_width-borderWidth)) return (Right);
    else if (_y <= borderWidth) return (Top);
    else if (_y >= (_height-borderWidth)) return (Bottom);
    else if (_y <= contentsMargins().top()) return (Move);
    else return (None);
}

void
GcWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (!resizable()) return;

    if (dragState == None) {
        // set the cursor shape
        setCursorShape(spotHotSpot(e));
        return;
    }

    // work out the relative move x and y
    int relx = mapFromGlobal(QCursor::pos()).x() - mX;
    int rely = mapFromGlobal(QCursor::pos()).y() - mY;

    switch (dragState) {

    default:
    case Move :
        //move(oX + relx, oY + rely);
#if QT_VERSION < 0x040700
        setCursor(Qt::ClosedHandCursor);
#else
        setCursor(Qt::DragMoveCursor);
#endif
        emit moving(this);
        break;

    case TLCorner :
        {
            int newWidth = oWidth - relx;
            int newHeight = oHeight - rely;

            // need to move and resize
            if (newWidth > 30 && newHeight > 30) {
                move(oX + relx, oY + rely);
                setNewSize(newWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case TRCorner :
        {
            int newWidth = oWidth + relx;
            int newHeight = oHeight - rely;

            // need to move and resize if changes on y plane
            if (newWidth > 30 && newHeight > 30) {
                move(oX, oY + rely);
                setNewSize(newWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case BLCorner :
        {
            int newWidth = oWidth - relx;
            int newHeight = oHeight + rely;

            // need to move and resize
            if (newWidth > 30 && newHeight > 30) {
                move(oX + relx, oY);
                setNewSize(newWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case BRCorner :
        {
            int newWidth = oWidth + relx;
            int newHeight = oHeight + rely;

            // need to move and resize
            if (newWidth > 30 && newHeight > 30) {
                setNewSize(newWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case Top :
        {
            int newHeight = oHeight - rely;

            // need to move and resize
            if (newHeight > 30) {
                move (oX, oY + rely);
                setNewSize(oWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case Bottom :
        {
            int newHeight = oHeight + rely;

            // need to move and resize
            if (newHeight > 30) {
                setNewSize(oWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case Left :
        {
            int newWidth = oWidth - relx;

            // need to move and resize
            if (newWidth > 30) {
                move (oX + relx, oY);
                setNewSize(newWidth, oHeight);
                emit resizing(this);
            }
        }
        break;

    case Right :
        {
            int newWidth = oWidth + relx;

            // need to move and resize
            if (newWidth > 30) {
                setNewSize(newWidth, oHeight);
                emit resizing(this);
            }
        }
        break;
    }

    oX = pos().x();
    oY = pos().y();
    //repaint();
    //QApplication::processEvents(); // flicker...
}

void
GcWindow::setNewSize(int w, int h)
{
    // convert to height factor
    double newHF = (double(oHeight) * oHeightFactor) / double(h);
    double newWF = (double(oWidth) * oWidthFactor) / double(w);

    // don't get too wide
    if (newWF < 1) return; // too big

    // now apply
    setFixedSize(QSize(w,h));

    // adjust factors
    setHeightFactor(newHF);
    setWidthFactor(newWF);
}

void
GcWindow::setDragState(DragState d)
{
    dragState = d;
    setProperty("gripped", dragState == Move ? true : false);
    setCursorShape(d);
}

void
GcWindow::setCursorShape(DragState d)
{
    // set cursor
    switch (d) {
    case Bottom:
    case Top:
        setCursor(Qt::SizeVerCursor);
        break;
    case Left:
    case Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case TLCorner:
    case BRCorner:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case TRCorner:
    case BLCorner:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case Move:
        //setCursor(Qt::OpenHandCursor); //XXX sub widgets don't set the cursor...
        setCursor(Qt::ArrowCursor);
        break;
    default:
    case Close:
    case None:
        setCursor(Qt::ArrowCursor);
        break;
    }
}
