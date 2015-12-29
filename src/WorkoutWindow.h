/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_WorkoutWindow_h
#define _GC_WorkoutWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QToolBar> // for Q_PROPERTY
#include <QObject> // for Q_PROPERTY
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>


#include "Context.h"
#include "RideFile.h" // for data series types

#include "Settings.h"
#include "Units.h"
#include "Colors.h"

class WorkoutWidget;
class WWPowerScale;
class WWLine;

class WorkoutWindow : public GcWindow
{
    Q_OBJECT

    public:

        WorkoutWindow(Context *context);

   public slots:

        // toolbar functions
        void saveFile();
        void undo();
        void redo();
        void drawMode();
        void selectMode();

        // trap signals
        void configChanged(qint32);

    private:

        Context *context;

        QToolBar *toolbar;
        QAction *saveAct, *undoAct, *redoAct,
                *drawAct, *selectAct;

        WorkoutWidget *workout; // will become editor.
        WWPowerScale *powerscale;
        WWLine *line;
        bool active;
};

#endif // _GC_WorkoutWindow_h
