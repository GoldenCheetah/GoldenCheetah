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

#ifndef _GC_MultiFilterProxyModel_h
#define _GC_MultiFilterProxyModel_h


#include <QSortFilterProxyModel>
#include <QList>

#include "ModelFilter.h"


class MultiFilterProxyModel: public QSortFilterProxyModel
{
    Q_OBJECT

public:
    MultiFilterProxyModel(QObject *parent = nullptr);
    virtual ~MultiFilterProxyModel();

    virtual void setFilters(QList<ModelFilter*> filters);
    void removeFilters(bool invalidate = true);
    virtual QList<ModelFilter*> filters() const;

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QList<ModelFilter*> _filters;
};

#endif // MultiFilterProxyModel_H
