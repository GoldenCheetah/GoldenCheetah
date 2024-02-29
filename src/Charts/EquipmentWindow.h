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

#ifndef _GC_EquipmentWindow_h
#define _GC_EquipmentWindow_h 1

#include "GoldenCheetah.h"
#include "EquipmentNode.h"

#include <QLabel>
#include <QLineEdit>
#include <QStackedWidget>
#include <QCheckBox>
#include <QVector>

 // The enum class eqWinType must align with the addition of widgets to the stackedWidget_
enum class eqWinType { BLANK_PAGE = 0, EQUIPMENT_DIST_PAGE, EQUIPMENT_TIME_PAGE, REFERENCE_PAGE, ACTIVITY_LINK_PAGE };

class EquipmentWindow : public GcChartWindow
{
    Q_OBJECT

    public:

		struct widgetMapType {
			QWidget* labelPtr;
			QWidget* fieldPtr;
			bool editable;
		};

        EquipmentWindow(Context *context);

	public slots:
		void updateEqSettings(bool load);
		void equipmentSelected(EquipmentNode* eqNode, bool eqListView);
		void eqRecalculationStart();
		void eqRecalculationEnd();

    protected slots:
        
        void configChanged(qint32);
		void saveButtonClicked();
		void undoButtonClicked();
		void startDateTimeStateChanged(int checkState);
		void endDateTimeStateChanged(int checkState);

    protected:

		void updateEquipmentDisplay();
		QString unitString();

		// prevent menu appearing (the menu allows the window to be closed)
		virtual void enterEvent(QEvent*) override {}

		// the equipment window's widgets
		QMap<eqWinType, QVector< widgetMapType>> equipWids_;

		QWidget* createWidsEquipmentDistanceItem();
		void displayEquipmentDistanceItem(EquipmentDistanceItem* eqNode);
		void saveEquipmentDistanceItem(EquipmentDistanceItem* eqNode);

		QWidget* createWidsEquipmentTimeItem();
		void displayEquipmentTimeItem(EquipmentTimeItem* eqNode);
		void saveEquipmentTimeItem(EquipmentTimeItem* eqNode);

		QWidget* createWidsReference();
		void displayReference(EquipmentRef* eqRef);
		void saveReference(EquipmentRef* eqRef);

		QWidget* createWidsEquipmentLink();
		void displayEquipmentLink(EquipmentLink* eqLink);
		void saveEquipmentLink(EquipmentLink* eqLink);

		QWidget* setupWids(QVector< widgetMapType>& widMap);

		Context* context_;
		EquipmentNode* selectedNode_;
		QStackedWidget* stackedWidget_;
		QLabel* lastUpdated_;
		bool metricUnits_;
		bool savingDisabled_; // during recalcuation
		QPushButton* saveButton_;
		QPushButton* undoButton_;
        QPalette palette_; // applied to all widgets

};

#endif // _GC_EquipmentWindow_h
