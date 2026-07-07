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

#ifndef _GC_PowerInfoWidget_h
#define _GC_PowerInfoWidget_h

#include <QWidget>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QMargins>


class PowerInfoWidget : public QWidget
{
    Q_OBJECT

    public:
        PowerInfoWidget(QWidget *parent = nullptr);
        ~PowerInfoWidget();

        void setPower(int minPower, int maxPower, int avgPower, int isoPower, int xPower);

        void setContentsMargins(int left, int top, int right, int bottom);
        void setContentsMargins(const QMargins &margins);

    protected:
        virtual void paintEvent(QPaintEvent *event);
        virtual void mouseReleaseEvent(QMouseEvent *event);
        virtual void changeEvent(QEvent *event);

    private:
        int minPower;
        int maxPower;
        int avgPower;
        int isoPower;
        int xPower;

        int powerRange;
        double avgPercent;
        double isoPercent;
        double xPercent;

        bool skiba;

        int rowTop(int rowHeight, int row) const;
        void adjustHeight();
};

#endif
