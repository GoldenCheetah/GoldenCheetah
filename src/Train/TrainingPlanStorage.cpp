#include "TrainingPlan.h"

#include "Athlete.h"
#include "Context.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>

QString TrainingPlan::planDirectory(Context *context)
{
    if (!context || !context->athlete || !context->athlete->home)
        return QString();
    return context->athlete->home->config().absolutePath()
           + QStringLiteral("/plans");
}

bool TrainingPlan::save(Context *context, QString *savedPath, QStringList *errors) const
{
    QString dirPath = planDirectory(context);
    if (dirPath.isEmpty()) {
        if (errors) *errors << QStringLiteral("Invalid context or athlete home");
        return false;
    }

    QDir dir(dirPath);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (errors) *errors << QStringLiteral("Could not create plans directory");
        return false;
    }

    QString effectiveId = id.isEmpty() ? generateId() : id;
    QString filename = QStringLiteral("plan_%1.json").arg(effectiveId);
    QString filepath = dir.absoluteFilePath(filename);

    QJsonDocument doc(toJson());
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errors) *errors << QStringLiteral("Could not open plan file for writing: %1").arg(filepath);
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    if (savedPath) *savedPath = filepath;
    return true;
}

TrainingPlan TrainingPlan::load(const QString &filepath, bool *ok)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (ok) *ok = false;
        return TrainingPlan();
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (ok) *ok = false;
        return TrainingPlan();
    }

    return fromJson(doc.object(), ok);
}

QList<TrainingPlan> TrainingPlan::listAll(Context *context)
{
    QList<TrainingPlan> result;
    QString dirPath = planDirectory(context);
    if (dirPath.isEmpty()) return result;

    QDir dir(dirPath);
    if (!dir.exists()) return result;

    QStringList filters;
    filters << QStringLiteral("plan_*.json");
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const auto &fi : files) {
        bool ok = false;
        TrainingPlan plan = load(fi.absoluteFilePath(), &ok);
        if (ok)
            result.append(plan);
    }

    return result;
}

bool TrainingPlan::remove(Context *context, const QString &planId, QStringList *errors)
{
    QString dirPath = planDirectory(context);
    if (dirPath.isEmpty()) {
        if (errors) *errors << QStringLiteral("Invalid context or athlete home");
        return false;
    }

    QDir dir(dirPath);
    QString filename = QStringLiteral("plan_%1.json").arg(planId);
    QString filepath = dir.absoluteFilePath(filename);

    if (!QFile::exists(filepath)) {
        if (errors) *errors << QStringLiteral("Plan not found: %1").arg(planId);
        return false;
    }

    if (!QFile::remove(filepath)) {
        if (errors) *errors << QStringLiteral("Could not delete plan file: %1").arg(filepath);
        return false;
    }

    return true;
}
