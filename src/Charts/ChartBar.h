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
#include <QScrollArea>
#include <QToolButton>
#include <QObject>
#include <QEvent>

class Context;
class GcScopeButton;
class GcLabel;
class ButtonBar;

class ChartBar : public QWidget
{
    Q_OBJECT

public:

    ChartBar(Context *context);
    ~ChartBar();


public slots:

    void paintEvent (QPaintEvent *event);
    bool eventFilter(QObject *object, QEvent *e); // trap resize
    void addWidget(QString);
    void clear();
    void clicked(int);
    void removeWidget(int);
    void setText(int index, QString);
    void setCurrentIndex(int index);
    void scrollLeft();
    void scrollRight();
    void tidy();
    void setChartMenu();
    void menuPopup();
    void configChanged(qint32); // appearance

signals:
    void currentIndexChanged(int);

private:
    void paintBackground(QPaintEvent *);

    Context *context;

    ButtonBar *buttonBar;
    QToolButton *left, *right; // scrollers, hidden if menu fits
    QToolButton *menuButton;
    QScrollArea *scrollArea;
    QHBoxLayout *layout;

    QFont buttonFont;
    QVector<GcScopeButton*> buttons;
    QSignalMapper *signalMapper;

    QMenu *barMenu, *chartMenu;

    int currentIndex_;
    bool state;
};

class ButtonBar : public QWidget
{
    Q_OBJECT;

public:

    ButtonBar(ChartBar *parent) : QWidget(parent), chartbar(parent) {}

public slots:

    void paintEvent (QPaintEvent *event);

private:

    void paintBackground(QPaintEvent *);
    ChartBar *chartbar;

};

#endif
