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

#include "GcPane.h"

GcPane::GcPane() : QWidget(NULL, Qt::FramelessWindowHint),
    borderWidth(4), dragState(None)
{
    closeImage = QPixmap(":images/toolbar/popbutton.png");
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    setMouseTracking(true);
    setContentsMargins(10,10,10,10);
    setStyleSheet(QString::fromUtf8(
        "border-color: rgba(255,255,255,0); \
         background-color: rgba(255, 255, 255, 0);"
    ));
    // from the graphics stuff to a normal widget
    widget = new QGraphicsProxyWidget(); // proxy from graphics to normal widgets
    widget->setContentsMargins(0,0,0,0);
    //widget->setAutoFillBackground(false);

    window = new QWidget();              // a normal widget we layout into
    //window->setAutoFillBackground(false);
    window->setAttribute(Qt::WA_TranslucentBackground);
    window->setContentsMargins(0,0,0,0);
    widget->setWidget(window);           // link them

    // setup the scene and view
    scene = new QGraphicsScene(this);
    scene->addItem(widget);

    view = new QGraphicsView(scene); // we made our own to resize nicely
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setAutoFillBackground(false);
    view->setFrameStyle(QFrame::NoFrame);
    view->viewport()->installEventFilter(this);
    view->viewport()->setMouseTracking(true);

    // we just display a view containing
    // all the other crap
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QWidget::setLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(view);
}

void
GcPane::setLayout(QLayout *layout)
{
    window->setLayout(layout);
}

bool
GcPane::eventFilter(QObject *object, QEvent *e)
{

    //if (object != (QObject *)plot()->canvas() )
        //return false;
    if (dragState != None) {
            switch (e->type()) {
            case QEvent::MouseMove:
                mouseMoveEvent((QMouseEvent*)e);
                break;
            case QEvent::MouseButtonRelease:
                mouseReleaseEvent((QMouseEvent*)e);
                break;
            default:
                return QObject::eventFilter(object, e);
            }
    } else {
        return QObject::eventFilter(object, e);
    }
    return true;
}

void
GcPane::paintEvent(QPaintEvent *)
{

    // Init paint settings
    QPainter painter(this);
    //painter.setRenderHint(QPainter::Antialiasing);
    QColor color = Qt::black; //GColor(CPOPUP);
    QPen pen(color);

    // border color
    if (dragState != None) pen.setColor(Qt::black);
    pen.setWidth(borderWidth);
    pen.color().setAlpha(40);
    painter.setPen(pen);

    // background color
    painter.setBrush(Qt::NoBrush);

    // draw border and background
    painter.drawRoundedRect(3, 0 + (closeImage.height()/2),
                            width()-(closeImage.width()/2), height()-(closeImage.height()/2)-3,
                            closeImage.width()/2, closeImage.height()/2);

    pen.setColor(Qt::white);
    if (underMouse())
        color.setAlpha(200);
    else
        color.setAlpha(150);
    pen.setWidth(1);
    painter.setPen(pen);
    //painter.setBrush(color);
    // contents fill with a linear gradient
    QLinearGradient linearGradient(0, 0, 0, height());
    linearGradient.setColorAt(0.0, QColor(80,80,80, 220));
    linearGradient.setColorAt(1.0, QColor(0, 0, 0, 220));
    linearGradient.setSpread(QGradient::PadSpread);
    painter.setBrush(linearGradient);
    // draw border and background
    painter.drawRoundedRect(4, 1 + (closeImage.height()/2),
                            width()-2-(closeImage.width()/2), height()-2-(closeImage.height()/2)-3,
                            closeImage.width()/2, closeImage.height()/2);
    // close button
    if (underMouse())
        painter.drawPixmap(width()-closeImage.width()-12, 16, closeImage.width(), closeImage.height(), closeImage);

}

// for the mouse position, are we in a hotspot?
// if so, what would the drag state become if we
// clicked?
GcPane::DragState
GcPane::spotHotSpot(QMouseEvent *e)
{
    // corner
    int corner = closeImage.width()/2;

    // account for offset
    int _y = e->y() - (closeImage.height()/2);
    int _x = e->x();
    int _height = height() - (closeImage.height()/2);
    int _width = width() - (closeImage.width()/2);

    if (e->x() > (width() - (closeImage.width()+10)) && e->y() < (closeImage.height()+12))
        return Close;
    else if (e->x() > (width() - 5 - closeImage.width() - flipImage.width()) &&
             e->x() < (width() - 5 - closeImage.width()) &&
             e->y() < (closeImage.height()-2))
        return Flip;
    else if (_x <= corner && _y <= corner) return (TLCorner);
    else if (_x >= (_width-corner) && _y <= corner) return (TRCorner);
    else if (_x <= corner && _y >= (_height-corner)) return (BLCorner);
    else if (_x >= (_width-corner) && _y >= (_height-corner)) return (BRCorner);
    else if (_x <= borderWidth) return (Left);
    else if (_x >= (_width-borderWidth)) return (Right);
    else if (_y <= borderWidth) return (Top);
    else if (_y >= (_height-borderWidth)) return (Bottom);
    else return (Move);
}

void
GcPane::enterEvent(QEvent *)
{
    repaint();
}

void
GcPane::leaveEvent(QEvent *)
{
    repaint();
}

void
GcPane::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::NoButton || isHidden()) {
        setDragState(None);
        return;
    }

    DragState h = spotHotSpot(e);

    // is it on the close icon?
    if (h == Close) {
        setDragState(None);
        hide();
        //emit exit();
        return;
    } else if (h == Flip) {
        setDragState(None);
        flip();
    }

    // get current window state
    oWidth = width();
    oHeight = height();
    oX = pos().x();
    oY = pos().y();
    mX = e->globalX();
    mY = e->globalY();

    setDragState(h); // set drag state then!

    repaint();
}

void
GcPane::mouseReleaseEvent(QMouseEvent *)
{
    setDragState(None);
    repaint();
}

void
GcPane::mouseMoveEvent(QMouseEvent *e)
{
    if (dragState == None) {
        // set the cursor shape
        setCursorShape(spotHotSpot(e));
        return;
    }

    // work out the relative move x and y
    int relx = e->globalX() - mX;
    int rely = e->globalY() - mY;

    switch (dragState) {

    default:
    case Move :
        move(oX + relx, oY + rely);
        break;

    case TLCorner :
        {
            int newWidth = oWidth - relx;
            int newHeight = oHeight - rely;

            // need to move and resize
            if (newWidth > 30 && newHeight > 30) {
                move(oX + relx, oY + rely);
                resize(newWidth, newHeight);
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
                resize(newWidth, newHeight);
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
                resize(newWidth, newHeight);
            }
        }
        break;

    case BRCorner :
        {
            int newWidth = oWidth + relx;
            int newHeight = oHeight + rely;

            // need to move and resize
            if (newWidth > 30 && newHeight > 30) {
                resize(newWidth, newHeight);
            }
        }
        break;

    case Top :
        {
            int newHeight = oHeight - rely;

            // need to move and resize
            if (newHeight > 30) {
                move (oX, oY + rely);
                resize(oWidth, newHeight);
            }
        }
        break;

    case Bottom :
        {
            int newHeight = oHeight + rely;

            // need to move and resize
            if (newHeight > 30) {
                resize(oWidth, newHeight);
            }
        }
        break;

    case Left :
        {
            int newWidth = oWidth - relx;

            // need to move and resize
            if (newWidth > 30) {
                move (oX + relx, oY);
                resize(newWidth, oHeight);
            }
        }
        break;

    case Right :
        {
            int newWidth = oWidth + relx;

            // need to move and resize
            if (newWidth > 30) {
                resize(newWidth, oHeight);
            }
        }
        break;
    }

    //repaint();
    //QApplication::processEvents(); // flicker...
}

void
GcPane::setDragState(DragState d)
{
    dragState = d;
    setCursorShape(d);
}

void
GcPane::setCursorShape(DragState d)
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
        setCursor(Qt::ArrowCursor);
        break;
    default:
    case Close:
    case None:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

void
GcPane::resizeEvent (QResizeEvent *)
{
    setUpdatesEnabled(false);

    // resize the scene, view and widget to fit
    widget->resize(width()-50, height()-50);
    view->scene()->setSceneRect(0, 0, width()-50, height()-50);
    view->resize(width()-50, height()-50);

    setUpdatesEnabled(true);
}

void
GcPane::flip() { }
