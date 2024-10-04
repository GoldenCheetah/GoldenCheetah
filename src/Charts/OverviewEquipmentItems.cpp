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

// supports tile move & clone
#include "OverviewEquipment.h"
#include "Perspective.h"

bool
OverviewEquipmentItemConfig::registerItems()
{
    // get the factory
    ChartSpaceItemRegistry &registry = ChartSpaceItemRegistry::instance();

    // Register      TYPE                           SHORT                           DESCRIPTION                         SCOPE                        CREATOR
    registry.addItem(OverviewItemType::EQ_ITEM,     QObject::tr("Equipment"),       QObject::tr("Equipment Item"),      OverviewScope::EQUIPMENT,    EquipmentItem::create);
    registry.addItem(OverviewItemType::EQ_SUMMARY,  QObject::tr("Eq Link Summary"), QObject::tr("Equipment Summary"),   OverviewScope::EQUIPMENT,    EquipmentSummary::create);
    registry.addItem(OverviewItemType::EQ_HISTORY,  QObject::tr("Eq Link History"), QObject::tr("Equipment History"),   OverviewScope::EQUIPMENT,    EquipmentHistory::create);
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
    endDate_ = QDate(9999, 12, 31);
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

int
EqTimeWindow::displayImportance() const
{
   if (!rangeIsValid()) return 9;

   int importance = 0;

   if (!startSet_)
       importance = (!endSet_) ? 4 : 1;
   else
       importance = (!endSet_) ? 3 : 2;
   
   if (isWithin(QDate::currentDate())) importance += 4;

    return importance;
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
        QStringList headers = (QStringList() << tr("EqLink Name") << tr("Start") << tr("Start Date") << tr("End") << tr("End Date"));
        eqTimeWindows->setHorizontalHeaderLabels(headers);
        eqTimeWindows->setColumnWidth(1, 40 * dpiXFactor);
        eqTimeWindows->setColumnWidth(2, 90 * dpiXFactor);
        eqTimeWindows->setColumnWidth(3, 40 * dpiXFactor);
        eqTimeWindows->setColumnWidth(4, 90 * dpiXFactor);
        eqTimeWindows->setMinimumWidth(400 * dpiXFactor);
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
        QPushButton *addEqLink = new QPushButton("Add EqLink");
        QPushButton *removeEqLink = new QPushButton("Remove EqLink");
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

        QHBoxLayout *replaceLayout = new QHBoxLayout();
        replaceDateSet = new QPushButton();
        replaceDateSet->setMaximumWidth(60 * dpiXFactor);
        connect(replaceDateSet, SIGNAL(clicked()), this, SLOT(repDateSetClicked()));
        replaceDate = new QDateEdit();
        replaceDate->setCalendarPopup(true);
        replaceDate->setStyleSheet(QString("QDateEdit { border: 0px; } "));
        QSizePolicy sp_retain = replaceDate->sizePolicy();
        sp_retain.setRetainSizeWhenHidden(true);
        replaceDate->setSizePolicy(sp_retain);
        connect(replaceDate, SIGNAL(dateChanged(QDate)), this, SLOT(dataChanged()));
        replaceLayout->addWidget(replaceDateSet);
        replaceLayout->addWidget(replaceDate);
        layout->insertRow(insertRow++, tr("Replacement date"), replaceLayout);

        notes = new QPlainTextEdit();
        connect(notes, SIGNAL(textChanged()), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("Notes"), notes);
    }

    if (item->type == OverviewItemType::EQ_SUMMARY) {

        eqLinkName = new QLineEdit();
        connect(eqLinkName, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("EqLink Name"), eqLinkName);

        eqCheckBox = new QCheckBox();
        connect(eqCheckBox, SIGNAL(stateChanged(int)), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("Show Athlete's Activities"), eqCheckBox);
    }

    if (item->type == OverviewItemType::EQ_HISTORY) {

        eqCheckBox = new QCheckBox();
        connect(eqCheckBox, SIGNAL(stateChanged(int)), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, tr("Most Recent First"), eqCheckBox);

        eqTimeWindows = new QTableWidget(0, 2);
        QStringList headers = (QStringList() << tr("Date") << tr("Description"));
        eqTimeWindows->setHorizontalHeaderLabels(headers);
        eqTimeWindows->setColumnWidth(0, 90 * dpiXFactor);
        eqTimeWindows->setColumnWidth(1, 410 * dpiXFactor);
        eqTimeWindows->setMinimumWidth(400 * dpiXFactor);
        eqTimeWindows->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        eqTimeWindows->setSelectionBehavior(QAbstractItemView::SelectRows);
        eqTimeWindows->setSelectionMode(QAbstractItemView::SingleSelection);
        QPalette palette = eqTimeWindows->palette();
        palette.setBrush(QPalette::Highlight, palette.brush(QPalette::Base));
        palette.setBrush(QPalette::HighlightedText, palette.brush(QPalette::Text));
        eqTimeWindows->setPalette(palette);
        eqTimeWindows->verticalHeader()->setVisible(false);
        connect(eqTimeWindows, SIGNAL(cellClicked(int, int)), this, SLOT(dataChanged()));
        layout->insertRow(insertRow++, "History", eqTimeWindows);

        QHBoxLayout* buttonRow = new QHBoxLayout();
        QPushButton *addHistory = new QPushButton("Add History");
        QPushButton *removeHistory = new QPushButton("Remove History");
        buttonRow->addWidget(addHistory);
        connect(addHistory, SIGNAL(clicked(bool)), this, SLOT(addHistoryRow()));
        buttonRow->addWidget(removeHistory);
        connect(removeHistory, SIGNAL(clicked(bool)), this, SLOT(removeHistoryRow()));
        layout->insertRow(insertRow++, "", buttonRow);
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
        EquipmentItem *mi = dynamic_cast<EquipmentItem*>(item);
        name->setText(mi->name);
        nonGCDistance->setText(QString::number(mi->getNonGCDistanceScaled() * EQ_SCALED_TO_REAL, 'f', 1));
        nonGCElevation->setText(QString::number(mi->getNonGCElevationScaled() * EQ_SCALED_TO_REAL, 'f', 1));
        replaceDistance->setText(QString::number(mi->repDistanceScaled_ * EQ_SCALED_TO_REAL, 'f', 1));
        replaceElevation->setText(QString::number(mi->repElevationScaled_ * EQ_SCALED_TO_REAL, 'f', 1));
        replaceDateSet->setText((mi->repDateSet_) ? "reset" : "set");
        replaceDate->setVisible(mi->repDateSet_);
        if (mi->repDateSet_) replaceDate->setDate(mi->repDate_);

        // clear the table
        eqTimeWindows->setRowCount(0);

        for (int tableRow = 0; tableRow < mi->eqLinkUseList_.size(); tableRow++) {

            eqTimeWindows->insertRow(tableRow);
            const EqTimeWindow& eqUse = mi->eqLinkUseList_[tableRow];
            setEqLinkRowWidgets(tableRow, &eqUse);

            static_cast<QLineEdit*>(eqTimeWindows->cellWidget(tableRow, 0))->setText(eqUse.eqLinkName_);

            static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 1))->setText((eqUse.startSet_) ? "reset" : "set");
            if (eqUse.startSet_) {
                static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 2))->setDate(eqUse.startDate_);
            }

            static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 3))->setText((eqUse.endSet_) ? "reset" : "set");
            if (eqUse.endSet_) {
                static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 4))->setDate(eqUse.endDate_);
            }
        }
        notes->setPlainText(mi->notes_);
    }
    break;

    case OverviewItemType::EQ_SUMMARY:
    {
        EquipmentSummary *mi = dynamic_cast<EquipmentSummary*>(item);
        name->setText(mi->name);
        eqLinkName->setText(mi->eqLinkName_);
        eqCheckBox->setChecked(mi->showActivitiesPerAthlete_);
    }
    break;

    case OverviewItemType::EQ_HISTORY:
    {
        EquipmentHistory *mi = dynamic_cast<EquipmentHistory*>(item);
        name->setText(mi->name);
        eqCheckBox->setChecked(mi->sortMostRecentFirst_);

        // clear the table
        eqTimeWindows->setRowCount(0);

        int tableRow = 0;
        for (const auto& eqHistory : mi->eqHistoryList_) {

            eqTimeWindows->insertRow(tableRow);
            setEqHistoryEntryRowWidgets(tableRow);

            static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 0))->setDate(eqHistory.date_);
            static_cast<QLineEdit*>(eqTimeWindows->cellWidget(tableRow, 1))->setText(eqHistory.text_);
            tableRow++;
        }
    }
    break;

    case OverviewItemType::EQ_NOTES:
    {
        EquipmentNotes *mi = dynamic_cast<EquipmentNotes*>(item);
        name->setText(mi->name);
        notes->setPlainText(mi->notes_);
    }
    break;
    }
    block = false;
}

void
OverviewEquipmentItemConfig::repDateSetClicked()
{
    if (replaceDateSet->text() == "reset") {

        replaceDateSet->setText("set");
        replaceDate->setVisible(false);
    }
    else
    {
        replaceDateSet->setText("reset");
        replaceDate->setVisible(true);
    }
    dataChanged();
}

void
OverviewEquipmentItemConfig::tableCellClicked(int row, int column)
{
    // only handle cell clicks on the checkboxs
    if (column == 1 || column == 3) {

        if (static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column))->text() == "reset") {

            static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column))->setText("set");

            eqTimeWindows->setCellWidget(row, column + 1, nullptr);
        }
        else
        {
            static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column))->setText("reset");

            QDateEdit* date = new QDateEdit();
            date->setCalendarPopup(true);
            date->setStyleSheet(QString("QDateEdit { border: 0px; } "));
            connect(date, SIGNAL(dateChanged(QDate)), this, SLOT(dataChanged()));
            eqTimeWindows->setCellWidget(row, column + 1, date);
        }
        dataChanged();
    }

    if (column == 2 || column == 4) {

        if (eqTimeWindows->cellWidget(row, column) == nullptr) {

            QDateEdit* date = new QDateEdit();
            date->setCalendarPopup(true);
            date->setStyleSheet(QString("QDateEdit { border: 0px; } "));
            connect(date, SIGNAL(dateChanged(QDate)), this, SLOT(dataChanged()));
            eqTimeWindows->setCellWidget(row, column, date);

            static_cast<QLabel*>(eqTimeWindows->cellWidget(row, column - 1))->setText("reset");
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
    QLineEdit *eqLink = new QLineEdit();
    eqLink->setFrame(false);
    connect(eqLink, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
    eqTimeWindows->setCellWidget(tableRow, 0, eqLink);

    QLabel *startSet = new QLabel();
    startSet->setAlignment(Qt::AlignHCenter);
    startSet->setText((eqUse && eqUse->startSet_) ? "reset" : "set");
    eqTimeWindows->setCellWidget(tableRow, 1, startSet);

    if (eqUse && eqUse->startSet_) {
        QDateEdit* startDate = new QDateEdit();
        startDate->setCalendarPopup(true);
        startDate->setStyleSheet(QString("QDateEdit { border: 0px; } "));
        connect(startDate, SIGNAL(dateChanged(QDate)), this, SLOT(dataChanged()));
        eqTimeWindows->setCellWidget(tableRow, 2, startDate);
    }

    QLabel *endSet = new QLabel();
    endSet->setAlignment(Qt::AlignHCenter);
    endSet->setText((eqUse && eqUse->endSet_) ? "reset" : "set");
    eqTimeWindows->setCellWidget(tableRow, 3, endSet);

    if (eqUse && eqUse->endSet_) {
        QDateEdit* endDate = new QDateEdit();
        endDate->setCalendarPopup(true);
        endDate->setStyleSheet(QString("QDateEdit { border: 0px; } "));
        connect(endDate, SIGNAL(dateChanged(QDate)), this, SLOT(dataChanged()));
        eqTimeWindows->setCellWidget(tableRow, 4, endDate);
    }
}

void
OverviewEquipmentItemConfig::addEqLinkRow()
{
    block = true;

    eqTimeWindows->insertRow(0);
    setEqLinkRowWidgets(0, nullptr);

    block = false;
    dataChanged();
}

void
OverviewEquipmentItemConfig::removeEqLinkRow()
{
    eqTimeWindows->removeRow(eqTimeWindows->currentRow());
    dataChanged();
}

void
OverviewEquipmentItemConfig::setEqHistoryEntryRowWidgets(int tableRow)
{
    QDateEdit* historyDate = new QDateEdit();
    historyDate->setCalendarPopup(true);
    historyDate->setStyleSheet(QString("QDateEdit { border: 0px; } "));
    connect(historyDate, SIGNAL(dateChanged(QDate)), this, SLOT(dataChanged()));
    eqTimeWindows->setCellWidget(tableRow, 0, historyDate);

    QLineEdit* historyText = new QLineEdit();
    historyText->setFrame(false);
    connect(historyText, SIGNAL(textChanged(QString)), this, SLOT(dataChanged()));
    eqTimeWindows->setCellWidget(tableRow, 1, historyText);
}

void
OverviewEquipmentItemConfig::addHistoryRow()
{
    block = true;

    eqTimeWindows->insertRow(0);
    setEqHistoryEntryRowWidgets(0);
    static_cast<QDateEdit*>(eqTimeWindows->cellWidget(0, 0))->setDate(QDate::currentDate());

    block = false;
    dataChanged();
}

void
OverviewEquipmentItemConfig::removeHistoryRow()
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
                bool startSet = static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 1))->text() == "reset";
                if (startSet) {
                    startDate = static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 2))->date();
                }

                QDate endDate;
                bool endSet = static_cast<QLabel*>(eqTimeWindows->cellWidget(tableRow, 3))->text() == "reset";
                if (endSet) {
                    endDate = static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 4))->date();
                }

                eqLinkUse.push_back(EqTimeWindow(eqLinkName, startSet, startDate, endSet, endDate));
            }
        }
        mi->eqLinkUseList_ = eqLinkUse;

        mi->repDistanceScaled_ = round(replaceDistance->text().toDouble() * EQ_REAL_TO_SCALED); // validator restricts 1 dp
        mi->repElevationScaled_ = round(replaceElevation->text().toDouble() * EQ_REAL_TO_SCALED); // validator restricts 1 dp

        mi->repDateSet_ = replaceDateSet->text() == "reset";
        if (mi->repDateSet_) mi->repDate_ = replaceDate->date();

        mi->notes_ = notes->toPlainText();
        mi->bgcolor = bgcolor->getColor().name();
    }
    break;

    case OverviewItemType::EQ_SUMMARY:
    {
        EquipmentSummary* mi = dynamic_cast<EquipmentSummary*>(item);
        mi->name = name->text();
        mi->eqLinkName_ = eqLinkName->text();
        mi->showActivitiesPerAthlete_ = eqCheckBox->isChecked();
        mi->bgcolor = bgcolor->getColor().name();
    }
    break;

    case OverviewItemType::EQ_HISTORY:
    {
        EquipmentHistory* mi = dynamic_cast<EquipmentHistory*>(item);
        mi->name = name->text();
        mi->sortMostRecentFirst_ = eqCheckBox->isChecked();

        QVector<EqHistoryEntry> eqHistory;
        for (int tableRow = 0; tableRow < eqTimeWindows->rowCount(); tableRow++) {

            eqHistory.push_back(EqHistoryEntry(static_cast<QDateEdit*>(eqTimeWindows->cellWidget(tableRow, 0))->date(),
                                          static_cast<QLineEdit*>(eqTimeWindows->cellWidget(tableRow, 1))->text()));
        }
        mi->eqHistoryList_ = eqHistory;
        mi->sortHistoryEntries();

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
// ------------------------------------- Common Equipment Item -----------------------------------------
//

CommonEquipmentItem::CommonEquipmentItem(ChartSpace* parent, const QString& name) :
    ChartSpaceItem(parent, name), tileDisplayHeight_(ROWHEIGHT * 5)
{
    setShowEdit(true);
}

void CommonEquipmentItem::DisplayTileEditMenu(const QPoint& pos)
{
    QMenu popMenu;

    for (const GcChartWindow* chart : parent->window->getPerspective()->getCharts()) {

        // add the clone tile option at the top first
        if (chart == parent->window) {
            QAction* metaAction = new QAction("Clone");
            GcChartWindow* thisChart = const_cast<GcChartWindow*>(chart);
            QVariant varPtr(QVariant::fromValue(static_cast<void*>(thisChart)));
            metaAction->setData(varPtr);
            popMenu.addAction(metaAction);
        }
    }

    QAction* metaAction = new QAction(tr("Expand"));
    popMenu.addAction(metaAction);

    metaAction = new QAction(tr("Collapse"));
    popMenu.addAction(metaAction);

    metaAction = new QAction(tr("Expand All"));
    popMenu.addAction(metaAction);

    metaAction = new QAction(tr("Collapse All"));
    popMenu.addAction(metaAction);

    for (const GcChartWindow* chart : parent->window->getPerspective()->getCharts()) {

        // Add the move to chart options
        if (chart != parent->window) {
            QAction* metaAction = new QAction("-->" + chart->title());
            GcChartWindow* anotherChart = const_cast<GcChartWindow*>(chart);
            QVariant varPtr(QVariant::fromValue(static_cast<void*>(anotherChart)));
            metaAction->setData(varPtr);
            popMenu.addAction(metaAction);
        }
    }

    if (!popMenu.isEmpty()) {

        connect(&popMenu, SIGNAL(triggered(QAction*)), this, SLOT(popupAction(QAction*)));
        popMenu.exec(pos);
    }
}

void CommonEquipmentItem::popupAction(QAction* action)
{
    if (action->text() == tr("Expand")) {
        parent->adjustItemHeight(this, round(tileDisplayHeight_/ROWHEIGHT));
        return;
    }
    if (action->text() == tr("Collapse")) {
        parent->adjustItemHeight(this, 5);
        return;
    }
    if (action->text() == tr("Expand All")) {

        for (ChartSpaceItem* item : parent->allItems()) {

            parent->adjustItemHeight(item, round(static_cast<CommonEquipmentItem*>(item)->tileDisplayHeight_ / ROWHEIGHT));
            updateGeometry();
        }
        return;
    }
    if (action->text() == tr("Collapse All")) {
        for (ChartSpaceItem* item : parent->allItems()) {

            parent->adjustItemHeight(item, 5);
            updateGeometry();
        }
        return;
    }

    GcChartWindow* toChart = static_cast<GcChartWindow*>(action->data().value<void*>());
    const ChartSpace* toChartSpace = static_cast<OverviewEquipmentWindow*>(toChart)->getSpace();

    if (parent == toChartSpace) {

        // clone me
        static_cast<OverviewEquipmentWindow*>(toChart)->cloneTile(this);
    }
    else
    {
        // move from existing to new chart
        parent->moveItem(this, const_cast<ChartSpace*>(toChartSpace));
    }
}

int
CommonEquipmentItem::setupScrollableText(const QFontMetrics& fm, const QString& tileText,
                                        QMap<int, QString>& rowTextMap, int rowOffset /* = 0 */)
{
    // Using the QFontMetrics rectangle on the whole string doesn't always give the correct number
    // of rows as the painter will not render partial words, and there maybe newline characters,
    // this algorithm calculates the rows for whole words like the painter.

    QChar chr;
    int lastSpace = 0;
    int beginingOfRow = 0;
    int widthOfLineChars = 0;
    int numRowsInNotes = 0;
    int rowWidth = round(geometry().width() - (ROWHEIGHT * 2));

    // determine the number of rows and the scrollable text for each scrollbar position,
    // based upon only whole words per row and any newline characters.
    int i = 0;
    while (i < tileText.length())
    {
        chr = tileText.at(i);

        if (chr.isSpace()) lastSpace = i;

        int charWidth = fm.horizontalAdvance(chr);
        widthOfLineChars += charWidth;

        if (chr == '\n') { // check for carriage return
            rowTextMap[numRowsInNotes+ rowOffset] = tileText.mid(beginingOfRow, i - beginingOfRow);
            numRowsInNotes++;
            widthOfLineChars = 0;
            beginingOfRow = i + 1;
        }
        else {

            if (widthOfLineChars > rowWidth) { // characters exceed row capacity

                if (i != lastSpace) i = lastSpace; // revert to last whole word
                rowTextMap[numRowsInNotes+ rowOffset] = tileText.mid(beginingOfRow, i - beginingOfRow);
                numRowsInNotes++;
                widthOfLineChars = 0;
                beginingOfRow = i + 1;
            }
        }

        i++; // advance to next character
    }
    rowTextMap[numRowsInNotes+ rowOffset] = tileText.mid(beginingOfRow, i - beginingOfRow);

    return numRowsInNotes+1;
}

//
// ---------------------------------------- Equipment Item --------------------------------------------
//

EquipmentItem::EquipmentItem(ChartSpace* parent, const QString& name) : CommonEquipmentItem(parent, name)
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
    repDateSet_ = false;
    numRowsInNotes_ = 0;

    configwidget_ = new OverviewEquipmentItemConfig(this);
    configwidget_->hide();

    configChanged(CONFIG_APPEARANCE);
}

EquipmentItem::EquipmentItem(ChartSpace* parent, const QString& name, QVector<EqTimeWindow>& eqLinkUse,
                            const uint64_t nonGCDistanceScaled, const uint64_t nonGCElevationScaled,
                            const uint64_t repDistanceScaled, const uint64_t repElevationScaled,
                            const bool repDateSet, const QDate& repDate, const QString& notes) : EquipmentItem(parent, name)
{
    eqLinkUseList_ = eqLinkUse;
    sortEquipmentWindows();

    nonGCDistanceScaled_ = nonGCDistanceScaled;
    nonGCElevationScaled_ = nonGCElevationScaled;
    repDistanceScaled_ = repDistanceScaled;
    repElevationScaled_ = repElevationScaled;
    repDateSet_ = repDateSet;
    repDate_ = repDate;
    notes_ = notes;

    // set the widgets again for the passed in parameters
    configwidget_->setWidgets();
}

void
EquipmentItem::setData(RideItem*)
{
    // called when the item's config dialog is closed.
    sortEquipmentWindows();
}

void
EquipmentItem::showEvent(QShowEvent*)
{
    // wait for tile geometry to be defined
    itemGeometryChanged();
}

void
EquipmentItem::itemGeometryChanged()
{
    QMap<int, QString> scrollableDisplayText;
    QFontMetrics fm(parent->smallfont, parent->device());

    numRowsInNotes_ = setupScrollableText(fm, notes_, scrollableDisplayText);
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
EquipmentItem::sortEquipmentWindows()
{
    std::sort(eqLinkUseList_.begin(), eqLinkUseList_.end(), [](const EqTimeWindow& a, const EqTimeWindow& b)
    {
        if (a.displayImportance() > b.displayImportance()) return true;
        if (a.displayImportance() < b.displayImportance()) return false;

        // same priority, so now order them on date
        switch (a.displayImportance())
        {
            case 1: // start & end set inactive
            case 5: // start & end set active
            case 3: // only start set inactive
            case 7: // only start set active
                return a.startDate_ < b.startDate_;
            case 2: // only end set inactive
            case 6: // only end set active
                return a.endDate_ < b.endDate_;
            default: // no start or end active, or invalid
                return a.eqLinkName_ < b.eqLinkName_;
        }
    });
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
    for (const auto& eqUse : eqLinkUseList_) {

        if (rideEqLinkNameList.contains(eqUse.eqLinkName_) && eqUse.isWithin(actDate)) return true;
    }
    return false;
}

bool
EquipmentItem::isWithin(const QDate& actDate) const
{
    for (const auto& eqUse : eqLinkUseList_) {

        if (eqUse.isWithin(actDate)) return true;
    }
    return false;
}

bool
EquipmentItem::rangeIsValid() const
{
    bool rangeIsValid = true;
    for (const auto& eqUse : eqLinkUseList_) {
        rangeIsValid &= (eqUse.startSet_ && eqUse.endSet_) ? eqUse.endDate_ >= eqUse.startDate_ : true;
    }
    return rangeIsValid;
}

void
EquipmentItem::configChanged(qint32 cfg) {

    if (cfg & CONFIG_APPEARANCE) {
        inactiveColor_ = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(100, 100, 100)) : QColor(QColor(170, 170, 170));
        textColor_ = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(200, 200, 200)) : QColor(QColor(70, 70, 70));
        alertColor_ = QColor(QColor(255, 170, 0));
    }
}

void
EquipmentItem::itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    
    // mid is slightly higher to account for space around title, move mid up
    static double mid = ROWHEIGHT * 3.0f;

    bool overDistance = (repDistanceScaled_ !=0) && (getTotalDistanceScaled() > repDistanceScaled_);
    bool overElevation = (repElevationScaled_ !=0) && (getTotalElevationScaled() > repElevationScaled_);
    bool overDate = (repDateSet_) && (QDate::currentDate() > repDate_);

    // we align centre and mid
    QFontMetrics fm(parent->bigfont);
    QString totalDistanceStr(QString("%L1").arg(getTotalDistanceScaled() * EQ_SCALED_TO_REAL, 0, 'f', 0));
    QRectF rect = QFontMetrics(parent->bigfont, parent->device()).boundingRect(totalDistanceStr);
    painter->setFont(parent->bigfont);

    if ((!rangeIsValid()) || overDistance || overElevation || overDate) {
        painter->setPen(alertColor_);
    } else if (isWithin(QDate::currentDate())) {
        painter->setPen(GColor(CPLOTMARKER));
    } else {
        painter->setPen(inactiveColor_);
    }

    // display the total distance
    painter->drawText(QPointF((geometry().width() - rect.width()) / 2.0f,
                        mid + (fm.ascent() / 3.0f)), totalDistanceStr); // divided by 3 to account for "gap" at top of font

    painter->setPen(QColor(100, 100, 100));
    painter->setFont(parent->smallfont);
    double addy = QFontMetrics(parent->smallfont).height();

    QString distUnits = GlobalContext::context()->useMetricUnits ? tr(" km") : tr(" miles");

    painter->drawText(QPointF((geometry().width() - QFontMetrics(parent->smallfont).horizontalAdvance(distUnits)) / 2.0f,
        mid + (fm.ascent() / 3.6f) + addy), distUnits); // divided by 3 to account for "gap" at top of font

    double rowY(ROWHEIGHT * 5);
    double rowWidth(geometry().width() - (ROWHEIGHT * 2));
    double rowHeight(geometry().height() - (ROWHEIGHT * 4));

    for (const auto& eqUse : eqLinkUseList_) {

        // display the date in either active, inactive or out of range colours
        if (!eqUse.rangeIsValid()) {
            painter->setPen(alertColor_);
        }
        else if (eqUse.isWithin(QDate::currentDate())) {
            painter->setPen(GColor(CPLOTMARKER));
        }
        else {
            painter->setPen(inactiveColor_);
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

        rect = QFontMetrics(parent->smallfont, parent->device()).boundingRect(dateString);
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), dateString);

        rowY += (ROWHEIGHT * ceil(rect.width() / rowWidth));
    }

    painter->setPen(textColor_);

    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth,rowHeight),
        QString(tr("Activities: ")) + QString("%L1").arg(getNumActivities()));

    rowY += (ROWHEIGHT * 1.2);
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Distance: ")) + QString("%L1").arg(getGCDistanceScaled() * EQ_SCALED_TO_REAL, 0, 'f', 0) + distUnits);

    rowY += ROWHEIGHT;
    QString elevUnits = GlobalContext::context()->useMetricUnits ? tr(" meters") : tr(" feet");
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Elevation: ")) + QString("%L1").arg(getGCElevationScaled() * EQ_SCALED_TO_REAL, 0, 'f', 0) + elevUnits);

    rowY += ROWHEIGHT;
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Time: ")) + QString("%L1").arg(activityTimeInSecs_ * HOURS_PER_SECOND, 0, 'f', 0) + " hrs");

    bool addNotesOffset = false;
    rowY += (ROWHEIGHT * 0.2);

    if (getNonGCDistanceScaled() != 0) {

        rowY += (ROWHEIGHT);
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Manual dst: ")) + QString("%L1").arg(getNonGCDistanceScaled() * EQ_SCALED_TO_REAL, 0, 'f', 0) + distUnits);
    }

    if (getNonGCElevationScaled() != 0) {

        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Manual elev: ")) + QString("%L1").arg(getNonGCElevationScaled() * EQ_SCALED_TO_REAL, 0, 'f', 0) + elevUnits);
    }

    if (repDistanceScaled_ != 0) {

        if (overDistance) painter->setPen(alertColor_);

        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Replace dist: ")) + QString("%L1").arg(repDistanceScaled_ * EQ_SCALED_TO_REAL, 0, 'f', 0) + distUnits);

        if (overDistance) painter->setPen(textColor_);
    }

    if (repElevationScaled_ != 0) {

        if (overElevation) painter->setPen(alertColor_);

        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Replace elev: ")) + QString("%L1").arg(repElevationScaled_ * EQ_SCALED_TO_REAL, 0, 'f', 0) + elevUnits);

        if (overElevation) painter->setPen(textColor_);
    }

    if (repDateSet_) {

        if (overDate) painter->setPen(alertColor_);

        rowY += ROWHEIGHT;
        addNotesOffset = true;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
            QString(tr("Replace date: ")) + repDate_.toString("->d MMM yy"));

        if (overDate) painter->setPen(textColor_);
    }

    if (!notes_.isEmpty()) {

        rowY +=  (addNotesOffset) ? ROWHEIGHT*1.3 : ROWHEIGHT;
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), "Notes:");
        rowY += (ROWHEIGHT);
        painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), notes_);
    }

    tileDisplayHeight_ = rowY + (ROWHEIGHT * (numRowsInNotes_+1));
}

//
// ---------------------------------------- Equipment Summary --------------------------------------------
//

EquipmentSummary::EquipmentSummary(ChartSpace* parent, const QString& name,
    const QString& eqLinkName, bool showActivitiesPerAthlete) :
    CommonEquipmentItem(parent, name)
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

    configChanged(CONFIG_APPEARANCE);
}

void
EquipmentSummary::resetEqItem()
{
    athleteActivityMap_.clear();
    eqLinkNumActivities_ = 0;
    eqLinkTotalDistanceScaled_ = 0;
    eqLinkTotalElevationScaled_ = 0;
    eqLinkTotalTimeInSecs_ = 0;
    eqLinkEarliestDate_ = QDate(1900, 01, 01);
    eqLinkLatestDate_ = eqLinkEarliestDate_;
}

void
EquipmentSummary::addActivity(const QString& athleteName, const QDate& activityDate,
    const uint64_t rideDistanceScaled, const uint64_t eqElevationScaled,
    const uint64_t rideTimeInSecs)
{
    activityMutex_.lock();

    athleteActivityMap_[athleteName] += 1;

    if (eqLinkNumActivities_++) {
        if (activityDate < eqLinkEarliestDate_) eqLinkEarliestDate_ = activityDate;
        if (activityDate > eqLinkLatestDate_) eqLinkLatestDate_ = activityDate;
    }
    else {
        eqLinkEarliestDate_ = activityDate;
        eqLinkLatestDate_ = activityDate;
    }

    activityMutex_.unlock();

    eqLinkTotalDistanceScaled_ += rideDistanceScaled;
    eqLinkTotalElevationScaled_ += eqElevationScaled;
    eqLinkTotalTimeInSecs_ += rideTimeInSecs;
}

void
EquipmentSummary::configChanged(qint32 cfg) {

    if (cfg & CONFIG_APPEARANCE) {
        textColor_ = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(200, 200, 200)) : QColor(QColor(70, 70, 70));
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
    painter->setPen(textColor_);
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Activities: ")) + QString("%L1").arg(eqLinkNumActivities_));

    rowY += ROWHEIGHT;
    if (showActivitiesPerAthlete_) {
        for (auto itr = athleteActivityMap_.keyValueBegin(); itr != athleteActivityMap_.keyValueEnd(); ++itr) {

            painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
                QString("   ") + itr->first + ": " + QString("%L1").arg(itr->second));

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
        QString(tr("Distance: ")) + QString("%L1").arg(eqLinkTotalDistanceScaled_*EQ_SCALED_TO_REAL, 0, 'f', 0) + distUnits);

    rowY += (ROWHEIGHT);
    QString elevUnits = GlobalContext::context()->useMetricUnits ? tr(" meters") : tr(" feet");
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Elevation: ")) + QString("%L1").arg(eqLinkTotalElevationScaled_*EQ_SCALED_TO_REAL, 0, 'f', 0) + elevUnits);

    rowY += ROWHEIGHT;
    painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight),
        QString(tr("Time: ")) + QString("%L1").arg(eqLinkTotalTimeInSecs_*HOURS_PER_SECOND, 0, 'f', 0) + " hrs");

    tileDisplayHeight_ = rowY + (ROWHEIGHT * 1.5);
}

//
// ------------------------------------------ EqTimeWindow --------------------------------------------
//

EqHistoryEntry::EqHistoryEntry()
{
}

EqHistoryEntry::EqHistoryEntry(const QDate& date, const QString& text) :
    date_(date), text_(text)
{
}

//
// ---------------------------------------- Equipment History --------------------------------------------
//

EquipmentHistory::EquipmentHistory(ChartSpace* parent, const QString& name) :
    CommonEquipmentItem(parent, name)
{
    // equipment summary item
    this->type = OverviewItemType::EQ_HISTORY;

    scrollPosn_ = 0;
    sortMostRecentFirst_ = false;

    configwidget_ = new OverviewEquipmentItemConfig(this);
    configwidget_->hide();

    scrollbar_ = new VScrollBar(this, parent);
    scrollbar_->show();
    configChanged(CONFIG_APPEARANCE);
}

EquipmentHistory::EquipmentHistory(ChartSpace* parent, const QString& name, const QVector<EqHistoryEntry>& eqHistory, const bool sortMostRecentFirst) :
    EquipmentHistory(parent, name)
{
    sortMostRecentFirst_ = sortMostRecentFirst;
    eqHistoryList_ = eqHistory;
    sortHistoryEntries();
}

void
EquipmentHistory::configChanged(qint32 cfg) {

    if (cfg & CONFIG_APPEARANCE) {
        textColor_ = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(200, 200, 200)) : QColor(QColor(70, 70, 70));
    }
}

void
EquipmentHistory::showEvent(QShowEvent*)
{
    // wait for tile geometry to be defined
    itemGeometryChanged();
}

void
EquipmentHistory::sortHistoryEntries()
{
    if (sortMostRecentFirst_)
        std::sort(eqHistoryList_.begin(), eqHistoryList_.end(), [](const EqHistoryEntry& a, const EqHistoryEntry& b) { return a.date_ > b.date_; });
    else
        std::sort(eqHistoryList_.begin(), eqHistoryList_.end(), [](const EqHistoryEntry& a, const EqHistoryEntry& b) { return a.date_ < b.date_; });
}

void
EquipmentHistory::itemGeometryChanged()
{
    QFontMetrics fm(parent->smallfont, parent->device());

    int numHistoryRows = 0;
    scrollableDisplayText_.clear();

    for (const auto& eqHistory : eqHistoryList_) {

        QString historyEntryStr(eqHistory.date_.toString("dd MMM yyyy") + ": " + eqHistory.text_);
        numHistoryRows += setupScrollableText(fm, historyEntryStr, scrollableDisplayText_, numHistoryRows);
    }

    double scrollwidth = fm.boundingRect("X").width();

    if ((geometry().height() - 40)  < ((numHistoryRows + 2.5) * ROWHEIGHT ))
    {
        // set the scrollbar to the rhs
        scrollbar_->show();
        scrollbar_->setGeometry(geometry().width() - scrollwidth, ROWHEIGHT * 2.5, scrollwidth, geometry().height() - (ROWHEIGHT * 2.5) - 40);
        scrollbar_->setAreaHeight(numHistoryRows * ROWHEIGHT);
    }
    else
    {
        scrollbar_->hide();
    }
}

void
EquipmentHistory::wheelEvent(QGraphicsSceneWheelEvent* w)
{
    if (scrollbar_->canscroll) {

        scrollbar_->movePos(w->delta());
        w->accept();
    }
}

void
EquipmentHistory::itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {

    double rowY(ROWHEIGHT * 2.5);
    double rowWidth(geometry().width() - (ROWHEIGHT * 2));
    double rowHeight(geometry().height() - rowY);

    painter->setFont(parent->smallfont);
    painter->setPen(textColor_);

    // Scale scrollbarPosn based on ratio of displayed rows and eqHistoryList_ entries
    scrollPosn_ = (scrollbar_->pos() + (ROWHEIGHT/2)) / ROWHEIGHT;

    // don't paint on the edges
    painter->setClipRect(40, 40, geometry().width() - 80, geometry().height() - 80);

    int listPosn = 0;
    for (const auto& eqHistory : scrollableDisplayText_) {

        if (listPosn >= scrollPosn_) {

            painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), eqHistory);

            if (eqHistory.at(11) == ':') {
                painter->setPen(GColor(CPLOTMARKER));
                painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, rowHeight), eqHistory.mid(0, 12));
            }
            painter->setPen(textColor_);

            rowY += ROWHEIGHT;
        }
        listPosn++;
    }

    tileDisplayHeight_ = rowY;
}

//
// ---------------------------------------- Equipment Notes --------------------------------------------
//

EquipmentNotes::EquipmentNotes(ChartSpace* parent, const QString& name, const QString& notes) :
    CommonEquipmentItem(parent, name)
{
    // equipment summary item
    this->type = OverviewItemType::EQ_NOTES;
    notes_ = notes;

    scrollPosn_ = 0;

    configwidget_ = new OverviewEquipmentItemConfig(this);
    configwidget_->hide();

    scrollbar_ = new VScrollBar(this, parent);
    scrollbar_->show();
    configChanged(CONFIG_APPEARANCE);
}

void
EquipmentNotes::configChanged(qint32 cfg) {

    if (cfg & CONFIG_APPEARANCE) {
        textColor_ = (GCColor::luminance(RGBColor(color())) < 127) ? QColor(QColor(200, 200, 200)) : QColor(QColor(70, 70, 70));
    }
}

void
EquipmentNotes::showEvent(QShowEvent*)
{
    // wait for tile geometry to be defined
    itemGeometryChanged();
}

void
EquipmentNotes::itemGeometryChanged()
{
    QFontMetrics fm(parent->smallfont, parent->device());

    scrollableDisplayText_.clear();
    int numRowsInNotes = setupScrollableText(fm, notes_, scrollableDisplayText_);

    // set the scrollbar width
    double charWidth = fm.boundingRect("X").width();

    if ((geometry().height() - 40) < ((numRowsInNotes+2) * ROWHEIGHT))
    {
        // set the scrollbar to the rhs
        scrollbar_->show();
        scrollbar_->setGeometry(geometry().width() - charWidth, ROWHEIGHT * 2.5, charWidth, geometry().height() - (3 * ROWHEIGHT));
        scrollbar_->setAreaHeight(numRowsInNotes * ROWHEIGHT);
    }
    else
    {
        scrollbar_->hide();
    }
}

void
EquipmentNotes::wheelEvent(QGraphicsSceneWheelEvent* w)
{
    if (scrollbar_->canscroll) {

        scrollbar_->movePos(w->delta());
        w->accept();
    }
}

void
EquipmentNotes::itemPaint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {

    painter->setPen(textColor_);
    painter->setFont(parent->smallfont);

    double rowY(ROWHEIGHT * 2.5);
    double rowWidth(geometry().width() - (ROWHEIGHT * 2));

    scrollPosn_ = (scrollbar_->pos() + (ROWHEIGHT /2)) / ROWHEIGHT;

    // don't paint on the edges
    painter->setClipRect(40, 40, geometry().width() - 80, geometry().height() - 80);

    int listPosn = 0;
    for (const auto& eqNotes : scrollableDisplayText_) {

        if (listPosn >= scrollPosn_) {
            painter->drawText(QRectF(ROWHEIGHT, rowY, rowWidth, ROWHEIGHT), eqNotes);
            rowY += ROWHEIGHT;
        }
        listPosn++;
    }

    tileDisplayHeight_ = rowY;
}

