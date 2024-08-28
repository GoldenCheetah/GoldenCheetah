/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_ComparePane_h
#define _GC_ComparePane_h 1

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QScrollArea>
#include <QDialog>

#include "GcSideBarItem.h"
#include "Context.h"
#include "Athlete.h"
#include "CompareInterval.h"

class ComparePane : public QWidget
{
    Q_OBJECT;

    public:
        enum mode { season, interval };
        typedef enum mode CompareMode;

        ComparePane(Context *context, QWidget *parent, CompareMode mode=interval);

    protected:
        void dragEnterEvent(QDragEnterEvent*);
        void dragLeaveEvent(QDragLeaveEvent*);
        void dropEvent(QDropEvent *);

    signals:

    public slots:

        void clear(); // wipe away whatever is there

        void configChanged(qint32);
        void filterChanged();

        void itemsWereSorted();
        void intervalButtonsChanged();
        void daterangeButtonsChanged();

    protected:
        void refreshTable();

    private:
        Context *context;
        CompareMode mode_; // remember the mode we were created as...
        QTableWidget *table;
        QScrollArea *scrollArea;
};

class RouteDropDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT


    public:
        RouteDropDialog(QWidget *, Context *, QString name, QVector<int>&, int&);

    public slots:
        void allClicked();
        void oneClicked();
        void seasonChanged(int);

    private:
        Context *context;
        QVector<int> &seasonCount;
        int &selected;

        QPushButton *all, *one;
        QComboBox *seasonSelector;
};

#endif // _GC_ComparePane_h
