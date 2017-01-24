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
    GcChartWindow(context), context(context)
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

    // sort out the view
    updateGeometry();

    // now scale to fit... nothing fancy for now
    view->setSceneRect(scene->itemsBoundingRect());
    view->fitInView(view->sceneRect(), Qt::KeepAspectRatio);

    // watch the view for mouse events
    view->setMouseTracking(true);
    scene->installEventFilter(this);
}

void
OverviewWindow::resizeEvent(QResizeEvent *)
{
    // hmmm, this isn't quite right !
    view->fitInView(view->sceneRect(), Qt::KeepAspectRatio);
}

static bool cardSort(const Card* left, const Card* right)
{
    return (left->column < right->column ? true : (left->column == right->column && left->order < right->order ? true : false));
}

void
OverviewWindow::updateGeometry()
{

    // order the items to their positions
    qSort(cards.begin(), cards.end(), cardSort);

    int y=25;
    int column=0;

    // just set their geometry for now, no interaction
    for(int i=0; i<cards.count(); i++) {

        // move on to next column
        if (cards[i]->column > column) { y=25; column = cards[i]->column; }

        // set geometry
        int ty = y;
        int tx = 25 + (column*400) + (column*25);
        int twidth = 400;
        int theight = cards[i]->deep * 25;

        cards[i]->setGeometry(tx, ty, twidth, theight);

        // add to scene if new
        if (!cards[i]->onscene) {
            scene->addItem(cards[i]);
            qDebug()<<"add col,order:"<<cards[i]->column<<cards[i]->order<<"y,x,width,height"<<ty<<tx<<twidth<<theight;
            cards[i]->onscene = true;
        }

        // set spot for next tile
        y += theight + 25;
    }
    qDebug()<<"scene="<<scene->itemsBoundingRect();
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

bool
OverviewWindow::eventFilter(QObject *, QEvent *event)
{
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

    } else  if (event->type() == QEvent::GraphicsSceneMouseMove) {

        // where am i ?
        QPointF pos = static_cast<QGraphicsSceneMouseEvent*>(event)->scenePos();
        QGraphicsItem *item = scene->itemAt(pos, view->transform());

        qDebug()<<"mouse:"<<pos;
        if (item) {
            qDebug()<<"POP..";
        }
    }
    return returning;
}
