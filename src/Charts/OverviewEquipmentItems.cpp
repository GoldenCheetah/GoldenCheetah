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


#include "ColorButton.h"
#include "OverviewEquipmentItems.h"

bool
OverviewEquipmentItemConfig::registerItems()
{
    // get the factory
    ChartSpaceItemRegistry &registry = ChartSpaceItemRegistry::instance();

    // Register      TYPE                           SHORT                           DESCRIPTION                         SCOPE                        CREATOR
    registry.addItem(OverviewItemType::EQ_ITEM,     QObject::tr("Equipment"),       QObject::tr("Equipment Item"),      OverviewScope::EQUIPMENT,    EquipmentItem::create);
    registry.addItem(OverviewItemType::EQ_SUMMARY,  QObject::tr("Eq Link Summary"), QObject::tr("Equipment Summary"),   OverviewScope::EQUIPMENT,    EquipmentSummary::create);
    registry.addItem(OverviewItemType::EQ_NOTES,    QObject::tr("Eq Link Notes"),   QObject::tr("Equipment Notes"),     OverviewScope::EQUIPMENT,    EquipmentNotes::create);

    return true;
}

//
// ------------------------------------------ EqTimeWindow --------------------------------------------
//

EqTimeWindow::EqTimeWindow()
{
    startSet_ = true;
    startDate_ = QDate(1900, 01, 01);
    endSet_ = true;
    endDate_ = QDate(2024, 01, 12);
}

EqTimeWindow::EqTimeWindow(const QString& eqLinkName, bool startSet, const QDate& startDate, bool endSet, const QDate& endDate) :
    eqLinkName_(eqLinkName), startSet_(startSet), startDate_(startDate), endSet_(endSet), endDate_(endDate)
{
}

bool
EqTimeWindow::isWithin(const QDate& actDate) const
{
    if (!startSet_)
        if (!endSet_)
            return true; // no range set
        else
            return (actDate <= endDate_); // end set but not beginning !
    else
        if (!endSet_)
            return (startDate_ <= actDate);
        else
            return (startDate_ <= actDate) && (actDate <= endDate_);
}

bool
EqTimeWindow::rangeIsValid() const
{
    return (startSet_ && endSet_) ? endDate_ >= startDate_ : true;
}

//
// ------------------------------------ OverviewEquipmentItemConfig --------------------------------------
//

OverviewEquipmentItemConfig::OverviewEquipmentItemConfig(ChartSpaceItem* item) :
    OverviewItemConfig(item)
{
    // Insert the fields between the default title & background colour button
    int insertRow = 1;

    if (item->type == OverviewItemType::EQ_ITEM) {

        // prevent negative values
        QDoubleValidator* eqValidator = new QDoubleValidator();
        eqValidator->setBottom(0);
        eqValidator->setDecimals(1);
        eqValidator->setNotation(QDoubleValidator::StandardNotation);

        nonGCDistance = new QLineEdit();
        nonGCDistance->setValidator(eqValidator);
        connect(nonGCDistance, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("Manual dst"), nonGCDistance);

        nonGCElevation = new QLineEdit();
        nonGCElevation->setValidator(eqValidator);
        connect(nonGCElevation, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("Manual elev"), nonGCElevation);

        eqTimeWindows = new QTableWidget(0, 5);
        QStringList headers = (QStringList() << "EqLink Name" << "Start set" << "Start Date"<< "End set" << "End Date");
        eqTimeWindows->setHorizontalHeaderLabels(headers);
        eqTimeWindows->setMinimumWidth(430 * dpiXFactor);
        eqTimeWindows->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        eqTimeWindows->setSelectionBehavior(QAbstractItemView::SelectRows);
        eqTimeWindows->setSelectionMode(QAbstractItemView::SingleSelection);
        QPalette palette = eqTimeWindows->palette();
        palette.setBrush(QPalette::Highlight, palette.brush(QPalette::Base));
        palette.setBrush(QPalette::HighlightedText, palette.brush(QPalette::Text));
        eqTimeWindows->setPalette(palette);
        eqTimeWindows->verticalHeader()->setVisible(false);
        connect(eqTimeWindows, SIGNAL(cellClicked(int, int)), this, SLOT(tableCellClicked(int, int)));
        layout->insertRow(insertRow++, "EqLink History", eqTimeWindows);

        QHBoxLayout* buttonRow = new QHBoxLayout();
        addEqLink = new QPushButton("Add EqLink");
        removeEqLink = new QPushButton("Remove EqLink");
        buttonRow->addWidget(addEqLink);
        connect(addEqLink, SIGNAL(clicked(bool)), this, SLOT(addEqLinkRow()));
        buttonRow->addWidget(removeEqLink);
        connect(removeEqLink, SIGNAL(clicked(bool)), this, SLOT(removeEqLinkRow()));
        layout->insertRow(insertRow++, "", buttonRow);

        replaceDistance = new QLineEdit();
        replaceDistance->setValidator(eqValidator);
        connect(replaceDistance, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("Replacement dst"), replaceDistance);

        replaceElevation = new QLineEdit();
        replaceElevation->setValidator(eqValidator);
        connect(replaceElevation, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("Replacement elev"), replaceElevation);

        notes = new QPlainTextEdit();
        connect(notes, SIGNAL(textChanged()), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("Notes"), notes);
    }

    if (item->type == OverviewItemType::EQ_SUMMARY) {

        eqLinkName = new QLineEdit();
        connect(eqLinkName, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("EqLink Name"), eqLinkName);

        startSet = new QCheckBox();
        connect(startSet, SIGNAL(stateChanged(int)), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("Show Athlete's Activities"), startSet);
    }

    if (item->type == OverviewItemType::EQ_NOTES) {

        notes = new QPlainTextEdit();
        connect(notes, SIGNAL(textChanged()), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("Notes"), notes);
    }

    setWidgets();
}

void
OverviewEquipmentItemConfig::setWidgets()
{
    block = true;

    // set the widget values from the item
    switch (item->type) {

    case OverviewItemType::EQ_ITEM:
    {
        EquipmentItem* mi = dynamic_cast<EquipmentItem*>(item);
        name->setText(mi->name);
        nonGCDistance->setText(QString::number(mi->getNonGCDistanceScaled() * EQ_SCALED_TO_REAL, 'f', 1));
        nonGCElevation->setText(QString::number(mi->getNonGCElevationScaled() * EQ_SCALED_TO_REAL, 'f', 1));
        replaceDistance->setText(QString::number(mi->repDistanceScaled_ * EQ_SCALED_TO_REAL, 'f', 1));
        replaceElevation->setText(QString::number(mi->repElevationScaled_ * EQ_SCALED_TO_REAL, 'f', 1));

        // clear the table
        eqTimeWindows->setRowCount(0);

        for (int tableRow = 0; tableRow < mi->eqLinkUse_.size(); tableRow++) {

            eqTimeWindows->insertRow(tableRow);
            const EqTimeWindow& eqUse = mi->eqLinkUse_[tableRow];
            setEqLinkRowWidgets(tableRow, &eqUse);

            static_cast<QLineEdit*>(eqTimeWindows->cellWidget(tableRow, 0))->setText(eqUse.eqLinkName_);

            static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 1))->setText((eqUse.startSet_) ? "set" : "unset");
            if (eqUse.startSet_) {
                static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 2))->setDate(eqUse.startDate_);
            }

            static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 3))->setText((eqUse.endSet_) ? "set" : "unset");
            if (eqUse.endSet_) {
                static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 4))->setDate(eqUse.endDate_);
            }
            
        }
        notes->setPlainText(mi->notes_);
    }
    break;

    case OverviewItemType::EQ_SUMMARY:
    {
        EquipmentSummary* mi = dynamic_cast<EquipmentSummary*>(item);
        name->setText(mi->name);
        eqLinkName->setText(mi->eqLinkName_);
        startSet->setChecked(mi->showActivitiesPerAthlete_);

    }
    break;

    case OverviewItemType::EQ_NOTES:
    {
        EquipmentNotes* mi = dynamic_cast<EquipmentNotes*>(item);
        name->setText(mi->name);
        notes->setPlainText(mi->notes_);
    }
    break;
    }
    block = false;
}

void
OverviewEquipmentItemConfig::tableCellClicked(int row, int column)
{
    // only handle cell clicks on the checkboxs
    if (column == 1 || column == 3) {

        if (static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column))->text() == "set") {

            static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column))->setText("unset");

            eqTimeWindows->setCellWidget(row, column + 1, nullptr);
        }
        else
        {
            static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column))->setText("set");

            QDateEdit* date = new QDateEdit();
            date->setCalendarPopup(true);
            connect(date, SIGNAL(dateChanged(QDate)), this, SLOT(dataChanged()));
            eqTimeWindows->setCellWidget(row, column + 1, date);
        }
        dataChanged();
    }

    if (column == 2 || column == 4) {

        if (eqTimeWindows->cellWidget(row, column) == nullptr) {

            QDateEdit* date = new QDateEdit();
            date->setCalendarPopup(true);
            connect(date, SIGNAL(dateChanged(QDate)), this, SLOT(dataChanged()));
            eqTimeWindows->setCellWidget(row, column, date);

            static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column - 1))->setText("set");
        }
        else
        {
            // cellClicked signal is captured by the QDateEdit so never received here
        }
        dataChanged();
    }
}

void
OverviewEquipmentItemConfig::setEqLinkRowWidgets(int tableRow, const EqTimeWindow* eqUse)
{
    QLineEdit* eqLink = new QLineEdit();
    eqLink->setFrame(false);
    connect(eqLink, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
    eqTimeWindows->setCellWidget(tableRow, 0, eqLink);

    QLabel* startSet = new QLabel();
    startSet->setAlignment(Qt::AlignHCenter);
    startSet->setText((eqUse && eqUse->startSet_) ? "set" : "unset");
    eqTimeWindows->setCellWidget(tableRow, 1, startSet);

    if (eqUse && eqUse->startSet_) {
        QDateEdit* startDate = new QDateEdit();
        startDate->setCalendarPopup(true);
        connect(startDate, SIGNAL(dateChanged(QDate)), this, SLOT(dataChanged()));
        eqTimeWindows->setCellWidget(tableRow, 2, startDate);
    }

    QLabel* endSet = new QLabel();
    endSet->setAlignment(Qt::AlignHCenter);
    endSet->setText((eqUse && eqUse->endSet_) ? "set" : "unset");
    eqTimeWindows->setCellWidget(tableRow, 3, endSet);

    if (eqUse && eqUse->endSet_) {
        QDateEdit* endDate = new QDateEdit();
        endDate->setCalendarPopup(true);
        connect(endDate, SIGNAL(dateChanged(QDate)), this, SLOT(dataChanged()));
        eqTimeWindows->setCellWidget(tableRow, 4, endDate);
    }
}

void
OverviewEquipmentItemConfig::addEqLinkRow()
{
    block = true;

    int tableRow = eqTimeWindows->rowCount();
    eqTimeWindows->insertRow(tableRow);
    setEqLinkRowWidgets(tableRow, nullptr);

    block = false;
}

void
OverviewEquipmentItemConfig::removeEqLinkRow()
{
    eqTimeWindows->removeRow(eqTimeWindows->currentRow());
    dataChanged();
}

void
OverviewEquipmentItemConfig::dataChanged()
{
    // user edited or programmatically the data was changed
    // so lets update the item to reflect those changes
    // if they are valid. But block set when the widgets
    // are being initialised
    if (block) return;

    // set the widget values from the item
    switch (item->type) {

    case OverviewItemType::EQ_ITEM:
    {
        EquipmentItem* mi = dynamic_cast<EquipmentItem*>(item);
        mi->name = name->text();
        mi->setNonGCDistance(nonGCDistance->text().toDouble()); // validator restricts 1 dp
        mi->setNonGCElevation(nonGCElevation->text().toDouble()); // validator restricts 1 dp
       
        QVector<EqTimeWindow> eqLinkUse;
        for (int tableRow = 0; tableRow < eqTimeWindows->rowCount(); tableRow++) {

            QString eqLinkName = static_cast<QLineEdit*>(eqTimeWindows->cellWidget(tableRow, 0))->text().simplified().remove(' ');

            if (eqLinkName != "") { // don't accept time windows without any eqLinkName text
                QDate startDate;
                bool startSet = static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 1))->text() == "set";
                if (startSet) {
                    startDate = static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 2))->date();
                }

                QDate endDate;
                bool endSet = static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 3))->text() == "set";
                if (endSet) {
                    endDate = static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 4))->date();
                }

                eqLinkUse.push_back(EqTimeWindow(eqLinkName, startSet, startDate, endSet, endDate));
            }
        }
        mi->eqLinkUse_ = eqLinkUse;

        mi->repDistanceScaled_ = round(replaceDistance->text().toDouble() * EQ_REAL_TO_SCALED); // validator restricts 1 dp
        mi->repElevationScaled_ = round(replaceElevation->text().toDouble() * EQ_REAL_TO_SCALED); // validator restricts 1 dp
        mi->notes_ = notes->toPlainText();
        mi->bgcolor = bgcolor->getColor().name();
    }
    break;

    case OverviewItemType::EQ_SUMMARY:
    {
        EquipmentSummary* mi = dynamic_cast<EquipmentSummary*>(item);
        mi->name = name->text();
        mi->eqLinkName_ = eqLinkName->text();
        mi->showActivitiesPerAthlete_ = startSet->isChecked();
        mi->bgcolor = bgcolor->getColor().name();
    }
    break;

    case OverviewItemType::EQ_NOTES:
    {
        EquipmentNotes* mi = dynamic_cast<EquipmentNotes*>(item);
        mi->name = name->text();
        mi->notes_ = notes->toPlainText();
        mi->bgcolor = bgcolor->getColor().name();
    }
    break;
    }
}

//
// ------------------------------------------ EquipmentItem --------------------------------------------
//

EquipmentItem::EquipmentItem(ChartSpace* parent, const QString& name) : ChartSpaceItem(parent, name)
{
    // equipment item
    this->type = OverviewItemType::EQ_ITEM;

    activities_ = 0;
    activityTimeInSecs_ = 0;
    nonGCDistanceScaled_ = 0;
    gcDistanceScaled_ = 0;
    totalDistanceScaled_ = 0;
    nonGCElevationScaled_ = 0;
    gcElevationScaled_ = 0;
    totalElevationScaled_ = 0;
    repDistanceScaled_ = 0;
    repElevationScaled_ = 0;

    configwidget_ = new OverviewEquipmentItemConfig(this);
    configwidget_->hide();

    configChanged(0);
}

EquipmentItem::EquipmentItem(ChartSpace* parent, const QString& name, QVector<EqTimeWindow>& eqLinkUse,
                            const uint64_t nonGCDistanceScaled, const uint64_t nonGCElevationScaled,
                            const uint64_t repDistanceScaled, const uint64_t repElevationScaled,
                            const QString& notes) : EquipmentItem(parent, name)
{
    eqLinkUse_ = eqLinkUse;
    nonGCDistanceScaled_ = nonGCDistanceScaled;
    nonGCElevationScaled_ = nonGCElevationScaled;
    repDistanceScaled_ = repDistanceScaled;
    repElevationScaled_ = repElevationScaled;
    notes_ = notes;

    configwidget_->setWidgets();
}

EquipmentSummary::EquipmentSummary(ChartSpace* parent, const QString& name,
                                   const QString& eqLinkName, bool showActivitiesPerAthlete) :
    ChartSpaceItem(parent, name)
{
    // equipment summary item
    this->type = OverviewItemType::EQ_SUMMARY;
    eqLinkName_ = eqLinkName;
    showActivitiesPerAthlete_ = showActivitiesPerAthlete;
    eqLinkTotalTimeInSecs_ = 0;
    eqLinkTotalDistanceScaled_ = 0;
    eqLinkTotalElevationScaled_ = 0;
    eqLinkNumActivities_ = 0;

    configwidget_ = new OverviewEquipmentItemConfig(this);
    configwidget_->hide();

    configChanged(0);
}

EquipmentNotes::EquipmentNotes(ChartSpace* parent, const QString& name, const QString& notes) :
    ChartSpaceItem(parent, name)
{
    // equipment summary item
    this->type = OverviewItemType::EQ_NOTES;
    notes_ = notes;
  
    configwidget_ = new OverviewEquipmentItemConfig(this);
    configwidget_->hide();

    configChanged(0);
}

void
EquipmentItem::resetEqItem()
{
    activities_ = 0;
    activityTimeInSecs_ = 0;
    gcDistanceScaled_ = 0;
    totalDistanceScaled_ = nonGCDistanceScaled_;
    gcElevationScaled_ = 0;
    totalElevationScaled_ = nonGCElevationScaled_;
}

void
EquipmentItem::unitsChanged()
{
    // Need to rescale for the units change for the user entered data.
    nonGCDistanceScaled_ = (GlobalContext::context()->useMetricUnits) ? round(nonGCDistanceScaled_ * KM_PER_MILE) : round (nonGCDistanceScaled_ * MILES_PER_KM);
    repDistanceScaled_ = (GlobalContext::context()->useMetricUnits) ? round(repDistanceScaled_ * KM_PER_MILE) : round(repDistanceScaled_ * MILES_PER_KM);
    nonGCElevationScaled_ = (GlobalContext::context()->useMetricUnits) ? round(nonGCElevationScaled_ * METERS_PER_FOOT) : round(nonGCElevationScaled_ * FEET_PER_METER);
    repElevationScaled_ = (GlobalContext::context()->useMetricUnits) ? round (repElevationScaled_ * METERS_PER_FOOT) : round (repElevationScaled_ * FEET_PER_METER);
}

void
EquipmentItem::setNonGCDistance(double nonGCDistance)
{
    // using integral type atomics (c++11) but to retain accuracy multiply by EQ_REAL_TO_SCALED, see overviewItems.h
    nonGCDistanceScaled_ = round(nonGCDistance*EQ_REAL_TO_SCALED);
    totalDistanceScaled_ = gcElevationScaled_ + nonGCDistanceScaled_;
}

void
EquipmentItem::setNonGCElevation(double nonGCElevation)
{
    // using integral type atomics (c++11) but to retain accuracy multiply by EQ_REAL_TO_SCALED, see overviewItems.h
    nonGCElevationScaled_ = round(nonGCElevation*EQ_REAL_TO_SCALED);
    totalElevationScaled_ = gcElevationScaled_ + nonGCElevationScaled_;
}

void
EquipmentItem::addActivity(uint64_t rideDistanceScaled, uint64_t rideElevationScaled, uint64_t rideTimeInSecs)
{
    // Atomic safe additions
    activities_ += 1;
    activityTimeInSecs_ += rideTimeInSecs;
    gcDistanceScaled_ += rideDistanceScaled;
    totalDistanceScaled_ += rideDistanceScaled;
    gcElevationScaled_ += rideElevationScaled;
    totalElevationScaled_ += rideElevationScaled;
}

bool
EquipmentItem::isWithin(const QStringList& rideEqLinkNameList, const QDate& actDate) const
{
    for (const auto& eqUse : eqLinkUse_) {

        if (rideEqLinkNameList.contains(eqUse.eqLinkName_) && eqUse.isWithin(actDate)) return true;
    }
    return false;
}

bool
EquipmentItem::isWithin(const QString& rideEqLinkName, const QDate& actDate) const
{
    for (const auto& eqUse : eqLinkUse_) {

        if ((eqUse.eqLinkName_ == rideEqLinkName) && eqUse.isWithin(actDate)) return true;
    }
    return false;
}

bool
EquipmentItem::isWithin(const QDate& actDate) const
{
    for (const auto& eqUse : eqLinkUse_) {

        if (eqUse.isWithin(actDate)) return true;
    }
    return false;
}

bool
EquipmentItem::rangeIsValid() const
{
    bool rangeIsValid = true;
    for (const auto& eqUse : eqLinkUse_) {
        rangeIsValid &= (eqUse.startSet_ && eqUse.endSet_) ? eqUse.endDate_ >= eqUse.startDate_ : true;
    }
    return rangeIsValid;
}

void
EquipmentSummary::resetAthleteActivity()
{
    athleteActivityMap_.clear();
}

void
EquipmentSummary::updateSummaryItem(const uint64_t eqNumActivities, const uint64_t eqTotalTimeInSecs,
                                    const uint64_t eqTotalDistanceScaled, const uint64_t eqTotalElevationScaled,
                                    const QDate& earliestDate, const QDate& latestDate)
{
    eqLinkNumActivities_ = eqNumActivities;
    eqLinkTotalTimeInSecs_ = eqTotalTimeInSecs;
    eqLinkTotalDistanceScaled_ = eqTotalDistanceScaled;
    eqLinkTotalElevationScaled_ = eqTotalElevationScaled;
    eqLinkEarliestDate_ = earliestDate;
    eqLinkLatestDate_ = latestDate;
}

void
EquipmentSummary::addAthleteActivity(const QString& athleteName)
{
    athleteActivityMutex_.lock();
    athleteActivityMap_[athleteName] += 1;
    athleteActivityMutex_.unlock();
}


void
EquipmentItem::configChanged(qint32 cfg) {

    if ((cfg == 0) || (cfg & CONFIG_APPEARANCE)) {
        inactiveColor = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(100, 100, 100)) : QColor(QColor(170, 170, 170));
        textColor = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(200, 200, 200)) : QColor(QColor(70, 70, 70));
        alertColor = QColor(QColor(255, 170, 0));
    }
}

void
EquipmentItem::itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    
    // mid is slightly higher to account for space around title, move mid up
    static double mid = ROWHEIGHT * 3.0f;

    bool overDistance = (repDistanceScaled_ !=0) && (getTotalDistanceScaled() > repDistanceScaled_);
    bool overElevation = (repElevationScaled_ !=0) && (getTotalElevationScaled() > repElevationScaled_);

    // we align centre and mid
    QFontMetrics fm(parent->bigfont);
    QString totalDistanceStr(QString::number(getTotalDistanceScaled() * EQ_SCALED_TO_REAL, 'f', 1));
    QRectF rect = QFontMetrics(parent->bigfont, parent->device()).boundingRect(totalDistanceStr);
    painter->setFont(parent->bigfont);

    if ((!rangeIsValid()) || overDistance || overElevation) {
        painter->setPen(alertColor);
    } else if (isWithin(QDate::currentDate())) {
        painter->setPen(GColor(CPLOTMARKER));
    } else {
        painter->setPen(inactiveColor);
    }

    // display the total distance
    painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                        mid + (fm.ascent() / 3.0f)), totalDistanceStr); // divided by 3 to account for "gap" at top of font

    painter->setPen(QColor(100, 100, 100));
    painter->setFont(parent->smallfont);
    double addy = QFontMetrics(parent->smallfont).height();

    QString distUnits = GlobalContext::context()->useMetricUnits ? tr(" km") : tr(" miles");

    painter->drawText(QPointF((geometry().width() - QFontMetrics(parent->smallfont).horizontalAdvance(distUnits)) / 2.0f,
        mid + (fm.ascent() / 3.0f) + addy), distUnits); // divided by 3 to account for "gap" at top of font

    double rowY(ROWHEIGHT * 5.5);
    double rowWidth(geometry().width() - (ROWHEIGHT * 2));
    double rowHeight(geometry().height() - (ROWHEIGHT * 4));

    for (const auto& eqUse : eqLinkUse_) {

        // display the date in either active, inactive or out of range colours
        if (!eqUse.rangeIsValid()) {
            painter->setPen(alertColor);
        }
        else if (isWithin(eqUse.eqLinkName_, QDate::currentDate())) {
            painter->setPen(GColor(CPLOTMARKER));
        }
        else {
            painter->setPen(inactiveColor);
        }

        QString dateString(eqUse.eqLinkName_ + ": ");

        // Format date field
        if (!eqUse.startSet_) {
            if (!eqUse.endSet_) {
                dateString += tr("All Dates");
            }
            else {
                dateString += eqUse.endDate_.toString("->d MMM yy");
            }
        }
        else {
            if (!eqUse.endSet_) {
                dateString += eqUse.startDate_.toString("d MMM yy ->");
            }
            else {
                dateString += (eqUse.startDate_.toString("d MMM yy->") + eqUse.endDate_.toString("d MMM yy"));
            }
        }

        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), dateString);
        rowY += (ROWHEIGHT);
    }

    painter->setPen(textColor);

    rowY += (ROWHEIGHT * 0.3);
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth,rowHeight),
        QString(tr("Activities: ")) + QString::number(getNumActivities()));

    rowY += (ROWHEIGHT * 1.3);
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Distance: ")) + QString::number(getGCDistanceScaled() * EQ_SCALED_TO_REAL, 'f', 1) + distUnits);

    rowY += ROWHEIGHT;
    QString elevUnits = GlobalContext::context()->useMetricUnits ? tr(" meters") : tr(" feet");
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Elevation: ")) + QString::number(getGCElevationScaled() * EQ_SCALED_TO_REAL, 'f', 1) + elevUnits);

    rowY += ROWHEIGHT;
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Time: ")) + QString::number(activityTimeInSecs_ * HOURS_PER_SECOND, 'f', 1) + " hrs");

    bool addNotesOffset = false;
    rowY += (ROWHEIGHT * 0.3);

    if (getNonGCDistanceScaled() != 0) {

        rowY += (ROWHEIGHT);
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Manual dst: ")) + QString::number(getNonGCDistanceScaled() * EQ_SCALED_TO_REAL, 'f', 1) + distUnits);
    }

    if (getNonGCElevationScaled() != 0) {

        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Manual elev: ")) + QString::number(getNonGCElevationScaled() * EQ_SCALED_TO_REAL, 'f', 1) + elevUnits);
    }

    if (repDistanceScaled_ != 0) {

        if (overDistance) painter->setPen(alertColor);

        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Replace dist: ")) + QString::number(repDistanceScaled_ * EQ_SCALED_TO_REAL, 'f', 1) + distUnits);

        if (overDistance) painter->setPen(textColor);
    }

    if (repElevationScaled_ != 0) {

        if (overElevation) painter->setPen(alertColor);

        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Replace elev: ")) + QString::number(repElevationScaled_ * EQ_SCALED_TO_REAL, 'f', 1) + elevUnits);

        if (overElevation) painter->setPen(textColor);
    }

    if (!notes_.isEmpty()) {

        rowY +=  (addNotesOffset) ? ROWHEIGHT*1.3 : ROWHEIGHT;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), "Notes:");
        rowY += (ROWHEIGHT);
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), notes_);
    }
}

void
EquipmentSummary::configChanged(qint32 cfg) {

    if ((cfg == 0) || (cfg & CONFIG_APPEARANCE)) {
        textColor = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(200, 200, 200)) : QColor(QColor(70, 70, 70));
    }
}

void
EquipmentSummary::itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {

    double rowY(ROWHEIGHT * 2.5);
    double rowWidth(geometry().width() - (ROWHEIGHT * 2));
    double rowHeight(geometry().height() - (ROWHEIGHT * 4));

    painter->setFont(parent->smallfont);

    painter->setPen(GColor(CPLOTMARKER));

    QString eqLinkName = (eqLinkName_ != "") ? eqLinkName_ : "All Activities";
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), eqLinkName);

    rowY += (ROWHEIGHT * 1.3);
    painter->setPen(textColor);
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Activities: ")) + QString::number(eqLinkNumActivities_));

    rowY += ROWHEIGHT;
    if (showActivitiesPerAthlete_) {
        for (auto itr = athleteActivityMap_.keyValueBegin(); itr != athleteActivityMap_.keyValueEnd(); ++itr) {

            painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
                QString("   ") + itr->first + ": " + QString::number(itr->second));

            rowY += ROWHEIGHT;
        }
    }

    rowY += (ROWHEIGHT * 0.25);
    QString eqLinkEarliestDate = (eqLinkNumActivities_ != 0) ? eqLinkEarliestDate_.toString("d MMM yyyy") : " --";
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Earliest Activity: ")) + eqLinkEarliestDate);

    rowY += (ROWHEIGHT);
    QString eqLinkLatestDate = (eqLinkNumActivities_ != 0) ? eqLinkLatestDate_.toString("d MMM yyyy") : " --";
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Latest Activity: ")) + eqLinkLatestDate);

    rowY += (ROWHEIGHT * 1.25);
    QString distUnits = GlobalContext::context()->useMetricUnits ? tr(" km") : tr(" miles");
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Distance: ")) + QString::number(eqLinkTotalDistanceScaled_*EQ_SCALED_TO_REAL, 'f', 1) + distUnits);

    rowY += (ROWHEIGHT);
    QString elevUnits = GlobalContext::context()->useMetricUnits ? tr(" meters") : tr(" feet");
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Elevation: ")) + QString::number(eqLinkTotalElevationScaled_*EQ_SCALED_TO_REAL, 'f', 1) + elevUnits);

    rowY += ROWHEIGHT;
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Time: ")) + QString::number(eqLinkTotalTimeInSecs_*HOURS_PER_SECOND, 'f', 1) + " hrs");
}

void
EquipmentNotes::configChanged(qint32 cfg) {

    if ((cfg == 0) || (cfg & CONFIG_APPEARANCE)) {
        textColor = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(200, 200, 200)) : QColor(QColor(70, 70, 70));
    }
}

void
EquipmentNotes::itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {

    painter->setPen(textColor);
    painter->setFont(parent->smallfont);

    double rowY(ROWHEIGHT * 2.5);
    double rowWidth(geometry().width() - (ROWHEIGHT * 2));
    double rowHeight(geometry().height() - (ROWHEIGHT * 4));

    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), notes_);
}

