/*
 * Copyright (c) 2012 Mark Liversedge (liversedge@gmail.com)
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

#ifndef QtMacSearchBox_H
#define QtMacSearchBox_H

#include <QtGui>

#include <qmaccocoaviewcontainer_mac.h>

class QtMacSearchBox;
class SearchWidget : public QMacCocoaViewContainer
{
    Q_OBJECT
public:
    SearchWidget(QtMacSearchBox *parent = 0);
    ~SearchWidget();

    QSize sizeHint() const;

    void textChanged(QString);
    void searchSubmit();
    void editFinished();

private:
    QtMacSearchBox *parent;
};

class QtMacSearchBox : public QWidget
{
    Q_OBJECT

    friend class ::SearchWidget;

public:
    QtMacSearchBox(QWidget *parent = 0);
    QSize sizeHint() const;
    QWidget *s;

signals:
    void textChanged(QString);
    void searchSubmit();
    void editFinished();
};

#endif
