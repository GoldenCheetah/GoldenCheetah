/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_MetadataWindow_h
#define _GC_MetadataWindow_h 1
#include "GoldenCheetah.h"
#include "RideItem.h"
#include "RideMetadata.h"

class Context;

class MetadataWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    public:

        MetadataWindow(Context *context);

    protected slots:

        void rideItemChanged();
        void configChanged(qint32);

    protected:

        // the chart displays information related to the selected ride
        bool selectedRideInfo() const override { return true; }

        Context *context;
        RideMetadata *rideMetadata;
};

#endif // _GC_MetadataWindow_h

