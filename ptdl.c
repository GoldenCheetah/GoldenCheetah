/* 
 * $Id: ptdl.c,v 1.10 2006/09/06 23:23:03 srhea Exp $
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
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pt.h"

#define MAX_DEVICES 20

int force;

static void
time_cb(struct tm *time, void *user_data)
{
    char outname[24];
    FILE **out = (FILE**) user_data;
    if (!*out) {
        if (!time) {
            fprintf(stderr, "Can't find ride time;"
                    " specify output file with -o.\n");
            exit(1);
        }
        sprintf(outname, "%04d_%02d_%02d_%02d_%02d_%02d.raw", 
                time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, 
                time->tm_hour, time->tm_min, time->tm_sec);
        assert(strlen(outname) == sizeof(outname) - 1);
        fprintf(stderr, "done.\nWriting to %s.\n", outname);
        if ((*out = fopen(outname, "r")) != NULL) {
            if (force)
                fclose(*out);
            else {
                fprintf(stderr, "Error: %s already exists!  "
                        "Specify -f to overwrite it.\n", outname);
                exit(1);
            }
        }
        if ((*out = fopen(outname, "w")) == NULL) {
            fprintf(stderr, "Couldn't open %s for writing: %s", 
                    outname, strerror(errno));
            exit(1);
        }
        fprintf(stderr, "Reading ride data...");
        fflush(stderr);
    }
}

static void
record_cb(unsigned char *buf, void *user_data) 
{
    static int count = 0;
    int i;
    FILE **out = (FILE**) user_data;
    for (i = 0; i < 6; ++i)
        fprintf(*out, "%02x%s", buf[i], (i == 5) ? "\n" : " ");
    if ((++count % 256) == 0) {
        fprintf(stderr, ".");
        fflush(stderr);
    }
}

static void
usage(const char *progname) 
{
    fprintf(stderr, "usage: %s [-d <device>] [-e] [-f] [-o <output file>]\n", 
            progname);
    exit(1);
}

int 
main(int argc, char *argv[])
{
    int i, ch, fd, r;
    char *devices[MAX_DEVICES];
    int dev_cnt = 0;
    char *outname;
    FILE *out = NULL;
    struct pt_read_version_state vstate;
    struct pt_read_data_state dstate;
    struct timeval timeout;
    fd_set readfds;
    int hwecho = 0;

    while ((ch = getopt(argc, argv, "d:efho:v")) != -1) {
        switch (ch) {
            case 'd':
                devices[0] = optarg;
                dev_cnt = 1;
                break;
            case 'e':
                hwecho = 1;
                break;
            case 'f':
                force = 1;
                break;
            case 'o':
                outname = optarg;
                if (strcmp(outname, "-") == 0)
                    out = stdout;
                else if ((out = fopen(outname, "w")) == NULL) {
                    fprintf(stderr, "Couldn't open %s for writing: %s", 
                            outname, strerror(errno));
                    exit(1);
                }
                break;
            case 'v':
                pt_debug_level = PT_DEBUG_MAX;
                break;
            case 'h':
            case '?':
            default:
                usage(argv[0]);
        }
    }
    argc -= optind;
    argv += optind;

    if (!dev_cnt) {
        dev_cnt = pt_find_device(devices, MAX_DEVICES);
        if (dev_cnt == 0) {
            fprintf(stderr, "Can't find device; specify one with -d.\n");
            exit(1);
        }
        if (dev_cnt > 1) {
            fprintf(stderr, "Multiple devices present; specify one with -d:\n");
            for (i = 0; i < dev_cnt; ++i)
                fprintf(stderr, "    %s\n", devices[i]);
            exit(1);
        }
    }

    fprintf(stderr, "Reading from %s.\n", devices[0]);
    if (pt_hwecho(devices[0]))
        hwecho = 1;

    if (hwecho)
        fprintf(stderr, "Expecting hardware echo.\n");

    if (pt_debug_level >= PT_DEBUG_MAX)
        fprintf(stderr, "Opening device %s.\n", devices[0]);
    fd = open(devices[0], O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    fprintf(stderr, "Reading version information...");
    fflush(stderr);

    pt_make_async(fd);
    memset(&vstate, 0, sizeof(vstate));
    if (pt_debug_level >= PT_DEBUG_MAX)
        fprintf(stderr, "\nCalling pt_read_version.\n");
    while ((r = pt_read_version(&vstate, fd, hwecho)) != PT_DONE) {
        assert(r == PT_NEED_READ);
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if (pt_debug_level >= PT_DEBUG_MAX)
            fprintf(stderr, "Calling select.\n");
        select(fd + 1, &readfds, NULL, NULL, &timeout);
        if (!FD_ISSET(fd, &readfds)) {
            fprintf(stderr, "timeout.\n");
            exit(1);
        }
        if (pt_debug_level >= PT_DEBUG_MAX)
            fprintf(stderr, "\nCalling pt_read_version.\n");
    }
    fprintf(stderr, "done.\n");
    close(fd);

    if (pt_debug_level >= PT_DEBUG_MAX)
        fprintf(stderr, "Opening device %s.\n", devices[0]);
    fd = open(devices[0], O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    if (out)
        fprintf(stderr, "Writing to %s.\nReading ride data...", outname);
    else
        fprintf(stderr, "Reading ride time...");
    fflush(stderr);

    pt_make_async(fd);
    memset(&dstate, 0, sizeof(dstate));
    if (pt_debug_level >= PT_DEBUG_MAX)
        fprintf(stderr, "\nCalling pt_read_data.\n");
    while ((r = pt_read_data(&dstate, fd, hwecho, time_cb, 
                             record_cb, &out)) != PT_DONE) {
        assert(r == PT_NEED_READ);
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        if (pt_debug_level >= PT_DEBUG_MAX)
            fprintf(stderr, "Calling select.\n");
        select(fd + 1, &readfds, NULL, NULL, &timeout);
        if (!FD_ISSET(fd, &readfds)) {
            fprintf(stderr, "timeout.\n");
            exit(1);
        }
        if (pt_debug_level >= PT_DEBUG_MAX)
            fprintf(stderr, "\nCalling pt_read_data.\n");
    }
    fprintf(stderr, "done.\n");

    return 0;
}

