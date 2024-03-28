#include "StyledItemDelegates.h"


// NoEditDelegate /////////////////////////////////////////////////////////////////

NoEditDelegate::NoEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


QWidget*
NoEditDelegate::createEditor
(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(parent);
    Q_UNUSED(option);
    Q_UNUSED(index);
    return nullptr;
}


// UniqueLabelEditDelegate ////////////////////////////////////////////////////////

UniqueLabelEditDelegate::UniqueLabelEditDelegate
(int uniqueColumn, QObject *parent)
: QStyledItemDelegate(parent), uniqueColumn(uniqueColumn)
{
}


void
UniqueLabelEditDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QString newData = editor->property("text").toString().trimmed();
    if (newData.isEmpty() || newData == index.data().toString()) {
        return;
    }
    int rowCount = model->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        if (newData == model->index(i, uniqueColumn).data().toString()) {
            return;
        }
    }
    QModelIndex modifiedIndex = model->sibling(index.row(), model->columnCount() - 1, index);

    model->setData(index, newData, Qt::EditRole);
    model->setData(modifiedIndex, true, Qt::EditRole);
}
