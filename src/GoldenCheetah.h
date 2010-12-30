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
    Q_PROPERTY(int widthFactor READ widthFactor WRITE setWidthFactor NOTIFY widthFactorChanged USER true);
    Q_PROPERTY(int heightFactor READ heightFactor WRITE setHeightFactor NOTIFY widthFactorChanged USER true);

    QWidget *_controls;
    QString _title;
    QString _instanceName;
    RideItem *_rideItem;
    GcWinID _type;
    int _widthFactor;
    int _heightFactor;

    // we paint a heading if there is space in the top margin
    void paintEvent (QPaintEvent * event);

signals:
    void controlsChanged(QWidget*);
    void titleChanged(QString);
    void rideItemChanged(RideItem*);
    void heightFactorChanged(int);
    void widthFactorChanged(int);

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

    void setWidthFactor(int);
    int widthFactor() const;

    void setHeightFactor(int);
    int heightFactor() const;

    GcWinID type() const { return _type; }
    void setType(GcWinID x) { _type = x; } // only really used by the window registry

    virtual bool amVisible();
};


#endif
