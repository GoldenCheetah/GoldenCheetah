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

#ifndef QXTSCHEDULEVIEW_P_INCLUDED
#define QXTSCHEDULEVIEW_P_INCLUDED

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


#include <QList>
#include <QLinkedList>
#include <QVector>
#include <QAbstractItemModel>
#include <QHeaderView>
#include <QPainterPath>
#include <QTimer>
#include "qxtnamespace.h"
#include "qxtscheduleitemdelegate.h"

class QxtScheduleView;
class QxtScheduleHeaderWidget;

class QxtScheduleInternalItem : public QObject
{
    Q_OBJECT
    friend class QxtScheduleView;

public:

    QxtScheduleInternalItem(QxtScheduleView *parent , QModelIndex index , QVector<QRect> geometries = QVector<QRect>());

    bool                setData(QVariant data , int role);
    QVariant            data(int role) const;

    int                 visualStartTableOffset() const;
    int                 visualEndTableOffset() const;

    int                 startTableOffset() const;
    int                 endTableOffset() const;
    void                setStartTableOffset(int iOffset);

    int                 rows() const;
    void                setRowsUsed(int rows);
    QxtScheduleView*    parentView() const;

    void                startMove();
    void                resetMove();
    void                stopMove();

    bool                contains(const QPoint &pt);
    void                setGeometry(const QVector<QRect> geo);
    QModelIndex         modelIndex() const;
    void                setDirty(bool state = true)
    {
        isDirty = state;
    }

    QVector<QRect>      geometry() const;

Q_SIGNALS:
    void                geometryChanged(QxtScheduleInternalItem * item , QVector<QRect> oldGeometry);

public:
    bool m_moving;
    bool isDirty;
    int m_iModelRow;
    QVector<QRect> m_geometries;
    QVector<QRect> m_SavedGeometries;
    QVector<QPixmap> m_cachedParts;
};

class QxtScheduleViewPrivate : public QObject, public QxtPrivate<QxtScheduleView>
{
    Q_OBJECT
public:

    QXT_DECLARE_PUBLIC(QxtScheduleView)
    QxtScheduleViewPrivate();


    int offsetToVisualColumn(const int iOffset) const;
    int offsetToVisualRow(const int iOffset) const;
    int visualIndexToOffset(const int iRow, const int iCol) const;
    int unixTimeToOffset(const uint constUnixTime, bool indexEndTime = false) const;
    int offsetToUnixTime(const int offset, bool indexEndTime = false) const;
    QVector< QRect > calculateRangeGeometries(const int iStartOffset, const int iEndOffset) const;
    int pointToOffset(const QPoint & point);
    void handleItemConcurrency(const int from, const int to);
    QList< QLinkedList<QxtScheduleInternalItem *> >findConcurrentItems(const int from, const int to) const;


    QxtScheduleInternalItem *internalItemAt(const QPoint &pt);
    QxtScheduleInternalItem *internalItemAtModelIndex(const QModelIndex &index);


    void init();
    void reloadItemsFromModel();

    QxtScheduleInternalItem * itemForModelIndex(const QModelIndex &index)const
    {
        for (int iLoop = 0; iLoop < m_Items.size(); iLoop++)
        {
            if (m_Items.at(iLoop)->modelIndex() == index)
                return m_Items.at(iLoop);
        }
        return 0;
    }

    void handleItemConcurrency(QxtScheduleInternalItem *item)
    {
        if (item)
        {
            int startOffset = item->startTableOffset();
            int endOffset = startOffset +  item->rows() - 1 ;
            handleItemConcurrency(startOffset, endOffset);
        }
    }

    QxtScheduleInternalItem *m_currentItem;
    QxtScheduleInternalItem *m_selectedItem;

    int m_lastMousePosOffset;
    int m_currentZoomDepth;
    int m_zoomStepWidth;
    int m_currentViewMode;
    uint m_startUnixTime;
    uint m_endUnixTime;

    QList<QxtScheduleInternalItem* > m_Items ;
    QList<QxtScheduleInternalItem* > m_InactiveItems; /*used for items that are not in the range of our view but we need to look if they get updates*/

    QTimer scrollTimer;

    QxtScheduleHeaderWidget *m_vHeader;
    QxtScheduleHeaderWidget *m_hHeader;

    int m_Cols;

    QAbstractItemModel *m_Model;
    bool handlesConcurrency;
    QxtScheduleItemDelegate *delegate;
    QxtScheduleItemDelegate *defaultDelegate;

public Q_SLOTS:
    void itemGeometryChanged(QxtScheduleInternalItem * item, QVector<QRect> oldGeometry);
    void scrollTimerTimeout();

};

bool qxtScheduleItemLessThan(const QxtScheduleInternalItem * item1, const QxtScheduleInternalItem * item2);

#endif
