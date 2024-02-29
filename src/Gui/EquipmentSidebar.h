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

#ifndef _GC_EquipmentSidebar_h
#define _GC_EquipmentSidebar_h 1

#include "GoldenCheetah.h"

#include "Context.h"
#include "GcSideBarItem.h"
#include "EquipmentNavigator.h"

#include <QTreeWidgetItem>
#include <QTreeWidget>
#include <QWidget>

class EquipmentSidebar : public QWidget
{
    Q_OBJECT

    public:

        EquipmentSidebar(Context *context, bool eqListView);
		~EquipmentSidebar();

        EquipmentNavigator *equipmentNavigator_;

    protected slots:

        // config etc
        void configChanged(qint32);

        // equipment menu
        void equipmentPopup();
        void showEquipmentMenu(const QPoint& pos);

        // equipment selected notification
        void equipmentSelected(EquipmentNode* eqNode, bool eqListView);

		// equipment menu handlers
		void addTopDistEqHandler();
		void addTopTimeEqHandler();
		void addEqLinkHandler();
		void loadEqSettingsHandler();
		void saveEqSettingsHandler();
		void warnOnEqDeleteHandler();

		// equipment menu handlers
		void addEquipmentHandler();
		void addEquipmentTimeHandler();
		void deleteEquipmentHandler();
		void moveUpEquipmentHandler();
		void moveDownEquipmentHandler();
		void leftShiftEquipmentHandler();

private:

		bool eqListView_;
		bool warnOnEqDelete_;
        Context *context_;
        GcSplitter *splitter_;
        EquipmentNode *currentEqItem_;
        QWidget *activityHistory_;
        GcSplitterItem *activityItem_;
};

#endif // _GC_EquipmentSidebar_h
