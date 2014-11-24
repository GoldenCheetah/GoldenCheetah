/*
 * Copyright (c) 2014 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_MUWidget_h
#define _GC_MUWidget_h 1

#include "GoldenCheetah.h"
#include "MUPlot.h"
#include "MUPool.h"

#include "CriticalPowerWindow.h"
#include <QWidget>
#include <QLabel>
#include <QDoubleSpinBox>

class MUWidget : public QWidget
{
    Q_OBJECT

    public:

        MUWidget(CriticalPowerWindow *parent, Context *);

    public slots:

        // colors/appearance changed
        void configChanged();

    public:

        Context *context;
        CriticalPowerWindow *parent;
        MUPlot *muPlot;

        // editables
        QLabel *massTitle;
        QDoubleSpinBox *mass;

        QLabel *maxTitle, *minTitle;
        QLabel *max, *min;

        QLabel *tauTitle;
        QDoubleSpinBox *tau0,*tau1;

        QLabel *pmaxTitle;
        QDoubleSpinBox *pmax0,*pmax1;

        QLabel *pminTitle;
        QDoubleSpinBox *pmin0,*pmin1;

        QLabel *alphaTitle;
        QDoubleSpinBox *alpha;
};

#endif // _GC_MUWidget_h
