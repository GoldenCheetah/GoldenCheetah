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
#ifndef QXTSCHEDULEITEMDELEGATE_H_INCLUDED
#define QXTSCHEDULEITEMDELEGATE_H_INCLUDED

#include <QAbstractItemDelegate>
#include <QPainterPath>
#include "qxtstyleoptionscheduleviewitem.h"
#include "qxtglobal.h"

QT_FORWARD_DECLARE_CLASS(QPainter)

/*!
    @author Benjamin Zeller <zbenjamin@users.sourceforge.net>
*/
class QXT_GUI_EXPORT QxtScheduleItemDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:

    enum ItemPart
    {
        Top,
        Middle,
        Bottom,
        Single

    };

    QxtScheduleItemDelegate(QObject *parent = 0);
    ~QxtScheduleItemDelegate();

    virtual void        paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual void        paintItemBody(QPainter *painter, const QRect rect , const QxtStyleOptionScheduleViewItem & option , const ItemPart part, const QModelIndex & index) const;
    virtual void        paintItemHeader(QPainter *painter, const QRect rect , const QxtStyleOptionScheduleViewItem & option , const QModelIndex & index) const;
    virtual void        paintSubItem(QPainter *painter, const QRect rect , const QxtStyleOptionScheduleViewItem & option , const QModelIndex & index) const;
    virtual QSize       sizeHint(const QStyleOptionViewItem & option , const QModelIndex & index) const;
    virtual void        createPainterPath(QPainterPath &emptyPath, const QRect &fullItemRect , const int iRoundTop, const int iRoundBottom) const;


};

#endif // QXTSCHEDULEITEMDELEGATE_H_INCLUDED
