/* 
 * $Id: cpint.h,v 1.1 2006/08/11 19:53:07 srhea Exp $
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

#ifndef __cpint_h
#define __cpint_h 1

#include <regex.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cpi_file_info {
    char *file, *inname, *outname;
    regmatch_t *pmatch;
    struct cpi_file_info *next;
};

extern struct cpi_file_info * cpi_files_to_update(const char *dir);

extern void update_cpi_file(struct cpi_file_info *info, 
                            int (*cancel_cb)(void *user_data),
                            void *user_data);

extern void free_cpi_file_info(struct cpi_file_info *head);

extern int read_cpi_file(const char *dir, const char *raw, 
                         double *bests[], int *bestlen);

extern void combine_cpi_files(const char *dir, double *bests[], int *bestlen);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __cpint_h */

