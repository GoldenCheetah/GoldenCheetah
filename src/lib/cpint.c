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
#include "pt.h"
#include "cpint.h"

struct point 
{
    double secs;
    int watts;
    struct point *next;
};

static struct point *head, *tail;
time_t start_since_epoch;
static double last_secs;
static int rec_int_ms;

static int
secs_to_interval(double secs)
{
    if (rec_int_ms == 0) {
        fprintf(stderr, "Missing recording interval.\n");
        exit(1);
    }
    return (int) round(secs * 1000.0 / rec_int_ms);
}

static void 
add_point(double secs, double watts) 
{
    struct point *p = (struct point*) malloc(sizeof(struct point));
    p->secs = secs;
    p->watts = watts;
    p->next = NULL;
    if (tail) tail->next = p; else head = p;
    tail = p;
}

static void 
config_cb(unsigned interval, unsigned rec_int, 
          unsigned wheel_sz_mm, void *context) 
{
    double new_rec_int_ms = rec_int * 1.26 * 1000.0;
    interval = wheel_sz_mm = 0;
    context = NULL;
    assert((rec_int_ms == 0.0) || (rec_int_ms == new_rec_int_ms));
    rec_int_ms = new_rec_int_ms;
}

static void
time_cb(struct tm *time, time_t since_epoch, void *context)
{
    double secs;
    context = NULL;
    time = NULL;
    if (start_since_epoch == 0)
        start_since_epoch = since_epoch;
    secs = since_epoch - start_since_epoch;
    /* Be conservative: a sleep interval counts as all zeros. */
    add_point(secs, 0.0);
    last_secs = secs;
}

static void
data_cb(double secs, double nm, double mph, double watts, double miles, 
        unsigned cad, unsigned hr, unsigned interval, void *context)
{
    context = NULL;
    nm = 0.0; miles = 0.0; cad = 0; hr = 0; interval = 0;
    /* Be conservative: count NaN's as zeros. */
    add_point(secs, (mph == -1.0) ? 0.0 : watts);
    last_secs = secs;
}

static void
error_cb(const char *msg, void *context) 
{
    context = NULL;
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

struct cpi_file_info *
cpi_files_to_update(const char *dir)
{
    DIR *dirp;
    struct dirent *dp;
    regex_t reg;
    struct stat sbi, sbo;
    char *inname, *outname;
    struct cpi_file_info *head = NULL, *tail = NULL;

    if (regcomp(&reg, "^([0-9][0-9][0-9][0-9])_([0-9][0-9])_([0-9][0-9])"
                "_([0-9][0-9])_([0-9][0-9])_([0-9][0-9])\\.raw$", REG_EXTENDED))
        assert(0);

    dirp = opendir(dir);
    assert(dirp);
    while ((dp = readdir(dirp)) != NULL) {
        int nmatch = 7;
        regmatch_t *pmatch = (regmatch_t*) calloc(nmatch, sizeof(regmatch_t));
        if (regexec(&reg, dp->d_name, nmatch, pmatch, 0) == 0) {
            inname = malloc(strlen(dir) + 25);
            outname = malloc(strlen(dir) + 25);
            sprintf(inname, "%s/%s", dir, dp->d_name);
            if (stat(inname, &sbi))
                assert(0);
            sprintf(outname, "%s/%s", dir, dp->d_name);
            strcpy(outname + strlen(outname) - 4, ".cpi");
            if ((stat(outname, &sbo)) || (sbo.st_mtime < sbi.st_mtime)) {
                struct cpi_file_info *info = (struct cpi_file_info*) 
                        malloc(sizeof(struct cpi_file_info));
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
update_cpi_file(struct cpi_file_info *info, 
                int (*cancel_cb)(void *user_data),
                void *user_data) 
{
    FILE *in, *out;
    int canceled = 0;
    double start_secs, prev_secs, dur_secs, avg, sum;
    int dur_ints, i, total_intervals;
    double *bests;
    struct point *p, *q;
    int progress_count = 0;

    in = fopen(info->inname, "r");
    assert(in);
    out = fopen(info->outname, "w");
    assert(out);
    if (head) {
        p = head;
        while (p) { q = p; p = p->next; free(q); }
        head = tail = NULL;
    }
    start_since_epoch = 0;
    last_secs = 0.0;

    pt_read_raw(in, 0 /* not compat */, NULL, 
                &config_cb, &time_cb, &data_cb, &error_cb);

    total_intervals = secs_to_interval(tail->secs);
    bests = (double*) calloc(total_intervals + 1, sizeof(double));

    start_secs = prev_secs = 0.0;
    for (p = head; p; p = p->next) {
        sum = 0.0;
        for (q = p; q; q = q->next) {
            if (cancel_cb && (++progress_count % 1000 == 0)) {
                if (cancel_cb(user_data)) {
                    canceled = 1;
                    goto done;
                }
            }
            sum += (q->secs - prev_secs) * q->watts;
            dur_secs = q->secs - start_secs;
            dur_ints = secs_to_interval(dur_secs);
            avg = sum / dur_secs;
            if (bests[dur_ints] < avg)
                bests[dur_ints] = avg;
            prev_secs = q->secs;
        } 
        start_secs = prev_secs = p->secs;
    }

    for (i = 0; i <= total_intervals; ++i) {
        if (bests[i] != 0)
            fprintf(out, "%6.3f %3.0f\n", i * rec_int_ms / 1000.0 / 60.0, 
                   round(bests[i]));
    }

done:
    fclose(in);
    fclose(out);
    if (canceled)
        unlink(info->outname);
}

void
free_cpi_file_info(struct cpi_file_info *head) 
{
    struct cpi_file_info *tmp;
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
    FILE *in;
    char line[40];
    int lineno;
    double mins;
    int watts;
    double *tmp;
    int interval;

    if (!(in = fopen(inname, "r")))
        return -1;
    lineno = 1;
    while (fgets(line, sizeof(line), in) != NULL) {
        if (sscanf(line, "%lf %d\n", &mins, &watts) != 2) {
            fprintf(stderr, "Bad match on line %d: %s", lineno, line);
            exit(1);
        }
        if (rec_int_ms == 0)
            rec_int_ms = mins * 60.0 * 1000.0;
        interval = secs_to_interval(mins * 60.0);
        while (interval >= *bestlen) {
            tmp = calloc(*bestlen * 2, sizeof(double));
            memcpy(tmp, *bests, *bestlen * sizeof(double));
            free(*bests);
            *bests = tmp;
            *bestlen *= 2;
        }
        if ((*bests)[interval] < watts)
            (*bests)[interval] = watts;
        ++lineno;
    }
    fclose(in);
    return 0;
}

int
read_cpi_file(const char *dir, const char *raw, double *bests[], int *bestlen)
{
    char *inname;
    int result;

    *bestlen = 1000;
    *bests = calloc(*bestlen, sizeof(double));
    inname = malloc(strlen(dir) + 25);
    sprintf(inname, "%s/%s", dir, raw);
    strcpy(inname + strlen(inname) - 4, ".cpi");
    result = read_one(inname, bests, bestlen);
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
    *bests = calloc(*bestlen, sizeof(double));
    inname = malloc(strlen(dir) + 25);
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


