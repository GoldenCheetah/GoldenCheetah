/* 
 * $Id: CpintPlot.cpp,v 1.2 2006/07/12 02:13:57 srhea Exp $
 *
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

extern "C" {
#include "cpint.h"
}

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
    setAxisScale(xBottom, 0.021, 60);

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
    char *dir = strdup(path.toAscii().constData());
    char *file = strdup(fileName.toAscii().constData());

    if (needToScanRides) {
        fprintf(stderr, "Scanning rides...\n");
        fflush(stderr);
        bool aborted = false;
        struct cpi_file_info *head = cpi_files_to_update(dir);
        int count = 0;
        struct cpi_file_info *tmp = head;
        while (tmp) {
            ++count;
            tmp = tmp->next;
        }
        progress = new QProgressDialog(
            QString(tr("Computing critical power intervals.\n"
                       "This may take a while.\n")),
            tr("Abort"), 0, count + 1, this);
        int endingOffset = progress->labelText().size();
        if (count) {
            tmp = head;
            count = 1;
            while (tmp) {
                QString existing = progress->labelText();
                existing.chop(progress->labelText().size() - endingOffset);
                progress->setLabelText(
                    existing + QString(tr("Processing %1...")).arg(tmp->file));
                progress->setValue(count++);
                update_cpi_file(tmp, cancel_cb, this);
                QCoreApplication::processEvents();
                if (progress->wasCanceled()) {
                    aborted = true;
                    break;
                }
                tmp = tmp->next;
            }
            free_cpi_file_info(head);
        }

        if (!aborted) {
            QString existing = progress->labelText();
            existing.chop(progress->labelText().size() - endingOffset);
            progress->setLabelText(
                existing + tr("Aggregating over all files."));
            progress->show();
            int i;
            double *bests;
            int bestlen;
            combine_cpi_files(dir, &bests, &bestlen);
            double *timeArray = new double[bestlen];
            int maxNonZero = 0;
            for (i = 0; i < bestlen; ++i) {
                timeArray[i] = i * 0.021;
                if (bests[i] > 0) maxNonZero = i;
            }
            if (maxNonZero > 1) {
                allCurve->setData(timeArray + 1, bests + 1, maxNonZero - 1);
                setAxisScale(xBottom, 0.021, maxNonZero * 0.021);
            }
            delete [] timeArray;
            free(bests);
            needToScanRides = false;
        }

        delete progress; 
        progress = NULL;
    }

    if (!needToScanRides) {
        delete thisCurve;
        int i;
        double *bests;
        int bestlen;
        read_cpi_file(dir, file, &bests, &bestlen);
        double *timeArray = new double[bestlen];
        int maxNonZero = 0;
        for (i = 0; i < bestlen; ++i) {
            timeArray[i] = i * 0.021;
            if (bests[i] > 0) maxNonZero = i;
        }
        if (maxNonZero > 1) {
            thisCurve = new QwtPlotCurve(
                dateTime.toString("ddd MMM d, yyyy h:mm AP"));
            thisCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
            thisCurve->setPen(QPen(Qt::green));
            thisCurve->attach(this);
            thisCurve->setData(timeArray + 1, bests + 1, maxNonZero - 1);
        }
        delete [] timeArray;
        free(bests);
    }
 
    replot();
    free(dir);
    free(file);
}

void
CpintPlot::showGrid(int state) 
{
    assert(state != Qt::PartiallyChecked);
    grid->setVisible(state == Qt::Checked);
    replot();
}

