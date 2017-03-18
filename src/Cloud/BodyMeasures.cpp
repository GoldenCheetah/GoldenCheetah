/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#include "BodyMeasures.h"

#include <QList>
#include <QMessageBox>
#include <QTextStream>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

bool
BodyMeasureParser::serialize(QString filename, QList<BodyMeasure> &data) {

    // open file - truncate contents
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Saving Body Measures"));
        msgBox.setInformativeText(QObject::tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return false;
    };
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    QJsonArray measures;
    for (int i = 0; i < data.count(); i++) {
        BodyMeasure m = data.at(i);
        QJsonObject measure;
        measure.insert("when", m.when.toMSecsSinceEpoch()/1000);
        measure.insert("comment", m.comment);
        measure.insert("weightkg", m.weightkg);
        measure.insert("fatkg", m.fatkg);
        measure.insert("boneskg", m.boneskg);
        measure.insert("musclekg", m.musclekg);
        measure.insert("leankg", m.leankg);
        measure.insert("fatpercent", m.fatpercent);
        measures.append(measure);
    }

    QJsonObject jsonObject;
    // add a version in case of format changes
    jsonObject.insert("version", 1);
    jsonObject.insert("measures",  QJsonValue(measures));

    QJsonDocument json;
    json.setObject(jsonObject);

    out << json.toJson();
    out.flush();
    file.close();
    return true;

}


bool
BodyMeasureParser::unserialize(QFile &file, QList<BodyMeasure> &data) {

    // open file - truncate contents
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Reading Body Measures"));
        msgBox.setInformativeText(QObject::tr("File: %1 cannot be opened for 'Reading'. Please check file properties.").arg(file.fileName()));
        msgBox.exec();
        return false;
    };
    QByteArray jsonFileContent = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(jsonFileContent, &parseError);

    if (parseError.error != QJsonParseError::NoError || document.isEmpty() || document.isNull()) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Parsing Body Measures"));
        msgBox.setInformativeText(QObject::tr("File: %1 is not a proper JSON file. Parsing error: %2").arg(file.fileName()).arg(parseError.errorString()));
        msgBox.exec();
        return false;
    }

    data.clear();
    QJsonObject object = document.object();
    QJsonArray measures = object["measures"].toArray();
    for (int i = 0; i < measures.count(); i++) {
        QJsonObject measure = measures.at(i).toObject();
        BodyMeasure m;
        m.when = QDateTime::fromMSecsSinceEpoch(measure["when"].toDouble()*1000);
        m.comment = measure["comment"].toString();
        m.weightkg = measure["weightkg"].toDouble();
        m.fatkg = measure["fatkg"].toDouble();
        m.boneskg = measure["boneskg"].toDouble();
        m.musclekg = measure["musclekg"].toDouble();
        m.leankg = measure["leankg"].toDouble();
        m.fatpercent = measure["fatpercent"].toDouble();
        data.append(m);
    }

    return true;
}

