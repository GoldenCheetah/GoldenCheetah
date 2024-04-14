/*
 * Copyright (c) 2023 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _GC_RatingWidget_h
#define _GC_RatingWidget_h

#include <QLabel>
#include <QMouseEvent>
#include <QEvent>


class RatingWidget : public QLabel
{
    Q_OBJECT

    public:
        RatingWidget(QWidget *parent = nullptr);
        ~RatingWidget();

        void setRating(int rating);

    signals:
        void rated(int rating);

    protected:
        virtual void leaveEvent(QEvent *event);
        virtual void mouseMoveEvent(QMouseEvent *event);
        virtual void mouseReleaseEvent(QMouseEvent *event);

    private:
        int rating;
        int mouseOverRating;

        void showRating(int r);
};

#endif
