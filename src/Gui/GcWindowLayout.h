/*
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_GcWindowLayout_h
#define _GC_GcWindowLayout_h

#include <QWidget>
#include <QLayout>
#include <QRect>
#include <QStyle>
#include <QWidgetItem>

// A simple layout manager that lays out widgets
// left to right and top to bottom. So as widgets
// are resized and their neighbour won't fit to
// the right they move down.
class GcWindowLayout : public QLayout
{
public:
    GcWindowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
    GcWindowLayout(int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~GcWindowLayout();

    void addItem(QLayoutItem *item);
    void insert(int index, QWidget *item);

    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const;
    bool hasHeightForWidth() const;
    int heightForWidth(int) const;
    int count() const;
    QLayoutItem *itemAt(int index) const;
    QSize minimumSize() const;
    void setGeometry(const QRect &rect);
    QSize sizeHint() const;
    QLayoutItem *takeAt(int index);

private:
    int smartSpacing(QStyle::PixelMetric pm) const;
    int doLayout(const QRect &rect, bool testOnly) const;

    QList<QLayoutItem *> itemList;
    int m_hSpace;
    int m_vSpace;
};

#endif
