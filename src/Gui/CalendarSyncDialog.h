/*
 * Copyright (c) 2026 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _GC_CalendarSyncDialog_h
#define _GC_CalendarSyncDialog_h 1

#include <QtGui>
#include <QProgressBar>
#include <QLabel>
#include <QDialogButtonBox>

#include "Context.h"
#include "CalendarSync.h"


class CalendarSyncDialog : public QDialog
{
    Q_OBJECT

    public:
        CalendarSyncDialog(Context *context, const CalendarSync::SyncObjects &syncObjects, QString cloudServiceName, QWidget *parent = nullptr);

    private:
        enum class State {
            Configuration,
            Running,
            Finished
        };

        Context *context;
        CalendarSync::SyncObjects syncObjects;
        QString cloudServiceName;
        CalendarSync::SyncSelections syncSelections;
        State state = State::Configuration;

        QStackedWidget *stackedWidget;

        // Config
        QComboBox *titleCombo;
        QComboBox *descriptionCombo;

        // Run
        QLabel *progressLabel;
        QProgressBar *syncBar;
        QProgressBar *cleanupBar;

        // Finish
        QLabel *resultLabel;
        QTreeWidget *overview;

        QDialogButtonBox *buttons;

        bool canceled = false;

        void setState(State state);
        QWidget *strategyWidget(CalendarSync::SyncSelections::SyncMode *mode, CalDAV::EntryType type, int available);
        QWidget *wrapStrategyCombo(QComboBox *combo, int count) const;
        void addResult(const QString &category, const CalendarSync::SyncResult &result, bool bold = false);

    private slots:
        void buttonClicked(QAbstractButton *button);
        void startSync();
        void syncTypeChange(CalDAV::EntryType type);
        void syncProgress(int current, int count);
        void syncFinished(CalendarSync::SyncResults results, bool canceled);
        void cleanupProgress(int current, int count);
        void cleanupStateChange(bool prepare);
        void cleanupFinished(CalendarSync::CleanupResults results, bool canceled);
};

#endif
