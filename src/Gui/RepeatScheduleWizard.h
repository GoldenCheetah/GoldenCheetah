/*
 * Copyright (c) 2025 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _GC_RepeatScheduleWizard_h
#define _GC_RepeatScheduleWizard_h 1

#include "GoldenCheetah.h"

#include <QtGui>
#include <QWizard>
#include <QWizardPage>
#include <QLabel>
#include <QTreeWidget>


struct SourceRide {
    RideItem *rideItem = nullptr;
    QDate sourceDate;
    QDate targetDate;
    bool selected = false;
    int conflictGroup = -1;
    bool targetBlocked = false;
};


class TargetRangeBar : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QColor highlightColor READ highlightColor WRITE setHighlightColor)

public:
    explicit TargetRangeBar(QString errorMsg, QWidget *parent = nullptr);

    void setResult(const QDate &start, const QDate &end, int activityCount, int deletedCount);
    void setFlashEnabled(bool enabled);

private:
    enum class State {
        Neutral,
        Warning,
        Error
    };

    QLabel *iconLabel;
    QLabel *textLabel;
    QColor baseColor;
    QColor borderColor;
    QColor hlColor;
    State currentState;
    const QString errorMsg;
    bool flashEnabled = true;

    void applyStateStyle(State state);
    QString formatDuration(const QDate &start, const QDate &end) const;
    QColor highlightColor() const;
    void setHighlightColor(const QColor& color);
    void flash();
};


class IndicatorDelegate : public QStyledItemDelegate
{
    public:
        enum Roles {
            IndicatorTypeRole = Qt::UserRole + 1, // [IndicatorType] Whether this item has an indicator
            IndicatorStateRole                    // [bool] Whether this items indicator is checked
        };

        enum IndicatorType {
            NoIndicator = 0,
            RadioIndicator = 1,
            CheckIndicator = 2
        };

        explicit IndicatorDelegate(QObject *parent = nullptr);

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;
        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};


class RepeatScheduleWizard : public QWizard
{
    Q_OBJECT

    public:
        enum {
            Finalize = -1,
            PageSetup,
            PageActivities,
            PageSummary
        };

        RepeatScheduleWizard(Context *context, const QDate &when, QWidget *parent = nullptr);

        QList<SourceRide> sourceRides;

        QDate getTargetRangeStart() const;
        QDate getTargetRangeEnd() const;
        int getPlannedInTargetRange() const;
        const QList<RideItem*> &getDeletionList() const;

        void updateTargetRange();
        void updateTargetRange(QDate sourceStart, QDate sourceEnd, bool keepGap, bool preferOriginal);

    signals:
        void targetRangeChanged();

    protected:
        virtual void done(int result) override;

    private:
        Context *context;
        QDate sourceRangeStart;
        QDate sourceRangeEnd;
        QDate targetRangeStart;
        QDate targetRangeEnd;
        int frontGap = 0;
        QList<RideItem*> deletionList;
        bool keepGap = false;
        bool preferOriginal = false;

        QDate getDate(RideItem const * const rideItem, bool preferOriginal) const;
};


class RepeatSchedulePageSetup : public QWizardPage
{
    Q_OBJECT

    public:
        RepeatSchedulePageSetup(Context *context, const QDate &when, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool isComplete() const override;

    private:
        Context *context;
        QDateEdit *startDate;
        QDateEdit *endDate;
        QCheckBox *keepGapCheck;
        QRadioButton *originalRadio;
        QRadioButton *currentRadio;
        TargetRangeBar *targetRangeBar;

    private slots:
        void refresh();
};


class RepeatSchedulePageActivities : public QWizardPage
{
    Q_OBJECT

    public:
        RepeatSchedulePageActivities(Context *context, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool isComplete() const override;

    private:
        Context *context;
        QTreeWidget *activityTree;
        TargetRangeBar *targetRangeBar;
        int numSelected = 0;
        QMetaObject::Connection dataChangedConnection;
};


class RepeatSchedulePageSummary : public QWizardPage
{
    Q_OBJECT

    public:
        RepeatSchedulePageSummary(Context *context, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;

    private:
        Context *context;

        QLabel *scheduleLabel;
        QTreeWidget *scheduleTree;
        QLabel *deletionLabel;
        QTreeWidget *deletionTree;
        TargetRangeBar *targetRangeBar;
};

#endif
