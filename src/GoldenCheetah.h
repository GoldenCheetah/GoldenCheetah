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

#define G_OBJECT Q_PROPERTY(QString instanceName READ instanceName WRITE setInstanceName)
#define setInstanceName(x) setProperty("instanceName", x)
#define myRideItem property("ride").value<RideItem*>()

#include <QString>
#include <QWidget>
#include <QObject>
#include <QMetaObject>
#include <QVariant>
#include <QMetaType>
#include <QFrame>

class RideItem;
class GcWindow;
#include "GcWindowRegistry.h"


Q_DECLARE_METATYPE(QWidget*);
Q_DECLARE_METATYPE(RideItem*);
Q_DECLARE_METATYPE(GcWinID);


class GcWindow : public QFrame
{
private:
    Q_OBJECT

    // what kind of window is this?
    Q_PROPERTY(GcWinID type READ type WRITE setType) // not a user modifiable property

    // each window has an instance name - default set
    // by the widget constructor but overide from layou manager
    Q_PROPERTY(QString instanceName READ instanceName WRITE _setInstanceName USER true)

    // controls are updated by widget when it is constructed
    Q_PROPERTY(QWidget* controls READ controls WRITE setControls NOTIFY controlsChanged)

    // the title is shown in the title bar - set by
    // the layout manager
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged USER true)

    // the ride to plot - not set by the widget but
    // informed by the layout manager
    Q_PROPERTY(RideItem* ride READ rideItem WRITE setRideItem NOTIFY rideItemChanged)

    // geometry factor
    Q_PROPERTY(double widthFactor READ widthFactor WRITE setWidthFactor NOTIFY widthFactorChanged USER true);
    Q_PROPERTY(double heightFactor READ heightFactor WRITE setHeightFactor NOTIFY widthFactorChanged USER true);

    // can be resized
    Q_PROPERTY(bool resizable READ resizable WRITE setResizable USER true);
    Q_PROPERTY(bool gripped READ gripped WRITE setGripped);

    QWidget *_controls;
    QString _title;
    QString _instanceName;
    RideItem *_rideItem;
    GcWinID _type;
    double _widthFactor;
    double _heightFactor;
    bool _resizable;
    bool _gripped;

    enum drag { None, Close, Flip, Move, Left, Right, Top, Bottom, TLCorner, TRCorner, BLCorner, BRCorner };
    typedef enum drag DragState;
    // state data for resizing tiles
    DragState dragState;
    int oWidth, oHeight, oX, oY, mX, mY;
    double oHeightFactor, oWidthFactor;


signals:
    void controlsChanged(QWidget*);
    void titleChanged(QString);
    void rideItemChanged(RideItem*);
    void heightFactorChanged(double);
    void widthFactorChanged(double);
    void resizing(GcWindow*);
    void moving(GcWindow*);
    void resized(GcWindow*); // finished resizing
    void moved(GcWindow*);   // finished moving

public:

    GcWindow();
    ~GcWindow();
    GcWindow(QWidget *p);

    void _setInstanceName(QString x); // GOBJECTS can set their instance name, but not be GcWindows
    QString instanceName() const;

    void setControls(QWidget *x);
    QWidget *controls() const;

    void setTitle(QString x);
    QString title() const;


    void setRideItem(RideItem *);
    RideItem *rideItem() const;

    void setWidthFactor(double);
    double widthFactor() const;

    void setHeightFactor(double);
    double heightFactor() const;

    void setResizable(bool);
    bool resizable() const;

    void setGripped(bool);
    bool gripped() const;

    GcWinID type() const { return _type; }
    void setType(GcWinID x) { _type = x; } // only really used by the window registry

    virtual bool amVisible();

    // for sorting... we look at x
    bool operator< (GcWindow right) const { return geometry().x() < right.geometry().x(); }

    // we paint a heading if there is space in the top margin
    void paintEvent (QPaintEvent * event);

    // mouse actions -- resizing and dragging tiles
    bool eventFilter(QObject *object, QEvent *e);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
    void setDragState(DragState);
    void setCursorShape(DragState);
    DragState spotHotSpot(QMouseEvent *);
    void setNewSize(int w, int h);

};


#endif
