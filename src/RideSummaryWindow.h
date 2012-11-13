/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_RideSummaryWindow_h
#define _GC_RideSummaryWindow_h 1
#include "GoldenCheetah.h"
#include "MainWindow.h"

#include <QWidget>
#include <QWebView>
#include <QWebFrame>

#include "SummaryMetrics.h"


class RideSummaryWindow : public GcWindow
{
    Q_OBJECT
    G_OBJECT


    public:

        // two modes - summarise ride or summarise date range
        RideSummaryWindow(MainWindow *parent, bool ridesummary = true);

    protected slots:

        void refresh();
        void rideSelected();
        void dateRangeChanged(DateRange);
        void rideItemChanged();
        void metadataChanged();

    protected:

        QString htmlSummary() const;

        MainWindow *mainWindow;
        QWebView *rideSummary;

        RideItem *_connected;
        bool ridesummary; // do we summarise ride or daterange?

        QList<SummaryMetrics> data; // when in date range mode
        DateRange current;
};

#endif // _GC_RideSummaryWindow_h

