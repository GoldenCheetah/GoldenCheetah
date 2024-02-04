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

	// Create temporary storage holder for a flat collection of tree items, once createEquipmentItemTree
	// has created the tree the ownership of the items memory will be a tree under the model's eqRootItem. 
	QVector<flatEqNode> flatEqNodes;
	QVector<flatEqNode> flatRefNodes;

	loadEquipmentsFromXML(flatEqNodes, equipModel_->rootItem_,
						  flatRefNodes, refsModel_->rootItem_);

	connect(context_, SIGNAL(updateEqSettings(bool)), this, SLOT(updateEqSettings(bool)));
	connect(context_, SIGNAL(eqRecalculationStart(void)), this, SLOT(eqRecalculationStart()));
	connect(context_, SIGNAL(equipmentAdded(EquipmentNode*, int)), this, SLOT(equipmentAdded(EquipmentNode*, int)));
	connect(context_, SIGNAL(equipmentDeleted(EquipmentNode*, bool)), this, SLOT(equipmentDeleted(EquipmentNode*, bool)));
	connect(context_, SIGNAL(equipmentMove(EquipmentNode*, bool, bool)), this, SLOT(equipmentMove(EquipmentNode*, bool, bool)));

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
	saveEquipmentsToXML(equipModel_->rootItem_, refsModel_->rootItem_);
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
		loadEquipmentsFromXML(flatEqNodes, equipModel_->rootItem_,
							flatRefNodes, refsModel_->rootItem_);

		equipModel_->endResetModel();
		refsModel_->endResetModel();
	}
	else
	{
		// save xml equipment file
		saveEquipmentsToXML(equipModel_->rootItem_, refsModel_->rootItem_);
	}
}

void
EquipmentModelManager::equipmentAdded(EquipmentNode* eqParent, int eqToAdd) {

	switch (eqToAdd) {

	case eqNodeType::EQ_LINK:
	case eqNodeType::EQ_TIME_SPAN: {
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
EquipmentModelManager::equipmentDeleted(EquipmentNode* eqItem, bool warnOnEqDelete)
{
	switch (eqItem->getEqNodeType()) {

	case eqNodeType::EQ_DIST_ITEM: {

		if (eqItem->childCount()) {

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
				refsModel_->deleteEquipmentsRefsMatching(eqItem);

				// Delete the equipment item
				equipModel_->removeAndDeleteEquipment(eqItem);
			}
		}
	} break;

	case eqNodeType::EQ_TIME_ITEM: {
		if (eqItem->childCount()) {

			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Critical);
			msgBox.setText(tr("Time Equipments with children cannot be deleted!"));
			msgBox.setInformativeText(tr("\nto proceed either:\n\n"
				"\t i) delete its children, or \n"
				"\tii) move them to another Equipment Item."));
			msgBox.exec();
		}
		else {
			equipModel_->removeAndDeleteEquipment(eqItem);
		}
	} break;

	case eqNodeType::EQ_LINK:
	case eqNodeType::EQ_TIME_SPAN: {
		if (eqItem->childCount() && warnOnEqDelete) {

			QMessageBox msgBox;
			msgBox.setIcon(QMessageBox::Critical);

			if (eqItem->getEqNodeType() == eqNodeType::EQ_LINK) {
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

				refsModel_->removeAndDeleteEquipment(eqItem);
			}
		}
		else {
			refsModel_->removeAndDeleteEquipment(eqItem);
		}
	} break;

	case eqNodeType::EQ_ITEM_REF: {
		refsModel_->removeAndDeleteEquipment(eqItem);
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
EquipmentModelManager::equipmentMove(EquipmentNode* eqItem, bool eqListView, bool up) {
	
	if (eqListView)
		equipModel_->equipmentMove(eqItem, up);
	else
		refsModel_->equipmentMove(eqItem, up);
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
	/// to provide a general calcluation of distances of equipments used by all athletes.

	// take a copy of the rides through the athlete's rides creating an update status table
	rideItemStatus_ = context_->athlete->rideCache->rides();

	// calculate number of threads and work per thread
	int maxthreads = QThreadPool::globalInstance()->maxThreadCount();
	int threads = maxthreads / 4; // Don't need many threads
	if (threads == 0) threads = 1; // need at least one!

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

	if (rideItemStatus_.isEmpty()) {
		returning = nullptr;
	}
	else {
		returning = rideItemStatus_.takeLast();
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
	// iterate through the top level Activity Equipments to match against ride metadata
	for (EquipmentNode* actItem : refsModel_->rootItem_->getChildren()) {

		if (actItem->getEqNodeType() == eqNodeType::EQ_LINK) {

			if (rideItem->getText("EquipmentLink", "abcde") == static_cast<EquipmentLink*>(actItem)->getEqLinkName()) {

				QDate dRef = QDate(1900, 01, 01).addDays(rideItem->getText("Start Date", "0").toInt());
				QTime tRef = QTime(0, 0, 0).addSecs(rideItem->getText("Start Time", "0").toInt());

				QDateTime ref = QDateTime(dRef, tRef);

				// get the distance metric
				double dist = rideItem->getStringForSymbol("total_distance", GlobalContext::context()->useMetricUnits).toDouble();

				for (EquipmentNode* eqActChild : actItem->getChildren()) {

					// If its a time span encompassing the ride/activity time then
					if (eqActChild->getEqNodeType() == eqNodeType::EQ_TIME_SPAN) {

						if (static_cast<EquipTimeSpan*>(eqActChild)->isWithin(ref)) {

							// now apply the distance to all the Equipment Items via the tree of references
							applyDistanceToRefTreeNodes(eqActChild, dist, false);
						}
					}
					else if (eqActChild->getEqNodeType() == eqNodeType::EQ_ITEM_REF) {

						// now apply the distance to all the Equipment Items via the tree of references
						applyDistanceToRefTreeNodes(eqActChild, dist, true);
					}
				}
			}
		}
	}
}

// ---------------------------- Recalculation of distances ---------------------------------


void // recursive function! 
EquipmentModelManager::ResetTreeNodesBelowEqNode(EquipmentNode* eqNodeTree) {

	for (EquipmentNode* eqItem : eqNodeTree->getChildren())
	{
		if (eqItem->getEqNodeType() == eqNodeType::EQ_ITEM_REF) {
			static_cast<EquipmentRef*>(eqItem)->resetRefDistanceCovered();
		}
		else if (eqItem->getEqNodeType() == eqNodeType::EQ_DIST_ITEM) {
			static_cast<EquipmentDistanceItem*>(eqItem)->resetDistanceCovered();
		}
		ResetTreeNodesBelowEqNode(eqItem);
	}
}

void // recursive function! 
EquipmentModelManager::applyDistanceToRefTreeNodes(EquipmentNode* eqNodeTree, double dist, bool incTopEqRef) {

	if (incTopEqRef) static_cast<EquipmentRef*>(eqNodeTree)->incrementDistanceCovered(dist);

	for (EquipmentNode* eqItem : eqNodeTree->getChildren())
	{
		if (!incTopEqRef) static_cast<EquipmentRef*>(eqItem)->incrementDistanceCovered(dist);

		applyDistanceToRefTreeNodes(eqItem, dist, incTopEqRef);
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
EquipmentModelManager::loadEquipmentsFromXML(QVector<flatEqNode>& flatEqNodes, EquipmentRoot* eqRootItem,
											 QVector<flatEqNode>& flatRefNodes, EquipmentRoot* refRootItem) {

    bool success = false;

    // load equipments from config file
    QString content = "";
    QString filename = QDir(gcroot).canonicalPath() + "/equipments.xml";
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

        success = createEquipmentTree(flatEqNodes, eqRootItem,
									  flatRefNodes, refRootItem);
    }

	if (success) {
		// Record the last calculation and the units used from the xml file
		eqRootItem->setLastRecalc(lastRecalc_);
		refRootItem->setLastRecalc(lastRecalc_);
		eqRootItem->setMetricUnits(xmlMetricUnits);
		refRootItem->setMetricUnits(xmlMetricUnits);
	}
	else {
		// Create an initial equipments which is normally loaded from the equipments.xml file.
		eqRootItem->insertChild(0, new EquipmentDistanceItem("New Distance Equipment",""));
		eqRootItem->insertChild(0, new EquipmentTimeItem("New Time Equipment", ""));

		// Create an initial equipment link which is normally loaded from the equipments.xml file.
		refRootItem->insertChild(0, new EquipmentLink("New Equipment Link", ""));

		// set null datetime for last recalculation
		eqRootItem->setLastRecalc(QDateTime());
		refRootItem->setLastRecalc(QDateTime());
		eqRootItem->setMetricUnits(GlobalContext::context()->useMetricUnits);
		refRootItem->setMetricUnits(GlobalContext::context()->useMetricUnits);
	}
	return success;
}

bool
EquipmentModelManager::createEquipmentTree(QVector<flatEqNode>& flatEqNodes, EquipmentRoot* eqRootItem,
											QVector<flatEqNode>& flatRefNodes, EquipmentRoot* refRootItem)
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
				equipModel_->addChildToParent(flatNode.eqNode_, eqRootItem);

			}
			else {
				// if not a root node, search through all the eqItem's for an Id matching the parentId
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
			equipModel_->addChildToParent(flatNode.eqNode_, refRootItem);
			} break;


		case eqNodeType::EQ_TIME_SPAN: {
			// search through all the eqItem's for an Id matching the parentId
			for (flatEqNode& flatParentNode : flatRefNodes)
			{
				if (flatParentNode.eqId_ == flatNode.eqParentId_) {

					// Set up parent - child relationship
					equipModel_->addChildToParent(flatNode.eqNode_, flatParentNode.eqNode_);
					break;
				}
			}
			} break;

		case eqNodeType::EQ_ITEM_REF: {

			// search through all the eqItem's for an Id matching the parentId
			for (flatEqNode& flatParentNode : flatRefNodes)
			{
				if (flatParentNode.eqId_ == flatNode.eqParentId_) {

					// Set up parent - child relationship
					equipModel_->addChildToParent(flatNode.eqNode_, flatParentNode.eqNode_);
					break;
				}
			}

			// search through all the eqItem's for the eqIdRef 
			for (flatEqNode& flatNodeSearch : flatEqNodes)
			{
				if ((flatNodeSearch.nodeType_ == eqNodeType::EQ_DIST_ITEM) &&
					(flatNodeSearch.eqId_ == flatNode.eqIdRef_)) {

					// Set up pointer reference to equipment item.
					static_cast<EquipmentRef*>(flatNode.eqNode_)->setEqItem(static_cast<EquipmentDistanceItem*>(flatNodeSearch.eqNode_));
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
EquipmentModelManager::saveEquipmentsToXML(EquipmentRoot* eqRootItem, EquipmentRoot* refRootItem) 
{
    QString filename = QDir(gcroot).canonicalPath() + "/equipments.xml";
    QFile file(filename);
    if (file.open(QFile::WriteOnly)) {

        file.resize(0);
        QTextStream out(&file);
        out.setCodec("UTF-8");

        // support different formats using the modelVersion id (held in xmlVersion_), possibly required in the future
        out << "<equipments>\n\n";
        out << "\t<!-- This file contains the equipments, equipment references & time spans. --> \n";
		out << "\t<!-- Note: The ids in this file will change between saves, but the structure and relationships will be retained. --> \n\n";
		out << "\t<eqModel version=\"1\" lastCalc=\"" << lastRecalc_.toString() << "\"";
		out << " units=" << ((eqRootItem->getMetricUnits()) ? "\"Metric\"" : "\"Imperial\"");
		out << " />\n\n";

		out << "\t<!-- The equipment list items: --> \n\n";

		// Write out the equipments
		writeEquipTreeToXML(out, eqRootItem);

		out << "\n\t<!-- The equipment references: --> \n";

		// Write out the equipment references
		writeEquipTreeToXML(out, refRootItem);

        out << "\n</equipments>\n";
        file.close();
    }
    else {

        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Problem Saving equipments.xml configuration file."));
        msgBox.setInformativeText(tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return;
    }
}

void // recursive function! 
EquipmentModelManager::writeEquipTreeToXML(QTextStream& out, EquipmentNode* eqNode) {

	for (EquipmentNode* eqItem : eqNode->getChildren())
	{
		switch (eqItem->getEqNodeType()) {

		case eqNodeType::EQ_DIST_ITEM: {
			out << "\t" << *(static_cast<EquipmentDistanceItem*>(eqItem));
		} break;
		case eqNodeType::EQ_TIME_ITEM: {
			out << "\t" << *(static_cast<EquipmentTimeItem*>(eqItem));
		} break;
		case eqNodeType::EQ_LINK: {
			out << "\n\t" << *(static_cast<EquipmentLink*>(eqItem));
		} break;
		case eqNodeType::EQ_TIME_SPAN: {
			out << "\t\t" << *(static_cast<EquipTimeSpan*>(eqItem));
		} break;
		case eqNodeType::EQ_ITEM_REF: {
			out << "\t\t" << *(static_cast<EquipmentRef*>(eqItem));
		} break;

		// Equipment root is never written to the xml file
		case eqNodeType::EQ_ROOT: {} break;

		default: {
			qDebug() << "Trying to write an unknown equipment type to xml file!";
		}break;
		}

		writeEquipTreeToXML(out, eqItem);
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

    if (qName == "equipments") {}

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

		QString eqId, eqParentId, description, notes, startDate, ReplaceDate;

		// get attributes
		for (int i = 0; i < attrs.count(); i++) {
			if (attrs.qName(i) == "id") eqId = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "parentId") eqParentId = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "desc") description = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "notes") notes = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "startDate") startDate = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "replaceDate") ReplaceDate = Utils::unprotect(attrs.value(i));
		}

		QDateTime Replace = ReplaceDate.isEmpty() ? QDateTime() : QDateTime().fromString(ReplaceDate);

		flatEqNodes_.push_back(flatEqNode(eqId, eqNodeType::EQ_TIME_ITEM, eqParentId, "",
			new EquipmentTimeItem(description, notes, QDateTime().fromString(startDate), Replace)));
	}

    else if (qName == "eqLink") {

        QString actId, eqLinkName, description;

        // get attributes
        for (int i = 0; i < attrs.count(); i++) {
            if (attrs.qName(i) == "id") actId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "eqLinkName") eqLinkName = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "desc") description = Utils::unprotect(attrs.value(i));
        }

		flatRefNodes_.push_back(flatEqNode(actId, eqNodeType::EQ_LINK, "", "",
						new EquipmentLink(eqLinkName, description)));
    }

    else if (qName == "eqTSp") {

        QString useId, useParentId, start, end, notes;

        // get attributes
        for (int i = 0; i < attrs.count(); i++) {
			if (attrs.qName(i) == "id") useId = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "parentId") useParentId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "start") start = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "end") end = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "notes") notes = Utils::unprotect(attrs.value(i));
        }

		QDateTime endTS = end.isEmpty() ? QDateTime() : QDateTime().fromString(end);

		flatRefNodes_.push_back(flatEqNode(useId, eqNodeType::EQ_TIME_SPAN, useParentId, "",
						new EquipTimeSpan(QDateTime().fromString(start), notes, endTS)));
    }

	else if (qName == "eqRef") {

		QString useId, useParentId, eqIdRef, refDistance;

		// get attributes
		for (int i = 0; i < attrs.count(); i++) {
			if (attrs.qName(i) == "id") useId = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "parentId") useParentId = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "eqId") eqIdRef = Utils::unprotect(attrs.value(i));
			if (attrs.qName(i) == "refDist") refDistance = Utils::unprotect(attrs.value(i));
		}

		double optrefDistance = (refDistance.isEmpty()) ? 0 : QVariant(refDistance).toDouble();

		flatRefNodes_.push_back(flatEqNode(useId, eqNodeType::EQ_ITEM_REF, useParentId, eqIdRef,
								new EquipmentRef(optrefDistance)));
	}

    return true;
}

bool EquipmentParser::characters(const QString& /* str */ ) {
    return true;
}

bool EquipmentParser::endDocument() {
	return true;
}


