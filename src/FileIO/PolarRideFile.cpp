/*
 * Copyright (c) 2007 Sean C. Rhea (srhea@srhea.net)
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

#include "PolarRideFile.h"
#include "Units.h"
#include <QRegExp>
#include <QTextStream>
#include <algorithm> // for std::sort
#include "cmath"
#include <assert.h>

static int polarFileReaderRegistered =
  RideFileFactory::instance().registerReader("hrm", "Polar Precision", new PolarFileReader());

bool ScanPddFile(QFile &file, QString &hrmFile, QString &hrvFile, QString &gpxFile,
		 QString &hrmFileCompare, QStringList &errors, QString &comment, QString &sport)
{
  QString section = NULL;
  bool haveExercise=false;
  bool useISO8859;

  int lineno = 1;
  int nrows = 12;
  int ntextrows = 5;

  // file.open(QFile::ReadOnly)
  if (file.open(QFile::ReadOnly)) {
      // Read the entire file into a QString -- we avoid using fopen since it
      // doesn't handle foreign characters well. Instead we use QFile and parse
      // from a QString
      QString contents;
      QTextStream in(&file);
      in.setCodec("UTF-8");
      contents = in.readAll();
      file.close();
      // check if the text string contains the replacement character for UTF-8 encoding
      // if yes, try to read with Latin1/ISO 8859-1
      useISO8859 = contents.contains(QChar::ReplacementCharacter);
  } else {
      errors << ("Could not open pdd file: \""
                 + file.fileName() + "\"");
      return false;
  }
  file.open(QFile::ReadOnly);
  QTextStream is(&file);
  if (useISO8859)
      is.setCodec ("ISO 8859-1");

  while (!is.atEnd()) {
    // the readLine() method doesn't handle old Macintosh CR line
    // endings this workaround will load the entire file if it
    // has CR endings then split and loop through each line
    // otherwise, there will be nothing to split and it will read
    // each line as expected.
    QString linesIn = is.readLine();
    QStringList lines = linesIn.split('\r');
    for (int li = 0; li < lines.size(); ++li) {
        QString line = lines[li];

        // workaround for empty lines
        if (lines.size() == 0) {
            lineno++;
            continue;
        }
        else if (line.startsWith("[ExerciseInfo")) {
            lineno = 0;
            comment = QString("");
            sport = QString("");
            haveExercise = true;
        }
        else
            lineno ++;

        if (haveExercise)
            {
                if (lineno == 1)
                    {
                        nrows = line.section('\t', 2, 2).toInt();
                        ntextrows = line.section('\t', 4, 4).toInt();
                    }
                else if (lineno > (nrows+1) && lineno <= (nrows+3))
                    {
                        comment += QString(line + "\n");
                    }
                else if (lineno == 3)
                    {
                        switch (line.section('\t', 0, 0).toInt())
                            {
                            case 1:
                                sport=QString("Run");
                                break;
                            case 2:
                                sport=QString("Bike");
                                break;
                            case 3:
                                sport=QString("Swim");
                                break;
                            default:
                                sport=QString("Bike");
                                break;
                            }
                    }
                else if (lineno == (nrows+4))
                    {
                        hrmFile.replace(0, hrmFile.length(), line);
                    }
                else if (lineno == (nrows+7))
                    {
                        gpxFile.replace(0, gpxFile.length(), line);
                    }
                else if (lineno == (nrows+8))
                    {
                        hrvFile.replace(0, hrvFile.length(), line);
                    }
                if (lineno == (nrows+ntextrows) && (hrmFile == hrmFileCompare || hrvFile == hrmFileCompare))
                    {
                        file.close();
                        return true;
                    }
            }
    }
  }
  file.close();
  errors << ("Could not find in pdd file: \""
	       + file.fileName() + "\"");
  return false;
}

void HrmRideFile(RideFile *rideFile, RideFile*gpxresult, bool haveGPX, XDataSeries *hrvXdata,
		 QFile &file, QStringList &errors, QString &note)
{
  /*
   * Polar HRM file format documented at www.polar.fi/files/Polar_HRM_file%20format.pdf
   *
   */
  QRegExp metricUnits("(km|kph|km/h)", Qt::CaseInsensitive);
  QRegExp englishUnits("(miles|mph|mp/h)", Qt::CaseInsensitive);
  bool metric = true;
  bool useISO8859;

  QDate date;

  double version=0;
  int monitor=0;

  double seconds=0;
  double distance=0;
  int interval = 0;
  int StartDelay = 0;

  bool speed = false;
  bool cadence = false;
  bool altitude = false;
  bool power = false;
  bool balance = false;

  int igpx = 0;
  int ngpx = 0;
  double lat=0,lon=0;
  int recInterval = 1;

  RideFilePoint *p;

  int lineno = 1;


  double next_interval=0;
  double hrv_time=0;
  QList<double> intervals;

  if (file.open(QFile::ReadOnly)) {
      // Read the entire file into a QString -- we avoid using fopen since it
      // doesn't handle foreign characters well. Instead we use QFile and parse
      // from a QString
      QString contents;
      QTextStream in(&file);
      in.setCodec("UTF-8");
      contents = in.readAll();
      file.close();
      // check if the text string contains the replacement character for UTF-8 encoding
      // if yes, try to read with Latin1/ISO 8859-1
      useISO8859 = contents.contains(QChar::ReplacementCharacter);
  } else {

    errors << ("Could not open ride file: \""
	       + file.fileName() + "\"");
    return;
  }
  file.open(QFile::ReadOnly);
  QTextStream is(&file);
  if (useISO8859)
      is.setCodec ("ISO 8859-1");
  QString section = NULL;

  if (haveGPX)
    ngpx = gpxresult->dataPoints().count();

  while (!is.atEnd()) {
    // the readLine() method doesn't handle old Macintosh CR line
    // endings this workaround will load the entire file if it
    // has CR endings then split and loop through each line
    // otherwise, there will be nothing to split and it will read
    // each line as expected.
    QString linesIn = is.readLine();
    QStringList lines = linesIn.split('\r');
    // workaround for empty lines
    if(lines.size() == 0) {
      lineno++;
      continue;
    }
    for (int li = 0; li < lines.size(); ++li) {
      QString line = lines[li];

      if (line == "") {

      }
      else if (line.startsWith("[")) {
	//fprintf(stderr, "section : %s\n", line.toAscii().constData());
	section=line;
	if (section == "[HRData]") {
	  // Some systems, like the Tacx HRM exporter, do not add an [IntTimes] section, so we need to
	  // specify that the whole ride is one big interval.
	  if (intervals.isEmpty())
	    intervals.append(seconds);
	  next_interval = intervals.at(0);
	}
      }
      else if (section == "[Params]"){
	if (line.contains("Version=")) {
	  QString versionString = QString(line);
	  versionString.remove(0,8).insert(1, ".");
	  version = versionString.toFloat();
	  rideFile->setFileFormat("Polar HRM v"+versionString+" (hrm)");
	} else if (line.contains("Monitor=")) {
	  QString monitorString = QString(line);
	  monitorString.remove(0,8);
	  monitor = monitorString.toInt();
	  switch (monitor) {
	  case 1: rideFile->setDeviceType("Polar Sport Tester / Vantage XL"); break;
	  case 2: rideFile->setDeviceType("Polar Vantage NV (VNV)"); break;
	  case 3: rideFile->setDeviceType("Polar Accurex Plus"); break;
	  case 4: rideFile->setDeviceType("Polar XTrainer Plus"); break;
	  case 6: rideFile->setDeviceType("Polar S520"); break;
	  case 7: rideFile->setDeviceType("Polar Coach"); break;
	  case 8: rideFile->setDeviceType("Polar S210"); break;
	  case 9: rideFile->setDeviceType("Polar S410"); break;
	  case 10: rideFile->setDeviceType("Polar S510"); break;
	  case 11: rideFile->setDeviceType("Polar S610 / S610i"); break;
	  case 12: rideFile->setDeviceType("Polar S710 / S710i"); break;
	  case 13: rideFile->setDeviceType("Polar S810 / S810i"); break;
	  case 15: rideFile->setDeviceType("Polar E600"); break;
	  case 20: rideFile->setDeviceType("Polar AXN500"); break;
	  case 21: rideFile->setDeviceType("Polar AXN700"); break;
	  case 22: rideFile->setDeviceType("Polar S625X / S725X"); break;
	  case 23: rideFile->setDeviceType("Polar S725"); break;
	  case 33: rideFile->setDeviceType("Polar CS400"); break;
	  case 34: rideFile->setDeviceType("Polar CS600X"); break;
	  case 35: rideFile->setDeviceType("Polar CS600"); break;
	  case 36: rideFile->setDeviceType("Polar RS400"); break;
	  case 37: rideFile->setDeviceType("Polar RS800"); break;
	  case 38: rideFile->setDeviceType("Polar RS800X"); break;

	  default: rideFile->setDeviceType(QString("Unknown Polar Device %1").arg(monitor));
	  }
	} else if (line.contains("SMode=")) {
	  line.remove(0,6);
	  QString smode = QString(line);
	  speed = smode.at(0) == '1';
	  cadence = smode.length()>0 && smode.at(1) == '1';
	  altitude = smode.length()>1 && smode.at(2) == '1';
	  power = smode.length()>2 && smode.at(3) == '1';
	  balance = smode.length()>3 && smode.at(4) == '1';
	  // pedaling_index = smode.length()>4 && smode.at(5)=='1';

	  //
	  // It appears that the Polar CS600 exports its data alays in metric when downloaded from the
	  // polar software even when English units are displayed on the unit..  It also never sets
	  // this bit low in the .hrm file.  This will have to get changed if other software downloads
	  // this differently
	  //

          if (smode.length()>6 && smode.at(7)=='1')
              metric = false;

	} else if (line.contains("Interval=")) {

	  recInterval = line.remove(0,9).toInt();

	  if (recInterval==238) {

	    /* This R-R data */
	    rideFile->setRecIntSecs(1);

	  } else {

	    rideFile->setRecIntSecs(recInterval);
	  }

	} else if (line.contains("Date=")) {
	  line.remove(0,5);
	  date= QDate(line.left(4).toInt(),
		      line.mid(4,2).toInt(),
		      line.mid(6,2).toInt());
	} else if (line.contains("StartTime=")) {
	  line.remove(0,10);
	  QDateTime datetime(date,
			     QTime(line.left(2).toInt(),
				   line.mid(3,2).toInt(),
				   line.mid(6,2).toInt()));
	  rideFile->setStartTime(datetime);

	} else if (line.contains("StartDelay=")) {

	  StartDelay = line.remove(0,11).toInt();

	  if (recInterval==238) {
	    seconds = StartDelay/1000.0;
	  } else {
	    seconds = recInterval;
	  }
	}

      } else if (section == "[Note]") {

	note.append(line);

      } else if (section == "[IntTimes]") {

	double int_seconds = line.left(2).toInt()*60*60+line.mid(3,2).toInt()*60+line.mid(6,3).toFloat();
	intervals.append(int_seconds);

	if (lines.size()==1) {
	  is.readLine();
	  is.readLine();
	  if (version>1.05) {
	    is.readLine();
	    is.readLine();
	  }
	} else {
	  li+=2;
	  if (version>1.05)
	    li+=2;
	}

      } else if (section == "[HRData]") {

	double nm=0,kph=0,watts=0,km=0,cad=0,hr=0,alt=0,hrm=0;
	double lrbalance=RideFile::NA;

	int i=0;
	hrm = line.section('\t', i, i).toDouble();
	i++;

	if (speed) {
	  kph = line.section('\t', i, i).toDouble()/10;
	  distance += kph/60/60*recInterval;
	  km = distance;
	  i++;
	}
	if (cadence) {
	  cad = line.section('\t', i, i).toDouble();
	  i++;
	}
	if (altitude) {
	  alt = line.section('\t', i, i).toDouble();
	  i++;
	}
	if (power) {
	  watts = line.section('\t', i, i).toDouble();
	  i++;
	}
	if (balance) {
	  // Power LRB + PI:  The value contains :
	  //  - Left Right Balance (LRB) and
	  //  - Pedaling Index (PI)
	  //
	  // in the following formula:
	  // value = PI * 256 + LRB   PI bits 15-8  LRB bits 7-0
	  // LRB is the value of left foot
	  // for example if LRB = 45, actual balance is L45 - 55R.
	  // PI values are percentages from 0 to 100.
	  // For example value 12857 (= 40 * 256 + 47)
	  // means: PI = 40 and LRB = 47 => L47 - 53R

	  lrbalance = line.section('\t', i, i).toInt() & 0xff;
	  i++;
	}

	if (next_interval < seconds) {
	  interval = intervals.indexOf(next_interval);
	  if (intervals.count()>interval+1){
	    interval++;
	    next_interval = intervals.at(interval);
	  }
	}

	if (!metric) {
	  km *= KM_PER_MILE;
	  kph *= KM_PER_MILE;
	  alt *= METERS_PER_FOOT;
	}

	if (recInterval==238){
	  XDataPoint *p_hrv = new XDataPoint();
	  hrv_time += hrm/1000.0;
	  p_hrv->secs = hrv_time;
	  p_hrv->number[0] = hrm;
	  hrvXdata->datapoints.append(p_hrv);
	  hr = 60000.0/hrm;
	} else {
	  hr = hrm;
	}

	if (haveGPX && gpxresult && (igpx<ngpx)) {

	  p = gpxresult->dataPoints()[igpx];

	  // Use previous value if GPS is momentarely
	  // lost. Should have option for interpolating.
	  if (p->lat!=0.0 && p->lon!=0.0) {

	    lat = p->lat;
	    lon = p->lon;

	    // Must check if current HRM speed is zero while
	    // we have GPX speed
	    if (kph==0.0 && p->kph>1.0) {

	      kph = p->kph;
	      distance += kph/60/60*recInterval;
	      km = distance;
	    }
	  }

	  if (seconds>=p->secs) igpx += 1;
	}

	rideFile->appendPoint(seconds, cad, hr, km, kph, nm, watts, alt, lon, lat,
			      0.0, 0.0,
			      RideFile::NA, lrbalance,
			      0.0, 0.0, 0.0, 0.0,
			      0.0, 0.0,
			      0.0, 0.0, 0.0, 0.0,
			      0.0, 0.0, 0.0, 0.0,
			      0.0, 0.0,
			      0.0, 0.0, 0.0, 0.0,
			      interval);

	// fprintf(stderr, " %f, %f, %f, %f, %f, %f, %f, %d\n", seconds, cad, hr, km, kph, nm, watts, alt, interval);
	if (recInterval==238) {
	  seconds += hrm / 1000.0;
	} else {
	  seconds += recInterval;
	}
      }

      ++lineno;
    }
  }

  rideFile->setTag("Notes", note);
  QRegExp rideTime("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_"
		   "(\\d\\d)_(\\d\\d)_(\\d\\d)\\.hrm$");
  if (rideTime.indexIn(file.fileName()) >= 0) {
    QDateTime datetime(QDate(rideTime.cap(1).toInt(),
			     rideTime.cap(2).toInt(),
			     rideTime.cap(3).toInt()),
		       QTime(rideTime.cap(4).toInt(),
			     rideTime.cap(5).toInt(),
			     rideTime.cap(6).toInt()));


    rideFile->setStartTime(datetime);
  }
  file.close();
}

RideFile *PolarFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*rideList) const
{
  /* Quit if file does not exist or cannot be read (use permissions instead)*/
  if (!file.open(QFile::ReadOnly)) {
    errors << ("Could not open ride file: \""
	       + file.fileName() + "\"");
    return NULL;
  }
  else
    file.close();

  QString hrmFile("");
  QString hrvFile("");
  QString gpxFile("");

  QString comment("");

  QFile hrmfile;
  QFile hrvfile;
  QFile gpxfile;

  RideFile *rideFile = new RideFile();
  bool haveGPX = false;
  bool haveHRV = false;
  bool haveHRM = false;

  // Read Polar GPX file (if exist with same name as hrm file).
  RideFile *gpxresult=NULL;
  QString suffix = file.fileName();
  int n_s = suffix.length();
  /* Try to scan .pdd file if exsist */
  QString location;
  QString hrmFileDate;
  QString hrmFile_orig;
  QFile pddfile;
  QString sport="";

  if (n_s > 12)
    {
      location = suffix.midRef(0, n_s - 12).toString();
      hrmFileDate = suffix.midRef(n_s - 12, 6).toString();
      hrmFile_orig = suffix.midRef(n_s - 12, n_s).toString();
      pddfile.setFileName(location + "20" + hrmFileDate + ".pdd");
    }

  if (n_s > 12 && pddfile.exists())
      {
          /* Scan .pdd file for main hrm, R-R version and gpx. Can use
	     either or both if exist */
          QString pddComment;
	  if (!ScanPddFile(pddfile, hrmFile, hrvFile, gpxFile, hrmFile_orig, errors, pddComment, sport))
              {
                  // Pdd file exist but it does not contain
                  // hrmFile_orig (probably been deleted)
                  delete rideFile;
                  return NULL;
              }
          else if (hrmFile_orig == hrvFile && hrmFile != hrvFile)
              {
                  errors << ("This is a R-R file: \""
                             + file.fileName() + "\"");
                  delete rideFile;
                  return NULL;
              }

	  haveHRM = hrmFile.length() > 0 && hrmFile == hrmFile_orig;
	  if (haveHRM)
            {
                hrmfile.setFileName(location + hrmFile);
                comment.append(pddComment);
            }

	  haveGPX = gpxFile.length() > 0;
	  if (haveGPX)
	    gpxfile.setFileName(location + gpxFile);

	  haveHRV = hrvFile.length() > 0 && haveHRM && hrmFile != hrvFile;
	  if (haveHRV)
	    hrvfile.setFileName(location + hrvFile);

	  if (!haveHRV&&!haveHRM)
	    {
                // Don't think this will ever happen.
                haveHRM = true;
                hrmfile.setFileName(suffix);
	    }
    }
  else
    {
        /* Search for <filename>.gpx and <filename>.rr.hrm */
        if (suffix.endsWith("rr.hrm"))
            {
                hrmfile.setFileName(QString(suffix).replace("rr.hrm",".hrm"));
                if (hrmfile.exists()){
                    // If both exist only read when equal to ".hrm"
                    errors << ("This is a R-R file: \""
                               + file.fileName() + "\"");
                    delete rideFile;
                    return NULL;
                }
                else {
                    // We read it as a hrm file and get the Xdata directly.
                    haveHRV = false;
                }
                gpxfile.setFileName(QString(suffix).replace("rr.hrm",".gpx"));
            }
        else
            {
                hrvfile.setFileName(QString(suffix).replace(".hrm",".rr.hrm"));
                gpxfile.setFileName(QString(suffix).replace(".hrm",".gpx"));
                haveHRV = hrvfile.exists();
            }

        hrmfile.setFileName(suffix);
        haveHRM = hrmfile.exists();
        if (!haveHRM)
            {
                errors << ("Can not find : \""
                           + file.fileName() + "\"");
                delete rideFile;
                return NULL;
            }
        haveGPX = gpxfile.exists();
    }


  if (haveGPX) {
    GpxFileReader reader;
    gpxresult = reader.openRideFile(gpxfile,errors,rideList);
  }

  XDataSeries *hrvXdata = new XDataSeries();

  hrvXdata->name = "HRV";
  hrvXdata->valuename << "R-R";
  hrvXdata->unitname << "msecs";

  if (haveHRM)
    {
      HrmRideFile(rideFile, gpxresult, haveGPX, hrvXdata, hrmfile, errors, comment);
      if (haveHRV)
	{
	  // Get the HRV data
	  RideFile *hrv_rideFile = new RideFile();
	  HrmRideFile(hrv_rideFile, gpxresult, haveGPX, hrvXdata, hrvfile, errors, comment);
	  delete hrv_rideFile;
	}
    }
  else if (hrvFile.length() > 0 && !file.fileName().contains(hrvFile))
    {
      HrmRideFile(rideFile, gpxresult, haveGPX, hrvXdata, file, errors, comment);
    }
  else
    {
        // Don't think this will ever happen.
      errors << ("This is a R-R file?: \""
		 + file.fileName() + "\"");
      delete rideFile;
      delete hrvXdata;
      return NULL;
    }

  if (hrvXdata->datapoints.count()>0)
    {
      rideFile->addXData("HRV", hrvXdata);
    }
  else
    delete hrvXdata;

  rideFile->setTag("Sport", sport);
  return rideFile;
}
