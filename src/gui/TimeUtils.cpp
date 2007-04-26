
#include "TimeUtils.h"
#include <math.h>

QString time_to_string(double secs) 
{
    QString result;
    unsigned rounded = (unsigned) round(secs);
    bool needs_colon = false;
    if (rounded >= 3600) {
        result += QString("%1").arg(rounded / 3600);
        rounded %= 3600;
        needs_colon = true;
    }
    if (needs_colon)
        result += ":";
    result += QString("%1").arg(rounded / 60, 2, 10, QLatin1Char('0'));
    rounded %= 60;
    result += ":";
    result += QString("%1").arg(rounded, 2, 10, QLatin1Char('0'));
    return result;
}

QString interval_to_str(double secs) 
{
    if (secs < 60.0)
        return QString("%1s").arg(secs, 0, 'f', 2, QLatin1Char('0'));
    QString result;
    unsigned rounded = (unsigned) round(secs);
    bool needs_colon = false;
    if (rounded >= 3600) {
        result += QString("%1h").arg(rounded / 3600);
        rounded %= 3600;
        needs_colon = true;
    }
    if (needs_colon || rounded >= 60) {
        if (needs_colon)
            result += " ";
        result += QString("%1m").arg(rounded / 60, 2, 10, QLatin1Char('0'));
        rounded %= 60;
        needs_colon = true;
    }
    if (needs_colon)
        result += " ";
    result += QString("%1s").arg(rounded, 2, 10, QLatin1Char('0'));
    return result;
}

