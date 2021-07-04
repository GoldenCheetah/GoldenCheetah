/*
 * Copyright (c) 2021 Mark Liversedge <liversedge@gmail.com>
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

#ifndef _GC_PerspectiveDialog_h
#define _GC_PerspectiveDialog_h 1
#include "GoldenCheetah.h"
#include "TabView.h"
#include "Perspective.h"

#include <QtGui>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>

class Context;

// reimplement QTableWidget to accept and process drop events
class PerspectiveTableWidget : public QTableWidget
{
    Q_OBJECT

    public:
        PerspectiveTableWidget(QWidget *parent) : QTableWidget(parent) { setMouseTracking(true); }

        // process drag and drop
        virtual void dragEnterEvent(QDragEnterEvent *);
        virtual void dragMoveEvent(QDragMoveEvent *);
        virtual void dropEvent(QDropEvent *);

    signals:
        void chartMoved(GcChartWindow*);

};

class ChartTableWidget : public QTableWidget
{
    Q_OBJECT

    public:
        ChartTableWidget(QWidget *parent) : QTableWidget(parent) {}

        virtual QStringList mimeTypes() const;
        virtual QMimeData *mimeData (const QList<QTableWidgetItem *> items) const;
};

class PerspectiveDialog : public QDialog
{
    Q_OBJECT
    G_OBJECT

    public:
        PerspectiveDialog(QWidget *parent, TabView *tabView);

    private slots:

        void perspectiveSelected();
        void setTables();
        void close();

        void removePerspectiveClicked();
        void addPerspectiveClicked();

        void upPerspectiveClicked();
        void downPerspectiveClicked();

        void perspectiveNameChanged(QTableWidgetItem *);

    signals:
        void perspectivesChanged();

    private:
        TabView *tabView;

        PerspectiveTableWidget *perspectiveTable;
        ChartTableWidget *chartTable;

        QPushButton *addPerspective, *removePerspective;
        QToolButton *upPerspective, *downPerspective;
        QLabel *instructions;

        QPushButton *closeButton;

        bool active;
};

#endif // _GC_PerspectiveDialog_h
