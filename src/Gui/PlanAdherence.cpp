/*
 * Copyright (c) 2026 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "PlanAdherence.h"

#include <QHeaderView>
#include <QToolTip>

#include "Colors.h"
#include "Settings.h"

static void sendFakeHoverMove(QWidget *viewport, const QPoint &pos);
static void sendFakeHoverLeave(QWidget *viewport);


//////////////////////////////////////////////////////////////////////////////
// StatisticBox

StatisticBox::StatisticBox
(const QString &title, const QString &description, DisplayState startupPreferred, QWidget *parent)
: QFrame(parent),
  description(description),
  startupPreferredState(startupPreferred),
  currentPreferredState(startupPreferred),
  currentState(startupPreferred)
{
    setObjectName("statbox");

    setFrameShape(QFrame::StyledPanel);
    setFrameShadow(QFrame::Raised);

    valueLabel = new QLabel("0", this);
    valueLabel->setAlignment(Qt::AlignLeft);

    QFont valueFont = valueLabel->font();
    valueFont.setPointSizeF(valueFont.pointSizeF() * 1.5);
    valueFont.setBold(true);
    valueLabel->setFont(valueFont);

    titleLabel = new QLabel(title, this);
    titleLabel->setObjectName("stattitle");
    titleLabel->setAlignment(Qt::AlignLeft);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 0.7);
    titleLabel->setFont(titleFont);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12 * dpiXFactor, 8 * dpiYFactor, 12 * dpiXFactor, 8 * dpiYFactor);
    layout->addWidget(valueLabel);
    layout->addWidget(titleLabel);

    setMouseTracking(true);
}


void
StatisticBox::setValue
(const QString &primary, const QString &secondary)
{
    primaryValue = primary;
    secondaryValue = secondary;
    valueLabel->setText(currentState == DisplayState::Secondary && ! secondaryValue.isEmpty() ? secondaryValue : primaryValue);
    QString html = QString("<div style='padding: %1px;'>").arg(4 * dpiXFactor);
    html += QString("<div style='font-weight: bold; font-size: 120%;'>%1</div><br/>").arg(description);
    if (secondaryValue.isEmpty()) {
        html += QString("%1").arg(primaryValue);
        setCursor(Qt::ArrowCursor);
    } else {
        html += QString("%1 (%2)").arg(primaryValue).arg(secondaryValue);
        setCursor(Qt::PointingHandCursor);
    }
    html += "</div>";
    setToolTip(html);
}


void
StatisticBox::setPreferredState
(const DisplayState &state)
{
    startupPreferredState = state;
    currentPreferredState = state;
    if (! secondaryValue.isEmpty()) {
        currentState = state;
        valueLabel->setText(currentState == DisplayState::Secondary ? secondaryValue : primaryValue);
    }
}


void
StatisticBox::changeEvent
(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange) {
        QWidget *parent = parentWidget();
        if (! parent) {
            return;
        }
        QPalette basePalette = parent->palette();
        QColor alt = basePalette.color(QPalette::AlternateBase);
        QColor border = GCColor::inactiveColor(alt, 0.25);
        QColor title = GCColor::inactiveColor(basePalette.color(QPalette::Text), 1.0);
        QString newStyle = QString(
            "#statbox {"
            "  background-color: %1;"
            "  border: 1px solid %2;"
            "}"
            "#stattitle {"
            "  color: %3;"
            "}"
        ).arg(alt.name())
         .arg(border.name())
         .arg(title.name());
        if (styleSheet() != newStyle) {
            setStyleSheet(newStyle);
        }
    }
    QWidget::changeEvent(event);
}


void
StatisticBox::mousePressEvent
(QMouseEvent *event)
{
    Q_UNUSED(event);

    if (secondaryValue.isEmpty()) {
        QFrame::mousePressEvent(event);
        return;
    }

    currentState = (currentState == DisplayState::Primary) ? DisplayState::Secondary : DisplayState::Primary;
    valueLabel->setText(currentState == DisplayState::Secondary ? secondaryValue : primaryValue);
    currentPreferredState = currentState;
}


//////////////////////////////////////////////////////////////////////////////
// JointLineTree

JointLineTree::JointLineTree
(QWidget *parent)
: QTreeWidget(parent)
{
    setSelectionMode(QAbstractItemView::NoSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAlternatingRowColors(false);
    setUniformRowHeights(false);
    setItemsExpandable(false);
    setRootIsDecorated(false);
    setIndentation(0);
    setFocusPolicy(Qt::NoFocus);
    setFrameShape(QFrame::NoFrame);
    header()->setVisible(false);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
}


bool
JointLineTree::isRowHovered
(const QModelIndex &index) const
{
    return    (   hoveredIndex.isValid()
               && hoveredIndex.model() == index.model()
               && hoveredIndex == index.siblingAtColumn(0))
           || (   contextMenuIndex.isValid()
               && contextMenuIndex.model() == index.model()
               && contextMenuIndex == index.siblingAtColumn(0));
}


QColor
JointLineTree::getHoverBG
() const
{
    QColor bgColor = palette().color(QPalette::Base);
    QColor hoverBG = palette().color(QPalette::Active, QPalette::Highlight);
    hoverBG.setAlphaF(0.2f);
    return GCColor::blendedColor(hoverBG, bgColor);
}


QColor
JointLineTree::getHoverHL
() const
{
    QColor bgColor = palette().color(QPalette::Base);
    QColor hoverHL = palette().color(QPalette::Active, QPalette::Highlight);
    hoverHL.setAlphaF(0.6f);
    return GCColor::blendedColor(hoverHL, bgColor);
}


void
JointLineTree::mouseMoveEvent
(QMouseEvent *event)
{
    if (! contextMenuIndex.isValid()) {
        updateHoveredIndex(event->pos());
    }
    QTreeWidget::mouseMoveEvent(event);
}


void
JointLineTree::wheelEvent
(QWheelEvent *event)
{
    if (! contextMenuIndex.isValid()) {
        updateHoveredIndex(event->position().toPoint());
    }
    QTreeWidget::wheelEvent(event);
}


void
JointLineTree::leaveEvent
(QEvent *event)
{
    if (! contextMenuIndex.isValid()) {
        clearHover();
    }
    QTreeWidget::leaveEvent(event);
}


void
JointLineTree::enterEvent
(QEnterEvent *event)
{
    if (! contextMenuIndex.isValid()) {
        QPointer<JointLineTree> self(this);
        QTimer::singleShot(0, this, [self]() {
            if (! self) {
                return;
            }
            QPoint pos = self->viewport()->mapFromGlobal(QCursor::pos());
            if (self->viewport()->rect().contains(pos)) {
                self->updateHoveredIndex(pos);
                sendFakeHoverMove(self->viewport(), pos);
            }
        });
    }
    QTreeWidget::enterEvent(event);
}


void
JointLineTree::contextMenuEvent
(QContextMenuEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (! index.isValid()) {
        return;
    }

    index = index.siblingAtColumn(0);
    QMenu *contextMenu = createContextMenu(index);
    if (contextMenu == nullptr) {
        return;
    }

    contextMenuIndex = QPersistentModelIndex(index);
    hoveredIndex = contextMenuIndex;

    QRect rowRect = visualRect(index);
    rowRect.setLeft(0);
    rowRect.setRight(viewport()->width());
    viewport()->update(rowRect);

    contextMenu->exec(event->globalPos());

    delete contextMenu;

    QPersistentModelIndex oldContextIndex = contextMenuIndex;
    contextMenuIndex = QPersistentModelIndex();

    QPoint currentPos = viewport()->mapFromGlobal(QCursor::pos());
    if (viewport()->rect().contains(currentPos)) {
        updateHoveredIndex(currentPos);
        sendFakeHoverMove(viewport(), currentPos);
    } else {
        if (oldContextIndex.isValid()) {
            QRect oldRowRect = visualRect(oldContextIndex);
            oldRowRect.setLeft(0);
            oldRowRect.setRight(viewport()->width());
            viewport()->update(oldRowRect);
        }
        hoveredIndex = QPersistentModelIndex();
        sendFakeHoverLeave(viewport());
    }
}


void
JointLineTree::changeEvent
(QEvent *event)
{
    if (event->type() == QEvent::ActivationChange) {
        if (! isActiveWindow()) {
            if (! contextMenuIndex.isValid()) {
                clearHover();
                sendFakeHoverLeave(viewport());
            }
        } else {
            QPointer<JointLineTree> self(this);
            QTimer::singleShot(0, this, [self]() {
                if (! self) {
                    return;
                }
                QPoint pos = self->viewport()->mapFromGlobal(QCursor::pos());
                if (self->viewport()->rect().contains(pos)) {
                    self->updateHoveredIndex(pos);
                    sendFakeHoverMove(self->viewport(), pos);
                }
            });
        }
    } else if (event->type() == QEvent::PaletteChange) {
        clearHover();
        QPointer<JointLineTree> self(this);
        QTimer::singleShot(0, this, [self]() {
            if (! self) {
                return;
            }
            QPalette treePal = self->palette();
            treePal.setColor(QPalette::Base, treePal.color(QPalette::Base));
            self->setPalette(treePal);
            self->viewport()->setPalette(treePal);
            self->setAutoFillBackground(false);
        });
    }

    QWidget::changeEvent(event);
}


bool
JointLineTree::viewportEvent
(QEvent *event)
{
    switch (event->type()) {
    case QEvent::HoverEnter:
    case QEvent::HoverMove:
        if (! contextMenuIndex.isValid()) {
            QHoverEvent *he = static_cast<QHoverEvent*>(event);
            updateHoveredIndex(he->position().toPoint());
        }
        break;
    case QEvent::HoverLeave:
    case QEvent::Leave: {
        if (contextMenuIndex.isValid()) {
            return true;
        }
        bool result = QTreeWidget::viewportEvent(event);
        clearHover();
        return result;
    }
    case QEvent::ToolTip: {
        QHelpEvent *helpEvent = static_cast<QHelpEvent*>(event);
        QModelIndex hoverIndex = indexAt(helpEvent->pos());
        if (! hoverIndex.isValid()) {
            return QTreeWidget::viewportEvent(event);
        }
        QString toolTipText = createToolTipText(hoverIndex);
        if (! toolTipText.isEmpty()) {
            QToolTip::showText(helpEvent->globalPos(), toolTipText, this);
            return true;
        }
        QToolTip::hideText();
        event->ignore();
        return true;
    }
    default:
        break;
    }
    return QTreeWidget::viewportEvent(event);
}


void
JointLineTree::drawRow
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool isHovered = isRowHovered(index);
    QStyleOptionViewItem opt = option;
    if (isHovered) {
        painter->save();

        QRect hoverRect(option.rect);
        hoverRect.setX(hoverRect.x() + 4 * dpiXFactor);
        painter->fillRect(hoverRect, getHoverBG());
        QRect markerRect(option.rect);
        markerRect.setWidth(4 * dpiXFactor);
        painter->fillRect(markerRect, getHoverHL());

        painter->restore();
        opt.state |= QStyle::State_MouseOver;
    }
    QTreeWidget::drawRow(painter, opt, index);
}


void
JointLineTree::clearHover
()
{
    if (hoveredIndex.isValid()) {
        QRect oldRect = visualRect(hoveredIndex);
        oldRect.setLeft(0);
        oldRect.setRight(viewport()->width());
        viewport()->update(oldRect);
        QToolTip::hideText();

        hoveredIndex = QPersistentModelIndex();
    }
}


void
JointLineTree::updateHoveredIndex
(const QPoint &pos)
{
    QModelIndex index = indexAt(pos);

    if (index.isValid()) {
        index = index.siblingAtColumn(0);
    }

    if (index != hoveredIndex) {
        QToolTip::hideText();
        QPersistentModelIndex oldIndex = hoveredIndex;
        hoveredIndex = QPersistentModelIndex(index);

        if (oldIndex.isValid()) {
            QRect oldRect = visualRect(oldIndex);
            oldRect.setLeft(0);
            oldRect.setRight(viewport()->width());
            viewport()->update(oldRect);
        }

        if (hoveredIndex.isValid()) {
            QRect newRect = visualRect(hoveredIndex);
            newRect.setLeft(0);
            newRect.setRight(viewport()->width());
            viewport()->update(newRect);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////
// PlanAdherenceOffsetHandler

void
PlanAdherenceOffsetHandler::setMinAllowedOffset
(int minOffset)
{
    minAllowedOffset = minOffset;
}


void
PlanAdherenceOffsetHandler::setMaxAllowedOffset
(int maxOffset)
{
    maxAllowedOffset = maxOffset;
}


void
PlanAdherenceOffsetHandler::setMinEntryOffset
(int minOffset)
{
    minEntryOffset = minOffset;
}


void
PlanAdherenceOffsetHandler::setMaxEntryOffset
(int maxOffset)
{
    maxEntryOffset = maxOffset;
}


bool
PlanAdherenceOffsetHandler::hasMinOverflow
() const
{
    return minEntryOffset < minAllowedOffset;
}


bool
PlanAdherenceOffsetHandler::hasMaxOverflow
() const
{
    return maxEntryOffset > maxAllowedOffset;
}


int
PlanAdherenceOffsetHandler::minVisibleOffset
() const
{
    return std::max(minEntryOffset, minAllowedOffset);
}


int
PlanAdherenceOffsetHandler::maxVisibleOffset
() const
{
    return std::min(maxEntryOffset, maxAllowedOffset);
}


bool
PlanAdherenceOffsetHandler::isMinOverflow
(int offset) const
{
    return offset < minAllowedOffset;
}


bool
PlanAdherenceOffsetHandler::isMaxOverflow
(int offset) const
{
    return offset > maxAllowedOffset;
}


int
PlanAdherenceOffsetHandler::indexFromOffset
(int offset) const
{
    if (isMaxOverflow(offset)) {
        return numCells() - 1;
    } else if (isMinOverflow(offset)) {
        return 0;
    }
    int index = offset - minVisibleOffset();
    if (hasMinOverflow()) {
        ++index;
    }
    return index;
}


int
PlanAdherenceOffsetHandler::numCells
() const
{
    int cellCount = maxVisibleOffset() - minVisibleOffset() + 1;
    if (hasMinOverflow()) {
        ++cellCount;
    }
    if (hasMaxOverflow()) {
        ++cellCount;
    }
    return cellCount;
}


//////////////////////////////////////////////////////////////////////////////
// PlanAdherenceTitleDelegate

PlanAdherenceTitleDelegate::PlanAdherenceTitleDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
PlanAdherenceTitleDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    PlanAdherenceTree const * const tree = qobject_cast<PlanAdherenceTree const * const>(opt.widget);
    if (tree == nullptr) {
        return;
    }

    PlanAdherenceEntry entry = index.siblingAtColumn(0).data(PlanAdherence::DataRole).value<PlanAdherenceEntry>();
    QColor textColor = opt.palette.color(QPalette::Active, QPalette::Text);
    QColor iconColor = entry.color;
    if (opt.state & QStyle::State_MouseOver) {
        iconColor = GCColor::invertColor(tree->getHoverBG());
        textColor = iconColor;
    }

    QFont titleFont = painter->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 0.9);
    QFontMetrics titleFM(titleFont);
    QFont dateFont = titleFont;
    dateFont.setWeight(QFont::ExtraLight);

    const int lineHeight = titleFM.height();
    const int iconWidth = 2 * lineHeight + lineSpacing;
    const QSize pixmapSize(iconWidth, iconWidth);
    const int paddingTop = (opt.rect.height() - 2 * lineHeight - lineSpacing) / 2;
    const int iconX = opt.rect.x() + leftPadding;
    const int textX = iconX + iconWidth + iconTextSpacing;
    const int line1Y = opt.rect.top() + paddingTop;
    const int maxTextWidth = opt.rect.width() - (leftPadding + iconWidth + iconTextSpacing);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    painter->setPen(textColor);

    QPixmap pixmap = svgAsColoredPixmap(entry.iconFile, pixmapSize, iconInnerSpacing, iconColor);
    painter->drawPixmap(iconX, line1Y, pixmap);
    painter->setFont(titleFont);
    QRect textRect(textX, line1Y, maxTextWidth, lineHeight);
    painter->drawText(textRect, Qt::AlignLeft, titleFM.elidedText(entry.titlePrimary, Qt::ElideRight, textRect.width()));
    textRect.translate(0, lineHeight + lineSpacing);
    painter->setFont(dateFont);
    QLocale locale;
    QString dateString = locale.toString(entry.date, tr("ddd dd.MM."));
    if (entry.shiftOffset != std::nullopt && entry.shiftOffset.value() != 0) {
        QDate shiftedDate = entry.date.addDays(entry.shiftOffset.value());
        dateString += " → " + locale.toString(shiftedDate, tr("dd.MM."));
    }
    painter->drawText(textRect, Qt::AlignLeft, dateString);

    painter->restore();
}


QSize
PlanAdherenceTitleDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    PlanAdherenceEntry entry = index.siblingAtColumn(0).data(PlanAdherence::DataRole).value<PlanAdherenceEntry>();

    QFont font = option.font;
    font.setPointSizeF(font.pointSizeF() * 0.9);
    const QFontMetrics fm(font);
    const int lineHeight = fm.height();
    const int iconWidth = 2 * lineHeight + lineSpacing;
    const int titleLength = fm.horizontalAdvance(entry.titlePrimary) * 1.15;
    QLocale locale;
    QString dateString = locale.toString(entry.date, tr("ddd dd.MM."));
    if (entry.shiftOffset != std::nullopt && entry.shiftOffset.value() != 0) {
        QDate shiftedDate = entry.date.addDays(entry.shiftOffset.value());
        dateString += " → " + locale.toString(shiftedDate, tr("dd.MM."));
    }
    const int dateLength = fm.horizontalAdvance(dateString) * 1.15;
    int width = leftPadding + iconWidth + std::max(titleLength, dateLength);

    return QSize(std::min(width, maxWidth), 2 * (vertMargin + lineHeight) + lineSpacing);
}


//////////////////////////////////////////////////////////////////////////////
// PlanAdherenceOffsetDelegate

PlanAdherenceOffsetDelegate::PlanAdherenceOffsetDelegate
(QObject *parent)
: QStyledItemDelegate(parent), PlanAdherenceOffsetHandler()
{
}


void
PlanAdherenceOffsetDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int cellCount = numCells();
    if (cellCount == 0) {
        return;
    }

    QColor bgColor = option.palette.color(QPalette::Base);

    PlanAdherenceEntry entry = index.siblingAtColumn(0).data(PlanAdherence::DataRole).value<PlanAdherenceEntry>();
    int shiftOffset = entry.shiftOffset != std::nullopt ? entry.shiftOffset.value() : 0;
    int minEntryOffset = std::min(0, shiftOffset);
    int maxEntryOffset = std::max(0, shiftOffset);
    bool hasActual = entry.actualOffset != std::nullopt;
    int actualOffset;
    if (hasActual) {
        actualOffset = entry.actualOffset.value();
        minEntryOffset = std::min(minEntryOffset, actualOffset);
        maxEntryOffset = std::max(maxEntryOffset, actualOffset);
    }
    const int availableHeight = option.rect.height() - 2 * vertMargin;
    const int availableWidth = option.rect.width() - 2 * horMargin;
    const int availableLeft = option.rect.left() + horMargin;
    const float cellWidth = availableWidth / cellCount;
    const float cellHorCenter = cellWidth / 2.0;
    const int radius = availableHeight / 4 - 2 * lineSpacing;
    const int vertCenter = option.rect.top() + option.rect.height() / 2;
    const int vertTop = vertCenter - availableHeight / 4;
    const int vertBottom = vertCenter + availableHeight / 4;

    int day0X = (indexFromOffset(0) + 1) * cellWidth - cellHorCenter + availableLeft;
    int shiftX = (indexFromOffset(shiftOffset) + 1) * cellWidth - cellHorCenter + availableLeft;
    int actualX = day0X;
    if (hasActual) {
        actualX = (indexFromOffset(actualOffset) + 1) * cellWidth - cellHorCenter + availableLeft;
    }
    int day0Y = vertCenter;
    int shiftY = vertCenter;
    int actualY = vertCenter;
    if (hasActual) {
        if (actualOffset == 0) {
            day0Y = vertTop;
            actualY = vertBottom;
        } else if (   actualOffset == shiftOffset
                   || (   actualOffset < minAllowedOffset
                       && shiftOffset < minAllowedOffset)
                   || (   actualOffset > maxAllowedOffset
                       && shiftOffset > maxAllowedOffset)) {
            shiftY = vertTop;
            actualY = vertBottom;
        }
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (! (option.state & QStyle::State_MouseOver)) {
        QColor overflowbg(option.palette.color(QPalette::Active, QPalette::AlternateBase));
        if (hasMinOverflow()) {
            painter->fillRect(QRect(availableLeft, option.rect.top(), cellWidth, option.rect.height()), overflowbg);
        }
        if (hasMaxOverflow()) {
            painter->fillRect(QRect(availableLeft + (cellCount - 1) * cellWidth, option.rect.top(), cellWidth, option.rect.height()), overflowbg);
        }
    }

    painter->save();
    QColor day0LineFg(option.palette.color(QPalette::Active, QPalette::Text));
    day0LineFg.setAlpha(20);
    day0LineFg = GCColor::blendedColor(day0LineFg, bgColor);
    QPen day0LinePen(day0LineFg);
    day0LinePen.setWidth(radius / 3);
    painter->setPen(day0LinePen);
    painter->drawLine(day0X, option.rect.top(), day0X, option.rect.bottom());
    painter->restore();

    if (minEntryOffset != maxEntryOffset) {
        const int leftX = (indexFromOffset(minEntryOffset) + 1) * cellWidth - cellHorCenter + availableLeft;
        const int rightX = (indexFromOffset(maxEntryOffset) + 1) * cellWidth - cellHorCenter + availableLeft;
        QColor connectorColor(option.palette.color(QPalette::Disabled, QPalette::Text));
        if (hasActual) {
            connectorColor.setAlpha(150);
        } else {
            connectorColor.setAlpha(75);
        }
        QPen pen(GCColor::blendedColor(connectorColor, bgColor));
        pen.setWidth(radius / 1.1);
        pen.setCapStyle(Qt::RoundCap);
        painter->setPen(pen);
        painter->drawLine(QPoint(leftX + 2 * radius, vertCenter), QPoint(rightX - 2 * radius, vertCenter));
    }
    painter->setPen(Qt::NoPen);

    if (hasActual) {
        painter->setBrush(GCColor::getSuccessColor(option.palette));
        painter->drawEllipse(QPoint(actualX, actualY), radius, radius);
    }

    QColor solidPlannedColor = GColor(CCALPLANNED);
    if (shiftOffset != 0) {
        QColor fadedPlannedColor(solidPlannedColor);
        fadedPlannedColor.setAlpha(100);
        fadedPlannedColor = GCColor::blendedColor(fadedPlannedColor, bgColor);
        painter->setBrush(fadedPlannedColor);
        painter->drawEllipse(QPoint(shiftX, shiftY), radius, radius);
    }
    if (entry.isPlanned) {
        painter->setBrush(solidPlannedColor);
    } else {
        QColor unplannedColor(option.palette.color(QPalette::Disabled, QPalette::Text));
        unplannedColor.setAlpha(150);
        painter->setBrush(GCColor::blendedColor(unplannedColor, bgColor));
    }
    painter->drawEllipse(QPoint(day0X, day0Y), radius, radius);

    painter->restore();
}


QSize
PlanAdherenceOffsetDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(index)

    const QFontMetrics fontMetrics(option.font);
    const int lineHeight = fontMetrics.height();

    return QSize(0, 2 * (vertMargin + lineHeight));
}


//////////////////////////////////////////////////////////////////////////////
// PlanAdherenceStatusDelegate

PlanAdherenceStatusDelegate::PlanAdherenceStatusDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
PlanAdherenceStatusDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    painter->save();

    QFont line1Font = opt.font;
    QFontMetrics line1FM(line1Font);
    painter->setPen(opt.palette.color(QPalette::Text));

    QString line1 = index.data(Line1Role).toString();
    QString line2 = index.data(Line2Role).toString();
    if (line2.isEmpty()) {
        QRect lineRect = option.rect.adjusted(horMargin, vertMargin, -(horMargin + rightPadding), -vertMargin);
        painter->setFont(line1Font);
        painter->drawText(lineRect, Qt::AlignRight | Qt::AlignVCenter, line1);
    } else {
        QFont::Weight line2Weight = static_cast<QFont::Weight>(index.data(Line2WeightRole).toInt());
        QFont line2Font = line1Font;
        line2Font.setWeight(line2Weight);
        QFontMetrics line2FM(line2Font);
        int line1Height = line1FM.height();
        int line2Height = line2FM.height();
        int extraHeight = (option.rect.height() - 2 * vertMargin - line1Height - lineSpacing - line2Height) / 2;
        QRect lineRect = option.rect.adjusted(horMargin, vertMargin + extraHeight, -(horMargin + rightPadding), 0);
        lineRect.setHeight(line1Height);
        painter->setFont(line1Font);
        painter->drawText(lineRect, Qt::AlignRight | Qt::AlignVCenter, line1);
        lineRect.moveTop(lineRect.top() + line1Height + lineSpacing);
        painter->setFont(line2Font);
        painter->drawText(lineRect, Qt::AlignRight | Qt::AlignVCenter, line2);
    }
    painter->restore();
}


QSize
PlanAdherenceStatusDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QString line1 = index.data(Line1Role).toString();
    QString line2 = index.data(Line2Role).toString();
    QFont line1Font = option.font;
    QFontMetrics line1FM(line1Font);
    int width = line1FM.horizontalAdvance(line1);
    int height = line1FM.height();
    if (! line2.isEmpty()) {
        QFont::Weight line2Weight = static_cast<QFont::Weight>(index.data(Line2WeightRole).toInt());
        QFont line2Font = line1Font;
        line2Font.setWeight(line2Weight);
        QFontMetrics line2FM(line2Font);
        int widthLine2 = line2FM.horizontalAdvance(line2);
        int heightLine2 = line2FM.height();
        width = std::max(width, widthLine2);
        height += lineSpacing + heightLine2;
    }
    width += 2 * horMargin + rightPadding;
    height += 2 * vertMargin;
    return QSize(width, height);
}


//////////////////////////////////////////////////////////////////////////////
// PlanAdherenceHeaderView

PlanAdherenceHeaderView::PlanAdherenceHeaderView
(Qt::Orientation orientation, QWidget *parent)
: QHeaderView(orientation, parent), PlanAdherenceOffsetHandler()
{
}


void
PlanAdherenceHeaderView::paintSection
(QPainter *painter, const QRect &rect, int logicalIndex) const
{
    QStyleOptionHeader opt;
    initStyleOption(&opt);
    opt.rect = rect;
    opt.section = logicalIndex;

    if (logicalIndex == 1) {
        style()->drawControl(QStyle::CE_HeaderSection, &opt, painter, this);
        int cellCount = numCells();
        if (cellCount == 0) {
            return;
        }

        const int availableWidth = rect.width() - 2 * horMargin;
        const int availableLeft = rect.left() + horMargin;
        const float cellWidth = availableWidth / cellCount;
        painter->save();
        QRect cellRect(availableLeft, rect.top(), availableWidth, rect.height());
        bool addMinOverflow = hasMinOverflow();
        bool addMaxOverflow = hasMaxOverflow();
        int offsetShift = addMinOverflow ? 1 : 0;
        for (int i = 0; i < cellCount; ++i) {
            QString label;
            if (   (i == 0 && addMinOverflow)
                || (i == cellCount - 1 && addMaxOverflow)) {
                label = "…";
            } else {
                int offset = i + minVisibleOffset() - offsetShift;
                if (offset == 0) {
                    label = "0";
                } else {
                    label = QString::asprintf("%+d", offset);
                }
                label += tr("d");
            }
            QRect labelRect(availableLeft + i * cellWidth, rect.top(), cellWidth - 4, rect.height());
            painter->drawText(labelRect, Qt::AlignCenter, label);
        }
        painter->restore();
    } else {
        opt.text = model()->headerData(logicalIndex, orientation(), Qt::DisplayRole).toString();
        style()->drawControl(QStyle::CE_Header, &opt, painter, this);
    }
}


//////////////////////////////////////////////////////////////////////////////
// PlanAdherenceTree

PlanAdherenceTree::PlanAdherenceTree
(QWidget *parent)
: JointLineTree(parent)
{
    headerView = new PlanAdherenceHeaderView(Qt::Horizontal, this);
    setHeader(headerView);
    header()->setStyleSheet("QHeaderView::section {"
                            "   border-left: none;"
                            "   border-right: none;"
                            "   border-top: none;"
                            "   border-bottom: 1px solid palette(mid);"
                            "}");

    setColumnCount(3);
    header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header()->setSectionResizeMode(1, QHeaderView::Stretch);
    header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header()->setStretchLastSection(false);
    headerItem()->setText(0, "");
    headerItem()->setText(2, "");

    setItemDelegateForColumn(0, new PlanAdherenceTitleDelegate());
    planAdherenceOffsetDelegate = new PlanAdherenceOffsetDelegate();
    setItemDelegateForColumn(1, planAdherenceOffsetDelegate);
    setItemDelegateForColumn(2, new PlanAdherenceStatusDelegate());
}


void
PlanAdherenceTree::fillEntries
(const QList<PlanAdherenceEntry> &entries, const PlanAdherenceOffsetRange &offsets)
{
    clear();
    QLocale locale;
    for (const PlanAdherenceEntry &entry : entries) {
        QString statusLine1;
        QString statusLine2;
        QFont::Weight statusLine2Weight = QFont::Normal;
        if (! entry.isPlanned) {
            statusLine1 = tr("Unplanned");
        } else if (entry.actualOffset != std::nullopt) {
            statusLine1 = tr("Completed");
            if (entry.actualOffset.value() != 0) {
                statusLine2 = QString::asprintf("%+lld", entry.actualOffset.value()) + tr("d");
                statusLine2Weight = QFont::Bold;
            } else {
                statusLine2 = tr("On-Time");
                statusLine2Weight = QFont::ExtraLight;
            }
        } else if (entry.date >= QDate::currentDate()) {
            statusLine1 = tr("Upcoming");
        } else if (entry.shiftOffset != std::nullopt && entry.date.addDays(entry.shiftOffset.value()) >= QDate::currentDate()) {
            statusLine1 = tr("Shifted");
            statusLine2 = tr("Upcoming");
            statusLine2Weight = QFont::ExtraLight;
        } else {
            statusLine1 = tr("Missed");
        }

        QTreeWidgetItem *entryItem = new QTreeWidgetItem(this);
        entryItem->setData(0, PlanAdherence::DataRole, QVariant::fromValue<PlanAdherenceEntry>(entry));
        entryItem->setData(2, PlanAdherenceStatusDelegate::Line1Role, statusLine1);
        entryItem->setData(2, PlanAdherenceStatusDelegate::Line2Role, statusLine2);
        entryItem->setData(2, PlanAdherenceStatusDelegate::Line2WeightRole, statusLine2Weight);
    }
    planAdherenceOffsetDelegate->setMinEntryOffset(offsets.min);
    planAdherenceOffsetDelegate->setMaxEntryOffset(offsets.max);
    planAdherenceOffsetDelegate->setMinAllowedOffset(minAllowedOffset);
    planAdherenceOffsetDelegate->setMaxAllowedOffset(maxAllowedOffset);

    headerView->setMinEntryOffset(offsets.min);
    headerView->setMaxEntryOffset(offsets.max);
    headerView->setMinAllowedOffset(minAllowedOffset);
    headerView->setMaxAllowedOffset(maxAllowedOffset);
}


QColor
PlanAdherenceTree::getHoverBG
() const
{
    QColor bgColor = palette().color(QPalette::Base);
    QColor hoverBG = palette().color(QPalette::Active, QPalette::Highlight);
    hoverBG.setAlphaF(0.2f);
    return GCColor::blendedColor(hoverBG, bgColor);
}


QColor
PlanAdherenceTree::getHoverHL
() const
{
    QColor bgColor = palette().color(QPalette::Base);
    QColor hoverHL = palette().color(QPalette::Active, QPalette::Highlight);
    hoverHL.setAlphaF(0.6f);
    return GCColor::blendedColor(hoverHL, bgColor);
}


void
PlanAdherenceTree::setMinAllowedOffset
(int offset)
{
    minAllowedOffset = offset;
}


void
PlanAdherenceTree::setMaxAllowedOffset
(int offset)
{
    maxAllowedOffset = offset;
}


QMenu*
PlanAdherenceTree::createContextMenu
(const QModelIndex &index)
{
    const QString ellipsis = QStringLiteral("...");
    PlanAdherenceEntry entry = index.siblingAtColumn(0).data(PlanAdherence::DataRole).value<PlanAdherenceEntry>();
    QMenu *contextMenu = new QMenu(this);
    if (! entry.plannedReference.isEmpty()) {
        contextMenu->addAction(tr("View planned activity") % ellipsis, this, [this, entry]() { emit viewActivity(entry.plannedReference, true); });
    }
    if (! entry.actualReference.isEmpty()) {
        contextMenu->addAction(tr("View actual activity") % ellipsis, this, [this, entry]() { emit viewActivity(entry.actualReference, false); });
    }
    return contextMenu;
}


QString
PlanAdherenceTree::createToolTipText
(const QModelIndex &index) const
{
    PlanAdherenceEntry entry = index.siblingAtColumn(0).data(PlanAdherence::DataRole).value<PlanAdherenceEntry>();
    QString html = QString("<div style='padding: %1px;'>").arg(4 * dpiXFactor);
    html += QString("<div style='font-weight: bold; font-size: 120%;'>%1</div>")
                   .arg(entry.titlePrimary);
    html += "<table>";
    QString status;
    QString dateLabel;
    bool hasActual = false;
    int actualOffset = 0;
    bool isShifted = false;
    int shiftOffset = 0;
    if (! entry.isPlanned) {
        status = tr("Unplanned");
        dateLabel = tr("Date");
    } else {
        hasActual = entry.actualOffset != std::nullopt;
        if (hasActual) {
            actualOffset = entry.actualOffset.value();
        }
        isShifted = entry.shiftOffset != std::nullopt;
        if (isShifted) {
            shiftOffset = entry.shiftOffset.value();
        }
        if (isShifted) {
            dateLabel = tr("Originally Planned");
        } else {
            dateLabel = tr("Planned");
        }
        if (hasActual) {
            status = tr("Completed");
        } else if (entry.date.addDays(shiftOffset) >= QDate::currentDate()) {
            status = tr("Upcoming");
        } else {
            status = tr("Missed");
        }
    }
    QLocale locale;
    html += QString("<tr><td><b>%1:</b></td><td>%2</td></tr>").arg(tr("Status")).arg(status);
    html += QString("<tr><td><b>%1:</b></td><td>%2</td></tr>")
                   .arg(dateLabel)
                   .arg(locale.toString(entry.date, QLocale::NarrowFormat));
    if (isShifted) {
        QString shiftDateStr = locale.toString(entry.date.addDays(shiftOffset), QLocale::NarrowFormat);
        QString shiftValue;
        if (shiftOffset < -1) {
            shiftValue = tr("%1 days earlier (→ %2)").arg(-shiftOffset).arg(shiftDateStr);
        } else if (shiftOffset == -1) {
            shiftValue = tr("1 day earlier (→ %1)").arg(shiftDateStr);
        } else if (shiftOffset == 1) {
            shiftValue = tr("1 day later (→ %1)").arg(shiftDateStr);
        } else {
            shiftValue = tr("%1 days later (→ %2)").arg(shiftOffset).arg(shiftDateStr);
        }
        html += QString("<tr><td><b>%1:</b></td><td>%2</td></tr>")
                       .arg(tr("Shifted"))
                       .arg(shiftValue);
    }
    if (hasActual) {
        QString actualDateStr = locale.toString(entry.date.addDays(actualOffset), QLocale::NarrowFormat);
        QString actualValue;
        if (actualOffset < -1) {
            actualValue = tr("%1 days early (%2)").arg(-actualOffset).arg(actualDateStr);
        } else if (actualOffset == -1) {
            actualValue = tr("1 day early (%1)").arg(actualDateStr);
        } else if (actualOffset == 1) {
            actualValue = tr("1 day late (%1)").arg(actualDateStr);
        } else if (actualOffset > 1) {
            actualValue = tr("%1 days late (%2)").arg(actualOffset).arg(actualDateStr);
        } else {
            actualValue = tr("On-Time");
        }
        html += QString("<tr><td><b>%1:</b></td><td>%2</td></tr>")
                       .arg(tr("Completed"))
                       .arg(actualValue);
        html += QString("<tr><td><b>%1:</b></td><td>%2</td></tr>")
                       .arg(tr("Completed as"))
                       .arg(entry.titleSecondary);
    }
    html += "</table></div>";
    return html;
}


//////////////////////////////////////////////////////////////////////////////
// PlanAdherenceView

PlanAdherenceView::PlanAdherenceView
(QWidget *parent)
: QWidget(parent)
{
    qRegisterMetaType<PlanAdherenceEntry>("PlanAdherenceEntry");

    toolbar = new QToolBar();

    prevAction = toolbar->addAction(tr("Previous Month"));
    nextAction = toolbar->addAction(tr("Next Month"));
    todayAction = toolbar->addAction(tr("Today"));
    separator = toolbar->addSeparator();

    dateNavigator = new QToolButton();
    dateNavigator->setPopupMode(QToolButton::InstantPopup);
    dateNavigatorAction = toolbar->addWidget(dateNavigator);

    dateMenu = new QMenu(this);
    connect(dateMenu, &QMenu::aboutToShow, this, &PlanAdherenceView::populateDateMenu);

    dateNavigator->setMenu(dateMenu);

    QWidget *spacer = new QWidget(toolbar);
    spacer->setFixedWidth(10 * dpiXFactor);
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    toolbar->addWidget(spacer);

    QLabel *filterLabel = new QLabel("<i>" + tr("Filters applied") + "</i>");
    filterLabelAction = toolbar->addWidget(filterLabel);

    QWidget *stretch = new QWidget();
    stretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(stretch);

    timeline = new PlanAdherenceTree();

    totalBox = new StatisticBox(tr("Total"), tr("Total number of activities in this period"));
    plannedBox = new StatisticBox(tr("Planned"), tr("Activities that were part of the training plan"));
    unplannedBox = new StatisticBox(tr("Unplanned"), tr("Activities completed without being in the plan"));
    onTimeBox = new StatisticBox(tr("On-Time"), tr("Planned activities completed on the scheduled date"));
    shiftedBox = new StatisticBox(tr("Shifted"), tr("Planned activities rescheduled to a different date"));
    missedBox = new StatisticBox(tr("Missed"), tr("Planned activities that were not completed"));
    avgShiftBox = new StatisticBox(tr("Avg Shift"), tr("Average number of days between original and rescheduled planned date"));
    totalShiftDaysBox = new StatisticBox(tr("Total Shift Days"), tr("Sum of days by which planned activities were rescheduled"));

    QHBoxLayout *statisticsLayout = new QHBoxLayout();
    statisticsLayout->addWidget(totalBox);
    statisticsLayout->addWidget(plannedBox);
    statisticsLayout->addWidget(unplannedBox);
    statisticsLayout->addWidget(onTimeBox);
    statisticsLayout->addWidget(shiftedBox);
    statisticsLayout->addWidget(missedBox);
    statisticsLayout->addWidget(avgShiftBox);
    statisticsLayout->addWidget(totalShiftDaysBox);

    QWidget *adherenceWidget = new QWidget();
    QVBoxLayout *adherenceLayout = new QVBoxLayout(adherenceWidget);
    adherenceLayout->addLayout(statisticsLayout);
    adherenceLayout->addSpacing(15 * dpiYFactor);
    adherenceLayout->addWidget(timeline);

    QString emptyHeadline = tr("No activities found");
    QString emptyInfoline = tr("There are no activities for the selected month.");
    QString emptyActionline = tr("Try selecting a different month, adjusting your filters, or creating a new activity.");
    QString emptyMsg = QString("<h3>%1</h3>%2<br/>%3").arg(emptyHeadline).arg(emptyInfoline).arg(emptyActionline);
    QLabel *emptyLabel = new QLabel(emptyMsg);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setWordWrap(true);

    viewStack = new QStackedWidget();
    viewStack->addWidget(adherenceWidget);
    viewStack->addWidget(emptyLabel);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(toolbar);
    layout->addWidget(viewStack);
    layout->addSpacing(16 * dpiYFactor);

    connect(timeline, &PlanAdherenceTree::viewActivity, this, &PlanAdherenceView::viewActivity);
    connect(prevAction, &QAction::triggered, this, [this]() {
        dateInMonth = QDate(dateInMonth.year(), dateInMonth.month(), 1).addMonths(-1);
        emit monthChanged(dateInMonth);
    });
    connect(nextAction, &QAction::triggered, this, [this]() {
        dateInMonth = QDate(dateInMonth.year(), dateInMonth.month(), 1).addMonths(1);
        emit monthChanged(dateInMonth);
    });
    connect(todayAction, &QAction::triggered, this, [this]() {
        dateInMonth = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
        emit monthChanged(dateInMonth);
    });
    connect(this, &PlanAdherenceView::monthChanged, this, [this]() {
        updateHeader();
        setNavButtonState();
    });
    applyNavIcons();
}


QDate
PlanAdherenceView::firstVisibleDay
() const
{
    if (! dateInMonth.isValid()) {
        return dateRange.from;
    }
    QDate firstOfMonth(dateInMonth.year(), dateInMonth.month(), 1);
    if (dateRange.pass(firstOfMonth)) {
        return firstOfMonth;
    } else {
        return dateRange.from;
    }
}


QDate
PlanAdherenceView::lastVisibleDay
() const
{
    if (! dateInMonth.isValid()) {
        return dateRange.to;
    }
    QDate lastOfMonth(dateInMonth.year(), dateInMonth.month(), dateInMonth.daysInMonth());
    if (dateRange.pass(lastOfMonth)) {
        return lastOfMonth;
    } else {
        return dateRange.to;
    }
}


void
PlanAdherenceView::fillEntries
(const QList<PlanAdherenceEntry> &entries, const PlanAdherenceStatistics &statistics, const PlanAdherenceOffsetRange &offsets, bool isFiltered)
{
    filterLabelAction->setVisible(isFiltered);
    timeline->fillEntries(entries, offsets);
    QLocale locale;
    totalBox->setValue(locale.toString(statistics.totalAbs));
    if (statistics.totalAbs == 0) {
        plannedBox->setValue(locale.toString(statistics.plannedAbs), tr("N/A"));
        unplannedBox->setValue(locale.toString(statistics.unplannedAbs), tr("N/A"));
    } else {
        plannedBox->setValue(locale.toString(statistics.plannedAbs), locale.toString(statistics.plannedRel, 'g', 3) + "%");
        unplannedBox->setValue(locale.toString(statistics.unplannedAbs), locale.toString(statistics.unplannedRel, 'g', 3) + "%");
    }
    if (statistics.plannedAbs == 0) {
        onTimeBox->setValue(locale.toString(statistics.onTimeAbs), tr("N/A"));
        shiftedBox->setValue(locale.toString(statistics.shiftedAbs), tr("N/A"));
        missedBox->setValue(locale.toString(statistics.missedAbs), tr("N/A"));
    } else {
        onTimeBox->setValue(locale.toString(statistics.onTimeAbs), locale.toString(statistics.onTimeRel, 'g', 3) + "%");
        shiftedBox->setValue(locale.toString(statistics.shiftedAbs), locale.toString(statistics.shiftedRel, 'g', 3) + "%");
        missedBox->setValue(locale.toString(statistics.missedAbs), locale.toString(statistics.missedRel, 'g', 3) + "%");
    }
    avgShiftBox->setValue(locale.toString(statistics.avgShift, 'g', 3));
    totalShiftDaysBox->setValue(locale.toString(statistics.totalShiftDaysAbs));
    viewStack->setCurrentIndex(entries.count() > 0 ? 0 : 1);
}


void
PlanAdherenceView::setDateRange
(const DateRange &dr)
{
    dateRange = dr;
    if (! dateInMonth.isValid()) {
        dateInMonth = QDate::currentDate();
    }
    if (! dateRange.pass(dateInMonth)) {
        QDate today = QDate::currentDate();
        if (dateRange.pass(today)) {
            dateInMonth = today;
        } else if (dateRange.from.isValid() && dateInMonth < dateRange.from) {
            dateInMonth = dateRange.from;
        } else if (dateRange.to.isValid() && dateInMonth > dateRange.to) {
            dateInMonth = dateRange.to;
        }
    }
    emit monthChanged(dateInMonth);
}


void
PlanAdherenceView::setMinAllowedOffset
(int offset)
{
    timeline->setMinAllowedOffset(offset);
}


void
PlanAdherenceView::setMaxAllowedOffset
(int offset)
{
    timeline->setMaxAllowedOffset(offset);
}


void
PlanAdherenceView::setPreferredStatisticsDisplay
(int mode)
{
    StatisticBox::DisplayState state = (mode == 0) ? StatisticBox::DisplayState::Primary : StatisticBox::DisplayState::Secondary;
    totalBox->setPreferredState(state);
    plannedBox->setPreferredState(state);
    unplannedBox->setPreferredState(state);
    onTimeBox->setPreferredState(state);
    shiftedBox->setPreferredState(state);
    missedBox->setPreferredState(state);
    avgShiftBox->setPreferredState(state);
    totalShiftDaysBox->setPreferredState(state);
}


void
PlanAdherenceView::populateDateMenu
()
{
    dateMenu->clear();
    dateMenu->addSection(tr("Season: %1").arg(dateRange.name));
    dateMenu->setEnabled(true);

    int yearFrom = dateRange.from.isValid() ? dateRange.from.year() : QDate::currentDate().addYears(-5).year();
    int yearTo = dateRange.to.isValid() ? dateRange.to.year() : QDate::currentDate().addYears(5).year();
    for (int year = yearFrom; year <= yearTo; ++year) {
        QAction *action = dateMenu->addAction(QString::number(year));
        if (year == dateInMonth.year()) {
            action->setEnabled(false);
        } else {
            QDate date(year, 1, 1);
            if (! dateRange.pass(date)) {
                if (date.year() == dateRange.from.year()) {
                    date = dateRange.from;
                } else {
                    date = dateRange.to;
                }
            }
            connect(action, &QAction::triggered, this, [this, date]() {
                dateInMonth = date;
                emit monthChanged(dateInMonth);
            });
        }
    }
}


void
PlanAdherenceView::setNavButtonState
()
{
    if (! dateInMonth.isValid()) {
        return;
    }
    QDate lastOfPrevMonth(dateInMonth.addMonths(-1));
    lastOfPrevMonth.setDate(lastOfPrevMonth.year(), lastOfPrevMonth.month(), lastOfPrevMonth.daysInMonth());
    QDate firstOfNextMonth(dateInMonth.addMonths(1));
    firstOfNextMonth.setDate(firstOfNextMonth.year(), firstOfNextMonth.month(), 1);
    prevAction->setEnabled(dateRange.pass(lastOfPrevMonth));
    nextAction->setEnabled(dateRange.pass(firstOfNextMonth));
    todayAction->setEnabled(dateRange.pass(QDate::currentDate()));
}


void
PlanAdherenceView::updateHeader
()
{
    QLocale locale;
    dateNavigator->setText(locale.toString(dateInMonth, "MMMM yyyy"));
    prevAction->setVisible(true);
    nextAction->setVisible(true);
    todayAction->setVisible(true);
    separator->setVisible(true);
    dateNavigatorAction->setVisible(true);
}


void
PlanAdherenceView::applyNavIcons
()
{
    double scale = appsettings->value(this, GC_FONT_SCALE, 1.0).toDouble();
    QFont font;
    font.setPointSize(font.pointSizeF() * scale * 1.3);
    font.setWeight(QFont::Bold);
    dateNavigator->setFont(font);
    QString mode = GCColor::isPaletteDark(palette()) ? "dark" : "light";
    toolbar->setMinimumHeight(dateNavigator->sizeHint().height() + 12 * dpiYFactor);

    prevAction->setIcon(QIcon(QString(":images/breeze/%1/go-previous.svg").arg(mode)));
    nextAction->setIcon(QIcon(QString(":images/breeze/%1/go-next.svg").arg(mode)));
}


//////////////////////////////////////////////////////////////////////////////
// static helpers

static void
sendFakeHoverMove
(QWidget *viewport, const QPoint &pos)
{
    QPointF posF(pos);
    QPointF globalF = viewport->mapToGlobal(posF);
    QHoverEvent fakeHover(QEvent::HoverMove, posF, globalF, posF);
    QCoreApplication::sendEvent(viewport, &fakeHover);
}


static void
sendFakeHoverLeave
(QWidget *viewport)
{
    QPointF outOfBounds(-1, -1);
    QPointF globalOutOfBounds = viewport->mapToGlobal(outOfBounds);
    QPointF oldPos = viewport->mapFromGlobal(QCursor::pos());
    QHoverEvent fakeLeave(QEvent::HoverLeave, outOfBounds, globalOutOfBounds, oldPos);
    QCoreApplication::sendEvent(viewport, &fakeLeave);
}
