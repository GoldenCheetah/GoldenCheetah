/*
 * Copyright (c) 2015 Joern Rischmueller (joern.rm@gmail.com)
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

#include "HrvMeasures.h"

#include <QList>
#include <QMessageBox>
#include <QTextStream>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

quint16
HrvMeasure::getFingerprint() const
{
    quint64 x = 0;

    x += when.date().toJulianDay();
    x += hr;
    x += avnn;
    x += sdnn;
    x += rmssd;
    x += pnn50;
    x += lf;
    x += hf;
    x += recovery_points;

    QByteArray ba = QByteArray::number(x);

    return qChecksum(ba, ba.length());
}

QString
HrvMeasure::getSourceDescription() const {

    switch(source) {
    case HrvMeasure::Manual:
        return QObject::tr("Manual entry");
    case HrvMeasure::CSV:
        return QObject::tr("CSV Upload");
    default:
        return QObject::tr("Unknown");
    }
}


bool
HrvMeasureParser::serialize(QString filename, QList<HrvMeasure> &data) {

    // open file - truncate contents
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Saving HRV Measurements"));
        msgBox.setInformativeText(QObject::tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return false;
    };
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    HrvMeasure *m = NULL;
    QJsonArray measures;
    for (int i = 0; i < data.count(); i++) {
        m = &data[i];
        QJsonObject measure;
        measure.insert("when", m->when.toMSecsSinceEpoch()/1000);
        measure.insert("hr", m->hr);
        measure.insert("avnn", m->avnn);
        measure.insert("sdnn", m->sdnn);
        measure.insert("rmssd", m->rmssd);
        measure.insert("pnn50", m->pnn50);
        measure.insert("lf", m->lf);
        measure.insert("hf", m->hf);
        measure.insert("recovery_points", m->recovery_points);
        measure.insert("source", m->source);
        measure.insert("originalsource", m->originalSource);
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
HrvMeasureParser::unserialize(QFile &file, QList<HrvMeasure> &data) {

    // open file - truncate contents
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Reading HRV Measurements"));
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
        msgBox.setText(QObject::tr("Problem Parsing HRV Measurements"));
        msgBox.setInformativeText(QObject::tr("File: %1 is not a proper JSON file. Parsing error: %2").arg(file.fileName()).arg(parseError.errorString()));
        msgBox.exec();
        return false;
    }

    data.clear();
    QJsonObject object = document.object();
    QJsonArray measures = object["measures"].toArray();
    for (int i = 0; i < measures.count(); i++) {
        QJsonObject measure = measures.at(i).toObject();
        HrvMeasure m;
        m.when = QDateTime::fromMSecsSinceEpoch(measure["when"].toDouble()*1000);
        m.hr = measure["hr"].toDouble();
        m.avnn = measure["avnn"].toDouble();
        m.sdnn = measure["sdnn"].toDouble();
        m.rmssd = measure["rmssd"].toDouble();
        m.pnn50 = measure["pnn50"].toDouble();
        m.lf = measure["lf"].toDouble();
        m.hf = measure["hf"].toDouble();
        m.recovery_points = measure["recovery_points"].toDouble();
        m.source = static_cast<HrvMeasure::HrvMeasureSource>(measure["source"].toInt());
        m.originalSource = measure["originalsource"].toString();
        data.append(m);
    }

    return true;
}

