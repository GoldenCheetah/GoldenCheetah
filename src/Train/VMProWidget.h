#ifndef VMPROWIDGET_H
#define VMPROWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLowEnergyService>
#include <QTextEdit>

#include "VMProConfigurator.h"

class VMProWidget : public QObject
{
    Q_OBJECT
public:
    VMProWidget(QLowEnergyService * service, QObject * parent);

    void onReconnected(QLowEnergyService * service);

private slots:
    // Add a message for display to user
    void addStatusMessage(const QString &message);

    // Add a message that ends up in log file
    //void addLogMessage(const QString &message);

    // Methods that should update the GUI to reflect the new states
    void onVolumeCorrectionModeChanged(VMProVolumeCorrectionMode mode);
    void onUserPieceSizeChanged(VMProVenturiSize size);
    void onIdleTimeoutChanged(VMProIdleTimeout state);
    void onCalibrationProgressChanged(quint8 percentCompleted);

    // Method to handle when user changes the values
    void onUserPieceSizePickerChanged(int index);
    void onIdleTimeoutPickerChanged(int index);
    void onVolumeCorrectionModePickerChanged(int index);

    // File IO
    void onSaveClicked();

signals:
    void setNotification(QString msg, int timeout);

private:
    QLowEnergyService * m_vmProService;
    VMProConfigurator * m_vmProConfigurator;

    QList<QString> m_logMessages;

    QWidget * m_widget;
    QTextEdit * m_deviceLog;
    QComboBox * m_userPiecePicker;
    QComboBox * m_volumePicker;
    QComboBox * m_autocalibPicker;
    QComboBox * m_idleTimeoutPicker;
    QLabel * m_calibrationProgressLabel;

    // Current settings
    VMProIdleTimeout m_currIdleTimeoutState;
    VMProVenturiSize m_currVenturiSize;
    VMProVolumeCorrectionMode m_currVolumeCorrectionMode;
};

#endif // VMPROWIDGET_H
