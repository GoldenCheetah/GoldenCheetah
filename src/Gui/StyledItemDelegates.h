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

#ifndef STYLEDITEMSDELEGATES_H
#define STYLEDITEMSDELEGATES_H

#include <QStyledItemDelegate>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QDialog>
#include <QTime>
#include <QStringList>
#include <QStringListModel>
#include <QListWidget>

#include "ActionButtonBox.h"


class ColorDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    ColorDelegate(QObject *parent = nullptr);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};


class NoEditDelegate: public QStyledItemDelegate
{
public:
    NoEditDelegate(QObject *parent = nullptr);

    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};



class NegativeListEditDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    NegativeListEditDelegate(const QStringList &negativeList, QObject *parent = nullptr);

    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:
    QStringList negativeList;
};



class UniqueLabelEditDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    UniqueLabelEditDelegate(QObject *parent = nullptr);

    void setNegativeList(const QStringList &negativeList);

    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:
    QStringList negativeList;
};



class CompleterEditDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    enum DataSource {
        Column = 0,
        External
    };

    CompleterEditDelegate(QObject *parent = nullptr);

    void setDataSource(DataSource source);
    void setCompletionList(const QStringList &list);

    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:
    DataSource source = Column;
    QStringList completionList;
};



class ComboBoxDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    ComboBoxDelegate(QObject *parent = nullptr);

    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    virtual QString displayText(const QVariant &value, const QLocale &locale) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void addItems(const QStringList &texts);

private slots:
    void commitAndCloseEditor();

private:
    QStringList texts;
    QSize _sizeHint;

    void fillSizeHint();
};



class DirectoryPathWidget: public QWidget
{
    Q_OBJECT

public:
    DirectoryPathWidget(QWidget *parent = nullptr);

    QString getPath() const;
    void setPlaceholderText(const QString &placeholder);
    void setDelegateMode(bool delegateMode);

public slots:
    void setPath(const QString &path);

signals:
    void editingFinished(bool accepted);
#ifdef Q_OS_MACOS
    void browseRequested();
#endif

private:
    bool delegateMode = false;
    QPushButton *browseButton;
    QLineEdit *lineEdit;
    bool lineEditAlreadyFinished = false;

private slots:
    void handleBrowseClicked();
    void openFileDialog();
    void lineEditFinished();
};


class DirectoryPathDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    DirectoryPathDelegate(QObject *parent = nullptr);

    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    virtual QString displayText(const QVariant &value, const QLocale &locale) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setMaxWidth(int maxWidth);
    void setPlaceholderText(const QString &placeholderText);

private:
    int maxWidth = -1;
    QString placeholderText;
#ifdef Q_OS_MACOS
    void openFileDialogForEditor(QWidget *editor, const QModelIndex &index) const;
#endif
};



class ListEditWidget: public QWidget
{
    Q_OBJECT

public:
    ListEditWidget(QWidget *parent = nullptr);

    void setTitle(const QString &text);
    void setList(const QStringList &list);
    QStringList getList() const;

public slots:
    void showDialog();

private slots:
    void itemChanged(QListWidgetItem *item);
    void dialogFinished(int result);
    void moveDown();
    void moveUp();
    void addItem();
    void deleteItem();

signals:
    void editingFinished();

private:
    QStringList data;

    QDialog *dialog;
    QLabel *title;
    QListWidget *listWidget;
    ActionButtonBox *actionButtons;
};


class ListEditDelegate: public QStyledItemDelegate
{
    Q_OBJECT

public:
    ListEditDelegate(QObject *parent = nullptr);

    void setTitle(const QString &title);
    void setDisplayLength(int limit, int reduce = 2);

    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    virtual QString displayText(const QVariant &value, const QLocale &locale) const override;

private slots:
    void commitAndCloseEditor();

private:
    QString title;
    int limit = -1;
    int reduce = 2;
};



class SpinBoxEditDelegate: public QStyledItemDelegate
{
public:
    SpinBoxEditDelegate(QObject *parent = nullptr);

    void setMinimum(int minimum);
    void setMaximum(int maximum);
    void setRange(int minimum, int maximum);
    void setSingleStep(int val);
    void setSuffix(const QString &suffix);
    void setShowSuffixOnEdit(bool show);
    void setShowSuffixOnDisplay(bool show);

    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    virtual QString displayText(const QVariant &value, const QLocale &locale) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    int minimum = 0;
    int maximum = 100;
    int singleStep = 1;
    QString suffix;
    bool showSuffixOnEdit = true;
    bool showSuffixOnDisplay = true;
    QSize _sizeHint;

    void fillSizeHint();
};



class DoubleSpinBoxEditDelegate: public QStyledItemDelegate
{
public:
    DoubleSpinBoxEditDelegate(QObject *parent = nullptr);

    void setMinimum(double minimum);
    void setMaximum(double maximum);
    void setRange(double minimum, double maximum);
    void setSingleStep(double val);
    void setDecimals(int prec);
    void setSuffix(const QString &suffix);
    void setShowSuffixOnEdit(bool show);
    void setShowSuffixOnDisplay(bool show);

    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    virtual QString displayText(const QVariant &value, const QLocale &locale) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    double minimum = 0;
    double maximum = 100;
    double singleStep = 1;
    double prec = 0;
    QString suffix;
    bool showSuffixOnEdit = true;
    bool showSuffixOnDisplay = true;
    QSize _sizeHint;

    void fillSizeHint();
};



class DateEditDelegate: public QStyledItemDelegate
{
public:
    DateEditDelegate(QObject *parent = nullptr);

    void setCalendarPopup(bool enable);

    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    virtual QString displayText(const QVariant &value, const QLocale &locale) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    bool calendarPopup = false;
    QSize _sizeHint;

    void fillSizeHint();
};



class TimeEditDelegate: public QStyledItemDelegate
{
public:
    TimeEditDelegate(QObject *parent = nullptr);

    void setFormat(const QString &format);
    void setSuffix(const QString &suffix);
    void setShowSuffixOnEdit(bool show);
    void setShowSuffixOnDisplay(bool show);
    void setTimeRange(const QTime &min, const QTime &max);

    virtual QWidget* createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    virtual void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
    virtual QString displayText(const QVariant &value, const QLocale &locale) const override;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    QString format;
    QString suffix;
    bool showSuffixOnEdit = true;
    bool showSuffixOnDisplay = true;
    QTime min;
    QTime max;
    QSize _sizeHint;

    void fillSizeHint();
};

#endif
