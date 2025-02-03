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

#include "MultiFilterProxyModel.h"

#include <QDebug>



MultiFilterProxyModel::MultiFilterProxyModel
(QObject *parent)
: QSortFilterProxyModel(parent), _filters()
{
}


MultiFilterProxyModel::~MultiFilterProxyModel
()
{
    removeFilters(true);
}


void
MultiFilterProxyModel::setFilters
(QList<ModelFilter*> filters)
{
    removeFilters(false);
    _filters = filters;
    invalidateFilter();
}


QList<ModelFilter*>
MultiFilterProxyModel::filters
() const
{
    return _filters;
}


void
MultiFilterProxyModel::removeFilters
(bool invalidate)
{
    for (auto &filter : _filters) {
        delete filter;
    }
    _filters.clear();
    if (invalidate) {
        invalidateFilter();
    }
}


bool
MultiFilterProxyModel::filterAcceptsRow
(int source_row, const QModelIndex &source_parent) const
{
    for (auto &filter : _filters) {
        if (filter->modelColumn() < 0 || filter->modelColumn() > sourceModel()->columnCount()) {
            return false;
        }
        if (! filter->accept(sourceModel()->index(source_row, filter->modelColumn(), source_parent).data())) {
            return false;
        }
    }
    return true;
}
