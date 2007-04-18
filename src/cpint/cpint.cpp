/* 
 * $Id: cpint.c,v 1.4 2006/08/11 19:53:07 srhea Exp $
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

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "srm.h"
#include "pt.h"
#include "cpint.h"

struct cpint_point 
{
    double secs;
    int watts;
    cpint_point(double s, int w) : secs(s), watts(w) {}
};

struct cpint_data {
    QList<cpint_point> points;
    time_t start_since_epoch;
    double last_secs;
    int rec_int_ms;
    cpint_data() : start_since_epoch(0), last_secs(0.0), rec_int_ms(0) {}
};

static void 
config_cb(unsigned interval, unsigned rec_int, 
          unsigned wheel_sz_mm, void *context) 
{
    (void) interval;
    (void) wheel_sz_mm;
    cpint_data *data = (cpint_data*) context;
    int new_rec_int_ms = rec_int * 1260;
    assert((data->rec_int_ms == 0) || (data->rec_int_ms == new_rec_int_ms));
    data->rec_int_ms = new_rec_int_ms;
}

static void
time_cb(struct tm *time, time_t since_epoch, void *context)
{
    (void) time;
    cpint_data *data = (cpint_data*) context;
    if (data->start_since_epoch == 0)
        data->start_since_epoch = since_epoch;
    double secs = since_epoch - data->start_since_epoch;
    /* Be conservative: a sleep interval counts as all zeros. */
    data->points.append(cpint_point(secs, 0));
    data->last_secs = secs;
}

static void
data_cb(double secs, double nm, double mph, double watts, double miles, 
        unsigned cad, unsigned hr, unsigned interval, void *context)
{
    cpint_data *data = (cpint_data*) context;
    (void) nm;
    (void) miles;
    (void) cad;
    (void) hr;
    (void) interval;
    /* Be conservative: count NaN's as zeros. */
    data->points.append(cpint_point(secs, ((mph == -1.0) ? 0 
                                           : (int) round(watts)))); 
    data->last_secs = secs;
}

static void
error_cb(const char *msg, void *context) 
{
    (void) context;
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

cpi_file_info *
cpi_files_to_update(const char *dir)
{
    DIR *dirp;
    struct dirent *dp;
    regex_t reg;
    struct stat sbi, sbo;
    char *inname, *outname;
    cpi_file_info *head = NULL, *tail = NULL;

    if (regcomp(&reg, "^([0-9][0-9][0-9][0-9])_([0-9][0-9])_([0-9][0-9])"
                "_([0-9][0-9])_([0-9][0-9])_([0-9][0-9])\\.raw$", REG_EXTENDED))
        assert(0);

    dirp = opendir(dir);
    assert(dirp);
    while ((dp = readdir(dirp)) != NULL) {
        int nmatch = 7;
        regmatch_t *pmatch = (regmatch_t*) calloc(nmatch, sizeof(regmatch_t));
        if (regexec(&reg, dp->d_name, nmatch, pmatch, 0) == 0) {
            inname = (char*) malloc(strlen(dir) + 25);
            outname = (char*) malloc(strlen(dir) + 25);
            sprintf(inname, "%s/%s", dir, dp->d_name);
            if (stat(inname, &sbi))
                assert(0);
            sprintf(outname, "%s/%s", dir, dp->d_name);
            strcpy(outname + strlen(outname) - 4, ".cpi");
            if ((stat(outname, &sbo)) || (sbo.st_mtime < sbi.st_mtime)) {
                cpi_file_info *info = 
                    (cpi_file_info*) malloc(sizeof(cpi_file_info));
                info->file = strdup(dp->d_name);
                info->inname = inname;
                info->outname = outname;
                info->pmatch = pmatch;
                info->next = NULL;
                if (head == NULL)
                    head = tail = info;
                else {
                    tail->next = info;
                    tail = info;
                }
            }
            else {
                free(inname);
                free(outname);
            }
        }
        else {
            free(pmatch);
        }
    }
    closedir(dirp);

    return head;
}
            
void
update_cpi_file(cpi_file_info *info, 
                int (*cancel_cb)(void *user_data),
                void *user_data) 
{
    FILE *in = fopen(info->inname, "r");
    assert(in);
    FILE *out = fopen(info->outname, "w");
    assert(out);

    cpint_data data;
    pt_read_raw(in, 0 /* not compat */, &data, 
                &config_cb, &time_cb, &data_cb, &error_cb);

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
            sum += (q->secs - prev_secs) * q->watts;
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
    fclose(in);
    fclose(out);
    if (canceled)
        unlink(info->outname);
}

void
free_cpi_file_info(cpi_file_info *head) 
{
    cpi_file_info *tmp;
    while (head) {
        free(head->file);
        free(head->inname);
        free(head->outname);
        free(head->pmatch);
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

static int 
read_one(const char *inname, double *bests[], int *bestlen)
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
        while (secs >= *bestlen) {
            double *tmp = (double*) calloc(*bestlen * 2, sizeof(double));
            memcpy(tmp, *bests, *bestlen * sizeof(double));
            free(*bests);
            *bests = tmp;
            *bestlen *= 2;
        }
        if ((*bests)[secs] < watts)
            (*bests)[secs] = watts;
        ++lineno;
    }
    fclose(in);
    return 0;
}

int
read_cpi_file(const char *dir, const char *raw, double *bests[], int *bestlen)
{
    *bestlen = 1000;
    *bests = (double*) calloc(*bestlen, sizeof(double));
    char *inname = (char*) malloc(strlen(dir) + 25);
    sprintf(inname, "%s/%s", dir, raw);
    strcpy(inname + strlen(inname) - 4, ".cpi");
    int result = read_one(inname, bests, bestlen);
    free(inname);
    return result;
}

void
combine_cpi_files(const char *dir, double *bests[], int *bestlen)
{
    DIR *dirp;
    struct dirent *dp;
    char *inname;

    *bestlen = 1000;
    *bests = (double*) calloc(*bestlen, sizeof(double));
    inname = (char*) malloc(strlen(dir) + 25);
    dirp = opendir(dir);
    assert(dirp);
    while ((dp = readdir(dirp)) != NULL) {
        if (strcmp(".cpi", dp->d_name + strlen(dp->d_name) - 4) == 0) {
            sprintf(inname, "%s/%s", dir, dp->d_name);
            read_one(inname, bests, bestlen);
        }
    }
    closedir(dirp);
    free(inname);
}

