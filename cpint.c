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
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include "pt.h"

struct point 
{
    double secs;
    int watts;
    struct point *next;
};

static struct point *head, *tail;
time_t start_since_epoch;
static double last_secs;
static int rec_int_ms = 1.26 * 1000.0;

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

void
one_file(FILE *in, FILE *out) 
{
    double start_secs, prev_secs, dur_secs, avg, sum;
    int dur_ints, i, total_intervals;
    double *bests;
    struct point *p, *q;

    if (head) {
        p = head;
        while (p) { q = p; p = p->next; free(q); }
        head = tail = NULL;
    }
    start_since_epoch = 0;
    last_secs = 0.0;

    pt_read_raw(in, 0 /* not compat */, NULL, NULL, 
                &time_cb, &data_cb, &error_cb);

    total_intervals = secs_to_interval(tail->secs);
    bests = (double*) calloc(total_intervals + 1, sizeof(double));

    start_secs = prev_secs = 0.0;
    for (p = head; p; p = p->next) {
        sum = 0.0;
        for (q = p; q; q = q->next) {
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
}

void
update_cpi_files(const char *dir, void (*one_done_cb)(const char *date))
{
    DIR *dirp;
    struct dirent *dp;
    regex_t reg;
    struct stat sbi, sbo;
    FILE *in, *out;
    char *outname;
    char year[5], mon[3], day[3], hour[3], min[3], sec[3];
    char longdate[26];
    struct tm time;
    time_t t;

    int nmatch = 7;
    regmatch_t *pmatch = (regmatch_t*) calloc(nmatch, sizeof(regmatch_t));

    if (regcomp(&reg, "^([0-9][0-9][0-9][0-9])_([0-9][0-9])_([0-9][0-9])"
                "_([0-9][0-9])_([0-9][0-9])_([0-9][0-9])\\.raw$", REG_EXTENDED))
        assert(0);

    outname = malloc(strlen(dir) + 25);
    dirp = opendir(dir);
    while ((dp = readdir(dirp)) != NULL) {
        if (regexec(&reg, dp->d_name, nmatch, pmatch, 0) == 0) {
            if (stat(dp->d_name, &sbi))
                assert(0);
            sprintf(outname, "%s/%s", dir, dp->d_name);
            strcpy(outname + strlen(outname) - 4, ".cpi");
            if ((stat(outname, &sbo)) || (sbo.st_mtime < sbi.st_mtime)) {
                strncpy(year, dp->d_name + pmatch[1].rm_so, 4); year[4] = '\0';
                strncpy(mon, dp->d_name + pmatch[2].rm_so, 2); mon[2] = '\0';
                strncpy(day, dp->d_name + pmatch[3].rm_so, 2); day[2] = '\0';
                strncpy(hour, dp->d_name + pmatch[4].rm_so, 2); hour[2] = '\0';
                strncpy(min, dp->d_name + pmatch[5].rm_so, 2); min[2] = '\0';
                strncpy(sec, dp->d_name + pmatch[6].rm_so, 2); sec[2] = '\0';
                memset(&time, 0, sizeof(time));
                time.tm_year = atoi(year) - 1900;
                time.tm_mon = atoi(mon) - 1;
                time.tm_mday = atoi(day);
                time.tm_hour = atoi(hour);
                time.tm_min = atoi(min);
                time.tm_sec = atoi(sec);
                time.tm_isdst = -1;
                t = mktime(&time);
                assert(t != -1);
                ctime_r(&t, longdate);
                longdate[24] = '\0'; /* get rid of newline */
                one_done_cb(longdate);
                fflush(stderr);
                in = fopen(dp->d_name, "r");
                assert(in);
                out = fopen(outname, "w");
                assert(out);
                one_file(in, out);
                fclose(in);
                fclose(out);
                fprintf(stderr, "done.\n");
            }
        }
    }
    closedir(dirp);
    free(pmatch);
}

void
combine_cpi_files(const char *dir, double *bests[], int *bestlen)
{
    DIR *dirp;
    struct dirent *dp;
    FILE *in;
    char line[40];
    int lineno;
    double mins;
    int watts;
    double *tmp;
    int interval;

    *bestlen = 1000;

    *bests = calloc(*bestlen, sizeof(double));
    dirp = opendir(dir);
    while ((dp = readdir(dirp)) != NULL) {
        if (strcmp(".cpi", dp->d_name + dp->d_namlen - 4) == 0) {
            in = fopen(dp->d_name, "r");
            assert(in);
            lineno = 1;
            while (fgets(line, sizeof(line), in) != NULL) {
                if (sscanf(line, "%lf %d\n", &mins, &watts) != 2) {
                    fprintf(stderr, "Bad match on line %d: %s", lineno, line);
                    exit(1);
                }
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
        }
    }
    closedir(dirp);
}


