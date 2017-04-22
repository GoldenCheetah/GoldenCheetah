/*
 * Copyright (c) 2017 Joern Rischmueller (joern.rm@gmail.com)
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

#include "BodyMeasuresCsvImport.h"
#include "Settings.h"
#include "Athlete.h"




BodyMeasuresCsvImport::BodyMeasuresCsvImport(Context *context) : context(context) {

    allowedHeaders << "ts" << "date" << "weightkg" << "fatkg" << "boneskg" << "musclekg" << "leankg"
                   << "fatpercent" << "comment";
}

BodyMeasuresCsvImport::~BodyMeasuresCsvImport() {
}


bool
BodyMeasuresCsvImport::getBodyMeasures(QString &error, QDateTime from, QDateTime to, QList<BodyMeasure> &data) {

  // all variables to be defined here (to allow :goto - for simplified error handling/less redundant code)
  bool tsExists = false;
  bool dateExists = false;
  bool weightkgExists = false;
  int lineNo = 0;

  QString fileName = QFileDialog::getOpenFileName(NULL, tr("Select body measurements file to import"), "", tr("CSV Files (*.csv)"));
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
  // get headers first / and check if this is a body measures file
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
      if (h == "ts") tsExists = true;
      if (h == "date") dateExists = true;
      if (h == "weightkg") weightkgExists = true;
      if (!allowedHeaders.contains(h)) {
          if (error.isEmpty()) error = tr("Unknown column header: ");
          if (h.isEmpty()) h = tr("SPACE");
          error.append(h + " ");
      }
  }
  if (error.length() > 0) {
      goto error;
  }
  if (!(tsExists || dateExists)) {
      error = tr("Date and Timestamp are missing - Column 'ts' for timestamp - Colum 'date' for Date/Time.");
      goto error;
  }
  if (tsExists && dateExists) {
      error = tr("Both column 'ts' - Timestamp and 'date' - Date/Time are defined.");
      goto error;
  }
  if (!weightkgExists) {
      error = tr("Column 'weightkg' - Weight in kg - is missing");
      goto error;
  }

  // all headers are valid, no duplicates and minimal "ts" and "weightkg" exist
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
      BodyMeasure m;
      for (int j = 0; j< headers.count(); j++) {
          QString h = headers.at(j);
          QString i = items.at(j);
          bool ok;
          if (h == "ts") {
             qint64 ts = i.toLongLong(&ok);
             if (ok) {
                // is is a valid timestamp (in Secs / not MSecs)
                m.when = QDateTime::fromMSecsSinceEpoch(ts*1000);
                // skip line if not in date range
                if (m.when < from || m.when > to) {
                    m.when = QDateTime::fromMSecsSinceEpoch(0);
                    break; // stop analysing the data items of the line
                }
             }
             if (!ok || !m.when.isValid()) {
                 error = tr("Invalid 'ts' - Timestamp - in line %1").arg(lineNo);
                 goto error;
             }
          } else if (h == "date") {
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
          } else if (h == "weightkg") {
              m.weightkg = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'weightkg' - Weight in kg - in line %1").arg(lineNo);
                  goto error;
              }
          } else if (h == "fatkg") {
              m.fatkg = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'fatkg' - Fat in kg - in line %1").arg(lineNo);
                  goto error;
              }
          } else if (h == "boneskg") {
              m.boneskg = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'boneskg' - Bones in kg - in line %1").arg(lineNo);
                  goto error;
              }
          } else if (h == "musclekg") {
              m.musclekg = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'musclekg' - Muscles in kg - in line %1").arg(j);
                  goto error;
              }
          } else if (h == "leankg") {
              m.leankg = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'leankg' - Lean mass in kg - in line %1").arg(lineNo);
                  goto error;
              }
          } else if (h == "fatpercent") {
              m.fatpercent = i.toDouble(&ok);
              if (!ok) {
                  error = tr("Invalid 'fatpercent' - Fat in percent - in line %1").arg(lineNo);
                  goto error;
              }

          } else if (h == "comment") {
              m.comment = i.trimmed();
          }
      }
      // only append if we have a good date
      if (m.when > QDateTime::fromMSecsSinceEpoch(0)) {
          m.source = BodyMeasure::CSV;
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



