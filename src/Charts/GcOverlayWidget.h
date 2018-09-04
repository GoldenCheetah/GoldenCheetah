/*
 * Copyright 2014 (c) Mark Liversedge (liversedge@gmail.com)
 *                (c) Aleksey Osipov (aliks-os@yandex.ru)
 *
 * Some of the code pinched from http://qt-project.org/wiki/Widget-moveable-and-resizeable
 * coz it was simpler than starting from scratch.
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

#ifndef _GC_GcOverlayWidget_h
#define _GC_GcOverlayWidget_h 1

#include <QtGui>
#include <QList>
#include <QAction>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QScrollArea>
#include <QToolButton>
#include <QObject>
#include <QEvent>


class Context;
class GcLabel;

class GcOverlayWidgetItem
{
    public:
        GcOverlayWidgetItem(QString name, QWidget *widget) : name(name), widget(widget) {}

        QString name;
        QWidget *widget;
};

class GcOverlayWidget : public QWidget
{
    Q_OBJECT

public:

    GcOverlayWidget(Context *context, QWidget *parent);
    ~GcOverlayWidget();

    // add remove children
    void addWidget(QString title, QWidget *widget);
    int currentIndex() const;

    // set arrow widgets if needed
    void setCursors();

public slots:

    // events
    void paintEvent (QPaintEvent *event);
    void setLabel(); // when name changes
    //void resizeEvent(QResizeEvent *);
    //bool eventFilter(QObject *object, QEvent *e); // trap resize
    void configChanged(qint32);

    // switch children
    void setCurrentIndex(int index);
    void scrollLeft();   // scroll left action
    void scrollRight();  // scroll right action

    // set the title
    void setTitle(int index, QString text) {
        items[index].name = text;
        setLabel();
    }

signals:

    void currentIndexChanged(int);
    void inFocus(bool mode);
    void outFocus(bool mode);
    void newGeometry(QRect rect);

protected:

    enum modes{ none = 0, moving = 1, resizetl = 2, resizet = 3, resizetr = 4, resizer = 5, resizebr = 6,
                resizeb = 7, resizebl = 8, resizel = 9 };

    int mode;
    QPoint position;
    QVBoxLayout* vLayout;
    void setCursorShape(const QPoint &e_pos);
    bool eventFilter( QObject *obj, QEvent *evt );
    void keyPressEvent(QKeyEvent *);
    void focusInEvent(QFocusEvent *);
    void focusOutEvent(QFocusEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    bool m_infocus;
    bool m_showMenu;
    bool m_isEditing;
    void popupShow(const QPoint &pt);
    QWidget *clone();

private:
    void paintBackground(QPaintEvent *);
    void setScrollers(); // do we need to show a scroll button ?

    Context *context;

    QToolButton *left, *right; // scrollers, hidden if menu fits
    GcLabel *titleLabel;
    QStackedWidget *stack; // where the widget gets put

    QList<GcOverlayWidgetItem> items;
    bool initial;
};
#endif
