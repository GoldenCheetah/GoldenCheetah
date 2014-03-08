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

#ifndef _GC_ChartBar_h
#define _GC_ChartBar_h 1

#include <QtGui>
#include <QList>
#include <QAction>
#include <QHBoxLayout>

class Context;
#ifdef Q_OS_MAC
class QtMacButton;
#else
class GcScopeButton;
#endif
class GcLabel;

class ChartBar : public QWidget
{
    Q_OBJECT

public:

    ChartBar(Context *context);
    ~ChartBar();

public slots:

    void paintEvent (QPaintEvent *event);
    void addWidget(QString);
    void clear();
    void clicked(int);
    void removeWidget(int);
    //void setCurrentIndex(int index);

signals:
    void currentIndexChanged(int);

private:
    void paintBackground(QPaintEvent *);

    Context *context;
    QHBoxLayout *layout;

    QFont buttonFont;
    QVector<GcScopeButton*> buttons;
    QSignalMapper *signalMapper;

    QLinearGradient active, inactive;
    int currentIndex_;
    bool state;
};

#endif
