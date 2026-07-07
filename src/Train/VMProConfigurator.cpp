#include "VMProConfigurator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>

#define VO2MASTERPRO_COMIN_CHAR_UUID "{00001525-1212-EFDE-1523-785FEABCD123}"
#define VO2MASTERPRO_COMOUT_CHAR_UUID "{00001526-1212-EFDE-1523-785FEABCD123}"

VMProConfigurator::VMProConfigurator(QLowEnergyService *service, QObject *parent) : QObject(parent)
{
    setupCharNotifications(service);
}

void VMProConfigurator::setupCharNotifications(QLowEnergyService * service)
{
    m_service = service;

    // Set up the two charasteristics used to query and control device settings
    m_comInChar =  m_service->characteristic(QBluetoothUuid(QString(VO2MASTERPRO_COMIN_CHAR_UUID)));
    m_comOutChar =  m_service->characteristic(QBluetoothUuid(QString(VO2MASTERPRO_COMOUT_CHAR_UUID)));
    connect(m_service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)),
            this, SLOT(onDeviceReply(QLowEnergyCharacteristic,QByteArray)));

    const QLowEnergyDescriptor notificationDesc = m_comOutChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
    if (notificationDesc.isValid()) {
        m_service->writeDescriptor(notificationDesc, QByteArray::fromHex("0100"));
    }
}

void VMProConfigurator::setUserPieceSize(VMProVenturiSize size)
{
    if (m_comInChar.isValid())
    {
        QByteArray cmd;
        cmd.append(VM_BLE_SET_VENTURI_SIZE);
        cmd.append(size);
        m_service->writeCharacteristic(m_comInChar, cmd);
    }
}

void VMProConfigurator::setVolumeCorrectionMode(VMProVolumeCorrectionMode mode)
{
    if (m_comInChar.isValid())
    {
        QByteArray cmd;
        cmd.append(VM_BLE_SET_VO2_REPORT_MODE);
        cmd.append(mode);
        m_service->writeCharacteristic(m_comInChar, cmd);
    }
}

void VMProConfigurator::setIdleTimeout(VMProIdleTimeout timeoutState)
{
    if (m_comInChar.isValid())
    {
        QByteArray cmd;
        cmd.append(VM_BLE_SET_IDLE_TIMEOUT_MODE);
        cmd.append(timeoutState);
        m_service->writeCharacteristic(m_comInChar, cmd);
    }
}

void VMProConfigurator::getUserPieceSize()
{
    if (m_comInChar.isValid())
    {
        QByteArray cmd;
        cmd.append(VM_BLE_GET_VENTURI_SIZE);
        cmd.append('\0');
        m_service->writeCharacteristic(m_comInChar, cmd);
    }
}

void VMProConfigurator::getIdleTimeout()
{
    if (m_comInChar.isValid())
    {
        QByteArray cmd;
        cmd.append(VM_BLE_GET_IDLE_TIMEOUT_MODE);
        cmd.append('\0');
        m_service->writeCharacteristic(m_comInChar, cmd);
    }
}

void VMProConfigurator::getVolumeCorrectionMode()
{
    if (m_comInChar.isValid())
    {
        QByteArray cmd;
        cmd.append(VM_BLE_GET_VO2_REPORT_MODE);
        cmd.append('\0');
        m_service->writeCharacteristic(m_comInChar, cmd);
    }
}

void VMProConfigurator::onDeviceReply(const QLowEnergyCharacteristic &c,
                                      const QByteArray &value)
{
    // Return if it's not the ComOut char that has been updated
    if (c.uuid() != QBluetoothUuid(QString(VO2MASTERPRO_COMOUT_CHAR_UUID)))
    {
        return;
    }

    // A reply consist of a "command" and "data" byte
    if (value.length() == 2)
    {
        VMProCommand cmd = static_cast<VMProCommand>(value[0]);
        quint8 data = value[1];

        switch (cmd) {
        case VM_BLE_UNKNOWN_RESPONSE:
            emit logMessage(tr("Unknown Response: %1").arg(data));
            break;
        case VM_BLE_SET_STATE:
            break;
        case VM_BLE_GET_STATE:
            emit logMessage(tr("Device State: %1").arg(data));
            break;
        case VM_BLE_SET_VENTURI_SIZE:
            break;
        case VM_BLE_GET_VENTURI_SIZE:
            emit logMessage(tr("User Piece Size: %1").arg(data));
            emit userPieceSizeChanged(static_cast<VMProVenturiSize>(data));
            break;
        case VM_BLE_GET_CALIB_PROGRESS:
            emit calibrationProgressChanged(data);
            emit logMessage(tr("Calibration Progress: %1 %").arg(data));
            break;
        case VM_BLE_ERROR:
            emit logMessage(VMProErrorToStringHelper::errorDescription(data));
            break;
        case VM_BLE_SET_VO2_REPORT_MODE:
            break;
        case VM_BLE_GET_VO2_REPORT_MODE:
            emit logMessage(tr("Volume Correction Mode: %1").arg(data));
            emit volumeCorrectionModeChanged(static_cast<VMProVolumeCorrectionMode>(data));
            break;
        case VM_BLE_GET_O2_CELL_AGE:
            emit logMessage(tr("O2 Cell Age: %1").arg(data));
            break;
        case VM_BLE_RESET_O2_CELL_AGE:
            break;
        case VM_BLE_SET_IDLE_TIMEOUT_MODE:
            break;
        case VM_BLE_GET_IDLE_TIMEOUT_MODE:
            emit logMessage(tr("Idle Timeout Mode: %1").arg(data));
            emit idleTimeoutStateChanged(static_cast<VMProIdleTimeout>(data));
            break;
        case VM_BLE_SET_AUTO_RECALIB_MODE:
            break;
        case VM_BLE_GET_AUTO_RECALIB_MODE:
            emit logMessage(tr("Auto-calibration Mode: %1").arg(data));
            break;
        case VM_BLE_BREATH_STATE_CHANGED:
            emit logMessage(tr("Breath State: %1").arg(data));
            break;
        }
    } else {
        qDebug() << "VMProConfigurator::onDeviceReply with unexpected length: " << value.length();
    }
}

QString VMProErrorToStringHelper::errorDescription(int errorCode)
{
    switch(errorCode)
    {
    //VM_ERROR_FATAL
    case (FatalErrorOffset + 0):        //VM_FATAL_ERROR_NONE
        return tr("No Error.");
    case (FatalErrorOffset + 1):        //VM_FATAL_ERROR_INIT
        return tr("Initialization error, shutting off.");
    case (FatalErrorOffset + 2):        //VM_FATAL_ERROR_TOO_HOT
        return tr("Too hot, shutting off.");
    case (FatalErrorOffset + 3):        //VM_FATAL_ERROR_TOO_COLD
        return tr("Too cold, shutting off.");
    case (FatalErrorOffset + 4):        //VM_FATAL_ERROR_IDLE_TIMEOUT
        return tr("Sat idle too long, shutting off.");
    case (FatalErrorOffset + 5):        //VM_FATAL_ERROR_DEAD_BATTERY_LBO_V
    case (FatalErrorOffset + 6):        //VM_FATAL_ERROR_DEAD_BATTERY_SOC
    case (FatalErrorOffset + 7):        //VM_FATAL_ERROR_DEAD_BATTERY_STARTUP
        return tr("Battery is out of charge, shutting off.");
    case (FatalErrorOffset + 8):        //VM_FATAL_ERROR_BME_NO_INIT
        return tr("Failed to initialize environmental sensor.\n Send unit in for service.");
    case (FatalErrorOffset + 9):        //VM_FATAL_ERROR_ADS_NO_INIT
        return tr("Failed to initialize oxygen sensor.\n Send unit in for service.");
    case (FatalErrorOffset + 10):        //VM_FATAL_ERROR_AMS_DISCONNECTED
        return tr("Failed to initialize flow sensor.\n Send unit in for service.");
    case (FatalErrorOffset + 11):        //VM_FATAL_ERROR_TWI_NO_INIT
        return tr("Failed to initialize sensor communication.\n Send unit in for service.");
    case (FatalErrorOffset + 12):        //VM_FATAL_ERROR_FAILED_FLASH_WRITE
        return tr("Failed to write a page to flash memory.\n Send unit in for service.");
    case (FatalErrorOffset + 13):        //VM_FATAL_ERROR_FAILED_FLASH_ERASE
        return tr("Failed to erase a page from flash memory.\n Send unit in for service.");

    //VM_ERROR_WARN
    case (WarningErrorOffset + 0):      //VM_WARN_ERROR_NONE
        return tr("No warning.");
    case (WarningErrorOffset + 1):      //VM_WARN_ERROR_MASK_LEAK
        return tr("Mask leak detected.");
    case (WarningErrorOffset + 2):      //VM_WARN_ERROR_VENTURI_TOO_SMALL
        return tr("User piece too small.");
    case (WarningErrorOffset + 3):      //VM_WARN_ERROR_VENTURI_TOO_BIG
        return tr("User piece too big.");
    case (WarningErrorOffset + 4):      //VM_WARN_ERROR_TOO_HOT
        return tr("Device very hot.");
    case (WarningErrorOffset + 5):      //VM_WARN_ERROR_TOO_COLD
        return tr("Device very cold.");
    case (WarningErrorOffset + 6):      //VM_WARN_ERROR_UNDER_BREATHING_VALVE
        return tr("Breathing less than valve trigger.");
    case (WarningErrorOffset + 7):      //VM_WARN_ERROR_O2_TOO_HUMID
        return tr("Oxygen sensor too humid.");
    case (WarningErrorOffset + 8):      //VM_WARN_ERROR_O2_TOO_HUMID_END
        return tr("Oxygen sensor dried.");
    case (WarningErrorOffset + 9):      //VM_WARN_ERROR_BREATHING_DURING_DIFFP_CALIB
        return tr("Breathing during ambient calibration.\n Hold your breath for 5 seconds.");
    case (WarningErrorOffset + 10):     //VM_WARN_ERROR_TOO_MANY_CONSECUTIVE_BREATHS_REJECTED
        return tr("Many breaths rejected.");
    case (WarningErrorOffset + 11):     //VM_WARN_ERROR_LOW_BATTERY_VOLTAGE
        return tr("Low battery.");
    case (WarningErrorOffset + 12):     //VM_WARN_ERROR_THERMAL_SHOCK_BEGIN
        return tr("Thermal change occurring.");
    case (WarningErrorOffset + 13):     //VM_WARN_ERROR_THERMAL_SHOCK_END
        return tr("Thermal change slowed.");
    case (WarningErrorOffset + 14):     //VM_WARN_ERROR_FINAL_VE_OUT_OF_RANGE
        return tr("Ventilation out of range.");

    //VM_ERROR_O2_RECALIB
    case (O2CalibrationErrorOffset + 0):    //VM_O2_RECALIB_NONE
        return tr("No calibration message.");
    case (O2CalibrationErrorOffset + 1):    //VM_O2_RECALIB_DRIFT
        return tr("O2 sensor signal drifted.");
    case (O2CalibrationErrorOffset + 2):    //VM_O2_RECALIB_PRESSURE_DRIFT
        return tr("Ambient pressure changed a lot.");
    case (O2CalibrationErrorOffset + 3):    //VM_O2_RECALIB_TEMPERATURE_DRIFT
        return tr("Temperature changed a lot.");
    case (O2CalibrationErrorOffset + 4):    //VM_O2_RECALIB_TIME_MAX
        return tr("Maximum time between calibrations reached.");
    case (O2CalibrationErrorOffset + 5):    //VM_O2_RECALIB_TIME_5MIN
        return tr("5 minute recalibration.");
    case (O2CalibrationErrorOffset + 6):    //VM_O2_RECALIB_REASON_THERMAL_SHOCK_OVER
        return tr("Post-thermal shock calibration.");

    //VM_ERROR_DIAG
    case (DiagnosticErrorOffset + 0):       //VM_DIAG_ERROR_FLOW_DELAY_RESET
        return tr("Calibration: waiting for user to start breathing.");
    case (DiagnosticErrorOffset + 1):       //VM_DIAG_ERROR_BREATH_TOO_JITTERY
        return tr("Breath rejected; too jittery.");
    case (DiagnosticErrorOffset + 2):       //VM_DIAG_ERROR_SEGMENT_TOO_SHORT
        return tr("Breath rejected; segment too short.");
    case (DiagnosticErrorOffset + 3):       //VM_DIAG_ERROR_BREATH_TOO_SHORT
        return tr("Breath rejected; breath too short.");
    case (DiagnosticErrorOffset + 4):       //VM_DIAG_ERROR_BREATH_TOO_SHALLOW
        return tr("Breath rejected; breath too small.");
    case (DiagnosticErrorOffset + 5):       //VM_DIAG_ERROR_FINAL_RF_OUT_OF_RANGE
        return tr("Breath rejected; Rf out of range.");
    case (DiagnosticErrorOffset + 6):       //VM_DIAG_ERROR_FINAL_TV_OUT_OF_RANGE
        return tr("Breath rejected; Tv out of range.");
    case (DiagnosticErrorOffset + 7):       //VM_DIAG_ERROR_FINAL_VE_OUT_OF_RANGE
        return tr("Breath rejected; Ve out of range.");
    case (DiagnosticErrorOffset + 8):       //VM_DIAG_ERROR_FINAL_FEO2_OUT_OF_RANGE
        return tr("Breath rejected; FeO2 out of range.");
    case (DiagnosticErrorOffset + 9):       //VM_DIAG_ERROR_FINAL_VO2_OUT_OF_RANGE
        return tr("Breath rejected; VO2 out of range.");
    case (DiagnosticErrorOffset + 10):      //VM_DIAG_ERROR_DEVICE_INITIALIZED
        return tr("Device initialized.");
    case (DiagnosticErrorOffset + 11):      //VM_DIAG_ERROR_TRIED_RECORD_BEFORE_CALIB
        return tr("Device attempted to enter Record mode\n before completing calibration.");
    case (DiagnosticErrorOffset + 12):      //VM_DIAG_ERROR_O2_CALIB_AVG_TOO_VOLATILE
        return tr("Oxygen sensor calibration waveform is volatile.");
    case (DiagnosticErrorOffset + 13):      //VM_DIAG_ERROR_ADS_HIT_MAX_VALUE
        return tr("Oxygen sensor reading clipped at its maximum value.");
    case (DiagnosticErrorOffset + 17):       //VM_DIAG_ERROR_VALVE_REQUEST_OPEN
        return tr("Valve opened.");
    case (DiagnosticErrorOffset + 18):       //VM_DIAG_ERROR_VALVE_REQUEST_CLOSE
        return tr("Valve closed.");
    case (DiagnosticErrorOffset + 19):       //VM_DIAG_ERROR_CALIB_ADC_THEO_DIFF_MINIMUM
        return tr("Calib diff: minimum.");
    case (DiagnosticErrorOffset + 20):       //VM_DIAG_ERROR_CALIB_ADC_THEO_DIFF_SMALL
        return tr("Calib diff: small.");
    case (DiagnosticErrorOffset + 21):       //VM_DIAG_ERROR_CALIB_ADC_THEO_DIFF_MEDIUM
        return tr("Calib diff: medium.");
    case (DiagnosticErrorOffset + 22):       //VM_DIAG_ERROR_CALIB_ADC_THEO_DIFF_LARGE
        return tr("Calib diff: large.");

    // Undisclosed error codes.
    case (DiagnosticErrorOffset + 14):
    case (DiagnosticErrorOffset + 15):
    case (DiagnosticErrorOffset + 16):
    default:
        return tr("Error Code: %1").arg(errorCode);
    }
}

void VMProConfigurator::startCalibration()
{
    if (m_comInChar.isValid())
    {
        QByteArray cmd;
        cmd.append(VM_BLE_SET_STATE);
        cmd.append(VM_STATE_CALIB);
        m_service->writeCharacteristic(m_comInChar, cmd);
    }
}
