/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_GProgressDialog_h
#define _GC_GProgressDialog_h 1

#include "GoldenCheetah.h"
#include "Settings.h"
#include "Colors.h"

#include <QWidget>
#include <QPaintEvent>
#include <QString>
#include <QDialog>

class GProgressDialog : public QDialog
{
    Q_OBJECT

    public:

        enum drag { None, Move };
        typedef enum drag DragState;

        // no frame, translucent, no button, no parent - always modal with chrome heading
        GProgressDialog(QString title, int min, int max, bool modal, QWidget *parent = NULL);

        // set value, which in turn repaints the progress at the bottom
        void setValue(int x);

        // set the progress text
        void setLabelText(QString text);

        // compatibility
        bool wasCanceled() { return false; }

        // painting 
        void paintEvent(QPaintEvent *);

        // watch for minimise click and move with mouse
        bool eventFilter(QObject *, QEvent *);
        void setDragState(DragState d);
        void mousePressEvent(QMouseEvent *);
        void mouseReleaseEvent(QMouseEvent *);
        void mouseMoveEvent(QMouseEvent *);

    private:
        QString title, text;
        int min, max, value;

        // drag / resize information
        DragState dragState;
        int oX, oY, mX, mY; // tracking movement

};
#endif
