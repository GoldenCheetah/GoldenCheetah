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

#ifndef _GC_Shy_h
#define _GC_Shy_h

#include <QWidget>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QEvent>
#include <QPropertyAnimation>


enum class ShyType {
    pixel,
    lines,
    percent
};


class Shy : public QWidget
{
    Q_OBJECT

    public:
        Shy(double amount = 40, ShyType type = ShyType::pixel, QWidget *parent = nullptr);
        virtual ~Shy();

        void setLayout(QLayout *layout);
        void setAmount(double amount);
        void setAmount(double amount, ShyType type);
        void setAnimationDuration(int duration);

    protected:
        virtual void mouseReleaseEvent(QMouseEvent *event);
        virtual void resizeEvent(QResizeEvent *event);
        virtual bool event(QEvent *event);

    private:
        bool collapsed;
        double amount;
        ShyType type;
        int duration;
        QPropertyAnimation *animation;
        QWidget *container;

        int collapsedHeight() const;
        int containerHeight() const;
        void updateHeight();
};

#endif
