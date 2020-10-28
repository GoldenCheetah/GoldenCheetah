//
//  SmartControl.h
//
//  Copyright © 2017 Kinetic. All rights reserved.
//
//  Integrated into golden cheetah by Eric Christoffersen 2020 (impolexg@outlook.com)

#ifndef SmartControl_h
#define SmartControl_h

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <QByteArray>
#include <QBluetoothUuid>

static const char KURT_SMART_CONTROL_SERVICE_UUID[]         = "E9410200-B434-446B-B5CC-36592FC4C724";
static const char KURT_SMART_CONTROL_SERVICE_POWER_UUID[]   = "E9410201-B434-446B-B5CC-36592FC4C724";
static const char KURT_SMART_CONTROL_SERVICE_CONFIG_UUID[]  = "E9410202-B434-446B-B5CC-36592FC4C724";
static const char KURT_SMART_CONTROL_SERVICE_CONTROL_UUID[] = "E9410203-B434-446B-B5CC-36592FC4C724";

static const QBluetoothUuid s_KurtSmartControlService_UUID = QBluetoothUuid(QString(KURT_SMART_CONTROL_SERVICE_UUID));
static const QBluetoothUuid s_KurtSmartControlService_Power_UUID = QBluetoothUuid(QString(KURT_SMART_CONTROL_SERVICE_POWER_UUID));
static const QBluetoothUuid s_KurtSmartControlService_Config_UUID = QBluetoothUuid(QString(KURT_SMART_CONTROL_SERVICE_CONFIG_UUID));
static const QBluetoothUuid s_KurtSmartControlService_Control_UUID = QBluetoothUuid(QString(KURT_SMART_CONTROL_SERVICE_CONTROL_UUID));

/*! Smart Control Resistance Mode */
typedef enum smart_control_mode
{
    SMART_CONTROL_MODE_ERG          = 0x00,
    SMART_CONTROL_MODE_FLUID        = 0x01,
    SMART_CONTROL_MODE_BRAKE        = 0x02,
    SMART_CONTROL_MODE_SIMULATION   = 0x03
} smart_control_mode;

/*! Smart Control Power Data */
struct smart_control_power_data
{
    /*! Current Resistance Mode */
    smart_control_mode mode;
    
    /*! Current Power (Watts) */
    uint16_t power;
    
    /*! Current Speed (KPH) */
    double speedKPH;
    
    /*! Current Cadence (Virtual RPM) */
    uint8_t cadenceRPM;
    
    /*! Current wattage the RU is Targetting */
    uint16_t targetResistance;
};

/*!
 Deserialize the raw power data (bytes) broadcast by Smart Control.
 
 @param data The raw data broadcast from the [Power Service -> Power] Characteristic
 @param size The size of the data array
 
 @return Smart Control Power Data Struct
 */
smart_control_power_data smart_control_process_power_data(const uint8_t *data, size_t size);

/*! Smart Control Calibration State */
typedef enum smart_control_calibration_state
{
    SMART_CONTROL_CALIBRATION_STATE_NOT_PERFORMED       = 0,
    SMART_CONTROL_CALIBRATION_STATE_INITIALIZING        = 1,
    SMART_CONTROL_CALIBRATION_STATE_SPEED_UP            = 2,
    SMART_CONTROL_CALIBRATION_STATE_START_COASTING      = 3,
    SMART_CONTROL_CALIBRATION_STATE_COASTING            = 4,
    SMART_CONTROL_CALIBRATION_STATE_SPEED_UP_DETECTED   = 5,
    SMART_CONTROL_CALIBRATION_STATE_COMPLETE            = 10
} smart_control_calibration_state;

/*! Smart Control Configuration Data */
struct smart_control_config_data
{
    /*! Power Data Update Rate (Hz) */
    uint8_t updateRate;
    
    /*! Current Calibration State of the RU */
    smart_control_calibration_state calibrationState;
    
    /*! Current Spindown Time being applied to the Power Data */
    double spindownTime;
    
    /*! Calibration Speed Threshold (KPH) */
    double calibrationThresholdKPH;
    
    /*! Brake Calibration Speed Threshold (KPH) */
    double brakeCalibrationThresholdKPH;
    
    /*! Clock Speed of Data Update (Hz) */
    uint32_t tickRate;
    
    /*! System Health Status (non-zero indicates problem) */
    uint16_t systemStatus;
    
    /*! Firmware Update State (Internal Use Only) */
    uint8_t firmwareUpdateState;
    
    /*! Normalized Brake Strength calculated by a Brake Calibration */
    uint8_t brakeStrength;
    
    /*! Normalized Brake Offset calculated by a Brake Calibration */
    uint8_t brakeOffset;
    
    /*! Noise Filter Strength */
    uint8_t noiseFilter;    
};

/*!
 Deserialize the raw config data (bytes) broadcast by Smart Control.
 
 @param data The raw data broadcast from the [Power Service -> Config] Characteristic
 @param size The size of the data array
 
 @return Smart Control Config Data Struct
 */
smart_control_config_data smart_control_process_config_data(const uint8_t *data, size_t size);

// Convert device calibration state into GC CALIBRATION_STATE*
uint8_t smart_control_state_to_calibration_state(smart_control_calibration_state);

// Note about this pack. Below structures are defined with gcc's
// __attribute__(packed). Because the base type of the struct is byte
// the structs all have natural alignment of 1 when compiling with
// msvc, so this pack isn't strictly necessary. For non msvc I have
// no idea what the packing rules are. I've replaced the __attribute__
// with the msvc form since gcc also supports it.
#pragma pack(push, 1)

/*! Command Structs to write to the Control Point Characteristic */
struct smart_control_set_mode_erg_data
{
    uint8_t bytes[5];
};
struct smart_control_set_mode_fluid_data
{
    uint8_t bytes[4];
};
struct smart_control_set_mode_brake_data
{
    uint8_t bytes[5];
};
struct smart_control_set_mode_simulation_data
{
    uint8_t bytes[13];
};
struct smart_control_calibration_command_data
{
    uint8_t bytes[4];
};

// End of stucture byte packing.
#pragma pack(pop)

/*!
 Creates the Command to put the Resistance Unit into ERG mode with a target wattage.
 
 @param targetWatts The target wattage the RU should try to maintain by adjusting the brake position
 
 @return Write the bytes of the struct to the Control Point Characteristic (w/ response)
 */
QByteArray smart_control_set_mode_erg_command(uint16_t targetWatts);

/*!
 Creates the Command to put the Resistance Unit into a "Fluid" mode, mimicking a fluid trainer.
 This mode is a simplified interface for the Simulation Mode, where:
    Rider + Bike weight is 85kg
    Rolling Coeff is 0.004
    Wind Resistance is 0.60
    Grade is equal to the "level" parameter
    Wind Speed is 0.0
 
 @param level Difficulty level (0-9) the RU should apply (simulated grade %)
 
 @return Write the bytes of the struct to the Control Point Characteristic (w/ response)
 */
QByteArray smart_control_set_mode_fluid_command(uint8_t level);

/*!
 Creates the Command to put the Resistance Unit Brake at a specific position (as a percent).
 
 @param percent Percent (0-1) of brake resistance to apply.
 
 @return Write the bytes of the struct to the Control Point Characteristic (w/ response)
 */
QByteArray smart_control_set_mode_brake_command(float percent);


/*!
 Creates the Command to put the Resistance Unit into Simulation mode.
 
 @param weightKG Weight of Rider and Bike in Kilograms (kg)
 @param rollingCoeff Rolling Resistance Coefficient (0.004 for asphault)
 @param windCoeff Wind Resistance Coeffienct (0.6 default)
 @param grade Grade (-45 to 45) of simulated hill
 @param windSpeedMPS Head or Tail wind speed (meters / second)
 
 @return Write the bytes of the struct to the Control Point Characteristic (w/ response)
 */
QByteArray smart_control_set_mode_simulation_command(float weightKG, float rollingCoeff, float windCoeff, float grade, float windSpeedMPS);


/*!
 Creates the Command to start the Calibration Process.
 
 @param brakeCalibration Calibrates the brake (only needs to be done once, result is stored on unit)
 
 @return Write the bytes of the struct to the Control Point Characteristic (w/ response)
 */
QByteArray smart_control_start_calibration_command(bool brakeCalibration);


/*!
 Creates the Command to stop the Calibration Process.
 This is not necessary if the calibration process is allowed to complete.
 
 @return Write the bytes of the struct to the Control Point Characteristic (w/ response)
 */
QByteArray smart_control_stop_calibration_command(void);

#endif /* SmartControl_h */
