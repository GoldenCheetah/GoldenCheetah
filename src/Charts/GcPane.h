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

#ifndef _GC_GcPane_h
#define _GC_GcPane_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsGridLayout>
#include <QGraphicsProxyWidget>

#include "Colors.h"

class GcPane : public QWidget
{
    Q_OBJECT
    G_OBJECT


    enum drag { None, Close, Flip, Move, Left, Right, Top, Bottom, TLCorner, TRCorner, BLCorner, BRCorner };
    typedef enum drag DragState;

    public:
        GcPane();

        void setLayout(QLayout *layout); // override standard setLayout

    protected:

    signals:
        void exit();

    private slots:

    protected:
        virtual void paintEvent(QPaintEvent *) override;
        virtual void mousePressEvent(QMouseEvent *) override;
        virtual void mouseReleaseEvent(QMouseEvent *) override;
        virtual void mouseMoveEvent(QMouseEvent *) override;
#if QT_VERSION >= 0x060000
        virtual void enterEvent(QEnterEvent *) override;
#else
        virtual void enterEvent(QEvent *) override;
#endif
        virtual void leaveEvent(QEvent *) override;
        virtual bool eventFilter(QObject *object, QEvent *e) override;
        virtual void resizeEvent (QResizeEvent * e) override;
        void flip();

    private:
        void setDragState(DragState);
        void setCursorShape(DragState);
        DragState spotHotSpot(QMouseEvent *);

        // member vars
        QPixmap closeImage, flipImage;
        int borderWidth;

        // drag / resize information
        DragState dragState;
        int oWidth, oHeight, oX, oY, mX, mY;

        // graphics
        QGraphicsView *view;
        QGraphicsScene *scene;
        QGraphicsProxyWidget *widget;
        QGraphicsGridLayout *layout;

        // normal
        QWidget *window;
};

#endif // _GC_GcPane_h
