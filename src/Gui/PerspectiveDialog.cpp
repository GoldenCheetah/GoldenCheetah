/*
 * Copyright (c) 2021 Mark Liversedge <liversedge@gmail.com>
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

#include "PerspectiveDialog.h"
#include "AbstractView.h"
#include "ActionButtonBox.h"
#include "Perspective.h"

#include <QFormLayout>
#include <QLabel>
#include <QMessageBox>
#include <QFileDialog>

///
/// PerspectiveDialog
///
PerspectiveDialog::PerspectiveDialog(QWidget *parent, AbstractView *tabView) : QDialog(parent), tabView(tabView), active(false)
{

    setWindowTitle(tr("Manage Perspectives"));
    setMinimumWidth(450*dpiXFactor);
    setMinimumHeight(450*dpiXFactor);

    //setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint | Qt::Tool);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // create all the widgets
    QLabel *xlabel = new QLabel(tr("Perspectives"));
    perspectiveTable = new PerspectiveTableWidget(this);
#ifdef Q_OS_MAX
    xdataTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

    QString typedesc;
    switch (tabView->type) {
    case VIEW_PLAN:
    case VIEW_TRENDS: typedesc=tr("Activities filter"); break;
    case VIEW_ANALYSIS: typedesc=tr("Switch expression"); break;
    case VIEW_TRAIN: typedesc=tr("Switch for"); break;
    default: qDebug() << "Unknown view type in PerspectiveDialog:" << tabView->type; break;
    }

    perspectiveTable->setColumnCount(2);
    QStringList headers = (QStringList() << tr(" Name ") << typedesc);
    perspectiveTable->setHorizontalHeaderLabels(headers);
    perspectiveTable->horizontalHeader()->setHighlightSections(false);
    QFont font = perspectiveTable->horizontalHeaderItem(0)->font();
    font.setBold(false);
    perspectiveTable->horizontalHeaderItem(0)->setFont(font);
    perspectiveTable->horizontalHeaderItem(1)->setFont(font);
    perspectiveTable->horizontalHeader()->setStretchLastSection(true);
    perspectiveTable->setSortingEnabled(false);
    perspectiveTable->verticalHeader()->hide();
    perspectiveTable->setShowGrid(false);
    perspectiveTable->setSelectionMode(QAbstractItemView::SingleSelection);
    perspectiveTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    perspectiveTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    perspectiveTable->setAcceptDrops(true);
    //perspectiveTable->setDropIndicatorShown(true);

    QLabel *xslabel = new QLabel(tr("Charts"));
    chartTable = new ChartTableWidget(this);
#ifdef Q_OS_MAX
    xdataSeriesTable->setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif
    chartTable->setColumnCount(1);
    chartTable->horizontalHeader()->setStretchLastSection(true);
    chartTable->horizontalHeader()->hide();
    chartTable->setSortingEnabled(false);
    chartTable->verticalHeader()->hide();
    chartTable->setShowGrid(false);
    chartTable->setSelectionMode(QAbstractItemView::SingleSelection);
    chartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    chartTable->setDragEnabled(true);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::UpDownGroup | ActionButtonBox::EditGroup | ActionButtonBox::AddDeleteGroup);
    QPushButton *exportPerspective = actionButtons->addButton(tr("Export"), ActionButtonBox::Right);
    QPushButton *importPerspective = actionButtons->addButton(tr("Import"), ActionButtonBox::Right);
    actionButtons->setMinViewItems(1);

    // not so obvious perhaps
    instructions = new QLabel(tr("Drag charts to move to a perspective"));

    // lay it out
    mainLayout->addWidget(xlabel);
    mainLayout->addWidget(perspectiveTable);
    mainLayout->addWidget(actionButtons);
    mainLayout->addWidget(xslabel);
    mainLayout->addWidget(chartTable);
    QHBoxLayout *xs = new QHBoxLayout();
    xs->addWidget(instructions);
    mainLayout->addLayout(xs);

    connect(perspectiveTable, SIGNAL(currentItemChanged(QTableWidgetItem*,QTableWidgetItem*)), this, SLOT(perspectiveSelected()));
    connect(perspectiveTable, SIGNAL(chartMoved(GcChartWindow*)), this, SLOT(perspectiveSelected())); // just reset the chart list
    connect(perspectiveTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(perspectiveNameChanged(QTableWidgetItem*))); // user edit
    connect(perspectiveTable, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(editPerspectiveClicked())); // Double click -> edit

    actionButtons->defaultConnect(perspectiveTable);
    connect(actionButtons, &ActionButtonBox::editRequested, this, &PerspectiveDialog::editPerspectiveClicked);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &PerspectiveDialog::addPerspectiveClicked);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &PerspectiveDialog::removePerspectiveClicked);
    connect(actionButtons, &ActionButtonBox::upRequested, this, &PerspectiveDialog::upPerspectiveClicked);
    connect(actionButtons, &ActionButtonBox::downRequested, this, &PerspectiveDialog::downPerspectiveClicked);
    connect(importPerspective, &QPushButton::clicked, this, &PerspectiveDialog::importPerspectiveClicked);
    connect(exportPerspective, &QPushButton::clicked, this, &PerspectiveDialog::exportPerspectiveClicked);

    // set table
    setTables();
}

void PerspectiveDialog::close()
{
}

// set the perspective part of the dialog, the chart list
// is set when a perspective is selected
void PerspectiveDialog::setTables()
{
    active = true;

    perspectiveTable->clearContents();
    perspectiveTable->setRowCount(tabView->perspectives_.count());

    // add a row for each perspective
    int perspectiverow=0;
    foreach(Perspective *perspective, tabView->perspectives_) {

        // add a row to the perspective table - editable by default
        QTableWidgetItem *add = new QTableWidgetItem(perspective->title_, 0);
        add->setFlags(add->flags() | Qt::ItemIsDropEnabled);

        // and the perspective we represent, so we can avoid dropping on ourselves
        add->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(perspective)));
        perspectiveTable->setItem(perspectiverow, 0, add);

        QString description;
        switch (tabView->type) {
        case VIEW_TRENDS:
        case VIEW_ANALYSIS:
        case VIEW_PLAN: description = perspective->expression(); break;
        case VIEW_TRAIN: {
            switch (perspective->trainswitch) {
            case Perspective::None: description=tr("Don't switch"); break;
            case Perspective::Erg: description=tr("Erg Workout"); break;
            case Perspective::Slope: description=tr("Slope Workout"); break;
            case Perspective::Video: description=tr("Video Workout"); break;
            case Perspective::Map: description=tr("Map Workout"); break;
            default: qDebug() << "Unknown train switch value in PerspectiveDialog:" << perspective->trainswitch; break;
        } break;
        default: qDebug() << "Unknown view type in PerspectiveDialog:" << tabView->type; break;
        }
        }

        // and the perspective's rule (for information)
        QTableWidgetItem *perspectiveInfo = new QTableWidgetItem(description, 0);
        perspectiveInfo->setFlags(add->flags() | Qt::ItemIsDropEnabled);

        // and the perspective we represent, so we can avoid dropping on ourselves
        perspectiveInfo->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(perspective)));
        perspectiveTable->setItem(perspectiverow++, 1, perspectiveInfo);
    }

    active = false;

    // set to first row
    if (perspectiveTable->currentRow()==0)  perspectiveSelected();
    else perspectiveTable->selectRow(0);

}

void PerspectiveDialog::perspectiveSelected()
{
    if (active) return; // ignore when programmatically maintaining views

    // charts listed are for the perspective currently selected
    chartTable->clear();

    // lets find the one we have selected...
    int perspectiverow =perspectiveTable->currentIndex().row();

    // check that we're in bounds
    if (perspectiverow < 0 || perspectiverow >= tabView->perspectives_.count()) return;

    // how many charts...
    chartTable->setRowCount(tabView->perspectives_[perspectiverow]->charts.count());

    // lets populate
    int chartrow=0;
    foreach(GcChartWindow *chart, tabView->perspectives_[perspectiverow]->charts) {

        // add a row for each name
        QTableWidgetItem *add = new QTableWidgetItem();
        add->setFlags((add->flags() | Qt::ItemIsDragEnabled) & (~Qt::ItemIsEditable));
        add->setText(chart->title());

        // add perspective and chart pointers so we can encode when dragging (for dropping)
        add->setData(Qt::UserRole, QVariant::fromValue(static_cast<void*>(tabView->perspectives_[perspectiverow])));
        add->setData(Qt::UserRole+1, QVariant::fromValue(static_cast<void*>(chart)));

        chartTable->setItem(chartrow++, 0, add);
    }

    if (chartrow) chartTable->selectRow(0);
}

void
PerspectiveDialog::removePerspectiveClicked()
{
    // just do diag first.. lets not be too hasty !!!
    int index = perspectiveTable->selectedItems()[0]->row();

    // wipe it - tabView will worry about bounds and switching if we delete the currently selected one
    tabView->removePerspective(tabView->perspectives_[index]);

    // set tables
    setTables();

    emit perspectivesChanged();
}

void
PerspectiveDialog::editPerspectiveClicked()
{
    int index = perspectiveTable->selectedItems()[0]->row();

    if (index >= 0 && index < tabView->perspectives_.count()) {

        Perspective *editing = tabView->perspectives_[index];
        QString expression=editing->expression();
        AddPerspectiveDialog *dialog= new AddPerspectiveDialog(this, tabView->context, editing->title_, expression, tabView->type, editing->trainswitch, true);
        int ret= dialog->exec();
        delete dialog;
        if (ret == QDialog::Accepted) {
            editing->setExpression(expression);
            emit perspectivesChanged();
            setTables();
        }
    }
}

void
PerspectiveDialog::addPerspectiveClicked()
{
    QString name;
    QString expression;
    Perspective::switchenum trainswitch = Perspective::None;
    AddPerspectiveDialog *dialog= new AddPerspectiveDialog(this, tabView->context, name, expression, tabView->type, trainswitch);
    int ret= dialog->exec();
    delete dialog;
    if (ret == QDialog::Accepted && name != "") {

         // add...
        Perspective *newone =tabView->addPerspective(name);

        switch (tabView->type) {
        case VIEW_TRENDS:
        case VIEW_ANALYSIS:
        case VIEW_PLAN: newone->setExpression(expression); break;
        case VIEW_TRAIN: newone->setTrainSwitch(trainswitch); break;
        default: qDebug() << "Unknown view type in PerspectiveDialog:" << tabView->type; break;
        }

    emit perspectivesChanged();

        setTables();
    }
}

void
PerspectiveDialog::exportPerspectiveClicked()
{
    int index = perspectiveTable->selectedItems()[0]->row();

    // wipe it - tabView will worry about bounds and switching if we delete the currently selected one
    Perspective *current = tabView->perspectives_[index];

    QString typedesc;
    switch (tabView->type) {
    case VIEW_TRENDS: typedesc="Trends"; break;
    case VIEW_ANALYSIS: typedesc="Analysis"; break;
    case VIEW_PLAN: typedesc="Plan"; break;
    case VIEW_TRAIN: typedesc="Train"; break;
    default: qDebug() << "Unknown view type in PerspectiveDialog:" << tabView->type; break;
    }

    // export the current perspective to a file
    QString suffix;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export Perspective"),
                       QDir::homePath()+"/"+ typedesc + " " + current->title() + ".gchartset",
                       ("*.gchartset;;"), &suffix, QFileDialog::DontUseNativeDialog); // native dialog hangs when threads in use (!)

    if (fileName.isEmpty()) {
        QMessageBox::critical(this, tr("Export Perspective"), tr("No perspective file selected!"));
    } else {
        tabView->exportPerspective(current, fileName);
    }
}

void
PerspectiveDialog::importPerspectiveClicked()
{
    // import a new perspective from a file
    QString fileName = QFileDialog::getOpenFileName(this, tr("Select Perspective file to import"), "", tr("GoldenCheetah Perspective Files (*.gchartset)"));
    if (fileName.isEmpty()) {
        QMessageBox::critical(this, tr("Import Perspective"), tr("No perspective file selected!"));
    } else {

        // import and select it
        if (tabView->importPerspective(fileName)) {

            // update the table
            setTables();

            // new one added
            emit perspectivesChanged();
        }
    }
}

void
PerspectiveDialog::upPerspectiveClicked()
{
    int index = perspectiveTable->selectedItems()[0]->row();
    if (index == 0) return; // already at beginning

    // move it
    tabView->swapPerspective(index, index-1);

    // set tables
    setTables();

    // select it again
    perspectiveTable->selectRow(index-1);

    emit perspectivesChanged();
}

void
PerspectiveDialog::downPerspectiveClicked()
{
    int index = perspectiveTable->selectedItems()[0]->row();
    if (index == (tabView->perspectives_.count()-1)) return; // already at end

    // move it
    tabView->swapPerspective(index, index+1);

    // set tables
    setTables();

    // select it again
    perspectiveTable->selectRow(index+1);

    emit perspectivesChanged();
}

void
PerspectiveDialog::perspectiveNameChanged(QTableWidgetItem *item)
{
    if (active) return;

    // if the name changed we need to update
    if (item->text() != tabView->perspectives_[item->row()]->title()) {

        // update the title
        tabView->perspectives_[item->row()]->title_ = item->text();

        // tell the world
        emit perspectivesChanged();
    }
}

// drag and drop processing
void
PerspectiveTableWidget::dragEnterEvent(QDragEnterEvent *event)
{
    bool accept = false;

    // must be a chartref (from dragging charts below) anything else we ignore
    // just in case someone tries to drag a file etc
    foreach (QString format, event->mimeData()->formats()) {
        if (format == "application/x-gc-chartref") accept = true;
    }

    if (accept) {
        event->acceptProposedAction(); // whatever you wanna drop we will try and process!
        return;
    }

    QTableWidget::dragEnterEvent(event);
}

void
PerspectiveTableWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QPoint pos = mapFromGlobal(QCursor::pos());

    // allow for the height of the perspective table header
    pos.setY(pos.y() - horizontalHeader()->height());

    QTableWidgetItem *hover = itemAt(pos);
    if (hover) {
        QTableWidget::dragMoveEvent(event);
        event->acceptProposedAction(); // not fussy yet
        return;
    }

    QTableWidget::dragMoveEvent(event);
}

void
PerspectiveTableWidget::dropEvent(QDropEvent *event)
{
    // dropping a chart onto a perspective
    QByteArray rawData = event->mimeData()->data("application/x-gc-chartref");
    QDataStream stream(&rawData, QIODevice::ReadOnly);
    stream.setVersion(QDataStream::Qt_4_6);

    // pack data
    quint64 p,c;
    stream >> p;
    stream >> c;

    // lets look at the context..
    Perspective *perspective = (Perspective*)(p);
    GcChartWindow *chart = (GcChartWindow*)(c);

    // to where?
    QPoint pos = mapFromGlobal(QCursor::pos());

    // allow for the height of the perspective table header
    pos.setY(pos.y() - horizontalHeader()->height());

    QTableWidgetItem *hover = itemAt(pos);
    if (hover) {
        QVariant v = hover->data(Qt::UserRole);
        Perspective *hoverp = static_cast<Perspective*>(v.value<void*>());
        event->accept();

        // move, but only if source and dest are not the same
        if (perspective != hoverp) {
            chart = perspective->takeChart(chart);
            if (chart) {
                hoverp->addChart(chart);
                emit chartMoved(chart);

                // let the chart know it has moved perspectives.
                chart->notifyPerspectiveChanged(hoverp);
            }
        }
    }
}

QStringList
ChartTableWidget::mimeTypes() const
{
    QStringList returning;
    returning << "application/x-gc-chartref";

    return returning;
}

QMimeData *
ChartTableWidget::mimeData
(const QList<QTableWidgetItem *> &items) const
{
    QMimeData *returning = new QMimeData;

    // we need to pack into a byte array
    QByteArray rawData;
    QDataStream stream(&rawData, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_6);

    // pack data
    foreach (QTableWidgetItem *p, items) {

        // convert to one of ours
        QVariant v = p->data(Qt::UserRole);
        Perspective *perspective = static_cast<Perspective*>(v.value<void*>());

        v = p->data(Qt::UserRole + 1);
        GcChartWindow *chart = static_cast<GcChartWindow*>(v.value<void*>());

        // serialize
        stream << (quint64)(perspective);
        stream << (quint64)(chart);

    }

    // and return as mime data
    returning->setData("application/x-gc-chartref", rawData);
    return returning;
}
