/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
 *                (c) Aleksey Osipov (aliks-os@yandex.ru)
 *
 * Some of the code pinched from http://qt-project.org/wiki/Widget-moveable-and-resizeable
 * coz it was simpler than starting from scratch.
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

#include "GcOverlayWidget.h"
#include "GcScopeBar.h"
#include "DiaryWindow.h"
#include "DiarySidebar.h"
#include "Context.h"
#include "QtMacButton.h"

GcOverlayWidget::GcOverlayWidget(Context *context, QWidget *parent) : QWidget(parent), context(context)
{
    // left / right scroller icon
    static QIcon leftIcon = iconFromPNG(":images/mac/left.png");
    static QIcon rightIcon = iconFromPNG(":images/mac/right.png");

    setContentsMargins(4,0,4,4);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
#ifdef GC_HAVE_MUMODEL
    setMinimumSize(400,200); //XX temp for MU model...
#else
    setMinimumSize(250,200);
#endif
    setFocus();
    mode = none;
    initial = true;

    // main layout
    QVBoxLayout *mlayout = new QVBoxLayout(this);
    mlayout->setSpacing(0);
    mlayout->setContentsMargins(1,0,1,1);

    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->setSpacing(0);
    titleLayout->setContentsMargins(0,0,0,0);
    mlayout->addLayout(titleLayout);

    // scroller buttons
    left = new QToolButton(this);
    left->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    left->setAutoFillBackground(false);
    left->setFixedSize(23,23);
    left->setIcon(leftIcon);
    left->setIconSize(QSize(23,23));
    left->setFocusPolicy(Qt::NoFocus);
    left->hide(); // don't show until we have >1 widgets
    titleLayout->addWidget(left);
    connect(left, SIGNAL(clicked()), this, SLOT(scrollLeft()));

    titleLabel = new GcLabel("No Title Set");
    titleLabel->setAutoFillBackground(false);
    titleLabel->setFixedHeight(23);

    // menu bar in the middle of the buttons
    titleLayout->addStretch();
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    right = new QToolButton(this);
    right->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    right->setAutoFillBackground(false);
    right->setFixedSize(20,20);
    right->setIcon(rightIcon);
    right->setIconSize(QSize(20,20));
    right->setFocusPolicy(Qt::NoFocus);
    right->hide();
    titleLayout->addWidget(right);
    connect(right, SIGNAL(clicked()), this, SLOT(scrollRight()));

    stack = new QStackedWidget(this);
    stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mlayout->addWidget(stack);

    // trap resize / mouse events
    m_infocus = true;
    m_showMenu = false;
    m_isEditing = true;
    installEventFilter(parent);

    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));

    // setup colors
    configChanged();
}

void
GcOverlayWidget::configChanged()
{
    if (GCColor::isFlat()) {
        titleLabel->setStyleSheet(QString("color: %1;").arg(GCColor::invertColor(GColor(CCHROME)).name()));
    } else {
        titleLabel->setStyleSheet("color: black;");
    }
}

void
GcOverlayWidget::addWidget(QString title, QWidget *widget)
{
    // we want this one
    //widget->setParent(this);
    //widget->releaseMouse();
    //widget->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // add to list
    items << GcOverlayWidgetItem(title, widget);
    stack->addWidget(widget);

    // but don't select it 
    // unless its the first one
    if (items.count() == 1) setCurrentIndex(0);

    // set cursors visible/hidden if needed
    setCursors();
}

void
GcOverlayWidget::setCurrentIndex(int index)
{
    if (initial || index != stack->currentIndex()) {
        titleLabel->setText(items.at(index).name);
        stack->setCurrentIndex(index);

        // tell everyone we changed
        emit currentIndexChanged(index);

        initial = false;
    }
}

int
GcOverlayWidget::currentIndex() const
{
    return stack->currentIndex();
}

void
GcOverlayWidget::setCursors()
{
    if (items.count() > 1) {

        left->show();
        right->show();

    } else {

        left->hide();
        right->hide();
    }
}

void
GcOverlayWidget::scrollRight()
{
    int rightIndex = stack->currentIndex() + 1;
    if (stack->count() > rightIndex) setCurrentIndex(rightIndex);
    else setCurrentIndex(0);
}

void
GcOverlayWidget::scrollLeft()
{
    if (stack->currentIndex() > 0) setCurrentIndex(stack->currentIndex() -1);
    else setCurrentIndex(stack->count() -1);
}

GcOverlayWidget::~GcOverlayWidget() {}

void
GcOverlayWidget::paintEvent (QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
    QWidget::paintEvent(event);
}

// paint is the same as sidebar
void
GcOverlayWidget::paintBackground(QPaintEvent *)
{
    // setup a painter and the area to paint
    QPainter painter(this);

    painter.save();
    QRect all(0,0,width(),height());
    QRect boundary(0,0,width()-1,height()-1);

    painter.fillRect(all, GColor(CPLOTBACKGROUND));
    painter.setPen(QPen(Qt::darkGray));
    painter.drawRect(boundary);

    // linear gradients
    QLinearGradient active = GCColor::linearGradient(23, true); 
    QLinearGradient inactive = GCColor::linearGradient(23, false); 

    // title
    QRect title(0,0,width(),23);
    painter.fillRect(title, QColor(Qt::white));
    painter.fillRect(title, isActiveWindow() ? active : inactive);

    if (!GCColor::isFlat()) {
        QPen black(QColor(100,100,100,200));
        painter.setPen(black);
        painter.drawLine(0,22, width()-1, 22);

        QPen gray(QColor(230,230,230));
        painter.setPen(gray);
        painter.drawLine(0,0, width()-1, 0);
    }

    painter.restore();
}

void GcOverlayWidget::focusInEvent(QFocusEvent *) 
{
    m_infocus = true;
    parentWidget()->installEventFilter(this);
    parentWidget()->repaint();
    emit inFocus(true);
}
 
void GcOverlayWidget::focusOutEvent(QFocusEvent *) 
{
    if (!m_isEditing) return;
    if (m_showMenu) return;
    mode = none;
    emit outFocus(false);
    m_infocus = false;
}
 
bool GcOverlayWidget::eventFilter( QObject *obj, QEvent *evt ) 
{
    //return QWidget::eventFilter(obj, evt);
    if (m_infocus) {
        QWidget *w = parentWidget();

        if (w == obj && evt->type()==QEvent::Paint) {
#if 0
            QPainter painter(w);
            QPoint p = this->mapTo(w,QPoint(-3,-3));
            QPoint LT = w->mapFrom(w,p);
            QPoint LB = w->mapFrom(w,QPoint(p.x(),p.y()+this->height()));
            QPoint RB = w->mapFrom(w,QPoint(p.x()+this->width(),p.y()+this->height()));
            QPoint RT = w->mapFrom(w,QPoint(p.x()+this->width(),p.y()));
 
            painter.fillRect(LT.x(),LT.y(),6,6,QColor("black"));
            painter.fillRect(LB.x(),LB.y(),6,6,QColor("black"));
            painter.fillRect(RB.x(),RB.y(),6,6,QColor("black"));
            painter.fillRect(RT.x(),RT.y(),6,6,QColor("black"));
#endif
            return QWidget::eventFilter(obj,evt);
        }
    }
    return QWidget::eventFilter(obj, evt);
}
 
void GcOverlayWidget::mousePressEvent(QMouseEvent *e) 
{
    position = QPoint(e->globalX()-geometry().x(), e->globalY()-geometry().y());
    if (!m_isEditing) return;
    if (!m_infocus) return;
    //QWidget::mouseMoveEvent(e);
    if (!e->buttons() & Qt::LeftButton) {
        setCursorShape(e->pos());
        return;
    }

    if(e->button()==Qt::RightButton) {
        //e->accept();
    }
}
 
void GcOverlayWidget::keyPressEvent(QKeyEvent *e) 
{
    if (!m_isEditing) return;
    if (e->key() == Qt::Key_Delete) {
        //this->deleteLater();
    }

    if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
        QPoint newPos(x(),y());
        if (e->key() == Qt::Key_Up) newPos.setY(newPos.y()-1);
        if (e->key() == Qt::Key_Down) newPos.setY(newPos.y()+1);
        if (e->key() == Qt::Key_Left) newPos.setX(newPos.x()-1);
        if (e->key() == Qt::Key_Right) newPos.setX(newPos.x()+1);
        move(newPos);
    }
    if (QApplication::keyboardModifiers() == Qt::ShiftModifier) {
        if (e->key() == Qt::Key_Up) resize(width(),height()-1);
        if (e->key() == Qt::Key_Down) resize(width(),height()+1);
        if (e->key() == Qt::Key_Left) resize(width()-1,height());
        if (e->key() == Qt::Key_Right) resize(width()+1,height());
    }
    emit newGeometry(geometry());
}
 
void GcOverlayWidget::setCursorShape(const QPoint &e_pos) 
{
    const int diff = 6;
    if (
            //Left-Bottom
            ((e_pos.y() > y() + height() - diff) &&          //Bottom
             (e_pos.x() < x()+diff)) ||                      //Left
            //Right-Bottom
            ((e_pos.y() > y() + height() - diff) &&          //Bottom
             (e_pos.x() > x() + width() - diff)) ||          //Right
            //Left-Top
            ((e_pos.y() < y() + diff) &&                     //Top
             (e_pos.x() < x() + diff)) ||                    //Left
            //Right-Top
            ((e_pos.y() < y() + diff) &&                     //Top
             (e_pos.x() > x() + width() - diff))             //Right
       )
    {
        //Left-Bottom
        if ((e_pos.y() > y() + height() - diff) &&           //Bottom
            (e_pos.x() < x() + diff)) {                      //Left
            mode = resizebl;
            setCursor(QCursor(Qt::SizeBDiagCursor));
        }
        //Right-Bottom
        if ((e_pos.y() > y() + height() - diff) &&           //Bottom
            (e_pos.x() > x() + width() - diff)) {            //Right
            mode = resizebr;
            setCursor(QCursor(Qt::SizeFDiagCursor));
        }
        //Left-Top
        if ((e_pos.y() < y() + diff) &&                      //Top
            (e_pos.x() < x() + diff)) {                      //Left
            mode = resizetl;
            setCursor(QCursor(Qt::SizeFDiagCursor));
        }
        //Right-Top
        if ((e_pos.y() < y() + diff) &&                      //Top
            (e_pos.x() > x() + width() - diff)) {            //Right
            mode = resizetr;
            setCursor(QCursor(Qt::SizeBDiagCursor));
        }
    }
    // проверка положения курсора по горизонтали
    else if ((e_pos.x() < x() + diff) ||             //Left
            ((e_pos.x() > x() + width() - diff))) {  //Right
        if (e_pos.x() < x() + diff) {                //Left
            setCursor(QCursor(Qt::SizeHorCursor));
            mode = resizel;
        } else {                                     //Right
            setCursor(QCursor(Qt::SizeHorCursor));
            mode = resizer;
        }
    }
    // проверка положения курсора по вертикали
    else if (((e_pos.y() > y() + height() - diff)) || //Bottom
              (e_pos.y() < y() + diff)) {             //Top
        if (e_pos.y() < y() + diff) {                 //Top
            setCursor(QCursor(Qt::SizeVerCursor));
            mode = resizet;
        } else {                                      //Bottom
            setCursor(QCursor(Qt::SizeVerCursor));
            mode = resizeb;
        }
    } else if (e_pos.y() <= y() + 23 ) {
        setCursor(QCursor(Qt::ArrowCursor));
        mode = moving;
    }
}
 
void GcOverlayWidget::mouseReleaseEvent(QMouseEvent *e) 
{
    QWidget::mouseReleaseEvent(e);
}
 
void GcOverlayWidget::mouseMoveEvent(QMouseEvent *e) 
{
    QWidget::mouseMoveEvent(e);
    if (!m_isEditing) return;
    if (!m_infocus) return;
    if (!e->buttons() & Qt::LeftButton) {
        QPoint p = QPoint(e->x()+geometry().x(), e->y()+geometry().y());
        setCursorShape(p);
        return;
    }
 
    if (mode == moving && e->buttons() && Qt::LeftButton) {
        QPoint toMove = e->globalPos() - position;
        if (toMove.x() < 0) return;
        if (toMove.y() < 0) return;
        if (toMove.x() > parentWidget()->width()-width()) return;
        move(toMove);
        emit newGeometry(geometry());
        parentWidget()->repaint();
        return;
    }
    if ((mode != moving) && e->buttons() && Qt::LeftButton) {
        switch (mode){
            case resizetl: {  //Left-Top
                int newwidth = e->globalX() - position.x() - geometry().x();
                int newheight = e->globalY() - position.y() - geometry().y();
                QPoint toMove = e->globalPos() - position;
                resize(geometry().width()-newwidth,geometry().height()-newheight);
                move(toMove.x(),toMove.y());
                break;
            }
            case resizetr: {  //Right-Top
                int newheight = e->globalY() - position.y() - geometry().y();
                QPoint toMove = e->globalPos() - position;
                resize(e->x(),geometry().height()-newheight);
                move(x(),toMove.y());
                break;
            }
            case resizebl: {  //Left-Bottom
                int newwidth = e->globalX() - position.x() - geometry().x();
                QPoint toMove = e->globalPos() - position;
                resize(geometry().width()-newwidth,e->y());
                move(toMove.x(),y());
                break;
            }
            case resizeb: {   //Bottom
                resize(width(),e->y()); break;}
            case resizel:  {  //Left
                int newwidth = e->globalX() - position.x() - geometry().x();
                QPoint toMove = e->globalPos() - position;
                resize(geometry().width()-newwidth,height());
                move(toMove.x(),y());
                break;
            }
            case resizet:  {  //Top
                int newheight = e->globalY() - position.y() - geometry().y();
                QPoint toMove = e->globalPos() - position;
                resize(width(),geometry().height()-newheight);
                move(x(),toMove.y());
                break;
            }
            case resizer:  {  //Right
                resize(e->x(),height()); break;}
            case resizebr: {  //Right-Bottom
                resize(e->x(),e->y()); break;}
        }
        parentWidget()->repaint();
    }
    emit newGeometry(geometry());
    //QWidget::mouseMoveEvent(e);
}
