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

    x += 1000.0 * hr;
    x += 1000.0 * avnn;
    x += 1000.0 * sdnn;
    x += 1000.0 * rmssd;
    x += 1000.0 * pnn50;
    x += 1000.0 * lf;
    x += 1000.0 * hf;
    x += 1000.0 * recovery_points;

    QByteArray ba = QByteArray::number(x);

    return qChecksum(ba, ba.length()) + Measure::getFingerprint();
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
        measure.insert("comment", m->comment);
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
        m.comment = measure["comment"].toString();
        m.hr = measure["hr"].toDouble();
        m.avnn = measure["avnn"].toDouble();
        m.sdnn = measure["sdnn"].toDouble();
        m.rmssd = measure["rmssd"].toDouble();
        m.pnn50 = measure["pnn50"].toDouble();
        m.lf = measure["lf"].toDouble();
        m.hf = measure["hf"].toDouble();
        m.recovery_points = measure["recovery_points"].toDouble();
        m.source = static_cast<Measure::MeasureSource>(measure["source"].toInt());
        m.originalSource = measure["originalsource"].toString();
        data.append(m);
    }

    file.close();
    return true;
}

HrvMeasures::HrvMeasures(QDir dir, bool withData) : dir(dir), withData(withData) {
    // don't load data if not requested
    if (!withData) return;

    // get hrv measurements if the file exists
    QFile hrvFile(QString("%1/hrvmeasures.json").arg(dir.canonicalPath()));
    if (hrvFile.exists()) {
        QList<HrvMeasure> hrvData;
        if (HrvMeasureParser::unserialize(hrvFile, hrvData)){
            setHrvMeasures(hrvData);
        }
    }
}

void
HrvMeasures::write() {
    // Nothing to do if data not loaded
    if (!withData) return;

    // now save data away
    HrvMeasureParser::serialize(QString("%1/hrvmeasures.json").arg(dir.canonicalPath()), hrvMeasures_);
}

void
HrvMeasures::setHrvMeasures(QList<HrvMeasure>&x)
{
    hrvMeasures_ = x;
    qSort(hrvMeasures_); // date order
}

QStringList
HrvMeasures::getFieldSymbols() const {
    static const QStringList symbols = QStringList()<<"HR"<<"AVNN"<<"SDNN"<<"RMSSD"<<"PNN50"<<"LF"<<"HF"<<"RecoveryPoints";
    return symbols;
}

QStringList
HrvMeasures::getFieldNames() const {
    static const QStringList names = QStringList()<<tr("HR")<<tr("AVNN")<<tr("SDNN")<<tr("RMSSD")<<tr("PNN50")<<tr("LF")<<tr("HF")<<tr("Recovery Points");
    return names;
}

QDate
HrvMeasures::getStartDate() const {
    if (!hrvMeasures_.isEmpty())
        return hrvMeasures_.first().when.date();
    else
        return QDate();
}

QDate
HrvMeasures::getEndDate() const {
    if (!hrvMeasures_.isEmpty())
        return hrvMeasures_.last().when.date();
    else
        return QDate();
}

QString
HrvMeasures::getFieldUnits(int field, bool useMetricUnits) const {
    Q_UNUSED(useMetricUnits);
    static const QStringList units = QStringList()<<tr("bpm")<<tr("msec")<<tr("msec")<<tr("msec")<<tr("%")<<tr("msec^2")<<tr("msec^2")<<tr("Rec.Points");
    return units.value(field);
}

void
HrvMeasures::getHrvMeasure(QDate date, HrvMeasure &here) const {
    // always set to not found before searching
    here = HrvMeasure();

    // loop
    foreach(HrvMeasure x, hrvMeasures_) {

        if (x.when.date() == date) here = x;
        if (x.when.date() > date) break;
    }

    // will be empty if none found
    return;
}

double
HrvMeasures::getFieldValue(QDate date, int field, bool useMetricUnits) const {
    Q_UNUSED(useMetricUnits);
    HrvMeasure hrv;
    getHrvMeasure(date, hrv);

    // return what was asked for!
    switch(field) {

        default:
        case HrvMeasure::HR : return hrv.hr;
        case HrvMeasure::AVNN : return hrv.avnn;
        case HrvMeasure::SDNN : return hrv.sdnn;
        case HrvMeasure::RMSSD : return hrv.rmssd;
        case HrvMeasure::PNN50 : return hrv.pnn50;
        case HrvMeasure::LF : return hrv.lf;
        case HrvMeasure::HF : return hrv.hf;
        case HrvMeasure::RECOVERY_POINTS : return hrv.recovery_points;
    }
}
