/*
 * Copyright (c) 2025 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef QT5COMPATIBILITY__H
#define QT5COMPATIBILITY__H

#include <QTreeWidget>


// Compatibility helper for Qt5
// exposes methods that turned public in Qt6 from protected in Qt5
#if QT_VERSION < 0x060000
class TreeWidget6 : public QTreeWidget
{
    Q_OBJECT

    public:
        TreeWidget6(QWidget *parent = nullptr): QTreeWidget(parent) {
        }

        QModelIndex indexFromItem(const QTreeWidgetItem *item, int column = 0) const {
            return QTreeWidget::indexFromItem(item, column);
        }

        QTreeWidgetItem* itemFromIndex(const QModelIndex &index) const {
            return QTreeWidget::itemFromIndex(index);
        }
};
#else
typedef QTreeWidget TreeWidget6;
#endif


#endif
