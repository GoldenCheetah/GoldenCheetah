#ifndef BT40DEVICECONNECTIONDIALOG_H
#define BT40DEVICECONNECTIONDIALOG_H

#include <QDialog>
#include <QWidget>
#include <QObject>
#include <QStackedLayout>

class BT40Controller;
class QTreeWidget;
class QLabel;

class BT40DeviceConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    BT40DeviceConnectionDialog(BT40Controller *controller);
    int exec() override;

private slots:
    void okClicked();
    void cancelClicked();
    void retryClicked();
    void deviceConnecting(QString address, QString uuid);
    void deviceConnected(QString address, QString uuid);
    void deviceDisconnected(QString address, QString uuid);
    void deviceConnectionError(QString address, QString uuid);
    void scanFinished(bool foundAnyDevices);

private:
    void createSubTitle();
    void createTree();
    void createButtons();
    void addDevices();
    void setDeviceStatus(const QString& address, const QString& uuid, const QString& status, QColor color);
    void updateLabel(QLabel* label, const QString& status, QColor color);

private:
    QVBoxLayout *mainLayout;
    QTreeWidget *tree;
    QPushButton *okButton;
    QPushButton *retryButton;
    QPushButton *cancelButton;

    BT40Controller *controller;

    QString searching;

    static const int AddressRole = Qt::UserRole + 1;
    static const int UuidRole = Qt::UserRole + 2;
};

#endif // BT40DEVICECONNECTIONDIALOG_H
