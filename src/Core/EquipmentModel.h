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

#ifndef _GC_EquipmentModel_h
#define _GC_EquipmentModel_h 1

#include "GoldenCheetah.h"
#include "EquipmentNode.h"

//#include <QXmlDefaultHandler>
//#include <QTextStream>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

class EquipmentModelManager;

class EquipmentModel : public QAbstractItemModel
{
	Q_OBJECT

    public:
    
		EquipmentModel(Context* context);
		~EquipmentModel();

		void equipmentDeleted(EquipmentNode* eqItem, bool warnOnEqDelete);
		EquipmentNode* equipmentFromIndex(const QModelIndex& modelIndex) const;

	public slots:

        // when updating metadata config
		void configChanged(qint32);

	protected:

		friend class ::EquipmentModelManager;

		// Minimal QAbstractItemModel function overrides
		QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
		QModelIndex parent(const QModelIndex& index) const override;
		int rowCount(const QModelIndex& parent = QModelIndex()) const override;
		int columnCount(const QModelIndex& parent = QModelIndex()) const override;
		QVariant data(const QModelIndex& index, int role) const override;
		Qt::ItemFlags flags(const QModelIndex& index) const override;

		// Drag & drop functions
		Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }
		Qt::DropActions supportedDragActions() const override { return Qt::MoveAction; }
		QStringList mimeTypes() const override;
		QMimeData* mimeData(const QModelIndexList& indexes) const override;
		bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent);
		void completeDropMimeData(EquipmentNode* draggedNode, int droppedRow, EquipmentNode* droppedOnNode, const QModelIndex& droppedOnNodeIdx);

		void equipmentAdded(EquipmentNode* eqParent, int eqToAdd);
		void equipmentMove(EquipmentNode* eqItem, bool up);
		void addChildToParent(EquipmentNode* eqChild, EquipmentNode* eqParent);

		void removeEquipment(EquipmentNode* eqNode);
		void removeAndDeleteEquipment(EquipmentNode* eqNode);
		void deleteEquipmentsRefsMatching(const EquipmentNode* eqNode);
		void findAndDeleteMatchingEqRefs(EquipmentNode* eqNodeTree, const EquipmentNode* matchEqNode); // recursive

		void insertEquipment(EquipmentNode* node, int row, EquipmentNode* parentNode, const QModelIndex& parentIdx);
		EquipmentNode* equipmentFromIndexPtr(const QModelIndex& index) const;

		// Creating a reference copy of the Equipment Item tree
		void createReferenceCopy(EquipmentNode* copyNode, int destRow, EquipmentNode* destNode, const QModelIndex& destIdx);
		EquipmentNode* copyItemTreeToRefTree(EquipmentNode* copyNodeTree); // recursive

		// for debugging
		void printfTree(int depth, EquipmentNode* eqItemTree); // recursive
		void printfEquipmentNode(const EquipmentNode* eqItem) const;

		Context *context_;
		EquipmentRoot* rootItem_;
};

#endif // _GC_EquipmentModel_h
