/*
 * Copyright (c) 2009 Mark Rages (mark@quarq.us)
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

#include <QString>
#include <iostream>
#include "QuarqParser.h"

QuarqParser::QuarqParser (RideFile* rideFile)
  : rideFile(rideFile),
    version(""),
    km(0),
    watts(0),
    cad(0),
    hr(0),
    initial_seconds(-1.0),
    seconds_from_start(0),
    kph(0),
    nm(0)
{
  ;
}

// implement sample-and-hold resampling

#define SAMPLE_INTERVAL 1.0 // seconds

void
QuarqParser::incrementTime( const double new_time )
{
  if (initial_seconds < 0.0) {
    initial_seconds = new_time;
  }

  float time_diff = new_time - initial_seconds;

  while (time_diff > seconds_from_start) {

    rideFile->appendPoint(seconds_from_start, cad, hr, km,
                          kph, nm, watts, 0, 0.0, 0.0, 0.0, 0.0, RideFile::NA, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0);

    seconds_from_start += SAMPLE_INTERVAL;
  }
  time.setSecsSinceEpoch(new_time);
}

bool
QuarqParser::startElement( const QString&, const QString&,
			 const QString& qName,
			 const QXmlAttributes& qAttributes)
{
    buf.clear();

    if (qName == "Qollector") {
      version = qAttributes.value("version");

      // reset the timer for a new <Qollector> tag
      seconds_from_start = 0.0;
      initial_seconds = -1;

      return true;
    }

#define CheckQuarqXml(name,unit,dest)  do { 				\
      if (qName== #name) {						\
	QString name = qAttributes.value( #unit );			\
	QString timestamp = qAttributes.value("timestamp");		\
									\
	if ((! name.isEmpty()) && (!timestamp.isEmpty()) &&		\
	    ( name.toLower() != "nan")) {				\
	  dest = name.toDouble();					\
	  incrementTime(timestamp.toDouble());				\
	}								\
	return true;							\
      }									\
    } while (0);

    CheckQuarqXml(Cadence, RPM, cad );
    CheckQuarqXml(Power, watts, watts );
    CheckQuarqXml(HeartRate, BPM, hr );
    // clearly bogus, equating RPM to kph.
    // Unless you have an 18 foot wheel, by chance
    CheckQuarqXml(Speed, RPM, kph );

#undef CheckQuarqXml

    // default case

    // only print the first time and unknown happens
    if (!unknown_keys[qName]++)
      std::cerr << "Unknown Element " << qPrintable(qName) << std::endl;
    return true;
}

bool
QuarqParser::endElement( const QString&, const QString&, const QString& qName)
{

    // flush one last data point
    if (qName == "Qollector") {
      rideFile->appendPoint(seconds_from_start, cad, hr, km,
                            kph, nm, watts, 0, 0.0, 0.0, 0.0, 0.0,
                            RideFile::NA, RideFile::NA,
                            0.0,0.0,0.0,0.0,
                            0.0,0.0,
                            0.0,0.0,0.0,0.0,
                            0.0,0.0,0.0,0.0,
                            0.0,0.0,
                            0.0,0.0,0.0,0.0,
                            0);
    }

    return true;
}

bool
QuarqParser::characters( const QString& str )
{
    buf += str;
    return true;
}
