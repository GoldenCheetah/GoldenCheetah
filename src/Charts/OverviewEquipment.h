/*
 * Copyright (c) 2024 Paul Johnson (paulj49457@gmail.com)
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

#ifndef _GC_OverviewEquipmentWindow_h
#define _GC_OverviewEquipmentWindow_h 1

#include "Overview.h"
#include "ChartSpace.h"

class OverviewEquipmentWindow : public OverviewWindow
{
    Q_OBJECT

    public:

        OverviewEquipmentWindow(Context* context, int scope = OverviewScope::EQUIPMENT, bool blank = false);
        virtual ~OverviewEquipmentWindow();

        void showChart(bool visible) override;

    public slots:

        ChartSpaceItem* addTile() override;
        void configItem(ChartSpaceItem*, QPoint) override;
        void cloneTile(ChartSpaceItem*);
        void calculationComplete();

        // athlete opening
        void openingAthlete(QString, Context*);
        void loadDone(QString, Context*);

        void configChanged(qint32 cfg);

    protected:

        QString getChartSource() const override;
        void getExtraConfiguration( ChartSpaceItem* item, QString& config) const override;
        ChartSpaceItem* setExtraConfiguration(QJsonObject& obj, int type, QString& name, QString& datafilter,
                                              int order, int column, int span, int deep) const override;

    private:
        bool reCalcOnVisible_;
        EquipCalculator* eqCalc_;

};

#endif // _GC_OverviewEquipmentWindow_h
