/*
 * Copyright (c) 2013 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#ifndef _GC_GcSideBarItem_h
#define _GC_GcSideBarItem_h 1

#include <QtGui>
#include <QList>
#include <QAction>

class GcSplitterItem;
class GcSplitter;

class GcSplitterHandle : public QSplitterHandle
{
    Q_OBJECT

public:
    GcSplitterHandle(QString title, GcSplitterItem *widget, Qt::Orientation orientation, GcSplitter *parent = 0);

    QSize sizeHint() const;
    GcSplitter *splitter() const;
    void addAction(QAction *action);
    void addActions(QList<QAction*> actions);
protected:
    void paintEvent(QPaintEvent *);

public slots:
    void showHideClicked();

    void setExpanded(bool expanded);

private:
    void paintBackground(QPaintEvent *);

     GcSplitter *gcSplitter;
     GcSplitterItem *widget;

     QHBoxLayout *titleLayout;
     QLabel *titleLabel;
     QToolBar *titleToolbar;
     QPushButton *showHide;

     QString title;
     int index;
     int fullHeight;
     bool state;
};

class GcSplitter : public QSplitter
{
    Q_OBJECT

public:
    GcSplitter(Qt::Orientation orientation, QWidget *parent = 0);

    void addWidget(QWidget *widget);
    void insertWidget(int index, QWidget *widget);

protected:
    QSplitterHandle *createHandle();

private:
    QList<QString> titles;
    QWidget * _insertedWidget;

};

class GcSplitterItem : public QWidget
{
    Q_OBJECT

public:

    GcSplitterItem(QString title, QWidget *parent);
    ~GcSplitterItem();

    QWidget *content;
    GcSplitterHandle *splitterHandle;

    bool state;
    QString title;

public slots:

    void addWidget(QWidget*);

private:
    QVBoxLayout *layout;



};

#endif
