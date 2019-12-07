/*
 * Copyright (c) Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_GoldenCheetah_h
#define _GC_GoldenCheetah_h
class GcWindow;
class Context;

#define G_OBJECT
//#define G_OBJECT Q_PROPERTY(QString instanceName READ instanceName WRITE setInstanceName)
//#define setInstanceName(x) setProperty("instanceName", x)
#define myRideItem property("ride").value<RideItem*>()
#define myDateRange property("dateRange").value<DateRange>()

#include <QString>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QWidget>
#include <QStackedLayout>
#include <QObject>
#include <QMetaObject>
#include <QVariant>
#include <QMetaType>
#include <QFrame>
#include <QtGui>

#include "GcWindowRegistry.h"
#include "TimeUtils.h"

class RideItem;
class GcOverlayWidget;


class GcWindow : public QFrame
{
private:
    Q_OBJECT

    // what kind of window is this?
    Q_PROPERTY(GcWinID type READ type WRITE setType) // not a user modifiable property

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
    int _style;
    bool _noevents; // don't work with events

    enum drag { None, Close, Flip, Move, Left, Right, Top, Bottom, TLCorner, TRCorner, BLCorner, BRCorner };
    typedef enum drag DragState;
    // state data for resizing tiles
    DragState dragState;
    int oWidth, oHeight, oX, oY, mX, mY;
    double oHeightFactor, oWidthFactor;

public slots:
    void _closeWindow();

signals:
    void controlsChanged(QWidget*);
    void titleChanged(QString);
    void subtitleChanged(QString);
    void rideItemChanged(RideItem*);
    void styleChanged(int);
    void heightFactorChanged(double);
    void widthFactorChanged(double);
    void colorChanged(QColor);
    void dateRangeChanged(DateRange);
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

    void setWidthFactor(double);
    double widthFactor() const;

    void setHeightFactor(double);
    double heightFactor() const;

    void setColor(QColor);
    QColor color() const;

    void setResizable(bool);
    bool resizable() const;

    void moveEvent(QMoveEvent *); // we trap move events to ungrab during resize
    void setGripped(bool);
    bool gripped() const;

    int style() const { return _style; }
    void setStyle(int x) { if (_style != x) {_style = x; emit styleChanged(x); }}

    GcWinID type() const { return _type; }
    void setType(GcWinID x) { _type = x; } // only really used by the window registry

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
    bool operator< (const GcWindow &right) const { return geometry().x() < right.geometry().x(); }

    // we paint a heading if there is space in the top margin
    void paintEvent (QPaintEvent * event);

    // mouse actions -- resizing and dragging tiles
    //bool eventFilter(QObject *object, QEvent *e);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);
    void setDragState(DragState);
    void setCursorShape(DragState);
    DragState spotHotSpot(QMouseEvent *);
    void setNewSize(int w, int h);

    QPushButton *settingsButton, *closeButton;
    QPushButton *menuButton;
    QMenu *menu;
    QList<QAction*>actions;
};

class GcChartWindow : public GcWindow
{
private:

    Q_OBJECT

    QStackedLayout *_layout;
    QStackedLayout *_mainLayout;
    QVBoxLayout *_defaultBlankLayout;

    QLayout *_chartLayout,
            *_revealLayout,
            *_blankLayout;

    QWidget *_mainWidget;
    QWidget *_blank;
    QWidget *_chart;

    // reveal controls
    QWidget *_revealControls;
    QPropertyAnimation *_revealAnim,
                       *_unrevealAnim;
    QTimer *_unrevealTimer;
    Context *context;

public:

    // reveal
    bool virtual hasReveal() { return false; }
    void reveal();
    void unreveal();

    // overlay widget
    GcOverlayWidget *overlayWidget;
    bool wantOverlay;

    // handle Chart Serialissation
    void serializeChartToQTextStream(QTextStream& out);


    GcChartWindow(Context *context);

    // parse a .gchart file / or string and return a list of charts expressed
    // as a property list in a QMap1
    static QList<QMap<QString,QString> > chartPropertiesFromFile(QString filename);
    static QList<QMap<QString,QString> > chartPropertiesFromString(QString contents);

    QWidget *mainWidget() { return _mainWidget; }
    GcOverlayWidget *helperWidget() { return overlayWidget; }

    void setChartLayout(QLayout *layout);
    void setRevealLayout(QLayout *layout);
    void setBlankLayout(QLayout *layout);
    void setIsBlank(bool value);
    void setControls(QWidget *x);
    void addHelper(QString name, QWidget *widget); // add to the overlay widget

public slots:
    void hideRevealControls();
    void saveImage();
    void saveChart();
#ifdef GC_HAS_CLOUD_DB
    void exportChartToCloudDB();
    bool chartHasUserMetrics();
#endif
    void colorChanged(QColor);
};

#endif
