#include "StyledItemDelegates.h"

#include <QDoubleSpinBox>
#include <QDateEdit>


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


// SpinBoxEditDelegate ////////////////////////////////////////////////////////////

SpinBoxEditDelegate::SpinBoxEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
SpinBoxEditDelegate::setMininum
(int minimum)
{
    this->minimum = minimum;
}


void
SpinBoxEditDelegate::setMaximum
(int maximum)
{
    this->maximum = maximum;
}


void
SpinBoxEditDelegate::setRange
(int minimum, int maximum)
{
    this->minimum = minimum;
    this->maximum = maximum;
}


void
SpinBoxEditDelegate::setSingleStep
(int val)
{
    singleStep = val;
}


void
SpinBoxEditDelegate::setSuffix
(const QString &suffix)
{
    this->suffix = suffix;
}


QWidget*
SpinBoxEditDelegate::createEditor
(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    QSpinBox *spinbox = new QSpinBox(parent);
    spinbox->setRange(minimum, maximum);
    spinbox->setSingleStep(singleStep);
    spinbox->setSuffix(suffix);

    return spinbox;
}


void
SpinBoxEditDelegate::setEditorData
(QWidget *editor, const QModelIndex &index) const
{
    int value = index.model()->data(index, Qt::EditRole).toInt();
    QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
    spinbox->setValue(value);
}


void
SpinBoxEditDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
    spinbox->interpretText();
    int newValue = spinbox->value();
    if (model->data(index, Qt::EditRole).toInt() != newValue) {
        model->setData(index, newValue, Qt::EditRole);
        model->setData(index, editor->sizeHint(), Qt::SizeHintRole);
    }
}


QSize
SpinBoxEditDelegate::staticSizeHint
()
{
    QSpinBox widget;
    return widget.sizeHint();
}


// DoubleSpinBoxEditDelegate //////////////////////////////////////////////////////

DoubleSpinBoxEditDelegate::DoubleSpinBoxEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
DoubleSpinBoxEditDelegate::setMininum
(double minimum)
{
    this->minimum = minimum;
}


void
DoubleSpinBoxEditDelegate::setMaximum
(double maximum)
{
    this->maximum = maximum;
}


void
DoubleSpinBoxEditDelegate::setRange
(double minimum, double maximum)
{
    this->minimum = minimum;
    this->maximum = maximum;
}


void
DoubleSpinBoxEditDelegate::setSingleStep
(double val)
{
    singleStep = val;
}


void
DoubleSpinBoxEditDelegate::setDecimals
(int prec)
{
    this->prec = prec;
}


void
DoubleSpinBoxEditDelegate::setSuffix
(const QString &suffix)
{
    this->suffix = suffix;
}


QWidget*
DoubleSpinBoxEditDelegate::createEditor
(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    QDoubleSpinBox *spinbox = new QDoubleSpinBox(parent);
    spinbox->setRange(minimum, maximum);
    spinbox->setSingleStep(singleStep);
    spinbox->setDecimals(prec);
    spinbox->setSuffix(suffix);

    return spinbox;
}


void
DoubleSpinBoxEditDelegate::setEditorData
(QWidget *editor, const QModelIndex &index) const
{
    double value = index.model()->data(index, Qt::EditRole).toDouble();
    QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
    spinbox->setValue(value);
}


void
DoubleSpinBoxEditDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
    spinbox->interpretText();
    double newValue = spinbox->value();
    if (model->data(index, Qt::EditRole).toDouble() != newValue) {
        model->setData(index, newValue, Qt::EditRole);
        model->setData(index, editor->sizeHint(), Qt::SizeHintRole);
    }
}


QSize
DoubleSpinBoxEditDelegate::staticSizeHint
()
{
    QDoubleSpinBox widget;
    return widget.sizeHint();
}


// DateEditDelegate ///////////////////////////////////////////////////////////////

DateEditDelegate::DateEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
DateEditDelegate::setCalendarPopup
(bool enable)
{
    calendarPopup = enable;
}


QWidget*
DateEditDelegate::createEditor
(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    QDateEdit *dateEdit = new QDateEdit(parent);
    dateEdit->setCalendarPopup(calendarPopup);

    return dateEdit;
}


void
DateEditDelegate::setEditorData
(QWidget *editor, const QModelIndex &index) const
{
    QDate value = index.model()->data(index, Qt::EditRole).toDate();
    QDateEdit *dateEdit = static_cast<QDateEdit*>(editor);
    dateEdit->setDate(value);
}


void
DateEditDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QDateEdit *dateEdit = static_cast<QDateEdit*>(editor);
    QDate newValue = dateEdit->date();
    if (model->data(index, Qt::EditRole).toDate() != newValue) {
        model->setData(index, newValue, Qt::EditRole);
        model->setData(index, editor->sizeHint(), Qt::SizeHintRole);
    }
}


QSize
DateEditDelegate::staticSizeHint
()
{
    QDateEdit widget;
    return widget.sizeHint();
}
