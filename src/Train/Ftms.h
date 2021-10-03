#ifndef FTMS_H
#define FTMS_H
#include <QDataStream>

// FTMS service assigned numbers
#define FTMSDEVICE_FTMS_UUID 0x1826
#define FTMSDEVICE_INDOOR_BIKE_CHAR_UUID 0x2AD2
#define FTMSDEVICE_POWER_RANGE_CHAR_UUID 0x2AD8
#define FTMSDEVICE_RESISTANCE_RANGE_CHAR_UUID 0x2AD6
#define FTMSDEVICE_FTMS_FEATURE_CHAR_UUID 0x2ACC
#define FTMSDEVICE_FTMS_CONTROL_POINT_CHAR_UUID 0x2AD9

enum FtmsControlPointCommand {
    FTMS_REQUEST_CONTROL = 0x00,
    FTMS_RESET,
    FTMS_SET_TARGET_SPEED,
    FTMS_SET_TARGET_INCLINATION,
    FTMS_SET_TARGET_RESISTANCE_LEVEL,
    FTMS_SET_TARGET_POWER,
    FTMS_SET_TARGET_HEARTRATE,
    FTMS_START_RESUME,
    FTMS_STOP_PAUSE,
    FTMS_SET_TARGETED_EXP_ENERGY,
    FTMS_SET_TARGETED_STEPS,
    FTMS_SET_TARGETED_STRIDES,
    FTMS_SET_TARGETED_DISTANCE,
    FTMS_SET_TARGETED_TIME,
    FTMS_SET_TARGETED_TIME_TWO_HR_ZONES,
    FTMS_SET_TARGETED_TIME_THREE_HR_ZONES,
    FTMS_SET_TARGETED_TIME_FIVE_HR_ZONES,
    FTMS_SET_INDOOR_BIKE_SIMULATION_PARAMS,
    FTMS_SET_WHEEL_CIRCUMFERENCE,
    FTMS_SPIN_DOWN_CONTROL,
    FTMS_SET_TARGETED_CADENCE,
    FTMS_RESPONSE_CODE = 0x80
};

enum FtmsResultCode {
    FTMS_SUCCESS = 0x01,
    FTMS_NOT_SUPPORTED,
    FTMS_INVALID_PARAMETER,
    FTMS_OPERATION_FAILED,
    FTMS_CONTROL_NOT_PERMITTED
};

enum FtmsMachineFeatures {
    FTMS_AVG_SPEED_SUPPORTED                            = 1 << 0,
    FTMS_CADENCE_SUPPORTED                              = 1 << 1,
    FTMS_TOTAL_DISTANCE_SUPPORTED                       = 1 << 2,
    FTMS_INCLINATION_SUPPORTED                          = 1 << 3,
    FTMS_ELEVATION_GAIN_SUPPORTED                       = 1 << 4,
    FTMS_PACE_SUPPORTED                                 = 1 << 5,
    FTMS_STEP_COUNT_SUPPORTED                           = 1 << 6,
    FTMS_RESISTANCE_LEVEL_SUPPORTED                     = 1 << 7,
    FTMS_STRIDE_COUNT_SUPPORTED                         = 1 << 8,
    FTMS_EXPENDED_ENERGY_SUPPORTED                      = 1 << 9,
    FTMS_HEART_RATE_SUPPORTED                           = 1 << 10,
    FTMS_METABOLIC_EQUIVALENT_SUPPORTED                 = 1 << 11,
    FTMS_ELAPSED_TIME_SUPPORTED                         = 1 << 12,
    FTMS_REMAINING_TIME_SUPPORTED                       = 1 << 13,
    FTMS_POWER_MEASUREMENT_SUPPORTED                    = 1 << 14,
    FTMS_FORCE_ON_BELT_AND_POWER_MEASUREMENT_SUPPORTED  = 1 << 15,
    FTMS_USER_DATA_RETENTION_SUPPORTED                  = 1 << 16
};

enum FtmsTargetSetting {
    FTMS_SPEED_TARGET_SUPPORTED                         = 1 << 0,
    FTMS_INCLINATION_TARGET_SUPPORTED                   = 1 << 1,
    FTMS_RESISTANCE_TARGET_SUPPORTED                    = 1 << 2,
    FTMS_POWER_TARGET_SUPPORTED                         = 1 << 3,
    FTMS_HEART_RATE_TARGET_SUPPORTED                    = 1 << 4,
    FTMS_EXPENDED_ENERGY_TARGET_SUPPORTED               = 1 << 5,
    FTMS_STEP_NUMBER_CONFIGURATION_SUPPORTED            = 1 << 6,
    FTMS_STRIDE_NUMBER_CONFIGURATION_SUPPORTED          = 1 << 7,
    FTMS_DISTANCE_CONFIGURATION_SUPPORTED               = 1 << 8,
    FTMS_TRAINING_TIME_CONFIGURATION_SUPPORTED          = 1 << 9,
    FTMS_TIME_IN_TWO_HEART_RATE_ZONES_SUPPORTED         = 1 << 10,
    FTMS_TIME_IN_THREE_HEART_RATE_ZONES_SUPPORTED       = 1 << 11,
    FTMS_TIME_IN_FIVE_HEART_RATE_ZONES_SUPPORTED        = 1 << 12,
    FTMS_INDOOR_BIKE_SIMULATION_SUPPORTED               = 1 << 13,
    FTMS_WHEEL_CIRCUMFERENCE_CONFIGURATION_SUPPORTED    = 1 << 14,
    FTMS_SPIN_DOWN_CONTROL_SUPPORTED                    = 1 << 15,
    FTMS_TARGETED_CADENCE_SUPPORTED                     = 1 << 16
};

enum FtmsIndoorBikeFlags {
    FTMS_MORE_DATA                                      = 1 << 0,
    FTMS_AVERAGE_SPEED_PRESENT                          = 1 << 1,
    FTMS_INST_CADENCE_PRESENT                           = 1 << 2,
    FTMS_AVERAGE_CADENCE_PRESENT                        = 1 << 3,
    FTMS_TOTAL_DISTANCE_PRESENT                         = 1 << 4,
    FTMS_RESISTANCE_LEVEL_PRESENT                       = 1 << 5,
    FTMS_INST_POWER_PRESENT                             = 1 << 6,
    FTMS_AVERAGE_POWER_PRESENT                          = 1 << 7,
    FTMS_EXPENDED_ENERGY_PRESENT                        = 1 << 8,
    FTMS_HEART_RATE_PRESENT                             = 1 << 9,
    FTMS_METABOLIC_EQUIV_PRESENT                        = 1 << 10,
    FTMS_ELAPSED_TIME_PRESENT                           = 1 << 11,
    FTMS_REMAINING_TIME_PRESENT                         = 1 << 12
};

struct FtmsIndoorBikeData {
    quint16 flags, inst_speed, avg_speed, inst_cadence, avg_cadence, tot_energy, energy_per_hour, elapsed_time, remaining_time;
    qint16 resistence_level, inst_power, avg_power;
    quint8 energy_per_min, heart_rate, met_equivalent;
};

struct FtmsDeviceInformation {
    bool supports_power_target = false;
    bool supports_resistance_target = false;
    bool supports_simulation_target = false;

    qint16 minimal_resistance = 0;
    qint16 maximal_resistance = 0;
    quint16 resistance_increment = 0;

    qint16 minimal_power = 0;
    qint16 maximal_power = 0;
    quint16 power_increment = 0;
};

void ftms_parse_indoor_bike_data(QDataStream &ds, FtmsIndoorBikeData &bd);

qint16 ftms_power_cap(qint16 power, FtmsDeviceInformation &device_info);
double ftms_resistance_cap(qint16 resistance, FtmsDeviceInformation &device_info);

#endif // FTMS_H
