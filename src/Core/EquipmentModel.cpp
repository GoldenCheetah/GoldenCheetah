/*
 * Copyright (c) 2024 Paul Johnson (paulj49457@gmail.com)
 * Drag & Drop Copyright (c) 2024 Patou355 (https://forum.qt.io/topic/76708/full-drag-and-drop-support-in-qtreeview/2)
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

#include "GoldenCheetah.h"
#include "Context.h"
#include "AbstractView.h"
#include "Athlete.h"

#include "MainWindow.h"
#include "Utils.h"
#include "EquipmentModel.h"

 // ---------------------------- EquipmentModel -----------------------------------

EquipmentModel::EquipmentModel(Context *context) :
        QAbstractItemModel(context), context_(context)
{
    configChanged(CONFIG_FIELDS | CONFIG_NOTECOLOR);

    connect(context_, SIGNAL(equipmentAdded(EquipmentNode*, int)), this, SLOT(equipmentAdded(EquipmentNode*, int)));
    connect(context_, SIGNAL(equipmentDeleted(EquipmentNode*)), this, SLOT(equipmentDeleted(EquipmentNode*)));
    connect(context_, SIGNAL(makeEquipmentRef(EquipmentNode*)), this, SLOT(makeEquipmentRef(EquipmentNode*)));
    connect(context_, SIGNAL(breakEquipmentRef(EquipmentNode*)), this, SLOT(breakEquipmentRef(EquipmentNode*)));
    connect(context_, SIGNAL(equipmentMove(EquipmentNode*, int)), this, SLOT(equipmentMove(EquipmentNode*, int)));

    connect(context_, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    rootItem_ = new EquipmentRoot();
}

EquipmentModel::~EquipmentModel()
{
    // deletes all of roots children, each child destructor deletes their own children
    delete rootItem_;
}

void
EquipmentModel::resetModel() {

    // deletes all of roots children, each child destructor deletes their own children
    QVector<EquipmentNode*>& refChildrenToDelete = rootItem_->getChildren();
    while (refChildrenToDelete.size()) {
        delete refChildrenToDelete.takeLast();
    }
}

// ------------------------------ QTreeView functions --------------------------------

QModelIndex EquipmentModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();

     EquipmentNode* parentItem = (parent.isValid()) ? static_cast<EquipmentNode*>(parent.internalPointer()) : rootItem_;

    EquipmentNode* childItem = parentItem->child(row);
    if (childItem) return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex EquipmentModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) return QModelIndex();

    EquipmentNode* childItem = static_cast<EquipmentNode*>(index.internalPointer());
    EquipmentNode* parentItem = childItem->getParentItem();

    if (parentItem == rootItem_) return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int EquipmentModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0) return 0;

    EquipmentNode* parentItem = (parent.isValid()) ? parentItem = static_cast<EquipmentNode*>(parent.internalPointer()) : parentItem = rootItem_;

    return parentItem->childCount();
}

int EquipmentModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return static_cast<EquipmentNode*>(parent.internalPointer())->columnCount();

    return rootItem_->columnCount();
}

QVariant EquipmentModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    EquipmentNode* item = static_cast<EquipmentNode*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags EquipmentModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;

    return QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

//
// ------------------------------ drag & drop --------------------------------
// Code adapted from (https://forum.qt.io/topic/76708/full-drag-and-drop-support-in-qtreeview/2)

static const char s_EquipmentItemMimeType[] = "application/x-equipmentitem";

//returns the mime type
QStringList
EquipmentModel::mimeTypes() const
{
    return QStringList() << s_EquipmentItemMimeType;
}

//receives a list of model indexes list
QMimeData*
EquipmentModel::mimeData(const QModelIndexList& indexes) const
{
    QMimeData* mimeData = new QMimeData;
    QByteArray data; //a kind of RAW format for datas

    // QDataStream is independent of the OS or proc architecture
    // serialization of C++'s basic data types, like char, short, int, char *, etc.
    // Serialization of more complex data is accomplished
    // by breaking up the data into primitive units.
    QDataStream stream(&data, QIODevice::WriteOnly);
    QList<EquipmentNode*> nodes;

    foreach(const QModelIndex & index, indexes) {
        EquipmentNode* node = equipmentFromIndexPtr(index);
        if (!nodes.contains(node))
            nodes << node;
    }
    stream << QCoreApplication::applicationPid();
    stream << nodes.count();

    foreach(EquipmentNode * node, nodes) {
        stream << reinterpret_cast<qlonglong>(node);
    }
    mimeData->setData(s_EquipmentItemMimeType, data);
    return mimeData;
}

bool
EquipmentModel::dropMimeData(const QMimeData* mimeData, Qt::DropAction action, int row, int column, const QModelIndex& droppedOnNodeIdx)
{
    Q_ASSERT(action == Qt::MoveAction);
    Q_UNUSED(column);

    // test if the data type is the good one
    if (!mimeData->hasFormat(s_EquipmentItemMimeType)) return false;

    QByteArray data = mimeData->data(s_EquipmentItemMimeType);
    QDataStream stream(&data, QIODevice::ReadOnly);
    qint64 senderPid;
    stream >> senderPid;

    // Let's not cast pointers that come from another process...
    if (senderPid != QCoreApplication::applicationPid()) return false;

    EquipmentNode* droppedOnNode = equipmentFromIndexPtr(droppedOnNodeIdx);
    Q_ASSERT(droppedOnNode);

    int count;
    stream >> count;

    // invalid index means an attempt to append beyond the last item
    if (row == -1) {
        if (droppedOnNodeIdx.isValid()) row = 0; else return false;
    }

    for (int i = 0; i < count; ++i) {

        // Decode data from the QMimeData
        qlonglong draggedNodePtr;
        stream >> draggedNodePtr;
        EquipmentNode* draggedNode = reinterpret_cast<EquipmentNode*>(draggedNodePtr);

        // Only allow equipment time span, equipment items & references to be dragged & dropped,
        // as activity links & equipment tree are strictly top level items.
        switch (draggedNode->getEqNodeType()) {

            case eqNodeType::EQ_ITEM_REF:
            case eqNodeType::EQ_DIST_ITEM:
            case eqNodeType::EQ_BANNER:
            case eqNodeType::EQ_TIME_ITEM: {
                if ((droppedOnNode->getEqNodeType() == eqNodeType::EQ_LINK) ||
                    (droppedOnNode->getEqNodeType() == eqNodeType::EQ_ITEM_REF) ||
                    (droppedOnNode->getEqNodeType() == eqNodeType::EQ_DIST_ITEM) ||
                    (droppedOnNode->getEqNodeType() == eqNodeType::EQ_TIME_ITEM) ||
                    (droppedOnNode->getEqNodeType() == eqNodeType::EQ_BANNER)) {

                            completeDropMimeData(draggedNode, row, droppedOnNode, droppedOnNodeIdx);
                        }
                else {
                    return false;
                }
            } break;

            case eqNodeType::EQ_SHARED_DIST_ITEM: {
                if ((droppedOnNode->getEqNodeType() == eqNodeType::EQ_SHARED) ||
                    (droppedOnNode->getEqNodeType() == eqNodeType::EQ_SHARED_DIST_ITEM)) {

                    completeDropMimeData(draggedNode, row, droppedOnNode, droppedOnNodeIdx);

                } else if ((droppedOnNode->getEqNodeType() == eqNodeType::EQ_LINK) ||
                            (droppedOnNode->getEqNodeType() == eqNodeType::EQ_ITEM_REF) ||
                            (droppedOnNode->getEqNodeType() == eqNodeType::EQ_DIST_ITEM) ||
                            (droppedOnNode->getEqNodeType() == eqNodeType::EQ_BANNER) ||
                            (droppedOnNode->getEqNodeType() == eqNodeType::EQ_TIME_ITEM)) {

                    // Create a reference copy of the node and its children
                    createReferenceCopy(draggedNode, row, droppedOnNode, droppedOnNodeIdx);

                    // Must not move the actual Equipment Shared Distance Item, so cancel the drag & drop.
                    return false;

                } else {
                    return false;
                }
            } break;

            // All other equipment types cannot be dragged & dropped
            default: { return false; } break;
        }
    }
    return true;
}

void
EquipmentModel::completeDropMimeData(EquipmentNode* draggedNode, int droppedRow, EquipmentNode* droppedOnNode, const QModelIndex& droppedOnNodeIdx) {

    // Adjust destination row for the case of moving an item
    // within the same parent, to a position further down.
    // Its own removal will reduce the final row number by one.
    if (draggedNode->row() < droppedRow && droppedOnNode == draggedNode->getParentItem()) --droppedRow;

    // Remove from old position
    removeEquipment(draggedNode);

    // Insert at new position
    insertEquipment(draggedNode, droppedRow, droppedOnNode, droppedOnNodeIdx);
}

// always returns a pointer for an "index"
EquipmentNode*
EquipmentModel::equipmentFromIndexPtr(const QModelIndex& index) const
{
    return (index.isValid()) ? static_cast<EquipmentNode*>(index.internalPointer()) : rootItem_;
}

void
EquipmentModel::removeEquipment(EquipmentNode* eqNode)
{
    const int row = eqNode->row();
    QModelIndex idx = createIndex(row, 0, eqNode);
    beginRemoveRows(idx.parent(), row, row);
    eqNode->getParentItem()->removeChild(row);
    endRemoveRows();
}

void
EquipmentModel::removeAndDeleteEquipment(EquipmentNode* eqNode)
{
    emit layoutAboutToBeChanged();

    removeEquipment(eqNode);
    delete eqNode;

    emit layoutChanged();
}

void
EquipmentModel::insertEquipment(EquipmentNode* node, int row, EquipmentNode* parentNode, const QModelIndex& parentIdx) {

    //qDebug() << "Inserting into" << parentIdx << row;
    beginInsertRows(parentIdx, row, row);
    parentNode->insertChild(row, node);
    endInsertRows();
}

EquipmentNode*
EquipmentModel::equipmentFromIndex(const QModelIndex& modelIndex) const
{
    return (modelIndex.isValid()) ? static_cast<EquipmentNode*>(modelIndex.internalPointer()) :nullptr;
}

// ------------------------------ GC Application --------------------------------

// when updating metadata config
void 
EquipmentModel::configChanged(qint32)
{
}

// --------------------------------- Tree of Equipment Items -----------------------------------

void
EquipmentModel::createReferenceCopy(EquipmentNode* copyNode, int destRow, EquipmentNode* destNode, const QModelIndex& destIdx) {

    // create copies of the equipment for the equipment reference tree
    EquipmentNode* refs = copyItemTreeToRefTree(copyNode);

    // insert the equipment copies into the equipment reference tree
    insertEquipment(refs, destRow, destNode, destIdx);
}

EquipmentNode* // recursive function! 
EquipmentModel::copyItemTreeToRefTree(EquipmentNode* copyNodeTree) {

    EquipmentNode* refNode = new EquipmentRef();

    // Add the equipment to the reference
    static_cast<EquipmentRef*>(refNode)->eqSharedDistNode_ = static_cast<EquipmentSharedDistanceItem*>(copyNodeTree);

    // Add the reference to the equipment
    static_cast<EquipmentSharedDistanceItem*>(copyNodeTree)->linkedRefs_.push_back(static_cast<EquipmentRef*>(refNode));

    for (EquipmentNode* eqItem : copyNodeTree->getChildren())
    {
        addChildToParent(copyItemTreeToRefTree(eqItem), refNode);
    }
    return refNode;
}

void
EquipmentModel::addChildToParent(EquipmentNode* eqChild, EquipmentNode* eqParent)
{
    eqChild->setParentItem(eqParent);
    eqParent->appendChild(eqChild);
}

void // recursive function! 
EquipmentModel::printfTree(int depth, EquipmentNode* eqNodeTree) {

    depth += 3;
    for (int i = 0; i < depth; i++) { putchar(' '); }

    printfEquipmentNode(eqNodeTree);

    for (EquipmentNode* eqNode : eqNodeTree->getChildren()) {
        printfTree(depth, eqNode);
    }
}

void
EquipmentModel::printfEquipmentNode(const EquipmentNode* eqNode) const {

    switch (eqNode->getEqNodeType()) {

    case eqNodeType::EQ_LINK: {
        printf("%s\n", static_cast<const EquipmentLink*>(eqNode)->data(1).toString().toStdString().c_str());
    } break;
    case eqNodeType::EQ_DIST_ITEM: {
        printf("%s\n", static_cast<const EquipmentDistanceItem*>(eqNode)->data(1).toString().toStdString().c_str());
    } break;
    case eqNodeType::EQ_SHARED_DIST_ITEM: {
        printf("%s\n", static_cast<const EquipmentSharedDistanceItem*>(eqNode)->data(1).toString().toStdString().c_str());
    } break;
    case eqNodeType::EQ_TIME_ITEM: {
        printf("%s\n", static_cast<const EquipmentTimeItem*>(eqNode)->data(1).toString().toStdString().c_str());
    } break;
    case eqNodeType::EQ_ITEM_REF: {
        printf("%s\n", static_cast<const EquipmentRef*>(eqNode)->data(1).toString().toStdString().c_str());
    } break;
    case eqNodeType::EQ_SHARED: {
        printf("%s\n", static_cast<const SharedEquipment*>(eqNode)->data(1).toString().toStdString().c_str());
    } break;
    case eqNodeType::EQ_BANNER: {
        printf("%s\n", static_cast<const EquipmentBanner*>(eqNode)->data(1).toString().toStdString().c_str());
    } break;
    case eqNodeType::EQ_ROOT: {
        printf("%s\n", static_cast<const EquipmentRoot*>(eqNode)->data(1).toString().toStdString().c_str());
    } break;
    default: {
        qDebug() << "Trying to display an unknown equipment type!";
    }break;
    }
}

// --------------------------------- EquipmentModel -----------------------------------

void
EquipmentModel::equipmentAdded(EquipmentNode* eqParent, int eqToAdd) {

    emit layoutAboutToBeChanged();

    eqNodeType addEquip = static_cast<eqNodeType>(eqToAdd);

    switch (addEquip) {

    case eqNodeType::EQ_LINK: {
        EquipmentLink* eqLink = new EquipmentLink(tr("New Eq Link"));
        addChildToParent(eqLink, rootItem_);
    } break;

    case eqNodeType::EQ_DIST_ITEM: {
        EquipmentNode* eqNode = new EquipmentDistanceItem(tr("New Distance Equipment"), "");
        addChildToParent(eqNode, eqParent);
    } break;

    case eqNodeType::EQ_SHARED_DIST_ITEM: {
        EquipmentNode* eqNode = new EquipmentSharedDistanceItem(tr("New Shared Distance Equipment"), "");
        addChildToParent(eqNode, eqParent);
    } break;

    case eqNodeType::EQ_TIME_ITEM: {
        EquipmentNode* eqNode = new EquipmentTimeItem(tr("New Time Equipment"), "");
        addChildToParent(eqNode, eqParent);
    } break;

    case eqNodeType::EQ_BANNER: {
        EquipmentNode* eqNode = new EquipmentBanner(tr("New Banner"));
        addChildToParent(eqNode, eqParent);
    } break;

    default: {
        qDebug() << "Trying to add invalid equipment node id:" << eqToAdd;
    } break;
    }

    emit layoutChanged();
}

void
EquipmentModel::equipmentDeleted(EquipmentNode* eqNode)
{
    if (eqNode->childCount()) {
        qDebug() << "Trying to delete equipment with children!";
        return;
    }

    switch (eqNode->getEqNodeType()) {

    case eqNodeType::EQ_SHARED_DIST_ITEM: {

        // Ensure the shared equipment item has no references
        if (!static_cast<EquipmentSharedDistanceItem*>(eqNode)->linkedRefs_.empty()) {

            QMessageBox msgBox;
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("Shared equipment with references cannot be deleted!"));
            msgBox.setInformativeText(tr("\nTo proceed deleted the item's references\n\n"));
            msgBox.exec();
        }
        else {
            removeAndDeleteEquipment(eqNode);
        }
    } break;

    case eqNodeType::EQ_LINK:
    case eqNodeType::EQ_BANNER:
    case eqNodeType::EQ_DIST_ITEM:
    case eqNodeType::EQ_TIME_ITEM: {
        removeAndDeleteEquipment(eqNode);
    } break;

    case eqNodeType::EQ_ITEM_REF: {

        // Remove reference from related Equipment Shared Distance Item
        if (static_cast<EquipmentRef*>(eqNode)->eqSharedDistNode_ != nullptr)
                static_cast<EquipmentRef*>(eqNode)->eqSharedDistNode_->linkedRefs_.removeOne(eqNode);

        removeAndDeleteEquipment(eqNode);
    } break;

    default: {
        qDebug() << "Trying to delete invalid equipment node id:"
                 << static_cast<int>(eqNode->getEqNodeType());
    } break;
    }
}

void
EquipmentModel::makeEquipmentRef(EquipmentNode* eqNode) {

    SharedEquipment* eqSharedNode = nullptr;

    // find the root item's shared equipment node
    for (EquipmentNode* rootChild : rootItem_->getChildren()) {

        if (rootChild->getEqNodeType() == eqNodeType::EQ_SHARED) {
            eqSharedNode = static_cast<SharedEquipment*>(rootChild);
            break;
        }
    }

    // ensure the equipment is correctly configured before converting it to a reference
    if ((eqNode != nullptr) &&
        (eqNode->getEqNodeType() == eqNodeType::EQ_DIST_ITEM) &&
        (eqSharedNode != nullptr))
    {
        const EquipmentDistanceItem* eqDistCopy = static_cast<EquipmentDistanceItem*>(eqNode);

        // Create the shared distance equipment
        EquipmentSharedDistanceItem* eqSharedDistNode = new EquipmentSharedDistanceItem(eqDistCopy->description_, eqDistCopy->notes_,
            eqDistCopy->getNonGCDistance(), eqDistCopy->getGCDistance(), eqDistCopy->replacementDistance_);

        // Create the equipment ref
        EquipmentRef* eqRefNode = new EquipmentRef(eqDistCopy->getGCDistance(), eqDistCopy->start_, eqDistCopy->end_);

        // Set up the cross reference links
        eqSharedDistNode->linkedRefs_.push_back(eqRefNode);
        eqRefNode->eqSharedDistNode_ = eqSharedDistNode;

        emit layoutAboutToBeChanged();

        // Add new shared distance equipment
        addChildToParent(eqSharedDistNode, eqSharedNode);

        // Add new equipment reference
        addChildToParent(eqRefNode, eqNode->getParentItem());

        // Delete distance equipment
        removeAndDeleteEquipment(eqNode);

        emit layoutChanged();
    }
}

void
EquipmentModel::breakEquipmentRef(EquipmentNode* eqNode) {

    // ensure the equipment is correctly configured before breaking its reference
    if ((eqNode != nullptr) &&
        (eqNode->getEqNodeType() == eqNodeType::EQ_ITEM_REF))
    {
        EquipmentRef* eqRefNode = static_cast<EquipmentRef*>(eqNode);
        const EquipmentSharedDistanceItem* eqSharedCopy = static_cast<EquipmentRef*>(eqRefNode)->eqSharedDistNode_;

        if (eqSharedCopy != nullptr)
        {
            EquipmentDistanceItem* eqDistNode = new EquipmentDistanceItem(eqSharedCopy->description_, eqSharedCopy->notes_,
                eqSharedCopy->getNonGCDistance(), eqRefNode->getRefDistanceCovered(), eqSharedCopy->replacementDistance_,
                eqRefNode->start_, eqRefNode->end_);

            // Remove reference from related Shared Equipment Distance Item
            eqRefNode->eqSharedDistNode_->linkedRefs_.removeOne(eqRefNode);

            emit layoutAboutToBeChanged();

            // Add new distance equipment
            addChildToParent(eqDistNode, eqRefNode->getParentItem());

            // Delete equipmentRef
            removeAndDeleteEquipment(eqRefNode);

            emit layoutChanged();
        }
    }
}

void
EquipmentModel::equipmentMove(EquipmentNode* eqNode, int move) {

    emit layoutAboutToBeChanged();

    QVector<EquipmentNode*>& parentsChildren = eqNode->getParentItem()->getChildren();
    int pos = parentsChildren.indexOf(eqNode);

    if (pos != 0) {
        // move up
        if (static_cast<eqNodeMoveType>(move) == eqNodeMoveType::EQ_UP) {
            parentsChildren.move(pos, pos - 1);
        }
        else {
            // move top
            if (static_cast<eqNodeMoveType>(move) == eqNodeMoveType::EQ_TOP) {
                parentsChildren.move(pos, 0);
            }
        }
    }

    if (pos != parentsChildren.size() - 1) {
        // move down
        if (static_cast<eqNodeMoveType>(move) == eqNodeMoveType::EQ_DOWN) {
            parentsChildren.move(pos, pos + 1);
        }
        else {
            // move bottom
            if (static_cast<eqNodeMoveType>(move) == eqNodeMoveType::EQ_BOTTOM) {
                parentsChildren.move(pos, parentsChildren.size() - 1);
            }
        }
    }

    // left shift so ensure my parent isn't root
    if ((static_cast<eqNodeMoveType>(move) == eqNodeMoveType::EQ_LEFT_SHIFT) &&
        (eqNode->getParentItem()->getEqNodeType() != eqNodeType::EQ_ROOT) &&
        (eqNode->getParentItem()->getEqNodeType() != eqNodeType::EQ_SHARED)) {

        // find new parent
        EquipmentNode* eqNewParent = eqNode->getParentItem()->getParentItem();

        // leave old parent
        eqNode->getParentItem()->removeChild(eqNode);

        // attach to new parent
        addChildToParent(eqNode, eqNewParent);
    }

    emit layoutChanged();
}

