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

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <dirent.h>
#include <math.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpint.h"

static void
one_done_cb(const char *longdate) 
{
    fprintf(stderr, "Compiling data for ride on %s...", longdate);
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
    update_cpi_files(dir, one_done_cb);
    combine_cpi_files(dir, &bests, &bestlen);
    for (i = 0; i < bestlen; ++i) {
        if (bests[i] != 0)
            printf("%6.3f %3.0f\n", i * 0.021, round(bests[i]));
    }
    return 0;
}

