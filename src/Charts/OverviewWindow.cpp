/*
 * Copyright (c) 2017 Mark Liversedge (liversedge@gmail.com)
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

#include "OverviewWindow.h"

#include "TabView.h"
#include "Athlete.h"
#include <QGraphicsSceneMouseEvent>

OverviewWindow::OverviewWindow(Context *context) :
    GcChartWindow(context), mode(CONFIG), state(NONE), context(context), group(NULL), _viewY(0),
                            yresizecursor(false), xresizecursor(false), block(false), scrolling(false),
                            setscrollbar(false), lasty(-1)
{
    setContentsMargins(0,0,0,0);
    setProperty("color", GColor(COVERVIEWBACKGROUND));
    setProperty("nomenu", true);
    setShowTitle(false);
    setControls(NULL);

    QHBoxLayout *main = new QHBoxLayout;

    // add a view and scene and centre
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(this);
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
    setChartLayout(main);

    // default column widths - max 10 columns;
    // note the sizing is such that each card is the equivalent of a full screen
    // so we can embed charts etc without compromising what they can display
    columns << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200 << 1200;

    // XXX lets hack in some tiles to start (will load from config later) XXX

    // column 0
    newCard("Sport", 0, 0, 5);
    newCard("Duration", 0, 1, 5, Card::METRIC, "workout_time");
    newCard("Route", 0, 2, 10);
    newCard("Distance", 0, 3, 5, Card::METRIC, "total_distance");
    newCard("Climbing", 0, 4, 5, Card::METRIC, "elevation_gain");
    newCard("Speed", 0, 6, 5, Card::METRIC, "average_speed");

    // column 1
    newCard("Heartrate", 1, 0, 5, Card::METRIC, "average_hr");
    newCard("HRV", 1, 1, 5);
    newCard("Heartrate Zones", 1, 2, 10);
    newCard("Pace Zones", 1, 3, 11);
    newCard("Cadence", 1, 4, 5, Card::METRIC, "average_cad");

    // column 2
    newCard("RPE", 2, 0, 5);
    newCard("Stress", 2, 1, 5, Card::METRIC, "coggan_tss");
    newCard("W'bal Zones", 2, 2, 10);
    newCard("Intervals", 2, 3, 17);

    // column 3
    newCard("Power", 3, 0, 5, Card::METRIC, "average_power");
    newCard("Intensity", 3, 1, 5, Card::METRIC, "coggan_if");
    newCard("Power Zones", 3, 2, 10);
    newCard("Equivalent Power", 3, 3, 5, Card::METRIC, "coggan_np");
    newCard("Power Model", 3, 4, 11);

    // for changing the view
    group = new QParallelAnimationGroup(this);
    viewchange = new QPropertyAnimation(this, "viewRect");
    viewchange->setEasingCurve(QEasingCurve(QEasingCurve::OutQuint));

    // for scrolling the view
    scroller = new QPropertyAnimation(this, "viewY");
    scroller->setEasingCurve(QEasingCurve(QEasingCurve::Linear));

    // sort out the view
    updateGeometry();

    // watch the view for mouse events
    view->setMouseTracking(true);
    scene->installEventFilter(this);

    // once all widgets created we can connect the signals
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(scroller, SIGNAL(finished()), this, SLOT(scrollFinished()));
    connect(scrollbar, SIGNAL(valueChanged(int)), this, SLOT(scrollbarMoved(int)));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));

    // set the widgets etc
    configChanged(CONFIG_APPEARANCE);
}

// when a ride is selected we need to notify all the cards
void
OverviewWindow::rideSelected()
{
    // ride item changed
    foreach(Card *card, cards) card->setData(myRideItem);

    // update
    updateView();
}

void
Card::setData(RideItem *item)
{
    if (type == METRIC) {
        value = item->getStringForSymbol(settings.symbol, parent->context->athlete->useMetricUnits);
    }
}

// cards need to show they are in config mode
void
Card::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setBrush(brush);
    QPainterPath path;
    path.addRoundedRect(QRectF(0,0,geometry().width(),geometry().height()), ROWHEIGHT/5, ROWHEIGHT/5);
    painter->setPen(Qt::NoPen);
    painter->fillPath(path, brush.color());
    painter->drawPath(path);
    painter->setPen(GColor(CPLOTGRID));
    //XXXpainter->drawLine(QLineF(0,ROWHEIGHT*2,geometry().width(),ROWHEIGHT*2));
    //painter->fillRect(QRectF(0,0,geometry().width()+1,geometry().height()+1), brush);
    QFont titlefont;
    titlefont.setPointSize(ROWHEIGHT-18); // need a bit of space
    //titlefont.setWeight(QFont::Bold);
    painter->setPen(QColor(200,200,200));
    painter->setFont(titlefont);
    painter->drawText(QPointF(ROWHEIGHT /2.0f, QFontMetrics(titlefont, parent->device()).height()), name);

    // only paint contents if not dragging
    if (drag) return;

    if (type == METRIC) {

        // we need the metric units
        if (metric == NULL) {
            // get the metric details
            RideMetricFactory &factory = RideMetricFactory::instance();
            metric = const_cast<RideMetric*>(factory.rideMetric(settings.symbol));
            if (metric) units = metric->units(parent->context->athlete->useMetricUnits);
        }

        // paint the value in the middle using a font 2xROWHEIGHT
        QFont bigfont;
        bigfont.setPointSize(ROWHEIGHT*2);
        painter->setPen(GColor(CPLOTMARKER));
        painter->setFont(bigfont);

        QFont smallfont;
        smallfont.setPointSize(ROWHEIGHT*0.6f);

        double addy = 0;
        if (units != "" && units != tr("seconds")) addy = QFontMetrics(smallfont).height();

        // mid is slightly higher to account for space around title, move mid up
        double mid = (ROWHEIGHT*1.5f) + ((geometry().height() - (ROWHEIGHT*2)) / 2.0f) - (addy/2);

        // we align centre and mid
        QFontMetrics fm(bigfont);
        QRectF rect = QFontMetrics(bigfont, parent->device()).boundingRect(value);

        painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                                  mid + (fm.ascent() / 3.0f)), value); // divided by 3 to account for "gap" at top of font

        // now units
        if (addy > 0) {
            painter->setPen(QColor(100,100,100));
            painter->setFont(smallfont);

            painter->drawText(QPointF((geometry().width() - QFontMetrics(smallfont).width(units)) / 2.0f,
                                  mid + (fm.ascent() / 3.0f) + addy), units); // divided by 3 to account for "gap" at top of font
        }
    }
}

static bool cardSort(const Card* left, const Card* right)
{
    return (left->column < right->column ? true : (left->column == right->column && left->order < right->order ? true : false));
}

void
OverviewWindow::updateGeometry()
{
    bool animated=false;
    group->clear();

    // order the items to their positions
    qSort(cards.begin(), cards.end(), cardSort);

    int y=SPACING;
    int maxy = y;
    int column=-1;

    int x=SPACING;

    // just set their geometry for now, no interaction
    for(int i=0; i<cards.count(); i++) {

        // don't show hidden
        if (!cards[i]->isVisible()) continue;

        // move on to next column, check if first item too
        if (cards[i]->column > column) {

            // once past the first column we need to update x
            if (column >= 0) x+= columns[column] + SPACING;

            int diff = cards[i]->column - column - 1;
            if (diff > 0) {

                // there are empty columns so shift the cols to the right
                // to the left to fill  the gap left and all  the column
                // widths also need to move down too
                for(int j=cards[i]->column-1; j < 8; j++) columns[j]=columns[j+1];
                for(int j=i; j<cards.count();j++) cards[j]->column -= diff;
            }
            y=SPACING; column = cards[i]->column;

        }

        // set geometry
        int ty = y;
        int tx = x;
        int twidth = columns[column];
        int theight = cards[i]->deep * ROWHEIGHT;

        // make em smaller when configuring visual cue stolen from Windows Start Menu
        int add = (state == DRAG) ? (ROWHEIGHT/2) : 0;


        // for setting the scene rectangle - but ignore a card if we are dragging it
        if (maxy < ty+theight+SPACING) maxy = ty+theight+SPACING;

        // add to scene if new
        if (!cards[i]->onscene) {
            cards[i]->setGeometry(tx, ty, twidth, theight);
            scene->addItem(cards[i]);
            cards[i]->onscene = true;

        } else if (cards[i]->invisible == false &&
                   (cards[i]->geometry().x() != tx+add ||
                    cards[i]->geometry().y() != ty+add ||
                    cards[i]->geometry().width() != twidth-(add*2) ||
                    cards[i]->geometry().height() != theight-(add*2))) {

            // we've got an animation to perform
            animated = true;

            // add an animation for this movement
            QPropertyAnimation *animation = new QPropertyAnimation(cards[i], "geometry");
            animation->setDuration(300);
            animation->setStartValue(cards[i]->geometry());
            animation->setEndValue(QRect(tx+add,ty+add,twidth-(add*2),theight-(add*2)));

            // when placing a little feedback helps
            if (cards[i]->placing) {
                animation->setEasingCurve(QEasingCurve(QEasingCurve::OutBack));
                cards[i]->placing = false;
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
OverviewWindow::configChanged(qint32)
{
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
OverviewWindow::updateView()
{
    scene->setSceneRect(sceneRect);
    scene->update();

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
OverviewWindow::edgeScroll(bool down)
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
OverviewWindow::scrollTo(int newY)
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
OverviewWindow::setViewRect(QRectF rect)
{
    viewRect = rect;

    // fit to scene width XXX need to fix scrollbars.
    double scale = view->frameGeometry().width() / viewRect.width();
    QRectF scaledRect(0,_viewY, viewRect.width(), view->frameGeometry().height() / scale);

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

        // move the card being dragged
        stateData.drag.card->setPos(pos.x()-stateData.drag.offx, pos.y()-stateData.drag.offy);
    }

    view->update();

}

bool
OverviewWindow::eventFilter(QObject *, QEvent *event)
{
    if (block) return false;

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
        // dragging a card around
        if (mode == CONFIG && state == NONE) {

            // we always trap clicks when configuring, to avoid
            // any inadvertent processing of clicks in the widget
            event->accept();
            returning = true;

            // where am i ?
            QPointF pos = static_cast<QGraphicsSceneMouseEvent*>(event)->scenePos();
            QGraphicsItem *item = scene->itemAt(pos, view->transform());
            Card *card = static_cast<Card*>(item);

            if (card) {

               // are we on the boundary of the card?
               double offx = pos.x()-card->geometry().x();
               double offy = pos.y()-card->geometry().y();


               if (card->geometry().height()-offy < 10) {

                    state = YRESIZE;

                    stateData.yresize.card = card;
                    stateData.yresize.deep = card->deep;
                    stateData.yresize.posy = pos.y();

               } else if (card->geometry().width()-offx < 10) {

                    state = XRESIZE;

                    stateData.xresize.column = card->column;
                    stateData.xresize.width = columns[card->column];
                    stateData.xresize.posx = pos.x();

               } else {

                    // we're grabbing a card, so lets
                    // work out the offset so we can move
                    // it around when we start dragging
                    state = DRAG;
                    card->invisible = true;
                    card->drag = true;
                    card->brush = GColor(CPLOTMARKER); //XXX hack whilst they're tiles
                    card->setZValue(100);

                    stateData.drag.card = card;
                    stateData.drag.offx = offx;
                    stateData.drag.offy = offy;
                    stateData.drag.width = columns[card->column];

                    // what is the offset?
                    //updateGeometry();
                    scene->update();
                    view->update();
                }
            }
        }

    } else  if (event->type() == QEvent::GraphicsSceneMouseRelease) {

        // stop dragging
        if (mode == CONFIG && (state == DRAG || state == YRESIZE || state == XRESIZE)) {

            // we want this one
            event->accept();
            returning = true;

            // set back to visible if dragging
            if (state == DRAG) {
                stateData.drag.card->invisible = false;
                stateData.drag.card->setZValue(10);
                stateData.drag.card->placing = true;
                stateData.drag.card->drag = false;
                stateData.drag.card->brush = GColor(CCARDBACKGROUND);
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

        // thanks we'll intercept that
        if (mode == CONFIG) {
            event->accept();
            returning = true;
        }

        if (mode == CONFIG && state == NONE) {                 // hovering

            // where am i ?
            QGraphicsItem *item = scene->itemAt(pos, view->transform());
            Card *card = static_cast<Card*>(item);

            if (card) {

                // are we on the boundary of the card?
                double offx = pos.x()-card->geometry().x();
                double offy = pos.y()-card->geometry().y();

                if (yresizecursor == false && card->geometry().height()-offy < 10) {

                    yresizecursor = true;
                    setCursor(QCursor(Qt::SizeVerCursor));

                } else if (yresizecursor == true && card->geometry().height()-offy > 10) {

                    yresizecursor = false;
                    setCursor(QCursor(Qt::ArrowCursor));

                }

                if (xresizecursor == false && card->geometry().width()-offx < 10) {

                    xresizecursor = true;
                    setCursor(QCursor(Qt::SizeHorCursor));

                } else if (xresizecursor == true && card->geometry().width()-offx > 10) {

                    xresizecursor = false;
                    setCursor(QCursor(Qt::ArrowCursor));

                }

            } else {

                // not hovering over tile, so if still have a resize cursor
                // set it back to the normal arrow pointer
                if (yresizecursor || xresizecursor) {
                    xresizecursor = yresizecursor = false;
                    setCursor(QCursor(Qt::ArrowCursor));
                }
            }

        } else if (mode == CONFIG && state == DRAG && !scrolling) {          // dragging?

            // move the card being dragged
            stateData.drag.card->setPos(pos.x()-stateData.drag.offx, pos.y()-stateData.drag.offy);

            // should I move?
            QList<QGraphicsItem *> overlaps = scene->items(pos);

            // we always overlap with ourself, so see if more
            if (overlaps.count() > 1) {

                Card *over = static_cast<Card*>(overlaps[1]);
                if (pos.y()-over->geometry().y() > over->geometry().height()/2) {

                    // place below the one its over
                    stateData.drag.card->column = over->column;
                    stateData.drag.card->order = over->order+1;
                    for(int i=cards.indexOf(over); i< cards.count(); i++) {
                        if (i>=0 && cards[i]->column == over->column && cards[i]->order > over->order && cards[i] != stateData.drag.card)
                            cards[i]->order += 1;
                    }

                } else {

                    // place above the one its over
                    stateData.drag.card->column = over->column;
                    stateData.drag.card->order = over->order;
                    for(int i=0; i< cards.count(); i++) {
                        if (i>=0 && cards[i]->column == over->column && cards[i]->order >= (over->order) && cards[i] != stateData.drag.card)
                            cards[i]->order += 1;
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

                if (cards.last()->column < 9 && targetcol < 0) {

                    // don't keep moving - if we're already alone in column 0 then no move is needed
                    if (stateData.drag.card->column != 0 || (cards.count()>1 && cards[1]->column == 0)) {

                        // new col to left
                        for(int i=0; i< cards.count(); i++) cards[i]->column += 1;
                        stateData.drag.card->column = 0;
                        stateData.drag.card->order = 0;

                        // shift columns widths to the right
                        for(int i=9; i>0; i--) columns[i] = columns[i-1];
                        columns[0] = stateData.drag.width;
                    }

                } else if (cards.last()->column < 9 && cards.last() && cards.last()->column < targetcol) {

                    // new col to the right
                    stateData.drag.card->column = cards.last()->column + 1;
                    stateData.drag.card->order = 0;

                    // make column width same as source width
                    columns[stateData.drag.card->column] = stateData.drag.width;

                } else {

                    // add to the end of the column
                    int last = -1;
                    for(int i=0; i<cards.count() && cards[i]->column <= targetcol; i++) {
                        if (cards[i]->column == targetcol) last=i;
                    }

                    // so long as its been dragged below the last entry on the column !
                    if (last >= 0 && pos.y() > cards[last]->geometry().bottom()) {
                        stateData.drag.card->column = targetcol;
                        stateData.drag.card->order = cards[last]->order+1;
                    }
                }
            }

            // drop it down
            updateGeometry();
            updateView();

        } else if (mode == CONFIG && state == YRESIZE) {

            // resize in rows, so in 75px units
            int addrows = (pos.y() - stateData.yresize.posy) / ROWHEIGHT;
            int setdeep = stateData.yresize.deep + addrows;

            //min height
            if (setdeep < 5) setdeep=5; // min of 5 rows

            stateData.yresize.card->deep = setdeep;

            // drop it down
            updateGeometry();
            updateView();

        } else if (mode == CONFIG && state == XRESIZE) {

            // multiples of 50 (smaller than margin)
            int addblocks = (pos.x() - stateData.xresize.posx) / 50;
            int setcolumn = stateData.xresize.width + (addblocks * 50);

            // min max width
            if (setcolumn < 800) setcolumn = 800;
            if (setcolumn > 1600) setcolumn = 1600;

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
Card::clicked()
{
    if (isVisible()) hide();
    else show();

    //if (brush.color() == GColor(CCARDBACKGROUND)) brush.setColor(Qt::red);
    //else brush.setColor(GColor(CCARDBACKGROUND));

    update(geometry());
}
