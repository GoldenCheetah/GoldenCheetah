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

#ifndef _GC_EquipmentNode_h
#define _GC_EquipmentNode_h 1
#include "GoldenCheetah.h"

#include <QString>
#include <iostream>
#include <memory>

enum class eqNodeType { EQ_ROOT, EQ_DIST_ITEM, EQ_SHARED_DIST_ITEM, EQ_TIME_ITEM, EQ_ITEM_REF, EQ_LINK, EQ_SHARED, EQ_BANNER};

// Abstract Node class, providing the tree connection and maintenance functions
class EquipmentNode : public QObject
{
    Q_OBJECT

public:

    EquipmentNode() { parentItem_ = nullptr; }
    virtual ~EquipmentNode() { qDeleteAll(childItems_); };

    virtual eqNodeType getEqNodeType() const = 0;
    virtual QVariant data(int /* column */) const = 0;

    void appendChild(EquipmentNode* eqChild) { childItems_.append(eqChild); }
    QVector<EquipmentNode*>& getChildren() { return childItems_; }
    int childCount() const { return childItems_.count(); }

    void setParentItem(EquipmentNode* eqParent) { parentItem_ = eqParent; }
    EquipmentNode* getParentItem() const { return parentItem_; }

    // QAbstractItemModel functions
    int columnCount() const { return 1; }
    EquipmentNode* child(int row);
    int row() const;

    // Drag & drop support
    int childPosition(EquipmentNode* eqChild) { return childItems_.indexOf(eqChild); }
    void insertChild(int pos, EquipmentNode* eqChild);
    void removeChild(int row) { childItems_.removeAt(row); }
    bool removeChild(EquipmentNode* eqChild) { return childItems_.removeOne(eqChild); }

protected:

    // Tree structure pointers
    EquipmentNode* parentItem_;
    QVector<EquipmentNode*> childItems_;
};


// There is a single instance of this class, it is the root of the all the equipment.
class EquipmentRoot : public EquipmentNode
{
public:
    EquipmentRoot() {};
    virtual ~EquipmentRoot() {};

    void setLastRecalc(const QDateTime& reclac) { lastRecalc_ = reclac; }
    QDateTime getLastRecalc() const { return lastRecalc_; }

    void setMetricUnits(bool metricUnits) { metricUnits_ = metricUnits; }
    bool getMetricUnits() const { return metricUnits_; }

    virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_ROOT; }
    virtual QVariant data(int /* column */) const override { return "Root"; }

protected:
    // The last time the tree's nodes were recalculated.
    QDateTime lastRecalc_;
    bool metricUnits_;
};

// There is a single instance of this class, it is the root of the all the shared equipment.
class SharedEquipment : public EquipmentNode
{
public:
    SharedEquipment() {};
    virtual ~SharedEquipment() {};

    virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_SHARED; }
    virtual QVariant data(int /* column */) const override { return "Shared Equipment"; }
    
    friend QTextStream& operator<<(QTextStream& out, const SharedEquipment& eqNode);
};

// Provides a separation for equipments
class EquipmentBanner : public EquipmentNode
{
public:
    EquipmentBanner(const QString& description);
    virtual ~EquipmentBanner() {};

    virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_BANNER; }
    virtual QVariant data(int /* column */) const override { return description_; }

    // Equipment banner description
    QString description_;

    friend QTextStream& operator<<(QTextStream& out, const EquipmentBanner& eqNode);
};

class EquipmentRef;

// Manage things that wear out through use: running shoes, tyres, brake pads, etc
class EquipmentDistanceNode : public EquipmentNode
{
public:

    EquipmentDistanceNode(const QString& description, const QString& notes);
    EquipmentDistanceNode(const QString& description, const QString& notes,
                         double nonGCDistance, double gcDistance, double replacementDistance);
    virtual ~EquipmentDistanceNode() {};

    void resetDistanceCovered();
    void incrementDistanceCovered(double addDistance); // Atomic safe 
    void setNonGCDistance(double nonGCDistance);
    double getNonGCDistance() const { return nonGCDistance_; }
    double getGCDistance() const { return gcDistance_; }
    double getTotalDistance() const { return totalDistance_; }
    bool overDistance() const;

    // Equipment description
    QString description_;
    QString notes_;
    double replacementDistance_;

protected:
    double nonGCDistance_;
    std::atomic<double> gcDistance_;
    std::atomic<double> totalDistance_;
};

// Manage things that wear out through use: running shoes, tyres, brake pads, etc
class EquipmentDistanceItem : public EquipmentDistanceNode
{
public:

    EquipmentDistanceItem(const QString& description, const QString& notes);
    EquipmentDistanceItem(const QString& description, const QString& notes,
                            double nonGCDistance, double gcDistance, double replacementDistance,
                            const QDateTime& start,    const QDateTime& end);
    virtual ~EquipmentDistanceItem() {};

    virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_DIST_ITEM; }
    virtual QVariant data(int column) const;

    bool isTimeRestrictionSet() const;
    bool rangeIsValid() const;
    bool isWithin(const QDateTime& time) const;
    bool startNotSet() const { return start_.isNull(); }
    bool endNotSet() const { return end_.isNull(); }

    QDateTime start_;
    QDateTime end_;

    friend QTextStream& operator<<(QTextStream& out, const EquipmentDistanceItem& eqNode);
};

// Manage things that are shared between equipment activity links (think wheelsets on bikes)
// Represents the actual distance based equipment which are assigned to EquipmentRefs
class EquipmentSharedDistanceItem : public EquipmentDistanceNode
{
public:

    EquipmentSharedDistanceItem(const QString& description, const QString& notes);
    EquipmentSharedDistanceItem(const QString& description, const QString& notes,
                        double nonGCDistance, double gcDistance, double replacementDistance);
    virtual ~EquipmentSharedDistanceItem() {};

    virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_SHARED_DIST_ITEM; }
    virtual QVariant data(int column) const;

    QVector<EquipmentNode*> linkedRefs_;

    friend QTextStream& operator<<(QTextStream& out, const EquipmentSharedDistanceItem& eqNode);
};


// Manage things that age over time: Bike services, Bleeding of brakes, etc
// Represents the actual time based equipment, held under the Equipment List
class EquipmentTimeItem : public EquipmentNode
{
public:
    EquipmentTimeItem(const QString& description, const QString& notes);
    EquipmentTimeItem(const QString& description, const QString& notes,
                    const QDateTime& start, const QDateTime& replace = QDateTime());
    virtual ~EquipmentTimeItem() {};

    virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_TIME_ITEM; }
    virtual QVariant data(int column) const;
    bool overTime() const;

    // Equipment description
    QString description_;
    QString notes_;
    QDateTime start_;
    QDateTime replace_;

    friend QTextStream& operator<<(QTextStream& out, const EquipmentTimeItem& eqNode);
};


// Holds an equipment reference, plus an optional time span for the equipment's usage
class EquipmentRef : public EquipmentNode
{
public:
    EquipmentRef();
    EquipmentRef(double refDistance, const QDateTime& start, const QDateTime& end);
    virtual ~EquipmentRef();

    virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_ITEM_REF; }
    virtual QVariant data(int column) const override;

    void resetRefDistanceCovered() { refDistanceCovered_ = 0; }
    void incrementDistanceCovered(double addDistance); // Atomic safe 
    double getRefDistanceCovered() const { return refDistanceCovered_; }

    bool isTimeRestrictionSet() const;
    bool rangeIsValid() const;
    bool isWithin(const QDateTime& time) const;
    bool startNotSet() const { return start_.isNull(); }
    bool endNotSet() const { return end_.isNull(); }

    // Reference back to the equipment
    EquipmentSharedDistanceItem* eqDistNode_;
    QDateTime start_;
    QDateTime end_;

    friend QTextStream& operator<<(QTextStream& out, const EquipmentRef& eqRef);

protected:

    std::atomic<double> refDistanceCovered_;
};


// The EquipmentLink represents the link to Activities/Rides
class EquipmentLink : public EquipmentNode
{
public:

    EquipmentLink(const QString& eqLinkName);
    EquipmentLink(const QString& eqLinkName, const uint32_t gcActivities, const double gcDistance);
    virtual ~EquipmentLink() {};

    virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_LINK; }
    virtual QVariant data(int column) const override;

    QString getEqLinkName() const { return eqLinkName_; }
    void setEqLinkName(const QString& eqLinkName) { eqLinkName_ = eqLinkName; }

    uint32_t getActivities() const { return gcActivities_; }
    double getGCDistance() const { return gcDistance_; }

    void resetActivities() { gcActivities_ = 0; }
    void resetDistanceCovered() { gcDistance_ = 0; }

    void incrementActivities() { gcActivities_++; } // Atomic safe 
    void incrementDistanceCovered(double addDistance); // Atomic safe 


    friend QTextStream& operator<<(QTextStream& out, const EquipmentLink& equipItem);

protected:

    std::atomic<uint32_t> gcActivities_;
    std::atomic<double> gcDistance_;

    // This field "links" to the ride/activity Metadata
    QString eqLinkName_;
};

Q_DECLARE_OPAQUE_POINTER(EquipmentNode*);
Q_DECLARE_METATYPE(EquipmentNode*)

#endif // _GC_EquipmentNode_h
