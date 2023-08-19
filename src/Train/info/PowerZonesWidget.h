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

#ifndef _GC_PowerZonesWidget_h
#define _GC_PowerZonesWidget_h

#include <utility>

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QParallelAnimationGroup>
#include <QMargins>
#include <QTextDocument>
#include <QList>
#include <QColor>
#include <QString>


class PowerZonesWidget : public QWidget
{
    Q_OBJECT

    public:
        PowerZonesWidget(QList<QColor> colors, QList<QString> names, QWidget *parent = nullptr);
        ~PowerZonesWidget();

        void setPowerZones(QList<double> zones, int dominantZone, long duration);

        void setContentsMargins(int left, int top, int right, int bottom);
        void setContentsMargins(const QMargins &margins);

    public slots:
        void setColors(QList<QColor> colors);
        void setNames(QList<QString> names);

    protected:
        virtual void paintEvent(QPaintEvent *event);
        virtual void resizeEvent(QResizeEvent *event);
        virtual void mouseReleaseEvent(QMouseEvent *event);
        virtual void changeEvent(QEvent *event);

    private:
        QList<QColor> colors;
        QList<QString> names;
        QList<double> zones;
        int dominantZone;
        long duration;
        bool collapsed;
        QParallelAnimationGroup toggleAnimation;
        QTextDocument detailsDoc;

        int rowTop(int rowHeight, int row) const;
        QColor color(int idx) const;
        QString name(int idx) const;
        void fillDetailsDoc();
        void adjustHeight();
        std::pair<int, int> heights() const;
};

#endif
