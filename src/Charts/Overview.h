/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_OverviewWindow_h
#define _GC_OverviewWindow_h 1

// basics
#include "GoldenCheetah.h"
#include "Settings.h"
#include "Units.h"
#include "Colors.h"
#include "Context.h"
#include "Athlete.h"
#include "RideItem.h"
#include "RideMetric.h"
#include "HrZones.h"
#include <QSpinBox>

#include "ChartSpace.h"
#include "OverviewItems.h"

class OverviewWindow : public GcChartWindow
{
    Q_OBJECT

    Q_PROPERTY(QString config READ getConfiguration WRITE setConfiguration USER true)
    Q_PROPERTY(int minimumColumns READ minimumColumns WRITE setMinimumColumns USER true)

    public:

        OverviewWindow(Context *context, int scope=OverviewScope::ANALYSIS, bool blank=false);

        // used by children
        Context *context;

    public slots:

        // get/set config
        QString getConfiguration() const;
        void setConfiguration(QString x);

        int minimumColumns() const { return mincolsEdit->value(); }
        void setMinimumColumns(int x) { if (x>0 && x< 11) {mincolsEdit->setValue(x); space->setMinimumColumns(x); }}

        // add a tile to the window
        void addTile();
        void importChart();
        void settings();

        // config item requested
        void configItem(ChartSpaceItem *);

    private:

        // gui setup
        ChartSpace *space;
        bool configured;
        int scope;
        bool blank;

        QSpinBox *mincolsEdit;
};

class OverviewConfigDialog : public QDialog
{
    Q_OBJECT

    public:
        OverviewConfigDialog(ChartSpaceItem*);
        ~OverviewConfigDialog();


    public slots:

        void removeItem();
        void exportChart();
        void close();

    private:
        ChartSpaceItem *item;
        QVBoxLayout *main;
        QPushButton *remove, *ok, *exp;
};

#endif // _GC_OverviewWindow_h
