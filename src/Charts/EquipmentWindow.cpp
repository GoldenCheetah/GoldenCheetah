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

#include "EquipmentWindow.h"

#include "Colors.h"
#include "Context.h"
#include "RideMetadata.h"
#include "MainWindow.h" // for gcroot
#include "HelpWhatsThis.h"

#include <QPlainTextEdit>


EquipmentWindow::EquipmentWindow(Context *context) :
    GcChartWindow(context), context_(context), selectedNode_(nullptr), savingDisabled_(false)
{
	// Setup GcChartWindow
	setControls(NULL);
	setRideItem(NULL);
	setContentsMargins(0, 0, 0, 0);

	menuButton->setEnabled(false);
	menuButton->hide();

    HelpWhatsThis* help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::EquipmentWindow_Details));

	// The widget adding order must match the page enum class eqWinType
	stackedWidget_ = new QStackedWidget();
	stackedWidget_->addWidget(new QLabel()); // blank page
	stackedWidget_->addWidget(createWidsEquipmentDistanceItem());
	stackedWidget_->addWidget(createWidsEquipmentTimeItem());
	stackedWidget_->addWidget(createWidsTimeSpan());
	stackedWidget_->addWidget(createWidsReference());
	stackedWidget_->addWidget(createWidsEquipmentLink());

	QVBoxLayout* saveCancellayout = new QVBoxLayout;
	saveButton_ = new QPushButton(tr("Save"));
	undoButton_ = new QPushButton(tr("Undo"));
	saveCancellayout->addWidget(new QLabel());
	saveCancellayout->addWidget(new QLabel());
	saveCancellayout->addWidget(saveButton_);
	saveCancellayout->addWidget(undoButton_);
	saveCancellayout->insertStretch(-1);

	QHBoxLayout* hlayout = new QHBoxLayout();
	hlayout->addWidget(stackedWidget_);
	hlayout->addLayout(saveCancellayout);
	hlayout->insertStretch(-1);

	QVBoxLayout* vlayout = new QVBoxLayout();
	lastUpdated_ = new QLabel();
	vlayout->addWidget(lastUpdated_);
	vlayout->addWidget(new QLabel());
	vlayout->addLayout(hlayout);
	vlayout->insertStretch(-1);

	setChartLayout(vlayout);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
	connect(saveButton_, SIGNAL(clicked()), this, SLOT(saveButtonClicked()));
	connect(undoButton_, SIGNAL(clicked()), this, SLOT(undoButtonClicked()));
    connect(context, SIGNAL(equipmentSelected(EquipmentNode*, bool)), this, SLOT(equipmentSelected(EquipmentNode*, bool)));
	connect(context, SIGNAL(eqRecalculationStart()), this, SLOT(eqRecalculationStart()));
	connect(context, SIGNAL(eqRecalculationEnd()), this, SLOT(eqRecalculationEnd()));
	connect(context, SIGNAL(updateEqSettings(bool)), this, SLOT(updateEqSettings(bool)));

	updateEquipmentDisplay();

    // set colors
    configChanged(CONFIG_APPEARANCE);
}

void
EquipmentWindow::equipmentSelected(EquipmentNode* eqItem, bool /* eqListView */)
{
	selectedNode_ = eqItem;
	updateEquipmentDisplay();
}

void
EquipmentWindow::eqRecalculationStart()
{
	savingDisabled_ = true;
	updateEquipmentDisplay();
}

void
EquipmentWindow::eqRecalculationEnd()
{
	savingDisabled_ = false;
	updateEquipmentDisplay();
}

void
EquipmentWindow::updateEqSettings(bool load)
{
	// the xml equipment file is being loaded, therefore the node List will be 
	// destroyed and the selectedNode_ pointer will become invalid.
	if (load) {
		selectedNode_ = nullptr;
		updateEquipmentDisplay();
	}
}

void
EquipmentWindow::updateEquipmentDisplay()
{
	bool hideButtons = false;

    if (selectedNode_) {

		// find the root equipment
		EquipmentNode* eqRoot = selectedNode_;
		while (eqRoot->getEqNodeType() != eqNodeType::EQ_ROOT) eqRoot = eqRoot->getParentItem();

		metricUnits_ = static_cast<EquipmentRoot*>(eqRoot)->getMetricUnits();

		QDateTime lastRecalc = static_cast<EquipmentRoot*>(eqRoot)->getLastRecalc();
		static QString time_format = "HH:mm   dd MMM yyyy";
		QVariant days(abs(QDateTime::currentDateTime().daysTo(lastRecalc)));
		lastUpdated_->setText("Last Recalc:  " +
			lastRecalc.toString(time_format) +
			((days==0) ? "  <today>" : ((days==1) ? "  <yesterday>" : "  <" + days.toString() + " days ago>")));

		switch (selectedNode_->getEqNodeType()) {

		case eqNodeType::EQ_LINK: {
			displayEquipmentLink(static_cast<EquipmentLink*>(selectedNode_));
		} break;

		case eqNodeType::EQ_TIME_SPAN: {
			displayTimeSpan(static_cast<EquipTimeSpan*>(selectedNode_));
		} break;

		case eqNodeType::EQ_ITEM_REF: {
			displayReference(static_cast<EquipmentRef*>(selectedNode_));
			hideButtons = true;
		} break;

		case eqNodeType::EQ_DIST_ITEM: {
			displayEquipmentDistanceItem(static_cast<EquipmentDistanceItem*>(selectedNode_));
		} break;

		case eqNodeType::EQ_TIME_ITEM: {
			displayEquipmentTimeItem(static_cast<EquipmentTimeItem*>(selectedNode_));
		} break;
		}
	}
	else {
		stackedWidget_->setCurrentIndex(int(eqWinType::BLANK_PAGE));
		hideButtons = true;
	}

	saveButton_->setHidden(hideButtons || savingDisabled_);
	undoButton_->setHidden(hideButtons || savingDisabled_);
}

// ------------------------------- Equipment Distance Item --------------------------------------

QWidget*
EquipmentWindow::createWidsEquipmentDistanceItem() {

	QVector< widgetMapType> pageWids;

	pageWids.push_back(struct widgetMapType { new QLabel(), new QLabel(tr("Distance Equipment")), false });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Description")), new QLineEdit(), true });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Non GC Distance")), new QLineEdit(), true });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("GC Distance")), new QLineEdit(), false });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Total Distance")), new QLineEdit(), false });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Replacement Distance")), new QLineEdit(), true });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Notes")), new QPlainTextEdit(), true });

	equipWids_[eqWinType::EQUIPMENT_DIST_PAGE] = pageWids;

	return setupWids(pageWids);
}

QString
EquipmentWindow::unitString()
{
	return (metricUnits_) ? "\n(km)" : "\n(miles)";
}

void
EquipmentWindow::displayEquipmentDistanceItem(EquipmentDistanceItem* eqItem) {

	QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_DIST_PAGE];

	static_cast<QLabel*>(pageWids[2].labelPtr)->setText(tr("Non GC Distance") + unitString());
	static_cast<QLabel*>(pageWids[3].labelPtr)->setText(tr("GC Distance") + unitString());
	static_cast<QLabel*>(pageWids[4].labelPtr)->setText(tr("Total Distance") + unitString());
	static_cast<QLabel*>(pageWids[5].labelPtr)->setText(tr("Replacement Distance") + unitString());

	static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(eqItem->description_);
	static_cast<QLineEdit*>(pageWids[2].fieldPtr)->setText(QString::number(eqItem->getNonGCDistance(), 'f', 1));
	static_cast<QLineEdit*>(pageWids[3].fieldPtr)->setText(QString::number(eqItem->getGCDistance(), 'f', 1));
	static_cast<QLineEdit*>(pageWids[4].fieldPtr)->setText(QString::number(eqItem->getTotalDistance(), 'f', 1));
	static_cast<QLineEdit*>(pageWids[5].fieldPtr)->setText(QString::number(eqItem->replacementDistance_, 'f', 1));
	static_cast<QPlainTextEdit*>(pageWids[6].fieldPtr)->setPlainText(eqItem->notes_);

	stackedWidget_->setCurrentIndex(int(eqWinType::EQUIPMENT_DIST_PAGE));
}

void
EquipmentWindow::saveEquipmentDistanceItem(EquipmentDistanceItem* eqItem) {

	QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_DIST_PAGE];

	eqItem->description_ = static_cast<QLineEdit*>(pageWids[1].fieldPtr)->text();
	eqItem->setNonGCDistance(QVariant(static_cast<QLineEdit*>(pageWids[2].fieldPtr)->text()).toDouble());
	eqItem->replacementDistance_ = QVariant(static_cast<QLineEdit*>(pageWids[5].fieldPtr)->text()).toDouble();
	eqItem->notes_ = static_cast<QPlainTextEdit*>(pageWids[6].fieldPtr)->toPlainText();

	// need to ensure new total distance is correctly displayed
	updateEquipmentDisplay();
}

// ------------------------------- Equipment Time Item --------------------------------------

QWidget*
EquipmentWindow::createWidsEquipmentTimeItem() {

	QVector< widgetMapType> pageWids;

	pageWids.push_back(struct widgetMapType { new QLabel(), new QLabel(tr("Time Equipment")), false });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Description")), new QLineEdit(), true });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Start Date")), new QDateTimeEdit(), true });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Replacement Date")), new QDateTimeEdit(), true });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Notes")), new QPlainTextEdit(), true });

	static_cast<QDateTimeEdit*>(pageWids[2].fieldPtr)->setCalendarPopup(true);
	static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->setCalendarPopup(true);

	equipWids_[eqWinType::EQUIPMENT_TIME_PAGE] = pageWids;

	return setupWids(pageWids);
}

void
EquipmentWindow::displayEquipmentTimeItem(EquipmentTimeItem* eqItem) {

	QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_TIME_PAGE];

	static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(eqItem->description_);

	// Setting null QDateTime doesn't reset the QDateTimeEdit field
	if (eqItem->startDate_.isNull()) {
		static_cast<QDateTimeEdit*>(pageWids[2].fieldPtr)->setDateTime(QDateTime(QDate::currentDate().startOfDay()));
	}
	else {
		static_cast<QDateTimeEdit*>(pageWids[2].fieldPtr)->setDateTime(eqItem->startDate_);
	}

	if (eqItem->replacementDate_.isNull()) {
		static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->setDateTime(QDateTime(QDate::currentDate().endOfDay()));
	}
	else {
		static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->setDateTime(eqItem->replacementDate_);
	}
	static_cast<QPlainTextEdit*>(pageWids[4].fieldPtr)->setPlainText(eqItem->notes_);

	stackedWidget_->setCurrentIndex(int(eqWinType::EQUIPMENT_TIME_PAGE));
}

void
EquipmentWindow::saveEquipmentTimeItem(EquipmentTimeItem* eqItem) {

	QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_TIME_PAGE];

	eqItem->description_ = static_cast<QLineEdit*>(pageWids[1].fieldPtr)->text();
	eqItem->startDate_ = static_cast<QDateTimeEdit*>(pageWids[2].fieldPtr)->dateTime();
	eqItem->replacementDate_ = static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->dateTime();
	eqItem->notes_ = static_cast<QPlainTextEdit*>(pageWids[4].fieldPtr)->toPlainText();
}

// ----------------------------- Equipment Time Span ------------------------------------

QWidget*
EquipmentWindow::createWidsTimeSpan() {

	QVector< widgetMapType> pageWids;

	pageWids.push_back(struct widgetMapType { new QLabel(), new QLabel(tr("Time Span")), false });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Start Date")), new QDateTimeEdit(), true });
	pageWids.push_back(struct widgetMapType { new QLabel(""), new QCheckBox(tr("No End Date")), true });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("End Date")), new QDateTimeEdit(), true });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Notes")), new QPlainTextEdit(), true });


	static_cast<QDateTimeEdit*>(pageWids[1].fieldPtr)->setCalendarPopup(true);
	connect(static_cast<QCheckBox*>(pageWids[2].fieldPtr), SIGNAL(stateChanged(int)), this, SLOT(endDateTimeStateChanged(int)));
	static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->setCalendarPopup(true);

	equipWids_[eqWinType::TIME_SPAN_PAGE] = pageWids;

	return setupWids(pageWids);
}

void
EquipmentWindow::endDateTimeStateChanged(int checkState) {

	QVector< widgetMapType>& pageWids = equipWids_[eqWinType::TIME_SPAN_PAGE];

	static_cast<QDateTimeEdit*>(pageWids[3].labelPtr)->setHidden(checkState == Qt::Checked);
	static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->setHidden(checkState == Qt::Checked);
}

void
EquipmentWindow::displayTimeSpan(EquipTimeSpan* eqTimeSpan) {

	QVector< widgetMapType>& pageWids = equipWids_[eqWinType::TIME_SPAN_PAGE];

	static_cast<QDateTimeEdit*>(pageWids[1].fieldPtr)->setDateTime(eqTimeSpan->start_);

	Qt::CheckState state = (eqTimeSpan->endIsNull()) ? Qt::Checked : Qt::Unchecked;
	static_cast<QCheckBox*>(pageWids[2].fieldPtr)->setCheckState(state);

	// Setting null QDateTime doesn't reset the QDateTimeEdit field
	if (eqTimeSpan->end_.isNull()) {
		static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->setDateTime(QDateTime(QDate::currentDate().endOfDay()));
	}
	else {
		static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->setDateTime(eqTimeSpan->end_);
	}
	static_cast<QPlainTextEdit*>(pageWids[4].fieldPtr)->setPlainText(eqTimeSpan->notes_);

	stackedWidget_->setCurrentIndex(int(eqWinType::TIME_SPAN_PAGE));
}

void
EquipmentWindow::saveTimeSpan(EquipTimeSpan* eqTimeSpan) {

	QVector< widgetMapType>& pageWids = equipWids_[eqWinType::TIME_SPAN_PAGE];

	eqTimeSpan->start_ = static_cast<QDateTimeEdit*>(pageWids[1].fieldPtr)->dateTime();

	if (static_cast<QCheckBox*>(pageWids[2].fieldPtr)->checkState() == Qt::Checked) {
		eqTimeSpan->end_ = QDateTime();
	} else {
		eqTimeSpan->end_ = static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->dateTime();
	}
	eqTimeSpan->notes_ = static_cast<QPlainTextEdit*>(pageWids[4].fieldPtr)->toPlainText();
}

// ----------------------------- Equipment Reference  ------------------------------------

QWidget*
EquipmentWindow::createWidsReference() {

	QVector< widgetMapType> pageWids;

	pageWids.push_back(struct widgetMapType { new QLabel(), new QLabel(tr("Distance Equipment Reference")), false });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Description")), new QLineEdit(), false	});
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Ref Distance Covered")), new QLineEdit(), false });

	equipWids_[eqWinType::REFERENCE_PAGE] = pageWids;

	return setupWids(pageWids);
}

void
EquipmentWindow::displayReference(EquipmentRef* eqRef) {

	QVector< widgetMapType>& pageWids = equipWids_[eqWinType::REFERENCE_PAGE];

	static_cast<QLabel*>(pageWids[2].labelPtr)->setText(tr("Ref Distance Covered") + unitString());

	EquipmentDistanceItem* eqItem = eqRef->getEqItem();

	if (eqItem) {

		static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(eqItem->description_);
		static_cast<QLineEdit*>(pageWids[2].fieldPtr)->setText(QString::number(eqRef->getRefDistanceCovered(), 'f', 1));
	}
	else {
		static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText("Referenced Equipment Missing!");
	}

	stackedWidget_->setCurrentIndex(int(eqWinType::REFERENCE_PAGE));
}

// ----------------------------- Activity Link  ------------------------------------

QWidget*
EquipmentWindow::createWidsEquipmentLink() {

	QVector< widgetMapType> pageWids;

	pageWids.push_back(struct widgetMapType { new QLabel(), new QLabel(tr("Equipment Link (to Activities)")), false });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Equipment Link")), new QLineEdit(), true });
	pageWids.push_back(struct widgetMapType { new QLabel(tr("Description")), new QLineEdit(), true });

	// Make the equipment link field the same size as the FIELD_SHORTTEXT
	QLineEdit* eqLinkEdit = static_cast<QLineEdit*>(pageWids[1].fieldPtr);
	QFontMetrics fm = eqLinkEdit->fontMetrics();
	int w = fm.boundingRect("XXXXXXXXXXXXXXX").width();
	eqLinkEdit->setMaximumWidth(w);

	equipWids_[eqWinType::ACTIVITY_LINK_PAGE] = pageWids;

	return setupWids(pageWids);
}

void
EquipmentWindow::displayEquipmentLink(EquipmentLink* actEqLink) {

	QVector< widgetMapType>& pageWids = equipWids_[eqWinType::ACTIVITY_LINK_PAGE];

	static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(actEqLink->getEqLinkName());
	static_cast<QLineEdit*>(pageWids[2].fieldPtr)->setText(actEqLink->description_);

	stackedWidget_->setCurrentIndex(int(eqWinType::ACTIVITY_LINK_PAGE));
}

void
EquipmentWindow::saveEquipmentLink(EquipmentLink* actEqLink) {


	QVector< widgetMapType>& pageWids = equipWids_[eqWinType::ACTIVITY_LINK_PAGE];
	
	actEqLink->description_ = static_cast<QLineEdit*>(pageWids[2].fieldPtr)->text();

	QList<FieldDefinition> fieldDefinitions = GlobalContext::context()->rideMetadata->getFields();

	// If the equipment Link name has changed, then if its
	// unique to this equipment link remove it from the choice list
	if (actEqLink->getEqLinkName() != static_cast<QLineEdit*>(pageWids[1].fieldPtr)->text()) {

		// find my root parent, this must be the reference tree as this is an equipment link
		EquipmentNode* eqRoot = actEqLink->getParentItem();

		bool uniqueEqLinkName = true;

		// now check that other equipment links do not have the same name as the one being replaced
		for (EquipmentNode* rootChild : eqRoot->getChildren()) {
			// don't check myself!
			if (rootChild != actEqLink) {
				uniqueEqLinkName &= (static_cast<EquipmentLink*>(rootChild)->getEqLinkName() != actEqLink->getEqLinkName());
			}
		}

		// if the old name is unique (and not used elsewhere) then remove it from the choice list
		if (uniqueEqLinkName) {
			for (FieldDefinition& field : fieldDefinitions) {
				if (field.name == "EquipmentLink") {
					for (int i = 0; i < field.values.size(); ++i) {
						if (field.values.at(i).contains(actEqLink->getEqLinkName()))
							field.values.removeAt(i);
					}
					break;
				}
			}
		}

		// now update the model with the new equipment link name
		actEqLink->setEqLinkName(static_cast<QLineEdit*>(pageWids[1].fieldPtr)->text());
	}

	// update the choice list with the new equipment link name, in case its not already in the choice list
	for (FieldDefinition& field : fieldDefinitions) {
		if (field.name == "EquipmentLink") {

			// if the equipment link name is missing t
			if (!field.values.contains(actEqLink->getEqLinkName())) {

				// add missing equipment link to the choice list
				field.values.append(actEqLink->getEqLinkName());

				// keep the choices tidy (just incase their are two eqLinks with this name)
				field.values.removeDuplicates();

				// write to metadata.xml
				RideMetadata::serialize(QDir(gcroot).absolutePath() + "/metadata.xml",
					GlobalContext::context()->rideMetadata->getKeywords(),
					fieldDefinitions,
					GlobalContext::context()->rideMetadata->getColorField(),
					GlobalContext::context()->rideMetadata->getDefaults());

				// global context changed, will be cascaded to each athlete context
				GlobalContext::context()->notifyConfigChanged(CONFIG_FIELDS);
			}
			break;
		}
	}
}

QWidget*
EquipmentWindow::setupWids(QVector< widgetMapType>& widMap) {

	QWidget* wid = new QWidget;

	QGridLayout* eqLayout = new QGridLayout(wid);
	eqLayout->setVerticalSpacing(20);
	eqLayout->setColumnMinimumWidth(1, 400);

	for (int row = 0; row < widMap.size(); row++) {

		eqLayout->addWidget(widMap[row].labelPtr, row, 0);

		eqLayout->addWidget(widMap[row].fieldPtr, row, 1);
		widMap[row].fieldPtr->setEnabled(widMap[row].editable);
	}

	eqLayout->addWidget(new QLabel(), widMap.size(), 0);
	eqLayout->setRowStretch(widMap.size(), 100);

	return wid;
}

// ----------------------------- Button Handlers  ------------------------------------

void
EquipmentWindow::saveButtonClicked() {

	if (selectedNode_) {

		switch (selectedNode_->getEqNodeType()) {

		case eqNodeType::EQ_LINK: {
			saveEquipmentLink(static_cast<EquipmentLink*>(selectedNode_));
		} break;

		case eqNodeType::EQ_TIME_SPAN: {
			saveTimeSpan(static_cast<EquipTimeSpan*>(selectedNode_));
		} break;

		case eqNodeType::EQ_DIST_ITEM: {
			saveEquipmentDistanceItem(static_cast<EquipmentDistanceItem*>(selectedNode_));
		} break;

		case eqNodeType::EQ_TIME_ITEM: {
			saveEquipmentTimeItem(static_cast<EquipmentTimeItem*>(selectedNode_));
		} break;

		// all other equipment types are read-only
		default: {} break;
		}
	}
}

void
EquipmentWindow::undoButtonClicked() {

	updateEquipmentDisplay();
}

void
EquipmentWindow::configChanged(qint32)
{
	// use the same colour scheme as RideMetadata

	if (context_) { // global doesn't have all the widgets etc
		setProperty("color", GColor(CPLOTBACKGROUND));

		palette_ = QPalette();

		palette_.setColor(QPalette::Window, GColor(CPLOTBACKGROUND));

		// only change base if moved away from white plots
		// which is a Mac thing
#ifndef Q_OS_MAC
		if (GColor(CPLOTBACKGROUND) != Qt::white)
#endif
		{
			palette_.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
			palette_.setColor(QPalette::Window, GColor(CPLOTBACKGROUND));
		}

		palette_.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
		palette_.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
		setPalette(palette_);
		stackedWidget_->setPalette(palette_);

		// use default font
		setFont(QFont());

		// update existing widgets on each of the stackedWidget_ pages
		QMapIterator< eqWinType, QVector<widgetMapType>> widItr (equipWids_);
		while (widItr.hasNext()) {
			widItr.next();

			for (int row = 0; row < widItr.value().size(); row++) {

				widItr.value()[row].labelPtr->setPalette(palette_);
				widItr.value()[row].fieldPtr->setPalette(palette_);
			}
		}
		lastUpdated_->setPalette(palette_);
	}
}

