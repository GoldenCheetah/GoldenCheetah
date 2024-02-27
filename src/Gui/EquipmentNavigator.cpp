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

#include "Context.h"
#include "Colors.h"
#include "EquipmentModelManager.h"
#include "MainWindow.h"
#include "EquipmentNavigator.h"
#include "AbstractView.h"
#include "HelpWhatsThis.h"

#include <QtGui>
#include <QString>
#include <QTreeView>
#include <QStyle>
#include <QStyleFactory>
#include <QScrollBar>

// There is a common selected equipment shared between both navigators so that
// the references of selected equipment can be highlight.
EquipmentNode* EquipmentNavigator::currentEqItem_ = nullptr;

EquipmentNavigator::EquipmentNavigator(Context* context, bool eqListView) :
		GcChartWindow(context), context_(context), eqListView_(eqListView)
{
    fontHeight = QFontMetrics(QFont()).height();

	QVBoxLayout* mainLayout = new QVBoxLayout();
    setChartLayout(mainLayout);
    mainLayout->setSpacing(0);
	mainLayout->setContentsMargins(5 * dpiXFactor, 0, 0, 0);

    // setup views
	if (eqListView)
		eqTreeView_ = new EquipmentTreeView(context_, this, context_->mainWindow->equipmentModelMngr->equipModel_); 
	else
		eqTreeView_ = new EquipmentTreeView(context_, this, context_->mainWindow->equipmentModelMngr->refsModel_);
		
    mainLayout->addWidget(eqTreeView_);

    HelpWhatsThis* helpeqTreeView = new HelpWhatsThis(eqTreeView_);
	eqTreeView_->setWhatsThis(helpeqTreeView->getWhatsThisText(HelpWhatsThis::SideBarEquipmentView_Eq));
	eqTreeView_->show();
	eqTreeView_->doItemsLayout();

    // refresh when config changes
    connect(context_, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    connect(eqTreeView_, SIGNAL(eqRowSelected(QItemSelection)), this, SLOT(eqSelectionChanged(QItemSelection)));
	connect(context_, SIGNAL(eqRecalculationEnd()), this, SLOT(eqRecalculationEnd()));
    connect(eqTreeView_, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(showTreeContextMenuPopup(const QPoint&)));

    // we accept drag and drop operations
    setAcceptDrops(true);

    // lets go
    configChanged(CONFIG_APPEARANCE | CONFIG_NOTECOLOR | CONFIG_FIELDS);
}

EquipmentNavigator::~EquipmentNavigator()
{
	delete eqTreeView_;
}

void
EquipmentNavigator::configChanged(qint32 state)
{
    fontHeight = QFontMetrics(QFont()).height();

#ifndef Q_OS_MAC
	eqTreeView_->setStyleSheet(AbstractView::ourStyleSheet());
#endif

    // if the fields changed we need to reset indexes etc
    if (state & CONFIG_FIELDS) 	eqTreeView_->doItemsLayout();

	setWidth();
}

void
EquipmentNavigator::widthChanged()
{
	setWidth();
}

void
EquipmentNavigator::resizeEvent(QResizeEvent*)
{
    // This is a main window .. we get told to resize
    // by the equipment view that the splitter has moved
}

void EquipmentNavigator::setWidth()
{
	pwidth_ = geometry().width();
	eqTreeView_->setColumnWidth(0, pwidth_);
}

void
EquipmentNavigator::showEvent(QShowEvent*)
{
	setWidth();
}

void
EquipmentNavigator::eqRecalculationEnd()
{
	// Update the presentation of the equipment navigator
	eqTreeView_->doItemsLayout();
}

void
EquipmentNavigator::eqSelectionChanged(QItemSelection selected)
{
    if (selected.isEmpty()) return;

    QModelIndex ref = selected.indexes().first();
    QModelIndex modelIndex = eqTreeView_->model()->index(ref.row(), 0, ref.parent());
	
	if (eqListView_)
		currentEqItem_ = context_->mainWindow->equipmentModelMngr->refsModel_->equipmentFromIndex(modelIndex);
	else
		currentEqItem_ = context_->mainWindow->equipmentModelMngr->equipModel_->equipmentFromIndex(modelIndex);

    // Notify the new selected item
	context_->notifyEquipmentSelected(currentEqItem_, eqListView_);
}

void
EquipmentNavigator::showTreeContextMenuPopup(const QPoint& pos)
{
    emit customContextMenuRequested(eqTreeView_->mapToGlobal(pos + QPoint(0, eqTreeView_->header()->geometry().height())));
}

//
// -------------------------------- EquipmentNavigatorCelldelegate --------------------------------
//

EquipmentNavigatorCellDelegate::EquipmentNavigatorCellDelegate(EquipmentNavigator* eqNav, EquipmentModel* eqModel, QObject* parent) :
    QItemDelegate(parent), eqNav_(eqNav), eqModel_(eqModel)
{
}

QSize EquipmentNavigatorCellDelegate::sizeHint(const QStyleOptionViewItem& /*option*/, const QModelIndex&  /*index*/ ) const
{
    QSize s;

    s.setHeight(eqNav_->fontHeight + 1);

    return s;
}

// anomalies are underlined in red, otherwise straight paintjob
void EquipmentNavigatorCellDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
											const QModelIndex& index) const
{
    // state of item
    bool selected = option.state & QStyle::State_Selected;

    QString value = index.model()->data(index, Qt::DisplayRole).toString();
    QStyleOptionViewItem myOption = option;
    myOption.displayAlignment = Qt::AlignLeft | Qt::AlignTop;
	QRect rect(myOption.rect.x(), myOption.rect.y() + 1, myOption.rect.width(), myOption.rect.height());

    // render text based upon hierarchy & types
     QColor userColor = GColor(CPLOTTRACKER).darker(150);

	static QColor odtColour = GColor(CHEARTRATE).darker(130);
	static QColor rootTimeColour = GColor(CCADENCE).lighter(110);
	static QColor rootDistColor = GColor(CPLOTTRACKER).lighter(120);
	static QColor timeColour = GColor(CCADENCE).darker(140);
	static QColor distColor = GColor(CPLOTTRACKER).darker(110);
	static QColor selectTxtColor = distColor;
	static QColor selectBkgdColor = GCColor::alternateColor(GColor(CPLOTBACKGROUND));

	EquipmentNode* eqNode = eqModel_->equipmentFromIndex(index);

	if (eqNode) {

		switch (eqNode->getEqNodeType()) {

		case eqNodeType::EQ_TIME_ITEM: {
			if (static_cast<EquipmentTimeItem*>(eqNode)->overTime())
				userColor = odtColour;
			else
				if (eqNode->getParentItem()->getEqNodeType() == eqNodeType::EQ_ROOT)
					userColor = rootTimeColour;
				else
					userColor = timeColour;
		} break;

		case eqNodeType::EQ_TIME_SPAN: {
			userColor = timeColour;
		} break;

		case eqNodeType::EQ_ITEM_REF: {
			if (static_cast<EquipmentRef*>(eqNode)->eqDistNode_ == nullptr)
				userColor = odtColour;
			else
				if (static_cast<EquipmentRef*>(eqNode)->eqDistNode_ == eqNav_->currentEqItem_) {

					painter->fillRect(myOption.rect, selectBkgdColor);
					userColor = selectTxtColor;
				}
				else
					if (static_cast<EquipmentRef*>(eqNode)->eqDistNode_->overDistance())
						userColor = odtColour;
					else
						userColor = distColor;
		} break;

		case eqNodeType::EQ_LINK: {
			if (eqNode->getParentItem()->getEqNodeType() == eqNodeType::EQ_ROOT)
				userColor = rootDistColor;
			else
				userColor = distColor;
		} break;

		case eqNodeType::EQ_DIST_ITEM: {
			if (static_cast<EquipmentDistanceItem*>(eqNode)->linkedRefs_.indexOf(eqNav_->currentEqItem_) != -1) {
				painter->fillRect(myOption.rect, selectBkgdColor);
				userColor = selectTxtColor;
			}
			else
				if (static_cast<EquipmentDistanceItem*>(eqNode)->overDistance())
					userColor = odtColour;
				else
					if (eqNode->getParentItem()->getEqNodeType() == eqNodeType::EQ_ROOT)
						userColor = rootDistColor;
					else
						userColor = distColor;
		} break;

		default: {
			qDebug() << "Cannot determine color for unknown equipment type!";
		}break;
		}
	}

    // clear first
    drawDisplay(painter, myOption, myOption.rect, "");

	// save original painter values to restore later
	QPen isColor = painter->pen();
	QFont isFont = painter->font();

    myOption.rect.setHeight(eqNav_->fontHeight + 1);

    if (!selected) {
        // not selected, so invert ride plot color
        painter->setPen(userColor);
    }

    painter->drawText(rect, value);
    
	// restore original painter values
    painter->setPen(isColor);
    painter->setFont(isFont);
}

//
// -------------------------------- EquipmentTreeView --------------------------------
//

EquipmentTreeView::EquipmentTreeView(Context *context, EquipmentNavigator* eqNav, EquipmentModel* eqModel) :
	context_(context), QTreeView(eqNav)
{
	delegate_ = new EquipmentNavigatorCellDelegate(eqNav, eqModel);

    setModel(eqModel);
    setItemDelegate(delegate_);

	// Set the QTreeView attributes
	setDragEnabled(true);
	setAnimated(false);
    viewport()->setAcceptDrops(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
	setDragDropOverwriteMode(false);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setDragDropMode(QAbstractItemView::DragDrop);
	setDropIndicatorShown(true);
	expandToDepth(10);
	setHeaderHidden(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);


#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    setAlternatingRowColors(false);
    setContextMenuPolicy(Qt::CustomContextMenu);

#ifdef Q_OS_WIN
    QStyle* cde = QStyleFactory::create(OS_STYLE);
    verticalScrollBar()->setStyle(cde);
#endif
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    setFrameStyle(QFrame::NoFrame);

    connect(context_, SIGNAL(equipmentSelected(EquipmentNode*, bool)), this, SLOT(equipmentSelected(EquipmentNode*, bool)));
}

EquipmentTreeView::~EquipmentTreeView()
{
	delete delegate_;
}

void
EquipmentTreeView::equipmentSelected(EquipmentNode*, bool)
{
    update();
}


