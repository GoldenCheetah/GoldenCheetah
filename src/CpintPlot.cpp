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
#include "CpintPlot.h"
#include <assert.h>
#include <unistd.h>
#include <QDebug>
#include <qwt_data.h>
#include <qwt_legend.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include "RideItem.h"
#include "LogTimeScaleDraw.h"
#include "LogTimeScaleEngine.h"
#include "RideFile.h"
#include "Season.h"

CpintPlot::CpintPlot(QString p) :
    progress(NULL),
    needToScanRides(true),
    path(p),
    thisCurve(NULL),
    CPCurve(NULL),
    grid(NULL),
    zones(NULL)
{
    insertLegend(new QwtLegend(), QwtPlot::BottomLegend);
    setCanvasBackground(Qt::white);
    setAxisTitle(yLeft, "Average Power (watts)");
    setAxisTitle(xBottom, "Interval Length");
    setAxisScaleDraw(xBottom, new LogTimeScaleDraw);
    setAxisScaleEngine(xBottom, new LogTimeScaleEngine);
    setAxisScale(xBottom, 1.0 / 60.0, 60);

    grid = new QwtPlotGrid();
    grid->enableX(false);
    QPen gridPen;
    gridPen.setStyle(Qt::DotLine);
    grid->setPen(gridPen);
    grid->attach(this);

    allCurves=     QList <QwtPlotCurve *>::QList();
    allZoneLabels= QList <QwtPlotMarker *>::QList();
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
    if (! rideFile)
        return;
    cpint_data data;
    data.rec_int_ms = (int) round(rideFile->recIntSecs() * 1000.0);
    foreach (const RideFilePoint *p, rideFile->dataPoints()) {
        double secs = round(p->secs * 1000.0) / 1000;
        if (secs > 0)
            data.points.append(cpint_point(secs, (int) round(p->watts)));
    }
    delete rideFile;

    FILE *out = fopen(info->outname.toAscii().constData(), "w");
    assert(out);

    int total_secs = (int) ceil(data.points.back().secs);

    // don't allow data more than one week
    #define SECONDS_PER_WEEK 7 * 24 * 60 * 60
    if (total_secs > SECONDS_PER_WEEK) {
        fclose(out);
        return;
    }


    QVector <double> ride_bests(total_secs + 1);  // was calloc'ed array (unfreed?), changed djconnel

    // initialize ride_bests
    for (int i = 0; i < ride_bests.size(); i ++)
        ride_bests[i] = 0.0;

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

QDate
cpi_filename_to_date(const QString filename) {
    QRegExp rx("^(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_\\d\\d_\\d\\d_\\d\\d\\.cpi$");
    if (rx.exactMatch(filename)) {
        assert(rx.numCaptures() == 3);
        QDate date = QDate(rx.cap(1).toInt(),
                           rx.cap(2).toInt(),
                           rx.cap(3).toInt());
        if (date.isValid())
            return date;
    }
    return QDate(); // nil date
}

static int
read_one(const char *inname, QVector<double> &bests, QVector<QDate> &bestDates, QHash <QString, bool> *cpiDataInBests)
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
        if (secs >= bests.size()) {
            bests.resize(secs + 1);
            bestDates.resize(secs + 1);
        }
        if (bests[secs] < watts){
            bests[secs] = watts;
            bestDates[secs] = cpi_filename_to_date(QString(inname));

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

static int
read_cpi_file(const QDir &dir, const QFileInfo &raw, QVector<double> &bests, QVector<QDate> &bestDates, QHash <QString, bool> *cpiDataInBests)
{
    QString inname = dir.absoluteFilePath(raw.completeBaseName() + ".cpi");
    return read_one(inname.toAscii().constData(), bests, bestDates, cpiDataInBests);
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
            fprintf(stderr, "maximum number of loops %d exceeded in cp model extraction\n", max_loops);
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
    assert(!CPCurve);

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
        cp_curve_power[i] = cp * (1 + tau / (t + t0));
    }

    // generate a plot
    QString curve_title;
#if USE_T0_IN_CP_MODEL
    curve_title.sprintf("CP=%.1f W; AWC/CP=%.2f m; t0=%.1f s", cp, tau, 60 * t0);
#else
    curve_title.sprintf("CP=%.1f W; AWC/CP=%.2f m", cp, tau);
#endif

    CPCurve = new QwtPlotCurve(curve_title);
    CPCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
    QPen pen(Qt::red);
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
        foreach (QwtPlotCurve *curve, allCurves) {
            if (curve)
                delete curve;
        }
        allCurves.clear();
    }

    // now delete any labels
    if (allZoneLabels.size()) {
        foreach (QwtPlotMarker *label, allZoneLabels) {
            if (label)
                delete label;
        }
        allZoneLabels.clear();
    }
}

void
CpintPlot::plot_allCurve(CpintPlot *thisPlot,
                         int n_values,
                         const double *power_values)
{
    clear_CP_Curves();

    QVector<double> time_values(n_values);
    // generate an array of time values
    for (int t = 1; t <= n_values; t++)
        time_values[t - 1] = t / 60.0;

    // generate zones from derived CP value
    if (cp > 0) {
        QList <int> power_zone;
        int n_zones = (*zones)->lowsFromCP(&power_zone, (int) int(cp));

        QList <int> n_zone;

        // the lowest zone goes to zero power, so mark its start at the last data point
        n_zone.append(n_values - 1);

        // start the search at the next-to-lowest zone
        int z = 1;

        // search the maximal power curve to extract the zone times
        for (int i = n_values; i-- > 0;) {
            // if we reach the beginning of the curve OR if we hit a zone boundary, we're done with the present zone
            if ((i == 0) || (power_values[i] > power_zone[z])) {
                n_zone.append(
                    (z == n_zones) ?
                    0 :
                    (
                        (
                            (i == n_values - 1) ||
                            (abs(power_values[i] - power_zone[z]) < abs(power_zone[z] - power_values[i + 1]))
                        ) ?
                        i :
                        i + 1
                    )
                    );

                // draw curves for the zone we're leaving, if it spans any segments
                if (n_zone[z - 1] > n_zone[z]) {
                    // define the individual code segments.  Note in the old code with a single segment, it was
                    // part of the class.  This curve is not a protected member of the class.  djconnel Apr2009
                    QwtPlotCurve *curve;
                    curve =
                        new QwtPlotCurve((*zones)->getDefaultZoneName(z - 1));
                    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
                    QPen pen(zoneColor(z - 1, n_zones));
                    pen.setWidth(2.0);
                    curve->setPen(pen);
                    curve->attach(thisPlot);
                    QColor brush_color = zoneColor(z - 1, n_zones);
                    brush_color.setAlpha(64);
                    curve->setBrush(brush_color);   // brush fills below the line
                    curve->setData(time_values.data() + n_zone[z],
                                   power_values + n_zone[z],
                                   n_zone[z - 1] - n_zone[z] + 1);

                    // add the curve to the list
                    allCurves.append(curve);

                    // render a colored label on the zone
                    QwtText text((*zones)->getDefaultZoneName(z - 1));
                    text.setFont(QFont("Helvetica",24, QFont::Bold));
                    QColor text_color = zoneColor(z - 1, n_zones);
                    text_color.setAlpha(128);
                    text.setColor(text_color);
                    QwtPlotMarker *label_mark;
                    label_mark = new QwtPlotMarker();

                    // place the text in the geometric mean in time, at a decent power
                    label_mark->setValue(sqrt(time_values[n_zone[z-1]] * time_values[n_zone[z]]),
                                         (power_values[n_zone[z-1]] + power_values[n_zone[z]]) / 5);
                    label_mark->setLabel(text);
                    label_mark->attach(thisPlot);
                    allZoneLabels.append(label_mark);
                }

                if (z < n_zones)
                    fprintf(stderr, "zone %s: %d watts, index = %d\n",
                            (*zones)->getDefaultZoneName(z).toAscii().constData(),
                            power_zone[z],
                            n_zone[z]);

                // if we're to the smallest time, we're done
                if (i == 0)
                    break;

                // increment zone number
                if (z < n_zones)
                    z ++;

                // if we're to the final zone, just jump to the beginning of the plot: we're done
                if (z == n_zones)
                    i = 1;

                // else, we've got to recheck this point for the next zone
                else
                    i ++;
            }
        }
    }
    // no zones available: just plot the curve without zones
    else {
        QwtPlotCurve *curve;
        curve = new QwtPlotCurve("maximal power");
        curve->setRenderHint(QwtPlotItem::RenderAntialiased);
        QPen pen(Qt::red);
        pen.setWidth(2.0);
        curve->setPen(pen);
        QColor brush_color = Qt::red;
        brush_color.setAlpha(64);
        curve->setBrush(brush_color);   // brush fills below the line
        curve->setData(time_values.data(),
                       power_values,
                       n_values);
        curve->attach(thisPlot);
        allCurves.append(curve);
    }

    // set the x-axis to span the time of the all-time curve, starting at 1 second
    thisPlot->setAxisScale(thisPlot->xBottom,
                           1.0 / 60,
                           time_values[n_values - 1]);

    // set the y-axis to go from zero to the maximum power, rounded up to nearest 100 watts
    thisPlot->setAxisScale(thisPlot->yLeft,
                           0,
                           100 * ceil( power_values[0] / 100 ));

}

void
CpintPlot::calculate(RideItem *rideItem)
{
    QString fileName = rideItem->fileName;
    QDateTime dateTime = rideItem->dateTime;
    QDir dir(path);
    QFileInfo file(fileName);
    zones = rideItem->zones;

    if (needToScanRides) {
        bests.clear();
        bestDates.clear();
        cpiDataInBests.clear();
        if (CPCurve) {
            delete CPCurve;
            CPCurve = NULL;
        }
        fflush(stderr);
        bool aborted = false;
        QList<cpi_file_info> to_update;
        cpi_files_to_update(dir, to_update);
        double progress_max = 0.0;
        if (!to_update.empty()) {
            foreach (const cpi_file_info &info, to_update) {
                QFile file(info.inname);
                QStringList errors;
                RideFile *rideFile =
                    RideFileFactory::instance().openRideFile(file, errors);
                if (rideFile) {
                    double x = rideFile->dataPoints().size();
                    progress_max += x * (x + 1.0) / 2.0;
                    delete rideFile;
                }
            }
        }
        progress = new QProgressDialog(
            QString(tr("Computing critical power intervals.\n"
                       "This may take a while.\n")),
            tr("Abort"), 0, 1000, this);
        double progress_sum = 0.0;
        int endingOffset = progress->labelText().size();
        if (!to_update.empty()) {
            int count = 1;
            foreach (const cpi_file_info &info, to_update) {
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
        if (!aborted) {
            QString existing = progress->labelText();
            existing.chop(progress->labelText().size() - endingOffset);
            QStringList filters;
            filters << "*.cpi";
            QStringList list = dir.entryList(filters, QDir::Files, QDir::Name);
            list = filterForSeason(list, startDate, endDate);
            progress->setLabelText(
                existing + tr("Aggregating over all files."));
            progress->setRange(0, list.size());
            progress->setValue(0);
            progress->show();
            foreach (const QString &filename, list) {
                QString path = dir.absoluteFilePath(filename);
                read_one(path.toAscii().constData(), bests, bestDates, &cpiDataInBests);
                progress->setValue(progress->value() + 1);
                QCoreApplication::processEvents();
                if (progress->wasCanceled()) {
                    aborted = true;
                    break;
                }
            }
        }
        if (!aborted && bests.size()) {
            int maxNonZero = 0;

            // check that total work doesn't decrease with time
            double maxwork = 0.0;

            for (int i = 0; i < bests.size(); ++i) {
                // record the date associated with each point's CPI file,
                if (bests[i] > 0)
                    maxNonZero = i;

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
                plot_CP_curve(this, cp, tau, t0);

                plot_allCurve(this, maxNonZero - 1, bests.constData() + 1);
            }
            needToScanRides = false;
        }
        delete progress;
        progress = NULL;
    }

    if (!needToScanRides) {
        if (thisCurve) {
            delete thisCurve;
            thisCurve = NULL;
        }
        QVector<double> bests;
        QVector<QDate> bestDates;
        if ((read_cpi_file(dir, file, bests, bestDates, NULL) == 0) && bests.size()) {
            QVector<double> timeArray(bests.size());
            int maxNonZero = 0;
            for (int i = 0; i < bests.size(); ++i) {
                timeArray[i] = i / 60.0;
                if (bests[i] > 0) maxNonZero = i;
            }
            if (maxNonZero > 1) {
                thisCurve = new QwtPlotCurve(
                    dateTime.toString("ddd MMM d, yyyy h:mm AP"));
                thisCurve->setRenderHint(QwtPlotItem::RenderAntialiased);
                thisCurve->setPen(QPen(Qt::black));
                thisCurve->attach(this);
                thisCurve->setData(timeArray.data() + 1, bests.constData() + 1,
                                   maxNonZero - 1);
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

void
CpintPlot::setStartDate(QDate _startDate)
{
    startDate = _startDate;
}

void
CpintPlot::setEndDate(QDate _endDate)
{
    endDate = _endDate;
}
