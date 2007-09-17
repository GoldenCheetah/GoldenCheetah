/* 
 * $Id: ptpk.c,v 1.1 2006/05/27 16:17:25 srhea Exp $
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

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pt.h"

void
pt_pack(FILE *in, FILE *out, int wheel_sz_mm, int rec_int)
{
    double mins, nm, mph, watts, miles;
    int cad, hr, interval = 0;
    double last_mins = 0.0, last_miles = 0.0;
    int last_interval = 0;
    int i;
    struct tm start_tm, inc_tm;
    time_t start_time = 0, inc_time;
    int lineno = 1;
    char line[256];
    unsigned char buf[6];
    regex_t date_reg;
    int date_nmatch = 7;
    regmatch_t *date_pmatch = 
        (regmatch_t*) calloc(date_nmatch, sizeof(regmatch_t));
    regex_t rider_reg;
    int rider_nmatch = 12;
    regmatch_t *rider_pmatch = 
        (regmatch_t*) calloc(rider_nmatch, sizeof(regmatch_t));

    if (regcomp(&date_reg, "^<RideDate>([0-9][0-9]?)\\\\([0-9][0-9]?)\\\\"
                "([0-9][0-9][0-9][0-9]) +([0-9][0-9]?):([0-9][0-9]?):"
                "([0-9][0-9]?)</RideDate> *$", REG_EXTENDED))
        assert(0);

   if (regcomp(&rider_reg, "^<Rider><Minutes>([0-9]+\\.[0-9]+)</Minutes>"
               "<TorqinLBS>([0-9]+(\\.[0-9]+)?)</TorqinLBS>"
               "<Mph>([0-9]+(\\.[0-9]+)?|NaN)</Mph><Watts>([0-9]+|NaN)</Watts>"
               "<Miles>([0-9]+(\\.[0-9]+)?)</Miles><Cadence>([0-9]+)</Cadence>"
               "<Hrate>([0-9]+|NaN)</Hrate><ID>([0-9]+)</ID></Rider>$", 
               REG_EXTENDED))
        assert(0);

    pt_pack_header(buf);
    pt_write_data(out, buf);

    while (fgets(line, sizeof(line), in)) {
        line[strlen(line) - 1] = '\0'; /* drop newline */
        if (strcmp(line, "<?xml version=\"1.0\"  encoding=\"UTF-8\"?>") == 0) {
            /* ignore it */
        }
        else if (strcmp(line, "<RideData>") == 0) {
            /* ignore it */
        }
        else if (strcmp(line, "</RideData>") == 0) {
            /* ignore it */
        }
        else if (!regexec(&date_reg, line, date_nmatch, date_pmatch, 0)) {
            for (i = 1; i < date_nmatch; ++i)
                line[date_pmatch[i].rm_eo] = '\0';
            memset(&start_tm, 0, sizeof(start_tm));
            start_tm.tm_year = atoi(line + date_pmatch[3].rm_so) - 1900;
            start_tm.tm_mon = atoi(line + date_pmatch[1].rm_so) - 1;
            start_tm.tm_mday = atoi(line + date_pmatch[2].rm_so);
            start_tm.tm_hour = atoi(line + date_pmatch[4].rm_so);
            start_tm.tm_min = atoi(line + date_pmatch[5].rm_so);
            start_tm.tm_sec = atoi(line + date_pmatch[6].rm_so);
            start_tm.tm_isdst = -1;
            start_time = mktime(&start_tm);
            assert(start_time != -1);
            pt_pack_time(buf, &start_tm);
            pt_write_data(out, buf);
            pt_pack_config(buf, interval, rec_int, wheel_sz_mm);
            pt_write_data(out, buf);
        }
        else if (!regexec(&rider_reg, line, rider_nmatch, rider_pmatch, 0)) {
            for (i = 1; i < rider_nmatch; ++i)
                line[rider_pmatch[i].rm_eo] = '\0';
            mins = atof(line + rider_pmatch[1].rm_so);
            nm = atof(line + rider_pmatch[2].rm_so);
            if (strcmp(line + rider_pmatch[4].rm_so, "NaN") == 0) 
                mph = -1.0;
            else
                mph = atof(line + rider_pmatch[4].rm_so);
            if (strcmp(line + rider_pmatch[6].rm_so, "NaN") == 0) 
                watts = -1.0;
            else
                watts = atof(line + rider_pmatch[6].rm_so);
            miles = atof(line + rider_pmatch[7].rm_so);
            cad = atoi(line + rider_pmatch[9].rm_so);
            if (strcmp(line + rider_pmatch[10].rm_so, "NaN") == 0) 
                hr = 255;
            else
                hr = atoi(line + rider_pmatch[10].rm_so);
            interval = atoi(line + rider_pmatch[11].rm_so);
            if (mins - last_mins - 0.021 > 1.0 / 60.0) {
                struct tm *tm_p;
                inc_time = start_time 
                    + (unsigned) round((mins - 0.021000001) * 60.0);
                tm_p = localtime(&inc_time);
                assert(tm_p);
                inc_tm = *tm_p;
                pt_pack_time(buf, &inc_tm);
                pt_write_data(out, buf);
                pt_pack_config(buf, interval, rec_int, wheel_sz_mm);
                pt_write_data(out, buf);
            }
            else if (last_interval != interval) {
                pt_pack_config(buf, interval, rec_int, wheel_sz_mm);
                pt_write_data(out, buf);
            }
            pt_pack_data(buf, wheel_sz_mm, nm, mph,
                         miles - last_miles, cad, hr); 
            pt_write_data(out, buf);
            last_mins = mins;
            last_interval = interval;
            last_miles = miles;
        }
        else {
            fprintf(stderr, "ERROR: line %d unrecognized: \"%s\"\n", 
                    lineno, line);
            exit(1);
        }
        ++lineno;
    }
}

static void
usage(const char *progname) 
{
    fprintf(stderr, "usage: %s [-o <output file>] [-r <recording interval>]"
            "[-w <wheel size (mm)>] [<input file>]\n", 
            progname);
    exit(1);
}

int 
main(int argc, char *argv[])
{
    int ch, i;
    int wheel_sz_mm = 2096;
    int rec_int = 1;
    char *inname = NULL, *outname = NULL;
    FILE *in, *out = NULL;

    while ((ch = getopt(argc, argv, "ho:r:w:")) != -1) {
        switch (ch) {
            case 'o':
                outname = optarg;
                if (strcmp(outname, "-") == 0) {
                    out = stdout;
                    outname = "STDOUT";
                }
                break;
            case 'r':
                rec_int = atoi(optarg);
                break;
            case 'w':
                wheel_sz_mm = atoi(optarg);
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
            outname = (char*) malloc(strlen(inname) + 5);
            strcpy(outname, inname);
            for (i = strlen(outname); i >= 0; --i)
                if (outname[i] == '.') break;
            if (i >= 0)
                strcpy(outname + i + 1, "raw");
            else
                strcpy(outname + strlen(outname), ".raw");
        }
    }

    if ((out == NULL) && ((out = fopen(outname, "w")) == NULL)) {
        fprintf(stderr, "Couldn't open %s for writing: %s\n", 
                outname, strerror(errno));
        exit(1);
    }

    pt_pack(in, out, wheel_sz_mm, rec_int);

    fclose(in);
    fclose(out);

    return 0;
}

