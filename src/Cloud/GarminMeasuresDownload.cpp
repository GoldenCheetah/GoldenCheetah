/*
 * Copyright (c) 2026 Damien.Grauser (damien.grauser@pev-geneve.ch)
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

#include "GarminMeasuresDownload.h"
#include "GarminPersonalAPI.h"
#include "Athlete.h"
#include "Settings.h"
#include "Secrets.h"

#include <QTimeZone>

#ifndef GARMIN_DEBUG
#define GARMIN_DEBUG false
#endif
#ifdef Q_CC_MSVC
#define printd(fmt, ...) do {                                                \
if (GARMIN_DEBUG) {                                 \
        printf("[%s:%d %s] " fmt , __FILE__, __LINE__,        \
               __FUNCTION__, __VA_ARGS__);                    \
        fflush(stdout);                                       \
}                                                         \
} while(0)
#else
#define printd(fmt, args...)                                            \
do {                                                                \
        if (GARMIN_DEBUG) {                                       \
            printf("[%s:%d %s] " fmt , __FILE__, __LINE__,              \
                   __FUNCTION__, ##args);                               \
            fflush(stdout);                                             \
    }                                                               \
} while(0)
#endif

GarminMeasuresDownload::GarminMeasuresDownload(Context *context) : context(context)
{

}

GarminMeasuresDownload::~GarminMeasuresDownload() = default;

bool GarminMeasuresDownload::initializeApi(QString &error)
{
    if (garminPersonalAPI) {
        return true;
    }

    const QString key =
        appsettings->cvalue(context->athlete->cyclist, GC_GARMIN_API_KEY, "")
            .toString();

    if (key.isEmpty()) {
        error = tr("You must set a Garmin Personal API key first");
        return false;
    }

    garminPersonalAPI =
        std::make_unique<GarminPersonalAPI>(key);

    return true;
}


bool
GarminMeasuresDownload::getBodyMeasures(QString &error, QDateTime from, QDateTime to,
                                        QList<Measure> &data)
{
    printd("GarminMeasuresDownload::getBodyMeasures\n");

    emit downloadStarted(100);

    if (!initializeApi(error)) {
        emit downloadEnded(100);
        return false;
    }

    auto listing =
        garminPersonalAPI->weightService(
            from.date(),
            to.date());

    if (listing.hasError()) {
        error = listing.errorText;
        emit downloadEnded(100);
        return false;
    }

    emit downloadProgress(50);

    for (auto [day, weight] : listing.weightEntries.asKeyValueRange()) {

        Measure m;

        m.when = weight.startTimeLocal.isValid()
                     ? weight.startTimeLocal
                     : weight.startTime;

        if (m.when.isValid()) {
            m.source = Measure::Garmin;
            m.originalSource = "Garmin";

            double weightKg = weight.weight/1000.0;

            m.values[Measure::WeightKg] = weightKg;
            printd("weight : %f", weightKg);

            if (weight.bodyFat > 0.0) {
                m.values[Measure::FatPercent] = weight.bodyFat;
                m.values[Measure::FatKg] =
                    weightKg * weight.bodyFat / 100.0;

                m.values[Measure::LeanKg] =
                    weightKg * (100.0 - weight.bodyFat) / 100.0;
            }

            if (weight.muscleMass > 0)
                m.values[Measure::MuscleKg] = weight.muscleMass;

            if (weight.boneMass > 0)
                m.values[Measure::BonesKg] = weight.boneMass;


            data.append(m);
        }
    }

    emit downloadEnded(100);
    return true;
}

bool
GarminMeasuresDownload::getHrvMeasures(QString &error, QDateTime from, QDateTime to,
                                       QList<Measure> &data)
{
    printd("GarminMeasuresDownload::getHrvMeasures\n");

    emit downloadStarted(100);

    if (!initializeApi(error)) {
        emit downloadEnded(100);
        return false;
    }

    auto listing =
        garminPersonalAPI->hrvService(
            from.date(),
            to.date());

    if (listing.hasError()) {
        error = listing.errorText;
        emit downloadEnded(100);
        return false;
    }

    emit downloadProgress(50);

    // HRV object has YYYYMMDD keys with [RMSSD, Baseline] tuples
    for (auto [day, hrv] : listing.hrvEntries.asKeyValueRange()) {

        Measure m;

        m.when = QDateTime(day, QTime(0,0), QTimeZone::UTC);

        m.source = Measure::Garmin;
        m.originalSource = "Garmin";

        printd("hrv (lastNight) : %i", hrv.lastNightAvg);


        // RMSSD (ms)
        m.values[0] = hrv.lastNightAvg;

        data.append(m);
    }

    emit downloadEnded(100);

    return true;
}
