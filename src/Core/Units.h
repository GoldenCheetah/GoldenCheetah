/*
 * Copyright (c) 2009 Sean C. Rhea (srhea@srhea.net)
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

#ifndef _GC_Units_h
#define _GC_Units_h 1

#define KM_PER_MILE 1.609344f
#define MILES_PER_KM 0.62137119f
#define FEET_PER_METER 3.2808399f
#define CM_PER_INCH 2.54f
#define INCH_PER_MM 0.039370f
#define INCH_PER_CM 0.39370f
#define METERS_PER_FOOT 0.3047999f
#define METERS_PER_YARD 0.9144f
#define LB_PER_KG 2.20462262f
#define KG_PER_LB 0.45359237f
#define FEET_LB_PER_NM 0.73756214837f
#define FAHRENHEIT_PER_CENTIGRADE 1.8f
#define FAHRENHEIT_ADD_CENTIGRADE 32.0f
#define GPS_COORD_TO_STRING 8
#define SECONDS_IN_A_WEEK 604800 // 7*24*60*60
#define FOOT_POUNDS_PER_METER 0.73756215f
#define KG_FORCE_PER_METER 9.80665f
#define MS_IN_ONE_HOUR 3600000
#define MS_IN_WKO_HOURS 360000 // yes this number of ms is required to match for WKO

#include <QString>
#include <QTime>

extern QTime kphToPaceTime(double kph, bool metric, bool isSwim=false);
extern QString kphToPace(double kph, bool metric, bool isSwim=false);
extern QString mphToPace(double mph, bool metric, bool isSwim=false);

#endif // _GC_Units_h
