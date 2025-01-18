/*
 * Copyright (c) 2023 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "WorkoutFilterBox.h"
#include "WorkoutFilter.h"

#include <QString>
#include <QDebug>
#include <QStyle>

WorkoutFilterBox::WorkoutFilterBox(QWidget *parent, Context *context) : FilterEditor(parent), context(context)
{
    QIcon workoutFilterErrorIcon = this->style()->standardIcon(QStyle::SP_MessageBoxCritical);
    workoutFilterErrorAction = this->addAction(workoutFilterErrorIcon, FilterEditor::LeadingPosition);
    workoutFilterErrorAction->setVisible(false);
    this->setClearButtonEnabled(true);
    this->setPlaceholderText(tr("Filter..."));
    this->setFilterCommands(workoutFilterCommands());
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(workoutFilterChanged(const QString&)));
}

WorkoutFilterBox::~WorkoutFilterBox()
{
}

void
WorkoutFilterBox::workoutFilterChanged(const QString &text)
{
    workoutFilterErrorAction->setVisible(false);
    bool ok = true;
    QString msg;
    QString input(text.trimmed());
    while (input.length() > 0 && (input.back().isSpace() || input.back() == ',')) {
        input.chop(1);
    }
    if (context && input.length() > 0) {
	auto filters = parseWorkoutFilter(input, ok, msg);
        context->setWorkoutFilters(filters);
        if (! ok) {
            workoutFilterErrorAction->setVisible(true);
            workoutFilterErrorAction->setToolTip(tr("ERROR: %1").arg(msg));
        }
    } else {
        context->clearWorkoutFilters();
    }
}
