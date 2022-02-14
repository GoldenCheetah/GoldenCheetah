#include "Ftms.h"

void ftms_parse_indoor_bike_data(QDataStream &ds, FtmsIndoorBikeData &bd)
{
    //quint16 flags, inst_speed, avg_speed, inst_cadence, avg_cadence, tot_energy, energy_per_hour, elapsed_time, remaining_time;
    //qint16 resistence_level, inst_power, avg_power;
    //quint8 energy_per_min, heart_rate, met_equivalent;
    quint16 dummy16;
    quint8 dummy8;

    ds >> bd.flags;

    if (!(bd.flags & FtmsIndoorBikeFlags::FTMS_MORE_DATA))
    {
        // If more data is not set, instant speed is present
        ds >> bd.inst_speed; // resolution: 0.01 km/h
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_AVERAGE_SPEED_PRESENT)
    {
        ds >> bd.avg_speed; // resolution: 0.01 km/h
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_INST_CADENCE_PRESENT)
    {
        ds >> bd.inst_cadence; // resolution: 0.5 rpm
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_AVERAGE_CADENCE_PRESENT)
    {
        ds >> bd.avg_cadence; // resolution: 0.5 rpm
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_TOTAL_DISTANCE_PRESENT)
    {
        ds >> dummy16 >> dummy8; // we don't care about this, so just read 24 bits
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_RESISTANCE_LEVEL_PRESENT)
    {
        ds >> bd.resistence_level; // resolution: unitless
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_INST_POWER_PRESENT)
    {
        ds >> bd.inst_power; // resolution: 1 watt
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_AVERAGE_POWER_PRESENT)
    {
        ds >> bd.avg_power; // resolution: 1 watt
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_EXPENDED_ENERGY_PRESENT)
    {
        ds >> bd.tot_energy >> bd.energy_per_hour >> bd.energy_per_min; // resolution: 1 kcal
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_HEART_RATE_PRESENT)
    {
        ds >> bd.heart_rate; // resolution: 1 bpm
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_METABOLIC_EQUIV_PRESENT)
    {
        ds >> bd.met_equivalent; // resolution: 1 MET
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_ELAPSED_TIME_PRESENT)
    {
        ds >> bd.elapsed_time; // resolution: 1 second
    }

    if (bd.flags & FtmsIndoorBikeFlags::FTMS_REMAINING_TIME_PRESENT)
    {
        ds >> bd.remaining_time; // resolution: 1 second
    }
}

qint16 ftms_power_cap(qint16 power, FtmsDeviceInformation &device_info) {

    power = qRound((double)power/device_info.power_increment)*device_info.power_increment;
    if (power > device_info.maximal_power)
    {
        power = device_info.maximal_power;
    } else if (power < device_info.minimal_power) {
        power = device_info.minimal_power;
    }

    return power;
}

double ftms_resistance_cap(qint16 resistance, FtmsDeviceInformation &device_info) {
    resistance = qRound((double)resistance/device_info.resistance_increment)*device_info.resistance_increment;
    if (resistance > device_info.maximal_resistance)
    {
        resistance = device_info.maximal_resistance;
    } else if (resistance < device_info.minimal_resistance) {
        resistance = device_info.minimal_resistance;
    }

    return resistance;
}
