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

EquipmentNavigator::EquipmentNavigator(Context* context) :
        GcChartWindow(context), context_(context)
{
    fontHeight = QFontMetrics(QFont()).height();

    QVBoxLayout* mainLayout = new QVBoxLayout();
    setChartLayout(mainLayout);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(5 * dpiXFactor, 0, 0, 0);

    // setup the view
    eqTreeView_ = new EquipmentTreeView(context_, this, context_->mainWindow->equipmentModelMngr->equipModel_);
        
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
    if (state & CONFIG_FIELDS)     eqTreeView_->doItemsLayout();

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
    
    currentEqItem_ = context_->mainWindow->equipmentModelMngr->equipModel_->equipmentFromIndex(modelIndex);

    // Notify the new selected item
    context_->notifyEquipmentSelected(currentEqItem_);
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

    static QColor odtColour = GColor(CHEARTRATE).darker(130);
    static QColor LinkAndSharedColor = GColor(CPLOTTRACKER).lighter(120);

    static QColor inactiveTimeColour = GColor(CCADENCE).darker(180);
    static QColor timeColour = GColor(CCADENCE).lighter(110);
    static QColor distColor = GColor(CPLOTTRACKER).darker(110);
    static QColor bannerColor = distColor;

    static QColor indirectSelectedTextColor = distColor;
    static QColor indirectSelectedBkgdColor = GCColor::alternateColor(GColor(CPLOTBACKGROUND));

    QColor userTextColor = distColor;
    QColor userTimeColor = timeColour;

    bool indirectlySelected(false);
    EquipmentNode* eqNode = eqModel_->equipmentFromIndex(index);

    // render text based upon hierarchy & types
    if (eqNode) {

        switch (eqNode->getEqNodeType()) {

        case eqNodeType::EQ_TIME_ITEM: {
            userTextColor = timeColour;
        } break;

        case eqNodeType::EQ_ITEM_REF: {
            if (static_cast<EquipmentRef*>(eqNode)->eqSharedDistNode_ == nullptr) {
                userTextColor = odtColour;
            } else {
                if (static_cast<EquipmentRef*>(eqNode)->eqSharedDistNode_ == eqNav_->currentEqItem_) {
                    indirectlySelected = true;
                } else {
                    userTextColor = distColor;
                }
            }
        } break;

        case eqNodeType::EQ_SHARED:
        case eqNodeType::EQ_LINK: {
            userTextColor = LinkAndSharedColor;
        } break;

        case eqNodeType::EQ_DIST_ITEM: {
            userTextColor = distColor;
        } break;

        case eqNodeType::EQ_BANNER: {
            userTextColor = bannerColor;
        } break;

        case eqNodeType::EQ_SHARED_DIST_ITEM: {
            if (static_cast<EquipmentSharedDistanceItem*>(eqNode)->linkedRefs_.indexOf(eqNav_->currentEqItem_) != -1) {
                indirectlySelected = true;
            } else {
                userTextColor = distColor;
            }
        } break;

        default: {
            qDebug() << "Cannot determine color for unknown equipment type!";
        }break;
        }
    }

    // save original painter values to restore later
    QPen isColor = painter->pen();
    QFont isFont = painter->font();

    // clear first
    drawDisplay(painter, myOption, myOption.rect, "");

    // not selected, so apply any required overr
    if (indirectlySelected) {

        myOption.rect.setHeight(eqNav_->fontHeight + 1);
        painter->fillRect(myOption.rect, indirectSelectedBkgdColor);
        userTextColor = indirectSelectedTextColor;
        userTimeColor = indirectSelectedTextColor;

    } else if (!selected) {

        switch (eqNode->getEqNodeType()) {

        case eqNodeType::EQ_DIST_ITEM: {

            // overwrite the date in either time color, inactive time or overdistance if range invalid
            if ((static_cast<EquipmentDistanceItem*>(eqNode)->overDistance()) ||
                (!static_cast<EquipmentDistanceItem*>(eqNode)->rangeIsValid())) {
                userTextColor = odtColour;
                userTimeColor = odtColour;
            } else {
                if (static_cast<EquipmentDistanceItem*>(eqNode)->isWithin(QDateTime::currentDateTime())) {
                    userTimeColor = timeColour;
                } else {
                    userTimeColor = inactiveTimeColour;
                }
            }
        } break;

        case eqNodeType::EQ_SHARED_DIST_ITEM: {
            if (static_cast<EquipmentSharedDistanceItem*>(eqNode)->overDistance()) userTextColor = odtColour;
        } break;

        case eqNodeType::EQ_ITEM_REF: {

            // overwrite the date in either time color, inactive time or overdistance if range invalid
            if ((static_cast<EquipmentRef*>(eqNode)->eqSharedDistNode_->overDistance()) ||
                (!static_cast<EquipmentRef*>(eqNode)->rangeIsValid())) {
                userTextColor = odtColour;
                userTimeColor = odtColour;
            } else {
                if (static_cast<EquipmentRef*>(eqNode)->isWithin(QDateTime::currentDateTime())) {
                    userTimeColor = timeColour;
                }
                else {
                    userTimeColor = inactiveTimeColour;
                }
            }
        } break;

        case eqNodeType::EQ_TIME_ITEM: {
            if (static_cast<EquipmentTimeItem*>(eqNode)->overTime()) userTextColor = odtColour;
        } break;

        default: {
            // No overrides required.
        }break;
        }
    }
   
    // Draw text 
    if (((eqNode->getEqNodeType() == eqNodeType::EQ_ITEM_REF) &&
        (static_cast<EquipmentRef*>(eqNode)->isTimeRestrictionSet())) ||
        ((eqNode->getEqNodeType() == eqNodeType::EQ_DIST_ITEM) &&
            (static_cast<EquipmentDistanceItem*>(eqNode)->isTimeRestrictionSet())))
    {
        static QRegularExpression rx("\\~");
        QStringList strList = value.split(rx);

        // draw the whole string without any '~'
        painter->setPen(userTextColor);
        painter->drawText(rect, value.remove(QChar('~')));

        // draw the time part of the string
        painter->setPen(userTimeColor);
        painter->drawText(rect, strList[0]);

    }
    else {

        // draw the whole string
        painter->setPen(userTextColor);
        painter->drawText(rect, value);
    }
    
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

    connect(context_, SIGNAL(equipmentSelected(EquipmentNode*)), this, SLOT(equipmentSelected(EquipmentNode*)));
}

EquipmentTreeView::~EquipmentTreeView()
{
    delete delegate_;
}

void
EquipmentTreeView::equipmentSelected(EquipmentNode*)
{
    update();
}


