#include "BanisterSimulator.h"

#include "Athlete.h"

BanisterParams
BanisterSimulator::extractParams(Athlete *athlete,
                                 const QString &loadMetric,
                                 const QDate &asOf)
{
    BanisterParams params = BanisterParams::defaultPriors();

    if (!athlete) return params;

    Banister *banister = athlete->getBanisterFor(loadMetric, QStringLiteral("power_index"), static_cast<int>(params.t1), static_cast<int>(params.t2));
    if (!banister) return params;

    params.t1 = banister->t1;
    params.t2 = banister->t2;

    QDate targetDate = asOf.isValid() ? asOf : QDate::currentDate();
    int bestIdx = -1;
    for (int i = 0; i < banister->windows.size(); i++) {
        const banisterFit &w = banister->windows[i];
        if (w.tests < 2) continue;

        if (targetDate >= w.startDate && targetDate <= w.stopDate) {
            bestIdx = i;
            break;
        }
        if (bestIdx < 0 || w.stopDate > banister->windows[bestIdx].stopDate) {
            if (w.tests >= 2) bestIdx = i;
        }
    }

    if (bestIdx >= 0) {
        const banisterFit &w = banister->windows[bestIdx];
        params.k1 = w.k1;
        params.k2 = w.k2;
        params.p0 = w.p0;
        params.t1 = w.t1 > 0 ? w.t1 : banister->t1;
        params.t2 = w.t2 > 0 ? w.t2 : banister->t2;
        params.isFitted = true;
    }

    if (!banister->data.isEmpty() && banister->start.isValid()) {
        long idx = banister->start.daysTo(targetDate);
        if (idx >= 0 && idx < banister->data.size()) {
            params.g = banister->data[idx].g;
            params.h = banister->data[idx].h;
        } else if (idx >= banister->data.size()) {
            long lastIdx = banister->data.size() - 1;
            long extraDays = idx - lastIdx;
            params.g = banister->data[lastIdx].g * std::exp(-extraDays / params.t1);
            params.h = banister->data[lastIdx].h * std::exp(-extraDays / params.t2);
        }
    }

    return params;
}
