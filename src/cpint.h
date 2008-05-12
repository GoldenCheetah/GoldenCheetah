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

#ifndef __cpint_h
#define __cpint_h 1

#include <QtCore>

struct cpi_file_info {
    QString file, inname, outname;
};

extern void cpi_files_to_update(const QDir &dir, QList<cpi_file_info> &result);

extern void update_cpi_file(const cpi_file_info *info, 
                            int (*cancel_cb)(void *user_data),
                            void *user_data);

extern int read_cpi_file(const QDir &dir, const QFileInfo &raw,
                         QVector<double> &bests);

extern void combine_cpi_files(const QDir &dir, QVector<double> &bests);

#endif /* __cpint_h */

