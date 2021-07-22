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

#include "MeasuresCsvImport.h"
#include "Settings.h"
#include "Athlete.h"

#include <QRegExp>
#include <algorithm>




MeasuresCsvImport::MeasuresCsvImport(Context *context, QWidget *parent) : context(context), parent(parent)
{
}

MeasuresCsvImport::~MeasuresCsvImport()
{
}

bool
MeasuresCsvImport::getMeasures(MeasuresGroup *measuresGroup, QString &error, QDateTime from, QDateTime to, QList<Measure> &data)
{

  // all variables to be defined here (to allow :goto - for simplified error handling/less redundant code)
  bool tsExists = false;
  bool dateExists = false;
  bool reqFieldExists = false; // the first field is required

  QString fileName = QFileDialog::getOpenFileName(parent, tr("Select %1 measurements file to import").arg(measuresGroup->getName()), "", tr("CSV Files (*.csv)"));
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

  int fieldCount = std::min(measuresGroup->getFieldSymbols().count(), MAX_MEASURES);

  // get all lines considering both LF and CR endings
  QStringList lines = QString(file.readAll()).split(QRegularExpression("[\n\r]"));

  // get headers first / and check if this is a valid measures file
  CsvString headerLine = lines[0];
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
      if (h == "ts" || h == "timestamp_measurement" || h == "Datetime") tsExists = true;
      if (h == "date") dateExists = true;
      if (measuresGroup->getFieldHeaders(0).contains(h)) reqFieldExists = true;
  }
  if (!tsExists && !dateExists) {
      error = tr("Column 'timestamp_measurement'/'Datetime'/'date' is missing.");
      goto error;
  }
  if (!reqFieldExists) {
      error = tr("Column '%1' is missing.").arg(measuresGroup->getFieldHeaders(0).join("/"));
      goto error;
  }

  // No duplicates and minimal "timestamp"/"date" and required field exist
  emit downloadProgress(50);
  for (int lineNo = 1; lineNo<lines.count(); lineNo++) {
      CsvString itemLine = lines[lineNo];
      QStringList items = itemLine.split();
      if (items.count() == 0) continue; // skip empty lines
      if (items.count() < headers.count()) {
          // we only process valid data - so stop here
          // independent if other items are ok
          error = tr("Number of data columns: %1 in line %2 lower than header columns: %3").arg(items.count()).arg(lineNo).arg(headers.count());
          goto error;
      }
      // extract the values
      Measure m;
      for (int j = 0; j< headers.count(); j++) {
          QString h = headers.at(j);
          QString i = items.at(j);
          bool ok;
          if (i.isEmpty() || i == "-") {
              continue; // skip empty values
          }
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
          } else if (h == "timestamp_measurement" || h == "Datetime" ||
                     (h == "date" && i.length() > 18)) { // Body measures date
              // parse date/time ISO 8601 (HRV4Training for iPhone or EliteHRV)
              tsExists = true;
              m.when = QDateTime::fromString(i, Qt::ISODate);
              if (!m.when.isValid()) {
                  // try 12hr format (HRV4Training for iPhone variant...)
                  m.when = QDateTime::fromString(i.left(19).trimmed(), "yyyy-MM-dd h:mm:ss");
                  if (i.contains("p", Qt::CaseInsensitive))
                      m.when = m.when.addSecs(12*3600);
              }
              if (m.when.isValid()) {
                  // skip line if not in date range
                  if (m.when < from || m.when > to) {
                      m.when = QDateTime::fromMSecsSinceEpoch(0);
                      break; // stop analysing the data items of the line
                  }
              } else {
                  error = tr("Invalid 'timestamp' - Date/Time not ISO 8601 format - in line %1").arg(lineNo) + ": " + i;
                  goto error;
              }
          } else if (!tsExists && h == "date") {
              // parse date (HRV4Training for Android)
              m.when = QDateTime(QDate::fromString(i, "yyyy-dd-MM"));
              if (m.when.date().isValid()) {
                  // skip line if not in date range
                  if (m.when < from || m.when > to) {
                      m.when = QDateTime::fromMSecsSinceEpoch(0);
                      break; // stop analysing the data items of the line
                  }
              } else {
                  error = tr("Invalid 'date' - Date not yyyy-dd-MM format - in line %1").arg(lineNo) + ": " + i;
                  goto error;
              }
          } else if (!tsExists && h == "time") {
              // parse time (HRV4Training for Android)
              m.when.setTime(QTime::fromString(i, "hh:mm"));
          } else if (h == "note") {
              m.comment = i.trimmed();
          } else {
              for (int k=0; k<fieldCount; k++)
                  if (measuresGroup->getFieldHeaders(k).contains(h)) {
                      if (i.contains(":")) {
                          // sexagesimal format
                          double f = 1.0;
                          foreach (QString s, i.split(":")) {
                              m.values[k] += s.toDouble(&ok) / f;
                              if (!ok) break;
                              f *= 60;
                          }
                      } else {
                          // decimal format
                          m.values[k] = i.toDouble(&ok);
                      }
                      if (!ok) {
                          error = tr("Invalid '%1' - in line %2")
                              .arg(measuresGroup->getFieldHeaders(k).join("/"))
                              .arg(lineNo);
                          goto error;
                      }
                      break;
                  }
          }
      }
      // only append if we have a good date & required field non zero
      if (m.when > QDateTime::fromMSecsSinceEpoch(0) && m.values[0] > 0.0) {
          m.source = Measure::CSV;
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
