//
//  inRide.c
//
//  Copyright © 2017 Kinetic. All rights reserved.
//
//  Integrated into golden cheetah by Eric Christoffersen 2020 (impolexg@outlook.com)

#include "KurtInRide.h"
#include <QDebug>
#include <QBluetoothUuid>

#include "CalibrationData.h"

// Convert inride device state into standard GC CALIBRATION_STATE.
// Show calibration result when trainer is not calibrating,
// show current calibration state when trainer is calibrating.
uint8_t inride_state_to_calibration_state(inride_state state, inride_calibration_result calibration_result) {
    switch (state) {
    case INRIDE_STATE_NORMAL:
        switch (calibration_result) {
        default:
        case INRIDE_CAL_RESULT_UNKNOWN:  return CALIBRATION_STATE_FAILURE;
        case INRIDE_CAL_RESULT_SUCCESS:  return CALIBRATION_STATE_SUCCESS;
        case INRIDE_CAL_RESULT_TOO_FAST: return CALIBRATION_STATE_FAILURE_SPINDOWN_TOO_FAST;
        case INRIDE_CAL_RESULT_TOO_SLOW: return CALIBRATION_STATE_FAILURE_SPINDOWN_TOO_SLOW;
        case INRIDE_CAL_RESULT_MIDDLE:   return CALIBRATION_STATE_FAILURE_SPINDOWN_TOO_SLOW;
        }
        break;

    case INRIDE_STATE_SPINDOWN_IDLE:
        return CALIBRATION_STATE_POWER;

    case INRIDE_STATE_SPINDOWN_READY:
    case INRIDE_STATE_SPINDOWN_ACTIVE:
        return CALIBRATION_STATE_COAST;
    default:
        break;
    }

    return CALIBRATION_STATE_FAILURE;
}

// Create string to show complete calibration state, for debug print.
const char* inride_state_to_debug_string(inride_state state, inride_calibration_result result) {
    switch (state) {
    case INRIDE_STATE_NORMAL:
        switch (result) {
        case INRIDE_CAL_RESULT_UNKNOWN:  return "INRIDE NORMAL          CALIBRATION:UNKNOWN";
        case INRIDE_CAL_RESULT_SUCCESS:  return "INRIDE NORMAL          CALIBRATION:SUCCESS";
        case INRIDE_CAL_RESULT_TOO_FAST: return "INRIDE NORMAL          CALIBRATION:TOO_FAST";
        case INRIDE_CAL_RESULT_TOO_SLOW: return "INRIDE NORMAL          CALIBRATION:TOO_SLOW";
        case INRIDE_CAL_RESULT_MIDDLE:   return "INRIDE NORMAL          CALIBRATION:MIDDLE";
        }
        break;
    case INRIDE_STATE_SPINDOWN_IDLE:
        switch (result) {
        case INRIDE_CAL_RESULT_UNKNOWN:  return "INRIDE SPINDOWN_IDLE   CALIBRATION:UNKNOWN";
        case INRIDE_CAL_RESULT_SUCCESS:  return "INRIDE SPINDOWN_IDLE   CALIBRATION:SUCCESS";
        case INRIDE_CAL_RESULT_TOO_FAST: return "INRIDE SPINDOWN_IDLE   CALIBRATION:TOO_FAST";
        case INRIDE_CAL_RESULT_TOO_SLOW: return "INRIDE SPINDOWN_IDLE   CALIBRATION:TOO_SLOW";
        case INRIDE_CAL_RESULT_MIDDLE:   return "INRIDE SPINDOWN_IDLE   CALIBRATION:MIDDLE";
        }
        break;
    case INRIDE_STATE_SPINDOWN_READY:
        switch (result) {
        case INRIDE_CAL_RESULT_UNKNOWN:  return "INRIDE SPINDOWN_READY  CALIBRATION:UNKNOWN";
        case INRIDE_CAL_RESULT_SUCCESS:  return "INRIDE SPINDOWN_READY  CALIBRATION:SUCCESS";
        case INRIDE_CAL_RESULT_TOO_FAST: return "INRIDE SPINDOWN_READY  CALIBRATION:TOO_FAST";
        case INRIDE_CAL_RESULT_TOO_SLOW: return "INRIDE SPINDOWN_READY  CALIBRATION:TOO_SLOW";
        case INRIDE_CAL_RESULT_MIDDLE:   return "INRIDE SPINDOWN_READY  CALIBRATION:MIDDLE";
        }
        break;
    case INRIDE_STATE_SPINDOWN_ACTIVE:
        switch (result) {
        case INRIDE_CAL_RESULT_UNKNOWN:  return "INRIDE SPINDOWN_ACTIVE CALIBRATION:UNKNOWN";
        case INRIDE_CAL_RESULT_SUCCESS:  return "INRIDE SPINDOWN_ACTIVE CALIBRATION:SUCCESS";
        case INRIDE_CAL_RESULT_TOO_FAST: return "INRIDE SPINDOWN_ACTIVE CALIBRATION:TOO_FAST";
        case INRIDE_CAL_RESULT_TOO_SLOW: return "INRIDE SPINDOWN_ACTIVE CALIBRATION:TOO_SLOW";
        case INRIDE_CAL_RESULT_MIDDLE:   return "INRIDE SPINDOWN_ACTIVE CALIBRATION:MIDDLE";
        }
    default:
        break;
    }

    return "INRIDE DEVICE UNKNOWN STATE";
}

const char* inride_state_to_rtd_string(inride_state state, inride_calibration_result result) {
    switch (state) {
    case INRIDE_STATE_NORMAL:
        switch (result) {
        case INRIDE_CAL_RESULT_UNKNOWN:  return "INRIDE NORMAL - CALIBRATION:UNKNOWN";
        case INRIDE_CAL_RESULT_SUCCESS:  return "INRIDE NORMAL - CALIBRATION:SUCCESS";
        case INRIDE_CAL_RESULT_TOO_FAST: return "INRIDE NORMAL - CALIBRATION:TOO_FAST";
        case INRIDE_CAL_RESULT_TOO_SLOW: return "INRIDE NORMAL - CALIBRATION:TOO_SLOW";
        case INRIDE_CAL_RESULT_MIDDLE:   return "INRIDE NORMAL - CALIBRATION:MIDDLE";
        }
        break;
    case INRIDE_STATE_SPINDOWN_IDLE:
        return "INRIDE SPINDOWN - STARTED, INCREASE SPEED";

    case INRIDE_STATE_SPINDOWN_READY:
        return "INRIDE SPINDOWN - READY, STOP PEDALLING";

    case INRIDE_STATE_SPINDOWN_ACTIVE:
        return "INRIDE SPINDOWN - COASTING, DO NOT PEDAL";
    default:
        break;
    }

    return "INRIDE UNKNOWN STATE";
}

QString inride_command_result_to_string(inride_command_result r) {
    switch (r) {
    case INRIDE_COM_RESULT_NONE:               return QString("INRIDE_COM_RESULT_NONE");
    case INRIDE_COM_RESULT_SUCCESS:            return QString("INRIDE_COM_RESULT_SUCCESS");
    case INRIDE_COM_RESULT_NOT_SUPPORTED:      return QString("INRIDE_COM_RESULT_NOT_SUPPORTED");
    case INRIDE_COM_RESULT_INVALID_REQUEST:    return QString("INRIDE_COM_RESULT_INVALID_REQUEST");
    case INRIDE_COM_RESULT_CALIBRATION_RESULT: return QString("INRIDE_COM_RESULT_CALIBRATION_RESULT");
    case INRIDE_COM_RESULT_UNKNOWN_ERROR:      return QString("INRIDE_COM_RESULT_UNKNOWN_ERROR");
    default: break;
    }
    return QString("Error");
}

void inride_BTDeviceInfoToSystemID(const QBluetoothDeviceInfo &devinfo, uint8_t systemID[6]) {

    const QBluetoothAddress addr = devinfo.address();
    uint64_t addr64 = 0;
    if (!addr.isNull()) {
        addr64 = addr.toUInt64();
    } else {
        // Mac doesn't 'have' bt addresses so use uuid instead?
        // I have no idea if its the same on mac. If this doesn't
        // work then we'll need to query the device systemid through
        // the bt deviceinformation characteristic.

        // This is the correct general way to obtain system id. But turns out for inride the bottom 6 bytes of
        // address works fine too.
        //     QByteArray systemid;
        //     foreach(QLowEnergyService* const& tservice, m_services) {
        //         qDebug() << "Service: " << tservice->serviceName();
        //         if (tservice->serviceUuid() == QBluetoothUuid(QBluetoothUuid::DeviceInformation)) {
        //             QLowEnergyCharacteristic characteristic = tservice->characteristic(QBluetoothUuid(QBluetoothUuid::SystemID));
        //             systemid = characteristic.value();
        //             if (systemid.size() == 6) {
        //                 qDebug() << "Got systemID from deviceinfo";
        //                 for (int i = 0; i < 6; i++) {
        //                     systemID[i] = systemid[i];
        //                 }
        //             }
        //         }
        //     }

        // Perhaps there's a quick easy way for this to also work on mac using
        // device uuid?
        
        QBluetoothUuid uuid = devinfo.deviceUuid();

        quint128 be_uuid128 = uuid.toUInt128();

// GC minimum Qt required for v3.8 is Qt6.5.3
#if QT_VERSION < 0x060600
        addr64 = *(uint64_t*)be_uuid128.data;
#else
        addr64 = *(uint64_t*)&be_uuid128;
#endif
    }

    uint8_t* paddr64 = (uint8_t*)&addr64;

    int dIdx = 0;
    for (int i = 0; i < 6; i++) {
        systemID[dIdx++] = paddr64[i];
    }


    qDebug() << "SystemID: " << Qt::hex << systemID[0] << systemID[1] << systemID[2] << systemID[3] << systemID[4] << systemID[5];
}


#define SensorHz                32768
#define SpindownMin             1.5
#define SpindownMinPro          4.7
#define SpindownMax             2.0
#define SpindownMaxPro          6.5
#define SpindownDefault         ((SpindownMin + SpindownMax) * 0.5)

bool inride_has_pro_flywheel(double spindown)
{
    return (spindown >= 4.7 && spindown <= 6.5);
}

double inride_speed_for_ticks(uint32_t ticks, uint8_t revs)
{
    if (ticks == 0 || revs <= 0) {
        return 0;
    }
    return (20012.256849 * ((double)revs)) / ((double)ticks);
}

double inride_ticks_to_seconds(uint32_t ticks)
{
    return ((double)ticks) / 32768.0;
}

typedef struct alpha_coast {
    double alpha;
    bool coasting;
} alpha_coast;

alpha_coast alpha(uint32_t interval, uint32_t ticks, uint32_t revs, double speedKPH, uint32_t ticksPrevious, uint32_t revsPrevious, double speedKPHPrevious, bool proFlywheel)
{
    Q_UNUSED(interval);
    alpha_coast result;
    result.alpha = 0.0;
    result.coasting = false;
    if (ticks > 0 && ticksPrevious > 0) {
        double tpr = ticks / (double)revs;
        double ptpr = ticksPrevious / (double)revsPrevious;
        double dtpr = tpr - ptpr;
        if (dtpr > 0) {
            double deltaSpeed = speedKPHPrevious - speedKPH;
            double alpha = deltaSpeed * dtpr;
            result.alpha = alpha;
            if (alpha > 200 && !proFlywheel) {
                result.coasting = true;
            } else if (alpha > 20 && proFlywheel) {
                result.coasting = true;
            }
        }
    }
    return result;
}

int power_for_speed(double kph, double spindown, double alpha, uint32_t revolutions)
{
    Q_UNUSED(alpha);
    Q_UNUSED(revolutions);
    double mph = kph * 0.621371;
    double rawPower = (5.244820 * mph) + (0.019168 * (mph * mph * mph));
    double dragOffset = 0;
    if (spindown > 0 && rawPower > 0) {
        bool proFlywheel = inride_has_pro_flywheel(spindown);
        double spindownTimeMS = spindown * 1000.0;
        double dragOffsetSlope = proFlywheel ? -0.021 : -0.1425;
        double dragOffsetPowerSlope = proFlywheel ? 2.62 : 4.55;
        double yIntercept = proFlywheel ? 104.97 : 236.20;
        dragOffset = (dragOffsetPowerSlope * spindownTimeMS * rawPower * 0.00001) + (dragOffsetSlope * spindownTimeMS) + yIntercept;
    } else {
        dragOffset = 0;
    }
    double power = rawPower + dragOffset;
    if (power < 0) {
        power = 0;
    }
    return (int)power;
}


inride_calibration_result result_for_spindown(double time)
{
    inride_calibration_result result = INRIDE_CAL_RESULT_UNKNOWN;
    if (time >= 1.5 && time <= 2.0) {
        result = INRIDE_CAL_RESULT_SUCCESS;
    } else if (time >= 4.7 && time <= 6.5) {
        result = INRIDE_CAL_RESULT_SUCCESS;
    } else if (time < 1.5) {
        result = INRIDE_CAL_RESULT_TOO_FAST;
    } else if (time > 6.5) {
        result = INRIDE_CAL_RESULT_TOO_SLOW;
    } else {
        result = INRIDE_CAL_RESULT_MIDDLE;
    }
    return result;
}

inride_config_data inride_process_config_data(const uint8_t * data)
{
    inride_config_data configData;
    configData.calibrationReady = (uint16_t)data[0];
    configData.calibrationReady |= ((uint16_t)data[1] << 8);
    configData.calibrationStart = (uint16_t)data[2];
    configData.calibrationStart |= ((uint16_t)data[3] << 8);
    configData.calibrationEnd = (uint16_t)data[4];
    configData.calibrationEnd |= ((uint16_t)data[5] << 8);
    configData.calibrationDebounce = (uint16_t)data[6];
    configData.calibrationDebounce |= ((uint16_t)data[7] << 8);
    uint32_t currentSpindownTicks = (uint32_t)data[8];
    currentSpindownTicks |= ((uint32_t)data[9] << 8);
    currentSpindownTicks |= ((uint32_t)data[10] << 16);
    currentSpindownTicks |= ((uint32_t)data[11] << 24);
    configData.currentSpindownTime = ((double)currentSpindownTicks) / 32768.0;
    configData.updateRateDefault = (uint16_t)data[12];
    configData.updateRateDefault |= ((uint16_t)data[13] << 8);
    configData.updateRateCalibration = (uint16_t)data[14];
    configData.updateRateCalibration |= ((uint16_t)data[15] << 8);
    configData.proFlywheel = inride_has_pro_flywheel(configData.currentSpindownTime);
    return configData;
}

// data must be ptr to 20 bytes.
inride_power_data inride_process_power_data(const uint8_t * data)
{
    inride_power_data powerData;
    
    // deobfuscate the power data
    uint8_t i = 0;
    uint8_t deob[20];
    for (i = 0; i < 20; ++i) {
        deob[i] = data[i];
    }
    uint8_t posRotate = (data[0] & 0xC0) >> 6;
    uint8_t xorIdx1 = posRotate + 1;
    xorIdx1 %= 4;
    uint8_t xorIdx2 = xorIdx1 + 1;
    xorIdx2 %= 4;
    static uint8_t indices[4][19] = {
        {14,15,12,16,11,5,17,3,2,1,19,13,6,4,8,9,10,18,7},
        {12,14,8,11,16,4,7,13,18,1,3,19,6,15,9,5,10,17,2},
        {11,5,1,9,4,18,7,15,6,2,10,12,16,3,14,13,19,17,8},
        {13,5,18,1,3,12,15,10,14,19,16,8,6,11,2,9,4,17,7}
    };
    for (i = 1; i < 20; ++i) {
        deob[i] = deob[i] ^ (indices[xorIdx1][i - 1] + indices[xorIdx2][i - 1]);
    }
    uint8_t powerBytes[20];
    powerBytes[0] = deob[0];
    for (i = 0; i < 19; ++i) {
        powerBytes[i + 1] = deob[indices[posRotate][i]];
    }
    
    powerData.state = (inride_sensor_state)(powerBytes[0] & 0x30);
    powerData.commandResult = (inride_command_result)(powerBytes[0] & 0x0F);
    
    i = 1;
    uint32_t interval = ((uint32_t)powerBytes[i++]);
    interval |= ((uint32_t)powerBytes[i++]) << 8;
    interval |= ((uint32_t)powerBytes[i++]) << 16;
    
    uint32_t ticks = ((uint32_t)powerBytes[i++]);
    ticks |= ((uint32_t)powerBytes[i++]) << 8;
    ticks |= ((uint32_t)powerBytes[i++]) << 16;
    ticks |= ((uint32_t)powerBytes[i++]) << 24;
    
    uint8_t revs = powerBytes[i++];
    
    uint32_t ticksPrevious = ((uint32_t)powerBytes[i++]);
    ticksPrevious |= ((uint32_t)powerBytes[i++]) << 8;
    ticksPrevious |= ((uint32_t)powerBytes[i++]) << 16;
    ticksPrevious |= ((uint32_t)powerBytes[i++]) << 24;
    
    uint8_t revsPrevious = powerBytes[i++];
    
    uint16_t cadenceRaw = ((uint16_t)powerBytes[i++]);
    cadenceRaw |= ((uint16_t)powerBytes[i++]) << 8;
    powerData.cadenceRPM = cadenceRaw == 0 ? 0 : (0.8652 * ((double)cadenceRaw) + 5.2617);
    
    uint32_t spindownTicks = ((uint32_t)powerBytes[i++]);
    spindownTicks |= ((uint32_t)powerBytes[i++]) << 8;
    spindownTicks |= ((uint32_t)powerBytes[i++]) << 16;
    spindownTicks |= ((uint32_t)powerBytes[i++]) << 24;
    
    powerData.lastSpindownResultTime = inride_ticks_to_seconds(spindownTicks);
    powerData.speedKPH = inride_speed_for_ticks(ticks, revs);
    
    powerData.rollerRPM = 0.0;
    if (ticks > 0) {
        double seconds = inride_ticks_to_seconds(ticks);
        double rollerRPS = revs / seconds;
        powerData.rollerRPM = rollerRPS * 60;
    }
    
    double speedKPHPrev = inride_speed_for_ticks(ticksPrevious, revsPrevious);
    powerData.proFlywheel = false;
    
    powerData.spindownTime = SpindownDefault;
    if (powerData.lastSpindownResultTime >= SpindownMin && powerData.lastSpindownResultTime <= SpindownMax) {
        powerData.spindownTime = powerData.lastSpindownResultTime;
    } else if (powerData.lastSpindownResultTime >= SpindownMinPro && powerData.lastSpindownResultTime <= SpindownMaxPro) {
        powerData.spindownTime = powerData.lastSpindownResultTime;
        powerData.proFlywheel = true;
    }
    
    if (!powerData.proFlywheel) {
        powerData.rollerResistance = 1 - ((powerData.spindownTime - SpindownMin) / (SpindownMax - SpindownMin));
    } else {
        powerData.rollerResistance = 1 - ((powerData.spindownTime - SpindownMinPro) / (SpindownMaxPro - SpindownMinPro));
    }
    
    alpha_coast ac = alpha(interval, ticks, revs, powerData.speedKPH, ticksPrevious, revsPrevious, speedKPHPrev, powerData.proFlywheel);
    powerData.coasting = ac.coasting;
    
    if (powerData.coasting) {
        powerData.power = 0;
    } else {
        powerData.power = power_for_speed(powerData.speedKPH, powerData.spindownTime, ac.alpha, revs);
    }
    
    powerData.calibrationResult = result_for_spindown(powerData.lastSpindownResultTime);
    
    return powerData;
}

uint16_t command_key(const uint8_t systemId[6])
{
    uint8_t sysidx1 = systemId[3] % 6;
    uint8_t sysidx2 = systemId[5] % 6;
    return ((uint16_t)systemId[sysidx1]) | (((uint16_t)(systemId[sysidx2])) << 8);
}

QByteArray inride_create_config_sensor_command_data(inride_update_rate updateRate, const uint8_t systemId[6])
{
    inride_config_sensor_command command;
    command.commandKey = command_key(systemId);
    command.command = 0x01;
    command.calReady = 602;
    command.calStart = 655;
    command.calEnd = 950;
    command.calDebounce = 327;
    command.updateRateDefault = updateRate;
    command.updateRateFast = INRIDE_UPDATE_RATE_250;

    return QByteArray((const char*)&command, sizeof(command));
}

QByteArray inride_create_start_calibration_command_data(const uint8_t systemId[6])
{
    inride_start_calibration_command command;
    command.commandKey = command_key(systemId);
    command.command = 0x03;

    return QByteArray((const char*)&command, sizeof(command));
}

QByteArray inride_create_stop_calibration_command_data(const uint8_t systemId[6])
{
    inride_stop_calibration_command command;
    command.commandKey = command_key(systemId);
    command.command = 0x04;

    return QByteArray((const char*)&command, sizeof(command));
}

QByteArray inride_create_set_spindown_time_command_data(double seconds, const uint8_t systemId[6])
{
    inride_set_spindown_time_command command;
    command.commandKey = command_key(systemId);
    command.command = 0x05;
    command.spindown = (uint32_t)(seconds * 32768);

    return QByteArray((const char*)&command, sizeof(command));
}
