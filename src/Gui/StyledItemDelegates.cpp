/*
 * Copyright (c) 2024 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "StyledItemDelegates.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QStandardPaths>
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


// ComboBoxDelegate ///////////////////////////////////////////////////////////////

ComboBoxDelegate::ComboBoxDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
    fillSizeHint();
}


void
ComboBoxDelegate::addItems
(const QStringList &texts)
{
    this->texts = texts;
    fillSizeHint();
}


void
ComboBoxDelegate::commitAndCloseEditor
()
{
    QWidget *editor = qobject_cast<QWidget*>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}


QWidget*
ComboBoxDelegate::createEditor
(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    QComboBox *combobox = new QComboBox(parent);
    combobox->addItems(texts);

    connect(combobox, SIGNAL(activated(int)), this, SLOT(commitAndCloseEditor()));

    return combobox;
}


void
ComboBoxDelegate::setEditorData
(QWidget *editor, const QModelIndex &index) const
{
    QComboBox *combobox = static_cast<QComboBox*>(editor);
    combobox->setCurrentIndex(index.data(Qt::DisplayRole).toInt());
}


void
ComboBoxDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *combobox = static_cast<QComboBox*>(editor);
    int newValue = combobox->currentIndex();
    if (model->data(index, Qt::DisplayRole).toInt() != newValue) {
        model->setData(index, newValue, Qt::DisplayRole);
    }
}


QString
ComboBoxDelegate::displayText
(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale);

    int index = value.toInt();
    if (index >= 0 && index < texts.length()) {
        return texts[index];
    } else {
        return QString("INDEX OUT OF RANGE: %1 with %2 texts").arg(index).arg(texts.length());
    }
}


QSize
ComboBoxDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return _sizeHint;
}


void
ComboBoxDelegate::fillSizeHint
()
{
    QComboBox widget;
    widget.addItems(texts);
    _sizeHint = widget.sizeHint();
}


// DirectoryPathWidget /////////////////////////////////////////////////////////////////

DirectoryPathWidget::DirectoryPathWidget
(QWidget *parent)
: QWidget(parent)
{
    lineEdit = new QLineEdit();

    openButton = new QPushButton(tr("Browse"));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(lineEdit, 1);
    layout->addWidget(openButton, 0);

    connect(openButton, SIGNAL(clicked()), this, SLOT(openDialog()));
    connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(lineEditFinished()));
}


void
DirectoryPathWidget::setPath
(const QString &path)
{
    lineEdit->setText(path);
}


QString
DirectoryPathWidget::getPath
() const
{
    return lineEdit->text().trimmed();
}


void
DirectoryPathWidget::setPlaceholderText
(const QString &placeholderText)
{
    lineEdit->setPlaceholderText(placeholderText);
}


void
DirectoryPathWidget::openDialog
()
{
    QFileDialog fileDialog(this);
    QStringList selectedDirs;
    fileDialog.setFileMode(QFileDialog::Directory);
    fileDialog.setOptions(QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    QString path = lineEdit->text();
    if (path.isEmpty()) {
        path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    fileDialog.setDirectory(path);
    if (fileDialog.exec()) {
        selectedDirs = fileDialog.selectedFiles();
    }
    if (selectedDirs.count() > 0) {
        QString dir = selectedDirs.at(0);
        if (dir != "") {
            lineEdit->setText(dir);
        }
    }
    emit editingFinished();
}


void
DirectoryPathWidget::lineEditFinished
()
{
    // Filter out duplicate editingFinished-signals emitted by QLineEdit when pressing enter
    if (lineEditAlreadyFinished) {
        return;
    }
    lineEditAlreadyFinished = true;

    if (! openButton->hasFocus()) {
        emit editingFinished();
    }
}


// DirectoryPathDelegate ///////////////////////////////////////////////////////////////

DirectoryPathDelegate::DirectoryPathDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


QWidget*
DirectoryPathDelegate::createEditor
(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    DirectoryPathWidget *editor = new DirectoryPathWidget(parent);
    editor->setPlaceholderText(placeholderText);
    connect(editor, SIGNAL(editingFinished()), this, SLOT(commitAndCloseEditor()));

    return editor;
}


void
DirectoryPathDelegate::setEditorData
(QWidget *editor, const QModelIndex &index) const
{
    DirectoryPathWidget *filepath = static_cast<DirectoryPathWidget*>(editor);
    filepath->setPath(index.data(Qt::DisplayRole).toString());
}


void
DirectoryPathDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    DirectoryPathWidget *filepath = static_cast<DirectoryPathWidget*>(editor);
    model->setData(index, filepath->getPath(), Qt::DisplayRole);
}


QString
DirectoryPathDelegate::displayText
(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale)

    QString text = value.toString();
    if (text.isEmpty() && ! placeholderText.isEmpty()) {
        return placeholderText;
    }
    return text;
}


QSize
DirectoryPathDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    DirectoryPathWidget widget;
    widget.setPath(index.data(Qt::DisplayRole).toString());
    QSize size = widget.sizeHint();

    QLineEdit le(index.data(Qt::DisplayRole).toString());
    QSize textSize = le.fontMetrics().size(0, le.text());
    QMargins textMargins = le.textMargins();
    QSize textMarginsSize = QSize(textMargins.left() + textMargins.right(), textMargins.top() + textMargins.bottom());
    QMargins contentsMargins = le.contentsMargins();
    QSize contentsMarginsSize = QSize(contentsMargins.left() + contentsMargins.right(), contentsMargins.top() + contentsMargins.bottom());
    QSize extraSize = QSize(8, 4);
    QSize contentsSize = textSize + textMarginsSize + contentsMarginsSize + extraSize;
    QSize leSize = le.style()->sizeFromContents(QStyle::CT_LineEdit, &option, contentsSize);

    size = size.expandedTo(leSize);
    if (maxWidth > 0) {
        size = size.boundedTo(QSize(maxWidth, 1000));
    }

    return size;
}


void
DirectoryPathDelegate::setMaxWidth
(int maxWidth)
{
    this->maxWidth = maxWidth;
}


void
DirectoryPathDelegate::setPlaceholderText
(const QString &placeholderText)
{
    this->placeholderText = placeholderText;
}


void
DirectoryPathDelegate::commitAndCloseEditor
()
{
    QWidget *editor = qobject_cast<QWidget*>(sender());
    emit commitData(editor);
    emit closeEditor(editor);
}


// SpinBoxEditDelegate ////////////////////////////////////////////////////////////

SpinBoxEditDelegate::SpinBoxEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
    fillSizeHint();
}


void
SpinBoxEditDelegate::setMinimum
(int minimum)
{
    this->minimum = minimum;
    fillSizeHint();
}


void
SpinBoxEditDelegate::setMaximum
(int maximum)
{
    this->maximum = maximum;
    fillSizeHint();
}


void
SpinBoxEditDelegate::setRange
(int minimum, int maximum)
{
    this->minimum = minimum;
    this->maximum = maximum;
    fillSizeHint();
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
    fillSizeHint();
}


void
SpinBoxEditDelegate::setShowSuffixOnEdit
(bool show)
{
    showSuffixOnEdit = show;
    fillSizeHint();
}


void
SpinBoxEditDelegate::setShowSuffixOnDisplay
(bool show)
{
    showSuffixOnDisplay = show;
    fillSizeHint();
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
    if (suffix.length() > 0) {
        spinbox->setSuffix(" " + suffix);
    } else {
        spinbox->setSuffix("");
    }

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
SpinBoxEditDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return _sizeHint;
}


void
SpinBoxEditDelegate::fillSizeHint
()
{
    QSpinBox widget;
    if ((showSuffixOnEdit || showSuffixOnDisplay) && ! suffix.isEmpty()) {
        widget.setSuffix(" " + suffix);
    }
    widget.setMaximum(maximum);
    widget.setValue(maximum);
    _sizeHint = widget.sizeHint();
}


// DoubleSpinBoxEditDelegate //////////////////////////////////////////////////////

DoubleSpinBoxEditDelegate::DoubleSpinBoxEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
    fillSizeHint();
}


void
DoubleSpinBoxEditDelegate::setMinimum
(double minimum)
{
    this->minimum = minimum;
    fillSizeHint();
}


void
DoubleSpinBoxEditDelegate::setMaximum
(double maximum)
{
    this->maximum = maximum;
    fillSizeHint();
}


void
DoubleSpinBoxEditDelegate::setRange
(double minimum, double maximum)
{
    this->minimum = minimum;
    this->maximum = maximum;
    fillSizeHint();
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
    fillSizeHint();
}


void
DoubleSpinBoxEditDelegate::setSuffix
(const QString &suffix)
{
    this->suffix = suffix;
    fillSizeHint();
}


void
DoubleSpinBoxEditDelegate::setShowSuffixOnEdit
(bool show)
{
    showSuffixOnEdit = show;
    fillSizeHint();
}


void
DoubleSpinBoxEditDelegate::setShowSuffixOnDisplay
(bool show)
{
    showSuffixOnDisplay = show;
    fillSizeHint();
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
    if (suffix.length() > 0) {
        spinbox->setSuffix(" " + suffix);
    } else {
        spinbox->setSuffix("");
    }

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
DoubleSpinBoxEditDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return _sizeHint;
}


void
DoubleSpinBoxEditDelegate::fillSizeHint
()
{
    QDoubleSpinBox widget;
    if ((showSuffixOnEdit || showSuffixOnDisplay) && ! suffix.isEmpty()) {
        widget.setSuffix(" " + suffix);
    }
    widget.setMaximum(maximum);
    widget.setValue(maximum);
    widget.setDecimals(prec);
    _sizeHint = widget.sizeHint();
}


// DateEditDelegate ///////////////////////////////////////////////////////////////

DateEditDelegate::DateEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
    fillSizeHint();
}


void
DateEditDelegate::setCalendarPopup
(bool enable)
{
    calendarPopup = enable;
    fillSizeHint();
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
    }
}


QString
DateEditDelegate::displayText
(const QVariant &value, const QLocale &locale) const
{
    return value.toDate().toString(locale.dateFormat(QLocale::ShortFormat));
}


QSize
DateEditDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return _sizeHint;
}


void
DateEditDelegate::fillSizeHint
()
{
    QDateEdit widget;
    widget.setCalendarPopup(calendarPopup);
    _sizeHint = widget.sizeHint();
}


// TimeEditDelegate ///////////////////////////////////////////////////////////////

TimeEditDelegate::TimeEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
    fillSizeHint();
}


void
TimeEditDelegate::setFormat
(const QString &format)
{
    this->format = format;
    fillSizeHint();
}


void
TimeEditDelegate::setSuffix
(const QString &suffix)
{
    this->suffix = suffix;
    fillSizeHint();
}


void
TimeEditDelegate::setShowSuffixOnEdit
(bool show)
{
    showSuffixOnEdit = show;
    fillSizeHint();
}


void
TimeEditDelegate::setShowSuffixOnDisplay
(bool show)
{
    showSuffixOnDisplay = show;
    fillSizeHint();
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
TimeEditDelegate::sizeHint
(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    return _sizeHint;
}


void
TimeEditDelegate::fillSizeHint
()
{
    QTimeEdit widget;
    widget.setTime(QTime(0, 0, 0));
    QString f = format;
    if (f.isEmpty()) {
        f = "mm:ss";
    }
    if ((showSuffixOnEdit || showSuffixOnDisplay) && ! suffix.isEmpty()) {
        f += " '" + suffix + "'";
    }
    widget.setDisplayFormat(f);
    _sizeHint = widget.sizeHint();
}
