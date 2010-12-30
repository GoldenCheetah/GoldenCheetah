/****************************************************************************
 **
 ** Copyright (C) Qxt Foundation. Some rights reserved.
 **
 ** This file is part of the QxtGui module of the Qxt library.
 **
 ** This library is free software; you can redistribute it and/or modify it
 ** under the terms of the Common Public License, version 1.0, as published
 ** by IBM, and/or under the terms of the GNU Lesser General Public License,
 ** version 2.1, as published by the Free Software Foundation.
 **
 ** This file is provided "AS IS", without WARRANTIES OR CONDITIONS OF ANY
 ** KIND, EITHER EXPRESS OR IMPLIED INCLUDING, WITHOUT LIMITATION, ANY
 ** WARRANTIES OR CONDITIONS OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY OR
 ** FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** You should have received a copy of the CPL and the LGPL along with this
 ** file. See the LICENSE file and the cpl1.0.txt/lgpl-2.1.txt files
 ** included with the source distribution for more information.
 ** If you did not receive a copy of the licenses, contact the Qxt Foundation.
 **
 ** <http://libqxt.org>  <foundation@libqxt.org>
 **
 ****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qxt API.  It exists for the convenience
// of other Qxt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//


#ifndef QXTSCHEDULEVIEWHEADERMODEL_P_H_INCLUDED
#define QXTSCHEDULEVIEWHEADERMODEL_P_H_INCLUDED

#include <QAbstractTableModel>
#include <QtGlobal>
#include <QPointer>

/*!
 *@author Benjamin Zeller <zbenjamin@users.sourceforge.net>
 *@desc This Model is used to tell the Header how many rows and columns the view has
 */

class QxtScheduleView;

class QxtScheduleViewHeaderModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    QxtScheduleViewHeaderModel(QObject *parent = 0);

    void                    setDataSource(QxtScheduleView *dataSource);

    virtual QModelIndex     parent(const QModelIndex & index) const;
    virtual int             rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual int             columnCount(const QModelIndex & parent = QModelIndex()) const;
    virtual  QVariant       data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    virtual bool            setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
    virtual bool            insertRow(int row, const QModelIndex & parent = QModelIndex());
    virtual QVariant        headerData(int section, Qt::Orientation orientation,  int role = Qt::DisplayRole) const;
    virtual Qt::ItemFlags   flags(const QModelIndex &index) const;
    virtual bool            hasChildren(const QModelIndex & parent = QModelIndex()) const;

public Q_SLOTS:
    void                    newZoomDepth(const int zoomDepth);
    void                    viewModeChanged(const int viewMode);

private:
    QPointer<QxtScheduleView> m_dataSource;
    int                       m_rowCountBuffer;
    int                       m_colCountBuffer;
};

#endif // SPLITVIEWHEADERMODEL_H
