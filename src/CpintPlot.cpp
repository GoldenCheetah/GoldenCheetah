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

#include "Zones.h"
#include "Colors.h"
#include "CpintPlot.h"
#include <assert.h>
#include <unistd.h>
#include <QDebug>
#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_engine.h>
#include "RideItem.h"
#include "LogTimeScaleDraw.h"
#include "LogTimeScaleEngine.h"
#include "RideFile.h"
#include "Season.h"
#include <boost/scoped_ptr.hpp>
#include <algorithm> // for std::lower_bound

#define USE_T0_IN_CP_MODEL 0 // added djconnel 08Apr2009: allow 3-parameter CP model

CpintPlot::CpintPlot(QString p, const Zones *zones) :
    needToScanRides(true),
    path(p),
    thisCurve(NULL),
    CPCurve(NULL),
    zones(zones),
    energyMode_(false)
{
    assert(!USE_T0_IN_CP_MODEL); // doesn't work with energyMode=true

    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setAxisTitle(yLeft, tr("Average Power (watts)"));
    setAxisTitle(xBottom, tr("Interval Length"));
    setAxisScaleDraw(xBottom, new LogTimeScaleDraw);
    setAxisScaleEngine(xBottom, new LogTimeScaleEngine);
    setAxisScale(xBottom, 1.0 / 60.0, 60);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    grid->attach(this);

    configChanged(); // apply colors
}

void
CpintPlot::configChanged()
{
    setCanvasBackground(GColor(CPLOTBACKGROUND));
    QPen gridPen(GColor(CPLOTGRID));
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
}

struct cpi_file_info {
    QString file, inname, outname;
};

QString
ride_filename_to_cpi_filename(const QString filename)
{
    return (QFileInfo(filename).completeBaseName() + ".cpi");
}

static void
cpi_files_to_update(const QDir &dir, QList<cpi_file_info> &result)
{
    QStringList filenames = RideFileFactory::instance().listRideFiles(dir);
    foreach (const QString &filename, filenames) {
        if (RideFileFactory::instance().rideFileRegExp().exactMatch(filename)) {
            QString inname = dir.absoluteFilePath(filename);
            QString outname =
                dir.absoluteFilePath(ride_filename_to_cpi_filename(filename));
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
    cpint_point() : secs(0.0), watts(0) {}
    cpint_point(double s, int w) : secs(s), watts(w) {}
};

struct cpint_data {
    QStringList errors;
    QVector<cpint_point> points;
    int rec_int_ms;
    cpint_data() : rec_int_ms(0) {}
};

static void
update_cpi_file(const cpi_file_info *info, QProgressDialog *progress,
                double &progress_sum, double progress_max)
{
    QFile file(info->inname);
    QStringList errors;
    boost::scoped_ptr<RideFile> rideFile(
        RideFileFactory::instance().openRideFile(file, errors));
    if (!rideFile || rideFile->dataPoints().isEmpty())
        return;
    cpint_data data;
    data.rec_int_ms = (int) round(rideFile->recIntSecs() * 1000.0);
    foreach (const RideFilePoint *p, rideFile->dataPoints()) {
        double secs = round(p->secs * 1000.0) / 1000;
        if (secs > 0)
            data.points.append(cpint_point(secs, (int) round(p->watts)));
    }
    if (!data.points.count()) return;

    FILE *out = fopen(info->outname.toAscii().constData(), "w");
    assert(out);

    int total_secs = (int) ceil(data.points.back().secs);

    // don't allow data more than one week
    #define SECONDS_PER_WEEK 7 * 24 * 60 * 60
    if (total_secs > SECONDS_PER_WEEK) {
        fclose(out);
        return;
    }

    QVector <double> ride_bests(total_secs + 1);
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
                if (ride_bests[k] < avg)
                    ride_bests[k] = avg;
            }
            prev_secs = q->secs;
        }
    }

    // avoid decreasing work with increasing duration
    {
        double maxwork = 0.0;
        for (int i = 1; i <= total_secs; ++i) {
            // note index is being used here in lieu of time, as the index
            // is assumed to be proportional to time
            double work = ride_bests[i] * i;
            if (maxwork > work)
                ride_bests[i] = round(maxwork / i);
            else
                maxwork = work;
            if (ride_bests[i] != 0)
                fprintf(out, "%6.3f %3.0f\n", i / 60.0, round(ride_bests[i]));
        }
    }

done:
    fclose(out);
    if (canceled)
        unlink(info->outname.toAscii().constData());
    progress_sum += progress_count;
}

static QDate
cpi_filename_to_date(const QString filename) {
    QRegExp rx("^(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_\\d\\d_\\d\\d_\\d\\d\\.cpi$");
    if (rx.exactMatch(filename)) {
        assert(rx.numCaptures() == 3);
        QDate date(rx.cap(1).toInt(), rx.cap(2).toInt(), rx.cap(3).toInt());
        if (date.isValid())
            return date;
    }
    return QDate(); // nil date
}

static int
read_one(const QDir& dir, const QString &filename, QVector<double> &bests,
         QVector<QDate> *bestDates, QHash<QString,bool> *cpiDataInBests)
{
    QString inname = dir.absoluteFilePath(filename);
    FILE *in = fopen(inname.toAscii().constData(), "r");
    if (!in)
        return -1;
    int lineno = 1;
    char line[40];

    while (fgets(line, sizeof(line), in) != NULL) {
        double mins;
        int watts;
        if (sscanf(line, "%lf %d\n", &mins, &watts) != 2) {
            QMessageBox::warning(
                NULL, "Warning",
                QString("Error reading %1, line %2").arg(inname).arg(line),
                QMessageBox::Ok,
                QMessageBox::NoButton);
            fclose(in);
            return -1;
        }
        int secs = (int) round(mins * 60.0);
        if (secs >= bests.size()) {
            bests.resize(secs + 1);
            if (bestDates)
                bestDates->resize(secs + 1);
        }
        if (bests[secs] < watts){
            bests[secs] = watts;
            if (bestDates)
                (*bestDates)[secs] = cpi_filename_to_date(filename);

            // mark the filename as having contributed to the bests
            // Note this contribution may subsequently be over-written, so
            // for example the first file scanned will always be tagged.
            if (cpiDataInBests)
                (*cpiDataInBests)[inname] = true;
        }
        ++lineno;
    }
    fclose(in);

    return 0;
}

void
CpintPlot::changeSeason(const QDate &start, const QDate &end)
{
    startDate = start;
    endDate = end;
    needToScanRides = true;
    delete CPCurve;
    CPCurve = NULL;
    clear_CP_Curves();
}

void
CpintPlot::setEnergyMode(bool value)
{
    energyMode_ = value;
    if (energyMode_) {
        setAxisTitle(yLeft, tr("Total work (kJ)"));
        setAxisScaleEngine(xBottom, new QwtLinearScaleEngine);
        setAxisScaleDraw(xBottom, new QwtScaleDraw);
        setAxisTitle(xBottom, tr("Interval Length (minutes)"));
    }
    else {
        setAxisTitle(yLeft, tr("Average Power (watts)"));
        setAxisScaleEngine(xBottom, new LogTimeScaleEngine);
        setAxisScaleDraw(xBottom, new LogTimeScaleDraw);
        setAxisTitle(xBottom, tr("Interval Length"));
    }
    delete CPCurve;
    CPCurve = NULL;
    clear_CP_Curves();
}

// extract critical power parameters which match the given curve
// model: maximal power = cp (1 + tau / [t + t0]), where t is the
// duration of the effort, and t, cp and tau are model parameters
// the basic critical power model is t0 = 0, but non-zero has
// been discussed in the literature
// it is assumed duration = index * seconds
void
CpintPlot::deriveCPParameters()
{
    // bounds on anaerobic interval in minutes
    const double t1 = USE_T0_IN_CP_MODEL ? 0.25 : 1;
    const double t2 = 6;

    // bounds on aerobic interval in minutes
    const double t3 = 10;
    const double t4 = 60;

    // bounds of these time valus in the data
    int i1, i2, i3, i4;

    // find the indexes associated with the bounds
    // the first point must be at least the minimum for the anaerobic interval, or quit
    for (i1 = 0; i1 < 60 * t1; i1++)
        if (i1 + 1 >= bests.size())
            return;
    // the second point is the maximum point suitable for anaerobicly dominated efforts.
    for (i2 = i1; i2 + 1 <= 60 * t2; i2++)
        if (i2 + 1 >= bests.size())
            return;
    // the third point is the beginning of the minimum duration for aerobic efforts
    for (i3 = i2; i3 < 60 * t3; i3++)
        if (i3 + 1 >= bests.size())
            return;
    for (i4 = i3; i4 + 1 <= 60 * t4; i4++)
        if (i4 + 1 >= bests.size())
            break;

    // initial estimate of tau
    if (tau == 0)
        tau = 1;

    // initial estimate of cp (if not already available)
    if (cp == 0)
        cp = 300;

    // initial estimate of t0: start small to maximize sensitivity to data
    t0 = 0;

    // lower bound on tau
    const double tau_min = 0.5;

    // convergence delta for tau
    const double tau_delta_max = 1e-4;
    const double t0_delta_max  = 1e-4;

    // previous loop value of tau and t0
    double tau_prev;
    double t0_prev;

    // maximum number of loops
    const int max_loops = 100;

    // loop to convergence
    int iteration = 0;
    do {
        if (iteration ++ > max_loops) {
            QMessageBox::warning(
                NULL, "Warning",
                QString("Maximum number of loops %d exceeded in cp model"
                        "extraction").arg(max_loops),
                QMessageBox::Ok,
                QMessageBox::NoButton);
            break;
        }

        // record the previous version of tau, for convergence
        tau_prev = tau;
        t0_prev  = t0;

        // estimate cp, given tau
        int i;
        cp = 0;
        for (i = i3; i <= i4; i++) {
            double cpn = bests[i] / (1 + tau / (t0 + i / 60.0));
            if (cp < cpn)
                cp = cpn;
        }

        // if cp = 0; no valid data; give up
        if (cp == 0.0)
            return;

        // estimate tau, given cp
        tau = tau_min;
        for (i = i1; i <= i2; i++) {
            double taun = (bests[i] / cp - 1) * (i / 60.0 + t0) - t0;
            if (tau < taun)
                tau = taun;
        }

        // update t0 if we're using that model
        #if USE_T0_IN_CP_MODEL
        t0 = tau / (bests[1] / cp - 1) - 1 / 60.0;
        #endif

    } while ((fabs(tau - tau_prev) > tau_delta_max) ||
             (fabs(t0 - t0_prev) > t0_delta_max)
            );
}

void
CpintPlot::plot_CP_curve(CpintPlot *thisPlot,     // the plot we're currently displaying
                         double cp,
                         double tau,
                         double t0)
{
    if (CPCurve) {
        delete CPCurve;
        CPCurve = NULL;
    }

    // if there's no cp, then there's nothing to do
    if (cp <= 0)
        return;

    // populate curve data with a CP curve
    const int curve_points = 100;
    double tmin = USE_T0_IN_CP_MODEL ? 1.0/60 : tau;
    double tmax = 180.0;
    QVector<double> cp_curve_power(curve_points);
    QVector<double> cp_curve_time(curve_points);
    int i;

    for (i = 0; i < curve_points; i ++) {
        double x = (double) i / (curve_points - 1);
        double t = pow(tmax, x) * pow(tmin, 1-x);
        cp_curve_time[i] = t;
        if (energyMode_)
            cp_curve_power[i] = (cp * t + cp * tau) * 60.0 / 1000.0;
        else
            cp_curve_power[i] = cp * (1 + tau / (t + t0));
    }

    // generate a plot
    QString curve_title;
#if USE_T0_IN_CP_MODEL
    curve_title.sprintf("CP=%.1f W; AWC/CP=%.2f m; t0=%.1f s", cp, tau, 60 * t0);
#else
    curve_title.sprintf("CP=%.0f W; AWC=%.0f kJ", cp, cp * tau * 60.0 / 1000.0);
#endif

    CPCurve = new QwtPlotCurve(curve_title);
    CPCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen pen(GColor(CCP));
    pen.setWidth(2.0);
    pen.setStyle(Qt::DashLine);
    CPCurve->setPen(pen);
    CPCurve->setData(cp_curve_time.data(), cp_curve_power.data(), curve_points);
    CPCurve->attach(thisPlot);
}

void
CpintPlot::clear_CP_Curves()
{
    // unattach any existing shading curves and reset the list
    if (allCurves.size()) {
        foreach (QwtPlotCurve *curve, allCurves)
            delete curve;
        allCurves.clear();
    }

    // now delete any labels
    if (allZoneLabels.size()) {
        foreach (QwtPlotMarker *label, allZoneLabels)
            delete label;
        allZoneLabels.clear();
    }
}

void
CpintPlot::plot_allCurve(CpintPlot *thisPlot,
                         int n_values,
                         const double *power_values)
{
    clear_CP_Curves();

    QVector<double> energyBests(n_values);
    QVector<double> time_values(n_values);
    // generate an array of time values
    for (int t = 0; t < n_values; t++) {
        time_values[t] = (t + 1) / 60.0;
        energyBests[t] = power_values[t] * time_values[t] * 60.0 / 1000.0;
    }

    // generate zones from derived CP value
    if (cp > 0) {
        QList <int> power_zone;
        int n_zones = zones->lowsFromCP(&power_zone, (int) int(cp));
        int high = n_values - 1;
        int zone = 0;
        while (zone < n_zones && high > 0) {
            int low = high - 1;
            int nextZone = zone + 1;
            if (nextZone >= power_zone.size())
                low = 0;
            else {
                while ((low > 0) && (power_values[low] < power_zone[nextZone]))
                    --low;
            }

            QColor color = zoneColor(zone, n_zones);
            QString name = zones->getDefaultZoneName(zone);
            QwtPlotCurve *curve = new QwtPlotCurve(name);
            curve->setRenderHint(QwtPlotItem::RenderAntialiased);
            QPen pen(color);
            pen.setWidth(2.0);
            curve->setPen(pen);
            curve->attach(thisPlot);
            color.setAlpha(64);
            curve->setBrush(color);  // brush fills below the line
            if (energyMode_) {
                curve->setData(time_values.data() + low,
                               energyBests.data() + low, high - low + 1);
            }
            else {
                curve->setData(time_values.data() + low,
                               power_values + low, high - low + 1);
            }
            allCurves.append(curve);

            if (!energyMode_ || energyBests[high] > 100.0) {
                QwtText text(name);
                text.setFont(QFont("Helvetica", 24, QFont::Bold));
                color.setAlpha(128);
                text.setColor(color);
                QwtPlotMarker *label_mark = new QwtPlotMarker();
                // place the text in the geometric mean in time, at a decent power
                double x, y;
                if (energyMode_) {
                    x = (time_values[low] + time_values[high]) / 2;
                    y = (energyBests[low] + energyBests[high]) / 5;
                }
                else {
                    x = sqrt(time_values[low] * time_values[high]);
                    y = (power_values[low] + power_values[high]) / 5;
                }
                label_mark->setValue(x, y);
                label_mark->setLabel(text);
                label_mark->attach(thisPlot);
                allZoneLabels.append(label_mark);
            }

            high = low;
            ++zone;
        }
    }
    // no zones available: just plot the curve without zones
    else {
        QwtPlotCurve *curve = new QwtPlotCurve(tr("maximal power"));
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);
        QPen pen(GColor(CCP));
        pen.setWidth(2.0);
        curve->setPen(pen);
        QColor brush_color = GColor(CCP);
        brush_color.setAlpha(64);
        curve->setBrush(brush_color);   // brush fills below the line
        if (energyMode_)
            curve->setData(time_values.data(), energyBests.data(), n_values);
        else
            curve->setData(time_values.data(), power_values, n_values);
        curve->attach(thisPlot);
        allCurves.append(curve);
    }

    // Energy mode is really only interesting in the range where energy is
    // linear in interval duration--up to about 1 hour.
    double xmax = energyMode_ ? 60.0 : time_values[n_values - 1];
    thisPlot->setAxisScale(thisPlot->xBottom, 1.0 / 60, xmax);

    double ymax;
    if (energyMode_) {
        int i = std::lower_bound(time_values.begin(), time_values.end(), 60.0) - time_values.begin();
        ymax = 10 * ceil(energyBests[i] / 10);
    }
    else {
        ymax = 100 * ceil(power_values[0] / 100);
    }
    thisPlot->setAxisScale(thisPlot->yLeft, 0, ymax);
}

void
CpintPlot::calculate(RideItem *rideItem)
{
    QString fileName = rideItem->fileName;
    QDateTime dateTime = rideItem->dateTime;
    QDir dir(path);
    QFileInfo file(fileName);

    if (needToScanRides) {
        bests.clear();
        bestDates.clear();
        cpiDataInBests.clear();
        bool aborted = false;
        QList<cpi_file_info> to_update;
        cpi_files_to_update(dir, to_update);
        double progress_max = 0.0;
        if (!to_update.empty()) {
            foreach (const cpi_file_info &info, to_update) {
                QFile file(info.inname);
                QStringList errors;
                boost::scoped_ptr<RideFile> rideFile(
                    RideFileFactory::instance().openRideFile(file, errors));
                if (rideFile) {
                    double x = rideFile->dataPoints().size();
                    progress_max += x * (x + 1.0) / 2.0;
                }
            }
        }
        QProgressDialog progress(
            QString(tr("Computing critical power intervals.\n"
                       "This may take a while.\n")),
            tr("Abort"), 0, 1000, this);
        double progress_sum = 0.0;
        int endingOffset = progress.labelText().size();
        if (!to_update.empty()) {
            int count = 1;
            foreach (const cpi_file_info &info, to_update) {
                QString existing = progress.labelText();
                existing.chop(progress.labelText().size() - endingOffset);
                progress.setLabelText(
                    existing + QString(tr("Processing %1...")).arg(info.file));
                progress.setValue(count++);
                update_cpi_file(&info, &progress, progress_sum, progress_max);
                QCoreApplication::processEvents();
                if (progress.wasCanceled()) {
                    aborted = true;
                    break;
                }
            }
        }
        if (!aborted) {
            QString existing = progress.labelText();
            existing.chop(progress.labelText().size() - endingOffset);
            QStringList filters;
            filters << "*.cpi";
            QStringList list = dir.entryList(filters, QDir::Files, QDir::Name);
            list = filterForSeason(list, startDate, endDate);
            progress.setLabelText(
                existing + tr("Aggregating over all files."));
            progress.setRange(0, list.size());
            progress.setValue(0);
            progress.show();
            foreach (const QString &filename, list) {
                QString path = dir.absoluteFilePath(filename);
                read_one(dir, filename, bests, &bestDates, &cpiDataInBests);
                progress.setValue(progress.value() + 1);
                QCoreApplication::processEvents();
                if (progress.wasCanceled()) {
                    aborted = true;
                    break;
                }
            }
        }
        if (!aborted && bests.size()) {
            // check that total work doesn't decrease with time
            double maxwork = 0.0;

            for (int i = 0; i < bests.size(); ++i) {
                // note index is being used here in lieu of time, as the index
                // is assumed to be proportional to time
                double work = bests[i] * i;
                if ((i > 0) && (maxwork > work)) {
                    bests[i] = round(maxwork / i);
                    bestDates[i] = bestDates[i - 1];
                }
                else
                    maxwork = work;
            }

            // derive CP model
            if (bests.size() > 1) {
                // cp model parameters
                cp  = 0;
                tau = 0;
                t0  = 0;

                // calculate CP model from all-time best data
                deriveCPParameters();
            }
            needToScanRides = false;
        }
    }

    if (!needToScanRides) {
        if (!CPCurve)
            plot_CP_curve(this, cp, tau, t0);
        else {
            // make sure color reflects latest config
            QPen pen(GColor(CCP));
            pen.setWidth(2.0);
            pen.setStyle(Qt::DashLine);
            CPCurve->setPen(pen);
        }
        if (allCurves.empty()) {
            int maxNonZero = 0;
            for (int i = 0; i < bests.size(); ++i) {
                if (bests[i] > 0)
                    maxNonZero = i;
            }
            plot_allCurve(this, maxNonZero - 1, bests.constData() + 1);
        }

        if (thisCurve) {
            delete thisCurve;
            thisCurve = NULL;
        }
        QVector<double> bests;
        QString filename = file.completeBaseName() + ".cpi";
        if ((read_one(dir, filename, bests, NULL, NULL) == 0) && bests.size()) {
            QVector<double> energyArray(bests.size());
            QVector<double> timeArray(bests.size());
            int maxNonZero = 0;
            for (int i = 0; i < bests.size(); ++i) {
                timeArray[i] = i / 60.0;
                energyArray[i] = timeArray[i] * bests[i] * 60.0 / 1000.0;
                if (bests[i] > 0) maxNonZero = i;
            }
            if (maxNonZero > 1) {
                thisCurve = new QwtPlotCurve(
                    dateTime.toString(tr("ddd MMM d, yyyy h:mm AP")));
                thisCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
                thisCurve->setPen(QPen(Qt::black));
                thisCurve->attach(this);
                if (energyMode_) {
                    thisCurve->setData(timeArray.data() + 1,
                                       energyArray.constData() + 1,
                                       maxNonZero - 1);
                }
                else {
                    thisCurve->setData(timeArray.data() + 1,
                                       bests.constData() + 1,
                                       maxNonZero - 1);
                }
            }
        }
    }

    replot();
}

// delete a CPI file
bool
CpintPlot::deleteCpiFile(QString filename)
{
    // first, get ride of the file
    if (! QFile::remove(filename))
        return false;

    // now check to see if this file contributed to the bests
    // in the current implementation a false means it does
    // not contribute, but a true only means it at one time
    // contributed (may not in the end).
    if (cpiDataInBests.contains(filename)) {
        if (cpiDataInBests[filename])
            needToScanRides = true;
        cpiDataInBests.remove(filename);
    }

    return true;
}

void
CpintPlot::showGrid(int state)
{
    assert(state != Qt::PartiallyChecked);
    grid->setVisible(state == Qt::Checked);
    replot();
}

QStringList
CpintPlot::filterForSeason(QStringList cpints, QDate startDate, QDate endDate)
{
    //Check to see if no date was assigned.
    QDate nilDate;
    if(startDate == nilDate)
        return cpints;
    QStringList returnList;
    foreach (const QString &cpi, cpints) {
        QDate cpiDate = cpi_filename_to_date(cpi);
        if(cpiDate > startDate && cpiDate < endDate)
            returnList << cpi;

    }
    return returnList;
}

