/*
 * Copyright 2015 (c) Mark Liversedge (liversedge@gmail.com)
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

#include "APIWebService.h"

#include "Settings.h"
#include "Context.h"
#include "GcUpgrade.h"
#include "RideDB.h"
#include "BanisterSimulator.h"
#include "TrainingSimulator.h"
#include "TrainingPlan.h"
#include "WorkoutGenerationService.h"

#include "RideFile.h"
#include "RideFileCache.h"
#include "CsvRideFile.h"

#include "Zones.h"
#include "HrZones.h"
#include "PaceZones.h"
#include "Measures.h"

#include <QTemporaryFile>
#include <QFile>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMutexLocker>

namespace {

QDate
parseApiDate(const QString &value)
{
    if (value.isEmpty()) {
        return QDate();
    }
    QDate parsed = QDate::fromString(value, Qt::ISODate);
    if (!parsed.isValid()) {
        parsed = QDate::fromString(value, QStringLiteral("yyyy/MM/dd"));
    }
    if (!parsed.isValid()) {
        parsed = QDate::fromString(value, QStringLiteral("yyyy-MM-dd"));
    }
    return parsed;
}

QTime
parseApiTime(const QString &value)
{
    if (value.isEmpty()) {
        return QTime();
    }
    QTime parsed = QTime::fromString(value, Qt::ISODate);
    if (!parsed.isValid()) {
        parsed = QTime::fromString(value, QStringLiteral("hh:mm"));
    }
    if (!parsed.isValid()) {
        parsed = QTime::fromString(value, QStringLiteral("hh:mm:ss"));
    }
    return parsed;
}

QDateTime
parseApiDateTime(const QString &whenValue, const QString &dateValue, const QString &timeValue)
{
    QString cleanedWhen = whenValue.trimmed();
    if (!cleanedWhen.isEmpty()) {
        QDateTime parsed = QDateTime::fromString(cleanedWhen, Qt::ISODate);
        if (!parsed.isValid()) {
            parsed = QDateTime::fromString(cleanedWhen, Qt::ISODateWithMs);
        }
        if (parsed.isValid()) {
            return parsed;
        }
    }

    QDate date = parseApiDate(dateValue);
    if (!date.isValid()) {
        return QDateTime();
    }

    QTime time = parseApiTime(timeValue);
    if (!time.isValid()) {
        time = QTime(6, 0, 0);
    }
    return QDateTime(date, time);
}

QJsonArray
toJsonArray(const QStringList &values)
{
    QJsonArray array;
    for (const QString &value : values) {
        array.append(value);
    }
    return array;
}

}

void
APIWebService::writeJson(HttpResponse &response, const QJsonDocument &document, int statusCode, const QByteArray &reason)
{
    response.setStatus(statusCode, reason);
    response.setHeader("Content-Type", "application/json; charset=UTF-8");
    response.write(document.toJson(QJsonDocument::Compact), true);
}

void
APIWebService::writeJsonError(HttpResponse &response, int statusCode, const QString &message, const QByteArray &reason)
{
    QJsonObject object;
    object.insert(QStringLiteral("error"), message);
    writeJson(response, QJsonDocument(object), statusCode, reason.isEmpty() ? QByteArray("Error") : reason);
}

bool
APIWebService::requireMethod(HttpRequest &request, HttpResponse &response, const QByteArray &method, const QByteArray &allow)
{
    if (request.getMethod().toUpper() != method.toUpper()) {
        response.setHeader("Allow", allow);
        writeJsonError(response, 405,
                       QStringLiteral("Method %1 not allowed").arg(QString::fromLatin1(request.getMethod())),
                       QByteArray("Method Not Allowed"));
        return false;
    }
    return true;
}

bool
APIWebService::requireJsonBody(HttpRequest &request, HttpResponse &response, QJsonDocument &document)
{
    QByteArray contentType = request.getHeader("Content-Type");
    if (contentType.isEmpty()) {
        contentType = request.getHeader("content-type");
    }
    QString loweredType = QString::fromLatin1(contentType).trimmed().toLower();
    if (!loweredType.startsWith(QStringLiteral("application/json"))) {
        writeJsonError(response, 415, QStringLiteral("Content-Type must be application/json"),
                       QByteArray("Unsupported Media Type"));
        return false;
    }

    QByteArray body = request.getBody();
    if (body.isEmpty()) {
        writeJsonError(response, 400, QStringLiteral("Request body is empty"), QByteArray("Bad Request"));
        return false;
    }

    QJsonParseError parseError;
    document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError || document.isNull() || !document.isObject()) {
        writeJsonError(response, 400,
                       QStringLiteral("Invalid JSON body: %1").arg(parseError.errorString()),
                       QByteArray("Bad Request"));
        return false;
    }
    return true;
}

void
APIWebService::service(HttpRequest &request, HttpResponse &response)
{
    // remove trailing '/' from request, just to be consistent
    QString fullPath = request.getPath();
    while (fullPath.endsWith("/")) fullPath.chop(1);

    // get the paths, strip empty stuff
    QStringList paths = QString(request.getPath()).split("/");
    while (paths.count() && paths[paths.count()-1] == "") paths.removeLast();
    while (paths.count() && paths[0] == "") paths.removeFirst();

    // we don't have a fave icon
    if (paths.count() && paths[0] == "favicon.ico") return;

    // ROOT PATH RETURNS A LIST OF ATHLETES
    if (paths.count() == 0) {
        listAthletes(request, response); // return csv list of all athlete and their characteristics
        return;
    }

    // Call to retreive athlete data, downstream will resolve
    // which functions to call for different data requests
    athleteData(paths, request, response);
}

void
APIWebService::athleteData(QStringList &paths, HttpRequest &request, HttpResponse &response)
{

    // check we have an athlete and it is valid
    if (paths.count() == 0) {

        response.setStatus(404); // malformed URL
        response.setHeader("Content-Type", "text; charset=ISO-8859-1");
        response.write("missing athlete.");
        return;
    } else {
        QFile ridedb(home.absolutePath() + "/" + paths[0] + "/cache/rideDB.json");
        if (!ridedb.exists()) {
            response.setStatus(404); // malformed URL
            response.setHeader("Content-Type", "text; charset=ISO-8859-1");
            response.write("unknown athlete " + paths[0].toLocal8Bit());
            return;
        }
    }
    if (paths.count() == 1) {

        // LIST ACTIVITIES FOR ATHLETE
        // http://localhost:12021/athlete
        listRides(paths[0], request, response);
        return;

    } else if (paths.count() == 2) {

        QString athlete = paths[0];
        paths.removeFirst();

        // GET ZONES
        // http://localhost:12021/athlete/zones
        if (paths[0] == "zones") {
            listZones(athlete, paths, request, response);
            return;
        }

        // GET Measures
        if (paths[0] == "measures") {

            // http://localhost:12021/athlete/measures
            paths.removeFirst();
            listMeasures(athlete, paths, request, response);
            return;
        }

    } else if (paths.count() == 3) {

        QString athlete = paths[0];
        paths.removeFirst();

        if (paths[0] == "ai") {
            paths.removeFirst();
            aiEndpoint(athlete, paths, request, response);
            return;
        }

        // GET ACTIVITY
        // http://localhost:12021/athlete/activity/filename
        // optional query parameters:
        //      ?format=json    (default)
        //      ?format=<xx>    xx = one of (csv, tcx, pwx)
        if (paths[0] == "activity") {

            paths.removeFirst();
            listActivity(athlete, paths, request, response);
            return;
        }

        // GET MMP
        if (paths[0] == "meanmax") {

            // http://localhost:12021/athlete/meanmax/filename
            // optional query parameter:
            //    ?series=watts     (default)
            //    ?series=<xx>  xx= one of (cad, speed, vam, IsoPower, xPower, nm)
            // http://localhost:12021/athlete/meanmax/bests
            // optional query parameter:
            //    ?series=watts     (default)
            //    ?series=<xx>  xx=1 of (cad, speed, vam, IsoPower, xPower, nm)
            paths.removeFirst();
            listMMP(athlete, paths, request, response);
            return;
        }

        // GET Measures
        if (paths[0] == "measures") {

            // http://localhost:12021/athlete/measures/Body
            // http://localhost:12021/athlete/measures/Hrv
            paths.removeFirst();
            listMeasures(athlete, paths, request, response);
            return;
        }

     }

    // GET HERE ITS BAD!
    response.setStatus(404); // malformed URL
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");
    response.write("malformed url");
}

void
APIWebService::aiEndpoint(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response)
{
    QMutexLocker locker(&aiMutex);

    if (paths.isEmpty()) {
        writeJsonError(response, 404, QStringLiteral("Missing AI endpoint"), QByteArray("Not Found"));
        return;
    }
    if (paths[0] == "snapshot") {
        aiSnapshot(athlete, request, response);
        return;
    }
    if (paths[0] == "draft") {
        aiDraft(athlete, request, response);
        return;
    }
    if (paths[0] == "save") {
        aiSave(athlete, request, response);
        return;
    }
    if (paths[0] == "plan") {
        aiPlan(athlete, request, response);
        return;
    }
    if (paths[0] == "simulate") {
        aiSimulate(athlete, request, response);
        return;
    }
    if (paths[0] == "banister") {
        aiBanister(athlete, request, response);
        return;
    }
    if (paths[0] == "plans") {
        QStringList subPaths = paths.mid(1);
        aiPlans(athlete, subPaths, request, response);
        return;
    }

    writeJsonError(response, 404, QStringLiteral("Unknown AI endpoint"), QByteArray("Not Found"));
}

void
APIWebService::aiSnapshot(QString athlete, HttpRequest &request, HttpResponse &response)
{
    if (!requireMethod(request, response, "GET", "GET")) {
        return;
    }

    Context *context = Context::findAthleteContext(athlete);
    if (context == NULL) {
        writeJsonError(response, 409,
                       QStringLiteral("AI endpoints currently require athlete '%1' to be open in GoldenCheetah").arg(athlete),
                       QByteArray("Conflict"));
        return;
    }

    WorkoutAthleteSnapshot snapshot =
        WorkoutGenerationService::athleteSnapshot(context, parseApiDate(QString::fromLatin1(request.getParameter("date"))));

    QJsonObject object;
    object.insert(QStringLiteral("snapshot"), snapshot.toJson());
    object.insert(QStringLiteral("generatorId"), QStringLiteral("goldencheetah-heuristic-v1"));
    object.insert(QStringLiteral("supportedWorkoutTypes"),
                  toJsonArray(QStringList()
                              << QStringLiteral("recovery")
                              << QStringLiteral("endurance")
                              << QStringLiteral("sweetspot")
                              << QStringLiteral("threshold")
                              << QStringLiteral("vo2max")
                              << QStringLiteral("anaerobic")
                              << QStringLiteral("mixed")));
    writeJson(response, QJsonDocument(object));
}

void
APIWebService::aiDraft(QString athlete, HttpRequest &request, HttpResponse &response)
{
    if (!requireMethod(request, response, "POST", "POST")) {
        return;
    }

    QJsonDocument document;
    if (!requireJsonBody(request, response, document)) {
        return;
    }

    Context *context = Context::findAthleteContext(athlete);
    if (context == NULL) {
        writeJsonError(response, 409,
                       QStringLiteral("AI endpoints currently require athlete '%1' to be open in GoldenCheetah").arg(athlete),
                       QByteArray("Conflict"));
        return;
    }

    QString parseError;
    bool ok = false;
    WorkoutHeuristicRequest generationRequest =
        WorkoutHeuristicRequest::fromJson(document.object(), &ok, &parseError);
    if (!ok) {
        writeJsonError(response, 400, parseError, QByteArray("Bad Request"));
        return;
    }

    WorkoutAthleteSnapshot snapshot = WorkoutGenerationService::athleteSnapshot(context, generationRequest.date);
    if (!snapshot.canGenerate()) {
        writeJsonError(response, 422,
                       QStringLiteral("Bike FTP/CP is not configured for the requested date"),
                       QByteArray("Unprocessable Entity"));
        return;
    }

    QStringList warnings;
    WorkoutDraft draft = WorkoutGenerationService::generateHeuristicDraft(snapshot, generationRequest, &warnings);

    QJsonObject object;
    object.insert(QStringLiteral("snapshot"), snapshot.toJson());
    object.insert(QStringLiteral("draft"), draft.toJson());
    object.insert(QStringLiteral("warnings"), toJsonArray(warnings));
    writeJson(response, QJsonDocument(object));
}

void
APIWebService::aiSave(QString athlete, HttpRequest &request, HttpResponse &response)
{
    if (!requireMethod(request, response, "POST", "POST")) {
        return;
    }

    QJsonDocument document;
    if (!requireJsonBody(request, response, document)) {
        return;
    }

    Context *context = Context::findAthleteContext(athlete);
    if (context == NULL) {
        writeJsonError(response, 409,
                       QStringLiteral("AI endpoints currently require athlete '%1' to be open in GoldenCheetah").arg(athlete),
                       QByteArray("Conflict"));
        return;
    }

    QJsonObject root = document.object();
    QJsonObject draftObject = root;
    if (root.contains(QStringLiteral("draft")) && root.value(QStringLiteral("draft")).isObject()) {
        draftObject = root.value(QStringLiteral("draft")).toObject();
    }

    bool ok = false;
    WorkoutDraft draft = WorkoutDraft::fromJson(draftObject, &ok);
    if (!ok) {
        writeJsonError(response, 400, QStringLiteral("Invalid WorkoutDraft payload"), QByteArray("Bad Request"));
        return;
    }

    QStringList errors;
    QString savedPath;
    bool saved = WorkoutGenerationService::saveDraft(
        context,
        draft,
        parseApiDate(root.value(QStringLiteral("date")).toString()),
        &savedPath,
        errors);
    if (!saved) {
        writeJsonError(response, 422, errors.join(QStringLiteral("; ")), QByteArray("Unprocessable Entity"));
        return;
    }

    QJsonObject object;
    object.insert(QStringLiteral("saved"), true);
    object.insert(QStringLiteral("filepath"), savedPath);
    object.insert(QStringLiteral("displayName"), draft.displayName);
    object.insert(QStringLiteral("generatorId"), draft.generatorId);
    writeJson(response, QJsonDocument(object));
}

void
APIWebService::aiPlan(QString athlete, HttpRequest &request, HttpResponse &response)
{
    QString method = QString::fromLatin1(request.getMethod()).toUpper();

    if (method != QLatin1String("POST") && method != QLatin1String("DELETE")) {
        writeJsonError(response, 405, QStringLiteral("Method not allowed"), QByteArray("Method Not Allowed"));
        response.setHeader("Allow", "POST, DELETE");
        return;
    }

    Context *context = Context::findAthleteContext(athlete);
    if (context == NULL) {
        writeJsonError(response, 409,
                       QStringLiteral("AI endpoints currently require athlete '%1' to be open in GoldenCheetah").arg(athlete),
                       QByteArray("Conflict"));
        return;
    }

    // DELETE /api/athlete/{name}/ai/plan — remove a planned activity by filepath
    if (method == QLatin1String("DELETE")) {
        QJsonDocument document;
        if (!requireJsonBody(request, response, document)) {
            return;
        }
        QString filepath = document.object().value(QStringLiteral("filepath")).toString().trimmed();
        if (filepath.isEmpty()) {
            writeJsonError(response, 400, QStringLiteral("filepath is required"), QByteArray("Bad Request"));
            return;
        }

        QStringList errors;
        if (!WorkoutGenerationService::deletePlannedActivity(context, filepath, errors)) {
            writeJsonError(response, 404, errors.join(QStringLiteral("; ")), QByteArray("Not Found"));
            return;
        }

        QJsonObject object;
        object.insert(QStringLiteral("deleted"), true);
        object.insert(QStringLiteral("filepath"), filepath);
        writeJson(response, QJsonDocument(object));
        return;
    }

    // POST /api/athlete/{name}/ai/plan — create a planned activity
    QJsonDocument document;
    if (!requireJsonBody(request, response, document)) {
        return;
    }

    QJsonObject root = document.object();
    QString workoutPath = root.value(QStringLiteral("workoutPath")).toString().trimmed();
    if (workoutPath.isEmpty()) {
        workoutPath = root.value(QStringLiteral("filepath")).toString().trimmed();
    }
    if (workoutPath.isEmpty()) {
        writeJsonError(response, 400, QStringLiteral("workoutPath is required"), QByteArray("Bad Request"));
        return;
    }

    QDateTime when = parseApiDateTime(root.value(QStringLiteral("when")).toString(),
                                      root.value(QStringLiteral("date")).toString(),
                                      root.value(QStringLiteral("time")).toString());
    if (!when.isValid()) {
        writeJsonError(response, 400, QStringLiteral("A valid when or date is required"), QByteArray("Bad Request"));
        return;
    }

    QStringList errors;
    QString savedPath;
    bool planned = WorkoutGenerationService::createPlannedActivity(
        context,
        workoutPath,
        when,
        root.value(QStringLiteral("sport")).toString(),
        root.value(QStringLiteral("title")).toString(),
        root.value(QStringLiteral("description")).toString(),
        &savedPath,
        errors);
    if (!planned) {
        writeJsonError(response, 422, errors.join(QStringLiteral("; ")), QByteArray("Unprocessable Entity"));
        return;
    }

    QJsonObject object;
    object.insert(QStringLiteral("saved"), true);
    object.insert(QStringLiteral("filepath"), savedPath);
    object.insert(QStringLiteral("workoutPath"), workoutPath);
    object.insert(QStringLiteral("when"), when.toString(Qt::ISODate));
    writeJson(response, QJsonDocument(object));
}

void
APIWebService::aiSimulate(QString athlete, HttpRequest &request, HttpResponse &response)
{
    if (!requireMethod(request, response, "POST", "GET")) {
        return;
    }

    Context *context = Context::findAthleteContext(athlete);
    if (context == NULL) {
        writeJsonError(response, 409,
                       QStringLiteral("AI endpoints currently require athlete '%1' to be open in GoldenCheetah").arg(athlete),
                       QByteArray("Conflict"));
        return;
    }

    // Parse parameters from either query string (GET) or JSON body (POST)
    QString goalStr, dateStr;
    int durationMin = 60;

    if (request.getMethod() == "POST") {
        QJsonDocument document;
        if (requireJsonBody(request, response, document)) {
            QJsonObject root = document.object();
            goalStr = root.value(QStringLiteral("goal")).toString();
            dateStr = root.value(QStringLiteral("date")).toString();
            durationMin = root.value(QStringLiteral("durationMin")).toInt(60);
        } else {
            return;
        }
    } else {
        goalStr = QString::fromLatin1(request.getParameter("goal"));
        dateStr = QString::fromLatin1(request.getParameter("date"));
        QString durStr = QString::fromLatin1(request.getParameter("durationMin"));
        if (!durStr.isEmpty()) durationMin = durStr.toInt();
    }

    TrainingGoal goal = trainingGoalFromString(goalStr);
    QDate when = parseApiDate(dateStr);

    WorkoutAthleteSnapshot snapshot = WorkoutGenerationService::athleteSnapshot(context, when);
    if (!snapshot.hasSimulationData()) {
        writeJsonError(response, 422,
                       QStringLiteral("Insufficient training data for simulation (no CTL/ATL history)"),
                       QByteArray("Unprocessable Entity"));
        return;
    }

    SimulationRanking ranking = TrainingSimulator::rankCandidates(snapshot, goal, durationMin);

    QJsonObject object;
    object.insert(QStringLiteral("snapshot"), snapshot.toJson());
    object.insert(QStringLiteral("ranking"), ranking.toJson());
    writeJson(response, QJsonDocument(object));
}

void
APIWebService::aiBanister(QString athlete, HttpRequest &request, HttpResponse &response)
{
    if (!requireMethod(request, response, "POST", "GET")) {
        return;
    }

    Context *context = Context::findAthleteContext(athlete);
    if (context == NULL) {
        writeJsonError(response, 409,
                       QStringLiteral("AI endpoints currently require athlete '%1' to be open in GoldenCheetah").arg(athlete),
                       QByteArray("Conflict"));
        return;
    }

    // Parse parameters
    QString dateStr, metricStr;
    int days = 28;
    double weeklyTSS = 0;

    if (request.getMethod() == "POST") {
        QJsonDocument document;
        if (requireJsonBody(request, response, document)) {
            QJsonObject root = document.object();
            dateStr = root.value(QStringLiteral("date")).toString();
            metricStr = root.value(QStringLiteral("metric")).toString();
            days = root.value(QStringLiteral("days")).toInt(28);
            weeklyTSS = root.value(QStringLiteral("weeklyTSS")).toDouble(0);
        } else {
            return;
        }
    } else {
        dateStr = QString::fromLatin1(request.getParameter("date"));
        metricStr = QString::fromLatin1(request.getParameter("metric"));
        QString daysStr = QString::fromLatin1(request.getParameter("days"));
        QString tssStr = QString::fromLatin1(request.getParameter("weeklyTSS"));
        if (!daysStr.isEmpty()) days = daysStr.toInt();
        if (!tssStr.isEmpty()) weeklyTSS = tssStr.toDouble();
    }

    if (metricStr.isEmpty()) metricStr = QStringLiteral("coggan_tss");
    QDate when = parseApiDate(dateStr);

    // Extract fitted parameters
    BanisterParams params = BanisterSimulator::extractParams(context->athlete, metricStr, when);

    QJsonObject object;
    object.insert(QStringLiteral("params"), params.toJson());

    // If weeklyTSS provided, compare plan templates
    if (weeklyTSS > 0) {
        QJsonArray comparisons;
        QStringList templates = BanisterSimulator::planTemplateNames();
        for (int i = 0; i < templates.size(); i++) {
            QVector<double> plan = BanisterSimulator::buildPlanTemplate(templates[i], weeklyTSS, days);
            BanisterProjection proj = BanisterSimulator::projectForward(params, plan, when);

            QJsonObject planObj;
            planObj[QStringLiteral("template")] = templates[i];
            planObj[QStringLiteral("projection")] = proj.toJson();
            comparisons.append(planObj);
        }
        object.insert(QStringLiteral("planComparisons"), comparisons);
    }

    writeJson(response, QJsonDocument(object));
}

void
APIWebService::aiPlans(QString athlete, QStringList subPaths, HttpRequest &request, HttpResponse &response)
{
    Context *context = Context::findAthleteContext(athlete);
    if (context == NULL) {
        writeJsonError(response, 409,
                       QStringLiteral("AI endpoints currently require athlete '%1' to be open in GoldenCheetah").arg(athlete),
                       QByteArray("Conflict"));
        return;
    }

    QString method = QString::fromLatin1(request.getMethod());

    // GET /ai/plans — list all plans
    if (subPaths.isEmpty() && method == QLatin1String("GET")) {
        QList<TrainingPlan> plans = TrainingPlan::listAll(context);
        QJsonArray arr;
        for (const auto &plan : plans)
            arr.append(plan.toJson());
        QJsonObject object;
        object.insert(QStringLiteral("plans"), arr);
        writeJson(response, QJsonDocument(object));
        return;
    }

    // POST /ai/plans — create a new plan
    if (subPaths.isEmpty() && method == QLatin1String("POST")) {
        QJsonDocument document;
        if (!requireJsonBody(request, response, document)) return;

        bool ok = false;
        QJsonObject root = document.object();

        // Assign id and timestamps if missing
        if (!root.contains(QStringLiteral("id")) || root.value(QStringLiteral("id")).toString().isEmpty())
            root[QStringLiteral("id")] = TrainingPlan::generateId();
        if (!root.contains(QStringLiteral("createdAt")))
            root[QStringLiteral("createdAt")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        root[QStringLiteral("updatedAt")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

        TrainingPlan plan = TrainingPlan::fromJson(root, &ok);
        if (!ok) {
            writeJsonError(response, 400, QStringLiteral("Invalid TrainingPlan payload (name and id are required)"),
                           QByteArray("Bad Request"));
            return;
        }

        QStringList errors;
        QString savedPath;
        if (!plan.save(context, &savedPath, &errors)) {
            writeJsonError(response, 422, errors.join(QStringLiteral("; ")), QByteArray("Unprocessable Entity"));
            return;
        }

        QJsonObject object;
        object.insert(QStringLiteral("saved"), true);
        object.insert(QStringLiteral("filepath"), savedPath);
        object.insert(QStringLiteral("plan"), plan.toJson());
        writeJson(response, QJsonDocument(object), 201, QByteArray("Created"));
        return;
    }

    // Routes with a plan ID: /ai/plans/{id}
    if (subPaths.size() == 1) {
        QString planId = subPaths[0];

        // GET /ai/plans/{id}
        if (method == QLatin1String("GET")) {
            QList<TrainingPlan> plans = TrainingPlan::listAll(context);
            for (const auto &plan : plans) {
                if (plan.id == planId) {
                    QJsonObject object;
                    object.insert(QStringLiteral("plan"), plan.toJson());
                    writeJson(response, QJsonDocument(object));
                    return;
                }
            }
            writeJsonError(response, 404, QStringLiteral("Plan not found: %1").arg(planId),
                           QByteArray("Not Found"));
            return;
        }

        // PUT /ai/plans/{id}
        if (method == QLatin1String("PUT")) {
            QJsonDocument document;
            if (!requireJsonBody(request, response, document)) return;

            QJsonObject root = document.object();
            root[QStringLiteral("id")] = planId;
            root[QStringLiteral("updatedAt")] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

            // Preserve createdAt from existing plan if not in payload
            if (!root.contains(QStringLiteral("createdAt"))) {
                QList<TrainingPlan> existing = TrainingPlan::listAll(context);
                for (const auto &p : existing) {
                    if (p.id == planId && p.createdAt.isValid()) {
                        root[QStringLiteral("createdAt")] = p.createdAt.toString(Qt::ISODate);
                        break;
                    }
                }
            }

            bool ok = false;
            TrainingPlan plan = TrainingPlan::fromJson(root, &ok);
            if (!ok) {
                writeJsonError(response, 400, QStringLiteral("Invalid TrainingPlan payload"),
                               QByteArray("Bad Request"));
                return;
            }

            QStringList errors;
            QString savedPath;
            if (!plan.save(context, &savedPath, &errors)) {
                writeJsonError(response, 422, errors.join(QStringLiteral("; ")),
                               QByteArray("Unprocessable Entity"));
                return;
            }

            QJsonObject object;
            object.insert(QStringLiteral("saved"), true);
            object.insert(QStringLiteral("filepath"), savedPath);
            object.insert(QStringLiteral("plan"), plan.toJson());
            writeJson(response, QJsonDocument(object));
            return;
        }

        // DELETE /ai/plans/{id}
        if (method == QLatin1String("DELETE")) {
            QStringList errors;
            if (!TrainingPlan::remove(context, planId, &errors)) {
                writeJsonError(response, 404, errors.join(QStringLiteral("; ")),
                               QByteArray("Not Found"));
                return;
            }

            QJsonObject object;
            object.insert(QStringLiteral("deleted"), true);
            object.insert(QStringLiteral("id"), planId);
            writeJson(response, QJsonDocument(object));
            return;
        }
    }

    writeJsonError(response, 404, QStringLiteral("Unknown plans endpoint"), QByteArray("Not Found"));
}

void
APIWebService::listAthletes(HttpRequest &, HttpResponse &response)
{
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");

    // This will read the user preferences and change the file list order as necessary:
    QFlags<QDir::Filter> spec = QDir::Dirs;
    QStringList names;
    names << "*"; // anything

    response.write("name,dob,weight,height,sex\n");
    foreach(QString name, home.entryList(names, spec, QDir::Name)) {

        // sure fire sign the athlete has been upgraded to post 3.2 and not some
        // random directory full of other things & check something basic is set
        QString ridedb = home.absolutePath() + "/" + name + "/cache/rideDB.json";
        if (QFile(ridedb).exists()) {
            // we need to initialize athlete settings for cvalue to work
            appsettings->initializeQSettingsAthlete(home.absolutePath(), name);
            if (appsettings->cvalue(name, GC_SEX, "") == "") continue;

            // we got one
            QString line = name;
            line += ", " + appsettings->cvalue(name, GC_DOB).toDate().toString("yyyy/MM/dd");
            line += ", " + QString("%1").arg(appsettings->cvalue(name, GC_WEIGHT).toDouble());
            line += ", " + QString("%1").arg(appsettings->cvalue(name, GC_HEIGHT).toDouble());
            line += (appsettings->cvalue(name, GC_SEX).toInt() == 0) ? ", Male" : ", Female";
            line += "\n";

            // out a line
            response.write(line.toLocal8Bit());
        }
    }
}


void 
APIWebService::writeRideLine(RideItem &item, HttpRequest *request, HttpResponse *response)
{

    // honour the since parameter
    QString sincep(request->getParameter("since"));
    QDate since(1900,01,01);
    if (sincep != "") since = QDate::fromString(sincep,"yyyy/MM/dd");

    // before parameter
    QString beforep(request->getParameter("before"));
    QDate before(3000,01,01);
    if (beforep != "") before = QDate::fromString(beforep,"yyyy/MM/dd");

    // in range?
    if (item.dateTime.date() < since) return;
    if (item.dateTime.date() > before) return;

    // are we doing rides or intervals?
    listRideSettings *settings = static_cast<listRideSettings *>(response->userData());

    if (settings->intervals == true) {

        // loop through all available intervals for this ride item
        foreach(IntervalItem *interval, item.intervals()){ 

            // date, time, filename
            response->bwrite(item.dateTime.date().toString("yyyy/MM/dd").toLocal8Bit());
            response->bwrite(", ");
            response->bwrite(item.dateTime.time().toString("hh:mm:ss").toLocal8Bit());;
            response->bwrite(", ");
            response->bwrite(item.fileName.toLocal8Bit());

            // now the interval name and type
            response->bwrite(", \"");
            response->bwrite(interval->name.toLocal8Bit());
            response->bwrite("\", ");
            response->bwrite(QString("%1").arg(static_cast<int>(interval->type)).toLocal8Bit());

            // essentially the same as below .. cut and paste (refactor?XXX)
            if (settings->wanted.count()) {
                // specific metrics
                foreach(int index, settings->wanted) {
                    double value = interval->metrics()[index];
                    response->bwrite(",");
                    response->bwrite(QString("%1").arg(value, 'f').simplified().toLocal8Bit());
                }
            } else {
    
                // all metrics...
                foreach(double value, interval->metrics()) {
                    response->bwrite(",");
                    response->bwrite(QString("%1").arg(value, 'f').simplified().toLocal8Bit());
                }
            }
            response->bwrite("\n");
        }

    } else {

        // date, time, filename
        response->bwrite(item.dateTime.date().toString("yyyy/MM/dd").toLocal8Bit());
        response->bwrite(",");
        response->bwrite(item.dateTime.time().toString("hh:mm:ss").toLocal8Bit());;
        response->bwrite(",");
        response->bwrite(item.fileName.toLocal8Bit());

        if (settings->wanted.count()) {
            // specific metrics
            foreach(int index, settings->wanted) {
                double value = item.metrics()[index];
                response->bwrite(",");
                response->bwrite(QString("%1").arg(value, 'f').simplified().toLocal8Bit());
            }
        } else {
    
            // all metrics...
            foreach(double value, item.metrics()) {
                response->bwrite(",");
                response->bwrite(QString("%1").arg(value, 'f').simplified().toLocal8Bit());
            }
        }

        // all the metadata asked for
        foreach(QString name, settings->metawanted) {
            QString text = item.getText(name,"");
            text.replace("\"","'");   // don't use double quotes...
            text.replace("\n","\\n"); // newlines
            text.replace("\r","\\r"); // carriage returns
            text.replace("\t","\\t"); // tabs

            response->bwrite(",\"");
            response->bwrite(text.toLocal8Bit());
            response->bwrite("\"");
        }

        response->bwrite("\n");
    }
}

void
APIWebService::listActivity(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response)
{
    // does it exist ?
    QString filename = QString("%1/%2/activities/%3").arg(home.absolutePath()).arg(athlete).arg(paths[0]);

    QString contents;
    QFile file(filename);
    if (file.exists() && file.open(QFile::ReadOnly | QFile::Text)) {

        // close as we will open properly below
        file.close();

        // what format to use ?
        QString format(request.getParameter("format"));
        if (format == "") {

            // if not passed in the URL then is content type
            // caller can accept listed in the header?
            // there is probably a more complete way of handling
            // wildcards etc, but the user can always force via     
            // the format parameter in the URL
            foreach(QByteArray accepts, request.getHeaders("Accept")) {
                if (accepts == "application/json") format="json";
                if (accepts == "text/csv") format="csv";
                if (accepts == "application/vnd.garmin.tcx") format="tcx";
                if (accepts == "application/vnd.trainingpeaks.pwx") format="pwx";
                if (accepts == "application/xml" || accepts == "text/xml") format="tcx";
                if (format != "") break;
            }
        }

        // default to json
        if (format == "") format = "json";

        // lets go with tcx/pwx as xml, full csv (not powertap) and GC json
        QStringList formats;
        formats << "tcx"; // garmin training centre
        formats << "csv"; // full csv list (not powertap)
        formats << "json"; // gc json
        formats << "pwx"; // gc json

        // unsupported format
        if (!formats.contains(format)) {
            response.setStatus(500);
            response.write("unsupported format; we support:");
            foreach(QString fmt, formats) {
                response.write(" ");
                response.write(fmt.toLocal8Bit());
            }
            response.write("\r\n");
            return;
        } else {

            // set the content type appropriately
            if (format == "tcx") response.setHeader("Content-Type", "application/vnd.garmin.tcx+xml; charset=ISO-8859-1");
            if (format == "csv") response.setHeader("Content-Type", "text/csv; charset=ISO-8859-1");
            if (format == "json") response.setHeader("Content-Type", "application/json; charset=ISO-8859-1");
            if (format == "pwx") response.setHeader("Content-Type", "application/vnd.trainingpeaks.pwx+xml; charset=ISO-8859-1");
        }

        // lets read the file in as a ridefile
        QStringList errors;
        RideFile *f = RideFileFactory::instance().openRideFile(NULL, file, errors);

        // error reading (!)
        if (f == NULL) {
            response.setStatus(500);
            foreach(QString error, errors) {
                response.write(error.toLocal8Bit());
                response.write("\r\n");
            }
            return;
        }

        // write out to a temporary file in
        // the format requested
        bool success;
        QTemporaryFile tempfile; // deletes file when goes out of scope
        QString tempname;
        if (tempfile.open()) tempname = tempfile.fileName();
        else {
            response.setStatus(500);
            response.write("error opening temporary file");
            return;
        }
        QFile out(tempname);

        if (format == "csv") {
            CsvFileReader writer;
            success = writer.writeRideFile(NULL, f, out, CsvFileReader::gc);
        } else {
            success = RideFileFactory::instance().writeRideFile(NULL, f, out, format);
        }

        if (success) {

            // read in the whole thing
            out.open(QFile::ReadOnly | QFile::Text);
            QTextStream in(&out);
            contents = in.readAll();
            out.close();

            // write back in one hit
            response.write(contents.toLocal8Bit(), true);
            return;

        } else {
            response.setStatus(500);
            response.write("unable to write output, internal error.\n");
            return;
        }

    } else {

       // nope?
       response.setStatus(404);
       response.write("file not found");
       return;
    }
}

void
APIWebService::listMMP(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response)
{
    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");

    // what series do we want ?
    QString seriesp = request.getParameter("series");
    if (seriesp == "") seriesp = "watts";
    RideFile::SeriesType series;

    // what asked for ?
    if (seriesp == "hr") series = RideFile::hr;
    else if (seriesp == "cad") series = RideFile::cad;
    else if (seriesp == "speed") series = RideFile::kph;
    else if (seriesp == "watts") series = RideFile::watts;
    else if (seriesp == "vam") series = RideFile::vam;
    else if (seriesp == "IsoPower") series = RideFile::IsoPower;
    else if (seriesp == "xPower") series = RideFile::xPower;
    else if (seriesp == "nm") series = RideFile::nm;
    else {

        // unknown series
        response.setStatus(500);
        response.write("unknown series requested.\n");
        return;
    }

    QString filename=paths[0];

    if (paths[0] == "bests") {

        // header
        response.bwrite("secs, ");
        response.bwrite(seriesp.toLocal8Bit());
        response.bwrite("\n");

        // honour the since parameter
        QString sincep(request.getParameter("since"));
        QDate since(1900,01,01);
        if (sincep != "") since = QDate::fromString(sincep,"yyyy/MM/dd");

        // before parameter
        QString beforep(request.getParameter("before"));
        QDate before(3000,01,01);
        if (beforep != "") before = QDate::fromString(beforep,"yyyy/MM/dd");

        int secs=0;
        foreach(float value, RideFileCache::meanMaxFor(home.absolutePath() + "/" + athlete + "/cache", series, since, before)) {
            if (secs >0) response.bwrite(QString("%1, %2\n").arg(secs).arg(value).toLocal8Bit());
            secs++;
        }


    } else {
        QString CPXfilename = home.absolutePath() + "/" + athlete + "/cache/" + QFileInfo(filename).completeBaseName() + ".cpx";

        // header
        response.bwrite("secs, ");
        response.bwrite(seriesp.toLocal8Bit());
        response.bwrite("\n");

        if (QFileInfo(CPXfilename).exists()) {
            int secs=0;
            foreach(float value, RideFileCache::meanMaxFor(CPXfilename, series)) {
                if (secs >0) response.bwrite(QString("%1, %2\n").arg(secs).arg(value).toLocal8Bit());
                secs++;
            }
        }
        response.flush();
    }
}

void
APIWebService::listZones(QString athlete, QStringList, HttpRequest &request, HttpResponse &response)
{
    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");

    // what zones we support
    QStringList zonelist;
    zonelist << "power" << "hr" << "pace" << "swimpace";

    // what series do we want ?
    QString zonesFor = request.getParameter("for");
    if (zonesFor == "") zonesFor = "power";
    else if (!zonelist.contains(zonesFor)) {
        response.setStatus(404);
        response.write("unknown zones type; one of power, hr, pace and swimpace expected.\n");
        return;
    }

    // power zones
    if (zonesFor == "power") {

        // Power Zones
        QFile zonesFile(home.absolutePath() + "/" + athlete + "/config/power.zones");
        if (zonesFile.exists()) {
            Zones *zones = new Zones;
            if (zones->read(zonesFile)) {

                // success - write out
                response.write("date, cp, w', pmax, aetp, ftp\n");
                for(int i=0; i<zones->getRangeSize(); i++) {
                    response.write(
                    QString("%1, %2, %3, %4, %5, %6\n")
                           .arg(zones->getStartDate(i).toString("yyyy/MM/dd"))
                           .arg(zones->getCP(i))
                           .arg(zones->getWprime(i))
                           .arg(zones->getPmax(i))
                           .arg(zones->getAeT(i))
                           .arg(zones->getFTP(i))
                           .toLocal8Bit()
                    );
                }
                return;
            }
        }

        // drop here on fail
        response.setStatus(500);
        response.write("unable to read/parse the athlete's power.zones file.\n");
        return;
    }

    // hr zones
    if (zonesFor == "hr") {

        // Zones
        QFile zonesFile(home.absolutePath() + "/" + athlete + "/config/hr.zones");
        if (zonesFile.exists()) {
            HrZones *zones = new HrZones;
            if (zones->read(zonesFile)) {

                // success - write out
                response.write("date, lthr, aethr, maxhr, rhr\n");
                for(int i=0; i<zones->getRangeSize(); i++) {
                    response.write(
                    QString("%1, %2, %3, %4, %5\n")
                           .arg(zones->getStartDate(i).toString("yyyy/MM/dd"))
                           .arg(zones->getLT(i))
                           .arg(zones->getAeT(i))
                           .arg(zones->getMaxHr(i))
                           .arg(zones->getRestHr(i))
                           .toLocal8Bit()
                    );
                }
                return;
            }
        }

        // drop here on fail
        response.setStatus(500);
        response.write("unable to read/parse the athlete's hr.zones file.\n");
        return;
    }

    // pace zones
    if (zonesFor == "pace") {

        // Zones
        QFile zonesFile(home.absolutePath() + "/" + athlete + "/config/run-pace.zones");
        if (zonesFile.exists()) {
            PaceZones *zones = new PaceZones;
            if (zones->read(zonesFile)) {

                // success - write out
                response.write("date, CV, AeTV\n");
                for(int i=0; i<zones->getRangeSize(); i++) {
                    response.write(
                    QString("%1, %2, %3\n")
                           .arg(zones->getStartDate(i).toString("yyyy/MM/dd"))
                           .arg(zones->getCV(i))
                           .arg(zones->getAeT(i))
                           .toLocal8Bit()
                    );
                }
                return;
            }
        }

        // drop here on fail
        response.setStatus(500);
        response.write("unable to read/parse the athlete's run-pace.zones file.\n");
        return;
    }

    // swim pace zones
    if (zonesFor == "swimpace") {

        // Zones
        QFile zonesFile(home.absolutePath() + "/" + athlete + "/config/swim-pace.zones");
        if (zonesFile.exists()) {
            PaceZones *zones = new PaceZones;
            if (zones->read(zonesFile)) {

                // success - write out
                response.write("date, CV, AeTV\n");
                for(int i=0; i<zones->getRangeSize(); i++) {
                    response.write(
                    QString("%1, %2, %3\n")
                           .arg(zones->getStartDate(i).toString("yyyy/MM/dd"))
                           .arg(zones->getCV(i))
                           .arg(zones->getAeT(i))
                           .toLocal8Bit()
                    );
                }
                return;
            }
        }

        // drop here on fail
        response.setStatus(500);
        response.write("unable to read/parse the athlete's swim-pace.zones file.\n");
        return;
    }
}

void
APIWebService::listMeasures(QString athlete, QStringList paths, HttpRequest &request, HttpResponse &response)
{
    QDir configDir(home.absolutePath() + "/" + athlete + "/config");

    // list activities and associated metrics
    response.setHeader("Content-Type", "text; charset=ISO-8859-1");

    if (paths.isEmpty()) {

        foreach (QString group, Measures(configDir).getGroupSymbols()) {
            response.write(group.toLocal8Bit());
            response.write("\n");
        }
        return;
    }

    Measures measures = Measures(configDir, true);
    int group_index = measures.getGroupSymbols().indexOf(paths[0]);
    MeasuresGroup* measuresGroup = measures.getGroup(group_index);
    if (group_index < 0 || measuresGroup == NULL) {

        // unknown group
        response.setStatus(500);
        response.write("unknown measures group requested.\n");
        return;
    }

    response.write("Date");
    QStringList field_symbols = measuresGroup->getFieldSymbols();
    for (int i=0; i<field_symbols.count(); i++) {
        response.write(", ");
        response.write(field_symbols[i].toLocal8Bit());
    }

    // honour the since parameter
    QString sincep(request.getParameter("since"));
    QDate since(1900,01,01);
    if (sincep != "") since = QDate::fromString(sincep,"yyyy/MM/dd");
    QDate date = measuresGroup->getStartDate();
    if (since > date) date = since;

    // before parameter
    QString beforep(request.getParameter("before"));
    QDate before(3000,01,01);
    if (beforep != "") before = QDate::fromString(beforep,"yyyy/MM/dd");
    QDate endDate = measuresGroup->getEndDate();
    if (before < endDate) endDate = before;

    while (date <= endDate) {
        response.write("\n");
        response.write(date.toString("yyyy/MM/dd").toLocal8Bit());

        for (int i=0; i<field_symbols.count(); i++)
            response.write(QString(", %1").arg(measuresGroup->getFieldValue(date, i)).toLocal8Bit());

        date = date.addDays(1);
    }
    response.write("\n");

}
