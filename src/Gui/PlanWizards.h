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

#ifndef _GC_PlanWizards_h
#define _GC_PlanWizards_h 1

#include "GoldenCheetah.h"

#include <QtGui>
#include <QWizard>
#include <QWizardPage>
#include <QLabel>
#include <QTreeWidget>
#include <QCheckBox>
#include <QRadioButton>
#include <QStyledItemDelegate>

#include "PlanBundle.h"
#include "Season.h"
#include "StyledItemDelegates.h"


class TargetRangeBar : public QFrame
{
    Q_OBJECT
    Q_PROPERTY(QColor highlightColor READ highlightColor WRITE setHighlightColor)

public:
    explicit TargetRangeBar(QString errorMsg, QWidget *parent = nullptr);

    void setResult(const QDate &start, const QDate &end, int activityCount, int deletedCount);
    void setFlashEnabled(bool enabled);
    void setCopyMsg(QString msg);

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
    QString copyMsg;
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


struct SourceRide {
    RideItem *rideItem = nullptr;
    QDate sourceDate;
    QDate targetDate;
    bool selected = false;
    int conflictGroup = -1;
    bool targetBlocked = false;
};


////////////////////////////////////////////////////////////////////////////////
// Repeat Wizard

class RepeatPlanWizard : public QWizard
{
    Q_OBJECT

    public:
        enum {
            Finalize = -1,
            PageSetup,
            PageActivities,
            PageSummary
        };

        RepeatPlanWizard(Context *context, const QDate &when, QWidget *parent = nullptr);

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
};


class RepeatPlanPageSetup : public QWizardPage
{
    Q_OBJECT

    public:
        RepeatPlanPageSetup(Context *context, const QDate &when, QWidget *parent = nullptr);

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


class RepeatPlanPageActivities : public QWizardPage
{
    Q_OBJECT

    public:
        RepeatPlanPageActivities(Context *context, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool isComplete() const override;
        void cleanupPage() override;

    private:
        Context *context;
        QTreeWidget *activityTree;
        TargetRangeBar *targetRangeBar;
        int numSelected = 0;
        QMetaObject::Connection dataChangedConnection;
};


class RepeatPlanPageSummary : public QWizardPage
{
    Q_OBJECT

    public:
        RepeatPlanPageSummary(Context *context, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;

    private:
        Context *context;

        QLabel *planLabel;
        QTreeWidget *planTree;
        QLabel *deletionLabel;
        QTreeWidget *deletionTree;
        TargetRangeBar *targetRangeBar;
};


////////////////////////////////////////////////////////////////////////////////
// Export Wizard

class ExportPlanWizard : public QWizard
{
    Q_OBJECT

    public:
        enum {
            Finalize = -1,
            PageSetup,
            PageActivities,
            PageMetadata,
            PageSummary
        };

        ExportPlanWizard(Context *context, Season const * const preselectSeason = nullptr, QWidget *parent = nullptr);

        QList<SourceRide> sourceRides;

        PlanExportDescription &description();
        int getPlannedInTargetRange() const;

        void updateRange();
        void updateRange(QDate sourceStart, QDate sourceEnd, bool preferOriginal, bool force = false);

        QString expandedDescription() const;

    signals:
        void rangeChanged();

    protected:
        virtual void done(int result) override;

    private:
        Context *context;
        PlanExportDescription _description;
};


class ExportPlanPageSetup : public QWizardPage
{
    Q_OBJECT

    public:
        ExportPlanPageSetup(Context *context, Season const * const preselectSeason = nullptr, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool isComplete() const override;

    private:
        Context *context;
        QDateEdit *startDate;
        QDateEdit *endDate;
        QRadioButton *originalRadio;
        QRadioButton *currentRadio;
        TargetRangeBar *targetRangeBar;

    private slots:
        void refresh();
};


class ExportPlanPageActivities : public QWizardPage
{
    Q_OBJECT

    public:
        ExportPlanPageActivities(Context *context, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool isComplete() const override;
        void cleanupPage() override;

    private:
        Context *context;
        QTreeWidget *activityTree;
        TargetRangeBar *targetRangeBar;
        int numSelected = 0;
        QMetaObject::Connection dataChangedConnection;
};


class ExportPlanPageMetadata : public QWizardPage
{
    Q_OBJECT

    public:
        ExportPlanPageMetadata(Context *context, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool validatePage() override;
        bool isComplete() const override;
        void cleanupPage() override;

    private:
        Context *context;
        QLineEdit *authorEdit;
        QLineEdit *nameEdit;
        QLineEdit *copyrightEdit;
        QTextEdit *descriptionEdit;
        TargetRangeBar *targetRangeBar;
};


class ExportPlanPageSummary : public QWizardPage
{
    Q_OBJECT

    public:
        ExportPlanPageSummary(Context *context, QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool isComplete() const override;

    private:
        Context *context;

        QLabel *authorKey;
        QLabel *authorValue;
        QLabel *nameKey;
        QLabel *nameValue;
        QLabel *descriptionValue;
        QLabel *rangeValue;
        QLabel *durationValue;
        QLabel *exportCountValue;
        QLabel *copyrightKey;
        QLabel *copyrightValue;
        QTreeWidget *planTree;
        DirectoryPathWidget *outputPathWidget;

        QString sanitizeFilename(QString input) const;
};


////////////////////////////////////////////////////////////////////////////////
// Import Wizard

class ImportPlanWizard : public QWizard
{
    Q_OBJECT

    public:
        enum {
            Finalize = -1,
            PageSetup,
            PageActivities,
            PageSummary,
            PageResult
        };

        ImportPlanWizard(Context *context, QDate targetDay, QWidget *parent = nullptr);

        PlanBundleReader planReader;
};


class ImportPlanPageSetup : public QWizardPage
{
    Q_OBJECT

    public:
        ImportPlanPageSetup(QDate targetDay, QWidget *parent = nullptr);

        int nextId() const override;
        bool isComplete() const override;

    private:
        QStackedWidget *overviewStack;
        QCheckBox *gapDayCheck;
        QLabel *overviewEmpty;
        QLabel *overviewError;
        QFrame *statusFrame;
        QLabel *statusLabel;
        QLabel *authorKey;
        QLabel *authorValue;
        QLabel *nameKey;
        QLabel *nameValue;
        QLabel *descriptionValue;
        QLabel *rangeValue;
        QLabel *durationValue;
        QLabel *countActivitiesValue;
        QLabel *copyrightKey;
        QLabel *copyrightValue;
        TargetRangeBar *targetRangeBar;

    private slots:
        void bundlePathChanged(QString path);
        void updateRanges();
};


class ImportPlanPageActivities : public QWizardPage
{
    Q_OBJECT

    public:
        ImportPlanPageActivities(QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool isComplete() const override;
        void cleanupPage() override;

    private:
        QTreeWidget *activityTree;
        TargetRangeBar *targetRangeBar;
        QMetaObject::Connection dataChangedConnection;
};


class ImportPlanPageSummary : public QWizardPage
{
    Q_OBJECT

    public:
        ImportPlanPageSummary(QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;
        bool validatePage() override;

    private:
        QLabel *planLabel;
        QTreeWidget *planTree;
        QLabel *deletionLabel;
        QTreeWidget *deletionTree;
        TargetRangeBar *targetRangeBar;
};


class ImportPlanPageResult : public QWizardPage
{
    Q_OBJECT

    public:
        ImportPlanPageResult(QWidget *parent = nullptr);

        int nextId() const override;
        void initializePage() override;

    private:
        QLabel *label;
};

#endif
