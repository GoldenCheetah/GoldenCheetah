#include "NewMainWindow.h"
#include "Colors.h"

#include <QScreen>

// assume this is ok, if not all bets are off.
// we resize for smaller screens but only so you
// can close the app.
constexpr double gl_minwidth = 1024;
constexpr double gl_minheight = 768;
constexpr double gl_toolbarheight = 50;
constexpr double gl_searchwidth = 500;
constexpr double gl_searchheight = 38;
constexpr double gl_quitheight = 38;
constexpr double gl_quitwidth = 38;
constexpr double gl_hoverzone = 10; // how many px to be hovering on an edge?

NewMainWindow::NewMainWindow(QApplication *app) : QMainWindow(NULL), state(Inactive), resize(None), app(app)
{
    // We handle all the aesthetics
#ifdef Q_OS_MAC
    setWindowFlags(windowFlags() | Qt::Window | Qt::WindowSystemMenuHint);
    macNativeSettings();
#else
    setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
#endif


    // setup
    main = new QWidget(this);
    main->setStyleSheet("background: white;");
    main->setAutoFillBackground(true);
    setCentralWidget(main);
    layout = new QVBoxLayout(main);
    layout->setSpacing(0);
    layout->setContentsMargins(QMargins());

    // toolbar
    setupToolbar();

    // set geometry etc
    initialPosition();

    // mouse tracking
    setMouseTracking(true);
    qApp->installEventFilter(this);
}

void
NewMainWindow::setupToolbar()
{
    // toolbar
#ifdef Q_OS_MAC
    toolbar = new QMacToolBar(this);
    this->window()->winId(); // this forces a windowhandle to be created
    toolbar->attachToWindow(this->window()->windowHandle());
#else
    toolbar = new QToolBar(this);
    toolbar->setFixedHeight(gl_toolbarheight * dpiYFactor);
    toolbar->setStyleSheet("background-color: lightGray;");
    toolbar->setAutoFillBackground(true);
    layout->addWidget(toolbar);
    layout->addStretch();

    // searchbox
    searchbox = new QLineEdit(this);
    searchbox->setFixedWidth(gl_searchwidth * dpiXFactor);
    searchbox->setFixedHeight(gl_searchheight * dpiXFactor);
    searchbox->setFrame(QFrame::NoFrame);
    searchbox->setStyleSheet("QLineEdit       { background: #eeeeee; border-radius: 7px; padding-left: 10px;}" //XXX hidpi constants
                             "QLineEdit:focus { background: white; }");
    searchbox->setPlaceholderText(tr("                                      Search or type a command "));

    // spacer to balance the minimize and close buttons
    QWidget *balanceSpacer = new QWidget(this);
    balanceSpacer->setFixedSize(gl_quitwidth*dpiXFactor * 2, gl_quitheight*dpiYFactor);
    balanceSpacer->setVisible(true);
    balanceSpacer->setAutoFillBackground(false);

    QWidget *leftSpacer = new QWidget(this);
    leftSpacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    leftSpacer->setVisible(true);
    leftSpacer->setAutoFillBackground(false);

    QWidget *rightSpacer = new QWidget(this);
    rightSpacer->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
    rightSpacer->setVisible(true);
    rightSpacer->setAutoFillBackground(false);

    toolbar->addWidget(balanceSpacer);
    toolbar->addWidget(leftSpacer);
    toolbar->addWidget(searchbox);
    toolbar->addWidget(rightSpacer);

    // and close push buttons on the right
    minimisebutton = new QPushButton(this);
    minimisebutton->setAutoFillBackground(true);
    minimisebutton->setFixedSize(gl_quitwidth * dpiXFactor, gl_quitheight *dpiXFactor);
    minimisebutton->setStyleSheet("QPushButton { border: none; background-color: lightGray; color: white; margin: 3px; }"
                                  "QPushButton:hover { background-color: darkGray; color: white; }");
    minimisebutton->setText("_"); // fix with icon later
    toolbar->addWidget(minimisebutton);

    // and close push buttons on the right
    quitbutton = new QPushButton(this);
    quitbutton->setAutoFillBackground(true);
    quitbutton->setFixedSize(gl_quitwidth * dpiXFactor, gl_quitheight *dpiXFactor);
    quitbutton->setStyleSheet("QPushButton { border: none; background-color: lightGray; color: white; margin: 3px; }"
                                  "QPushButton:hover { background-color: red; color: white; }");

    quitbutton->setText("X"); // fix with icon later
    toolbar->addWidget(quitbutton);

    // connect up
    connect(quitbutton, SIGNAL(clicked()), this, SLOT(close()));
    connect(minimisebutton, SIGNAL(clicked()), this, SLOT(minimizeWindow()));

    // remove focus from linedit
    setFocus();

    toolbar->show();
#endif
}

void
NewMainWindow::initialPosition()
{
    // so we might have multiple screens, for now drop onto the largest we
    // can find, but will fix that later - so you can choose
    const QScreen *biggest=NULL;
    foreach(const QScreen *screen, app->screens()) {
        if (biggest == NULL || screen->availableGeometry().width() > biggest->availableGeometry().width())
            biggest = screen;
    }

    // absolute minimum - for modern displays this is nothing
    setMinimumWidth(1024 *dpiXFactor);
    setMinimumHeight(600 *dpiYFactor);

    // lets go
    show();


    // center on the biggest screen
    windowHandle()->setScreen(const_cast<QScreen*>(biggest));

    // work out where geom should be
    QRect geom(0,0, gl_minwidth *dpiXFactor, gl_minheight *dpiYFactor);
    geom.moveCenter(biggest->availableGeometry().center());
    move(geom.x(), geom.y());
}

// mouse events when no window manager to help us
// start with drag, but will need resize at some
// point too.
bool
NewMainWindow::isDragHotSpot(QPoint pos)
{
#ifdef Q_OS_MAC
    // we don't handle resize on Mac
    return false;
#endif
    if (pos.y() > gl_hoverzone && pos.y() < gl_toolbarheight * dpiYFactor) return true;
    return false;

}

int
NewMainWindow::isResizeHotSpot(QPoint pos)
{
    int returning = None;

#ifdef Q_OS_MAC
    // we don't handle resize on Mac
    return returning;
#endif

    if (pos.y() < gl_hoverzone) {
        returning = Top;
        if (pos.x() < gl_hoverzone) returning = TL;
        if (pos.x() > width()-gl_hoverzone) returning = TR;
    } else if (pos.y() > height() -gl_hoverzone) {
        returning = Bottom;
        if (pos.x() < gl_hoverzone) returning = BL;
        if (pos.x() > width()-gl_hoverzone) returning = BR;
    } else if (pos.x() <gl_hoverzone) {
        returning = Left;
    } else if (pos.x() > width()-gl_hoverzone) {
        returning = Right;
    }

    return returning;
}

void
NewMainWindow::mousePressEvent(QMouseEvent *event) {

    // always grab focus, user is 'clicking away' from a widget
    setFocus();

    if (state == EdgeHover) {

        clickPos.setX(event->x());
        clickPos.setY(event->y());
        clickGeom=geometry();
        state = Resize;
        resize = static_cast<ResizeType>(isResizeHotSpot(event->pos()));

    } else if (isDragHotSpot(event->pos())) {

        clickPos.setX(event->x());
        clickPos.setY(event->y());
        clickGeom=geometry();
        state = Drag;
    }
}

void
NewMainWindow::mouseReleaseEvent(QMouseEvent *) {

    state = Inactive;
    setCursor(Qt::ArrowCursor);
}

bool
NewMainWindow::eventFilter(QObject *, QEvent *e)
{
    if (e->type() == QEvent::MouseMove) this->mouseMoveEvent(static_cast<QMouseEvent*>(e));

    // we must NEVER steal events, this is on the entire app
    return false;
}

void
NewMainWindow::mouseMoveEvent(QMouseEvent *event) {

    int hotspot=None;

    if (state == Drag) {

        move(event->globalX()-clickPos.x(),event->globalY()-clickPos.y());

    } else if (state == Resize) {

        this->setUpdatesEnabled(false);

        QRect geom = geometry();
        switch(resize) {
        case None:
            // should never get here !
            state = Inactive;
            break;
        case BR:
            QMainWindow::resize(event->globalX()-geom.x(),event->globalY()-geom.y());
            break;
        case Right:
            QMainWindow::resize(event->globalX()-geom.x(),geom.height());
            break;
        case Bottom:
            QMainWindow::resize(geom.width(),event->globalY()-geom.y());
            break;
        case TR:
            {
            int newwidth = event->globalX()-geom.x();
            newwidth = newwidth > minimumWidth() ? newwidth : minimumWidth();
            int newheight =  clickGeom.height() + (clickGeom.y() - event->globalY());
            newheight = newheight > minimumHeight() ? newheight : minimumHeight();

            QMainWindow::resize(newwidth, newheight);
            move(clickGeom.bottomLeft().x(),clickGeom.bottomLeft().y()-newheight);
            }
            break;
        case TL:
            {
            int newwidth = clickGeom.width() + (clickGeom.x() - event->globalX());
            newwidth = newwidth > minimumWidth() ? newwidth : minimumWidth();
            int newheight =  clickGeom.height() + (clickGeom.y() - event->globalY());
            newheight = newheight > minimumHeight() ? newheight : minimumHeight();

            QMainWindow::resize(newwidth, newheight);
            move(clickGeom.bottomRight().x()-newwidth,clickGeom.bottomRight().y()-newheight);
            }
            break;
        case BL:
            {
            int newwidth = clickGeom.width() + (clickGeom.x() - event->globalX());
            newwidth = newwidth > minimumWidth() ? newwidth : minimumWidth();
            int newheight =  event->globalY()-geom.y();
            newheight = newheight > minimumHeight() ? newheight : minimumHeight();

            QMainWindow::resize(newwidth, newheight);
            move(clickGeom.bottomRight().x()-newwidth,clickGeom.y());
            }
            break;

        case Left:
            {
            int newwidth = clickGeom.width() + (clickGeom.x() - event->globalX());
            newwidth = newwidth > minimumWidth() ? newwidth : minimumWidth();

            QMainWindow::resize(newwidth, clickGeom.height());
            move(clickGeom.bottomRight().x()-newwidth,clickGeom.y());
            }
            break;

        case Top:
            {
            int newheight =  clickGeom.height() + (clickGeom.y() - event->globalY());
            newheight = newheight > minimumHeight() ? newheight : minimumHeight();

            QMainWindow::resize(clickGeom.width(), newheight);
            move(clickGeom.x(),clickGeom.bottomRight().y()-newheight);
            }
            break;

        default:
            break;
        }

        this->setUpdatesEnabled(true);

    }  else if (state != Resize && state != Drag) {

        hotspot=isResizeHotSpot(event->pos());

        switch (hotspot) {
        case  None:

            // unhover
            setCursor(Qt::ArrowCursor);
            break;
        case  Top:
        case  Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case  Left:
        case  Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case  TL:
        case  BR:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case  TR:
        case  BL:
            setCursor(Qt::SizeBDiagCursor);
            break;
        }

        // state transition
        if (state != EdgeHover && hotspot != None) state = EdgeHover;
        if (state == EdgeHover && hotspot == None) state = Inactive;
    }
}

void
NewMainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->y() < gl_toolbarheight * dpiYFactor) {
        if (windowState() & Qt::WindowMaximized) setWindowState(windowState() & ~Qt::WindowMaximized);
        else setWindowState(Qt::WindowMaximized);
    }
}

void
NewMainWindow::minimizeWindow()
{
    setWindowState(Qt::WindowMinimized);
}
