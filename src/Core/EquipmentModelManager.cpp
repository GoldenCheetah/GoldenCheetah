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

#include "GoldenCheetah.h"
#include "Context.h"
#include "AbstractView.h"
#include "Athlete.h"

#include "MainWindow.h"
#include "Utils.h"
#include "EquipmentModelManager.h"


// ---------------------------- EquipmentModelManager -----------------------------------

EquipmentModelManager::EquipmentModelManager(Context* context) :
	context_(context), xmlVersion_(0)
{
	equipModel_ = new EquipmentModel(context);
	refsModel_ = new EquipmentModel(context);

	// Create temporary storage holder for a flat collection of tree nodes, once createEquipmentNodeTree
	// has created the tree the ownership of the item's memory will be a tree under the model's eqRootNode. 
	QVector<flatEqNode> flatEqNodes;
	QVector<flatEqNode> flatRefNodes;

	loadEquipmentFromXML(flatEqNodes, equipModel_->rootItem_,
						  flatRefNodes, refsModel_->rootItem_);

	connect(context_, SIGNAL(updateEqSettings(bool)), this, SLOT(updateEqSettings(bool)));
	connect(context_, SIGNAL(eqRecalculationStart(void)), this, SLOT(eqRecalculationStart()));
	connect(context_, SIGNAL(equipmentAdded(EquipmentNode*, int)), this, SLOT(equipmentAdded(EquipmentNode*, int)));
	connect(context_, SIGNAL(equipmentDeleted(EquipmentNode*, bool)), this, SLOT(equipmentDeleted(EquipmentNode*, bool)));
	connect(context_, SIGNAL(equipmentMove(EquipmentNode*, bool, int)), this, SLOT(equipmentMove(EquipmentNode*, bool, int)));

	// for debug
	// printfEquipment();
}

EquipmentModelManager::~EquipmentModelManager()
{
	// delete the equipment models
	delete equipModel_;
	delete refsModel_;
}

void
EquipmentModelManager::close()
{
	saveEquipmentToXML(equipModel_->rootItem_, refsModel_->rootItem_);
}

void
EquipmentModelManager::updateEqSettings(bool load)
{
	// load or save the xml equipment file
	if (load) {

		equipModel_->beginResetModel();
		refsModel_->beginResetModel();

		// deletes all of roots children, each child destructor deletes its own children
		QVector<EquipmentNode*>& childrenToDelete = equipModel_->rootItem_->getChildren();
		while (childrenToDelete.size()) {
			delete childrenToDelete.takeLast();
		}

		QVector<EquipmentNode*>& refChildrenToDelete = refsModel_->rootItem_->getChildren();
		while (refChildrenToDelete.size()) {
			delete refChildrenToDelete.takeLast();
		}

		QVector<flatEqNode> flatEqNodes;
		QVector<flatEqNode> flatRefNodes;

		// reload the xml equipment file
		loadEquipmentFromXML(flatEqNodes, equipModel_->rootItem_,
							flatRefNodes, refsModel_->rootItem_);

		equipModel_->endResetModel();
		refsModel_->endResetModel();
	}
	else
	{
		// save xml equipment file
		saveEquipmentToXML(equipModel_->rootItem_, refsModel_->rootItem_);
	}
}

void
EquipmentModelManager::equipmentAdded(EquipmentNode* eqParent, int eqToAdd) {

	switch (eqToAdd) {

	case eqNodeType::EQ_LINK: {
		refsModel_->equipmentAdded(eqParent, eqToAdd);
	} break;

	case eqNodeType::EQ_DIST_ITEM:
	case eqNodeType::EQ_TIME_ITEM: {
		if (eqParent == nullptr)
			equipModel_->equipmentAdded(equipModel_->rootItem_, eqToAdd);
		else
			equipModel_->equipmentAdded(eqParent, eqToAdd);
	} break;

	// Equipment root should never be added.
	case eqNodeType::EQ_ROOT: {
		qDebug() << "Trying to add an equipment root node!";
	} break;
	}
}

void
EquipmentModelManager::equipmentDeleted(EquipmentNode* eqNode, bool warnOnEqDelete)
{
	switch (eqNode->getEqNodeType()) {

	case eqNodeType::EQ_DIST_ITEM: {

		if (eqNode->childCount()) {

			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.setText(tr("Equipment Items with children cannot be deleted!"));
			msgBox.setInformativeText(tr("\nto proceed either:\n\n"
				"\t i) delete its children, or \n"
				"\tii) move them to another Equipment Item."));
			msgBox.exec();
		}
		else {

			bool deleteEq = true;

			if (warnOnEqDelete) {

				QMessageBox msgBox;
				msgBox.setIcon(QMessageBox::Warning);
				msgBox.setText(tr("Deleting an Equipment Item."));
				msgBox.setInformativeText(tr("\nProceeding will delete the selected Equipment Item,\n"
					"and ALL of its reference children.\n"));
				QPushButton* deleteButton = msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);
				msgBox.addButton(tr("Cancel"), QMessageBox::ActionRole);
				msgBox.setDefaultButton(QMessageBox::Cancel);
				msgBox.exec();

				deleteEq = (msgBox.clickedButton() == deleteButton);
			}

			if (deleteEq) {

				// find and delete any references to the equipment.
				refsModel_->deleteEquipmentRefsMatching(eqNode);

				// Delete the equipment item
				equipModel_->removeAndDeleteEquipment(eqNode);
			}
		}
	} break;

	case eqNodeType::EQ_TIME_ITEM: {
		if (eqNode->childCount()) {

			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.setText(tr("Time Equipment with children cannot be deleted!"));
			msgBox.setInformativeText(tr("\nto proceed either:\n\n"
				"\t i) delete its children, or \n"
				"\tii) move them to another Equipment Item."));
			msgBox.exec();
		}
		else {
			equipModel_->removeAndDeleteEquipment(eqNode);
		}
	} break;

	case eqNodeType::EQ_LINK: {
		if (eqNode->childCount() && warnOnEqDelete) {

			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Critical);

			if (eqNode->getEqNodeType() == eqNodeType::EQ_LINK) {
				msgBox.setText(tr("Deleting an Equipment Link with children."));
				msgBox.setInformativeText(tr("\nProceeding will delete the selected Equipment Link,\n"
					"and ALL of its reference children.\n"));
			} else {
				msgBox.setText(tr("Deleting a Time Span with children."));
				msgBox.setInformativeText(tr("\nProceeding will delete the selected Time Span,\n"
					"and ALL of its reference children.\n"));
			}
			QPushButton* deleteButton = msgBox.addButton(tr("Delete"), QMessageBox::ActionRole);
			msgBox.addButton(tr("Cancel"), QMessageBox::ActionRole);
			msgBox.setDefaultButton(QMessageBox::Cancel);
			msgBox.exec();

			if (msgBox.clickedButton() == deleteButton) {

				refsModel_->removeAndDeleteEquipment(eqNode);
			}
		}
		else {
			refsModel_->removeAndDeleteEquipment(eqNode);
		}
	} break;

	case eqNodeType::EQ_ITEM_REF: {
		// find the linked equipment and remove myself from their list
		static_cast<EquipmentRef*>(eqNode)->eqDistNode_->linkedRefs_.removeOne(static_cast<EquipmentRef*>(eqNode));
		refsModel_->removeAndDeleteEquipment(eqNode);
	} break;

	// Equipment root should never be deleted!
	case eqNodeType::EQ_ROOT: {
		qDebug() << "Trying to delete the equipment root node!";
	} break;

	default: {
		qDebug() << "Trying to delete an unknown equipment type!";
	}break;
	}
}

void
EquipmentModelManager::equipmentMove(EquipmentNode* eqNode, bool eqListView, int move) {
	
	if (eqListView)
		equipModel_->equipmentMove(eqNode, move);
	else
		refsModel_->equipmentMove(eqNode, move);
}

void
EquipmentModelManager::eqRecalculationStart() {

	// already recalcuating !
	if (recalculationThreads_.count()) return;

	// reset the calculated distance first
	ResetTreeNodesBelowEqNode(equipModel_->rootItem_);
	ResetTreeNodesBelowEqNode(refsModel_->rootItem_);

	// update the last recalcuation time & units used
	lastRecalc_ = QDateTime::currentDateTime();
	equipModel_->rootItem_->setMetricUnits(GlobalContext::context()->useMetricUnits);
	refsModel_->rootItem_->setMetricUnits(GlobalContext::context()->useMetricUnits);

	// Currently this code restricts the calculation to a single athlete, so needs looking at
	/// to provide a general calcluation of distances of equipment used by all athletes.

	// take a copy of the rides through the athlete's rides creating a ride List to process
	rideItemList_ = context_->athlete->rideCache->rides();

	// calculate number of threads and work per thread
	int maxthreads = QThreadPool::globalInstance()->maxThreadCount();
	int threads = maxthreads / 4; // Don't need many threads
	if (threads == 0) threads = 1; // but need at least one!

	// keep launching the threads
	while (threads--) {

		// if goes past last make it the last
		EquipmentModelRecalculationThread* thread = new EquipmentModelRecalculationThread(this);
		recalculationThreads_ << thread;

		thread->start();
	}
}

RideItem*
EquipmentModelManager::nextRideToCheck()
{
	RideItem* returning;
	updateMutex_.lock();

	if (rideItemList_.isEmpty()) {
		returning = nullptr;
	}
	else {
		returning = rideItemList_.takeLast();
	}
	updateMutex_.unlock();
	return(returning);
}

void
EquipmentModelRecalculationThread::run() {

	RideItem* item;
	while (item = model_->nextRideToCheck()) {
		model_->RecalculateEq(item);
	}
	model_->threadCompleted(this);
	return;
}

void
EquipmentModelManager::threadCompleted(EquipmentModelRecalculationThread* thread)
{
	updateMutex_.lock();
	recalculationThreads_.removeOne(thread);
	updateMutex_.unlock();

	if (recalculationThreads_.count() == 0) {

		equipModel_->rootItem_->setLastRecalc(lastRecalc_);
		refsModel_->rootItem_->setLastRecalc(lastRecalc_);

		// Notify that recalculation is complete
		context_->notifyEqRecalculationEnd();
	}
}

void
EquipmentModelManager::RecalculateEq(RideItem* rideItem)
{
	// iterate through the top level Equipment LInks to match against ride metadata
	for (EquipmentNode* rootChild : refsModel_->rootItem_->getChildren()) {

		if (rootChild->getEqNodeType() == eqNodeType::EQ_LINK) {

			if (rideItem->getText("EquipmentLink", "abcde") == static_cast<EquipmentLink*>(rootChild)->getEqLinkName()) {

				QDate dRef = QDate(1900, 01, 01).addDays(rideItem->getText("Start Date", "0").toInt());
				QTime tRef = QTime(0, 0, 0).addSecs(rideItem->getText("Start Time", "0").toInt());

				QDateTime ref = QDateTime(dRef, tRef);

				// get the distance metric
				double dist = rideItem->getStringForSymbol("total_distance", GlobalContext::context()->useMetricUnits).toDouble();

				for (EquipmentNode* eqNode : rootChild->getChildren()) {

					if (eqNode->getEqNodeType() == eqNodeType::EQ_ITEM_REF) {

						// If its within any defined time span encompassing the ride/activity time then
						if (static_cast<EquipmentRef*>(eqNode)->isWithin(ref)) {

							// now apply the distance to all the Equipment Items via the tree of references
							applyDistanceToRefTreeNodes(eqNode, dist);
						}
					}
				}
			}
		}
	}
}

// ---------------------------- Recalculation of distances ---------------------------------

void // recursive function! 
EquipmentModelManager::ResetTreeNodesBelowEqNode(EquipmentNode* eqNodeTree) {

	for (EquipmentNode* eqNode : eqNodeTree->getChildren())
	{
		if (eqNode->getEqNodeType() == eqNodeType::EQ_ITEM_REF) {
			static_cast<EquipmentRef*>(eqNode)->resetRefDistanceCovered();
		}
		else if (eqNode->getEqNodeType() == eqNodeType::EQ_DIST_ITEM) {
			static_cast<EquipmentDistanceItem*>(eqNode)->resetDistanceCovered();
		}
		ResetTreeNodesBelowEqNode(eqNode);
	}
}

void // recursive function! 
EquipmentModelManager::applyDistanceToRefTreeNodes(EquipmentNode* eqNodeTree, const double dist) {

	static_cast<EquipmentRef*>(eqNodeTree)->incrementDistanceCovered(dist);

	for (EquipmentNode* eqNode : eqNodeTree->getChildren())
	{
		applyDistanceToRefTreeNodes(eqNode, dist);
	}
}

void
EquipmentModelManager::printfEquipment() {

	printf("Last recalculation: %s\n", lastRecalc_.toString().toStdString().c_str());

	equipModel_->printfTree(0, equipModel_->rootItem_);
	refsModel_->printfTree(0, refsModel_->rootItem_);
}

// ---------------------------- XML Inport/Export functions ---------------------------------

bool
EquipmentModelManager::loadEquipmentFromXML(QVector<flatEqNode>& flatEqNodes, EquipmentRoot* eqRootNode,
											 QVector<flatEqNode>& flatRefNodes, EquipmentRoot* refRootNode) {

    bool success = false;

    // load equipment from config file
    QString content = "";
	QString filename = context_->athlete->home->config().canonicalPath() + "/equipment.xml";
    QFile file(filename);

    if (file.open(QIODevice::ReadOnly)) {
        content = file.readAll();
        file.close();
		success = !content.isEmpty();
	}

	bool xmlMetricUnits;

    if (success) {

        // setup the handler
        QXmlInputSource source;
        source.setData(content);
        QXmlSimpleReader xmlReader;
		
        EquipmentParser handler(flatEqNodes, flatRefNodes,
								lastRecalc_, xmlMetricUnits, xmlVersion_);
        xmlReader.setContentHandler(&handler);
        xmlReader.setErrorHandler(&handler);

        // parse and instantiate the charts
        xmlReader.parse(source);

        success = createEquipmentNodeTree(flatEqNodes, eqRootNode,
									  flatRefNodes, refRootNode);
    }

	if (success) {
		// Record the last calculation and the units used from the xml file
		eqRootNode->setLastRecalc(lastRecalc_);
		refRootNode->setLastRecalc(lastRecalc_);
		eqRootNode->setMetricUnits(xmlMetricUnits);
		refRootNode->setMetricUnits(xmlMetricUnits);
	}
	else {
		// Create an initial equipment which is normally loaded from the equipment.xml file.
		eqRootNode->insertChild(0, new EquipmentDistanceItem(tr("New Distance Equipment"),""));
		eqRootNode->insertChild(0, new EquipmentTimeItem(tr("New Time Equipment"), ""));

		// Create an initial equipment link which is normally loaded from the equipment.xml file.
		refRootNode->insertChild(0, new EquipmentLink(tr("New Equipment Link"), ""));

		// set null datetime for last recalculation
		eqRootNode->setLastRecalc(QDateTime());
		refRootNode->setLastRecalc(QDateTime());
		eqRootNode->setMetricUnits(GlobalContext::context()->useMetricUnits);
		refRootNode->setMetricUnits(GlobalContext::context()->useMetricUnits);
	}
	return success;
}

bool
EquipmentModelManager::createEquipmentNodeTree(QVector<flatEqNode>& flatEqNodes, EquipmentRoot* eqRootNode,
											QVector<flatEqNode>& flatRefNodes, EquipmentRoot* refRootNode)
{
    bool success = true;

    // search through all the equipment items
	for (flatEqNode& flatNode : flatEqNodes)
	{
		switch (flatNode.nodeType_) {

		case eqNodeType::EQ_TIME_ITEM:
		case eqNodeType::EQ_DIST_ITEM: {

			if (flatNode.eqParentId_.contains("eqRootNode")) {
				// Set up toplevel parent - child relationship
				equipModel_->addChildToParent(flatNode.eqNode_, eqRootNode);

			}
			else {
				// if not a root node, search through all the eqNode's for an Id matching the parentId
				for (flatEqNode& flatParentNode : flatEqNodes)
				{
					if (flatParentNode.eqId_ == flatNode.eqParentId_) {

						// Set up parent - child relationship
						equipModel_->addChildToParent(flatNode.eqNode_, flatParentNode.eqNode_);
						break;
					}
				}
			}
			} break;

		default: {
			success = false;
			} break;
		}
		success &= (flatNode.eqNode_->getParentItem() != nullptr);
	}

	// search through all the equipment references
	for (flatEqNode& flatNode : flatRefNodes)
	{
		switch (flatNode.nodeType_) {

		case eqNodeType::EQ_LINK: {

			// Set up toplevel parent - child relationship
			equipModel_->addChildToParent(flatNode.eqNode_, refRootNode);
			} break;

		case eqNodeType::EQ_ITEM_REF: {

			// search through all the eqNode's for an Id matching the parentId
			for (flatEqNode& flatParentNode : flatRefNodes)
			{
				if (flatParentNode.eqId_ == flatNode.eqParentId_) {

					// Set up parent - child relationship
					equipModel_->addChildToParent(flatNode.eqNode_, flatParentNode.eqNode_);
					break;
				}
			}

			// search through all the eqNode's for the eqIdRef 
			for (flatEqNode& flatNodeSearch : flatEqNodes)
			{
				if ((flatNodeSearch.nodeType_ == eqNodeType::EQ_DIST_ITEM) &&
					(flatNodeSearch.eqId_ == flatNode.eqIdRef_)) {

					// Add the equipment to the reference
					static_cast<EquipmentRef*>(flatNode.eqNode_)->eqDistNode_ = static_cast<EquipmentDistanceItem*>(flatNodeSearch.eqNode_);

					// Add the reference to the equipment
					static_cast<EquipmentDistanceItem*>(flatNodeSearch.eqNode_)->linkedRefs_.push_back(static_cast<EquipmentRef*>(flatNode.eqNode_));
					break;
				}
			}
			} break;

		default: {
			success = false;
			} break;
		}
		success &= (flatNode.eqNode_->getParentItem() != nullptr);
	}

    return success;
}

void
EquipmentModelManager::saveEquipmentToXML(EquipmentRoot* eqRootNode, EquipmentRoot* refRootNode) 
{
	QString filename = context_->athlete->home->config().canonicalPath() + "/equipment.xml";
    QFile file(filename);
    if (file.open(QFile::WriteOnly)) {

        file.resize(0);
        QTextStream out(&file);
        out.setCodec("UTF-8");

        // support different formats using the modelVersion id (held in xmlVersion_), possibly required in the future
        out << "<equipment>\n\n";
        out << "\t<!-- This file contains the equipment, equipment references & time spans. --> \n";
		out << "\t<!-- Note: The ids in this file will change between saves, but the structure and relationships will be retained. --> \n\n";
		out << "\t<eqModel version=\"1\" lastCalc=\"" << lastRecalc_.toString() << "\"";
		out << " units=" << ((eqRootNode->getMetricUnits()) ? "\"Metric\"" : "\"Imperial\"");
		out << " />\n\n";

		out << "\t<!-- The equipment list items: --> \n\n";

		// Write out the equipment
		writeEquipTreeToXML(out, eqRootNode);

		out << "\n\t<!-- The equipment references: --> \n";

		// Write out the equipment references
		writeEquipTreeToXML(out, refRootNode);

        out << "\n</equipment>\n";
        file.close();
    }
    else {

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Problem Saving equipment.xml configuration file."));
        msgBox.setInformativeText(tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return;
    }
}

void // recursive function! 
EquipmentModelManager::writeEquipTreeToXML(QTextStream& out, EquipmentNode* eqTreeNode) {

	for (EquipmentNode* eqNode : eqTreeNode->getChildren())
	{
		switch (eqNode->getEqNodeType()) {

		case eqNodeType::EQ_DIST_ITEM: {
			out << "\t" << *(static_cast<EquipmentDistanceItem*>(eqNode));
		} break;
		case eqNodeType::EQ_TIME_ITEM: {
			out << "\t" << *(static_cast<EquipmentTimeItem*>(eqNode));
		} break;
		case eqNodeType::EQ_LINK: {
			out << "\n\t" << *(static_cast<EquipmentLink*>(eqNode));
		} break;
		case eqNodeType::EQ_ITEM_REF: {
			out << "\t\t" << *(static_cast<EquipmentRef*>(eqNode));
		} break;

		// Equipment root is never written to the xml file
		case eqNodeType::EQ_ROOT: {} break;

		default: {
			qDebug() << "Trying to write an unknown equipment type to xml file!";
		}break;
		}

		writeEquipTreeToXML(out, eqNode);
	}
}

// ---------------------------- EquipmentParser - reads in gcroot/equipment.xml ---------------------------------

bool EquipmentParser::startDocument() {
    return true;
}

bool EquipmentParser::endElement(const QString&, const QString&, const QString& ) {
    return true;
}

bool EquipmentParser::startElement(const QString&, const QString&, const QString& qName, const QXmlAttributes& attrs) {

    if (qName == "equipment") {}

    else if (qName == "eqModel") {

        QString version, lastCalc, metricUnits;

        // get attributes
        for (int i = 0; i < attrs.count(); i++) {
            if (attrs.qName(i) == "version") version = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "lastCalc") lastCalc = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "units") metricUnits = Utils::unprotect(attrs.value(i));
        }
		lastRecalcRef_ = QDateTime().fromString(lastCalc);
		metricUnitsRef_ = metricUnits.contains("Metric");

		// use version in the future to support parsing of different formats
		versionRef_ = QVariant(version).toUInt();
    }
  
    else if (qName == "eqDist") {

        QString eqId, eqParentId, description, notes, nonGCDistance, gcDistance, replacementDistance;

        // get attributes
        for (int i = 0; i < attrs.count(); i++) {
            if (attrs.qName(i) == "id") eqId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "parentId") eqParentId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "desc") description = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "notes") notes = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "nonGCDist") nonGCDistance = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "gcDist") gcDistance = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "replaceDist") replacementDistance = Utils::unprotect(attrs.value(i));
        }

		double optNonGCDistance = (nonGCDistance.isEmpty()) ? 0 : QVariant(nonGCDistance).toDouble();
		double optGCDistance = (gcDistance.isEmpty()) ? 0 : QVariant(gcDistance).toDouble();
		double optreplacementDistance = (replacementDistance.isEmpty()) ? 0 : QVariant(replacementDistance).toDouble();

		flatEqNodes_.push_back(flatEqNode(eqId, eqNodeType::EQ_DIST_ITEM, eqParentId, "",
					new EquipmentDistanceItem(description, notes, optNonGCDistance, optGCDistance, optreplacementDistance)));
    }

	else if (qName == "eqTime") {

		QString eqId, eqParentId, description, notes, start, replace;

		// get attributes
		for (int i = 0; i < attrs.count(); i++) {
			if (attrs.qName(i) == "id") eqId = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "parentId") eqParentId = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "desc") description = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "notes") notes = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "start") start = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "replace") replace = Utils::unprotect(attrs.value(i));
		}

		QDateTime Replace = replace.isEmpty() ? QDateTime() : QDateTime().fromString(replace);

		flatEqNodes_.push_back(flatEqNode(eqId, eqNodeType::EQ_TIME_ITEM, eqParentId, "",
			new EquipmentTimeItem(description, notes, QDateTime().fromString(start), Replace)));
	}

    else if (qName == "eqLink") {

        QString linkId, eqLinkName, description;

        // get attributes
        for (int i = 0; i < attrs.count(); i++) {
            if (attrs.qName(i) == "id") linkId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "eqLinkName") eqLinkName = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "desc") description = Utils::unprotect(attrs.value(i));
        }

		flatRefNodes_.push_back(flatEqNode(linkId, eqNodeType::EQ_LINK, "", "",
						new EquipmentLink(eqLinkName, description)));
    }

	else if (qName == "eqRef") {

		QString useId, useParentId, eqIdRef, refDistance, start, end, notes;

		// get attributes
		for (int i = 0; i < attrs.count(); i++) {
			if (attrs.qName(i) == "id") useId = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "parentId") useParentId = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "eqId") eqIdRef = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "refDist") refDistance = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "start") start = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "end") end = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "notes") notes = Utils::unprotect(attrs.value(i));
		}

		double optrefDistance = (refDistance.isEmpty()) ? 0 : QVariant(refDistance).toDouble();

		QDateTime startTS = start.isEmpty() ? QDateTime() : QDateTime().fromString(start);
		QDateTime endTS = end.isEmpty() ? QDateTime() : QDateTime().fromString(end);

		flatRefNodes_.push_back(flatEqNode(useId, eqNodeType::EQ_ITEM_REF, useParentId, eqIdRef,
								new EquipmentRef(optrefDistance, startTS, endTS, notes)));
	}

    return true;
}

bool EquipmentParser::characters(const QString& /* str */ ) {
    return true;
}

bool EquipmentParser::endDocument() {
	return true;
}


