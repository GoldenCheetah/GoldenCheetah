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

#include <assert.h>
#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include "LogTimeScaleDraw.h"
#include "LogTimeScaleEngine.h"
#include "RideFile.h"

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

struct cpi_file_info {
    QString file, inname, outname;
};

static void
cpi_files_to_update(const QDir &dir, QList<cpi_file_info> &result)
{
    QRegExp re("^([0-9][0-9][0-9][0-9])_([0-9][0-9])_([0-9][0-9])"
                "_([0-9][0-9])_([0-9][0-9])_([0-9][0-9])\\.(raw|srm|csv|tcx)$");
    QStringList filenames = RideFileFactory::instance().listRideFiles(dir);
    QListIterator<QString> i(filenames);
    while (i.hasNext()) {
        const QString &filename = i.next();
        if (re.exactMatch(filename)) {
            QString inname = dir.absoluteFilePath(filename);
            QString outname = dir.absoluteFilePath(
                QFileInfo(filename).completeBaseName() + ".cpi");
            QFileInfo ifi(inname), ofi(outname);
            if (!ofi.exists() || (ofi.lastModified() < ifi.lastModified())) {
                cpi_file_info info;
                info.file = filename;
                info.inname = inname;
                info.outname = outname;
                result.append(info);
            }
        }
    }
}

struct cpint_point 
{
    double secs;
    int watts;
    cpint_point(double s, int w) : secs(s), watts(w) {}
};

struct cpint_data {
    QStringList errors;
    QList<cpint_point> points;
    int rec_int_ms;
    cpint_data() : rec_int_ms(0) {}
};

static void
update_cpi_file(const cpi_file_info *info, QProgressDialog *progress,
                double &progress_sum, double progress_max)
{
    QFile file(info->inname);
    QStringList errors;
    RideFile *rideFile = 
        RideFileFactory::instance().openRideFile(file, errors);
    assert(rideFile);
    cpint_data data;
    data.rec_int_ms = (int) round(rideFile->recIntSecs() * 1000.0);
    QListIterator<RideFilePoint*> i(rideFile->dataPoints());
    while (i.hasNext()) {
        const RideFilePoint *p = i.next();
        double secs = round(p->secs * 1000.0) / 1000;
        data.points.append(cpint_point(secs, (int) round(p->watts)));
    }
    delete rideFile;

    FILE *out = fopen(info->outname.toAscii().constData(), "w");
    assert(out);

    int total_secs = (int) ceil(data.points.back().secs);
    if (total_secs > 7*24*60*60) {
        fclose(out);
        return;
    }
    double *bests = (double*) calloc(total_secs + 1, sizeof(double));

    bool canceled = false;
    int progress_count = 0;
    for (int i = 0; i < data.points.size() - 1; ++i) {
        cpint_point *p = &data.points[i];
        double sum = 0.0;
        double prev_secs = p->secs;
        for (int j = i + 1; j < data.points.size(); ++j) {
            cpint_point *q = &data.points[j];
            if (++progress_count % 1000 == 0) {
                double x = (progress_count + progress_sum)
                    / progress_max * 1000.0;
                // Use min() just in case math is wrong...
                int n = qMin((int) round(x), 1000);
                progress->setValue(n);
                QCoreApplication::processEvents();
                if (progress->wasCanceled()) {
                    canceled = true;
                    goto done;
                }
            }
            sum += data.rec_int_ms / 1000.0 * q->watts;
            double dur_secs = q->secs - p->secs;
            double avg = sum / dur_secs;
            int dur_secs_top = (int) floor(dur_secs);
            int dur_secs_bot = 
                qMax((int) floor(dur_secs - data.rec_int_ms / 1000.0), 0);
            for (int k = dur_secs_top; k > dur_secs_bot; --k) {
                if (bests[k] < avg)
                    bests[k] = avg;
            }
            prev_secs = q->secs;
        } 
    }

    for (int i = 1; i <= total_secs; ++i) {
        if (bests[i] != 0)
            fprintf(out, "%6.3f %3.0f\n", i / 60.0, round(bests[i]));
    }

done:
    fclose(out);
    if (canceled)
        unlink(info->outname.toAscii().constData());
    progress_sum += progress_count;
}

static int 
read_one(const char *inname, QVector<double> &bests)
{
    FILE *in = fopen(inname, "r");
    if (!in)
        return -1;
    int lineno = 1;
    char line[40];
    while (fgets(line, sizeof(line), in) != NULL) {
        double mins;
        int watts;
        if (sscanf(line, "%lf %d\n", &mins, &watts) != 2) {
            fprintf(stderr, "Bad match on line %d: %s", lineno, line);
            exit(1);
        }
        int secs = (int) round(mins * 60.0);
        if (secs >= bests.size())
            bests.resize(secs + 1);
        if (bests[secs] < watts)
            bests[secs] = watts;
        ++lineno;
    }
    fclose(in);
    return 0;
}

static int
read_cpi_file(const QDir &dir, const QFileInfo &raw, QVector<double> &bests)
{
    QString inname = dir.absoluteFilePath(raw.completeBaseName() + ".cpi");
    return read_one(inname.toAscii().constData(), bests);
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
        double progress_max = 0.0;
        if (!to_update.empty()) {
            QListIterator<cpi_file_info> i(to_update);
            while (i.hasNext()) {
                const cpi_file_info &info = i.next();
                QFile file(info.inname);
                QStringList errors;
                RideFile *rideFile = 
                    RideFileFactory::instance().openRideFile(file, errors);
                assert(rideFile);
                double x = rideFile->dataPoints().size();
                progress_max += x * (x - 1.0) / 2.0;
                delete rideFile;
            }
        }
        progress = new QProgressDialog(
            QString(tr("Computing critical power intervals.\n"
                       "This may take a while.\n")),
            tr("Abort"), 0, 1000, this);
        double progress_sum = 0.0;
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
                update_cpi_file(&info, progress, progress_sum, progress_max);
                QCoreApplication::processEvents();
                if (progress->wasCanceled()) {
                    aborted = true;
                    break;
                }
            }
        }
        QVector<double> bests;
        if (!aborted) {
            QString existing = progress->labelText();
            existing.chop(progress->labelText().size() - endingOffset);
            QStringList filters;
            filters << "*.cpi";
            QStringList list = dir.entryList(filters, QDir::Files, QDir::Name);
            progress->setLabelText(
                existing + tr("Aggregating over all files."));
            progress->setRange(0, list.size());
            progress->setValue(0);
            progress->show();
            QListIterator<QString> i(list);
            while (i.hasNext()) {
                const QString &filename = i.next();
                QString path = dir.absoluteFilePath(filename);
                read_one(path.toAscii().constData(), bests);
                progress->setValue(progress->value() + 1);
                QCoreApplication::processEvents();
                if (progress->wasCanceled()) {
                    aborted = true;
                    break;
                }
            }
        }
        if (!aborted) {
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

