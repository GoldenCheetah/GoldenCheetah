/* 
 * Copyright (c) 2006 Sean C. Rhea (srhea@srhea.net)
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

#include "CpintPlot.h"
#include "cpint.h"

#include <assert.h>
#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include "LogTimeScaleDraw.h"
#include "LogTimeScaleEngine.h"

CpintPlot::CpintPlot(QString p) : 
    progress(NULL), needToScanRides(true), path(p), allCurve(NULL), 
    thisCurve(NULL), grid(NULL)
{
    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(Qt::white);
    setAxisTitle(yLeft, "Average Power (watts)");
    setAxisTitle(xBottom, "Interval Length");
    setAxisScaleDraw(xBottom, new LogTimeScaleDraw);
    setAxisScaleEngine(xBottom, new LogTimeScaleEngine);
    setAxisScale(xBottom, 1.0 / 60.0, 60);

    allCurve = new QwtPlotCurve("All Rides");
    allCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    allCurve->setPen(QPen(Qt::red));
    allCurve->attach(this);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    QPen gridPen;
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
    grid->attach(this);
}

static int
cancel_cb(void *user_data) 
{
    CpintPlot *self = (CpintPlot*) user_data;
    QCoreApplication::processEvents();
    return self->progress->wasCanceled();
}

void
CpintPlot::calculate(QString fileName, QDateTime dateTime) 
{
    QDir dir(path);
    QFileInfo file(fileName);

    if (needToScanRides) {
        fflush(stderr);
        bool aborted = false;
        QList<cpi_file_info> to_update; 
        cpi_files_to_update(dir, to_update);
        progress = new QProgressDialog(
            QString(tr("Computing critical power intervals.\n"
                       "This may take a while.\n")),
            tr("Abort"), 0, to_update.size() + 1, this);
        int endingOffset = progress->labelText().size();
        if (!to_update.empty()) {
            QListIterator<cpi_file_info> i(to_update);
            int count = 1;
            while (i.hasNext()) {
                const cpi_file_info &info = i.next();
                QString existing = progress->labelText();
                existing.chop(progress->labelText().size() - endingOffset);
                progress->setLabelText(
                    existing + QString(tr("Processing %1...")).arg(info.file));
                progress->setValue(count++);
                update_cpi_file(&info, cancel_cb, this);
                QCoreApplication::processEvents();
                if (progress->wasCanceled()) {
                    aborted = true;
                    break;
                }
            }
        }
        if (!aborted) {
            QString existing = progress->labelText();
            existing.chop(progress->labelText().size() - endingOffset);
            progress->setLabelText(
                existing + tr("Aggregating over all files."));
            progress->show();
            QVector<double> bests;
            combine_cpi_files(dir, bests);
            double *timeArray = new double[bests.size()];
            int maxNonZero = 0;
            for (int i = 0; i < bests.size(); ++i) {
                timeArray[i] = i / 60.0;
                if (bests[i] > 0) maxNonZero = i;
            }
            if (bests.size() > 1) {
                allCurve->setData(timeArray + 1, bests.constData() + 1,
                                  maxNonZero - 1);
                setAxisScale(xBottom, 1.0 / 60.0, maxNonZero / 60.0);
            }
            delete [] timeArray;
            needToScanRides = false;
        }
        delete progress; 
        progress = NULL;
    }

    if (!needToScanRides) {
        delete thisCurve;
        thisCurve = NULL;
        QVector<double> bests;
        if (read_cpi_file(dir, file, bests) == 0) {
            double *timeArray = new double[bests.size()];
            int maxNonZero = 0;
            for (int i = 0; i < bests.size(); ++i) {
                timeArray[i] = i / 60.0;
                if (bests[i] > 0) maxNonZero = i;
            }
            if (maxNonZero > 1) {
                thisCurve = new QwtPlotCurve(
                    dateTime.toString("ddd MMM d, yyyy h:mm AP"));
                thisCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
                thisCurve->setPen(QPen(Qt::green));
                thisCurve->attach(this);
                thisCurve->setData(timeArray + 1, bests.constData() + 1,
                                   maxNonZero - 1);
            }
            delete [] timeArray;
        }
    }
 
    replot();
}

void
CpintPlot::showGrid(int state) 
{
    assert(state != Qt::PartiallyChecked);
    grid->setVisible(state == Qt::Checked);
    replot();
}

