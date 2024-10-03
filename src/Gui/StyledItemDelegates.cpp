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


void
SpinBoxEditDelegate::setShowSuffixOnEdit
(bool show)
{
    showSuffixOnEdit = show;
}


void
SpinBoxEditDelegate::setShowSuffixOnDisplay
(bool show)
{
    showSuffixOnDisplay = show;
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


QString
SpinBoxEditDelegate::displayText
(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale)

    QString ret = QString::number(value.toInt());
    if (showSuffixOnDisplay && ! suffix.isEmpty()) {
        ret += " " + suffix;
    }
    return ret;
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


void
DoubleSpinBoxEditDelegate::setShowSuffixOnEdit
(bool show)
{
    showSuffixOnEdit = show;
}


void
DoubleSpinBoxEditDelegate::setShowSuffixOnDisplay
(bool show)
{
    showSuffixOnDisplay = show;
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


QString
DoubleSpinBoxEditDelegate::displayText
(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale)

    QString ret = QString::number(value.toDouble(), 'f', prec);
    if (showSuffixOnDisplay && ! suffix.isEmpty()) {
        ret += " " + suffix;
    }
    return ret;
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


QString
DateEditDelegate::displayText
(const QVariant &value, const QLocale &locale) const
{
    return value.toDate().toString(locale.dateFormat(QLocale::ShortFormat));
}


QSize
DateEditDelegate::staticSizeHint
()
{
    QDateEdit widget;
    return widget.sizeHint();
}


// TimeEditDelegate ///////////////////////////////////////////////////////////////

TimeEditDelegate::TimeEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
TimeEditDelegate::setFormat
(const QString &format)
{
    this->format = format;
}


void
TimeEditDelegate::setSuffix
(const QString &suffix)
{
    this->suffix = suffix;
}


void
TimeEditDelegate::setShowSuffixOnEdit
(bool show)
{
    showSuffixOnEdit = show;
}


void
TimeEditDelegate::setShowSuffixOnDisplay
(bool show)
{
    showSuffixOnDisplay = show;
}


void
TimeEditDelegate::setTimeRange
(const QTime &min, const QTime &max)
{
    this->min = min;
    this->max = max;
}


QWidget*
TimeEditDelegate::createEditor
(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    QTimeEdit *timeEdit = new QTimeEdit(parent);
    QString f = format;
    if (f.isEmpty()) {
        f = "mm:ss";
    }
    if (showSuffixOnEdit && ! suffix.isEmpty()) {
        f += " '" + suffix + "'";
    }
    timeEdit->setDisplayFormat(f);
    if (min.isValid()) {
        timeEdit->setMinimumTime(min);
    }
    if (max.isValid()) {
        timeEdit->setMaximumTime(max);
    }

    return timeEdit;
}


void
TimeEditDelegate::setEditorData
(QWidget *editor, const QModelIndex &index) const
{
    QTimeEdit *timeEdit = static_cast<QTimeEdit*>(editor);
    timeEdit->setTime(index.data(Qt::DisplayRole).toTime());
}


void
TimeEditDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QTimeEdit *timeEdit = static_cast<QTimeEdit*>(editor);
    QTime newValue = timeEdit->time();
    if (model->data(index, Qt::EditRole).toTime() != newValue) {
        model->setData(index, newValue, Qt::EditRole);
        model->setData(index, editor->sizeHint(), Qt::SizeHintRole);
    }
}


QString
TimeEditDelegate::displayText
(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale)

    QString f = format;
    if (f.isEmpty()) {
        f = "mm:ss";
    }
    if (showSuffixOnDisplay && ! suffix.isEmpty()) {
        f += " '" + suffix + "'";
    }

    return value.toTime().toString(f);
}


QSize
TimeEditDelegate::staticSizeHint
()
{
    QTimeEdit widget;
    return widget.sizeHint();
}
