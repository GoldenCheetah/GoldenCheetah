/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_WorkoutPlotWindow_h
#define _GC_WorkoutPlotWindow_h 1
#include "GoldenCheetah.h"

#include <QtGui>
#include <QObject> // for Q_PROPERTY


#include "Context.h"
#include "RideFile.h" // for data series types
#include "ErgFilePlot.h"
#include "RealtimeData.h" // for realtimedata structure

#include "Settings.h"
#include "Colors.h"

#include <QLabel>
#include <QComboBox>

class WorkoutPlotWindow : public GcChartWindow
{
    Q_OBJECT

    Q_PROPERTY(int showNotifications READ showNotifications WRITE setShowNotifications USER true)
    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth USER true)
    Q_PROPERTY(int showCurves READ showCurves WRITE setShowCurves USER true)
    Q_PROPERTY(int showColorZones READ showColorZones WRITE setShowColorZones USER true)
    Q_PROPERTY(int colorZonesTransparency READ colorZonesTransparency WRITE setColorZonesTransparency USER true)
    Q_PROPERTY(int showTooltip READ showTooltip WRITE setShowTooltip USER true)

    public:

        WorkoutPlotWindow(Context *context);

    public slots:

        // trap signals
        void setNow(long now);
        void ergFileSelected(ErgFile *);
        void configChanged(qint32);

        bool showNotifications() const;
        void setShowNotifications(bool show);
        double lineWidth() const;
        void setLineWidth(double width);
        int showCurves() const;
        void setShowCurves(int curves);
        int showColorZones() const;
        void setShowColorZones(int index);
        int colorZonesTransparency() const;
        void setColorZonesTransparency(int transparency);
        int showTooltip() const;
        void setShowTooltip(int index);

    private:
        QString title;

        Context *context;
        ErgFilePlot *ergPlot;

        QLabel *ctrlsCommonLabel;
        QCheckBox *ctrlsShowNotification;
        QLabel *ctrlsLineWidthLabel;
        QSlider *ctrlsLineWidth;
        QLabel *ctrlsShowCurveLabel;
        QCheckBox *ctrlsShowWbalCurvePredict;
        QCheckBox *ctrlsShowWbalCurve;
        QCheckBox *ctrlsShowWattsCurve;
        QCheckBox *ctrlsShowHrCurve;
        QCheckBox *ctrlsShowCadCurve;
        QCheckBox *ctrlsShowSpeedCurve;
        QLabel *ctrlsErgmodeLabel;
        QLabel *ctrlsSituationLabel;
        QComboBox *ctrlsSituation;
        QLabel *ctrlsTransparencyLabel;
        QSlider *ctrlsTransparencySlider;
        QLabel *ctrlsShowTooltipLabel;
        QComboBox *ctrlsShowTooltip;
};

#endif // _GC_WorkoutPlotWindow_h
