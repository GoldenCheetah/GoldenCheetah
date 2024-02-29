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

#include "EquipmentNode.h"
#include "Utils.h"

// ---------------------------------- Equipment Node  ---------------------------------

// returns the child that corresponds to the specified row number in the item's list of child items
EquipmentNode*
EquipmentNode::child(int row)
{
    return (row < 0 || row >= childItems_.size()) ? nullptr : childItems_.at(row);
}

//  returns the item's location within its parent's list of items:
int
EquipmentNode::row() const
{
	if (parentItem_ == nullptr) return 0;
	int index = parentItem_->childItems_.indexOf(const_cast<EquipmentNode*>(this));
	return (index == -1) ? 0 : index;
}

void
EquipmentNode::insertChild(int pos, EquipmentNode* child)
{
    childItems_.insert(pos, child);
    child->parentItem_ = this;
}

// ---------------------------------- Equipment Distance Item ---------------------------------

EquipmentDistanceItem::EquipmentDistanceItem(const QString& description, const QString& notes,
				double nonGCDistance, double gcDistance, double replacementDistance) :
	EquipmentNode(), description_(description), notes_(notes), nonGCDistance_(nonGCDistance),
	gcDistance_(gcDistance), replacementDistance_(replacementDistance),
	totalDistance_(nonGCDistance + gcDistance)
{
}

void
EquipmentDistanceItem::resetDistanceCovered()
{
	gcDistance_ = 0;
	totalDistance_ = nonGCDistance_;
}

void
EquipmentDistanceItem::setNonGCDistance(double nonGCDistance)
{
	nonGCDistance_ = nonGCDistance;
	totalDistance_ = nonGCDistance_ + gcDistance_;
}

void
EquipmentDistanceItem::incrementDistanceCovered(double addDistance) {

	// Atomic safe addition of doubles, without c++20
	auto current = gcDistance_.load();
	while (!gcDistance_.compare_exchange_weak(current, current + addDistance))
		;

	auto current1 = totalDistance_.load();
	while (!totalDistance_.compare_exchange_weak(current1, current1 + addDistance))
		;
}

bool
EquipmentDistanceItem::overDistance() const {

	return ((replacementDistance_ != 0) && (totalDistance_ > replacementDistance_));
}

QVariant
EquipmentDistanceItem::data(int /*column*/) const {

	return description_ + " [" + QString::number(totalDistance_, 'f', 0) + "]";
}

QTextStream&
operator<<(QTextStream& out, const EquipmentDistanceItem& eqNode) {

	out << "<eqDist "
		<< "id=\"" << &eqNode << "\" ";
		if (eqNode.parentItem_->getEqNodeType() == eqNodeType::EQ_ROOT)
			out << "parentId=\"eqRootNode\" ";
		else
			out << "parentId=\"" << eqNode.parentItem_ << "\" ";
		if (eqNode.nonGCDistance_ != 0) {
			out << "nonGCDist=\"" << eqNode.nonGCDistance_ << "\" ";
		}
		if (eqNode.gcDistance_ != 0) {
			out << "gcDist=\"" << eqNode.gcDistance_ << "\" ";
		}
		if (eqNode.replacementDistance_ != 0) {
			out << "replaceDist=\"" << eqNode.replacementDistance_ << "\" ";
		}
		if (!eqNode.description_.isEmpty()) {
			out << "desc=\"" << Utils::xmlprotect(eqNode.description_) << "\" ";
		}
		if (!eqNode.notes_.isEmpty()) {
			out << "notes=\"" << Utils::xmlprotect(eqNode.notes_) << "\" ";
		}
		out << "/>\n";

	return out;
}

// ---------------------------------- Equipment Time Item  ---------------------------------

EquipmentTimeItem::EquipmentTimeItem(const QString& description, const QString& notes) :
	EquipmentNode(), description_(description), notes_(notes)
{
	start_ = QDateTime(QDate::currentDate().startOfDay());
	replace_ = QDateTime();
}

EquipmentTimeItem::EquipmentTimeItem(const QString& description, const QString& notes,
		const QDateTime& start,	const QDateTime& replace) :
	EquipmentNode(), description_(description), notes_(notes),
	start_(start)
{
	replace_ = (replace.isNull()) ? QDateTime(QDate::currentDate().endOfDay()) : replace;
}

bool
EquipmentTimeItem::overTime() const {
	return (QDateTime::currentDateTime().daysTo(replace_) < 0);
}

QVariant
EquipmentTimeItem::data(int /*column*/) const {

	return description_ + " <" + QVariant(abs(QDateTime::currentDateTime().daysTo(replace_))).toString() + " days>";
}

QTextStream&
operator<<(QTextStream& out, const EquipmentTimeItem& eqNode) {

	out << "<eqTime "
		<< "id=\"" << &eqNode << "\" ";
	if (eqNode.parentItem_->getEqNodeType() == eqNodeType::EQ_ROOT)
		out << "parentId=\"eqRootNode\" ";
	else
		out << "parentId=\"" << eqNode.parentItem_ << "\" ";
	out << "start=\"" << eqNode.start_.toString() << "\" ";
	if (!eqNode.replace_.isNull()) {
		out << "replaceDate=\"" << eqNode.replace_.toString() << "\" ";
	}
	if (!eqNode.description_.isEmpty()) {
		out << "desc=\"" << Utils::xmlprotect(eqNode.description_) << "\" ";
	}
	if (!eqNode.notes_.isEmpty()) {
		out << "notes=\"" << Utils::xmlprotect(eqNode.notes_) << "\" ";
	}
	out << "/>\n";

	return out;
}


// ---------------------------------- Equipment Reference  ---------------------------------

EquipmentRef::EquipmentRef() :
	EquipmentNode(), eqDistNode_(nullptr), refDistanceCovered_(0),
	start_(QDateTime()), end_(QDateTime()), notes_(QString())
{
}

EquipmentRef::EquipmentRef(double refDistance, const QDateTime& start,
							const QDateTime& end, const QString& notes) :
	EquipmentNode(), eqDistNode_(nullptr), refDistanceCovered_(refDistance),
	start_(start), end_(end), notes_(notes)
{
}

QVariant
EquipmentRef::data(int /*column*/) const {

	QString dataString;

	// use ':' to split date & equipment text in equipment navigator
	if (start_.isNull())
		if (!end_.isNull())
			dataString += end_.toString("->d MMM yyyy: ");
		else
			;
	else
		if (end_.isNull())
			dataString += start_.toString("d MMM yyyy->: ");
		else
			dataString += start_.toString("d MMM yyyy->") + end_.toString("d MMM yyyy: ");

	dataString += (eqDistNode_) ? eqDistNode_->description_ + " [" + QString::number(refDistanceCovered_, 'f', 0) + "]" : tr("Referenced Equipment Missing!");

	return dataString;
}

bool
EquipmentRef::rangeIsValid() const {

	if (!start_.isNull() && (!end_.isNull()))
		return end_ >= start_;
	else
		return true;
}


bool
EquipmentRef::isWithin(const QDateTime& time) const {

	if (start_.isNull())
		if (end_.isNull())
			return true; // no range set
		else
			return (time < end_); // end set but not beginning !
	else
		if (end_.isNull())
			return (start_ <= time);
		else
			return (start_ <= time) && (time < end_);
}

void
EquipmentRef::incrementDistanceCovered(double addDistance) {

	// Atomic safe addition of doubles, without c++20
	auto current = refDistanceCovered_.load();
	while (!refDistanceCovered_.compare_exchange_weak(current, current + addDistance))
		;

	// Update the referenced original equipment item
	if (eqDistNode_) eqDistNode_->incrementDistanceCovered(addDistance);
}

QTextStream& operator<<(QTextStream& out, const EquipmentRef& eqRef)
{
	out << "<eqRef "
		<< "id=\"" << &eqRef << "\" "
		<< "parentId=\"" << eqRef.parentItem_ << "\" ";
	if (eqRef.eqDistNode_)
		out << "eqId=\"" << eqRef.eqDistNode_ << "\" ";
	else
		out << "eqId=\"eqReferenceMissing\" ";
	if (eqRef.refDistanceCovered_ != 0) {
		out << "refDist=\"" << eqRef.refDistanceCovered_ << "\" ";
	}
	if (!eqRef.start_.isNull()) {
		out << "start=\"" << eqRef.start_.toString() << "\" ";
	}
	if (!eqRef.end_.isNull()) {
		out << "end=\"" << eqRef.end_.toString() << "\" ";
	}
	if (!eqRef.notes_.isEmpty()) {
		out << "notes=\"" << Utils::xmlprotect(eqRef.notes_) << "\" ";
	}
	out << "/>\n";

	return out;
}

// ------------------------------ Equipment Link ------------------------------------

EquipmentLink::EquipmentLink(const QString& eqLinkName, const QString& description) :
	EquipmentNode(), eqLinkName_(eqLinkName), description_(description)
{
}

QVariant
EquipmentLink::data(int /*column*/) const {
    return eqLinkName_;
}

QTextStream& operator<<(QTextStream& out, const EquipmentLink& eqNode) {

	out << "<eqLink "
		<< "id=\"" << &eqNode << "\" "
		<< "eqLinkName=\"" << Utils::xmlprotect(eqNode.eqLinkName_) << "\" ";
	if (!eqNode.description_.isEmpty()) {
		out << "desc=\"" << Utils::xmlprotect(eqNode.description_) << "\" ";
	}
	out << "/>\n";

    return out;
}

