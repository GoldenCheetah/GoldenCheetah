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

#ifndef _GC_WorkoutFilterBox_h
#define _GC_WorkoutFilterBox_h

#include <QString>
#include <QStringList>
#include <QList>

#include "FilterEditor.h"
#include "Context.h"

class WorkoutFilterBox : public FilterEditor
{
    Q_OBJECT

public:
    WorkoutFilterBox(QWidget *parent=nullptr, Context *context=nullptr);
    virtual ~WorkoutFilterBox();

private slots:
    void workoutFilterChanged(const QString &text);

private:
    Context *context;
    QAction *workoutFilterErrorAction;
};

#endif
