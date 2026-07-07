/*
 * Copyright (c) 2009 Justin F. Knotzke (jknotzke@shampoo.ca)
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

#ifndef SEASONDIALOGS_H_
#define SEASONDIALOGS_H_

#include <QDialog>
#include <QTreeWidget>
#include <QStackedWidget>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QSpinBox>
#include <QDateEdit>
#include <QLineEdit>
#include <QTextEdit>

#include "Context.h"
#include "Season.h"


class EditSeasonDialog : public QDialog
{
    Q_OBJECT

    public:
        enum SeasonRange { absolute = 0, relative = 1, duration = 2, ytd = 3 };

        EditSeasonDialog(Context *, Season *);

    private slots:
        void applyClicked();
        void cancelClicked();
        void nameChanged();

        void updateAllowedCombinations();
        void startTypeChanged(int idx);
        void endTypeChanged(int idx);

        void updateTimeRange();

    private:
        Context *context;
        Season *season;

        QPushButton *applyButton;

        QLineEdit *nameEdit;
        QComboBox *typeCombo;


        QComboBox *startCombo;
        QStackedWidget *startValueStack;
        QDateEdit *startAbsoluteEdit;
        QSpinBox *startRelativeEdit;
        QComboBox *startRelativeCombo;
        QSpinBox *startDurationYearsEdit;
        QSpinBox *startDurationMonthsEdit;
        QSpinBox *startDurationDaysEdit;

        QComboBox *endCombo;
        QStackedWidget *endValueStack;
        QDateEdit *endAbsoluteEdit;
        QSpinBox *endRelativeEdit;
        QComboBox *endRelativeCombo;
        QSpinBox *endDurationYearsEdit;
        QSpinBox *endDurationMonthsEdit;
        QSpinBox *endDurationDaysEdit;

        QDoubleSpinBox *seedEdit;
        QDoubleSpinBox *lowEdit;

        QLabel *statusLabel;
        QLabel *warningLabel;

        void transferUIToSeason(Season &season) const;
        void transferSeasonToUI(const Season &season);
        void setEnabledItem(QComboBox *combo, int data, bool enabled);
        void getSeasonOffset(int offset, int typeIndex, int &years, int &months, int &weeks) const;
};


class EditSeasonEventDialog : public QDialog
{
    Q_OBJECT

    public:
        EditSeasonEventDialog(Context *, SeasonEvent *, Season &);

    public slots:
        void applyClicked();
        void cancelClicked();
        void nameChanged();

    private:
        Context *context;
        SeasonEvent *event;

        QPushButton *applyButton;
        QLineEdit *nameEdit;
        QDateEdit *dateEdit;
        QComboBox *priorityEdit;
        QTextEdit *descriptionEdit;
};


class EditPhaseDialog : public QDialog
{
    Q_OBJECT

    public:
        EditPhaseDialog(Context *, Phase *, Season &);

    public slots:
        void applyClicked();
        void cancelClicked();
        void nameChanged();

    private:
        Context *context;
        Phase *phase;

        QPushButton *applyButton;
        QLineEdit *nameEdit;
        QComboBox *typeEdit;
        QDateEdit *fromEdit, *toEdit;
        QDoubleSpinBox *seedEdit;
        QDoubleSpinBox *lowEdit;
};


class SeasonTreeView : public QTreeWidget
{
    Q_OBJECT

    public:
        SeasonTreeView(Context *);

        // for drag/drop
        QStringList mimeTypes () const override;
        QMimeData *mimeData(const QList<QTreeWidgetItem *> &items) const override;

    signals:
        void itemMoved(QTreeWidgetItem* item, int previous, int actual);

    protected:
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dropEvent(QDropEvent* event) override;
        Context *context;
};


class SeasonEventTreeView : public QTreeWidget
{
    Q_OBJECT

    public:
        SeasonEventTreeView();

    signals:
        void itemMoved(QTreeWidgetItem* item, int previous, int actual);

    protected:
        void dropEvent(QDropEvent* event);
};

#endif /* SEASON_H_ */
