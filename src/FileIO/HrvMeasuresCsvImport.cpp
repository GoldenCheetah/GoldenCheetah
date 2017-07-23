/*
 * Copyright (c) 2017 Joern Rischmueller (joern.rm@gmail.com)
 * Copyright (c) 2017 Ale Martinez (amtriathlon@gmail.com)
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

#include "HrvMeasuresCsvImport.h"
#include "Settings.h"
#include "Athlete.h"




HrvMeasuresCsvImport::HrvMeasuresCsvImport(Context *context) : context(context) {

    allowedHeaders << "timestamp_measurement" << "HR" << "AVNN" << "SDNN" << "rMSSD" << "pNN50" << "LF" << "HF" << "HRV4T_Recovery_Points";
}

HrvMeasuresCsvImport::~HrvMeasuresCsvImport() {
}


bool
HrvMeasuresCsvImport::getHrvMeasures(QString &error, QDateTime from, QDateTime to, QList<HrvMeasure> &data) {

  // all variables to be defined here (to allow :goto - for simplified error handling/less redundant code)
  bool tsExists = false;
  bool rmssdExists = false;
  int lineNo = 0;

  QString fileName = QFileDialog::getOpenFileName(NULL, tr("Select HRV measurements file to import"), "", tr("CSV Files (*.csv)"));
  if (fileName.isEmpty()) {
      error = tr("No file selected.");
      return false;
  }
  QFile file(fileName);
  if (!file.open(QFile::ReadOnly)) {
      error = tr("Selected file %1 cannot be opened for reading.").arg(fileName);
      file.close();
      return false;
  };

  emit downloadStarted(100);
  // get headers first / and check if this is a HRV measures file
  CsvString headerLine= QString(file.readLine());
  QStringList headers = headerLine.split();
  // remove whitespaces from strings
  QMutableStringListIterator iterator(headers);
  while (iterator.hasNext()) {
      QString simple = iterator.next().simplified();
      iterator.setValue(simple);
  }

  // check for duplicates - which are not allowed
  if (headers.removeDuplicates() > 0) {
      error = tr("Column header contains duplicate identifier");
      goto error;
  }

  foreach(QString h, headers) {
      if (h == "timestamp_measurement") tsExists = true;
      if (h == "rMSSD") rmssdExists = true;
      if (!allowedHeaders.contains(h)) {
          if (error.isEmpty()) error = tr("Unknown column header: ");
          if (h.isEmpty()) h = tr("SPACE");
          error.append(h + " ");
      }
  }
  if (error.length() > 0) {
      goto error;
  }
  if (!tsExists) {
      error = tr("Date and Timp are missing - Column 'timestamp_measurement' for measurement timestamp.");
      goto error;
  }
  if (!rmssdExists) {
      error = tr("Column 'rMSSD' - is missing");
      goto error;
  }

  // all headers are valid, no duplicates and minimal "timestamp_measurement" and "rmssd" exist
  emit downloadProgress(50);
  while (!file.atEnd()) {
      CsvString itemLine = QString(file.readLine());
      lineNo ++;
      QStringList items = itemLine.split();
      if (items.count() != headers.count()) {
          // we only process valid data - so stop here
          // independent if other items are ok
          error = tr("Number of data columns: %1 in line %2 deviates from header columns: %3").arg(items.count()).arg(lineNo).arg(headers.count());
          goto error;
      }
      // extract the values
      HrvMeasure m;
      for (int j = 0; j< headers.count(); j++) {
          QString h = headers.at(j);
          QString i = items.at(j);
          bool ok;
          if (h == "timestamp_measurement") {
            // parse date ISO 8601
            m.when = QDateTime::fromString(i, Qt::ISODate);
            if (m.when.isValid()) {
                // skip line if not in date range
                if (m.when < from || m.when > to) {
                    m.when = QDateTime::fromMSecsSinceEpoch(0);
                    break; // stop analysing the data items of the line
                }
            } else {
                error = tr("Invalid 'date' - Date/Time not ISO 8601 format - in line %1").arg(lineNo);
                goto error;
            }
          } else if (h == "rMSSD") {
              m.rmssd = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'rMSSD' - in line %1").arg(lineNo);
                  goto error;
              }
          } else if (h == "HR") {
              m.hr = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'HR' - in line %1").arg(lineNo);
                  goto error;
              }
          } else if (h == "AVNN") {
              m.avnn = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'AVNN' - in line %1").arg(lineNo);
                  goto error;
              }
          } else if (h == "SDNN") {
              m.sdnn = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'SDNN' - in line %1").arg(j);
                  goto error;
              }
          } else if (h == "pNN50") {
              m.pnn50 = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'pNN50' - in line %1").arg(lineNo);
                  goto error;
              }
          } else if (h == "LF") {
              m.lf = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'LF' - in line %1").arg(lineNo);
                  goto error;
              }
          } else if (h == "HF") {
              m.hf = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'HF' - in line %1").arg(lineNo);
                  goto error;
              }
          } else if (h == "HRV4T_Recovery_Points") {
              m.recovery_points = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'HRV4T_Recovery_Points' - in line %1").arg(lineNo);
                  goto error;
              }

          }
      }
      // only append if we have a good date
      if (m.when > QDateTime::fromMSecsSinceEpoch(0)) {
          m.source = HrvMeasure::CSV;
          data.append(m);
      }
  }
  // all fine
  file.close();
  emit downloadEnded(100);
  return true;

  // in case of errors - clean up and return
  error:
  file.close();
  return false;


}

