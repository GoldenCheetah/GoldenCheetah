#ifndef STYLEDITEMSDELEGATES_H
#define STYLEDITEMSDELEGATES_H

#include <QStyledItemDelegate>
#include <QTime>


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



class SpinBoxEditDelegate: public QStyledItemDelegate
{
public:
    SpinBoxEditDelegate(QObject *parent = nullptr);

    void setMininum(int minimum);
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
};



class DoubleSpinBoxEditDelegate: public QStyledItemDelegate
{
public:
    DoubleSpinBoxEditDelegate(QObject *parent = nullptr);

    void setMininum(double minimum);
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
};

#endif
