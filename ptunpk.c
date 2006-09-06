/* 
 * $Id: ptunpk.c,v 1.4 2006/06/04 14:32:34 srhea Exp $
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

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pt.h"

#define KM_TO_MI 0.62137119
#define BAD_KM_TO_MI 0.62

static FILE *out;

static void 
config_cb(unsigned interval, unsigned rec_int, unsigned wheel_sz_mm, 
          void *context)
{
    context = NULL;
    fprintf(out, "# wheel size=%d mm, interval=%d, rec int=%d\n", 
           wheel_sz_mm, interval, rec_int);
}

static void
time_cb(struct tm *time, time_t since_epoch, void *context)
{
    context = NULL;
    fprintf(out, "# %d/%d/%d %d:%02d:%02d %d\n", 
           time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, 
           time->tm_hour, time->tm_min, time->tm_sec, (int) since_epoch);
}

static void
data_cb(double secs, double nm, double mph, double watts, double miles, 
        unsigned cad, unsigned hr, unsigned interval, void *context)
{
    context = NULL;
    fprintf(out, "%.3f %.1f", secs / 60.0, nm);
    if (mph == -1.0)
        fprintf(out, " NaN NaN");
    else
        fprintf(out, " %0.3f %.0f", mph, watts);
    fprintf(out, " %.5f %d", miles, cad);
    if (hr == 0)
        fprintf(out, " NaN");
    else
        fprintf(out, " %d", hr);
    fprintf(out, " %d\n", interval);
}

static void
error_cb(const char *msg, void *context) 
{
    context = NULL;
    fprintf(stderr, "%s\n", msg);
    exit(1);
}

static void
usage(const char *progname) 
{
    fprintf(stderr, "usage: %s [-c] [-o <output file>] [<input file>]\n", 
            progname);
    exit(1);
}

int 
main(int argc, char *argv[])
{
    int ch, i, compat = 0;
    char *inname = NULL, *outname = NULL;
    FILE *in;

    while ((ch = getopt(argc, argv, "cho:")) != -1) {
        switch (ch) {
            case 'c':
                compat = 1;
                break;
            case 'o':
                outname = optarg;
                if (strcmp(outname, "-") == 0) {
                    out = stdout;
                    outname = "STDOUT";
                }
                break;
            case 'h':
            case '?':
            default:
                usage(argv[0]);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0) {
        in = stdin;
        inname = "STDIN";
        if (outname == NULL) {
            out = stdout;
            outname = "STDOUT";
        }
    }
    else {
        inname = argv[0];
        if ((in = fopen(inname, "r")) == NULL) {
            fprintf(stderr, "Couldn't open %s for reading: %s\n", 
                    inname, strerror(errno));
            exit(1);
        }
        if (outname == NULL) {
            outname = malloc(strlen(inname) + 5);
            strcpy(outname, inname);
            for (i = strlen(outname); i >= 0; --i)
                if (outname[i] == '.') break;
            if (i >= 0)
                strcpy(outname + i + 1, "dat");
            else
                strcpy(outname + strlen(outname), ".dat");
        }
    }

    if ((out == NULL) && ((out = fopen(outname, "w")) == NULL)) {
        fprintf(stderr, "Couldn't open %s for writing: %s\n", 
                outname, strerror(errno));
        exit(1);
    }

    fprintf(out, "# Time Torq MPH Watts Miles Cad HR Int\n");

    pt_read_raw(in, compat, NULL, config_cb, time_cb, data_cb, error_cb);

    return 0;
}

