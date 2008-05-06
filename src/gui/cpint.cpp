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

#include <QVector>
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "cpint.h"
#include "RideFile.h"

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

void
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
            
void
update_cpi_file(const cpi_file_info *info, 
                int (*cancel_cb)(void *user_data),
                void *user_data) 
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
    double *bests = (double*) calloc(total_secs + 1, sizeof(double));

    bool canceled = false;
    int progress_count = 0;
    for (int i = 0; i < data.points.size() - 1; ++i) {
        cpint_point *p = &data.points[i];
        double sum = 0.0;
        double prev_secs = p->secs;
        for (int j = i + 1; j < data.points.size(); ++j) {
            cpint_point *q = &data.points[j];
            if (cancel_cb && (++progress_count % 1000 == 0)) {
                if (cancel_cb(user_data)) {
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

int
read_cpi_file(const QDir &dir, const QFileInfo &raw, QVector<double> &bests)
{
    QString inname = dir.absoluteFilePath(raw.completeBaseName() + ".cpi");
    return read_one(inname.toAscii().constData(), bests);
}

void
combine_cpi_files(const QDir &dir, QVector<double> &bests)
{
    QStringList filters;
    filters << "*.cpi";
    QStringList list = dir.entryList(filters, QDir::Files, QDir::Name);
    QListIterator<QString> i(list);
    while (i.hasNext()) {
        const QString &filename = i.next();
        QString path = dir.absoluteFilePath(filename);
        read_one(path.toAscii().constData(), bests);
    }
}

