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
#include <QPlainTextEdit>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QScrollBar>
#include <QMessageBox>
#include <QPointF>

#include "Context.h"
#include "RideFile.h" // for data series types
#include "Library.h"  // workout library
#include "AbstractView.h"  // stylesheet for scroller

#include "Settings.h"
#include "Units.h"
#include "Colors.h"

#include "../qtsolutions/codeeditor/codeeditor.h"

class WorkoutWidget;
class WWPowerScale;
class WWWBalScale;
class WWTTE;
class WWLine;
class WWWBLine;
class WWRect;
class WWBlockCursor;
class WWBlockSelection;
class WWMMPCurve;
class WWSmartGuide;
class WWLap;
class WWNow;
class WWTelemetry;

class WorkoutWindow : public GcChartWindow
{
    Q_OBJECT

    public:

        WorkoutWindow(Context *context);

        // the ergfile we are editing
        ErgFile *ergFile;
        ErgFileFormat format;

        // edit the definition
        QLabel *codeFormat;
        QLabel *coalescedSections;
        QWidget *codeContainer;
        CodeEditor *code;

        // workout widget updates these
        QLabel *xlabel, *ylabel;
        QLabel *TSSlabel, *IFlabel;

        QAction *newAct, *saveAsAct,
                *saveAct, *propertiesAct,
                *undoAct, *redoAct,
                *drawAct, *selectAct,
                *cutAct, *copyAct, *pasteAct,
                *zoomInAct, *zoomOutAct;

        bool draw; // draw or select mode?

   public slots:

        // toolbar functions
        void newErgFile();
        void newMrcFile();
        void saveFile();
        void saveAs();
        void properties();
        void drawMode();
        void selectMode();

        // zooming and scrolling
        void zoomIn();
        void zoomOut();

        // set scrollbar min/max/page
        // via min/max view encoded in QPointF
        void setScroller(QPointF);

        // user moved scrollbar
        void scrollMoved();

        // and erg file was selected
        void ergFileSelected(ErgFile *, ErgFileFormat format = ErgFileFormat::unknown);

        // qwkcode edited!
        void qwkcodeChanged();

        // trap signals
        void configChanged(qint32);

        // start/stop running
        void stop();
        void start();

        // show hide toolbar if too small
        void resizeEvent(QResizeEvent * event);

        // settings changes
        void plotHrChanged(int value);
        void plotPwrChanged(int value);
        void plotCadenceChanged(int value);
        void plotWbalChanged(int value);
        void plotVo2Changed(int value);
        void plotVentilationChanged(int value);
        void plotSpeedChanged(int value);
        void plotHrAvgChanged(int value);
        void plotPwrAvgChanged(int value);
        void plotCadenceAvgChanged(int value);
        void plotVo2AvgChanged(int value);
        void plotVentilationAvgChanged(int value);
        void plotSpeedAvgChanged(int value);

        // get settings
        bool shouldPlotHr() { return plotHr;}
        bool shouldPlotPwr() { return plotPwr;}
        bool shouldPlotCadence() { return plotCadence;}
        bool shouldPlotWbal() { return plotWbal;}
        bool shouldPlotVo2() { return plotVo2;}
        bool shouldPlotVentilation() { return plotVentilation;}
        bool shouldPlotSpeed() { return plotSpeed;}

        int hrPlotAvgLength() { return plotHrAvg; }
        int pwrPlotAvgLength() { return plotPwrAvg; }
        int cadencePlotAvgLength() { return plotCadenceAvg; }
        int vo2PlotAvgLength() { return plotVo2Avg; }
        int ventilationPlotAvgLength() { return plotVentilationAvg; }
        int speedPlotAvgLength() { return plotSpeedAvg; }

    protected:
        bool eventFilter(QObject *obj, QEvent *event);

    private:

        Context *context;

        QToolBar *toolbar;
        WorkoutWidget *workout; // will become editor.
        QScrollBar *scroll;     // for controlling the position
        QCheckBox *plotHrCB, *plotPwrCB, *plotCadenceCB, *plotWbalCB, *plotVo2CB, *plotVentilationCB, *plotSpeedCB;
        QSpinBox *plotHrSB, *plotPwrSB, *plotCadenceSB, *plotVo2SB, *plotVentilationSB, *plotSpeedSB;

        WWPowerScale *powerscale;
        WWWBalScale *wbalscale;
        WWTTE *tte;
        WWLine *line;
        WWWBLine *wbline;
        WWRect *rect;
        WWBlockCursor *bcursor;
        WWBlockSelection *brect;
        WWMMPCurve *mmp;
        WWSmartGuide *guide;
        WWLap *lap;
        WWNow *now;
        WWTelemetry *telemetry;

        bool active;
        bool recording;
        bool plotHr, plotPwr, plotCadence, plotWbal, plotVo2, plotVentilation, plotSpeed;
        int plotHrAvg, plotPwrAvg, plotCadenceAvg, plotVo2Avg, plotVentilationAvg, plotSpeedAvg;
};

#endif // _GC_WorkoutWindow_h
