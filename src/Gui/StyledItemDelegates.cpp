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

#include <QLineEdit>
#include <QCompleter>
#include <QStringListModel>
#include <QComboBox>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QMessageBox>
#include <QDialogButtonBox>

#include "Colors.h"


// ColorDelegate //////////////////////////////////////////////////////////////////

ColorDelegate::ColorDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
ColorDelegate::paint
(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    QColor textColor;
    QVariant colorVariant = index.data(Qt::UserRole);
    if (colorVariant.canConvert<QColor>()) {
        textColor = colorVariant.value<QColor>();
    }
    QColor backgroundColor = option.palette.base().color();
    if (option.state & QStyle::State_Selected) {
        backgroundColor = option.palette.highlight().color();
        textColor = option.palette.highlightedText().color();
    } else if (! backgroundColor.isValid()) {
        textColor = option.palette.text().color();
    }
    painter->fillRect(option.rect, backgroundColor);
    QString text = index.data(Qt::DisplayRole).toString();
    painter->setPen(textColor);
    painter->drawText(option.rect.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, text);
    painter->restore();
}


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



// NegativeListEditDelegate ///////////////////////////////////////////////////////

NegativeListEditDelegate::NegativeListEditDelegate
(const QStringList &negativeList, QObject *parent)
: QStyledItemDelegate(parent), negativeList(negativeList)
{
}


void
NegativeListEditDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QString newData = editor->property("text").toString().trimmed();
    if (newData == index.data().toString() || negativeList.contains(newData)) {
        return;
    }
    model->setData(index, newData, Qt::EditRole);
}



// UniqueLabelEditDelegate ////////////////////////////////////////////////////////

UniqueLabelEditDelegate::UniqueLabelEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
UniqueLabelEditDelegate::setNegativeList
(const QStringList &negativeList)
{
    this->negativeList = negativeList;
}


void
UniqueLabelEditDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QString newData = editor->property("text").toString().trimmed();
    if (newData.isEmpty() || newData == index.data().toString() || negativeList.contains(newData)) {
        return;
    }
    int rowCount = model->rowCount();
    for (int i = 0; i < rowCount; ++i) {
        if (newData == model->index(i, index.column()).data().toString()) {
            QMessageBox msgBox;
            msgBox.setText(tr("The given value \"%1\" is not unique").arg(newData));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.exec();
            return;
        }
    }
    model->setData(index, newData, Qt::EditRole);
}


// CompleterEditDelegate //////////////////////////////////////////////////////////

CompleterEditDelegate::CompleterEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
CompleterEditDelegate::setDataSource
(DataSource source)
{
    this->source = source;
}


void
CompleterEditDelegate::setCompletionList
(const QStringList &list)
{
    completionList = list;
}


QWidget*
CompleterEditDelegate::createEditor
(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    QLineEdit *editor = new QLineEdit(parent);
    QStringListModel *completionModel = new QStringListModel();
    QCompleter *completer = new QCompleter(completionModel);
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);
    completer->setWrapAround(true);
    completer->setFilterMode(Qt::MatchContains);
    completer->setModelSorting(QCompleter::UnsortedModel);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    editor->setCompleter(completer);

    return editor;
}


void
CompleterEditDelegate::setEditorData
(QWidget *editor, const QModelIndex &index) const
{
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    lineEdit->setText(index.data(Qt::DisplayRole).toString());
    if (lineEdit->completer() != nullptr) {
        QStringListModel *completionModel = static_cast<QStringListModel*>(lineEdit->completer()->model());
        if (source == Column) {
            QSet<QString> items;
            int rowCount = index.model()->rowCount();
            for (int i = 0; i < rowCount; ++i) {
                items.insert(index.model()->index(i, index.column()).data().toString());
            }
            completionModel->setStringList(items.values());
        } else {
            completionModel->setStringList(completionList);
        }
    }
}


void
CompleterEditDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QLineEdit *lineEdit = static_cast<QLineEdit*>(editor);
    QString newValue = lineEdit->text();
    if (model->data(index, Qt::DisplayRole).toString() != newValue) {
        model->setData(index, newValue, Qt::DisplayRole);
    }
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

    browseButton = new QPushButton(tr("Browse"));

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(lineEdit, 1);
    layout->addWidget(browseButton, 0);

    connect(browseButton, &QPushButton::clicked, this, &DirectoryPathWidget::handleBrowseClicked);
    connect(lineEdit, &QLineEdit::editingFinished, this, &DirectoryPathWidget::lineEditFinished);
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
DirectoryPathWidget::setDelegateMode
(bool delegateMode)
{
    this->delegateMode = delegateMode;
}


void
DirectoryPathWidget::handleBrowseClicked
()
{
    if (delegateMode) {
        QTimer::singleShot(0, this, [=]() {
            openFileDialog();
        });
    } else {
        openFileDialog();
    }
}


void
DirectoryPathWidget::openFileDialog
()
{
#ifdef Q_OS_MACOS
    // On macOS in delegate mode, use Qt's cross-platform dialog for maximum safety.
    // DontUseNativeDialog ensures consistent behavior and prevents delegate lifecycle issues.
    if (delegateMode) {
        QFileDialog dialog(window());
        dialog.setFileMode(QFileDialog::Directory);
        dialog.setOptions(  QFileDialog::ShowDirsOnly
                          | QFileDialog::DontResolveSymlinks
                          | QFileDialog::DontUseNativeDialog);  // Use Qt dialog for safety
        QString path = lineEdit->text();
        if (path.isEmpty()) {
            path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        }
        dialog.setDirectory(path);
        // Store result in local variables (independent of widget lifecycle)
        QString selectedPath;
        bool accepted = false;
        if (dialog.exec() == QDialog::Accepted) {
            const QStringList selectedDirs = dialog.selectedFiles();
            if (! selectedDirs.isEmpty()) {
                selectedPath = selectedDirs.first();
                accepted = ! selectedPath.isEmpty();
            }
        }
        if (accepted && !selectedPath.isEmpty()) {
            setPath(selectedPath);  // Safe because native dialog prevents delegate destruction
        }
        QCoreApplication::processEvents();
        emit editingFinished(accepted);
        return;
    }
#endif

    // Standard modal dialog approach for non-delegate mode or non-macOS
    QFileDialog dialog(window());
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOptions(  QFileDialog::ShowDirsOnly
                      | QFileDialog::DontResolveSymlinks);
    QString path = lineEdit->text();
    if (path.isEmpty()) {
        path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    }
    dialog.setDirectory(path);
    bool accepted = false;
    if (dialog.exec() == QDialog::Accepted) {
        const QStringList selectedDirs = dialog.selectedFiles();
        if (! selectedDirs.isEmpty()) {
            QString selectedDir = selectedDirs.first();
            if (! selectedDir.isEmpty()) {
                setPath(selectedDir);
                accepted = true;
            }
        }
    }
    emit editingFinished(accepted);
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

    if (! browseButton->hasFocus()) {
        emit editingFinished(true);
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
    editor->setDelegateMode(true);
    editor->setPlaceholderText(placeholderText);

    DirectoryPathDelegate *delegate = const_cast<DirectoryPathDelegate*>(this);
    connect(editor, &DirectoryPathWidget::editingFinished, delegate, [delegate, editor](bool accepted) {
        if (accepted) {
            emit delegate->commitData(editor);
        }
        emit delegate->closeEditor(editor);
    });

    return editor;
}


void
DirectoryPathDelegate::setEditorData
(QWidget *editor, const QModelIndex &index) const
{
    if (DirectoryPathWidget *pathEditor = qobject_cast<DirectoryPathWidget*>(editor)) {
        pathEditor->setPath(index.data(Qt::EditRole).toString());
    }
}


void
DirectoryPathDelegate::setModelData
(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (DirectoryPathWidget *pathEditor = qobject_cast<DirectoryPathWidget*>(editor)) {
        model->setData(index, pathEditor->getPath(), Qt::EditRole);
    }
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


// ListEditWidget //////////////////////////////////////////////////////////////////////

ListEditWidget::ListEditWidget
(QWidget *parent)
: QWidget(parent)
{
    dialog = new QDialog(parent);
    dialog->setModal(true);

    title = new QLabel();
    title->setWordWrap(true);
    title->setVisible(false);

    listWidget = new QListWidget();
    listWidget->setEditTriggers(  QAbstractItemView::DoubleClicked
                                | QAbstractItemView::SelectedClicked
                                | QAbstractItemView::AnyKeyPressed);
    listWidget->setAlternatingRowColors(true);
    listWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    actionButtons = new ActionButtonBox(ActionButtonBox::UpDownGroup | ActionButtonBox::AddDeleteGroup);
    actionButtons->defaultConnect(ActionButtonBox::UpDownGroup, listWidget);
    actionButtons->defaultConnect(ActionButtonBox::AddDeleteGroup, listWidget);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    QVBoxLayout *dialogLayout = new QVBoxLayout(dialog);
    dialogLayout->addWidget(title);
    dialogLayout->addWidget(listWidget);
    dialogLayout->addWidget(actionButtons);
    dialogLayout->addSpacing(10 * dpiYFactor);
    dialogLayout->addWidget(buttons);

    connect(listWidget, &QListWidget::itemChanged, this, &ListEditWidget::itemChanged);
    connect(actionButtons, &ActionButtonBox::upRequested, this, &ListEditWidget::moveUp);
    connect(actionButtons, &ActionButtonBox::downRequested, this, &ListEditWidget::moveDown);
    connect(actionButtons, &ActionButtonBox::addRequested, this, &ListEditWidget::addItem);
    connect(actionButtons, &ActionButtonBox::deleteRequested, this, &ListEditWidget::deleteItem);
    connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(dialog, &QDialog::finished, this, &ListEditWidget::dialogFinished);
}


void
ListEditWidget::setTitle
(const QString &text)
{
    if (text.trimmed().size() > 0) {
        title->setText(text);
        title->setVisible(true);
    } else {
        title->setVisible(false);
    }
}


void
ListEditWidget::setList
(const QStringList &list)
{
    data = list;
    listWidget->clear();
    for (const QString &entry : list) {
        QListWidgetItem *add = new QListWidgetItem(entry, listWidget);
        add->setFlags(add->flags() | Qt::ItemIsEditable);
    }
    listWidget->setCurrentRow(0);
}


QStringList
ListEditWidget::getList
() const
{
    return data;
}


void
ListEditWidget::showDialog
()
{
    dialog->show();
}


void
ListEditWidget::itemChanged
(QListWidgetItem *item)
{
    if (item->data(Qt::DisplayRole).toString().trimmed().isEmpty()) {
        delete item;
    }
}


void
ListEditWidget::dialogFinished
(int result)
{
    if (result == QDialog::Accepted) {
        data.clear();
        for (int i = 0; i < listWidget->count(); ++i) {
            data << listWidget->item(i)->data(Qt::DisplayRole).toString().trimmed();
        }
    }
    emit editingFinished();
}


void
ListEditWidget::moveDown
()
{
    if (listWidget->currentItem()) {
        int index = listWidget->row(listWidget->currentItem());
        if (index > listWidget->count() - 1) {
            return;
        }
        QListWidgetItem *moved = listWidget->takeItem(index);
        listWidget->insertItem(index + 1, moved);
        listWidget->setCurrentItem(moved);
    }
}


void
ListEditWidget::moveUp
()
{
    if (listWidget->currentItem()) {
        int index = listWidget->row(listWidget->currentItem());
        if (index == 0) {
            return;
        }
        QListWidgetItem *moved = listWidget->takeItem(index);
        listWidget->insertItem(index - 1, moved);
        listWidget->setCurrentItem(moved);
    }
}


void
ListEditWidget::addItem
()
{
    int index = listWidget->count();
    if (listWidget->currentItem() != nullptr) {
        index = listWidget->row(listWidget->currentItem());
    }
    if (index < 0) {
        index = 0;
    }
    QListWidgetItem *add = new QListWidgetItem();
    add->setFlags(add->flags() | Qt::ItemIsEditable);
    listWidget->insertItem(index, add);
    QString text = tr("New");
    for (int i = 0; listWidget->findItems(text, Qt::MatchExactly).count() > 0; ++i) {
        text = tr("New (%1)").arg(i + 1);
    }
    add->setData(Qt::DisplayRole, text);
    listWidget->setCurrentItem(add);
    listWidget->editItem(add);
}


void
ListEditWidget::deleteItem
()
{
    if (listWidget->currentItem() != nullptr) {
        delete listWidget->currentItem();
    }
}



// ListEditDelegate ////////////////////////////////////////////////////////////////////

ListEditDelegate::ListEditDelegate
(QObject *parent)
: QStyledItemDelegate(parent)
{
}


void
ListEditDelegate::setTitle
(const QString &title)
{
    this->title = title;
}


void
ListEditDelegate::setDisplayLength
(int limit, int reduce)
{
    this->limit = limit;
    this->reduce = reduce;
}


QWidget*
ListEditDelegate::createEditor
(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)

    ListEditWidget *editor = new ListEditWidget(parent);
    editor->setTitle(title);
    connect(editor, &ListEditWidget::editingFinished, this, &ListEditDelegate::commitAndCloseEditor);
    editor->showDialog();

    return editor;
}


void
ListEditDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    ListEditWidget *listEditor = static_cast<ListEditWidget*>(editor);
    listEditor->setList(index.data(Qt::DisplayRole).toString().split(','));
}


void
ListEditDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ListEditWidget *listEditor = static_cast<ListEditWidget*>(editor);
    QString newValue = listEditor->getList().join(',');
    if (model->data(index, Qt::EditRole).toString() != newValue) {
        model->setData(index, newValue, Qt::EditRole);
    }
}


QString
ListEditDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    Q_UNUSED(locale)

    QString text = value.toString();
    if (limit > 0 && text.size() > limit) {
        text.truncate(limit - reduce);
        text += "...";
    }
    return text;
}


void
ListEditDelegate::commitAndCloseEditor
()
{
    ListEditWidget *editor = qobject_cast<ListEditWidget*>(sender());
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
    // There doesn't seem to be a built in way to prevent trailing zeros when converting floats to QString
    while (ret.back() == '0') {
        ret.chop(1);
    }
    if (ret.back() == '.') {
        ret.chop(1);
    }
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
