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

enum class eqNodeType { EQ_ROOT, EQ_DIST_ITEM, EQ_TIME_ITEM, EQ_TIME_SPAN, EQ_ITEM_REF, EQ_LINK };

// Abstract Node class, providing the tree connection and maintenance functions
class EquipmentNode : public QObject
{
	Q_OBJECT

public:

	EquipmentNode() { parentItem_ = nullptr; }
	virtual ~EquipmentNode() { qDeleteAll(childItems_); };

	virtual eqNodeType getEqNodeType() const = 0;
	virtual QVariant data(int /* column */) const { return "node"; }

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
	void insertChild(int pos, EquipmentNode* eqChild);
	void removeChild(int row) { childItems_.removeAt(row); }
	bool removeChild(EquipmentNode* eqChild) { return childItems_.removeOne(eqChild); }

protected:

    // Tree structure pointers
    EquipmentNode* parentItem_;
    QVector<EquipmentNode*> childItems_;
};


// There is a single instance of this class, it is the root of the all the equipments.
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
	virtual QVariant data(int /* column */ ) const override { return "Root"; }

protected:
	// The last time the tree's nodes were recalculated.
	QDateTime lastRecalc_;
	bool metricUnits_;
};

class EquipmentRef;

// Manage things that wear out through use: running shoes, tyres, brake pads, etc
// Represents the actual distance based equipments which are assigned to EquipmentRefs
class EquipmentDistanceItem : public EquipmentNode
{
public:

    EquipmentDistanceItem(const QString& description, const QString& notes, double nonGCDistance = 0,
					double gcDistance=0, double replacementDistance=0);
	virtual ~EquipmentDistanceItem() {};

    virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_DIST_ITEM; }
    virtual QVariant data(int column) const;

	void resetDistanceCovered();
	void incrementDistanceCovered(double addDistance);
	void setNonGCDistance(double nonGCDistance);
	double getNonGCDistance() const { return nonGCDistance_; }
	double getGCDistance() const { return gcDistance_; }
	double getTotalDistance() const { return totalDistance_; }
	bool overDistance() const;

	// Equipment description
	QString description_;
	QString notes_;
	double replacementDistance_;

	QVector<EquipmentNode*> linkedRefs_;

    friend QTextStream& operator<<(QTextStream& out, const EquipmentDistanceItem& eqItem);

protected:
	double nonGCDistance_;
	std::atomic<double> gcDistance_;
	std::atomic<double> totalDistance_;
};


// Manage things that age over time: Bike services, Bleeding of brakes, etc
// Represents the actual time based equipments, held under the Equipment List
class EquipmentTimeItem : public EquipmentNode
{
public:
	EquipmentTimeItem(const QString& description, const QString& notes);
	EquipmentTimeItem(const QString& description, const QString& notes,
		const QDateTime& startDate, const QDateTime& replacementDate = QDateTime());
	virtual ~EquipmentTimeItem() {};

	virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_TIME_ITEM; }
	virtual QVariant data(int column) const;
	bool overTime() const;

	// Equipment description
	QString description_;
	QString notes_;
	QDateTime startDate_;
	QDateTime replacementDate_;

	friend QTextStream& operator<<(QTextStream& out, const EquipmentTimeItem& eqItem);
};


// Provides a mechanism to assign a time span to an equipment usage.
class EquipTimeSpan : public EquipmentNode
{
public:
	EquipTimeSpan();
	EquipTimeSpan(const QDateTime& start, const QString& notes = QString(), const QDateTime& end = QDateTime());
	virtual ~EquipTimeSpan() {};

	virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_TIME_SPAN; }
	virtual QVariant data(int column) const override;

	bool isWithin(const QDateTime& time) const;
    bool endIsNull() const { return end_.isNull(); }

	QString notes_;
	QDateTime start_;
	QDateTime end_;

    friend QTextStream& operator<<(QTextStream& out, const EquipTimeSpan& equipUse);
};


// Holds a reference
class EquipmentRef : public EquipmentNode
{
public:
	EquipmentRef(double refDistance = 0);
	virtual ~EquipmentRef() {};

	virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_ITEM_REF; }
	virtual QVariant data(int column) const override;

	void resetRefDistanceCovered() { refDistanceCovered_ = 0; }
	void incrementDistanceCovered(double addDistance); // Atomic safe 
	double getRefDistanceCovered() const { return refDistanceCovered_; }
	
	// Reference back to the equipment
	EquipmentDistanceItem* eqItem_;

	friend QTextStream& operator<<(QTextStream& out, const EquipmentRef& eqRef);

protected:

	std::atomic<double> refDistanceCovered_;

};


// The EquipmentLink represents the link to Activities/Rides
class EquipmentLink : public EquipmentNode
{
public:

    EquipmentLink(const QString& eqLinkName, const QString& description);
	virtual ~EquipmentLink() {};

    virtual eqNodeType getEqNodeType() const { return eqNodeType::EQ_LINK; }
    virtual QVariant data(int column) const override;

    QString getEqLinkName() const { return eqLinkName_; }
    void setEqLinkName(const QString& eqLinkName) { eqLinkName_ = eqLinkName; }

	// Equipment Link description
	QString description_;

    friend QTextStream& operator<<(QTextStream& out, const EquipmentLink& equipItem);

protected:

    // This field "links" to the ride/activity Metadata
    QString eqLinkName_;
};

Q_DECLARE_OPAQUE_POINTER(EquipmentNode*);
Q_DECLARE_METATYPE(EquipmentNode*)
 
#endif // _GC_EquipmentNode_h
