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

class GcSubSplitter;
class GcSplitterControl;
class GcSplitterItem;

class GcSplitter : public QWidget
{
    Q_OBJECT

public:
    GcSplitter(Qt::Orientation orientation, QWidget *parent = 0);

    void addWidget(QWidget *widget);
    void insertWidget(int index, QWidget *widget);

    void setOpaqueResize(bool opaque = true);
    void setSizes(const QList<int> &list);

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

Q_SIGNALS:
    void splitterMoved(int pos, int index);

public slots:
    void subSplitterMoved(int pos, int index);

private:
    GcSubSplitter *splitter;
    GcSplitterControl *control;
};

class GcSubSplitter : public QSplitter
{
    Q_OBJECT

public:
    GcSubSplitter(Qt::Orientation orientation, GcSplitterControl *control, QWidget *parent = 0);

    void addWidget(QWidget *widget);
    void insertWidget(int index, QWidget *widget);

protected:
    QSplitterHandle *createHandle();

private:
    QList<QString> titles;
    QWidget * _insertedWidget;

    GcSplitterControl *control;

};

class GcSplitterHandle : public QSplitterHandle
{
    Q_OBJECT

public:
    GcSplitterHandle(QString title, GcSplitterItem *widget, Qt::Orientation orientation, GcSubSplitter *parent = 0);

    QSize sizeHint() const;
    GcSubSplitter *splitter() const;
    void addAction(QAction *action);
    void addActions(QList<QAction*> actions);
protected:
    void paintEvent(QPaintEvent *);

public slots:
    void showHideClicked();

    void setExpanded(bool expanded);

private:
    void paintBackground(QPaintEvent *);

     GcSubSplitter *gcSplitter;
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

class GcSplitterControl : public QWidget
{
    Q_OBJECT

public:
    GcSplitterControl(QWidget *parent);

    void addAction(QAction *action);
    void selectAction();

protected:
    void paintEvent(QPaintEvent *);

private:
    void paintBackground(QPaintEvent *);

     QHBoxLayout *titleLayout;
     QToolBar *titleToolbar;
};

class GcSplitterItem : public QWidget
{
    Q_OBJECT

public:

    GcSplitterItem(QString title, QIcon icon, QWidget *parent);
    ~GcSplitterItem();

    QWidget *content;
    GcSplitterHandle *splitterHandle;

    bool state;
    QString title;
    QIcon icon;

public slots:

    void addWidget(QWidget*);
    void selectHandle();

private:
    QVBoxLayout *layout;





};

#endif
