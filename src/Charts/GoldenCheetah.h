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
class Perspective;

#define G_OBJECT
//#define G_OBJECT Q_PROPERTY(QString instanceName READ instanceName WRITE setInstanceName)
//#define setInstanceName(x) setProperty("instanceName", x)
#define myRideItem property("ride").value<RideItem*>()
#define myDateRange property("dateRange").value<DateRange>()
#define myPerspective property("perspective").value<Perspective*>()

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
#include <QObject>

#include "GcWindowRegistry.h"
#include "TimeUtils.h"
#include "GcWindow.h" // Includes GcWindow definition
#include "GcChartWindow.h" // Includes GcChartWindow definition

class RideItem;

#if QT_VERSION >= 0x060000
// For RideItem and Perspective properties, this is required.
// A normal include would lead to a circular dependency here.
Q_MOC_INCLUDE("RideItem.h");
Q_MOC_INCLUDE("Perspective.h");
#endif

class GcOverlayWidget;
class Perspective;

#endif

