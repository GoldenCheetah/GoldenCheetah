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

#ifndef ACTIONBUTTONBOX_H
#define ACTIONBUTTONBOX_H

#include <QPushButton>
#ifndef Q_OS_MAC
#include <QToolButton>
#endif
#include <QHBoxLayout>
#include <QAbstractItemView>


class ActionButtonBox : public QWidget {
    Q_OBJECT

    public:
        enum StandardButtonGroup {
            NoGroup = 0x0,
            UpDownGroup = 0x1,
            AddDeleteGroup = 0x2,
            EditGroup = 0x4
        };
        Q_DECLARE_FLAGS(StandardButtonGroups, StandardButtonGroup)

        enum StandardButton {
            Up = 0,
            Down = 1,
            Add = 2,
            Delete = 3,
            Edit = 4
        };

        enum ActionPosition {
            Left = 0,
            Right = 1
        };

        ActionButtonBox(StandardButtonGroups standardButtonGroups, QWidget *parent = nullptr);

        QAbstractButton *which(StandardButton which) const;

        void setButtonHidden(StandardButton standardButton, bool hidden);
        void setButtonEnabled(StandardButton standardButton, bool enabled);

        QPushButton *addButton(const QString &text, ActionPosition pos = ActionPosition::Left);
        void addWidget(QWidget *widget);
        void addStretch(ActionPosition pos = ActionPosition::Left);

        void defaultConnect(QAbstractItemView *view);
        void defaultConnect(StandardButtonGroup group, QAbstractItemView *view);

        void setMinViewItems(int minItems);
        void setMaxViewItems(int maxItems);

    signals:
        void upRequested();
        void downRequested();
        void addRequested();
        void deleteRequested();
        void editRequested();

    private:
        QHBoxLayout *layout;
#ifdef Q_OS_MAC
        QPushButton *up = nullptr;
        QPushButton *down = nullptr;
#else
        QToolButton *up = nullptr;
        QToolButton *down = nullptr;
#endif
        QPushButton *add = nullptr;
        QPushButton *del = nullptr;
        QPushButton *edit = nullptr;

        int minViewItems = 0;
        int maxViewItems = std::numeric_limits<int>::max();

        int leftOffset = 0;
        int rightOffset = 0;

        void updateButtonState(StandardButtonGroup group, QAbstractItemView *view);
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ActionButtonBox::StandardButtonGroups)

#endif
