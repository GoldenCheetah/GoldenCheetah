/*
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

#include "Measures.h"
#include "Units.h"
#include "MainWindow.h" // for gcroot

#include <QList>
#include <QMessageBox>
#include <QTextStream>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <QDebug>

///////////////////////////// Measure class /////////////////////////////////

quint16
Measure::getFingerprint() const
{
    quint64 x = 0;

    x += when.toSecsSinceEpoch();

    for (int i = 0; i<MAX_MEASURES; i++) x += 1000.0 * values[i];

    QByteArray ba = QByteArray::number(x);
    ba.append(comment.toUtf8());

#if QT_VERSION < 0x060000
    return qChecksum(ba, ba.length());
#else
    return qChecksum(ba);
#endif
}

QString
Measure::getSourceDescription() const
{

    switch(source) {
    case Measure::Manual:
        return tr("Manual entry");
    case Measure::Withings:
        return tr("Withings");
    case Measure::TodaysPlan:
        return tr("Today's Plan");
    case Measure::CSV:
        return tr("CSV Upload");
    default:
        return tr("Unknown");
    }
}

///////////////////////////// MeasuresGroup class ///////////////////////////

MeasuresGroup::MeasuresGroup(QString symbol, QString name, QStringList symbols, QStringList names, QStringList metricUnits, QStringList imperialUnits, QList<double>unitsFactors, QList<QStringList> headers,  QDir dir, bool withData) : dir(dir), withData(withData), symbol(symbol), name(name), symbols(symbols), names(names), metricUnits(metricUnits), imperialUnits(imperialUnits), unitsFactors(unitsFactors), headers(headers)
{
    // don't load data if not requested
    if (!withData) return;

    // get measures if the file exists
    QFile measuresFile(QString("%1/%2measures.json").arg(dir.canonicalPath()).arg(symbol.toLower()));
    if (measuresFile.exists()) {
        QList<Measure> measures;
        if (unserialize(measuresFile, measures)){
            setMeasures(measures);
        }
    }
}

void
MeasuresGroup::write()
{
    // Nothing to do if data not loaded
    if (!withData) return;

    // now save data away
    serialize(QString("%1/%2measures.json").arg(dir.canonicalPath()).arg(symbol.toLower()), measures_);
}

void
MeasuresGroup::setMeasures(QList<Measure>&x)
{
    measures_ = x;
    std::sort(measures_.begin(), measures_.end()); // date order
}

QDate
MeasuresGroup::getStartDate() const
{
    if (!measures_.isEmpty())
        return measures_.first().when.date();
    else
        return QDate();
}

QDate
MeasuresGroup::getEndDate() const
{
    if (!measures_.isEmpty())
        return measures_.last().when.date();
    else
        return QDate();
}

void
MeasuresGroup::getMeasure(QDate date, Measure &here) const
{
    // always set to not found before searching
    here = Measure();

    // loop
    foreach(Measure x, measures_) {

        if (symbol == "Body" && x.when.date() < date) here = x;//TODO generalize
        if (x.when.date() == date) here = x;
        if (x.when.date() > date) break;
    }

    // will be empty if none found
    return;
}

double
MeasuresGroup::getFieldValue(QDate date, int field, bool useMetricUnits) const
{
    Measure measure;
    getMeasure(date, measure);

    // return what was asked for!
    if (field >= 0 && field < MAX_MEASURES)
        return measure.values[field]*(useMetricUnits ? 1.0 : unitsFactors[field]);
    else
        return 0.0;
}

bool
MeasuresGroup::serialize(QString filename, QList<Measure> &data)
{

    // open file - truncate contents
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Saving Measures"));
        msgBox.setInformativeText(QObject::tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return false;
    };
    file.resize(0);
    QTextStream out(&file);
#if QT_VERSION < 0x060000
    out.setCodec("UTF-8");
#endif

    Measure *m = NULL;
    QJsonArray measures;
    for (int i = 0; i < data.count(); i++) {
        m = &data[i];
        QJsonObject measure;
        measure.insert("when", m->when.toMSecsSinceEpoch()/1000);
        measure.insert("comment", m->comment);

        int k = 0;
        foreach (QString field, getFieldSymbols())
            measure.insert(field.toLower(), (k < MAX_MEASURES) ? m->values[k++] : 0);

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
MeasuresGroup::unserialize(QFile &file, QList<Measure> &data)
{

    // open file - truncate contents
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(QObject::tr("Problem Reading Measures"));
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
        msgBox.setText(QObject::tr("Problem Parsing Measures"));
        msgBox.setInformativeText(QObject::tr("File: %1 is not a proper JSON file. Parsing error: %2").arg(file.fileName()).arg(parseError.errorString()));
        msgBox.exec();
        return false;
    }

    data.clear();
    QJsonObject object = document.object();
    QJsonArray measures = object["measures"].toArray();
    for (int i = 0; i < measures.count(); i++) {
        QJsonObject measure = measures.at(i).toObject();
        Measure m;
        m.when = QDateTime::fromMSecsSinceEpoch(measure["when"].toDouble()*1000);
        m.comment = measure["comment"].toString();

        int k = 0;
        foreach (QString field, getFieldSymbols())
            if (measure.contains(field.toLower()))
                if (k < MAX_MEASURES)
                    m.values[k++] = measure[field.toLower()].toDouble();

        m.source = static_cast<Measure::MeasureSource>(measure["source"].toInt());
        m.originalSource = measure["originalsource"].toString();
        data.append(m);
    }

    return true;
}

MeasuresField
MeasuresGroup::getField(int field)
{
    MeasuresField fieldSettings;

    if (field >= 0 && field < symbols.count()) {

        fieldSettings.symbol = symbols[field];
        fieldSettings.name = names[field];
        fieldSettings.metricUnits = metricUnits[field];
        fieldSettings.imperialUnits = imperialUnits[field];
        fieldSettings.unitsFactor = unitsFactors[field];
        fieldSettings.headers = headers[field];
    }

    return fieldSettings;
}

void
MeasuresGroup::addField(MeasuresField &fieldSettings)
{
    if (symbols.count() < MAX_MEASURES && !symbols.contains(fieldSettings.symbol) && !names.contains(fieldSettings.name)) {

        symbols.append(fieldSettings.symbol);
        names.append(fieldSettings.name);
        metricUnits.append(fieldSettings.metricUnits);
        imperialUnits.append(fieldSettings.imperialUnits);
        unitsFactors.append(fieldSettings.unitsFactor);
        headers.append(fieldSettings.headers);
    }
}

void
MeasuresGroup::setField(int field, MeasuresField &fieldSettings)
{
    if (field >= 0 && field < symbols.count() && (symbols[field] == fieldSettings.symbol || !symbols.contains(fieldSettings.symbol))) {

        symbols[field] = fieldSettings.symbol;
        names[field] = fieldSettings.name;
        metricUnits[field] = fieldSettings.metricUnits;
        imperialUnits[field] = fieldSettings.imperialUnits;
        unitsFactors[field] = fieldSettings.unitsFactor;
        headers[field] = fieldSettings.headers;
    }
}

void
MeasuresGroup::removeField(int field)
{
    if (field >= 0 && field < symbols.count()) {
        symbols.removeAt(field);
        names.removeAt(field);
        metricUnits.removeAt(field);
        imperialUnits.removeAt(field);
        unitsFactors.removeAt(field);
        headers.removeAt(field);
    }
}

///////////////////////////// Measures class ////////////////////////////////

Measures::Measures(QDir dir, bool withData) : dir(dir), withData(withData)
{
    // load user defined measures from measures.ini
    QString filename = QDir(gcroot).canonicalPath() + "/measures.ini";

    if (!QFile(filename).exists()) {
        // pre-load mandatory measures in MeasuresGroupType order

        groups.append(new MeasuresGroup("Body", tr("Body"),
            QStringList()<<"WEIGHTKG"<<"FATKG"<<"MUSCLEKG"<<"BONESKG"<<"LEANKG"<<"FATPERCENT",
            QStringList()<<tr("Weight")<<tr("Fat Mass")<<tr("Muscle Mass")<<tr("Bone Mass")<<tr("Lean Mass")<<tr("Fat Percent"),
            QStringList()<<tr("kg")<<tr("kg")<<tr("kg")<<tr("kg")<<tr("kg")<<tr("%"),
            QStringList()<<tr("lbs")<<tr("lbs")<<tr("lbs")<<tr("lbs")<<tr("lbs")<<tr("%"),
            QList<double>()<<LB_PER_KG<<LB_PER_KG<<LB_PER_KG<<LB_PER_KG<<LB_PER_KG<<1.0,
            QList<QStringList>()<<
                (QStringList()<<"weightkg")<<
                (QStringList()<<"fatkg")<<
                (QStringList()<<"musclekg")<<
                (QStringList()<<"boneskg")<<
                (QStringList()<<"leankg")<<
                (QStringList()<<"fatpercent"),
            dir, withData));

        groups.append(new MeasuresGroup("Hrv", tr("Hrv"),
            QStringList()<<"RMSSD"<<"HR"<<"AVNN"<<"SDNN"<<"PNN50"<<"LF"<<"HF"<<"RECOVERY_POINTS",
            QStringList()<<tr("RMSSD")<<tr("HR")<<tr("AVNN")<<tr("SDNN")<<tr("PNN50")<<tr("LF")<<tr("HF")<<tr("Recovery Points"),
            QStringList()<<tr("msec")<<tr("bpm")<<tr("msec")<<tr("msec")<<tr("%")<<tr("msec^2")<<tr("msec^2")<<tr("Rec.Points"),
            QStringList()<<tr("msec")<<tr("bpm")<<tr("msec")<<tr("msec")<<tr("%")<<tr("msec^2")<<tr("msec^2")<<tr("Rec.Points"),
            QList<double>()<<1.0<<1.0<<1.0<<1.0<<1.0<<1.0<<1.0<<1.0,
            QList<QStringList>()<<
                (QStringList()<<"rMSSD"<<"rMSSD_lying"<<"Rmssd")<<
                (QStringList()<<"HR"<<"HR_lying")<<
                (QStringList()<<"AVNN"<<"AVNN_lying")<<
                (QStringList()<<"SDNN"<<"SDNN_lying"<<"Sdnn")<<
                (QStringList()<<"pNN50"<<"pNN50_lying"<<"Pnn50")<<
                (QStringList()<<"LF"<<"LF_lying")<<
                (QStringList()<<"HF"<<"HF_lying")<<
                (QStringList()<<"HRV4T_Recovery_Points"<<"lnRmssd"),
            dir, withData));

            // other standard measures can be loaded from resources
            filename = ":/ini/measures.ini";
    }

    QSettings config(filename, QSettings::IniFormat);

#if QT_VERSION < 0x060000
    config.setIniCodec("UTF-8"); // to allow translated names
#endif

    foreach (QString group, config.value("Measures", "").toStringList()) {

        if (getGroupSymbols().contains(group)) continue;

        config.beginGroup(group);

        QString name = config.value("Name", group).toString();

        QStringList fields = config.value("Fields", "").toStringList();
        if (fields.count() == 0 || fields.count() > MAX_MEASURES) continue;

        QStringList names = config.value("Names", "").toStringList();
        if (names.count() == 1 && names[0] == "") names = fields;
        if (fields.count() != names.count()) continue;

        QStringList units = config.value("MetricUnits", "").toStringList();
        if (units.count() == 1 && units[0] == "") {
            units = QStringList();
            for (int i=0; i<fields.count(); i++) units << "";
        }
        if (fields.count() != units.count()) continue;

        QStringList imperialUnits = config.value("ImperialUnits", units).toStringList();
        if (imperialUnits.count() == 1 && imperialUnits[0] == "") {
            imperialUnits = QStringList();
            for (int i=0; i<fields.count(); i++) imperialUnits << "";
        }
        if (fields.count() != imperialUnits.count()) continue;

        QStringList factors = config.value("UnitsFactors", "").toStringList();
        if (factors.count() == 1 && factors[0] == "") {
            factors = QStringList();
            for (int i=0; i<fields.count(); i++) factors << "1";
        }
        if (fields.count() != factors.count()) continue;
        QList<double> unitsFactors;
        foreach (QString factor, factors) {
            bool ok;
            double unitFactor = factor.toDouble(&ok);
            unitsFactors.append(ok ? unitFactor : 1.0);
        }

        QList<QStringList> headersList;
        foreach (QString field, fields) {
            QStringList headers = config.value(field, "").toStringList();
            headersList.append(headers);
        }

        groups.append(new MeasuresGroup(group, name, fields, names, units, imperialUnits, unitsFactors, headersList, dir, withData));

        config.endGroup();
    }
}

Measures::~Measures()
{
    foreach (MeasuresGroup* measuresGroup, groups)
        delete measuresGroup;
}

void
Measures::saveConfig()
{
    // save measures configuration to measures.ini
    QString filename = QDir(gcroot).canonicalPath() + "/measures.ini";
    QSettings config(filename, QSettings::IniFormat);

#if QT_VERSION < 0x060000
    config.setIniCodec("UTF-8"); // to allow translated names
#endif

    config.clear();

    config.setValue("Measures", getGroupSymbols());

    foreach (MeasuresGroup* group, getGroups()) {

        config.beginGroup(group->getSymbol());

        config.setValue("Name", group->getName());
        config.setValue("Fields", group->getFieldSymbols());
        config.setValue("Names", group->getFieldNames());
        config.setValue("MetricUnits", group->getFieldMetricUnits());
        config.setValue("ImperialUnits", group->getFieldImperialUnits());

        QStringList unitsFactors;
        foreach (double unitsFactor, group->getFieldUnitsFactors())
            unitsFactors<<QString::number(unitsFactor);
        config.setValue("UnitsFactors", unitsFactors);

        for (int i=0; i < group->getFieldSymbols().count(); i++)
            config.setValue(group->getField(i).symbol, group->getFieldHeaders(i));

        config.endGroup();
    }
    config.sync();
}

QStringList
Measures::getGroupSymbols() const
{
    QStringList symbols;
    foreach (MeasuresGroup* measuresGroup, groups)
        symbols.append(measuresGroup->getSymbol());
    return symbols;
}

QStringList
Measures::getGroupNames() const
{
    QStringList names;
    foreach (MeasuresGroup* measuresGroup, groups)
        names.append(measuresGroup->getName());
    return names;
}

MeasuresGroup*
Measures::getGroup(int group)
{
    if (group >= 0 && group < groups.count())
        return groups[group];
    else
        return NULL;
}

void
Measures::removeGroup(int group)
{
    if (group >= 0 && group < groups.count())
        groups.removeAt(group);
}

void
Measures::addGroup(MeasuresGroup* measuresGroup)
{
    if (!getGroupSymbols().contains(measuresGroup->getSymbol()) &&
        !getGroupNames().contains(measuresGroup->getName()))
        groups.append(measuresGroup);
}

QStringList
Measures::getFieldSymbols(int group) const
{
    return groups.at(group) != NULL ? groups.at(group)->getFieldSymbols() : QStringList();
}

QStringList
Measures::getFieldNames(int group) const
{
    return groups.at(group) != NULL ? groups.at(group)->getFieldNames() : QStringList();
}

QDate
Measures::getStartDate(int group) const
{
    return groups.at(group) != NULL ? groups.at(group)->getStartDate() : QDate();
}

QDate
Measures::getEndDate(int group) const
{
    return groups.at(group) != NULL ? groups.at(group)->getEndDate() : QDate();
}

QString
Measures::getFieldUnits(int group, int field, bool useMetricUnits) const
{
    return groups.at(group) != NULL ? groups.at(group)->getFieldUnits(field, useMetricUnits) : QString();
}

double
Measures::getFieldValue(int group, QDate date, int field, bool useMetricUnits) const
{
    return groups.at(group) != NULL ? groups.at(group)->getFieldValue(date, field, useMetricUnits) : 0.0;
}
