/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "SrdRideFile.h"

// System includes
#include <string.h>
#include <stdio.h>
#if Q_CC_MSVC
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef Q_OS_WIN
// '_open': This function or variable may be unsafe.
// 'strcpy': This function or variable may be unsafe.
// 'strerror': This function or variable may be unsafe.
#pragma warning(disable:4996)
#endif

static int srdFileReaderRegistered =
    RideFileFactory::instance().registerReader(
        "srd", "Polar SRD files", new SrdFileReader());

RideFile *SrdFileReader::openRideFile(QFile &file, QStringList &errorStrings, QList<RideFile*>*) const
{
    workout_t *      w;
    S710_Filter      filter = S710_FILTER_OFF;

    // use s710 lib to read in file and construct its workout_t structure
    w = read_workout((char*)QFileInfo(file).absoluteFilePath().toLatin1().constData(),filter,S710_HRM_AUTO);

    if (w) {
        // Success!

        // ok, so lets build up a new RideFile based upon the workout returned
        RideFile *result = new RideFile;

        //
        // first class variables
        //
        result->setRecIntSecs(w->recording_interval);
        result->setStartTime(QDateTime(QDate(w->date.tm_year+1900, w->date.tm_mon+1, w->date.tm_mday),
                                       QTime(w->date.tm_hour, w->date.tm_min, w->date.tm_sec, 0))); // (tm) S710_Date w->date

        switch (w->type) {
        case S710_HRM_S610:  result->setDeviceType("Polar S610"); break;
        case S710_HRM_S710:  result->setDeviceType("Polar S710"); break;
        case S710_HRM_S810:  result->setDeviceType("Polar S810"); break;
        case S710_HRM_S625X: result->setDeviceType("Polar S625X"); break;
        case S710_HRM_AUTO:
        case S710_HRM_UNKNOWN:
        default:
            result->setDeviceType("Polar HRM");
            break;
        }
        result->setFileFormat("Polar SRD files (srd)");

        //
        // metadata variables
        //
        result->setTag("Workout Code", w->exercise_label);
        // Not available in current file format
        // result->setTag("Device Info", );
        // result->setTag("Sport", );
        // result->setTag("Objective", );
        // result->setTag("Weight", );

        //
        // dataPoints
        //
        double time = 0;
        for (int i = 0; i < w->samples; i++ ) {
            double cad=0, hr=0, km=0, kph=0, nm=0, watts=0, alt=0, lon=0, lat=0, wind=0;

            // get values
            hr = w->hr_data[i];
            if (S710_HAS_SPEED(w->mode)) kph = w->speed_data[i] * 10.0 / 160.0;
            if (S710_HAS_CADENCE(w->mode)) cad = w->cad_data[i];
            if (S710_HAS_ALTITUDE(w->mode)) alt = w->alt_data[i];
            if (S710_HAS_POWER(w->mode)) watts = w->power_data[i].power; // note: pedal_index and lr balance not extracted
            km = w->dist_data[i];

            // add to ride
            result->appendPoint(time, cad, hr, km, kph, nm, watts, alt, lon, lat, wind, 0.0,
                                RideFile::NA, RideFile::NA,
                                0.0, 0.0, 0.0, 0.0,
                                0.0, 0.0,
                                0.0, 0.0, 0.0, 0.0,
                                0.0, 0.0, 0.0, 0.0,
                                0.0, 0.0,
                                0.0, 0.0, 0.0, 0.0,
                                0);

            // keep a track of time
            time += w->recording_interval;
        }

        //
        // intervals last so we can check against the data points
        //
        for (int i=0; i<w->laps; i++) {

            RideFileInterval add;

            // name is ok, but start stop need to be in time rather than datapoint no.
            add.name = QString("Interval #%1").arg(i+1);
            add.start = w->lap_data[i].split.seconds +
                               (60 * (w->lap_data[i].split.minutes)) +
                               (3600 * w->lap_data[i].split.hours);

            add.stop = w->lap_data[i].cumulative.seconds +
                             (60 * (w->lap_data[i].cumulative.minutes)) +
                             (3600 * w->lap_data[i].cumulative.hours);

            result->addInterval(RideFileInterval::DEVICE, add.start, add.stop, add.name);
        }

        // free up s710 resources
        free_workout(w);

        return result;

    } else {
        // Failed to read
        errorStrings << "Could not read file";
    }
    return NULL;
}


//===================================================================================
//
//                            ********************
//
//       FROM HERE TO BOTTOM OF SOURCE FILE ARE THE SRD WORKOUT ROUTINES
//   INCLUDED FROM THE "S710" LINUX PROJECT see: http://code.google.com/p/s710
//
//     To avoid dependency on the s710 source (which has other dependencies)
//    the code has been incorporated into this source file as an extract and
//   converted to static functions within this source file (the original code
//     is part of a library). The last modification date for this source was
//             May 9th 2007, it is now largely a dormant project).
//
//          The code has been /slightly/ modified in porting from C to C++
//
//                            ********************
//
// The original source is published under the GPL v2 License
//
//       Original Author:
//       Dave Bailey <dave <at> daveb <dot> net>
//
//       Numerous bugfixes, protocol/file format information and feature enhancements
//       have been contributed by (in alphabetical order by last name):
//       Bjorn Andersson    <bjorn <at> iki <dot> fi>
//       Jani Averbach      <jaa <at> jaa <dot> iki <dot> fi>
//       Markus Braun       <Markus <dot> Braun <at> krawel <dot> de>
//       Robert Estes       <estes <at> transmeta <dot> com>
//       Michael Karbach    <karbach <at> physik <dot> uni-wuppertal <dot> de>
//       Stefan Kleditzsch  <Stefan.Kleditzsch <at> iwr <dot> uni-heidelberg <dot> de>
//       Max Matveev        <makc <at> sgi <dot> com>
//       Berend Ozceri      <berend <at> alumni <dot> cmu <dot> edu>
//       Matti Tahvonen     <matti <at> tahvonen <dot> com>
//       Frank Vercruesse   <frank <at> vercruesse <dot> de>
//       Aldy Hernandez     <aldyh <at> redhat <dot> com>
//
//===================================================================================

// modified from #defined straight string constants
static char S710_SPEED_KPH[] = "kph";
static char S710_SPEED_MPH[] = "mph";
static char S710_DISTANCE_KM[] = "km";
static char S710_DISTANCE_MI[] = "mi";
static char S710_ALTITUDE_M[] = "m";
static char S710_ALTITUDE_FT[] = "ft";
static char S710_TEMPERATURE_C[] = "C";
static char S710_TEMPERATURE_F[] = "F";

static void diff_s710_time ( S710_Time *t1, S710_Time *t2, S710_Time *diff )
{
  int t_t1;
  int t_t2;
  int t_diff;
  int negative = 0;

  /* first compute t1 and t2 in tenths of a second */

  t_t1 = ((t1->hours * 60 + t1->minutes) * 60 + t1->seconds) * 10 + t1->tenths;
  t_t2 = ((t2->hours * 60 + t2->minutes) * 60 + t2->seconds) * 10 + t2->tenths;

  t_diff = t_t2 - t_t1;
  if ( t_diff < 0 ) {
    negative = 1;
    t_diff = -t_diff;
  }

  diff->tenths = t_diff % 10;
  t_diff = (t_diff - diff->tenths) / 10;

  /* now t_diff is in seconds */

  diff->seconds = t_diff % 60;
  t_diff = (t_diff - diff->seconds) / 60;

  /* now t_diff is in minutes */

  diff->minutes = t_diff % 60;
  t_diff = (t_diff - diff->minutes) / 60;

  /* the rest is the hours */

  diff->hours = t_diff;

  /* if we got a negative time, switch the sign of everything. */

  if ( negative ) {
    diff->hours   = -diff->hours;
    diff->minutes = -diff->minutes;
    diff->seconds = -diff->seconds;
    diff->tenths  = -diff->tenths;
  }
}

//----------------------------------------------------------------------
//
// SRD FILE READING FUNCTIONS
//
//----------------------------------------------------------------------
static void read_preamble ( workout_t * w, unsigned char * buf )
{
  /* number of bytes in the buffer (including these two) */

  w->bytes = buf[0] + (buf[1]<<8);

  w->exercise_number = buf[2];
  if ( w->exercise_number > 0 && w->exercise_number <= 5 ) {
    extract_label(&buf[3],&w->exercise_label,7);
  } else {
    strcpy(w->exercise_label,"<empty>");
  }
}


static void read_date ( workout_t * w, unsigned char * buf )
{
  /* date of workout */

  w->date.tm_sec   = BCD(buf[0]);
  w->date.tm_min   = BCD(buf[1]);
  w->date.tm_hour  = BCD(buf[2] & 0x7f);

  /* PATCH for AM/PM mode detection from Berend Ozceri */

  w->ampm = S710_AM_PM_MODE_UNSET;
  if ( buf[3] & 0x80 ) w->ampm |= S710_AM_PM_MODE_SET;
  if ( buf[2] & 0x80 ) w->ampm |= S710_AM_PM_MODE_PM;

  w->date.tm_hour += (buf[3] & 0x80) ?                      /* am/pm mode?   */
    ((buf[2] & 0x80) ? ((w->date.tm_hour < 12) ? 12 : 0) :  /* yes, pm set   */
     ((w->date.tm_hour >= 12) ? -12 : 0)) :                 /* yes, pm unset */
    0;                                                      /* no            */

  w->date.tm_mon   = LNIB(buf[5]) - 1;
  w->date.tm_mday  = BCD(buf[3] & 0x7f);
  w->date.tm_year  = 100 + BCD(buf[4]);
  w->date.tm_isdst = -1; /* Daylight savings time not known yet? */
  w->unixtime = mktime(&w->date);
}


static void read_duration ( workout_t * w, unsigned char * buf )
{
  w->duration.tenths  = UNIB(buf[0]);
  w->duration.seconds = BCD(buf[1]);
  w->duration.minutes = BCD(buf[2]);
  w->duration.hours   = BCD(buf[3]);
}


static void read_units ( workout_t * w, unsigned char b )
{
  if ( b & 0x02 ) { /* english */

    w->units.system      = S710_UNITS_ENGLISH;
    w->units.altitude    = S710_ALTITUDE_FT;
    w->units.speed       = S710_SPEED_MPH;
    w->units.distance    = S710_DISTANCE_MI;
    w->units.temperature = S710_TEMPERATURE_F;

  } else {         /* metric */

    w->units.system      = S710_UNITS_METRIC;
    w->units.altitude    = S710_ALTITUDE_M;
    w->units.speed       = S710_SPEED_KPH;
    w->units.distance    = S710_DISTANCE_KM;
    w->units.temperature = S710_TEMPERATURE_C;
  }
}

static int get_recording_interval ( unsigned char b )
{
  int ri = 0;

  switch ( b & 0x0f ) {
  case 0:  ri = 5;  break;
  case 1:  ri = 15; break;
  case 2:  ri = 60; break;
  default:          break;
  }

  return ri;
}


static void read_recording_interval ( workout_t * w, unsigned char * buf )
{
  w->recording_interval = get_recording_interval(buf[0]);
}


static void read_hr_limits ( workout_t * w, unsigned char * buf )
{
  /* HR limits */

  w->hr_limit[0].lower     = buf[0];
  w->hr_limit[0].upper     = buf[1];
  w->hr_limit[1].lower     = buf[2];
  w->hr_limit[1].upper     = buf[3];
  w->hr_limit[2].lower     = buf[4];
  w->hr_limit[2].upper     = buf[5];

  /* time below, within, above hr limits 1 */

  w->hr_zone[0][0].tenths  = 0;
  w->hr_zone[0][0].seconds = BCD(buf[9]);   /* Below zone 1, seconds */
  w->hr_zone[0][0].minutes = BCD(buf[10]);  /* Below zone 1, minutes */
  w->hr_zone[0][0].hours   = BCD(buf[11]);  /* Below zone 1, hours   */
  w->hr_zone[0][1].seconds = BCD(buf[12]);  /* Within zone 1, seconds */
  w->hr_zone[0][1].minutes = BCD(buf[13]);  /* Within zone 1, minutes */
  w->hr_zone[0][1].hours   = BCD(buf[14]);  /* Within zone 1, hours   */
  w->hr_zone[0][2].seconds = BCD(buf[15]);  /* Above zone 1, seconds */
  w->hr_zone[0][2].minutes = BCD(buf[16]);  /* Above zone 1, minutes */
  w->hr_zone[0][2].hours   = BCD(buf[17]);  /* Above zone 1, hours   */

  /* time below, within, above hr limits 2 */

  w->hr_zone[1][0].tenths  = 0;
  w->hr_zone[1][0].seconds = BCD(buf[18]);  /* Below zone 2, seconds */
  w->hr_zone[1][0].minutes = BCD(buf[19]);  /* Below zone 2, minutes */
  w->hr_zone[1][0].hours   = BCD(buf[20]);  /* Below zone 2, hours   */
  w->hr_zone[1][1].seconds = BCD(buf[21]);  /* Within zone 2, seconds */
  w->hr_zone[1][1].minutes = BCD(buf[22]);  /* Within zone 2, minutes */
  w->hr_zone[1][1].hours   = BCD(buf[23]);  /* Within zone 2, hours   */
  w->hr_zone[1][2].seconds = BCD(buf[24]);  /* Above zone 2, seconds */
  w->hr_zone[1][2].minutes = BCD(buf[25]);  /* Above zone 2, minutes */
  w->hr_zone[1][2].hours   = BCD(buf[26]);  /* Above zone 2, hours   */

  /* time below, within, above hr limits 3 */

  w->hr_zone[2][0].tenths  = 0;
  w->hr_zone[2][0].seconds = BCD(buf[27]);  /* Below zone 3, seconds */
  w->hr_zone[2][0].minutes = BCD(buf[28]);  /* Below zone 3, minutes */
  w->hr_zone[2][0].hours   = BCD(buf[29]);  /* Below zone 3, hours   */
  w->hr_zone[2][1].seconds = BCD(buf[30]);  /* Within zone 3, seconds */
  w->hr_zone[2][1].minutes = BCD(buf[31]);  /* Within zone 3, minutes */
  w->hr_zone[2][1].hours   = BCD(buf[32]);  /* Within zone 3, hours   */
  w->hr_zone[2][2].seconds = BCD(buf[33]);  /* Above zone 3, seconds */
  w->hr_zone[2][2].minutes = BCD(buf[34]);  /* Above zone 3, minutes */
  w->hr_zone[2][2].hours   = BCD(buf[35]);  /* Above zone 3, hours   */
}


static void read_bestlap_split ( workout_t * w, unsigned char * buf )
{
  w->bestlap_split.tenths  = UNIB(buf[0]);
  w->bestlap_split.seconds = BCD(buf[1]);
  w->bestlap_split.minutes = BCD(buf[2]);
  w->bestlap_split.hours   = BCD(buf[3]);
}


static void read_energy ( workout_t * w, unsigned char * buf )
{
  w->energy       = BCD(buf[0])/10 + 10 * BCD(buf[1]) + 1000 * BCD(buf[2]);
  w->total_energy = BCD(buf[3]) + 100 * BCD(buf[4]) + 10000 * BCD(buf[5]);
}


static void read_cumulative_exercise ( workout_t * w, unsigned char * buf )
{
  w->cumulative_exercise.tenths  = 0;
  w->cumulative_exercise.seconds = 0;
  w->cumulative_exercise.minutes = BCD(buf[2]);
  w->cumulative_exercise.hours   = BCD(buf[0]) + BCD(buf[1]) * 100;
}


static void read_ride_info( workout_t * w, unsigned char * buf )
{
  w->cumulative_ride.tenths      = 0;
  w->cumulative_ride.seconds     = 0;
  w->cumulative_ride.minutes     = BCD(buf[2]);
  w->cumulative_ride.hours       = BCD(buf[0]) + BCD(buf[1]) * 100;

  w->odometer = 10000 * BCD(buf[5]) + 100 * BCD(buf[4]) + BCD(buf[3]);

  /* exercise distance */

  w->exercise_distance = buf[6] + (buf[7]<<8);

  /* avg and max speed */

  w->avg_speed = buf[8] | ((buf[9] & 0xfUL) << 8);
  w->max_speed = (buf[10] << 4) | (buf[9] >> 4);

  /* avg, max cadence */

  w->avg_cad  = buf[11];
  w->max_cad  = buf[12];

  /* min, avg, max temperature */

  if ( w->units.system == S710_UNITS_ENGLISH ) {

    /* English units */

    w->min_temp = buf[19];
    w->avg_temp = buf[20];
    w->max_temp = buf[21];

  } else {

    /* Metric units */

    w->min_temp = buf[19] & 0x7f;
    w->min_temp = ( buf[19] & 0x80 ) ? w->min_temp : - w->min_temp;

    w->avg_temp = buf[20] & 0x7f;
    w->avg_temp = ( buf[20] & 0x80 ) ? w->avg_temp : - w->avg_temp;

    w->max_temp = buf[21] & 0x7f;
    w->max_temp = ( buf[21] & 0x80 ) ? w->max_temp : - w->max_temp;
  }

  /* altitude, ascent */

  w->min_alt  = buf[13] + ((buf[14] & 0x7f)<<8);
  w->min_alt  = ( buf[14] & 0x80 ) ? w->min_alt : - w->min_alt;

  w->avg_alt  = buf[15] + ((buf[16] & 0x7f)<<8);
  w->avg_alt  = ( buf[16] & 0x80 ) ? w->avg_alt : - w->avg_alt;

  w->max_alt  = buf[17] + ((buf[18] & 0x7f)<<8);
  w->max_alt  = ( buf[18] & 0x80 ) ? w->max_alt : - w->max_alt;

  w->ascent   = (buf[23] << 8) + buf[22];

  /* avg, max power data */

  w->avg_power.power       = buf[24] + (LNIB(buf[25]) << 8);
  w->max_power.power       = UNIB(buf[25]) + (buf[26] << 4);
  w->avg_power.pedal_index = buf[27];
  w->max_power.pedal_index = buf[28];
  w->avg_power.lr_balance  = buf[29];
  /* there is no max_power LR balance */
  w->max_power.lr_balance  = 0;
}


/* Extract the lap data. */

static void read_laps ( workout_t * w, unsigned char * buf )
{
  S710_Distance  prev_lap_dist;
  S710_Altitude  prev_lap_ascent;
  int            offset;
  int            lap_size;
  int            hdr_size;
  lap_data_t *   l;
  int            i;

  prev_lap_ascent = 0;
  prev_lap_dist   = 0;
  lap_size        = bytes_per_lap(w->type,w->mode,w->interval_mode);
  hdr_size        = header_size(w);
  w->lap_data     = (lap_data_t*)calloc(w->laps,sizeof(lap_data_t));

  for ( i = 0; i < w->laps; i++ ) {

    /* position to the start of the lap */

    offset = hdr_size + i * lap_size;
    l = &w->lap_data[i];

    /* timestamp (split) */

    l->cumulative.hours   = buf[offset+2];
    l->cumulative.minutes = buf[offset+1] & 0x3f;
    l->cumulative.seconds = buf[offset] & 0x3f;
    l->cumulative.tenths  = ((buf[offset+1] & 0xc0)>>4) |
      ((buf[offset] & 0xc0)>>6);

    if ( i == 0 )
      memcpy(&l->split,&l->cumulative,sizeof(S710_Time));
    else
      diff_s710_time(&w->lap_data[i-1].cumulative,&l->cumulative,&l->split);

    /* heart rate data */

    l->lap_hr = buf[offset+3];
    l->avg_hr = buf[offset+4];
    l->max_hr = buf[offset+5];
    offset += 6;

    /* altitude data */

    if ( S710_HAS_ALTITUDE(w->mode) ) {
      l->alt = buf[offset] + (buf[offset+1]<<8) - 512;
      /* This ascent data is cumulative from start of the exercise */
      l->cumul_ascent = buf[offset+2] + (buf[offset+3]<<8);
      l->ascent = l->cumul_ascent - prev_lap_ascent;
      prev_lap_ascent = l->cumul_ascent;

      if ( w->units.system == S710_UNITS_ENGLISH ) {  /* English units */
	l->temp = buf[offset+4] + 14;
	l->alt *= 5;
      } else {
	l->temp = buf[offset+4] - 10;
      }

      offset += 5;
    }

    /* bike data */

    if ( S710_HAS_SPEED(w->mode) ) {

      /* cadence data */

      if ( S710_HAS_CADENCE(w->mode) ) {
	l->cad  = buf[offset];
	offset += 1;
      }

      /* next 4 bytes are power data */

      if ( S710_HAS_POWER(w->mode) ) {
	l->power.power = buf[offset] + (buf[offset+1]<<8);
	l->power.lr_balance  = buf[offset+3];   /* ??? switched  ??? */
	l->power.pedal_index = buf[offset+2];   /* ??? with this ??? */
	offset += 4;
      }

      /*
         next 4 bytes are distance/speed data
         this is cumulative distance from start (at least with S720i)
      */

      l->cumul_distance = buf[offset] + (buf[offset+1]<<8);
      l->distance = l->cumul_distance - prev_lap_dist;
      prev_lap_dist = l->cumul_distance;

      /*
	 The offset + 2 appears to be 4b.4b, where the upper 4b is the
	 integer portion of the speed and the lower 4b is the fractional
	 portion in sixteenths.  I'm not sure where the most significant
	 bits come from, but it might be the high order nibble of the next
	 byte- lets try that.
      */

      l->speed = buf[offset+2] + ((buf[offset+3] & 0xf0) << 4);

      /* fprintf(stderr, "distance/speed: %02x %02x %02x %02x\n",
	 buf[offset], buf[offset+1], buf[offset+2], buf[offset+3]); */
      /*  fprintf(stderr, "SPEED: %X\n", l->speed); */

    }
  }
}


static int read_samples ( workout_t * w, unsigned char * buf )
{
  int            offset;
  int            lap_size;
  int            sample_size;
  unsigned long  accum;
  int            ok = 1;
  int            i;
  int            s;
  int            x;

  lap_size    = bytes_per_lap(w->type,w->mode,w->interval_mode);
  sample_size = bytes_per_sample(w->mode);
  offset      = header_size(w);

  if ( offset != 0 ) {

    /* now add the offset due to laps */

    offset += w->laps * lap_size;

    /* number of samples */

    w->samples = (w->bytes - offset)/sample_size;

    /* allocate memory */

    ok = allocate_sample_space(w);

    /* if we succeeded in allocating the buffers, ok will not be 0 here. */

    if ( ok ) {

      /* At last, we can extract the samples.  They are in reverse order. */

      for ( i = 0; i < w->samples; i++ ) {
	s = offset + i * sample_size;
	x = w->samples - 1 - i;
	w->hr_data[x] = buf[s];
	s++;

	if ( S710_HAS_ALTITUDE(w->mode) ) {
	  w->alt_data[x] = buf[s] + ((buf[s+1] & 0x1f)<<8) - 512;
	  if ( w->units.system == S710_UNITS_ENGLISH ) {
	    w->alt_data[x] *= 5;
	  }
	  s += 2;
	}

	if ( S710_HAS_SPEED(w->mode) ) {
	  if ( S710_HAS_ALTITUDE(w->mode) ) s -= 1;
	  w->speed_data[x] = ((buf[s] & 0xe0) << 3) + buf[s+1];
	  s += 2;
	  if ( S710_HAS_POWER(w->mode) ) {
	    w->power_data[x].power = buf[s] + (buf[s+1]<<8);
	    w->power_data[x].lr_balance = buf[s+2];
	    w->power_data[x].pedal_index = buf[s+3];
	    s += 4;
	  }
	  if ( S710_HAS_CADENCE(w->mode) ) w->cad_data[x] = buf[s];
	}
      }

      if ( S710_HAS_SPEED(w->mode) ) {
	accum = 0;
	for ( i = 0; i < w->samples; i++ ) {
	  w->dist_data[i] = accum / 57600.0;
	  accum += w->speed_data[i] * w->recording_interval;
	}
      }

    }
  } else {
    /* we don't know what kind of HRM this is */

    ok = 0;
  }

  return ok;
}


static void compute_speed_info ( workout_t * w )
{
  int    i;
  int    j;
  float  avg;

  /* compute median speed and highest sampled speed */

  if ( S710_HAS_SPEED(w->mode) ) {
    avg = 0;
    w->highest_speed = 0;
    j = 0;
    for ( i = 0; i < w->samples; i++ ) {
      if ( w->speed_data[i] > w->highest_speed )
	w->highest_speed = w->speed_data[i];
      if ( w->speed_data[i] > 0 ) {
	avg = (float)(avg * j + w->speed_data[i])/(j+1);
	j++;
      }
    }
    w->median_speed = avg;
  }
}


/* don't call this unless you're sure it's going to be ok. */

static workout_t * extract_workout ( unsigned char *  buf,
		  S710_Filter      filter,
		  S710_HRM_Type    type )
{
  workout_t * w  = NULL;
  int         ok = 1;

  if ( (w = (workout_t*)calloc(1,sizeof(workout_t))) == NULL ) {
    fprintf(stderr,"extract_workout: calloc(%ld): %s\n",
	    (long)sizeof(workout_t),strerror(errno));
    return NULL;
  }

  /* Define the type of the HRM */

  w->type = type;

  /* Now extract the header data */

  read_preamble(w,buf);
  read_date(w,buf+10);
  read_duration(w,buf+15);

  w->avg_hr        = buf[19];
  w->max_hr        = buf[20];
  w->laps          = BCD(buf[21]);
  w->manual_laps   = BCD(buf[22]);
  w->interval_mode = (S710_Interval_Mode)buf[23];
  w->user_id       = BCD(buf[24]);

  read_units(w,buf[25]);

  /* recording mode and interval */

  if ( w->type == S710_HRM_S610 ) {
    w->mode = 0;
    read_recording_interval  (w,buf+26);
    read_hr_limits           (w,buf+28);
    read_bestlap_split       (w,buf+65);
    read_energy              (w,buf+69);
    read_cumulative_exercise (w,buf+75);
  } else {
    w->mode = buf[26];
    read_recording_interval  (w,buf+27);
    read_hr_limits           (w,buf+29);
    read_bestlap_split       (w,buf+66);
    read_energy              (w,buf+70);
    read_cumulative_exercise (w,buf+76);
    read_ride_info           (w,buf+79);
  }

  read_laps(w,buf);
  ok = read_samples(w,buf);

  /* never let a partially allocated workout get through. */

  if ( !ok ) {
    free_workout(w);
    return NULL;
  }

  /* if filtering was requested, filter the HR data and recompute
     avg/max HR values. */

  if ( filter == S710_FILTER_ON ) {
    filter_workout(w);
  }

  compute_speed_info(w);

  return w;
}


/*
   Attempt to auto-detect the HRM type based on some information in the file.
   This may not always succeed, but it seems to work relatively well.
*/

static S710_HRM_Type detect_hrm_type ( unsigned char * buf, unsigned int bytes )
{
  S710_HRM_Type type = S710_HRM_UNKNOWN;
  int           duration = 0;
  int           samples;
  int           laps;
  int           header = 0;
  int           bps = 0;
  int           bpl = 0;
  int           ri;

  if ( buf[34] == 0 && buf[36] == 251 ) {

    /* this is a s610 HRM */

    type = S710_HRM_S610;

  } else if ( (buf[35] == 0 || buf[35] == 48) && (buf[37] == 251 || buf[37] == 75) ) {

    /* this is either an s710 or s625x or...? */

    if ( (ri = get_recording_interval(buf[27])) != 0 ) {

      /* compute the number of bytes per sample and per lap */

      bps = bytes_per_sample(buf[26]);
      bpl = bytes_per_lap(S710_HRM_UNKNOWN,buf[26],buf[23]);

      /* obtain the number of laps and samples in the file */

      duration   = BCD(buf[16])+60*(BCD(buf[17])+60*BCD(buf[18]));
      samples    = duration / ri + 1;
      laps       = BCD(buf[21]);

      /* now compute the size of the file header */

      header     = bytes - samples * bps - laps * bpl;

      /* based on the header size, we can make a guess at
	 the HRM type.  note that this will NOT work if the
	 file was truncated due to the watch memory filling
	 up before recording was stopped. */

      /* assume it's an S710 unless the header size matches
	 the S625x header size. */

      if ( header == S710_HEADER_SIZE_S625X ) {
	type = S710_HRM_S625X;
      } else {
	type = S710_HRM_S710;
      }
    }
  }

  return type;
}


static workout_t * read_workout ( char *filename, S710_Filter filter, S710_HRM_Type type )
{
  int            fd;
  struct stat    sb;
  workout_t *    w = NULL;
  unsigned char  buf[65536];

  if ( stat(filename,&sb) != -1 ) {
#if Q_CC_MSVC
    if ( (fd = _open(filename,O_RDONLY)) != -1 ) {
      if ( sb.st_size < (int)sizeof(buf) ) {
    if ( _read(fd,buf,sb.st_size) == sb.st_size ) {
	  if ( buf[0] + (buf[1]<<8) == sb.st_size ) {
#else
    if ( (fd = open(filename,O_RDONLY)) != -1 ) {
      if ( sb.st_size < (int)sizeof(buf) ) {
    if ( read(fd,buf,sb.st_size) == sb.st_size ) {
      if ( buf[0] + (buf[1]<<8) == sb.st_size ) {
#endif
	    /*
	       if type == S710_HRM_AUTO, try to guess the "real" type.
	       we do this using some heuristics that may or may not be
	       reliable.
	    */

	    if ( type == S710_HRM_AUTO ) {
	      type = detect_hrm_type(buf,sb.st_size);
	    }

	    /* we're good to go */

	    if ( type != S710_HRM_UNKNOWN ) {
	      w = extract_workout(buf,filter,type);
	    } else {
	      fprintf(stderr,"%s: unable to auto-detect HRM type\n",
		      filename);
	    }
	  } else {
	    fprintf(stderr,"%s: invalid data [%d] [%d] (size %ld)\n",
		    filename,buf[0],buf[1],(long)sb.st_size);
	  }
	} else {
	  fprintf(stderr,"%s: read(%ld): %s\n",
		  filename,(long)sb.st_size,strerror(errno));
	}
      } else {
	fprintf(stderr,"%s: file size of %ld bytes is too big!\n",
		filename,(long)sb.st_size);
      }
#ifdef Q_CC_MSVC
      _close(fd);
#else
      close(fd);
#endif
    } else {
      fprintf(stderr,"open(%s): %s\n",filename,strerror(errno));
    }
  } else {
    fprintf(stderr,"stat(%s): %s\n",filename,strerror(errno));
  }

  return w;
}

static int header_size ( workout_t * w )
{
  int size = 0;

  switch ( w->type ) {
  case S710_HRM_S610:  size = S710_HEADER_SIZE_S610;  break;
  case S710_HRM_S625X: size = S710_HEADER_SIZE_S625X; break;
  case S710_HRM_S710:  size = S710_HEADER_SIZE_S710;  break;
  default:                                            break;
  }

  return size;
}


static int bytes_per_lap ( S710_HRM_Type type, unsigned char bt, unsigned char bi )
{
  int lap_size = 6;

  /* Compute the number of bytes per lap. */

  if ( S710_HAS_ALTITUDE(bt) )  lap_size += 5;
  if ( S710_HAS_SPEED(bt) ) {
    if ( S710_HAS_CADENCE(bt) ) lap_size += 1;
    if ( S710_HAS_POWER(bt) )   lap_size += 4;
    lap_size += 4;
  }

  /*
     This is Matti Tahvonen's fix for handling laps with interval mode.
     Applies to the S625X and the S710/S720i.
  */

  if ( type != S710_HRM_S610 && bi != 0 ) {
    lap_size += 5;
  }

  return lap_size;
}


static int bytes_per_sample ( unsigned char bt )
{
  int recsiz = 1;

  if ( S710_HAS_ALTITUDE(bt) )   recsiz += 2;
  if ( S710_HAS_SPEED(bt) ) {
    if ( S710_HAS_ALTITUDE(bt) ) recsiz -= 1;
    recsiz += 2;
    if ( S710_HAS_POWER(bt) )    recsiz += 4;
    if ( S710_HAS_CADENCE(bt) )  recsiz += 1;
  }

  return recsiz;
}


static int allocate_sample_space ( workout_t * w )
{
  int ok = 1;

#define  MAKEBUF(a,b)				                \
  if ( (w->a = (b *)calloc(w->samples,sizeof(b))) == NULL ) {	\
    fprintf(stderr,"%s: calloc(%d,%ld): %s\n",			\
	    #a,w->samples,(long)sizeof(b),strerror(errno));	\
    ok = 0;							\
  }

  MAKEBUF(hr_data,S710_Heart_Rate);
  if ( S710_HAS_ALTITUDE(w->mode) ) MAKEBUF(alt_data, S710_Altitude);
  if ( S710_HAS_SPEED(w->mode) ) {
    MAKEBUF(speed_data, S710_Speed);
    MAKEBUF(dist_data, S710_Distance);
    if ( S710_HAS_POWER(w->mode) )   MAKEBUF(power_data, S710_Power);
    if ( S710_HAS_CADENCE(w->mode) ) MAKEBUF(cad_data, S710_Cadence);
  }

#undef MAKEBUF

  return ok;
}

static void free_workout ( workout_t *w )
{
  if ( w != NULL ) {
    if ( w->lap_data )   free(w->lap_data);
    if ( w->alt_data )   free(w->alt_data);
    if ( w->speed_data ) free(w->speed_data);
    if ( w->dist_data )  free(w->dist_data);
    if ( w->cad_data )   free(w->cad_data);
    if ( w->power_data ) free(w->power_data);
    free(w);
  }
}

static char alpha_map ( unsigned char c )
{
  char a = '?';

  switch ( c ) {
  case 0: case 1: case 2: case 3: case 4:
  case 5: case 6: case 7: case 8: case 9:
    a = '0' + c;
    break;
  case 10:
    a = ' ';
    break;
  case 11: case 12: case 13: case 14: case 15:
  case 16: case 17: case 18: case 19: case 20:
  case 21: case 22: case 23: case 24: case 25:
  case 26: case 27: case 28: case 29: case 30:
  case 31: case 32: case 33: case 34: case 35:
  case 36:
    a = 'A' + c - 11;
    break;
  case 37: case 38: case 39: case 40: case 41:
  case 42: case 43: case 44: case 45: case 46:
  case 47: case 48: case 49: case 50: case 51:
  case 52: case 53: case 54: case 55: case 56:
  case 57: case 58: case 59: case 60: case 61:
  case 62:
    a = 'a' + c - 37;
    break;
  case 63: a = '-'; break;
  case 64: a = '%'; break;
  case 65: a = '/'; break;
  case 66: a = '('; break;
  case 67: a = ')'; break;
  case 68: a = '*'; break;
  case 69: a = '+'; break;
  case 70: a = '.'; break;
  case 71: a = ':'; break;
  case 72: a = '?'; break;
  default:          break;
  }

  return a;
}

static void extract_label ( unsigned char *buf, S710_Label *label, int bytes )
{
  int   i;
  char *p = (char *)label;

  for ( i = 0; i < bytes && i < (int)sizeof(S710_Label)-1; i++ ) {
    *(p+i) = alpha_map(buf[i]);
  }

  *(p+i) = 0;
}


/*
   This function filters out bad HR data from a workout and recomputes
   the average and max HR from the filtered data.
*/

static void filter_workout ( workout_t *w )
{
  int              v;
  int              lv;
  int              i;
  int              j;
  float            f_interp;
  int              remax = 0;
  float            avg;

  /* clean up the sample data */

  if ( w->hr_data != NULL ) {
    v  = 1;
    lv = 0;
    for ( i = 0; i < w->samples; i++ ) {
      if ( !v && w->hr_data[i] <= S710_MAX_VALID_HR && w->hr_data[i] != 0 ) {
	if ( lv >= 0 ) {
	  for ( j = lv; j < i; j++ ) {
	    f_interp = (float)w->hr_data[lv] +
	      (w->hr_data[i]-w->hr_data[lv])*(j-lv)/(i-lv);
	    w->hr_data[j] = (int) f_interp;
	  }
	}
	v = 1;
	remax = 1;
      } else if ( v &&
		  (w->hr_data[i] > S710_MAX_VALID_HR || w->hr_data[i] == 0) ) {
	v  = 0;
	lv = i - 1;
      }
    }

    /* recompute max and avg HR if we have to */

    if ( remax != 0 ) {
      w->max_hr = 0;
      avg = 0;
      j = 0;
      for ( i = 0; i < w->samples; i++ ) {
	if ( w->hr_data[i] > w->max_hr ) w->max_hr = w->hr_data[i];
	if ( w->hr_data[i] > 0 ) {
	  avg = (float)(avg * j + w->hr_data[i])/(j+1);
	  j++;
	}
      }
      w->avg_hr = (int)avg;
    }
  }

  if ( w->cad_data != NULL ) {
    v  = 1;
    lv = 0;
    for ( i = 0; i < w->samples; i++ ) {
      if ( !v && w->cad_data[i] <= S710_MAX_VALID_CAD ) {
	if ( lv >= 0 ) {
	  for ( j = lv; j < i; j++ ) {
	    f_interp = (float)w->cad_data[lv] +
	      (w->cad_data[i]-w->cad_data[lv])*(j-lv)/(i-lv);
	    w->cad_data[j] = (int) f_interp;
	  }
	}
	v = 1;
	remax = 1;
      } else if ( v &&
		  ( w->cad_data[i] > S710_MAX_VALID_CAD ) ) {
	v  = 0;
	lv = i - 1;
      }
    }

    /* recompute max and avg cadence if we have to */

    if ( remax != 0 ) {
      w->max_cad = 0;
      avg = 0;
      j = 0;
      for ( i = 0; i < w->samples; i++ ) {
	if ( w->cad_data[i] > w->max_cad ) w->max_cad = w->cad_data[i];
	if ( w->cad_data[i] > 0 ) {
	  avg = (float)(avg * j + w->cad_data[i])/(j+1);
	  j++;
	}
      }
      w->avg_cad = (int)avg;
    }
  }

  w->filtered = 1;
}
