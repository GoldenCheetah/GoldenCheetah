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
#include <QPropertyAnimation>

class Context;
class ChartBarItem;
class GcLabel;
class ButtonBar;

class ChartBar : public QWidget
{
    Q_OBJECT

    friend class ::ChartBarItem;

public:

    ChartBar(Context *context);
    ~ChartBar();


public slots:

    void paintEvent (QPaintEvent *event);
    bool eventFilter(QObject *object, QEvent *e); // trap resize
    void addWidget(QString);
    void clear();
    void clicked(int);
    void triggerContextMenu(int);
    void removeWidget(int);
    void setText(int index, QString);
    void setColor(int index, QColor);
    void setCurrentIndex(int index);
    void scrollLeft();
    void scrollRight();
    void tidy(bool setwidth);
    void setChartMenu();
    void menuPopup();
    void showPopup();
    void configChanged(qint32); // appearance

signals:
    void contextMenu(int,int);
    void currentIndexChanged(int);
    void itemMoved(int from, int to);

protected:

    // dragging needs to work with these
    QScrollArea *scrollArea;
    QHBoxLayout *layout;
    QList<ChartBarItem*> buttons;

private:

    void paintBackground(QPaintEvent *);

    Context *context;

    ButtonBar *buttonBar;
    QToolButton *left, *right, *showButton; // scrollers, hidden if menu fits
    QPropertyAnimation *anim; // scroll left and right - animated to show whats happening
    QToolButton *menuButton;

    QFont buttonFont;
    QSignalMapper *signalMapper, *menuMapper;

    QMenu *barMenu, *chartMenu, *showMenu;

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

class ChartBarItem : public QWidget
{
    Q_OBJECT

    public:
        ChartBarItem(ChartBar *parent);
        void setText(QString _text) { text = _text; }
        void setColor(QColor _color) { color = _color; update(); }
        void setChecked(bool _checked) { checked = _checked; update(); }
        bool isChecked() { return checked; }
        void setWidth(int x) { setFixedWidth(x); }
        void setHighlighted(bool x) { highlighted = x; }
        bool ishighlighted() const { return highlighted; }
        void setRed(bool x) { red = x; }

        QString text;
    signals:
        void clicked(bool);
        void contextMenu();

    public slots:
        void paintEvent(QPaintEvent *);
        bool event(QEvent *e);
        void clicked();

    private:
        ChartBar *chartbar;

        // managing dragging of tabs
        enum { Idle, Click, Clone, Drag } state;
        QPoint clickpos;
        int originalindex;
        int indexPos(int); // calculating drop position
        ChartBarItem *dragging;

        QColor color; // background when selected
        bool checked;
        bool highlighted;
        bool red;

        QPainterPath triangle, hotspot;
};
#endif
