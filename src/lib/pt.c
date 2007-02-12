/* 
 * $Id: pt.c,v 1.9 2006/09/06 23:23:03 srhea Exp $
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
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "pt.h"

#define MAGIC_CONSTANT 147375.0
#define PI 3.14159265
#define TIME_UNIT_MIN 0.021

#define LBFIN_TO_NM 0.11298483
#define KM_TO_MI 0.62137119

#define BAD_LBFIN_TO_NM_1 0.112984
#define BAD_LBFIN_TO_NM_2 0.1129824
#define BAD_KM_TO_MI 0.62

unsigned pt_debug_level;

static unsigned char
check(unsigned value) 
{
    assert(value < 256);
    return (unsigned char) value;
}

int
pt_find_device(char *result[], int capacity)
{
    regex_t reg;
    DIR *dirp;
    struct dirent *dp;
    int count = 0;
    if (regcomp(&reg, 
                "^(cu\\.(usbserial-[0-9A-F]+|KeySerial[0-9])|ttyUSB[0-9]|ttyS[0-2])$", 
                REG_EXTENDED|REG_NOSUB)) {
        assert(0);
    }
    dirp = opendir("/dev");
    while ((count < capacity) && ((dp = readdir(dirp)) != NULL)) {
        if (regexec(&reg, dp->d_name, 0, NULL, 0) == 0) {
            result[count] = malloc(6 + strlen(dp->d_name));
            sprintf(result[count], "/dev/%s", dp->d_name);
            ++count;
        }
    }
    return count;
}

void
pt_make_async(int fd) 
{
    struct termios tty;
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        assert(0);
    }
    if (tcgetattr(fd, &tty) == -1) {
        perror("tcgetattr");
        assert(0);
    }
    tty.c_cflag &= ~CRTSCTS; /* no hardware flow control */
    tty.c_cflag &= ~(PARENB | PARODD); /* no parity */
    tty.c_cflag &= ~CSTOPB; /* 1 stop bit */
    tty.c_cflag &= ~CSIZE; /* clear size bits */
    tty.c_cflag |= CS8; /* 8 bits */
    tty.c_cflag |= CLOCAL | CREAD; /* ignore modem control lines */
    if (cfsetspeed(&tty, B9600) == -1) {
        perror("cfsetspeed");
        assert(0);
    }
    tty.c_iflag = IGNBRK; /* ignore BREAK condition on input */
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 1; /* all reads return at least one character */
    if (tcsetattr(fd, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        assert(0);
    }
}

static void
fprintb(FILE *file, unsigned char *buf, int cnt) 
{
    unsigned char *end = buf + cnt;
    while (buf < end)
        fprintf(file, "%02x", 0xff & *buf++);
}

int
pt_read_version(struct pt_read_version_state *state, int fd, int *hwecho) 
{
    char c = 0x56;
    int n;

    if (state->state == 0) {
        if (pt_debug_level >= PT_DEBUG_MAX)
            fprintf(stderr, "Writing 0x%x to device.\n", (unsigned) c);
        if ((n = write(fd, &c, 1)) < 1) {
            perror("write");
            exit(1);
        }
        state->state = 1;
        state->i = 0;
    }

    assert(state->state == 1);
    while (state->i < 29) {
        if (pt_debug_level >= PT_DEBUG_MAX)
            fprintf(stderr, "Need %d bytes.  Calling read on device.\n",
                    29 - state->i);
        n = read(fd, state->buf + state->i, sizeof(state->buf) - state->i);
        if (n <= 0) {
            if ((n < 0) && (errno == EAGAIN)) {
                if (pt_debug_level >= PT_DEBUG_MAX)
                    fprintf(stderr, "Need read.\n");
                return PT_NEED_READ;
            }
            perror("read");
            exit(1);
        }
        if (pt_debug_level >= PT_DEBUG_MAX) {
            fprintf(stderr, "Read %d bytes: ", n);
            fprintb(stderr, state->buf + state->i, n);
            fprintf(stderr, ".\n");
        }
        if ((state->i == 0) && (state->buf[0] == 0x56)) {
            if (pt_debug_level >= PT_DEBUG_MAX)
                fprintf(stderr, "Hardware echo detected.\n");
            *hwecho = 1;
            for (int j = 0; j < n - 1; ++j)
                state->buf[j] = state->buf[j+1];
            state->i = n - 1;
        }
        else {
            state->i += n;
        }
    }

    return PT_DONE;
}

int
pt_read_data(struct pt_read_data_state *state,
             int fd, int hwecho, 
             void (*time_cb)(struct tm *, void *), 
             void (*record_cb)(unsigned char *, void *),
             void *user_data) 
{
    char c = 0x44;
    int j, n;
    unsigned csum;
    struct tm time;

    if (state->state == 0) {
        if (pt_debug_level >= PT_DEBUG_MAX)
            fprintf(stderr, "Writing 0x%x to device.\n", (unsigned) c);
        if ((n = write(fd, &c, 1)) < 1) {
            perror("write");
            exit(1);
        }
        state->block = 1;
        state->i = 0;
        state->state = 1;
    }

    if (state->state == 1) {
        if (hwecho) {
            if (pt_debug_level >= PT_DEBUG_MAX)
                fprintf(stderr, "Calling read on device.\n");
            n = read(fd, &c, 1);
            if (n <= 0) {
                if ((n < 0) && (errno == EAGAIN)) {
                    if (pt_debug_level >= PT_DEBUG_MAX)
                        fprintf(stderr, "Need read.\n");
                    return PT_NEED_READ;
                }
                perror("read");
                exit(1);
            }
            if (pt_debug_level >= PT_DEBUG_MAX)
                fprintf(stderr, "Read %d bytes: %02x.\n", n, 0xff & c);
            assert(n == 1);
        }
        state->state = 2;
    }

    if (state->state == 2) {
        while (state->i < (int) sizeof(state->header)) {
            if (pt_debug_level >= PT_DEBUG_MAX)
                fprintf(stderr, "Calling read on device.\n");
            n = read(fd, state->header + state->i, 
                     sizeof(state->header) - state->i);
            if (n <= 0) {
                if ((n < 0) && (errno == EAGAIN)) {
                    if (pt_debug_level >= PT_DEBUG_MAX)
                        fprintf(stderr, "Need read.\n");
                    return PT_NEED_READ;
                }
                perror("read");
                exit(1);
            }
            if (pt_debug_level >= PT_DEBUG_MAX) {
                fprintf(stderr, "Read %d bytes: ", n);
                fprintb(stderr, state->buf + state->i, n);
                fprintf(stderr, ".\n");
            }
            state->i += n;
        }
        state->state = 3;
        state->i = 0;
    }

    while (1) {
        if (state->state == 3) {
            while (state->i < (int) sizeof(state->buf)) {
                if (pt_debug_level >= PT_DEBUG_MAX)
                    fprintf(stderr, "Calling read on device.\n");
                n = read(fd, state->buf + state->i, 
                         sizeof(state->buf) - state->i);
                if (n <= 0) {
                    if ((n < 0) && (errno == EAGAIN)) {
                        if (pt_debug_level >= PT_DEBUG_MAX)
                            fprintf(stderr, "Need read.\n");
                        return PT_NEED_READ;
                    }
                    perror("read");
                    exit(1);
                }
                if (pt_debug_level >= PT_DEBUG_MAX) {
                    fprintf(stderr, "Read %d bytes: ", n);
                    fprintb(stderr, state->buf + state->i, n);
                    fprintf(stderr, ".\n");
                }
                state->i += n;
                /* TODO: why is this next if statement here? */
                if ((state->i == 2) && (state->buf[0] == 0x0d) 
                    && (state->buf[1] == 0x0a)) {
                    return PT_DONE;
                }
            }
            if (state->block == 1) {
                n = 0; 
                if (pt_is_config(state->buf + n))
                    n += 6;
                if (!pt_is_time(state->buf + n) 
                    || (pt_unpack_time(state->buf + n, &time) == -1))
                    time_cb(NULL, user_data);
                else 
                    time_cb(&time, user_data);
                record_cb(state->header, user_data);
            }
            csum = 0;
            for (j = 0; j < state->i - 1; ++j) 
                csum += state->buf[j];
            if ((csum % 256) != state->buf[state->i-1]) {
                fprintf(stderr, "\nbad checksum on block %d: %d vs %d",
                        state->block, state->buf[state->i-1], csum);
            }
            for (j = 0; j < state->i - 1; j += 6) {
                if (state->buf[j])
                    record_cb(state->buf + j, user_data);
                else
                    break;
            }
            c = 0x71;
            if (pt_debug_level >= PT_DEBUG_MAX)
                fprintf(stderr, "Writing 0x%x to device.\n", (unsigned) c);
            n = write(fd, &c, 1);
            if (n < 1) {
                perror("write");
                exit(1);
            }
            ++(state->block);
            state->i = 0;
            state->state = 4;
        }

        assert(state->state == 4);
        if (hwecho) {
            if (pt_debug_level >= PT_DEBUG_MAX)
                fprintf(stderr, "Calling read on device.\n");
            n = read(fd, &c, 1);
            if (n <= 0) {
                if ((n < 0) && (errno == EAGAIN)) {
                    if (pt_debug_level >= PT_DEBUG_MAX)
                        fprintf(stderr, "Need read.\n");
                    return PT_NEED_READ;
                }
                perror("read");
                exit(1);
            }
            if (pt_debug_level >= PT_DEBUG_MAX) {
                fprintf(stderr, "Read %d bytes: ", n);
                fprintb(stderr, state->buf + state->i, n);
                fprintf(stderr, ".\n");
            }
            assert(n == 1);
        }
        state->state = 3;
    }

    return PT_DONE;
}

int
pt_is_time(unsigned char *buf)
{
    return buf[0] == 0x60;
}

time_t 
pt_unpack_time(unsigned char *buf, struct tm *time) 
{
    memset(time, 0, sizeof(*time));
    time->tm_year = 2000 + buf[1] - 1900;
    time->tm_mon = buf[2] - 1;
    time->tm_mday = buf[3] & 0x1f;
    time->tm_hour = buf[4] & 0x1f;
    time->tm_min = buf[5] & 0x3f;
    time->tm_sec = ((buf[3] >> 5) << 3) | (buf[4] >> 5);
    time->tm_isdst = -1;
    return mktime(time);
}

int
pt_is_config(unsigned char *buf)
{
    return buf[0] == 0x40;
}

int 
pt_unpack_config(unsigned char *buf, unsigned *interval, 
                 unsigned *last_interval, unsigned *rec_int, 
                 unsigned *wheel_sz_mm)
{
    *wheel_sz_mm = (buf[1] << 8) | buf[2];
    /* Data from device wraps interval after 9... */
    if (buf[3] != *last_interval) {
        *last_interval = buf[3];
        ++*interval;
    }
    switch (buf[4]) {
        case 0x0:  *rec_int = 1;  break;
        case 0x1:  *rec_int = 2;  break;
        case 0x3:  *rec_int = 5;  break;
        case 0x7:  *rec_int = 10; break;
        case 0x17: *rec_int = 30; break;
        default:
            return -1;
    }
    return 0;
}

int
pt_is_data(unsigned char *buf) 
{
    return (buf[0] & 0x80) == 0x80;
}

static double 
my_round(double x)
{
    int i = (int) x;
    double z = x - i;
    /* For some unknown reason, the PowerTap software rounds 196.5 down... */
    if ((z > 0.5) || ((z == 0.5) && (i != 196)))
        ++i;
    return i;
}

void
pt_unpack_data(unsigned char *buf, int compat, unsigned rec_int, 
               unsigned wheel_sz_mm, double *time_secs, double *torque_Nm, 
               double *mph, double *watts, double *dist_m, unsigned *cad, 
               unsigned *hr)
{
    double kph10;
    unsigned speed;
    unsigned torque_inlbs;
    double rotations;
    double radians;
    double joules;

    *time_secs += rec_int * TIME_UNIT_MIN * 60.0;
    torque_inlbs = ((buf[1] & 0xf0) << 4) | buf[2];
    if (torque_inlbs == 0xfff)
        torque_inlbs = 0;
    speed = ((buf[1] & 0x0f) << 8) | buf[3];
    if ((speed < 100) || (speed == 0xfff)) {
        if ((speed != 0) && (speed < 1000)) {
            fprintf(stderr, "possible error: speed=%.1f; ignoring it\n", 
                    MAGIC_CONSTANT / speed / 10.0);
        }
        *mph = -1.0;
        *watts = -1.0;
    }
    else {
        if (compat)
            *torque_Nm = torque_inlbs * BAD_LBFIN_TO_NM_2;
        else
            *torque_Nm = torque_inlbs * LBFIN_TO_NM;
        kph10 = MAGIC_CONSTANT / speed;
        if (compat)
            *mph = my_round(kph10) / 10.0 * BAD_KM_TO_MI;
        else
            *mph = kph10 / 10.0 * KM_TO_MI;
        rotations = rec_int * TIME_UNIT_MIN * 100000.0 * kph10 
            / wheel_sz_mm / 60.0;
        radians = rotations * 2.0 * PI;
        joules = *torque_Nm * radians;
        *watts = joules / (rec_int * TIME_UNIT_MIN * 60);
        if (compat)
            *watts = my_round(*watts);
        else 
            *watts = round(*watts);
    }
    if (compat)
        *torque_Nm = torque_inlbs * BAD_LBFIN_TO_NM_1;
    *dist_m += (buf[0] & 0x7f) * wheel_sz_mm / 1000.0;
    *cad = buf[4];
    if (*cad == 0xff)
        *cad = 0;
    *hr = buf[5];
    if (*hr == 0xff)
        *hr = 0;
}

void
pt_write_data(FILE *out, unsigned char *buf)
{
    int i;
    for (i = 0; i < 5; ++i)
        fprintf(out, "%02x ", buf[i]);
    fprintf(out, "%02x\n", buf[i]);
}

void
pt_pack_header(unsigned char *buf)
{
    unsigned char src[] = { 0x57, 0x56, 0x55, 0x64, 0x02, 0x15 };
    memcpy(buf, src, 6);
}

void 
pt_pack_time(unsigned char *buf, struct tm *time) 
{
    buf[0] = 0x60;
    buf[1] = check(time->tm_year + 1900 - 2000);
    buf[2] = check(time->tm_mon + 1);
    buf[3] = check(time->tm_mday) | check((time->tm_sec >> 3) << 5);
    buf[4] = check(time->tm_hour) | check((time->tm_sec & 0x7) << 5);
    buf[5] = check(time->tm_min);
}

void 
pt_pack_config(unsigned char *buf, unsigned interval, 
               unsigned rec_int, unsigned wheel_sz_mm)
{
    buf[0] = 0x40;
    buf[1] = check(wheel_sz_mm >> 8);
    buf[2] = wheel_sz_mm & 0xff;
    buf[3] = check(interval % 9);
    switch (rec_int) {
        case 1:  buf[4] = 0x0; break;
        case 2:  buf[4] = 0x1; break;
        case 5:  buf[4] = 0x3; break;
        case 10: buf[4] = 0x7; break;
        case 30: buf[4] = 0x17; break;
        default: assert(0);
    }
    buf[5] = 0x0;
}

void
pt_pack_data(unsigned char *buf, unsigned wheel_sz_mm, double nm, 
             double mph, double miles, unsigned cad, unsigned hr) 
{
    double rotations = miles / BAD_KM_TO_MI * 1000.00 * 1000.0 / wheel_sz_mm;
    unsigned inlbs = round(nm / BAD_LBFIN_TO_NM_2);
    double kph10 = mph * 10.0 / BAD_KM_TO_MI;
    unsigned speed;
    if (mph == -1.0)
        speed = 0xfff;
    else
        speed = round(MAGIC_CONSTANT / kph10); 
    buf[0] = 0x80 | check(round(rotations));
    buf[1] = ((inlbs & 0xf00) >> 4) | ((speed & 0xf00) >> 8);
    buf[2] = inlbs & 0xff;
    buf[3] = speed & 0xff;
    buf[4] = check(cad);
    buf[5] = check(hr);
}


void 
pt_read_raw(FILE *in, int compat, void *context,
            void (*config_cb)(unsigned interval, unsigned rec_int, 
                              unsigned wheel_sz_mm, void *context),
            void (*time_cb)(struct tm *time, time_t since_epoch, void *context),
            void (*data_cb)(double secs, double nm, double mph, 
                            double watts, double miles, unsigned cad, 
                            unsigned hr, unsigned interval, void *context),
            void (*error_cb)(const char *msg, void *context))
{
    unsigned interval = 0;
    unsigned last_interval = 0;
    unsigned wheel_sz_mm = 0;
    unsigned rec_int = 0;
    int i, n, row = 0;
    unsigned char buf[6];
    unsigned sbuf[6];
    double meters = 0.0;
    double secs = 0.0, start_secs = 0.0;
    double miles;
    double mph;
    double nm;
    double watts;
    unsigned cad;
    unsigned hr;
    struct tm time;
    time_t since_epoch;
    char ebuf[256];

    while ((n = fscanf(in, "%x %x %x %x %x %x\n", 
            sbuf, sbuf+1, sbuf+2, sbuf+3, sbuf+4, sbuf+5)) == 6) {
        ++row;
        for (i = 0; i < 6; ++i) {
            if (sbuf[i] > 0xff) { n = 1; break; }
            buf[i] = sbuf[i];
        }
        if (row == 1) {
            /* Serial number? */
        }
        else if (pt_is_config(buf)) {
            if (pt_unpack_config(buf, &interval, &last_interval, 
                                 &rec_int, &wheel_sz_mm) < 0) {
                sprintf(ebuf, "Couldn't unpack config record.");
                if (error_cb) error_cb(ebuf, context);
                return;
            }
            if (config_cb) config_cb(interval, rec_int, wheel_sz_mm, context);
        }
        else if (pt_is_time(buf)) {
            since_epoch = pt_unpack_time(buf, &time);
            if (start_secs == 0.0)
                start_secs = since_epoch;
            else if (since_epoch - start_secs > secs)
                    secs = since_epoch - start_secs;
            else 
                return; /* ignore it; don't let time go backwards */
            if (time_cb) time_cb(&time, since_epoch, context);
        }
        else if (pt_is_data(buf)) {
            if (wheel_sz_mm == 0) {
                sprintf(ebuf, "Read data row before wheel size set.");
                if (error_cb) error_cb(ebuf, context);
                return;
            }
            pt_unpack_data(buf, compat, rec_int, wheel_sz_mm, &secs, 
                           &nm, &mph, &watts, &meters, &cad, &hr);
            if (compat)
                miles = round(meters) / 1000.0 * BAD_KM_TO_MI;
            else 
                miles = meters / 1000.0 * KM_TO_MI;
            if (data_cb) 
                data_cb(secs, nm, mph, watts, miles, cad, 
                        hr, interval, context);
        }
        else { 
            sprintf(ebuf, "Unknown record type 0x%x on row %d.", buf[0], row);
            if (error_cb) error_cb(ebuf, context);
            return;
        }
    }
    if (n != -1) {
        sprintf(ebuf, "Parse error on row %d.", row);
        if (error_cb) error_cb(ebuf, context);
        return;
    }
}

#define NMATCH 9
void
pt_read_dat(FILE *in, void (*record_cb)(double, double, double, int, 
                                        double, int, int, int, void*),
            void *user_data)
{
    regex_t reg_com, reg_dat;
    regmatch_t pmatch[NMATCH];
    char line[256];
    double min, nm, mph, miles;
    int watts, cad, hr, intv;
    int i, len;

    if (regcomp(&reg_com, "^#", REG_EXTENDED | REG_NOSUB))
        assert(0);
    if (regcomp(&reg_dat, "^([0-9]+\\.[0-9]+) +([0-9]+\\.[0-9]+) +"
                "([0-9]+\\.[0-9]+|NaN) +([0-9]+|NaN) +([0-9]+\\.[0-9]+) +"
                "([0-9]+) +([0-9]+|NaN) +([0-9]+)$", REG_EXTENDED))
        assert(0);

    while (fgets(line, sizeof(line), in)) {
        len = strlen(line);
        if (!line[len-1] == '\n') 
            assert(0);
        line[len-1] = '\0';
        if (regexec(&reg_com, line, 0, NULL, 0) == 0) {
            /* do nothing */
        }
        else if (regexec(&reg_dat, line, NMATCH, pmatch, 0) == 0) {
            for (i = 0; i < NMATCH; ++i) 
                line[pmatch[i].rm_eo] = '\0';
            if (sscanf(line + pmatch[1].rm_so, "%lf", &min) != 1)
                assert(0);
            if (sscanf(line + pmatch[2].rm_so, "%lf", &nm) != 1)
                assert(0);
            if (sscanf(line + pmatch[3].rm_so, "%lf", &mph) != 1)
                mph = -1.0;
            if (sscanf(line + pmatch[4].rm_so, "%d", &watts) != 1)
                watts = -1;
            if (sscanf(line + pmatch[5].rm_so, "%lf", &miles) != 1)
                assert(0);
            if (sscanf(line + pmatch[6].rm_so, "%d", &cad) != 1)
                assert(0);
            if (sscanf(line + pmatch[7].rm_so, "%d", &hr) != 1)
                hr = -1;
            if (sscanf(line + pmatch[8].rm_so, "%d", &intv) != 1)
                assert(0);
            record_cb(min, nm, mph, watts, miles, cad, hr, intv, user_data);
        }
        else {
            fprintf(stderr, "Bad line: \"%s\"\n", line);
            exit(1);
        }
    }
}

