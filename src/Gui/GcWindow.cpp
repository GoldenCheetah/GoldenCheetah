#include "GcWindow.h"
#include "Context.h"
#include "Colors.h"
#include "Settings.h"
#include "Utils.h"
#include "MainWindow.h"
#include "Colors.h"
#include "RideItem.h"
#include "Perspective.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

QWidget *GcWindow::controls() const
{
    return _controls;
}

void GcWindow::setControls(QWidget *x)
{
    _controls = x;
    emit controlsChanged(_controls);

    if (x != NULL) {

        // add any other actions
        if (actions.count()) {
            if (actions.count() > 1) menu->addSeparator();

            foreach(QAction *act, actions) {
                menu->addAction(act->text(), act, SIGNAL(triggered()));
            }

            menu->addSeparator();

        } else {

            menu->addAction(tr("Chart Settings..."), this, SIGNAL(showControls()));
            menu->addSeparator();

        }

        menu->addAction(tr("Remove Chart"), this, SLOT(_closeWindow()));
    }
}

#if 0
QString GcWindow::instanceName() const
{
    return _instanceName;
}

void GcWindow::_setInstanceName(QString x)
{
    _instanceName = x;
}
#endif

QString GcWindow::subtitle() const
{
    return _subtitle;
}

QString GcWindow::title() const
{
    return _title;
}

void GcWindow::setSubTitle(QString x)
{
    _subtitle = x;
    emit subtitleChanged(_title);
    repaint();
}

void GcWindow::setTitle(QString x)
{
    _title = x;
    emit titleChanged(_title);
}

RideItem* GcWindow::rideItem() const
{
    return _rideItem;
}

void GcWindow::setRideItem(RideItem* x)
{
    _rideItem = x;
    emit rideItemChanged(_rideItem);
}

void GcWindow::setDateRange(DateRange dr)
{
    _dr = dr;
    emit dateRangeChanged(_dr);
}

DateRange GcWindow::dateRange() const
{
    return _dr;
}

double GcWindow::widthFactor() const
{
    return _widthFactor;
}

void GcWindow::setWidthFactor(double x)
{
    _widthFactor = x;
    emit widthFactorChanged(x);
}

double  GcWindow::heightFactor() const
{
    return _heightFactor;
}

void GcWindow::setHeightFactor(double x)
{
    _heightFactor = x;
    emit heightFactorChanged(x);
}

void GcWindow::setResizable(bool x)
{
    _resizable = x;
}

bool GcWindow::resizable() const
{
    return _resizable;
}

// if a window moves whilst it is being
// resized we should let go, because the
// geometry tends to warp to stupid or
// tiny sizes as a result
void GcWindow::moveEvent(QMoveEvent *)
{
    if (_noevents) return;

    if (dragState != Move) setDragState(None);
}

void GcWindow::setGripped(bool x)
{
    _gripped = x;
}

bool GcWindow::gripped() const
{
    return _gripped;
}

GcWindow::GcWindow(Context *context) : QFrame(context->mainWidget())
{

    // NO NO NO -- we do NOT destroy on close since we are managed
    // by the application itself.
    //setAttribute(Qt::WA_DeleteOnClose);

    qRegisterMetaType<QWidget*>("controls");
    qRegisterMetaType<RideItem*>("ride");
    qRegisterMetaType<GcWinID>("type");
    qRegisterMetaType<QColor>("color");
    qRegisterMetaType<DateRange>("dateRange");
    qRegisterMetaType<Perspective*>("perspective");
    nomenu = false;
    revealed = false;
    setParent(context->mainWidget());
    setControls(NULL);
    setRideItem(NULL);
    setPerspective(NULL);
    setTitle("");
    showtitle=true;
    setContentsMargins(0,0,0,0);
    setResizable(false);
    setMouseTracking(true);
    setProperty("color", GColor(CPLOTBACKGROUND));
    menu = NULL;

    // make sure its underneath the toggle button
    menuButton = new QPushButton(this);
    menuButton->setStyleSheet("QPushButton::menu-indicator { image: none; } QPushButton { border: 0px; padding: 0px; }");
    menuButton->setFocusPolicy(Qt::NoFocus);
    //menuButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
    menuButton->setText(tr("More..."));
    //menuButton->setArrowType(Qt::NoArrow);
    //menuButton->setPopupMode(QPushButton::InstantPopup);

    menu = new QMenu(this);
    menuButton->setMenu(menu);
    menu->addAction(tr("Remove Chart"), this, SLOT(_closeWindow()));

    menuButton->hide();
    menuButton->move(1,1);

    _noevents = false;
}

GcWindow::~GcWindow()
{
}

bool
GcWindow::amVisible()
{
    // if we're hidden then say false!
    if (isHidden()) return false;
    return true;
}

void 
GcWindow::setColor(QColor x)
{
    _color = x;
    emit colorChanged(x);
}

QColor
GcWindow::color() const
{
    return _color;
}

void
GcWindow::paintEvent(QPaintEvent * /*event*/)
{
    static QPixmap closeImage = QPixmap(":images/toolbar/popbutton.png");
    static QPalette defaultPalette;

    // setup a painter and the area to paint
    QPainter painter(this);
    // background light gray for now?
    QRect all(0,0,width(),height());
    painter.fillRect(all, property("color").value<QColor>());

    if (contentsMargins().top() > 0) {

        // fill in the title bar
        QRect bar(0,0,width(),contentsMargins().top());
        painter.setPen(Qt::darkGray);
        //painter.drawRect(QRect(0,0,width()-1,height()-1));
        //XXX need scopebar as tabbar before we do this
        //XXX painter.drawLine(0,0,width(),0);

        // heading
        QFont font;
        // font too large on hidpi scaling
        int pixelsize =pixelSizeForFont(font, contentsMargins().top());
        font.setPixelSize(pixelsize);
        font.setWeight(QFont::Bold);
        painter.setFont(font);
        QString subtitle = property("subtitle").toString();
        QString title = property("title").toString();
        QString heading = subtitle != "" ? subtitle : title;

        if (showtitle) {
            // pen color needs to contrast to background color
            QColor bgColor = property("color").value<QColor>();
            QColor fgColor = GCColor::invertColor(bgColor); // return the contrasting color

            painter.setPen(fgColor);
            painter.drawText(bar, heading, Qt::AlignVCenter | Qt::AlignCenter);

            if (isCompare()) {
                // overlay in highlight color
                QColor over = QColor(Qt::red);
                over.setAlpha(220);
                painter.setPen(over);
                painter.drawText(bar, heading, Qt::AlignVCenter | Qt::AlignCenter);
            }

            if (isFiltered()) {
                // overlay in highlight color
                QColor over = GColor(CCALCURRENT);
                over.setAlpha(220);
                painter.setPen(over);
                painter.drawText(bar, heading, Qt::AlignVCenter | Qt::AlignCenter);
            }
        }

        // border
        painter.setBrush(Qt::NoBrush);
        if (underMouse() && property("style") == 2) {

            QPixmap sized = closeImage.scaled(QSize(contentsMargins().top()-6,
                                                    contentsMargins().top()-6));
            QRect all(0,0,width()-1,height()-1);
            QPen pen(GColor(CPLOTMARKER));
            pen.setWidth(1);
            painter.setPen(pen);
            painter.drawRect(all);

        } else {
            painter.setPen(Qt::darkGray);
        }
    } else {
        // is this a layout manager?
        // background light gray for now?
        QRect all(0,0,width(),height());
        if (property("isManager").toBool() == true) {
            //painter.fillRect(all, QColor("#B3B4BA"));
            painter.fillRect(all, GColor(CPLOTBACKGROUND));
        }
    }
}

/*----------------------------------------------------------------------
 * Drag and resize tiles
 *--------------------------------------------------------------------*/
void
GcWindow::mousePressEvent(QMouseEvent *e)
{
    if (_noevents) return;

    // always propagate
    e->ignore();

    if (!resizable() || e->button() == Qt::NoButton || isHidden()) {
        setDragState(None);
        return;
    }

    DragState h = spotHotSpot(e);

    // is it on the close icon?
    if (h == Close) {
        setDragState(None);
        return;
    } else if (h == Flip) {
        setDragState(None);
    }

    // get current window state
    oWidth = width();
    oHeight = height();
    oWidthFactor = widthFactor();
    oHeightFactor = heightFactor();
    oX = pos().x();
    oY = pos().y();
    mX = mapFromGlobal(QCursor::pos()).x();
    mY = mapFromGlobal(QCursor::pos()).y();

    setDragState(h); // set drag state then!

    repaint();
}

void
GcWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (_noevents) return;

    // always propagate
    e->ignore();

    // tell the owner!
    if (dragState == Move) {
        setProperty("gripped", false);
        emit moved(this);
    } else if (dragState == Left ||
               dragState == Right ||
               dragState == Top ||
               dragState == Bottom ||
               dragState == TLCorner ||
               dragState == TRCorner ||
               dragState == BLCorner ||
               dragState == BRCorner) {
        emit resized(this);
    }
    setDragState(None);
    repaint();
}

// for the mouse position, are we in a hotspot?
// if so, what would the drag state become if we
// clicked?
GcWindow::DragState
GcWindow::spotHotSpot(QMouseEvent *e)
{
    // corner
    int corner = 9;
    int borderWidth = 3;

    // account for offset by mapping to GcWindow geom
    int _y = e->y();
    int _x = e->x();
    int _height = height();
    int _width = width();

    //if (e->x() > (2 + width() - corner) && e->y() < corner) return (Close);
    if (_x <= corner && _y <= corner) return (TLCorner);
    else if (_x >= (_width-corner) && _y <= corner) return (TRCorner);
    else if (_x <= corner && _y >= (_height-corner)) return (BLCorner);
    else if (_x >= (_width-corner) && _y >= (_height-corner)) return (BRCorner);
    else if (_x <= borderWidth) return (Left);
    else if (_x >= (_width-borderWidth)) return (Right);
    else if (_y <= borderWidth) return (Top);
    else if (_y >= (_height-borderWidth)) return (Bottom);
    else if (_y <= contentsMargins().top()) return (Move);
    else return (None);
}

void
GcWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (_noevents) return;

    // always propagate
    e->ignore();

    if (!resizable()) return;

    if (dragState == None) {
        // set the cursor shape
        setCursorShape(spotHotSpot(e));
        return;
    }

    // work out the relative move x and y
    int relx = mapFromGlobal(QCursor::pos()).x() - mX;
    int rely = mapFromGlobal(QCursor::pos()).y() - mY;

    switch (dragState) {

    default:
    case Move :
        setCursor(Qt::DragMoveCursor);
        emit moving(this);
        break;

    case TLCorner :
        {
            int newWidth = oWidth - relx;
            int newHeight = oHeight - rely;

            // need to move and resize
            if (newWidth > 30 && newHeight > 30) {
                move(oX + relx, oY + rely);
                setNewSize(newWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case TRCorner :
        {
            int newWidth = oWidth + relx;
            int newHeight = oHeight - rely;

            // need to move and resize if changes on y plane
            if (newWidth > 30 && newHeight > 30) {
                move(oX, oY + rely);
                setNewSize(newWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case BLCorner :
        {
            int newWidth = oWidth - relx;
            int newHeight = oHeight + rely;

            // need to move and resize
            if (newWidth > 30 && newHeight > 30) {
                move(oX + relx, oY);
                setNewSize(newWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case BRCorner :
        {
            int newWidth = oWidth + relx;
            int newHeight = oHeight + rely;

            // need to move and resize
            if (newWidth > 30 && newHeight > 30) {
                setNewSize(newWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case Top :
        {
            int newHeight = oHeight - rely;

            // need to move and resize
            if (newHeight > 30) {
                move (oX, oY + rely);
                setNewSize(oWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case Bottom :
        {
            int newHeight = oHeight + rely;

            // need to move and resize
            if (newHeight > 30) {
                setNewSize(oWidth, newHeight);
                emit resizing(this);
            }
        }
        break;

    case Left :
        {
            int newWidth = oWidth - relx;

            // need to move and resize
            if (newWidth > 30) {
                move (oX + relx, oY);
                setNewSize(newWidth, oHeight);
                emit resizing(this);
            }
        }
        break;

    case Right :
        {
            int newWidth = oWidth + relx;

            // need to move and resize
            if (newWidth > 30) {
                setNewSize(newWidth, oHeight);
                emit resizing(this);
            }
        }
        break;
    }

    oX = pos().x();
    oY = pos().y();
}

void
GcWindow::setNewSize(int w, int h)
{
    // convert to height factor
    double newHF = (double(oHeight) * oHeightFactor) / double(h);
    double newWF = (double(oWidth) * oWidthFactor) / double(w);

    // don't get too wide
    if (newWF < 1) newWF = 1;
    if (newWF > 10) newWF = 10;
    if (newHF < 1) newHF = 1;
    if (newWF < 1) newWF = 1;

    // SNAP TO GRID BY DEFAULT
    int wg = double((1.0 / newWF) * 50);
    newWF = 1.0 / (double(wg)/50);
    int hg = double((1.0 / newHF) * 50);
    newHF = 1.0 / (double(hg) / 50);
    h = (oHeight * oHeightFactor) / newHF;
    w = (oWidth * oWidthFactor) / newWF;
    // END OF SNAP TO GRID BY DEFAULT

    // now apply
    setFixedSize(QSize(w,h));

    // adjust factors
    setHeightFactor(newHF);
    setWidthFactor(newWF);
}

void
GcWindow::setDragState(DragState d)
{
    if (_noevents) return;

    dragState = d;
    setProperty("gripped", dragState == Move ? true : false);
    setCursorShape(d);
}

void
GcWindow::setCursorShape(DragState d)
{
    if (_noevents) return;

    // set cursor
    switch (d) {
    case Bottom:
    case Top:
        setCursor(Qt::SizeVerCursor);
        break;
    case Left:
    case Right:
        setCursor(Qt::SizeHorCursor);
        break;
    case TLCorner:
    case BRCorner:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case TRCorner:
    case BLCorner:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case Move:
        setCursor(Qt::ArrowCursor);
        break;
    default:
    case Close:
    case None:
        setCursor(Qt::ArrowCursor);
        break;
    }
}

void
#if QT_VERSION >= 0x060000
GcWindow::enterEvent(QEnterEvent *)
#else
GcWindow::enterEvent(QEvent *)
#endif
{
    if (_noevents) return;

    if (nomenu == false) {
        if (contentsMargins().top() > (20*dpiYFactor)) menuButton->setFixedSize(80*dpiXFactor,30*dpiYFactor);
        else menuButton->setFixedSize(80*dpiXFactor, 15*dpiYFactor);
        menuButton->raise();
        menuButton->show();
    }
}


void
GcWindow::leaveEvent(QEvent *)
{
    if (_noevents) return;

    menuButton->hide();
}

void
GcWindow::_closeWindow()
{
    emit closeWindow(this);
}
