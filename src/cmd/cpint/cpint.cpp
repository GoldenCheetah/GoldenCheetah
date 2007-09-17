/* 
 * $Id: cpint-cmd.c,v 1.1 2006/08/11 19:53:07 srhea Exp $
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

#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cpint.h"

struct cpi_file_info *head;

static void
canceled(int unused) 
{
    unused = 0;
    if (head) {
        fprintf(stderr, "calceled.\n");
        unlink(head->outname);
    }
    exit(1);
}

int
main(int argc, char *argv[])
{
    int i;
    double *bests;
    int bestlen;
    char *dir = ".";
    if (argc > 1)
        dir = argv[1];
    signal(SIGINT, canceled);
    head = cpi_files_to_update(dir);
    while (head) {
        fprintf(stderr, "Processing ride file %s...", head->file);
        fflush(stderr);
        update_cpi_file(head, NULL, NULL);
        fprintf(stderr, "done.\n");
        head = head->next;
    }
    combine_cpi_files(dir, &bests, &bestlen);
    for (i = 0; i < bestlen; ++i) {
        if (bests[i] != 0)
            printf("%6.3f %3.0f\n", i / 60.0, round(bests[i]));
    }
    return 0;
}

