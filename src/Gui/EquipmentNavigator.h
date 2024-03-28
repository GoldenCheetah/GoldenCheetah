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

#ifndef _GC_EquipmentNavigator_h
#define _GC_EquipmentNavigator_h 1
#include "GoldenCheetah.h"

#include "Context.h"
#include "Settings.h"
#include "Colors.h"
#include "EquipmentNode.h"
#include "EquipmentModel.h"

#include <QTreeView>
#include <QItemDelegate>
#include <QHeaderView>
#include <QScrollBar>
#include <QScrollArea>
#include <QDragMoveEvent>
#include <QDragEnterEvent>

class EquipmentNavigatorCellDelegate;
class EquipmentTreeView;

// The EquipmentNavigator - a list of equipment displayed using a QTreeView

class EquipmentNavigator : public GcChartWindow
{
    Q_OBJECT

    friend class ::EquipmentNavigatorCellDelegate;

    public:
        EquipmentNavigator(Context* context, bool eqListView);
        ~EquipmentNavigator();

        void borderMenu(const QPoint& pos);

        // the tree view, the cell delegate needs access
        EquipmentTreeView* eqTreeView_;

		// the equipment view noitifes me the splitter has moved
		void widthChanged();

    public slots:

        void configChanged(qint32);

        void showEvent(QShowEvent* event);
        void resizeEvent(QResizeEvent*);

        void showTreeContextMenuPopup(const QPoint&);

        // user selection so line up
        void eqSelectionChanged(QItemSelection);

		// Equipment calculation complete
		void eqRecalculationEnd();

     protected:

		void setWidth();

		bool eqListView_;
		Context* context_;
		static EquipmentNode* currentEqItem_;

		int pwidth_; // selction box width
        int fontHeight; // font metrics for display etc

};

//
// Used to paint the cells in the ride navigator
//
class EquipmentNavigatorCellDelegate : public QItemDelegate
{
    Q_OBJECT
        G_OBJECT

    public:
        EquipmentNavigatorCellDelegate(EquipmentNavigator* eqNav, EquipmentModel* eqModel, QObject* parent = 0);

        // These are all null since we don't allow editing
        QWidget* createEditor(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const { return NULL; }
        void setEditorData(QWidget*, const QModelIndex&) const { }
        void setModelData(QWidget*, QAbstractItemModel*, const QModelIndex&) const { }
        void updateEditorGeometry(QWidget*, const QStyleOptionViewItem&, const QModelIndex&) const { }

        // We increase the row height if there is a calendar text to display
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;

        // override standard painter to use color config to paint background
        // and perform correct level of rounding for each column before displaying
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

    public slots:
        // These are null since we don't allow editing
        bool helpEvent(QHelpEvent*, QAbstractItemView*, const QStyleOptionViewItem&, const QModelIndex&) { return true; }

    private slots:
        // These are null since we don't allow editing
        void commitAndCloseEditor() {}

    private:
		bool eqListView_;
		EquipmentModel* eqModel_;
		EquipmentNavigator* eqNav_;
};

class EquipmentTreeView : public QTreeView
{
    Q_OBJECT;

    public:
        EquipmentTreeView(Context*, EquipmentNavigator* eqNav, EquipmentModel* model);
		~EquipmentTreeView();

    signals:
        void eqRowSelected(QItemSelection);

    public slots:
        void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
            eqRowSelected(selected);
            QTreeView::selectionChanged(selected, deselected);
        }

        void equipmentSelected(EquipmentNode* eqNode, bool eqListView);

    protected:

		EquipmentNavigatorCellDelegate* delegate_;
        Context* context_;
};

#endif
