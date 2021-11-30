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
#include <QSplitter>
#include <QToolBar>
#include <QAction>
#include <QHBoxLayout>
#include <QVBoxLayout>

class GcSubSplitter;
class GcSplitterControl;
class GcSplitterItem;
class GcLabel;

class GcSplitter : public QWidget
{
    Q_OBJECT

public:
    GcSplitter(Qt::Orientation orientation, QWidget *parent = 0);

    void addWidget(QWidget *widget);
    void insertWidget(int index, QWidget *widget);

    void setOpaqueResize(bool opaque = true);
    QList<int> sizes() const;
    void setSizes(const QList<int> &list);

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

    void prepare(QString cyclist, QString name); // get ready for first show, you're now configured
                                // I'm gonna call you "name" -- which is used to
                                // save and restore state

signals:
    void splitterMoved(int pos, int index);

public slots:
    void subSplitterMoved(int pos, int index);
    void saveSettings();

private:
    GcSubSplitter *splitter;
    GcSplitterControl *control;
    QString cyclist, name;
};

class GcSubSplitter : public QSplitter
{
    Q_OBJECT

public:
    GcSubSplitter(Qt::Orientation orientation, GcSplitterControl *control, GcSplitter *parent, bool plainstyle=false);

    // plainstyle splitters have no control (pass NULL), no metal styling and no toolbar on the handle

    void addWidget(QWidget *widget);
    void insertWidget(int index, QWidget *widget);
    GcSplitterItem *removeItem(QString text);

protected:
    QSplitterHandle *createHandle();

private:
    QList<QString> titles;
    QWidget * _insertedWidget;

    GcSplitterControl *control;
    GcSplitter *gcSplitter;
    bool plainstyle;

};

class GcSplitterHandle : public QSplitterHandle
{
    Q_OBJECT

    friend class ::GcSplitterItem;

public:
    GcSplitterHandle(QString title, Qt::Orientation orientation, QSplitter *parent = 0, bool metal = true);
    GcSplitterHandle(QString title, Qt::Orientation orientation, QWidget *left, QWidget *mid, QWidget *right,
                     QSplitter *parent = 0, bool metal = true);

    QString title() const { return _title; }
    QSize sizeHint() const;
    GcSubSplitter *splitter() const;
    void addAction(QAction *action);
    void addActions(QList<QAction*> actions);

protected:
    void paintEvent(QPaintEvent *);
    GcSubSplitter *gcSplitter;
    int index;

private:
    void init(QString title, Qt::Orientation orientation, QWidget *left, QWidget *mid, QWidget *right,
                     QSplitter *parent, bool metal);
    void paintBackground(QPaintEvent *);

    QHBoxLayout *titleLayout;
    GcLabel *titleLabel;

    QString _title;
    int fullHeight;
    bool metal;
};

class GcSplitterControl : public QToolBar
{
    Q_OBJECT

public:
    GcSplitterControl(QWidget *parent);
    void selectAction();

protected:
    void paintEvent(QPaintEvent *);

private:
    void paintBackground(QPaintEvent *);

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
    QAction *controlAction;

public slots:

    void addWidget(QWidget*);
    void selectHandle();

private:
    QVBoxLayout *layout;

};

extern QIcon iconFromPNG(QString filename, QSize size);
extern QIcon iconFromPNG(QString filename, bool emboss = true);
#endif
