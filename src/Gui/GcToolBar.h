/*
 * Copyright 2011 (c) Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_GcToolBar_h
#define _GC_GcToolBar_h 1

#include <QtGui>
#include <QList>
#include <QAction>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include "Colors.h"

class GcToolBar : public QWidget
{
    Q_OBJECT

public:

    GcToolBar(QWidget *parent);

public slots:
    void paintEvent (QPaintEvent *event);
    void addWidget(QWidget *); // any widget but doesn't toggle selection
    void addStretch();

private:
    QHBoxLayout *layout;
    void paintBackground(QPaintEvent *);
};

// handy spacer
class Spacer : public QWidget
{
public:
    Spacer(QWidget *parent) : QWidget(parent) {
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setSizePolicy(sizePolicy);
    }
    QSize sizeHint() const { return QSize(40*dpiXFactor, 1); }
};

#endif
