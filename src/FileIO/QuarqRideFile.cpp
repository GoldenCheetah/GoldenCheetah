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

#include "QuarqRideFile.h"
#include "QuarqParser.h"
#include <iostream>
#include <assert.h>

static QString installed_path = "";


QProcess *getInterpreterProcess( QString path ) {

    QProcess *antProcess;

    antProcess = new QProcess( );
    antProcess->start( path, QStringList() /* arguments */);

    if (!antProcess->waitForStarted()) {
      antProcess->deleteLater();
      antProcess = NULL;
    }

    return antProcess;
}

/*
   Thanks to ANT+ nondisclosure agreements, the Quarq ANT+
   interpretation code lives in a closed source binary. It takes the
   log on stdin and writes XML to stdout.

   If the binary is not available, no Quarq ANT+ capability is shown
   in the menu, nor are any ANT+ files opened.

   QProcess note:

     It turns out that the interpreter must actually be opened and
     executed before we can be sure it's there.  Checking the return
     value of start() isn't sufficient.  On my Linux system, start()
     returns true upon opening the OS X build of qollector_interpret.

 */


bool quarqInterpreterInstalled( void ) {

    static bool checkedInstallation = false;
    static bool installed;

    if (!checkedInstallation) {

      QString interpreterPath="/usr/local/bin/qollector_interpret/build-";
      QStringList executables;
      executables << "linux-i386/qollector_interpret";
      executables << "osx-ppc-i386/qollector_interpret";
      executables << "win32/qollector_interpret.exe";

      for ( QStringList::Iterator ex = executables.begin(); ex != executables.end(); ++ex ) {

	QProcess *antProcess = getInterpreterProcess( interpreterPath + *ex );

	if (NULL == antProcess) {
	  installed = false;
	} else {

	  antProcess->closeWriteChannel();
	  antProcess->waitForFinished(-1);

	  installed=((QProcess::NormalExit == antProcess->exitStatus()) &&
		     (0 == antProcess->exitCode()));

	  delete antProcess;

	  if (installed) {
	    installed_path = interpreterPath + *ex;
	    break;
	  }
	}
      }

      if (!installed)
	//std::cerr << "Cannot open qollector_interpret program, available from http://opensource.quarq.us/qollector_interpret." << std::endl;

      checkedInstallation = true;
    }

    return installed;
}

static int antFileReaderRegistered = quarqInterpreterInstalled()
    ? RideFileFactory::instance().registerReader(
        "qla", "Quarq ANT+ Files", new QuarqFileReader())
    : 0;

RideFile *QuarqFileReader::openRideFile(QFile &file, QStringList &errors, QList<RideFile*>*) const
{
    (void) errors;
    RideFile *rideFile = new RideFile();
    rideFile->setDeviceType("Quarq Qollector");
    rideFile->setFileFormat("Quarq ANT+ Files (qla)");
    rideFile->setRecIntSecs(1.0);

    QuarqParser handler(rideFile);

    QProcess *antProcess = getInterpreterProcess( installed_path );

    assert(antProcess);

    QXmlInputSource source (antProcess);
    QXmlSimpleReader reader;
    reader.setContentHandler (&handler);

    // this could done be a loop to "save memory."
    file.open(QIODevice::ReadOnly);
    (void)antProcess->write(file.readAll());
    antProcess->closeWriteChannel();
    antProcess->waitForFinished(-1);

    assert(QProcess::NormalExit == antProcess->exitStatus());
    assert(0 == antProcess->exitCode());

    reader.parse(source);

    reader.parseContinue();

    QRegExp rideTime("^.*/(\\d\\d\\d\\d)_(\\d\\d)_(\\d\\d)_"
                     "(\\d\\d)_(\\d\\d)_(\\d\\d)\\.qla$");
    if (rideTime.indexIn(file.fileName()) >= 0) {
        QDateTime datetime(QDate(rideTime.cap(1).toInt(),
                                 rideTime.cap(2).toInt(),
                                 rideTime.cap(3).toInt()),
                           QTime(rideTime.cap(4).toInt(),
                                 rideTime.cap(5).toInt(),
                                 rideTime.cap(6).toInt()));
        rideFile->setStartTime(datetime);
    }

    delete antProcess;

    return rideFile;
}

