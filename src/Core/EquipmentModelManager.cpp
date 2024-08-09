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

    // Create temporary storage holder for a flat collection of tree nodes, once createEquipmentNodeTree
    // has created the tree the ownership of the item's memory will be a tree under the model's eqRootNode. 
    QVector<flatEqNode> flatEqNodes;

    loadEquipmentFromXML(flatEqNodes);

    connect(context_, SIGNAL(updateEqSettings(bool)), this, SLOT(updateEqSettings(bool)));
    connect(context_, SIGNAL(eqRecalculationStart(void)), this, SLOT(eqRecalculationStart()));

    // for debug
    // printfEquipment();
}

EquipmentModelManager::~EquipmentModelManager()
{
    // The equipModel_ memory created in the constructor is deleted by Qt when the Equipment Navigator's
    // QTreeView is deleted, QTreeView is derived from QAbstractItemModel and this is mentioned
    // in https://doc.qt.io/qt-5/qabstractitemview.html#setModel, and is the case when checking the GC
    // destruction stack :"The view does not take ownership of the model unless it is the model's parent
    // object because the model may be shared between many different views."
}

void
EquipmentModelManager::close()
{
    saveEquipmentToXML();
}

void
EquipmentModelManager::updateEqSettings(bool load)
{
    // load or save the xml equipment file
    if (load) {

        equipModel_->beginResetModel();

        // remove all equipment from the model
        equipModel_->resetModel();

        QVector<flatEqNode> flatEqNodes;

        // reload the xml equipment file
        loadEquipmentFromXML(flatEqNodes);

        equipModel_->endResetModel();
    }
    else
    {
        // save xml equipment file
        saveEquipmentToXML();
    }
}

void
EquipmentModelManager::eqRecalculationStart() {

    // already recalcuating !
    if (recalculationThreads_.count()) return;

    // reset the calculated distance first
    ResetTreeNodesBelowEqNode(equipModel_->rootItem_);

    // update the last recalcuation time & units used
    equipModel_->rootItem_->setMetricUnits(GlobalContext::context()->useMetricUnits);

    // Currently this code restricts the calculation to a single athlete, so possibly needs looking
    // at to provide a general calcluation of distances of equipment used by all athletes.

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

        equipModel_->rootItem_->setLastRecalc(QDateTime::currentDateTime());

        // Notify that recalculation is complete
        context_->notifyEqRecalculationEnd();
    }
}

void
EquipmentModelManager::RecalculateEq(RideItem* rideItem)
{
    // iterate through the root item's links to find the equipment links to match against ride metadata
    for (EquipmentNode* rootChild : equipModel_->rootItem_->getChildren()) {

        if (rootChild->getEqNodeType() == eqNodeType::EQ_LINK) {

            if (rideItem->getText("EquipmentLink", "abcde") == static_cast<EquipmentLink*>(rootChild)->getEqLinkName()) {

                QDate dRef = QDate(1900, 01, 01).addDays(rideItem->getText("Start Date", "0").toInt());
                QTime tRef = QTime(0, 0, 0).addSecs(rideItem->getText("Start Time", "0").toInt());

                QDateTime actDateTime = QDateTime(dRef, tRef);

                // get the distance metric
                double dist = rideItem->getStringForSymbol("total_distance", GlobalContext::context()->useMetricUnits).toDouble();

                static_cast<EquipmentLink*>(rootChild)->incrementActivities();
                static_cast<EquipmentLink*>(rootChild)->incrementDistanceCovered(dist);

                // now apply the distance to all the relevant equipment items
                applyDistanceToTreeNodes(rootChild, actDateTime, dist);
            }
        }
    }
}

// ---------------------------- Recalculation of distances ---------------------------------

void // recursive function! 
EquipmentModelManager::ResetTreeNodesBelowEqNode(EquipmentNode* eqNode) {

    for (EquipmentNode* eqChildNode : eqNode->getChildren())
    {
        if (eqChildNode->getEqNodeType() == eqNodeType::EQ_LINK) {
            static_cast<EquipmentLink*>(eqChildNode)->resetActivities();
            static_cast<EquipmentLink*>(eqChildNode)->resetDistanceCovered();
        }
        else if (eqChildNode->getEqNodeType() == eqNodeType::EQ_ITEM_REF) {
            static_cast<EquipmentRef*>(eqChildNode)->resetRefDistanceCovered();
        }
        else if (eqChildNode->getEqNodeType() == eqNodeType::EQ_DIST_ITEM) {
            static_cast<EquipmentDistanceItem*>(eqChildNode)->resetDistanceCovered();
        }
        else if (eqChildNode->getEqNodeType() == eqNodeType::EQ_SHARED_DIST_ITEM) {
            static_cast<EquipmentSharedDistanceItem*>(eqChildNode)->resetDistanceCovered();
        }
        ResetTreeNodesBelowEqNode(eqChildNode);
    }
}

void // recursive function! 
EquipmentModelManager::applyDistanceToTreeNodes(EquipmentNode* eqNode, const QDateTime& actDateTime, const double dist) {

    // the children are governed by any time restriction on their parent.
    for (EquipmentNode* eqChildNode : eqNode->getChildren())
    {
        if ((eqChildNode->getEqNodeType() == eqNodeType::EQ_DIST_ITEM) &&
            (static_cast<EquipmentDistanceItem*>(eqChildNode)->isWithin(actDateTime))) {

            static_cast<EquipmentDistanceItem*>(eqChildNode)->incrementDistanceCovered(dist);
        }
        else if ((eqChildNode->getEqNodeType() == eqNodeType::EQ_ITEM_REF) &&
                (static_cast<EquipmentRef*>(eqChildNode)->isWithin(actDateTime))) {

            // Updates the reference and the associated Shared Equipment Distance Item
            static_cast<EquipmentRef*>(eqChildNode)->incrementDistanceCovered(dist);
        }
        applyDistanceToTreeNodes(eqChildNode, actDateTime, dist);
    }
}

void
EquipmentModelManager::printfEquipment() {

    printf("Last recalculation: %s\n", equipModel_->rootItem_->getLastRecalc().toString().toStdString().c_str());
    equipModel_->printfTree();
}

// ---------------------------- XML Inport/Export functions ---------------------------------

void
EquipmentModelManager::loadEquipmentFromXML(QVector<flatEqNode>& flatEqNodes)
{
    bool success = false;

    // load equipment from config file
    QString content = "";
    QString filename = context_->athlete->home->config().canonicalPath() + "/equipment.xml";
    QFile file(filename);

    if (file.open(QIODevice::ReadOnly)) {
        content = file.readAll();
        file.close();
    }

    // if the opened file has content then parse it.
    if (!content.isEmpty()) {

        // setup the handler
        QXmlInputSource source;
        source.setData(content);
        QXmlSimpleReader xmlReader;
        bool xmlMetricUnits;
        QDateTime lastRecalc;

        EquipmentParser handler(flatEqNodes, lastRecalc, xmlMetricUnits, xmlVersion_);
        xmlReader.setContentHandler(&handler);
        xmlReader.setErrorHandler(&handler);

        // parse the equipment file
        if (xmlReader.parse(source)) {

            // Create the equipment node tree from the flat nodes
            if (createEquipmentNodeTree(flatEqNodes)) {

                // Record the last calculation and the units used from the xml file
                equipModel_->rootItem_->setLastRecalc(lastRecalc);
                equipModel_->rootItem_->setMetricUnits(xmlMetricUnits);

                success = true;
            }
        }
    }

    if (!success) {
            
        // remove all equipment from the model
        equipModel_->resetModel();

        // Create an initial equipment link which is normally loaded from the equipment.xml file.
        equipModel_->rootItem_->insertChild(0, new EquipmentLink(tr("New Eq Link")));

        // Create the shared equipment banner which is normally loaded from the equipment.xml file.
        equipModel_->rootItem_->insertChild(1, new SharedEquipment());

        // set null datetime for last recalculation
        equipModel_->rootItem_->setLastRecalc(QDateTime());
        equipModel_->rootItem_->setMetricUnits(GlobalContext::context()->useMetricUnits);
    }
}

bool
EquipmentModelManager::createEquipmentNodeTree(QVector<flatEqNode>& flatEqNodes)
{
    bool success = true;

    // search through all the equipment references
    for (flatEqNode& flatNode : flatEqNodes)
    {
        switch (flatNode.nodeType_) {

        case eqNodeType::EQ_SHARED:
        case eqNodeType::EQ_LINK: {

            equipModel_->addChildToParent(flatNode.eqNode_, equipModel_->rootItem_);
        } break;

        case eqNodeType::EQ_BANNER:
        case eqNodeType::EQ_TIME_ITEM:
        case eqNodeType::EQ_DIST_ITEM:
        case eqNodeType::EQ_SHARED_DIST_ITEM: {

            // search through all the eqNode's for an Id matching the parentId
            for (flatEqNode& flatParentNode : flatEqNodes)
            {
                if (flatParentNode.eqId_ == flatNode.eqParentId_) {

                    // Set up parent - child relationship
                    equipModel_->addChildToParent(flatNode.eqNode_, flatParentNode.eqNode_);
                    break;
                }
            }
        } break;

        case eqNodeType::EQ_ITEM_REF: {

            // search through all the eqNode's for an Id matching the parentId
            for (flatEqNode& flatParentNode : flatEqNodes)
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
                if ((flatNodeSearch.nodeType_ == eqNodeType::EQ_SHARED_DIST_ITEM) &&
                    (flatNodeSearch.eqId_ == flatNode.eqIdRef_)) {

                    // Add the equipment to the reference
                    static_cast<EquipmentRef*>(flatNode.eqNode_)->eqSharedDistNode_ = static_cast<EquipmentSharedDistanceItem*>(flatNodeSearch.eqNode_);

                    // Add the reference to the equipment
                    static_cast<EquipmentSharedDistanceItem*>(flatNodeSearch.eqNode_)->linkedRefs_.push_back(static_cast<EquipmentRef*>(flatNode.eqNode_));
                    break;
                }
            }
        } break;

        }
        success &= (flatNode.eqNode_->getParentItem() != nullptr);
    }

    return success;
}

void
EquipmentModelManager::saveEquipmentToXML() 
{
    QString filename = context_->athlete->home->config().canonicalPath() + "/equipment.xml";
    QFile file(filename);
    if (file.open(QFile::WriteOnly)) {

        file.resize(0);
        QTextStream out(&file);
        out.setCodec("UTF-8");

        // support different formats using the modelVersion id (held in xmlVersion_), possibly required in the future
        out << "<equipment>\n\n";
        out << "\t<!-- Note: The ids in this file will change between GC sessions, but the structure and relationships will be retained. --> \n\n";
        out << "\t<eqModel version=\"1\" lastCalc=\"" << equipModel_->rootItem_->getLastRecalc().toString() << "\"";
        out << " units=" << ((equipModel_->rootItem_->getMetricUnits()) ? "\"Metric\"" : "\"Imperial\"");
        out << " />\n\n\t<!-- The equipment list items: --> \n";

        // Write out the equipment list
        writeEquipTreeToXML(out, equipModel_->rootItem_);

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

        case eqNodeType::EQ_LINK: {
            out << "\n\t" << *(static_cast<EquipmentLink*>(eqNode));
        } break;
        case eqNodeType::EQ_DIST_ITEM: {
            out << "\t\t" << *(static_cast<EquipmentDistanceItem*>(eqNode));
        } break;

        case eqNodeType::EQ_TIME_ITEM: {
            out << "\t\t" << *(static_cast<EquipmentTimeItem*>(eqNode));
        } break;
        case eqNodeType::EQ_ITEM_REF: {
            out << "\t\t" << *(static_cast<EquipmentRef*>(eqNode));
        } break;
        case eqNodeType::EQ_SHARED: {
            out << "\n\t" << *(static_cast<SharedEquipment*>(eqNode));
        } break;
        case eqNodeType::EQ_SHARED_DIST_ITEM: {
            out << "\t\t" << *(static_cast<EquipmentSharedDistanceItem*>(eqNode));
        } break;
        case eqNodeType::EQ_BANNER: {
            out << "\t\t" << *(static_cast<EquipmentBanner*>(eqNode));
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
  
    else if (qName == "eqLink") {

        QString linkId, eqLinkName, gcActivities, gcDistance;

        // get attributes
        for (int i = 0; i < attrs.count(); i++) {
            if (attrs.qName(i) == "id") linkId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "eqLinkName") eqLinkName = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "gcActivities") gcActivities = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "gcDist") gcDistance = Utils::unprotect(attrs.value(i));
        }

        uint32_t optGCActivities = (gcActivities.isEmpty()) ? 0 : QVariant(gcActivities).toUInt();
        double optGCDistance = (gcDistance.isEmpty()) ? 0 : QVariant(gcDistance).toDouble();

        flatEqNodes_.push_back(flatEqNode(linkId, eqNodeType::EQ_LINK, "", "",
                    new EquipmentLink(eqLinkName, optGCActivities, optGCDistance)));
    }

    else if (qName == "eqDist") {

        QString eqId, eqParentId, description, notes, nonGCDistance;
        QString gcDistance, replacementDistance, start, end;

        // get attributes
        for (int i = 0; i < attrs.count(); i++) {
            if (attrs.qName(i) == "id") eqId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "parentId") eqParentId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "desc") description = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "notes") notes = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "nonGCDist") nonGCDistance = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "gcDist") gcDistance = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "replaceDist") replacementDistance = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "start") start = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "end") end = Utils::unprotect(attrs.value(i));
        }

        double optNonGCDistance = (nonGCDistance.isEmpty()) ? 0 : QVariant(nonGCDistance).toDouble();
        double optGCDistance = (gcDistance.isEmpty()) ? 0 : QVariant(gcDistance).toDouble();
        double optreplacementDistance = (replacementDistance.isEmpty()) ? 0 : QVariant(replacementDistance).toDouble();

        QDateTime startTS = start.isEmpty() ? QDateTime() : QDateTime().fromString(start);
        QDateTime endTS = end.isEmpty() ? QDateTime() : QDateTime().fromString(end);

        flatEqNodes_.push_back(flatEqNode(eqId, eqNodeType::EQ_DIST_ITEM, eqParentId, "",
                    new EquipmentDistanceItem(description, notes, optNonGCDistance, optGCDistance,
                                                optreplacementDistance, startTS, endTS)));
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

    else if (qName == "eqRefs") {

        QString useId, useParentId, eqIdRef, refDistance, start, end;

        // get attributes
        for (int i = 0; i < attrs.count(); i++) {
            if (attrs.qName(i) == "id") useId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "parentId") useParentId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "eqId") eqIdRef = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "refDist") refDistance = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "start") start = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "end") end = Utils::unprotect(attrs.value(i));
        }

        double optrefDistance = (refDistance.isEmpty()) ? 0 : QVariant(refDistance).toDouble();

        QDateTime startTS = start.isEmpty() ? QDateTime() : QDateTime().fromString(start);
        QDateTime endTS = end.isEmpty() ? QDateTime() : QDateTime().fromString(end);

        flatEqNodes_.push_back(flatEqNode(useId, eqNodeType::EQ_ITEM_REF, useParentId, eqIdRef,
                                new EquipmentRef(optrefDistance, startTS, endTS)));
    }

    else if (qName == "eqShared") {

        QString linkId;

        // get attributes
        for (int i = 0; i < attrs.count(); i++) {
            if (attrs.qName(i) == "id") linkId = Utils::unprotect(attrs.value(i));
        }

        flatEqNodes_.push_back(flatEqNode(linkId, eqNodeType::EQ_SHARED, "", "",
                                new SharedEquipment()));
    }

    else if (qName == "eqSharedDist") {

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

        flatEqNodes_.push_back(flatEqNode(eqId, eqNodeType::EQ_SHARED_DIST_ITEM, eqParentId, "",
            new EquipmentSharedDistanceItem(description, notes, optNonGCDistance, optGCDistance, optreplacementDistance)));
    }

    else if (qName == "eqBanner") {

        QString eqId, eqParentId, description;

        // get attributes
        for (int i = 0; i < attrs.count(); i++) {
            if (attrs.qName(i) == "id") eqId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "parentId") eqParentId = Utils::unprotect(attrs.value(i));
            if (attrs.qName(i) == "desc") description = Utils::unprotect(attrs.value(i));
        }

        flatEqNodes_.push_back(flatEqNode(eqId, eqNodeType::EQ_BANNER, eqParentId, "",
                                    new EquipmentBanner(description)));
    }

    return true;
}

bool EquipmentParser::characters(const QString& /* str */ ) {
    return true;
}

bool EquipmentParser::endDocument() {
    return true;
}


