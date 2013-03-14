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

#ifndef _GC_GcScopeBar_h
#define _GC_GcScopeBar_h 1

#include <QtGui>
#include <QList>
#include <QAction>

#ifdef Q_OS_MAC
class QtMacButton;
#else
class GcScopeButton : public QWidget
{
    Q_OBJECT

    public:
        GcScopeButton(QWidget *parent);
        void setText(QString _text) { text = _text; }
        void setChecked(bool _checked) { checked = _checked; repaint(); }
        void setWidth(int x) { setFixedWidth(x); }

    signals:
        void clicked(bool);

    public slots:
        void paintEvent(QPaintEvent *);
        bool event(QEvent *e);

    private:
        bool checked;
        QString text;
};

#endif

class GcScopeBar : public QWidget
{
    Q_OBJECT

public:

    GcScopeBar(QWidget *parent, QWidget *traintool);
    ~GcScopeBar();

public slots:
    void paintEvent (QPaintEvent *event);

    void clickedHome();
    void clickedAnal();
    void clickedTrain();
    void clickedDiary();

    // mainwindow tells us when it switched without user clicking.
    void selected(int index);
    void addWidget(QWidget*);

signals:
    void selectHome();
    void selectAnal();
    void selectTrain();
    void selectDiary();

    void addChart();

private:
    void paintBackground(QPaintEvent *);

    QHBoxLayout *layout;
#ifdef Q_OS_MAC
    QtMacButton *home, *diary, *anal, *train;
#else
    GcScopeButton *home, *diary, *anal, *train;
#endif
    bool state;
};

#endif
