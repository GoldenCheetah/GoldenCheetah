/* 
 * $Id: pt.h,v 1.9 2006/09/06 23:23:03 srhea Exp $
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

#ifndef __pt_h
#define __pt_h 1

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>

#define PT_DEBUG_NONE 0
#define PT_DEBUG_MAX 1
extern unsigned pt_debug_level;

extern int pt_find_device(char *result[], int capacity);
extern int pt_hwecho(const char *device);

#define PT_DONE 0
#define PT_NEED_READ 1

extern void pt_make_async(int fd);

struct pt_read_version_state {
    int state;
    int i;
    unsigned char buf[30];
};
extern int pt_read_version(struct pt_read_version_state *state, 
                           int fd, int hwecho);

struct pt_read_data_state {
    int state;
    int i;
    unsigned block;
    unsigned char header[6];
    unsigned char buf[256 * 6 + 1];
};
extern int pt_read_data(struct pt_read_data_state *state,
                        int fd, int hwecho,
                        void (*time_cb)(struct tm *, void *), 
                        void (*record_cb)(unsigned char *, void *),
                        void *user_data);

extern int pt_is_time(unsigned char *buf);
extern time_t pt_unpack_time(unsigned char *buf, struct tm *time);

extern int pt_is_config(unsigned char *buf);
extern int pt_unpack_config(unsigned char *buf, unsigned *interval, 
                            unsigned *last_interval, unsigned *rec_int, 
                            unsigned *wheel_sz_mm);

extern int pt_is_data(unsigned char *buf);
extern void pt_unpack_data(unsigned char *buf, int compat, unsigned rec_int, 
                           unsigned wheel_sz_mm, double *time_secs, 
                           double *torque_Nm, double *mph, double *watts,
                           double *dist_m, unsigned *cad, unsigned *hr);

void pt_write_data(FILE *out, unsigned char *buf);
void pt_pack_header(unsigned char *buf);
void pt_pack_time(unsigned char *buf, struct tm *time);
void pt_pack_config(unsigned char *buf, unsigned interval, 
                    unsigned rec_int, unsigned wheel_sz_mm);
void pt_pack_data(unsigned char *buf, unsigned wheel_sz_mm, double nm, 
                  double mph, double miles, unsigned cad, unsigned hr);

extern void pt_read_raw(FILE *in, int compat, void *context,
    void (*config_cb)(unsigned interval, unsigned rec_int, 
                      unsigned wheel_sz_mm, void *context),
    void (*time_cb)(struct tm *time, time_t since_epoch, void *context),
    void (*data_cb)(double secs, double nm, double mph, 
                    double watts, double miles, unsigned cad, 
                    unsigned hr, unsigned interval, void *context),
    void (*error_cb)(const char *msg, void *context));

extern void pt_read_dat(FILE *in, 
                        void (*record_cb)(double, double, double, int, 
                                          double, int, int, int, void*),
                        void *user_data);

#endif /* __pt_h */

