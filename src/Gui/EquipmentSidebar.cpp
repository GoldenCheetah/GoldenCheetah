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

#include "EquipmentSidebar.h"
#include "MainWindow.h"
#include "Context.h"
#include "Settings.h"
#include "HelpWhatsThis.h"

#include <QtGui>
#include <QDebug>


EquipmentSidebar::EquipmentSidebar(Context *context, bool eqListView) :
    context_(context), QWidget(context->mainWindow), warnOnEqDelete_(true),
	eqListView_(eqListView), currentEqItem_(nullptr)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->setSpacing(0);
    setContentsMargins(0,0,0,0);
    
    splitter_ = new GcSplitter(Qt::Vertical);
    mainLayout->addWidget(splitter_);

    // Equipment Navigator
    equipmentNavigator_ = new EquipmentNavigator(context_, eqListView_);
    equipmentNavigator_->showMore(false);

    activityHistory_ = new QWidget(this);
    activityHistory_->setContentsMargins(0,0,0,0);
#ifndef Q_OS_MAC // not on mac thanks
    activityHistory_->setStyleSheet("padding: 0px; border: 0px; margin: 0px;");
#endif
    QVBoxLayout *activityLayout = new QVBoxLayout(activityHistory_);
    activityLayout->setSpacing(0);
    activityLayout->setContentsMargins(0,0,0,0);
    activityLayout->addWidget(equipmentNavigator_);

	if (eqListView_)
		activityItem_ = new GcSplitterItem(tr("Equipment List"), iconFromPNG(":images/sidebar/folder.png"), this);

	else
		activityItem_ = new GcSplitterItem(tr("Equipment Links & Distance References"), iconFromPNG(":images/sidebar/folder.png"), this);

    QAction *activityAction = new QAction(iconFromPNG(":images/sidebar/extra.png"), tr("Menu"), this);
    activityItem_->addAction(activityAction);

    activityItem_->addWidget(activityHistory_);
 
    splitter_->addWidget(activityItem_);

    // GC signal
    connect(context_, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // Sidebar menu
    connect(activityAction, SIGNAL(triggered(void)), this, SLOT(equipmentPopup()));

    // right click menus...
    connect(equipmentNavigator_,SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(showEquipmentMenu(const QPoint &)));

    // Receive equipment selection notifications
    connect(context_, SIGNAL(equipmentSelected(EquipmentNode*, bool)), this, SLOT(equipmentSelected(EquipmentNode*, bool)));

    configChanged(CONFIG_APPEARANCE);
}

EquipmentSidebar::~EquipmentSidebar()
{
	delete equipmentNavigator_;
}

void
EquipmentSidebar::configChanged(qint32)
{
    splitter_->setPalette(GCColor::palette());
    activityHistory_->setStyleSheet(QString("background: %1;").arg(GColor(CPLOTBACKGROUND).name()));
    equipmentNavigator_->eqTreeView_->viewport()->setPalette(GCColor::palette());
    equipmentNavigator_->eqTreeView_->viewport()->setStyleSheet(QString("background: %1;").arg(GColor(CPLOTBACKGROUND).name()));

    repaint();
}

void
EquipmentSidebar::equipmentSelected(EquipmentNode* eqNode, bool eqListView) {

	if (eqListView_ == eqListView) {
		currentEqItem_ = eqNode;
	}
	else { // deselect the other equipment sidebar
		currentEqItem_ = nullptr;
		equipmentNavigator_->eqTreeView_->selectionModel()->clearSelection();
	}
}

//
// -------------------- Equipment Sidebar Menus ----------------------------------------------
//

void
EquipmentSidebar::equipmentPopup()
{
    QMenu popMenu(this);

	if (eqListView_) {

		QAction* addEquipmentLink = new QAction(tr("Add Distance Equipment"), equipmentNavigator_);
		connect(addEquipmentLink, SIGNAL(triggered(void)), this, SLOT(addTopDistEqHandler()));
		popMenu.addAction(addEquipmentLink);

		QAction* addTimeEquipment = new QAction(tr("Add Time Equipment"), equipmentNavigator_);
		connect(addTimeEquipment, SIGNAL(triggered(void)), this, SLOT(addTopTimeEqHandler()));
		popMenu.addAction(addTimeEquipment);
		popMenu.addSeparator();
	}
	else {
		QAction* recalculateEquipment = new QAction(tr("Recalculate Distances"), equipmentNavigator_);
		connect(recalculateEquipment, SIGNAL(triggered(void)), context_, SLOT(notifyEqRecalculationStart()));
		popMenu.addAction(recalculateEquipment);
		popMenu.addSeparator();

		QAction* addEquipmentLink = new QAction(tr("Add Equipment Link"), equipmentNavigator_);
		connect(addEquipmentLink, SIGNAL(triggered(void)), this, SLOT(addEqLinkHandler()));
		popMenu.addAction(addEquipmentLink);
		popMenu.addSeparator();
	}

	if (eqListView_) {

		QAction* loadEquipment = new QAction(tr("Load Equipment"), equipmentNavigator_);
		connect(loadEquipment, SIGNAL(triggered(void)), this, SLOT(loadEqSettingsHandler()));
		popMenu.addAction(loadEquipment);

		QAction* saveEquipment = new QAction(tr("Save Equipment"), equipmentNavigator_);
		connect(saveEquipment, SIGNAL(triggered(void)), this, SLOT(saveEqSettingsHandler()));
		popMenu.addAction(saveEquipment);
		popMenu.addSeparator();

		QAction* warnOnDelete;
		if (warnOnEqDelete_)
			warnOnDelete = new QAction(tr("Disable Delete Warnings"), equipmentNavigator_);
		else
			warnOnDelete = new QAction(tr("Enable Delete Warnings"), equipmentNavigator_);

		connect(warnOnDelete, SIGNAL(triggered(void)), this, SLOT(warnOnEqDeleteHandler()));
		popMenu.addAction(warnOnDelete);
		popMenu.addSeparator();
	}

	QAction* expandAll = new QAction(tr("Expand All"), equipmentNavigator_);
	connect(expandAll, SIGNAL(triggered(void)), equipmentNavigator_->eqTreeView_, SLOT(expandAll()));
	popMenu.addAction(expandAll);

	QAction* collapseAll = new QAction(tr("Collapse All"), equipmentNavigator_);
	connect(collapseAll, SIGNAL(triggered(void)), equipmentNavigator_->eqTreeView_, SLOT(collapseAll()));
	popMenu.addAction(collapseAll);

    popMenu.exec(this->mapToGlobal(QPoint(activityItem_->pos().x() + activityItem_->width() - 20, activityItem_->pos().y())));
}

void
EquipmentSidebar::addEqLinkHandler() {
	context_->notifyEquipmentAdded(currentEqItem_, static_cast<int>(eqNodeType::EQ_LINK));
}


void
EquipmentSidebar::addTopDistEqHandler() {
	context_->notifyEquipmentAdded(nullptr, static_cast<int>(eqNodeType::EQ_DIST_ITEM));
}

void
EquipmentSidebar::addTopTimeEqHandler() {
	context_->notifyEquipmentAdded(nullptr, static_cast<int>(eqNodeType::EQ_TIME_ITEM));
}

void
EquipmentSidebar::loadEqSettingsHandler() {
	context_->notifyUpdateEqSettings(true);
}

void
EquipmentSidebar::saveEqSettingsHandler() {
	context_->notifyUpdateEqSettings(false);
}

void
EquipmentSidebar::warnOnEqDeleteHandler() {
	warnOnEqDelete_ = !warnOnEqDelete_;
}

void
EquipmentSidebar::showEquipmentMenu(const QPoint &pos)
{
    if (currentEqItem_ != nullptr) {

		QAction* addEquipment = nullptr;
		QAction* addTimeEquipment = nullptr;
		QAction* deleteEquipment = nullptr;
		QAction* leftShiftEquipment = nullptr;

		switch (currentEqItem_->getEqNodeType()) {

			// Activity Trees
			case eqNodeType::EQ_LINK: {
				deleteEquipment = new QAction(tr("Delete"), equipmentNavigator_);
			} break;

			case eqNodeType::EQ_ITEM_REF: {
				deleteEquipment = new QAction(tr("Remove Reference"), equipmentNavigator_);
			} break;

			case eqNodeType::EQ_TIME_ITEM:
			case eqNodeType::EQ_DIST_ITEM: {
				addEquipment = new QAction(tr("Add Distance Equipment"), equipmentNavigator_);
				addTimeEquipment = new QAction(tr("Add Time Equipment"), equipmentNavigator_);
				deleteEquipment = new QAction(tr("Delete"), equipmentNavigator_);
				if (currentEqItem_->getParentItem()->getEqNodeType() != eqNodeType::EQ_ROOT) {
					leftShiftEquipment = new QAction(tr("Shift Left"), equipmentNavigator_);
				}
			} break;

			// Equipment root should never be selectable (it is hidden)
			default: {
				qDebug() << "Unknown equipment type:" << static_cast<int>(currentEqItem_->getEqNodeType());
			} break;
		}

		QMenu menu(this);

		if (addEquipment) {
			connect(addEquipment, SIGNAL(triggered(void)), this, SLOT(addEquipmentHandler()));
			menu.addAction(addEquipment);
		}
		if (addTimeEquipment) {
			connect(addTimeEquipment, SIGNAL(triggered(void)), this, SLOT(addEquipmentTimeHandler()));
			menu.addAction(addTimeEquipment);
		}
		if (deleteEquipment) {
			connect(deleteEquipment, SIGNAL(triggered(void)), this, SLOT(deleteEquipmentHandler()));
			menu.addAction(deleteEquipment);
		}
		menu.addSeparator();

		QVector<EquipmentNode*>& parentsChildren = currentEqItem_->getParentItem()->getChildren();
		int currentEqItemPosn = parentsChildren.indexOf(currentEqItem_);

		if (currentEqItemPosn != 0) {
			QAction* upEquipment = new QAction(tr("Move Up"), equipmentNavigator_);
			connect(upEquipment, SIGNAL(triggered(void)), this, SLOT(moveUpEquipmentHandler()));
			menu.addAction(upEquipment);
		}
		if (currentEqItemPosn != (parentsChildren.size()-1)) {
			QAction* downEquipment = new QAction(tr("Move Down"), equipmentNavigator_);
			connect(downEquipment, SIGNAL(triggered(void)), this, SLOT(moveDownEquipmentHandler()));
			menu.addAction(downEquipment);
		}

		if (leftShiftEquipment) {
			if (parentsChildren.size() > 1) menu.addSeparator();
			connect(leftShiftEquipment, SIGNAL(triggered(void)), this, SLOT(leftShiftEquipmentHandler()));
			menu.addAction(leftShiftEquipment);
		}
		
		menu.exec(pos);
    }
}

void
EquipmentSidebar::addEquipmentHandler() {
	context_->notifyEquipmentAdded(currentEqItem_, static_cast<int>(eqNodeType::EQ_DIST_ITEM));
}

void
EquipmentSidebar::addEquipmentTimeHandler() {
	context_->notifyEquipmentAdded(currentEqItem_, static_cast<int>(eqNodeType::EQ_TIME_ITEM));
}

void
EquipmentSidebar::deleteEquipmentHandler() {
	context_->notifyEquipmentDeleted(currentEqItem_, warnOnEqDelete_);
}

void
EquipmentSidebar::moveUpEquipmentHandler() {
	context_->notifyEquipmentMove(currentEqItem_, eqListView_, static_cast<int>(eqNodeMoveType::EQ_UP));
}

void
EquipmentSidebar::moveDownEquipmentHandler() {
	context_->notifyEquipmentMove(currentEqItem_, eqListView_, static_cast<int>(eqNodeMoveType::EQ_DOWN));
}

void
EquipmentSidebar::leftShiftEquipmentHandler() {
	context_->notifyEquipmentMove(currentEqItem_, eqListView_, static_cast<int>(eqNodeMoveType::EQ_LEFT_SHIFT));
}
