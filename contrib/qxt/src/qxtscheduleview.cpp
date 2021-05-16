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
 ** <http://libqxt.org>  <foundation\libqxt.org>
 **
 ****************************************************************************/

#include "qxtscheduleview.h"
#include "qxtscheduleview_p.h"
#include "qxtscheduleheaderwidget.h"
#include "qxtscheduleviewheadermodel_p.h"
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
#include <QWheelEvent>


/*!
  \class QxtScheduleView QxtScheduleView
  \inmodule QxtGui
  \brief The QxtScheduleView class provides an iCal like view to plan events

  QxtScheduleView is a item based View,inspired by iCal, that makes it possible to visualize event planning.

  \raw HTML
  It's  time based and can show the events in different modes:
  <ul>
  <li><strong>DayMode</strong>    : Every column in the view shows one day</li>
  <li><strong>HourMode</strong>   : Every column in the view shows one hour</li>
  <li><strong>MinuteMode</strong> : Every column in the view shows one minute</li>
  </ul>
  In addition you can adjust how much time every cell represents in the view. The default value is 900 seconds
  or 15 minutes and DayMode.
  \endraw


  \image qxtscheduleview.png QxtScheduleView

*/

QxtScheduleView::QxtScheduleView(QWidget *parent)
        : QAbstractScrollArea(parent)
{
    QXT_INIT_PRIVATE(QxtScheduleView);

    /*standart values are 15 minutes per cell and 69 rows == 1 Day*/
    qxt_d().m_currentZoomDepth = 15 * 60;
    qxt_d().m_currentViewMode  = DayView;
    qxt_d().m_startUnixTime    = QDateTime(QDate::currentDate(),QTime(0, 0, 0)).toTime_t();
    qxt_d().m_endUnixTime      = QDateTime(QDate::currentDate().addDays(6),QTime(23, 59, 59)).toTime_t();
    qxt_d().delegate = qxt_d().defaultDelegate = new QxtScheduleItemDelegate(this);

#if 0
    qxt_d().m_vHeader = new QxtScheduleHeaderWidget(Qt::Vertical, this);
    connect(qxt_d().m_vHeader, SIGNAL(geometriesChanged()), this, SLOT(updateGeometries()));
    qxt_d().m_vHeader->hide();

    qxt_d().m_hHeader = new QxtScheduleHeaderWidget(Qt::Horizontal, this);
    connect(qxt_d().m_hHeader, SIGNAL(geometriesChanged()), this, SLOT(updateGeometries()));
    qxt_d().m_hHeader->hide();
#else
    //init will take care of initializing headers
    qxt_d().m_vHeader = 0;
    qxt_d().m_hHeader = 0;
#endif

}

/*!
 * returns the vertial header
 *\note can be NULL if the view has not called init() already (FIXME)
 */
QHeaderView* QxtScheduleView::verticalHeader ( ) const
{
    return qxt_d().m_vHeader;
}

/**
 * returns the horizontal header
 *\note can be NULL if the view has not called init() already (FIXME)
 */
QHeaderView* QxtScheduleView::horizontalHeader ( ) const
{
    return qxt_d().m_hHeader;
}
    
/**
 *  sets the model for QxtScheduleView
 *
 * \a model
 */
void QxtScheduleView::setModel(QAbstractItemModel *model)
{
    if (qxt_d().m_Model)
    {
        /*delete all cached items*/
        qDeleteAll(qxt_d().m_Items.begin(), qxt_d().m_Items.end());
        qxt_d().m_Items.clear();

        /*disconnect all signals*/
        disconnect(qxt_d().m_Model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this, SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));
        disconnect(qxt_d().m_Model, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)), this, SLOT(rowsAboutToBeInserted(const QModelIndex &, int , int)));
        disconnect(qxt_d().m_Model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsInserted(const QModelIndex &, int , int)));
        disconnect(qxt_d().m_Model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)), this, SLOT(rowsAboutToBeRemoved(const QModelIndex &, int , int)));
        disconnect(qxt_d().m_Model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(rowsRemoved(const QModelIndex &, int , int)));

        /*don't delete the model maybe someone else will use it*/
        qxt_d().m_Model = 0;
    }

    if (model != 0)
    {
        /*initialize the new model*/
        qxt_d().m_Model = model;
        connect(model, SIGNAL(dataChanged(const QModelIndex &, const  QModelIndex &)), this, SLOT(dataChanged(const QModelIndex &, const QModelIndex &)));
        connect(model, SIGNAL(rowsAboutToBeInserted(const QModelIndex &, int, int)), this, SLOT(rowsAboutToBeInserted(const QModelIndex &, int , int)));
        connect(model, SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(rowsInserted(const QModelIndex &, int , int)));
        connect(model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)), this, SLOT(rowsAboutToBeRemoved(const QModelIndex &, int , int)));
        connect(model, SIGNAL(rowsRemoved(const QModelIndex &, int, int)), this, SLOT(rowsRemoved(const QModelIndex &, int , int)));
    }
    qxt_d().init();
}

QAbstractItemModel * QxtScheduleView::model() const
{
    return qxt_d().m_Model;
}

/*!
 *  changes the current ViewMode
 * The QxtScheduleView supports some different viewmodes. A viewmode defines how much time a column holds.
 * It is also possible to define custom viewmodes. To do that you have to set the currentView mode to Custom and
 * reimplement timePerColumn
 *
 * \a  QxtScheduleView::ViewMode mode the new ViewMode
 *
 * \sa timePerColumn()
 * \sa viewMode()
 */
void QxtScheduleView::setViewMode(const QxtScheduleView::ViewMode mode)
{
    qxt_d().m_currentViewMode = mode;

    //this will calculate the correct alignment
    //\BUG this may not work because the currentZoomDepth may not fit into the new viewMode
    setCurrentZoomDepth(qxt_d().m_currentZoomDepth);
}

/*!
 *\\esc returns the current used delegate
 */
QxtScheduleItemDelegate* QxtScheduleView::delegate () const
{
    return qxt_d().delegate;
}

/*!
 *Sets the item delegate for this view and its model to delegate. This is useful if you want complete control over the editing and display of items.
*Any existing delegate will be removed, but not deleted. QxtScheduleView does not take ownership of delegate.
*Passing a 0 pointer will restore the view to use the default delegate.
*\Warning You should not share the same instance of a delegate between views. Doing so can cause incorrect or unintuitive behavior.
 */
void QxtScheduleView::setItemDelegate (QxtScheduleItemDelegate * delegate)
{
    if(!delegate)
        qxt_d().delegate = qxt_d().defaultDelegate;
    else
        qxt_d().delegate = delegate;
        
    //the delegate changed repaint everything
    viewport()->update();
}

/*!
 *  returns the current ViewMode
 *
 * Returns QxtScheduleView::ViewMode
 * \sa setViewMode()
 */
QxtScheduleView::ViewMode QxtScheduleView::viewMode() const
{
    return (ViewMode)qxt_d().m_currentViewMode;
}

/*!
 *  changes the current Zoom step width
 * Changes the current Zoom step width. Zooming in QxtScheduleView means to change the amount
 * of time one cell holds. For example 5 Minutes. The zoom step width defines how many time
 * is added / removed from the cell when zooming the view.
 *
 * \a int zoomWidth the new zoom step width
 * \a Qxt::Timeunit unit the unit of the new step width (Minutes , Seconds , Hours)
 *
 * \sa zoomIn() zoomOut() setCurrentZoomDepth()
 */
void QxtScheduleView::setZoomStepWidth(const int zoomWidth , const Qxt::Timeunit unit)
{
    switch (unit)
    {
    case Qxt::Second:
    {
        qxt_d().m_zoomStepWidth = zoomWidth;
    }
    break;
    case Qxt::Minute:
    {
        qxt_d().m_zoomStepWidth = zoomWidth * 60;
    }
    break;
    case Qxt::Hour:
    {
        qxt_d().m_zoomStepWidth = zoomWidth * 60 * 60;
    }
    break;
    default:
        qWarning() << "This Timeunit is not implemented yet you can use Second,Minute,Hour using standart 15 minutes";
        qxt_d().m_zoomStepWidth = 900;
        break;
    }
}

/*!
 *  changes the current zoom depth
 * The current zoom depth in QxtScheduleView defines how many time one cell holds in the view.
 * If the new depth does not fit in the view the next possible value is used. If no possible value can be found
 * nothing changes.
 * Normally this is used only to initialize the view, later you want to use zoomIn and zoomOut
 *
 * \a int depth
 * \a Qxt::Timeunit unit
 *
 * \sa zoomIn() zoomOut() setCurrentZoomDepth()
 */
void QxtScheduleView::setCurrentZoomDepth(const int depth , const Qxt::Timeunit unit)
{
    int newZoomDepth = 900;

    //a zoom depth of 0 is invalid
    if (depth == 0)
        return;

    switch (unit)
    {
    case Qxt::Second:
    {
        newZoomDepth = depth;
    }
    break;
    case Qxt::Minute:
    {
        newZoomDepth = depth * 60;
    }
    break;
    case Qxt::Hour:
    {
        newZoomDepth = depth * 60 * 60;
    }
    break;
    default:
        qWarning() << "This Timeunit is not implemented yet you can use Second,Minute,Hour using standart 15 minutes";
        break;
    }

    //now we have to align the currentZoomDepth to the viewMode
    int timePerCol = timePerColumn();

    newZoomDepth = newZoomDepth >  timePerCol ? timePerCol : newZoomDepth;
    newZoomDepth = newZoomDepth <=     0      ?     1      : newZoomDepth;

    while (timePerCol % newZoomDepth)
    {
        if (depth > qxt_d().m_currentZoomDepth)
        {
            newZoomDepth++;
            if (newZoomDepth >= timePerCol)
                return;
        }

        else
        {
            newZoomDepth--;
            if (newZoomDepth <= 1)
                return;
        }
    }

    //qDebug() << "Zoomed, old zoom depth: " << qxt_d().m_currentZoomDepth << " new zoom depth: " << newZoomDepth;

    qxt_d().m_currentZoomDepth = newZoomDepth;
    emit this->newZoomDepth(newZoomDepth);

    /*reinit the view*/
    if (model())
    {
        updateGeometries();
        qxt_d().reloadItemsFromModel();
    }
}

/*!
 *  returns the current zoom depth
 */
int QxtScheduleView::currentZoomDepth(const Qxt::Timeunit unit)
{
    switch (unit)
    {
    case Qxt::Second:
    {
        return qxt_d().m_currentZoomDepth;
    }
    break;
    case Qxt::Minute:
    {
        return qxt_d().m_currentZoomDepth / 60;
    }
    break;
    case Qxt::Hour:
    {
        return qxt_d().m_currentZoomDepth / 60 / 60;
    }
    break;
    default:
        qWarning() << "This Timeunit is not implemented yet you can use Second,Minute,Hour returning seconds";
        return qxt_d().m_currentZoomDepth;
        break;
    }
}

/*!
 *  zooms one step in
 *
 * \sa zoomOut() setCurrentZoomDepth() setZoomStepWidth()
 */
void QxtScheduleView::zoomIn()
{
    setCurrentZoomDepth(qxt_d().m_currentZoomDepth - qxt_d().m_zoomStepWidth);
}

/*!
 *  zooms one step out
 *
 * \sa zoomIn() setCurrentZoomDepth() setZoomStepWidth()
 */
void QxtScheduleView::zoomOut()
{
    setCurrentZoomDepth(qxt_d().m_currentZoomDepth + qxt_d().m_zoomStepWidth);
}

void QxtScheduleView::paintEvent(QPaintEvent * /*event*/)
{
    if (model())
    {
        /*paint the grid*/

        int iNumRows = qxt_d().m_vHeader->count();
        //qDebug() << "Painting rows " << iNumRows;

        int xRowEnd = qxt_d().m_hHeader->sectionViewportPosition(qxt_d().m_hHeader->count() - 1) + qxt_d().m_hHeader->sectionSize(qxt_d().m_hHeader->count() - 1);
        QPainter painter(viewport());


        painter.save();
        painter.setPen(QColor(220, 220, 220));

        bool thinLine;
        thinLine = false;
        for (int iLoop = 0; iLoop < iNumRows; iLoop += 2)
        {
            painter.drawLine(0 , qxt_d().m_vHeader->sectionViewportPosition(iLoop), xRowEnd, qxt_d().m_vHeader->sectionViewportPosition(iLoop));
        }

        int iNumCols = qxt_d().m_hHeader->count();
        int iYColEnd = qxt_d().m_vHeader->sectionViewportPosition(qxt_d().m_vHeader->count() - 1) + qxt_d().m_vHeader->sectionSize(qxt_d().m_vHeader->count() - 1);

        for (int iLoop = 0; iLoop < iNumCols ; iLoop++)
        {
            painter.drawLine(qxt_d().m_hHeader->sectionViewportPosition(iLoop), 0, qxt_d().m_hHeader->sectionViewportPosition(iLoop), iYColEnd);
        }

        painter.restore();

        QListIterator<QxtScheduleInternalItem *> itemIterator(qxt_d().m_Items);
        while (itemIterator.hasNext())
        {
            QxtScheduleInternalItem * currItem = itemIterator.next();
            QxtStyleOptionScheduleViewItem style;

            //\BUG use the correct section here or find a way to forbit section resizing
            style.roundCornersRadius = qxt_d().m_vHeader->sectionSize(1) / 2;
            style.itemHeaderHeight   = qxt_d().m_vHeader->sectionSize(1);
            style.maxSubItemHeight   = qxt_d().m_vHeader->sectionSize(1);

            if (currItem->isDirty)
                currItem->m_cachedParts.clear();

            style.itemGeometries = currItem->m_geometries;
            style.itemPaintCache = &currItem->m_cachedParts;
            style.translate = QPoint(-qxt_d().m_hHeader->offset(), -qxt_d().m_vHeader->offset());
            painter.save();
            qxt_d().delegate->paint(&painter, style, currItem->modelIndex());
            painter.restore();
            currItem->setDirty(false);
        }

        painter.end();
    }
}

void QxtScheduleView::updateGeometries()
{
	//check if we are already initialized
    if(!qxt_d().m_Model || !qxt_d().m_vHeader || !qxt_d().m_hHeader)
        return;
        
    this->setViewportMargins(qxt_d().m_vHeader->sizeHint().width() + 1, qxt_d().m_hHeader->sizeHint().height() + 1, 0, 0);


    verticalScrollBar()->setRange(0, qxt_d().m_vHeader->count()*qxt_d().m_vHeader->defaultSectionSize() - viewport()->height());
    verticalScrollBar()->setSingleStep(qxt_d().m_vHeader->defaultSectionSize());
    verticalScrollBar()->setPageStep(qxt_d().m_vHeader->defaultSectionSize());

    int left = 2;
    int top = qxt_d().m_hHeader->sizeHint().height() + 2;
    int width = qxt_d().m_vHeader->sizeHint().width();
    int height = viewport()->height();
    qxt_d().m_vHeader->setGeometry(left, top, width, height);

    left = left + width;
    top = 1;
    width = viewport()->width();
    height = qxt_d().m_hHeader->sizeHint().height();

    qxt_d().m_hHeader->setGeometry(left, top, width, height);
    qxt_d().m_hHeader->setDefaultSectionSize(viewport()->width() / 7);

    for (int iLoop = 0; iLoop < qxt_d().m_hHeader->count(); iLoop++)
        qxt_d().m_hHeader->resizeSection(iLoop, viewport()->width() / 7);
    qxt_d().m_hHeader->setResizeMode(QHeaderView::Fixed);


    horizontalScrollBar()->setRange(0, (qxt_d().m_hHeader->count() * qxt_d().m_hHeader->defaultSectionSize() - viewport()->width()));
    horizontalScrollBar()->setSingleStep(qxt_d().m_hHeader->defaultSectionSize());
    horizontalScrollBar()->setPageStep(qxt_d().m_hHeader->defaultSectionSize());


    qxt_d().m_vHeader->show();
    qxt_d().m_hHeader->show();
    qxt_d().handleItemConcurrency(0, this->rows() * this->cols() - 1);
    viewport()->update();
}

void QxtScheduleView::scrollContentsBy(int dx, int dy)
{
    qxt_d().m_vHeader->setOffset(qxt_d().m_vHeader->offset() - dy);
    qxt_d().m_hHeader->setOffset(qxt_d().m_hHeader->offset() - dx);
    QAbstractScrollArea::scrollContentsBy(dx, dy);
}

void QxtScheduleView::mouseMoveEvent(QMouseEvent * e)
{
    if (qxt_d().m_selectedItem)
    {
        int currentMousePosTableOffset = qxt_d().pointToOffset((e->pos()));

        if (currentMousePosTableOffset != qxt_d().m_lastMousePosOffset)
        {
            if (currentMousePosTableOffset >= 0)
            {
                /*i cannot use the model data here because all changes are committed to the model only when the move ends*/
                int startTableOffset = qxt_d().m_selectedItem->visualStartTableOffset();
                int endTableOffset = -1;

                /*i simply use the shape to check if we have a move or a resize. Because we enter this codepath the shape gets not changed*/
                if (this->viewport()->cursor().shape() == Qt::SizeVerCursor)
                {
                    QVector<QRect> geo = qxt_d().m_selectedItem->geometry();
                    QRect rect = geo[geo.size()-1];
                    endTableOffset = currentMousePosTableOffset;
                }
                else
                {
                    /*well the duration is the same for a move*/
                    //qint32 difference = qxt_d().rowsTo(qxt_d().m_lastMousePosIndex,currentMousePos);  // tableCellToUnixTime(currentMousePos) -  tableCellToUnixTime(this->m_lastMousePosIndex);
                    int difference = currentMousePosTableOffset - qxt_d().m_lastMousePosOffset;
                    //qDebug()<<"Difference Rows: "<<difference;

                    startTableOffset =  startTableOffset + difference;
                    endTableOffset = startTableOffset + qxt_d().m_selectedItem->rows() - 1;
                }
                if (startTableOffset >= 0 && endTableOffset >= startTableOffset && endTableOffset < (rows()*cols()))
                {
                    QVector< QRect > newGeometry = qxt_d().calculateRangeGeometries(startTableOffset, endTableOffset);

                    int oldStartOffset = qxt_d().m_selectedItem->visualStartTableOffset();
                    int newStartOffset = qxt_d().m_selectedItem->visualEndTableOffset();

                    qxt_d().m_selectedItem->setGeometry(newGeometry);
                    qxt_d().m_selectedItem->setDirty();
                    qxt_d().m_lastMousePosOffset = currentMousePosTableOffset;
#if 1
                    if (newGeometry.size() > 0)
                    {
                        int start = qxt_d().m_selectedItem->visualStartTableOffset();
                        int end   = qxt_d().m_selectedItem->visualEndTableOffset();
                        qxt_d().handleItemConcurrency(oldStartOffset, newStartOffset);
                        qxt_d().handleItemConcurrency(start, end);
                    }
#endif

                }
            }
        }
        return;
    }
    else
    {
        /*change the cursor to show the resize arrow*/
        QPoint translatedPos = mapFromViewport(e->pos());
        QxtScheduleInternalItem * it = qxt_d().internalItemAt(translatedPos);
        if (it)
        {
            QVector<QRect> geo = it->geometry();
            QRect rect = geo[geo.size()-1];
            if (rect.contains(translatedPos) && (translatedPos.y() >= rect.bottom() - 5 &&  translatedPos.y() <= rect.bottom()))
            {
                this->viewport()->setCursor(Qt::SizeVerCursor);
                return;
            }
        }

        if (this->viewport()->cursor().shape() != Qt::ArrowCursor)
            this->viewport()->setCursor(Qt::ArrowCursor);
    }
}

void QxtScheduleView::mouseDoubleClickEvent ( QMouseEvent * e )
{
    qxt_d().m_currentItem  = qxt_d().internalItemAt(mapFromViewport(e->pos()));
    if (qxt_d().m_currentItem)
    {
        emit indexDoubleClicked(qxt_d().m_currentItem->modelIndex());
    }
}

void QxtScheduleView::mousePressEvent(QMouseEvent * e)
{
    qxt_d().m_currentItem  = qxt_d().internalItemAt(mapFromViewport(e->pos()));

    if (qxt_d().m_currentItem)
    {
        emit indexSelected(qxt_d().m_currentItem->modelIndex());
    }
    else
        emit indexSelected(QModelIndex());

    if (e->button() == Qt::RightButton)
    {
        if (qxt_d().m_currentItem)
            emit contextMenuRequested(qxt_d().m_currentItem->modelIndex());
    }
    else
    {
        qxt_d().m_lastMousePosOffset = qxt_d().pointToOffset(e->pos());
        if (qxt_d().m_lastMousePosOffset >= 0)
        {
            qxt_d().m_selectedItem = qxt_d().m_currentItem;


            if (qxt_d().m_selectedItem)
            {
                //qDebug() << "Selected Item:" << qxt_d().m_selectedItem->m_iModelRow;
                raiseItem(qxt_d().m_selectedItem->modelIndex());
                qxt_d().m_selectedItem->startMove();
                qxt_d().scrollTimer.start(100);
            }
            else
                qxt_d().m_lastMousePosOffset = -1;
        }
    }


}

void QxtScheduleView::mouseReleaseEvent(QMouseEvent * /*e*/)
{
    qxt_d().scrollTimer.stop();
    if (qxt_d().m_selectedItem)
    {
        int oldStartTableOffset = qxt_d().m_selectedItem->startTableOffset();
        int oldEndTableOffset = oldStartTableOffset + qxt_d().m_selectedItem->rows() - 1 ;


        QVector<QRect> geo = qxt_d().m_selectedItem->geometry();
        if (geo.size() == 0) return; //XXX crash!
        QRect rect = geo[geo.size()-1];

        int newStartTableOffset = qxt_d().m_selectedItem->visualStartTableOffset();
        int newEndTableOffset   = qxt_d().m_selectedItem->visualEndTableOffset();

        qxt_d().m_selectedItem->stopMove();

        QVariant newStartUnixTime;
        QVariant newDuration;

        newStartUnixTime = qxt_d().offsetToUnixTime(newStartTableOffset);
        model()->setData(qxt_d().m_selectedItem->modelIndex(), newStartUnixTime, Qxt::ItemStartTimeRole);
        newDuration = qxt_d().offsetToUnixTime(newEndTableOffset, true) - newStartUnixTime.toInt();
        model()->setData(qxt_d().m_selectedItem->modelIndex(), newDuration, Qxt::ItemDurationRole);

        qxt_d().m_selectedItem = NULL;
        qxt_d().m_lastMousePosOffset = -1;

        /*only call for the old geometry the dataChanged slot will call it for the new position*/
        qxt_d().handleItemConcurrency(oldStartTableOffset, oldEndTableOffset);
        //qxt_d().handleItemConcurrency(newStartIndex,newEndIndex);

    }
    // this forces a repaint!
    setCurrentZoomDepth(qxt_d().m_currentZoomDepth);
}

void QxtScheduleView::wheelEvent(QWheelEvent  * e)
{
    /*time scrolling when pressing ctrl while using the mouse wheel*/
    if (e->modifiers() & Qt::ControlModifier)
    {
        if (e->delta() < 0)
            zoomOut();
        else
            zoomIn();

    }
    else
        QAbstractScrollArea::wheelEvent(e);
}

/*!
 *  returns the current row count of the view
 */
int QxtScheduleView::rows() const
{
    if (!model())
        return 0;

    int timePerCol = timePerColumn();

    Q_ASSERT(timePerCol % qxt_d().m_currentZoomDepth == 0);
    int iNeededRows = timePerCol / qxt_d().m_currentZoomDepth;

    return iNeededRows;

}

/*!
 *  returns the current column count of the view
 */
int QxtScheduleView::cols() const
{
    if (!model())
        return 0;

    //int cols = 0;
    //int timeToShow = qxt_d().m_endUnixTime - qxt_d().m_startUnixTime + 1 ;
    //int timePerCol = timePerColumn();

    //Q_ASSERT(timeToShow % timePerCol == 0);
    //cols = (timeToShow / timePerCol);

    //return cols;
    return 7;
}

/*!
 * reimplement this to support custom view modes
 *Returns the time per column in seconds
 */
int QxtScheduleView::timePerColumn() const
{
    int timePerColumn = 0;

    switch (qxt_d().m_currentViewMode)
    {
    case DayView:
        timePerColumn = 24 * 60 * 60;
        break;
    case HourView:
        timePerColumn = 60 * 60;
        break;
    case MinuteView:
        timePerColumn = 60;
        break;
    default:
        Q_ASSERT(false);
    }

    return timePerColumn;
}

/*!
 *  reimplement this to support custom view modes
 * This function has to adjust the given start and end time to the current view mode:
 * For example, the DayMode always adjust to time 0:00:00am for startTime and 11:59:59pm for endTime
 */
void QxtScheduleView::adjustRangeToViewMode(QDateTime *startTime, QDateTime *endTime) const
{
    switch (qxt_d().m_currentViewMode)
    {
    case DayView:
        startTime->setTime(QTime(0, 0));
        endTime  ->setTime(QTime(23, 59, 59));
        break;
    case HourView:
        startTime->setTime(QTime(startTime->time().hour(), 0));
        endTime  ->setTime(QTime(endTime->time().hour(), 59, 59));
        break;
    case MinuteView:
        startTime->setTime(QTime(startTime->time().hour(), startTime->time().minute(), 0));
        endTime  ->setTime(QTime(endTime->time().hour(), endTime->time().minute(), 59));
        break;
    default:
        Q_ASSERT(false);
    }
}

QPoint QxtScheduleView::mapFromViewport(const QPoint & point) const
{
    return point + QPoint(qxt_d().m_hHeader->offset(), qxt_d().m_vHeader->offset());
}

QPoint QxtScheduleView::mapToViewport(const QPoint & point) const
{
    return point - QPoint(qxt_d().m_hHeader->offset(), qxt_d().m_vHeader->offset());
}

/*!
 *  raises the item belonging to index
 */
void QxtScheduleView::raiseItem(const QModelIndex &index)
{
    QxtScheduleInternalItem *item = qxt_d().itemForModelIndex(index);
    if (item)
    {
        int iItemIndex  = -1;
        if ((iItemIndex = qxt_d().m_Items.indexOf(item)) >= 0)
        {
            qxt_d().m_Items.takeAt(iItemIndex);
            qxt_d().m_Items.append(item);
            viewport()->update();
        }
    }
}

void QxtScheduleView::dataChanged(const QModelIndex & topLeft, const  QModelIndex & bottomRight)
{
    for (int iLoop = topLeft.row(); iLoop <= bottomRight.row();iLoop++)
    {
        QModelIndex index = model()->index(iLoop, 0);
        QxtScheduleInternalItem * item = qxt_d().itemForModelIndex(index);
        if (item)
        {
            int startOffset = item->startTableOffset();
            int endIndex = item->startTableOffset() + item->rows() - 1;

            if (item->m_geometries.count() > 0)
            {
                int oldStartOffset = qxt_d().pointToOffset(mapToViewport(item->m_geometries[0].topLeft()));
                int oldEndOffset = qxt_d().pointToOffset(mapToViewport(item->m_geometries[item->m_geometries.size()-1].bottomRight()));
                qxt_d().handleItemConcurrency(oldStartOffset, oldEndOffset);
            }

            /*that maybe will set a empty geometry thats okay because the item maybe out of bounds of the view */
            item->setGeometry(qxt_d().calculateRangeGeometries(startOffset, endIndex));
            /*force item cache update even if the geometry is the same*/
            item->setDirty();

            qxt_d().handleItemConcurrency(startOffset, endIndex);


            viewport()->update();
        }
    }
}

/*!
 *  triggers the view to relayout the items that are concurrent to index
 */
void QxtScheduleView::handleItemConcurrency(const QModelIndex &index)
{
    QxtScheduleInternalItem *item = qxt_d().itemForModelIndex(index);
    if (item)
        qxt_d().handleItemConcurrency(item);
}


void QxtScheduleView::resizeEvent(QResizeEvent * /* e*/)
{
    updateGeometries();
}


void QxtScheduleView::rowsRemoved(const QModelIndex & parent, int start, int end)
{
    /*!
     *\FIXME write correct code here
     */
    return qxt_d().reloadItemsFromModel();
    /*for now we care only about toplevel items*/
    if (!parent.isValid())
    {
        for (int iLoop = 0; iLoop < qxt_d().m_Items.count();iLoop++)
        {
            QxtScheduleInternalItem *item = qxt_d().m_Items.at(iLoop);
            if (item)
            {
                if (item->m_iModelRow >= start && item->m_iModelRow <= end)
                {
                    qxt_d().m_Items.takeAt(iLoop);
                    if (item == qxt_d().m_currentItem)
                    {
                        qxt_d().m_currentItem  = 0;
                        emit indexSelected(QModelIndex());
                    }
                    delete item;
                    continue;
                }
                if (item->m_iModelRow > end)
                {
                    int iDifference = end - start + 1;
                    item->m_iModelRow -= iDifference;
                }
            }
        }
    }
}

void QxtScheduleView::rowsInserted(const QModelIndex & parent, int start, int end)
{
    /*for now we care only about toplevel items*/
    if (!parent.isValid())
    {
        for (int iLoop = start; iLoop <= end;iLoop++)
        {
            /*now create the items*/
            QxtScheduleInternalItem *currentItem = new QxtScheduleInternalItem(this, model()->index(iLoop, 0));
            qxt_d().m_Items.append(currentItem);
            connect(currentItem, SIGNAL(geometryChanged(QxtScheduleInternalItem*, QVector<QRect>)), &qxt_d(), SLOT(itemGeometryChanged(QxtScheduleInternalItem * , QVector< QRect >)));
            qxt_d().handleItemConcurrency(currentItem);
        }
    }

    viewport()->update();
}

void QxtScheduleView::rowsAboutToBeRemoved(const QModelIndex & parent, int start, int end)
{
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);
    /*for now we care only about toplevel items*/
    return;
}

void QxtScheduleView::rowsAboutToBeInserted(const QModelIndex & parent, int start, int end)
{
    /*for now we care only about toplevel items*/
    if (!parent.isValid())
    {
        int iDifference = end - start;
        for (int iLoop = 0; iLoop < qxt_d().m_Items.count();iLoop++)
        {
            QxtScheduleInternalItem * item = qxt_d().m_Items[iLoop];
            if (item)
                if (item->m_iModelRow >= start && item->m_iModelRow < model()->rowCount())
                    item->m_iModelRow += iDifference + 1;
        }
    }
}

/*!
 *  returns the current selected index
 */
QModelIndex QxtScheduleView::currentIndex()
{
    QModelIndex currIndex;
    if (qxt_d().m_currentItem)
        currIndex = qxt_d().m_currentItem->modelIndex();
    return currIndex;

}

/*!
 *  sets the timerange
 * This function will set a Timerange from fromDate 00:00am to toDate 23:59pm
 */
void QxtScheduleView::setDateRange(const QDate & fromDate, const QDate & toDate)
{
    Q_UNUSED(fromDate);
    Q_UNUSED(toDate);

    QDateTime startTime = QDateTime(fromDate, QTime(0, 0, 0));
    QDateTime endTime =  QDateTime(toDate, QTime(23, 59, 59));
    setTimeRange(startTime, endTime);
}

/*!
 *  sets the timerange
 * This function will set the passed timerange, but may adjust it to the current viewmode.
 * e.g You cannot start at 1:30am in a DayMode, this gets adjusted to 00:00am
 */
void QxtScheduleView::setTimeRange(const QDateTime & fromDateTime, const QDateTime & toDateTime)
{
    QDateTime startTime = fromDateTime;
    QDateTime endTime = toDateTime;

    //adjust the timeranges to fit in the view
    adjustRangeToViewMode(&startTime, &endTime);
    qxt_d().m_startUnixTime = startTime.toTime_t();
    qxt_d().m_endUnixTime = endTime.toTime_t();
}

QDateTime QxtScheduleView::getStartTime() const
{
    return QDateTime().fromTime_t(qxt_d().m_startUnixTime);
}


