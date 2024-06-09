#ifndef STYLEDITEMSDELEGATES_H
#define STYLEDITEMSDELEGATES_H

#include <QStyledItemDelegate>


class NoEditDelegate: public QStyledItemDelegate
{
public:
    NoEditDelegate(QObject *parent = nullptr);

    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};



class UniqueLabelEditDelegate: public QStyledItemDelegate
{
public:
    UniqueLabelEditDelegate(int uniqueColumn, QObject *parent = nullptr);

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:
    int uniqueColumn;
};

#endif
