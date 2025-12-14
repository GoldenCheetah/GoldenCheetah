#ifndef _GC_GcWindow_h
#define _GC_GcWindow_h

#include <QFrame>
#include <QMenu>
#include <QPushButton>
#include <QObject>
#include <QVariant>
#include <QColor>
// #include "Context.h"
#include "GcWindowRegistry.h"
#include "TimeUtils.h"

class RideItem;
Q_DECLARE_OPAQUE_POINTER(RideItem*)
class Context;
class Perspective;
Q_DECLARE_OPAQUE_POINTER(Perspective*)

class GcWindow : public QFrame
{
private:
    Q_OBJECT

    // what kind of window is this?
    Q_PROPERTY(GcWinID type READ type WRITE setType) // not a user modifiable property
    Q_PROPERTY(Perspective *perspective READ getPerspective WRITE setPerspective USER false)

    // each window has an instance name - default set
    // by the widget constructor but overide from layou manager
    //Q_PROPERTY(QString instanceName READ instanceName WRITE _setInstanceName USER true)

    // controls are updated by widget when it is constructed
    Q_PROPERTY(QWidget* controls READ controls WRITE setControls NOTIFY controlsChanged)

    // the title is shown in the tab bar - set by
    // the layout manager
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged USER true)
    Q_PROPERTY(QString subtitle READ subtitle WRITE setSubTitle NOTIFY subtitleChanged USER true)

    // the ride to plot - not set by the widget but
    // informed by the layout manager
    Q_PROPERTY(RideItem* ride READ rideItem WRITE setRideItem NOTIFY rideItemChanged)

    // all charts have a date range, they don't all implement it
    Q_PROPERTY(DateRange dateRange READ dateRange WRITE setDateRange NOTIFY dateRangeChanged)

    // geometry factor
    Q_PROPERTY(double widthFactor READ widthFactor WRITE setWidthFactor NOTIFY widthFactorChanged USER true)
    Q_PROPERTY(double heightFactor READ heightFactor WRITE setHeightFactor NOTIFY heightFactorChanged USER true)

    // can be resized
    Q_PROPERTY(int style READ style WRITE setStyle USER true NOTIFY styleChanged)
    Q_PROPERTY(bool resizable READ resizable WRITE setResizable USER true)
    Q_PROPERTY(bool gripped READ gripped WRITE setGripped)

    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged USER false)
    QColor _color;

    QWidget *_controls;

    bool showtitle;
    QString _title;
    QString _subtitle;
    //QString _instanceName;
    RideItem *_rideItem;
    GcWinID _type;
    DateRange _dr;
    double _widthFactor;
    double _heightFactor;
    bool _resizable;
    bool _gripped;
    int _style = 0;
    bool _noevents; // don't work with events
    Perspective *_perspective;

    enum drag { None, Close, Flip, Move, Left, Right, Top, Bottom, TLCorner, TRCorner, BLCorner, BRCorner };
    typedef enum drag DragState;
    // state data for resizing tiles
    DragState dragState = None;
    int oWidth, oHeight, oX, oY, mX, mY;
    double oHeightFactor, oWidthFactor;

public Q_SLOTS:
    void _closeWindow();

Q_SIGNALS:
    void controlsChanged(QWidget*);
    void titleChanged(QString);
    void subtitleChanged(QString);
    void rideItemChanged(RideItem*);
    void styleChanged(int);
    void heightFactorChanged(double);
    void widthFactorChanged(double);
    void colorChanged(QColor);
    void dateRangeChanged(DateRange);
    void perspectiveFilterChanged(QString);
    void perspectiveChanged(Perspective*);
    void resizing(GcWindow*);
    void moving(GcWindow*);
    void resized(GcWindow*); // finished resizing
    void moved(GcWindow*);   // finished moving

    void showControls();
    void closeWindow(GcWindow*);

public:

    GcWindow(Context *context);
    ~GcWindow();

    //void _setInstanceName(QString x); // GOBJECTS can set their instance name, but not be GcWindows
    //QString instanceName() const;

    // must call this before set controls
    void addAction(QAction *act) { actions << act; }
    void setNoEvents(bool x) { _noevents = x; }

    void setPerspective(Perspective *x) { _perspective=x; }
    Perspective *getPerspective() const { return _perspective; }

    void virtual setControls(QWidget *x);
    QWidget *controls() const;

    void setSubTitle(QString x);
    QString subtitle() const;

    void setTitle(QString x);
    QString title() const;
    void setShowTitle(bool x) { showtitle=x; }
    bool showTitle() const { return showtitle; }

    void setRideItem(RideItem *);
    RideItem *rideItem() const;

    void setDateRange(DateRange);
    DateRange dateRange() const;

    void notifyPerspectiveFilterChanged(QString x) { emit perspectiveFilterChanged(x); }
    void notifyPerspectiveChanged(Perspective *x) { emit perspectiveChanged(x); }

    void setWidthFactor(double);
    double widthFactor() const;

    void setHeightFactor(double);
    double heightFactor() const;

    void setColor(QColor);
    QColor color() const;

    void setResizable(bool);
    bool resizable() const;

    void moveEvent(QMoveEvent *) override; // we trap move events to ungrab during resize
    void setGripped(bool);
    bool gripped() const;

    int style() const { return _style; }
    void setStyle(int x) { if (_style != x) {_style = x; emit styleChanged(x); }}

    GcWinID type() const { return _type; }
    void setType(GcWinID x) { _type = x; } // only really used by the window registry

    void showMore(bool x) { nomenu=!x; }
    virtual bool amVisible();

    // popover controls
    virtual bool hasReveal() { return false;}
    virtual void reveal() {}
    virtual void unreveal() {}
    bool revealed;

    // is filtered?
    virtual bool isFiltered() const { return false;}

    // is comparing?
    virtual bool isCompare() const { return false;}

    // for sorting... we look at x
    bool operator< (GcWindow right) const { return geometry().x() < right.geometry().x(); }

    // we paint a heading if there is space in the top margin
    void paintEvent (QPaintEvent * event) override;

    // mouse actions -- resizing and dragging tiles
    //bool eventFilter(QObject *object, QEvent *e);
    void mousePressEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
#if QT_VERSION >= 0x060000
    void enterEvent(QEnterEvent *) override;
#else
    void enterEvent(QEvent *) override;
#endif
    void leaveEvent(QEvent *) override;
    void setDragState(DragState);
    void setCursorShape(DragState);
    DragState spotHotSpot(QMouseEvent *);
    void setNewSize(int w, int h);

    QPushButton *settingsButton, *closeButton;
    QPushButton *menuButton;
    QMenu *menu;
    bool nomenu;
    QList<QAction*>actions;
};

#endif
