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
    stackedWidget_->addWidget(createWidsEquipmentSharedDistanceItem());
    stackedWidget_->addWidget(createWidsEquipmentTimeItem());
    stackedWidget_->addWidget(createWidsReference());
    stackedWidget_->addWidget(createWidsEquipmentLink());
    stackedWidget_->addWidget(createWidsSharedEquipment());
    stackedWidget_->addWidget(createWidsEquipmentBanner());

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

    QHBoxLayout* hReclacLayout = new QHBoxLayout();
    recalcButton_ = new QPushButton(tr("Recalculate"));
    hReclacLayout->addWidget(recalcButton_);
    hReclacLayout->insertStretch(-1);

    QVBoxLayout* vlayout = new QVBoxLayout();
    lastUpdated_ = new QLabel();
    vlayout->addLayout(hReclacLayout);
    vlayout->addWidget(lastUpdated_);
    vlayout->addWidget(new QLabel());
    vlayout->addLayout(hlayout);
    vlayout->insertStretch(-1);

    setChartLayout(vlayout);

    // Listen for internal events
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(saveButton_, SIGNAL(clicked()), this, SLOT(saveButtonClicked()));
    connect(undoButton_, SIGNAL(clicked()), this, SLOT(undoButtonClicked()));
    connect(recalcButton_, SIGNAL(clicked(void)), context_, SLOT(notifyEqRecalculationStart()));

    // Listen for signals
    connect(context, SIGNAL(equipmentSelected(EquipmentNode*)), this, SLOT(equipmentSelected(EquipmentNode*)));
    connect(context, SIGNAL(eqRecalculationStart()), this, SLOT(eqRecalculationStart()));
    connect(context, SIGNAL(eqRecalculationEnd()), this, SLOT(eqRecalculationEnd()));
    connect(context, SIGNAL(updateEqSettings(bool)), this, SLOT(updateEqSettings(bool)));

    updateEquipmentDisplay();

    // set colors
    configChanged(CONFIG_APPEARANCE);
}

void
EquipmentWindow::equipmentSelected(EquipmentNode* eqNode)
{
    selectedNode_ = eqNode;
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
    bool hideRecalcButton = false;

    if (selectedNode_) {

        // find the root equipment
        EquipmentNode* eqRoot = selectedNode_;
        while (eqRoot->getEqNodeType() != eqNodeType::EQ_ROOT) eqRoot = eqRoot->getParentItem();

        metricUnits_ = static_cast<EquipmentRoot*>(eqRoot)->getMetricUnits();

        QDateTime lastRecalc = static_cast<EquipmentRoot*>(eqRoot)->getLastRecalc();
        static QString time_format = "HH:mm   dd MMM yyyy";
        QVariant days(abs(QDateTime::currentDateTime().daysTo(lastRecalc)));
        lastUpdated_->setText(tr("Last Recalculation:  ") +
            lastRecalc.toString(time_format) +
            ((days==0) ? tr("  <today>") : ((days==1) ? tr("  <yesterday>") : "  <" + days.toString() + tr(" days ago>"))));

        switch (selectedNode_->getEqNodeType()) {

        case eqNodeType::EQ_LINK: {
            displayEquipmentLink(static_cast<EquipmentLink*>(selectedNode_));
        } break;

        case eqNodeType::EQ_ITEM_REF: {
            displayReference(static_cast<EquipmentRef*>(selectedNode_));
        } break;

        case eqNodeType::EQ_DIST_ITEM: {
            displayEquipmentDistanceItem(static_cast<EquipmentDistanceItem*>(selectedNode_));
        } break;

        case eqNodeType::EQ_TIME_ITEM: {
            displayEquipmentTimeItem(static_cast<EquipmentTimeItem*>(selectedNode_));
            hideRecalcButton = true;
        } break;

        case eqNodeType::EQ_SHARED: {
            displaySharedEquipment(static_cast<SharedEquipment*>(selectedNode_));
            hideRecalcButton = hideButtons = true;
        } break;

        case eqNodeType::EQ_SHARED_DIST_ITEM: {
            displayEquipmentSharedDistanceItem(static_cast<EquipmentSharedDistanceItem*>(selectedNode_));
        } break;

        case eqNodeType::EQ_BANNER: {
            displayEquipmentBanner(static_cast<EquipmentBanner*>(selectedNode_));
            hideRecalcButton = true;
        } break;
        }
    }
    else {
        stackedWidget_->setCurrentIndex(int(eqWinType::BLANK_PAGE));
        hideRecalcButton = hideButtons = true;
    }

    recalcButton_->setHidden(hideRecalcButton || savingDisabled_);
    saveButton_->setHidden(hideButtons || savingDisabled_);
    undoButton_->setHidden(hideButtons || savingDisabled_);
}

// ------------------------------- Equipment Distance Item --------------------------------------

QWidget*
EquipmentWindow::createWidsEquipmentDistanceItem() {

    QVector< widgetMapType> pageWids;

    pageWids.push_back( widgetMapType { new QLabel(), new QLabel(tr("Distance Equipment")), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Description")), new QLineEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Non GC Distance")), new QLineEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("GC Distance")), new QLineEdit(), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Total Distance")), new QLineEdit(), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Replacement Distance")), new QLineEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(""), new QCheckBox(tr("Start Date")), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Start Date")), new QDateTimeEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(""), new QCheckBox(tr("End Date")), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("End Date")), new QDateTimeEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Distance\nNotes")), new QPlainTextEdit(), true } );

    connect(static_cast<QCheckBox*>(pageWids[6].fieldPtr), SIGNAL(stateChanged(int)), this, SLOT(distStartDateTimeStateChanged(int)));
    static_cast<QDateTimeEdit*>(pageWids[7].fieldPtr)->setCalendarPopup(true);
    connect(static_cast<QCheckBox*>(pageWids[8].fieldPtr), SIGNAL(stateChanged(int)), this, SLOT(distEndDateTimeStateChanged(int)));
    static_cast<QDateTimeEdit*>(pageWids[9].fieldPtr)->setCalendarPopup(true);

    equipWids_[eqWinType::EQUIPMENT_DIST_PAGE] = pageWids;

    distStartDateTimeStateChanged(Qt::Unchecked);
    distEndDateTimeStateChanged(Qt::Unchecked);

    return setupWids(pageWids);
}

void
EquipmentWindow::distStartDateTimeStateChanged(int checkState) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_DIST_PAGE];

    static_cast<QLabel*>(pageWids[7].labelPtr)->setHidden(checkState == Qt::Unchecked);
    static_cast<QDateTimeEdit*>(pageWids[7].fieldPtr)->setHidden(checkState == Qt::Unchecked);
}

void
EquipmentWindow::distEndDateTimeStateChanged(int checkState) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_DIST_PAGE];

    static_cast<QLabel*>(pageWids[9].labelPtr)->setHidden(checkState == Qt::Unchecked);
    static_cast<QDateTimeEdit*>(pageWids[9].fieldPtr)->setHidden(checkState == Qt::Unchecked);
}

QString
EquipmentWindow::unitString()
{
    return (metricUnits_) ? tr("\n(km)") : tr("\n(miles)");
}

void
EquipmentWindow::displayEquipmentDistanceItem(EquipmentDistanceItem* eqNode) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_DIST_PAGE];

    static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(eqNode->description_);
    static_cast<QLabel*>(pageWids[2].labelPtr)->setText(tr("Non GC Distance") + unitString());
    static_cast<QLineEdit*>(pageWids[2].fieldPtr)->setText(QString::number(eqNode->getNonGCDistance(), 'f', 1));
    static_cast<QLabel*>(pageWids[3].labelPtr)->setText(tr("GC Distance") + unitString());
    static_cast<QLineEdit*>(pageWids[3].fieldPtr)->setText(QString::number(eqNode->getGCDistance(), 'f', 1));
    static_cast<QLabel*>(pageWids[4].labelPtr)->setText(tr("Total Distance") + unitString());
    static_cast<QLineEdit*>(pageWids[4].fieldPtr)->setText(QString::number(eqNode->getTotalDistance(), 'f', 1));
    static_cast<QLabel*>(pageWids[5].labelPtr)->setText(tr("Replacement Distance") + unitString());
    static_cast<QLineEdit*>(pageWids[5].fieldPtr)->setText(QString::number(eqNode->replacementDistance_, 'f', 1));

    Qt::CheckState state = (eqNode->startNotSet()) ? Qt::Unchecked : Qt::Checked;
    static_cast<QCheckBox*>(pageWids[6].fieldPtr)->setCheckState(state);

    // Setting null QDateTime doesn't reset the QDateTimeEdit field
    if (eqNode->start_.isNull())
        static_cast<QDateTimeEdit*>(pageWids[7].fieldPtr)->setDateTime(QDateTime(QDate::currentDate().startOfDay()));
    else
        static_cast<QDateTimeEdit*>(pageWids[7].fieldPtr)->setDateTime(eqNode->start_);

    state = (eqNode->endNotSet()) ? Qt::Unchecked : Qt::Checked;
    static_cast<QCheckBox*>(pageWids[8].fieldPtr)->setCheckState(state);

    // Setting null QDateTime doesn't reset the QDateTimeEdit field
    if (eqNode->end_.isNull())
        static_cast<QDateTimeEdit*>(pageWids[9].fieldPtr)->setDateTime(QDateTime(QDate::currentDate().endOfDay()));
    else
        static_cast<QDateTimeEdit*>(pageWids[9].fieldPtr)->setDateTime(eqNode->end_);

    static_cast<QPlainTextEdit*>(pageWids[10].fieldPtr)->setPlainText(eqNode->notes_);

    stackedWidget_->setCurrentIndex(int(eqWinType::EQUIPMENT_DIST_PAGE));
}

void
EquipmentWindow::saveEquipmentDistanceItem(EquipmentDistanceItem* eqNode) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_DIST_PAGE];

    eqNode->description_ = static_cast<QLineEdit*>(pageWids[1].fieldPtr)->text();
    eqNode->setNonGCDistance(QVariant(static_cast<QLineEdit*>(pageWids[2].fieldPtr)->text()).toDouble());
    eqNode->replacementDistance_ = QVariant(static_cast<QLineEdit*>(pageWids[5].fieldPtr)->text()).toDouble();

    if (static_cast<QCheckBox*>(pageWids[6].fieldPtr)->checkState() == Qt::Unchecked) {
        eqNode->start_ = QDateTime();
    }
    else {
        eqNode->start_ = static_cast<QDateTimeEdit*>(pageWids[7].fieldPtr)->dateTime();
    }

    if (static_cast<QCheckBox*>(pageWids[8].fieldPtr)->checkState() == Qt::Unchecked) {
        eqNode->end_ = QDateTime();
    }
    else {
        eqNode->end_ = static_cast<QDateTimeEdit*>(pageWids[9].fieldPtr)->dateTime();
    }

    eqNode->notes_ = static_cast<QPlainTextEdit*>(pageWids[10].fieldPtr)->toPlainText();

    // need to ensure new total distance is correctly displayed
    updateEquipmentDisplay();
}

// ------------------------------- Equipment Shared Distance Item --------------------------------------

QWidget*
EquipmentWindow::createWidsEquipmentSharedDistanceItem() {

    QVector< widgetMapType> pageWids;

    pageWids.push_back( widgetMapType { new QLabel(), new QLabel(tr("Shared Distance Equipment")), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Description")), new QLineEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Non GC Distance")), new QLineEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("GC Distance")), new QLineEdit(), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Total Distance")), new QLineEdit(), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Replacement Distance")), new QLineEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Distance\nNotes")), new QPlainTextEdit(), true } );

    equipWids_[eqWinType::EQUIPMENT_SHARED_DIST_PAGE] = pageWids;

    return setupWids(pageWids);
}

void
EquipmentWindow::displayEquipmentSharedDistanceItem(EquipmentSharedDistanceItem* eqNode) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_SHARED_DIST_PAGE];

    static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(eqNode->description_);
    static_cast<QLabel*>(pageWids[2].labelPtr)->setText(tr("Non GC Distance") + unitString());
    static_cast<QLineEdit*>(pageWids[2].fieldPtr)->setText(QString::number(eqNode->getNonGCDistance(), 'f', 1));
    static_cast<QLabel*>(pageWids[3].labelPtr)->setText(tr("GC Distance") + unitString());
    static_cast<QLineEdit*>(pageWids[3].fieldPtr)->setText(QString::number(eqNode->getGCDistance(), 'f', 1));
    static_cast<QLabel*>(pageWids[4].labelPtr)->setText(tr("Total Distance") + unitString());
    static_cast<QLineEdit*>(pageWids[4].fieldPtr)->setText(QString::number(eqNode->getTotalDistance(), 'f', 1));
    static_cast<QLabel*>(pageWids[5].labelPtr)->setText(tr("Replacement Distance") + unitString());
    static_cast<QLineEdit*>(pageWids[5].fieldPtr)->setText(QString::number(eqNode->replacementDistance_, 'f', 1));
    static_cast<QPlainTextEdit*>(pageWids[6].fieldPtr)->setPlainText(eqNode->notes_);

    stackedWidget_->setCurrentIndex(int(eqWinType::EQUIPMENT_SHARED_DIST_PAGE));
}

void
EquipmentWindow::saveEquipmentSharedDistanceItem(EquipmentSharedDistanceItem* eqNode) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_SHARED_DIST_PAGE];

    eqNode->description_ = static_cast<QLineEdit*>(pageWids[1].fieldPtr)->text();
    eqNode->setNonGCDistance(QVariant(static_cast<QLineEdit*>(pageWids[2].fieldPtr)->text()).toDouble());
    eqNode->replacementDistance_ = QVariant(static_cast<QLineEdit*>(pageWids[5].fieldPtr)->text()).toDouble();
    eqNode->notes_ = static_cast<QPlainTextEdit*>(pageWids[6].fieldPtr)->toPlainText();

    // need to ensure new total distance is correctly displayed
    updateEquipmentDisplay();
}

// ------------------------------- Equipment Time Item --------------------------------------

QWidget*
EquipmentWindow::createWidsEquipmentTimeItem() {

    QVector< widgetMapType> pageWids;

    pageWids.push_back( widgetMapType { new QLabel(), new QLabel(tr("Time Equipment")), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Description")), new QLineEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Start Date")), new QDateTimeEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Replacement Date")), new QDateTimeEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Time\nNotes")), new QPlainTextEdit(), true } );

    static_cast<QDateTimeEdit*>(pageWids[2].fieldPtr)->setCalendarPopup(true);
    static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->setCalendarPopup(true);

    equipWids_[eqWinType::EQUIPMENT_TIME_PAGE] = pageWids;

    return setupWids(pageWids);
}

void
EquipmentWindow::displayEquipmentTimeItem(EquipmentTimeItem* eqNode) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_TIME_PAGE];

    static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(eqNode->description_);

    // Setting null QDateTime doesn't reset the QDateTimeEdit field
    if (eqNode->start_.isNull()) {
        static_cast<QDateTimeEdit*>(pageWids[2].fieldPtr)->setDateTime(QDateTime(QDate::currentDate().startOfDay()));
    }
    else {
        static_cast<QDateTimeEdit*>(pageWids[2].fieldPtr)->setDateTime(eqNode->start_);
    }

    if (eqNode->replace_.isNull()) {
        static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->setDateTime(QDateTime(QDate::currentDate().endOfDay()));
    }
    else {
        static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->setDateTime(eqNode->replace_);
    }

    static_cast<QPlainTextEdit*>(pageWids[4].fieldPtr)->setPlainText(eqNode->notes_);

    stackedWidget_->setCurrentIndex(int(eqWinType::EQUIPMENT_TIME_PAGE));
}

void
EquipmentWindow::saveEquipmentTimeItem(EquipmentTimeItem* eqNode) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::EQUIPMENT_TIME_PAGE];

    eqNode->description_ = static_cast<QLineEdit*>(pageWids[1].fieldPtr)->text();
    eqNode->start_ = static_cast<QDateTimeEdit*>(pageWids[2].fieldPtr)->dateTime();
    eqNode->replace_ = static_cast<QDateTimeEdit*>(pageWids[3].fieldPtr)->dateTime();
    eqNode->notes_ = static_cast<QPlainTextEdit*>(pageWids[4].fieldPtr)->toPlainText();
}

// ----------------------------- Equipment Reference  ------------------------------------

QWidget*
EquipmentWindow::createWidsReference() {

    QVector< widgetMapType> pageWids;

    pageWids.push_back( widgetMapType { new QLabel(), new QLabel(tr("Distance Equipment Reference")), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Description")), new QLineEdit(), false    } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Reference Distance")), new QLineEdit(), false } );
    pageWids.push_back( widgetMapType { new QLabel(""), new QCheckBox(tr("Start Date")), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Start Date")), new QDateTimeEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(""), new QCheckBox(tr("End Date")), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("End Date")), new QDateTimeEdit(), true } );

    connect(static_cast<QCheckBox*>(pageWids[3].fieldPtr), SIGNAL(stateChanged(int)), this, SLOT(refsStartDateTimeStateChanged(int)));
    static_cast<QDateTimeEdit*>(pageWids[4].fieldPtr)->setCalendarPopup(true);
    connect(static_cast<QCheckBox*>(pageWids[5].fieldPtr), SIGNAL(stateChanged(int)), this, SLOT(refsEndDateTimeStateChanged(int)));
    static_cast<QDateTimeEdit*>(pageWids[6].fieldPtr)->setCalendarPopup(true);

    equipWids_[eqWinType::REFERENCE_PAGE] = pageWids;

    refsStartDateTimeStateChanged(Qt::Unchecked);
    refsEndDateTimeStateChanged(Qt::Unchecked);

    return setupWids(pageWids);
}

void
EquipmentWindow::refsStartDateTimeStateChanged(int checkState) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::REFERENCE_PAGE];

    static_cast<QLabel*>(pageWids[4].labelPtr)->setHidden(checkState == Qt::Unchecked);
    static_cast<QDateTimeEdit*>(pageWids[4].fieldPtr)->setHidden(checkState == Qt::Unchecked);
}

void
EquipmentWindow::refsEndDateTimeStateChanged(int checkState) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::REFERENCE_PAGE];

    static_cast<QLabel*>(pageWids[6].labelPtr)->setHidden(checkState == Qt::Unchecked);
    static_cast<QDateTimeEdit*>(pageWids[6].fieldPtr)->setHidden(checkState == Qt::Unchecked);
}

void
EquipmentWindow::displayReference(EquipmentRef* eqRef) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::REFERENCE_PAGE];

    static_cast<QLabel*>(pageWids[2].labelPtr)->setText(tr("Reference Distance") + unitString());

    EquipmentSharedDistanceItem* eqNode = eqRef->eqSharedDistNode_;

    if (eqNode) {

        static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(eqNode->description_);
        static_cast<QLineEdit*>(pageWids[2].fieldPtr)->setText(QString::number(eqRef->getRefDistanceCovered(), 'f', 1));
    }
    else {
        static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(tr("Referenced Equipment Missing!"));
    }

    Qt::CheckState state = (eqRef->startNotSet()) ? Qt::Unchecked : Qt::Checked;
    static_cast<QCheckBox*>(pageWids[3].fieldPtr)->setCheckState(state);

    // Setting null QDateTime doesn't reset the QDateTimeEdit field
    if (eqRef->start_.isNull())
        static_cast<QDateTimeEdit*>(pageWids[4].fieldPtr)->setDateTime(QDateTime(QDate::currentDate().startOfDay()));
    else
        static_cast<QDateTimeEdit*>(pageWids[4].fieldPtr)->setDateTime(eqRef->start_);

    state = (eqRef->endNotSet()) ? Qt::Unchecked : Qt::Checked;
    static_cast<QCheckBox*>(pageWids[5].fieldPtr)->setCheckState(state);

    // Setting null QDateTime doesn't reset the QDateTimeEdit field
    if (eqRef->end_.isNull())
        static_cast<QDateTimeEdit*>(pageWids[6].fieldPtr)->setDateTime(QDateTime(QDate::currentDate().endOfDay()));
    else
        static_cast<QDateTimeEdit*>(pageWids[6].fieldPtr)->setDateTime(eqRef->end_);

    stackedWidget_->setCurrentIndex(int(eqWinType::REFERENCE_PAGE));
}

void
EquipmentWindow::saveReference(EquipmentRef* eqRef) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::REFERENCE_PAGE];

    if (static_cast<QCheckBox*>(pageWids[3].fieldPtr)->checkState() == Qt::Unchecked) {
        eqRef->start_ = QDateTime();
    }
    else {
        eqRef->start_ = static_cast<QDateTimeEdit*>(pageWids[4].fieldPtr)->dateTime();
    }

    if (static_cast<QCheckBox*>(pageWids[5].fieldPtr)->checkState() == Qt::Unchecked) {
        eqRef->end_ = QDateTime();
    }
    else {
        eqRef->end_ = static_cast<QDateTimeEdit*>(pageWids[6].fieldPtr)->dateTime();
    }
}

// ----------------------------- Activity Link  ------------------------------------

QWidget*
EquipmentWindow::createWidsEquipmentLink() {

    QVector< widgetMapType> pageWids;

    pageWids.push_back( widgetMapType { new QLabel(), new QLabel(tr("Equipment Link (to Activities)")), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Equipment Link")), new QLineEdit(), true } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Activities")), new QLineEdit(), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("GC Distance")), new QLineEdit(), false } );

    equipWids_[eqWinType::ACTIVITY_LINK_PAGE] = pageWids;

    return setupWids(pageWids);
}

void
EquipmentWindow::displayEquipmentLink(EquipmentLink* eqLink) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::ACTIVITY_LINK_PAGE];

    static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(eqLink->getEqLinkName());
    static_cast<QLineEdit*>(pageWids[2].fieldPtr)->setText(QString::number(eqLink->getActivities()));
    static_cast<QLabel*>(pageWids[3].labelPtr)->setText(tr("GC Distance") + unitString());
    static_cast<QLineEdit*>(pageWids[3].fieldPtr)->setText(QString::number(eqLink->getGCDistance(), 'f', 1));

    stackedWidget_->setCurrentIndex(int(eqWinType::ACTIVITY_LINK_PAGE));
}

void
EquipmentWindow::saveEquipmentLink(EquipmentLink* eqLink) {


    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::ACTIVITY_LINK_PAGE];
    
    QList<FieldDefinition> fieldDefinitions = GlobalContext::context()->rideMetadata->getFields();

    // If the equipment Link name has changed, then if its
    // unique to this equipment link remove it from the choice list
    if (eqLink->getEqLinkName() != static_cast<QLineEdit*>(pageWids[1].fieldPtr)->text()) {

        // find my root parent, this must be the reference tree as this is an equipment link
        EquipmentNode* eqRoot = eqLink->getParentItem();

        bool uniqueEqLinkName = true;

        // now check that other equipment links do not have the same name as the one being replaced
        for (EquipmentNode* rootChild : eqRoot->getChildren()) {
            // don't check myself!
            if ((rootChild->getEqNodeType() == eqNodeType::EQ_LINK) && (rootChild != eqLink)) {
                uniqueEqLinkName &= (static_cast<EquipmentLink*>(rootChild)->getEqLinkName() != eqLink->getEqLinkName());
            }
        }

        // if the old name is unique (and not used elsewhere) then remove it from the choice list
        if (uniqueEqLinkName) {
            for (FieldDefinition& field : fieldDefinitions) {
                if (field.name == "EquipmentLink") {
                    for (int i = 0; i < field.values.size(); ++i) {
                        if (field.values.at(i).contains(eqLink->getEqLinkName()))
                            field.values.removeAt(i);
                    }
                    break;
                }
            }
        }

        // now update the model with the new equipment link name
        eqLink->setEqLinkName(static_cast<QLineEdit*>(pageWids[1].fieldPtr)->text());
    }

    // update the choice list with the new equipment link name, in case its not already in the choice list
    for (FieldDefinition& field : fieldDefinitions) {
        if (field.name == "EquipmentLink") {

            // if the equipment link name is missing t
            if (!field.values.contains(eqLink->getEqLinkName())) {

                // add missing equipment link to the choice list
                field.values.append(eqLink->getEqLinkName());

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

// ----------------------------- Shared Equipment  ------------------------------------

QWidget*
EquipmentWindow::createWidsSharedEquipment() {

    QVector< widgetMapType> pageWids;

    pageWids.push_back( widgetMapType { new QLabel(), new QLabel(tr("Shared Equipment")), false } );

    equipWids_[eqWinType::SHARED_PAGE] = pageWids;

    return setupWids(pageWids);
}

void
EquipmentWindow::displaySharedEquipment(SharedEquipment* /* eqNode */) {

    stackedWidget_->setCurrentIndex(int(eqWinType::SHARED_PAGE));
}


// ----------------------------- Equipment Banner  ------------------------------------

QWidget*
EquipmentWindow::createWidsEquipmentBanner() {

    QVector< widgetMapType> pageWids;

    pageWids.push_back( widgetMapType { new QLabel(), new QLabel(tr("Equipment Banner")), false } );
    pageWids.push_back( widgetMapType { new QLabel(tr("Description")), new QLineEdit(), true } );
    
    equipWids_[eqWinType::BANNER_PAGE] = pageWids;

    return setupWids(pageWids);
}

void
EquipmentWindow::displayEquipmentBanner(EquipmentBanner* eqBanner) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::BANNER_PAGE];

    static_cast<QLineEdit*>(pageWids[1].fieldPtr)->setText(eqBanner->description_);
    
    stackedWidget_->setCurrentIndex(int(eqWinType::BANNER_PAGE));
}

void
EquipmentWindow::saveEquipmentBanner(EquipmentBanner* eqBanner) {

    QVector< widgetMapType>& pageWids = equipWids_[eqWinType::BANNER_PAGE];

    eqBanner->description_ = static_cast<QLineEdit*>(pageWids[1].fieldPtr)->text();
}


// ----------------------------- Button Handlers  ------------------------------------

void
EquipmentWindow::saveButtonClicked() {

    if (selectedNode_) {

        switch (selectedNode_->getEqNodeType()) {

        case eqNodeType::EQ_LINK: {
            saveEquipmentLink(static_cast<EquipmentLink*>(selectedNode_));
        } break;

        case eqNodeType::EQ_DIST_ITEM: {
            saveEquipmentDistanceItem(static_cast<EquipmentDistanceItem*>(selectedNode_));
        } break;

        case eqNodeType::EQ_TIME_ITEM: {
            saveEquipmentTimeItem(static_cast<EquipmentTimeItem*>(selectedNode_));
        } break;

        case eqNodeType::EQ_ITEM_REF: {
            saveReference(static_cast<EquipmentRef*>(selectedNode_));
        } break;

        case eqNodeType::EQ_SHARED_DIST_ITEM: {
            saveEquipmentSharedDistanceItem(static_cast<EquipmentSharedDistanceItem*>(selectedNode_));
        } break;

        case eqNodeType::EQ_BANNER: {
            saveEquipmentBanner(static_cast<EquipmentBanner*>(selectedNode_));
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

        // only change base if moved away from white plots which is a Mac thing
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

