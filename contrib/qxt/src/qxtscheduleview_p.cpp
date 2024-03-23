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

#include "qxtscheduleview_p.h"
#include "qxtscheduleview.h"
#include <QPainter>
#include <QScrollBar>
#include <QBrush>
#include <QMouseEvent>
#include <QDebug>
#include <QApplication>
#include <QTimer>
#include <QStringList>
#include <QWidget>
#include <QList>
#include <QListIterator>
#include <QMutableLinkedListIterator>

#include "qxtscheduleheaderwidget.h"

/*-------------------------------------start private functions-------------------------------------------------------*/

int QxtScheduleViewPrivate::offsetToVisualColumn(const int iOffset) const
{
    if (iOffset >= 0)
        return iOffset / qxt_p().rows();
    return -1;
}

int QxtScheduleViewPrivate::visualIndexToOffset(const int iRow, const int iCol) const
{
    return (iCol* qxt_p().rows()) + iRow;
}

int QxtScheduleViewPrivate::offsetToVisualRow(const int iOffset) const
{
    if (iOffset >= 0 && qxt_p().model())
        return iOffset % qxt_p().rows();
    return -1;
}

QVector< QRect > QxtScheduleViewPrivate::calculateRangeGeometries(const int iStartOffset, const int iEndOffset) const
{
    QVector<QRect> rects;
//qDebug()<<"calc geometries..."<<iStartOffset<<iEndOffset;
    if (iStartOffset < 0 || iEndOffset < 0)
        return rects;

    if (iEndOffset < iStartOffset)
        return rects;

    int iCurrentStartOffset = iStartOffset;
    int iCurrentEndOffset;

//qDebug()<<"calc geometries... got to the do!";
    do
    {
        if (offsetToVisualColumn(iCurrentStartOffset) == offsetToVisualColumn(iEndOffset))
            iCurrentEndOffset = iEndOffset;
        else
            iCurrentEndOffset = visualIndexToOffset(m_vHeader->count() - 1, offsetToVisualColumn(iCurrentStartOffset));

        qint32 iLeft = m_hHeader->sectionPosition(offsetToVisualColumn(iCurrentStartOffset));
        qint32 iTop = m_vHeader->sectionPosition(offsetToVisualRow(iCurrentStartOffset));
        qint32 iBottom = m_vHeader->sectionPosition(offsetToVisualRow(iCurrentEndOffset)) + m_vHeader->sectionSize(offsetToVisualRow(iCurrentEndOffset));
        qint32 iRight = m_hHeader->sectionPosition(offsetToVisualColumn(iCurrentEndOffset)) + m_hHeader->sectionSize(offsetToVisualColumn(iCurrentEndOffset));
        rects.append(QRect(iLeft + 1, iTop + 1, iRight - iLeft - 1, iBottom - iTop - 1));

        iCurrentStartOffset = visualIndexToOffset(0, offsetToVisualColumn(iCurrentEndOffset) + 1);

    }
    while (iCurrentEndOffset < iEndOffset);

//qDebug()<<"calc geometries... returing:"<<rects.size();
    return rects;

}

int QxtScheduleViewPrivate::pointToOffset(const QPoint & point)
{
    int iRow = m_vHeader->visualIndexAt(point.y());
    int iCol = m_hHeader->visualIndexAt(point.x());
    return visualIndexToOffset(iRow, iCol);
}



QxtScheduleInternalItem * QxtScheduleViewPrivate::internalItemAt(const QPoint & pt)
{
    QListIterator<QxtScheduleInternalItem *>iterator(m_Items);
    QxtScheduleInternalItem *currentItem;
    iterator.toBack();
    while (iterator.hasPrevious())
    {
        currentItem = iterator.previous();
        if (currentItem->contains(pt))
            return currentItem;
    }
    return 0;
}


void QxtScheduleViewPrivate::reloadItemsFromModel()
{
    qDeleteAll(m_Items.begin(), m_Items.end());
    m_Items.clear();
    m_selectedItem = NULL;

    int iNumItems = qxt_p().model()->rowCount();
    //delete all old stuff here
    QxtScheduleInternalItem *currentItem;
    for (int iLoop = 0; iLoop < iNumItems; iLoop++)
    {
        currentItem = new QxtScheduleInternalItem(&qxt_p(), qxt_p().model()->index(iLoop, 0));
        m_Items.append(currentItem);
        connect(currentItem, SIGNAL(geometryChanged(QxtScheduleInternalItem*, QVector<QRect>)), this, SLOT(itemGeometryChanged(QxtScheduleInternalItem * , QVector< QRect >)));
    }

    handleItemConcurrency(0, (qxt_p().rows()*qxt_p().cols()) - 1);
}

void QxtScheduleViewPrivate::init()
{
    if (qxt_p().model())
    {
        qxt_p().viewport()->setMouseTracking(true);

        if (!m_vHeader)
        {
            m_vHeader = new QxtScheduleHeaderWidget(Qt::Vertical, &qxt_p());
            connect(m_vHeader, SIGNAL(geometriesChanged()), &qxt_p(), SLOT(updateGeometries()));
        }
        m_vHeader->show();

        if (!m_hHeader)
        {
            m_hHeader = new QxtScheduleHeaderWidget(Qt::Horizontal, &qxt_p());
            connect(m_hHeader, SIGNAL(geometriesChanged()), &qxt_p(), SLOT(updateGeometries()));
        }
        m_hHeader->show();

        /*here we also initialize the items*/
        m_vHeader->setDefaultSectionSize(20);
        m_vHeader->setResizeMode(QHeaderView::Fixed);
        reloadItemsFromModel();
    }
    qxt_p().updateGeometries();
}

/*!
 * @desc collects groups of concurrent items in the offset range
 */
QList< QLinkedList<QxtScheduleInternalItem *> > QxtScheduleViewPrivate::findConcurrentItems(const int from, const int to) const
{
    QList< QLinkedList<QxtScheduleInternalItem *> > allConcurrentItems;

    QList<QxtScheduleInternalItem *> allItemsSorted = m_Items;
    
    if(m_Items.size() == 0)
        return allConcurrentItems;
    
    qSort(allItemsSorted.begin(), allItemsSorted.end(), qxtScheduleItemLessThan);

    int startItem = 0;
    int endItem = allItemsSorted.size() - 1;

    //find the startitem that interferes with our range
    for (int i = 0; i < allItemsSorted.size(); i++)
    {
        if (i > 0)
        {
            if (!(allItemsSorted.at(i - 1)->visualEndTableOffset() >=  allItemsSorted.at(i)->visualStartTableOffset()
                    && allItemsSorted.at(i - 1)->visualStartTableOffset() <=  allItemsSorted.at(i)->visualEndTableOffset()))
                startItem = i;
        }

        if (allItemsSorted.at(i)->visualEndTableOffset() >= from && allItemsSorted.at(i)->visualStartTableOffset() <= to)
            break;
    }

    //find the last item that interferes with our range
    for (int i = allItemsSorted.size() - 1; i >= 0 ; i--)
    {
        if (i < allItemsSorted.size() - 1)
        {
            if (!(allItemsSorted.at(i + 1)->visualEndTableOffset() >=  allItemsSorted.at(i)->visualStartTableOffset()
                    && allItemsSorted.at(i + 1)->visualStartTableOffset() <=  allItemsSorted.at(i)->visualEndTableOffset()))
                endItem = i;
        }

        if (allItemsSorted.at(i)->visualEndTableOffset() >= from && allItemsSorted.at(i)->visualStartTableOffset() <= to)
            break;
    }

    int startOffset = allItemsSorted.at(startItem)->visualStartTableOffset();
    int endOffset = allItemsSorted.at(endItem)->visualEndTableOffset();

    /*now we have to populate a list with all items that interfere with our range */
    QLinkedList<QxtScheduleInternalItem *> concurrentItems;
    for (int iAllItemLoop = startItem; iAllItemLoop <= endItem; iAllItemLoop++)
    {
        int tempStartOffset = allItemsSorted.at(iAllItemLoop)->visualStartTableOffset();
        int tempEndOffset = allItemsSorted.at(iAllItemLoop)->visualEndTableOffset();

        if (tempEndOffset >= startOffset && tempStartOffset <= endOffset)
        {

            if (concurrentItems.size() >= 1)
            {
                bool bAppend = false;
                /*check all items in the list if the current items interfers although the items are ordered by startIndex
                *we can loose some of them if the endTime of the last Item is before the endTime of the pre last item
                */

                for (QLinkedList<QxtScheduleInternalItem *>::iterator it = concurrentItems.begin(); it != concurrentItems.end(); ++it)
                {
                    int lastStartOffset = (*it)->visualStartTableOffset();
                    int lastEndOffset   = (*it)->visualEndTableOffset();

                    if (tempEndOffset >= lastStartOffset && tempStartOffset <= lastEndOffset)
                    {
                        bAppend = true;
                        break;
                    }
                }

                if (bAppend)
                {
                    concurrentItems.append(allItemsSorted.at(iAllItemLoop));
                }
                else
                {
                    allConcurrentItems.append(concurrentItems);
                    concurrentItems.clear();
                    concurrentItems.append(allItemsSorted.at(iAllItemLoop));
                }
            }
            else
                concurrentItems.append(allItemsSorted.at(iAllItemLoop));

            if (tempStartOffset < startOffset)
                startOffset = tempStartOffset;

            if (tempEndOffset > endOffset)
                endOffset = tempEndOffset;
        }
    }
    if (concurrentItems.size() > 0)
        allConcurrentItems.append(concurrentItems);

    return allConcurrentItems;
}

void QxtScheduleViewPrivate::handleItemConcurrency(const int from, const int to)
{ 
    /*collect all items that interfere only in that range*/
    if (from < 0  ||  to < 0 || m_Items.size() == 0){
        //do a update or we may have artifacts
        qxt_p().viewport()->update();
        return;
    }

    //qDebug() << "handleItemConcurrency";

    if (handlesConcurrency)
        return;

    handlesConcurrency = true;

    QList< QLinkedList<QxtScheduleInternalItem *> > allConcurrentItems =  findConcurrentItems(from, to);

    /*thx to ahigerd for suggesting that algorithm*/
    //[16:24] <ahigerd> Start with the first event. Put it in a list.
    //[16:25] <ahigerd> Iterate until you find an event that doesn't overlap with the first event. Put it in the list. Repeat until you've reached the last event.
    //[16:25] <ahigerd> This fills the left column optimally.
    //[16:25] <ahigerd> Repeat the algorithm for the second column, etc., until there aren't any events left that don't have a column.
    //[16:27] <ahigerd> This algorithm is O(n*m), where n is the number of events and m is the maximum number of overlapping events.

    QList< QList< QxtScheduleInternalItem *> >virtualTable;

    for (int iListLoop = 0; iListLoop < allConcurrentItems.size(); iListLoop++)
    {
        QLinkedList<QxtScheduleInternalItem *> & currentItems = allConcurrentItems[iListLoop];
        QList< QxtScheduleInternalItem * > currentColumn;

        //qDebug() << "handle overlapping for " << currentItems.size() << " Items";

        virtualTable.clear();

        //we iterate over the currect collection and remove every item that can be placed in the current column
        //when the collection is empty we are done
        while (currentItems.size())
        {
            QMutableLinkedListIterator< QxtScheduleInternalItem * > iter(currentItems);

            while (iter.hasNext())
            {
                iter.next();
                //initialize the current column
                if (currentColumn.isEmpty() || currentColumn[currentColumn.size()-1]->visualEndTableOffset() < iter.value()->visualStartTableOffset())
                {
                    currentColumn.append(iter.value());
                    iter.remove();
                    continue;
                }
            }

            if (!currentColumn.isEmpty())
            {
                virtualTable.append(currentColumn);
                currentColumn.clear();
            }
        }

        //qDebug() << "Found columns" << virtualTable.size();

        //this code part resizes the item geometries
        for (int col = 0; col < virtualTable.size(); col++)
        {
            for (int item = 0; item < virtualTable.at(col).size() ; item++)
            {
                int startVisualCol = offsetToVisualColumn(virtualTable[col][item]->visualStartTableOffset());
                QVector<QRect> geo = virtualTable[col][item]->geometry();

                for (int rect = 0; rect < geo.size(); rect++)
                {
                    int sectionStart = m_hHeader->sectionPosition(startVisualCol);
                    int fullWidth    = m_hHeader->sectionSize(startVisualCol);
                    int oneItemWidth = fullWidth / virtualTable.size();
                    int itemWidth    = oneItemWidth;
                    int itemXStart   = (col * oneItemWidth) + sectionStart;
                    int overlap      = oneItemWidth / 10;
                    int adjustX1     = 0;
                    int adjustX2     = 0;

                    //this is very expensive.I try to check if my item can span over more than one col
                    int possibleCols = 1;
                    bool foundCollision;

                    for (int tmpCol = col + 1; tmpCol < virtualTable.size(); tmpCol++)
                    {
                        foundCollision = false;
                        for (int tmpItem = 0; tmpItem < virtualTable.at(tmpCol).size() ; tmpItem++)
                        {
                            if ((virtualTable[tmpCol][tmpItem]->visualEndTableOffset() >=  virtualTable[col][item]->visualStartTableOffset()
                                    && virtualTable[tmpCol][tmpItem]->visualStartTableOffset() <=  virtualTable[col][item]->visualEndTableOffset()))
                            {
                                foundCollision = true;
                                break;
                            }
                        }

                        if (!foundCollision)
                            possibleCols++;
                        else
                            break;
                    }
                    //now lets adjust the size to get a nice overlapping of items
                    if (virtualTable.size() > 1)
                    {
                        if (col == 0)
                            adjustX2 = overlap;
                        else if (col == virtualTable.size() - 1)
                        {
                            adjustX1 = -overlap;
                            adjustX2 = overlap;
                        }
                        else
                        {
                            if (col + possibleCols == virtualTable.size())
                                adjustX2 = overlap;
                            else
                                adjustX2 = overlap * 2;

                            adjustX1 = -overlap;
                        }
                    }

                    // possibleCols = 1;
                    itemWidth = oneItemWidth * possibleCols;

                    //qDebug() << "orginial rect" << geo[rect];
                    geo[rect].setLeft(itemXStart + adjustX1);
                    geo[rect].setWidth(itemWidth + adjustX2);
                    //qDebug() << "new rect" << geo[rect];


                    startVisualCol++;
                }
                virtualTable[col][item]->setGeometry(geo);
            }
        }
    }
    handlesConcurrency = false;
    qxt_p().viewport()->update();
}

QxtScheduleViewPrivate::QxtScheduleViewPrivate()
{
    m_Cols = 0;
    m_vHeader = 0;
    m_hHeader = 0;
    m_Model = 0;
    m_selectedItem = 0;
    handlesConcurrency = false;
    delegate = 0;
    m_zoomStepWidth = 0;

    connect(&scrollTimer, SIGNAL(timeout()), this, SLOT(scrollTimerTimeout()));
}

void QxtScheduleViewPrivate::itemGeometryChanged(QxtScheduleInternalItem * item, QVector< QRect > oldGeometry)
{
    QRegion oldRegion;

    if (item->geometry() == oldGeometry)
        return;

    QVectorIterator<QRect> iter(oldGeometry);
    QRect currRect;
    while (iter.hasNext())
    {
        currRect = iter.next();
        currRect.adjust(-1, -1, 2, 2);
        oldRegion += currRect;
    }
    //viewport()->update(oldRegion);


    QRegion newRegion;
    QVectorIterator<QRect> newIter(item->geometry());
    while (newIter.hasNext())
    {
        currRect = newIter.next();
        currRect.adjust(-1, -1, 2, 2);
        newRegion += currRect;
    }
    //viewport()->update(newRegion);
    qxt_p().viewport()->update();
}

int QxtScheduleViewPrivate::unixTimeToOffset(const uint constUnixTime, bool indexEndTime) const
{
    uint unixTime = constUnixTime;
    if (unixTime >= m_startUnixTime && unixTime <= m_endUnixTime)
    {
        if (indexEndTime)
        {
            unixTime -= m_currentZoomDepth;
        }
        qint32 rows = qxt_p().rows();
        qint32 iOffset = unixTime - m_startUnixTime;

        //round to the closest boundaries
        iOffset = qRound((qreal)iOffset / (qreal)m_currentZoomDepth);

        qint32 iCol = iOffset / rows;
        qint32 iRow = iOffset % rows;
        return visualIndexToOffset(iRow, iCol);
    }
    //virtual void handleItemOverlapping(QxtScheduleInternalItem *item);
    return -1;
}

void QxtScheduleViewPrivate::scrollTimerTimeout()
{
    QPoint globalPos = QCursor::pos();
    QPoint viewportPos = qxt_p().viewport()->mapFromGlobal(globalPos);

    int iScrollVertical = this->m_vHeader->defaultSectionSize();
    int iScrollHorizontal = this->m_hHeader->defaultSectionSize();

    if (viewportPos.y() <= iScrollVertical)
    {
        int iCurrPos = qxt_p().verticalScrollBar()->value();
        if (iCurrPos > qxt_p().verticalScrollBar()->minimum() + iScrollVertical)
        {
            qxt_p().verticalScrollBar()->setValue(iCurrPos - iScrollVertical);
        }
        else
            qxt_p().verticalScrollBar()->setValue(qxt_p().verticalScrollBar()->minimum());

    }
    else if (viewportPos.y() >= qxt_p().viewport()->height() - iScrollVertical)
    {
        int iCurrPos = qxt_p().verticalScrollBar()->value();
        if (iCurrPos < qxt_p().verticalScrollBar()->maximum() - iScrollVertical)
        {
            qxt_p().verticalScrollBar()->setValue(iCurrPos + iScrollVertical);
        }
        else
            qxt_p().verticalScrollBar()->setValue(qxt_p().verticalScrollBar()->maximum());
    }

    if (viewportPos.x() <= iScrollHorizontal / 2)
    {
        int iCurrPos = qxt_p().horizontalScrollBar()->value();
        if (iCurrPos > qxt_p().horizontalScrollBar()->minimum() + iScrollHorizontal)
        {
            qxt_p().horizontalScrollBar()->setValue(iCurrPos - iScrollHorizontal);
        }
        else
            qxt_p().horizontalScrollBar()->setValue(qxt_p().horizontalScrollBar()->minimum());

    }
    else if (viewportPos.x() >= qxt_p().viewport()->width() - (iScrollHorizontal / 2))
    {
        int iCurrPos = qxt_p().horizontalScrollBar()->value();
        if (iCurrPos < qxt_p().horizontalScrollBar()->maximum() - iScrollHorizontal)
        {
            qxt_p().horizontalScrollBar()->setValue(iCurrPos + iScrollHorizontal);
        }
        else
            qxt_p().horizontalScrollBar()->setValue(qxt_p().horizontalScrollBar()->maximum());
    }

}

int QxtScheduleViewPrivate::offsetToUnixTime(const int offset, bool indexEndTime) const
{
    qint32 rows = qxt_p().rows();
    uint unixTime = (offsetToVisualRow(offset) + (offsetToVisualColumn(offset) * rows)) * m_currentZoomDepth;
    unixTime += m_startUnixTime;

    if (indexEndTime)
    {
        unixTime += m_currentZoomDepth;
    }

    if (unixTime >= m_startUnixTime && unixTime <= m_endUnixTime + 1)
        return unixTime;
    return -1;
}


QxtScheduleInternalItem::QxtScheduleInternalItem(QxtScheduleView *parent, QModelIndex index, QVector<QRect> geometries)
        : QObject(parent), m_iModelRow(index.row()), m_geometries(geometries)
{
    m_moving = false;
    if (parent)
    {
        if (index.isValid())
        {
            if (m_geometries.empty())
            {
//qDebug()<<"add item: geometries empty so adding";
                int startOffset = this->startTableOffset();
                int endOffset = startOffset + this->rows() - 1;
                m_geometries = parent->qxt_d().calculateRangeGeometries(startOffset, endOffset);
            }
        }
    }
}

QxtScheduleView * QxtScheduleInternalItem::parentView() const
{
    return qobject_cast<QxtScheduleView *>(parent());
}

/*!
 * @desc returns the currently VISUAL offset (changes when a item moves)
 */
int QxtScheduleInternalItem::visualStartTableOffset() const
{
    if (m_geometries.size() == 0 || !parentView())
        return -1;

    //are we in a move?
    if (!m_moving)
        return startTableOffset();

    int offset = parentView()->qxt_d().pointToOffset(parentView()->mapToViewport((this->geometry()[0].topLeft())));
    return offset;
}

/*!
 * @desc returns the currently VISUAL offset (changes when a item moves)
 */
int QxtScheduleInternalItem::visualEndTableOffset() const
{
    if (m_geometries.size() == 0 || !parentView())
        return -1;

    //are we in a move?
    if (!m_moving)
        return endTableOffset();

    QRect rect = m_geometries[m_geometries.size()-1];
    int endTableOffset = parentView()->qxt_d().pointToOffset(parentView()->mapToViewport(rect.bottomRight()));
    return endTableOffset;
}

bool QxtScheduleInternalItem::setData(QVariant data, int role)
{
    if (parentView() && parentView()->model())
    {
        return parentView()->model()->setData(modelIndex(), data, role);
    }
    return false;
}

QVariant QxtScheduleInternalItem::data(int role) const
{
    if (modelIndex().isValid())
        return modelIndex().data(role);
    return QVariant();
}

int QxtScheduleInternalItem::startTableOffset() const
{
    if (parentView() && parentView()->model())
    {
        int startTime = data(Qxt::ItemStartTimeRole).toInt();
        int zoomDepth = parentView()->currentZoomDepth(Qxt::Second);

        qint32 offset = startTime - parentView()->qxt_d().m_startUnixTime;

        //the start of the current item does not fit in the view
        //so we have to align it to the nearest boundaries
        if (offset % zoomDepth)
        {
            int lower = offset / zoomDepth * zoomDepth;
            int upper = lower + zoomDepth;

            offset = (offset - lower >= upper - offset ? upper : lower);

            return parentView()->qxt_d().unixTimeToOffset(offset + parentView()->qxt_d().m_startUnixTime);
        }

//qDebug()<<"start Table offset kicked in... offset="<<offset<<"returning="<<parentView()->qxt_d().unixTimeToOffset(startTime);
        return parentView()->qxt_d().unixTimeToOffset(startTime);
    }
    return -1;
}

int QxtScheduleInternalItem::endTableOffset() const
{
    return startTableOffset() + rows() - 1;
}

void QxtScheduleInternalItem::setStartTableOffset(int iOffset)
{
    if (parentView() && parentView()->model())
    {
        setData(parentView()->qxt_d().offsetToUnixTime(iOffset), Qxt::ItemStartTimeRole);
    }
}

void QxtScheduleInternalItem::setRowsUsed(int rows)
{
    if (parentView() && parentView()->model())
    {
        int seconds = rows * parentView()->currentZoomDepth(Qxt::Second);
        setData(seconds, Qxt::ItemDurationRole);
    }
}

int QxtScheduleInternalItem::rows() const
{
    if (parentView() && parentView()->model())
    {
        int iNumSecs = data(Qxt::ItemDurationRole).toInt();
        int zoomDepth = parentView()->currentZoomDepth(Qxt::Second);

        //the length of the current item does not fit in the view
        //so we have to align it to the nearest boundaries
        if (iNumSecs % zoomDepth)
        {
            int lower = iNumSecs / zoomDepth * zoomDepth;
            int upper = lower + zoomDepth;

            return (iNumSecs - lower >= upper - iNumSecs ? (upper / zoomDepth) : (lower / zoomDepth));

        }
        return (iNumSecs / zoomDepth);
    }
    return -1;
}

bool qxtScheduleItemLessThan(const QxtScheduleInternalItem * item1, const QxtScheduleInternalItem * item2)
{
    if (item1->visualStartTableOffset() <   item2->visualStartTableOffset())
        return true;
    if (item1->visualStartTableOffset() ==  item2->visualStartTableOffset() && item1->modelIndex().row() < item2->modelIndex().row())
        return true;
    return false;
}

bool QxtScheduleInternalItem::contains(const QPoint & pt)
{
    QVectorIterator<QRect> iterator(this->m_geometries);
    while (iterator.hasNext())
    {
        if (iterator.next().contains(pt))
            return true;
    }
    return false;
}

void QxtScheduleInternalItem::setGeometry(const QVector< QRect > geo)
{
    if (!this->parent())
        return;

    QVector<QRect> oldGeo = this->m_geometries;
    this->m_geometries.clear();
    this->m_geometries = geo;
    emit geometryChanged(this, oldGeo);
}

QVector< QRect > QxtScheduleInternalItem::geometry() const
{
    return this->m_geometries;
}

void QxtScheduleInternalItem::startMove()
{
    //save old geometry before we start to move
    this->m_SavedGeometries = this->m_geometries;
    m_moving = true;
}

void QxtScheduleInternalItem::resetMove()
{
    this->setGeometry(this->m_SavedGeometries);
    this->m_SavedGeometries.clear();
    m_moving = false;
}

void QxtScheduleInternalItem::stopMove()
{
    this->m_SavedGeometries.clear();
    m_moving = false;
}

QModelIndex QxtScheduleInternalItem::modelIndex() const
{
    QModelIndex indx;
    if (parentView() && parentView()->model())
    {
        indx = parentView()->model()->index(this->m_iModelRow, 0);
    }
    return indx;
}
