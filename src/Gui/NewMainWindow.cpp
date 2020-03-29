#include "NewMainWindow.h"
#include "Colors.h"

#include <QScreen>

// assume this is ok, if not all bets are off.
// we resize for smaller screens but only so you
// can close the app.
constexpr double gl_minwidth = 1024;
constexpr double gl_minheight = 768;
constexpr double gl_toolbarheight = 50;
constexpr double gl_searchwidth = 350;
constexpr double gl_searchheight = 38;
constexpr double gl_quitheight = 38;
constexpr double gl_quitwidth = 38;

NewMainWindow::NewMainWindow(QApplication *app) : QMainWindow(NULL), app(app), state(None)
{
    // setup
    main = new QWidget(this);
    main->setStyleSheet("background: white;");
    main->setAutoFillBackground(true);
    setCentralWidget(main);
    layout = new QVBoxLayout(main);
    layout->setSpacing(0);
    layout->setMargin(0);

    // toolbar
    setupToolbar();

    // more to come...

    // set geometry etc
    initialPosition();
}

void
NewMainWindow::setupToolbar()
{

    // toolbar
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
    searchbox->setStyleSheet("QLineEdit       { background: #eeeeee; border-radius: 7px; }"
                             "QLineEdit:focus { background: white; }");
    searchbox->setPlaceholderText(tr("                    Search or type a command "));

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

    // no frame thanks
    setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);

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
NewMainWindow::isHotSpot(QPoint pos)
{
    if (pos.y() < gl_toolbarheight * dpiYFactor) return true;
    return false;

}

void
NewMainWindow::mousePressEvent(QMouseEvent *event) {

    if (isHotSpot(event->pos())) {
        clickPos.setX(event->x());
        clickPos.setY(event->y());
        state = Drag;
    }
}

void
NewMainWindow::mouseReleaseEvent(QMouseEvent *) {
    state = None;
}

void
NewMainWindow::mouseMoveEvent(QMouseEvent *event) {

    if (state == Drag) move(event->globalX()-clickPos.x(),event->globalY()-clickPos.y());
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
