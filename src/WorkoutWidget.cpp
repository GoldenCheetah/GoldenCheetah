/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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


#include "WorkoutWidget.h"
#include "WorkoutWindow.h"
#include "WorkoutWidgetItems.h"

#include "WPrime.h"
#include "ErgFile.h"
#include "RideFileCache.h"

#include "TimeUtils.h" // time_to_string()
#include <QFontMetrics>

#include <cmath>
#include <float.h> // DBL_EPSILON

// height and width of widgets
static const double IHEIGHT = 10;
static const double THEIGHT = 25;
static const double BHEIGHT = 35;
static const double LWIDTH = 55;
static const double RWIDTH = 35;

// axis tic marks
static const int XTICLENGTH = 3;
static const int YTICLENGTH = 0;
static const int XTICS = 20;
static const int YTICS = 10;

static const int SPACING = 2; // between labels and tics (if there are tics)

static const int XMOVE = 5; // how many to move X when cursoring
static const int YMOVE = 1; // how many to move Y when cursoring

// grid lines (y only)
static bool GRIDLINES = true;

WorkoutWidget::WorkoutWidget(WorkoutWindow *parent, Context *context) :
    QWidget(parent),  state(none), ergFile(NULL), dragging(NULL), parent(parent), context(context), stackptr(0)
{
    maxX_=3600;
    maxY_=300;

    onDrag = onCreate = onRect = atRect = QPointF(-1,-1);

    // watch mouse events for user interaction
    installEventFilter(this);
    setMouseTracking(true);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(ergFileSelected(ErgFile*)));
    configChanged(CONFIG_APPEARANCE);
}

void
WorkoutWidget::timeout()
{
    // into event filter (our state machine)
    QEvent timer(QEvent::Timer);
    eventFilter(this, &timer);
}

bool
WorkoutWidget::eventFilter(QObject *obj, QEvent *event)
{
    // we nearly always return false for filtering
    // except wheel events (for example)
    bool returning = false;

    // process as normal if not one of ours
    if (obj != this) return false;

    // where is the cursor
    QPoint p = mapFromGlobal(QCursor::pos());
    QPointF v = reverseTransform(p.x(),p.y());

    // is a repaint going to be needed?
    bool updateNeeded=false;

    //
    // STATE MACHINE [no edit mode or selection mode yet]
    //
    //   EVENT              STATE       ACTION                              NEXT STATE
    //   --------------     ------      ---------------                     ----------
    // 1 mouse move         none        hover/unhover point/block           none
    //                      drag        move point around                   drag
    //                      rect        resize and scan for selections      rect
    //
    // 2 mouse click        none        hovering? drag point                drag
    //   not shifted        none        not hovering? create                drag
    //
    // 3 mouse release      drag        unselect                            none
    //                      rect        none                                none
    //
    // 4 mouse timeout      drag        no move? click-hold                 none
    //   [not active yet]   drag        moved? ignore                       drag
    //
    //
    // 5 mouse wheel        none        rescale selectes/all                none
    //   up and down        drag        ignore                              drag
    //
    //
    // 6 mouse click        none        hovering? select point              none
    //   shifted            none        not hovering? begin select          rect
    //
    //
    // 7 key press          any         ESC clear selection                 unchanged
    //                      any         ^Z undo, ^Y redo                    unchanged
    //                      any         cursors move selected               unchanged
    //                      any         DEL delete selected                 unchanged
    //
    //
    // 8 mouse enter        any         grab keyboard focus                 unchanged
    //
    //
    //

    //
    // 1 MOUSE MOVE [we always repaint]
    //
    if (event->type() == QEvent::MouseMove) {

        // forget the onCreate!
        onCreate = QPoint(-1,-1);

        // always update the x/y values in the toolbar
        parent->xlabel->setText(time_to_string(v.x()));
        parent->ylabel->setText(QString("%1w").arg(v.y()));

        if (state == none) {

            // if we're not in any particular state then just
            // highlight for hover/unhover
            updateNeeded = setBlockCursor();

            // is the mouse on the canvas?
            if (canvas().contains(p)) {

                // unser/set hover/unhover state *IF NEED TO*
                foreach(WWPoint *point, points_) {
                    if (point->bounding().contains(p)) {
                        if (!point->hover) {
                            point->hover = true;
                            updateNeeded=true;
                        }
                    } else {
                        if (point->hover) {
                            point->hover = false;
                            updateNeeded=true;
                        }
                    }
                }
            }

        } else if (state == drag) {

            // we're dragging this point around, get on and
            // do that, but apply constrains
            if (dragging) {
                updateNeeded = movePoint(p);

                // this may possibly be too expensive
                // on slower hardware?
                recompute();
            } else {
                // not possible?
                state = none;
                qDebug()<<"WW FSM: drag state dragging=NULL";
            }

        } else if (state == rect) {

            // we're selecting via a rectangle
            atRect = p;
            updateNeeded = selectPoints(); // go and check / select
        }
    }

    //
    // 2 AND 6 MOUSE PRESS
    //
    if (event->type() == QEvent::MouseButtonPress) {

        // watch for shift when clicking
        Qt::KeyboardModifiers kmod = static_cast<QInputEvent*>(event)->modifiers();

        // if not in draw mode toggle shift to select
        // so if press shift in draw -> select
        // if press shift in select -> draw
        if (parent->draw == false) kmod ^= Qt::ShiftModifier;

        if (state == none && canvas().contains(p)) {

            // either select existing to drag
            // or create a new one to drag
            bool hover=false;
            foreach(WWPoint *point, points_) {
                if (point->bounding().contains(p)) {

                    //
                    // HOVERING
                    //

                    // SHIFT-CLICK TO TOGGLE SELECT
                    if (kmod & Qt::ShiftModifier) {
                        point->selected = !point->selected;
                        hover=true;
                        updateNeeded=true;
                    } else {

                        // PLAIN CLICK TO DRAG
                        updateNeeded=true;
                        dragging = point;
                        hover=true;
                        onDrag = QPointF(dragging->x, dragging->y);
                        state = drag;
                        
                    }
                    break;
                }
            }

            // if state is still none and we're not hovering, 
            // we aren't on top of a point, so create a new
            // one or start select mode if shift is pressed
            if (state == none && !hover) {

                // SHIFT RECTANGLE SELECT
                if (kmod & Qt::ShiftModifier) {

                    // where are we
                    atRect = onRect = p;
                    state = rect;
                    updateNeeded = true;
 
                } else {
                    // UNSHIFTED CREATE A POINT
                    updateNeeded = createPoint(p);

                    // recompute metrics
                    recompute();

                    // but we may press and hold for a snip
                    // so lets set the timer and remember
                    // where we were
                    onCreate = p;
                    QTimer::singleShot(500, this, SLOT(timeout()));
                }
            }
        }
    }

    //
    // 3. MOUSE RELEASED
    //
    if (event->type() == QEvent::MouseButtonRelease) {

        // DRAGGING
        if (state == drag && dragging) {

            // create command to reflect the drag, but only
            // if it actually moved!
            if (dragging->x != onDrag.x() || dragging->y != onDrag.y())
                new MovePointCommand(this, onDrag, QPointF(dragging->x, dragging->y), points_.indexOf(dragging));

            // recompute metrics
            recompute();
        }

        // SELECTING ENDS [so turn selecting -> selected]
        if (state == rect) {
            selectedPoints();
            onRect = atRect = QPointF(-1,-1);
        }

        state = none;
        dragging = NULL;
        updateNeeded = true;
    }

    //
    // 4. MOUSE TIMEOUT [click and hold]
    //
    if (event->type() == QEvent::Timer) {
        if (state == drag && onCreate == p) {

            // TODO .. click and hold ..
        }
    }

    //
    // 5. MOUSE WHEEL
    //
    if (event->type() == QEvent::Wheel) {

        // not for drag state, but everything else is fine
        if (state == none) {
            QWheelEvent *w = static_cast<QWheelEvent*>(event);
#if QT_VERSION >= 0x050000
            updateNeeded = scale(w->angleDelta());
#else
            updateNeeded = scale(QPoint(0,w->delta()));
#endif
            returning = true;
        }

        // will need to ..
        recompute();
    }

    //
    // 6. KEYPRESS
    //
    if (event->type() == QEvent::KeyPress) {

        // we care about cmd / ctrl
        Qt::KeyboardModifiers kmod = static_cast<QInputEvent*>(event)->modifiers();
        bool ctrl = (kmod & Qt::ControlModifier) != 0;
        int key;

        switch((key=static_cast<QKeyEvent*>(event)->key())) {

        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Left:
        case Qt::Key_Right:
            updateNeeded=movePoints(key,kmod);
            break;

        case Qt::Key_Escape:
            updateNeeded=selectClear();
            break;

        case Qt::Key_Y:
            if (ctrl) {
                redo();
                updateNeeded=true;
            }
            break;

        case Qt::Key_Z:
            if (ctrl) {
                undo();
                updateNeeded=true;
            }
            break;

        case Qt::Key_Delete:
            // delete!
            updateNeeded=deleteSelected();
            break;
        }

        // we moved / deleted etc, so redo metrics et al
        if (updateNeeded) recompute();
    }

    //
    // Mouse enters
    //
    if (event->type() == QEvent::Enter) {

        // grab focus if we don't have it
        if (!hasFocus()) {
            setFocus(Qt::MouseFocusReason);
        }
    }

    // ALL DONE

    // trigger an update if one is needed
    if (updateNeeded) {

        // the cursor may now hover over a block or point
        // but don't interfere whilst state processing
        if (state == none) setBlockCursor();

        // repaint
        update();
    }

    // return false - we are eavesdropping not processing.
    // except for wheel events which we steal
    return returning;
}

bool
WorkoutWidget::setBlockCursor()
{
    // where is the mouse?
    QPoint c = mapFromGlobal(QCursor::pos());

    // not on canvas?
    if (!canvas().contains(c)) {
        if (cursorBlock != QPainterPath()) {
            cursorBlock = QPainterPath();
            return true;
        }
        return false;
    }

    bool returning=false;
    QPointF last(0,0);
    int lastx=0;

    foreach(WWPoint *p, points_) {

        // might be better to always use float?
        QPoint center = transform(p->x,p->y);
        QPointF dot(center.x(), center.y());

        // cursor in ?
        if (last.x() > 0 && c.x() > last.x() && c.x() < dot.x() && dot.x() > last.x()) {

            // found the cursor, but is it in the block?
            QPointF begin(last.x(), canvas().bottom());
            QPainterPath block(begin);
            block.lineTo(last);
            block.lineTo(dot);
            block.lineTo(dot.x(),begin.y());
            block.lineTo(begin);

            // is it inside?
            if (block.contains(c)) {

                // if different then update and want a repaint
                if (cursorBlock != block) {
                    cursorBlock = block;
                    cursorBlockText = time_to_string(p->x - lastx);
                    returning = true;
                }
            } else if (cursorBlock != QPainterPath()) {

                // not inside but not set to null
                cursorBlock = QPainterPath();
                returning = true;
            }
            break;
        }

        // moving on
        last = dot;
        lastx = p->x;
    }
    return returning;
}

static bool doubles_equal(double a, double b)
{
    double errorB = b * DBL_EPSILON;
    return (a >= b - errorB) && (a <= b + errorB);
}

// move the selected points in he direction of the cursor key
// constrained to the limits of the points not selected
bool
WorkoutWidget::movePoints(int key, Qt::KeyboardModifiers kmod)
{
    Q_UNUSED(kmod); // perhaps in the future...

    // apply constraints, don't move if we are constrained
    // by moving off the scale or create a workout that is
    // invalid (e.g. time goes backwards
    bool constrained = false;

    for(int index=0; index < points_.count(); index++) {

        WWPoint *p = points_[index];

        // going left
        if (key == Qt::Key_Left) {

            // look at prev
            WWPoint *prev = index ? points_[index-1] : NULL;

            // hit the start of the workout
            if (p->selected && (p->x-XMOVE) < 0) { constrained=true; break; }

            // hit the previous unselected
            if (p->selected && prev && !prev->selected && prev->x > (p->x-XMOVE)) {
                constrained=true; break;
            }
        }

        // going right
        if (key == Qt::Key_Right) {

            // look at next
            WWPoint *next = index+1 < points_.count() ? points_[index+1] : NULL;

            // hit the end of the workout
            if (p->selected && (p->x+XMOVE) > maxX()) { constrained=true; break; }

            // hit the next unselected
            if (p->selected && next && !next->selected && next->x < (p->x+XMOVE)) {
                constrained=true; break;
            }
        }

        // going down
        if (key == Qt::Key_Down) {

            // hit zero
            if (p->selected && (p->y-YMOVE) < 0) { constrained=true; break; }
        }
    }

    // no dice, skip update we changed nothing
    if (constrained) return false;

    // create command as we go
    QList<PointMemento> before, after;

    // visit every point and move if it is selected
    for(int index=0; index < points_.count(); index++) {

        WWPoint *p = points_[index];

        // just selected ones
        if (p->selected == false) continue;

        // add before
        before << PointMemento(p->x, p->y, index);

        switch(key) {

        case Qt::Key_Up:
            p->y += YMOVE;
            break;

        case Qt::Key_Down:
            p->y -= YMOVE;
            break;

        case Qt::Key_Left:
            p->x -= XMOVE;
            break;

        case Qt::Key_Right:
            p->x += XMOVE;
            break;
        }

        // add after
        after << PointMemento(p->x, p->y, index);
    }

    if (before.count()) {
        new MovePointsCommand(this, before, after);
        return true;
    }

    return false;
}

bool
WorkoutWidget::movePoint(QPoint p)
{
    int index = points_.indexOf(dragging); // XXX optimise this out on set drag state
    QPoint f = transform(dragging->x, dragging->y); // current loc of point
    QPointF to = reverseTransform(p.x(), p.y()); // watts/secs to move to

    // boom .. we don't exist!
    if (index == -1) return false;

    // moving left
    if (p.x() < f.x()) {

        // we are constrained by another point?
        if (index > 0) {

            // get constraining point to the left
            QPoint c = transform(points_[index-1]->x, points_[index-1]->y); // current loc of point

            // not beyond just move, otherwise align to constraint
            if (c.x() < p.x()) dragging->x = to.x();
            else dragging->x = points_[index-1]->x;

        } else {

            // unconstrained
            dragging->x = to.x();

        }
    }

    // moving right
    if (p.x() > f.x()) {

        // we are constrained by another point?
        if ((index+1) < points_.count()) {

            // get constraining point to the right
            QPoint c = transform(points_[index+1]->x, points_[index+1]->y); // current loc of point

            // not beyond just move, otherwise align to constraint
            if (c.x() > p.x()) dragging->x = to.x();
            else dragging->x = points_[index+1]->x;

        } else {

            // unconstrained
            dragging->x = to.x();

        }
    }

    // we don't constrain y, but highlight points that align
    foreach(WWPoint *point, points_) {
        if (point == dragging) continue;

        point->hover=false;
        if (doubles_equal(point->y, to.y())) point->hover = true;
    }
    dragging->y = to.y();
    return true;
}

bool
WorkoutWidget::createPoint(QPoint p)
{
    // add a point!
    QPointF to = reverseTransform(p.x(), p.y());

    // don't auto append, we are going to insert     vvvvv
    WWPoint *add = new WWPoint(this, to.x(), to.y(), false);

    onDrag = QPointF(add->x, add->y);      // yuk, this should be done in the FSM (eventFilter)
                                           // action at a distance ... yuk XXX TIDY THIS XXX
    // add into the points
    for(int i=0; i<points_.count(); i++) {
        if (points_[i]->x > to.x()) {
            points_.insert(i, add);
            new CreatePointCommand(this, to.x(), to.y(), i);

            // enter drag mode -- add command resets we
            // are an edge case so handle it ourselves
            state = drag;
            dragging = add;
            return true;
        }
    }

    // after current
    points_.append(add);
    new CreatePointCommand(this, to.x(), to.y(), -1);

    // enter drag mode - edge case as above
    state = drag;
    dragging = add;
    return true;
}

bool
WorkoutWidget::scale(QPoint p)
{
    // scale selected (all at present) points
    // up of y is positive, down 1% if y is negative
    if (p.y() == 0) return false;

    double factor = p.y() > 0 ? 1.01 : 0.99;

    // scale
    foreach (WWPoint *p, points_) p->y *= factor;

    // register command
    new ScaleCommand(this, 1.01, 0.99, p.y() > 0);

    return true;
}

bool
WorkoutWidget::deleteSelected()
{
    // get a work list for the command then delete them backwards
    QList<PointMemento> list;
    for(int i=0; i<points_.count(); i++) {
        WWPoint *p = points_[i];
        if (p->selected) list << PointMemento(p->x, p->y, i);
    }

    // run the command instead of duplicating here :)
    for (int j=list.count()-1; j>=0; j--) {
        PointMemento m = list[j];
        WWPoint *rm = points_.takeAt(m.index);
        delete rm;
    }

    // create a command on stack
    new DeleteWPointsCommand(this, list);

    return true;
}

bool
WorkoutWidget::selectPoints()
{
    // if points are in rectangle then set selecting
    // if they are not then unset selecting
    QRectF rect(onRect,atRect);
    foreach(WWPoint *p, points_) {

        // experiment with deselecting when using a 
        // rect selection tool since more often than
        // not I keep forgetting points are highlighted
        // XXX maybe a keyboard modifier in the future ?
        p->selected=false;

        if (p->bounding().intersects(rect))
            p->selecting = true;
        else
            p->selecting = false;
    }
    return true;
}

bool
WorkoutWidget::selectedPoints()
{
    // any points marked as selecting are now selected
    foreach(WWPoint *p, points_) {
        if (p->selecting) {
            p->selected=true;
            p->selecting=false;
        }
    }
    return true;
}

bool
WorkoutWidget::selectClear()
{
    // clear all selection
    foreach(WWPoint *p, points_) {
        p->selected=false;
        p->selecting=false;
    }
    return true;
}

void
WorkoutWidget::ergFileSelected(ErgFile *ergFile)
{
    // reset state and stack
    state = none;
    dragging = NULL;
    foreach (WorkoutWidgetCommand *p, stack) delete p;
    stack.clear();
    stackptr = 0;
    parent->undoAct->setEnabled(false);
    parent->redoAct->setEnabled(false);
    //XXX consider refactoring this !!! XXX

    // wipe out points
    foreach(WWPoint *point, points_) delete point;
    points_.clear();

    // we suport ERG but not MRC/CRS currently
    if (ergFile && ergFile->format == ERG) {

        maxX_=0;
        maxY_=300;

        // add points for this....
        foreach(ErgFilePoint point, ergFile->Points) {
            WWPoint *add = new WWPoint(this, point.x / 1000.0f, point.y); // in ms
            if (add->x > maxX_) maxX_ = add->x;
            if (add->y > maxY_) maxY_ = add->y;
        }

        maxY_ *= 1.1f;
    }

    // reset metrics etc
    recompute();

    // repaint
    repaint();
}

void
WorkoutWidget::recompute()
{
    //QTime timer;
    //timer.start();

    int rnum=-1;
    if (context->athlete->zones(false) == NULL ||
        (rnum = context->athlete->zones(false)->whichRange(QDate::currentDate())) == -1) {

        // no cp or ftp set
        parent->TSSlabel->setText("- TSS");
        parent->IFlabel->setText("- IF");

    }

    //
    // PREPARE DATA
    //

    // get CP/FTP to use in calculation
    int WPRIME = context->athlete->zones(false)->getWprime(rnum);
    int CP = context->athlete->zones(false)->getCP(rnum);
    int FTP = context->athlete->zones(false)->getFTP(rnum);
    bool useCPForFTP = (appsettings->cvalue(context->athlete->cyclist,
                        context->athlete->zones(false)->useCPforFTPSetting(), 0).toInt() == 0);
    if (useCPForFTP) FTP=CP;

    // compute the metrics based upon the data...
    QVector<int> wattsArray;

    // running time and watts for interpolating
    int ctime = 0;
    double cwatts = 0;

    // resample the erg file into 1s samples
    foreach(WWPoint *p, points_) {

        // ramprate per second
        double ramp = double(p->y - cwatts) / double(p->x - ctime);

        while (ctime < p->x) {
            cwatts += ramp;
            ctime++;
            wattsArray << int(cwatts);
        }

        cwatts = p->y;
    }


    //
    // COMPUTE KEY METRICS TSS/IF
    //

    // The Workout Window has labels for TSS and IF.
    double NP=0, TSS=0, IF=0;

    // calculating NP
    QVector<int> NProlling(30);
    NProlling.fill(0,30);
    double NPtotal=0;
    double NPsum=0;
    int NPindex=0;
    int NPcount=0;

    foreach(int watts, wattsArray) {

        //
        // Normalised Power
        //

        // sum last 30secs
        NPsum += watts;
        NPsum -= NProlling[NPindex];
        NProlling[NPindex] = watts;

        // running total and count
        NPtotal += pow(NPsum/30,4); // raise rolling average to 4th power
        NPcount ++;

        // it moves up and down during the ride
        if (NPcount > 30) {
            NP = pow(double(NPtotal) / double(NPcount), 0.25f);
        }

        // move index on/round
        NPindex = (NPindex >= 29) ? 0 : NPindex+1;
    }

    // IF.....
    IF = double(NP) / double(FTP);

    // TSS.....
    double normWork = NP * NPcount;
    double rawTSS = normWork * IF;
    double workInAnHourAtCP = FTP * 3600;
    TSS = rawTSS / workInAnHourAtCP * 100.0;

    parent->IFlabel->setText(QString("%1 IF").arg(IF, 0, 'f', 2));
    parent->TSSlabel->setText(QString("%1 TSS").arg(TSS, 0, 'f', 0));

    //
    // COMPUTE W'BAL
    //
    wpBal.setWatts(context, wattsArray, CP, WPRIME);

    //
    // MEAN MAX [works but need to think about UI]
    //
    //QVector<int>mmpArray;
    //RideFileCache::fastSearch(wattsArray, mmpArray);
    //qDebug()<<"RECOMPUTE:"<<timer.elapsed()<<"ms"<<wattsArray.count()<<"samples";
}

void
WorkoutWidget::configChanged(qint32)
{
    setProperty("color", GColor(CTRAINPLOTBACKGROUND));

    gridPen = QPen(GColor(CPLOTGRID));
    gridPen.setWidth(1);

    markerPen = QPen(GColor(CPLOTMARKER));
    markerPen.setWidth(1);

    markerFont.fromString(appsettings->value(this, GC_FONT_CHARTLABELS, QFont().toString()).toString());
    markerFont.setPointSize(appsettings->value(NULL, GC_FONT_CHARTLABELS_SIZE, 8).toInt());

    bigFont = markerFont;
    bigFont.setPointSize(markerFont.pointSize() * 2);
    bigFont.setWeight(QFont::Bold);

    repaint();
}

void
WorkoutWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.save();

    // use antialiasing when drawing
    painter.setRenderHint(QPainter::Antialiasing, true);

    // just for debug for now
    //DEBUG painter.setPen(QPen(QColor(255,255,255,30)));
    //DEBUG painter.drawRect(left());
    //DEBUG painter.drawRect(right());
    //DEBUG painter.drawRect(top());
    //DEBUG painter.drawRect(bottom());
    //DEBUG painter.setPen(QPen(QColor(255,0,0,30)));
    //DEBUG painter.drawRect(canvas());

    //
    // SCALES [refactor later]
    //

    // paint the scale backbones
    painter.setPen(markerPen);
    painter.setFont(markerFont);
    QFontMetrics fontMetrics(markerFont);

    // backbone
    if (YTICLENGTH) painter.drawLine(left().topRight(), left().bottomRight()); //Y
    if (XTICLENGTH) painter.drawLine(bottom().topLeft(), bottom().topRight()); //X

    // start with 5 min tics and get longer and longer
    int tsecs = 5 * 60; // 5 minute tics
    int xrange = maxX() - minX();
    while (double(xrange) / double(tsecs) > XTICS) tsecs *= 2;

    // now paint them
    for(int i=minX(); i<=maxX(); i += tsecs) {

        painter.setPen(markerPen);

        // paint a tic
        int x= transform(i, 0).x();

        if (XTICLENGTH) { // we can make the tics disappear

            painter.drawLine(QPoint(x, bottom().topLeft().y()),
                             QPoint(x, bottom().topLeft().y()+XTICLENGTH));
        }

        // always paint the label
        QString label = time_to_string(i);
        QRect bound = fontMetrics.boundingRect(label);
        painter.drawText(QPoint(x - (bound.width() / 2),
                                bottom().topLeft().y()+fontMetrics.ascent()+XTICLENGTH+(XTICLENGTH ? SPACING : 0)),
                                label);

    }

    // Y-SCALE (absolute shown on canvas next to grid lines)
    QString label = tr("Intensity");
    QRect box = fontMetrics.boundingRect(label);
    painter.rotate(-90);
    painter.setPen(markerPen);
    //painter.drawText(QPoint(left().x()+box.height(), left().center().y()+(box.width()/2)), label);
    painter.drawText(0,box.height(),label);
    painter.rotate(90);

    // start with 50w tics and get longer and longer
    int twatts = 50; // 50w tics
    int yrange = maxY() - minY();
    while (double(yrange) / double(twatts) > YTICS) twatts *= 2;

    // now paint them
    for(int i=minY(); i<=maxY(); i += twatts) {

        painter.setPen(markerPen);

        // paint a tic
        int y= transform(0, i).y();

        if (YTICLENGTH) { // we can make the tics disappear

        painter.drawLine(QPoint(left().topRight().x(), y),
                         QPoint(left().topRight().x()-YTICLENGTH, y));

        }

        // always paint the watts as a reference
        if (GRIDLINES && i>0) {
            painter.setPen(markerPen);

            QString label = QString("%1w").arg(i);
            QRect bound = fontMetrics.boundingRect(label);
            painter.drawText(QPoint(canvas().left()+SPACING,
                                    y+bound.height()), // we use ascent not height to line up numbers
                                    label);

#if 0       // ONLY SHOW GRIDLINES FROM POWERSCALE
            // GRIDLINES - but not on top and bottom border of canvas
            if (y > canvas().y() && y < canvas().height() && GRIDLINES == true) {
                painter.setPen(gridPen);
                painter.drawLine(QPoint(canvas().x(), y), QPoint(canvas().x()+canvas().width(), y));
            }
#endif
        }
    }

    // now paint the scene and related widgets
    foreach(WorkoutWidgetItem*x, children_) x->paint(&painter);

    // now paint the points
    foreach(WorkoutWidgetItem*x, points_) x->paint(&painter);

    painter.restore();
}

QRectF
WorkoutWidget::left()
{
    QRect all = geometry();
    return QRectF(0,THEIGHT, LWIDTH, all.height() - IHEIGHT - THEIGHT - BHEIGHT);
}

QRectF
WorkoutWidget::right()
{
    QRect all = geometry();
    return QRectF(all.width()-RWIDTH,THEIGHT, RWIDTH, all.height() - IHEIGHT - THEIGHT - BHEIGHT);
}

QRectF
WorkoutWidget::bottom()
{
    QRect all = geometry();
    return QRectF(LWIDTH, all.height() - BHEIGHT, all.width() - LWIDTH - RWIDTH, BHEIGHT);
}

QRectF
WorkoutWidget::top()
{
    QRect all = geometry();
    return QRectF(LWIDTH, 0, all.width() - LWIDTH - RWIDTH, THEIGHT);
}

QRectF
WorkoutWidget::canvas()
{
    QRect all = geometry();
    return QRectF(LWIDTH, THEIGHT, all.width() - LWIDTH - RWIDTH, all.height() - IHEIGHT - THEIGHT - BHEIGHT);
}

WorkoutWidgetItem::WorkoutWidgetItem(WorkoutWidget *w) : w(w)
{
}

double
WorkoutWidget::maxX()
{
    return maxX_;
}

double
WorkoutWidget::maxY()
{
    return maxY_;
}

// transform from plot to painter co-ordinate
QPoint
WorkoutWidget::transform(double seconds, double watts)
{
    // from plot coords to painter coords on the canvas
    QRectF c = canvas();

    // ratio of pixels to plot units
    double yratio = double(c.height()) / (maxY()-minY());
    double xratio = double(c.width()) / (maxX()-minX());

    return QPoint(c.x() + (seconds * xratio), c.bottomLeft().y() - (watts * yratio));
}

// transform from painter to plot co-ordinate
QPointF
WorkoutWidget::reverseTransform(int x, int y)
{
    // from painter coords to plot cords on the canvas
    QRectF c = canvas();

    // ratio of pixels to plot units
    double yratio = double(c.height()) / (maxY()-minY());
    double xratio = double(c.width()) / (maxX()-minX());

    return QPoint((x-c.x()) / xratio, (c.bottomLeft().y() - y) / yratio);
}

WorkoutWidgetCommand::WorkoutWidgetCommand(WorkoutWidget *w) : workoutWidget_(w)
{
    // add us
    w->addCommand(this);
}

// add to the stack, don't execute since it was already executed
// and this is a memento to enable undo / redo - its a trigger
// to recompute metrics though since the model has been changed
void
WorkoutWidget::addCommand(WorkoutWidgetCommand *cmd)
{
    // stop dragging
    dragging = NULL;
    state = none;

    // truncate if needed
    if (stack.count()) {
        // wipe away commands we can no longer redo
        while (stack.count() > stackptr) {
            WorkoutWidgetCommand *p = stack.takeAt(stackptr);
            delete p;
        }
    }

    // add to stack
    stack.append(cmd);
    stackptr++;

    // set undo enabled, redo disabled
    parent->undoAct->setEnabled(true);
    parent->redoAct->setEnabled(false);
}

void
WorkoutWidget::redo()
{
    // stop dragging
    dragging = NULL;
    state = none;

    // redo if we can
    if (stackptr >= 0 && stackptr < stack.count()) stack[stackptr++]->redo();

    // disable/enable buttons
    if (stackptr > 0) parent->undoAct->setEnabled(true);
    if (stackptr >= stack.count()) parent->redoAct->setEnabled(false);

    // recompute metrics
    recompute();

    // update
    update();
}

void
WorkoutWidget::undo()
{
    // stop dragging
    dragging = NULL;
    state = none;

    // run it
    if (stackptr > 0) stack[--stackptr]->undo();

    // disable/enable button
    if (stackptr <= 0) parent->undoAct->setEnabled(false);
    if (stackptr < stack.count()) parent->redoAct->setEnabled(true);

    // recompute metrics
    recompute();

    // update
    update();
}
