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
#include "Units.h"

#include <QList>
#include <QMessageBox>
#include <QTextStream>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

quint16
BodyMeasure::getFingerprint() const
{
    quint64 x = 0;

    x += 1000.0 * weightkg;
    x += 1000.0 * fatkg;
    x += 1000.0 * musclekg;
    x += 1000.0 * boneskg;
    x += 1000.0 * leankg;
    x += 1000.0 * fatpercent;

    QByteArray ba = QByteArray::number(x);

    return qChecksum(ba, ba.length()) + Measure::getFingerprint();
}


bool
BodyMeasureParser::serialize(QString filename, QList<BodyMeasure> &data) {

    // open file - truncate contents
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Saving Body Measurements"));
        msgBox.setInformativeText(QObject::tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return false;
    };
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    BodyMeasure *m = NULL;
    QJsonArray measures;
    for (int i = 0; i < data.count(); i++) {
        m = &data[i];
        QJsonObject measure;
        measure.insert("when", m->when.toMSecsSinceEpoch()/1000);
        measure.insert("comment", m->comment);
        measure.insert("weightkg", m->weightkg);
        measure.insert("fatkg", m->fatkg);
        measure.insert("boneskg", m->boneskg);
        measure.insert("musclekg", m->musclekg);
        measure.insert("leankg", m->leankg);
        measure.insert("fatpercent", m->fatpercent);
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
BodyMeasureParser::unserialize(QFile &file, QList<BodyMeasure> &data) {

    // open file - truncate contents
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Reading Body Measurements"));
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
        msgBox.setText(QObject::tr("Problem Parsing Body Measurements"));
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
        m.source = static_cast<Measure::MeasureSource>(measure["source"].toInt());
        m.originalSource = measure["originalsource"].toString();
        data.append(m);
    }

    file.close();
    return true;
}

BodyMeasures::BodyMeasures(QDir dir, bool withData) : dir(dir), withData(withData) {
    // don't load data if not requested
    if (!withData) return;

    // get body measurements if the file exists
    QFile bodyFile(QString("%1/bodymeasures.json").arg(dir.canonicalPath()));
    if (bodyFile.exists()) {
        QList<BodyMeasure> bodyData;
        if (BodyMeasureParser::unserialize(bodyFile, bodyData)){
            setBodyMeasures(bodyData);
        }
    }
}

void
BodyMeasures::write() {
    // Nothing to do if data not loaded
    if (!withData) return;

    // now save data away
    BodyMeasureParser::serialize(QString("%1/bodymeasures.json").arg(dir.canonicalPath()), bodyMeasures_);
}

void
BodyMeasures::setBodyMeasures(QList<BodyMeasure>&x)
{
    bodyMeasures_ = x;
    qSort(bodyMeasures_); // date order
}

QStringList
BodyMeasures::getFieldSymbols() const {
    static const QStringList symbols = QStringList()<<"Weight"<<"FatKg"<<"MuscleKg"<<"BonesKg"<<"LeanKg"<<"FatPercent";
    return symbols;
}

QStringList
BodyMeasures::getFieldNames() const {
    static const QStringList names = QStringList()<<tr("Weight")<<tr("Fat Mass")<<tr("Muscle Mass")<<tr("Bones Mass")<<tr("Lean Mass")<<tr("Fat Percent");
    return names;
}

QDate
BodyMeasures::getStartDate() const {
    if (!bodyMeasures_.isEmpty())
        return bodyMeasures_.first().when.date();
    else
        return QDate();
}

QDate
BodyMeasures::getEndDate() const {
    if (!bodyMeasures_.isEmpty())
        return bodyMeasures_.last().when.date();
    else
        return QDate();
}

QString
BodyMeasures::getFieldUnits(int field, bool useMetricUnits) const {
    static const QStringList metricUnits = QStringList()<<tr("kg")<<tr("kg")<<tr("kg")<<tr("kg")<<tr("kg")<<tr("%");
    static const QStringList imperialUnits = QStringList()<<tr("lbs")<<tr("lbs")<<tr("lbs")<<tr("lbs")<<tr("lbs")<<tr("%");
    return useMetricUnits ? metricUnits.value(field) : imperialUnits.value(field);
}

void
BodyMeasures::getBodyMeasure(QDate date, BodyMeasure &here) const {
    // the optimisation below is not thread safe and should be encapsulated
    // by a mutex, but this kind of defeats to purpose of the optimisation!

    //if (withings_.count() && withings_.last().when.date() <= date) here = withings_.last();
    //if (!withings_.count() || withings_.first().when.date() > date) here = WithingsReading();

    // always set to not found before searching
    here = BodyMeasure();

    // loop
    foreach(BodyMeasure x, bodyMeasures_) {

        // we only look for weight readings at present
        // some readings may not include this so skip them
        if (x.weightkg <= 0) continue;

        if (x.when.date() <= date) here = x;
        if (x.when.date() > date) break;
    }

    // will be empty if none found
    return;
}

double
BodyMeasures::getFieldValue(QDate date, int field, bool useMetricUnits) const {
    const double units_factor = useMetricUnits ? 1.0 : LB_PER_KG;
    BodyMeasure weight;
    getBodyMeasure(date, weight);

    // return what was asked for!
    switch(field) {

        default:
        case BodyMeasure::WeightKg : return weight.weightkg * units_factor;
        case BodyMeasure::FatKg : return weight.fatkg * units_factor;
        case BodyMeasure::MuscleKg : return weight.musclekg * units_factor;
        case BodyMeasure::BonesKg : return weight.boneskg * units_factor;
        case BodyMeasure::LeanKg : return weight.leankg * units_factor;
        case BodyMeasure::FatPercent : return weight.fatpercent;
    }
}
