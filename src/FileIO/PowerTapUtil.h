/*
 * Copyright (c) 2008 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_PowerTapUtil_h
#define _GC_PowerTapUtil_h 1
#include "GoldenCheetah.h"

#include <time.h>

struct PowerTapUtil
{
    static bool is_Ver81(unsigned char *bufHeader);

    static bool is_ignore_record(unsigned char *buf, bool bVer81);

    static int is_time(unsigned char *buf, bool bVer81);
    static time_t unpack_time(unsigned char *buf, struct tm *time, bool bVer81);

    static int is_config(unsigned char *buf, bool bVer81);
    static int unpack_config(unsigned char *buf, unsigned *interval,
                                unsigned *last_interval, double *rec_int_secs,
                                unsigned *wheel_sz_mm, bool bVer81);

    static int is_data(unsigned char *buf, bool bVer81);
    static void unpack_data(unsigned char *buf, double rec_int_secs,
                            unsigned wheel_sz_mm, double *time_secs,
                            double *torque_Nm, double *mph, double *watts,
                            double *dist_m, unsigned *cad, unsigned *hr, bool bVer81);
};

#endif // _GC_PowerTapUtil_h

