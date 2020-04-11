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
static int spacing_=4;
#else
static int spacing_=4;
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
    scrollArea->setAutoFillBackground(false);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    scrollArea->setContentsMargins(0,0,0,0);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


    scrollArea->setWidget(buttonBar);

    // scroll area turns it on .. we turn it off!
    buttonBar->setAutoFillBackground(false);

    // scroller buttons
    left = new QToolButton(this);
    left->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    left->setAutoFillBackground(false);
    left->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    left->setIcon(leftIcon);
    left->setIconSize(QSize(20*dpiXFactor,20*dpiYFactor));
    left->setFocusPolicy(Qt::NoFocus);
    mlayout->addWidget(left);
    connect(left, SIGNAL(clicked()), this, SLOT(scrollLeft()));

    // menu bar in the middle of the buttons
    mlayout->addWidget(scrollArea);

    right = new QToolButton(this);
    right->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    right->setAutoFillBackground(false);
    right->setFixedSize(20*dpiXFactor,20*dpiYFactor);
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

    menuButton = new QToolButton(this);
    menuButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    menuButton->setAutoFillBackground(false);
    menuButton->setFixedSize(20*dpiXFactor,20*dpiYFactor);
    menuButton->setIcon(iconFromPNG(":images/sidebar/extra.png"));
    menuButton->setIconSize(QSize(10*dpiXFactor,10*dpiYFactor));
    menuButton->setFocusPolicy(Qt::NoFocus);
    mlayout->addWidget(menuButton);
    //connect(p, SIGNAL(clicked()), action, SLOT(trigger()));

    signalMapper = new QSignalMapper(this); // maps each option
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(clicked(int)));

    barMenu = new QMenu("Add");
    chartMenu = barMenu->addMenu(tr("Add Chart"));

    barMenu->addAction(tr("Import Chart..."), context->mainWindow, SLOT(importChart()));

#ifdef GC_HAS_CLOUD_DB
    barMenu->addAction(tr("Upload Chart..."), context->mainWindow, SLOT(exportChartToCloudDB()));
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

    foreach(ChartBarItem *b, buttons) {
        int width = fs.width(b->text) + (30 * dpiXFactor);
        if (width < (80*dpiXFactor)) width=80*dpiXFactor;
    	b->setFont(buttonFont);
        b->setFixedWidth(width);
        b->setFixedHeight(height);
    }
}

void
ChartBar::addWidget(QString title)
{
    ChartBarItem *newbutton = new ChartBarItem(this);
    newbutton->setText(title);
    newbutton->setFont(buttonFont);

    // make the right size
    QFontMetrics fontMetric(buttonFont);
    int width = fontMetric.width(title) + (30 * dpiXFactor);
    int height = (fontMetric.height()+(spacing_*dpiXFactor));
    if (width < (80*dpiXFactor)) width=80*dpiXFactor;
    newbutton->setFixedWidth(width);
    newbutton->setFixedHeight(height);

    // add to layout
    layout->addWidget(newbutton);
    buttons << newbutton;

    // map signals
    connect(newbutton, SIGNAL(clicked(bool)), signalMapper, SLOT(map()));
    signalMapper->setMapping(newbutton, buttons.count()-1);

    newbutton->installEventFilter(this);

    // tidy up scrollers etc
    tidy();
}

void
ChartBar::setChartMenu()
{
    context->mainWindow->setChartMenu(chartMenu);
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
    int width = fontMetric.width(text) + (30*dpiXFactor);
    buttons[index]->setWidth(width < (80*dpiXFactor) ? (80*dpiXFactor) : width);

    tidy(); // still fit ?
}


// tidy up the scrollers on first show...
void
ChartBar::tidy()
{
    // resize to button widths + 2px spacing
    int width = 2*dpiXFactor;
    foreach (ChartBarItem *button, buttons) {
        width += button->geometry().width() + (2*dpiXFactor);
    }
    buttonBar->setMinimumWidth(width);

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
        tidy();
    }

    // showing us - tidy up
    if (object == this && e->type() == QEvent::Show) {
        tidy();
    }

    // enter/leave we can track approximate mouse position and decide 
    // if we want to 'autoscroll'
    if (e->type() == QEvent::Leave || e->type() == QEvent::Enter) {
        tidy(); // tidy up anyway

        // XXX for later, perhaps when drag/dropping
        //     we should try and be a little more fluid / animate ...
        //     which will probably mean using QScrollArea::ScrollContentsBy
    }

    return false;
}

void
ChartBar::scrollRight()
{
    // scroll to the right...
    int w = buttonBar->width();
    scrollArea->ensureVisible(w-(10*dpiXFactor),0,10*dpiXFactor,10*dpiYFactor);
}

void
ChartBar::scrollLeft()
{
    // scroll to the left
    scrollArea->ensureVisible(0,0,10*dpiXFactor,10*dpiYFactor);
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
    buttons.remove(index);

    // reset mappings
    for (int i=0; i<buttons.count(); i++)
        signalMapper->setMapping(buttons[i], i);

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

    // linear gradients
    QLinearGradient active = GCColor::linearGradient(23*dpiYFactor, true);
    QLinearGradient inactive = GCColor::linearGradient(23*dpiYFactor, false);

    // fill with a linear gradient
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, QColor(Qt::white));
    painter.fillRect(all, isActiveWindow() ? active : inactive);

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

    // linear gradients
    QLinearGradient active = GCColor::linearGradient(23*dpiYFactor, true);
    QLinearGradient inactive = GCColor::linearGradient(23*dpiYFactor, false);

    // fill with a linear gradient
    painter.setPen(Qt::NoPen);
    painter.fillRect(all, QColor(Qt::white));
    painter.fillRect(all, isActiveWindow() ? active : inactive);

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

ChartBarItem::ChartBarItem(QWidget *parent) : QWidget(parent)
{
    red = highlighted = checked = false;
    QFont font;
    font.setPointSize(10);
    //font.setWeight(QFont::Black);
    setFont(font);
}

void
ChartBarItem::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.save();
    painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing, true);

    // widget rectangle
    QRectF body(0,0,width(), height());

    painter.setClipRect(body);
    painter.setPen(Qt::NoPen);

    // background - chrome or slected colour
    QBrush brush(GColor(CCHROME));
    if (checked) brush = QBrush(GColor(CPLOTBACKGROUND));
    painter.fillRect(body, brush);

    // now paint the text
    QPen pen(GCColor::invertColor(brush.color()));
    painter.setPen(pen);
    painter.drawText(body, text, Qt::AlignBottom | Qt::AlignCenter);

    // draw the bar
    if (checked) painter.fillRect(QRect(0,0,geometry().width(), 3*dpiXFactor), QBrush(GColor(CPLOTMARKER)));
    painter.restore();
}

bool
ChartBarItem::event(QEvent *e)
{
    // entry / exit event repaint for hover color
    if (e->type() == QEvent::Leave || e->type() == QEvent::Enter) repaint();
    if (e->type() == QEvent::MouseButtonPress && underMouse()) emit clicked(checked);
    return QWidget::event(e);
}
