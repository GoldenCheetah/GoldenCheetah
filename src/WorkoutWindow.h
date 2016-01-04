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
class WWWBalScale;
class WWLine;
class WWWBLine;
class WWRect;
class WWBlockCursor;
class WWBlockSelection;
class WWMMPCurve;
class WWSmartGuide;

class WorkoutWindow : public GcWindow
{
    Q_OBJECT

    public:

        WorkoutWindow(Context *context);

        // workout widget updates these
        QLabel *xlabel, *ylabel;
        QLabel *TSSlabel, *IFlabel;

        QAction *saveAct, *undoAct, *redoAct,
                *drawAct, *selectAct,
                *cutAct, *copyAct, *pasteAct;

        bool draw; // draw or select mode?

   public slots:

        // toolbar functions
        void saveFile();
        void drawMode();
        void selectMode();

        // trap signals
        void configChanged(qint32);

    private:

        Context *context;

        QToolBar *toolbar;
        WorkoutWidget *workout; // will become editor.
        WWPowerScale *powerscale;
        WWWBalScale *wbalscale;
        WWLine *line;
        WWWBLine *wbline;
        WWRect *rect;
        WWBlockCursor *bcursor;
        WWBlockSelection *brect;
        WWMMPCurve *mmp;
        WWSmartGuide *guide;
        bool active;
};

#endif // _GC_WorkoutWindow_h
