/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_DiaryWindow_h
#define _GC_DiaryWindow_h 1

#include "GoldenCheetah.h"

#include <QtGui>
#include <QStackedWidget>

#include "Context.h"
#include "RideMetadata.h"

// list view
#include "RideNavigator.h"

// monthly view
#include <QTableView>
#include "GcCalendarModel.h"

class DiaryWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int view READ view WRITE setView USER true)

    public:

        DiaryWindow(Context *);

        int view() const { return 0; /* viewMode->currentIndex(); */ }
        void setView(int /* x */ ) { /* viewMode->setCurrentIndex(x); */ }

        bool isFiltered() const { return (context->ishomefiltered || context->isfiltered); }

    public slots:
        void rideSelected();
        void configChanged(qint32);
        void nextClicked();
        void prevClicked();
        void dateChanged(const QDate &date);
        void setDefaultView(int);
        bool eventFilter(QObject *object, QEvent *e); // traps monthly view

    protected:
        Context *context;
        RideItem *ride;

        QDateEdit *title;
        QPushButton *prev, *next;

        QComboBox *viewMode;
        QStackedWidget *allViews;

        QTableView *monthlyView;
        GcCalendarModel *calendarModel;

        bool active;
        QList<FieldDefinition> fieldDefinitions;
};
#endif // _GC_DiaryWindow_h
