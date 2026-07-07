/*
 * Copyright (c) 2025, Joachim Kohlhammer <joachim.kohlhammer@gmx.de>
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

#include "WorkoutMenuProvider.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStandardItemModel>

#include "TrainDB.h"
#include "ActionButtonBox.h"
#include "Colors.h"
#include "StyledItemDelegates.h"


WorkoutFilterStore::WorkoutFilterStore
(const QString &fileName)
: fileName(fileName)
{
}


QList<WorkoutFilterStore::StorableWorkoutFilter>
WorkoutFilterStore::getWorkoutFilters
() const
{
    return workoutFilters;
}


void
WorkoutFilterStore::append
(const QString &name, const QString &filter)
{
    workoutFilters.append(StorableWorkoutFilter(name, filter));
}


void
WorkoutFilterStore::insert
(int index, const QString &name, const QString &filter)
{
    workoutFilters.insert(index, StorableWorkoutFilter(name, filter));
}


void
WorkoutFilterStore::update
(int index, const QString &name, const QString &filter)
{
    if (index >= 0 && index < workoutFilters.count()) {
        workoutFilters[index].name = name;
        workoutFilters[index].filter = filter;
    }
}


void
WorkoutFilterStore::remove
(int index)
{
    workoutFilters.removeAt(index);
}


void
WorkoutFilterStore::moveUp
(int index)
{
    if (index > 0) {
        workoutFilters.swapItemsAt(index, index - 1);
    }
}


void
WorkoutFilterStore::moveDown
(int index)
{
    if (index < workoutFilters.count() - 1) {
        workoutFilters.swapItemsAt(index, index + 1);
    }
}


void
WorkoutFilterStore::load
()
{
    workoutFilters.clear();
    QFile file(fileName);
    if (! file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "Cannot open file" << fileName << "for reading -" << file.errorString();
        return;
    }
    QXmlStreamReader reader(&file);
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartDocument) {
            continue;
        }
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            QString elemName = reader.name().toString();
            if (elemName == "workoutFilters") {
                parseWorkoutFilters(reader);
            }
        }
    }
    if (reader.hasError()) {
        qCritical() << "Can't parse" << fileName << "-" << reader.errorString() << "in line" << reader.lineNumber() << "column" << reader.columnNumber();
    }
    file.close();
}


void
WorkoutFilterStore::save
()
{
    QFile file(fileName);
    if (! file.open(QFile::WriteOnly | QFile::Text)) {
        qCritical() << "Cannot open file" << fileName << "for writing -" << file.errorString();
        return;
    }
    QXmlStreamWriter stream(&file);
    stream.setAutoFormatting(true);
    stream.writeStartDocument();
    stream.writeStartElement("workoutFilters");
    for (const WorkoutFilterStore::StorableWorkoutFilter &filter : workoutFilters) {
        stream.writeStartElement("workoutFilter");
        stream.writeTextElement("name", filter.name);
        stream.writeTextElement("filter", filter.filter);
        stream.writeEndElement();
    }
    stream.writeEndElement();
    stream.writeEndDocument();
    file.close();
}


bool
WorkoutFilterStore::parseWorkoutFilters
(QXmlStreamReader &reader)
{
    bool ok = true;
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            QString elemName = reader.name().toString();
            if (elemName == "workoutFilter") {
                ok &= parseWorkoutFilter(reader);
            }
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            QString elemName = reader.name().toString();
            if (elemName == "workoutFilters") {
                break;
            }
        }
    }
    return ok;
}


bool
WorkoutFilterStore::parseWorkoutFilter
(QXmlStreamReader &reader)
{
    QString text;
    QString name;
    QString filter;
    while (! reader.atEnd()) {
        reader.readNext();
        if (reader.tokenType() == QXmlStreamReader::Characters) {
            text = reader.text().toString().trimmed();
        } else if (reader.tokenType() == QXmlStreamReader::EndElement) {
            QString elemName = reader.name().toString();
            if (elemName == "name") {
                name = text;
            } else if (elemName == "filter") {
                filter = text;
            } else if (elemName == "workoutFilter") {
                break;
            }
        }
    }
    if (! name.isEmpty() && ! filter.isEmpty()) {
        workoutFilters.append(StorableWorkoutFilter(name, filter));
        return true;
    }
    return false;
}



TopLevelFilterProxyModel::TopLevelFilterProxyModel
(QObject *parent)
: QSortFilterProxyModel(parent)
{
}


void
TopLevelFilterProxyModel::setContains
(const QString &contains)
{
    this->contains = contains;
    invalidateFilter();
}


bool
TopLevelFilterProxyModel::filterAcceptsRow
(int sourceRow, const QModelIndex &sourceParent) const
{
    if (! contains.isEmpty() && sourceParent == QModelIndex()) {    // ignore children
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        return index.data(Qt::DisplayRole).toString().contains(contains, Qt::CaseInsensitive);
    } else {
        return true;
    }
}



WorkoutMenuProvider::WorkoutMenuProvider
(WorkoutFilterBox *editor, const QString &filterFile)
: QObject(), editor(editor), store(filterFile)
{
    store.load();
}


void
WorkoutMenuProvider::addActions
(QMenu *menu) const
{
    bool editorDiffers = true;
    QString editorText = editor->text().trimmed();

    QList<WorkoutFilterStore::StorableWorkoutFilter> filters = store.getWorkoutFilters();
    for (const WorkoutFilterStore::StorableWorkoutFilter &filter : filters) {
        QAction *action = menu->addAction(filter.name);
        connect(action, &QAction::triggered, this, [this, filter]() {
            editor->setText(filter.filter);
        });
        editorDiffers &= (filter.filter != editorText);
    }

    if (filters.count() > 0) {
        menu->addSeparator();
    }
    QAction *tagAction = menu->addAction(tr("Select Tags") + "...");
    tagAction->setEnabled(trainDB->getTags().count() > 0);
    menu->addSeparator();
    QAction *manageAction = menu->addAction(tr("Manage Filters") + "...");
    QAction *addAction = menu->addAction(tr("Add Filter") + "...");
    addAction->setEnabled(editorDiffers && ! editor->text().trimmed().isEmpty());  // Disable if editor is empty or filter already added

    connect(tagAction, &QAction::triggered, this, &WorkoutMenuProvider::addTagDialog);
    connect(manageAction, &QAction::triggered, this, &WorkoutMenuProvider::manageFiltersDialog);
    connect(addAction, &QAction::triggered, this, &WorkoutMenuProvider::addFilterDialog);
}


void
WorkoutMenuProvider::addTagDialog
()
{
    QDialog dialog;
    dialog.setWindowTitle(tr("Add Tag to Filter"));

    QLineEdit *filterEdit = new QLineEdit();
    filterEdit->setPlaceholderText(tr("Filter Tags..."));
    filterEdit->setClearButtonEnabled(true);

    QStandardItemModel itemModel;
    QList<TagStore::Tag> tags = trainDB->getTags();
    for (const TagStore::Tag &tag : tags) {
        itemModel.appendRow(new QStandardItem(tag.label));
    }

    TopLevelFilterProxyModel *filterModel = new TopLevelFilterProxyModel();
    filterModel->setSourceModel(&itemModel);

    QListView *tagList = new QListView();
    tagList->setAlternatingRowColors(true);
    tagList->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tagList->setSelectionMode(QAbstractItemView::MultiSelection);
    tagList->setModel(filterModel);

    QCheckBox *matchMode = new QCheckBox(tr("Match all selected tags"));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->addWidget(filterEdit);
    layout->addWidget(tagList);
    layout->addWidget(matchMode);
    layout->addWidget(buttonBox);

    connect(filterEdit, &WorkoutFilterBox::textChanged, this, [filterModel](const QString &text) {
        filterModel->setContains(text.trimmed());
    });
    connect(tagList->selectionModel(), &QItemSelectionModel::selectionChanged, this, [buttonBox, tagList]() {
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(tagList->selectionModel()->hasSelection());
    });
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        QModelIndexList selection = tagList->selectionModel()->selectedIndexes();
        if (selection.count() > 0) {
            QStringList addTags;
            for (const QModelIndex &index : selection) {
                QString tagText(index.data(Qt::DisplayRole).toString().trimmed());
                if (tagText.contains(" ")) {
                    tagText = "\"" + tagText + "\"";
                }
                addTags << tagText;
            }
            QString addText = addTags.join(matchMode->isChecked() ? ", " : " ");
            if (editor->text().trimmed().size() > 0) {
                editor->setText(QString("%1, %2").arg(editor->text()).arg(addText));
            } else {
                editor->setText(addText);
            }
        }
    }
}


void
WorkoutMenuProvider::manageFiltersDialog
()
{
    QDialog dialog;
    dialog.setWindowTitle(tr("Manage Workout Filters"));
    dialog.resize(800 * dpiXFactor, 600 * dpiYFactor);

    NegativeListEditDelegate nameDelegate({ "" });
    NegativeListEditDelegate filterDelegate({ "" });

    QTreeWidget *filterTree = new QTreeWidget();
    filterTree->headerItem()->setText(0, tr("Name"));
    filterTree->headerItem()->setText(1, tr("Filter"));
    filterTree->headerItem()->setText(2, tr("_index"));
    filterTree->setColumnCount(3);
    filterTree->setColumnHidden(2, true);
    filterTree->setItemDelegateForColumn(0, &nameDelegate);
    filterTree->setItemDelegateForColumn(1, &filterDelegate);
    basicTreeWidgetStyle(filterTree);

    ActionButtonBox *actionButtons = new ActionButtonBox(ActionButtonBox::UpDownGroup | ActionButtonBox::AddDeleteGroup);
    QPushButton *execute = actionButtons->addButton(tr("Execute"));
    QPushButton *update = actionButtons->addButton(tr("Update"));
    actionButtons->defaultConnect(ActionButtonBox::UpDownGroup, filterTree);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->addWidget(filterTree);
    layout->addWidget(actionButtons);
    layout->addWidget(buttonBox);

    connect(filterTree, &QTreeWidget::itemChanged, this, &WorkoutMenuProvider::manageItemChanged);
    QMetaObject::Connection conn = connect(this, &WorkoutMenuProvider::itemsRecreated, this, [this, filterTree, actionButtons, execute, update]() {
        bool hasItem = filterTree->currentItem() != nullptr;
        actionButtons->setButtonEnabled(ActionButtonBox::Delete, hasItem);
        execute->setEnabled(hasItem);
        update->setEnabled(hasItem && ! editor->text().trimmed().isEmpty());
    });
    connect(execute, &QPushButton::clicked, this, [this, filterTree]() {
        if (filterTree->currentItem() != nullptr) {
            editor->setText(filterTree->currentItem()->data(1, Qt::DisplayRole).toString());
        }
    });
    connect(update, &QPushButton::clicked, this, [this, filterTree]() {
        if (filterTree->currentItem() != nullptr && ! editor->text().trimmed().isEmpty()) {
            int index = filterTree->currentItem()->data(2, Qt::DisplayRole).toInt();
            QString name = filterTree->currentItem()->data(0, Qt::DisplayRole).toString();
            store.update(index, name, editor->text().trimmed());
            manageFillItems(filterTree, index);
        }
    });
    connect(actionButtons, &ActionButtonBox::upRequested, this, [this, filterTree]() { manageMoveUp(filterTree->currentItem()); });
    connect(actionButtons, &ActionButtonBox::downRequested, this, [this, filterTree]() { manageMoveDown(filterTree->currentItem()); });
    connect(actionButtons, &ActionButtonBox::addRequested, this, [this, filterTree]() { manageAdd(filterTree); });
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, [this, filterTree]() { manageDelete(filterTree->currentItem()); });
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::accept);
    QMetaObject::Connection updateEnabledConn = connect(editor, &WorkoutFilterBox::textChanged, this, [update](const QString &text) {
        update->setEnabled(! text.trimmed().isEmpty());
    });

    manageFillItems(filterTree);

    if (dialog.exec() == QDialog::Accepted) {
        store.save();
    }
    disconnect(conn);
    disconnect(updateEnabledConn);
}


void
WorkoutMenuProvider::addFilterDialog
()
{
    QDialog dialog;
    dialog.setWindowTitle(tr("Add Workout Filter"));

    QLineEdit *nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText(tr("Name of the Filter..."));
    nameEdit->setClearButtonEnabled(true);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->addWidget(nameEdit);
    layout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(nameEdit, &QLineEdit::textChanged, this, [buttonBox](const QString &text) {
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(! text.trimmed().isEmpty());
    });
    nameEdit->setClearButtonEnabled(true);

    if (dialog.exec() == QDialog::Accepted) {
        store.append(nameEdit->text(), editor->text());
        store.save();
    }
}


void
WorkoutMenuProvider::manageItemChanged
(QTreeWidgetItem *item)
{
    if (item != nullptr) {
        store.update(item->data(2, Qt::DisplayRole).toInt(), item->data(0, Qt::DisplayRole).toString(), item->data(1, Qt::DisplayRole).toString());
        manageFillItems(item->treeWidget(), item->data(2, Qt::DisplayRole).toInt());
    }
}


void
WorkoutMenuProvider::manageAdd
(QTreeWidget *tree)
{
    int index = store.getWorkoutFilters().count();
    if (tree->currentItem() != nullptr) {
        index = tree->currentItem()->data(2, Qt::DisplayRole).toInt();
    }
    QString token = tr("New");
    for (int i = 0; tree->findItems(token, Qt::MatchExactly, 0).count() > 0 || tree->findItems(token, Qt::MatchExactly, 1).count() > 0; i++) {
        token = tr("New (%1)").arg(i + 1);
    }
    store.insert(index, token, token);
    manageFillItems(tree, index);
}


void
WorkoutMenuProvider::manageMoveUp
(QTreeWidgetItem *item)
{
    if (item != nullptr) {
        store.moveUp(item->data(2, Qt::DisplayRole).toInt());
        manageFillItems(item->treeWidget(), item->data(2, Qt::DisplayRole).toInt() - 1);
    }
}


void
WorkoutMenuProvider::manageMoveDown
(QTreeWidgetItem *item)
{
    if (item != nullptr) {
        store.moveDown(item->data(2, Qt::DisplayRole).toInt());
        manageFillItems(item->treeWidget(), item->data(2, Qt::DisplayRole).toInt() + 1);
    }
}


void
WorkoutMenuProvider::manageDelete
(QTreeWidgetItem *item)
{
    if (item != nullptr) {
        store.remove(item->data(2, Qt::DisplayRole).toInt());
        manageFillItems(item->treeWidget(), item->data(2, Qt::DisplayRole).toInt());
    }
}


void
WorkoutMenuProvider::manageFillItems
(QTreeWidget *tree, int selected)
{
    tree->blockSignals(true);
    tree->clear();
    QList<WorkoutFilterStore::StorableWorkoutFilter> filters = store.getWorkoutFilters();
    int select = std::min(selected, int(filters.count() - 1));
    QTreeWidgetItem *selectedItem = nullptr;
    int i = 0;
    for (const WorkoutFilterStore::StorableWorkoutFilter &filter : filters) {
        QTreeWidgetItem *add = new QTreeWidgetItem(tree->invisibleRootItem());
        add->setFlags(add->flags() | Qt::ItemIsEditable);
        add->setData(0, Qt::DisplayRole, filter.name);
        add->setData(1, Qt::DisplayRole, filter.filter);
        add->setData(2, Qt::DisplayRole, i);
        if (select == i) {
            selectedItem = add;
        }
        ++i;
    }
    tree->blockSignals(false);
    if (selectedItem != nullptr) {
        tree->setCurrentItem(selectedItem);
    }
    emit itemsRecreated();
}
