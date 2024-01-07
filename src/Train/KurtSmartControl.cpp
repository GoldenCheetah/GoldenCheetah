//
//  SmartControl.c
//
//  Copyright © 2017 Kinetic. All rights reserved.
//
//  Integrated into golden cheetah by Eric Christoffersen 2020 (impolexg@outlook.com)

#include "KurtSmartControl.h"

#include <random>
#include <mutex>

#include <QRandomGenerator>

#include "CalibrationData.h"

#define SensorHz                10000

uint8_t smart_control_state_to_calibration_state(smart_control_calibration_state calibration_state) {

    switch(calibration_state) {
    case SMART_CONTROL_CALIBRATION_STATE_NOT_PERFORMED:
        return CALIBRATION_STATE_FAILURE;

    case SMART_CONTROL_CALIBRATION_STATE_INITIALIZING:
        return CALIBRATION_STATE_STARTING;

    case SMART_CONTROL_CALIBRATION_STATE_SPEED_UP:
        return CALIBRATION_STATE_POWER;

    case SMART_CONTROL_CALIBRATION_STATE_START_COASTING:
    case SMART_CONTROL_CALIBRATION_STATE_COASTING:
        return CALIBRATION_STATE_COAST;

    case SMART_CONTROL_CALIBRATION_STATE_SPEED_UP_DETECTED:
        return CALIBRATION_STATE_POWER;

    case SMART_CONTROL_CALIBRATION_STATE_COMPLETE:
        return CALIBRATION_STATE_SUCCESS;
    }

    return CALIBRATION_STATE_FAILURE;
}

typedef enum smart_control_command
{
    SMART_CONTROL_COMMAND_SET_PERFORMANCE       = 0x00,
    SMART_CONTROL_COMMAND_SPINDOWN_CALIBRATION  = 0x03
} smart_control_command;

uint8_t hash8WithSeed(uint8_t hash, const uint8_t *buffer, uint8_t length)
{
    const uint8_t crc8_table[256] = {
        0x00, 0x91, 0xe3, 0x72, 0x07, 0x96, 0xe4, 0x75,
        0x0e, 0x9f, 0xed, 0x7c, 0x09, 0x98, 0xea, 0x7b,
        0x1c, 0x8d, 0xff, 0x6e, 0x1b, 0x8a, 0xf8, 0x69,
        0x12, 0x83, 0xf1, 0x60, 0x15, 0x84, 0xf6, 0x67,
        0x38, 0xa9, 0xdb, 0x4a, 0x3f, 0xae, 0xdc, 0x4d,
        0x36, 0xa7, 0xd5, 0x44, 0x31, 0xa0, 0xd2, 0x43,
        0x24, 0xb5, 0xc7, 0x56, 0x23, 0xb2, 0xc0, 0x51,
        0x2a, 0xbb, 0xc9, 0x58, 0x2d, 0xbc, 0xce, 0x5f,
        0x70, 0xe1, 0x93, 0x02, 0x77, 0xe6, 0x94, 0x05,
        0x7e, 0xef, 0x9d, 0x0c, 0x79, 0xe8, 0x9a, 0x0b,
        0x6c, 0xfd, 0x8f, 0x1e, 0x6b, 0xfa, 0x88, 0x19,
        0x62, 0xf3, 0x81, 0x10, 0x65, 0xf4, 0x86, 0x17,
        0x48, 0xd9, 0xab, 0x3a, 0x4f, 0xde, 0xac, 0x3d,
        0x46, 0xd7, 0xa5, 0x34, 0x41, 0xd0, 0xa2, 0x33,
        0x54, 0xc5, 0xb7, 0x26, 0x53, 0xc2, 0xb0, 0x21,
        0x5a, 0xcb, 0xb9, 0x28, 0x5d, 0xcc, 0xbe, 0x2f,
        0xe0, 0x71, 0x03, 0x92, 0xe7, 0x76, 0x04, 0x95,
        0xee, 0x7f, 0x0d, 0x9c, 0xe9, 0x78, 0x0a, 0x9b,
        0xfc, 0x6d, 0x1f, 0x8e, 0xfb, 0x6a, 0x18, 0x89,
        0xf2, 0x63, 0x11, 0x80, 0xf5, 0x64, 0x16, 0x87,
        0xd8, 0x49, 0x3b, 0xaa, 0xdf, 0x4e, 0x3c, 0xad,
        0xd6, 0x47, 0x35, 0xa4, 0xd1, 0x40, 0x32, 0xa3,
        0xc4, 0x55, 0x27, 0xb6, 0xc3, 0x52, 0x20, 0xb1,
        0xca, 0x5b, 0x29, 0xb8, 0xcd, 0x5c, 0x2e, 0xbf,
        0x90, 0x01, 0x73, 0xe2, 0x97, 0x06, 0x74, 0xe5,
        0x9e, 0x0f, 0x7d, 0xec, 0x99, 0x08, 0x7a, 0xeb,
        0x8c, 0x1d, 0x6f, 0xfe, 0x8b, 0x1a, 0x68, 0xf9,
        0x82, 0x13, 0x61, 0xf0, 0x85, 0x14, 0x66, 0xf7,
        0xa8, 0x39, 0x4b, 0xda, 0xaf, 0x3e, 0x4c, 0xdd,
        0xa6, 0x37, 0x45, 0xd4, 0xa1, 0x30, 0x42, 0xd3,
        0xb4, 0x25, 0x57, 0xc6, 0xb3, 0x22, 0x50, 0xc1,
        0xba, 0x2b, 0x59, 0xc8, 0xbd, 0x2c, 0x5e, 0xcf
    };
    
    for (uint8_t byte_index = 0; byte_index < length; byte_index++) {
        hash = crc8_table[hash ^ buffer[byte_index]];
    }
    return hash;
}

double smart_control_speed_for_ticks(uint16_t ticks)
{
    if (ticks == 0 || ticks == 65535) {
        return 0;
    }
    return (6107.2561186) / ((double)ticks);
}

double smart_control_ticks_to_seconds(uint32_t ticks)
{
    return (double)ticks / (double)SensorHz;
}

smart_control_power_data smart_control_process_power_data(const uint8_t *data, size_t size)
{
    smart_control_power_data powerData;

    if (size >= 14 && size <= 20) {

        uint8_t hashSeed = 0x42;
        uint8_t inData[20];
        for (size_t i = 0; i < size; ++i) {
            inData[i] = data[i];
        }
        uint8_t hash = hash8WithSeed(hashSeed, &inData[size - 1], 1);
        for (unsigned index = 0; index < size - 1; index++) {
            inData[index] ^= hash;
            hash = hash8WithSeed(hash, &inData[index], 1);
        }
        
        powerData.mode = (smart_control_mode)inData[0];
        powerData.targetResistance = ((uint16_t)inData[1] << 8) | (uint16_t)inData[2];
        powerData.power = ((uint16_t)inData[3] << 8) | (uint16_t)inData[4];
        powerData.cadenceRPM = inData[12];
        
        if (size >= 18) {
            uint32_t metersPerHour = ((uint32_t)inData[13] << 24) | ((uint32_t)inData[14] << 16) | ((uint32_t)inData[15] << 8) | (uint32_t)inData[16];
            powerData.speedKPH = metersPerHour / 1000.0;
        } else {
            uint16_t rollerTicks = ((uint16_t)inData[5] << 8) | (uint16_t)inData[6];
            powerData.speedKPH = smart_control_speed_for_ticks(rollerTicks);
        }
    } else {
        powerData.mode = SMART_CONTROL_MODE_ERG;
        powerData.targetResistance = 0;
        powerData.cadenceRPM = 0;
        powerData.power = 0;
        powerData.speedKPH = 0;
    }

    return powerData;
}

smart_control_config_data smart_control_process_config_data(const uint8_t *data, size_t size)
{
    uint8_t hashSeed = 0x42;
    uint8_t* inData = (uint8_t*)malloc(size);
    for (size_t i = 0; i < size; ++i) {
        inData[i] = data[i];
    }
    uint8_t hash = hash8WithSeed(hashSeed, &inData[size - 1], 1);
    for (unsigned index = 0; index < size - 1; index++) {
        inData[index] ^= hash;
        hash = hash8WithSeed(hash, &inData[index], 1);
    }
    
    smart_control_config_data configData;
    
    if (size >= 5) {
        
        configData.updateRate = inData[0];
        configData.tickRate = ((uint32_t)inData[1] << 16) | ((uint32_t)inData[2] << 8) | (uint32_t)inData[3];
        configData.firmwareUpdateState = inData[4];
        
        if (size >= 13) {
            configData.systemStatus = ((uint16_t)inData[5] << 8) | (uint16_t)inData[6];
            configData.calibrationState = (smart_control_calibration_state)inData[7];
            uint32_t spindownTicks = ((uint32_t)inData[8] << 24) | ((uint32_t)inData[9] << 16) | ((uint32_t)inData[10] << 8) | (uint32_t)inData[11];
            configData.spindownTime = smart_control_ticks_to_seconds(spindownTicks);
        }
        if (size >= 15) {
            uint16_t metersPerHour = ((uint16_t)inData[12] << 8) | (uint16_t)inData[13];
            configData.calibrationThresholdKPH = metersPerHour / 1000.0;
        } else {
            configData.calibrationThresholdKPH = 33.8;
        }
        if (size >= 18) {
            uint16_t metersPerHour = ((uint16_t)inData[14] << 8) | (uint16_t)inData[15];
            configData.brakeCalibrationThresholdKPH = metersPerHour / 1000.0;
            configData.brakeStrength = inData[16];
        } else {
            configData.brakeCalibrationThresholdKPH = 45;
            configData.brakeStrength = 55;
        }
        if (size >= 19) {
            configData.brakeOffset = inData[17];
        } else {
            configData.brakeOffset = 128;
        }
        if (size >= 20) {
            configData.noiseFilter = inData[18];
        } else {
            configData.noiseFilter = 1;
        }
    } else {
        configData.updateRate = 1;
        configData.tickRate = 10000;
        configData.firmwareUpdateState = 0;
        configData.systemStatus = 0;
        configData.calibrationState = SMART_CONTROL_CALIBRATION_STATE_NOT_PERFORMED;
        configData.spindownTime = 0;
        configData.calibrationThresholdKPH = 33.8;
        configData.brakeCalibrationThresholdKPH = 45;
        configData.brakeStrength = 55;
        configData.brakeOffset = 128;
        configData.noiseFilter = 1;
    }
    
    free(inData);

    return configData;
}



#ifndef MIN
#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

// Helper that generates random byte in range low to high
uint8_t RandomByteInRange(unsigned low, unsigned high) {
    return (uint8_t)QRandomGenerator::global()->bounded(low, high);
}

QByteArray smart_control_set_mode_erg_command(uint16_t targetWatts)
{
    smart_control_set_mode_erg_data data;
    
    uint16_t clamped = MAX(0, MIN(0xFFFF, targetWatts));
    
    data.bytes[0] = SMART_CONTROL_COMMAND_SET_PERFORMANCE;
    data.bytes[1] = SMART_CONTROL_MODE_ERG;
    data.bytes[2] = (uint8_t)((uint16_t)clamped >> 8);
    data.bytes[3] = (uint8_t)((uint16_t)clamped);
    data.bytes[4] = RandomByteInRange(0, 0x100 - 1); // distrib(gen); // nonce arc4random_uniform(0x100); // nonce
    uint8_t dataLength = 5;
    
    // Encode Packet
    uint8_t hashSeed = 0x42;
    uint8_t hash = hash8WithSeed(hashSeed, &data.bytes[dataLength - 1], 1);
    for (uint8_t index = 0; index < dataLength - 1; index++) {
        uint8_t temp = data.bytes[index];
        data.bytes[index] ^= hash;
        hash = hash8WithSeed(hash, &temp, 1);
    }

    return QByteArray((const char*)&data, sizeof(data));
}



QByteArray smart_control_set_mode_fluid_command(uint8_t level)
{
    smart_control_set_mode_fluid_data data;
    
    uint8_t clamped = MAX(0, MIN(9, level));
    
    data.bytes[0] = SMART_CONTROL_COMMAND_SET_PERFORMANCE;
    data.bytes[1] = SMART_CONTROL_MODE_FLUID;
    data.bytes[2] = clamped;
    data.bytes[3] = RandomByteInRange(0, 0x100 - 1); //arc4random_uniform(0x100); // nonce
    uint8_t dataLength = 4;
    
    // Encode Packet
    uint8_t hashSeed = 0x42;
    uint8_t hash = hash8WithSeed(hashSeed, &data.bytes[dataLength - 1], 1);
    for (uint8_t index = 0; index < dataLength - 1; index++) {
        uint8_t temp = data.bytes[index];
        data.bytes[index] ^= hash;
        hash = hash8WithSeed(hash, &temp, 1);
    }

    return QByteArray((const char*)&data, sizeof(data));
}

QByteArray smart_control_set_mode_brake_command(float percent)
{
    smart_control_set_mode_brake_data data;
    
    // normalize to 0-65535
    float clamped = MAX(0, MIN(1, percent));
    uint16_t normalized = (uint16_t) round(65535 * clamped);
    
    data.bytes[0] = SMART_CONTROL_COMMAND_SET_PERFORMANCE;
    data.bytes[1] = SMART_CONTROL_MODE_BRAKE;
    data.bytes[2] = normalized >> 8;
    data.bytes[3] = normalized;
    data.bytes[4] = RandomByteInRange(0, 0x100 - 1); //arc4random_uniform(0x100); // nonce
    uint8_t dataLength = 5;
    
    // Encode Packet
    uint8_t hashSeed = 0x42;
    uint8_t hash = hash8WithSeed(hashSeed, &data.bytes[dataLength - 1], 1);
    for (uint8_t index = 0; index < dataLength - 1; index++) {
        uint8_t temp = data.bytes[index];
        data.bytes[index] ^= hash;
        hash = hash8WithSeed(hash, &temp, 1);
    }

    return QByteArray((const char*)&data, sizeof(data));
}

QByteArray smart_control_set_mode_simulation_command(float weightKG, float rollingCoeff, float windCoeff, float grade, float windSpeedMPS)
{
    smart_control_set_mode_simulation_data data;
    
    data.bytes[0] = SMART_CONTROL_COMMAND_SET_PERFORMANCE;
    data.bytes[1] = SMART_CONTROL_MODE_SIMULATION;
    
    // weight is in KGs ... multiply by 100 to get 2 points of precision
    uint16_t weight100 = (uint16_t) roundf(MIN(655.36, weightKG) * 100);
    data.bytes[2] = weight100 >> 8;
    data.bytes[3] = weight100;
    
    // Rolling coeff is < 1. multiply by 10,000 to get 5 points of precision
    // coeff cannot be larger than 6.5536 otherwise it rolls over ...
    uint16_t rr10000 = (uint16_t) roundf(MIN(6.5536, rollingCoeff) * 10000);
    data.bytes[4] = rr10000 >> 8;
    data.bytes[5] = rr10000;
    
    // Wind coeff is typically < 1. multiply by 10,000 to get 5 points of precision
    // coeff cannot be larger than 6.5536 otherwise it rolls over ...
    uint16_t wr10000 = (uint16_t) roundf(MIN(6.5536, windCoeff) * 10000);
    data.bytes[6] = wr10000 >> 8;
    data.bytes[7] = wr10000;
    
    // Grade is between -45.0 and 45.0
    // Mulitply by 100 to get 2 points of precision
    int16_t grade100 = (int16_t) roundf(MAX(-45, MIN(45, grade)) * 100);
    data.bytes[8] = grade100 >> 8;
    data.bytes[9] = grade100;
    
    // windspeed is in meters / second. convert to CM / second
    int16_t windSpeedCM = (int16_t) roundf(windSpeedMPS * 100);
    data.bytes[10] = windSpeedCM >> 8;
    data.bytes[11] = windSpeedCM;
    
    data.bytes[12] = RandomByteInRange(0, 0x100 - 1); //arc4random_uniform(0x100); // nonce
    uint8_t dataLength = 13;    
    
    // Encode Packet
    uint8_t hashSeed = 0x42;
    uint8_t hash = hash8WithSeed(hashSeed, &data.bytes[dataLength - 1], 1);
    for (uint8_t index = 0; index < dataLength - 1; index++) {
        uint8_t temp = data.bytes[index];
        data.bytes[index] ^= hash;
        hash = hash8WithSeed(hash, &temp, 1);
    }

    return QByteArray((const char*)&data, sizeof(data));
}

QByteArray smart_control_start_calibration_command(bool brakeCalibration)
{
    smart_control_calibration_command_data data;
    
    data.bytes[0] = SMART_CONTROL_COMMAND_SPINDOWN_CALIBRATION;
    data.bytes[1] = 0x01;
    data.bytes[2] = brakeCalibration ? 0x01 : 0x00;
    data.bytes[3] = RandomByteInRange(0, 0x100 - 1); //arc4random_uniform(0x100); // nonce
    uint8_t dataLength = 4;
    
    // Encode Packet
    uint8_t hashSeed = 0x42;
    uint8_t hash = hash8WithSeed(hashSeed, &data.bytes[dataLength - 1], 1);
    for (uint8_t index = 0; index < dataLength - 1; index++) {
        uint8_t temp = data.bytes[index];
        data.bytes[index] ^= hash;
        hash = hash8WithSeed(hash, &temp, 1);
    }

    return QByteArray((const char*)&data, sizeof(data));
}

QByteArray smart_control_stop_calibration_command()
{
    smart_control_calibration_command_data data;
    
    data.bytes[0] = SMART_CONTROL_COMMAND_SPINDOWN_CALIBRATION;
    data.bytes[1] = 0x00;
    data.bytes[2] = 0x00;
    data.bytes[3] = RandomByteInRange(0, 0x100 - 1); //arc4random_uniform(0x100); // nonce
    uint8_t dataLength = 4;
    
    // Encode Packet
    uint8_t hashSeed = 0x42;
    uint8_t hash = hash8WithSeed(hashSeed, &data.bytes[dataLength - 1], 1);
    for (uint8_t index = 0; index < dataLength - 1; index++) {
        uint8_t temp = data.bytes[index];
        data.bytes[index] ^= hash;
        hash = hash8WithSeed(hash, &temp, 1);
    }

    return QByteArray((const char*)&data, sizeof(data));
}
