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

#include <QtGui>
#include "GcWindowLayout.h"

#include <stdio.h>

GcWindowLayout::GcWindowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

GcWindowLayout::GcWindowLayout(int margin, int hSpacing, int vSpacing)
    : m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

GcWindowLayout::~GcWindowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}

void GcWindowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}

void GcWindowLayout::insert(int index, QWidget *widget)
{
    addChildWidget(widget);
    itemList.insert(index, new QWidgetItem(widget));
}

int GcWindowLayout::horizontalSpacing() const
{
    if (m_hSpace >= 0) {
        return m_hSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
    }
}

int GcWindowLayout::verticalSpacing() const
{
    if (m_vSpace >= 0) {
        return m_vSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
    }
}

int GcWindowLayout::count() const
{
    return itemList.size();
}

QLayoutItem *GcWindowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

QLayoutItem *GcWindowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    else
        return 0;
}

Qt::Orientations GcWindowLayout::expandingDirections() const
{
    return 0;
}

bool GcWindowLayout::hasHeightForWidth() const
{
    return true;
}

int GcWindowLayout::heightForWidth(int width) const
{
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}

void GcWindowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize GcWindowLayout::sizeHint() const
{
    return minimumSize();
}

QSize GcWindowLayout::minimumSize() const
{
    QSize size;
    QLayoutItem *item;
    foreach (item, itemList)
        size = size.expandedTo(item->minimumSize());

    size += QSize(2*margin(), 2*margin());
    return size;
}

// we sort the points to try from top to bottom
// and then left to right since this is the sequence
// we follow to layout the charts
class QPointS : public QPoint
{
    public:
        QPointS(int x, int y) : QPoint(x,y) {}

        bool operator< (QPointS right) const { 
            if (y() == right.y())
                return x() < right.x();
            else
                return y() < right.y();
        }
};

int GcWindowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    // geometry for the layout
    int left, top, right, bottom;
    int spaceX = horizontalSpacing();
    int spaceY = verticalSpacing();
    getContentsMargins(&left, &top, &right, &bottom);

    // the space we have to line up our charts
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);

    // Charts already positioned are held here
    QList <QRect> placed; 

    // iterate through each chart, placing it in the
    // most optimal location top to bottom then
    // left to right (packed flow)
    foreach (QLayoutItem *item, itemList) {

        // what widget is in this item
        QWidget *wid = item->widget();

        // handle when no spacing is set (always is for HomeWindow layouts)
        if (spaceX == -1)
            spaceX = wid->style()->layoutSpacing(QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
        if (spaceY == -1)
            spaceY = wid->style()->layoutSpacing( QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);


        // the location we are going to place
        int x=-1, y=-1;

        if (placed.count() == 0) { // is first item!

            x = effectiveRect.x();
            y = effectiveRect.y();

        } else {

            // get a list of all the locations that the chart could be placed at
            QList<QPointS> attempts;

            // iterate through the charts and create a list of placement
            // points that we can attempt to put the current chart
            foreach(QRect here, placed) {

                // always try to the right of another chart
                attempts.append(QPointS(here.x()+here.width()+spaceX, here.y()));

                int ax = here.x();
                foreach(QRect there, placed)
                    attempts.append(QPointS(ax, there.y() + there.height() + spaceY));
            }

            // now sort the list in order of preference
            qSort(attempts);

            // see if it fits!
            foreach(QPointS attempt, attempts) {

                QRect mine(attempt.x(),
                           attempt.y(),
                           item->sizeHint().width()+spaceX, 
                           item->sizeHint().height()+spaceY);

                // doesn't even fit in the boundary
                if (mine.x()+mine.width()-spaceX > effectiveRect.right()) continue;


                // lets assume it will fit and check  if it overlaps with existing
                bool fits = true;
                foreach (QRect here, placed) {

                    // add spacing
                    QRect bigger(here.x(), here.y(), here.width()+spaceY, here.height()+spaceX);

                    // overlaps?
                    if (bigger.intersects(mine)) {
                        fits = false;
                        break;
                    }
                }

                // it fits!
                if (fits == true) {
                    x = attempt.x();
                    y = attempt.y();
                    break;
                }
            }
        }

        // its just too big ...  fnarr fnarr
        // stick it at the bottom
        // this can happen when the layout is being called
        // to set geometry when width and height are being
        // derived
        if ((x < 0 || y < 0) && placed.count()) {
            int maxy=0;
            foreach(QRect geom, placed) {
                if (geom.y()+geom.height()+spaceY > maxy) {
                    maxy = geom.y()+geom.height()+spaceY;
                }
            }
            x = effectiveRect.x();
            y = maxy;
        }

        QRect placement(QPoint(x,y), item->sizeHint());
        placed.append(placement);

        if (!testOnly) item->setGeometry(placement);

    }

    // Return the length of the layout
    int maxy=0;
    foreach(QRect geom, placed) {
        if (geom.y()+geom.height()+spaceY > maxy) {
            maxy = geom.y()+geom.height()+spaceY;
        }
    }
    return maxy;
}

int GcWindowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject *parent = this->parent();
    if (!parent) {
        return -1;
    } else if (parent->isWidgetType()) {
        QWidget *pw = static_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, 0, pw);
    } else {
        return static_cast<QLayout *>(parent)->spacing();
    }
}
