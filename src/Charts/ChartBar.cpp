/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "ChartBar.h"
#include "DiaryWindow.h"
#include "DiarySidebar.h"
#include "Context.h"

#include <QFontMetrics>

#ifdef Q_OS_MAC
static int spacing_=12;
#else
static int spacing_=8;
#endif
ChartBar::ChartBar(Context *context) : QWidget(context->mainWindow), context(context)
{
    // left / right scroller icon
    static QIcon leftIcon = iconFromPNG(":images/mac/left.png");
    static QIcon rightIcon = iconFromPNG(":images/mac/right.png");

    setContentsMargins(0,0,0,0);

    // main layout
    QHBoxLayout *mlayout = new QHBoxLayout(this);
    mlayout->setSpacing(0);
    mlayout->setContentsMargins(0,0,0,0);

    // buttonBar Widget
    buttonBar = new ButtonBar(this);
    buttonBar->setContentsMargins(0,0,0,0);

    QHBoxLayout *vlayout = new QHBoxLayout(buttonBar); 
    vlayout->setSpacing(0);
    vlayout->setContentsMargins(0,0,0,0);

    layout = new QHBoxLayout;
    layout->setSpacing(2 *dpiXFactor);
    layout->setContentsMargins(0,0,0,0);
    vlayout->addLayout(layout);
    vlayout->addStretch();

    // scrollarea
    scrollArea = new QScrollArea(this);
    scrollArea->setAutoFillBackground(true);
    scrollArea->setWidgetResizable(false);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setContentsMargins(0,0,0,0);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidget(buttonBar);

    // scroll area turns it on .. we turn it off!
    buttonBar->setAutoFillBackground(false);

    // for scrolling buttonbar
    anim = new QPropertyAnimation(buttonBar, "pos", this);

    // scroller buttons
    left = new QPushButton(this);
    left->setAutoFillBackground(false);
    left->setFixedSize(24*dpiXFactor,24*dpiYFactor);
    left->setIcon(leftIcon);
    left->setIconSize(QSize(20*dpiXFactor,20*dpiYFactor));
    left->setFocusPolicy(Qt::NoFocus);
    mlayout->addWidget(left);
    connect(left, SIGNAL(clicked()), this, SLOT(scrollLeft()));

    // menu bar in the middle of the buttons
    mlayout->addWidget(scrollArea);

    right = new QPushButton(this);
    right->setAutoFillBackground(false);
    right->setFixedSize(24*dpiXFactor,24*dpiYFactor);
    right->setIcon(rightIcon);
    right->setIconSize(QSize(20*dpiXFactor,20*dpiYFactor));
    right->setFocusPolicy(Qt::NoFocus);
    mlayout->addWidget(right);
    connect(right, SIGNAL(clicked()), this, SLOT(scrollRight()));

    // spacer to make the menuButton on the right
    QLabel *spacer = new QLabel("", this);
    spacer->setAutoFillBackground(false);
    spacer->setFixedHeight(20*dpiYFactor);
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    mlayout->addWidget(spacer);

    menuButton = new QPushButton(this);
    menuButton->setAutoFillBackground(false);
    menuButton->setFixedSize(24*dpiXFactor,24*dpiYFactor);
    menuButton->setIcon(iconFromPNG(":images/sidebar/plus.png", QSize(12*dpiXFactor, 12*dpiXFactor)));
    menuButton->setIconSize(QSize(20*dpiXFactor,20*dpiYFactor));
    menuButton->setFocusPolicy(Qt::NoFocus);
    mlayout->addWidget(menuButton);
    //connect(p, SIGNAL(clicked()), action, SLOT(trigger()));

    signalMapper = new QSignalMapper(this); // maps each option
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(clicked(int)));

    menuMapper = new QSignalMapper(this); // maps each option
    connect(menuMapper, SIGNAL(mapped(int)), this, SLOT(triggerContextMenu(int)));

    barMenu = new QMenu("Add");
    chartMenu = barMenu->addMenu(tr("New Chart"));

    barMenu->addAction(tr("Import Chart ..."), context->mainWindow, SLOT(importChart()));

#ifdef GC_HAS_CLOUD_DB
    barMenu->addAction(tr("Download Chart..."), context->mainWindow, SLOT(addChartFromCloudDB()));
#endif

    // menu
    connect(menuButton, SIGNAL(clicked()), this, SLOT(menuPopup()));
    connect(chartMenu, SIGNAL(aboutToShow()), this, SLOT(setChartMenu()));
    connect(chartMenu, SIGNAL(triggered(QAction*)), context->mainWindow, SLOT(addChart(QAction*)));

    // trap resize / mouse events
    installEventFilter(this);

    // appearance update
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    configChanged(0);
}

void
ChartBar::configChanged(qint32)
{
    buttonFont = QFont();
    QFontMetrics fs(buttonFont);
    int height = (fs.height()+(spacing_*dpiXFactor));

    setFixedHeight(height);
    scrollArea->setFixedHeight(height);
    buttonBar->setFixedHeight(height);

    QColor col=GColor(CCHARTBAR);
    scrollArea->setStyleSheet(QString("QScrollArea { background: rgb(%1,%2,%3); }").arg(col.red()).arg(col.green()).arg(col.blue()));

    foreach(ChartBarItem *b, buttons) {
        int width = fs.width(b->text) + (60 * dpiXFactor);
        if (width < (90*dpiXFactor)) width=90*dpiXFactor;
    	b->setFont(buttonFont);
        b->setFixedWidth(width);
        b->setFixedHeight(height);
    }

    QString buttonstyle = QString("QPushButton { border: none; border-radius: %2px; background-color: %1; "
                                                "padding-left: 0px; padding-right: 0px; "
                                                "padding-top:  0px; padding-bottom: 0px; }"
                                  "QPushButton:hover { background-color: %3; }"
                                  "QPushButton:hover:pressed { background-color: %3; }"
                                ).arg(GColor(CCHARTBAR).name()).arg(3 * dpiXFactor).arg(GColor(CHOVER).name());
    menuButton->setStyleSheet(buttonstyle);
    left->setStyleSheet(buttonstyle);
    right->setStyleSheet(buttonstyle);
}

void
ChartBar::addWidget(QString title)
{
    ChartBarItem *newbutton = new ChartBarItem(this);
    newbutton->setText(title);
    newbutton->setFont(buttonFont);

    // make the right size
    QFontMetrics fontMetric(buttonFont);
    int width = fontMetric.width(title) + (60 * dpiXFactor);
    int height = (fontMetric.height()+(spacing_*dpiXFactor));
    if (width < (90*dpiXFactor)) width=90*dpiXFactor;
    newbutton->setFixedWidth(width);
    newbutton->setFixedHeight(height);

    // add to layout
    layout->addWidget(newbutton);
    buttons << newbutton;

    // map signals
    connect(newbutton, SIGNAL(clicked(bool)), signalMapper, SLOT(map()));
    connect(newbutton, SIGNAL(contextMenu()), menuMapper, SLOT(map()));
    signalMapper->setMapping(newbutton, buttons.count()-1);
    menuMapper->setMapping(newbutton, buttons.count()-1);

    newbutton->installEventFilter(this);

    // tidy up scrollers etc
    tidy(true);
}

void
ChartBar::setChartMenu()
{
    context->mainWindow->setChartMenu(chartMenu);
}

void
ChartBar::triggerContextMenu(int index)
{
    QPoint tl = buttons[index]->geometry().topLeft();
    int x = scrollArea->widget()->mapToGlobal(tl).x();
    emit contextMenu(index, x);
}

void
ChartBar::menuPopup()
{
    // set the point for the menu and call below
    barMenu->exec(this->mapToGlobal(QPoint(menuButton->pos().x(), menuButton->pos().y()+20)));
}

void
ChartBar::setText(int index, QString text)
{
    buttons[index]->setText(text);
    QFontMetrics fontMetric(buttonFont);
    int width = fontMetric.width(text) + (60*dpiXFactor);
    buttons[index]->setWidth(width < (90*dpiXFactor) ? (90*dpiXFactor) : width);
    buttons[index]->update();

    tidy(true); // still fit ?
}

void
ChartBar::setColor(int index, QColor color)
{
    buttons[index]->setColor(color);
}


// tidy up the scrollers on first show...
void
ChartBar::tidy(bool setwidth)
{
    // resize to button widths + 2px spacing
    if (setwidth) {
        int width = 2*dpiXFactor;
        foreach (ChartBarItem *button, buttons) {
            width += button->geometry().width() + (2*dpiXFactor);
        }
        buttonBar->setFixedWidth(width);
    }

    if (buttonBar->width() > scrollArea->width()) {
        left->show(); right->show();
    } else {
        left->hide(); right->hide();
    }
}

bool 
ChartBar::eventFilter(QObject *object, QEvent *e)
{
    // show/hide scrollers on resize event
    if (object == this && e->type() == QEvent::Resize) {

        // we do NOT move the position, we just show/hide
        // the left and right scrollers
        tidy(false);
    }

    // showing us - tidy up
    if (object == this && e->type() == QEvent::Show) {
        tidy(false);
    }

    // enter/leave we can track approximate mouse position and decide 
    // if we want to 'autoscroll'
    if (e->type() == QEvent::Leave || e->type() == QEvent::Enter) {
        tidy(false); // tidy up anyway
    }

    return false;
}

void
ChartBar::scrollRight()
{

    // old position and pos we want
    QPoint opos = buttonBar->pos();
    QPoint pos = opos;

    // just do a 3rd at a time, so we can see it move
    pos.setX(pos.x() - (scrollArea->geometry().width()/3));

    // constrain to just fit
    if (pos.x() + buttonBar->geometry().width() < scrollArea->geometry().width())
        pos.setX(scrollArea->geometry().width() - buttonBar->geometry().width());

    // animated scroll
    anim->setDuration(400);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    anim->setStartValue(opos);
    anim->setEndValue(pos);
    anim->start();
}

void
ChartBar::scrollLeft()
{
    // old and new position
    QPoint opos = buttonBar->pos();
    QPoint pos = opos;

    // a 3rd at a time
    pos.setX(pos.x() + (scrollArea->geometry().width()/3));
    if (pos.x() > 0) pos.setX(0);

    // animated scroll
    anim->setDuration(400);
    anim->setEasingCurve(QEasingCurve::InOutQuad);
    anim->setStartValue(opos);
    anim->setEndValue(pos);
    anim->start();
}

void
ChartBar::clear()
{
    foreach(ChartBarItem *button, buttons) {
        layout->removeWidget(button);
        delete button;
    }
    buttons.clear();
}

void
ChartBar::removeWidget(int index)
{
    layout->removeWidget(buttons[index]);
    delete buttons[index];
    buttons.takeAt(index);

    // reset mappings
    for (int i=0; i<buttons.count(); i++) {
        signalMapper->setMapping(buttons[i], i);
        menuMapper->setMapping(buttons[i], i);
    }
    tidy(true);
}

void
ChartBar::setCurrentIndex(int index)
{
    clicked(index);
}

void
ChartBar::clicked(int index)
{
    setUpdatesEnabled(false);
    // set selected
    for(int i=0; i<buttons.count(); i++) {
        buttons[i]->setChecked(i == index);
    }
    setUpdatesEnabled(true);
    emit currentIndexChanged(index);
}


ChartBar::~ChartBar() { }

void
ChartBar::paintEvent (QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
    QWidget::paintEvent(event);
}

// paint is the same as sidebar
void
ChartBar::paintBackground(QPaintEvent *)
{
    // setup a painter and the area to paint
    QPainter painter(this);

    painter.save();
    QRect all(0,0,width(),height());

    painter.fillRect(all, GColor(CCHARTBAR));

    painter.restore();
}

void
ButtonBar::paintEvent(QPaintEvent *event)
{
    // paint the darn thing!
    paintBackground(event);
    QWidget::paintEvent(event);
}

// paint is the same as sidebar
void
ButtonBar::paintBackground(QPaintEvent *)
{
    // setup a painter and the area to paint
    QPainter painter(this);

    painter.save();
    QRect all(0,0,width(),height());

    // fill with a linear gradient
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, QColor(Qt::white));
    painter.fillRect(all, GColor(CCHARTBAR));

    if (!GCColor::isFlat()) {
        QPen black(QColor(100,100,100,200));
        painter.setPen(black);
        painter.drawLine(0,height()-1, width()-1, height()-1);

        QPen gray(QColor(230,230,230));
        painter.setPen(gray);
        painter.drawLine(0,0, width()-1, 0);
    }

    painter.restore();
}

ChartBarItem::ChartBarItem(ChartBar *chartbar) : QWidget(chartbar), chartbar(chartbar)
{
    red = highlighted = checked = false;
    state = Idle;
    QFont font;
    font.setPointSize(10);
    setFont(font);
    setMouseTracking(true);
}

void
ChartBarItem::paintEvent(QPaintEvent *)
{
    if (state == Drag) return; // invisible when dragging

    QPainter painter(this);
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing, true);

    // widget rectangle
    QRectF body(0,0,width(), height());

    painter.setClipRect(body);
    painter.setPen(Qt::NoPen);

    // background - chrome or slected colour
    QBrush brush(GColor(CCHARTBAR));
    if (underMouse() && !checked) brush = GColor(CHOVER);
    if (checked) brush = color;
    painter.fillRect(body, brush);

    // now paint the text
    QPen pen(GCColor::invertColor(brush.color()));
    painter.setPen(pen);
    painter.drawText(body, text, Qt::AlignHCenter | Qt::AlignVCenter);

    // draw the bar
    if (checked) {
        // at the top if the chartbar background is different to the plot background
        if (GColor(CCHARTBAR) != color) painter.fillRect(QRect(0,0,geometry().width(), 3*dpiXFactor), QBrush(GColor(CPLOTMARKER)));
        else {
            // only underline the text with a little extra (why adding "XXX" below)
            QFontMetrics fm(font());
            double width = fm.boundingRect(text+"XXX").width();
            painter.fillRect(QRect((geometry().width()-width)/2.0,geometry().height()-(3*dpiXFactor),width, 3*dpiXFactor), QBrush(GColor(CPLOTMARKER)));
        }
    }

    // draw the menu indicator
    if (underMouse()) {

        QPoint mouse = mapFromGlobal(QCursor::pos());
        painter.setPen (Qt :: NoPen);

        if (checked) {

            // different color if under mouse
            QBrush brush(GCColor::invertColor(color));
            if (hotspot.contains(mouse)) brush.setColor(GColor(CPLOTMARKER));
            painter.fillPath (triangle, brush);
        } else {

            // visual clue there is a menu option when tab selected
            QBrush brush(GColor(CHOVER));
            painter.fillPath (triangle, brush);
        }
    }
    painter.restore();
}

int
ChartBarItem::indexPos(int x)
{
    // map via global coord
    QPoint global = mapToGlobal(QPoint(x,0));

    // where in the scrollarea's widget are we now?
    // this is equivalent to chartbar->buttonbar->mapFromGlobal(..)
    int cpos = chartbar->scrollArea->widget()->mapFromGlobal(global).x();

    // work through the layout items to find which widgets
    for(int i=0; i<chartbar->layout->count(); i++) {
        QPoint center = chartbar->layout->itemAt(i)->geometry().center();
        if (center.x() > cpos) return i;
    }
    return -1;
}

bool
ChartBarItem::event(QEvent *e)
{
    // resize?
    if (e->type() == QEvent::Resize) {
        int startx = width() - (25*dpiXFactor);
        int depth = height() / 4;
        int starty = (height() / 2.0) - (depth/2) + 1*dpiXFactor; // middle, taking into account bar at top
        int hs = 3 * dpiXFactor;

        // set the triangle
        hotspot.clear();
        hotspot.moveTo (startx-hs, starty-hs);
        hotspot.lineTo (startx+(hs*2)+(8*dpiXFactor), starty-hs);
        hotspot.lineTo (startx+(hs*2)+(8*dpiXFactor), starty+depth+hs);
        hotspot.lineTo (startx-hs, starty+depth+hs);
        hotspot.lineTo (startx, starty);

        triangle.clear();
        triangle.moveTo (startx, starty);
        triangle.lineTo (startx+(8*dpiXFactor), starty);
        triangle.lineTo (startx+(4*dpiXFactor),   starty+depth);
        triangle.lineTo (startx, starty);
    }

    // entry / exit event repaint for hover color
    if (e->type() == QEvent::Leave || e->type() == QEvent::Enter) {
        update();
    }

    if (e->type() == QEvent::MouseButtonPress && underMouse()) {

        // menu?
        if (checked && hotspot.contains(mapFromGlobal(QCursor::pos()))) {

            // menu activated
            state = Idle;
            emit contextMenu();
            return true; // no more processing of this event please - lots of stuff happens off that menu !

        } else {

            // selected with a click (not release)
            state = Click;
            clickpos.setX(static_cast<QMouseEvent*>(e)->x());
            clickpos.setY(static_cast<QMouseEvent*>(e)->y());
            emit clicked(checked);
        }
    }

    if (e->type() == QEvent::MouseButtonRelease) {

        if (state == Drag) {

            // finish dragging, so drop into where we moved it
            delete dragging;
            int index =  chartbar->layout->indexOf(this);

            if (index != originalindex) {

                // even the button array is used to index by ChartBar::setText(..)
                // and then signal mapped to the index being selected. oh my.
                // bit naughty modding from child here, but no easy way around it.
                ChartBarItem *me = chartbar->buttons.takeAt(originalindex);
                chartbar->buttons.insert(index, me);
                for (int i=0; i<chartbar->buttons.count(); i++) {
                    chartbar->signalMapper->setMapping(chartbar->buttons[i], i);
                    chartbar->menuMapper->setMapping(chartbar->buttons[i], i);
                }

                // tell homewindow
                chartbar->itemMoved(index, originalindex);
            }
            update();
        }
        state = Idle;
    }

    if (e->type() == QEvent::MouseMove) {

        if (state == Click) {

            // enter drag state - moved mouse before releasing the button click
            state = Drag;
            originalindex = chartbar->layout->indexOf(this);
            repaint();

            dragging = new ChartBarItem(chartbar);
            dragging->setColor(color);
            dragging->state = Clone;
            dragging->text = text;
            dragging->checked = checked;
            dragging->setFixedWidth(geometry().width());
            dragging->setFixedHeight(geometry().height());
            QPoint newpos = chartbar->mapFromGlobal(static_cast<QMouseEvent*>(e)->globalPos());
            dragging->move(QPoint(newpos.x()-clickpos.x(),0));
            dragging->show();

        } else if (state == Drag) {

            // move the clone tab for visual feedback
            QPoint newpos = chartbar->mapFromGlobal(static_cast<QMouseEvent*>(e)->globalPos());
            dragging->move(QPoint(newpos.x()-clickpos.x(),0));

            // where are we currently?
            int cindex = chartbar->layout->indexOf(this);

            // work out where we should have dragged to
            int indexpos = indexPos(static_cast<QMouseEvent*>(e)->x());

            // if moving left, just do it...
            if (cindex > indexpos) {
                QLayoutItem *me = chartbar->layout->takeAt(cindex);
                chartbar->layout->insertItem(indexpos, me);
            } else if (indexpos > (cindex+1)) {
                QLayoutItem *me = chartbar->layout->takeAt(cindex);
                chartbar->layout->insertItem(indexpos-1, me); // indexpos-1 because we just got removed
            }
        }
        update(); // in case hovering over stuff
    }
    return QWidget::event(e);
}
