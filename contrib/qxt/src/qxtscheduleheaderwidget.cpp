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
#include "qxtscheduleheaderwidget.h"
#include "qxtscheduleview.h"
#include "qxtscheduleviewheadermodel_p.h"

#include <QPainter>
#include <QDateTime>
#include <QDate>
#include <QTime>


/*!
 *  @internal the QxtAgendaHeaderWidget operates on a internal model , that uses the QxtScheduleView as DataSource
 *
 */

QxtScheduleHeaderWidget::QxtScheduleHeaderWidget(Qt::Orientation orientation , QxtScheduleView *parent) : QHeaderView(orientation, parent)
{
    QxtScheduleViewHeaderModel *model = new QxtScheduleViewHeaderModel(this);
    setModel(model);

    if (parent)
    {
        model->setDataSource(parent);
    }
}

void QxtScheduleHeaderWidget::paintSection(QPainter * painter, const QRect & rect, int logicalIndex) const
{
    if (model())
    {
        switch (orientation())
        {
        case Qt::Horizontal:
        {
            QHeaderView::paintSection(painter, rect, logicalIndex);
        }
        break;
        case Qt::Vertical:
        {
            QTime time = model()->headerData(logicalIndex, Qt::Vertical, Qt::DisplayRole).toTime();
            if (time.isValid())
            {
                QRect temp = rect;
                temp.adjust(1, 1, -1, -1);

                painter->fillRect(rect, this->palette().background());

                if (time.minute() == 0)
                {
                    painter->drawLine(temp.topLeft() + QPoint(temp.width() / 3, 0), temp.topRight());
                    painter->drawText(temp, Qt::AlignTop | Qt::AlignRight, time.toString("hh:mm"));
                }
            }
        }
        break;
        default:
            Q_ASSERT(false); //this will never happen... normally
        }
    }
}

void QxtScheduleHeaderWidget::setModel(QxtScheduleViewHeaderModel *model)
{
    QHeaderView::setModel(model);
}
