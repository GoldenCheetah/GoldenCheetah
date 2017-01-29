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

#include <QGraphicsSceneMouseEvent>

OverviewWindow::OverviewWindow(Context *context) :
    GcChartWindow(context), mode(CONFIG), state(NONE), context(context), group(NULL),
                            yresizecursor(false), xresizecursor(false), block(false)
{
    setContentsMargins(0,0,0,0);
    setProperty("color", GColor(COVERVIEWBACKGROUND));

    setControls(NULL);

    QVBoxLayout *main = new QVBoxLayout;

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // add a view and scene and centre
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(this);

    // how to move etc
    //view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setFrameStyle(QFrame::NoFrame);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setScene(scene);
    main->addWidget(view);
    setChartLayout(main);

    // default column widths - max 10 columns;
    columns << 400 << 800 << 800 << 400 << 800 << 800 << 400 << 800 << 800 << 400;

    // XXX lets hack in some tiles to start (will load from config later XXX
    newCard(0, 1, 5);
    newCard(0, 2, 10);
    newCard(0, 3, 10);
    newCard(0, 4, 5);
    newCard(1, 1, 10);
    newCard(1, 2, 5);
    newCard(1, 3, 10);
    newCard(1, 4, 5);
    newCard(2, 1, 10);
    newCard(2, 2, 5);
    newCard(2, 3, 10);
    newCard(2, 4, 5);
    newCard(3, 1, 5);
    newCard(3, 2, 10);
    newCard(3, 3, 5);
    newCard(3, 4, 10);

    // set the widgets etc
    configChanged(CONFIG_APPEARANCE);

    // for changing the view
    viewchange = new QPropertyAnimation(this, "viewRect");
    viewchange->setEasingCurve(QEasingCurve(QEasingCurve::OutQuint));

    // sort out the view
    updateGeometry();

    // watch the view for mouse events
    view->setMouseTracking(true);
    scene->installEventFilter(this);
}

static bool cardSort(const Card* left, const Card* right)
{
    return (left->column < right->column ? true : (left->column == right->column && left->order < right->order ? true : false));
}

void
OverviewWindow::updateGeometry()
{
    bool animated=false;

    // order the items to their positions
    qSort(cards.begin(), cards.end(), cardSort);

    int y=70;
    int maxy = y;
    int column=-1;

    int x=70;

    // just set their geometry for now, no interaction
    for(int i=0; i<cards.count(); i++) {

        // don't show hidden
        if (!cards[i]->isVisible()) continue;

        // move on to next column, check if first item too
        if (cards[i]->column > column) {

            // once past the first column we need to update x
            if (column >= 0) x+= columns[column] + 70;

            int diff = cards[i]->column - column - 1;
            if (diff > 0) {

                // there are empty columns so shift the cols to the right
                // to the left to fill  the gap left and all  the column
                // widths also need to move down too
                for(int j=cards[i]->column-1; j < 8; j++) columns[j]=columns[j+1];
                for(int j=i; j<cards.count();j++) cards[j]->column -= diff;
            }
            y=70; column = cards[i]->column;

        }

        // set geometry
        int ty = y;
        int tx = x;
        int twidth = columns[column];
        int theight = cards[i]->deep * 70;


        // for setting the scene rectangle
        if (maxy < ty+theight+70) maxy = ty+theight+70;

        // add to scene if new
        if (!cards[i]->onscene) {
            cards[i]->setGeometry(tx, ty, twidth, theight);
            scene->addItem(cards[i]);
            cards[i]->onscene = true;

        } else if (cards[i]->invisible == false &&
                   (cards[i]->geometry().x() != tx ||
                    cards[i]->geometry().y() != ty ||
                    cards[i]->geometry().width() != twidth ||
                    cards[i]->geometry().height() != theight)) {

            // its moved, so animate that.
            if (animated == false) {

                // we've got an animation to perform
                animated = true;

                // we're starting to animate so clear and restart any animations
                if (group) group->clear();
                else {
                    group = new QParallelAnimationGroup(this);
                    //connect(group, SIGNAL(finished()), this, SLOT(fitView()));
                }
            }

            // add an animation for this movement
            QPropertyAnimation *animation = new QPropertyAnimation(cards[i], "geometry");
            animation->setDuration(300);
            animation->setStartValue(cards[i]->geometry());
            animation->setEndValue(QRect(tx,ty,twidth,theight));
            animation->setEasingCurve(QEasingCurve(QEasingCurve::OutQuint));

            group->addAnimation(animation);
        }

        // set spot for next tile
        y += theight + 70;
    }

    // set the scene rectangle, columns start at 0
    sceneRect = QRectF(0, 0, columns[column] + x + 70, maxy);

    if (animated) group->start();
}

void
OverviewWindow::configChanged(qint32)
{
    setProperty("color", GColor(COVERVIEWBACKGROUND));
    view->setBackgroundBrush(QBrush(GColor(COVERVIEWBACKGROUND)));
    scene->setBackgroundBrush(QBrush(GColor(COVERVIEWBACKGROUND)));

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
    if (state != XRESIZE && state != DRAG) {

        // much of a resize / change ?
        double dx = fabs(viewRect.x() - sceneRect.x());
        double dy = fabs(viewRect.y() - sceneRect.y());
        double dwidth = fabs(viewRect.width() - sceneRect.width());
        double dheight = fabs(viewRect.height() - sceneRect.height());

        // scale immediately if not a bit change
        // otherwise it feels unresponsive
        if (viewRect.width() == 0 || (dx < 20 && dy < 20 && dwidth < 20 && dheight < 20)) {
            setViewRect(sceneRect);
        } else {

            // tempting to make this longer but feels ponderous at longer durations
            viewchange->setDuration(400);
            viewchange->setStartValue(viewRect);
            viewchange->setEndValue(sceneRect);
            viewchange->start();
        }
    }
}

void
OverviewWindow::setViewRect(QRectF rect)
{
    //static bool __block = false;
    //if (__block) return;

    //__block = true;
    viewRect = rect;

    // fit to scene width XXX need to fix scrollbars.
    double scale = view->frameGeometry().width() / viewRect.width();
    QRectF scaledRect(0,0, viewRect.width(), view->frameGeometry().height() / scale);

    // scale to selection
    view->scale(scale,scale);
    view->setSceneRect(scaledRect);
    view->fitInView(scaledRect, Qt::KeepAspectRatio);
    view->update();

    //__block = false;
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

        }

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
            }

            // end state;
            state = NONE;

            // drop it down
            updateGeometry();
            updateView();
        }

    } else if (event->type() == QEvent::GraphicsSceneMouseMove) {

        // thanks we'll intercept that
        if (mode == CONFIG) {
            event->accept();
            returning = true;
        }

        if (mode == CONFIG && state == NONE) {                 // hovering

            // where am i ?
            QPointF pos = static_cast<QGraphicsSceneMouseEvent*>(event)->scenePos();
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

        } else if (mode == CONFIG && state == DRAG) {          // dragging?

            // where am i ?
            QPointF pos = static_cast<QGraphicsSceneMouseEvent*>(event)->scenePos();

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
                //int targetcol = (pos.x()-stateData.drag.offx)/870;
                // create a new column to the right?
                int x=70;
                int targetcol = -1;
                for(int i=0; i<10; i++) {
                    if (pos.x() > x && pos.x() < (x+columns[i]+70)) {
                        targetcol = i;
                        break;
                    }
                    x += columns[i]+70;
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

                    // make column width sane as source width
                    columns[cards.last()->column+1] = stateData.drag.width;

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

            QPointF pos = static_cast<QGraphicsSceneMouseEvent*>(event)->scenePos();

            // resize in rows, so in 75px units
            int addrows = (pos.y() - stateData.yresize.posy) / 75;
            int setdeep = stateData.yresize.deep + addrows;
            if (setdeep < 3) setdeep=3;

            stateData.yresize.card->deep = setdeep;

            // drop it down
            updateGeometry();
            updateView();
        } else if (mode == CONFIG && state == XRESIZE) {

            QPointF pos = static_cast<QGraphicsSceneMouseEvent*>(event)->scenePos();

            // multiples of 50 (smaller than margin)
            int addblocks = (pos.x() - stateData.xresize.posx) / 50;
            int setcolumn = stateData.xresize.width + (addblocks * 50);
            if (setcolumn < 200) setcolumn = 200;

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
