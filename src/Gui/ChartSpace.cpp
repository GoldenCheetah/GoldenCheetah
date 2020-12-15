/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#include "ChartSpace.h"

#include "TabView.h"
#include "Athlete.h"
#include "RideCache.h"

#include <cmath>
#include <QGraphicsSceneMouseEvent>
#include <QGLWidget>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include "QtAwesome.h"


double gl_major;
ChartSpaceItemRegistry *ChartSpaceItemRegistry::_instance;
QtAwesome* awesome;

ChartSpace::ChartSpace(Context *context, int scope) :
    state(NONE), context(context), scope(scope), group(NULL), fixedZoom(0), _viewY(0),
    yresizecursor(false), xresizecursor(false), block(false), scrolling(false),
    setscrollbar(false), lasty(-1)
{
    setContentsMargins(0,0,0,0);

    QHBoxLayout *main = new QHBoxLayout;
    main->setSpacing(0);
    main->setContentsMargins(0,0,0,0);

    // add a view and scene and centre
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(this);

#ifdef Q_OS_LINUX
    // if we have OpenGL and its 2.0 or higher, lets use it.
    // this is pretty much any GPU since 2004 and keeps Qt happy.
    // we only do this on Linux since Windows and MacOS have issues
    // on Windows AMD Radeon drivers are dreadful and on MacOS
    // OpenGL support is deprecated and users have reported rendering
    // issues when this is enabled.
    if (gl_major >= 2.0)  view->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers | QGL::DirectRendering)));
#endif

    view->viewport()->setAttribute(Qt::WA_AcceptTouchEvents, false); // stops it stealing focus on mouseover
    scrollbar = new QScrollBar(Qt::Vertical, this);

    // how to move etc
    //view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setFrameStyle(QFrame::NoFrame);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setScene(scene);

    // layout
    main->addWidget(view);
    main->addWidget(scrollbar);

    // all the widgets
    setLayout(main);

    // by default these are the column sizes (user can adjust)
    columns << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200;

    // for changing the view
    group = new QParallelAnimationGroup(this);
    viewchange = new QPropertyAnimation(this, "viewRect");
    viewchange->setEasingCurve(QEasingCurve(QEasingCurve::OutQuint));

    // for scrolling the view
    scroller = new QPropertyAnimation(this, "viewY");
    scroller->setEasingCurve(QEasingCurve(QEasingCurve::Linear));

    // watch the view for mouse events
    view->setMouseTracking(true);
    scene->installEventFilter(this);

    // once all widgets created we can connect the signals
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(scroller, SIGNAL(finished()), this, SLOT(scrollFinished()));
    connect(scrollbar, SIGNAL(valueChanged(int)), this, SLOT(scrollbarMoved(int)));

    // set the widgets etc
    configChanged(CONFIG_APPEARANCE);

    // we're ready to plot, but not configured
    configured=false;
    stale=true;
    currentRideItem=NULL;

    awesome = new QtAwesome( qApp );
    awesome->initFontAwesome();
}

// add the item
void
ChartSpace::addItem(int order, int column, int deep, ChartSpaceItem *item)
{
    item->order= order;
    item->column = column;
    item->deep = deep;
    items.append(item);
    if (scope&OverviewScope::ANALYSIS && currentRideItem) item->setData(currentRideItem);
    if (scope&OverviewScope::TRENDS) item->setDateRange(currentDateRange);
}

void
ChartSpace::removeItem(ChartSpaceItem *item)
{
    for(int i=0; i<items.count(); i++) {
        ChartSpaceItem *p = items.at(i);
        if (p == item) {
            scene->removeItem(p);
            items.removeAt(i);
            delete p;
            return;
        }
    }
}

// when a ride is selected we need to notify all the ChartSpaceItems
void
ChartSpace::rideSelected(RideItem *item)
{
    // don't plot when we're not visible, unless we have nothing plotted yet
    if (!isVisible() && currentRideItem != NULL && item != NULL) {
        stale=true;
        return;
    }

    // don't replot .. we already did this one
    if (currentRideItem == item && stale == false) {
        return;
    }

    // ride item changed
    foreach(ChartSpaceItem *ChartSpaceItem, items) ChartSpaceItem->setData(item);

    // update
    updateView();

    // ok, remember we did this one
    currentRideItem = item;
    stale=false;
}

void
ChartSpace::refresh()
{
    stale = true;
    if (scope == TRENDS) dateRangeChanged(currentDateRange);
    else if (scope == ANALYSIS) rideSelected(currentRideItem);
}

void
ChartSpace::filterChanged()
{
    // redo trends
    dateRangeChanged(currentDateRange);
}

void
ChartSpace::dateRangeChanged(DateRange dr)
{
    // remember
    currentDateRange = dr;

    // don't plot when we're not visible, unless we have nothing plotted yet
    if (!isVisible()) {
        stale=true;
        return;
    }

    // ride item changed
    foreach(ChartSpaceItem *ChartSpaceItem, items) ChartSpaceItem->setDateRange(dr);

    // update
    updateView();

    // ok, remember we did this one
    stale=false;
}

void
ChartSpaceItem::setData(RideItem *item)
{
    Q_UNUSED(item);
    // ignored
}

void
ChartSpaceItem::setDrag(bool x)
{
    if (drag == x) return; // unchanged

    drag = x;

    // hide stuff
    dragChanged(drag);
    if (!drag) geometryChanged();
}

void
ChartSpaceItem::geometryChanged()
{
    itemGeometryChanged();
}

bool
ChartSpaceItem::sceneEvent(QEvent *event)
{
    // skip whilst dragging and resizing
    if (parent->state != ChartSpace::NONE) return false;

    // repaint when mouse enters and leaves
    if (event->type() == QEvent::GraphicsSceneHoverLeave ||
        event->type() == QEvent::GraphicsSceneHoverEnter) {

        // force repaint
        update();
        scene()->update();

    // repaint when in the corner
    } else if (event->type() == QEvent::GraphicsSceneHoverMove && inCorner() != incorner) {

        incorner = inCorner();
        update();
        scene()->update();
    }
    return false;
}

bool
ChartSpaceItem::inHotspot()
{
    if (showconfig == false) return false;

    QPoint vpos = parent->view->mapFromGlobal(QCursor::pos());
    QPointF spos = parent->view->mapToScene(vpos);

    QRectF spot(hotspot().topLeft()+geometry().topLeft(), hotspot().bottomRight()+geometry().topLeft());
    if (spot.width() >0 && spot.contains(spos.x(), spos.y()))  return true;
    return false;
}

bool
ChartSpaceItem::inCorner()
{
    if (showconfig == false) return false;

    QPoint vpos = parent->view->mapFromGlobal(QCursor::pos());
    QPointF spos = parent->view->mapToScene(vpos);

    if (geometry().contains(spos.x(), spos.y())) {
        if (spos.y() - geometry().top() < (ROWHEIGHT+40) &&
            geometry().width() - (spos.x() - geometry().x()) < (ROWHEIGHT+40))
            return true;
    }
    return false;

}

bool
ChartSpaceItem::underMouse()
{
    QPoint vpos = parent->view->mapFromGlobal(QCursor::pos());
    QPointF spos = parent->view->mapToScene(vpos);

    if (geometry().contains(spos.x(), spos.y())) return true;
    return false;
}

// ChartSpaceItems need to show they are in config mode
void
ChartSpaceItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *opt, QWidget *widget) {

    if (drag) painter->setBrush(QBrush(GColor(CPLOTMARKER)));
    else painter->setBrush(GColor(CCARDBACKGROUND));

    QPainterPath path;
    path.addRoundedRect(QRectF(0,0,geometry().width(),geometry().height()), ROWHEIGHT/5, ROWHEIGHT/5);
    painter->setPen(Qt::NoPen);
    //painter->fillPath(path, brush.color());
    painter->drawPath(path);
    painter->setPen(GColor(CPLOTGRID));
    //XXXpainter->drawLine(QLineF(0,ROWHEIGHT*2,geometry().width(),ROWHEIGHT*2));
    //painter->fillRect(QRectF(0,0,geometry().width()+1,geometry().height()+1), brush);
    //titlefont.setWeight(QFont::Bold);
    if (GCColor::luminance(GColor(CCARDBACKGROUND)) < 127) painter->setPen(QColor(200,200,200));
    else painter->setPen(QColor(70,70,70));

    painter->setFont(parent->titlefont);
    painter->drawText(QPointF(ROWHEIGHT /2.0f, QFontMetrics(parent->titlefont, parent->device()).height()), name);

    // only paint contents if not dragging
    if (drag) return;

    // not dragging so we can get to work painting the rest

    // config icon
    if (showconfig) {
        if (parent->state != ChartSpace::DRAG && underMouse()) {

            if (inCorner()) {

                // if hovering over the button show a background to indicate
                // that pressing a button is good
                QPainterPath path;
                path.addRoundedRect(QRectF(geometry().width()-40-ROWHEIGHT,0,
                                    ROWHEIGHT+40, ROWHEIGHT+40), ROWHEIGHT/5, ROWHEIGHT/5);
                painter->setPen(Qt::NoPen);
                QColor darkgray(GColor(CCARDBACKGROUND).lighter(200));
                painter->setBrush(darkgray);
                painter->drawPath(path);
                painter->fillRect(QRectF(geometry().width()-40-ROWHEIGHT, 0, ROWHEIGHT+40-(ROWHEIGHT/5), ROWHEIGHT+40), QBrush(darkgray));
                painter->fillRect(QRectF(geometry().width()-40-ROWHEIGHT, ROWHEIGHT/5, ROWHEIGHT+40, ROWHEIGHT+40-(ROWHEIGHT/5)), QBrush(darkgray));

                // draw the config button and make it more obvious
                // when hovering over the card
                int drawSize = ROWHEIGHT*1;
                QChar chr = QChar( static_cast<int>(fa::cog) );
                painter->setFont( awesome->font( drawSize ) );
                painter->setPen( QColor(150,150,150) );
                painter->drawText( QRect( QPoint(geometry().width()-20-(ROWHEIGHT*1), 20), QSize(ROWHEIGHT*1, ROWHEIGHT*1) ), chr , QTextOption( Qt::AlignCenter|Qt::AlignVCenter ) );
            } else {
                // hover on card - make it more obvious there is a config button
                int drawSize = ROWHEIGHT*1;
                QChar chr = QChar( static_cast<int>(fa::cog) );
                painter->setFont( awesome->font( drawSize ) );
                painter->setPen( QColor(100,100,100) );
                painter->drawText( QRect( QPoint(geometry().width()-20-(ROWHEIGHT*1), 20), QSize(ROWHEIGHT*1, ROWHEIGHT*1) ), chr , QTextOption( Qt::AlignCenter|Qt::AlignVCenter ) );
            }
        } else {
            int drawSize = ROWHEIGHT*1;
            QChar chr = QChar( static_cast<int>(fa::cog) );
            painter->setFont( awesome->font( drawSize ) );
            painter->setPen( GColor(COVERVIEWBACKGROUND).lighter(75) );
            painter->drawText( QRect( QPoint(geometry().width()-20-(ROWHEIGHT*1), 20), QSize(ROWHEIGHT*1, ROWHEIGHT*1) ), chr , QTextOption( Qt::AlignCenter|Qt::AlignVCenter ) );
        }
    }

    // thin border
    if (!drag) {
        QPainterPath path;
        path.addRoundedRect(QRectF(1*dpiXFactor,1*dpiXFactor,geometry().width()-(2*dpiXFactor),geometry().height()-(2*dpiXFactor)), ROWHEIGHT/5, ROWHEIGHT/5);
        QColor edge(GColor(CCARDBACKGROUND));
        edge = edge.darker(105);
        QPen pen(edge);
        pen.setWidth(3*dpiXFactor);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path);
    }

    // item paints itself and can completely repaint everything
    // including the title etc, in case this is useful
    itemPaint(painter, opt, widget);

}

static bool ChartSpaceItemSort(const ChartSpaceItem* left, const ChartSpaceItem* right)
{
    return (left->column < right->column ? true : (left->column == right->column && left->order < right->order ? true : false));
}

void
ChartSpace::updateGeometry()
{
    // can't update geom if nothing to see.
    if (items.count() == 0) return;

    bool animated=false;

    // prevent a memory leak
    group->stop();
    delete group;
    group = new QParallelAnimationGroup(this);

    // order the items to their positions
    std::sort(items.begin(), items.end(), ChartSpaceItemSort);

    int y=SPACING;
    int maxy = y;
    int column=-1;

    int x=SPACING;

    // just set their geometry for now, no interaction
    for(int i=0; i<items.count(); i++) {

        // don't show hidden
        if (!items[i]->isVisible()) continue;

        // move on to next column, check if first item too
        if (items[i]->column > column) {

            // once past the first column we need to update x
            if (column >= 0) x+= columns[column] + SPACING;

            int diff = items[i]->column - column - 1;
            if (diff > 0) {

                // there are empty columns so shift the cols to the right
                // to the left to fill  the gap left and all  the column
                // widths also need to move down too
                for(int j=items[i]->column-1; j < 8; j++) columns[j]=columns[j+1];
                for(int j=i; j<items.count();j++) items[j]->column -= diff;
            }
            y=SPACING; column = items[i]->column;

        }

        // set geometry
        int ty = y;
        int tx = x;
        int twidth = columns[column];
        int theight = items[i]->deep * ROWHEIGHT;

        // make em smaller when configuring visual cue stolen from Windows Start Menu
        int add = 0; //XXX PERFORMANCE ISSSE XXX (state == DRAG) ? (ROWHEIGHT/2) : 0;


        // for setting the scene rectangle - but ignore a ChartSpaceItem if we are dragging it
        if (maxy < ty+theight+SPACING) maxy = ty+theight+SPACING;

        // add to scene if new
        if (!items[i]->onscene) {
            scene->addItem(items[i]);
            items[i]->setGeometry(tx, ty, twidth, theight);
            items[i]->onscene = true;

        } else if (items[i]->invisible == false &&
                   (items[i]->geometry().x() != tx+add ||
                    items[i]->geometry().y() != ty+add ||
                    items[i]->geometry().width() != twidth-(add*2) ||
                    items[i]->geometry().height() != theight-(add*2))) {

            // we've got an animation to perform
            animated = true;

            // add an animation for this movement
            QPropertyAnimation *animation = new QPropertyAnimation(items[i], "geometry");
            animation->setDuration(300);
            animation->setStartValue(items[i]->geometry());
            animation->setEndValue(QRect(tx+add,ty+add,twidth-(add*2),theight-(add*2)));

            // when placing a little feedback helps
            if (items[i]->placing) {
                animation->setEasingCurve(QEasingCurve(QEasingCurve::OutBack));
                items[i]->placing = false;
            } else animation->setEasingCurve(QEasingCurve(QEasingCurve::OutQuint));

            group->addAnimation(animation);
        }

        // set spot for next tile
        y += theight + SPACING;
    }

    // set the scene rectangle, columns start at 0
    sceneRect = QRectF(0, 0, columns[column] + x + SPACING, maxy);

    if (animated) group->start();
}

void
ChartSpace::configChanged(qint32)
{
    // set fonts
    bigfont.setPixelSize(pixelSizeForFont(bigfont, ROWHEIGHT *2.5f));
    titlefont.setPixelSize(pixelSizeForFont(titlefont, ROWHEIGHT)); // need a bit of space
    midfont.setPixelSize(pixelSizeForFont(midfont, ROWHEIGHT *0.8f));
    smallfont.setPixelSize(pixelSizeForFont(smallfont, ROWHEIGHT*0.7f));
    tinyfont.setPixelSize(pixelSizeForFont(smallfont, ROWHEIGHT*0.5f));

    setProperty("color", GColor(COVERVIEWBACKGROUND));
    view->setBackgroundBrush(QBrush(GColor(COVERVIEWBACKGROUND)));
    scene->setBackgroundBrush(QBrush(GColor(COVERVIEWBACKGROUND)));
    scrollbar->setStyleSheet(TabView::ourStyleSheet());

    // text edit colors
    QPalette palette;
    palette.setColor(QPalette::Window, GColor(COVERVIEWBACKGROUND));
    palette.setColor(QPalette::Background, GColor(COVERVIEWBACKGROUND));

    // only change base if moved away from white plots
    // which is a Mac thing
#ifndef Q_OS_MAC
    if (GColor(COVERVIEWBACKGROUND) != Qt::white)
#endif
    {
        //palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CTRAINPLOTBACKGROUND)));
        palette.setColor(QPalette::Base, GColor(COVERVIEWBACKGROUND));
        palette.setColor(QPalette::Window, GColor(COVERVIEWBACKGROUND));
    }

#ifndef Q_OS_MAC // the scrollers appear when needed on Mac, we'll keep that
    //code->setStyleSheet(TabView::ourStyleSheet());
#endif

    palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(COVERVIEWBACKGROUND)));
    palette.setColor(QPalette::Text, GCColor::invertColor(GColor(COVERVIEWBACKGROUND)));
    //code->setPalette(palette);
    repaint();
}

void
ChartSpace::setFixedZoom(int zoom)
{
    fixedZoom=zoom;
    updateView();
}

void
ChartSpace::updateView()
{
    scene->setSceneRect(sceneRect);
    scene->update();

    if (items.count() == 0) {
        scrollbar->setEnabled(false);
        return;
    }

    // don'r scale whilst resizing on x?
    if (scrolling || (state != YRESIZE && state != XRESIZE && state != DRAG)) {

        // much of a resize / change ?
        double dx = fabs(viewRect.x() - sceneRect.x());
        double dy = fabs(viewRect.y() - sceneRect.y());
        double vy = fabs(viewRect.y()-double(_viewY));
        double dwidth = fabs(viewRect.width() - sceneRect.width());
        double dheight = fabs(viewRect.height() - sceneRect.height());

        // scale immediately if not a bit change
        // otherwise it feels unresponsive
        if (viewRect.width() == 0 || (vy < 20 && dx < 20 && dy < 20 && dwidth < 20 && dheight < 20)) {
            setViewRect(sceneRect);
        } else {

            // tempting to make this longer but feels ponderous at longer durations
            viewchange->setDuration(400);
            viewchange->setStartValue(viewRect);
            viewchange->setEndValue(sceneRect);
            viewchange->start();
        }
    }

    if (view->sceneRect().height() >= scene->sceneRect().height()) {
        scrollbar->setEnabled(false);
    } else {

        // now set scrollbar
        setscrollbar = true;
        scrollbar->setMinimum(0);
        scrollbar->setMaximum(scene->sceneRect().height()-view->sceneRect().height());
        scrollbar->setValue(_viewY);
        scrollbar->setPageStep(view->sceneRect().height());
        scrollbar->setEnabled(true);
        setscrollbar = false;
    }
}

void
ChartSpace::edgeScroll(bool down)
{
    // already scrolling, so don't move
    if (scrolling) return;
    // we basically scroll the view if the cursor is at or above
    // the top of the view, or at or below the bottom and the mouse
    // is moving away. Needs to work in normal and full screen.
    if (state == DRAG || state == YRESIZE) {

        QPointF pos =this->mapFromGlobal(QCursor::pos());

        if (!down && pos.y() <= 0) {

            // at the top of the screen, go up a qtr of a screen
            scrollTo(_viewY - (view->sceneRect().height()/4));

        } else if (down && (geometry().height()-pos.y()) <= 0) {

            // at the bottom of the screen, go down a qtr of a screen
            scrollTo(_viewY + (view->sceneRect().height()/4));

        }
    }
}

void
ChartSpace::scrollTo(int newY)
{

    // bound the target to the top or a screenful from the bottom, except when we're
    // resizing on Y as we are expanding the scene by increasing the size of an object
    if ((state != YRESIZE) && (newY +view->sceneRect().height()) > sceneRect.bottom())
        newY = sceneRect.bottom() - view->sceneRect().height();
    if (newY < 0)
        newY = 0;

    if (_viewY != newY) {

        if (abs(_viewY - newY) < 20) {

            // for small scroll increments just do it, its tedious to wait for animations
            _viewY = newY;
            updateView();

        } else {

            // disable other view updates whilst scrolling
            scrolling = true;

            // make it snappy for short distances - ponderous for drag scroll
            // and vaguely snappy for page by page scrolling
            if (state == DRAG || state == YRESIZE) scroller->setDuration(300);
            else if (abs(_viewY-newY) < 100) scroller->setDuration(150);
            else scroller->setDuration(250);
            scroller->setStartValue(_viewY);
            scroller->setEndValue(newY);
            scroller->start();
        }
    }
}

void
ChartSpace::setViewRect(QRectF rect)
{
    viewRect = rect;
    if (fixedZoom) viewRect.setWidth(fixedZoom);

    // fit to scene width XXX need to fix scrollbars.
    double scale = view->frameGeometry().width() / viewRect.width();
    QRectF scaledRect(0,_viewY, fixedZoom ? fixedZoom :viewRect.width(), view->frameGeometry().height() / scale);

    // fprintf(stderr, "setViewRect: scale=%f, scaledRect=%f,%f width=%f height=%f\n", scale, scaledRect.x(), scaledRect.y(), scaledRect.width(), scaledRect.height()); fflush(stderr);

    // scale to selection
    view->scale(scale,scale);
    view->setSceneRect(scaledRect);
    view->fitInView(scaledRect, Qt::KeepAspectRatio);

    // if we're dragging, as the view changes it can be really jarring
    // as the dragged item is not under the mouse then snaps back
    // this might need to be cleaned up as a little too much of spooky
    // action at a distance going on here !
    if (state == DRAG) {

        // update drag point
        QPoint vpos = view->mapFromGlobal(QCursor::pos());
        QPointF pos = view->mapToScene(vpos);

        // move the ChartSpaceItem being dragged
        stateData.drag.item->setPos(pos.x()-stateData.drag.offx, pos.y()-stateData.drag.offy);
    }

    view->update();

}

bool
ChartSpace::eventFilter(QObject *, QEvent *event)
{
    if (block || (event->type() != QEvent::KeyPress && event->type() != QEvent::GraphicsSceneWheel &&
                  event->type() != QEvent::GraphicsSceneMousePress && event->type() != QEvent::GraphicsSceneMouseRelease &&
                  event->type() != QEvent::GraphicsSceneMouseMove)) {
        return false;
    }
    block = true;
    bool returning = false;

    // we only filter out keyboard shortcuts for undo redo etc
    // in the qwkcode editor, anything else is of no interest.
    if (event->type() == QEvent::KeyPress) {

        // we care about cmd / ctrl
        Qt::KeyboardModifiers kmod = static_cast<QInputEvent*>(event)->modifiers();
        bool ctrl = (kmod & Qt::ControlModifier) != 0;

        switch(static_cast<QKeyEvent*>(event)->key()) {

        case Qt::Key_Y:
            if (ctrl) {
                //workout->redo();
                returning = true; // we grab all key events
            }
            break;

        case Qt::Key_Z:
            if (ctrl) {
                //workout->undo();
                returning=true;
            }
            break;

        case Qt::Key_Home:
            scrollTo(0);
            break;

        case Qt::Key_End:
            scrollTo(scene->sceneRect().bottom());
            break;

        case Qt::Key_PageDown:
            scrollTo(_viewY + view->sceneRect().height());
            break;

        case Qt::Key_PageUp:
            scrollTo(_viewY - view->sceneRect().height());
            break;

        case Qt::Key_Down:
            scrollTo(_viewY + ROWHEIGHT);
            break;

        case Qt::Key_Up:
            scrollTo(_viewY - ROWHEIGHT);
            break;
        }

    } else  if (event->type() == QEvent::GraphicsSceneWheel) {

        // take it as applied
        QGraphicsSceneWheelEvent *w = static_cast<QGraphicsSceneWheelEvent*>(event);
        scrollTo(_viewY - (w->delta()*2));
        event->accept();
        returning = true;

    } else  if (event->type() == QEvent::GraphicsSceneMousePress) {

        // we will process clicks when configuring so long as we're
        // not in the middle of something else - this is to start
        // dragging a ChartSpaceItem around
        if (state == NONE) {

            // where am i ?
            QPointF pos = static_cast<QGraphicsSceneMouseEvent*>(event)->scenePos();
            QGraphicsItem *gritem = scene->itemAt(pos, view->transform());
            ChartSpaceItem *item = static_cast<ChartSpaceItem*>(gritem);

            // ignore other scene elements (e.g. charts)
            if (!items.contains(item)) item=NULL;

            // the item may have a hotspot it wants us to honour
            if (item && item->inHotspot()) item=NULL;

            // trigger config. so drop out completely as
            // we may end up deleting the item etc
            // so we explicity unblock too
            if (item && item->inCorner()) {

                block = false; // reeentry is allowed
                emit itemConfigRequested(item);
                return true;
            }

            // only respond to clicks not in config corner button
            if (item && ! item->inCorner()) {

               // are we on the boundary of the ChartSpaceItem?
               double offx = pos.x()-item->geometry().x();
               double offy = pos.y()-item->geometry().y();


               if (item->geometry().height()-offy < 10) {

                    state = YRESIZE;

                    stateData.yresize.item = item;
                    stateData.yresize.deep = item->deep;
                    stateData.yresize.posy = pos.y();

                    // thanks we'll take that
                    event->accept();
                    returning = true;

               } else if (item->geometry().width()-offx < 10) {

                    state = XRESIZE;

                    stateData.xresize.column = item->column;
                    stateData.xresize.width = columns[item->column];
                    stateData.xresize.posx = pos.x();

                    // thanks we'll take that
                    event->accept();
                    returning = true;

               } else {

                    // we're grabbing a ChartSpaceItem, so lets
                    // work out the offset so we can move
                    // it around when we start dragging
                    state = DRAG;
                    item->invisible = true;
                    item->setDrag(true);
                    item->setZValue(100);

                    stateData.drag.item = item;
                    stateData.drag.offx = offx;
                    stateData.drag.offy = offy;
                    stateData.drag.width = columns[item->column];

                    // thanks we'll take that
                    event->accept();
                    returning = true;

                    // what is the offset?
                    //updateGeometry();
                    scene->update();
                    view->update();
                }
            }
        }

    } else  if (event->type() == QEvent::GraphicsSceneMouseRelease) {

        // stop dragging
        if (state == DRAG || state == YRESIZE || state == XRESIZE) {

            // we want this one
            event->accept();
            returning = true;

            // set back to visible if dragging
            if (state == DRAG) {
                stateData.drag.item->invisible = false;
                stateData.drag.item->setZValue(10);
                stateData.drag.item->placing = true;
                stateData.drag.item->setDrag(false);
            }

            // end state;
            state = NONE;

            // drop it down
            updateGeometry();
            updateView();
        }

    } else if (event->type() == QEvent::GraphicsSceneMouseMove) {

        // where is the mouse now?
        QPointF pos = static_cast<QGraphicsSceneMouseEvent*>(event)->scenePos();

        // check for autoscrolling at edges
        if (state == DRAG || state == YRESIZE) edgeScroll(lasty < pos.y());

        // remember pos
        lasty = pos.y();

        if (state == NONE) {                 // hovering

            // where am i ?
            QGraphicsItem *gritem = scene->itemAt(pos, view->transform());
            ChartSpaceItem *item = static_cast<ChartSpaceItem*>(gritem);

            // ignore other scene elements (e.g. charts)
            if (!items.contains(item)) item=NULL;

            if (item) {

                // are we on the boundary of the ChartSpaceItem?
                double offx = pos.x()-item->geometry().x();
                double offy = pos.y()-item->geometry().y();

                if (yresizecursor == false && item->geometry().height()-offy < 10) {

                    yresizecursor = true;
                    setCursor(QCursor(Qt::SizeVerCursor));

                } else if (yresizecursor == true && item->geometry().height()-offy > 10) {

                    yresizecursor = false;
                    setCursor(QCursor(Qt::ArrowCursor));

                }

                if (xresizecursor == false && item->geometry().width()-offx < 10) {

                    xresizecursor = true;
                    setCursor(QCursor(Qt::SizeHorCursor));

                } else if (xresizecursor == true && item->geometry().width()-offx > 10) {

                    xresizecursor = false;
                    setCursor(QCursor(Qt::ArrowCursor));

                }

            } else {

                // not hovering over tile, so if still have a resize cursor
                // set it back to the normal arrow pointer
                if (yresizecursor || xresizecursor || cursor().shape() != Qt::ArrowCursor) {
                    xresizecursor = yresizecursor = false;
                    setCursor(QCursor(Qt::ArrowCursor));
                }
            }

        } else if (state == DRAG && !scrolling) {          // dragging?

            // whilst mouse moves, only update geom when changed
            bool changed = false;

            // move the ChartSpaceItem being dragged
            stateData.drag.item->setPos(pos.x()-stateData.drag.offx, pos.y()-stateData.drag.offy);

            // should I move?
            QList<QGraphicsItem *> overlaps;
            foreach(QGraphicsItem *p, scene->items(pos))
                if(items.contains(static_cast<ChartSpaceItem*>(p)))
                    overlaps << p;

            // we always overlap with ourself, so see if more
            if (overlaps.count() > 1) {

                ChartSpaceItem *over = static_cast<ChartSpaceItem*>(overlaps[1]);
                if (pos.y()-over->geometry().y() > over->geometry().height()/2) {

                    // place below the one its over
                    stateData.drag.item->column = over->column;
                    stateData.drag.item->order = over->order+1;
                    for(int i=items.indexOf(over); i< items.count(); i++) {
                        if (i>=0 && items[i]->column == over->column && items[i]->order > over->order && items[i] != stateData.drag.item) {
                            items[i]->order += 1;
                            changed = true;
                        }
                    }

                } else {

                    // place above the one its over
                    stateData.drag.item->column = over->column;
                    stateData.drag.item->order = over->order;
                    for(int i=0; i< items.count(); i++) {
                        if (i>=0 && items[i]->column == over->column && items[i]->order >= (over->order) && items[i] != stateData.drag.item) {
                            items[i]->order += 1;
                            changed = true;
                        }
                    }
                }

            } else {

                // columns are now variable width
                // create a new column to the right?
                int x=SPACING;
                int targetcol = -1;
                for(int i=0; i<10; i++) {
                    if (pos.x() > x && pos.x() < (x+columns[i]+SPACING)) {
                        targetcol = i;
                        break;
                    }
                    x += columns[i]+SPACING;
                }

                if (items.last()->column < 9 && targetcol < 0) {

                    // don't keep moving - if we're already alone in column 0 then no move is needed
                    if (stateData.drag.item->column != 0 || (items.count()>1 && items[1]->column == 0)) {

                        // new col to left
                        for(int i=0; i< items.count(); i++) items[i]->column += 1;
                        stateData.drag.item->column = 0;
                        stateData.drag.item->order = 0;

                        // shift columns widths to the right
                        for(int i=9; i>0; i--) columns[i] = columns[i-1];
                        columns[0] = stateData.drag.width;

                        changed = true;
                    }

                } else if (items.last()->column < 9 && items.last() && items.last()->column < targetcol) {

                    // new col to the right
                    stateData.drag.item->column = items.last()->column + 1;
                    stateData.drag.item->order = 0;

                    // make column width same as source width
                    columns[stateData.drag.item->column] = stateData.drag.width;

                    changed = true;

                } else {

                    // add to the end of the column
                    int last = -1;
                    for(int i=0; i<items.count() && items[i]->column <= targetcol; i++) {
                        if (items[i]->column == targetcol) last=i;
                    }

                    // so long as its been dragged below the last entry on the column !
                    if (last >= 0 && pos.y() > items[last]->geometry().bottom()) {
                        stateData.drag.item->column = targetcol;
                        stateData.drag.item->order = items[last]->order+1;
                    }

                    changed = true;
                }

            }

            if (changed) {
                // drop it down
                updateGeometry();
                updateView();
            }

        } else if (state == YRESIZE) {

            // resize in rows, so in 75px units
            int addrows = (pos.y() - stateData.yresize.posy) / ROWHEIGHT;
            int setdeep = stateData.yresize.deep + addrows;

            //min height
            if (setdeep < 5) setdeep=5; // min of 5 rows

            stateData.yresize.item->deep = setdeep;

            // drop it down
            updateGeometry();
            updateView();

        } else if ( state == XRESIZE) {

            // multiples of 50 (smaller than margin)
            int addblocks = (pos.x() - stateData.xresize.posx) / 50;
            int setcolumn = stateData.xresize.width + (addblocks * 50);

            // min max width
            if (setcolumn < 800) setcolumn = 800;
            if (setcolumn > 4400) setcolumn = 4400;

            columns[stateData.xresize.column] = setcolumn;

            // animate
            updateGeometry();
            updateView();
        }
    }

    block = false;
    return returning;
}


void
ChartSpaceItem::clicked()
{
    if (isVisible()) hide();
    else show();

    //if (brush.color() == GColor(CChartSpaceItemBACKGROUND)) brush.setColor(Qt::red);
    //else brush.setColor(GColor(CChartSpaceItemBACKGROUND));

    update(geometry());
}

ChartSpaceItemDetail
ChartSpaceItemRegistry::detailForType(int type)
{
    for(int i=0; i<_items.count(); i++) {
        if (_items.at(i).type == type) return _items.at(i);
    }

    // not found :(
    return ChartSpaceItemDetail();
}
