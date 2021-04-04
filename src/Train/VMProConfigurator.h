#ifndef VMPROCONFIGURATOR_H
#define VMPROCONFIGURATOR_H

#include <QObject>
#include <QLowEnergyService>

class QLabel;

enum VMProCommand
{
    VM_BLE_UNKNOWN_RESPONSE = 0,    /* Unrecognized repsonse. */
    VM_BLE_SET_STATE,               /*Set new state.*/
    VM_BLE_GET_STATE,               /*Get current state.*/
    VM_BLE_SET_VENTURI_SIZE,        /*Set new venturi size.*/
    VM_BLE_GET_VENTURI_SIZE,        /*Get current venturi size.*/
    VM_BLE_GET_CALIB_PROGRESS,      /*Get 0-100% progress update on calibration*/
    VM_BLE_ERROR,                   /*Send phone a new error.*/
    VM_BLE_SET_VO2_REPORT_MODE,     /*Set the environmental correction mode for VO2 measurements: {STPD, BTPS/ATPS}*/
    VM_BLE_GET_VO2_REPORT_MODE,     /*Get the environmental correction mode for VO2 measurements: {STPD, BTPS/ATPS}*/
    VM_BLE_GET_O2_CELL_AGE,         /*Get the current o2 cell age (0-100%).*/
    VM_BLE_RESET_O2_CELL_AGE,       /*Resets original o2 cell reading. This should only be used if a new o2 cell is installed and
                                      the device isn't automatically correcting for it.*/
    VM_BLE_SET_IDLE_TIMEOUT_MODE,   /*Stops the device from turning itself off after a predetermined amount of time it sits
                                      idle.*/
    VM_BLE_GET_IDLE_TIMEOUT_MODE,   /*Allows the device to turn itself off after a predetermined amount of time it sits idle.*/
    VM_BLE_SET_AUTO_RECALIB_MODE,   /*Stop device from automatically re-calibrating.*/
    VM_BLE_GET_AUTO_RECALIB_MODE,   /*Allows device to automatically recalibrate.*/
    VM_BLE_BREATH_STATE_CHANGED,    /*Sent from firmware to phone, when breath state changes.*/
};

enum VMProVenturiSize
{
    VM_VENTURI_SMALL = 0,
    VM_VENTURI_MEDIUM,
    VM_VENTURI_LARGE,
    VM_VENTURI_XLARGE,
    VM_VENTURI_RESTING
};

enum VMProVolumeCorrectionMode
{
    VM_STPD = 0,
    VM_BTPS
};

enum VMProIdleTimeout
{
    VM_OFF = 0,
    VM_ON
};

enum VMProState
{
    VM_STATE_UNUSED = 0, /*Depreciated. If IDLE, state is assumed to be calib.*/
    VM_STATE_CALIB,      /*Begin polling sensors. Await user action to calibrate device.*/
    VM_STATE_RECORD      /*Device must be calibrated before recording. Polls sensors in order to produce VO2 metrics.*/
};

class VMProErrorToStringHelper : public QObject {
    Q_OBJECT
public:
    static const int FatalErrorOffset = 0;             //Offset for fatal errors.
    static const int WarningErrorOffset = 50;          //Offset for warnings errors.
    static const int O2CalibrationErrorOffset = 100;   //Offset for recalibration errors.
    static const int DiagnosticErrorOffset = 120;      //Offset for diagnostic errors. These are not reported to the user but are recorded.

    static QString errorDescription(int errorCode);
};

Q_DECLARE_METATYPE(VMProCommand)
Q_DECLARE_METATYPE(VMProVenturiSize)
Q_DECLARE_METATYPE(VMProVolumeCorrectionMode)
Q_DECLARE_METATYPE(VMProIdleTimeout)
Q_DECLARE_METATYPE(VMProState)

class VMProConfigurator : public QObject
{
    Q_OBJECT

public:

    explicit VMProConfigurator(QLowEnergyService * service,
                               QObject *parent = nullptr);

signals:
    void userPieceSizeChanged(VMProVenturiSize size);
    void idleTimeoutStateChanged(VMProIdleTimeout timeoutState);
    void volumeCorrectionModeChanged(VMProVolumeCorrectionMode timeoutState);
    void calibrationProgressChanged(quint8 percentCompleted);
    void logMessage(const QString &msg);

public slots:
    void getUserPieceSize();
    void getIdleTimeout();
    void getVolumeCorrectionMode();

    void setUserPieceSize(VMProVenturiSize size);
    void setIdleTimeout(VMProIdleTimeout timeoutState);
    void setVolumeCorrectionMode(VMProVolumeCorrectionMode mode);

    void startCalibration();

    void setupCharNotifications(QLowEnergyService * service);

private slots:
    void onDeviceReply(const QLowEnergyCharacteristic &c, const QByteArray &value);

private:
    QLowEnergyService * m_service;
    QLowEnergyCharacteristic m_comInChar;
    QLowEnergyCharacteristic m_comOutChar;

};

#endif // VMPROCONFIGURATOR_H
