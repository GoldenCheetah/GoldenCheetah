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

#ifndef _GC_EquipmentModelManager_h
#define _GC_EquipmentModelManager_h 1

#include "GoldenCheetah.h"
#include "EquipmentModel.h"

#include <QXmlDefaultHandler>
#include <QTextStream>
#include <QModelIndex>
#include <QVariant>
#include <QThread>

// Equipment identification, when importing from XML
class flatEqNode {
    public:
        flatEqNode(const QString& eqId, const eqNodeType nodeType,
                    const QString& eqParentId, const QString& eqIdRef, EquipmentNode* eqNode) :
            eqId_(eqId), nodeType_(nodeType),
            eqParentId_(eqParentId), eqIdRef_(eqIdRef), eqNode_(eqNode) {}

        QString eqId_;
        eqNodeType nodeType_;
        QString eqParentId_;
        QString eqIdRef_;
        EquipmentNode* eqNode_;
};

// reads in equipment
class EquipmentParser : public QXmlDefaultHandler {

    public:
        EquipmentParser(QVector<flatEqNode>& flatEqNodes, QDateTime& lastRecalcRef,
                        bool& metricUnitsRef, uint32_t& versionRef) :
             versionRef_(versionRef), metricUnitsRef_(metricUnitsRef),
             lastRecalcRef_(lastRecalcRef), flatEqNodes_(flatEqNodes) {}

        bool startDocument();
        bool endDocument();
        bool endElement(const QString&, const QString&, const QString& qName);
        bool startElement(const QString&, const QString&, const QString& qName, const QXmlAttributes& attrs);
        bool characters(const QString& str);

    protected:
        uint32_t& versionRef_;
        bool& metricUnitsRef_;
        QDateTime& lastRecalcRef_;
        QVector<flatEqNode>& flatEqNodes_;
};

class EquipmentModelManager;

class EquipmentModelRecalculationThread : public QThread
{
    public:
        EquipmentModelRecalculationThread(EquipmentModelManager* model) : model_(model) {}

    protected:

        // recalculate distances
        virtual void run() override;

    private:
        EquipmentModelManager* model_;
};

class EquipmentModelManager : public QObject
{
    Q_OBJECT

    public:

        EquipmentModelManager(Context* context);
        ~EquipmentModelManager();

        void close();

        EquipmentModel* equipModel_;

    public slots:

        void updateEqSettings(bool load);
        void eqRecalculationStart();

    protected:

        friend class ::EquipmentModelRecalculationThread;

        // XML input functions
        void loadEquipmentFromXML(QVector<flatEqNode>& flatEqNodes);
        bool createEquipmentNodeTree(QVector<flatEqNode>& flatEqNodes);

        // XML output functions
        void saveEquipmentToXML();
        void writeEquipTreeToXML(QTextStream& out, EquipmentNode* eqTreeNode); // recursive

        // distance calculation
        void RecalculateEq(RideItem* rideItem);
        RideItem* nextRideToCheck();
        void threadCompleted(EquipmentModelRecalculationThread* thread);
        void ResetTreeNodesBelowEqNode(EquipmentNode* eqNode); // recursive
        void applyDistanceToTreeNodes(EquipmentNode* eqNode, const QDateTime& actDateTime, const double dist); // recursive

        // Equipment distance recalculation
        QMutex updateMutex_;
        QVector<EquipmentModelRecalculationThread*> recalculationThreads_;
        QVector<RideItem*>  rideItemList_;

        // for debugging
        void printfEquipment();

        uint32_t xmlVersion_;
        Context* context_;
};

#endif // _GC_EquipmentModelManager_h
